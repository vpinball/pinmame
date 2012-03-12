/************************************************************************************************
 IDSA (Spain)
 ------------
   Hardware:
   ---------
		CPU:     Z80 @ 4 MHz
			INT: IRQ @ 977 Hz (4MHz/2048/2)
		IO:      DMA, AY8910 ports
		DISPLAY: 7-digit 7-segment panels with direct segment access, driven by 4094 serial controllers.
		SOUND:	 2 x AY8910 @ 2 MHz plus SP0256 @ 3.12 MHz on board
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/z80/z80.h"
#include "machine/4094.h"
#include "sound/ay8910.h"
#include "sound/sp0256.h"

#define IDSA_VBLANKFREQ   60 /* VBLANK frequency */
#define IDSA_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    swCol;
  UINT32 dispData;
} locals;

static INTERRUPT_GEN(IDSA_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

static INTERRUPT_GEN(IDSA_nmi) {
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
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static WRITE_HANDLER(disp_w) {
  data ^= 0xff;
  // top 5 bits: switch column strobes
  locals.swCol = core_BitColToNum(data >> 3);
  // bottom 3 bits: serial display data
  HC4094_data_w (0, GET_BIT0);
  HC4094_strobe_w(0, GET_BIT2);
  HC4094_strobe_w(1, GET_BIT2);
  HC4094_strobe_w(2, GET_BIT2);
  HC4094_strobe_w(3, GET_BIT2);
  HC4094_clock_w(0, GET_BIT1);
  HC4094_clock_w(1, GET_BIT1);
  HC4094_clock_w(2, GET_BIT1);
  HC4094_clock_w(3, GET_BIT1);
}

static WRITE_HANDLER(parallel_0_out) {
  locals.dispData = (locals.dispData & 0xffffff00) | data;
}
static WRITE_HANDLER(parallel_1_out) {
  locals.dispData = (locals.dispData & 0xffff00ff) | (data << 8);
}
static WRITE_HANDLER(parallel_2_out) {
  locals.dispData = (locals.dispData & 0xff00ffff) | (data << 16);
}
static WRITE_HANDLER(parallel_3_out) {
  locals.dispData = (locals.dispData & 0x00ffffff) | (data << 24);
}
static WRITE_HANDLER(qs1pin_0_out) {
  HC4094_data_w(1, data);
}
static WRITE_HANDLER(qs1pin_1_out) {
  HC4094_data_w(2, data);
}
static WRITE_HANDLER(qs1pin_2_out) {
  HC4094_data_w(3, data);
}

static HC4094interface hc4094idsa = {
  4, // 4 chips
  { parallel_0_out, parallel_1_out, parallel_2_out, parallel_3_out },
  { 0 },
  { qs1pin_0_out, qs1pin_1_out, qs1pin_2_out }
};

static WRITE_HANDLER(ay8910_0_ctrl_w) { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w) { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_0_r)      { return AY8910Read(0); }

static WRITE_HANDLER(ay8910_1_ctrl_w) { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w) { AY8910Write(1,1,data); }
static READ_HANDLER (ay8910_1_r)      { return AY8910Read(1); }

static WRITE_HANDLER(ay8910_0_portA_w) {
  coreGlobals.lampMatrix[0] = data;
}
static WRITE_HANDLER(ay8910_0_portB_w) {
  coreGlobals.lampMatrix[1] = data;
}
static WRITE_HANDLER(ay8910_1_portA_w) {
  coreGlobals.lampMatrix[2] = data;
}
static WRITE_HANDLER(ay8910_1_portB_w) {
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

struct sp0256_interface IDSA_sp0256Int = {
  50, /* volume */
  3120000 /* clock */
};

static MEMORY_READ_START(IDSA_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0x8000,0x87ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(IDSA_writemem)
  {0x0000,0x7fff, MWA_NOP},
  {0x8000,0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(IDSA_readport)
  {0xb0,0xb0, ay8910_0_r},
  {0xb1,0xb1, ay8910_1_r},
MEMORY_END

static PORT_WRITE_START(IDSA_writeport)
  {0xe0,0xe0, ay8910_0_ctrl_w},
  {0xe1,0xe1, ay8910_0_data_w},
  {0xf0,0xf0, ay8910_1_ctrl_w},
  {0xf1,0xf1, ay8910_1_data_w},
MEMORY_END

static MACHINE_INIT(IDSA) {
  memset(&locals, 0, sizeof locals);
  HC4094_init(&hc4094idsa);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_oe_w(2, 1);
  HC4094_oe_w(3, 1);
}

static MACHINE_RESET(IDSA) {
  sp0256_reset();
}

MACHINE_DRIVER_START(idsa)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, IDSA_CPUFREQ)
  MDRV_CPU_MEMORY(IDSA_readmem, IDSA_writemem)
  MDRV_CPU_PORTS(IDSA_readport, IDSA_writeport)
  MDRV_CPU_VBLANK_INT(IDSA_vblank, 1)
  MDRV_CPU_PERIODIC_INT(IDSA_irq, IDSA_CPUFREQ / 4096)
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
    COREPORT_BIT     (0x0001, "Reset", KEYCODE_0)
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
  {0, 0, 0,16, CORE_SEG7},
  {3, 0,16,16, CORE_SEG7},
  {0}
};

static core_tGameData v1GameData = {0, idsa_disp};
static void init_v1(void) {
  core_gameData = &v1GameData;
}
#define input_ports_v1 input_ports_idsa
ROM_START(v1)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("v1.128", 0x0000, 0x4000, CRC(4e08f7bc) SHA1(eb6ef00e489888dd9c53010c525840de06bcd0f3))
ROM_END
CORE_GAMEDEFNV(v1, "V-1", 198?, "IDSA (Spain)", idsa, GAME_NOT_WORKING)

static core_tGameData bsktballGameData = {0, idsa_disp};
static void init_bsktball(void) {
  core_gameData = &bsktballGameData;
}
#define input_ports_bsktball input_ports_idsa
ROM_START(bsktball)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("bsktball.256", 0x0000, 0x8000, CRC(d474e29b) SHA1(750cbacef34dde0b3dcb6c1e4679db78a73643fd))
ROM_END
CORE_GAMEDEFNV(bsktball, "Basket Ball", 1987, "IDSA (Spain)", idsa, GAME_NOT_WORKING)
