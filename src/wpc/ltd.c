#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "ltd.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  int vblankCount;
  int diagnosticLed;
  UINT8 db;
  UINT32 solenoids;
  core_tSeg segments;
  UINT8 pseg1, pseg2;
} locals;

#define LTD_CPUFREQ	3579545/4
#define LTD_IRQFREQ 250

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(LTD_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % LTD_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((locals.vblankCount % LTD_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
  }
  /*-- display --*/
  if ((locals.vblankCount % LTD_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(TRUE);
}

static INTERRUPT_GEN(ltd_irq) {
  static int state = 0;
  cpu_set_irq_line(LTD_CPU, 0, state ? ASSERT_LINE : CLEAR_LINE);
  state = !state;
}

static SWITCH_UPDATE(LTD) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[LTD_COMINPORT] & 0x01;
    cpu_set_nmi_line(LTD_CPU, coreGlobals.swMatrix[0] & 0x01);
  }
}

static int LTD_sw2m(int no) {
  return no + 8;
}

static int LTD_m2sw(int col, int row) {
  return col*8 + row - 8;
}

static UINT8* ram;
static WRITE_HANDLER(ram_w) {
  static int dispPos[] = { 6, 14, 4, 12, 2, 10, 0, 8 };
  int panel;
  ram[offset] = data;
  locals.db = data;

  if (offset > 0x5f && offset < 0x70) { // display
    panel = (offset - 0x60) / 8 * 16 + dispPos[offset % 8];
    locals.segments[panel].w = core_bcd2seg[ram[offset] >> 4];
    locals.segments[1 + panel].w = core_bcd2seg[ram[offset] & 0x0f];
  }

  if (offset > 0x6f && offset < 0x80) { // lamps
    coreGlobals.tmpLampMatrix[offset - 0x70] = data;
  }
}

static READ_HANDLER(ram_r) {

  if (offset > 0x50 && offset < 0x59) { // switches
    ram[offset] |= coreGlobals.swMatrix[offset - 0x50];
  }

  if (offset > 0x57 && offset < 0x60) { // switches
    ram[offset] |= coreGlobals.swMatrix[offset - 0x57];
  }

  return ram[offset];
}

/* Activate periphal write:
   0-6 : INT1-INT7 - Lamps & flipper enable
   7   : INT8      - Solenoids
   8   : INT9      - Display data
   9   : INT10     - Display strobe & sound
   15  : CLR       - resets all output
 */
static WRITE_HANDLER(sw_w) {
//  int intr = 1 + (data & 0x0f);
//  logerror("INT%d = %02x\n", intr, locals.db);
/*
  int i, snd, pos;
  switch (intr) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
      coreGlobals.tmpLampMatrix[intr-1] = locals.db;
      break;
    case 8:
      locals.solenoids = locals.db;
      break;
    case 9:
      locals.pseg1 = core_bcd2seg[locals.db >> 4];
      locals.pseg2 = core_bcd2seg[locals.db & 0x0f];
      break;
    case 10:
      snd = (locals.db & 0x3c) >> 2;
      pos = (locals.db & 0xc0) >> 4 | (locals.db & 0x03);
      locals.segments[pos * 2].w = locals.pseg1;
      locals.segments[1 + pos * 2].w = locals.pseg2;
      break;
    case 16:
      for (i=0; i < 7; i++)
        coreGlobals.tmpLampMatrix[i] = 0;
      locals.solenoids = 0;
      for (i=0; i < 32; i++)
        locals.segments[i].w = 0;
  }
*/
}

/*-----------------------------------------
/  Memory map for CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem)
//	AM_RANGE(0x0000,0x00ff) AM_READ(ram_r)		/* RAM */
//	AM_RANGE(0xc000,0xdfff) AM_READ(MRA_ROM)		/* ROM */
//	AM_RANGE(0xe000,0xffff) AM_READ(MRA_ROM)		/* reset vector */
  {0x0000,0x00ff,ram_r},
  {0xc000,0xdfff,MRA_ROM},
  {0xe000,0xffff,MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem)
//	AM_RANGE(0x0000,0x00ff) AM_WRITE(ram_w) AM_BASE(&ram)	/* RAM */
//	AM_RANGE(0xc000,0xdfff) AM_WRITE(MWA_ROM)		/* ROM */
//	AM_RANGE(0xfff5,0xfff5) AM_WRITE(sw_w)		/* periphal select */
  {0x0000,0x00ff,ram_w,&ram},
  {0xc000,0xdfff,MWA_ROM},
  {0xe000,0xffff,sw_w},
MEMORY_END

static MACHINE_INIT(LTD) {
  memset(&locals, 0, sizeof locals);
}

MACHINE_DRIVER_START(LTD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6800, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem, LTD_writemem)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ltd_irq, LTD_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(LTD)
MACHINE_DRIVER_END
