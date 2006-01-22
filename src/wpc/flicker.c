/************************************************************************************************
 Nutting Associates, Inc.
 ------------------------
 Flicker, Nutting's only machine, was a customized version of a Bally EM machine;
 It remained a prototype; there was no production run.
 The design was revolutionary though, and impressed the Bally executives a lot.
 Nutting sold the solid state pinball patent to Midway in 1977, and a few home pinball models
 were built using the same or a similar hardware design.
 Note this was probably the very first CPU-based entertainment machine ever built!

	Hardware:
	---------
		CPU:     I4004 @ 750 kHz
		IO:      4004 Ports
		DISPLAY: 2 x 6 Digit, 2 x 2 Digit, 1 x 1 Digit 7-Segment panels
		SOUND:	 Chimes
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/i4004/i4004.h"
#include "core.h"

#define NUTTING_VBLANKFREQ   60 /* VBLANK frequency in HZ */
#define NUTTING_SOLSMOOTH     4 /* Smooth the Solenoids over this number of VBLANKS */
#define NUTTING_LAMPSMOOTH    1 /* Smooth the lamps over this number of VBLANKS */
#define NUTTING_DISPLAYSMOOTH 3 /* Smooth the display over this number of VBLANKS */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    diagnosticLed;
  int    tmpSwCol;
  UINT32 solenoids;
  UINT8  tmpLampData;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments;
} locals;

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *NUTTING_CMOS;
static WRITE_HANDLER(NUTTING_CMOS_w) {
  NUTTING_CMOS[offset] = data;
}

static NVRAM_HANDLER(NUTTING) {
  core_nvram(file, read_or_write, NUTTING_CMOS, 256, 0x00);
}

static MACHINE_INIT(NUTTING) {
  memset(&locals, 0, sizeof locals);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(NUTTING_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % NUTTING_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % (NUTTING_SOLSMOOTH)) == 0)
    locals.solenoids &= 0x8000;
  /*-- display --*/
  if ((locals.vblankCount % NUTTING_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0x00, sizeof locals.segments);
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(NUTTING) {
  if (inports) {
    coreGlobals.swMatrix[5] = (coreGlobals.swMatrix[5] & ~0xe2) | (inports[CORE_COREINPORT] & 0xff);
    coreGlobals.swMatrix[6] = (coreGlobals.swMatrix[6] & ~0x9f) | (inports[CORE_COREINPORT] >> 8);
  }
}

/* "dips" (just connectable wires IRL) & normal switches */
static READ_HANDLER(rom2_r) {
  UINT8 sw;
  if (offset > 7)
    sw = core_getDip(offset/2 - 4);
  else
    sw = coreGlobals.swMatrix[offset/2 + 1];
  return (offset % 2) ? sw >> 4 : sw & 0x0f;
}

/* display (16 digits) */
static WRITE_HANDLER(rom1_w) {
  locals.segments[15 - offset].w = core_bcd2seg[data];
  // These next lines make up for a flaw in the rom code:
  // when you first make a 10 pt target in the game, and a
  // 1000 pt target thereafter, the 100s digit in between
  // will remain off! I'm sure Dave & Jeff did it similarily
  // on the prototype by wiring the segments accordingly;
  // maybe they also rewrote some of the code to fix it later on?
  if (locals.segments[2].w && !locals.segments[3].w)
    locals.segments[3].w = core_bcd2seg[0];
  if (locals.segments[9].w && !locals.segments[10].w)
    locals.segments[10].w = core_bcd2seg[0];
}

/* lamps (up to 64 are possible) */
static WRITE_HANDLER(rom2_w) {
  UINT8 lamp = locals.lampMatrix[offset/2];
  locals.lampMatrix[offset/2] = (offset % 2) ? (lamp & 0x0f) | (data << 4) : (lamp & 0xf0) | data;
  // map game on to solenoid #16
  locals.solenoids = (locals.solenoids & 0x7fff) | (((locals.lampMatrix[0] & 0x05) == 0x05) << 15);
  // Display the "0" at the single match units.
  if (locals.lampMatrix[5] & 0x01)
    locals.segments[16].w = core_bcd2seg[0];
}

/* solenoids 1-15 & test switch reading */
static WRITE_HANDLER(ram_w) {
  if (data && data != offset)
    locals.solenoids |= (1 << (data-1));
  else {
	UINT16 specialSw = (coreGlobals.swMatrix[6] << 8) | coreGlobals.swMatrix[5];
    // The 4004's test line is used to gain an extra switch row.
    i4004_set_TEST((specialSw >> offset) & 1);
  }
}

PORT_READ_START(NUTTING_readport)
  { 0x20, 0x2f, rom2_r },
PORT_END

PORT_WRITE_START(NUTTING_writeport)
  { 0x00, 0x0f, rom1_w },
  { 0x10, 0x1f, rom2_w },
  { 0x110,0x11f,ram_w },
PORT_END

/*-----------------------------------------
/  Memory map for Flicker CPU board
/------------------------------------------
0000-03ff  1K ROM
1000-10ff  NVRAM (planned)
2000-20ff  RAM
*/
static MEMORY_READ_START(NUTTING_readmem)
  {0x0000,0x03ff, MRA_ROM},	/* ROM */
  {0x1000,0x10ff, MRA_RAM},	/* NVRAM */
  {0x2000,0x20ff, MRA_RAM},	/* RAM */
MEMORY_END

static MEMORY_WRITE_START(NUTTING_writemem)
  {0x1000,0x10ff, NUTTING_CMOS_w, &NUTTING_CMOS},	/* NVRAM */
  {0x2000,0x20ff, MWA_RAM},	/* RAM */
MEMORY_END

MACHINE_DRIVER_START(NUTTING)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", 4004, 750000)
  MDRV_CPU_MEMORY(NUTTING_readmem, NUTTING_writemem)
  MDRV_CPU_PORTS(NUTTING_readport, NUTTING_writeport)
  MDRV_CPU_VBLANK_INT(NUTTING_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(NUTTING,NULL,NULL)
  MDRV_NVRAM_HANDLER(NUTTING) // not used so far, but there'll be a bootleg that does!
  MDRV_DIPS(32) // 32 dips
  MDRV_SWITCH_UPDATE(NUTTING)
MACHINE_DRIVER_END

/*-------------------------------------------------------------------
/ Flicker (Prototype, 09/1974)
/-------------------------------------------------------------------*/
INPUT_PORTS_START(flicker)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */ \
    /* switch column 5 */ \
    COREPORT_BITIMP(  0x0002, "Slam Tilt",         KEYCODE_HOME) \
    COREPORT_BITIMP(  0x0020, "1 Credit",          KEYCODE_5) \
    COREPORT_BITIMP(  0x0040, "2 Credits",         KEYCODE_3) \
    COREPORT_BITIMP(  0x0080, "3 Credits",         KEYCODE_4) \
    /* switch column 6 */ \
    COREPORT_BITIMP(  0x0100, "4 Credits",         KEYCODE_6) \
    COREPORT_BITIMP(  0x0200, "5 Credits",         KEYCODE_7) \
    COREPORT_BITIMP(  0x0400, "6 Credits",         KEYCODE_8) \
    COREPORT_BIT(     0x0800, "Ball Tilt",         KEYCODE_INSERT) \
    COREPORT_BITIMP(  0x1000, "Game Start",        KEYCODE_1) \
    COREPORT_BIT(     0x8000, "Test",              KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "Balls per Game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0010, "5" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Straight (no specials)") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0100, "0" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "Add-a-ball award") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "Replay award") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Match feature") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "15K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "20K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "25K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "30K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "35K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "40K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "45K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "50K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "55K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "60K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "65K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "70K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "75K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "80K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "85K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "90K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "95K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "100K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "105K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "110K") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

core_tLCDLayout flicker_disp[] = {
  {0, 0, 0,6,CORE_SEG7}, {0,14, 7,6,CORE_SEG7},
  {3, 4,14,2,CORE_SEG7}, {3,12, 6,1,CORE_SEG7},
  {3,18,13,1,CORE_SEG7}, {3,20,16,1,CORE_SEG7}, {0}
};
static core_tGameData flickerGameData = {0,flicker_disp,{FLIP_SW(FLIP_L),0,0}};
static void init_flicker(void) {
  core_gameData = &flickerGameData;
}
ROM_START(flicker)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("flicker.rom", 0x0000, 0x0400, CRC(c692e586) SHA1(5cabb28a074d18b589b5b8f700c57e1610071c68))
ROM_END
CORE_GAMEDEFNV(flicker,"Flicker (Prototype)",1974,"Nutting Associates",NUTTING,GAME_USES_CHIMES)
