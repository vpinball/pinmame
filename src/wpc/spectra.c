/******************************************************************************************
  Valley Spectra IV
  -----------------
  Rotating game, like Midway's "Rotation VIII".
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "sound/sn76477.h"

static struct {
  UINT8 swStrobe;
  int   inhibitNmi;
} locals;

static INTERRUPT_GEN(spectra_vblank) {
  core_updateSw(core_getSol(11));
}

static void spectra_irq(int irq) {
  cpu_set_irq_line(0, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static void spectra_nmi(int data) {
  if (!locals.inhibitNmi) {
    cpu_set_nmi_line(0, PULSE_LINE);
  }
}

static WRITE_HANDLER(riot_porta_w) {
  locals.swStrobe = data & 0x1f;
  locals.inhibitNmi = (data & 20) >> 5;
}

static READ_HANDLER(riot_porta_r) {
  int sw = core_getSw(1 + locals.swStrobe) ? 0 : 1;
  return locals.swStrobe | (locals.inhibitNmi << 5) | (sw << 6) | ((coreGlobals.swMatrix[0] & 1) << 7);
}

static WRITE_HANDLER(riot_portb_w) {
  double vco = 5.5 - (core_getDip(0) >> 4) / 6.0;
  vco -= ((data & 8) ? vco / 2.0 : 0) + ((data & 4) ? vco / 4.0 : 0)+ ((data & 2) ? vco / 8.0 : 0)+ ((data & 1) ? vco / 16.0 : 0);
  logerror("RIOT B WRITE %02x, vco: %0f\n", data, vco);
  SN76477_set_vco_voltage(0, 5.5 - vco);
  SN76477_enable_w(0, data & 0x10 ? 0 : 1); // strobe: toggles enable
  SN76477_envelope_w(0, data & 0x20 ? 0 : 1); //decay: toggles envelope
  SN76477_vco_w(0, data & 0x40 ? 1 : 0); // "phaser" sound: VCO toggled
  SN76477_mixer_w(0, data & 0x80 ? 2 : 0); // "pulse" sound: pins 25 & 27 changed
}

static READ_HANDLER(riot_portb_r) {
  logerror("RIOT B READ\n");
  return (core_getDip(0) & 1) ? 0x5a : 0;
}

static struct riot6532_interface riot6532_intf = {
  /* in  : A/B, */ riot_porta_r, riot_portb_r,
  /* out : A/B, */ riot_porta_w, riot_portb_w,
  /* irq :      */ spectra_irq
};

static MACHINE_INIT(SPECTRA) {
  riot6532_config(0, &riot6532_intf);
  riot6532_set_clock(0, 905000);
}

static MACHINE_RESET(SPECTRA) {
  memset(&locals, 0x00, sizeof(locals));
  riot6532_reset();
}

static MACHINE_STOP(SPECTRA) {
  riot6532_unconfig();
}

static SWITCH_UPDATE(SPECTRA) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x01, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xf8, 4);
  }
}

static UINT8 *spectra_CMOS;
static NVRAM_HANDLER(SPECTRA) {
  core_nvram(file, read_or_write, spectra_CMOS, 0x100, 0x00);
}
static WRITE_HANDLER(spectra_CMOS_w) {
  spectra_CMOS[offset] = data;
  if (offset < 0x28) {
    UINT8 digit;
    int segData = data & 0x0f;
    switch (segData) { // emulate the 74c912 segment layout
      case 0x0a: // o
        digit = 0x5c; break;
      case 0x0b: // °
        digit = 0x63; break;
      case 0x0c: // top line
        digit = 0x01; break;
      case 0x0d: // -
        digit = 0x40; break;
      case 0x0e: // _
        digit = 0x08; break;
      default:
        digit = core_bcd2seg7[segData];
    }
    coreGlobals.segments[offset].w = digit;
    coreGlobals.lampMatrix[10 - offset / 8] &= 0xff ^ (1 << (offset % 8));
    coreGlobals.lampMatrix[10 - offset / 8] |= ((data >> 4) & 1) << (offset % 8);
  } else if (offset > 0x3f && offset < 0x70) {
    coreGlobals.lampMatrix[(offset - 0x40) / 8] &= 0xff ^ (1 << (offset % 8));
    coreGlobals.lampMatrix[(offset - 0x40) / 8] |= (data & 1) << (offset % 8);
  } else if (offset > 0x6f && offset < 0x80) {
    coreGlobals.solenoids &= 0xffff ^ (1 << (offset - 0x70));
    coreGlobals.solenoids |= (data & 1) << (offset - 0x70);
  }
}

static MEMORY_READ_START(cpu_readmem)
  {0x0000, 0x00ff, MRA_RAM}, // 5101 RAM (battery backed)
  {0x0100, 0x017f, MRA_RAM}, // RIOT RAM
  {0x0180, 0x019f, riot6532_0_r}, // RIOT registers
  {0x0400, 0x0fff, MRA_ROM},
  {0xfff8, 0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  {0x0000, 0x00ff, spectra_CMOS_w, &spectra_CMOS},
  {0x0100, 0x017f, MWA_RAM},
  {0x0180, 0x019f, riot6532_0_w},
  {0x0400, 0x0fff, MWA_ROM},
MEMORY_END

static struct SN76477interface spectra_sn76477Int = { 1, { 50 }, /* mixing level */
	                	/* pin description		*/
	{ RES_M(1000) },	/*  4  noise_res		*/
	{ RES_M(1000) },	/*  5  filter_res		*/
	{ CAP_N(0)    },	/*  6  filter_cap		*/
	{ RES_K(470)  },	/*  7  decay_res		*/
	{ CAP_N(1)    },	/*  8  attack_decay_cap */
	{ RES_K(22)   },	/* 10  attack_res		*/
	{ RES_K(100)  },	/* 11  amplitude_res	*/
	{ RES_K(52)   },	/* 12  feedback_res 	*/
	{ 5.0         },	/* 16  vco_voltage		*/
	{ CAP_U(0.01) },	/* 17  vco_cap			*/
	{ RES_K(390)  },	/* 18  vco_res			*/
	{ 0.0         },  /* 19  pitch_voltage	*/
	{ RES_M(1)    },	/* 20  slf_res			*/
	{ CAP_U(0.1)  },	/* 21  slf_cap			*/
	{ CAP_U(0.47) },	/* 23  oneshot_cap		*/
	{ RES_K(470)  }		/* 24  oneshot_res		*/
};

MACHINE_DRIVER_START(spectra)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6502, 3579545/4)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_VBLANK_INT(spectra_vblank, 1)
  MDRV_TIMER_ADD(spectra_nmi, 120)
  MDRV_CORE_INIT_RESET_STOP(SPECTRA,SPECTRA,SPECTRA)
  MDRV_NVRAM_HANDLER(SPECTRA)
  MDRV_SWITCH_UPDATE(SPECTRA)
  MDRV_SOUND_ADD(SN76477, spectra_sn76477Int)
  MDRV_DIPS(8) // added so the NVRAM can be reset, and for tone pitch
MACHINE_DRIVER_END

#define INITGAME(name, disp, flip) \
static core_tGameData name##GameData = {0,disp,{flip,0,3}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define SPECTRA_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0010, "Demand",     KEYCODE_1) \
    COREPORT_BIT   (0x0008, "Coin/Reset", KEYCODE_5) \
    COREPORT_BITTOG(0x0040, "Set",        KEYCODE_7) \
    COREPORT_BITTOG(0x0020, "Test",       KEYCODE_8) \
    COREPORT_BIT   (0x0080, "Tilt",       KEYCODE_INSERT) \
    COREPORT_BIT   (0x0001, "Slam Tilt",  KEYCODE_HOME) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Reset NVRAM") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0001, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x00f0, 0x0020, "Tone pitch") \
      COREPORT_DIPSET(0x0000, "0") \
      COREPORT_DIPSET(0x0010, "1") \
      COREPORT_DIPSET(0x0020, "2") \
      COREPORT_DIPSET(0x0030, "3") \
      COREPORT_DIPSET(0x0040, "4") \
      COREPORT_DIPSET(0x0050, "5") \
      COREPORT_DIPSET(0x0060, "6") \
      COREPORT_DIPSET(0x0070, "7") \
      COREPORT_DIPSET(0x0080, "8") \
      COREPORT_DIPSET(0x0090, "9") \
  INPUT_PORTS_END

static core_tLCDLayout dispAlpha[] = {
  { 9, 0,24, 6,CORE_SEG8D},
  { 6, 0,16, 6,CORE_SEG8D},
  { 3, 0, 8, 6,CORE_SEG8D},
  { 0, 0, 0, 6,CORE_SEG8D},
  {12, 0,32, 6,CORE_SEG8D},
  {0}
};

ROM_START(spectra)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("spect_u3.dat", 0x0c00, 0x0400, CRC(9ca7510f) SHA1(a87849f16903836158063d593bb4a2e90c7473c8))
      ROM_RELOAD(0xfc00, 0x0400)
    ROM_LOAD("spect_u4.dat", 0x0800, 0x0400, CRC(15e53712) SHA1(e03049178569313cb89cfe0f09043c21d05b1988))
    ROM_LOAD("spect_u5.dat", 0x0400, 0x0400, CRC(49e0759f) SHA1(c3badc90ff834cbc92d8c519780069310c2b1507))
ROM_END

INITGAME(spectra,dispAlpha,FLIP_SW(FLIP_L))
SPECTRA_COMPORTS(spectra, 1)
CORE_GAMEDEFNV(spectra, "Spectra IV", 1979, "Valley", spectra, 0)
