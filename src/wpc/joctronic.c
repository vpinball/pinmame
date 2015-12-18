/******************************************************************************************
  Joctronic games
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "sndbrd.h"

static struct {
  UINT8 snd;
  UINT8 cols[8];
  int dispNo, resetCounter;
} locals;

static INTERRUPT_GEN(joctronic_vblank) {
  core_updateSw(TRUE);
}

static void joctronic_nmi(int data) {
  z80ctc_0_trg3_w(0, 0);
  z80ctc_0_trg3_w(0, 1);

  if (locals.resetCounter < 50) {
    locals.resetCounter++;
  } else {
    cpu_set_nmi_line(0, PULSE_LINE);
  }
}

static WRITE_HANDLER(joctronic_sndirq) {
  cpu_set_irq_line(1, 0, data ? ASSERT_LINE : CLEAR_LINE);
}

static void joctronic_sndnmi(void) {
  cpu_set_nmi_line(1, PULSE_LINE);
}

/* z80 ctc */
static void ctc_interrupt(int state) {
  cpu_set_irq_line_and_vector(0, 0, state, Z80_VECTOR(0, state));
}

static z80ctc_interface ctc_intf = {
  1,								/* 1 chip */
  { 0 },							/* clock (filled in from the CPU 0 clock */
  { 0 /*NOTIMER_1 | NOTIMER_3*/ },		/* timer disables */
  { ctc_interrupt },			/* interrupt handler */
  { &joctronic_sndirq },	/* ZC/TO0 callback */
  { 0 },							/* ZC/TO1 callback */
  { 0 }							/* ZC/TO2 callback */
};

static Z80_DaisyChain JOCTRONIC_DaisyChain[] = {
  {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
  {0,0,0,-1}
};

static MACHINE_INIT(JOCTRONIC) {
  /* init CTC */
  ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_CPU2), NULL, NULL);
}

static MACHINE_RESET(JOCTRONIC) {
  memset(&locals, 0x00, sizeof(locals));
}

static SWITCH_UPDATE(JOCTRONIC) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 4);
  }
}

static READ_HANDLER(sw_r) {
  return offset ? coreGlobals.swMatrix[offset+1] : ~coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r) {
  return core_getDip(offset);
}

static WRITE_HANDLER(cols_w) {
  locals.cols[offset] = data;
  if (offset == 3 && locals.cols[3] == 0xa1) locals.dispNo = 0;
logerror("A00x: %02x %02x %02x %02x %02x %02x %02x %02x\n", locals.cols[0], locals.cols[1], locals.cols[2], locals.cols[3], locals.cols[4], locals.cols[5], locals.cols[6], locals.cols[7]);
}

static WRITE_HANDLER(disp_w) {
logerror("C020:%2d %02x\n", locals.dispNo, data);
  if (locals.dispNo < 40) coreGlobals.segments[locals.dispNo].w = data;
  locals.dispNo++;
}

static WRITE_HANDLER(led_w) {
  coreGlobals.diagnosticLed = data;
}

static READ_HANDLER(snd_r) {
  return locals.snd;
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0x9000, 0x9003, sw_r},
  {0x9004, 0x9007, dip_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x0000, 0x3fff, MWA_ROM},
  {0x8000, 0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xa000, 0xa007, cols_w},
  {0xc020, 0xc020, disp_w},
  {0xe000, 0xe000, sndbrd_0_data_w},
MEMORY_END

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0xc000, 0xc000, snd_r},
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  {0x0000, 0x3fff, MWA_ROM},
  {0x8000, 0x87ff, MWA_RAM},
  {0xe000, 0xe000, MWA_NOP},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x00,0x03, z80ctc_0_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x00,0x03, z80ctc_0_w},
PORT_END

static PORT_READ_START(snd_readport)
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0x00,0x00, AY8910_control_port_0_w},
  {0x01,0x01, AY8910_write_port_0_w},
  {0x02,0x02, AY8910_control_port_1_w},
  {0x03,0x03, AY8910_write_port_1_w},
PORT_END

static struct AY8910interface JOCTRONIC_ay8910Int = {
  2,
  1500000,
  { MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) },
  { 0 }, { 0 },
  { DAC_0_data_w },
  { DAC_1_data_w },
};

static struct DACinterface JOCTRONIC_dacInt = {
  2,
  { 25, 25 }
};

MACHINE_DRIVER_START(joctronic)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(JOCTRONIC,JOCTRONIC,NULL)
  MDRV_SWITCH_UPDATE(JOCTRONIC)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(32)
  MDRV_DIAGNOSTIC_LEDH(1)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 3000000)
  MDRV_CPU_CONFIG(JOCTRONIC_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(joctronic_vblank, 1)
  MDRV_TIMER_ADD(joctronic_nmi, 100)

  MDRV_CPU_ADD_TAG("scpu", Z80, 6000000)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_SOUND_ADD(AY8910, JOCTRONIC_ay8910Int)
  MDRV_SOUND_ADD(DAC, JOCTRONIC_dacInt)
MACHINE_DRIVER_END

// sound board interface

static WRITE_HANDLER(snd_data_w) {
  locals.snd = data;
  joctronic_sndnmi();
}
const struct sndbrdIntf joctronicIntf = {
  "JOCTRONIC", NULL, NULL, NULL, snd_data_w, snd_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

// dips and display

#define JOCTRONIC_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0001, "Key 1",      KEYCODE_1) \
    COREPORT_BIT   (0x0002, "Key 2",      KEYCODE_2) \
    COREPORT_BIT   (0x0004, "Key 3",      KEYCODE_3) \
    COREPORT_BIT   (0x0008, "Key 4",      KEYCODE_4) \
    COREPORT_BIT   (0x0010, "Key 5",      KEYCODE_5) \
    COREPORT_BIT   (0x0020, "Key 6",      KEYCODE_6) \
    COREPORT_BIT   (0x0040, "Key 7",      KEYCODE_7) \
    COREPORT_BIT   (0x0080, "Test",       KEYCODE_8) \
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
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
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
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
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
      COREPORT_DIPSET(0x8000, "1" ) \
  INPUT_PORTS_END

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0, 8,CORE_SEG7},
  {2, 0, 8, 8,CORE_SEG7},
  {4, 0,16, 8,CORE_SEG7},
  {6, 0,24, 8,CORE_SEG7},
  {8, 0,32, 8,CORE_SEG7},
  {0}
};

// games

/*-------------------------------------------------------------------
/ Punky Willy (1986)
/-------------------------------------------------------------------*/
static core_tGameData punkywilGameData = {0,dispAlpha,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_JOCTRONIC}};
static void init_punkywil(void) { core_gameData = &punkywilGameData; }
ROM_START(punkywil)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("pw_game.bin", 0x0000, 0x4000, NO_DUMP)
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("pw_sound.bin", 0x0000, 0x4000, CRC(b2e3a201) SHA1(e3b0a5b22827683382b61c21607201cd470062ee))
ROM_END
JOCTRONIC_COMPORTS(punkywil, 1)
CORE_GAMEDEFNV(punkywil, "Punky Willy", 1986, "Joctronic", joctronic, GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Walkyria (198?)
/-------------------------------------------------------------------*/
static core_tGameData walkyriaGameData = {0,dispAlpha,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_JOCTRONIC}};
static void init_walkyria(void) { core_gameData = &walkyriaGameData; }
ROM_START(walkyria)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("wk_game.bin", 0x0000, 0x4000, CRC(709722bc) SHA1(4d7b68e9d4a50846cf8eb308ba92f5e2115395d5))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("wk_sound.bin", 0x0000, 0x4000, CRC(81f74b0a) SHA1(1ef73bf42f5b1f54526202d3ff89698a04c7b41a))
ROM_END
JOCTRONIC_COMPORTS(walkyria, 1)
CORE_GAMEDEFNV(walkyria, "Walkyria", 198?, "Joctronic", joctronic, GAME_NOT_WORKING)

// Rider's Surf (1986)
// Pin Ball (1987)
