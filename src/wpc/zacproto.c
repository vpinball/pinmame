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
} locals;

static INTERRUPT_GEN(vblank) {
  locals.vblankCount++;

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));

  if (locals.solenoids & 0xffff) {
    coreGlobals.solenoids = locals.solenoids;
    locals.vblankCount = 1;
  }
  if ((locals.vblankCount % 4) == 0)
    coreGlobals.solenoids = locals.solenoids;

  core_updateSw(coreGlobals.lampMatrix[0] & 0x01);
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

static WRITE_HANDLER(disp_w) {
  locals.segments[9-offset*2].w = core_bcd2seg7e[data & 0x0f];
  locals.segments[8-offset*2].w = core_bcd2seg7e[data >> 4];
}

static WRITE_HANDLER(sol_w) {
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xffffff00) | data;
  else
    locals.solenoids = (locals.solenoids & 0xffff00ff) | (data << 8);
}

static WRITE_HANDLER(sound_w) {
  if (!offset)
    locals.solenoids = (locals.solenoids & 0xff00ffff) | data << 16;
  else
    locals.solenoids = (locals.solenoids & 0x00ffffff) | (data << 24);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x0bff, MRA_ROM },
  { 0x0c00, 0x0cff, MRA_NOP },
  { 0x0d00, 0x0dff, MRA_RAM },
  { 0x0e00, 0x0e03, sw_r },
  { 0x0e04, 0x0e07, dip_r },
  { 0x0e08, 0x13fe, MRA_NOP },
  { 0x13ff, 0x17ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0d00, 0x0dff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x0e00, 0x0e01, sol_w },
  { 0x0e02, 0x0e06, disp_w },
  { 0x0e07, 0x0e08, sound_w },
  { 0x0e09, 0x0e16, lamp_w },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 4, 6,CORE_SEG7},
  {3, 0, 0, 2,CORE_SEG7},
  {3, 8, 2, 2,CORE_SEG7},
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
MACHINE_DRIVER_END

INPUT_PORTS_START(zacProto) \
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
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Coin Slot #1/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "Coin Slot #1/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Coin Slot #1/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Coin Slot #1/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Coin Slot #2/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Coin Slot #2/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "Coin Slot #2/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Coin Slot #2/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "Match feature") \
      COREPORT_DIPSET(0x0000, " off" ) \
      COREPORT_DIPSET(0x0008, " on" ) \
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
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0600, 0x0200, "Balls in Play") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0200, "3" ) \
      COREPORT_DIPSET(0x0400, "5" ) \
      COREPORT_DIPSET(0x0600, "7" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

ROM_START(strike) \
  NORMALREGION(0x8000, REGION_CPU1) \
    ROM_LOAD("strike1.bin", 0x0000, 0x0400, CRC(650abc54) SHA1(6a4f83016a38338ba6a04271532f0880264e61a7)) \
    ROM_LOAD("strike2.bin", 0x0400, 0x0400, CRC(13c5a168) SHA1(2da3a5bc0c28a2aacd8c1396dac95cf35f8797cd)) \
    ROM_LOAD("strike3.bin", 0x0800, 0x0400, CRC(ebbbf315) SHA1(c87e961c8e5e99b0672cd632c5e104ea52088b5d)) \
    ROM_LOAD("strike4.bin", 0x1400, 0x0400, CRC(ca0eddd0) SHA1(52f9faf791c56b68b1806e685d0479ea67aba019))
ROM_END

#define input_ports_strike input_ports_zacProto

CORE_GAMEDEFNV(strike, "Strike", 1978, "Zaccaria", zacProto, GAME_NO_SOUND)
