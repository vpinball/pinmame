#include <stdarg.h>
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"

#define CC_VBLANKFREQ    60 /* VBLANK frequency */
#define CC_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define CC_ZCFREQ        85 /* Zero cross frequency */

#define CC_SOLSMOOTH       2 /* Smooth the Solenoids over this numer of VBLANKS */
#define CC_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define CC_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

static struct {
  int vblankCount;
  void *zctimer;
} locals;

static NVRAM_HANDLER(cc);

static INTERRUPT_GEN(cc_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;
#if 0
  /*-- lamps --*/
  if ((locals.vblankCount % CC_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % CC_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % CC_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
#endif
  core_updateSw(TRUE);
}

static void cc_updSw(int *inports) {
  if (inports) {
  }
}

static core_tData ccData = {
  32, /* 32 Dips */
  cc_updSw, 1, NULL, "cc",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void cc_zeroCross(int data) {
}

static MACHINE_INIT(cc) {
  if (locals.zctimer) timer_remove(locals.zctimer);
  memset(&locals, 0, sizeof(locals));

  if (core_init(&ccData)) return;
  locals.vblankCount = 1;
  locals.zctimer = timer_alloc(cc_zeroCross);
  timer_adjust(locals.zctimer, 0,0, TIME_IN_HZ(CC_ZCFREQ));
}

static MACHINE_STOP(cc) {
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
  core_exit();
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
#if 0
PB0/IACK2 : XC Ack
PB1/IACK3 : Pulse ?
PB2/IACK5 : SW3 ?
PB3/IACK6 : SW4 ?
PB4/IRQ2  : XC
PB6/IRQ5  : SW6
IRQ1      : Not used
PB5/IRQ3  : Not used
IRQ4      : Not used
PB7/IRQ6  : Not used
IRQ7      : Not used
BG        : ?
BGACK     : Not used
BR        : Not used
BRERR     : Not used
RESET     : Not used
HALT      : Not used
PA0       : ?
PA1
PA2
PA3       : Diagnostic LED1
PA4       : Line S
PA5       : Line V
PA6       : V Set
PA7       : GReset

CS1 DRAM
CS2 I/O
CS3 NVRAM (D0-D7, A0-A14)
CS4 A20
CS5 A21
CS6 A22
CS7 A23
!I/O & A23

CS0
0x000000-0x1fffff : ROM0 (U1)
0x400000-0x5fffff : ROM1 (U2)
0x800000-0x9fffff : ROM2 (U3)
0xc00000-0xdfffff : ROM3 (U4)

CS2
AUX
EXT
SWITCH0
CS

CS0 = ff000000,80000000 => 80000000-80ffffff (R)
CS1 = fffc0000,00000000 => 00000000-0003ffff (RW)
CS2 = ff000000,40000000 => 40000000-40ffffff (RW)
CS3 = ffff0000,30000000 => 30000000-3000ffff (RW)
#endif

static MEMORY_READ16_START(cc_readmem)
  { 0x00000000, 0x00ffffbf, MRA16_ROM },
  { 0x01000000, 0x0107ffff, MRA16_RAM },
//{ 0x02000000, 0x02000001, MRA16_RAM }, /* AUX I/O */
//{ 0x02400000, 0x02400001, MRA16_RAM }, /* EXT I/O */
//{ 0x02800000, 0x02800001, MRA16_RAM }, /* SWITCH0 */
//{ 0x02C00000, 0x02C00001, MRA16_RAM }, /* CS */
  { 0x03000000, 0x0300ffff, MRA16_RAM }, /* NVRAM */
MEMORY_END

static MEMORY_WRITE16_START(cc_writemem)
  { 0x00000000, 0x00ffffbf, MWA16_ROM },
  { 0x01000000, 0x0107ffff, MWA16_RAM },
  { 0x03000000, 0x0300ffff, MWA16_RAM }, /* NVRAM */
MEMORY_END

MACHINE_DRIVER_START(cc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M68306, 16670000/4)
  MDRV_CPU_MEMORY(cc_readmem, cc_writemem)
  MDRV_CPU_VBLANK_INT(cc_vblank, 1)
  MDRV_MACHINE_INIT(cc) MDRV_MACHINE_STOP(cc)
  MDRV_VIDEO_UPDATE(core_led)
  MDRV_NVRAM_HANDLER(cc)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(cc) {
}
