/******************************************************************************************
  Nuova Bell Games
  ----------------
  by Steve Ellenoff and Gerrit Volkenborn

  Main CPU Board:
  Bally-35. Turned out we can reuse the same code all the way...
  A few games use alpha displays, this is implemented in by35.c

  Issues/Todo:
  - Skill Flight schematics show an additional TMS5220C speech chip which is not used?!
  - U-Boat 65 has an additional music board, no info available
*******************************************************************************************/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/i8051/i8051.h"
#include "machine/6821pia.h"
#include "sound/tms5220.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "by35.h"
#include "by35snd.h"

// For later models, the CMOS is bigger than on normal Bally machines,
// and even occupies the same RAM area as the PIAs!
static UINT8 *nuova_CMOS;
static NVRAM_HANDLER(nuova) { core_nvram(file, read_or_write, nuova_CMOS, 0x800, 0xff); }
static WRITE_HANDLER(nuova_CMOS_w) { nuova_CMOS[offset] = data | 0x0f; }

static MEMORY_READ_START(nuova_readmem)
  { 0x0000, 0x007f, MRA_RAM }, /* Internal 128 Byte Ram, this needs all 8 bits */
  { 0x0088, 0x008b, pia_r(0) }, /* U9 PIA: Switches + Display + Lamps */
  { 0x0090, 0x0093, pia_r(1) }, /* U10 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0000, 0x07ff, MRA_RAM }, /* U6 2K CMOS Battery Backed, overlaps other areas */
  { 0x1000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(nuova_writemem)
  { 0x0000, 0x007f, MWA_RAM }, /* Internal 128 Byte Ram, this needs all 8 bits */
  { 0x0088, 0x008b, pia_w(0) }, /* U9 PIA: Switches + Display + Lamps */
  { 0x0090, 0x0093, pia_w(1) }, /* U10 PIA: Solenoids/Sounds + Display Strobe */
  { 0x0000, 0x07ff, nuova_CMOS_w, &nuova_CMOS }, /* U6 2K CMOS Battery Backed, overlaps other areas */
MEMORY_END


// sound section

static struct {
  struct sndbrdData brdData;
  UINT8 sndCmd;
  UINT8 pia_a, pia_b;
  int mute, mute2, enable, enable2;
} locals;

static READ_HANDLER(nuova_pia_a_r) { return locals.pia_a; }
static WRITE_HANDLER(nuova_pia_a_w) { locals.pia_a = data; }
static WRITE_HANDLER(nuova_pia_b_w) {
  if (~data & 0x01) // read
    locals.pia_a = tms5220_status_r(0);
  else if (~data & 0x02) // write
    tms5220_data_w(0, locals.pia_a);
  pia_set_input_ca2(2, tms5220_ready_r());
  locals.pia_b = data;
}
static READ_HANDLER(nuova_pia_cb1_r) {
  return tms5220_int_r();
}
static READ_HANDLER(nuova_pia_ca2_r) {
  return tms5220_ready_r();
}
static void nuova_irq(int state) {
  cpu_set_irq_line(1, M6803_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct pia6821_interface nuova_pia = {
  /*i: A/B,CA/B1,CA/B2 */ nuova_pia_a_r, 0, PIA_UNUSED_VAL(1), nuova_pia_cb1_r, nuova_pia_ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ nuova_pia_a_w, nuova_pia_b_w, 0, 0,
  /*irq: A/B           */ nuova_irq, nuova_irq
};

void tx_cb(int data);
int rx_cb(void);

static void nuova_init(struct sndbrdData *brdData) {
  memset(&locals, 0x00, sizeof(locals));
  locals.brdData = *brdData;

  if (!_strnicmp(Machine->gamedrv->name, "skflight", 8)) {
    memset(memory_region(REGION_CPU2), 0xff, 0x100);
    pia_config(2, PIA_STANDARD_ORDERING, &nuova_pia);
    tms5220_reset();
    tms5220_set_variant(TMS5220_IS_5220C);
  }

  if (!_strnicmp(Machine->gamedrv->name, "uboat65", 7)) {
    //Setup serial line callbacks, needs to be set before CPU reset by design!
    i8051_set_serial_tx_callback(tx_cb);
    i8051_set_serial_rx_callback(rx_cb);
  }
}

static void nuova_diag(int button) {
  cpu_set_nmi_line(locals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(nuova_data_w) {
  locals.sndCmd = data & 0x0f;
}

static WRITE_HANDLER(nuova_ctrl_w) {
  if (data & 1) {
    cpu_boost_interleave(TIME_IN_USEC(4), TIME_IN_USEC(500));
    cpu_set_irq_line(1, M6803_TIN_LINE, PULSE_LINE);
    if (!_strnicmp(Machine->gamedrv->name, "f1gp", 4)) {
      cpu_set_irq_line(2, M6803_TIN_LINE, PULSE_LINE);
    }
  }
  if (!_strnicmp(Machine->gamedrv->name, "skflight", 8)) {
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x01) | ((data & 1) << 1);
  }
}

// The Nuova Bell sound board needs two nybbles sent in perfect sync with the main CPU!
static WRITE_HANDLER(nuova_man_w) {
  int i;
  nuova_ctrl_w(0, 1);
  nuova_data_w(0, data >> 4);
  for (i = 0; i < 50; i++)
    run_one_timeslice();
  nuova_data_w(0, data);
}

static WRITE_HANDLER(dac_w) {
  if (!locals.mute) DAC_0_data_w(0, data);
}

static WRITE_HANDLER(enable_w) {
  locals.enable = data & 0x10 ? 0 : 1;
  logerror("enable:%d\n", locals.enable);
}

static WRITE_HANDLER(bank_w) {
  static int lastBank;
  int bank;
  if (locals.enable) {
    bank = core_BitColToNum((~data & 0x0f) ^ 0x01);
    cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000 * bank);
    if (bank) {
      if (bank != lastBank)
        logerror("bank:%d\n", bank);
      lastBank = bank;
    }
    locals.mute = (data >> 6) & 1;
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x05) | ((~data >> 7) << 1);
  }
}

static READ_HANDLER(snd_cmd_r) {
  return ((~locals.sndCmd & 0x07) << 1) | ((~locals.sndCmd & 0x08) >> 3);
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },
  { 0x8000, 0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },
  { 0x8000, 0xffff, bank_w },
MEMORY_END

static PORT_READ_START(snd_readport)
  { M6803_PORT2, M6803_PORT2, snd_cmd_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { M6803_PORT1, M6803_PORT1, dac_w },
  { M6803_PORT2, M6803_PORT2, enable_w },
PORT_END

static struct DACinterface nuova_dacInt = { 1, { 30 }};

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf nuovaIntf = {
  "NUOVA", nuova_init, NULL, nuova_diag, nuova_man_w, nuova_data_w, NULL, nuova_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(nuova)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu", M6800, 530000)
  MDRV_CPU_MEMORY(nuova_readmem, nuova_writemem)
  MDRV_NVRAM_HANDLER(nuova)
  MDRV_DIAGNOSTIC_LEDH(2)
  // CPU clock adjusted to fit recorded live sample
  MDRV_CPU_ADD_TAG("scpu", M6803, 3579545/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_SOUND_ADD(DAC, nuova_dacInt)
MACHINE_DRIVER_END

// some games have a higher main CPU clock (soundboard needs data bits accurately clocked in)
MACHINE_DRIVER_START(nuovaFast)
  MDRV_IMPORT_FROM(nuova)
  MDRV_CPU_REPLACE("mcpu", M6800, 5000000/4) // guessed, seems to work OK for all 3 games using it
MACHINE_DRIVER_END

// games below

#define INITGAME(name, gen, disp, flip, lamps, sb, gs1) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,0,BY35GD_NOSOUNDE | gs1}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMENB(name, gen, disp, flip, lamps, sb, gs1) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,0,BY35GD_FAKEZERO | gs1}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEAL(name, gen, disp, flip, lamps, sb, gs1) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,0,BY35GD_ALPHA | gs1}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

static core_tLCDLayout dispNB[] = {
  {0, 0, 2,7,CORE_SEG87F}, {0,16,10,7,CORE_SEG87F},
  {2, 0,18,7,CORE_SEG87F}, {2,16,26,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG7},   {4,10,38,2,CORE_SEG7},{0}
};
static core_tLCDLayout dispBy7[] = {
  {0, 0, 1,7,CORE_SEG87F}, {0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F}, {2,16,25,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG7},   {4,10,38,2,CORE_SEG7},{0}
};
static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0,16,CORE_SEG16S},
  {4, 0,16,16,CORE_SEG16S},
  {0}
};

/*--------------------------------
/ Super Bowl (X's & O's Clone without aux lamps board)
/-------------------------------*/
INITGAMENB(suprbowl,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_BY51N,BY35GD_NOSOUNDE)
BY35_ROMSTARTx00(suprbowl,"sbowlu2.732", CRC(bc497a13) SHA1(f428373bde72f0302c45c326aebbe56e8b09c2d6),
                          "sbowlu6.732", CRC(a9c92719) SHA1(972da0cf87863b637b88575c329f1d8162098d6f))
BY56_SOUNDROM(            "suprbowl.snd",CRC(97fc0f7a) SHA1(595aa080a6d2c1ab7e718974c4d01e846e142cc1))
BY35_ROMEND
BY35_INPUT_PORTS_START(suprbowl, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(suprbowl,"Super Bowl",1984,"Bell Games",by35_mBY35_51NS,0)

/*--------------------------------
/ Super Bowl (Free Play) (uses a modified -53 version of Bally U6 ROM)
/-------------------------------*/
INITGAMENB(sprbwlfp,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_BY51N,BY35GD_NOSOUNDE)
BY35_ROMSTARTx00(sprbwlfp,"sbwlfpu2.732",CRC(94be32b4) SHA1(a20d645ab48b58cc5e009aa0ba39172b1a2e98e7),
                          "sbwlfpu6.732",CRC(691db61b) SHA1(270b63d6945f29d5fb3086e7a14dff69b7d310e0))
BY56_SOUNDROM(            "suprbowl.snd",CRC(97fc0f7a) SHA1(595aa080a6d2c1ab7e718974c4d01e846e142cc1))
BY35_ROMEND
BY35_INPUT_PORTS_START(sprbwlfp, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(sprbwlfp,suprbowl,"Super Bowl (Free Play)",2018,"Bell Games / Quench",by35_mBY35_51NS,0)

/*--------------------------------
/ Tiger Rag (Kings Of Steel Clone)
/-------------------------------*/
ROM_START(tigerrag)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("tigerrag.mpu", 0xe000, 0x2000, CRC(3eb389ba) SHA1(bdfdcf00f4a2200d39d7e469fe633e0b7b8f1676))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROM11("kngsu3.snd", CRC(11b02dca) SHA1(464eee1aa1fd9b6e26d4ba635777fffad0222106),
                "kngsu4.snd", CRC(f3e4d2f6) SHA1(93f4e9e1348b1225bc02db38c994e3338afb175c))
ROM_END
INITGAMENB(tigerrag,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_INPUT_PORTS_START(tigerrag, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(tigerrag,kosteel,"Tiger Rag",1984,"Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Cosmic Flash (ROMs almost the same as Flash Gordon but different gameplay)
/-------------------------------*/
INITGAMENB(cosflash,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(cosflash,"cf2d.532",    CRC(939e941d) SHA1(889862043f351762e8c866aefb36a9ea75cbf828),
                          "cf6d.532",    CRC(7af93d2f) SHA1(2d939b14f7fe79f836e12926f44b70037630cd3f))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e) SHA1(ecdeb07c31c22ec313b55774f4358a9923c5e9e7),
                          "834-18_5.532",CRC(8799e80e) SHA1(f255b4e7964967c82cfc2de20ebe4b8d501e3cb0))
BY35_ROMEND
BY35_INPUT_PORTS_START(cosflash, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(cosflash,"Cosmic Flash",1985,"Bell Games",by35_mBY35_61S,0)

/*--------------------------------
/ Saturn 2 (Spy Hunter Clone)
/-------------------------------*/
INITGAMENB(saturn2,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(saturn2,"spy-2732.u2",CRC(9e930f2d) SHA1(fb48ce0d8d8f8a695827c0eea57510b53daa7c39),
                         "saturn2.u6", CRC(ca72a96b) SHA1(efcd8b41bf0c19ebd7db492632e046b348619460))
BY45_SOUNDROMx2("sat2_snd.764", CRC(6bf15541) SHA1(dcdd4e8f662818584de9b1ed7ae69d57362ebadb))
BY35_ROMEND
BY35_INPUT_PORTS_START(saturn2, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(saturn2,spyhuntr,"Saturn 2",1985,"Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ New Wave (Black Pyramid Clone)
/-------------------------------*/
INITGAMENB(newwave,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(newwave,"blkp2732.u2",CRC(600535b0) SHA1(33d080f4430ad9c33ee9de1bfbb5cfde50f0776e),
                         "newwu6.532", CRC(ca72a96b) SHA1(efcd8b41bf0c19ebd7db492632e046b348619460))
BY45_SOUNDROM11(         "bp_u3.532",  CRC(a5005067) SHA1(bd460a20a6e8f33746880d72241d6776b85126cf),
                         "newwu4.532", CRC(6f4f2a95) SHA1(a7a375827c0429b8b3d2ee9e471f557152492993))
BY35_ROMEND
BY35_INPUT_PORTS_START(newwave, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(newwave,blakpyra,"New Wave",1985,"Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ World Defender
/-------------------------------*/
ROM_START(worlddef)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("worlddef.764", 0xe000, 0x2000, CRC(ad1a7ba3) SHA1(d799b6d1cd252cd6d9fb72586099c43de7c22a00))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROMx2("wodefsnd.764", CRC(b8d4dc20) SHA1(5aecac4a2deb7ea8e0ff0600ea459ef272dcd5f0))
ROM_END
INITGAMENB(worlddef,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_BY45,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(worlddef, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(worlddef,"World Defender",1985,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ World Defender (Free Play)
/-------------------------------*/
ROM_START(worlddfp)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("worlddfp.764", 0xe000, 0x2000, CRC(233ddce8) SHA1(9b0d3906d95407b7ce7a5758381f3f9dbce912cc))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROMx2("wodefsnd.764", CRC(b8d4dc20) SHA1(5aecac4a2deb7ea8e0ff0600ea459ef272dcd5f0))
ROM_END
INITGAMENB(worlddfp,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_BY45,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(worlddfp, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(worlddfp,worlddef,"World Defender (Free Play)",1985,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Space Hawks
/-------------------------------*/
ROM_START(spacehaw)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("spacehaw.bin", 0xe000, 0x2000, CRC(f070b2c3) SHA1(405d959e6e3976d50470594deadd3e3625fc1791))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROM2x("sh_sound.264", CRC(2b548d24) SHA1(83ac9b75ae9c1960ad73abcf40adc2bc46827568))
ROM_END
INITGAMENB(spacehaw,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(spacehaw, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(spacehaw,"Space Hawks",1986,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Space Hawks (Free Play) (dips 25 & 26 must be on)
/-------------------------------*/
ROM_START(spchawfp)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("spacehfp.rom", 0xe000, 0x2000, CRC(7369638d) SHA1(6ad5a60aea18752dc4083c9e41278b173584c173))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROM2x("sh_sound.264", CRC(2b548d24) SHA1(83ac9b75ae9c1960ad73abcf40adc2bc46827568))
ROM_END
INITGAMENB(spchawfp,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(spchawfp, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(spchawfp,spacehaw,"Space Hawks (Free Play)",1986,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Dark Shadow
/-------------------------------*/
ROM_START(darkshad)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7.bin", 0xe000, 0x2000, CRC(8d04c546) SHA1(951e75d9867b85a0bf9f04fe9aa647a53b6830bc))
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x0800)
    ROM_COPY(REGION_CPU1, 0xe800, 0x1800,0x0800)
    ROM_COPY(REGION_CPU1, 0xf000, 0x5000,0x0800)
    ROM_COPY(REGION_CPU1, 0xf800, 0x5800,0x0800)
BY45_SOUNDROM2x("darkshad.snd", CRC(9fd6ee82) SHA1(6486fa56c663152e565e160b8f517be824338a9a))
ROM_END
INITGAME(darkshad,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_INPUT_PORTS_START(darkshad, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(darkshad,"Dark Shadow",1986,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Skill Flight
/-------------------------------*/
// gv 08/08/18: non-inverted sound command, bank swapping using P24, enable not used, more RAM, extra speech board

static READ_HANDLER(snd_cmd_r_skflight) {
  return (locals.sndCmd & 0x0f) << 1;
}

static WRITE_HANDLER(bank_w_skflight) {
  int bank = data & 0x10 ? 0 : 1;
  logerror("bank:%d\n", bank);
  cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000 * bank);
}

static MEMORY_READ_START(snd_readmem_skflight)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x4000, 0x4003, pia_r(2) },
  { 0x8000, 0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem_skflight)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0000, 0x00ff, MWA_RAM },
  { 0x4000, 0x4003, pia_w(2) },
  { 0x8000, 0xffff, MWA_NOP },
MEMORY_END

static PORT_READ_START(snd_readport_skflight)
  { M6803_PORT2, M6803_PORT2, snd_cmd_r_skflight },
PORT_END

static PORT_WRITE_START(snd_writeport_skflight)
  { M6803_PORT1, M6803_PORT1, dac_w },
  { M6803_PORT2, M6803_PORT2, bank_w_skflight },
PORT_END

static void nuova_5220Irq(int state) { pia_set_input_cb1(2, state); }
static void nuova_5220Rdy(int state) { pia_set_input_ca2(2, state); }
static struct TMS5220interface skflight_tms5220Int = { 640000, 75, nuova_5220Irq, nuova_5220Rdy };

MACHINE_DRIVER_START(skflight)
  MDRV_IMPORT_FROM(nuova)
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(snd_readmem_skflight, snd_writemem_skflight)
  MDRV_CPU_PORTS(snd_readport_skflight, snd_writeport_skflight)

  MDRV_SOUND_ADD(TMS5220, skflight_tms5220Int)
MACHINE_DRIVER_END

ROM_START(skflight)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game_u7.64", 0xe000, 0x2000, CRC(fe5001eb) SHA1(f7d56d484141ba8ec82664b6aebbf3a683547d20))
    ROM_LOAD("game_u8.64", 0xc000, 0x2000, CRC(58f259fe) SHA1(505f3996f66dbb4027bd47f6b7ba9e4baaeb6e51))
    ROM_COPY(REGION_CPU1, 0xc000, 0x9000,0x1000)
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x1000)
    ROM_COPY(REGION_CPU1, 0xf000, 0x5000,0x1000)

  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("snd_u3.256", 0x0000, 0x8000, CRC(43424fb1) SHA1(428d2f7444cd71b6c49c04749b42263e3c185856))
    ROM_LOAD("snd_u4.256", 0x8000, 0x8000, CRC(10378feb) SHA1(5da2b9c530167c80b9d411da159e4b6e95b76647))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x8000, 0x8000,0x8000)
ROM_END

INITGAME(skflight,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(skflight, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(skflight, "Skill Flight", 1986, "Nuova Bell Games", skflight, 0)

/*--------------------------------
/ Cobra
/-------------------------------*/
ROM_START(cobra)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7.256", 0xc000, 0x4000, CRC(c0f89577) SHA1(16d351f2bf642bf886e808b58173b3e699a44fd6))
    ROM_COPY(REGION_CPU1, 0xc000, 0x1000,0x1000)
    ROM_COPY(REGION_CPU1, 0xd000, 0x5000,0x1000)
    ROM_COPY(REGION_CPU1, 0xe000, 0x9000,0x1000)
    ROM_COPY(REGION_CPU1, 0xf000, 0xd000,0x1000)

  NORMALREGION(0x20000, REGION_SOUND1)
    ROM_LOAD("snd_u8.256", 0x00000,0x8000, CRC(cdf2a28d) SHA1(d4969370109b4c7f31f48a3ebd8925268caf9c44))
    ROM_LOAD("snd_u9.256", 0x08000,0x8000, CRC(08bd0db9) SHA1(af851b8c993649b61645a414459000c206516bec))
    ROM_LOAD("snd_u10.256",0x10000,0x8000, CRC(634bc64c) SHA1(8389fda08ee7bf0e5002153cec22e219bf786993))
    ROM_LOAD("snd_u11.256",0x18000,0x8000, CRC(d4da383c) SHA1(032a4a425936d5c822fba6e46483f03a87c1a6ec))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAME(cobra,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(cobra, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(cobra, "Cobra", 1987, "Nuova Bell Games", nuovaFast, 0)

/*--------------------------------
/ Future Queen
/-------------------------------*/
ROM_START(futrquen)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("mpu_u2.bin", 0xc000, 0x4000, CRC(bc66b636) SHA1(65f3e6461a1eca8542bbbc5b8c7cd1fca1b3011f))
  ROM_COPY(REGION_CPU1, 0xc000, 0x1000,0x0800)
  ROM_COPY(REGION_CPU1, 0xd000, 0x1800,0x0800)
  ROM_COPY(REGION_CPU1, 0xc800, 0x5000,0x0800)
  ROM_COPY(REGION_CPU1, 0xd800, 0x5800,0x0800)
  ROM_COPY(REGION_CPU1, 0xe000, 0x9000,0x0800)
  ROM_COPY(REGION_CPU1, 0xf000, 0x9800,0x0800)
  ROM_COPY(REGION_CPU1, 0xe800, 0xd000,0x0800)
  ROM_COPY(REGION_CPU1, 0xf800, 0xd800,0x0800)

  NORMALREGION(0x20000, REGION_SOUND1)
    ROM_LOAD("snd_u8.bin", 0x00000,0x8000, CRC(3d254d89) SHA1(2b4aa3387179e2c0fbf18684128761d3f778dcb2))
    ROM_LOAD("snd_u9.bin", 0x08000,0x8000, CRC(9560f2c3) SHA1(3de6d074e2a3d3c8377fa330d4562b2d266bbfff))
    ROM_LOAD("snd_u10.bin",0x10000,0x8000, CRC(70f440bc) SHA1(9fa4d33cc6174ce8f43f030487171bfbacf65537))
    ROM_LOAD("snd_u11.bin",0x18000,0x8000, CRC(71d98d17) SHA1(9575b80a91a67b1644e909f70d364e0a75f73b02))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMENB(futrquen,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(futrquen, 2) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(futrquen, "Future Queen", 1987, "Nuova Bell Games", nuova, 0)

/*--------------------------------
/ F1 Grand Prix
/-------------------------------*/
//U6 Ram Notes
// A0-A10 Used (0x800) size - accessible during E-cycle at any address?

//U7 Rom Notes
//
// The schematic has got to be wrong for a variety of reasons, but luckily, I found the startup
//
// After finally finding the right map to get the cpu started
// -Watching the rom boot up it actually tests different sections of rom, which is a huge help!
// - I have no idea how this actually makes any sense!
// Check - Data @ 0x1000 = 0x1A (found @ 0000)
// Check - Data @ 0x5000 = 0x55 (found @ 1000) (A14->A12? or not used?)
// Check - Data @ 0x7000 = 0x7A (found @ 5000) (A13 = 0?)
// Check - Data @ 0x9000 = 0x95 (found @ 2000) (A15 = 0, A12 -> A13)
// Check - Data @ 0xB000 = 0xBA (found @ 6000) (A15->A13, A12->A14, A14->A12)
// Check - Data @ 0xD000 = 0xD5 (found @ 3000) (A15->A13, A14=0)
// Check - Data @ 0xF000 = 0xFA (found @ 7000) (A15->A13, A12->A14, A14->A12)

// gv 08/02/18: f1gp uses two complete soundboards, one for fx and one for music!

static WRITE_HANDLER(bank_w_f1gp) {
  static int lastBank;
  int bank;
  if (locals.enable2) {
    bank = core_BitColToNum((~data & 0x0f) ^ 0x01);
    cpu_setbank(2, memory_region(REGION_SOUND2) + 0x8000 * bank);
    if (bank) {
      if (bank != lastBank)
        logerror("bank2:%d\n", bank);
      lastBank = bank;
    }
    locals.mute2 = (data >> 6) & 1;
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x03) | ((~data >> 7) << 2);
  }
}

static WRITE_HANDLER(dac_w_f1gp) {
  if (!locals.mute2) DAC_1_data_w(0, data);
}

static WRITE_HANDLER(enable_w_f1gp) {
  locals.enable2 = data & 0x10 ? 0 : 1;
  logerror("enable2:%d\n", locals.enable2);
}

static MEMORY_READ_START(snd_readmem_f1gp)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },
  { 0x8000, 0xffff, MRA_BANKNO(2) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem_f1gp)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },
  { 0x8000, 0xffff, bank_w_f1gp },
MEMORY_END

static PORT_WRITE_START(snd_writeport_f1gp)
  { M6803_PORT1, M6803_PORT1, dac_w_f1gp },
  { M6803_PORT2, M6803_PORT2, enable_w_f1gp },
PORT_END

static struct DACinterface nuova_dacInt2 = { 2, { 30, 50 }};

MACHINE_DRIVER_START(f1gp)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_REPLACE("mcpu", M6800, 5000000/4) // faster clock needed, see above
  MDRV_CPU_MEMORY(nuova_readmem, nuova_writemem)
  MDRV_NVRAM_HANDLER(nuova)
  MDRV_DIAGNOSTIC_LEDH(3)

  MDRV_CPU_ADD_TAG("scpu", M6803, 1000000) // faster clock to match sound samples
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)

  MDRV_CPU_ADD_TAG("scpu2", M6803, 1000000) // faster clock to match sound samples
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem_f1gp, snd_writemem_f1gp)
  MDRV_CPU_PORTS(snd_readport, snd_writeport_f1gp)

  MDRV_SOUND_ADD(DAC, nuova_dacInt2)
MACHINE_DRIVER_END

ROM_START(f1gp)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7", 0x8000, 0x8000, CRC(2287dea1) SHA1(5438752bf63aadaa6b6d71bbf56a72d8b67b545a))
  ROM_COPY(REGION_CPU1, 0x8000, 0x1000,0x1000)
  ROM_COPY(REGION_CPU1, 0x9000, 0x5000,0x1000)
  ROM_COPY(REGION_CPU1, 0xd000, 0x7000,0x1000)
  ROM_COPY(REGION_CPU1, 0xa000, 0x9000,0x1000)
  ROM_COPY(REGION_CPU1, 0xb000, 0xd000,0x1000)
  ROM_COPY(REGION_CPU1, 0xe000, 0xb000,0x1000)

  NORMALREGION(0x20000, REGION_SOUND1)
    ROM_LOAD("snd_u8a", 0x00000,0x8000, CRC(3a2af90b) SHA1(f6eeae74b3bfb1cfd9235c5214f7c029e0ad14d6))
    ROM_LOAD("snd_u9a", 0x08000,0x8000, CRC(681ee99c) SHA1(955cd782073a1ce0be7a427c236d47fcb9cccd20))
    ROM_LOAD("snd_u10a",0x10000,0x8000, CRC(4d3fc9bb) SHA1(d43cd134f399e128a678b86e57b1917fad70df76))
    ROM_LOAD("snd_u11a",0x18000,0x8000, CRC(884dc754) SHA1(b121476ea621eae7a7ba0b9a1b5e87051e1e9e3d))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)

  NORMALREGION(0x20000, REGION_SOUND2)
    ROM_LOAD("snd_u8b", 0x00000,0x8000, CRC(14cddb29) SHA1(667b54174ad5dd8aa45037574916ecb4ee996a94))
    ROM_LOAD("snd_u9b", 0x08000,0x8000, CRC(726920b5) SHA1(002e7a072a173836c89746cceca7e5d2ac26356d))
    ROM_LOAD("snd_u10b",0x10000,0x8000, CRC(9de359fb) SHA1(ce75a78dc4ed747421a386d172fa0f8a1369e860))
    ROM_LOAD("snd_u11b",0x18000,0x8000, CRC(2394b498) SHA1(bf0884a6556a27791e7e801051be5975dd6b95c4))
  NORMALREGION(0x10000, REGION_CPU3)
  ROM_COPY(REGION_SOUND2, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMEAL(f1gp,GEN_BY35,dispAlpha,FLIP_SWNO(48,0),8,SNDBRD_NUOVA,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(f1gp, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(f1gp, "F1 Grand Prix", 1987, "Nuova Bell Games", f1gp, 0)

/*--------------------------------
/ Top Pin
/-------------------------------*/
// gv 08/09/18: different bank swapping, enable not used

static WRITE_HANDLER(bank_w_toppin) {
  static int swap[16] = { 0, 3, 5, 0, 7, 0, 0, 0, 0, 2, 4, 0, 6, 0, 0, 0 };
  int bank = swap[(~data & 0x1e) >> 1];
  logerror("bank:%d\n", bank);
  cpu_setbank(1, memory_region(REGION_SOUND1) + 0x4000 * bank);
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x01) | ((data >> 7) << 1);
}

static MEMORY_READ_START(snd_readmem_toppin)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },
  { 0x8000, 0xbfff, MRA_BANKNO(1) },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem_toppin)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },
  { 0x8000, 0xffff, bank_w_toppin },
MEMORY_END

MACHINE_DRIVER_START(toppin)
  MDRV_IMPORT_FROM(nuova)
  MDRV_CPU_REPLACE("scpu", M6803, 1000000) // faster clock to match sound samples
  MDRV_CPU_MEMORY(snd_readmem_toppin, snd_writemem_toppin)
MACHINE_DRIVER_END

ROM_START(toppin)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_256.bin", 0xc000, 0x4000, CRC(3aa32c96) SHA1(989fdc642efe6fa41319d7ccae6681ab4d76feb4))
  ROM_COPY(REGION_CPU1, 0xc000, 0x1000,0x0800)
  ROM_COPY(REGION_CPU1, 0xd000, 0x1800,0x0800)
  ROM_COPY(REGION_CPU1, 0xc800, 0x5000,0x0800)
  ROM_COPY(REGION_CPU1, 0xd800, 0x5800,0x0800)
  ROM_COPY(REGION_CPU1, 0xe000, 0x9000,0x0800)
  ROM_COPY(REGION_CPU1, 0xf000, 0x9800,0x0800)
  ROM_COPY(REGION_CPU1, 0xe800, 0xd000,0x0800)
  ROM_COPY(REGION_CPU1, 0xf800, 0xd800,0x0800)

  NORMALREGION(0x20000, REGION_SOUND1)
    ROM_LOAD("snd_u8.bin", 0x00000,0x8000, CRC(2cb9c931) SHA1(2537976c890ceff857b9aaf204c48ab014aad94e))
    ROM_LOAD("snd_u9.bin", 0x08000,0x8000, CRC(72690344) SHA1(c2a13aa59f0c605eb616256cd288b79cceca003b))
    ROM_LOAD("snd_u10.bin",0x10000,0x8000, CRC(bca9a805) SHA1(0deb3172b5c8fc91c4b02b21b1e3794ed7adef13))
    ROM_LOAD("snd_u11.bin",0x18000,0x8000, CRC(513d06a9) SHA1(3785398649fde5579b5a0461b52360ef83d71323))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMENB(toppin,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_NUOVA,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(toppin, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(toppin, "Top Pin", 1988, "Nuova Bell Games", toppin, 0)

/*--------------------------------
/ U-Boat 65 - additional "CSC 387.1" music board with 8032 MCU
/-------------------------------*/

static MEMORY_READ_START(snd_readmem2)
  { 0x00000, 0x0ffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem2)
MEMORY_END

static READ_HANDLER(port_r) {
  static UINT8 val, cnt;
  cnt++;
  if (cnt % 32 == 0) val++;
//printf("%02x ", val);
  // no idea what enables the internal timers, but they need to remain off!
  if (i8752_internal_r(0x80 + TCON) & 0x50) {
    i8752_internal_w(0x80 + TCON, i8752_internal_r(0x80 + TCON) & 0x0f);
  }
  switch (offset) {
    case 2: logerror(" port  2 R\n"); break;
    case 3: logerror("  port 3 R\n"); break;
  }
  return offset == 3 ? val : 0;
}

static WRITE_HANDLER(port_w) {
  // no idea what enables the internal timers, but they need to remain off!
  if (i8752_internal_r(0x80 + TCON) & 0x50) {
    i8752_internal_w(0x80 + TCON, i8752_internal_r(0x80 + TCON) & 0x0f);
  }
//printf("snd data: %x: %02x\n", offset, data);
  switch (offset) {
    case 1: logerror("port   1 W %02x\n", data); break;
    case 2: logerror(" port  2 W %02x\n", data); break;
    case 3: logerror("  port 3 W %02x\n", data); break;
  }
}

static PORT_READ_START(snd_readport2)
  { 0, 3, port_r },
PORT_END

static PORT_WRITE_START(snd_writeport2)
  { 0, 3, port_w },
PORT_END

void tx_cb(int data) {
//printf("TX:%02x ", data);
}

int rx_cb(void) {
  static UINT8 byte;
  byte++;
//printf("RX:%02x ", byte);
  return byte;
}

static void serial(int data) {
  static int rx;
  rx = !rx;
  cpu_set_irq_line(2, I8051_RX_LINE, rx ? ASSERT_LINE : CLEAR_LINE);
}

static void tf0(int data) {
  static int timeout;
  timeout = !timeout;
  cpu_set_irq_line(2, I8051_T0_LINE, timeout ? ASSERT_LINE : CLEAR_LINE);
  if (timeout)
    i8752_internal_w(0x80 + IE, i8752_internal_r(0x80 + IE) & 0x7f); // prevent all irqs temporarily
  else
    i8752_internal_w(0x80 + IE, i8752_internal_r(0x80 + IE) | 0x80); // reenable irqs
}

static void int1(int data) {
  static int irq;
  irq = !irq;
  cpu_set_irq_line(2, I8051_INT1_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(odd) {
  // nothing done here, but sound is more accurate once this callback is in place!?
}

MACHINE_DRIVER_START(uboat)
  MDRV_IMPORT_FROM(nuovaFast)
  MDRV_CPU_REPLACE("scpu", M6803, 1000000) // faster clock to match sound samples

  MDRV_CPU_ADD_TAG("scpu2", I8752, 12000000) // I8032 actually (no internal ROM, 256 bytes internal RAM)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem2, snd_writemem2)
  MDRV_CPU_PORTS(snd_readport2, snd_writeport2)
  MDRV_CPU_PERIODIC_INT(odd, 500)
  MDRV_TIMER_ADD(int1, 200)
  MDRV_TIMER_ADD(tf0, 100)
  MDRV_TIMER_ADD(serial, 10)
MACHINE_DRIVER_END

ROM_START(uboat65)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7.256", 0x8000, 0x8000, CRC(f0fa1cbc) SHA1(4373bb37927dde01f5a4da5ef6094424909e9bc6))
  ROM_COPY(REGION_CPU1, 0x8000, 0x1000,0x1000)
  ROM_COPY(REGION_CPU1, 0x9000, 0x5000,0x1000)
  ROM_COPY(REGION_CPU1, 0xd000, 0x7000,0x1000)
  ROM_COPY(REGION_CPU1, 0xa000, 0x9000,0x1000)
  ROM_COPY(REGION_CPU1, 0xb000, 0xd000,0x1000)
  ROM_COPY(REGION_CPU1, 0xe000, 0xb000,0x1000)

  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("snd_u8.bin", 0x8000, 0x8000, CRC(d00fd4fd) SHA1(23f6b7c5d60821eb7fa2fdcfc85caeb536eef99a))
  NORMALREGION(0x20000, REGION_CPU3)
    ROM_LOAD("snd_ic3.256", 0x10000, 0x4000, CRC(c7811983) SHA1(7924248dcc08b05c34d3ddf2e488b778215bc7ea))
      ROM_CONTINUE(0x00000, 0x4000)
    ROM_LOAD("snd_ic5.256", 0x14000, 0x4000, CRC(bc35e5cf) SHA1(a809b0056c576416aa76ead0437e036c2cdbd1ef))
      ROM_CONTINUE(0x04000, 0x4000)
ROM_END

INITGAMEAL(uboat65,GEN_BY35,dispAlpha,FLIP_SWNO(12,5),0,SNDBRD_NUOVA,BY35GD_NOSOUNDE)
BY35_INPUT_PORTS_START(uboat65, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(uboat65, "U-Boat 65", 1988, "Nuova Bell Games", uboat, GAME_IMPERFECT_SOUND)
