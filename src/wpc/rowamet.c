/************************************************************************************************
  Rowamet
  ---------
  by Gerrit Volkenborn

  Main CPU board:
  ---------------
  CPU: Z80
  other than that, it uses the same architecture as Taito!

  Sound board:
  ------------
  CPU: Z80
  Clock: 17/9 Mhz (guessed)
  DAC
************************************************************************************************/
#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "taito.h"

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static struct {
  UINT8 sndCmd;
  int volume;
} locals;

static void rowamet_init(struct sndbrdData *brdData) {
  memset(&locals, 0x00, sizeof(locals));
  locals.volume = 100;
}

static WRITE_HANDLER(rowamet_data_w) {
  locals.sndCmd = data;
  locals.volume = 100;
  mixer_set_volume(0, 100);
  cpu_set_nmi_line(1, PULSE_LINE);
//printf("D:%02x ", locals.sndCmd);
}

static WRITE_HANDLER(mute_w) {
//printf("V:%d ", locals.volume);
//  mixer_set_volume(0, locals.volume);
//  locals.volume /= 2;
}

static READ_HANDLER(snd_data_r) {
//printf("R ");
  return locals.sndCmd;
}

//Memory map for sound CPU
static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x0fff, MRA_ROM },
  { 0x1000, 0x17ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x1000, 0x17ff, MWA_RAM },
MEMORY_END

//Port map for sound CPU
static PORT_READ_START(snd_readport)
  { 0x00, 0x00, snd_data_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { 0x00, 0x00, mute_w },
  { 0x01, 0x01, DAC_0_data_w },
PORT_END

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf rowametIntf = {
  "ROWAMET", rowamet_init, NULL, NULL, rowamet_data_w, rowamet_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static struct DACinterface rowamet_dac_intf = { 1, { 25 }};

MACHINE_DRIVER_START(rowamet)
  MDRV_IMPORT_FROM(taito)
  MDRV_CPU_REPLACE("mcpu", Z80, 1888888)

  MDRV_CPU_ADD_TAG("scpu", Z80, 1888888)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ADD(DAC, rowamet_dac_intf)
MACHINE_DRIVER_END

/*-------------------
/ games start here
/--------------------*/
static core_tLCDLayout disp[] = {
  {0, 2,24,1,CORE_SEG7},
  {3, 0, 0,6,CORE_SEG7},
  {5, 0, 6,6,CORE_SEG7},
  {7, 0,12,6,CORE_SEG7},
  {9, 0,18,6,CORE_SEG7},
  {12,2,25,1,CORE_SEG7},
  {0}
};
static core_tGameData heavymtlGameData = {GEN_ZAC2, disp, {FLIP_SW(FLIP_L),0,0,0,SNDBRD_ROWAMET,0}};
static void init_heavymtl(void) {
  core_gameData = &heavymtlGameData;
}
TAITO_INPUT_PORTS_START(heavymtl, 1) TAITO_INPUT_PORTS_END

ROM_START(heavymtl) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("hvymtl_c.bin", 0x0000, 0x1000, CRC(8f36d3da) SHA1(beec79c5d794ede96d95105bad7466b67762606d)) \
    ROM_LOAD("hvymtl_b.bin", 0x1000, 0x1000, CRC(357f1252) SHA1(ddc55ded0dc1c8632c31d809bfadfb45ae248cfd)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("hvymtl_s.bin", 0x0000, 0x1000, CRC(c525e6cb) SHA1(144e06fbbdd1f3e45ccca8bace6b04f876b1312c)) \
ROM_END

CORE_GAMEDEFNV(heavymtl, "Heavy Metal", 198?, "Rowamet", rowamet, GAME_IMPERFECT_SOUND)
