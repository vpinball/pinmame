/************************************************************************************************
 Peyper (Spain)
 --------------
   Hardware:
   ---------
		CPU:     Z80 @ 2.5 MHz
			INT: IRQ @ 1250 Hz ?
		IO:      Z80 ports, AY8910 ports for lamps
		DISPLAY: 7-segment panels in both sizes
		SOUND:	 2 x AY8910 @ 2.5 MHz
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "peyper.h"
#include "sndbrd.h"

#define PEYPER_VBLANKFREQ   60 /* VBLANK frequency */
#define PEYPER_IRQFREQ    1800 /* IRQ frequency */
#define PEYPER_CPUFREQ 2500000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  dispCol;
  UINT8  swCol;
  core_tSeg segments;
} locals;

static INTERRUPT_GEN(PEYPER_irq) {
  cpu_set_irq_line(PEYPER_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(PEYPER_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % PEYPER_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % PEYPER_SOLSMOOTH) == 0)
  	locals.solenoids &= 0xff000000;
  /*-- display --*/
  if ((locals.vblankCount % PEYPER_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));
  }
  // for some odd reason, core_getSol(30) doesn't work!?
  core_updateSw(locals.solenoids & 0x20000000);
}

static SWITCH_UPDATE(PEYPER) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static int PEYPER_sw2m(int no) {
  return no < 10 ? 8 + no : 8 + (no / 10) * 8 + (no % 10);
}

static int PEYPER_m2sw(int col, int row) {
  return (col - 1) * 10 + row;
}

static READ_HANDLER(dip_r) {
  return core_getDip(offset / 4);
}

static READ_HANDLER(sw0_r) {
  return ~coreGlobals.swMatrix[0];
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[locals.swCol+1];
}

// switch and display strobing done here
static WRITE_HANDLER(col_w) {
  if ((data & 0x40) == 0x40) locals.swCol = data & 0x0f;
  else if ((data & 0x90) == 0x90) locals.dispCol = 0;
}

static WRITE_HANDLER(disp_w) {
  static int colMap[16] = { 8, 10, 12, 0, 0, 0, 0, 0, 9, 11, 13 };
  locals.segments[15-locals.dispCol].w = core_bcd2seg7[data >> 4];
  locals.segments[31-locals.dispCol].w = core_bcd2seg7[data & 0x0f];
  // mapping various lamps (million, player up, game over, tilt) from segments data
  if (colMap[locals.dispCol]) coreGlobals.tmpLampMatrix[colMap[locals.dispCol]] = data;
  // mapping lamps to million display digit for early Sonic games
  locals.segments[32].w = core_bcd2seg7[coreGlobals.tmpLampMatrix[12] & 0x40 ? 1 : 0x0f];
  locals.segments[33].w = core_bcd2seg7[coreGlobals.tmpLampMatrix[13] & 0x40 ? 1 : 0x0f];
  locals.segments[34].w = core_bcd2seg7[coreGlobals.tmpLampMatrix[13] & 0x04 ? 1 : 0x0f];
  locals.segments[35].w = core_bcd2seg7[coreGlobals.tmpLampMatrix[12] & 0x04 ? 1 : 0x0f];
  locals.dispCol = (locals.dispCol + 1) % 16;
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[4 + offset / 4] = data;
  // mapping game enable lamp to solenoid 30 here
  locals.solenoids = (locals.solenoids & 0x00ffffff) | ((coreGlobals.tmpLampMatrix[6] & 0xe0) << 24);
}

static WRITE_HANDLER(lamp7_w) {
  coreGlobals.tmpLampMatrix[7] = data;
}

// the right decoder provided, you could access up to 255 solenoids this way; I guess only 16 are used though
static WRITE_HANDLER(sol_w) {
  if (data) locals.solenoids |= (1 << (data-1));
}

static WRITE_HANDLER(ay8910_0_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w)   { AY8910Write(0,1,data); }
static WRITE_HANDLER(ay8910_0_porta_w)	{ coreGlobals.tmpLampMatrix[0] = data; }
static WRITE_HANDLER(ay8910_0_portb_w)	{ coreGlobals.tmpLampMatrix[1] = data; }
static WRITE_HANDLER(ay8910_1_ctrl_w)   { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w)   { AY8910Write(1,1,data); }
static WRITE_HANDLER(ay8910_1_porta_w)	{ coreGlobals.tmpLampMatrix[2] = data; }
static WRITE_HANDLER(ay8910_1_portb_w)	{ coreGlobals.tmpLampMatrix[3] = data; }

struct AY8910interface PEYPER_ay8910Int = {
	2,			/* 2 chip */
	2500000,	/* 2.5 MHz */
	{ 30, 30 },		/* Volume */
	{ 0, 0 }, { 0, 0 },
	{ ay8910_0_porta_w, ay8910_1_porta_w },
	{ ay8910_0_portb_w, ay8910_1_portb_w },
};

static MEMORY_READ_START(PEYPER_readmem)
  {0x0000,0x5fff, MRA_ROM},
  {0x6000,0x67ff, MRA_RAM},
MEMORY_END

// NVRAM works, but not always?
static MEMORY_WRITE_START(PEYPER_writemem)
  {0x6000,0x67ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(PEYPER_readport)
  {0x00,0x00, sw_r},
  {0x20,0x24, dip_r}, // only 20, 24 used
  {0x28,0x28, sw0_r},
PORT_END

static PORT_WRITE_START(PEYPER_writeport)
  {0x00,0x00, disp_w},
  {0x01,0x01, col_w},
  {0x04,0x04, ay8910_0_ctrl_w},
  {0x06,0x06, ay8910_0_data_w},
  {0x08,0x08, ay8910_1_ctrl_w},
  {0x0a,0x0a, ay8910_1_data_w},
  {0x0c,0x0c, sol_w},
  {0x10,0x18, lamp_w}, // only 10, 14, 18 used
  {0x2c,0x2c, lamp7_w},
PORT_END

static MACHINE_INIT(PEYPER) {
  memset(&locals, 0, sizeof locals);
}

MACHINE_DRIVER_START(PEYPER)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, PEYPER_CPUFREQ)
  MDRV_CPU_MEMORY(PEYPER_readmem, PEYPER_writemem)
  MDRV_CPU_PORTS(PEYPER_readport, PEYPER_writeport)
  MDRV_CPU_VBLANK_INT(PEYPER_vblank, 1)
  MDRV_CPU_PERIODIC_INT(PEYPER_irq, PEYPER_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(PEYPER,NULL,NULL)
  MDRV_DIPS(16)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_SWITCH_UPDATE(PEYPER)
  MDRV_SWITCH_CONV(PEYPER_sw2m,PEYPER_m2sw)
  MDRV_SOUND_ADD(AY8910, PEYPER_ay8910Int)
MACHINE_DRIVER_END
