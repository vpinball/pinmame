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
  DISP_SEG_CREDIT(40,41,CORE_SEG9), DISP_SEG_BALLS(42,43,CORE_SEG9), {0}
};
/* 5 x 6 BCD + Ball, Credit */
static core_tLCDLayout dispNumeric2[] = {
  {0, 0, 2, 6,CORE_SEG9}, {0,16, 9, 6,CORE_SEG9},
  {4, 0,22, 6,CORE_SEG9}, {4,16,29, 6,CORE_SEG9},
  DISP_SEG_CREDIT(40,41,CORE_SEG9), DISP_SEG_BALLS(42,43,CORE_SEG9),
  {6, 8,50, 6,CORE_SEG9}, {0}
};
/* 4 x 7 BCD + Ball,Credit */
static core_tLCDLayout dispNumeric3[] = {
  {0, 0, 2, 7,CORE_SEG98F}, {0,16, 9, 7,CORE_SEG98F},
  {4, 0,22, 7,CORE_SEG98F}, {4,16,29, 7,CORE_SEG98F},
  DISP_SEG_CREDIT(40,41,CORE_SEG9), DISP_SEG_BALLS(42,43,CORE_SEG9), {0}
};

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0,20,CORE_SEG16},  {2, 0,20,20,CORE_SEG16}, {0}
};

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

// System80

/*-------------------------------------------------------------------
/ Spiderman
/-------------------------------------------------------------------*/
INITGAME(spidermn,GEN_GTS80,0, dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_2_ROMSTART(spidermn,"653-1.cpu",    CRC(674ddc58),
                          "653-2.cpu",    CRC(ff1ddfd7),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "653.snd",      CRC(f5650c46),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_spidermn input_ports_gts80
CORE_GAMEDEFNV(spidermn,"Spiderman",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Panthera
/-------------------------------------------------------------------*/
INITGAME(panthera,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(panthera,"652.cpu",      CRC(5386e5fb),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "652.snd",      CRC(4d0cf2c0),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_panthera input_ports_gts80
CORE_GAMEDEFNV(panthera,"Panthera",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Circus
/-------------------------------------------------------------------*/
INITGAME(circus,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_2_ROMSTART(circus,"654-1.cpu",    CRC(0eeb2731),
                        "654-2.cpu",    CRC(01e23569),
                        "u2_80.bin",    CRC(4f0bc7b1),
                        "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(      "654.snd",      CRC(75c3ad67),
                        "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_circus input_ports_gts80
CORE_GAMEDEFNV(circus,"Circus",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Counterforce
/-------------------------------------------------------------------*/
INITGAME(cntforce,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_2_ROMSTART(cntforce,"656-1.cpu",    CRC(42baf51d),
                          "656-2.cpu",    CRC(0e185c30),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "656.snd",      CRC(0be2cbe9),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_cntforce input_ports_gts80
CORE_GAMEDEFNV(cntforce,"Counterforce",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Star Race
/-------------------------------------------------------------------*/
INITGAME(starrace,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_2_ROMSTART(starrace, "657-1.cpu",    CRC(27081372),
                           "657-2.cpu",    CRC(c56e31c8),
                           "u2_80.bin",    CRC(4f0bc7b1),
                           "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(         "657.snd",      CRC(3a1d3995),
                           "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_starrace input_ports_gts80
CORE_GAMEDEFNV(starrace,"Star Race",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ James Bond (Timed Play)
/-------------------------------------------------------------------*/
INITGAME(jamesb,GEN_GTS80,0,dispNumeric2, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(jamesb, "658-1.cpu",    CRC(b841ad7a),
                         "u2_80.bin",    CRC(4f0bc7b1),
                         "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(       "658.snd",      CRC(962c03df),
                         "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_jamesb input_ports_gts80
CORE_GAMEDEFNV(jamesb,"James Bond (Timed Play)",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ James Bond (3/5 Ball Play)
/-------------------------------------------------------------------*/
GTS80_1_ROMSTART(jamesb2, "658-x.cpu",    CRC(e7e0febf),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "658.snd",      CRC(962c03df),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_jamesb2 input_ports_gts80
#define init_jamesb2 init_jamesb
CORE_CLONEDEFNV(jamesb2,jamesb,"James Bond (3/5-Ball)",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Time Line
/-------------------------------------------------------------------*/
static core_tLCDLayout dispTimeline[] = {
  DISP_SEG_IMPORT(dispNumeric1), {6, 12, 50, 2, CORE_SEG9}, {0}
};
INITGAME(timeline,GEN_GTS80,0,dispTimeline, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(timeline, "659.cpu",      CRC(d6950e3b),
                           "u2_80.bin",    CRC(4f0bc7b1),
                           "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(         "659.snd",      CRC(28185568),
                           "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_timeline input_ports_gts80
CORE_GAMEDEFNV(timeline,"Time Line",1980,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Force II
/-------------------------------------------------------------------*/
INITGAME(forceii,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(forceii, "661-2.cpu",    CRC(a4fa42a4),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "661.snd",      CRC(650158a7),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_forceii input_ports_gts80
CORE_GAMEDEFNV(forceii,"Force II",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Pink Panther
/-------------------------------------------------------------------*/
INITGAME(pnkpnthr,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(pnkpnthr, "664-1.cpu",    CRC(a0d3e69a),
                           "u2_80.bin",    CRC(4f0bc7b1),
                           "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(         "664.snd",      CRC(18f4abfd),
                           "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_pnkpnthr input_ports_gts80
CORE_GAMEDEFNV(pnkpnthr,"Pink Panther",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Mars - God of War
/-------------------------------------------------------------------*/
INITGAME(mars,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(mars, "666-1.cpu",  CRC(bb7d476a),
                       "u2_80.bin",  CRC(4f0bc7b1),
                       "u3_80.bin",  CRC(1e69f9d0))
GTS80SS22_ROMSTART(    "666-s1.snd", CRC(d33dc8a5),
                       "666-s2.snd", CRC(e5616f3e))
GTS80_ROMEND
#define input_ports_mars input_ports_gts80
CORE_GAMEDEFNV(mars,"Mars - God of War",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Volcano (Sound and Speech)
/-------------------------------------------------------------------*/
INITGAME(vlcno_ax,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(vlcno_ax, "667-a-x.cpu", CRC(1f51c351),
                           "u2_80.bin",   CRC(4f0bc7b1),
                           "u3_80.bin",   CRC(1e69f9d0))
GTS80SS22_ROMSTART(        "667-s1.snd",  CRC(ba9d40b7),
                           "667-s2.snd",  CRC(b54bd123))
GTS80_ROMEND
#define input_ports_vlcno_ax input_ports_gts80
CORE_GAMEDEFNV(vlcno_ax,"Volcano",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Volcano (Sound Only)
/-------------------------------------------------------------------*/
INITGAME(vlcno_1b,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(vlcno_1b,"667-1b.cpu" ,  CRC(a422d862),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART("667-a-s.snd",  CRC(894b4e2e),
                  "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_vlcno_1b input_ports_gts80
CORE_CLONEDEFNV(vlcno_1b,vlcno_ax,"Volcano (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 4)
/-------------------------------------------------------------------*/
INITGAME(blckhole,GEN_GTS80,0,dispNumeric2, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(blckhole, "668-4.cpu",  CRC(01b53045),
                           "u2_80.bin",  CRC(4f0bc7b1),
                           "u3_80.bin",  CRC(1e69f9d0))
GTS80SS22_ROMSTART(        "668-s1.snd", CRC(23d5045d),
                           "668-s2.snd", CRC(d63da498))
GTS80_ROMEND
#define input_ports_blckhole input_ports_gts80
CORE_GAMEDEFNV(blckhole,"Black Hole",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 2)
/-------------------------------------------------------------------*/
GTS80_1_ROMSTART(blkhole2, "668-2.cpu",  CRC(df03ffea),
                           "u2_80.bin",  CRC(4f0bc7b1),
                           "u3_80.bin",  CRC(1e69f9d0))
GTS80SS22_ROMSTART(        "668-s1.snd", CRC(23d5045d),
                           "668-s2.snd", CRC(d63da498))
GTS80_ROMEND
#define input_ports_blkhole2 input_ports_gts80
#define init_blkhole2 init_blckhole
CORE_CLONEDEFNV(blkhole2,blckhole,"Black Hole (Rev. 2)",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Black Hole (Sound Only)
/-------------------------------------------------------------------*/
INITGAME(blkholea,GEN_GTS80,0,dispNumeric2, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(blkholea, "668-a2.cpu" ,  CRC(df56f896),
                           "u2_80.bin",    CRC(4f0bc7b1),
                           "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(         "668-a-s.snd",  CRC(5175f307),
                           "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_blkholea input_ports_gts80
CORE_CLONEDEFNV(blkholea,blckhole,"Black Hole (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Haunted House, since serial no. 5000
/-------------------------------------------------------------------*/
INITGAME(hh,GEN_GTS80,0,dispNumeric2, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(hh, "669-2.cpu",  CRC(f3085f77),
                     "u2_80.bin",  CRC(4f0bc7b1),
                     "u3_80.bin",  CRC(1e69f9d0))
GTS80SS22_ROMSTART(  "669-s1.snd", CRC(52ec7335),
                     "669-s2.snd", CRC(a3317b4b))
GTS80_ROMEND
#define input_ports_hh input_ports_gts80
CORE_GAMEDEFNV(hh,"Haunted House (Rev 2)",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Haunted House up to serial no. 4999
/-------------------------------------------------------------------*/
GTS80_1_ROMSTART(hh_1, "669-1.cpu",  CRC(96e72b93),
                      "u2_80.bin",  CRC(4f0bc7b1),
                      "u3_80.bin",  CRC(1e69f9d0))
GTS80SS22_ROMSTART(   "669-s1.snd", CRC(52ec7335),
                      "669-s2.snd", CRC(a3317b4b))
GTS80_ROMEND
#define input_ports_hh_1 input_ports_gts80
#define init_hh_1 init_hh
CORE_CLONEDEFNV(hh_1,hh,"Haunted House (Rev 1)",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Eclipse
/-------------------------------------------------------------------*/
INITGAME(eclipse,GEN_GTS80,0,dispNumeric2, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(eclipse, "671-a.cpu",    CRC(efad7312),
                          "u2_80.bin",    CRC(4f0bc7b1),
                          "u3_80.bin",    CRC(1e69f9d0))
GTS80S1K_ROMSTART(        "671-a-s.snd",  CRC(5175f307),
                          "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_eclipse input_ports_gts80
CORE_GAMEDEFNV(eclipse,"Eclipse",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ System 80 Test Fixture
/-------------------------------------------------------------------*/
INITGAME(s80tst,GEN_GTS80,0,dispNumeric1, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(s80tst, "80tst.cpu",    CRC(a0f9e56b),
                         "u2_80.bin",    CRC(4f0bc7b1),
                         "u3_80.bin",    CRC(1e69f9d0))
GTS80SS22_ROMSTART(      "80tst-s1.snd", CRC(b9dbdd21),
                         "80tst-s2.snd", CRC(1a4b1e9d))
GTS80_ROMEND
#define input_ports_s80tst input_ports_gts80
CORE_GAMEDEFNV(s80tst,"System 80 Test",1981,"Gottlieb",gl_mGTS80SS,0)

// System 80a

/*-------------------------------------------------------------------
/ Devil's Dare (Sound and Speech) (#670)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispDevilsdare[] = {
  DISP_SEG_IMPORT(dispNumeric3),
  {6, 1,50,6,CORE_SEG9}, {6,17,44,6,CORE_SEG9}, {0}
};
INITGAME(dvlsdre,GEN_GTS80A,0,dispDevilsdare, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(dvlsdre,"670-1.cpu", CRC(6318bce2),
                         "u2_80a.bin", CRC(241de1d4),
                         "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(      "670-s1.snd", CRC(506bc22a),
                         "670-s2.snd", CRC(f662ee4b))
GTS80_ROMEND
#define input_ports_dvlsdre input_ports_gts80
CORE_GAMEDEFNV(dvlsdre,"Devil's Dare",1981,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Devil's Dare (Sound Only) (#670)
/-------------------------------------------------------------------*/
INITGAME(dvlsdre2,GEN_GTS80A,0,dispDevilsdare, SNDBRD_GTS80S,0,0)
GTS80_1_ROMSTART(dvlsdre2, "670-a.cpu",    CRC(353b2e18),
                           "u2_80a.bin",   CRC(241de1d4),
                           "u3_80a.bin",   CRC(2d77ccdc))
GTS80S1K_ROMSTART(         "670-a-s.snd",  CRC(f141d535),
                           "6530sy80.bin", CRC(c8ba951d))
GTS80_ROMEND
#define input_ports_dvlsdre2 input_ports_gts80
CORE_CLONEDEFNV(dvlsdre2,dvlsdre,"Devil's Dare (Sound Only)",1981,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Caveman (#PV-810) Pinball/Video Combo
/-------------------------------------------------------------------*/
INITGAME(caveman, GEN_GTS80A,0, GTS80_dispCaveman, SNDBRD_GTS80SS,GTS80_DISPVIDEO,0)
GTS80_1_ROMSTART(caveman,"pv810-1.cpu", CRC(dd8d516c),
                         "u2_80a.bin",  CRC(241de1d4),
                         "u3_80a.bin",  CRC(2d77ccdc))
GTS80SS22_ROMSTART(      "pv810-s1.snd",CRC(a491664d),
                         "pv810-s2.snd",CRC(d8654e6e))
VIDEO_ROMSTART(          "v810-u8.bin", CRC(514aa152),
                         "v810-u7.bin", CRC(74c6533e),
                         "v810-u6.bin", CRC(2fd0ee95),
                         "v810-u5.bin", CRC(2fb15da3),
                         "v810-u4.bin", CRC(2dfe8492),
                         "v810-u3.bin", CRC(740e9ec3),
                         "v810-u2.bin", CRC(b793baf9),
                         "v810-u1.bin", CRC(0a283b15))
GTS80_ROMEND
#define input_ports_caveman input_ports_gts80vid
CORE_GAMEDEFNV(caveman,"Caveman (Pinball/Video Combo)",1981,"Gottlieb",gl_mGTS80VID,0)

GTS80_1_ROMSTART(cavemana,"pv810-1.cpu", CRC(dd8d516c),
                         "u2_80a.bin",  CRC(241de1d4),
                         "u3_80a.bin",  CRC(2d77ccdc))
GTS80SS22_ROMSTART(      "pv810-s1.snd",CRC(a491664d),
                         "pv810-s2.snd",CRC(d8654e6e))
VIDEO_ROMSTART(          "v810-u8.bin", CRC(514aa152),
                         "v810-u7.bin", CRC(74c6533e),
                         "v810-u6.bin", CRC(2fd0ee95),
                         "v810-u5.bin", CRC(2fb15da3),
                         "v810-u4a.bin",CRC(3437c697),
                         "v810-u3a.bin",CRC(729819f6),
                         "v810-u2a.bin",CRC(ab6193c2),
                         "v810-u1a.bin",CRC(7c6410fb))
GTS80_ROMEND
#define input_ports_cavemana input_ports_caveman
#define init_cavemana init_caveman
CORE_CLONEDEFNV(cavemana,caveman,"Caveman (Pinball/Video Combo) (set 2)",1981,"Gottlieb",gl_mGTS80VID,0)

/*-------------------------------------------------------------------
/ Rocky (#672)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispRocky[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6, 10,54,2,CORE_SEG9}, {6,16,50,2,CORE_SEG9}, {0}
};
INITGAME(rocky, GEN_GTS80A,0, dispRocky, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(rocky, "672-2x.cpu", CRC(8e2f0d39),
                        "u2_80a.bin", CRC(241de1d4),
                        "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(     "672-s1.snd", CRC(10ba523c),
                        "672-s2.snd", CRC(5e77117a))
GTS80_ROMEND
#define input_ports_rocky input_ports_gts80
CORE_GAMEDEFNV(rocky,"Rocky",1982,"Gottlieb",gl_mGTS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Spirit (#673)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispSpirit[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6, 9,50,6,CORE_SEG9}, {0}
};
INITGAME(spirit, GEN_GTS80A,0, dispSpirit, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(spirit, "673-2.cpu",  CRC(a7dc2207),
                         "u2_80a.bin", CRC(241de1d4),
                         "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART("673-s1.snd", CRC(fd3062ae),
                   "673-s2.snd", CRC(7cf923f1))
GTS80_ROMEND
#define input_ports_spirit input_ports_gts80
CORE_GAMEDEFNV(spirit,"Spirit",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Striker (#675)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispStriker[] = { /* as displayed on real machine */
  {2, 0, 2, 7,CORE_SEG98F}, {6, 0, 9, 7,CORE_SEG98F},
  {2,16,22, 7,CORE_SEG98F}, {6,16,29, 7,CORE_SEG98F},
  {0, 2,40, 2,CORE_SEG9},   {0, 8,42, 2,CORE_SEG9}, /* ball/credit */
  {4, 2,52, 2,CORE_SEG9},   {4, 8,50, 2,CORE_SEG9},
  {4,18,48, 2,CORE_SEG9},   {4,24,46, 2,CORE_SEG9}, {0}
};
INITGAME(striker, GEN_GTS80A,0, dispStriker, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(striker, "675.cpu",    CRC(06b66ce8),
                          "u2_80a.bin", CRC(241de1d4),
                          "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(       "675-s1.snd", CRC(cc11c487),
                          "675-s2.snd", CRC(ec30a3d9))
GTS80_ROMEND
#define input_ports_striker input_ports_gts80
CORE_GAMEDEFNV(striker,"Striker",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Punk! (#674)
/-------------------------------------------------------------------*/
INITGAME(punk, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(punk, "674.cpu",    CRC(70cccc57),
                       "u2_80a.bin", CRC(241de1d4),
                       "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(    "674-s1.snd", CRC(b75f79d5),
                       "674-s2.snd", CRC(005d123a))
GTS80_ROMEND
#define input_ports_punk input_ports_gts80
CORE_GAMEDEFNV(punk,"Punk!",1982,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Goin' Nuts (#682)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispGoinNuts[] = {
  DISP_SEG_IMPORT(dispNumeric3), {6,12,53,3,CORE_SEG9}, {0}
};
INITGAME(goinnuts, GEN_GTS80A,0, dispGoinNuts, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(goinnuts, "682.cpu",    CRC(51c7c6de),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(        "682-s1.snd", CRC(f00dabf3),
                           "682-s2.snd", CRC(3be8ac5f))
GTS80_ROMEND
#define input_ports_goinnuts input_ports_gts80
CORE_GAMEDEFNV(goinnuts,"Goin' Nuts",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Krull (#676)
/-------------------------------------------------------------------*/
static core_tLCDLayout dispKrull[] = {
  DISP_SEG_IMPORT(dispNumeric3),
  {6, 8,50,3,CORE_SEG9}, {6,16,53,3,CORE_SEG9}, {0}
};
INITGAME(krull, GEN_GTS80A,0, dispKrull, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(krull, "676-3.cpu",  CRC(71507430),
                        "u2_80a.bin", CRC(241de1d4),
                        "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(     "676-s1.snd", CRC(b1989d8f),
                        "676-s2.snd", CRC(05fade11))
GTS80_ROMEND
#define input_ports_krull input_ports_gts80
CORE_GAMEDEFNV(krull,"Krull",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Q*Bert's Quest (#677)
/-------------------------------------------------------------------*/
INITGAME(qbquest, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(qbquest, "677.cpu",    CRC(fd885874),
                          "u2_80a.bin", CRC(241de1d4),
                          "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(       "677-s1.snd", CRC(af7bc8b7),
                          "677-s2.snd", CRC(820aa26f))
GTS80_ROMEND
#define input_ports_qbquest input_ports_gts80
CORE_GAMEDEFNV(qbquest,"Q*Bert's Quest",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Super Orbit (#680)
/-------------------------------------------------------------------*/
INITGAME(sorbit, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(sorbit, "680.cpu",    CRC(decf84e6),
                         "u2_80a.bin", CRC(241de1d4),
                         "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(      "680-s1.snd", CRC(fccbbbdd),
                         "680-s2.snd", CRC(d883d63d))
GTS80_ROMEND
#define input_ports_sorbit input_ports_gts80
CORE_GAMEDEFNV(sorbit,"Super Orbit",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Royal Flush Deluxe (#681)
/-------------------------------------------------------------------*/
INITGAME(rflshdlx, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(rflshdlx, "681-2.cpu",  CRC(0b048658),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(        "681-s1.snd", CRC(33455bbd),
                           "681-s2.snd", CRC(639c93f9))
GTS80_ROMEND
#define input_ports_rflshdlx input_ports_gts80
CORE_GAMEDEFNV(rflshdlx,"Royal Flush Deluxe",1983,"Gottlieb",gl_mGTS80SS,0)

/*-------------------------------------------------------------------
/ Amazon Hunt (#684)
/-------------------------------------------------------------------*/
INITGAME(amazonh, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SS,0,0)
GTS80_1_ROMSTART(amazonh, "684-2.cpu",  CRC(b0d0c4af),
                          "u2_80a.bin", CRC(241de1d4),
                          "u3_80a.bin", CRC(2d77ccdc))
GTS80SS22_ROMSTART(       "684-s1.snd", CRC(86d239df),
                          "684-s2.snd", CRC(4d8ea26c))
GTS80_ROMEND
#define input_ports_amazonh input_ports_gts80
CORE_GAMEDEFNV(amazonh,"Amazon Hunt",1983,"Gottlieb",gl_mGTS80SS,0)

//Amazon II  (No Roms)
//Amazon III (No Roms)

/*-------------------------------------------------------------------
/ Rack 'Em Up (#685)
/-------------------------------------------------------------------*/
INITGAME(rackemup, GEN_GTS80A,0,dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(rackemup, "685.cpu",    CRC(4754d68d),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "685-s.snd",  CRC(d4219987))
GTS80_ROMEND
#define input_ports_rackemup input_ports_gts80
CORE_GAMEDEFNV(rackemup,"Rack 'Em Up",1983,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Ready...Aim...Fire! (#686)
/-------------------------------------------------------------------*/
INITGAME(raimfire, GEN_GTS80A,0,dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(raimfire, "686.cpu",    CRC(d1e7a0de),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "686-s.snd",  CRC(09740682))
GTS80_ROMEND
#define input_ports_raimfire input_ports_gts80
CORE_GAMEDEFNV(raimfire,"Ready...Aim...Fire!",1983,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Jacks To Open (#687)
/-------------------------------------------------------------------*/
INITGAME(jack2opn,GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(jack2opn, "687.cpu",    CRC(0080565e),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "687-s.snd",  CRC(f9d10b7a))
GTS80_ROMEND
#define input_ports_jack2opn input_ports_gts80
CORE_GAMEDEFNV(jack2opn,"Jacks to Open",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Alien Star (#689)
/-------------------------------------------------------------------*/
INITGAME(alienstr, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(alienstr, "689.cpu",    CRC(4262006b),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "689-s.snd",  CRC(e1e7a610))
GTS80_ROMEND
#define input_ports_alienstr input_ports_gts80
CORE_GAMEDEFNV(alienstr,"Alien Star",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ The Games (#691)
/-------------------------------------------------------------------*/
INITGAME(thegames, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(thegames, "691.cpu",    CRC(50f620ea),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "691-s.snd", CRC(d7011a31))
GTS80_ROMEND
#define input_ports_thegames input_ports_gts80
CORE_GAMEDEFNV(thegames,"The Games",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Touchdown (#688)
/-------------------------------------------------------------------*/
INITGAME(touchdn, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(touchdn, "688.cpu",    CRC(e531ab3f),
                          "u2_80a.bin", CRC(241de1d4),
                          "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(        "688-s.snd", CRC(5e9988a6))
GTS80_ROMEND
#define input_ports_touchdn input_ports_gts80
CORE_GAMEDEFNV(touchdn,"Touchdown",1984,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ El Dorado City of Gold (#692)
/-------------------------------------------------------------------*/
INITGAME(eldorado, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(eldorado, "692-2.cpu",  CRC(4ee6d09b),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "692-s.snd", CRC(d5a10e53))
GTS80_ROMEND
#define input_ports_eldorado input_ports_gts80
CORE_GAMEDEFNV(eldorado,"El Dorado City of Gold",1984,"Gottlieb",gl_mGTS80S,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Ice Fever (#695)
/-------------------------------------------------------------------*/
INITGAME(icefever, GEN_GTS80A,0, dispNumeric3, SNDBRD_GTS80SP,0,0)
GTS80_1_ROMSTART(icefever, "695.cpu",    CRC(2f6e9caf),
                           "u2_80a.bin", CRC(241de1d4),
                           "u3_80a.bin", CRC(2d77ccdc))
GTS80S2K_ROMSTART(         "695-s.snd", CRC(daededc2))
GTS80_ROMEND
#define input_ports_icefever input_ports_gts80
CORE_GAMEDEFNV(icefever,"Ice Fever",1985,"Gottlieb",gl_mGTS80S,0)

// System 80b

/*-------------------------------------------------------------------
/ Chicago Cubs' Triple Play (#696)
/-------------------------------------------------------------------*/
// using System80 sound only board
INITGAME(triplay, GEN_GTS80B, FLIP616, dispAlpha,SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(triplay, "prom1.cpu", CRC(42b29b01))
GTS80S2K_ROMSTART(          "696-s.snd",CRC(deedea61))
GTS80_ROMEND
#define input_ports_triplay input_ports_gts80
CORE_GAMEDEFNV(triplay, "Triple Play",1985,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Bounty Hunter (#694)
/-------------------------------------------------------------------*/
// using System80 sound only board
INITGAME(bountyh, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(bountyh, "prom1.cpu", CRC(e8190df7))
GTS80S2K_ROMSTART(          "694-s.snd", CRC(a0383e41))
GTS80_ROMEND
#define input_ports_bountyh input_ports_gts80
CORE_GAMEDEFNV(bountyh, "Bounty Hunter",1985,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Tag-Team Wrestling (#698)
/-------------------------------------------------------------------*/
INITGAME(tagteam, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80SP,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(tagteam, "prom2.cpu", CRC(fd1615ce),
                            "prom1.cpu", CRC(65931038))
GTS80S2K_ROMSTART("698-s.snd", CRC(9c8191b7))
GTS80_ROMEND
#define input_ports_tagteam input_ports_gts80
CORE_GAMEDEFNV(tagteam, "Tag-Team Wrestling",1985,"Gottlieb",gl_mGTS80S,0)

/*-------------------------------------------------------------------
/ Rock (#697)
/-------------------------------------------------------------------*/
//(I assume these are using Gen.1 hardware, but there's 1 less rom, so who knows)
INITGAME(rock, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_8K_ROMSTART(rock, "prom1.cpu", CRC(1146c1d3))
GTS80BSSOUND88(          "drom1.snd", CRC(03830e81),
                         "yrom1.snd", CRC(effba2ad))
GTS80_ROMEND
#define input_ports_rock input_ports_gts80
CORE_GAMEDEFNV(rock, "Rock",1985,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Raven
/-------------------------------------------------------------------*/
//(I assume these are using Gen.1 hardware, but there's 1 less rom, so who knows)
INITGAME(raven, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(raven, "prom2.cpu", CRC(481f3fb8),
                          "prom1.cpu", CRC(edc88561))
GTS80BSSOUND88(           "drom1.snd", CRC(a04bf7d0),
                          "yrom1.snd", CRC(ee5f868b))
GTS80_ROMEND
#define input_ports_raven input_ports_gts80
CORE_GAMEDEFNV(raven, "Raven",1986,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Hollywood Heat
/-------------------------------------------------------------------*/
INITGAME(hlywoodh, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(hlywoodh, "prom2.cpu", CRC(a465e5f3),
                             "prom1.cpu", CRC(0493e27a))
GTS80BSSOUND888(             "drom1.snd", CRC(a698ec33),
                             "yrom1.snd", CRC(9232591e),
                             "yrom2.snd", CRC(51709c2f))
GTS80_ROMEND
#define input_ports_hlywoodh input_ports_gts80
CORE_GAMEDEFNV(hlywoodh, "Hollywood Heat",1986,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Genesis (#705)
/-------------------------------------------------------------------*/
INITGAME(genesis, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(genesis, "prom2.cpu", CRC(ac9f3a0f),
                            "prom1.cpu", CRC(4a2f185c))
GTS80BSSOUND888(            "drom1.snd", CRC(758e1743),
                            "yrom1.snd", CRC(4869b0ec),
                            "yrom2.snd", CRC(0528c024))
GTS80_ROMEND
#define input_ports_genesis input_ports_gts80
CORE_GAMEDEFNV(genesis, "Genesis",1986,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Gold Wings (#707)
/-------------------------------------------------------------------*/
INITGAME(goldwing, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(goldwing, "prom2.cpu", CRC(a5318c20),
                             "prom1.cpu", CRC(bf242185))
GTS80BSSOUND888(             "drom1.snd", CRC(892dbb21),
                             "yrom1.snd", CRC(e17e9b1f),
                             "yrom2.snd", CRC(4e482023))
GTS80_ROMEND
#define input_ports_goldwing input_ports_gts80
CORE_GAMEDEFNV(goldwing, "Gold Wings",1986,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Monte Carlo
/-------------------------------------------------------------------*/
INITGAME(mntecrlo, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(mntecrlo, "prom2.cpu", CRC(6860e315),
                             "prom1.cpu", CRC(0fbf15a3))
GTS80BSSOUND888(             "drom1.snd", CRC(1a53ac15),
                             "yrom1.snd", CRC(6e234c49),
                             "yrom2.snd", CRC(a95d1a6b))
GTS80_ROMEND
#define input_ports_mntecrlo input_ports_gts80
CORE_GAMEDEFNV(mntecrlo, "Monte Carlo",1987,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Spring Break
/-------------------------------------------------------------------*/
INITGAME(sprbreak, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(sprbreak, "prom2.cpu", CRC(47171062),
                             "prom1.cpu", CRC(53ed608b))
GTS80BSSOUND888(             "drom1.snd", NO_DUMP,
                             "yrom1.snd", NO_DUMP,
                             "yrom2.snd", NO_DUMP)
GTS80_ROMEND
#define input_ports_sprbreak input_ports_gts80
CORE_GAMEDEFNV(sprbreak, "Spring Break",1987,"Gottlieb",gl_mGTS80BS1,0)

/*-------------------------------------------------------------------
/ Arena
/-------------------------------------------------------------------*/
INITGAME(arena, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(arena, "prom2.cpu", CRC(4783b689),
                          "prom1.cpu", CRC(8c9f8ee9))
GTS80BSSOUND888(          "drom1.snd", CRC(78e6cbf1),
                          "yrom1.snd", CRC(f7a951c2),
                          "yrom2.snd", CRC(cc2aef4e))
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
GTS80B_2K_ROMSTART(victory, "prom2.cpu", CRC(6a42eaf4),
                            "prom1.cpu", CRC(e724db90))
GTS80BSSOUND3232(           "drom1.snd", CRC(4ab6dab7),
                            "yrom1.snd", CRC(921a100e))
GTS80_ROMEND
#define input_ports_victory input_ports_gts80
CORE_GAMEDEFNV(victory, "Victory",1987,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Diamond Lady
/-------------------------------------------------------------------*/
INITGAME(diamond, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(diamond, "prom2.cpu", CRC(862951dc),
                            "prom1.cpu", CRC(7a011757))
GTS80BSSOUND3232(           "drom1.snd", CRC(c216d1e4),
                            "yrom1.snd", CRC(0a18d626))
GTS80_ROMEND
#define input_ports_diamond input_ports_gts80
CORE_GAMEDEFNV(diamond, "Diamond Lady",1988,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ TX-Sector
/-------------------------------------------------------------------*/
INITGAME(txsector, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(txsector, "prom2.cpu", CRC(f12514e6),
                             "prom1.cpu", CRC(e51d39da))
GTS80BSSOUND3232(            "drom1.snd", CRC(61d66ca1),
                             "yrom1.snd", CRC(469ef444))
GTS80_ROMEND
#define input_ports_txsector input_ports_gts80
CORE_GAMEDEFNV(txsector, "TX-Sector",1988,"Gottlieb",gl_mGTS80BS2,0)

/*-------------------------------------------------------------------
/ Robo-War
/-------------------------------------------------------------------*/
INITGAME(robowars, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0)
GTS80B_2K_ROMSTART(robowars, "prom2.cpu", CRC(893177ed),
                             "prom1.cpu", CRC(cd1587d8))
GTS80BSSOUND3232(            "drom1.snd", CRC(ea59b6a1),
                             "yrom1.snd", CRC(7ecd8b67))
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
GTS80B_4K_ROMSTART(excalibr, "prom2.cpu", CRC(499e2e41),
                             "prom1.cpu", CRC(ed1083d7))
GTS80BSSOUND3232(            "drom1.snd", CRC(a4368cd0),
                             "yrom1.snd", CRC(9f194744))
GTS80_ROMEND
#define input_ports_excalibr input_ports_gts80
CORE_GAMEDEFNV(excalibr, "Excalibur",1988,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Bad Girls
/-------------------------------------------------------------------*/
INITGAME(badgirls, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(badgirls, "prom2.cpu", CRC(583933ec),
                             "prom1.cpu", CRC(956aeae0))
GTS80BSSOUND3232(            "drom1.snd", CRC(452dec20), //Should be labeled DROM!
                             "yrom1.snd", CRC(ab3b8e2d))
GTS80_ROMEND
#define input_ports_badgirls input_ports_gts80
CORE_GAMEDEFNV(badgirls, "Bad Girls",1988,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Big House
/-------------------------------------------------------------------*/
INITGAME(bighouse, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bighouse, "prom2.cpu", CRC(047c8ef5),
                             "prom1.cpu", CRC(0ecef900))
GTS80BSSOUND3232(            "drom1.snd", CRC(f330fd04),
                             "yrom1.snd", CRC(0b1ba1cb))
GTS80_ROMEND
#define input_ports_bighouse input_ports_gts80
CORE_GAMEDEFNV(bighouse, "Big House",1989,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Hot Shots
/-------------------------------------------------------------------*/
INITGAME(hotshots, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(hotshots, "prom2.cpu", CRC(7695c7db),
                             "prom1.cpu", CRC(122ff4a8))
GTS80BSSOUND3232(            "drom1.snd", CRC(42c3cc3d),
                             "yrom1.snd", CRC(2933a80e))
GTS80_ROMEND
#define input_ports_hotshots input_ports_gts80
CORE_GAMEDEFNV(hotshots, "Hot Shots",1989,"Gottlieb",gl_mGTS80BS3,0)

/*-------------------------------------------------------------------
/ Bone Busters (Why is there an extra drom2 listed in the rom file? Could this use different hardware?)
/-------------------------------------------------------------------*/
INITGAME(bonebstr, GEN_GTS80B, FLIP616, dispAlpha, SNDBRD_GTS80B,GTS80_DISPALPHA,0x80)
GTS80B_4K_ROMSTART(bonebstr, "prom2.cpu", CRC(681643df),
                             "prom1.cpu", CRC(052f97be))
GTS80BSSOUND3232(            "drom1.snd", CRC(ec43f4e9),
                             "yrom1.snd", CRC(a95eedfc))
GTS80_ROMEND
#define input_ports_bonebstr input_ports_gts80
CORE_GAMEDEFNV(bonebstr, "Bone Busters",1989,"Gottlieb",gl_mGTS80BS3,0)
