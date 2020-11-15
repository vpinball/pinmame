#include "driver.h"
#include "core.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8155.h"

static struct {
  int swCol;
  int bitCnt;
  int vblankCnt;
} locals;

static INTERRUPT_GEN(vblank) {
  locals.vblankCnt = (locals.vblankCnt + 1) % 4;
  if (!locals.vblankCnt) {
    coreGlobals.lampMatrix[4] = coreGlobals.lampMatrix[4] & 0xef;
  }
  core_updateSw(core_getSol(5));
}

static WRITE_HANDLER(riot_porta_w) {
  locals.bitCnt = 0;
  locals.swCol = core_BitColToNum(0x3f & ~data);
  if (!(data & 0x0f)) {
    coreGlobals.lampMatrix[4] = (coreGlobals.lampMatrix[4] & 0x7f) | (data & 0x80);
  }
  if (data == 0x0a) coreGlobals.diagnosticLed = 2; // indicates fault, CPU is looped until reset or NMI
  logerror("%04x: A %02x %d\n", activecpu_get_pc(), data, locals.swCol);
}

static WRITE_HANDLER(riot_portb_w) {
  coreGlobals.solenoids = core_revbyte(data ^ 0xc8);
  logerror("%04x: B %02x\n", activecpu_get_pc(), data);
}

static int colForBit(UINT8 data) {
  if (~data & 0x02) return 1;
  if (~data & 0x04) return 2;
  if (~data & 0x08) return 3;
  return 0;
}

static WRITE_HANDLER(riot_portc_w) {
  static UINT8 lastC;
  static UINT32 latch;
  int strobe;
  int col = colForBit(data);
  if (col) {
    if (data & 1)
      latch &= ~(1 << locals.bitCnt);
    else
      latch |= 1 << locals.bitCnt;
    locals.bitCnt++;
    switch (col) {
      case 1:
      case 2:
        if (locals.bitCnt > 15) {
          strobe = (latch & 0x1000) ? 1 : 0; // this is probably how additional displays are accessed, needs other games to confirm
          coreGlobals.segments[strobe + 16 - 8 * col].w = core_bcd2seg7[core_revnyb(0x0f & latch)];
          coreGlobals.segments[strobe + 18 - 8 * col].w = core_bcd2seg7[core_revnyb(0x0f & latch >> 4)];
          coreGlobals.segments[strobe + 20 - 8 * col].w = core_bcd2seg7[core_revnyb(0x0f & latch >> 8)];
          coreGlobals.segments[strobe + 22 - 8 * col].w = core_bcd2seg7[core_revnyb(0x0f & latch >> 12)];
        }
        break;
      case 3:
        if (locals.bitCnt > 31) {
          coreGlobals.lampMatrix[0] = core_revbyte(latch);
          coreGlobals.lampMatrix[1] = core_revbyte(latch >> 8);
          coreGlobals.lampMatrix[2] = core_revbyte(latch >> 16);
          coreGlobals.lampMatrix[3] = core_revbyte(latch >> 24);
        }
    }
  }
  cpu_set_irq_line(0, I8085_RST75_LINE, ~lastC & data & 0x20 ? ASSERT_LINE : CLEAR_LINE);
  lastC = data;
  if (~data & 0x10) {
    locals.vblankCnt = 0;
    coreGlobals.lampMatrix[4] = (coreGlobals.lampMatrix[4] & 0xef) | 0x10;
  }
  // HACK to make tilt sound stop again
  if (data == 0x0b && memory_region(REGION_CPU1)[0x8014] & 1) {
    AY8910_reset(0);
  }
  logerror("%04x: C %02x %d %d\n", activecpu_get_pc(), data, col, data & 1);
}

//static WRITE_HANDLER(riot_timer_w) { logerror("8155 Timer out: %x\n", data); }

static i8155_interface i8155_intf = {
  1,
  {0}, {0}, {0},
  {riot_porta_w}, {riot_portb_w}, {riot_portc_w},
  {0} // {riot_timer_w}
};

static void sod_cb(int state) {
  coreGlobals.diagnosticLed = state;
}

static void install_sod_cb(int dummy) {
  i8085_set_SID(1); // SID line is fed by battery voltage (hi means NVRAM is good)
  i8085_set_SOD_callback(sod_cb);
}

static MACHINE_INIT(regama) {
  memset(&locals, 0, sizeof(locals));
  i8155_init(&i8155_intf);
}

static MACHINE_RESET(regama) {
  i8155_reset(0);
  AY8910_reset(0);
  timer_set(TIME_IN_USEC(1), 0, install_sod_cb); // needs to be installed after reset is through because setting CPU context overrides it!
}

static SWITCH_UPDATE(regama) {
  static UINT8 lastSw7;
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x80, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT],      0x1c, 6);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x07, 7);
  }
  i8085_set_SID(coreGlobals.swMatrix[7] & 1 ? 0 : 1); // clear and verify RAM on reset when SID line is low
  // HACK to make coin switches work. No idea what feeds these values but I don't think it's done by manipulating the RAM directly!
  if (~lastSw7 & coreGlobals.swMatrix[7] & 2) {
    memory_region(REGION_CPU1)[0x803f] = 1;
    memory_region(REGION_CPU1)[0x803d] |= 2;
  } else if (~lastSw7 & coreGlobals.swMatrix[7] & 4) {
    memory_region(REGION_CPU1)[0x803f] = 2;
  } else if (~lastSw7 & coreGlobals.swMatrix[7] & 8) {
    memory_region(REGION_CPU1)[0x803f] = 3;
  } else if (~lastSw7 & coreGlobals.swMatrix[7] & 0x10) {
    memory_region(REGION_CPU1)[0x803f] = 4;
    memory_region(REGION_CPU1)[0x803d] |= 2;
  }
  cpu_set_irq_line(0, I8085_INTR_LINE, coreGlobals.swMatrix[7] & 0x40 ? ASSERT_LINE : CLEAR_LINE); // INTR stalls the machine
  cpu_set_nmi_line(0, coreGlobals.swMatrix[7] & 0x80 ? ASSERT_LINE : CLEAR_LINE); // NMI resets the machine
  lastSw7 = coreGlobals.swMatrix[7];
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0x80ff, MRA_RAM },
  { 0x8800, 0x88ff, MRA_RAM },
  { 0x8900, 0x8907, i8155_0_r },
  { 0x9000, 0x9000, AY8910_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0000, 0x7fff, MWA_NOP },
  { 0x8000, 0x80ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x8800, 0x88ff, MWA_RAM },
  { 0x8900, 0x8907, i8155_0_w },
  { 0x9001, 0x9001, AY8910_control_port_0_w },
  { 0x9002, 0x9002, AY8910_write_port_0_w },
MEMORY_END

static READ_HANDLER(ay8910_porta_r) {
  return core_getDip(0);
}

static READ_HANDLER(ay8910_portb_r) {
  return ~coreGlobals.swMatrix[1 + locals.swCol];
}

static struct AY8910interface ay8910Int = {
  1,
  6144000/2,
  { 30 },
  { ay8910_porta_r },
  { ay8910_portb_r },
};

MACHINE_DRIVER_START(regama)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(regama,regama,NULL)
  MDRV_CPU_ADD_TAG("mcpu", 8085A, 6144000./2.)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(regama)
  MDRV_DIPS(8)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_SOUND_ADD(AY8910, ay8910Int)
MACHINE_DRIVER_END

INPUT_PORTS_START(trebol)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT(     0x0200, "Coin #1",     KEYCODE_3)
    COREPORT_BIT(     0x0400, "Coin #2",     KEYCODE_5)
    COREPORT_BIT(     0x0004, "Start",       KEYCODE_1)
    COREPORT_BIT(     0x8000, "Tilt",        KEYCODE_DEL)
    COREPORT_BITTOG(  0x0008, "Coin Door",   KEYCODE_END)
    COREPORT_BIT(     0x0010, "Bookkeeping", KEYCODE_7)
    COREPORT_BIT(     0x0100, "Clear NVRAM", KEYCODE_0)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0001, 0x0000, "Credits for coin #1")
      COREPORT_DIPSET(0x0000, "1" )
      COREPORT_DIPSET(0x0001, "2" )
    COREPORT_DIPNAME( 0x0006, 0x0000, "Credits for coin #2")
      COREPORT_DIPSET(0x0000, "4" )
      COREPORT_DIPSET(0x0002, "5" )
      COREPORT_DIPSET(0x0004, "6" )
      COREPORT_DIPSET(0x0006, "7" )
    COREPORT_DIPNAME( 0x0018, 0x0000, "Balls per game")
      COREPORT_DIPSET(0x0000, "3" )
      COREPORT_DIPSET(0x0008, "4" )
      COREPORT_DIPSET(0x0010, "5" )
      COREPORT_DIPSET(0x0018, "5" )
    COREPORT_DIPNAME( 0x0060, 0x0000, "Replay scores")
      COREPORT_DIPSET(0x0000, "600K / 1.2M" )
      COREPORT_DIPSET(0x0020, "700K / 1.4M" )
      COREPORT_DIPSET(0x0040, "800K / 1.6M" )
      COREPORT_DIPSET(0x0060, "900K / 1.8M" )
    COREPORT_DIPNAME( 0x0080, 0x0080, "Repetitive Special")
      COREPORT_DIPSET(0x0080, DEF_STR(Off) )
      COREPORT_DIPSET(0x0000, DEF_STR(On) )
INPUT_PORTS_END

static core_tLCDLayout disp[] = {
  {3, 0, 0, 6,CORE_SEG7},
  {0,16, 8, 1,CORE_SEG7}, {0,20,10, 2,CORE_SEG7},
  {0}
};
static core_tGameData trebolGameData = {0,disp,{FLIP_SW(FLIP_L)}};
static void init_trebol(void) {
  core_gameData = &trebolGameData;
}

ROM_START(trebol)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("m69.bin", 0x0000, 0x2000, CRC(8fb8cd39) SHA1(4ed505d06b489ce83316fdaa39f7ce128011fb4b))
ROM_END
CORE_GAMEDEFNV(trebol, "Trebol", 1985, "Regama (Spain)", regama, 0)
