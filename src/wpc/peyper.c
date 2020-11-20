/************************************************************************************************
 Peyper (Spain)
 --------------
   Hardware:
   ---------
		CPU:     Z80B @ 5 MHz
			INT: IRQ @ ~1600 Hz (R/C timer, needs to be measured on real machine, eg. Video Dens uses a slower rate)
		IO:      Z80 ports, Intel 8279 KDI chip, AY8910 ports for lamps
		DISPLAY: 7-segment panels in both sizes
		SOUND:	 2 x AY8910 @ 2.5 MHz
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "peyper.h"
#include "sndbrd.h"

#define VD_IRQFREQ         800 /* IRQ frequency for Video Dens games on this hardware */
#define PEYPER_IRQFREQ    1600 /* IRQ frequency */
#define PEYPER_CPUFREQ 5000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments;
  int    i8279cmd;
  int    i8279reg;
  UINT8  i8279ram[16];
} locals;

static INTERRUPT_GEN(PEYPER_irq) {
  cpu_set_irq_line(PEYPER_CPU, 0, HOLD_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(PEYPER_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
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
  return ~core_revbyte(core_getDip(offset / 4));
}

static READ_HANDLER(sw0_r) {
  return ~coreGlobals.swMatrix[0];
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
  if (data) {
    locals.solenoids |= (1 << (data-1));
    locals.vblankCount = 0;
  }
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

// handles the 8279 keyboard / display interface chip
static READ_HANDLER(i8279_r) {
  static UINT8 lastData;
  logerror("i8279 r%d (cmd %02x, reg %02x)\n", offset, locals.i8279cmd, locals.i8279reg);
  if ((locals.i8279cmd & 0xe0) == 0x40) lastData = coreGlobals.swMatrix[1 + (locals.i8279cmd & 0x07)]; // read switches (only 4 columns actually used)
  else if ((locals.i8279cmd & 0xe0) == 0x60) lastData = locals.i8279ram[locals.i8279reg]; // read display ram
  else logerror("i8279 r:%02x\n", locals.i8279cmd);
  if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  return lastData;
}
static WRITE_HANDLER(i8279_w) {
  if (offset) { // command
    locals.i8279cmd = data;
    if ((locals.i8279cmd & 0xe0) == 0x40)
      logerror("I8279 read switches: %x\n", data & 0x07);
    else if ((locals.i8279cmd & 0xe0) == 0x80)
      logerror("I8279 write display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x60)
      logerror("I8279 read display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x20)
      logerror("I8279 scan rate: %02x\n", data & 0x1f);
    else if ((locals.i8279cmd & 0xe0) == 0)
      logerror("I8279 set modes: display %x, keyboard %x\n", (data >> 3) & 0x03, data & 0x07);
    else logerror("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = data & 0x0f; // reset data for auto-increment
  } else { // data
    if ((locals.i8279cmd & 0xe0) == 0x80) { // write display ram
      if (!core_gameData->hw.gameSpecific1 && (coreGlobals.tmpLampMatrix[8] & 0x11) == 0x11) { // load replay values
        locals.segments[40 + locals.i8279reg].w = core_bcd2seg7[data >> 4];
      } else {
        locals.segments[15 - locals.i8279reg].w = core_bcd2seg7[data >> 4];
        locals.segments[31 - locals.i8279reg].w = core_bcd2seg7[data & 0x0f];
      }
      // mapping various lamps (million, player up, game over, tilt) from segments data
      switch (locals.i8279reg) {
        case  2:
          coreGlobals.tmpLampMatrix[8] = data;
          // mapping to million display digit for early Sonic games
          locals.segments[32].w = data & 0x40 ? core_bcd2seg7[1] : 0;
          locals.segments[35].w = data & 0x04 ? core_bcd2seg7[1] : 0;
          break;
        case  6: if (!locals.segments[36].w) locals.segments[36].w = core_bcd2seg7[0]; break; // odin(dlx)
        case  7: if (!locals.segments[36].w) locals.segments[36].w = core_bcd2seg7[0]; break; // others
        case  8: coreGlobals.tmpLampMatrix[9] = data; break;
        case  9: coreGlobals.tmpLampMatrix[10] = data; break;
        case 10:
          coreGlobals.tmpLampMatrix[11] = data;
          // mapping to million display digit for early Sonic games
          locals.segments[33].w = data & 0x40 ? core_bcd2seg7[1] : 0;
          locals.segments[34].w = data & 0x04 ? core_bcd2seg7[1] : 0;
          break;
      }
    } else logerror("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  }
}

static MEMORY_READ_START(PEYPER_readmem)
  {0x0000,0x5fff, MRA_ROM},
  {0x6000,0x67ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(PEYPER_writemem)
  {0x0000,0x5fff, MWA_NOP},
  {0x6000,0x67ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(PEYPER_readport)
  {0x00,0x01, i8279_r},
  {0x20,0x24, dip_r}, // only 20, 24 used
  {0x28,0x28, sw0_r},
PORT_END

static PORT_WRITE_START(PEYPER_writeport)
  {0x00,0x01, i8279_w},
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

MACHINE_DRIVER_START(PEYPER_VD)
  MDRV_IMPORT_FROM(PEYPER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(PEYPER_irq, VD_IRQFREQ)
MACHINE_DRIVER_END
