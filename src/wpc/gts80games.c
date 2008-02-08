#include "driver.h"
#include "sndbrd.h"
#include "sim.h"
#include "gts80.h"
#include "gts80s.h"

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
GAMEX(1980,gts80s,gts80,gts80s,gts80,gts80,ROT0,"Gottlieb","System 80",NOT_A_DRIVER)

INIT_S80A(gts80a, dispNumeric3, SNDBRD_GTS80SS, 0) GTS80_ROMEND
GAMEX(1981,gts80a,0,gts80ss,gts80,gts80,ROT0,"Gottlieb","System 80A with speech board",NOT_A_DRIVER)
INIT_S80A(gts80as, dispNumeric3, SNDBRD_GTS80S, 0) GTS80_ROMEND
GAMEX(1981,gts80as,gts80a,gts80s,gts80,gts80,ROT0,"Gottlieb","System 80A",NOT_A_DRIVER)

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

// System80

/*-------------------------------------------------------------------
/ Spiderman
/-------------------------------------------------------------------*/
INIT_S80(spidermn, dispNumeric1, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("653-1.cpu",    CRC(674ddc58) SHA1(c9be45391b8dd58a0836801807d593d4c7da9904),
                  "653-2.cpu",    CRC(ff1ddfd7) SHA1(dd7b98e491045916153b760f36432506277a4093))
GTS80S1K_ROMSTART("653.snd",      CRC(f5650c46) SHA1(2d0e50fa2f4b3d633daeaa7454630e3444453cb2),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_spidermn input_ports_gts80
CORE_CLONEDEFNV(spidermn,gts80s,"Spiderman",1980,"Gottlieb",gl_mGTS80S,0)

INIT_S80D7(spiderm7, dispNumeric3, SNDBRD_GTS80S)
GTS80_2_ROMSTART ("653-1.cpu",    CRC(674ddc58) SHA1(c9be45391b8dd58a0836801807d593d4c7da9904),
                  "653-2.cpu",    CRC(ff1ddfd7) SHA1(dd7b98e491045916153b760f36432506277a4093))
GTS80S1K_ROMSTART("653.snd",      CRC(f5650c46) SHA1(2d0e50fa2f4b3d633daeaa7454630e3444453cb2),
                  "6530sy80.bin", CRC(c8ba951d) SHA1(e4aa152b36695a0205c19a8914e4d77373f64c6c))
GTS80_ROMEND
#define input_ports_spiderm7 input_ports_spidermn
CORE_CLONEDEFNV(spiderm7,spidermn,"Spiderman (7-digit conversion)",2008,"Oliver",gl_mGTS80S,0)

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

INIT_S80D7(mars7, dispNumeric3, SNDBRD_GTS80SS_VOTRAX)
GTS80_1_ROMSTART  ("666-1.cpu",  CRC(bb7d476a) SHA1(22d5d7f0e52c5180f73a1ca0b3c6bd4b7d0843d6))
GTS80SS22_ROMSTART("666-s1.snd", CRC(d33dc8a5) SHA1(8d071c392996a74c3cdc2cf5ea3be3c86553ce89),
                   "666-s2.snd", CRC(e5616f3e) SHA1(a6b5ebd0b456a555db0889cd63ce79aafc64dbe5))
GTS80_ROMEND
#define input_ports_mars7 input_ports_mars
CORE_CLONEDEFNV(mars7,mars,"Mars - God of War (7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

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
CORE_CLONEDEFNV(blckhole,gts80,"Black Hole",1981,"Gottlieb",gl_mGTS80SS,0)

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
CORE_CLONEDEFNV(blkhole2,blckhole,"Black Hole (Rev. 2)",1981,"Gottlieb",gl_mGTS80SS,0)

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
CORE_CLONEDEFNV(hh,gts80,"Haunted House (Rev 2)",1982,"Gottlieb",gl_mGTS80SS,0)

INIT_S80D7(hh7, dispNumeric4, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-2.cpu",  CRC(f3085f77) SHA1(ebd43588401a735d9c941d06d67ac90183139e90))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh7 input_ports_hh
CORE_CLONEDEFNV(hh7,hh,"Haunted House (Rev 2, 7-digit conversion)",2008,"Oliver",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Haunted House up to serial no. 4999
/-------------------------------------------------------------------*/
INIT_S80(hh_1, dispNumeric2, SNDBRD_GTS80SS)
GTS80_1_ROMSTART  ("669-1.cpu",  CRC(96e72b93) SHA1(3eb3d3e064ba2fe637bba2a93ffd07f00edfa0f2))
GTS80SS22_ROMSTART("669-s1.snd", CRC(52ec7335) SHA1(2b08dd8a89057c9c8c184d5b723ecad01572129f),
                   "669-s2.snd", CRC(a3317b4b) SHA1(c3b14aa58fd4588c8b8fa3540ea6331a9ee40f1f))
GTS80_ROMEND
#define input_ports_hh_1 input_ports_hh
CORE_CLONEDEFNV(hh_1,hh,"Haunted House (Rev 1)",1982,"Gottlieb",gl_mGTS80SS,0)

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
CORE_CLONEDEFNV(s80tst,gts80,"System 80 Test",1981,"Gottlieb",gl_mGTS80SS,0)

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
GTS80_1_ROMSTART  ("pv810-1.cpu",CRC(dd8d516c) SHA1(011d8744a7984ed4c7ceb1f57dcbd8fdb22e21fe))
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

//Amazon II  (No Roms)
//Amazon III (No Roms)

/*-------------------------------------------------------------------
/ Rack 'Em Up (#685)
/-------------------------------------------------------------------*/
INIT_S80A(rackemup, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("685.cpu",   CRC(4754d68d) SHA1(2af743287c1a021f3e130d3d6e191ec9724d640c))
GTS80S2K_ROMSTART("685-s.snd", CRC(d4219987) SHA1(7385d8723bdc937e7c9d6bf7f26ca06f64a9a212))
GTS80_ROMEND
#define input_ports_rackemup input_ports_gts80
CORE_CLONEDEFNV(rackemup,gts80as,"Rack 'Em Up",1983,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Ready...Aim...Fire! (#686)
/-------------------------------------------------------------------*/
INIT_S80A(raimfire, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("686.cpu",   CRC(d1e7a0de) SHA1(b9af2fcaadc55d37c7d9d22621c3817eb751de6b))
GTS80S2K_ROMSTART("686-s.snd", CRC(09740682) SHA1(4f36d78207bd5b8e7abb7118f03acbb3885173c2))
GTS80_ROMEND
#define input_ports_raimfire input_ports_gts80
CORE_CLONEDEFNV(raimfire,gts80as,"Ready...Aim...Fire!",1983,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Jacks To Open (#687)
/-------------------------------------------------------------------*/
INIT_S80A(jack2opn, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("687.cpu",   CRC(0080565e) SHA1(c08412ba24d2ffccf11431e80bd2fc95fc4ce02b))
GTS80S2K_ROMSTART("687-s.snd", CRC(f9d10b7a) SHA1(db255711ed6cb46d183c0ae3894df447f3d8a8e3))
GTS80_ROMEND
#define input_ports_jack2opn input_ports_gts80
CORE_CLONEDEFNV(jack2opn,gts80as,"Jacks to Open",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Alien Star (#689)
/-------------------------------------------------------------------*/
INIT_S80A(alienstr, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("689.cpu",   CRC(4262006b) SHA1(66520b66c31efd0dc654630b2d3567da799b4d89))
GTS80S2K_ROMSTART("689-s.snd", CRC(e1e7a610) SHA1(d4eddfc970127cf3a7d086ad46cbc7b95fdc269d))
GTS80_ROMEND
#define input_ports_alienstr input_ports_gts80
CORE_CLONEDEFNV(alienstr,gts80as,"Alien Star",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ The Games (#691)
/-------------------------------------------------------------------*/
INIT_S80A(thegames, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("691.cpu",   CRC(50f620ea) SHA1(2f997a637eba4eb362586d3aa8caac44acccc795))
GTS80S2K_ROMSTART("691-s.snd", CRC(d7011a31) SHA1(edf5de6cf5ddc1eb577dd1d8dcc9201522df8315))
GTS80_ROMEND
#define input_ports_thegames input_ports_gts80
CORE_CLONEDEFNV(thegames,gts80as,"The Games",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Touchdown (#688)
/-------------------------------------------------------------------*/
INIT_S80A(touchdn, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("688.cpu",   CRC(e531ab3f) SHA1(695aef0dd911fee27ac2d1493a9646b5430a07d5))
GTS80S2K_ROMSTART("688-s.snd", CRC(5e9988a6) SHA1(5f531491722d3c30cf4a7c17982813a7c548387a))
GTS80_ROMEND
#define input_ports_touchdn input_ports_gts80
CORE_CLONEDEFNV(touchdn,gts80as,"Touchdown",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ El Dorado City of Gold (#692)
/-------------------------------------------------------------------*/
INIT_S80A(eldorado, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("692-2.cpu", CRC(4ee6d09b) SHA1(5da0556204e76029380366f9fbb5662715cc3257))
GTS80S2K_ROMSTART("692-s.snd", CRC(9bfbf400) SHA1(58aed9c0b1f52bcd0b53edcdf7af576bb175e3d6))
GTS80_ROMEND
#define input_ports_eldorado input_ports_gts80
CORE_CLONEDEFNV(eldorado,gts80as,"El Dorado City of Gold",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Ice Fever (#695)
/-------------------------------------------------------------------*/
INIT_S80A(icefever, dispNumeric3, SNDBRD_GTS80SP,0)
GTS80_1_ROMSTART ("695.cpu",   CRC(2f6e9caf) SHA1(4f9eeafcbaf758ee6bbad74611b4912ff75b8576))
GTS80S2K_ROMSTART("695-s.snd", CRC(daededc2) SHA1(b43303c1e39b21f3fcbc339d440ea051ced1ea26))
GTS80_ROMEND
#define input_ports_icefever input_ports_gts80
CORE_CLONEDEFNV(icefever,gts80as,"Ice Fever",1985,"Gottlieb",gl_mGTS80S,0)

// System 80b

/*-------------------------------------------------------------------
/ Chicago Cubs' Triple Play (#696)
/-------------------------------------------------------------------*/
// using System80 sound only board
INITGAME(triplay, GEN_GTS80B, FLIP616, dispAlpha,SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(triplay, "prom1.cpu", CRC(42b29b01) SHA1(58145ce10939d00faff49972ada669005a223792))
GTS80S2K_ROMSTART(          "696-s.snd",CRC(deedea61) SHA1(6aec221397f250d5dd99faefa313e8028c8818f7))
GTS80_ROMEND
#define input_ports_triplay input_ports_gts80
CORE_GAMEDEFNV(triplay, "Triple Play",1985,"Gottlieb",gl_mGTS80B,0)

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

/*-------------------------------------------------------------------
/ Tag-Team Wrestling (#698)
/-------------------------------------------------------------------*/
INITGAME(tagteam, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteam, "prom2.cpu", CRC(fd1615ce) SHA1(3a6c3525552286b86e5340af2bf196f12adc9b35),
                            "prom1.cpu", CRC(65931038) SHA1(6d2f1a9fb1b3ce4610074fd3f2ac37ad6af70a44))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagteam input_ports_gts80
CORE_GAMEDEFNV(tagteam, "Tag-Team Wrestling",1985,"Gottlieb",gl_mGTS80B,0)

INITGAME(tagteam2, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteam2,"prom2a.cpu", CRC(6d56b636) SHA1(8f50f2742be727835e7343307787b4b5daa1623a),
                            "prom1a.cpu", CRC(92766607) SHA1(29744dd3c447cc51fb123750ae1456329122e986))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7) SHA1(12b017692f078dcdc8e4bbf1ffcea1c5d0293d06))
GTS80_ROMEND
#define input_ports_tagteam2 input_ports_gts80
CORE_CLONEDEFNV(tagteam2,tagteam,"Tag-Team Wrestling (rev.2)",1985,"Gottlieb",gl_mGTS80B,0)

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

/*-------------------------------------------------------------------
/ Raven
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

/*-------------------------------------------------------------------
/ Hollywood Heat
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

/*-------------------------------------------------------------------
/ Monte Carlo
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

/*-------------------------------------------------------------------
/ Spring Break
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

/*-------------------------------------------------------------------
/ Arena
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

/****************************************/
/* Start of Generation 2 Sound Hardware */
/****************************************/

/*-------------------------------------------------------------------
/ Victory
/-------------------------------------------------------------------*/
INITGAME(victory, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(victory, "prom2.cpu", CRC(6a42eaf4) SHA1(3e28b01473266db463986a4283e1be85f2410fb1),
                            "prom1.cpu", CRC(e724db90) SHA1(10e760e129ce89f11372c6dd3616216d45f2c926))
GTS80BSSOUND3232(           "drom1.snd", CRC(4ab6dab7) SHA1(7e21e69029e60052112ddd5c7481582ea6684dc1),
                            "yrom1.snd", CRC(921a100e) SHA1(0c3c7eae4ceeb5a1a8150bac52203d3f1e8f917e))
GTS80_ROMEND
#define input_ports_victory input_ports_gts80
CORE_GAMEDEFNV(victory, "Victory",1987,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Diamond Lady
/-------------------------------------------------------------------*/
INITGAME(diamond, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamond, "prom2.cpu", CRC(862951dc) SHA1(b15899ecf7ec869e3722cef3f5c16b0dadd2514e),
                            "prom1.cpu", CRC(7a011757) SHA1(cc49ec7451feae035670ea9d70cc8f6b32747c90))
GTS80BSSOUND3232(           "drom1.snd", CRC(c216d1e4) SHA1(aa38db5ad36d1d1d35e727ab27c1f1c05a9627cd),
                            "yrom1.snd", CRC(0a18d626) SHA1(6b367668be55ca04c69c4c4c5a4a524ae8f790f8))
GTS80_ROMEND
#define input_ports_diamond input_ports_gts80
CORE_GAMEDEFNV(diamond, "Diamond Lady",1988,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ TX-Sector
/-------------------------------------------------------------------*/
INITGAME(txsector, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsector, "prom2.cpu", CRC(f12514e6) SHA1(80bca17c33df99ed1a7acc21f7f70ea90e7c0463),
                             "prom1.cpu", CRC(e51d39da) SHA1(b6e4d573b62cc441a153cc4d8b647ee46b4dd2a7))
GTS80BSSOUND3232(            "drom1.snd", CRC(61d66ca1) SHA1(59b1705b13d46b29f45257c566274f3cdce15ec2),
                             "yrom1.snd", CRC(469ef444) SHA1(faa16f34357a53c3fc61b59251fabdc44c605000))
GTS80_ROMEND
#define input_ports_txsector input_ports_gts80
CORE_GAMEDEFNV(txsector, "TX-Sector",1988,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Robo-War
/-------------------------------------------------------------------*/
INITGAME(robowars, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowars, "prom2.cpu", CRC(893177ed) SHA1(791540a64d498979e5b0c8baf4ceb2fd5ff7f047),
                             "prom1.cpu", CRC(cd1587d8) SHA1(77e8e02dc03d052e9e4ce19c9431439e4211a29f))
GTS80BSSOUND3232(            "drom1.snd", CRC(ea59b6a1) SHA1(6a4cdd37ba85f94f703afd1c5d3f102f51fedf46),
                             "yrom1.snd", CRC(7ecd8b67) SHA1(c5167b0acc64e535d389ba70be92a65672e119f6))
GTS80_ROMEND
#define input_ports_robowars input_ports_gts80
CORE_GAMEDEFNV(robowars, "Robo-War",1988,"Gottlieb",gl_mGTS80BS2,0)

/****************************************/
/* Start of Generation 3 Sound Hardware */
/****************************************/

/*-------------------------------------------------------------------
/ Excalibur
/-------------------------------------------------------------------*/
INITGAME(excalibr, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_4K_ROMSTART(excalibr, "prom2.cpu", CRC(499e2e41) SHA1(1e3fcba18882bd7df30a43843916aa5d7968eecc),
                             "prom1.cpu", CRC(ed1083d7) SHA1(3ff829ecfaba7d20c75268d3ee5224cb3cac3507))
GTS80BSSOUND3232(            "drom1.snd", CRC(a4368cd0) SHA1(c48513e56899938dc83a3545d8ee9def3dc1491f),
                             "yrom1.snd", CRC(9f194744) SHA1(dbd73b546071c3d4f0dcfe21e3e646da716c5b71))
GTS80_ROMEND
#define input_ports_excalibr input_ports_gts80
CORE_GAMEDEFNV(excalibr, "Excalibur",1988,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Bad Girls
/-------------------------------------------------------------------*/
INITGAME(badgirls, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgirls, "prom2.cpu", CRC(583933ec) SHA1(89da6750d779d68db578715b058f9321695b79b0),
                             "prom1.cpu", CRC(956aeae0) SHA1(24d9d514fc83aba1ab310bfe4ed80605df399417))
GTS80BSSOUND3232(            "drom1.snd", CRC(452dec20) SHA1(a9c41dfb2d83c5671ab96e946f13df774b567976),
                             "yrom1.snd", CRC(ab3b8e2d) SHA1(b57a0b804b42b923bb102d295e3b8a69b1033d27))
GTS80_ROMEND
#define input_ports_badgirls input_ports_gts80
CORE_GAMEDEFNV(badgirls, "Bad Girls",1988,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Big House
/-------------------------------------------------------------------*/
INITGAME(bighouse, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighouse, "prom2.cpu", CRC(047c8ef5) SHA1(3afa2a0011b724836b69b2ef386597e0953dfadf),
                             "prom1.cpu", CRC(0ecef900) SHA1(78e4ed6e40fdb45dde2d0f2cf60d4c8a7ea2e39e))
GTS80BSSOUND3232(            "drom1.snd", CRC(f330fd04) SHA1(1288c47f636d9d5b826a2b870b81788a630e489e),
                             "yrom1.snd", CRC(0b1ba1cb) SHA1(26327689992018837b1c9957c515ab67248623eb))
GTS80_ROMEND
#define input_ports_bighouse input_ports_gts80
CORE_GAMEDEFNV(bighouse, "Big House",1989,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Hot Shots
/-------------------------------------------------------------------*/
INITGAME(hotshots, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshots, "prom2.cpu", CRC(7695c7db) SHA1(90188ff83b888262ba849e5af9d99145c5bc1c30),
                             "prom1.cpu", CRC(122ff4a8) SHA1(195392b9f2050b52392a123831bb7a9428087c1b))
GTS80BSSOUND3232(            "drom1.snd", CRC(42c3cc3d) SHA1(26ca7f3a71b83df18ac6be1d1eb28da20120285e),
                             "yrom1.snd", CRC(2933a80e) SHA1(5982b9ed361d90f8ea47047fc29770ef142acbec))
GTS80_ROMEND
#define input_ports_hotshots input_ports_gts80
CORE_GAMEDEFNV(hotshots, "Hot Shots",1989,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Bone Busters Inc.
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


/*-------------------------------------------------------------------
/ Night Moves C-103
/-------------------------------------------------------------------*/
INITGAME(nmoves, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(nmoves, "nmovsp2.732", CRC(a2bc00e4) SHA1(5c3e9033f5c72b87058b2f70a0ff0811cc6770fa),
                             "nmovsp1.764", CRC(36837146) SHA1(88312ae1d1fe76defc4aa2d0a0570c5bb56253e9))
GTS80BSSOUND3232(            "nmovdrom.256", CRC(90929841) SHA1(e203ccd3552c9843c91fc49a437f60ae2dd49142),
                             "nmovyrom.256", CRC(cb74a687) SHA1(af8275807491eb35643cdeb6c898025fde47ceac))
GTS80_ROMEND
#define input_ports_nmoves input_ports_gts80
CORE_GAMEDEFNV(nmoves, "Night Moves",1989,"International Concepts",gl_mGTS80BS3,0)
