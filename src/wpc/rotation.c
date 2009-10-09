/************************************************************************************************
 Midway Pinball games
 --------------------
 Only one game produced before the Midway/Bally merger: Rotation VIII,
 a rotating 4-player cocktail table.
 Funny issue: Midway used a built-in keyboard for diagnostics and maintenace,
 a cute idea that was discarded at first, but picked up again by Bally in 1986 for their
 6803-based pinball game series!

	Hardware:
	---------
		CPU:     Z80 @ 1.77 MHz
			INT: NMI via 8156 timer output
		IO:      Z80 Ports, 8156 PIA
		DISPLAY: 5 x 6 Digit 7-Segment panels
		SOUND:	 integrated analog chip (part no. 0066-117XX, marked as "K3-4" on schematic
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"

#define MIDWAY_VBLANKFREQ   60 /* VBLANK frequency in HZ */
#define MIDWAY_NMIFREQ    1350 /* NMI frequency in HZ - at this rate, bumpers seems OK. */

#define MIDWAY_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
#define MIDWAY_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define MIDWAY_DISPLAYSMOOTH   3 /* Smooth the display over this number of VBLANKS */

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
  core_tSeg segments, pseg;
} locals;

static int MIDWAY_sw2m(int no) {
  return no + 8;
}

static int MIDWAY_m2sw(int col, int row) {
  return col*8 + row - 8;
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *MIDWAY_CMOS;
static WRITE_HANDLER(MIDWAY_CMOS_w) {
  MIDWAY_CMOS[offset] = data;
}

static NVRAM_HANDLER(MIDWAY) {
  core_nvram(file, read_or_write, MIDWAY_CMOS, 256, 0x00);
}

static MACHINE_INIT(MIDWAY) {
  memset(&locals, 0, sizeof locals);
}

static INTERRUPT_GEN(MIDWAY_nmihi) {
  cpu_set_nmi_line(0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(MIDWAY_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % MIDWAY_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % MIDWAY_SOLSMOOTH) == 0)
  	locals.solenoids = 0;
  /*-- display --*/
  if ((locals.vblankCount % MIDWAY_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0x00, sizeof locals.segments);
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(12));
}

static SWITCH_UPDATE(MIDWAY) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>4, 0xf0, 2);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0xd0, 3);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>7, 0x40, 4);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1], 0xff, 8);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1]>>8, 0xff, 9);
  }
}

/* if this read operation returns 0xc3, more rom code exists, starting at address 0x1800 */
static READ_HANDLER(mem1800_r) {
  return 0x00; /* 0xc3 */
}

/* game switches */
static READ_HANDLER(port_2x_r) {
  if (!offset)
    return coreGlobals.swMatrix[locals.tmpSwCol];
  else // translate keypad to usable switch columns
    switch (locals.tmpSwCol) {
      // 3, 2, 1, 0
      case 1: return ((coreGlobals.swMatrix[8] & 0x07) << 3) | ((coreGlobals.swMatrix[8] & 0x80) >> 1);
      // 7, 6, 5, 4
      case 2: return coreGlobals.swMatrix[8] & 0x78;
       // set, dot, 9, 8
      case 3: return ((coreGlobals.swMatrix[0] & 0x07) << 3) | ((coreGlobals.swMatrix[0] & 0x80) >> 1);
      // test 3, test 2, test 1, game
      case 4: return coreGlobals.swMatrix[0] & 0x78;
       // test 7, test 6, test 5, test 4
      case 5: return ((coreGlobals.swMatrix[9] & 0x07) << 3) | ((coreGlobals.swMatrix[9] & 0x80) >> 1);
      // end, test 10, test 9, test 8
      case 6: return coreGlobals.swMatrix[9] & 0x78;
      default: return 0;
    }
}

/* lamps & solenoids */
static WRITE_HANDLER(port_0x_w) {
  if (offset == 0)
    locals.tmpLampData = data; // latch lamp data for strobe_w call
  else
    locals.solenoids |= (data << ((offset-1) * 8));
}

/* sound maybe? */
static WRITE_HANDLER(port_1x_w) {
  // Deposit the output as solenoids until we know what it's for.
  // So in case the sounds are just solenoid chimes, we're already done.
  if (offset == 1)
    locals.solenoids = (locals.solenoids & 0xff00ffff) | (data << 16);
  else if (data != 0 && !((offset == 0 && data == 0x47) || (offset == 6 && data == 0x0f)))
    logerror("Unexpected output on port 1%x = %02x\n", offset, data);
}

/* display data */
static WRITE_HANDLER(disp_w) {
  locals.pseg[2*offset].w = core_bcd2seg[data >> 4];
  locals.pseg[2*offset + 1].w = core_bcd2seg[data & 0x0f];
}

/* this handler updates lamps, switch columns & displays - all at the same time!!! */
static WRITE_HANDLER(port_2x_w) {
  switch (offset) {
    case 3:
      if (data < 7) { // only 7 columns are used
        int ii;
        for (ii = 0; ii < 6; ii++)
          locals.segments[ii*7 + data].w = locals.pseg[ii].w;
        locals.lampMatrix[data] = locals.tmpLampData;
        locals.tmpSwCol = data + 1;
      } else
        logerror("Write to column %x\n", data);
      break;
    default:
      logerror("Write to port 2%x = %02x\n", offset, data);
  }
}

PORT_READ_START( midway_readport )
  { 0x21, 0x22, port_2x_r },
PORT_END

PORT_WRITE_START( midway_writeport )
  { 0x00, 0x02, disp_w },
  { 0x03, 0x07, port_0x_w },
  { 0x10, 0x17, port_1x_w },
  { 0x20, 0x25, port_2x_w },
PORT_END

/*-----------------------------------------
/  Memory map for Rotation VIII CPU board
/------------------------------------------
0000-17ff  3 x 2K ROM
c000-c0ff  RAM
e000-e0ff  NVRAM
*/
static MEMORY_READ_START(MIDWAY_readmem)
  {0x0000,0x17ff, MRA_ROM},	/* ROM */
  {0x1800,0x1800, mem1800_r},	/* Possible code extension. More roms to come? */
  {0xc000,0xc0ff, MRA_RAM},	/* RAM */
  {0xe000,0xe0ff, MRA_RAM},	/* NVRAM */
MEMORY_END

static MEMORY_WRITE_START(MIDWAY_writemem)
  {0xc000,0xc0ff, MWA_RAM},	/* RAM */
  {0xe000,0xe0ff, MIDWAY_CMOS_w, &MIDWAY_CMOS},	/* NVRAM */
MEMORY_END

MACHINE_DRIVER_START(MIDWAY)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 14138000/8)
  MDRV_CPU_MEMORY(MIDWAY_readmem, MIDWAY_writemem)
  MDRV_CPU_PORTS(midway_readport,midway_writeport)
  MDRV_CPU_VBLANK_INT(MIDWAY_vblank, 1)
  MDRV_CPU_PERIODIC_INT(MIDWAY_nmihi, MIDWAY_NMIFREQ)
  MDRV_CORE_INIT_RESET_STOP(MIDWAY,NULL,NULL)
  MDRV_NVRAM_HANDLER(MIDWAY)
  MDRV_DIPS(1) // no dips actually, but needed for extra core inport!
  MDRV_SWITCH_UPDATE(MIDWAY)
MACHINE_DRIVER_END

/*-------------------------------------------------------------------
/ Rotation VIII (09/1978)
/-------------------------------------------------------------------*/
INPUT_PORTS_START(rotation)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */ \
    /* switch column 2 */ \
    COREPORT_BIT(     0x0100, "Player #1 Start",   KEYCODE_1) \
    COREPORT_BIT(     0x0400, "Player #2 Start",   KEYCODE_2) \
    COREPORT_BIT(     0x0800, "Player #3 Start",   KEYCODE_3) \
    COREPORT_BIT(     0x0200, "Player #4 Start",   KEYCODE_4) \
    /* switch column 3 */ \
    COREPORT_BIT(     0x8000, "Ball Tilt",         KEYCODE_INSERT) \
    COREPORT_BIT(     0x1000, "Slam Tilt",         KEYCODE_HOME) \
    COREPORT_BITDEF(  0x4000, IPT_COIN1,           IP_KEY_DEFAULT) \
    /* switch column 4 */ \
    COREPORT_BITDEF(  0x2000, IPT_COIN2,           KEYCODE_6) \
    /* switch column 0 */ \
    COREPORT_BIT(     0x0080, "Keypad 8",          KEYCODE_8_PAD) \
    COREPORT_BIT(     0x0004, "Keypad 9",          KEYCODE_9_PAD) \
    COREPORT_BIT(     0x0001, "Keypad Set",        KEYCODE_ENTER) \
    COREPORT_BIT(     0x0002, "Keypad Dot",        KEYCODE_STOP) \
    COREPORT_BIT(     0x0040, "Keypad Game",       KEYCODE_0) \
    COREPORT_BIT(     0x0020, "Test 1 (Lamps)",    KEYCODE_9) \
    COREPORT_BIT(     0x0010, "Test 2 (Switches)", KEYCODE_8) \
    COREPORT_BIT(     0x0008, "Test 3 (Solenoids)",KEYCODE_7) \
  PORT_START /* 1 */ \
    /* switch column 9 */ \
    COREPORT_BIT(     0x8000, "Test 4 (Encoder)",  KEYCODE_V) \
    COREPORT_BIT(     0x0400, "Test 5 (Display)",  KEYCODE_C) \
    COREPORT_BIT(     0x0200, "Test 6 (Position)", KEYCODE_X) \
    COREPORT_BIT(     0x0100, "Test 7 (unused)",   KEYCODE_Z) \
    COREPORT_BIT(     0x4000, "Test 8 (Reset NVRAM)",KEYCODE_M) \
    COREPORT_BIT(     0x2000, "Test 9 (unused)",   KEYCODE_N) \
    COREPORT_BIT(     0x1000, "Test 10 (unused)",  KEYCODE_B) \
    COREPORT_BIT(     0x0800, "Keypad End",        KEYCODE_END) \
    /* switch column 8 */ \
    COREPORT_BIT(     0x0080, "Keypad 0",          KEYCODE_0_PAD) \
    COREPORT_BIT(     0x0004, "Keypad 1",          KEYCODE_1_PAD) \
    COREPORT_BIT(     0x0002, "Keypad 2",          KEYCODE_2_PAD) \
    COREPORT_BIT(     0x0001, "Keypad 3",          KEYCODE_3_PAD) \
    COREPORT_BIT(     0x0040, "Keypad 4",          KEYCODE_4_PAD) \
    COREPORT_BIT(     0x0020, "Keypad 5",          KEYCODE_5_PAD) \
    COREPORT_BIT(     0x0010, "Keypad 6",          KEYCODE_6_PAD) \
    COREPORT_BIT(     0x0008, "Keypad 7",          KEYCODE_7_PAD)
INPUT_PORTS_END

core_tLCDLayout rot_disp[] = {
  {0, 0, 0, 6,CORE_SEG7}, {0,16, 7, 6,CORE_SEG7},
  {2, 0,14, 6,CORE_SEG7}, {2,16,21, 6,CORE_SEG7},
  {4, 8,35, 6,CORE_SEG7}, {0}
};
static core_tGameData rotationGameData = {0,rot_disp,{FLIP_SW(FLIP_L),0,-1}};
static void init_rotation(void) {
  core_gameData = &rotationGameData;
}
ROM_START(rotation)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("rot-a117.dat", 0x0000, 0x0800, CRC(7bb6beb3) SHA1(5ee62246032158c68d426c11a4a9a889ee7655d7))
    ROM_LOAD("rot-b117.dat", 0x0800, 0x0800, CRC(538e37b2) SHA1(d283ac4d0024388b92b6494fcde63957b705bf48))
    ROM_LOAD("rot-c117.dat", 0x1000, 0x0800, CRC(3321ff08) SHA1(d6d94fea27ef58ca648b2829b32d62fcec108c9b))
ROM_END
CORE_GAMEDEFNV(rotation,"Rotation VIII",1978,"Midway",MIDWAY,GAME_NO_SOUND)
