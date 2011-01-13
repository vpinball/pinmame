/******************************************************************************************
  Jeutel Games
  ------------
*******************************************************************************************/
#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "cpu/z80/z80.h"
#include "sound/tms5110.h"
#include "machine/8255ppi.h"

static struct {
  UINT8 lampData;
  UINT8 swStrobe;
  int lampStrobe;
  int dispStrobe;
  int dipStrobe;
} locals;

static INTERRUPT_GEN(jeutel_vblank) {
  core_updateSw(core_getSol(16));
}

static INTERRUPT_GEN(jeutel_irq) {
  static int irq = 0;
  irq = !irq;
  cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static void jeutel_nmi(int data) {
  if (cpunum_get_reg(1, Z80_HALT)) { // only fire the NMI when CPU #1 is halted!
//    printf(".");
    cpu_set_nmi_line(1, PULSE_LINE);
  }
}

static UINT8 *shared_RAM;
static WRITE_HANDLER(shared_ram_w) {
  shared_RAM[offset] = data;
}
static READ_HANDLER(shared_ram_r) {
  return shared_RAM[offset];
}

static MEMORY_READ_START(cpu_readmem0)
  {0x0000, 0x1fff, MRA_ROM},
  {0xc000, 0xc3ff, shared_ram_r},
  {0xc400, 0xc7ff, MRA_RAM},
  {0xe000, 0xe003, ppi8255_2_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem0)
  {0xc000, 0xc3ff, shared_ram_w, &shared_RAM},
  {0xc400, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xe000, 0xe003, ppi8255_2_w},
MEMORY_END

static WRITE_HANDLER(m4000_w) {
  logerror("m4000_w: %02x\n", data);
}

static MEMORY_READ_START(cpu_readmem1)
  {0x0000, 0x0fff, MRA_ROM},
  {0x2000, 0x2003, ppi8255_0_r},
  {0x3000, 0x3003, ppi8255_1_r},
  {0x8000, 0x83ff, MRA_RAM},
  {0xc000, 0xc3ff, shared_ram_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem1)
  {0x2000, 0x2003, ppi8255_0_w},
  {0x3000, 0x3003, ppi8255_1_w},
  {0x4000, 0x4000, m4000_w},
  {0x8000, 0x83ff, MWA_RAM},
  {0xc000, 0xc3ff, shared_ram_w},
MEMORY_END

// display row & data
static WRITE_HANDLER(ppi0_porta_w) {
  UINT16 digit;
  if ((data & 0x70) == 0x40) {
    coreGlobals.segments[32 + locals.dispStrobe].w = core_bcd2seg7[data & 0x0f];
    return;
  }
  switch (data & 0x0f) {
    case 0x0a: // letter T
      digit = 0x301; break;
    case 0x0b: // letter E
      digit = 0x79; break;
    case 0x0c: // letter L
      digit = 0x38; break;
    case 0x0d: // letter U
      digit = 0x3e; break;
    case 0x0e: // letter J
      digit = 0x1e; break;
    default:
      digit = core_bcd2seg9[data & 0x0f];
  }
  if ((data & 0x70) == 0x10) {
    coreGlobals.segments[locals.dispStrobe].w = digit;
  } else if ((data & 0x70) == 0x20) {
    coreGlobals.segments[16 + locals.dispStrobe].w = digit;
  }
}
// lamp & display strobe
static WRITE_HANDLER(ppi0_portb_w) {
  locals.lampStrobe = data >> 4;
  locals.dispStrobe = 15 - (data & 0x0f);
  if (locals.lampStrobe < 8) {
    coreGlobals.lampMatrix[locals.lampStrobe] = locals.lampData;
  } else if (locals.lampStrobe == 8) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xff00) | locals.lampData;
  } else if (locals.lampStrobe == 9) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0x00ff) | (locals.lampData << 8);
  }
}
// lamp data
static WRITE_HANDLER(ppi0_portc_w) {
  locals.lampData = data;
}

// switch strobe
static WRITE_HANDLER(ppi1_porta_w) {
  locals.swStrobe = ~data;
}
// switch returns
static READ_HANDLER(ppi1_portb_r) {
  return ~coreGlobals.swMatrix[1 + core_BitColToNum(locals.swStrobe)];
}
static WRITE_HANDLER(ppi1_portb_w) { logerror("8255 #1 port B: %02x\n", data); }
static WRITE_HANDLER(ppi1_portc_w) { logerror("8255 #1 port C: %02x\n", data); }

// dip strobe
static WRITE_HANDLER(ppi2_porta_w) {
  locals.dipStrobe = ~data & 0x0f;
}
// dip return
static READ_HANDLER(ppi2_portb_r) {
  return ~core_getDip(core_BitColToNum(locals.dipStrobe));
}
// sound command
static WRITE_HANDLER(ppi2_portc_w) {
  logerror("snd cmd: %02x\n", data);
  sndbrd_data_w(0, data);
}

static ppi8255_interface ppi8255_intf =
{
  3, /* 3 chips in total used by the 2 main CPUs. */
  {0, 0, 0},	/* Port A read */
  {0, ppi1_portb_r, ppi2_portb_r},	/* Port B read */
  {0, 0, 0},	/* Port C read */
  {ppi0_porta_w, ppi1_porta_w, ppi2_porta_w},	/* Port A write */
  {ppi0_portb_w, ppi1_portb_w, 0},	/* Port B write */
  {ppi0_portc_w, ppi1_portc_w, ppi2_portc_w},	/* Port C write */
};

static MACHINE_INIT(JEUTEL) {
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(REGION_CPU3), NULL, NULL);
}

static MACHINE_RESET(JEUTEL) {
  memset(&locals, 0x00, sizeof(locals));
	ppi8255_init(&ppi8255_intf);
}

static SWITCH_UPDATE(JEUTEL) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x3f, 5);
  }
}


// sound section

static struct {
  UINT8 sndCmd;
} sndlocals;

static void jeutel_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0x00, sizeof(sndlocals));
}

static WRITE_HANDLER(jeutel_data_w) {
  sndlocals.sndCmd = data;
  cpu_set_nmi_line(2, PULSE_LINE);
}

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x2fff, MRA_ROM},
  {0x4000, 0x43ff, MRA_RAM},
MEMORY_END

static WRITE_HANDLER(m8000_w) {
  logerror("m8000_w: %02x\n", data);
  tms5110_CTL_w(0, data >> 4);
  tms5110_PDC_w(0, 1);
  tms5110_PDC_w(0, 0);
}

static MEMORY_WRITE_START(snd_writemem)
  {0x4000, 0x43ff, MWA_RAM},
  {0x8000, 0x8000, m8000_w},
MEMORY_END

static READ_HANDLER(snd_cmd_r) {
  return sndlocals.sndCmd;
}

static PORT_READ_START(snd_readport)
  {0, 0, AY8910_read_port_0_r},
  {4, 4, snd_cmd_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0, 0, AY8910_control_port_0_w},
  {1, 1, AY8910_write_port_0_w},
PORT_END

static WRITE_HANDLER(ay8910_porta_w) { logerror("8910 port A: %02x\n", data); }
static WRITE_HANDLER(ay8910_portb_w) { logerror("8910 port B: %02x\n", data); }

static struct AY8910interface jeutel_8910Int = {
  1,
  3333333/2,
  { 30 },
  { 0 },
  { 0 },
  { ay8910_porta_w },
  { ay8910_portb_w },
};

static void tms5110_irq(int data) {
  logerror("5110 irq: %d\n", data);
  cpu_set_irq_line(2, 0, data ? ASSERT_LINE : CLEAR_LINE);
}

static int tms5110_callback(void) {
  logerror("5110 callback\n");
  return 0;
}

static struct TMS5110interface jeutel_5110Int = {
  3300000/4,				/* clock rate = 80 * output sample rate,     */
								/* usually 640000 for 8000 Hz sample rate or */
								/* usually 800000 for 10000 Hz sample rate.  */
  100,					/* volume */
  tms5110_irq,		/* IRQ callback function */
  tms5110_callback	/* function to be called when chip requests another bit*/
};

const struct sndbrdIntf jeutelIntf = {
  "JEUTEL", jeutel_init, NULL, NULL, jeutel_data_w, jeutel_data_w, NULL, jeutel_data_w
};

MACHINE_DRIVER_START(jeutel)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu0", Z80, 3300000)
  MDRV_CPU_MEMORY(cpu_readmem0, cpu_writemem0)

  MDRV_CPU_ADD_TAG("mcpu1", Z80, 3300000)
  MDRV_CPU_MEMORY(cpu_readmem1, cpu_writemem1)

  MDRV_INTERLEAVE(250)
  MDRV_CPU_VBLANK_INT(jeutel_vblank, 1)
  MDRV_CPU_PERIODIC_INT(jeutel_irq, 100)
  MDRV_TIMER_ADD(jeutel_nmi, 250)
  MDRV_CORE_INIT_RESET_STOP(JEUTEL,JEUTEL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(JEUTEL)
  MDRV_DIAGNOSTIC_LEDH(1)

  MDRV_CPU_ADD_TAG("scpu", Z80, 3300000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)

  MDRV_SOUND_ADD(AY8910, jeutel_8910Int)
  MDRV_SOUND_ADD(TMS5110, jeutel_5110Int)
MACHINE_DRIVER_END


// games below

#define INITGAME(name, disp, flip) \
static core_tGameData name##GameData = {0,disp,{flip,0,0,0,SNDBRD_JEUTEL}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define JEUTEL_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(  0x0002, IPT_START1, IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,  IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,  KEYCODE_3) \
    COREPORT_BITDEF(  0x0010, IPT_COIN3,  KEYCODE_4) \
    COREPORT_BITDEF(  0x0001, IPT_TILT,   KEYCODE_INSERT) \
    COREPORT_BIT(     0x0020, "Test",     KEYCODE_7) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S1/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S1/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S1/5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S1/6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S2/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S2/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S2/5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S2/6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S3/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S3/5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S3/6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S4/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S4/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S4/5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S4/6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S1/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S1/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S1/7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S1/8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S2/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S2/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S2/7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S2/8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S3/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S3/7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S3/8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S4/3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S4/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S4/7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S4/8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
  INPUT_PORTS_END

static core_tLCDLayout dispAlpha[] = {
  {0, 0,10,6,CORE_SEG10}, {0,12, 1,1,CORE_SEG10}, {0,16, 2,6,CORE_SEG10}, {0,28, 9,1,CORE_SEG10},
  {5, 0,26,6,CORE_SEG10}, {5,12,17,1,CORE_SEG10}, {5,16,18,6,CORE_SEG10}, {5,28,25,1,CORE_SEG10},
  {3,27,37,2,CORE_SEG7S}, {3,33,39,1,CORE_SEG7S}, {3,35,41,1,CORE_SEG7S},
#ifdef MAME_DEBUG
  {3,25,36,1,CORE_SEG7S}, // 100's credits is actually stored & displayable!
#endif
  {0}
};

/*--------------------------------
/ Le King
/-------------------------------*/
ROM_START(leking)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-m.bin", 0x0000, 0x2000, CRC(4b66517a) SHA1(1939ea78932d469a16441507bb90b032c5f77b1e))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-v.bin", 0x0000, 0x1000, CRC(cbbc8b55) SHA1(4fe150fa3b565e5618896c0af9d51713b381ed88))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-v.bin", 0x0000, 0x1000, CRC(36130e7b) SHA1(d9b66d43b55272579b3972005355b8a18ce6b4a9))
    ROM_LOAD("sound-p.bin", 0x1000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d))
ROM_END

INITGAME(leking,dispAlpha,FLIP_SW(FLIP_L))
JEUTEL_COMPORTS(leking, 3)
CORE_GAMEDEFNV(leking, "Le King", 1983, "Jeutel", jeutel, GAME_NOT_WORKING)

/*--------------------------------
/ Olympic Games
/-------------------------------*/
ROM_START(olympic)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-jo1.bin", 0x0000, 0x2000, CRC(c9f040cf) SHA1(c689f3a82d904d3f9fc8688d4c06082c51645b2f))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-v.bin", 0x0000, 0x1000, CRC(cd284a20) SHA1(94568e1247994c802266f9fbe4a6f6ed2b55a978))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-j0.bin", 0x0000, 0x1000, CRC(5c70ce72) SHA1(b0b6cc7b6ec3ed9944d738b61a0d144b77b07000))
    ROM_LOAD("sound-p.bin", 0x1000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d))
ROM_END

INITGAME(olympic,dispAlpha,FLIP_SW(FLIP_L))
JEUTEL_COMPORTS(olympic, 1)
CORE_GAMEDEFNV(olympic, "Olympic Games", 1984, "Jeutel", jeutel, GAME_NOT_WORKING)
