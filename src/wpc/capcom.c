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

static SWITCH_UPDATE(cc) {
  if (inports) {
  }
}


static void cc_zeroCross(int data) {
}

static MACHINE_INIT(cc) {
  memset(&locals, 0, sizeof(locals));

  locals.vblankCount = 1;
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

static data16_t *ramptr;
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
  { 0x01000000, 0x0107ffff, MWA16_RAM, &ramptr },
  { 0x03000000, 0x0300ffff, MWA16_RAM }, /* NVRAM */
MEMORY_END

static VIDEO_UPDATE(cc_dmd);

MACHINE_DRIVER_START(cc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(cc, NULL, NULL)
  MDRV_CPU_ADD(M68306, 16670000/4)
  MDRV_CPU_MEMORY(cc_readmem, cc_writemem)
  MDRV_CPU_VBLANK_INT(cc_vblank, 1)
  MDRV_VIDEO_UPDATE(cc_dmd)
  MDRV_NVRAM_HANDLER(cc)
  MDRV_SWITCH_UPDATE(cc)
  MDRV_TIMER_ADD(cc_zeroCross, CC_ZCFREQ)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(cc) {
}

const core_tLCDLayout cc_dispDMD[] = {
  {0,0,32,128,CORE_DMD}, {0}
};
static VIDEO_UPDATE(cc_dmd) {
  static UINT32 offset = 0;
  tDMDDot dotCol;
  int ii, jj, kk, ll;
  UINT16 *RAM;

  core_textOutf(50,20,1,"offset=%4x", offset);
  memset(dotCol,0,sizeof(dotCol));
  if(keyboard_pressed_memory_repeat(KEYCODE_A,2))
	  offset+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
	  offset-=0x100;
  RAM = ramptr+offset;
  for (kk = 0, ii = 1; ii < 33; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 8; jj++) {
      UINT16 d = RAM[kk++];
      for (ll = 0; ll < 16; ll++)
        { *line++ = d & 0x01; d>>=1; }
    }
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, core_gameData->lcdLayout ? core_gameData->lcdLayout : &cc_dispDMD[0]);
}

