#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "taito.h"

#define TAITO_VBLANKFREQ    60 /* VBLANK frequency */
#define TAITO_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define TAITO_ZCFREQ        85 /* Zero cross frequency */

#define TAITO_SOLSMOOTH      2 /* Smooth the Solenoids over this numer of VBLANKS */
#define TAITO_LAMPSMOOTH     2 /* Smooth the lamps over this number of VBLANKS */
#define TAITO_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS */

static struct {
  int bcd[7];
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
} locals;

static NVRAM_HANDLER(taito);

static void piaIrq(int state) {
  //cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void taito_dispStrobe(int mask) {
  int ii,jj;
  int digit = 0;
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
    }
}

static void taito_lampStrobe(int board, int lampadr) {
}

static INTERRUPT_GEN(taito_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % TAITO_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % TAITO_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
}

static SWITCH_UPDATE(taito) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[TAITO_COMINPORT]>>10) & 0x07;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x60)) |
                              ((inports[TAITO_COMINPORT]<<5) & 0x60);
    // Adjust Coins, and Slam Tilt Switches for Stern MPU-200 Games!
    if ((core_gameData->gen & GEN_STMPU200))
      coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x87)) |
                                ((inports[TAITO_COMINPORT]>>2) & 0x87);
    else
      coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0x87)) |
                                ((inports[TAITO_COMINPORT]>>2) & 0x87);
  }
  /*-- Diagnostic buttons on CPU board --*/
  cpu_set_nmi_line(0, core_getSw(TAITO_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
  /*-- coin door switches --*/
}

static INTERRUPT_GEN(taito_irq) {
}

static void taito_zeroCross(int data) {
}

static MACHINE_INIT(taito) {
  memset(&locals, 0, sizeof(locals));

  locals.vblankCount = 1;
}

static UINT8 *taito_CMOS;

static WRITE_HANDLER(taito_CMOS_w) {
  if ((core_gameData->gen & (GEN_STMPU100|GEN_STMPU200)) == 0) data |= 0x0f;
  taito_CMOS[offset] = data;
}

static WRITE_HANDLER(lamps) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(display1) {
  ((int *)locals.pseg)[offset]=data;
}

static WRITE_HANDLER(display2) {
  ((int *)locals.pseg)[offset+8]=data;
}

static WRITE_HANDLER(display3) {
  ((int *)locals.pseg)[offset+16]=data;
}

static READ_HANDLER(readx) {
  return 0xff;
}

static READ_HANDLER(ready) {
  return 0xff;
}

static READ_HANDLER(readz) {
  return 0xff;
}

static MEMORY_READ_START(taito_readmem)
  { 0x2000, 0x20ff, MRA_RAM }, /* CMOS Battery Backed*/
  { 0x4026, 0x4027, readx },
  { 0x4092, 0x4093, ready },
  { 0x409b, 0x409b, readz },
MEMORY_END

static MEMORY_WRITE_START(taito_writemem)
  { 0x2000, 0x20ff, taito_CMOS_w, &taito_CMOS }, /* CMOS Battery Backed*/
  { 0x4026, 0x402f, lamps },
  { 0x4092, 0x4092, display1 },
  { 0x4093, 0x4093, display2 },
  { 0x409b, 0x409b, display3 },
MEMORY_END

MACHINE_DRIVER_START(taito)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(taito,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", 8080, 4000000)
  MDRV_CPU_MEMORY(taito_readmem, taito_writemem)
  MDRV_CPU_VBLANK_INT(taito_vblank, 1)
  MDRV_CPU_PERIODIC_INT(taito_irq, TAITO_IRQFREQ)
  MDRV_NVRAM_HANDLER(taito)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(taito)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(taito_zeroCross, TAITO_ZCFREQ)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, taito_CMOS, 0x100,0xff);
}
#if 0
static core_tData taitoData = {
  32, /* 32 Dips */
  taito_updSw, 1, sndbrd_0_data_w, "taito",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};
#endif
