/* Technoplay hardware */
/* CPU: Motorola M68K */

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "sim.h"

static struct {
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
} locals;

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids = locals.solenoids;
  coreGlobals.solenoids2 = locals.sols2;
  core_updateSw(1);
}

static MACHINE_INIT(xforce) {
  memset(&locals, 0, sizeof(locals));
}

static READ16_HANDLER(sw_r) {
  int swL = core_getSw(offset*2) ? 1 : 0;
  int swH = core_getSw(offset*2+1) ? 1 : 0;
  return 0x8080 | (swL << 8) | swH;
}

static READ16_HANDLER(input_key_r) {
  return 0;
}

static const UINT16 core_ascii2seg[] = {
  /* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x20-0x27 */ 0x0000, 0x0309, 0x0220, 0x2A4E, 0x2A6D, 0x6E65, 0x135D, 0x0400,
  /* 0x28-0x2f */ 0x1400, 0x4100, 0x7F40, 0x2A40, 0x0000, 0x0840, 0x0000, 0x4400,
  /* 0x30-0x37 */ 0x003f, 0x2200, 0x085B, 0x084f, 0x0866, 0x086D, 0x087D, 0x0007,
  /* 0x38-0x3f */ 0x087F, 0x086F, 0x0009, 0x4001, 0x4408, 0x0848, 0x1108, 0x2803,
  /* 0x40-0x47 */ 0x205F, 0x0877, 0x2A0F, 0x0039, 0x220F, 0x0079, 0x0071, 0x083D,
  /* 0x48-0x4f */ 0x0876, 0x2209, 0x001E, 0x1470, 0x0038, 0x0536, 0x1136, 0x003f,
  /* 0x50-0x57 */ 0x0873, 0x103F, 0x1873, 0x086D, 0x2201, 0x003E, 0x4430, 0x5036,
  /* 0x58-0x5f */ 0x5500, 0x2500, 0x4409, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};
static WRITE16_HANDLER(disp_w) {
  locals.segments[offset*2].w = core_ascii2seg[data >> 8];
  locals.segments[offset*2 + 1].w = core_ascii2seg[data & 0xff];
}

static MEMORY_READ16_START(readmem)
  { 0x000000, 0x003fff, MRA16_ROM },
  { 0x004000, 0x004255, MRA16_RAM },
  { 0x004256, 0x004295, sw_r },
  { 0x004296, 0x004fff, MRA16_RAM },
  { 0x015000, 0x015001, input_key_r },
  { 0x006000, 0x00ffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START(writemem)
  { 0x004000, 0x00404f, MWA16_RAM },
  { 0x004050, 0x00406f, disp_w },
  { 0x004070, 0x004fff, MWA16_RAM },
  { 0x017800, 0x017801, MWA16_NOP },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG16},
  {3, 0,16,16,CORE_SEG16},
  {0}
};
static core_tGameData xforceGameData = {GEN_ZAC2, disp};
static void init_xforce(void) {
  core_gameData = &xforceGameData;
}

MACHINE_DRIVER_START(xforce)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(xforce, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", M68000, 8000000)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
MACHINE_DRIVER_END

INPUT_PORTS_START(xforce) \
  CORE_PORTS \
  SIM_PORTS(4) \
  PORT_START /* 0 */ \
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
      COREPORT_DIPSET(0x0080, "1" )
INPUT_PORTS_END

ROM_START(xforce) \
  NORMALREGION(0x1000000, REGION_CPU1) \
    ROM_LOAD16_BYTE("ic15", 0x000001, 0x8000, CRC(fb8d2853)) \
    ROM_LOAD16_BYTE("ic17", 0x000000, 0x8000, CRC(122ef649))
ROM_END

CORE_GAMEDEFNV(xforce, "X Force", 1987, "Technoplay", xforce, GAME_NO_SOUND)
