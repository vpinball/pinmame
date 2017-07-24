/* Zaccaria Prototype Solid State machines */
/* CPU: National Semiconductor SC/MP ISP-8A 500D */

#include "driver.h"
#include "cpu/scamp/scamp.h"
#include "core.h"
#include "sim.h"

static struct {
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int vblankCount;
  int dispCount, output, enable;
  UINT8 sndCmd;
} locals;

static INTERRUPT_GEN(vblank) {
  locals.vblankCount++;

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  if (++locals.dispCount > 15) {
    locals.dispCount = 0;
    memset(locals.segments, 0, sizeof(locals.segments));
  }
  if (locals.solenoids & 0xffff) {
    coreGlobals.solenoids = locals.solenoids;
    locals.vblankCount = 1;
  }
  if ((locals.vblankCount % 4) == 0)
    coreGlobals.solenoids = locals.solenoids;

  core_updateSw(core_getSol(17));
}

static MACHINE_INIT(zacProto) {
  memset(&locals, 0, sizeof(locals));
}

static MACHINE_STOP(zacProto) {
  memset(&locals, 0, sizeof(locals));
}

static SWITCH_UPDATE(zacProto) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],     0x03,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8,0x0f,1);
  }
}

static READ_HANDLER(sw_r) {
  SCAMP_set_sense_a(coreGlobals.swMatrix[0] & 0x01);
  SCAMP_set_sense_b(coreGlobals.swMatrix[0] & 0x02);
  return coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r) {
  return core_getDip(offset);
}

static int bcd2seg7f[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,
/* A    B    C    D    E    F */
  0x77,0x7c,0x39,0x5e,0x79,0x71
};

static WRITE_HANDLER(disp_w) {
  locals.dispCount = 0;
  locals.segments[9-offset*2].w = bcd2seg7f[data & 0x0f];
  locals.segments[8-offset*2].w = bcd2seg7f[data >> 4];
  // fake single 0 digit
  locals.segments[10].w = locals.segments[9].w ? bcd2seg7f[0] : 0;
  // turn on periods
  if (locals.segments[4].w) locals.segments[4].w |= 0x80;
  if (locals.segments[7].w) locals.segments[7].w |= 0x80;
}

static WRITE_HANDLER(sol_w) {
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xffffff00) | data;
  else
    locals.solenoids = (locals.solenoids & 0xffff00ff) | (data << 8);
}

static WRITE_HANDLER(sound_w) {
  if (data) {
    if (!offset) {
      locals.sndCmd = (locals.sndCmd & 0x40) | (data & 0x3f);
    } else {
      locals.enable = (data & 0x02) >> 1;
      locals.sndCmd = (locals.sndCmd & 0x3f) | ((data & 0x01) << 6);
      if (!locals.enable)
        locals.output = (data & 0x1c) >> 2;
    }
    logerror("sound %d: snd data = %02x, enable = %x, output = %x\n", offset, locals.sndCmd, locals.enable, locals.output);
    discrete_sound_w(1, ((locals.sndCmd << (7-locals.output)) & 0xff) >> (7-locals.output));
    discrete_sound_w(2, 1);
  }
}

static void zacProto_sndTimer(int data) {
  if (locals.sndCmd) {
    locals.sndCmd--;
    discrete_sound_w(1, ((locals.sndCmd << (7-locals.output)) & 0xff) >> (7-locals.output));
  } else
    discrete_sound_w(2, 0);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xfffeffff) | ((data & 0x01) << 16);
  if (offset > 7) logerror("lamp write to col. %d: %02x\n", offset, data);
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x0bff, MRA_ROM },
  { 0x0d00, 0x0dff, MRA_RAM },
  { 0x0e00, 0x0e04, sw_r },
  { 0x0e05, 0x0e07, dip_r },
  { 0x13ff, 0x17ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0d00, 0x0dff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x0e00, 0x0e01, sol_w },
  { 0x0e02, 0x0e06, disp_w },
  { 0x0e07, 0x0e08, sound_w },
  { 0x0e09, 0x0e16, lamp_w },
MEMORY_END

DISCRETE_SOUND_START(zacProto_discInt)
	DISCRETE_INPUT(NODE_01,1,0x0003,0) // tone
	DISCRETE_INPUT(NODE_02,2,0x0003,0) // enable
	DISCRETE_MULTADD(NODE_05,1,NODE_01,2,90)
	DISCRETE_SINEWAVE(NODE_10,NODE_02,NODE_05,20000,10000,0)
	DISCRETE_GAIN(NODE_20,NODE_10,12)
	DISCRETE_OUTPUT(NODE_20, 50)
DISCRETE_SOUND_END

static core_tLCDLayout disp[] = {
  {0, 0, 4, 1,CORE_SEG8D},{0, 2, 5, 2,CORE_SEG7},{0, 6, 7, 1,CORE_SEG8D},{0, 8, 8, 3,CORE_SEG7},
  {3, 2, 0, 2,CORE_SEG7S},
  {3,13, 2, 2,CORE_SEG7S},
  {0}
};
static core_tGameData strikeGameData = {GEN_ZAC1,disp};
static void init_strike(void) {
  core_gameData = &strikeGameData;
}

MACHINE_DRIVER_START(zacProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(zacProto,NULL,zacProto)
  MDRV_CPU_ADD_TAG("mcpu", SCAMP, 1000000)
  MDRV_SWITCH_UPDATE(zacProto)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(24)
  MDRV_SOUND_ADD(DISCRETE, zacProto_discInt)
  MDRV_TIMER_ADD(zacProto_sndTimer, 250)
MACHINE_DRIVER_END

INPUT_PORTS_START(strike) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0400, "Start Button",  KEYCODE_1) \
    COREPORT_BIT   (0x0100, "Coin Slot #1",  KEYCODE_3) \
    COREPORT_BIT   (0x0200, "Coin Slot #2",  KEYCODE_5) \
    COREPORT_BITTOG(0x0002, "Coin Door",     KEYCODE_END) \
    COREPORT_BIT   (0x0800, "Tilt/Advance",  KEYCODE_INSERT) \
    COREPORT_BITTOG(0x0001, "Sense Input A", KEYCODE_PGDN) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coin slot #1 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1/2" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0003, "1 1/2" ) \
      COREPORT_DIPSET(0x0004, "2" ) \
      COREPORT_DIPSET(0x0005, "2 1/2" ) \
      COREPORT_DIPSET(0x0006, "3" ) \
      COREPORT_DIPSET(0x0007, "3 1/2" ) \
      COREPORT_DIPSET(0x0008, "4" ) \
      COREPORT_DIPSET(0x0009, "4 1/2" ) \
      COREPORT_DIPSET(0x000a, "5" ) \
      COREPORT_DIPSET(0x000b, "5 1/2" ) \
      COREPORT_DIPSET(0x000c, "6" ) \
      COREPORT_DIPSET(0x000d, "6 1/2" ) \
      COREPORT_DIPSET(0x000e, "7" ) \
      COREPORT_DIPSET(0x000f, "7 1/2" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0040, "Coin slot #2 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1/2" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPSET(0x0030, "1 1/2" ) \
      COREPORT_DIPSET(0x0040, "2" ) \
      COREPORT_DIPSET(0x0050, "2 1/2" ) \
      COREPORT_DIPSET(0x0060, "3" ) \
      COREPORT_DIPSET(0x0070, "3 1/2" ) \
      COREPORT_DIPSET(0x0080, "4" ) \
      COREPORT_DIPSET(0x0090, "4 1/2" ) \
      COREPORT_DIPSET(0x00a0, "5" ) \
      COREPORT_DIPSET(0x00b0, "5 1/2" ) \
      COREPORT_DIPSET(0x00c0, "6" ) \
      COREPORT_DIPSET(0x00d0, "6 1/2" ) \
      COREPORT_DIPSET(0x00e0, "7" ) \
      COREPORT_DIPSET(0x00f0, "7 1/2" ) \
    COREPORT_DIPNAME( 0x0300, 0x0200, "Replays for beating HSTD") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0300, "3" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "HSTD/Match award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0400, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Match feature") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0800, " on" ) \
    COREPORT_DIPNAME( 0x3000, 0x2000, "HSTD/Random award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x1000, "Extra Ball" ) \
      COREPORT_DIPSET(0x2000, "Replay" ) \
      COREPORT_DIPSET(0x3000, "Super Bonus" ) \
    COREPORT_DIPNAME( 0xc000, 0x8000, "Special award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x4000, "Extra Ball" ) \
      COREPORT_DIPSET(0x8000, "Replay" ) \
      COREPORT_DIPSET(0xc000, "Super Bonus" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Random feature") \
      COREPORT_DIPSET(0x0000, " on" ) \
      COREPORT_DIPSET(0x0001, " off" ) \
    COREPORT_DIPNAME( 0x0006, 0x0004, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0002, "3" ) \
      COREPORT_DIPSET(0x0004, "5" ) \
      COREPORT_DIPSET(0x0006, "7" ) \
    COREPORT_DIPNAME( 0x0018, 0x0018, "Strikes needed for Special") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0008, "2" ) \
      COREPORT_DIPSET(0x0010, "3" ) \
      COREPORT_DIPSET(0x0018, "4" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Unlimited Specials") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0020, " on" ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "Bonus Ball Award") \
      COREPORT_DIPSET(0x0000, "200,000 Pts" ) \
      COREPORT_DIPSET(0x0040, "Bonus Ball" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Unknown ") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0080, " on" )
INPUT_PORTS_END

ROM_START(strike) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("strike1.bin", 0x0000, 0x0400, CRC(650abc54) SHA1(6a4f83016a38338ba6a04271532f0880264e61a7)) \
    ROM_LOAD("strike2.bin", 0x0400, 0x0400, CRC(13c5a168) SHA1(2da3a5bc0c28a2aacd8c1396dac95cf35f8797cd)) \
    ROM_LOAD("strike3.bin", 0x0800, 0x0400, CRC(ebbbf315) SHA1(c87e961c8e5e99b0672cd632c5e104ea52088b5d)) \
    ROM_LOAD("strike4.bin", 0x1400, 0x0400, CRC(ca0eddd0) SHA1(52f9faf791c56b68b1806e685d0479ea67aba019))
ROM_END

CORE_GAMEDEFNV(strike, "Strike", 1978, "Zaccaria", zacProto, GAME_IMPERFECT_SOUND)

INPUT_PORTS_START(skijump) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0400, "Start Button",  KEYCODE_1) \
    COREPORT_BIT   (0x0100, "Coin Slot #1",  KEYCODE_3) \
    COREPORT_BIT   (0x0200, "Coin Slot #2",  KEYCODE_5) \
    COREPORT_BITTOG(0x0002, "Coin Door",     KEYCODE_END) \
    COREPORT_BIT   (0x0800, "Tilt/Advance",  KEYCODE_INSERT) \
    COREPORT_BITTOG(0x0001, "Sense Input A", KEYCODE_PGDN) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coin slot #1 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1/2" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0003, "1 1/2" ) \
      COREPORT_DIPSET(0x0004, "2" ) \
      COREPORT_DIPSET(0x0005, "2 1/2" ) \
      COREPORT_DIPSET(0x0006, "3" ) \
      COREPORT_DIPSET(0x0007, "3 1/2" ) \
      COREPORT_DIPSET(0x0008, "4" ) \
      COREPORT_DIPSET(0x0009, "4 1/2" ) \
      COREPORT_DIPSET(0x000a, "5" ) \
      COREPORT_DIPSET(0x000b, "5 1/2" ) \
      COREPORT_DIPSET(0x000c, "6" ) \
      COREPORT_DIPSET(0x000d, "6 1/2" ) \
      COREPORT_DIPSET(0x000e, "7" ) \
      COREPORT_DIPSET(0x000f, "7 1/2" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0040, "Coin slot #2 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1/2" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPSET(0x0030, "1 1/2" ) \
      COREPORT_DIPSET(0x0040, "2" ) \
      COREPORT_DIPSET(0x0050, "2 1/2" ) \
      COREPORT_DIPSET(0x0060, "3" ) \
      COREPORT_DIPSET(0x0070, "3 1/2" ) \
      COREPORT_DIPSET(0x0080, "4" ) \
      COREPORT_DIPSET(0x0090, "4 1/2" ) \
      COREPORT_DIPSET(0x00a0, "5" ) \
      COREPORT_DIPSET(0x00b0, "5 1/2" ) \
      COREPORT_DIPSET(0x00c0, "6" ) \
      COREPORT_DIPSET(0x00d0, "6 1/2" ) \
      COREPORT_DIPSET(0x00e0, "7" ) \
      COREPORT_DIPSET(0x00f0, "7 1/2" ) \
    COREPORT_DIPNAME( 0x0300, 0x0200, "Replays for beating HSTD") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0300, "3" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "HSTD/Match award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0400, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Match feature") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0800, " on" ) \
    COREPORT_DIPNAME( 0x3000, 0x2000, "HSTD/Random award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x1000, "Extra Ball" ) \
      COREPORT_DIPSET(0x2000, "Replay" ) \
      COREPORT_DIPSET(0x3000, "Super Bonus" ) \
    COREPORT_DIPNAME( 0xc000, 0x8000, "Special award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x4000, "Extra Ball" ) \
      COREPORT_DIPSET(0x8000, "Replay" ) \
      COREPORT_DIPSET(0xc000, "Super Bonus" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Random feature") \
      COREPORT_DIPSET(0x0000, " on" ) \
      COREPORT_DIPSET(0x0001, " off" ) \
    COREPORT_DIPNAME( 0x0006, 0x0004, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0002, "3" ) \
      COREPORT_DIPSET(0x0004, "5" ) \
      COREPORT_DIPSET(0x0006, "7" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
INPUT_PORTS_END

ROM_START(skijump) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("skijump1.bin", 0x0000, 0x0400, CRC(c0c0e18c) SHA1(d28ec2541f6c2e86e5b5514c7f9e558df68be72a)) \
    ROM_LOAD("skijump2.bin", 0x0400, 0x0400, CRC(b08aafb5) SHA1(ff6df4efa20a4461d525209a487d04896eeef29e)) \
    ROM_LOAD("skijump3.bin", 0x0800, 0x0400, CRC(9a8731c0) SHA1(9f7aaa8c6df04b925c8beff8b426c59bc3696f50)) \
    ROM_LOAD("skijump4.bin", 0x1400, 0x0400, CRC(fa064b51) SHA1(d4d02ca661e4084805f00247f31c0701320ab62d))
ROM_END
#define init_skijump init_strike

CORE_GAMEDEFNV(skijump,"Ski Jump", 1978, "Zaccaria", zacProto, GAME_IMPERFECT_SOUND)

INPUT_PORTS_START(spacecty) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0400, "Start Button",  KEYCODE_1) \
    COREPORT_BIT   (0x0100, "Coin Slot #1",  KEYCODE_3) \
    COREPORT_BIT   (0x0200, "Coin Slot #2",  KEYCODE_5) \
    COREPORT_BITTOG(0x0002, "Coin Door",     KEYCODE_END) \
    COREPORT_BIT   (0x0800, "Tilt/Advance",  KEYCODE_INSERT) \
    COREPORT_BITTOG(0x0001, "Sense Input A", KEYCODE_PGDN) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coin slot #1 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1/2" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0003, "1 1/2" ) \
      COREPORT_DIPSET(0x0004, "2" ) \
      COREPORT_DIPSET(0x0005, "2 1/2" ) \
      COREPORT_DIPSET(0x0006, "3" ) \
      COREPORT_DIPSET(0x0007, "3 1/2" ) \
      COREPORT_DIPSET(0x0008, "4" ) \
      COREPORT_DIPSET(0x0009, "4 1/2" ) \
      COREPORT_DIPSET(0x000a, "5" ) \
      COREPORT_DIPSET(0x000b, "5 1/2" ) \
      COREPORT_DIPSET(0x000c, "6" ) \
      COREPORT_DIPSET(0x000d, "6 1/2" ) \
      COREPORT_DIPSET(0x000e, "7" ) \
      COREPORT_DIPSET(0x000f, "7 1/2" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0040, "Coin slot #2 plays") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1/2" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPSET(0x0030, "1 1/2" ) \
      COREPORT_DIPSET(0x0040, "2" ) \
      COREPORT_DIPSET(0x0050, "2 1/2" ) \
      COREPORT_DIPSET(0x0060, "3" ) \
      COREPORT_DIPSET(0x0070, "3 1/2" ) \
      COREPORT_DIPSET(0x0080, "4" ) \
      COREPORT_DIPSET(0x0090, "4 1/2" ) \
      COREPORT_DIPSET(0x00a0, "5" ) \
      COREPORT_DIPSET(0x00b0, "5 1/2" ) \
      COREPORT_DIPSET(0x00c0, "6" ) \
      COREPORT_DIPSET(0x00d0, "6 1/2" ) \
      COREPORT_DIPSET(0x00e0, "7" ) \
      COREPORT_DIPSET(0x00f0, "7 1/2" ) \
    COREPORT_DIPNAME( 0x0300, 0x0200, "Special award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x0100, "Extra Ball" ) \
      COREPORT_DIPSET(0x0200, "Replay" ) \
      COREPORT_DIPSET(0x0300, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0c00, 0x0800, "Score award") \
      COREPORT_DIPSET(0x0000, "500,000 Pts" ) \
      COREPORT_DIPSET(0x0400, "Extra Ball" ) \
      COREPORT_DIPSET(0x0800, "Replay" ) \
      COREPORT_DIPSET(0x0c00, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x3000, 0x2000, "Type of Random number") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x1000, "Low" ) \
      COREPORT_DIPSET(0x2000, "Random" ) \
      COREPORT_DIPSET(0x3000, "High" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Random award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x4000, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "Extra ball award") \
      COREPORT_DIPSET(0x0000, "200,000 Pts" ) \
      COREPORT_DIPSET(0x8000, "Extra Ball" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "HSTD award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0001, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Match feature") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0002, " on" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Match award") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0004, "Super Bonus" ) \
    COREPORT_DIPNAME( 0x0018, 0x0008, "Balls per game") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0008, "3" ) \
      COREPORT_DIPSET(0x0010, "5" ) \
      COREPORT_DIPSET(0x0018, "7" ) \
    COREPORT_DIPNAME( 0x0060, 0x0000, "Pre-light lamps") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0020, "1st row" )
      COREPORT_DIPSET(0x0040, "2nd row" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Unlimited Specials") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0080, " on" ) \
INPUT_PORTS_END

ROM_START(spacecty) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("zsc1.dat", 0x0000, 0x0400, CRC(4405368f) SHA1(037ad7e7158424bb714b28e4effa2c96c8736ce4)) \
    ROM_LOAD("zsc2.dat", 0x0400, 0x0400, CRC(a6c41475) SHA1(7d7d851efb2db7d9a1988265cdff676260d753c3)) \
    ROM_LOAD("zsc3.dat", 0x0800, 0x0400, CRC(e6a2dcee) SHA1(d2dfff896ae90208c28179f9bbe43f93d7f2131c)) \
    ROM_LOAD("zsc4.dat", 0x1400, 0x0400, CRC(69e0bb95) SHA1(d9a1d0159bf49445b0ece0f9d7806ed80657c2b2))
ROM_END
#define init_spacecty init_strike

CORE_GAMEDEFNV(spacecty,"Space City", 1979, "Zaccaria", zacProto, GAME_IMPERFECT_SOUND)
