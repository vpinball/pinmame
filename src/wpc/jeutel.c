/******************************************************************************************
  Jeutel Games
  ------------
  by G. Volkenborn, 03/03/2011

  Jeutel is using two Z80 CPUs, and they are interacting heavily
  with lots of bus signals. I doubt it can be fully handled by MAME
  but it's close enough to make the games work as it is now;
  only thing I can say for sure is they're running way too fast!
  For sound this is using a TMS5110A speak n spell
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
  int dispBlank;
  int dipStrobe;
} locals;

static INTERRUPT_GEN(jeutel_vblank) {
  core_updateSw(core_getSol(16));
}

static INTERRUPT_GEN(jeutel_irq) {
  static int irq = 0;
  irq = !irq;
  cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static void jeutel_nmi(int data) {
  if (cpunum_get_reg(0, Z80_HALT)) { // only fire the NMI when CPU is halted!
    cpu_set_nmi_line(0, PULSE_LINE);
  }
}

static UINT8 *shared_RAM;
static WRITE_HANDLER(shared_ram_w) {
  shared_RAM[offset] = data;
}
static READ_HANDLER(shared_ram_r) {
  return shared_RAM[offset];
}

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

static MEMORY_READ_START(cpu_readmem2)
  {0x0000, 0x1fff, MRA_ROM},
  {0xc000, 0xc3ff, shared_ram_r},
  {0xc400, 0xc7ff, MRA_RAM},
  {0xe000, 0xe003, ppi8255_2_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem2)
  {0xc000, 0xc3ff, shared_ram_w, &shared_RAM},
  {0xc400, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xe000, 0xe003, ppi8255_2_w},
MEMORY_END

// display row & data
static WRITE_HANDLER(ppi0_porta_w) {
  UINT16 digit;
  locals.dispBlank = data & 0x80 ? 0 : 1;
  if (data & 0x40) {
    coreGlobals.segments[32 + locals.dispStrobe].w = locals.dispBlank ? 0 : core_bcd2seg7[data & 0x0f];
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
  if (data & 0x10) {
    coreGlobals.segments[locals.dispStrobe].w = locals.dispBlank ? 0 : digit;
  } else if (data & 0x20) {
    coreGlobals.segments[16 + locals.dispStrobe].w = locals.dispBlank ? 0 : digit;
  }
}
// lamp & display strobe
static WRITE_HANDLER(ppi0_portb_w) {
  locals.lampStrobe = data >> 4;
  locals.dispStrobe = 15 - (data & 0x0f);
  if (locals.lampStrobe < 8) {
    coreGlobals.lampMatrix[locals.lampStrobe] = locals.lampData;
  } else if (locals.lampStrobe == 8) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xfff00) | locals.lampData;
  } else if (locals.lampStrobe == 9) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xf00ff) | (locals.lampData << 8);
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
static WRITE_HANDLER(ppi1_portc_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x0ffff) | ((data & 0x07) << 16);
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x04) | ((data & 0x30) >> 4);
}

// dip strobe, sound control
static WRITE_HANDLER(ppi2_porta_w) {
  locals.dipStrobe = ~data & 0x0f;
  sndbrd_ctrl_w(0, data >> 6);
}
// dip return
static READ_HANDLER(ppi2_portb_r) {
  return ~core_getDip(core_BitColToNum(locals.dipStrobe));
}
// sound command
static WRITE_HANDLER(ppi2_portc_w) {
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
  UINT8 tmsLatch;
  UINT16 tmsAddr;
  int tmsBit;
} sndlocals;

static void jeutel_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0x00, sizeof(sndlocals));
  tms5110_reset();
  AY8910_reset(0);
}

static WRITE_HANDLER(jeutel_data_w) {
  sndlocals.sndCmd = data;
}

static WRITE_HANDLER(jeutel_ctrl_w) {
  if (!(data & 0x01)) {
    logerror(" CPU RESET\n");
    cpuint_reset_cpu(2);
    return;
  }
  if (data & 0x02) logerror(" CPU NMI\n");
  cpu_set_nmi_line(2, data & 0x02 ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(jeutel_manCmd_w) {
  jeutel_data_w(0, data);
  jeutel_ctrl_w(0, 3);
  jeutel_ctrl_w(0, 1);
}

static WRITE_HANDLER(m8000_w) {
  sndlocals.tmsLatch = data;
}

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x0fff, MRA_ROM},
  {0x4000, 0x43ff, MRA_RAM},
  {0x6000, 0x7fff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  {0x4000, 0x43ff, MWA_RAM},
  {0x8000, 0x8000, m8000_w},
MEMORY_END

static PORT_READ_START(snd_readport)
  {4, 4, AY8910_read_port_0_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0, 0, AY8910_control_port_0_w},
  {1, 1, AY8910_write_port_0_w},
PORT_END

/*
 * Bits 0 - 7: NC, NC, J6, LDH, CLR/LDL, CCLK, C1, C0
 */
static WRITE_HANDLER(ay8910_porta_w) {
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x03) | (data & 0x04);
  logerror("8910 port A: %c%c%c%c%c%cxx\n", data & 0x80 ? '0' : ' ', data & 0x40 ? '1' : ' ', data & 0x20 ? 'K' : ' ', data & 0x10 ? ' ' : 'L', data & 0x08 ? ' ' : 'H', data & 0x04 ? 'J' : ' ');
  if (~data & 0x10) { // load lower byte & reset bit counter
    sndlocals.tmsBit = 0;
    sndlocals.tmsAddr = (sndlocals.tmsAddr & 0xff00) | sndlocals.tmsLatch;
  }
  if (~data & 0x08) { // load upper byte
    sndlocals.tmsAddr = (sndlocals.tmsAddr & 0x00ff) | (sndlocals.tmsLatch << 8);
    logerror(" TMS offset:   %04x\n", sndlocals.tmsAddr);
  }
  if ((data & 0xf0) == 0xf0) {
    tms5110_CTL_w(0, TMS5110_CMD_RESET);
    tms5110_PDC_w(0, 1);
    tms5110_PDC_w(0, 0);
  }
  if ((data & 0xf0) == 0xd0) {
    tms5110_CTL_w(0, TMS5110_CMD_SPEAK);
    tms5110_PDC_w(0, 1);
    tms5110_PDC_w(0, 0);
  }
}
static READ_HANDLER(ay8910_portb_r) {
  logerror("8910 port B read: %02x\n", sndlocals.sndCmd);
  return sndlocals.sndCmd;
}

static struct AY8910interface jeutel_8910Int = {
  1,
  2000000,
  { 20 },
  { NULL },
  { ay8910_portb_r },
  { ay8910_porta_w },
};

static void tms5110_irq(int data) {
  cpu_set_irq_line(2, 0, tms5110_status_r(0) ? ASSERT_LINE : CLEAR_LINE);
}

static int tms5110_callback(void) {
  int value;
  value = (memory_region(REGION_SOUND1)[sndlocals.tmsAddr] >> sndlocals.tmsBit) & 1;
  sndlocals.tmsBit++;
  if (sndlocals.tmsBit > 7) {
    sndlocals.tmsAddr++;
    sndlocals.tmsBit = 0;
  }
  return value;
}

static struct TMS5110interface jeutel_5110Int = {
  640000,			/* clock rate = 80 * output sample rate,     */
					/* usually 640000 for 8000 Hz sample rate or */
					/* usually 800000 for 10000 Hz sample rate.  */
  50,				/* volume */
  tms5110_irq,		/* IRQ callback function (not implemented!) */
  tms5110_callback	/* function to be called when chip requests another bit*/
};

const struct sndbrdIntf jeutelIntf = {
  "JEUTEL", jeutel_init, NULL, NULL, jeutel_manCmd_w, jeutel_data_w, NULL, jeutel_ctrl_w
};

MACHINE_DRIVER_START(jeutel)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpue", Z80, 2000000) // should be 4 MHz, yet games run way too fast then
  MDRV_CPU_MEMORY(cpu_readmem1, cpu_writemem1)

  MDRV_CPU_ADD_TAG("mcpum", Z80, 2000000) // should be 4 MHz, yet games run way too fast then
  MDRV_CPU_MEMORY(cpu_readmem2, cpu_writemem2)

  MDRV_INTERLEAVE(250)
  MDRV_CPU_VBLANK_INT(jeutel_vblank, 1)
  MDRV_CPU_PERIODIC_INT(jeutel_irq, 500)
  MDRV_TIMER_ADD(jeutel_nmi, 200) // this is not correct; the 2nd CPU should trigger this on BUSAK actually
  MDRV_CORE_INIT_RESET_STOP(JEUTEL,JEUTEL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(JEUTEL)
  MDRV_DIAGNOSTIC_LEDH(3)

  MDRV_CPU_ADD_TAG("scpu", Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_TIMER_ADD(tms5110_irq, 100) // needed so the chip can tell the CPU it's ready

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
    COREPORT_DIPNAME( 0x0040, 0x0040, "S1/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "S1/2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S1/5") \
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
    COREPORT_DIPNAME( 0x0800, 0x0800, "S3/6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S4/1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "S4/2") \
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
    COREPORT_DIPNAME( 0x4000, 0x4000, "S1/4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S1/7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "S1/8") \
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
  {0, 0,10,1,CORE_SEG10}, {0, 2,11,2,CORE_SEG9}, {0, 6,13,1,CORE_SEG10}, {0, 8,14,2,CORE_SEG9}, {0,12, 1,1,CORE_SEG9},
  {0,16, 2,1,CORE_SEG10}, {0,18, 3,2,CORE_SEG9}, {0,22, 5,1,CORE_SEG10}, {0,24, 6,2,CORE_SEG9}, {0,28, 9,1,CORE_SEG9},
  {5, 0,26,1,CORE_SEG10}, {5, 2,27,2,CORE_SEG9}, {5, 6,29,1,CORE_SEG10}, {5, 8,30,2,CORE_SEG9}, {5,12,17,1,CORE_SEG9},
  {5,16,18,1,CORE_SEG10}, {5,18,19,2,CORE_SEG9}, {5,22,21,1,CORE_SEG10}, {5,24,22,2,CORE_SEG9}, {5,28,25,1,CORE_SEG9},
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
    ROM_LOAD("game-v.bin", 0x0000, 0x1000, CRC(cbbc8b55) SHA1(4fe150fa3b565e5618896c0af9d51713b381ed88))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-m.bin", 0x0000, 0x2000, CRC(4b66517a) SHA1(1939ea78932d469a16441507bb90b032c5f77b1e))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-v.bin", 0x0000, 0x1000, CRC(36130e7b) SHA1(d9b66d43b55272579b3972005355b8a18ce6b4a9))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("sound-p.bin", 0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d) BAD_DUMP)
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME(leking,dispAlpha,FLIP_SW(FLIP_L))
JEUTEL_COMPORTS(leking, 3)
CORE_GAMEDEFNV(leking, "Le King", 1983, "Jeutel", jeutel, GAME_NOT_WORKING)

/*--------------------------------
/ Olympic Games
/-------------------------------*/
ROM_START(olympic)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-v.bin", 0x0000, 0x1000, CRC(cd284a20) SHA1(94568e1247994c802266f9fbe4a6f6ed2b55a978))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-jo1.bin", 0x0000, 0x2000, CRC(c9f040cf) SHA1(c689f3a82d904d3f9fc8688d4c06082c51645b2f))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-j0.bin", 0x0000, 0x1000, CRC(5c70ce72) SHA1(b0b6cc7b6ec3ed9944d738b61a0d144b77b07000))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("sound-p.bin", 0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d))
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME(olympic,dispAlpha,FLIP_SW(FLIP_L))
JEUTEL_COMPORTS(olympic, 1)
CORE_GAMEDEFNV(olympic, "Olympic Games", 1984, "Jeutel", jeutel, GAME_NOT_WORKING)
