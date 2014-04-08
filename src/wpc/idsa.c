/************************************************************************************************
 IDSA (Spain)
 ------------
   Hardware:
   ---------
		CPU:     Z80 @ 4 MHz
			INT: IRQ @ 977 Hz (4MHz/2048/2)
		IO:      DMA, AY8910 ports
		DISPLAY: 7-digit 7-segment panels with PROM-based 5-bit BCD data (allowing a simple alphabet)
		SOUND:	 2 x AY8910 @ 2 MHz plus SP0256 @ 3.12 MHz on board
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/sp0256.h"

#define IDSA_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT16 dispData[4];
  int dispCol;
  int dispRow;
} locals;

static INTERRUPT_GEN(IDSA_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

static void IDSA_nmi(int data) {
  cpu_set_nmi_line(0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(IDSA_vblank) {
  core_updateSw(TRUE);
}

static SWITCH_UPDATE(IDSA) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x78, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x02, 3);
  }
}

static WRITE_HANDLER(ay8910_0_ctrl_w) { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w) { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_0_r)      { return AY8910Read(0); }

static WRITE_HANDLER(ay8910_1_ctrl_w) { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w) { AY8910Write(1,1,data); }
static READ_HANDLER (ay8910_1_r)      { return AY8910Read(1); }

static WRITE_HANDLER(ay8910_0_portA_w) {
//printf("1:%02x ", data);
  coreGlobals.lampMatrix[0] = data;
}
static WRITE_HANDLER(ay8910_0_portB_w) {
//printf("2:%02x ", data);
  coreGlobals.lampMatrix[1] = data;
}
static WRITE_HANDLER(ay8910_1_portA_w) {
  if ((data >> 4) != 0x0f) {
    locals.dispCol = data >> 4;
  } else {
    locals.dispRow = 0;
    coreGlobals.segments[32 + locals.dispCol].w = locals.dispData[0];
    coreGlobals.segments[16 + locals.dispCol].w = locals.dispData[1];
    coreGlobals.segments[locals.dispCol].w = locals.dispData[2];
  }
  coreGlobals.lampMatrix[2] = data & 0x0f;
}
static WRITE_HANDLER(ay8910_1_portB_w) {
//printf("4:%02x ", data);
  coreGlobals.lampMatrix[3] = data;
}
struct AY8910interface IDSA_ay8910Int = {
	2,					/* 2 chips */
	2000000,			/* 2 MHz */
	{ 30, 30 },				/* Volume */
	{ NULL },
	{ NULL },
	{ ay8910_0_portA_w, ay8910_1_portA_w },	/* Output Port A callback */
	{ ay8910_0_portB_w, ay8910_1_portB_w },	/* Output Port B callback */
};

static READ_HANDLER(sp0256_r) {
  static UINT16 retVal;
  if (!offset) retVal = spb640_r(0, 0);
  return offset ? (UINT8)(retVal >> 8) : (UINT8)(retVal & 0xff);
}
static WRITE_HANDLER(sp0256_w) {
  spb640_w(0, 0, data);
}
struct sp0256_interface IDSA_sp0256Int = {
  50, /* volume */
  3120000, /* clock */
  NULL,
  NULL,
  REGION_SOUND1
};

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[1 + (offset >> 4)];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(offset >> 4);
}

static MEMORY_READ_START(IDSA_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0x8000,0x87ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(IDSA_writemem)
  {0x0000,0x7fff, MWA_NOP},
  {0x8000,0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

// IDSA used small 32x8 6331 color PROMs to decode their 5-bit alphabet; I'd love to see this dumped someday! ;)
// Letters used by game: "Error", "P(Abc)-F", "FALtA", "1d5A"
static UINT16 idsa2seg7(UINT8 data) {
  switch (data & 0x1f) {
    case 0x0a: return 0x77; // A
    case 0x0b: return 0x7c; // b
    case 0x0c: return 0x58; // c
    case 0x0d: return 0x5e; // d
    case 0x0e: return 0x79; // E
    case 0x0f: return 0x71; // F
    case 0x10: return 0x3d; // G
    case 0x11: return 0x76; // H
    case 0x12: return 0x1e; // J
    case 0x13: return 0x75; // K
    case 0x14: return 0x38; // L
    case 0x15: return 0x56; // m? pretty much impossible with 7 segs...
    case 0x16: return 0x54; // n
    case 0x17: return 0x55; // n with tilde?
    case 0x18: return 0x5c; // o
    case 0x19: return 0x73; // P
    case 0x1a: return 0x50; // r
    case 0x1b: return 0x40; // -
    case 0x1c: return 0x78; // t
    case 0x1d: return 0x1c; // u / v
    case 0x1e: return 0x6e; // y
    case 0x1f: return 0;
    default: return core_bcd2seg7[data & 0x0f];
  }
}

static WRITE_HANDLER(col_w) {
//printf("%x:%02x ", offset, data);
}

static WRITE_HANDLER(disp_w) {
  locals.dispData[locals.dispRow] = idsa2seg7(data);
  locals.dispRow = (locals.dispRow + 1) % 4;
}

static PORT_READ_START(IDSA_readport)
  {0x00,0x50, sw_r},
  {0x60,0x70, dip_r},
  {0xb0,0xb1, sp0256_r},
MEMORY_END

static PORT_WRITE_START(IDSA_writeport)
  {0x80,0x8f, col_w},
  {0x90,0x90, disp_w},
  {0xc0,0xc1, sp0256_w},
  {0xe0,0xe0, ay8910_0_ctrl_w},
  {0xe1,0xe1, ay8910_0_data_w},
  {0xf0,0xf0, ay8910_1_ctrl_w},
  {0xf1,0xf1, ay8910_1_data_w},
MEMORY_END

static MACHINE_INIT(IDSA) {
}

static MACHINE_RESET(IDSA) {
  memset(&locals, 0, sizeof locals);
  sp0256_reset();
}

MACHINE_DRIVER_START(idsa)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, IDSA_CPUFREQ)
  MDRV_CPU_MEMORY(IDSA_readmem, IDSA_writemem)
  MDRV_CPU_PORTS(IDSA_readport, IDSA_writeport)
  MDRV_CPU_VBLANK_INT(IDSA_vblank, 1)
  MDRV_CPU_PERIODIC_INT(IDSA_irq, IDSA_CPUFREQ / 4096)
  MDRV_TIMER_ADD(IDSA_nmi, 0)
  MDRV_CORE_INIT_RESET_STOP(IDSA,IDSA,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(IDSA)
  MDRV_SOUND_ADD(AY8910, IDSA_ay8910Int)
  MDRV_SOUND_ADD(SP0256, IDSA_sp0256Int)
MACHINE_DRIVER_END

INPUT_PORTS_START(idsa)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT     (0x0040, "Start",  KEYCODE_1)
    COREPORT_BIT     (0x0020, "Coin 1", KEYCODE_3)
    COREPORT_BIT     (0x0010, "Coin 2", KEYCODE_4)
    COREPORT_BIT     (0x0008, "Coin 3", KEYCODE_5)
    COREPORT_BIT     (0x0200, "Tilt",   KEYCODE_INSERT)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0001, "1" )
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0002, "1" )
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0004, "1" )
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0008, "1" )
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0010, "1" )
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0020, "1" )
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0040, "1" )
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0080, "1" )
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0100, "1" )
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0200, "1" )
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0400, "1" )
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0800, "1" )
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x1000, "1" )
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x2000, "1" )
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x4000, "1" )
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

core_tLCDLayout idsa_disp[] = {
  {0, 0, 0, 7, CORE_SEG7}, {0,16, 7, 7, CORE_SEG7},
  {3, 0,16, 7, CORE_SEG7}, {3,16,23, 7, CORE_SEG7},
  {6, 6,36, 1, CORE_SEG7}, {6,10,38, 2, CORE_SEG7}, {6,16,41, 1, CORE_SEG7}, {6,20,43, 2, CORE_SEG7},
  {0}
};

static core_tGameData v1GameData = {0,idsa_disp,{FLIP_SWNO(28,30)}};
static void init_v1(void) {
  core_gameData = &v1GameData;
}
#define input_ports_v1 input_ports_idsa
ROM_START(v1)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("v1.128", 0x0000, 0x4000, CRC(4e08f7bc) SHA1(eb6ef00e489888dd9c53010c525840de06bcd0f3))
  NORMALREGION(0x10000, REGION_SOUND1)
  ROM_LOAD("v1.128", 0x0000, 0x4000, CRC(4e08f7bc) SHA1(eb6ef00e489888dd9c53010c525840de06bcd0f3))
  ROM_RELOAD(0x4000, 0x4000)
  ROM_RELOAD(0x8000, 0x4000)
  ROM_RELOAD(0xc000, 0x4000)
ROM_END
CORE_GAMEDEFNV(v1, "V-1", 198?, "IDSA (Spain)", idsa, GAME_NOT_WORKING)

static core_tGameData bsktballGameData = {0, idsa_disp,{FLIP_SWNO(28,30)}};
static void init_bsktball(void) {
  core_gameData = &bsktballGameData;
}
#define input_ports_bsktball input_ports_idsa
ROM_START(bsktball)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("bsktball.256", 0x0000, 0x8000, CRC(d474e29b) SHA1(750cbacef34dde0b3dcb6c1e4679db78a73643fd))
  NORMALREGION(0x10000, REGION_SOUND1)
  ROM_LOAD("bsktball.256", 0x0000, 0x8000, CRC(d474e29b) SHA1(750cbacef34dde0b3dcb6c1e4679db78a73643fd))
  ROM_RELOAD(0x8000, 0x8000)
ROM_END
CORE_GAMEDEFNV(bsktball, "Basket Ball", 1987, "IDSA (Spain)", idsa, GAME_NOT_WORKING)
