// Mirco Games: Lucky Draw

#include "driver.h"
#include "core.h"
#include "cpu/i8039/i8039.h"

static struct {
  int vblankCount;
  UINT8 p1, p2;
  UINT32 solenoids;
} locals;

static INTERRUPT_GEN(mirco_vblank) {
  locals.vblankCount++;
  if (!(locals.vblankCount % 2)) {
    coreGlobals.solenoids = locals.solenoids;
  }
  core_updateSw(TRUE); // flippers are directly driven by solenoids
}

static INTERRUPT_GEN(t0) {
  static int tc;
  i8035_set_reg(I8035_TC, tc ? -1 : 0);
  tc = !tc;
}

static SWITCH_UPDATE(mirco) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],      0x01, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x0f, 4);
  }
  cpu_set_irq_line(0, 0, coreGlobals.swMatrix[0] & 1 ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(port_r) {
  UINT8 *ram = memory_region(REGION_CPU1) + 0x1000;
  UINT8 retVal = ram[offset];
  logerror("%03x: R%x %02x - %02x\n", activecpu_get_previouspc(), locals.p2 >> 4, offset, locals.p1);
  switch (locals.p2 >> 4) {
    case 0: retVal = ~core_revbyte(coreGlobals.swMatrix[1 + core_BitColToNum(0xff ^ locals.p1)]); break;
    // case 1: unused by code, possible expansion (J4-7)
    case 4: retVal = core_getDip(0); break;
    case 5: retVal = core_getDip(1); break;
    case 6: retVal = core_getDip(3); printf("DIP3 used!!!"); break; // unused, so place it at the end and don't map it
    case 7: retVal = core_getDip(2); break;
  }
  return retVal;
}

static READ_HANDLER(p1_r) {
  logerror("%03x: RP1\n", activecpu_get_previouspc());
  return locals.p1;
}

static READ_HANDLER(p2_r) {
  logerror("%03x: RP2\n", activecpu_get_previouspc());
  return locals.p2;
}

static WRITE_HANDLER(port_w) {
  static int solBank;
  static UINT8 lampData;
  static UINT8 segData[4];
  static UINT8 dispBlank;
  int col;
  UINT8 *ram = memory_region(REGION_CPU1) + 0x1000;
  logerror("%03x: W%x %02x: %02x\n", activecpu_get_previouspc(), locals.p2 >> 4, offset, data);
  switch (locals.p2 >> 4) {
    case 0:
    case 0x0e:
      ram[offset] = data;
      break;
    case 1:
      solBank = data;
      if (!solBank) {
        locals.solenoids = 0;
      }
      break;
    case 2:
      if (solBank) {
        locals.vblankCount = 0;
        if (solBank & 1) {
          locals.solenoids = (locals.solenoids & 0xff00) | data;
        }
        if (solBank & 2) {
          locals.solenoids = (locals.solenoids & 0x00ff) | (data << 8);
        }
        if (data) { // sometimes data is flaky, so allow fast overwrites and also keep previous data
          coreGlobals.solenoids = locals.solenoids;
        }
      }
      break;
    case 3:
      dispBlank = 0xff ^ data;
      break;
    // case 5/6: unused by code, possible expansion (J4-14, J4-13)
    case 8:
      segData[0] = data;
      break;
    case 9:
      segData[1] = data;
      break;
    case 0x0a:
      segData[2] = data;
      break;
    case 0x0b:
      segData[3] = data;
      break;
    case 0x0c:
      lampData = data;
      break;
    case 0x0d:
      col = core_BitColToNum(data);
      coreGlobals.lampMatrix[col] = lampData;
      coreGlobals.segments[col].w = dispBlank & 1 ? 0 : core_bcd2seg7[segData[0] & 0x0f];
      coreGlobals.segments[8 + col].w = dispBlank & 2 ? 0 : core_bcd2seg7[segData[0] >> 4];
      coreGlobals.segments[16 + col].w = dispBlank & 4 ? 0 : core_bcd2seg7[segData[1] & 0x0f];
      coreGlobals.segments[24 + col].w = dispBlank & 8 ? 0 : core_bcd2seg7[segData[1] >> 4];
      coreGlobals.segments[32 + col].w = dispBlank & 0x10 ? 0 : core_bcd2seg7[segData[2] & 0x0f];
      coreGlobals.segments[40 + col].w = dispBlank & 0x20 ? 0 : core_bcd2seg7[segData[2] >> 4];
      coreGlobals.segments[48 + col].w = dispBlank & 0x40 ? 0 : core_bcd2seg7[segData[3] & 0x0f];
      coreGlobals.segments[56 + col].w = dispBlank & 0x80 ? 0 : core_bcd2seg7[segData[3] >> 4];
      break;
    default:
      printf("%03x: W%x %02x: %02x\n", activecpu_get_previouspc(), locals.p2 >> 4, offset, data);
  }
}

static WRITE_HANDLER(p1_w) {
  logerror("%03x: WP1: %02x\n", activecpu_get_previouspc(), data);
  locals.p1 = data;
}

static WRITE_HANDLER(p2_w) {
  logerror("%03x: WP2: %02x\n", activecpu_get_previouspc(), data);
  locals.p2 = data;
}

static MEMORY_READ_START(mirco_readmem)
  { 0x0000, 0x0bff, MRA_ROM },
  { 0x1000, 0x10ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(mirco_writemem)
  { 0x0000, 0x0bff, MWA_ROM },
  { 0x1000, 0x10ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
MEMORY_END

static PORT_READ_START(mirco_readport)
  { 0x00, 0xff, port_r },
  { I8039_p1, I8039_p1, p1_r },
  { I8039_p2, I8039_p2, p2_r },
MEMORY_END

static PORT_WRITE_START(mirco_writeport)
  { 0x00, 0xff, port_w },
  { I8039_p1, I8039_p1, p1_w },
  { I8039_p2, I8039_p2, p2_w },
MEMORY_END

static MACHINE_INIT(mirco) {
  memset(&locals, 0, sizeof(locals));
  locals.p1 = 0xff;
}

MACHINE_DRIVER_START(mirco)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", I8035, 6000000/15) // 8035 has internal divider by 15!
  MDRV_CPU_MEMORY(mirco_readmem, mirco_writemem)
  MDRV_CPU_PORTS(mirco_readport, mirco_writeport)
  MDRV_SWITCH_UPDATE(mirco)
  MDRV_CORE_INIT_RESET_STOP(mirco,NULL,NULL)
  MDRV_CPU_VBLANK_INT(mirco_vblank, 1)
  MDRV_CPU_PERIODIC_INT(t0, 200)
  MDRV_DIPS(24)
  MDRV_NVRAM_HANDLER(generic_0fill)
MACHINE_DRIVER_END

static const core_tLCDLayout disp[] = {
  {0, 0, 0,6,CORE_SEG7}, {0,28, 8,6,CORE_SEG7},
  {5, 0,16,6,CORE_SEG7}, {5,28,24,6,CORE_SEG7},
  {3,14,32,2,CORE_SEG7}, {3,22,36,2,CORE_SEG7},
  {0}
};
static core_tGameData mircoGameData = {0,disp,{FLIP_SWNO(32,31),0,1}};
static void init_lckydraw(void) { core_gameData = &mircoGameData; }
INPUT_PORTS_START(lckydraw)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */ \
    COREPORT_BIT(     0x0200, "Start",        KEYCODE_1) \
    COREPORT_BIT(     0x0100, "Coin",         KEYCODE_5) \
    COREPORT_BIT(     0x0001, "Self Test",    KEYCODE_7) \
    COREPORT_BIT(     0x0400, "Tilt",         KEYCODE_INSERT) \
    COREPORT_BIT(     0x0800, "Mercury Tilt", KEYCODE_HOME) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0101, 0x0000, "Games / coins") \
      COREPORT_DIPSET(0x0000, "1/1" ) \
      COREPORT_DIPSET(0x0101, "3/2" ) \
      COREPORT_DIPSET(0x0001, "2/1" ) \
      COREPORT_DIPSET(0x0100, "3/1" ) \
    COREPORT_DIPNAME( 0x0202, 0x0202, "Max. replays") \
      COREPORT_DIPSET(0x0000, "5" ) \
      COREPORT_DIPSET(0x0002, "10" ) \
      COREPORT_DIPSET(0x0200, "15" ) \
      COREPORT_DIPSET(0x0202, "20" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "Match feature") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0008, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Balls per game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0010, "5" ) \
    COREPORT_DIPNAME( 0x0020, 0x0020, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Special award") \
      COREPORT_DIPSET(0x0000, "Extra Ball" ) \
      COREPORT_DIPSET(0x0800, "Replay" ) \
    COREPORT_DIPNAME( 0x1000, 0x1000, "Startup tune") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x2000, "Attract mode lamps") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Free Play ") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0001, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
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

ROM_START(lckydraw)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("lckydrw1.rom", 0x0000, 0x0400, CRC(58ebb50f) SHA1(016ed66b4ee9979aa109c0ce085597a62d33bf8d))
    ROM_LOAD("lckydrw2.rom", 0x0400, 0x0400, CRC(816b9e20) SHA1(0dd8acc633336f250960ebe89cc707fd115afeee))
    ROM_LOAD("lckydrw3.rom", 0x0800, 0x0400, CRC(464155bb) SHA1(5bbf784dba9149575444e6b1250ac9b5c2bced87))
ROM_END
CORE_GAMEDEFNV(lckydraw,"Lucky Draw",1978,"Mirco",mirco,GAME_USES_CHIMES)
