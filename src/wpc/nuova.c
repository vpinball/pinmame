/******************************************************************************************
  Nuova Bell Games
  ----------------
  by Steve Ellenoff and Gerrit Volkenborn

  Main CPU Board:
  Bally-35. Turned out we can reuse the same code all the way...
  A few games use alpha displays, this is implemented in by35.c

  Issues/Todo:
  Sound Board with 6803 CPU and DAC, some games also use an additional TMS5220 speech chip.
*******************************************************************************************/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
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
  UINT8 sndCmd, latch[2];
  UINT8 pia_a, pia_b;
  int bank, LED, mute, enable;
} locals;

static READ_HANDLER(nuova_pia_a_r) { return locals.pia_a; }
static WRITE_HANDLER(nuova_pia_a_w) { locals.pia_a = data; }
static WRITE_HANDLER(nuova_pia_b_w) {
  if (~data & 0x02) // write
    tms5220_data_w(0, locals.pia_a);
  if (~data & 0x01) // read
    locals.pia_a = tms5220_status_r(0);
  pia_set_input_ca2(2, 1);
  locals.pia_b = data;
}
static READ_HANDLER(nuova_pia_cb1_r) {
  return !tms5220_int_r();
}
static READ_HANDLER(nuova_pia_ca2_r) {
  return !tms5220_ready_r();
}
static void nuova_irq(int state) {
  cpu_set_irq_line(1, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void nuova_5220Irq(int state) { pia_set_input_cb1(2, !state); }

static const struct pia6821_interface nuova_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ nuova_pia_a_r, 0, PIA_UNUSED_VAL(1), nuova_pia_cb1_r, nuova_pia_ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ nuova_pia_a_w, nuova_pia_b_w, 0, 0,
  /*irq: A/B           */ nuova_irq, nuova_irq
}};

static void nuova_init(struct sndbrdData *brdData) {
  memset(&locals, 0x00, sizeof(locals));
  locals.brdData = *brdData;
  pia_config(2, PIA_STANDARD_ORDERING, &nuova_pia[0]);
}

static void nuova_diag(int button) {
  cpu_set_nmi_line(locals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(nuova_data_w) {
  locals.sndCmd = (locals.sndCmd & 0xf0) | (data & 0x0f);
}

static WRITE_HANDLER(nuova_ctrl_w) {
  locals.sndCmd = (locals.sndCmd & 0x0f) | (data << 4);
}

static WRITE_HANDLER(nuova_man_w) {
  locals.sndCmd = data;
}

static WRITE_HANDLER(dac_w) {
  if (!locals.mute) DAC_0_data_w(0, data);
}

static WRITE_HANDLER(enable_w) {
  locals.enable = data & 0x10 ? 0 : 1;
  logerror("enable:%d\n", locals.enable);
}

static WRITE_HANDLER(bank_w) {
  if (locals.enable) {
    locals.bank = core_BitColToNum((~data & 0x0f) ^ 0x01);
//  logerror("bank: %d\n", locals.bank);
    cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000 * locals.bank);
    locals.mute = (data >> 6) & 1;
    locals.LED = ~data >> 7;
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 1) | (locals.LED << 1);
  }
}

static READ_HANDLER(snd_cmd_r) {
//printf("snd_cmd_r: %02x\n", locals.sndCmd);
  return locals.sndCmd ^ 0x1f;
}

static READ_HANDLER(latch_r) {
  UINT8 latch = locals.latch[offset];
  if (offset) locals.latch[1] = snd_cmd_r(0);
  return latch;
}

static WRITE_HANDLER(latch_w) {
//printf("latch_w[%d]: %02x\n", offset, data);
  locals.latch[offset] = data;
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00fe, MRA_RAM },
  { 0x00fe, 0x00ff, latch_r },
  { 0x4000, 0x4003, pia_r(2) },
  { 0x8000, 0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00fe, MWA_RAM },
  { 0x00fe, 0x00ff, latch_w },
  { 0x4000, 0x4003, pia_w(2) },
  { 0xc000, 0xc000, bank_w },
MEMORY_END

static PORT_READ_START(snd_readport)
  { M6803_PORT2, M6803_PORT2, snd_cmd_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { M6803_PORT1, M6803_PORT1, dac_w },
  { M6803_PORT2, M6803_PORT2, enable_w },
PORT_END

static struct DACinterface nuova_dacInt = { 1, { 50 }};
static struct TMS5220interface nuova_tms5220Int = { 660000, 80, nuova_5220Irq };

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
  MDRV_SOUND_ADD(TMS5220, nuova_tms5220Int)
MACHINE_DRIVER_END


// games below

#define INITGAME(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_NOSOUNDE}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMENB(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_FAKEZERO}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEAL(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_ALPHA}}; \
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
/ Cosmic Flash (Flash Gordon Clone)
/-------------------------------*/
INITGAMENB(cosflash,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(cosflash,"cf2d.532",    CRC(939e941d) SHA1(889862043f351762e8c866aefb36a9ea75cbf828),
                          "cf6d.532",    CRC(7af93d2f) SHA1(2d939b14f7fe79f836e12926f44b70037630cd3f))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e) SHA1(ecdeb07c31c22ec313b55774f4358a9923c5e9e7),
                          "834-18_5.532",CRC(8799e80e) SHA1(f255b4e7964967c82cfc2de20ebe4b8d501e3cb0))
BY35_ROMEND
BY35_INPUT_PORTS_START(cosflash, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(cosflash,flashgdn,"Cosmic Flash",1985,"Bell Games",by35_mBY35_61S,0)

/*--------------------------------
/ Saturn 2 (Spy Hunter Clone)
/-------------------------------*/
INITGAMENB(saturn2,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(saturn2,"spy-2732.u2",CRC(9e930f2d) SHA1(fb48ce0d8d8f8a695827c0eea57510b53daa7c39),
                         "saturn2.u6", CRC(ca72a96b) SHA1(efcd8b41bf0c19ebd7db492632e046b348619460))
BY45_SOUNDROM11(         "spy_u3.532", CRC(95ffc1b8) SHA1(28f058f74abbbee120dca06f7321bcb588bef3c6),
                         "spy_u4.532", CRC(a43887d0) SHA1(6bbc55943fa9f0cd97f946767f21652e19d85265))
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
INITGAMENB(worlddef,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_BY45,0)
BY35_INPUT_PORTS_START(worlddef, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(worlddef,"World Defender",1985,"Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Space Hawks (Cybernaut Clone)
/-------------------------------*/
INITGAMENB(spacehaw,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(spacehaw,"cybe2732.u2g",CRC(d4a5e2f6) SHA1(841e940632993919a68c905546f533ff38a0ce31),
                          "spacehaw.u6",CRC(b154a3a3) SHA1(d632c5eddd0582ba2ca778ab03e11ca3f6f4e1ed))
BY45_SOUNDROMx2(          "cybu3.snd",  CRC(a3c1f6e7) SHA1(35a5e828a6f2dd9009e165328a005fa079bad6cb))
BY35_ROMEND
BY35_INPUT_PORTS_START(spacehaw, 1) BY35_INPUT_PORTS_END
CORE_CLONEDEFNV(spacehaw,cybrnaut,"Space Hawks",1986,"Nuova Bell Games",by35_mBY35_45S,0)

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
BY45_SOUNDROM11(         "bp_u3.532",  CRC(a5005067) SHA1(bd460a20a6e8f33746880d72241d6776b85126cf),
                         "bp_u4.532",  CRC(57978b4a) SHA1(4995837790d81b02325d39b548fb882a591769c5))
ROM_END
INITGAME(darkshad,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_INPUT_PORTS_START(darkshad, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(darkshad,"Dark Shadow",1986,"Nuova Bell Games",by35_mBY35_45S,0)

/*--------------------------------
/ Skill Flight
/-------------------------------*/
ROM_START(skflight)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game_u7.64", 0xe000, 0x2000, CRC(fe5001eb) SHA1(f7d56d484141ba8ec82664b6aebbf3a683547d20))
    ROM_LOAD("game_u8.64", 0xc000, 0x2000, CRC(58f259fe) SHA1(505f3996f66dbb4027bd47f6b7ba9e4baaeb6e51))
    ROM_COPY(REGION_CPU1, 0xc000, 0x9000,0x1000)
    ROM_COPY(REGION_CPU1, 0xe000, 0x1000,0x1000)
    ROM_COPY(REGION_CPU1, 0xf000, 0x5000,0x1000)

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u3.256", 0x0000, 0x8000, CRC(43424fb1) SHA1(428d2f7444cd71b6c49c04749b42263e3c185856))
      ROM_RELOAD(0x10000, 0x8000)
      ROM_RELOAD(0x20000, 0x8000)
      ROM_RELOAD(0x30000, 0x8000)
    ROM_LOAD("snd_u4.256", 0x8000, 0x8000, CRC(10378feb) SHA1(5da2b9c530167c80b9d411da159e4b6e95b76647))
      ROM_RELOAD(0x18000, 0x8000)
      ROM_RELOAD(0x28000, 0x8000)
      ROM_RELOAD(0x38000, 0x8000)
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAME(skflight,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(skflight, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(skflight, "Skill Flight", 1986, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)

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

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8.256", 0x00000,0x8000, CRC(cdf2a28d) SHA1(d4969370109b4c7f31f48a3ebd8925268caf9c44))
      ROM_RELOAD(0x20000, 0x8000)
    ROM_LOAD("snd_u9.256", 0x08000,0x8000, CRC(08bd0db9) SHA1(af851b8c993649b61645a414459000c206516bec))
      ROM_RELOAD(0x28000, 0x8000)
    ROM_LOAD("snd_u10.256",0x10000,0x8000, CRC(634bc64c) SHA1(8389fda08ee7bf0e5002153cec22e219bf786993))
      ROM_RELOAD(0x30000, 0x8000)
    ROM_LOAD("snd_u11.256",0x18000,0x8000, CRC(d4da383c) SHA1(032a4a425936d5c822fba6e46483f03a87c1a6ec))
      ROM_RELOAD(0x38000, 0x8000)
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAME(cobra,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(cobra, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(cobra, "Cobra", 1987, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)

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

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8.bin", 0x00000,0x8000, CRC(3d254d89) SHA1(2b4aa3387179e2c0fbf18684128761d3f778dcb2))
      ROM_RELOAD(0x20000, 0x8000)
    ROM_LOAD("snd_u9.bin", 0x08000,0x8000, CRC(9560f2c3) SHA1(3de6d074e2a3d3c8377fa330d4562b2d266bbfff))
      ROM_RELOAD(0x28000, 0x8000)
    ROM_LOAD("snd_u10.bin",0x10000,0x8000, CRC(70f440bc) SHA1(9fa4d33cc6174ce8f43f030487171bfbacf65537))
      ROM_RELOAD(0x30000, 0x8000)
    ROM_LOAD("snd_u11.bin",0x18000,0x8000, CRC(71d98d17) SHA1(9575b80a91a67b1644e909f70d364e0a75f73b02))
      ROM_RELOAD(0x38000, 0x8000)
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMENB(futrquen,GEN_BY35,dispNB,FLIP_SW(FLIP_L),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(futrquen, 2) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(futrquen, "Future Queen", 1987, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)

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

ROM_START(f1gp)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7", 0x8000, 0x8000, CRC(2287dea1) SHA1(5438752bf63aadaa6b6d71bbf56a72d8b67b545a))
  ROM_COPY(REGION_CPU1, 0x8000, 0x1000,0x1000)
  ROM_COPY(REGION_CPU1, 0x9000, 0x5000,0x1000)
  ROM_COPY(REGION_CPU1, 0xd000, 0x7000,0x1000)
  ROM_COPY(REGION_CPU1, 0xa000, 0x9000,0x1000)
  ROM_COPY(REGION_CPU1, 0xb000, 0xd000,0x1000)
  ROM_COPY(REGION_CPU1, 0xe000, 0xb000,0x1000)

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8a", 0x20000,0x8000, CRC(3a2af90b) SHA1(f6eeae74b3bfb1cfd9235c5214f7c029e0ad14d6))
    ROM_LOAD("snd_u8b", 0x00000,0x8000, CRC(14cddb29) SHA1(667b54174ad5dd8aa45037574916ecb4ee996a94))
    ROM_LOAD("snd_u9a", 0x28000,0x8000, CRC(681ee99c) SHA1(955cd782073a1ce0be7a427c236d47fcb9cccd20))
    ROM_LOAD("snd_u9b", 0x08000,0x8000, CRC(726920b5) SHA1(002e7a072a173836c89746cceca7e5d2ac26356d))
    ROM_LOAD("snd_u10a",0x30000,0x8000, CRC(4d3fc9bb) SHA1(d43cd134f399e128a678b86e57b1917fad70df76))
    ROM_LOAD("snd_u10b",0x10000,0x8000, CRC(9de359fb) SHA1(ce75a78dc4ed747421a386d172fa0f8a1369e860))
    ROM_LOAD("snd_u11a",0x38000,0x8000, CRC(884dc754) SHA1(b121476ea621eae7a7ba0b9a1b5e87051e1e9e3d))
    ROM_LOAD("snd_u11b",0x18000,0x8000, CRC(2394b498) SHA1(bf0884a6556a27791e7e801051be5975dd6b95c4))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMEAL(f1gp,GEN_BY35,dispAlpha,FLIP_SWNO(48,0),8,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(f1gp, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(f1gp, "F1 Grand Prix", 1987, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Top Pin
/-------------------------------*/
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

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8.bin", 0x00000,0x8000, CRC(2cb9c931) SHA1(2537976c890ceff857b9aaf204c48ab014aad94e))
      ROM_RELOAD(0x20000, 0x8000)
    ROM_LOAD("snd_u9.bin", 0x08000,0x8000, CRC(72690344) SHA1(c2a13aa59f0c605eb616256cd288b79cceca003b))
      ROM_RELOAD(0x28000, 0x8000)
    ROM_LOAD("snd_u10.bin",0x10000,0x8000, CRC(bca9a805) SHA1(0deb3172b5c8fc91c4b02b21b1e3794ed7adef13))
      ROM_RELOAD(0x30000, 0x8000)
    ROM_LOAD("snd_u11.bin",0x18000,0x8000, CRC(1814a50d) SHA1(6fe22e774fa90725d0db9f1020bad88bae0ef85c))
      ROM_RELOAD(0x38000, 0x8000)
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMENB(toppin,GEN_BY35,dispNB,FLIP_SW(FLIP_L),0,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(toppin, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(toppin, "Top Pin", 1988, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)

/*--------------------------------
/ U-boat 65
/-------------------------------*/
ROM_START(uboat65)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7.256", 0x8000, 0x8000, CRC(f0fa1cbc) SHA1(4373bb37927dde01f5a4da5ef6094424909e9bc6))
  ROM_COPY(REGION_CPU1, 0x8000, 0x1000,0x1000)
  ROM_COPY(REGION_CPU1, 0x9000, 0x5000,0x1000)
  ROM_COPY(REGION_CPU1, 0xd000, 0x7000,0x1000)
  ROM_COPY(REGION_CPU1, 0xa000, 0x9000,0x1000)
  ROM_COPY(REGION_CPU1, 0xb000, 0xd000,0x1000)
  ROM_COPY(REGION_CPU1, 0xe000, 0xb000,0x1000)

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_ic3.256", 0x0000, 0x8000, CRC(c7811983) SHA1(7924248dcc08b05c34d3ddf2e488b778215bc7ea))
      ROM_RELOAD(0x10000, 0x8000)
      ROM_RELOAD(0x20000, 0x8000)
      ROM_RELOAD(0x30000, 0x8000)
    ROM_LOAD("snd_ic5.256", 0x8000, 0x8000, CRC(bc35e5cf) SHA1(a809b0056c576416aa76ead0437e036c2cdbd1ef))
      ROM_RELOAD(0x18000, 0x8000)
      ROM_RELOAD(0x28000, 0x8000)
      ROM_RELOAD(0x38000, 0x8000)
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

INITGAMEAL(uboat65,GEN_BY35,dispAlpha,FLIP_SWNO(12,5),0,SNDBRD_NUOVA,0)
BY35_INPUT_PORTS_START(uboat65, 3) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(uboat65, "U-boat 65", 1988, "Nuova Bell Games", nuova, GAME_IMPERFECT_SOUND)
