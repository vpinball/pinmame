/******************************************************************************************
  Stargame: Space Ship, White Force
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/z80/z80.h"
#include "sound/mea8000.h"
#include "machine/z80fmly.h"

static struct {
  int swCol, dispCnt, isWf;
  UINT8 sndCmd;
} locals;

static INTERRUPT_GEN(stargame_vblank) {
  core_updateSw(core_getSol(18));
}

static INTERRUPT_GEN(stargame_zc) {
  static int zc;
  z80ctc_0_trg2_w(0, zc);
  z80ctc_0_trg3_w(0, !zc);
  zc = !zc;
}

static void ctc_interrupt(int state) {
  cpu_set_irq_line_and_vector(0, 0, state, Z80_VECTOR(0, state));
}

static WRITE_HANDLER(to0_w) {
  cpu_set_irq_line(1, 0, data ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(to1_w) {
//printf("TO1:%02x\n", data);
}

static z80ctc_interface ctc_intf = {
  1,				/* 1 chip */
  { 0 },		/* clock (0 = CPU 0 clock) */
  { 0 },		/* timer disables (none) */
  { ctc_interrupt },
  { to0_w },
  { to1_w },
  { 0 }
};

static Z80_DaisyChain STARGAME_DaisyChain[] = {
  {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
  {0,0,0,-1}
};

static MACHINE_INIT(STARGAME) {
  /* init CTC */
  ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);

  // MEA8000 uses DAC #0 as output device, so don't change order in driver definition!
  mea8000_config(0, 0);
  mixer_set_name(0, "MEA 8000");
}

static MACHINE_RESET(STARGAME) {
  memset(&locals, 0x00, sizeof(locals));
  
  mea8000_reset();
}

static MACHINE_RESET(WHTFORCE) {
  memset(&locals, 0x00, sizeof(locals));
  locals.isWf = 1;
  
  mea8000_reset();
}

static MACHINE_STOP(STARGAME) {
}

#ifdef MAME_DEBUG
static void adjust_snd(int offset) {
  static char s[4];
  locals.sndCmd += offset;
  sprintf(s, "%02x", locals.sndCmd);
  core_textOut(s, 4, 25, 2, 5);
}
#endif /* MAME_DEBUG */

static SWITCH_UPDATE(STARGAME) {
  if (inports) {
    if (locals.isWf) {
      CORE_SETKEYSW(inports[CORE_COREINPORT], 0x0f, 7);
      CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x0e, 8);
    } else {
      CORE_SETKEYSW(inports[CORE_COREINPORT] >> 9, 0x0f, 0);
      CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x40, 8);
      CORE_SETKEYSW(inports[CORE_COREINPORT], 0x75, 9);
      CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x01, 10);
    }
  }
#ifdef MAME_DEBUG
  if (keyboard_pressed_memory_repeat(KEYCODE_B, 2)) {
    adjust_snd(-1);
  }
  if (keyboard_pressed_memory_repeat(KEYCODE_N, 2)) {
    adjust_snd(1);
  }
  if (keyboard_pressed_memory_repeat(KEYCODE_M, 2)) {
    cpu_set_nmi_line(1, ASSERT_LINE);
  }
#endif /* MAME_DEBUG */
}

static WRITE_HANDLER(port_00_w) {
  locals.sndCmd = data;
  cpu_set_nmi_line(1, ASSERT_LINE);
}

// watchdog related, ignore
static WRITE_HANDLER(port_1x_w) {
}

// switch column - whtforce only
static WRITE_HANDLER(port_20_w) {
  locals.swCol = core_BitColToNum(data);
}

// additional switches (7x4 bits), spcship only
static READ_HANDLER(port_20_r) {
  switch (locals.swCol) {
    case 0: return coreGlobals.swMatrix[8] & 0x0f; break;
    case 1: return coreGlobals.swMatrix[8] >> 4; break;
    case 2: return coreGlobals.swMatrix[9] & 0x0f; break;
    case 3: return coreGlobals.swMatrix[9] >> 4; break;
    case 4: return coreGlobals.swMatrix[10] & 0x0f; break;
    case 5: return coreGlobals.swMatrix[10] >> 4; break;
    case 6: return coreGlobals.swMatrix[0] & 0x0f; break;
  }
  return 0;
}

static READ_HANDLER(port_30_r) {
  return coreGlobals.swMatrix[1 + locals.swCol];
}

// switch column - spcship only, yet whtforce writes here as well?!
static WRITE_HANDLER(port_40_w) {
  if (locals.isWf) {
//  coreGlobals.lampMatrix[8] = data;
    return;
  }
  locals.swCol = core_BitColToNum(data);
}

// solenoids are part of the lamp matrix, as usual on Spanish manufacturers, but this time very heavily so!
static WRITE_HANDLER(port_5x_w) {
  coreGlobals.lampMatrix[offset] = data;
  if (locals.isWf) {
    switch (offset) {
      case 0: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffefe) | ((data >> 3) & 1) | (((data >> 5) & 1) << 8); break;
      case 1: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffdfd) | (((data >> 3) & 1) << 1) | (((data >> 5) & 1) << 9); break;
      case 2: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffbfb) | (((data >> 3) & 1) << 2) | (((data >> 5) & 1) << 10); break;
      case 3: coreGlobals.solenoids = (coreGlobals.solenoids & 0xff7ff) | (((data >> 5) & 1) << 11); break;
      case 4: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfefff) | (((data >> 5) & 1) << 12); break;
      case 5: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfdfff) | (((data >> 5) & 1) << 13); break;
      case 6: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfbfff) | (((data >> 5) & 1) << 14); break;
      case 7: coreGlobals.solenoids = (coreGlobals.solenoids & 0xf7fff) | (((data >> 5) & 1) << 15); break;
    }
  } else {
    switch (offset) {
      case 0: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffffe) | (data & 1); break;
      case 1: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffffd) | ((data & 1) << 1); break;
      case 2: coreGlobals.solenoids = (coreGlobals.solenoids & 0xffbfb) | ((data & 1) << 2) | (((data >> 1) & 1) << 10); break;
      case 3: coreGlobals.solenoids = (coreGlobals.solenoids & 0xff7f7) | ((data & 1) << 3) | (((data >> 1) & 1) << 11); break;
      case 4: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfefef) | ((data & 1) << 4) | (((data >> 1) & 1) << 12); break;
      case 5: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfdfdf) | ((data & 1) << 5) | (((data >> 1) & 1) << 13); break;
      case 6: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfbfbf) | ((data & 1) << 6) | (((data >> 1) & 1) << 14); break;
      case 7: coreGlobals.solenoids = (coreGlobals.solenoids & 0xfff7f) | ((data & 1) << 7); break;
    }
  }
}

static WRITE_HANDLER(port_6x_w) {
  switch (offset) {
    case 2: coreGlobals.solenoids = (coreGlobals.solenoids & 0xeffff) | ((data & 1) << 16); break; // game on
    case 3: coreGlobals.solenoids = (coreGlobals.solenoids & 0xdffff) | ((data & 1) << 17); break; // flipper power
    case 4: coreGlobals.solenoids = (coreGlobals.solenoids & 0xbffff) | ((data & 1) << 18); break; // aux. output
    case 5: locals.dispCnt = 0; break;
    case 6: if (data) {
              cpuint_reset_cpu(1);
              AY8910_reset(0);
            }
            break;
  }
}

// serial display output
static WRITE_HANDLER(port_68_w) {
  static int dispCol;
  if (locals.isWf) {
    data ^= 0xff;
  }
  switch (locals.dispCnt) {
    case 3: dispCol = data & 0x07; break;
    case 11: coreGlobals.segments[27 - dispCol].w = data; break;
    case 19: coreGlobals.segments[20 - dispCol].w = data; break;
    case 27: coreGlobals.segments[34 - dispCol].w = data; break;
    case 35: coreGlobals.segments[13 - dispCol].w = data; break;
    case 43: coreGlobals.segments[6 - dispCol].w = data; break;
    case 51: coreGlobals.segments[41 - dispCol].w = data; break;
  }
  locals.dispCnt++;
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x8000, 0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(cpu_readport)
  {0x20, 0x20, port_20_r},
  {0x30, 0x30, port_30_r},
  {0x70, 0x73, z80ctc_0_r},
PORT_END

static PORT_WRITE_START(cpu_writeport)
  {0x00, 0x00, port_00_w},
  {0x10, 0x17, port_1x_w},
  {0x20, 0x20, port_20_w},
  {0x40, 0x40, port_40_w},
  {0x50, 0x57, port_5x_w},
  {0x60, 0x67, port_6x_w},
  {0x68, 0x68, port_68_w},
  {0x70, 0x73, z80ctc_0_w},
PORT_END

static READ_HANDLER(snd_resint_r) {
  cpu_set_irq_line(1, 0, CLEAR_LINE);
  return 0;
}

static READ_HANDLER(snd_cmd_r) {
  cpu_set_nmi_line(1, CLEAR_LINE);
  return locals.sndCmd;
}

static struct DACinterface STARGAME_dacInt = {
  2,
  { 100, 30 }
};

static struct AY8910interface STARGAME_ay8910Int = {
  1,
  15000000/8,
  { 30 },
  { 0 },
  { 0 },
  { DAC_1_data_w },
  { 0 }
};

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x4000, 0x4001, mea8000_r},
  {0x8000, 0x87ff, MRA_RAM},
  {0xc000, 0xc000, snd_resint_r},
  {0xe000, 0xe000, snd_cmd_r},
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  {0x0000, 0x3fff, MWA_NOP},
  {0x4000, 0x4001, mea8000_w},
  {0x8000, 0x87ff, MWA_RAM},
MEMORY_END

static PORT_READ_START(snd_readport)
  {0x00, 0x00, AY8910_read_port_0_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0x00, 0x00, AY8910_control_port_0_w},
  {0x01, 0x01, AY8910_write_port_0_w},
PORT_END

static MACHINE_DRIVER_START(spcship)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_FRAMES_PER_SECOND(50)

  MDRV_CPU_ADD_TAG("mcpu", Z80, 15000000./4.)
  MDRV_CPU_CONFIG(STARGAME_DaisyChain)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_PORTS(cpu_readport, cpu_writeport)
  MDRV_CPU_VBLANK_INT(stargame_vblank, 1)
  MDRV_CPU_PERIODIC_INT(stargame_zc, 100)

  MDRV_CPU_ADD_TAG("scpu", Z80, 15000000./4. + 15000000./8.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)

  MDRV_CORE_INIT_RESET_STOP(STARGAME,STARGAME,STARGAME)
  MDRV_SWITCH_UPDATE(STARGAME)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SOUND_ADD(DAC, STARGAME_dacInt)
  MDRV_SOUND_ADD(AY8910, STARGAME_ay8910Int)
MACHINE_DRIVER_END

#define STARGAME_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls)

#define INITGAME(name, disp, flip, lamps) \
static core_tGameData name##GameData = {0,disp,{flip,0,lamps}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \

static core_tLCDLayout dispSpcship[] = {
  {0, 0, 0,7,CORE_SEG7}, {0,16, 7,7,CORE_SEG7},
  {3, 0,14,7,CORE_SEG7}, {3,16,21,7,CORE_SEG7},
  {7, 0,29,2,CORE_SEG7}, {7, 6,32,1,CORE_SEG7}, {7,10,33,2,CORE_SEG7},
  {0}
};

ROM_START(spcship)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("sss-1g.bin", 0x0000, 0x4000, CRC(119a3064) SHA1(d915ecf44279a9e16a50a723eb9523afec1fb380))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("sss-1a0.bin", 0x0000, 0x4000, CRC(eae78e63) SHA1(9fa3587ae3ee6f674bb16102680e70069e9d275e))
ROM_END
STARGAME_COMPORTS(spcship, 1)
  PORT_START /* 0 */
    COREPORT_BIT   (0x0100, "Game start",  KEYCODE_1)
    COREPORT_BIT   (0x0004, "Coin 1",      KEYCODE_3)
    COREPORT_BIT   (0x0040, "Coin 2",      KEYCODE_4)
    COREPORT_BIT   (0x4000, "Coin 3",      KEYCODE_5)
    COREPORT_BIT   (0x0020, "Tilt",        KEYCODE_INSERT)
    COREPORT_BIT   (0x0001, "Test",        KEYCODE_7)
    COREPORT_BIT   (0x0010, "Advance",     KEYCODE_8)
    COREPORT_BIT   (0x0200, "F1",          KEYCODE_1_PAD)
    COREPORT_BIT   (0x0400, "F2",          KEYCODE_2_PAD)
    COREPORT_BIT   (0x0800, "F3",          KEYCODE_3_PAD)
    COREPORT_BIT   (0x1000, "F4",          KEYCODE_4_PAD)
  INPUT_PORTS_END
INITGAME(spcship,dispSpcship,FLIP_SW(FLIP_L),0)
CORE_GAMEDEFNV(spcship, "Space Ship", 1986, "Stargame", spcship, 0)

static MACHINE_DRIVER_START(whtforce)
  MDRV_IMPORT_FROM(spcship)
  MDRV_CORE_INIT_RESET_STOP(STARGAME,WHTFORCE,STARGAME)
MACHINE_DRIVER_END

static core_tLCDLayout dispWhtforce[] = {
  {0, 0, 0,7,CORE_SEG7}, {4, 0, 7,7,CORE_SEG7},
  {0,28,14,7,CORE_SEG7}, {4,28,21,7,CORE_SEG7},
  {2,15,29,2,CORE_SEG7}, {2,20,32,1,CORE_SEG7}, {2,23,33,2,CORE_SEG7},
  {0}
};

ROM_START(whtforce)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("m5l.bin", 0x0000, 0x4000, CRC(22495322) SHA1(b34a34dec875f215d566d18a5e877b9185a22ab7))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("sound.bin", 0x0000, 0x4000, CRC(4b2a1580) SHA1(62133fd186b1aab4f5aecfbff8151ba416328021))
ROM_END
STARGAME_COMPORTS(whtforce, 1)
  PORT_START /* 0 */
    COREPORT_BIT   (0x0004, "Game start",  KEYCODE_1)
    COREPORT_BIT   (0x0001, "Coin 1",      KEYCODE_3)
    COREPORT_BIT   (0x0002, "Coin 2",      KEYCODE_4)
    COREPORT_BIT   (0x0008, "Coin 3",      KEYCODE_5)
    COREPORT_BIT   (0x0400, "Tilt",        KEYCODE_INSERT)
    COREPORT_BIT   (0x0800, "Test",        KEYCODE_7)
    COREPORT_BIT   (0x0200, "Advance",     KEYCODE_8)
  INPUT_PORTS_END
INITGAME(whtforce,dispWhtforce,FLIP_SW(FLIP_L),0)
CORE_GAMEDEFNV(whtforce, "White Force", 1987, "Stargame", whtforce, 0)
