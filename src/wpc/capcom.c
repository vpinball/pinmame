#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
//#include "capcom.h"

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

#define CC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 (Switches #6 & #7) */ \
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    /* Switch Column 2 (Switches #9 - #16) */ \
	/* For Stern MPU-200 (Switches #1-3, and #8) */ \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0200, "Slam Tilt",        KEYCODE_HOME)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0400, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0800, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x1000, "Sound Diagnostic", KEYCODE_0) \

#define CC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    CC_COMPORTS

#define CC_INPUT_PORTS_END INPUT_PORTS_END

#define CC_ROMSTART(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    NORMALREGION(0x00200000, REGION_CPU1) \
      ROM_LOAD16_BYTE(n1, 0x000001, 0x80000, chk1) \
      ROM_LOAD16_BYTE(n2, 0x000000, 0x80000, chk2) \
      ROM_LOAD16_BYTE(n3, 0x100001, 0x80000, chk3) \
      ROM_LOAD16_BYTE(n4, 0x100000, 0x80000, chk4)
#define CC_ROMEND ROM_END

static core_tGameData pinmagicGameData = {0};
static void init_pinmagic(void) { core_gameData = &pinmagicGameData; }
CC_INPUT_PORTS_START(pinmagic,3) CC_INPUT_PORTS_END
CC_ROMSTART(pinmagic,"u1l_v112.bin",0xc8362623,
                     "u1h_v112.bin",0xf6232c74,
                     "u2l_v10.bin", 0xd3e4241d,
                     "u2h_v10.bin", 0x9276fd62)
CC_ROMEND
CORE_GAMEDEFNV(pinmagic,"Pinball Magic",1996,"Capcom",cc,GAME_NO_SOUND)
