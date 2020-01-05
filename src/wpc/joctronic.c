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
  UINT8 sndCmd;
  int dispNo, dispCol, swRow;
  core_tSeg segments;
} locals;

static INTERRUPT_GEN(joctronic_vblank) {
  static int count;
  int i;
  if (count > 2) {
    for (i = 0; i < 8; i++) {
      coreGlobals.segments[40 + i].w = locals.segments[i].w;
      locals.segments[i].w = 0;
    }
  }
  count = (count + 1) % 4;
  core_updateSw(TRUE); // game enables flippers directly
}

static INTERRUPT_GEN(joctronic_zc) {
  static int zc;
  z80ctc_0_trg3_w(0, zc);
  zc = !zc;
}

static WRITE_HANDLER(to0_w) {
  cpu_set_irq_line(1, 0, data ? HOLD_LINE : CLEAR_LINE);
}

static void ctc_interrupt(int state) {
  cpu_set_irq_line_and_vector(0, 0, state, Z80_VECTOR(0, state));
}

static z80ctc_interface ctc_intf = {
  1,				/* 1 chip */
  { 0 },		/* clock (CPU 0 clock) */
  { 0 },		/* timer disables (none) */
  { ctc_interrupt },
  { to0_w },
  { 0 },
  { 0 }
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

static MACHINE_STOP(JOCTRONIC) {
  int i;
  cpu_set_nmi_line(0, PULSE_LINE); // NMI routine makes sure the NVRAM is valid!
  for (i = 0; i < 70; i++) { // run some timeslices before shutdown so the NMI routine can finish
    run_one_timeslice();
  }
}

static SWITCH_UPDATE(JOCTRONIC) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x80, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x3b, 11);
  }
}

static READ_HANDLER(sw_r) {
  if (!offset) {
    return ~coreGlobals.swMatrix[1 + locals.swRow];
  }
  return ~coreGlobals.swMatrix[8 + offset];
}

static READ_HANDLER(dip_r) {
  if (!offset) {
    return ~(core_getDip(0) | (coreGlobals.swMatrix[0] & 0x80));
  }
  return ~core_getDip(offset);
}

static WRITE_HANDLER(cols_w) {
  if (offset == 0) {
    locals.dispCol = data & 0x07;
    locals.dispNo = 0;
  } else if (offset == 4) {
  	locals.swRow = data;
  }
}

static WRITE_HANDLER(disp2_w) {
  if ((locals.dispNo % 8) == 7) {
    locals.segments[7 - locals.dispCol].w = coreGlobals.segments[47 - locals.dispCol].w = data & 0x7f;
  }
  locals.dispNo++;
}

static WRITE_HANDLER(disp_w) {
  switch (locals.dispNo) {
    case 7: coreGlobals.segments[7 - locals.dispCol].w = data & 0x7f; break;
    case 15: coreGlobals.segments[15 - locals.dispCol].w = data & 0x7f; break;
    case 23: coreGlobals.segments[23 - locals.dispCol].w = data & 0x7f; break;
    case 31: coreGlobals.segments[31 - locals.dispCol].w = data & 0x7f; break;
    case 39: coreGlobals.segments[39 - locals.dispCol].w = data & 0x7f; break;
  }
  locals.dispNo++;
}

static WRITE_HANDLER(sol_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xff00) | data;
}

static WRITE_HANDLER(sol2_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x00ff) | (data << 8);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.lampMatrix[offset] = data;
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
  {0xc000, 0xc000, disp2_w},
  {0xc020, 0xc020, disp_w},
  {0xc028, 0xc028, sol_w},
  {0xc030, 0xc030, sol2_w},
  {0xc038, 0xc03f, lamp_w},
  {0xe000, 0xe000, sndbrd_0_data_w},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x00,0x03, z80ctc_0_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x00,0x03, z80ctc_0_w},
PORT_END

static READ_HANDLER(snd_r) {
  return locals.sndCmd;
}

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

static PORT_READ_START(snd_readport)
PORT_END

static PORT_READ_START(snd_readport2)
  {0x00,0x00, YM2203_status_port_0_r},
  {0x01,0x01, YM2203_read_port_0_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0x00,0x00, AY8910_control_port_1_w},
  {0x01,0x01, AY8910_write_port_1_w},
  {0x02,0x02, AY8910_control_port_0_w},
  {0x03,0x03, AY8910_write_port_0_w},
PORT_END

static PORT_WRITE_START(snd_writeport2)
  {0x00,0x00, YM2203_control_port_0_w},
  {0x01,0x01, YM2203_write_port_0_w},
  {0x02,0x02, AY8910_control_port_0_w},
  {0x03,0x03, AY8910_write_port_0_w},
PORT_END

static struct AY8910interface JOCTRONIC_ay8910Int = {
  1,
  1500000,
  { MIXER(30,MIXER_PAN_RIGHT) },
  { 0 }, { 0 },
  { 0 }, { 0 }
};

static struct AY8910interface JOCTRONIC_ay8910Int2 = {
  2,
  1500000,
  { MIXER(30,MIXER_PAN_RIGHT), MIXER(30,MIXER_PAN_LEFT) },
  { 0 }, { 0 },
  { 0, DAC_0_data_w }, { 0, DAC_1_data_w },
};

static void ym2203_irq(int state) {
//  cpu_set_irq_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface JOCTRONIC_ym2203Int = {
  1,
  3000000,
  { MIXER(30,MIXER_PAN_LEFT) },
  { NULL }, { NULL },
  { DAC_0_data_w }, { DAC_1_data_w },
  { &ym2203_irq }
};

static struct DACinterface JOCTRONIC_dacInt = {
  2,
  { 25, 25 }
};

static MACHINE_DRIVER_START(joctronic)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(JOCTRONIC,JOCTRONIC,JOCTRONIC)
  MDRV_SWITCH_UPDATE(JOCTRONIC)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(32)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 3000000)
  MDRV_CPU_CONFIG(JOCTRONIC_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(joctronic_vblank, 1)
  MDRV_CPU_PERIODIC_INT(joctronic_zc, 200)

  MDRV_CPU_ADD_TAG("scpu", Z80, 6000000)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(joctronicS1)
  MDRV_IMPORT_FROM(joctronic)

  MDRV_SOUND_ADD(AY8910, JOCTRONIC_ay8910Int2)
  MDRV_SOUND_ADD(DAC, JOCTRONIC_dacInt)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(joctronicS2)
  MDRV_IMPORT_FROM(joctronic)

  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_PORTS(snd_readport2, snd_writeport2)

  MDRV_SOUND_ADD(AY8910, JOCTRONIC_ay8910Int)
  MDRV_SOUND_ADD(YM2203, JOCTRONIC_ym2203Int)
  MDRV_SOUND_ADD(DAC, JOCTRONIC_dacInt)
MACHINE_DRIVER_END

// sound board interface

static WRITE_HANDLER(snd_data_w) {
  locals.sndCmd = data;
  cpu_set_nmi_line(1, PULSE_LINE);
}
const struct sndbrdIntf joctronicIntf = {
  "JOCTRONIC", NULL, NULL, NULL, snd_data_w, snd_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

// dips and display

static core_tLCDLayout dispAlpha[] = {
  {0, 0,41,7,CORE_SEG7},
  {3,13, 1,7,CORE_SEG7S},
  {5,13, 9,7,CORE_SEG7S},
  {7,13,17,7,CORE_SEG7S},
  {9,13,25,7,CORE_SEG7S},
  {8, 0,33,2,CORE_SEG7S},{8, 5,36,1,CORE_SEG7S},{8, 8,38,2,CORE_SEG7S},
  {0}
};

// games

/*-------------------------------------------------------------------
/ Punky Willy (1986)
/-------------------------------------------------------------------*/
static core_tGameData punkywilGameData = {0,dispAlpha,{FLIP_SWNO(88,87),0,0,0,SNDBRD_JOCTRONIC}};
static void init_punkywil(void) { core_gameData = &punkywilGameData; }
ROM_START(punkywil)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("pw_game.bin", 0x0000, 0x4000, CRC(f408367a) SHA1(967ab8a16e64273abf8e8cc4faab60e2c9a4856b))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("pw_sound.bin", 0x0000, 0x4000, CRC(b2e3a201) SHA1(e3b0a5b22827683382b61c21607201cd470062ee))
ROM_END
INPUT_PORTS_START(punkywil)
CORE_PORTS
SIM_PORTS(1)
PORT_START /* 0 */
  COREPORT_BIT   (0x0008, "Game start",     KEYCODE_1)
  COREPORT_BIT   (0x0001, "Coin 1",         KEYCODE_3)
  COREPORT_BIT   (0x0002, "Coin 2",         KEYCODE_4)
  COREPORT_BIT   (0x0010, "Service credit", KEYCODE_5)
  COREPORT_BIT   (0x0020, "Tilt",           KEYCODE_INSERT)
  COREPORT_BIT   (0x8000, "Test",           KEYCODE_7)
PORT_START /* 1 */
  COREPORT_DIPNAME( 0x0080, 0x0000, "Test mode")
    COREPORT_DIPSET(0x0000, DEF_STR(Off))
    COREPORT_DIPSET(0x0080, DEF_STR(On))
  COREPORT_DIPNAME( 0x0040, 0x0040, "Attract tune")
    COREPORT_DIPSET(0x0000, DEF_STR(Off))
    COREPORT_DIPSET(0x0040, DEF_STR(On))
  COREPORT_DIPNAME( 0x000f, 0x000f, "Slot #1 pricing (cred./coins)")
    COREPORT_DIPSET(0x000f, "1/1" )
    COREPORT_DIPSET(0x000e, "2/1" )
    COREPORT_DIPSET(0x000d, "3/1" )
    COREPORT_DIPSET(0x000c, "4/1" )
    COREPORT_DIPSET(0x000b, "5/1" )
    COREPORT_DIPSET(0x000a, "6/1" )
    COREPORT_DIPSET(0x0009, "8/1" )
    COREPORT_DIPSET(0x0007, "1/2" )
    COREPORT_DIPSET(0x0006, "2/2" )
    COREPORT_DIPSET(0x0005, "3/2" )
    COREPORT_DIPSET(0x0004, "1/3" )
    COREPORT_DIPSET(0x0003, "2/3" )
    COREPORT_DIPSET(0x0002, "1/4" )
    COREPORT_DIPSET(0x0001, "1/5" )
    COREPORT_DIPSET(0x0008, "0")
    COREPORT_DIPSET(0x0000, "0")
  COREPORT_DIPNAME( 0x0030, 0x0020, "Bonus credits for x coins #1")
    COREPORT_DIPSET(0x0030, DEF_STR(Off))
    COREPORT_DIPSET(0x0020, "2" )
    COREPORT_DIPSET(0x0010, "3" )
    COREPORT_DIPSET(0x0000, "4" )
  COREPORT_DIPNAME( 0x0f00, 0x0d00, "Slot #2 pricing (cred./coins)")
    COREPORT_DIPSET(0x0f00, "1/1" )
    COREPORT_DIPSET(0x0e00, "2/1" )
    COREPORT_DIPSET(0x0d00, "3/1" )
    COREPORT_DIPSET(0x0c00, "4/1" )
    COREPORT_DIPSET(0x0b00, "5/1" )
    COREPORT_DIPSET(0x0a00, "6/1" )
    COREPORT_DIPSET(0x0900, "8/1" )
    COREPORT_DIPSET(0x0700, "1/2" )
    COREPORT_DIPSET(0x0600, "2/2" )
    COREPORT_DIPSET(0x0500, "3/2" )
    COREPORT_DIPSET(0x0400, "1/3" )
    COREPORT_DIPSET(0x0300, "2/3" )
    COREPORT_DIPSET(0x0200, "1/4" )
    COREPORT_DIPSET(0x0100, "1/5" )
    COREPORT_DIPSET(0x0800, "0")
    COREPORT_DIPSET(0x0000, "0")
  COREPORT_DIPNAME( 0x3000, 0x3000, "Bonus credits for x coins #2")
    COREPORT_DIPSET(0x3000, DEF_STR(Off))
    COREPORT_DIPSET(0x2000, "2" )
    COREPORT_DIPSET(0x1000, "3" )
    COREPORT_DIPSET(0x0000, "4" )
  COREPORT_DIPNAME( 0x4000, 0x0000, "S15")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x4000, "1" )
  COREPORT_DIPNAME( 0x8000, 0x0000, "S16")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x8000, "1" )
PORT_START /* 2 */
  COREPORT_DIPNAME( 0x0700, 0x0500, "Balls per game")
    COREPORT_DIPSET(0x0700, "1" )
    COREPORT_DIPSET(0x0600, "2" )
    COREPORT_DIPSET(0x0500, "3" )
    COREPORT_DIPSET(0x0400, "4" )
    COREPORT_DIPSET(0x0300, "5" )
    COREPORT_DIPSET(0x0200, "6" )
    COREPORT_DIPSET(0x0100, "8" )
    COREPORT_DIPSET(0x0000, "10" )
  COREPORT_DIPNAME( 0x0001, 0x0000, "S17")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0001, "1" )
  COREPORT_DIPNAME( 0x0002, 0x0000, "S18")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0002, "1" )
  COREPORT_DIPNAME( 0x0004, 0x0000, "S19")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0004, "1" )
  COREPORT_DIPNAME( 0x0008, 0x0000, "S20")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0008, "1" )
  COREPORT_DIPNAME( 0x0010, 0x0000, "S21")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0010, "1" )
  COREPORT_DIPNAME( 0x0020, 0x0000, "S22")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0020, "1" )
  COREPORT_DIPNAME( 0x0040, 0x0000, "S23")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0040, "1" )
  COREPORT_DIPNAME( 0x0080, 0x0000, "S24")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0080, "1" )
  COREPORT_DIPNAME( 0x0800, 0x0000, "S28")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0800, "1" )
  COREPORT_DIPNAME( 0x1000, 0x0000, "S29")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x1000, "1" )
  COREPORT_DIPNAME( 0x2000, 0x0000, "S30")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x2000, "1" )
  COREPORT_DIPNAME( 0x4000, 0x0000, "S31")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x4000, "1" )
  COREPORT_DIPNAME( 0x8000, 0x0000, "S32")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END
CORE_GAMEDEFNV(punkywil, "Punky Willy", 1986, "Joctronic", joctronicS2, 0)

/*-------------------------------------------------------------------
/ Walkyria (1986)
/-------------------------------------------------------------------*/
static core_tGameData walkyriaGameData = {0,dispAlpha,{FLIP_SWNO(88,87),0,0,0,SNDBRD_JOCTRONIC}};
static void init_walkyria(void) { core_gameData = &walkyriaGameData; }
ROM_START(walkyria)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("wk_game.bin", 0x0000, 0x4000, CRC(709722bc) SHA1(4d7b68e9d4a50846cf8eb308ba92f5e2115395d5))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("wk_sound.bin", 0x0000, 0x4000, CRC(81f74b0a) SHA1(1ef73bf42f5b1f54526202d3ff89698a04c7b41a))
ROM_END
INPUT_PORTS_START(walkyria)
CORE_PORTS
SIM_PORTS(1)
PORT_START /* 0 */
  COREPORT_BIT   (0x0008, "Game start",     KEYCODE_1)
  COREPORT_BIT   (0x0001, "Coin 1",         KEYCODE_3)
  COREPORT_BIT   (0x0002, "Coin 2",         KEYCODE_4)
  COREPORT_BIT   (0x0010, "Service credit", KEYCODE_5)
  COREPORT_BIT   (0x0020, "Tilt",           KEYCODE_INSERT)
  COREPORT_BIT   (0x8000, "Test",           KEYCODE_7)
PORT_START /* 1 */
  COREPORT_DIPNAME( 0x0080, 0x0000, "Test mode")
    COREPORT_DIPSET(0x0000, DEF_STR(Off))
    COREPORT_DIPSET(0x0080, DEF_STR(On))
  COREPORT_DIPNAME( 0x0040, 0x0040, "Attract tune")
    COREPORT_DIPSET(0x0000, DEF_STR(Off))
    COREPORT_DIPSET(0x0040, DEF_STR(On))
  COREPORT_DIPNAME( 0x000f, 0x000f, "Slot #1 pricing (cred./coins)")
    COREPORT_DIPSET(0x000f, "1/1" )
    COREPORT_DIPSET(0x000e, "2/1" )
    COREPORT_DIPSET(0x000d, "3/1" )
    COREPORT_DIPSET(0x000c, "4/1" )
    COREPORT_DIPSET(0x000b, "5/1" )
    COREPORT_DIPSET(0x000a, "6/1" )
    COREPORT_DIPSET(0x0009, "8/1" )
    COREPORT_DIPSET(0x0007, "1/2" )
    COREPORT_DIPSET(0x0006, "2/2" )
    COREPORT_DIPSET(0x0005, "3/2" )
    COREPORT_DIPSET(0x0004, "1/3" )
    COREPORT_DIPSET(0x0003, "2/3" )
    COREPORT_DIPSET(0x0002, "1/4" )
    COREPORT_DIPSET(0x0001, "1/5" )
    COREPORT_DIPSET(0x0008, "0")
    COREPORT_DIPSET(0x0000, "0")
  COREPORT_DIPNAME( 0x0030, 0x0020, "Bonus credits for x coins #1")
    COREPORT_DIPSET(0x0030, DEF_STR(Off))
    COREPORT_DIPSET(0x0020, "2" )
    COREPORT_DIPSET(0x0010, "3" )
    COREPORT_DIPSET(0x0000, "4" )
  COREPORT_DIPNAME( 0x0f00, 0x0d00, "Slot #2 pricing (cred./coins)")
    COREPORT_DIPSET(0x0f00, "1/1" )
    COREPORT_DIPSET(0x0e00, "2/1" )
    COREPORT_DIPSET(0x0d00, "3/1" )
    COREPORT_DIPSET(0x0c00, "4/1" )
    COREPORT_DIPSET(0x0b00, "5/1" )
    COREPORT_DIPSET(0x0a00, "6/1" )
    COREPORT_DIPSET(0x0900, "8/1" )
    COREPORT_DIPSET(0x0700, "1/2" )
    COREPORT_DIPSET(0x0600, "2/2" )
    COREPORT_DIPSET(0x0500, "3/2" )
    COREPORT_DIPSET(0x0400, "1/3" )
    COREPORT_DIPSET(0x0300, "2/3" )
    COREPORT_DIPSET(0x0200, "1/4" )
    COREPORT_DIPSET(0x0100, "1/5" )
    COREPORT_DIPSET(0x0800, "0")
    COREPORT_DIPSET(0x0000, "0")
  COREPORT_DIPNAME( 0x3000, 0x3000, "Bonus credits for x coins #2")
    COREPORT_DIPSET(0x3000, DEF_STR(Off))
    COREPORT_DIPSET(0x2000, "2" )
    COREPORT_DIPSET(0x1000, "3" )
    COREPORT_DIPSET(0x0000, "4" )
  COREPORT_DIPNAME( 0x4000, 0x0000, "S15")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x4000, "1" )
  COREPORT_DIPNAME( 0x8000, 0x0000, "S16")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x8000, "1" )
PORT_START /* 2 */
  COREPORT_DIPNAME( 0x0070, 0x0050, "Balls per game")
    COREPORT_DIPSET(0x0070, "1" )
    COREPORT_DIPSET(0x0060, "2" )
    COREPORT_DIPSET(0x0050, "3" )
    COREPORT_DIPSET(0x0040, "4" )
    COREPORT_DIPSET(0x0030, "5" )
    COREPORT_DIPSET(0x0020, "6" )
    COREPORT_DIPSET(0x0010, "8" )
    COREPORT_DIPSET(0x0000, "10" )
  COREPORT_DIPNAME( 0x0001, 0x0000, "S17")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0001, "1" )
  COREPORT_DIPNAME( 0x0002, 0x0000, "S18")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0002, "1" )
  COREPORT_DIPNAME( 0x0004, 0x0000, "S19")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0004, "1" )
  COREPORT_DIPNAME( 0x0008, 0x0000, "S20")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0008, "1" )
  COREPORT_DIPNAME( 0x0080, 0x0000, "S24")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0080, "1" )
  COREPORT_DIPNAME( 0x0100, 0x0000, "S25")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0100, "1" )
  COREPORT_DIPNAME( 0x0200, 0x0000, "S26")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0200, "1" )
  COREPORT_DIPNAME( 0x0400, 0x0000, "S27")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0400, "1" )
  COREPORT_DIPNAME( 0x0800, 0x0000, "S28")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x0800, "1" )
  COREPORT_DIPNAME( 0x1000, 0x0000, "S29")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x1000, "1" )
  COREPORT_DIPNAME( 0x2000, 0x0000, "S30")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x2000, "1" )
  COREPORT_DIPNAME( 0x4000, 0x0000, "S31")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x4000, "1" )
  COREPORT_DIPNAME( 0x8000, 0x0000, "S32")
    COREPORT_DIPSET(0x0000, "0" )
    COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END
CORE_GAMEDEFNV(walkyria, "Walkyria", 1986, "Joctronic", joctronicS1, 0)

// Pin Ball (1987)
