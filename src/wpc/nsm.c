/************************************************************************************************
 NSM, Germany (aka Loewen)
 -------------------------
 NSM (along with Bell Games, Italy) developed an entirely new kind of pinball hardware,
 based almost entirely on the HC4094 serial decoder chip, in 1985.
 The trick was to use a total of NINE of these chips in order to handle (in sequence)
 the switch / lamp / display strobe, then the lamps, solenoids, and display segments.
 This method of using serial data for the displays was later copied by Juegos Populares.

	Hardware:
	---------
		CPU:     TMS9995 @ 11.052 MHz
		IO:      CPU ports, 9x HC4094 decoder chips
		DISPLAY: 5 x 8 Digit, 7-Segment panels
		SOUND:	 2 x AY8912 (same as AY8910 but one I/O port only)
************************************************************************************************/
#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/4094.h"
#include "core.h"
#include "sim.h"

static struct {
  core_tSeg segments;
  UINT8 strobe;
  UINT32 solenoids;
  int zc, uv, cruCount, lockRamWrite;
} locals;

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.tmpLampMatrix[8] = 0;
  core_updateSw(TRUE);
}

static void nsm_zc(int data) {
  locals.zc = !locals.zc;
}

static WRITE_HANDLER(oe_w) {
  int i;
  for (i=0; i < 9; i++) {
    HC4094_oe_w(i, 1);
    HC4094_oe_w(i, 0);
  }
  locals.cruCount = 0;
}

static WRITE_HANDLER(cru_w) {
  int i;
  HC4094_data_w(0, GET_BIT0);
  for (i=0; i < 9; i++) {
    HC4094_strobe_w(i, 1);
    HC4094_clock_w(i, 1);
    HC4094_clock_w(i, 0);
    HC4094_strobe_w(i, 0);
  }
  locals.cruCount++;
  if (locals.cruCount >= 72) {
    oe_w(0, 1);
  }
}

static WRITE_HANDLER(parallel_0_out) {
  locals.strobe = core_BitColToNum(data);
}
static WRITE_HANDLER(parallel_1_out) {
  coreGlobals.tmpLampMatrix[locals.strobe] = ~data;
}
static WRITE_HANDLER(parallel_2_out) {
  locals.solenoids = (locals.solenoids & 0xffff00) | core_revbyte(data);
  coreGlobals.solenoids = locals.solenoids;
}
static WRITE_HANDLER(parallel_3_out) {
  if (data & 0xf0) {
    locals.solenoids = (locals.solenoids & 0xff00ff) | (0x100 << (0x0f - (data >> 4)));
  } else {
    locals.solenoids &= 0xff00ff;
  }
  if (data & 0x0f) {
    locals.solenoids = (locals.solenoids & 0x00ffff) | (0x10000 << (0x0f - (data & 0x0f)));
  } else {
    locals.solenoids &= 0x00ffff;
  }
  coreGlobals.solenoids = locals.solenoids;
}
static WRITE_HANDLER(parallel_4_out) {
  locals.segments[locals.strobe].w = ~data;
}
static WRITE_HANDLER(parallel_5_out) {
  locals.segments[8+locals.strobe].w = ~data;
}
static WRITE_HANDLER(parallel_6_out) {
  locals.segments[16+locals.strobe].w = ~data;
}
static WRITE_HANDLER(parallel_7_out) {
  locals.segments[24+locals.strobe].w = ~data;
}
static WRITE_HANDLER(parallel_8_out) {
  locals.segments[32+locals.strobe].w = ~data;
}
static WRITE_HANDLER(qspin_0_out) { HC4094_data_w(1, data); }
static WRITE_HANDLER(qspin_1_out) { HC4094_data_w(2, data); }
static WRITE_HANDLER(qspin_2_out) { HC4094_data_w(3, data); }
static WRITE_HANDLER(qspin_3_out) { HC4094_data_w(4, data); }
static WRITE_HANDLER(qspin_4_out) { HC4094_data_w(5, data); }
static WRITE_HANDLER(qspin_5_out) { HC4094_data_w(6, data); }
static WRITE_HANDLER(qspin_6_out) { HC4094_data_w(7, data); }
static WRITE_HANDLER(qspin_7_out) { HC4094_data_w(8, data); }
/*
Flow of the serial data line:

Page HC# Clk Load What?
1    0   out out  Strobe
1    1   out out  Lamps
4    2   out out  Solenoids
4    3   out out  Solenoids
3    4   out out  Display
3    5   out out  Display
3    6   out out  Display
3    7   out out  Display
3    8   out out  Display
2    -   in  in   Switches
7    -   in  in   Switches
7    -   in  in   Switches
*/
static HC4094interface hc4094nsm = {
  9, // 9 chips!
  { parallel_0_out, parallel_1_out, parallel_2_out, parallel_3_out, parallel_4_out, parallel_5_out, parallel_6_out, parallel_7_out, parallel_8_out },
  { qspin_0_out, qspin_1_out, qspin_2_out, qspin_3_out, qspin_4_out, qspin_5_out, qspin_6_out, qspin_7_out }
};

static WRITE_HANDLER(ay8912_0_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8912_0_data_w)   { AY8910Write(0,1,data); }
static WRITE_HANDLER(ay8912_0_port_w)	{ logerror("AY8912 #0: port write=%02x\n", data); }
static WRITE_HANDLER(ay8912_1_ctrl_w)   { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8912_1_data_w)   { AY8910Write(1,1,data); }
static WRITE_HANDLER(ay8912_1_port_w)	{ logerror("AY8912 #1: port write=%02x\n", data); }
static struct AY8910interface nsm_ay8912Int = {
  2,			/* 2 chips */
  11052000. / 8.,		/* 1.3815 MHz */
  { 20, 20 },	/* Volume */
  { 0 }, { 0 },
  { ay8912_0_port_w },
  { ay8912_1_port_w },
};

static READ_HANDLER(read_ba) {
  return 0;
}
static READ_HANDLER(read_uv) {
  return locals.uv ? 0 : 0x05;
}
static READ_HANDLER(read_zc) {
  return locals.zc;
}

static READ_HANDLER(read_diag1) {
  return coreGlobals.swMatrix[0] & 0x80 ? 0xff : 0;
}
static READ_HANDLER(read_diag2) {
  return coreGlobals.swMatrix[0] & 0x40 ? 0 : 0xff;
}
static READ_HANDLER(read_diag3) {
  return coreGlobals.swMatrix[0] & 0x20 ? 0 : 0xff;
}

static READ_HANDLER(keypad_r) {
  static int toggle;
  toggle = !toggle;
  return ~coreGlobals.swMatrix[11 + toggle];
}

static READ_HANDLER(cru_r) {
  static int toggle;
  static int colMap[8] = { 2, 3, 4, 5, 6, 7, 8, 1 };
  switch (offset) {
    case 0: // tells the next read from offset 4 that it's 16-bit, meaning to read switch columns 9 and 10 (combining offsets 4 and 5)
      toggle = 1;
      return 0; // does not matter
    case 2:
      return 0xff; // unknown
    case 4:
      if (toggle) {
        toggle = 0;
        return 0xf0 ^ core_revbyte(coreGlobals.swMatrix[10]); // SW 72 .. 79 (upper nybble inverted)
      }
      return core_revbyte(coreGlobals.swMatrix[colMap[locals.strobe]]);
    case 5:
      return 0xcf ^ core_revbyte(coreGlobals.swMatrix[9]); // bits 1 .. 6 are test returns from lamps and solenoids, bit 7 is SW 65 (inverted)
    case 0x0e:
    case 0x0f:
      locals.lockRamWrite = 0; // seems the only way to enable the write to 0xe600 but don't know really
      return 0xff; // unknown
    default:
      logerror("cru_r offset %x strobe %d\n", offset, locals.strobe);
      return 0;
  }
}

static WRITE_HANDLER(we600) {
  if (offset && (data & 0x10)) {
    locals.lockRamWrite = 1; // relocks if data bit 5 is set maybe? br test always shows "16"...
  }
  if (!locals.lockRamWrite) {
    generic_nvram[0x600 + offset] = memory_region(REGION_CPU1)[0xe600 + offset] = data;
  }
}

static WRITE_HANDLER(w7f80) {
  coreGlobals.tmpLampMatrix[8] = (coreGlobals.tmpLampMatrix[8] & 0xfe) | data;
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8];
}

static WRITE_HANDLER(w7fc0) {
  coreGlobals.tmpLampMatrix[8] = (coreGlobals.tmpLampMatrix[8] & 0xfd) | (data << 1);
  coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8];
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0xe000, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0xe600, 0xe601, we600 }, // reports test error "br failed" if this RAM address is always writable!
  { 0xe000, 0xe7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  { 0xe800, 0xefff, MWA_RAM }, // not needed / equipped on most games, can't hurt to have it
  { 0xffec, 0xffec, ay8912_0_ctrl_w },
  { 0xffed, 0xffed, ay8912_0_data_w },
  { 0xffee, 0xffee, ay8912_1_ctrl_w },
  { 0xffef, 0xffef, ay8912_1_data_w },
MEMORY_END

static PORT_WRITE_START( writeport )
  { 0x0f70, 0x0f7f, MWA_NOP },  // internal registers
  { 0x7f80, 0x7f80, w7f80 },	// ???
  { 0x7fb0, 0x7fbf, cru_w },
  { 0x7fc0, 0x7fc0, w7fc0 },	// ???
  { 0x7fd0, 0x7fd0, oe_w },
PORT_END

static PORT_READ_START( readport )
  { 0x00,  0x00,  read_uv },	// undervolt perc. on bit 1, also needs bit 3 set to pass "OE 614" test
  { 0x10,  0x10,  read_zc },	// antenna
  { 0x30,  0x30,  read_diag1 },	// J702 pin 7 (service connector, default lo)
  { 0x40,  0x40,  read_diag2 },	// J702 pin 11 (service connector, default hi)
  { 0x50,  0x50,  read_ba },	// batt. test, lo for battery voltage ok
  { 0x60,  0x60,  read_diag3 },	// J702 pin 13 (service connector, default hi)
  { 0xfe4, 0xfe4, keypad_r },
  { 0xff0, 0xfff, cru_r },
PORT_END

static MACHINE_INIT(nsm) {
  HC4094_init(&hc4094nsm);
}

static MACHINE_RESET(nsm) {
  // Disable auto wait state generation on reset
  tms9995reset_param param = { 0 };
  cpunum_reset(0, &param, NULL);
  memset(&locals, 0, sizeof(locals));
}

static MACHINE_STOP(nsm) {
  int i;
  locals.uv = 1;
  cpu_set_irq_line(0, 0, PULSE_LINE); // IRQ routine saves NVRAM
  // wait some timeslices before shutdown so the IRQ routine can finish
  for (i=0; i < 90; i++) {
    run_one_timeslice();
  }
}

static core_tLCDLayout dispNsm[] = {
  {0, 0, 0,8,CORE_SEG8D}, {0,18, 8,8,CORE_SEG8D},
  {3, 0,16,8,CORE_SEG8D}, {3,18,24,8,CORE_SEG8D},
#ifdef MAME_DEBUG
  {6, 9,32,8,CORE_SEG8D},
#else
  {6,11,33,2,CORE_SEG8D}, {6,19,38,2,CORE_SEG8D},
#endif
  {0}
};

static SWITCH_UPDATE(nsm) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xe0, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xe3, 10);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1], 0xfa, 11);
    CORE_SETKEYSW(inports[CORE_COREINPORT+1] >> 8, 0xfc, 12);
  }
}

MACHINE_DRIVER_START(nsm)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(nsm,nsm,nsm)
  MDRV_CPU_ADD_TAG("mcpu", TMS9995, 11052000)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_PORTS(readport, writeport)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_UPDATE(nsm)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_TIMER_ADD(nsm_zc, 100)
  MDRV_DIPS(1) // needed for extra core inport!

  MDRV_SOUND_ADD(AY8910, nsm_ay8912Int)
MACHINE_DRIVER_END

INPUT_PORTS_START(nsm)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT   (0x0002, "Start",    KEYCODE_1)
    COREPORT_BIT   (0x0080, "Coin 1",   KEYCODE_3)
    COREPORT_BIT   (0x0040, "Coin 2",   KEYCODE_4)
    COREPORT_BIT   (0x0020, "Coin 3",   KEYCODE_5)
    COREPORT_BIT   (0x0001, "Tilt",     KEYCODE_DEL)
    COREPORT_BIT   (0x8000, "Test 1",   KEYCODE_7)
    COREPORT_BITTOG(0x4000, "Test 2",   KEYCODE_8)
    COREPORT_BITTOG(0x2000, "Test 3",   KEYCODE_9)
  PORT_START /* 1 */
    COREPORT_BIT   (0x0020, "Keypad 1", KEYCODE_1_PAD)
    COREPORT_BIT   (0x0010, "Keypad 2", KEYCODE_2_PAD)
    COREPORT_BIT   (0x0008, "Keypad 3", KEYCODE_3_PAD)
    COREPORT_BIT   (0x0400, "Keypad 4", KEYCODE_4_PAD)
    COREPORT_BIT   (0x0080, "Keypad 5", KEYCODE_5_PAD)
    COREPORT_BIT   (0x0040, "Keypad 6", KEYCODE_6_PAD)
    COREPORT_BIT   (0x2000, "Keypad 7", KEYCODE_7_PAD)
    COREPORT_BIT   (0x1000, "Keypad 8", KEYCODE_8_PAD)
    COREPORT_BIT   (0x0800, "Keypad 9", KEYCODE_9_PAD)
    COREPORT_BIT   (0x0002, "Keypad *", KEYCODE_ASTERISK)
    COREPORT_BIT   (0x8000, "Keypad 0", KEYCODE_0_PAD)
    COREPORT_BIT   (0x4000, "Keypad #", KEYCODE_ENTER_PAD)
INPUT_PORTS_END

static core_tGameData nsmGameData = {0,dispNsm,{FLIP_SWNO(75,66),1,1}};

// The Games (06/85)
static void init_gamesnsm(void) { core_gameData = &nsmGameData; }
ROM_START(gamesnsm)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("151595-602.bin", 0x0000, 0x2000, CRC(18f3e309) SHA1(f587d40ddf128f4e040e660c054e98cbebad99c7))
    ROM_LOAD("151596-603.bin", 0x2000, 0x2000, CRC(fdf1b48b) SHA1(fd63ef5e49aa4b84b10972e118bd54219d680d36))
    ROM_LOAD("151597-604.bin", 0x4000, 0x2000, CRC(5c8a3547) SHA1(843a56012227a61ff068bc1e14baf090d4a95fe1))
ROM_END
#define input_ports_gamesnsm input_ports_nsm
CORE_GAMEDEFNV(gamesnsm,"Games, The (NSM)",1985,"NSM (Germany)",nsm,0)

// Cosmic Flash (10/85)
static void init_cosflnsm(void) { core_gameData = &nsmGameData; }
ROM_START(cosflnsm)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("ic602.bin", 0x0000, 0x2000, CRC(1ce79cd7) SHA1(d5caf6d4323cc43a9c4379b51630190bf5799202))
    ROM_LOAD("ic603.bin", 0x2000, 0x2000, CRC(538de9f8) SHA1(c64942ffa600a2a7a37b986e1a346d351d0b65eb))
    ROM_LOAD("ic604.bin", 0x4000, 0x2000, CRC(4b52e5d7) SHA1(1547bb7a06ff0bdf55c635b2f4e57b7d93a191ee))
ROM_END
#define input_ports_cosflnsm input_ports_nsm
CORE_GAMEDEFNV(cosflnsm,"Cosmic Flash (NSM)",1985,"NSM (Germany)",nsm,0)

// Hot Fire Birds (12/85)
static void init_firebird(void) { core_gameData = &nsmGameData; }
ROM_START(firebird)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("nsmf02.764", 0x0000, 0x2000, CRC(236b5780) SHA1(19ef6e1fc900e5d94f615a4316f0383ed5ee939c))
    ROM_LOAD("nsmf03.764", 0x2000, 0x2000, CRC(d88c6ef5) SHA1(00edeefaab7e1141741aa132e6f7e56a911573be))
    ROM_LOAD("nsmf04.764", 0x4000, 0x2000, CRC(38a8add4) SHA1(74f781edc31aad07411feacad53c5f6cc73d09f4))
ROM_END
#define input_ports_firebird input_ports_nsm
CORE_GAMEDEFNV(firebird,"Hot Fire Birds",1985,"NSM (Germany)",nsm,0)

// Tag-Team Pinball (??/86)
// Amazon Hunt (??/8?)
