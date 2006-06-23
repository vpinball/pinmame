/************************************************************************************************
  Nuova Bell Games
  ----------------
  by Steve Ellenoff and Gerrit Volkenborn

  Main CPU Board:
  Bally-35. Turned out we can reuse the same code all the way...
  A few games use alpha displays, this is implemented in by35.c

  Issues/Todo:
  Sound Board with 6803 CPU and DAC, Skill Flight probably uses a modified Squalk & Talk board.
************************************************************************************************/
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
  UINT8 sndCmd, latch;
  int bank, LED, mute, enable;
} locals;

static void nuova_init(struct sndbrdData *brdData) {
  memset(&locals, 0x00, sizeof(locals));
  locals.brdData = *brdData;
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
    locals.bank = core_BitColToNum(~data & 0x0f);
//    logerror("bank: %d\n", locals.bank);
    cpu_setbank(1, memory_region(REGION_SOUND1) + 0x8000 * locals.bank);
    locals.mute = (data >> 6) & 1;
    locals.LED = ~data >> 7;
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 1) | (locals.LED << 1);
  }
}

static READ_HANDLER(snd_cmd_r) {
  return locals.sndCmd ^ 0x1f;
}

static READ_HANDLER(latch_r) {
  locals.latch = locals.sndCmd ^ 0x1f;
  return locals.latch;
}

static WRITE_HANDLER(latch_w) {
  locals.latch = data;
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00fe, MRA_RAM },
  { 0x00ff, 0x00ff, latch_r },
  { 0x8000, 0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00fe, MWA_RAM },
  { 0x00ff, 0x00ff, latch_w },
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

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf nuovaIntf = {
  "NUOVA", nuova_init, NULL, nuova_diag, nuova_man_w, nuova_data_w, NULL, nuova_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(nuova)
  MDRV_IMPORT_FROM(by35)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(nuova_readmem, nuova_writemem)
  MDRV_NVRAM_HANDLER(nuova)
  MDRV_DIAGNOSTIC_LEDH(2)
  // CPU clock adjusted to fit recorded live sample
  MDRV_CPU_ADD_TAG("scpu", M6803, 975000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_SOUND_ADD(DAC, nuova_dacInt)
MACHINE_DRIVER_END


// games below

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

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0,16,CORE_SEG16S},
  {4, 0,16,16,CORE_SEG16S},
  {0}
};

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
CORE_GAMEDEFNV(f1gp, "F1 Grand Prix", 1987, "Nuova Bell Games", nuova, 0)
