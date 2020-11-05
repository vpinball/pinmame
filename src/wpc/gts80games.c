#include "driver.h"
#include "sndbrd.h"
#include "sim.h"
#include "gts80.h"
#include "gts80s.h"
#include "machine/6532riot.h"

#define INITGAME(name, gen, flip, disptype, sb, disp, inv) \
  static core_tGameData name##GameData = {gen, disptype, {flip,0,0,0,sb,disp},NULL,{{0},{inv}}}; \
  static void init_##name(void) { core_gameData = &name##GameData; }

#define FLIP616   FLIP_SWNO(6,16)

GTS80_INPUT_PORTS_START(gts80, 1)       GTS80_INPUT_PORTS_END
GTS80VID_INPUT_PORTS_START(gts80vid, 1) GTS80_INPUT_PORTS_END

/* 4 x 6 BCD + Ball,Credit */
static core_tLCDLayout dispNumeric1[] = {
  {0, 0, 2, 6,CORE_SEG9}, {0,16, 9, 6,CORE_SEG9},
  {4, 0,22, 6,CORE_SEG9}, {4,16,29, 6,CORE_SEG9},
  DISP_SEG_CREDIT(40,41,CORE_SEG7S), DISP_SEG_BALLS(42,43,CORE_SEG7S), {0}
};
/* 5 x 6 BCD + Ball, Credit */
static core_tLCDLayout dispNumeric2[] = {
  {0, 0, 2, 6,CORE_SEG9}, {0,16, 9, 6,CORE_SEG9},
  {4, 0,22, 6,CORE_SEG9}, {4,16,29, 6,CORE_SEG9},
  DISP_SEG_CREDIT(40,41,CORE_SEG7S), DISP_SEG_BALLS(42,43,CORE_SEG7S),
  {6, 8,50, 6,CORE_SEG9}, {0}
};
/* 4 x 7 BCD + Ball,Credit */
static core_tLCDLayout dispNumeric3[] = {
  {0, 0, 2, 7,CORE_SEG98F}, {0,16, 9, 7,CORE_SEG98F},
  {4, 0,22, 7,CORE_SEG98F}, {4,16,29, 7,CORE_SEG98F},
  DISP_SEG_CREDIT(40,41,CORE_SEG7S), DISP_SEG_BALLS(42,43,CORE_SEG7S), {0}
};
/* 4 x 7 BCD + 1 x 6 BCD + Ball, Credit */
static core_tLCDLayout dispNumeric4[] = {
  {0, 0, 2, 7,CORE_SEG98F}, {0,16, 9, 7,CORE_SEG98F},
  {4, 0,22, 7,CORE_SEG98F}, {4,16,29, 7,CORE_SEG98F},
  DISP_SEG_CREDIT(40,41,CORE_SEG7S), DISP_SEG_BALLS(42,43,CORE_SEG7S),
  {6,9,50, 6,CORE_SEG9}, {0}
};

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0,20,CORE_SEG16},  {2, 0,20,20,CORE_SEG16}, {0}
};

#define INIT_S80(name, disptype, sb) \
static core_tGameData name##GameData = {GEN_GTS80, disptype, {0,0,0,0,sb,0},NULL,{{0},{0}}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \
ROM_START(name) \
  NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
    ROM_LOAD("u2_80.bin", 0x2000, 0x1000, CRC(4f0bc7b1) SHA1(612cbacdca5cfa6ad23940796df3b7c385be79fe)) \
      ROM_RELOAD(0x6000, 0x1000) \
      ROM_RELOAD(0xa000, 0x1000) \
      ROM_RELOAD(0xe000, 0x1000) \
    ROM_LOAD("u3_80.bin", 0x3000, 0x1000, CRC(1e69f9d0) SHA1(ad738cac2555830257b531e5e533b15362f624b9)) \
      ROM_RELOAD(0x7000, 0x1000) \
      ROM_RELOAD(0xb000, 0x1000) \
      ROM_RELOAD(0xf000, 0x1000)

#define INIT_S80D7(name, disptype, sb) \
static core_tGameData name##GameData = {GEN_GTS80, disptype, {0,0,0,0,sb,0},NULL,{{0},{0}}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \
ROM_START(name) \
  NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
    ROM_LOAD("u2g807dc.bin", 0x2000, 0x1000, CRC(f8a687b3) SHA1(ba7747c04a5967df760ace102e47c91d42e07a12)) \
      ROM_RELOAD(0x6000, 0x1000) \
      ROM_RELOAD(0xa000, 0x1000) \
      ROM_RELOAD(0xe000, 0x1000) \
    ROM_LOAD("u3g807dc.bin", 0x3000, 0x1000, CRC(6e31242e) SHA1(14e371a0352a6068dec20af1f2b344e34a5b9011)) \
      ROM_RELOAD(0x7000, 0x1000) \
      ROM_RELOAD(0xb000, 0x1000) \
      ROM_RELOAD(0xf000, 0x1000)

#define INIT_S80A(name, disptype, sb, disp) \
static core_tGameData name##GameData = {GEN_GTS80A, disptype, {0,0,0,0,sb,disp},NULL,{{0},{0}}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \
ROM_START(name) \
  NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
    ROM_LOAD("u2_80a.bin", 0x2000, 0x1000, CRC(241de1d4) SHA1(9d5942704cbdec6565d6335e33e9f7e4c60a41ac)) \
      ROM_RELOAD(0x6000, 0x1000) \
      ROM_RELOAD(0xa000, 0x1000) \
      ROM_RELOAD(0xe000, 0x1000) \
    ROM_LOAD("u3_80a.bin", 0x3000, 0x1000, CRC(2d77ccdc) SHA1(47241ccd365e8d74d5aa5b775acf6445cc95b8a8)) \
      ROM_RELOAD(0x7000, 0x1000) \
      ROM_RELOAD(0xb000, 0x1000) \
      ROM_RELOAD(0xf000, 0x1000)

INIT_S80(gts80, dispNumeric1, SNDBRD_GTS80SS) GTS80_ROMEND
GAMEX(1981,gts80,0,gts80ss,gts80,gts80,ROT0,"Gottlieb","System 80 with speech board",NOT_A_DRIVER)
INIT_S80(gts80s, dispNumeric1, SNDBRD_GTS80S) GTS80_ROMEND
GAMEX(1980,gts80s,gts80,gts80s,gts80,gts80s,ROT0,"Gottlieb","System 80",NOT_A_DRIVER)

INIT_S80A(gts80a, dispNumeric3, SNDBRD_GTS80SS, 0) GTS80_ROMEND
GAMEX(1981,gts80a,0,gts80ss,gts80,gts80a,ROT0,"Gottlieb","System 80A with speech board",NOT_A_DRIVER)
INIT_S80A(gts80as, dispNumeric3, SNDBRD_GTS80S, 0) GTS80_ROMEND
GAMEX(1981,gts80as,gts80a,gts80s,gts80,gts80as,ROT0,"Gottlieb","System 80A",NOT_A_DRIVER)

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

// System80

/*-------------------------------------------------------------------
/ The Amazing Spider-Man
/-------------------------------------------------------------------*/
INIT_S80(spidermn, dispNumeric1, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("653-1.cpu",    CRC(674ddc58) SHA1(c9be45391b8dd58a0836801807d593d4c7da9904),
                  "653-2.cpu",    CRC(ff1ddfd7) SHA1(dd7b98e491045916153b760f36432506277a4093))
GTS80S1K_ROMSTART("653.snd",      CRC(f5650c46) SHA1(2d0e50fa2f4b3d633daeaa7454630e3444453cb2),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_spidermn input_ports_gts80
CORE_CLONEDEFNV(spidermn,gts80s,"Amazing Spider-Man, The",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(spiderm7, dispNumeric3, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("653-1.cpu",    CRC(674ddc58) SHA1(c9be45391b8dd58a0836801807d593d4c7da9904),
                  "653-2.cpu",    CRC(ff1ddfd7) SHA1(dd7b98e491045916153b760f36432506277a4093))
GTS80S1K_ROMSTART("653.snd",      CRC(f5650c46) SHA1(2d0e50fa2f4b3d633daeaa7454630e3444453cb2),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_spiderm7 input_ports_spidermn
CORE_CLONEDEFNV(spiderm7,spidermn,"Amazing Spider-Man, The (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Panthera
/-------------------------------------------------------------------*/
INIT_S80(panthera, dispNumeric1, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("652.cpu",      CRC(5386e5fb) SHA1(822f47951b702f9c6a1ce674baaab0a596f34413))
GTS80S1K_ROMSTART("652.snd",      CRC(4d0cf2c0) SHA1(0da5d118ffd19b1e78dfaaee3e31c43750d45c8d),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_panthera input_ports_gts80
CORE_CLONEDEFNV(panthera,gts80s,"Panthera",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(panther7, dispNumeric3, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("652.cpu",      CRC(5386e5fb) SHA1(822f47951b702f9c6a1ce674baaab0a596f34413))
GTS80S1K_ROMSTART("652.snd",      CRC(4d0cf2c0) SHA1(0da5d118ffd19b1e78dfaaee3e31c43750d45c8d),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_panther7 input_ports_panthera
CORE_CLONEDEFNV(panther7,panthera,"Panthera (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Circus
/-------------------------------------------------------------------*/
INIT_S80(circus, dispNumeric1, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("654-1.cpu",    CRC(0eeb2731) SHA1(087cd6400bf0775bda0264422b3f790a77852bc4),
                  "654-2.cpu",    CRC(01e23569) SHA1(47088421254e487aa1d1e87ea911dc1634e7d9ad))
GTS80S1K_ROMSTART("654.snd",      CRC(75c3ad67) SHA1(4f59c451b8659d964d5242728814c2d97f68445b),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_circus input_ports_gts80
CORE_CLONEDEFNV(circus,gts80s,"Circus",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(circus7, dispNumeric3, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("654-1.cpu",    CRC(0eeb2731) SHA1(087cd6400bf0775bda0264422b3f790a77852bc4),
                  "654-2.cpu",    CRC(01e23569) SHA1(47088421254e487aa1d1e87ea911dc1634e7d9ad))
GTS80S1K_ROMSTART("654.snd",      CRC(75c3ad67) SHA1(4f59c451b8659d964d5242728814c2d97f68445b),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_circus7 input_ports_circus
CORE_CLONEDEFNV(circus7,circus,"Circus (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Counterforce
/-------------------------------------------------------------------*/
INIT_S80(cntforce, dispNumeric1, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("656-1.cpu",    CRC(42baf51d) SHA1(6c7947df6e4d7ed2fd48410705018bde91db3356),
                  "656-2.cpu",    CRC(0e185c30) SHA1(01d9fb5d335c24bed9f747d6e23f57adb6ef09a5))
GTS80S1K_ROMSTART("656.snd",      CRC(0be2cbe9) SHA1(306a3e7d93733562360285de35b331b5daae7250),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_cntforce input_ports_gts80
CORE_CLONEDEFNV(cntforce,gts80s,"Counterforce",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(cntforc7, dispNumeric3, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("656-1.cpu",    CRC(42baf51d) SHA1(6c7947df6e4d7ed2fd48410705018bde91db3356),
                  "656-2.cpu",    CRC(0e185c30) SHA1(01d9fb5d335c24bed9f747d6e23f57adb6ef09a5))
GTS80S1K_ROMSTART("656.snd",      CRC(0be2cbe9) SHA1(306a3e7d93733562360285de35b331b5daae7250),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_cntforc7 input_ports_cntforce
CORE_CLONEDEFNV(cntforc7,cntforce,"Counterforce (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Star Race
/-------------------------------------------------------------------*/
INIT_S80(starrace, dispNumeric1, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("657-1.cpu",    CRC(27081372) SHA1(2d9cd81ffa44c389c4895043fa1e93b899544158),
                  "657-2.cpu",    CRC(c56e31c8) SHA1(1e129fb6309e015a16f2bdb1e389cbc85d1919a7))
GTS80S1K_ROMSTART("657.snd",      CRC(3a1d3995) SHA1(6f0bdb34c4fa11d5f8ecbb98ae55bafeb5d62c9e),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_starrace input_ports_gts80
CORE_CLONEDEFNV(starrace,gts80s,"Star Race",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(starrac7, dispNumeric3, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("657-1.cpu",    CRC(27081372) SHA1(2d9cd81ffa44c389c4895043fa1e93b899544158),
                  "657-2.cpu",    CRC(c56e31c8) SHA1(1e129fb6309e015a16f2bdb1e389cbc85d1919a7))
GTS80S1K_ROMSTART("657.snd",      CRC(3a1d3995) SHA1(6f0bdb34c4fa11d5f8ecbb98ae55bafeb5d62c9e),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_starrac7 input_ports_starrace
CORE_CLONEDEFNV(starrac7,starrace,"Star Race (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ James Bond (Timed Play)
/-------------------------------------------------------------------*/
INIT_S80(jamesb, dispNumeric2, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("658-1.cpu",    CRC(b841ad7a) SHA1(3396e82351c975781cac9112bfa341a3b799f296))
GTS80S1K_ROMSTART("658.snd",      CRC(962c03df) SHA1(e8ff5d502a038531a921380b75c27ef79b6feac8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_jamesb input_ports_gts80
CORE_CLONEDEFNV(jamesb,gts80s,"James Bond (Timed Play)",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(jamesb7, dispNumeric4, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("658-1.cpu",    CRC(b841ad7a) SHA1(3396e82351c975781cac9112bfa341a3b799f296))
GTS80S1K_ROMSTART("658.snd",      CRC(962c03df) SHA1(e8ff5d502a038531a921380b75c27ef79b6feac8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_jamesb7 input_ports_jamesb
CORE_CLONEDEFNV(jamesb7,jamesb,"James Bond (Timed Play, 7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ James Bond (3/5 Ball Play)
/-------------------------------------------------------------------*/
INIT_S80(jamesb2, dispNumeric2, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("658-x.cpu",    CRC(e7e0febf) SHA1(2c101a88b61229f30ed15d38f395bc538999d766))
GTS80S1K_ROMSTART("658.snd",      CRC(962c03df) SHA1(e8ff5d502a038531a921380b75c27ef79b6feac8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_jamesb2 input_ports_jamesb
CORE_CLONEDEFNV(jamesb2,jamesb,"James Bond (3/5-Ball)",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(jamesb7b, dispNumeric4, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("658-x.cpu",    CRC(e7e0febf) SHA1(2c101a88b61229f30ed15d38f395bc538999d766))
GTS80S1K_ROMSTART("658.snd",      CRC(962c03df) SHA1(e8ff5d502a038531a921380b75c27ef79b6feac8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_jamesb7b input_ports_jamesb
CORE_CLONEDEFNV(jamesb7b,jamesb,"James Bond (3/5-Ball, 7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Time Line
/-------------------------------------------------------------------*/
static core_tLCDLayout dispTimeline[] = {
  DISP_SEG_IMPORT(dispNumeric1), {6, 12, 50, 2, CORE_SEG9}, {0}
};
INIT_S80(timeline, dispTimeline, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("659.cpu",      CRC(d6950e3b) SHA1(939b45a9ee4bb122fbea534ad728ec6b85120416))
GTS80S1K_ROMSTART("659.snd",      CRC(28185568) SHA1(2fd26e7e0a8f050d67159f17634df2b1fc47cbd3),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_timeline input_ports_gts80
CORE_CLONEDEFNV(timeline,gts80s,"Time Line",1980,"Gottlieb",gl_mGTS80S,0)

static core_tLCDLayout dispTimeline7[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6, 13, 50, 2, CORE_SEG9}, {0}
};
INIT_S80D7(timelin7, dispTimeline7, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("659.cpu",      CRC(d6950e3b) SHA1(939b45a9ee4bb122fbea534ad728ec6b85120416))
GTS80S1K_ROMSTART("659.snd",      CRC(28185568) SHA1(2fd26e7e0a8f050d67159f17634df2b1fc47cbd3),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_timelin7 input_ports_timeline
CORE_CLONEDEFNV(timelin7,timeline,"Time Line (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Force II
/-------------------------------------------------------------------*/
INIT_S80(forceii, dispNumeric1, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("661-2.cpu",    CRC(a4fa42a4) SHA1(c17af4f0da6d5630e43db44655bece0e26b0112a))
GTS80S1K_ROMSTART("661.snd",      CRC(650158a7) SHA1(c7a9d521d1e7de1e00e7abc3a97aaaee04f8052e),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_forceii input_ports_gts80
CORE_CLONEDEFNV(forceii,gts80s,"Force II",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(forceii7, dispNumeric3, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("661-2.cpu",    CRC(a4fa42a4) SHA1(c17af4f0da6d5630e43db44655bece0e26b0112a))
GTS80S1K_ROMSTART("661.snd",      CRC(650158a7) SHA1(c7a9d521d1e7de1e00e7abc3a97aaaee04f8052e),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_forceii7 input_ports_forceii
CORE_CLONEDEFNV(forceii7,forceii,"Force II (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Pink Panther
/-------------------------------------------------------------------*/
static core_tLCDLayout dispPinkPanther[] = {
  DISP_SEG_IMPORT(dispNumeric1),
  {6, 8,50,2,CORE_SEG9}, {6,16,54,2,CORE_SEG9}, {0}
};
INIT_S80(pnkpnthr, dispPinkPanther, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("664-1.cpu",    CRC(a0d3e69a) SHA1(590e68dc28067e61832927cd4b3eefcc066f0a92))
GTS80S1K_ROMSTART("664.snd",      CRC(18f4abfd) SHA1(9e85eb7e9b1e2fe71be828ff1b5752424ed42588),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_pnkpnthr input_ports_gts80
CORE_CLONEDEFNV(pnkpnthr,gts80s,"Pink Panther",1981,"Gottlieb",gl_mGTS80S,0)

static core_tLCDLayout dispPinkPanther7[] = {
  DISP_SEG_IMPORT(dispNumeric3),
  {6,10,50,2,CORE_SEG9}, {6,16,54,2,CORE_SEG9}, {0}
};
INIT_S80D7(pnkpntr7, dispPinkPanther7, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("664-1.cpu",    CRC(a0d3e69a) SHA1(590e68dc28067e61832927cd4b3eefcc066f0a92))
GTS80S1K_ROMSTART("664.snd",      CRC(18f4abfd) SHA1(9e85eb7e9b1e2fe71be828ff1b5752424ed42588),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_pnkpntr7 input_ports_pnkpnthr
CORE_CLONEDEFNV(pnkpntr7,pnkpnthr,"Pink Panther (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

INIT_S80(pnkpntrs, dispPinkPanther, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("664-1.cpu",          CRC(a0d3e69a) SHA1(590e68dc28067e61832927cd4b3eefcc066f0a92))
GTS80S1K_ROMSTART("664_flipprojets.snd",CRC(48aeb325) SHA1(49dae08c635f191841188565bd89f07c4ad44c08),
                  "6530sy80.bin",       CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_pnkpntrs input_ports_pnkpnthr
CORE_CLONEDEFNV(pnkpntrs,pnkpnthr,"Pink Panther (sound correction fix)",1981,"Flipprojets",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Mars - God of War
/-------------------------------------------------------------------*/
INIT_S80(mars, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666-1.cpu",  CRC(bb7d476a) SHA1(22d5d7f0e52c5180f73a1ca0b3c6bd4b7d0843d6))
GTS80SS22_ROMSTART("666-s1.snd", CRC(d33dc8a5) SHA1(8d071c392996a74c3cdc2cf5ea3be3c86553ce89),
                   "666-s2.snd", CRC(e5616f3e) SHA1(a6b5ebd0b456a555db0889cd63ce79aafc64dbe5))
GTS80_ROMEND
#define input_ports_mars input_ports_gts80
CORE_CLONEDEFNV(mars,gts80,"Mars - God of War",1981,"Gottlieb",gl_mGTS80SS,0)

//From flipprojets.fr: After analysis of the 666-S ROM, it turns out that there are very few differences
//                     compared to the normal version. In fact, only the timers for the rise of the target 
//                     banks and the ejection of the outhole have been corrected.
//                     On the "sample" version it should therefore not always working very fine!
INIT_S80(marsp, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666s-1.cpu", CRC(029e0bcf) SHA1(20764464ede38bee2a726fc2ae98a60375b3cb1c))
GTS80SS22_ROMSTART("666-s1.snd", CRC(d33dc8a5) SHA1(8d071c392996a74c3cdc2cf5ea3be3c86553ce89),
                   "666-s2.snd", CRC(e5616f3e) SHA1(a6b5ebd0b456a555db0889cd63ce79aafc64dbe5))
GTS80_ROMEND
#define input_ports_marsp input_ports_mars
CORE_CLONEDEFNV(marsp,mars,"Mars - God of War (Prototype)",1981,"Gottlieb",gl_mGTS80SS,0)

INIT_S80(marsf, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666-1.cpu",   CRC(bb7d476a) SHA1(22d5d7f0e52c5180f73a1ca0b3c6bd4b7d0843d6))
GTS80SS22_ROMSTART("f666-s1.snd", CRC(f9f782c5) SHA1(83438fcf3475bc2cb24c828036d94063c263a031),
                   "f666-s2.snd", CRC(7bd64d94) SHA1(a52492820e69f2072fd1dffb5cbb48fb960e19ce))
GTS80_ROMEND
#define input_ports_marsf input_ports_mars
CORE_CLONEDEFNV(marsf,mars,"Mars - God of War (French Speech)",1981,"Gottlieb",gl_mGTS80SS,0)

INIT_S80D7(mars7, dispNumeric3, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666-1.cpu",  CRC(bb7d476a) SHA1(22d5d7f0e52c5180f73a1ca0b3c6bd4b7d0843d6))
GTS80SS22_ROMSTART("666-s1.snd", CRC(d33dc8a5) SHA1(8d071c392996a74c3cdc2cf5ea3be3c86553ce89),
                   "666-s2.snd", CRC(e5616f3e) SHA1(a6b5ebd0b456a555db0889cd63ce79aafc64dbe5))
GTS80_ROMEND
#define input_ports_mars7 input_ports_mars
CORE_CLONEDEFNV(mars7,mars,"Mars - God of War (7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

INIT_S80(mars_2, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666-2.cpu",  CRC(6fb6d10b) SHA1(bd6fcebb52733e56f6fb66ce527cbbe3573b7250))
GTS80SS22_ROMSTART("666-s1.snd", CRC(d33dc8a5) SHA1(8d071c392996a74c3cdc2cf5ea3be3c86553ce89),
                   "666-s2.snd", CRC(e5616f3e) SHA1(a6b5ebd0b456a555db0889cd63ce79aafc64dbe5))
GTS80_ROMEND
#define input_ports_mars_2 input_ports_mars
CORE_CLONEDEFNV(mars_2,mars,"Mars - God of War (rev. 2 unofficial MOD)",1981,"Flipprojets",gl_mGTS80SS,0) // fixes a potential startup problem


/*-------------------------------------------------------------------
/ Volcano (Sound and Speech)
/-------------------------------------------------------------------*/
INIT_S80(vlcno_ax, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("667-a-x.cpu", CRC(1f51c351) SHA1(8e1850808faab843ac324040ca665a83809cdc7b))
GTS80SS22_ROMSTART("667-s1.snd",  CRC(ba9d40b7) SHA1(3d6640b259cd8ae87b998cbf1ae2dc13a2913e4f),
                   "667-s2.snd",  CRC(b54bd123) SHA1(3522ccdcb28bfacff2287f5537d52f22879249ab))
GTS80_ROMEND
#define input_ports_vlcno_ax input_ports_gts80
CORE_CLONEDEFNV(vlcno_ax,gts80,"Volcano",1981,"Gottlieb",gl_mGTS80SS,0)

INIT_S80D7(vlcno_a7, dispNumeric3, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("667-a-x.cpu", CRC(1f51c351) SHA1(8e1850808faab843ac324040ca665a83809cdc7b))
GTS80SS22_ROMSTART("667-s1.snd",  CRC(ba9d40b7) SHA1(3d6640b259cd8ae87b998cbf1ae2dc13a2913e4f),
                   "667-s2.snd",  CRC(b54bd123) SHA1(3522ccdcb28bfacff2287f5537d52f22879249ab))
GTS80_ROMEND
#define input_ports_vlcno_a7 input_ports_vlcno_ax
CORE_CLONEDEFNV(vlcno_a7,vlcno_ax,"Volcano (7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Volcano (Sound Only)
/-------------------------------------------------------------------*/
INIT_S80(vlcno_1b, dispNumeric1, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("667-1b.cpu",   CRC(a422d862) SHA1(2785388eb43c08405774a9413ffa52c1591a84f2))
GTS80S1K_ROMSTART("667-a-s.snd",  CRC(894b4e2e) SHA1(d888f8e00b2b50cef5cc916d46e4c5e6699914a1),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_vlcno_1b input_ports_gts80
CORE_CLONEDEFNV(vlcno_1b,gts80s,"Volcano (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80(vlcno_1a, dispNumeric1, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("667-1a.cpu",   CRC(5931c6f7) SHA1(e104a6c3ca2175bb49199e06963e26185dd563d2))
GTS80S1K_ROMSTART("667-a-s.snd",  CRC(894b4e2e) SHA1(d888f8e00b2b50cef5cc916d46e4c5e6699914a1),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_vlcno_1a input_ports_vlcno_1b
CORE_CLONEDEFNV(vlcno_1a,vlcno_1b,"Volcano (Sound Only, alternate version)",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80(vlcno_1c, dispNumeric1, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("667-1c.cpu",   CRC(e364202d) SHA1(128eaa5b390e309f4cf89f3631da0341f1419ffe))
GTS80S1K_ROMSTART("667-a-s.snd",  CRC(894b4e2e) SHA1(d888f8e00b2b50cef5cc916d46e4c5e6699914a1),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_vlcno_1c input_ports_vlcno_1b
CORE_CLONEDEFNV(vlcno_1c, vlcno_1b, "Volcano (Sound Only, alternate version 2)",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(vlcno_b7, dispNumeric3, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("667-1b.cpu",   CRC(a422d862) SHA1(2785388eb43c08405774a9413ffa52c1591a84f2))
GTS80S1K_ROMSTART("667-a-s.snd",  CRC(894b4e2e) SHA1(d888f8e00b2b50cef5cc916d46e4c5e6699914a1),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_vlcno_b7 input_ports_vlcno_1b
CORE_CLONEDEFNV(vlcno_b7,vlcno_1b,"Volcano (Sound Only, 7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 4)
/-------------------------------------------------------------------*/
INIT_S80(blckhole, dispNumeric2, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("668-4.cpu",  CRC(01b53045) SHA1(72d73bbb09358b331696cd1cc44fc4958feffbe2))
GTS80SS22_ROMSTART("668-s1.snd", CRC(23d5045d) SHA1(a20bf02ece97e8238d1dbe8d35ca63d82b62431e),
                   "668-s2.snd", CRC(d63da498) SHA1(84dd87783f47fbf64b1830284c168501f9b455e2))
GTS80_ROMEND
#define input_ports_blckhole input_ports_gts80
CORE_CLONEDEFNV(blckhole,gts80,"Black Hole (rev. 4)",1981,"Gottlieb",gl_mGTS80SS,0)

INIT_S80D7(blkhole7, dispNumeric4, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("668-4.cpu",  CRC(01b53045) SHA1(72d73bbb09358b331696cd1cc44fc4958feffbe2))
GTS80SS22_ROMSTART("668-s1.snd", CRC(23d5045d) SHA1(a20bf02ece97e8238d1dbe8d35ca63d82b62431e),
                   "668-s2.snd", CRC(d63da498) SHA1(84dd87783f47fbf64b1830284c168501f9b455e2))
GTS80_ROMEND
#define input_ports_blkhole7 input_ports_blckhole
CORE_CLONEDEFNV(blkhole7,blckhole,"Black Hole (7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 2)
/-------------------------------------------------------------------*/
INIT_S80(blkhole2, dispNumeric2, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("668-2.cpu",  CRC(df03ffea) SHA1(7ca8fc321f74b9193104c282c7b4b92af93694c9))
GTS80SS22_ROMSTART("668-s1.snd", CRC(23d5045d) SHA1(a20bf02ece97e8238d1dbe8d35ca63d82b62431e),
                   "668-s2.snd", CRC(d63da498) SHA1(84dd87783f47fbf64b1830284c168501f9b455e2))
GTS80_ROMEND
#define input_ports_blkhole2 input_ports_blckhole
CORE_CLONEDEFNV(blkhole2,blckhole,"Black Hole (rev. 2)",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Black Hole (Sound Only)
/-------------------------------------------------------------------*/
INIT_S80(blkholea, dispNumeric2, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("668-a2.cpu" ,  CRC(df56f896) SHA1(1ec945a7ed8d25064476791adab2b554371dadbe))
GTS80S1K_ROMSTART("668-a-s.snd",  CRC(5175f307) SHA1(97be8f2bbc393cc45a07fa43daec4bbba2336af8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_blkholea input_ports_gts80
CORE_CLONEDEFNV(blkholea,gts80s,"Black Hole (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(blkhol7s, dispNumeric4, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("668-a2.cpu" ,  CRC(df56f896) SHA1(1ec945a7ed8d25064476791adab2b554371dadbe))
GTS80S1K_ROMSTART("668-a-s.snd",  CRC(5175f307) SHA1(97be8f2bbc393cc45a07fa43daec4bbba2336af8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_blkhol7s input_ports_blkholea
CORE_CLONEDEFNV(blkhol7s,blkholea,"Black Hole (Sound Only, 7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Haunted House, since serial no. 5000
/-------------------------------------------------------------------*/
INIT_S80(hh, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-2.cpu",  CRC(f3085f77) SHA1(ebd43588401a735d9c941d06d67ac90183139e90))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh input_ports_gts80
CORE_CLONEDEFNV(hh,gts80,"Haunted House (rev. 2)",1982,"Gottlieb",gl_mGTS80SS,0)

INIT_S80D7(hh7, dispNumeric4, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-2.cpu",  CRC(f3085f77) SHA1(ebd43588401a735d9c941d06d67ac90183139e90))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh7 input_ports_hh
CORE_CLONEDEFNV(hh7,hh,"Haunted House (rev. 2, 7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

INIT_S80(hh_3, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-3.cpu",  CRC(cf178411) SHA1(284e709ff3a569e84d1499a23f41adcbd8553930))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh_3 input_ports_hh
CORE_CLONEDEFNV(hh_3,hh,"Haunted House (rev. 3 unofficial MOD)",1982,"Flipprojets",gl_mGTS80SS,0)

INIT_S80(hh_3a, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-3a.cpu", CRC(effe6851) SHA1(fdf2fdddfebdf9c871d4395c307bf1f3ca2b2d10))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh_3a input_ports_hh
CORE_CLONEDEFNV(hh_3a,hh,"Haunted House (rev. 3 unofficial MOD, LED)",1982,"Flipprojets",gl_mGTS80SS,0)

INIT_S80(hh_3b, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-3b.cpu", CRC(2bfceb85) SHA1(9635cd29d5b53a4641b69f1648c1201924edd486))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh_3b input_ports_hh
CORE_CLONEDEFNV(hh_3b,hh,"Haunted House (rev. 3 unofficial MOD, LED+Secret Tunnel)",1982,"Flipprojets",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Haunted House, since serial no. 5000 with added Speech and additional hardware Votrax MOD
/-------------------------------------------------------------------*/
INIT_S80(hh_4, dispNumeric2, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("669-4.cpu",  CRC(8724cdf8) SHA1(9c8d7433ea14b8b50014deb7574c9d5043a794cc))
GTS80SS22_ROMSTART("669-4-s1.snd", NO_DUMP, // unreleased by the Flipprojets guys so far
                   "669-4-s2.snd", NO_DUMP)
GTS80_ROMEND
#define input_ports_hh_4 input_ports_hh
CORE_CLONEDEFNV(hh_4,hh,"Haunted House (rev. 4 unofficial Votrax speech MOD)",1982,"Flipprojets",gl_mGTS80SS,0)

INIT_S80(hh_4a, dispNumeric2, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("669-4a.cpu", CRC(7b906d4b) SHA1(b22c5c94e52190fbf514b531674887be42a1f559))
GTS80SS22_ROMSTART("669-4-s1.snd", NO_DUMP,
                   "669-4-s2.snd", NO_DUMP)
GTS80_ROMEND
#define input_ports_hh_4a input_ports_hh
CORE_CLONEDEFNV(hh_4a,hh,"Haunted House (rev. 4 unofficial Votrax speech MOD, LED)",1982,"Flipprojets",gl_mGTS80SS,0)

INIT_S80(hh_4b, dispNumeric2, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("669-4b.cpu", CRC(71025a8a) SHA1(5a682a6dbff825217e000df4f824dea6ad89223b))
GTS80SS22_ROMSTART("669-4-s1.snd", NO_DUMP,
                   "669-4-s2.snd", NO_DUMP)
GTS80_ROMEND
#define input_ports_hh_4b input_ports_hh
CORE_CLONEDEFNV(hh_4b,hh,"Haunted House (rev. 4 unofficial Votrax speech MOD, LED+Secret Tunnel)",1982,"Flipprojets",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Haunted House up to serial no. 4999
/-------------------------------------------------------------------*/
INIT_S80(hh_1, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-1.cpu",  CRC(96e72b93) SHA1(3eb3d3e064ba2fe637bba2a93ffd07f00edfa0f2))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh_1 input_ports_hh
CORE_CLONEDEFNV(hh_1,hh,"Haunted House (rev. 1)",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Eclipse
/-------------------------------------------------------------------*/
INIT_S80(eclipse, dispNumeric2, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("671-a.cpu",    CRC(efad7312) SHA1(fcfd5e5c7924d65ac42561994797156a80018667))
GTS80S1K_ROMSTART("671-a-s.snd",  CRC(5175f307) SHA1(97be8f2bbc393cc45a07fa43daec4bbba2336af8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_eclipse input_ports_gts80
CORE_CLONEDEFNV(eclipse,gts80s,"Eclipse",1981,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(eclipse7, dispNumeric4, SNDBRD_GTS80S)
GTS80_1_ROMSTART ("671-a.cpu",    CRC(efad7312) SHA1(fcfd5e5c7924d65ac42561994797156a80018667))
GTS80S1K_ROMSTART("671-a-s.snd",  CRC(5175f307) SHA1(97be8f2bbc393cc45a07fa43daec4bbba2336af8),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_eclipse7 input_ports_eclipse
CORE_CLONEDEFNV(eclipse7,eclipse,"Eclipse (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ System 80 Test Fixture
/-------------------------------------------------------------------*/
INIT_S80(s80tst, dispNumeric1, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("80tst.cpu",    CRC(a0f9e56b) SHA1(5146745ab61fea4b3070c6cf4324a9e77a7cee36))
GTS80SS22_ROMSTART("80tst-s1.snd", CRC(b9dbdd21) SHA1(dfe42c9e6e02f82ffd0cafe164df3211cdc2d966),
                   "80tst-s2.snd", CRC(1a4b1e9d) SHA1(18e7ffbdbdaf83ab1c8daa5fa5201d9f54390758))
GTS80_ROMEND
#define input_ports_s80tst input_ports_gts80
CORE_CLONEDEFNV(s80tst,gts80,"System 80 Test Fixture",1981,"Gottlieb",gl_mGTS80SS,0)

// System 80a

/*-------------------------------------------------------------------
/ Devil's Dare (Sound and Speech) (#670)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispDevilsdare[] = {
  DISP_SEG_IMPORT(dispNumeric3),
  {6, 1,50,6,CORE_SEG9}, {6,17,44,6,CORE_SEG9}, {0}
};
INIT_S80A(dvlsdre, dispDevilsdare, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("670-1.cpu",  CRC(6318bce2) SHA1(1b13a87d18693fe7986fdd79bd00a80d877940c3))
GTS80SS22_ROMSTART("670-s1.snd", CRC(506bc22a) SHA1(3c69f8d0c38c51796c31fb38c02d00afe8a4b8c5),
                   "670-s2.snd", CRC(f662ee4b) SHA1(0f63e01672b7c07a4913e150f0bbe07ecfc06e7c))
GTS80_ROMEND
#define input_ports_dvlsdre input_ports_gts80
CORE_CLONEDEFNV(dvlsdre,gts80a,"Devil's Dare",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Devil's Dare (Sound Only) (#670)
/-------------------------------------------------------------------*/
INIT_S80A(dvlsdre2, dispDevilsdare, SNDBRD_GTS80S,0)
GTS80_1_ROMSTART ("670-a.cpu",    CRC(353b2e18) SHA1(270365ea8276b64e38939f0bf88ddb955d59cd4d))
GTS80S1K_ROMSTART("670-a-s.snd",  CRC(f141d535) SHA1(91e4ab9ce63b5ff3e395b6447a104286327b5533),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_dvlsdre2 input_ports_gts80
CORE_CLONEDEFNV(dvlsdre2,gts80as,"Devil's Dare (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Caveman (#PV-810) Pinball/Video Combo
/-------------------------------------------------------------------*/
INIT_S80A(caveman, GTS80_dispCaveman, SNDBRD_GTS80SS_VOTRAX,GTS80_DISPVIDEO)
GTS80_1_ROMSTART  ("pv810-1.cpu", CRC(dd8d516c) SHA1(011d8744a7984ed4c7ceb1f57dcbd8fdb22e21fe))
GTS80SS22_ROMSTART("pv810-s1.snd",CRC(a491664d) SHA1(45031bcbddb75b4f3a5c3b623a0f2723fb95f92f),
                   "pv810-s2.snd",CRC(d8654e6e) SHA1(75d4f1f966ed5a1632536723229166b9cc7d77c7))
VIDEO_ROMSTART    ("v810-u8.bin", CRC(514aa152) SHA1(f61a98bbc95f202417cf97b35fe9835108200477),
                   "v810-u7.bin", CRC(74c6533e) SHA1(8fe373c28dc4089bd9e573c69682113315236c72),
                   "v810-u6.bin", CRC(2fd0ee95) SHA1(8374b7729b2de9e73784617ada6f9d895f54cc8d),
                   "v810-u5.bin", CRC(2fb15da3) SHA1(ba2927bc88c1ee1b8dd682234b2616d2013c7e7c),
                   "v810-u4.bin", CRC(2dfe8492) SHA1(a29604cda968504f95577e36c715ae97034bb5f8),
                   "v810-u3.bin", CRC(740e9ec3) SHA1(ba4839680694bf5acff540147af4319c64c313e8),
                   "v810-u2.bin", CRC(b793baf9) SHA1(cf1618cd0134529d057bc8245b9b366c3aae2326),
                   "v810-u1.bin", CRC(0a283b15) SHA1(4a57ae5be36500c22b55ac17dc71968bd833298b))
GTS80_ROMEND
#define input_ports_caveman input_ports_gts80vid
CORE_CLONEDEFNV(caveman,gts80a,"Caveman (Pinball/Video Combo)",1981,"Gottlieb",gl_mGTS80VID,0)

INIT_S80A(cavemana, GTS80_dispCaveman, SNDBRD_GTS80SS_VOTRAX,GTS80_DISPVIDEO)
GTS80_1_ROMSTART  ("pv810-1.cpu", CRC(dd8d516c) SHA1(011d8744a7984ed4c7ceb1f57dcbd8fdb22e21fe))
GTS80SS22_ROMSTART("pv810-s1.snd",CRC(a491664d) SHA1(45031bcbddb75b4f3a5c3b623a0f2723fb95f92f),
                   "pv810-s2.snd",CRC(d8654e6e) SHA1(75d4f1f966ed5a1632536723229166b9cc7d77c7))
VIDEO_ROMSTART    ("v810-u8.bin", CRC(514aa152) SHA1(f61a98bbc95f202417cf97b35fe9835108200477),
                   "v810-u7.bin", CRC(74c6533e) SHA1(8fe373c28dc4089bd9e573c69682113315236c72),
                   "v810-u6.bin", CRC(2fd0ee95) SHA1(8374b7729b2de9e73784617ada6f9d895f54cc8d),
                   "v810-u5.bin", CRC(2fb15da3) SHA1(ba2927bc88c1ee1b8dd682234b2616d2013c7e7c),
                   "v810-u4a.bin",CRC(3437c697) SHA1(e35822ed04eeb7f8a54a0bfdd2b63d54fa9b2263),
                   "v810-u3a.bin",CRC(729819f6) SHA1(6f684d05d1dcdbb975d3b97cfa0b1d657e7a98a5),
                   "v810-u2a.bin",CRC(ab6193c2) SHA1(eb898b3a3dfef15f992f7ef6f2d636a3e124ca13),
                   "v810-u1a.bin",CRC(7c6410fb) SHA1(6606d853d4955ce18ace71814bd2ae3d25e0c046))
GTS80_ROMEND
#define input_ports_cavemana input_ports_caveman
CORE_CLONEDEFNV(cavemana,caveman,"Caveman (Pinball/Video Combo) (set 2)",1981,"Gottlieb",gl_mGTS80VID,0)

INIT_S80A(cavemane, GTS80_dispCaveman, SNDBRD_GTS80SS_VOTRAX,GTS80_DISPVIDEO)
GTS80_1_ROMSTART  ("pv810-2.cpu", CRC(341697b9) SHA1(c7ca7227dd655380043b083f580baf2eaaedc034))
GTS80SS22_ROMSTART("pv810-s1.snd",CRC(a491664d) SHA1(45031bcbddb75b4f3a5c3b623a0f2723fb95f92f),
                   "pv810-s2.snd",CRC(d8654e6e) SHA1(75d4f1f966ed5a1632536723229166b9cc7d77c7))
VIDEO_ROMSTART    ("v810-u8.bin", CRC(514aa152) SHA1(f61a98bbc95f202417cf97b35fe9835108200477),
                   "v810-u7.bin", CRC(74c6533e) SHA1(8fe373c28dc4089bd9e573c69682113315236c72),
                   "v810-u6.bin", CRC(2fd0ee95) SHA1(8374b7729b2de9e73784617ada6f9d895f54cc8d),
                   "v810-u5.bin", CRC(2fb15da3) SHA1(ba2927bc88c1ee1b8dd682234b2616d2013c7e7c),
                   "v810-u4.bin", CRC(2dfe8492) SHA1(a29604cda968504f95577e36c715ae97034bb5f8),
                   "v810-u3.bin", CRC(740e9ec3) SHA1(ba4839680694bf5acff540147af4319c64c313e8),
                   "v810-u2.bin", CRC(b793baf9) SHA1(cf1618cd0134529d057bc8245b9b366c3aae2326),
                   "v810-u1.bin", CRC(0a283b15) SHA1(4a57ae5be36500c22b55ac17dc71968bd833298b))
GTS80_ROMEND
#define input_ports_cavemane input_ports_caveman
CORE_CLONEDEFNV(cavemane,caveman,"Caveman (Pinball/Video Combo) (Evolution, unofficial MOD)",1981,"Flipprojets",gl_mGTS80VID,0)

/*-------------------------------------------------------------------
/ Rocky (#672)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispRocky[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6, 10,54,2,CORE_SEG9}, {6,16,50,2,CORE_SEG9}, {0}
};
INIT_S80A(rocky, dispRocky, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("672-2x.cpu", CRC(8e2f0d39) SHA1(eb0982d2bfa910b3c95d6d55c04dc58395789411))
GTS80SS22_ROMSTART("672-s1.snd", CRC(10ba523c) SHA1(4289acd1437d7bf69fb442884a98290dc1b5f493),
                   "672-s2.snd", CRC(5e77117a) SHA1(7836b1ee0b2afe621ae414d5710111b550db0e63))
GTS80_ROMEND
#define input_ports_rocky input_ports_gts80
CORE_CLONEDEFNV(rocky,gts80a,"Rocky",1982,"Gottlieb",gl_mGTS80SS,0)

INIT_S80A(rockyf, dispRocky, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("672-2x.cpu", CRC(8e2f0d39) SHA1(eb0982d2bfa910b3c95d6d55c04dc58395789411))
GTS80SS22_ROMSTART("f672-s1.snd", CRC(57a0ce22) SHA1(cdc167b5eb72e8c3235d3ffd9143faf8e6c0a2ef),
                   "f672-s2.snd", CRC(87a0474f) SHA1(62fe995f3bc7fe23422d75b043d508c2f84f745a))
GTS80_ROMEND
#define input_ports_rockyf input_ports_gts80
CORE_CLONEDEFNV(rockyf,rocky,"Rocky (French Speech)",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Spirit (#673)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispSpirit[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6, 9,50,6,CORE_SEG9}, {0}
};
INIT_S80A(spirit, dispSpirit, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("673-2.cpu",  CRC(a7dc2207) SHA1(9098e740639af364a12857f89bdc4e2c7c89ff23))
GTS80SS22_ROMSTART("673-s1.snd", CRC(fd3062ae) SHA1(6eae04ec470afd4363ca448ee106e3e89fbf471e),
                   "673-s2.snd", CRC(7cf923f1) SHA1(2182324c30e8cb22735e59b74d4f6b268d3750e6))
GTS80_ROMEND
#define input_ports_spirit input_ports_gts80
CORE_CLONEDEFNV(spirit,gts80a,"Spirit",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Striker (#675)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispStriker[] = { /* as displayed on real machine */
  {2, 0, 2, 7,CORE_SEG98F}, {6, 0, 9, 7,CORE_SEG98F},
  {2,16,22, 7,CORE_SEG98F}, {6,16,29, 7,CORE_SEG98F},
  {0, 2,40, 2,CORE_SEG7S},  {0, 8,42, 2,CORE_SEG7S}, /* ball/credit */
  {4, 2,52, 2,CORE_SEG9},   {4, 8,50, 2,CORE_SEG9},
  {4,18,48, 2,CORE_SEG9},   {4,24,46, 2,CORE_SEG9}, {0}
};
INIT_S80A(striker, dispStriker, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("675.cpu",    CRC(06b66ce8) SHA1(399d98753e2da5c835c629a673069e853a4ce3c3))
GTS80SS22_ROMSTART("675-s1.snd", CRC(cc11c487) SHA1(fe880dd7dc03f368b2c7ea81059c4b176018b86e),
                   "675-s2.snd", CRC(ec30a3d9) SHA1(895be373598786d618bed635fe43daae7245c8ac))
GTS80_ROMEND
#define input_ports_striker input_ports_gts80
CORE_CLONEDEFNV(striker,gts80a,"Striker",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Punk! (#674)
/-------------------------------------------------------------------*/
INIT_S80A(punk, dispNumeric3, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("674.cpu",    CRC(70cccc57) SHA1(c2446ecf072174ce3e8524c1a01b1eea72875226))
GTS80SS22_ROMSTART("674-s1.snd", CRC(b75f79d5) SHA1(921774dacccb025c9465ea7e24534aca2d29d6f1),
                   "674-s2.snd", CRC(005d123a) SHA1(ebe258786de09488ec0a104a47e208c66b3613b5))
GTS80_ROMEND
#define input_ports_punk input_ports_gts80
CORE_CLONEDEFNV(punk,gts80a,"Punk!",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Krull (#676)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispKrull[] = {
  DISP_SEG_IMPORT(dispNumeric3),
  {6, 8,50,3,CORE_SEG9}, {6,16,53,3,CORE_SEG9}, {0}
};
INIT_S80A(krull, dispKrull, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("676-3.cpu",  CRC(71507430) SHA1(cbd7dd186ec928829585d3166ec10956d708d850))
GTS80SS22_ROMSTART("676-s1.snd", CRC(b1989d8f) SHA1(f1a7eac8aa9c7685f4d37f1c73bba27f4fa8b6ae),
                   "676-s2.snd", CRC(05fade11) SHA1(538f6225235b5338504597acdf6bafd1de24284e))
GTS80_ROMEND
#define input_ports_krull input_ports_gts80
CORE_CLONEDEFNV(krull,gts80a,"Krull",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Q*Bert's Quest (#677)
/-------------------------------------------------------------------*/
INIT_S80A(qbquest, dispNumeric3, SNDBRD_GTS80SS_VOTRAX,0)
GTS80_1_ROMSTART  ("677.cpu",    CRC(fd885874) SHA1(d4414949eca45fd063c4f31079e9fa095044ab9c))
GTS80SS22_ROMSTART("677-s1.snd", CRC(af7bc8b7) SHA1(33100d63629be7a5b768efd82a1ed1280c845d25),
                   "677-s2.snd", CRC(820aa26f) SHA1(7181ceedcf61204277d7b9fdba621915960999ad))
GTS80_ROMEND
#define input_ports_qbquest input_ports_gts80
CORE_CLONEDEFNV(qbquest,gts80a,"Q*Bert's Quest",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Super Orbit (#680)
/-------------------------------------------------------------------*/
INIT_S80A(sorbit, dispNumeric3, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("680.cpu",    CRC(decf84e6) SHA1(0c6f5e1abac58aede15016b5e30db72d1a3f6c11))
GTS80SS22_ROMSTART("680-s1.snd", CRC(fccbbbdd) SHA1(089f2b15ab1cc46550351614e18d8915b3d6a8bf),
                   "680-s2.snd", CRC(d883d63d) SHA1(1777a16bc9df7e5be2643ed18754ba120c7a954b))
GTS80_ROMEND
#define input_ports_sorbit input_ports_gts80
CORE_CLONEDEFNV(sorbit,gts80a,"Super Orbit",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Royal Flush Deluxe (#681)
/-------------------------------------------------------------------*/
INIT_S80A(rflshdlx, dispNumeric3, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("681-2.cpu",  CRC(0b048658) SHA1(c68ce525cbb44194090df17401b220d6a070eccb))
GTS80SS22_ROMSTART("681-s1.snd", CRC(33455bbd) SHA1(04db645060d93d7d9faff56ead9fa29a9c4723ec),
                   "681-s2.snd", CRC(639c93f9) SHA1(1623fea6681a009e7a755357fa85206cf2ce6897))
GTS80_ROMEND
#define input_ports_rflshdlx input_ports_gts80
CORE_CLONEDEFNV(rflshdlx,gts80a,"Royal Flush Deluxe",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Goin' Nuts (#682)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispGoinNuts[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6,12,53,3,CORE_SEG9}, {0}
};
INIT_S80A(goinnuts, dispGoinNuts, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("682.cpu",    CRC(51c7c6de) SHA1(31accbc8d29038679f2b0396202490233657e538))
GTS80SS22_ROMSTART("682-s1.snd", CRC(f00dabf3) SHA1(a6e3078220ab23dc41fd48fd528e679aefec3693),
                   "682-s2.snd", CRC(3be8ac5f) SHA1(0112d3417c0793e672733eff58058d8c9ad10421))
GTS80_ROMEND
#define input_ports_goinnuts input_ports_gts80
CORE_CLONEDEFNV(goinnuts,gts80a,"Goin' Nuts",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Amazon Hunt (#684)
/-------------------------------------------------------------------*/
INIT_S80A(amazonh, dispNumeric3, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("684-2.cpu",  CRC(b0d0c4af) SHA1(e81f568983d95cecb62d34598c40c5a5e6dcb3e2))
GTS80SS22_ROMSTART("684-s1.snd", CRC(86d239df) SHA1(f18efdc6b84d18b1cf01e79224284c5180c57d22),
                   "684-s2.snd", CRC(4d8ea26c) SHA1(d76d535bf29297247f1e5abd080a52b7dfc3811b))
GTS80_ROMEND
#define input_ports_amazonh input_ports_gts80
CORE_CLONEDEFNV(amazonh,gts80a,"Amazon Hunt",1983,"Gottlieb",gl_mGTS80SS,0)

INIT_S80A(amazonha, dispNumeric3, SNDBRD_GTS80SS,0)
GTS80_1_ROMSTART  ("684-1.cpu",  CRC(7fac5132) SHA1(2fbcda45935c1817b2230598921b86c6f52564c8))
GTS80SS22_ROMSTART("684-s1.snd", CRC(86d239df) SHA1(f18efdc6b84d18b1cf01e79224284c5180c57d22),
                   "684-s2.snd", CRC(4d8ea26c) SHA1(d76d535bf29297247f1e5abd080a52b7dfc3811b))
GTS80_ROMEND
#define input_ports_amazonha input_ports_amazonh
CORE_CLONEDEFNV(amazonha, amazonh, "Amazon Hunt (alternate set)",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Rack 'Em Up (#685)
/-------------------------------------------------------------------*/
INIT_S80A(rackemup, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("685.cpu",   CRC(4754d68d) SHA1(2af743287c1a021f3e130d3d6e191ec9724d640c))
GTS80S2K_ROMSTART("685-s.snd", CRC(d4219987) SHA1(7385d8723bdc937e7c9d6bf7f26ca06f64a9a212))
GTS80_ROMEND
#define input_ports_rackemup input_ports_gts80
CORE_CLONEDEFNV(rackemup,gts80as,"Rack 'Em Up",1983,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ Ready...Aim...Fire! (#686)
/-------------------------------------------------------------------*/
INIT_S80A(raimfire, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("686.cpu",   CRC(d1e7a0de) SHA1(b9af2fcaadc55d37c7d9d22621c3817eb751de6b))
GTS80S2K_ROMSTART("686-s.snd", CRC(09740682) SHA1(4f36d78207bd5b8e7abb7118f03acbb3885173c2))
GTS80_ROMEND
#define input_ports_raimfire input_ports_gts80
CORE_CLONEDEFNV(raimfire,gts80as,"Ready...Aim...Fire!",1983,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ Jacks To Open (#687)
/-------------------------------------------------------------------*/
INIT_S80A(jack2opn, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("687.cpu",   CRC(0080565e) SHA1(c08412ba24d2ffccf11431e80bd2fc95fc4ce02b))
GTS80S2K_ROMSTART("687-s.snd", CRC(f9d10b7a) SHA1(db255711ed6cb46d183c0ae3894df447f3d8a8e3))
GTS80_ROMEND
#define input_ports_jack2opn input_ports_gts80
CORE_CLONEDEFNV(jack2opn,gts80as,"Jacks to Open",1984,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ Alien Star (#689)
/-------------------------------------------------------------------*/
INIT_S80A(alienstr, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("689a.cpu",  CRC(4262006b) SHA1(66520b66c31efd0dc654630b2d3567da799b4d89))
GTS80S2K_ROMSTART("689-s.snd", CRC(e1e7a610) SHA1(d4eddfc970127cf3a7d086ad46cbc7b95fdc269d))
GTS80_ROMEND
#define input_ports_alienstr input_ports_gts80
CORE_CLONEDEFNV(alienstr,gts80as,"Alien Star",1984,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ The Games (#691)
/-------------------------------------------------------------------*/
INIT_S80A(thegames, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("691.cpu",   CRC(50f620ea) SHA1(2f997a637eba4eb362586d3aa8caac44acccc795))
GTS80S2K_ROMSTART("691-s.snd", CRC(d7011a31) SHA1(edf5de6cf5ddc1eb577dd1d8dcc9201522df8315))
GTS80_ROMEND
#define input_ports_thegames input_ports_gts80
CORE_CLONEDEFNV(thegames,gts80as,"Games, The",1984,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ Touchdown (#688)
/-------------------------------------------------------------------*/
INIT_S80A(touchdn, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("688.cpu",   CRC(e531ab3f) SHA1(695aef0dd911fee27ac2d1493a9646b5430a07d5))
GTS80S2K_ROMSTART("688-s.snd", CRC(5e9988a6) SHA1(5f531491722d3c30cf4a7c17982813a7c548387a))
GTS80_ROMEND
#define input_ports_touchdn input_ports_gts80
CORE_CLONEDEFNV(touchdn,gts80as,"Touchdown",1984,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ El Dorado City of Gold (#692)
/-------------------------------------------------------------------*/
INIT_S80A(eldorado, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("692-2.cpu", CRC(4ee6d09b) SHA1(5da0556204e76029380366f9fbb5662715cc3257))
GTS80S2K_ROMSTART("692-s.snd", CRC(9bfbf400) SHA1(58aed9c0b1f52bcd0b53edcdf7af576bb175e3d6))
GTS80_ROMEND
#define input_ports_eldorado input_ports_gts80
CORE_CLONEDEFNV(eldorado,gts80as,"El Dorado City of Gold",1984,"Gottlieb",gl_mGTS80SP,0)

/*-------------------------------------------------------------------
/ Ice Fever (#695)
/-------------------------------------------------------------------*/
INIT_S80A(icefever, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("695.cpu",   CRC(2f6e9caf) SHA1(4f9eeafcbaf758ee6bbad74611b4912ff75b8576))
GTS80S2K_ROMSTART("695-s.snd", CRC(daededc2) SHA1(b43303c1e39b21f3fcbc339d440ea051ced1ea26))
GTS80_ROMEND
#define input_ports_icefever input_ports_gts80
CORE_CLONEDEFNV(icefever,gts80as,"Ice Fever",1985,"Gottlieb",gl_mGTS80SP,0)

// System 80b

/*-------------------------------------------------------------------
/ Chicago Cubs Triple Play (#696)
/-------------------------------------------------------------------*/
// using System80 sound only board
INITGAME(triplay, GEN_GTS80B, FLIP616, dispAlpha,SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(triplay, "prom1.cpu", CRC(42b29b01) SHA1(58145ce10939d00faff49972ada669005a223792))
GTS80S2K_ROMSTART(          "696-s.snd", CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define input_ports_triplay input_ports_gts80
CORE_GAMEDEFNV(triplay, "Chicago Cubs Triple Play",1985,"Gottlieb",gl_mGTS80B,0)

GTS80B_8K_ROMSTART(triplaya, "prom1a.cpu", CRC(fc2145cb) SHA1(f7b9648c533997e9f777a8b40dad9852f26abd9a))
GTS80S2K_ROMSTART(           "696-s.snd",  CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define init_triplaya init_triplay
#define input_ports_triplaya input_ports_triplay
CORE_CLONEDEFNV(triplaya, triplay, "Chicago Cubs Triple Play (rev. 1)",1985,"Gottlieb",gl_mGTS80B,0)

GTS80B_8K_ROMSTART(triplyfp, "prom1_fp.cpu", CRC(521946d4) SHA1(527ed3f221e0ca5fe1778e3095c9b8a414911206))
GTS80S2K_ROMSTART(           "696-s.snd",    CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define init_triplyfp init_triplay
#define input_ports_triplyfp input_ports_triplay
CORE_CLONEDEFNV(triplyfp, triplay, "Chicago Cubs Triple Play (Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

GTS80B_8K_ROMSTART(triplyf1, "prom1_f1.cpu", CRC(4b58be44) SHA1(db7734692b3ff158cbd229b2d3ca723cfe963c7b))
GTS80S2K_ROMSTART(           "696-s.snd",    CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define init_triplyf1 init_triplay
#define input_ports_triplyf1 input_ports_triplay
CORE_CLONEDEFNV(triplyf1, triplay, "Chicago Cubs Triple Play (rev. 1 Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

GTS80B_8K_ROMSTART(triplayg, "prom1g.cpu", CRC(5e2bf7a9) SHA1(fdbec615b22416bb4b2e712d47c54c945d849252))
GTS80S2K_ROMSTART(           "696-s.snd",  CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define init_triplayg init_triplay
#define input_ports_triplayg input_ports_triplay
CORE_CLONEDEFNV(triplayg, triplay, "Chicago Cubs Triple Play (German)",1985,"Gottlieb",gl_mGTS80B,0)

GTS80B_8K_ROMSTART(triplgfp, "prom1g_fp.cpu",CRC(7b6e6819) SHA1(ca2a739301a8be1ff7dd139171cd28f29d5aad59))
GTS80S2K_ROMSTART(           "696-s.snd",    CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define init_triplgfp init_triplay
#define input_ports_triplgfp input_ports_triplay
CORE_CLONEDEFNV(triplgfp, triplay, "Chicago Cubs Triple Play (German Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

/*-------------------------------------------------------------------
/ Bounty Hunter (#694)
/-------------------------------------------------------------------*/
// using System80 sound only board
INITGAME(bountyh, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(bountyh, "prom1.cpu", CRC(e8190df7) SHA1(5304918d35e379da17ab19d8879a7ace5c864326))
GTS80S2K_ROMSTART(          "694-s.snd", CRC(a0383e41) SHA1(156514d2b52fcd89b608b85991c5066780949979))
GTS80_ROMEND
#define input_ports_bountyh input_ports_gts80
CORE_GAMEDEFNV(bountyh, "Bounty Hunter",1985,"Gottlieb",gl_mGTS80B,0)

INITGAME(bounthfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(bounthfp, "prom1_fp.cpu", CRC(9e68b714) SHA1(bb33b1e8fb50776731c450e2c05c49dcd5535f41))
GTS80S2K_ROMSTART(           "694-s.snd",    CRC(a0383e41) SHA1(156514d2b52fcd89b608b85991c5066780949979))
GTS80_ROMEND
#define input_ports_bounthfp input_ports_bountyh
CORE_CLONEDEFNV(bounthfp, bountyh, "Bounty Hunter (Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

// #694Y
INITGAME(bountyhg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(bountyhg, "prom1g.cpu", CRC(ea4b7e2d) SHA1(9141c950b33e32ae8ad76fd0dd06d1a13d38be9d))
GTS80S2K_ROMSTART(           "694-s.snd",  CRC(a0383e41) SHA1(156514d2b52fcd89b608b85991c5066780949979))
GTS80_ROMEND
#define input_ports_bountyhg input_ports_bountyh
CORE_CLONEDEFNV(bountyhg, bountyh, "Bounty Hunter (German)",1985,"Gottlieb",gl_mGTS80B,0)

INITGAME(bountgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(bountgfp, "prom1g_fp.cpu", CRC(07b9333f) SHA1(1355201be26ac8f7bca96275443e33c9a01eedf3))
GTS80S2K_ROMSTART(           "694-s.snd",     CRC(a0383e41) SHA1(156514d2b52fcd89b608b85991c5066780949979))
GTS80_ROMEND
#define input_ports_bountgfp input_ports_bountyh
CORE_CLONEDEFNV(bountgfp, bountyh, "Bounty Hunter (German Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

/*-------------------------------------------------------------------
/ Tag-Team Pinball (#698)
/-------------------------------------------------------------------*/
INITGAME(tagteam, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteam, "prom2.cpu", CRC(fd1615ce) SHA1(3a6c3525552286b86e5340af2bf196f12adc9b35),
                            "prom1.cpu", CRC(65931038) SHA1(6d2f1a9fb1b3ce4610074fd3f2ac37ad6af70a44))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagteam input_ports_gts80
CORE_GAMEDEFNV(tagteam, "Tag-Team Pinball",1985,"Gottlieb",gl_mGTS80B,0) //!! rev.1 actually ??

INITGAME(tagteam2, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteam2,"prom2a.cpu", CRC(6d56b636) SHA1(8f50f2742be727835e7343307787b4b5daa1623a),
                            "prom1a.cpu", CRC(92766607) SHA1(29744dd3c447cc51fb123750ae1456329122e986))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagteam2 input_ports_tagteam
CORE_CLONEDEFNV(tagteam2,tagteam,"Tag-Team Pinball (rev. 2)",1985,"Gottlieb",gl_mGTS80B,0)

INITGAME(tagtemfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagtemfp,"prom2.cpu",    CRC(fd1615ce) SHA1(3a6c3525552286b86e5340af2bf196f12adc9b35),
                            "prom1_fp.cpu", CRC(3f052c44) SHA1(176fbe35a4ad5832b1ba61889a858b8585dc86be))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagtemfp input_ports_tagteam
CORE_CLONEDEFNV(tagtemfp,tagteam,"Tag-Team Pinball (Free Play)",1985,"Flipprojets",gl_mGTS80B,0) //!! rev.1 actually ??

// #698Y
INITGAME(tagteamg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteamg,"prom2g.cpu", CRC(5e6d2da7) SHA1(9b23d1ac34163edeaceffe806a2a559f3d408b41),
                            "prom1g.cpu", CRC(e206c519) SHA1(0d5b3237807b6f11633ab9be2b0e5b000369a0e8))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagteamg input_ports_tagteam
CORE_CLONEDEFNV(tagteamg,tagteam,"Tag-Team Pinball (German)",1985,"Gottlieb",gl_mGTS80B,0)

INITGAME(tagtmgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagtmgfp,"prom2g.cpu",    CRC(5e6d2da7) SHA1(9b23d1ac34163edeaceffe806a2a559f3d408b41),
                            "prom1g_fp.cpu", CRC(ae1ed7a2) SHA1(e1f640bd350c8c9edc8742de897d92bb58950c3c))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagtmgfp input_ports_tagteam
CORE_CLONEDEFNV(tagtmgfp,tagteam,"Tag-Team Pinball (German Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

INITGAME(tagtem2f, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagtem2f,"prom2a.cpu",   CRC(6d56b636) SHA1(8f50f2742be727835e7343307787b4b5daa1623a),
                            "prom1_2f.cpu", CRC(9c2c0058) SHA1(92e28a0e5fb454b046d1cd365e39ebdd6fa6baf1))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagtem2f input_ports_tagteam
CORE_CLONEDEFNV(tagtem2f,tagteam,"Tag-Team Pinball (rev. 2 Free Play)",1985,"Flipprojets",gl_mGTS80B,0)

/* These games carry a SP0250 sound chip, but none features speech from it:
- Rock (Encore)
- Raven
- Hollywood Heat
- Genesis
- Gold Wings
- Monte Carlo
- Spring Break
- Arena
also Amazon Hunt II, technically, but it doesn't sport a Y-ROM.

Only the System 80B Test Fixture has one speech command for the SP0250.
*/

/*-------------------------------------------------------------------
/ Rock (#697)
/-------------------------------------------------------------------*/
//(I assume these are using Gen.1 hardware, but there's 1 less rom, so who knows)
INITGAME(rock, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rock, "prom1.cpu", CRC(1146c1d3) SHA1(1e838756017cdc51239c082f8d491cd2824d273d))
GTS80BSSOUND88(          "drom1.snd", CRC(03830e81) SHA1(786f85eba5a8f5e9cc659305623e1d178b5410f6),
                         "yrom1.snd", CRC(effba2ad) SHA1(2288a4f655376e0aa18f8ecd9a3818ed4d6c6891))
GTS80_ROMEND
#define input_ports_rock input_ports_gts80
CORE_GAMEDEFNV(rock, "Rock",1985,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(rockfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rockfp, "prom1_fp.cpu", CRC(77736ac3) SHA1(30c6322dcda033cbd1cbda5f1bfe97c2067c37f5))
GTS80BSSOUND88(            "drom1.snd",    CRC(03830e81) SHA1(786f85eba5a8f5e9cc659305623e1d178b5410f6),
                           "yrom1.snd",    CRC(effba2ad) SHA1(2288a4f655376e0aa18f8ecd9a3818ed4d6c6891))
GTS80_ROMEND
#define input_ports_rockfp input_ports_rock
CORE_CLONEDEFNV(rockfp,rock, "Rock (Free Play)",1985,"Flipprojets",gl_mGTS80BS1,0)

// #697Y
INITGAME(rockg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rockg, "prom1g.cpu", CRC(2de3f1e5) SHA1(ceb964292703080bb742dbc073a14dbf745ad38e))
GTS80BSSOUND88(           "drom1.snd",  CRC(03830e81) SHA1(786f85eba5a8f5e9cc659305623e1d178b5410f6),
                          "yrom1.snd",  CRC(effba2ad) SHA1(2288a4f655376e0aa18f8ecd9a3818ed4d6c6891))
GTS80_ROMEND
#define input_ports_rockg input_ports_rock
CORE_CLONEDEFNV(rockg,rock, "Rock (German)",1985,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(rockgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rockgfp, "prom1g_fp.cpu", CRC(792b4be4) SHA1(4ee2755024dcc31aaa4cb3b4266fa48291e49d23))
GTS80BSSOUND88(             "drom1.snd",     CRC(03830e81) SHA1(786f85eba5a8f5e9cc659305623e1d178b5410f6),
                            "yrom1.snd",     CRC(effba2ad) SHA1(2288a4f655376e0aa18f8ecd9a3818ed4d6c6891))
GTS80_ROMEND
#define input_ports_rockgfp input_ports_rock
CORE_CLONEDEFNV(rockgfp,rock, "Rock (German Free Play)",1985,"Flipprojets",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Raven (#702)
/-------------------------------------------------------------------*/
//(I assume these are using Gen.1 hardware, but there's 1 less rom, so who knows)
INITGAME(raven, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(raven, "prom2.cpu", CRC(481f3fb8) SHA1(22ffa55ed362219ebedbc40edcf866ff152a01b9),
                          "prom1.cpu", CRC(edc88561) SHA1(101878527307c6f04d141dd74e04102c4ea53105))
GTS80BSSOUND88(           "drom1.snd", CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                          "yrom1.snd", CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define input_ports_raven input_ports_gts80
CORE_GAMEDEFNV(raven, "Raven",1986,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(ravena, "prom2a.cpu",CRC(a693785e) SHA1(7c8878f1c3c5205b3ae46a78c881bbd2b722838d),
                           "prom1.cpu", CRC(edc88561) SHA1(101878527307c6f04d141dd74e04102c4ea53105)) // actually this would also be different, but in practice it isn't by accident (as the internal checksum for PROM2 stored in here is the same)
GTS80BSSOUND88(            "drom1.snd", CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                           "yrom1.snd", CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_ravena init_raven
#define input_ports_ravena input_ports_raven
CORE_CLONEDEFNV(ravena, raven, "Raven (rev. 1)",1986,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(ravenfp, "prom2.cpu",    CRC(481f3fb8) SHA1(22ffa55ed362219ebedbc40edcf866ff152a01b9),
                            "prom1_fp.cpu", CRC(d6e5120b) SHA1(1d00bce8170b5ad4185e6517ba1a0f46c8ae7444))
GTS80BSSOUND88(             "drom1.snd",    CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                            "yrom1.snd",    CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_ravenfp init_raven
#define input_ports_ravenfp input_ports_raven
CORE_CLONEDEFNV(ravenfp, raven, "Raven (Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #702Y
GTS80B_2K_ROMSTART(raveng, "prom2g.cpu", CRC(4ca540a5) SHA1(50bb240465d80b7763574e1261f8d0ddda5ad587),
                           "prom1g.cpu", CRC(3441aeda) SHA1(12dd2faac64170bad5cf5b9247283f64df9e5337))
GTS80BSSOUND88(            "drom1.snd",  CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                           "yrom1.snd",  CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_raveng init_raven
#define input_ports_raveng input_ports_raven
CORE_CLONEDEFNV(raveng, raven, "Raven (German)",1986,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(ravengfp, "prom2g.cpu",    CRC(4ca540a5) SHA1(50bb240465d80b7763574e1261f8d0ddda5ad587),
                             "prom1g_fp.cpu", CRC(ab3bbef5) SHA1(199ebb3359a1617148264b307b8b294c037f27a4))
GTS80BSSOUND88(              "drom1.snd",     CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                             "yrom1.snd",     CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_ravengfp init_raven
#define input_ports_ravengfp input_ports_raven
CORE_CLONEDEFNV(ravengfp, raven, "Raven (German Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(ravenafp, "prom2a.cpu",   CRC(a693785e) SHA1(7c8878f1c3c5205b3ae46a78c881bbd2b722838d),
                             "prom1_fp.cpu", CRC(d6e5120b) SHA1(1d00bce8170b5ad4185e6517ba1a0f46c8ae7444)) // actually this would also be different, but in practice it isn't by accident (as the internal checksum for PROM2 stored in here is the same)
GTS80BSSOUND88(              "drom1.snd",    CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                             "yrom1.snd",    CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_ravenafp init_raven
#define input_ports_ravenafp input_ports_raven
CORE_CLONEDEFNV(ravenafp, raven, "Raven (rev. 1 Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(rambo, "prom2a.cpu", CRC(a693785e) SHA1(7c8878f1c3c5205b3ae46a78c881bbd2b722838d),
                          "prom1r.cpu", CRC(51629598) SHA1(a5408fad2baec43633f407665f006fae74f3d9aa))
GTS80BSSOUND88(           "drom1.snd",  CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881),
                          "yrom1.snd",  CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55))
GTS80_ROMEND
#define init_rambo init_raven
#define input_ports_rambo input_ports_raven
CORE_CLONEDEFNV(rambo, raven, "Rambo (Raven unofficial MOD)",1986,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Rock Encore (#704)
/-------------------------------------------------------------------*/
INITGAME(rock_enc, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rock_enc, "prom1.cpu",  CRC(1146c1d3) SHA1(1e838756017cdc51239c082f8d491cd2824d273d))
GTS80BSSOUND888(             "drom1a.snd", CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                             "yrom1a.snd", CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                             "yrom2a.snd", CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_rock_enc input_ports_rock
CORE_CLONEDEFNV(rock_enc, rock, "Rock Encore",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(rock_efp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rock_efp, "prom1_fp.cpu", CRC(77736ac3) SHA1(30c6322dcda033cbd1cbda5f1bfe97c2067c37f5))
GTS80BSSOUND888(             "drom1a.snd",   CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                             "yrom1a.snd",   CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                             "yrom2a.snd",   CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_rock_efp input_ports_rock
CORE_CLONEDEFNV(rock_efp, rock, "Rock Encore (Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #704Y
INITGAME(rock_eg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rock_eg, "prom1g.cpu", CRC(2de3f1e5) SHA1(ceb964292703080bb742dbc073a14dbf745ad38e))
GTS80BSSOUND888(            "drom1a.snd", CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                            "yrom1a.snd", CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                            "yrom2a.snd", CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_rock_eg input_ports_rock
CORE_CLONEDEFNV(rock_eg, rock, "Rock Encore (German)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(rockegfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rockegfp, "prom1g_fp.cpu",CRC(792b4be4) SHA1(4ee2755024dcc31aaa4cb3b4266fa48291e49d23))
GTS80BSSOUND888(             "drom1a.snd",   CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                             "yrom1a.snd",   CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                             "yrom2a.snd",   CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_rockegfp input_ports_rock
CORE_CLONEDEFNV(rockegfp, rock, "Rock Encore (German Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

INITGAME(clash, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(clash,    "prom1c.cpu", CRC(410d02f6) SHA1(87968576bf5dcca886bcadd4ab379fff080e6eeb))
GTS80BSSOUND888(             "drom1a.snd", CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                             "yrom1a.snd", CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                             "yrom2a.snd", CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_clash input_ports_rock
CORE_CLONEDEFNV(clash, rock, "Clash, The (Rock Encore unofficial MOD)",2018,"Onevox",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Hollywood Heat (#703)
/-------------------------------------------------------------------*/
INITGAME(hlywoodh, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywoodh, "prom2.cpu", CRC(a465e5f3) SHA1(56afa2f67aebcd17345bba76ecb814653719ee7b),
                             "prom1.cpu", CRC(0493e27a) SHA1(72c603cda3cc43ed0f841a9fcc6f40d020475e74))
GTS80BSSOUND888(             "drom1.snd", CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd", CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd", CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywoodh input_ports_gts80
CORE_GAMEDEFNV(hlywoodh, "Hollywood Heat",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(hlywdhfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywdhfp, "prom2.cpu",    CRC(a465e5f3) SHA1(56afa2f67aebcd17345bba76ecb814653719ee7b),
                             "prom1_fp.cpu", CRC(63bc8395) SHA1(b4007b5a6b78e162c9b3e0243f01fa30501323ae))
GTS80BSSOUND888(             "drom1.snd",    CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",    CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",    CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywdhfp input_ports_hlywoodh
CORE_CLONEDEFNV(hlywdhfp,hlywoodh, "Hollywood Heat (Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #703Y
INITGAME(hlywodhg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywodhg, "prom2g.cpu", CRC(bf60b631) SHA1(944089895d4253dd094a8f6b7168f9e62a75568a),
                             "prom1g.cpu", CRC(0f212d15) SHA1(b671b8fbc50f5528f0de061c7695932035266a0e))
GTS80BSSOUND888(             "drom1.snd",  CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",  CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",  CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywodhg input_ports_hlywoodh
CORE_CLONEDEFNV(hlywodhg,hlywoodh, "Hollywood Heat (German)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(hlywhgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywhgfp, "prom2g.cpu",    CRC(bf60b631) SHA1(944089895d4253dd094a8f6b7168f9e62a75568a),
                             "prom1g_fp.cpu", CRC(11fa2432) SHA1(c08a7481d5d2f74ead9de1b0c8816d3dbf321f0f))
GTS80BSSOUND888(             "drom1.snd",     CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",     CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",     CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywhgfp input_ports_hlywoodh
CORE_CLONEDEFNV(hlywhgfp,hlywoodh, "Hollywood Heat (German Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #703X
INITGAME(hlywodhf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywodhf, "prom2f.cpu", CRC(969ca81f) SHA1(2606a0f63434056c5d2b509a885c9919a7a5d70f),
                             "prom1f.cpu", CRC(ddc45d2d) SHA1(8bd50f3e0049fe322f7bc626d39f9787cfea1940))
GTS80BSSOUND888(             "drom1.snd",  CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",  CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",  CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywodhf input_ports_hlywoodh
CORE_CLONEDEFNV(hlywodhf,hlywoodh, "Hollywood Heat (French)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(hlywhffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywhffp, "prom2f.cpu",    CRC(969ca81f) SHA1(2606a0f63434056c5d2b509a885c9919a7a5d70f),
                             "prom1f_fp.cpu", CRC(9b7df518) SHA1(02c649370c3424929813dcf8321bcc5f8cc85c88))
GTS80BSSOUND888(             "drom1.snd",     CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",     CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",     CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_hlywhffp input_ports_hlywoodh
CORE_CLONEDEFNV(hlywhffp,hlywoodh, "Hollywood Heat (French Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

INITGAME(bubba, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(bubba, "prom2.cpu",   CRC(a465e5f3) SHA1(56afa2f67aebcd17345bba76ecb814653719ee7b),
                          "prom1_b.cpu", CRC(6556d711) SHA1(9d0ccaf05d0aa5a68e5514a2ade7773959868bbb))
GTS80BSSOUND888(          "drom1.snd",   CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                          "yrom1.snd",   CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                          "yrom2.snd",   CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_bubba input_ports_hlywoodh
CORE_CLONEDEFNV(bubba,hlywoodh, "Bubba the Redneck Werewolf (Hollywood Heat unofficial MOD)",2017,"HauntFreaks",gl_mGTS80BS1,0)

INITGAME(beachbms, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(beachbms, "prom2.cpu",   CRC(a465e5f3) SHA1(56afa2f67aebcd17345bba76ecb814653719ee7b),
                             "prom1_bb.cpu",CRC(a035eb2d) SHA1(0f467b506bd514129e4175af3e35a666e09ec41b))
GTS80BSSOUND888(             "drom1.snd",   CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",   CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",   CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_beachbms input_ports_hlywoodh
CORE_CLONEDEFNV(beachbms,hlywoodh, "Beach Bums (Hollywood Heat unofficial MOD)",2018,"watacaractr",gl_mGTS80BS1,0)

INITGAME(tomjerry, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tomjerry, "prom2.cpu",   CRC(a465e5f3) SHA1(56afa2f67aebcd17345bba76ecb814653719ee7b),
                             "prom1_tj.cpu",CRC(45164b17) SHA1(40cf8bd6725d2e7d5e15cfaf8215e36c0d32183d))
GTS80BSSOUND888(             "drom1.snd",   CRC(a698ec33) SHA1(e7c1d28279ec4f12095c3a106c6cefcc2a84b31e),
                             "yrom1.snd",   CRC(9232591e) SHA1(72883e0c542c572226c6c654bea14749cc9e351f),
                             "yrom2.snd",   CRC(51709c2f) SHA1(5834d7b72bd36e30c87377dc7c3ad0cf26ff303a))
GTS80_ROMEND
#define input_ports_tomjerry input_ports_hlywoodh
CORE_CLONEDEFNV(tomjerry,hlywoodh, "Tom & Jerry (Hollywood Heat unofficial MOD)",2019,"watacaractr",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Genesis (#705)
/-------------------------------------------------------------------*/
INITGAME(genesis, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesis, "prom2.cpu", CRC(ac9f3a0f) SHA1(0e44888dc046121794e824d128628f991245c1cb),
                            "prom1.cpu", CRC(4a2f185c) SHA1(b45982b1ce9777292731ad523516c76cde4ddfa4))
GTS80BSSOUND888(            "drom1.snd", CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                            "yrom1.snd", CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                            "yrom2.snd", CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesis input_ports_gts80
CORE_GAMEDEFNV(genesis, "Genesis",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(genesifp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesifp, "prom2.cpu",    CRC(ac9f3a0f) SHA1(0e44888dc046121794e824d128628f991245c1cb),
                             "prom1_fp.cpu", CRC(662722b1) SHA1(1a7cf2f6cf92b5e6a288272ff785f215b241842f))
GTS80BSSOUND888(             "drom1.snd",    CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd",    CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd",    CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesifp input_ports_genesis
CORE_CLONEDEFNV(genesifp,genesis, "Genesis (Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #705Y
INITGAME(genesisg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesisg, "prom2g.cpu", CRC(e8fc30af) SHA1(2401bff3cf566cae4e6de6167fa004c5fe232928),
                             "prom1g.cpu", CRC(68a27ec1) SHA1(b14a933e6c7e2972faef8dfecebabe3da4021367))
GTS80BSSOUND888(             "drom1.snd",  CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd",  CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd",  CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesisg input_ports_genesis
CORE_CLONEDEFNV(genesisg,genesis, "Genesis (German)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(genesgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesgfp, "prom2g.cpu",   CRC(e8fc30af) SHA1(2401bff3cf566cae4e6de6167fa004c5fe232928),
                             "prom1g_fp.cpu",CRC(24af8cef) SHA1(4b54f5ed32afc11bf3dc8b16e046add6ddbf93ab))
GTS80BSSOUND888(             "drom1.snd",    CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd",    CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd",    CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesgfp input_ports_genesis
CORE_CLONEDEFNV(genesgfp,genesis, "Genesis (German Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #705X
INITGAME(genesisf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesisf, "prom2f.cpu", CRC(ea7f824f) SHA1(45f619153e0584cffd33e6e09e6f5a97ab9522b2),
                             "prom1f.cpu", CRC(e7ef875b) SHA1(37ac83d9a75ce604c5a4173ce918beb64f75cd3e))
GTS80BSSOUND888(             "drom1.snd",  CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd",  CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd",  CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesisf input_ports_genesis
CORE_CLONEDEFNV(genesisf,genesis, "Genesis (French)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(genesffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesffp, "prom2f.cpu",   CRC(ea7f824f) SHA1(45f619153e0584cffd33e6e09e6f5a97ab9522b2),
                             "prom1f_fp.cpu",CRC(4d94f012) SHA1(8da4793345365330d13873edee9ffded173ed935))
GTS80BSSOUND888(             "drom1.snd",    CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd",    CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd",    CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_genesffp input_ports_genesis
CORE_CLONEDEFNV(genesffp,genesis, "Genesis (French Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Gold Wings (#707)
/-------------------------------------------------------------------*/
INITGAME(goldwing, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(goldwing, "prom2.cpu", CRC(a5318c20) SHA1(8b4dcf45b13657ff753237a2e7d0352fda7755ef),
                             "prom1.cpu", CRC(bf242185) SHA1(0bf231050aa29f8bba5cb478a815b3d83bad93b3))
GTS80BSSOUND888(             "drom1.snd", CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd", CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd", CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_goldwing input_ports_gts80
CORE_GAMEDEFNV(goldwing, "Gold Wings",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(goldwgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(goldwgfp, "prom2.cpu",    CRC(a5318c20) SHA1(8b4dcf45b13657ff753237a2e7d0352fda7755ef),
                             "prom1_fp.cpu", CRC(58f19602) SHA1(b17c7aadcb314e6639446ed08de7666f5ea3dd66))
GTS80BSSOUND888(             "drom1.snd",    CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd",    CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd",    CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_goldwgfp input_ports_goldwing
CORE_CLONEDEFNV(goldwgfp,goldwing, "Gold Wings (Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #707Y
INITGAME(gldwingg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(gldwingg, "prom2g.cpu", CRC(f69c963c) SHA1(9e39344ecfcca1115e12c559c66eaa21716c0ce2),
                             "prom1g.cpu", CRC(a9349b2f) SHA1(836c86d8db8be5ac29013bbe4daec8d96d15fba0))
GTS80BSSOUND888(             "drom1.snd",  CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd",  CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd",  CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_gldwingg input_ports_goldwing
CORE_CLONEDEFNV(gldwingg,goldwing, "Gold Wings (German)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(gldwggfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(gldwggfp, "prom2g.cpu",   CRC(f69c963c) SHA1(9e39344ecfcca1115e12c559c66eaa21716c0ce2),
                             "prom1g_fp.cpu",CRC(912c5086) SHA1(ecd8d42ebc0840098b9ee3a6b9fe8fde4cb1467f))
GTS80BSSOUND888(             "drom1.snd",    CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd",    CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd",    CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_gldwggfp input_ports_goldwing
CORE_CLONEDEFNV(gldwggfp,goldwing, "Gold Wings (German Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

// #707X
INITGAME(gldwingf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(gldwingf, "prom2f.cpu", CRC(50337adf) SHA1(dc286d52e6872edd68af442cbd0442babc174b93),
                             "prom1f.cpu", CRC(ec046fc0) SHA1(856f09f420e0f37488b0a896a37fffad62f18d6d))
GTS80BSSOUND888(             "drom1.snd",  CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd",  CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd",  CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_gldwingf input_ports_goldwing
CORE_CLONEDEFNV(gldwingf,goldwing, "Gold Wings (French)",1986,"Gottlieb",gl_mGTS80BS1,0)

INITGAME(gldwgffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(gldwgffp, "prom2f.cpu",   CRC(50337adf) SHA1(dc286d52e6872edd68af442cbd0442babc174b93),
                             "prom1f_fp.cpu",CRC(90dd07b7) SHA1(0058812c0ba94e4bb62579e84bc3f61918d2e6ab))
GTS80BSSOUND888(             "drom1.snd",    CRC(892dbb21) SHA1(e24611544693e95dd2b9c0f2532c4d1f0b8ac10c),
                             "yrom1.snd",    CRC(e17e9b1f) SHA1(ada9a6139a13ef31173801d560ec732d5a285140),
                             "yrom2.snd",    CRC(4e482023) SHA1(62e97d229eb28ff67f0ebc4ee04c1b4918a4affe))
GTS80_ROMEND
#define input_ports_gldwgffp input_ports_goldwing
CORE_CLONEDEFNV(gldwgffp,goldwing, "Gold Wings (French Free Play)",1986,"Flipprojets",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Monte Carlo (#708)
/-------------------------------------------------------------------*/
INITGAME(mntecrlo, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(mntecrlo, "prom2.cpu", CRC(6860e315) SHA1(cecb1815334506dfebf29efe3e4e2a838010e8db),
                             "prom1.cpu", CRC(0fbf15a3) SHA1(0155b39c2c38224301857313ab784c1d39f1183b))
GTS80BSSOUND888(             "drom1.snd", CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd", CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd", CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define input_ports_mntecrlo input_ports_gts80
CORE_GAMEDEFNV(mntecrlo, "Monte Carlo",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntecrfp, "prom2.cpu",    CRC(6860e315) SHA1(cecb1815334506dfebf29efe3e4e2a838010e8db),
                             "prom1_fp.cpu", CRC(5b703cf8) SHA1(dfb3eb886675989a1bbae7a2581e522869d81392))
GTS80BSSOUND888(             "drom1.snd",    CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",    CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",    CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntecrfp init_mntecrlo
#define input_ports_mntecrfp input_ports_mntecrlo
CORE_CLONEDEFNV(mntecrfp,mntecrlo, "Monte Carlo (Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #708Y
GTS80B_2K_ROMSTART(mntecrlg, "prom2g.cpu", CRC(2a5e0c4f) SHA1(b386168bd911b9977104c47da962d0248f22614b),
                             "prom1g.cpu", CRC(25e015f1) SHA1(4b1467438def657eac3b8a858d7b17c102e14f0d))
GTS80BSSOUND888(             "drom1.snd",  CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",  CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",  CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntecrlg init_mntecrlo
#define input_ports_mntecrlg input_ports_mntecrlo
CORE_CLONEDEFNV(mntecrlg,mntecrlo, "Monte Carlo (German)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntcrgfp, "prom2g.cpu",   CRC(2a5e0c4f) SHA1(b386168bd911b9977104c47da962d0248f22614b),
                             "prom1g_fp.cpu",CRC(bcf93933) SHA1(846c7d7c1da7516dbe0d19b4fc87eecfb69b13c1))
GTS80BSSOUND888(             "drom1.snd",    CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",    CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",    CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntcrgfp init_mntecrlo
#define input_ports_mntcrgfp input_ports_mntecrlo
CORE_CLONEDEFNV(mntcrgfp,mntecrlo, "Monte Carlo (German Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #708X
GTS80B_2K_ROMSTART(mntecrlf, "prom2f.cpu", CRC(f6842631) SHA1(7447994d2055c7fa12aaf35e93436ee829f5b7ae),
                             "prom1f.cpu", CRC(33a8dbc9) SHA1(5ef586e2b1ba7f245723584bc14c60c2860d19fc))
GTS80BSSOUND888(             "drom1.snd",  CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",  CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",  CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntecrlf init_mntecrlo
#define input_ports_mntecrlf input_ports_mntecrlo
CORE_CLONEDEFNV(mntecrlf,mntecrlo, "Monte Carlo (French)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntcrffp, "prom2f.cpu",   CRC(f6842631) SHA1(7447994d2055c7fa12aaf35e93436ee829f5b7ae),
                             "prom1f_fp.cpu",CRC(dbe0f749) SHA1(2a1fc7606dbc99ac534901ed91943d6dd49bd4e2))
GTS80BSSOUND888(             "drom1.snd",    CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",    CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",    CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntcrffp init_mntecrlo
#define input_ports_mntcrffp input_ports_mntecrlo
CORE_CLONEDEFNV(mntcrffp,mntecrlo, "Monte Carlo (French Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntecrla, "prom2a.cpu", CRC(5dd75c06) SHA1(911f7e56b7602c9bc9b51dde7719d3e0562f0702),
                             "prom1a.cpu", CRC(de980755) SHA1(0df99526a432e26fb73288b529dc0f4f49623e81))
GTS80BSSOUND888(             "drom1.snd",  CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",  CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",  CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntecrla init_mntecrlo
#define input_ports_mntecrla input_ports_mntecrlo
CORE_CLONEDEFNV(mntecrla, mntecrlo, "Monte Carlo (rev. 1)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntcrafp, "prom2a.cpu",    CRC(5dd75c06) SHA1(911f7e56b7602c9bc9b51dde7719d3e0562f0702),
                             "prom1a_fp.cpu", CRC(25787b75) SHA1(f8ad7a22018b5414bf1ea412004ee63cb55c2036))
GTS80BSSOUND888(             "drom1.snd",     CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",     CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",     CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntcrafp init_mntecrlo
#define input_ports_mntcrafp input_ports_mntecrlo
CORE_CLONEDEFNV(mntcrafp, mntecrlo, "Monte Carlo (rev. 1 Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntecrl2, "prom2_2.cpu", CRC(8e72a68f) SHA1(8320c44020f7d5f9e887b17556252f1c617235ac),
                             "prom1_2.cpu", CRC(9bd6a010) SHA1(680ce076452ab3fd911fa58fc48c07ea2ec793da))
GTS80BSSOUND888(             "drom1.snd",   CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",   CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",   CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntecrl2 init_mntecrlo
#define input_ports_mntecrl2 input_ports_mntecrlo
CORE_CLONEDEFNV(mntecrl2, mntecrlo, "Monte Carlo (rev. 2)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(mntcr2fp, "prom2_2.cpu",    CRC(8e72a68f) SHA1(8320c44020f7d5f9e887b17556252f1c617235ac),
                             "prom1_2_fp.cpu", CRC(d47c24ae) SHA1(034ae4515e3ab94b054e85aef96f4376b43a1157))
GTS80BSSOUND888(             "drom1.snd",      CRC(1a53ac15) SHA1(f2751664a09431e908873580ddf4f44df9b4eda7),
                             "yrom1.snd",      CRC(6e234c49) SHA1(fdb4126ecdaac378d144e9dd3c29b4e79290da2a),
                             "yrom2.snd",      CRC(a95d1a6b) SHA1(91946ef7af0e4dd96db6d2d6f4f2e9a3a7279b81))
GTS80_ROMEND
#define init_mntcr2fp init_mntecrlo
#define input_ports_mntcr2fp input_ports_mntecrlo
CORE_CLONEDEFNV(mntcr2fp, mntecrlo, "Monte Carlo (rev. 2 Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Spring Break (#706)
/-------------------------------------------------------------------*/
INITGAME(sprbreak, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(sprbreak, "prom2.cpu", CRC(47171062) SHA1(0d2e7777f695ab22170be861019c05ddeade5f85),
                             "prom1.cpu", CRC(53ed608b) SHA1(555a6c02d637ea03e8265bb2b0fba95f2e2584b3))
GTS80BSSOUND888(             "drom1.snd", CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd", CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd", CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define input_ports_sprbreak input_ports_gts80
CORE_GAMEDEFNV(sprbreak, "Spring Break",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrkfp, "prom2.cpu",    CRC(47171062) SHA1(0d2e7777f695ab22170be861019c05ddeade5f85),
                             "prom1_fp.cpu", CRC(2424a214) SHA1(d4d48082652e99731833fbb57a1c04fea6b564b0))
GTS80BSSOUND888(             "drom1.snd",    CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",    CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",    CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrkfp init_sprbreak
#define input_ports_sprbrkfp input_ports_sprbreak
CORE_CLONEDEFNV(sprbrkfp, sprbreak, "Spring Break (Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #706Y
GTS80B_2K_ROMSTART(sprbrkg, "prom2g.cpu", CRC(fa4b750d) SHA1(89f797f65fc18473419080810bca4590f77e2502),
                            "prom1g.cpu", CRC(2d9c4640) SHA1(3671a962334f5c84ae2635891ee90c62be69da5c))
GTS80BSSOUND888(            "drom1.snd",  CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                            "yrom1.snd",  CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                            "yrom2.snd",  CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrkg init_sprbreak
#define input_ports_sprbrkg input_ports_sprbreak
CORE_CLONEDEFNV(sprbrkg, sprbreak, "Spring Break (German)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrgfp, "prom2g.cpu",   CRC(fa4b750d) SHA1(89f797f65fc18473419080810bca4590f77e2502),
                             "prom1g_fp.cpu",CRC(11ae0ad4) SHA1(b187c31a0fc2aa7f53655820be26e370e379004c))
GTS80BSSOUND888(             "drom1.snd",    CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",    CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",    CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrgfp init_sprbreak
#define input_ports_sprbrgfp input_ports_sprbreak
CORE_CLONEDEFNV(sprbrgfp, sprbreak, "Spring Break (German Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #706X
GTS80B_2K_ROMSTART(sprbrkf, "prom2f.cpu", CRC(c0ee0555) SHA1(3d2aef5a8a6452f9f87b4ec2040643dda5843ebd),
                            "prom1f.cpu", CRC(608cf4d5) SHA1(41193eb036da7c7d05f313d1a68723504a7a90f4))
GTS80BSSOUND888(            "drom1.snd",  CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                            "yrom1.snd",  CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                            "yrom2.snd",  CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrkf init_sprbreak
#define input_ports_sprbrkf input_ports_sprbreak
CORE_CLONEDEFNV(sprbrkf, sprbreak, "Spring Break (French)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrffp, "prom2f.cpu",   CRC(c0ee0555) SHA1(3d2aef5a8a6452f9f87b4ec2040643dda5843ebd),
                             "prom1f_fp.cpu",CRC(8866bddf) SHA1(e8e54dbd5887241d96f21cb878024436e35f4e40))
GTS80BSSOUND888(             "drom1.snd",    CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",    CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",    CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrffp init_sprbreak
#define input_ports_sprbrffp input_ports_sprbreak
CORE_CLONEDEFNV(sprbrffp, sprbreak, "Spring Break (French Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrka,  "prom2a.cpu", CRC(d9d841b4) SHA1(8b9773e5ae9917d27089deca3b8311cb74e7f88e),
                             "prom1a.cpu", CRC(93db71e9) SHA1(59f75c4ef2c36b4f1f94dd365f2df82e7bcf53f8))
GTS80BSSOUND888(             "drom1.snd",  CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",  CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",  CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrka init_sprbreak
#define input_ports_sprbrka input_ports_sprbreak
CORE_CLONEDEFNV(sprbrka, sprbreak, "Spring Break (rev. 1)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrafp, "prom2a.cpu", CRC(d9d841b4) SHA1(8b9773e5ae9917d27089deca3b8311cb74e7f88e),
                             "prom1a_fp.cpu", CRC(3638cb30) SHA1(6c19ca94255a3dbceb8dd33b2e56287836b1ecba))
GTS80BSSOUND888(             "drom1.snd",  CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",  CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",  CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrafp init_sprbreak
#define input_ports_sprbrafp input_ports_sprbreak
CORE_CLONEDEFNV(sprbrafp, sprbreak, "Spring Break (rev. 1 Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrks,  "prom2.rv2", CRC(911cd14f) SHA1(2bc3ff6a3889da69b97f8ec318f93208e3d42cfe),
                             "prom1.rv2", CRC(d67d9d2f) SHA1(ebb82f0a1b7d6a2ec2607d4000e58fb6bfa73fe7))
GTS80BSSOUND888(             "drom1.snd", CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd", CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd", CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrks init_sprbreak
#define input_ports_sprbrks input_ports_sprbreak
CORE_CLONEDEFNV(sprbrks, sprbreak, "Spring Break (single ball game)",1987,"Gottlieb",gl_mGTS80BS1,0)

GTS80B_2K_ROMSTART(sprbrsfp, "prom2.rv2",    CRC(911cd14f) SHA1(2bc3ff6a3889da69b97f8ec318f93208e3d42cfe),
                             "prom1_fp.rv2", CRC(dc956db7) SHA1(8bdb357c0a4c78967b4bb053f9d807897a28ad88))
GTS80BSSOUND888(             "drom1.snd",    CRC(97d3f9ba) SHA1(1b34c7e51373c26d29d757c57a2b0333fe38d19e),
                             "yrom1.snd",    CRC(5ea89df9) SHA1(98ce7661a4d862fd02c77e69b0f6e9372c3ade2b),
                             "yrom2.snd",    CRC(0fb0128e) SHA1(3bdc5ed11b8e062f71f2a78b955830bd985e80a3))
GTS80_ROMEND
#define init_sprbrsfp init_sprbreak
#define input_ports_sprbrsfp input_ports_sprbreak
CORE_CLONEDEFNV(sprbrsfp, sprbreak, "Spring Break (single ball game, Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Amazon Hunt II (#684C)
/-------------------------------------------------------------------*/
INITGAME(amazonh2, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(amazonh2, "684c-cpu.rom", CRC(0b5040c3) SHA1(104e5a63b4097ea72a5b31df1a7d5198342be5c4))
GTS80BSSOUND8(               "684c-snd.rom", CRC(182d64e1) SHA1(c0aaa646a3d53cf00aa23e0b8d46bbb70ce46e5c))
GTS80_ROMEND
#define input_ports_amazonh2 input_ports_gts80
CORE_GAMEDEFNV(amazonh2, "Amazon Hunt II (French)", 1987, "Gottlieb", gl_mGTS80BS1, 0)

INITGAME(amazn2fp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(amazn2fp, "684c-cpu_fp.rom", CRC(fe08af7d) SHA1(54ea8205f649c9ab8a62354b023acf823c24fc1f))
GTS80BSSOUND8(               "684c-snd.rom",    CRC(182d64e1) SHA1(c0aaa646a3d53cf00aa23e0b8d46bbb70ce46e5c))
GTS80_ROMEND
#define input_ports_amazn2fp input_ports_amazonh2
CORE_CLONEDEFNV(amazn2fp,amazonh2, "Amazon Hunt II (French Free Play)", 1987, "Flipprojets", gl_mGTS80BS1, 0)

//Amazon Hunt III (#684D, at the end of the file)

/*-------------------------------------------------------------------
/ Arena (#709)
/-------------------------------------------------------------------*/
INITGAME(arena, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(arena, "prom2.cpu", CRC(4783b689) SHA1(d10d4cbf8d00c9d0db57cdac32ef96498275eea6),
                          "prom1.cpu", CRC(8c9f8ee9) SHA1(840505d08e387c3f7de105305e183f8ed3a6d5c6))
GTS80BSSOUND888(          "drom1.snd", CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                          "yrom1.snd", CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                          "yrom2.snd", CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arena input_ports_gts80
CORE_GAMEDEFNV(arena, "Arena",1987,"Gottlieb",gl_mGTS80BS1,0)

#define init_arena_fp init_arena
GTS80B_2K_ROMSTART(arena_fp, "prom2.cpu",    CRC(4783b689) SHA1(d10d4cbf8d00c9d0db57cdac32ef96498275eea6),
                             "prom1_fp.cpu", CRC(33ade406) SHA1(daea04b9ccd95b2e4ee3d45ad8ea6e1851cf8cc6))
GTS80BSSOUND888(             "drom1.snd",    CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                             "yrom1.snd",    CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                             "yrom2.snd",    CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arena_fp input_ports_arena
CORE_CLONEDEFNV(arena_fp,arena, "Arena (Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #709Y
#define init_arenag init_arena
GTS80B_2K_ROMSTART(arenag, "prom2g.cpu", CRC(e170d1cd) SHA1(bd7919eb9e480309f794ac25a371c7b818dcd01b),
                           "prom1g.cpu", CRC(71fd6e48) SHA1(5c87ba79968085d386fd1357c9d8b2b7a745682a))
GTS80BSSOUND888(           "drom1.snd",  CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                           "yrom1.snd",  CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                           "yrom2.snd",  CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenag input_ports_arena
CORE_CLONEDEFNV(arenag,arena, "Arena (German)",1987,"Gottlieb",gl_mGTS80BS1,0)

#define init_arenagfp init_arena
GTS80B_2K_ROMSTART(arenagfp, "prom2g.cpu",   CRC(e170d1cd) SHA1(bd7919eb9e480309f794ac25a371c7b818dcd01b),
                             "prom1g_fp.cpu",CRC(d41ce2b1) SHA1(044fd0bcabb317d0fa84ff17036f6dba90201cbf))
GTS80BSSOUND888(             "drom1.snd",    CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                             "yrom1.snd",    CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                             "yrom2.snd",    CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenagfp input_ports_arena
CORE_CLONEDEFNV(arenagfp,arena, "Arena (German Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

// #709X
#define init_arenaf init_arena
GTS80B_2K_ROMSTART(arenaf, "prom2f.cpu", CRC(49b127d8) SHA1(0436f83e969b4bfc7edaf881bf7556a868c88cdc),
                           "prom1f.cpu", CRC(391fb7de) SHA1(ec47a6e057d18a0043afccb694c23d0fa0d42aa0))
GTS80BSSOUND888(           "drom1.snd",  CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                           "yrom1.snd",  CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                           "yrom2.snd",  CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenaf input_ports_arena
CORE_CLONEDEFNV(arenaf,arena, "Arena (French)",1987,"Gottlieb",gl_mGTS80BS1,0)

#define init_arenaffp init_arena
GTS80B_2K_ROMSTART(arenaffp, "prom2f.cpu",   CRC(49b127d8) SHA1(0436f83e969b4bfc7edaf881bf7556a868c88cdc),
                             "prom1f_fp.cpu",CRC(7389f4dc) SHA1(3dc72f011ebb2debbc005761da4b9720a279db2a))
GTS80BSSOUND888(             "drom1.snd",    CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                             "yrom1.snd",    CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                             "yrom2.snd",    CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenaffp input_ports_arena
CORE_CLONEDEFNV(arenaffp,arena, "Arena (French Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

#define init_arenaa init_arena
GTS80B_2K_ROMSTART(arenaa,"prom2a.cpu", CRC(13c8813b) SHA1(756e3583fd55b72e0bfb15e9b4a60740b389ca2e),
                          "prom1a.cpu", CRC(253eceb1) SHA1(b46ccec4b3e8fc57fb3295b675b4f27dafc0322e))
GTS80BSSOUND888(          "drom1.snd",  CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                          "yrom1.snd",  CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                          "yrom2.snd",  CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenaa input_ports_arena
CORE_CLONEDEFNV(arenaa, arena, "Arena (rev. 1)",1987,"Gottlieb",gl_mGTS80BS1,0)

#define init_arenaafp init_arena
GTS80B_2K_ROMSTART(arenaafp,"prom2a.cpu",    CRC(13c8813b) SHA1(756e3583fd55b72e0bfb15e9b4a60740b389ca2e),
                            "prom1a_fp.cpu", CRC(21f500cb) SHA1(ce76a8a5eb71e57aca70880a2e40979f083173ef))
GTS80BSSOUND888(            "drom1.snd",     CRC(78e6cbf1) SHA1(7b66a0cb211a93cf475172aa0465a952009e1a59),
                            "yrom1.snd",     CRC(f7a951c2) SHA1(12d7a6119d9033ae02c6312c9af888bfc7c63ad1),
                            "yrom2.snd",     CRC(cc2aef4e) SHA1(a6e243de99f6a76eb527e879f4441c036dd379b6))
GTS80_ROMEND
#define input_ports_arenaafp input_ports_arena
CORE_CLONEDEFNV(arenaafp, arena, "Arena (rev. 1 Free Play)",1987,"Flipprojets",gl_mGTS80BS1,0)

/****************************************/
/* Start of Generation 2 Sound Hardware */
/****************************************/

/*-------------------------------------------------------------------
/ Victory (#710)
/-------------------------------------------------------------------*/
INITGAME(victory, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victory, "prom2.cpu", CRC(6a42eaf4) SHA1(3e28b01473266db463986a4283e1be85f2410fb1),
                            "prom1.cpu", CRC(e724db90) SHA1(10e760e129ce89f11372c6dd3616216d45f2c926))
GTS80BSSOUND3232(           "drom1.snd", CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd", CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victory input_ports_gts80
CORE_GAMEDEFNV(victory, "Victory",1987,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(victryfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victryfp,"prom2.cpu",    CRC(6a42eaf4) SHA1(3e28b01473266db463986a4283e1be85f2410fb1),
                            "prom1_fp.cpu", CRC(3eba52ec) SHA1(e3cbdd803373e614c5b9bb5e61c9e2dfcf25df6c))
GTS80BSSOUND3232(           "drom1.snd",    CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd",    CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victryfp input_ports_victory
CORE_CLONEDEFNV(victryfp,victory, "Victory (Free Play)",1987,"Flipprojets",gl_mGTS80BS2,0)

// #710Y
INITGAME(victoryg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victoryg,"prom2g.cpu", CRC(b191a87a) SHA1(f205ffb41c5ba34e3cefc96ca870a5d08bee8854),
                            "prom1g.cpu", CRC(097b9062) SHA1(e7f05084b36f84b9948702ba297700473386ae6d))
GTS80BSSOUND3232(           "drom1.snd",  CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd",  CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victoryg input_ports_victory
CORE_CLONEDEFNV(victoryg,victory, "Victory (German)",1987,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(victrgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victrgfp,"prom2g.cpu",   CRC(b191a87a) SHA1(f205ffb41c5ba34e3cefc96ca870a5d08bee8854),
                            "prom1g_fp.cpu",CRC(6daebe71) SHA1(bc49c074210f3f3cc5314282e32cebb7ce67a81d))
GTS80BSSOUND3232(           "drom1.snd",    CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd",    CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victrgfp input_ports_victory
CORE_CLONEDEFNV(victrgfp,victory, "Victory (German Free Play)",1987,"Flipprojets",gl_mGTS80BS2,0)

// #710X
INITGAME(victoryf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victoryf,"prom2f.cpu", CRC(dffcfa77) SHA1(3efaca85295ca55268b8d7c7cfe8f09f159d5fbd),
                            "prom1f.cpu", CRC(d3a9df20) SHA1(7e0a97a4c1b488af89959cbaa693e23302479d0a))
GTS80BSSOUND3232(           "drom1.snd",  CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd",  CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victoryf input_ports_victory
CORE_CLONEDEFNV(victoryf,victory, "Victory (French)",1987,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(victrffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victrffp,"prom2f.cpu",   CRC(dffcfa77) SHA1(3efaca85295ca55268b8d7c7cfe8f09f159d5fbd),
                            "prom1f_fp.cpu",CRC(a626da77) SHA1(300674ffb48deed503aae62a3b53b9941058605b))
GTS80BSSOUND3232(           "drom1.snd",    CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd",    CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victrffp input_ports_victory
CORE_CLONEDEFNV(victrffp,victory, "Victory (French Free Play)",1987,"Flipprojets",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Diamond Lady (#711)
/-------------------------------------------------------------------*/
INITGAME(diamond, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamond, "prom2.cpu", CRC(862951dc) SHA1(b15899ecf7ec869e3722cef3f5c16b0dadd2514e),
                            "prom1.cpu", CRC(7a011757) SHA1(cc49ec7451feae035670ea9d70cc8f6b32747c90))
GTS80BSSOUND3232(           "drom1.snd", CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                            "yrom1.snd", CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamond input_ports_gts80
CORE_GAMEDEFNV(diamond, "Diamond Lady",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(diamonfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamonfp, "prom2.cpu",    CRC(862951dc) SHA1(b15899ecf7ec869e3722cef3f5c16b0dadd2514e),
                             "prom1_fp.cpu", CRC(eef4da86) SHA1(74b274adfddc29fed91d00af52fc4e477b571fe8))
GTS80BSSOUND3232(            "drom1.snd",    CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                             "yrom1.snd",    CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamonfp input_ports_diamond
CORE_CLONEDEFNV(diamonfp,diamond, "Diamond Lady (Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #711Y
INITGAME(diamondg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamondg, "prom2g.cpu", CRC(f0ef69f6) SHA1(1f48bb656bb20073e2ff261199cb94919f0bb2ab),
                             "prom1g.cpu", CRC(961cfdf9) SHA1(97135f77705969736f704acdeda6157bb765c73e))
GTS80BSSOUND3232(            "drom1.snd",  CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                             "yrom1.snd",  CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamondg input_ports_diamond
CORE_CLONEDEFNV(diamondg,diamond, "Diamond Lady (German)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(diamngfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamngfp, "prom2g.cpu",   CRC(f0ef69f6) SHA1(1f48bb656bb20073e2ff261199cb94919f0bb2ab),
                             "prom1g_fp.cpu",CRC(4a070002) SHA1(80f1b2bd36c7133d92a35fb995cf268ff4259e86))
GTS80BSSOUND3232(            "drom1.snd",    CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                             "yrom1.snd",    CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamngfp input_ports_diamond
CORE_CLONEDEFNV(diamngfp,diamond, "Diamond Lady (German Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #711X
INITGAME(diamondf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamondf, "prom2f.cpu", CRC(943019a8) SHA1(558c3696339bb6e150b4ddb499bc60897d5954ec),
                             "prom1f.cpu", CRC(479b0267) SHA1(a9586c5b2cc3561ba3409123eca5a73ebabfd823))
GTS80BSSOUND3232(            "drom1.snd",  CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                             "yrom1.snd",  CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamondf input_ports_diamond
CORE_CLONEDEFNV(diamondf,diamond, "Diamond Lady (French)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(diamnffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamnffp, "prom2f.cpu",   CRC(943019a8) SHA1(558c3696339bb6e150b4ddb499bc60897d5954ec),
                             "prom1f_fp.cpu",CRC(1d7feafd) SHA1(70f02157fcd94ff7b66750054b542642a3a051b2))
GTS80BSSOUND3232(            "drom1.snd",    CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                             "yrom1.snd",    CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamnffp input_ports_diamond
CORE_CLONEDEFNV(diamnffp,diamond, "Diamond Lady (French Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ TX-Sector (#712)
/-------------------------------------------------------------------*/
INITGAME(txsector, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsector, "prom2.cpu", CRC(f12514e6) SHA1(80bca17c33df99ed1a7acc21f7f70ea90e7c0463),
                             "prom1.cpu", CRC(e51d39da) SHA1(b6e4d573b62cc441a153cc4d8b647ee46b4dd2a7))
GTS80BSSOUND3232(            "drom1.snd", CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd", CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsector input_ports_gts80
CORE_GAMEDEFNV(txsector, "TX-Sector",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(txsectfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsectfp, "prom2.cpu",    CRC(f12514e6) SHA1(80bca17c33df99ed1a7acc21f7f70ea90e7c0463),
                             "prom1_fp.cpu", CRC(13283b01) SHA1(e4e0602ead0ec4d4f54a39df3d08b1aaeb92f1ca))
GTS80BSSOUND3232(            "drom1.snd",    CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd",    CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsectfp input_ports_txsector
CORE_CLONEDEFNV(txsectfp,txsector, "TX-Sector (Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #712Y
INITGAME(txsectrg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsectrg, "prom2g.cpu", CRC(2b17261f) SHA1(a3195190c0d5116b60e487a7b7f3a28c1f110e89),
                             "prom1g.cpu", CRC(83ea2f11) SHA1(ac3570597512c71c099aa15f0750a12a3e206b83))
GTS80BSSOUND3232(            "drom1.snd",  CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd",  CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsectrg input_ports_txsector
CORE_CLONEDEFNV(txsectrg,txsector, "TX-Sector (German)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(txsecgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsecgfp, "prom2g.cpu",   CRC(2b17261f) SHA1(a3195190c0d5116b60e487a7b7f3a28c1f110e89),
                             "prom1g_fp.cpu",CRC(0c374395) SHA1(52a7035598ba83aaf149550e7d08190f9773c25a))
GTS80BSSOUND3232(            "drom1.snd",    CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd",    CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsecgfp input_ports_txsector
CORE_CLONEDEFNV(txsecgfp,txsector, "TX-Sector (German Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #712X
INITGAME(txsectrf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsectrf, "prom2f.cpu", CRC(1bd08247) SHA1(968cc30e5e5c783e73cb3278a58189c4f8b8186f),
                             "prom1f.cpu", CRC(8df27155) SHA1(67aeeab0d50e43674082e1dd99a849db64ba00b2))
GTS80BSSOUND3232(            "drom1.snd",  CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd",  CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsectrf input_ports_txsector
CORE_CLONEDEFNV(txsectrf,txsector, "TX-Sector (French)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(txsecffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsecffp, "prom2f.cpu",   CRC(1bd08247) SHA1(968cc30e5e5c783e73cb3278a58189c4f8b8186f),
                             "prom1f_fp.cpu",CRC(45e47931) SHA1(a92d323c4892cd1aa429cd884a8f1d33f0379667))
GTS80BSSOUND3232(            "drom1.snd",    CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd",    CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsecffp input_ports_txsector
CORE_CLONEDEFNV(txsecffp,txsector, "TX-Sector (French Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Robo-War (#714)
/-------------------------------------------------------------------*/
INITGAME(robowars, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowars, "prom2.cpu", CRC(893177ed) SHA1(791540a64d498979e5b0c8baf4ceb2fd5ff7f047),
                             "prom1.cpu", CRC(cd1587d8) SHA1(77e8e02dc03d052e9e4ce19c9431439e4211a29f))
GTS80BSSOUND3232(            "drom1.snd", CRC(ea59b6a1) SHA1(6a4cdd37ba85f94f703afd1c5d3f102f51fedf46),
                             "yrom1.snd", CRC(7ecd8b67) SHA1(c5167b0acc64e535d389ba70be92a65672e119f6))
GTS80_ROMEND
#define input_ports_robowars input_ports_gts80
CORE_GAMEDEFNV(robowars, "Robo-War",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(robowrfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowrfp, "prom2.cpu",    CRC(893177ed) SHA1(791540a64d498979e5b0c8baf4ceb2fd5ff7f047),
                             "prom1_fp.cpu", CRC(96da77eb) SHA1(402208f5ac52d8f1e7193bf7c86faa106afe3492))
GTS80BSSOUND3232(            "drom1.snd",    CRC(ea59b6a1) SHA1(6a4cdd37ba85f94f703afd1c5d3f102f51fedf46),
                             "yrom1.snd",    CRC(7ecd8b67) SHA1(c5167b0acc64e535d389ba70be92a65672e119f6))
GTS80_ROMEND
#define input_ports_robowrfp input_ports_robowars
CORE_CLONEDEFNV(robowrfp,robowars, "Robo-War (Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #714X
INITGAME(robowarf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowarf, "prom2f.cpu", CRC(1afa0e69) SHA1(178813494b877ac9ca36863661596b4df04df1bb),
                             "prom1f.cpu", CRC(263cb8f9) SHA1(ba27ca0618b9ed68c258a654bdd00a24f8413239))
GTS80BSSOUND3232(            "drom1.snd",  CRC(ea59b6a1) SHA1(6a4cdd37ba85f94f703afd1c5d3f102f51fedf46),
                             "yrom1.snd",  CRC(7ecd8b67) SHA1(c5167b0acc64e535d389ba70be92a65672e119f6))
GTS80_ROMEND
#define input_ports_robowarf input_ports_robowars
CORE_CLONEDEFNV(robowarf,robowars, "Robo-War (French)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(robowffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowffp, "prom2f.cpu",   CRC(1afa0e69) SHA1(178813494b877ac9ca36863661596b4df04df1bb),
                             "prom1f_fp.cpu",CRC(51cd5108) SHA1(9ba30ba0eaaabc8e60c79dc2322aeec51e4de09a))
GTS80BSSOUND3232(            "drom1.snd",    CRC(ea59b6a1) SHA1(6a4cdd37ba85f94f703afd1c5d3f102f51fedf46),
                             "yrom1.snd",    CRC(7ecd8b67) SHA1(c5167b0acc64e535d389ba70be92a65672e119f6))
GTS80_ROMEND
#define input_ports_robowffp input_ports_robowars
CORE_CLONEDEFNV(robowffp,robowars, "Robo-War (French Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Excalibur (#715)
/-------------------------------------------------------------------*/
INITGAME(excaliba, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excaliba, "prom2.cpu", CRC(082d64ab) SHA1(0eae3b549839fc281d2487d483d0b4e723ebdc48),
                             "prom1.cpu", CRC(e8902c16) SHA1(c3e4ece6be7027a4deef052ba4be752070e9b542))
GTS80BSSOUND3232(            "drom1.snd", CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd", CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excaliba input_ports_gts80
CORE_GAMEDEFNV(excaliba, "Excalibur",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(excalbfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalbfp, "prom2.cpu",   CRC(082d64ab) SHA1(0eae3b549839fc281d2487d483d0b4e723ebdc48),
                             "prom1_fp.cpu",CRC(86cf464b) SHA1(c857187e6f3dd1f5b5013c95d3ded8a9a5a2e485))
GTS80BSSOUND3232(            "drom1.snd",   CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd",   CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalbfp input_ports_excaliba
CORE_CLONEDEFNV(excalbfp,excaliba, "Excalibur (Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #715Y
INITGAME(excalibg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalibg, "prom2g.cpu", CRC(49079396) SHA1(92361a87464e39afeb74fe531b7d4356323405b8),
                             "prom1g.cpu", CRC(504fad7a) SHA1(6648778d537161e9bdcf2955209e1525e90a3617))
GTS80BSSOUND3232(            "drom1.snd",  CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd",  CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalibg input_ports_excaliba
CORE_CLONEDEFNV(excalibg,excaliba, "Excalibur (German)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(excalgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalgfp, "prom2g.cpu",   CRC(49079396) SHA1(92361a87464e39afeb74fe531b7d4356323405b8),
                             "prom1g_fp.cpu",CRC(76d20188) SHA1(fd822702a6ad880b88f88886b752d7a1087095fd))
GTS80BSSOUND3232(            "drom1.snd",    CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd",    CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalgfp input_ports_excaliba
CORE_CLONEDEFNV(excalgfp,excaliba, "Excalibur (German Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

// #715X
INITGAME(excalibr, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalibr, "prom2f.cpu", CRC(499e2e41) SHA1(1e3fcba18882bd7df30a43843916aa5d7968eecc),
                             "prom1f.cpu", CRC(ed1083d7) SHA1(3ff829ecfaba7d20c75268d3ee5224cb3cac3507))
GTS80BSSOUND3232(            "drom1.snd",  CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd",  CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalibr input_ports_excaliba
CORE_CLONEDEFNV(excalibr,excaliba, "Excalibur (French)",1988,"Gottlieb",gl_mGTS80BS2,0)

INITGAME(excalffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalffp, "prom2f.cpu",   CRC(499e2e41) SHA1(1e3fcba18882bd7df30a43843916aa5d7968eecc),
                             "prom1f_fp.cpu",CRC(65601620) SHA1(d8a0f13618f5af4954e0079890ad1ce6ae490d57))
GTS80BSSOUND3232(            "drom1.snd",    CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd",    CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalffp input_ports_excaliba
CORE_CLONEDEFNV(excalffp,excaliba, "Excalibur (French Free Play)",1988,"Flipprojets",gl_mGTS80BS2,0)

/****************************************/
/* Start of Generation 3 Sound Hardware */
/****************************************/

/*-------------------------------------------------------------------
/ Bad Girls (#717)
/-------------------------------------------------------------------*/
INITGAME(badgirls, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgirls, "prom2.cpu", CRC(583933ec) SHA1(89da6750d779d68db578715b058f9321695b79b0),
                             "prom1.cpu", CRC(956aeae0) SHA1(24d9d514fc83aba1ab310bfe4ed80605df399417))
GTS80BSSOUND3232(            "drom1.snd", CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd", CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgirls input_ports_gts80
CORE_GAMEDEFNV(badgirls, "Bad Girls",1988,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(badgrlfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgrlfp, "prom2.cpu",    CRC(583933ec) SHA1(89da6750d779d68db578715b058f9321695b79b0),
                             "prom1_fp.cpu", CRC(05e8259b) SHA1(d1e4e50e44e215dcfa510e4d45d6c39e136452b1))
GTS80BSSOUND3232(            "drom1.snd",    CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd",    CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgrlfp input_ports_badgirls
CORE_CLONEDEFNV(badgrlfp,badgirls, "Bad Girls (Free Play)",1988,"Flipprojets",gl_mGTS80BS3,0)

// #717Y
INITGAME(badgirlg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgirlg, "prom2g.cpu", CRC(55aa30ac) SHA1(9544485ccf52a2ad51a00cce0c12871db099699f),
                             "prom1g.cpu", CRC(f2923255) SHA1(645b62d015e3a4feaf485c600eb345824f551b9e))
GTS80BSSOUND3232(            "drom1.snd",  CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd",  CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgirlg input_ports_badgirls
CORE_CLONEDEFNV(badgirlg,badgirls, "Bad Girls (German)",1988,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(badgrgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgrgfp, "prom2g.cpu",   CRC(55aa30ac) SHA1(9544485ccf52a2ad51a00cce0c12871db099699f),
                             "prom1g_fp.cpu",CRC(34a93b4b) SHA1(14522c5c1c476d5507100d3554db6c2236d48df3))
GTS80BSSOUND3232(            "drom1.snd",    CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd",    CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgrgfp input_ports_badgirls
CORE_CLONEDEFNV(badgrgfp,badgirls, "Bad Girls (German Free Play)",1988,"Flipprojets",gl_mGTS80BS3,0)

// #717X
INITGAME(badgirlf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgirlf, "prom2f.cpu", CRC(58c35099) SHA1(ff76bd28175ea0f5d0437c16c5ae6886339edfe2),
                             "prom1f.cpu", CRC(9861147a) SHA1(e9d31cd1130bc1785db26c23f52944842fdd4ca0))
GTS80BSSOUND3232(            "drom1.snd",  CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd",  CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgirlf input_ports_badgirls
CORE_CLONEDEFNV(badgirlf,badgirls, "Bad Girls (French)",1988,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(badgrffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgrffp, "prom2f.cpu",   CRC(58c35099) SHA1(ff76bd28175ea0f5d0437c16c5ae6886339edfe2),
                             "prom1f_fp.cpu",CRC(32f42091) SHA1(0709f251b5633a68a93066721d105141fb79d74a))
GTS80BSSOUND3232(            "drom1.snd",    CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd",    CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgrffp input_ports_badgirls
CORE_CLONEDEFNV(badgrffp,badgirls, "Bad Girls (French Free Play)",1988,"Flipprojets",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Big House (#713)
/-------------------------------------------------------------------*/
INITGAME(bighouse, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighouse, "prom2.cpu", CRC(047c8ef5) SHA1(3afa2a0011b724836b69b2ef386597e0953dfadf),
                             "prom1.cpu", CRC(0ecef900) SHA1(78e4ed6e40fdb45dde2d0f2cf60d4c8a7ea2e39e))
GTS80BSSOUND3232(            "drom1.snd", CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd", CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighouse input_ports_gts80
CORE_GAMEDEFNV(bighouse, "Big House",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(bighosfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighosfp, "prom2.cpu",    CRC(047c8ef5) SHA1(3afa2a0011b724836b69b2ef386597e0953dfadf),
                             "prom1_fp.cpu", CRC(8a8510a2) SHA1(729a7254d00fee7c4aa29684e944df4eab113565))
GTS80BSSOUND3232(            "drom1.snd",    CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd",    CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighosfp input_ports_bighouse
CORE_CLONEDEFNV(bighosfp,bighouse, "Big House (Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

// #713Y
INITGAME(bighousg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighousg, "prom2g.cpu", CRC(214f0afb) SHA1(9874773e4ffa2472e78d42dfa9e21a621bf7b49e),
                             "prom1g.cpu", CRC(374f3593) SHA1(e90d867fff28ee86f017b1b638bc26f1bcde6b81))
GTS80BSSOUND3232(            "drom1.snd",  CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd",  CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighousg input_ports_bighouse
CORE_CLONEDEFNV(bighousg,bighouse, "Big House (German)",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(bighsgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighsgfp, "prom2g.cpu",   CRC(214f0afb) SHA1(9874773e4ffa2472e78d42dfa9e21a621bf7b49e),
                             "prom1g_fp.cpu",CRC(8f51d4c3) SHA1(972582aedfdbccd7a14d841b4ec156ab73e8c88f))
GTS80BSSOUND3232(            "drom1.snd",    CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd",    CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighsgfp input_ports_bighouse
CORE_CLONEDEFNV(bighsgfp,bighouse, "Big House (German Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

// #713X
INITGAME(bighousf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighousf, "prom2f.cpu", CRC(767efc44) SHA1(6b8f9a580e6a6ad92c9efe9f4345496d5063b7a8),
                             "prom1f.cpu", CRC(b87150bc) SHA1(2ebdf27ede3445ac99068c8cec712c06e57c7ffc))
GTS80BSSOUND3232(            "drom1.snd",  CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd",  CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighousf input_ports_bighouse
CORE_CLONEDEFNV(bighousf,bighouse, "Big House (French)",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(bighsffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighsffp, "prom2f.cpu",   CRC(767efc44) SHA1(6b8f9a580e6a6ad92c9efe9f4345496d5063b7a8),
                             "prom1f_fp.cpu",CRC(51012001) SHA1(5ad45694273234b2d13028b90c2a58245394095e))
GTS80BSSOUND3232(            "drom1.snd",    CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd",    CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighsffp input_ports_bighouse
CORE_CLONEDEFNV(bighsffp,bighouse, "Big House (French Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Hot Shots (#718)
/-------------------------------------------------------------------*/
INITGAME(hotshots, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshots, "prom2.cpu", CRC(7695c7db) SHA1(90188ff83b888262ba849e5af9d99145c5bc1c30),
                             "prom1.cpu", CRC(122ff4a8) SHA1(195392b9f2050b52392a123831bb7a9428087c1b))
GTS80BSSOUND3232(            "drom1.snd", CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd", CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshots input_ports_gts80
CORE_GAMEDEFNV(hotshots, "Hot Shots",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(hotshtfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshtfp, "prom2.cpu",    CRC(7695c7db) SHA1(90188ff83b888262ba849e5af9d99145c5bc1c30),
                             "prom1_fp.cpu", CRC(74c0f7d7) SHA1(7af87d03fb604e7192ff4ee7581c034e8b2556da))
GTS80BSSOUND3232(            "drom1.snd",    CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd",    CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshtfp input_ports_hotshots
CORE_CLONEDEFNV(hotshtfp,hotshots, "Hot Shots (Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

// #718Y
INITGAME(hotshotg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshotg, "prom2g.cpu", CRC(7e2f0d59) SHA1(b8a7b9be3e4d705631e017da87b27be53ed23f30),
                             "prom1g.cpu", CRC(e07b46ad) SHA1(c7b48dcfb074f3d0f38a6d49028ba172946467fc))
GTS80BSSOUND3232(            "drom1.snd",  CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd",  CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshotg input_ports_hotshots
CORE_CLONEDEFNV(hotshotg,hotshots, "Hot Shots (German)",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(hotshgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshgfp, "prom2g.cpu",   CRC(7e2f0d59) SHA1(b8a7b9be3e4d705631e017da87b27be53ed23f30),
                             "prom1g_fp.cpu",CRC(dca0b300) SHA1(d7473ea1398dff2bd861d4b49b0cee2764599b34))
GTS80BSSOUND3232(            "drom1.snd",    CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd",    CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshgfp input_ports_hotshots
CORE_CLONEDEFNV(hotshgfp,hotshots, "Hot Shots (German Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

// #718X
INITGAME(hotshotf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshotf, "prom2f.cpu", CRC(476e260c) SHA1(2b88920c77462d190f9b98aebf8fcb5c9e853ecd),
                             "prom1f.cpu", CRC(8d74aca7) SHA1(c25b015ad8a6fa142c7cb46e2ac0229eb00289cf))
GTS80BSSOUND3232(            "drom1.snd",  CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd",  CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshotf input_ports_hotshots
CORE_CLONEDEFNV(hotshotf,hotshots, "Hot Shots (French)",1989,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(hotshffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshffp, "prom2f.cpu",   CRC(476e260c) SHA1(2b88920c77462d190f9b98aebf8fcb5c9e853ecd),
                             "prom1f_fp.cpu",CRC(dedba56b) SHA1(b9b435173f1325e57532c7001777dec862213d97))
GTS80BSSOUND3232(            "drom1.snd",    CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd",    CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshffp input_ports_hotshots
CORE_CLONEDEFNV(hotshffp,hotshots, "Hot Shots (French Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Bone Busters Inc. (#719)
/-------------------------------------------------------------------*/
INITGAME(bonebstr, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebstr, "prom2.cpu", CRC(681643df) SHA1(76af6951e4403b4951298d35a9058bcebfa6bc43),
                             "prom1.cpu", CRC(052f97be) SHA1(0ee108e79c4196dffedc64d7f7a576e0394427c1))
GTS80BSSOUND3x32(            "drom2.snd", CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd", CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd", CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebstr input_ports_gts80
CORE_GAMEDEFNV(bonebstr, "Bone Busters Inc.",1989,"Gottlieb",gl_mGTS80BS3A,0)

INITGAME(bonebsfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebsfp, "prom2.cpu",    CRC(681643df) SHA1(76af6951e4403b4951298d35a9058bcebfa6bc43),
                             "prom1_fp.cpu", CRC(c5914b1f) SHA1(599a6da358d294304e07425fdde3f1ece0f4f57a))
GTS80BSSOUND3x32(            "drom2.snd",    CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd",    CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd",    CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebsfp input_ports_bonebstr
CORE_CLONEDEFNV(bonebsfp,bonebstr, "Bone Busters Inc. (Free Play)",1989,"Flipprojets",gl_mGTS80BS3A,0)

// #719Y
INITGAME(bonebstg, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebstg, "prom2g.cpu", CRC(3b85c8bd) SHA1(5c99349dc3ae05b82932d6ec9d2d1a29c2a7e36d),
                             "prom1g.cpu", CRC(a0aab93e) SHA1(b7fa3d6eeb1977e4d91644aab1ac03aeee6934d0))
GTS80BSSOUND3x32(            "drom2.snd",  CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd",  CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd",  CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebstg input_ports_bonebstr
CORE_CLONEDEFNV(bonebstg,bonebstr, "Bone Busters Inc. (German)",1989,"Gottlieb",gl_mGTS80BS3A,0)

INITGAME(bonebgfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebgfp, "prom2g.cpu",   CRC(3b85c8bd) SHA1(5c99349dc3ae05b82932d6ec9d2d1a29c2a7e36d),
                             "prom1g_fp.cpu",CRC(0063411b) SHA1(8bf7350acff3ac7d76ed2dee42aceee1de486497))
GTS80BSSOUND3x32(            "drom2.snd",    CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd",    CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd",    CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebgfp input_ports_bonebstr
CORE_CLONEDEFNV(bonebgfp,bonebstr, "Bone Busters Inc. (German Free Play)",1989,"Flipprojets",gl_mGTS80BS3A,0)

// #719X
INITGAME(bonebstf, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebstf, "prom2f.cpu",CRC(73b6486e) SHA1(1baf17f31b16d564ed5e3bdf9f74b21f83ed76fa),
                             "prom1f.cpu",CRC(3d334065) SHA1(6d44819cf84bee375a9f62351b00375404f6d3e3))
GTS80BSSOUND3x32(            "drom2.snd", CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd", CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd", CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebstf input_ports_bonebstr
CORE_CLONEDEFNV(bonebstf,bonebstr, "Bone Busters Inc. (French)",1989,"Gottlieb",gl_mGTS80BS3A,0)

INITGAME(bonebffp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebffp, "prom2f.cpu",   CRC(73b6486e) SHA1(1baf17f31b16d564ed5e3bdf9f74b21f83ed76fa),
                             "prom1f_fp.cpu",CRC(5aa1bb17) SHA1(6499608e83261c1fd152e9bf982ce1470b6edf93))
GTS80BSSOUND3x32(            "drom2.snd",    CRC(d147d78d) SHA1(f8f6d6a1921685b883b224a9ea85ead52a32a4c3),
                             "drom1.snd",    CRC(ec43f4e9) SHA1(77b0988700be7a597dca7e5f06ac5d3c6834ce21),
                             "yrom1.snd",    CRC(a95eedfc) SHA1(5ced2d6869a9895f8ff26d830b21d3c9364b32e7))
GTS80_ROMEND
#define input_ports_bonebffp input_ports_bonebstr
CORE_CLONEDEFNV(bonebffp,bonebstr, "Bone Busters Inc. (French Free Play)",1989,"Flipprojets",gl_mGTS80BS3A,0)

/*-------------------------------------------------------------------
/ Amazon Hunt III (#684D)
/-------------------------------------------------------------------*/
INITGAME(amazonh3, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_8K_ROMSTART(amazonh3, "684d-cpu.rom", CRC(2ec8bd4c) SHA1(46a08ddccba952fa69b79739802b676567f6386f))
GTS80BSSOUND32(              "684d-snd.rom", CRC(a660f233) SHA1(3b80629696a2fd5aa4a86ed472e60c95d3cfa906))
GTS80_ROMEND
#define input_ports_amazonh3 input_ports_gts80
CORE_GAMEDEFNV(amazonh3, "Amazon Hunt III (French)",1991,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(amazn3fp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_8K_ROMSTART(amazn3fp, "684d-cpu_fp.rom", CRC(7db1a9a9) SHA1(41927730ae4571d0a0488baee4d10b756524440e))
GTS80BSSOUND32(              "684d-snd.rom",    CRC(a660f233) SHA1(3b80629696a2fd5aa4a86ed472e60c95d3cfa906))
GTS80_ROMEND
#define input_ports_amazn3fp input_ports_amazonh3
CORE_CLONEDEFNV(amazn3fp,amazonh3, "Amazon Hunt III (French Free Play)",1991,"Flipprojets",gl_mGTS80BS3,0)

INITGAME(amazon3a, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_8K_ROMSTART(amazon3a, "684d-1-cpu.rom", CRC(bf4674e1) SHA1(30974f89f9e4cbb61f8f620499ee6a64c9b7b31c))
GTS80BSSOUND32(              "684d-snd.rom",   CRC(a660f233) SHA1(3b80629696a2fd5aa4a86ed472e60c95d3cfa906))
GTS80_ROMEND
#define input_ports_amazon3a input_ports_amazonh3
CORE_CLONEDEFNV(amazon3a,amazonh3, "Amazon Hunt III (rev. 1)",1991,"Gottlieb",gl_mGTS80BS3,0)

INITGAME(amaz3afp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_8K_ROMSTART(amaz3afp, "684d-1-cpu_fp.rom", CRC(6110c1f0) SHA1(e478510b02fa7d26c0c619f2bfdadd1503a1f6d1))
GTS80BSSOUND32(              "684d-snd.rom",      CRC(a660f233) SHA1(3b80629696a2fd5aa4a86ed472e60c95d3cfa906))
GTS80_ROMEND
#define input_ports_amaz3afp input_ports_amazonh3
CORE_CLONEDEFNV(amaz3afp,amazonh3, "Amazon Hunt III (rev. 1 French Free Play)",1991,"Flipprojets",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ System 80B Test Fixture
/-------------------------------------------------------------------*/
INITGAME(s80btest, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(s80btest, "test2.cpu", CRC(6199c002) SHA1(d997e7a2f10b1780532aea689ee00e0c60e1cc64),
                             "test1.cpu", CRC(032ccbff) SHA1(e6703bd061d7c8c7e8917371d253647cf1320356))
GTS80BSSOUND88(              "testd.snd", CRC(5d04a6d9) SHA1(f83bd8692146af7d234c1a32d0b688e76d1b2b85),
                             "testy.snd", CRC(bd998860) SHA1(8a23376cc646c9854af204e32034bf40ebe23656) BAD_DUMP) // only a bad/patched dump exists, try 03 in sound commander for a surprise (SP0250)
GTS80_ROMEND
#define input_ports_s80btest input_ports_gts80
CORE_GAMEDEFNV(s80btest, "System 80B Test Fixture",198?,"Gottlieb",gl_mGTS80BS1,0)

// Game produced by Premier for International Concepts
/*-------------------------------------------------------------------
/ Night Moves (C-101)
/-------------------------------------------------------------------*/
INITGAME(nmoves, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(nmoves, "nmovsp2.732",  CRC(a2bc00e4) SHA1(5c3e9033f5c72b87058b2f70a0ff0811cc6770fa),
                           "nmovsp1.764",  CRC(36837146) SHA1(88312ae1d1fe76defc4aa2d0a0570c5bb56253e9))
GTS80BSSOUND3232(          "nmovdrom.256", CRC(90929841) SHA1(e203ccd3552c9843c91fc49a437f60ae2dd49142),
                           "nmovyrom.256", CRC(cb74a687) SHA1(af8275807491eb35643cdeb6c898025fde47ceac))
GTS80_ROMEND
#define input_ports_nmoves input_ports_gts80
CORE_GAMEDEFNV(nmoves, "Night Moves",1989,"International Concepts",gl_mGTS80BS3,0)

INITGAME(nmovesfp, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(nmovesfp, "nmovsp2.732",  CRC(a2bc00e4) SHA1(5c3e9033f5c72b87058b2f70a0ff0811cc6770fa),
                             "prom1_fp.cpu", CRC(1cd28cac) SHA1(304139bcd4d496f913399d9945a46aadf32078f9))
GTS80BSSOUND3232(            "nmovdrom.256", CRC(90929841) SHA1(e203ccd3552c9843c91fc49a437f60ae2dd49142),
                             "nmovyrom.256", CRC(cb74a687) SHA1(af8275807491eb35643cdeb6c898025fde47ceac))
GTS80_ROMEND
#define input_ports_nmovesfp input_ports_nmoves
CORE_CLONEDEFNV(nmovesfp,nmoves, "Night Moves (Free Play)",1989,"Flipprojets",gl_mGTS80BS3,0)


// Games by other manufacturers

/*-------------------------------------------------------------------
/ Le Grand 8 (1985)
/-------------------------------------------------------------------*/
INPUT_PORTS_START(grand8)
  CORE_PORTS
  SIM_PORTS(1)
  GTS80_COMPORTS
  GTS80_DIPS
  PORT_START /* 3 */
  COREPORT_DIPNAME( 0x0001, 0x0000, "Sound 1")
    COREPORT_DIPSET(0x0000, " off" )
    COREPORT_DIPSET(0x0001, " on" )
  COREPORT_DIPNAME( 0x0002, 0x0000, "Sound 2")
    COREPORT_DIPSET(0x0000, " off" )
    COREPORT_DIPSET(0x0002, " on" )
  COREPORT_DIPNAME( 0x0004, 0x0000, "Sound 3")
    COREPORT_DIPSET(0x0000, " off" )
    COREPORT_DIPSET(0x0004, " on" )
  COREPORT_DIPNAME( 0x0008, 0x0000, "Sound 4")
    COREPORT_DIPSET(0x0000, " off" )
    COREPORT_DIPSET(0x0008, " on" )
INPUT_PORTS_END
INIT_S80(grand8, dispNumeric1, SNDBRD_TABART3)
GTS80_1_ROMSTART("652.cpu", CRC(5386e5fb) SHA1(822f47951b702f9c6a1ce674baaab0a596f34413))
SOUNDREGION(0x10000, REGION_CPU2)
  ROM_LOAD("grand8.bin", 0, 0x2000, CRC(b7cfaaae) SHA1(60eb4f9bc7b7d11ec6d353b0ae02484cf1c0c9ee))
GTS80_ROMEND
CORE_CLONEDEFNV(grand8,gts80,"Grand 8, Le",1985,"Christian Tabart (France)",gl_mGTS80TAB,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ ManilaMatic: using entire address space, no multiple mapping needed
/-------------------------------------------------------------------*/
static MEMORY_READ_START(manila_readmem)
  {0x0000,0x017f, MRA_RAM},      /*combined RIOT RAM space*/
  {0x0200,0x027f, riot6532_0_r}, /*U4 - I/O*/
  {0x0280,0x02ff, riot6532_1_r}, /*U5 - I/O*/
  {0x0300,0x037f, riot6532_2_r}, /*U6 - I/O*/
  {0x1000,0x17ff, MRA_ROM},      /*Game Prom*/
  {0x1800,0x1fff, MRA_RAM},      /*RAM - 2KBytes*/
  {0x2000,0xffff, MRA_ROM},      /*Game Prom (continued)*/
MEMORY_END

static MEMORY_WRITE_START(manila_writemem)
  {0x0000,0x017f, MWA_RAM},      /*combined RIOT RAM space*/
  {0x0200,0x027f, riot6532_0_w}, /*U4 - I/O*/
  {0x0280,0x02ff, riot6532_1_w}, /*U5 - I/O*/
  {0x0300,0x037f, riot6532_2_w}, /*U6 - I/O*/
  {0x1000,0x17ff, MWA_NOP},      /*Game Prom*/
  {0x1800,0x1fff, MWA_RAM, &generic_nvram, &generic_nvram_size}, /*RAM - 2KBytes*/
  {0x2000,0xffff, MWA_NOP},      /*Game Prom (continued)*/
MEMORY_END

extern MACHINE_DRIVER_EXTERN(gts80);
static MACHINE_DRIVER_START(manila)
  MDRV_IMPORT_FROM(gts80)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(manila_readmem, manila_writemem)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SCREEN_SIZE(320, 200)
  MDRV_VISIBLE_AREA(0, 319, 0, 199)
  MDRV_IMPORT_FROM(gts80s_b1)
MACHINE_DRIVER_END

/*-------------------------------------------------------------------
/ Top Sound (1988)
/-------------------------------------------------------------------*/
INITGAME(topsound, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
ROM_START(topsound)
  NORMALREGION(0x10000, GTS80_MEMREG_CPU)
    ROM_LOAD("mm_ts_1.cpu", 0x6000, 0x2000, CRC(8ade048f) SHA1(f8527d99461b61a865023e0576ac5a9d33e4f0b0))
    ROM_LOAD("mm_ts_2.cpu", 0x2000, 0x2000, CRC(a525aac8) SHA1(9389688e053beb7db45278524c4d62cf067f817d))
      ROM_RELOAD         (0xe000, 0x2000) // reset vector
  GTS80BSSOUND888(           "drom1a.snd", CRC(b8aa8912) SHA1(abff690256c0030807b2d4dfa0516496516384e8),
                             "yrom1a.snd", CRC(a62e3b94) SHA1(59636c2ac7ebbd116a0eb39479c97299ba391906),
                             "yrom2a.snd", CRC(66645a3f) SHA1(f06261af81e6b1829d639933297d2461a8c993fc))
GTS80_ROMEND
#define input_ports_topsound input_ports_gts80
CORE_GAMEDEFNV(topsound,"Top Sound (French)",1988,"ManilaMatic",manila,0)

/*-------------------------------------------------------------------
/ Master (1988)
/-------------------------------------------------------------------*/
INITGAME(mmmaster, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
ROM_START(mmmaster)
  NORMALREGION(0x10000, GTS80_MEMREG_CPU)
    ROM_LOAD("gprom.cpu", 0x0000, 0x8000, CRC(0ffacb1d) SHA1(c609f49e0933ceb3d7eb1725a3ba0f1486978bd6))
      ROM_RELOAD         (0x8000, 0x8000) // reset vector
  GTS80BSSOUND888(           "drom.snd",  CRC(758e1743) SHA1(6df3011c044796afcd88e52d1ca69692cb489ff4),
                             "yrom1.snd", CRC(4869b0ec) SHA1(b8a56753257205af56e06105515b8a700bb1935b),
                             "yrom2.snd", CRC(0528c024) SHA1(d24ff7e088b08c1f35b54be3c806f8a8757d96c7))
GTS80_ROMEND
#define input_ports_mmmaster input_ports_gts80
CORE_GAMEDEFNV(mmmaster,"Master (Italian)",1988,"ManilaMatic",manila,0)
