/************************************************************************************************
 Inder (Spain)
 -------------

 ISSUES:
 - Sound for Brave Team (does it use an AY8910 as well? Only 8 bits are hooked to the chip...)
 - Missing memory write handlers, esp. for 0x4805, which seems to be a timer of some sort?
 - Does the NVRAM really use 8 bits? It's 256 Bytes long, and has 2 5101 chips.

 NOTE:
 - According to the manual, these games could read 10 switch columns, but only 3 + 5 are used.

   Hardware:
   ---------
		CPU:     Z80 @ 2.5 MHz
			INT: IRQ @ 250 Hz (4 ms)
		IO:      DMA only
		DISPLAY: 5 x 7 digit 7-segment panels (6 digits on Brave Team)
		SOUND:	 AY8910 @ 2 MHz
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "inder.h"
#include "sound/ay8910.h"

#define INDER_VBLANKFREQ   60 /* VBLANK frequency */
#define INDER_IRQFREQ     250 /* IRQ frequency */
#define INDER_CPUFREQ 2500000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  dispSeg[5];
  UINT8  swCol[5];
  core_tSeg segments;
} locals;

// switches start at 50 for column 1, and each column adds 10.
static int INDER_sw2m(int no) { return (no/10 - 4)*8 + no%10; }
static int INDER_m2sw(int col, int row) { return 40 + col*10 + row; }

static MACHINE_INIT(INDER) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_STOP(INDER) {
}

static INTERRUPT_GEN(INDER_irq) {
  cpu_set_irq_line(INDER_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(INDER_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % INDER_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % INDER_SOLSMOOTH) == 0)
  	locals.solenoids = 0;
  /*-- display --*/
  if ((locals.vblankCount % INDER_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));
  }

  core_updateSw(core_getSol(5));
}

static SWITCH_UPDATE(INDER) {
  int i;
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xfb, 1);
  }
  for (i=0; i < 5; i++) locals.swCol[i] = ~coreGlobals.swMatrix[i+1];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(3-offset);
}

static READ_HANDLER(sw_r) {
  logerror("sw_r(%d): %02x\n", offset, locals.swCol[offset]);
  return locals.swCol[offset];
}

// switch debouncing; I wonder if this is how it is really done?
static WRITE_HANDLER(sw_w) {
  locals.swCol[offset] = data;
  logerror("sw_w(%d): %02x\n", offset, data);
}

// Display uses direct segment addressing, so simple chars are possible.
static WRITE_HANDLER(disp_w) {
  int i;
  if (offset < 5) locals.dispSeg[offset] = data;
  else for (i=0; i < 5; i++)
    locals.segments[8*i + 7-(data >> 3)].w = locals.dispSeg[i];
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

// It's hard to draw the line between solenoids and lamps, as they share the outputs...
static WRITE_HANDLER(sol_w) {
  locals.solenoids |= data;
}

/*--------------------------------------------------
/ Sound section. Just a 8910 onboard, no extra ROMs.
/---------------------------------------------------*/
struct AY8910interface INDER_ay8910Int = {
	1,			/* 1 chip */
	2000000,	/* 2 MHz */
	{ 30 },		/* Volume */
};
static WRITE_HANDLER(ay8910_0_ctrl_w)   { AY8910Write(0,0,data); logerror("sound ctrl_w:%02x\n", data); }
static WRITE_HANDLER(ay8910_0_data_w)   { AY8910Write(0,1,data); logerror("sound data_w:%02x\n", data); }
static WRITE_HANDLER(ay8910_0_common_w) { AY8910Write(0,0,~data); AY8910Write(0,1,data); logerror("sound w:%02x\n", data); }
static READ_HANDLER (ay8910_0_r)        { return AY8910Read(0); }

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *INDER_CMOS;

static NVRAM_HANDLER(INDER) {
  core_nvram(file, read_or_write, INDER_CMOS, 0x100, 0x00);
}
// 5101 RAM only uses 4 bits, but they used 2 chips for 8 bits?
static WRITE_HANDLER(INDER_CMOS_w) { INDER_CMOS[offset] = data; }
static READ_HANDLER(INDER_CMOS_r)  { return INDER_CMOS[offset]; }

static MEMORY_READ_START(INDER_readmem)
  {0x0000,0x1fff, MRA_ROM},
  {0x4000,0x43ff, MRA_RAM},
  {0x4400,0x44ff, INDER_CMOS_r},
  {0x4800,0x4802, dip_r},
  {0x4805,0x4809, sw_r},
  {0x4b01,0x4b01, ay8910_0_r },
MEMORY_END

static MEMORY_WRITE_START(INDER_writemem)
  {0x2000,0x20ff, disp_w},
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
//{0x4800,0x4805, ?}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4b00,0x4b00, ay8910_0_ctrl_w },
  {0x4b02,0x4b02, ay8910_0_data_w },
MEMORY_END

static MEMORY_WRITE_START(INDER_writemem_old)
  {0x2000,0x20ff, disp_w},
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
//{0x4800,0x4805, ?}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4b00,0x4b00, ay8910_0_common_w },
MEMORY_END

MACHINE_DRIVER_START(INDER1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, INDER_CPUFREQ)
  MDRV_CPU_MEMORY(INDER_readmem, INDER_writemem)
  MDRV_CPU_VBLANK_INT(INDER_vblank, 1)
  MDRV_CPU_PERIODIC_INT(INDER_irq, INDER_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(INDER,NULL,INDER)
  MDRV_NVRAM_HANDLER(INDER)
  MDRV_DIPS(24)
  MDRV_SWITCH_CONV(INDER_sw2m,INDER_m2sw)
  MDRV_SWITCH_UPDATE(INDER)
  MDRV_SOUND_ADD(AY8910, INDER_ay8910Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(INDER0)
  MDRV_IMPORT_FROM(INDER1)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER_readmem, INDER_writemem_old)
MACHINE_DRIVER_END
