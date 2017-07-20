/************************************************************************************************
 IDSA (Spain)
 ------------
   Hardware:
   ---------
		CPU:     Z80 @ 4 MHz
			INT: IRQ @ 977 Hz (4MHz/2048/2) or 488 Hz (4MHz/2048/4)
		IO:      DMA, AY8910 ports
		DISPLAY: bsktball: 7-digit 7-segment panels with PROM-based 5-bit BCD data (allowing a simple alphabet)
		         v1: 6-digit 7-segment panels with BCD decoding
		SOUND:	 2 x AY8910 @ 2 MHz plus SP0256 @ 3.12 MHz on board
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/z80/z80.h"

#define IDSA_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT16 dispData[4];
  int dispCol;
  int dispRow;
  core_tSeg segments;
  int isV1, lrq;
} locals;

static INTERRUPT_GEN(IDSA_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(IDSA_vblank) {
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
//  memset(locals.segments, 0, sizeof(locals.segments));
  core_updateSw(core_getSol(10));
}

static SWITCH_UPDATE(IDSA) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 12, 0x01, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x78, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x02, locals.isV1 ? 2 : 3);
  }
}

// IDSA used small 32x8 6331 color PROMs to decode their 5-bit alphabet; I'd love to see this dumped someday! ;)
// Strings used by game: "Error (123456)", "P(Abc)(12)-F(01)", "FALtA", "1d5A", "Fin"
static UINT16 idsa2seg7(UINT8 data) {
  switch (data & 0x1f) {
    case 0x0a: return 0x77; // A !
    case 0x0b: return 0x7c; // b !
    case 0x0c: return 0x58; // c !
    case 0x0d: return 0x5e; // d !
    case 0x0e: return 0x79; // E !
    case 0x0f: return 0x71; // F !
    case 0x10: return 0x3d; // G ?
    case 0x11: return 0x76; // H ?
    case 0x12: return 0x10; // i !
    case 0x13: return 0x1e; // J / k ?
    case 0x14: return 0x38; // L !
    case 0x15: return 0x56; // m ? pretty much impossible with 7 segs...
    case 0x16: return 0x54; // n !
    case 0x17: return 0x55; // n with tilde ?
    case 0x18: return 0x5c; // o !
    case 0x19: return 0x73; // P !
    case 0x1a: return 0x50; // r !
    case 0x1b: return 0x40; // - !
    case 0x1c: return 0x78; // t !
    case 0x1d: return 0x1c; // u / v / w / x ?
    case 0x1e: return 0x6e; // y ?
    case 0x1f: return 0;
    default: return core_bcd2seg7[data & 0x0f];
  }
}

static WRITE_HANDLER(ay8910_0_portA_w) {
  if (locals.isV1) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xfffffcff) | ((~data & 3) << 8);
    coreGlobals.lampMatrix[0] = ~data;
  } else
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xffffff00) | (data ^ 0xff);
}
static WRITE_HANDLER(ay8910_0_portB_w) {
  if (locals.isV1)
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xffffff00) | (data ^ 0xff);
  else
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xff00ffff) | ((data ^ 0xff) << 16);
}
static WRITE_HANDLER(ay8910_1_portA_w) {
  if (locals.isV1) {
    coreGlobals.lampMatrix[3] = ~data;
    return;
  }
  if ((data >> 4) != 0x0f) {
    locals.dispCol = data >> 4;
  } else {
    locals.dispRow = 0;
    locals.segments[32 + locals.dispCol].w = idsa2seg7(locals.dispData[0]);
    locals.segments[16 + locals.dispCol].w = idsa2seg7(locals.dispData[1]);
    locals.segments[locals.dispCol].w = idsa2seg7(locals.dispData[2]);
    locals.segments[48].w = idsa2seg7(locals.dispData[3] >> 4);
    locals.segments[49].w = idsa2seg7(locals.dispData[3] & 0x0f);
  }
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x00ffffff) | (((data & 0x0f) ^ 0x0f) << 24);
}
static WRITE_HANDLER(ay8910_1_portB_w) {
  if (locals.isV1)
    coreGlobals.lampMatrix[1] = ~data;
  else
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xffff00ff) | ((data ^ 0xff) << 8);
}
struct AY8910interface IDSA_ay8910Int = {
	2,					/* 2 chips */
	2000000,			/* 2 MHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },	/* Volume */
	{ NULL },
	{ NULL },
	{ ay8910_0_portA_w, ay8910_1_portA_w },	/* Output Port A callback */
	{ ay8910_0_portB_w, ay8910_1_portB_w },	/* Output Port B callback */
};

static READ_HANDLER(snd_status_r) {
  switch (offset) {
    case 1: return locals.lrq ? 0xff : 0;
    default: return 0;
  }
}
static void lrq_callback(int state) {
	locals.lrq = !state;
}
struct sp0256_interface IDSA_sp0256Int = {
  100, /* volume */
  3120000, /* clock */
  lrq_callback,
  NULL,
  REGION_SOUND1
};

static READ_HANDLER(sw_r) {
  return ~coreGlobals.swMatrix[1 + (offset >> 4)];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(offset >> 4);
}

static READ_HANDLER(port_bd_r) {
  return 0x01 ^ coreGlobals.swMatrix[0];
}

static WRITE_HANDLER(row_w) {
  locals.dispRow++;
}

static WRITE_HANDLER(disp_w) {
  if (locals.dispRow < 4)
    locals.dispData[locals.dispRow] = data;
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *IDSA_CMOS;
static NVRAM_HANDLER(IDSA) {
  core_nvram(file, read_or_write, IDSA_CMOS, 0x800, 0x00);
}
static WRITE_HANDLER(IDSA_CMOS_w) {
  UINT16 val16;
  IDSA_CMOS[offset] = data;
  if (locals.isV1) {
    if (offset > 0x0c && offset < 0x27)
      locals.segments[0x26 - offset].w = core_bcd2seg7e[data & 0x0f];
    else if (offset == 0x2e) { // lottery / space ship lamps
      val16 = 1 << (data & 0x0f);
      coreGlobals.lampMatrix[8] = val16 & 0xff;
      coreGlobals.lampMatrix[9] = val16 >> 8;
      val16 = 1 << (data >> 4);
      coreGlobals.lampMatrix[10] = val16 & 0xff;
      coreGlobals.lampMatrix[11] = val16 >> 8;
    } else if (offset == 0x2f) // ball in play / game over lamp!
      coreGlobals.lampMatrix[2] = data & 0xe0 ? data & 0xe0 : 1 << (data & 7);
    else if (offset == 0x30) { // bonus lamps
      val16 = 1 << (data & 0x0f);
      coreGlobals.lampMatrix[4] = val16 & 0xff;
      coreGlobals.lampMatrix[5] = val16 >> 8;
      val16 = 1 << (data >> 4);
      coreGlobals.lampMatrix[6] = val16 & 0xff;
      coreGlobals.lampMatrix[7] = val16 >> 8;
    } else if (offset > 0x81 && offset < 0x86) { // player up
      if (locals.segments[6 * (3-(offset - 0x82)) + 6].w)
        locals.segments[6 * (3-(offset - 0x82)) + 7].w = core_bcd2seg7e[data & 0x0f];
    } else if (offset == 0x107) { // game over does not update after game over
      if (data & 0xe0)
        coreGlobals.lampMatrix[2] = data & 0xe0;
    }
  } else if (offset > 0xa7 && offset < 0xb2)
    coreGlobals.lampMatrix[offset - 0xa8] = data;
}
static READ_HANDLER(IDSA_CMOS_r) {
  return IDSA_CMOS[offset];
}

static MEMORY_READ_START(IDSA_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0x8000,0x87ff, IDSA_CMOS_r},
MEMORY_END

static MEMORY_WRITE_START(IDSA_writemem)
  {0x0000,0x7fff, MWA_NOP},
  {0x8000,0x87ff, IDSA_CMOS_w, &IDSA_CMOS},
MEMORY_END

static PORT_READ_START(IDSA_readport)
  {0x00,0x50, sw_r},
  {0x60,0x70, dip_r},
  {0xb0,0xb3, snd_status_r},
  {0xbd,0xbd, port_bd_r},
MEMORY_END

static PORT_WRITE_START(IDSA_writeport)
  {0x80,0x8f, row_w},
  {0x90,0x90, disp_w},
  {0xd0,0xd0, sp0256_ALD_w},
  {0xe0,0xe0, AY8910_control_port_0_w},
  {0xe1,0xe1, AY8910_write_port_0_w},
  {0xf0,0xf0, AY8910_control_port_1_w},
  {0xf1,0xf1, AY8910_write_port_1_w},
MEMORY_END

static void reset_common(void) {
  static int inverted;
  memset(&locals, 0, sizeof locals);
  sp0256_reset();
  if (!inverted) {
    sp0256_bitrevbuff(memory_region(REGION_SOUND1), 0, 0x10000);
    inverted = 1;
  }
}

static MACHINE_RESET(IDSA) {
  reset_common();
}

MACHINE_DRIVER_START(idsa)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, IDSA_CPUFREQ)
  MDRV_CPU_MEMORY(IDSA_readmem, IDSA_writemem)
  MDRV_CPU_PORTS(IDSA_readport, IDSA_writeport)
  MDRV_CPU_VBLANK_INT(IDSA_vblank, 1)
  MDRV_CPU_PERIODIC_INT(IDSA_irq, IDSA_CPUFREQ / 4096)
  MDRV_CORE_INIT_RESET_STOP(NULL,IDSA,NULL)
  MDRV_NVRAM_HANDLER(IDSA)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(IDSA)
  MDRV_SOUND_ADD(AY8910, IDSA_ay8910Int)
  MDRV_SOUND_ADD(SP0256, IDSA_sp0256Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

// we need to "reset" the CPU manually as game reset calls the NMI routine and halts the CPU
static void v1_reset_timer(int dummy) {
  cpunum_set_reg(0, Z80_R, 0);
  cpunum_set_reg(0, Z80_HALT, 0);
  cpunum_set_reg(0, Z80_AF, 0x0040);
  cpunum_set_reg(0, Z80_SP, 0);
  cpunum_set_reg(0, Z80_PC, 0);
}

static MACHINE_RESET(v1) {
  reset_common();
  locals.isV1 = 1;
  // NMI routine saves the credits to NVRAM (called upon power down on real machine)
  cpu_set_nmi_line(0, PULSE_LINE);
  timer_adjust(timer_alloc(v1_reset_timer), TIME_IN_MSEC(1), 0, TIME_NEVER);
}

MACHINE_DRIVER_START(v1)
  MDRV_IMPORT_FROM(idsa)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(IDSA_irq, IDSA_CPUFREQ / 8192)
  MDRV_CORE_INIT_RESET_STOP(NULL,v1,NULL)
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
    COREPORT_BIT     (0x1000, "Slam Tilt", KEYCODE_HOME)
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

core_tLCDLayout v1_disp[] = {
  {0, 0,20, 6, CORE_SEG7}, {0,20,14, 6, CORE_SEG7},
  {3, 0, 8, 6, CORE_SEG7}, {3,20, 2, 6, CORE_SEG7},
  {3,14, 0, 2, CORE_SEG7},
  {0}
};

#define SP0256_ROM \
  NORMALREGION(0x10000, REGION_SOUND1) \
  ROM_LOAD("sp0256-al2.bin", 0x0000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) BAD_DUMP) \
  ROM_RELOAD(0x0800, 0x0800) \
  ROM_RELOAD(0x1000, 0x0800) \
  ROM_RELOAD(0x1800, 0x0800) \
  ROM_RELOAD(0x2000, 0x0800) \
  ROM_RELOAD(0x2800, 0x0800) \
  ROM_RELOAD(0x3000, 0x0800) \
  ROM_RELOAD(0x3800, 0x0800) \
  ROM_RELOAD(0x4000, 0x0800) \
  ROM_RELOAD(0x4800, 0x0800) \
  ROM_RELOAD(0x5000, 0x0800) \
  ROM_RELOAD(0x5800, 0x0800) \
  ROM_RELOAD(0x6000, 0x0800) \
  ROM_RELOAD(0x6800, 0x0800) \
  ROM_RELOAD(0x7000, 0x0800) \
  ROM_RELOAD(0x7800, 0x0800) \
  ROM_RELOAD(0x8000, 0x0800) \
  ROM_RELOAD(0x8800, 0x0800) \
  ROM_RELOAD(0x9000, 0x0800) \
  ROM_RELOAD(0x9800, 0x0800) \
  ROM_RELOAD(0xa000, 0x0800) \
  ROM_RELOAD(0xa800, 0x0800) \
  ROM_RELOAD(0xb000, 0x0800) \
  ROM_RELOAD(0xb800, 0x0800) \
  ROM_RELOAD(0xc000, 0x0800) \
  ROM_RELOAD(0xc800, 0x0800) \
  ROM_RELOAD(0xd000, 0x0800) \
  ROM_RELOAD(0xd800, 0x0800) \
  ROM_RELOAD(0xe000, 0x0800) \
  ROM_RELOAD(0xe800, 0x0800) \
  ROM_RELOAD(0xf000, 0x0800) \
  ROM_RELOAD(0xf800, 0x0800)

static core_tGameData v1GameData = {0,v1_disp,{FLIP_SW(FLIP_L),0,4}};
static void init_v1(void) {
  core_gameData = &v1GameData;
}
#define input_ports_v1 input_ports_idsa
ROM_START(v1)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("v1.128", 0x0000, 0x4000, CRC(4e08f7bc) SHA1(eb6ef00e489888dd9c53010c525840de06bcd0f3))
  SP0256_ROM
ROM_END
CORE_GAMEDEFNV(v1, "V.1", 1985, "IDSA (Spain)", v1, 0)

core_tLCDLayout idsa_disp[] = {
  {0, 0, 0, 7, CORE_SEG7}, {0,16, 7, 7, CORE_SEG7},
  {3, 0,16, 7, CORE_SEG7}, {3,16,23, 7, CORE_SEG7},
  {6, 6,36, 1, CORE_SEG7}, {6,10,38, 2, CORE_SEG7}, {6,16,41, 1, CORE_SEG7}, {6,20,43, 2, CORE_SEG7},
  {9, 0,48, 2, CORE_SEG7},
  {0}
};

static core_tGameData bsktballGameData = {0, idsa_disp,{FLIP_SWNO(28,30),0,2}};
static void init_bsktball(void) {
  core_gameData = &bsktballGameData;
}
#define input_ports_bsktball input_ports_idsa
ROM_START(bsktball)
  NORMALREGION(0x10000, REGION_CPU1)
  ROM_LOAD("bsktball.256", 0x0000, 0x8000, CRC(d474e29b) SHA1(750cbacef34dde0b3dcb6c1e4679db78a73643fd))
  SP0256_ROM
ROM_END
CORE_GAMEDEFNV(bsktball, "Basketball", 1987, "IDSA (Spain)", idsa, 0)
