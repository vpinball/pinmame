#include "driver.h"
#include "core.h"
#include "by35.h"
#include "sndbrd.h"

static const core_tLCDLayout dispst6[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};
static const core_tLCDLayout dispst7[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG87}, {4,10,38,2,CORE_SEG87}, {0}
};

BY35_INPUT_PORTS_START(st,1) BY35_INPUT_PORTS_END

#define INITGAME(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*----------------------------------
/ Black Sheep Squadron (Astro game)
/---------------------------------*/
static const core_tLCDLayout dispBlkSheep[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 0,33,7,CORE_SEG87}, {0}
};
INITGAME(blkshpsq,GEN_ASTRO,dispBlkSheep,FLIP_SW(FLIP_L),0,SNDBRD_ASTRO,0)
ASTRO_ROMSTART88(blkshpsq,"cpu_u2.716",CRC(b9ac5204),
                          "cpu_u6.716",CRC(e16fbde1))
BY35_ROMEND
#define input_ports_blkshpsq input_ports_st
CORE_GAMEDEFNV(blkshpsq,"Black Sheep Squadron",1979,"Astro",by35_mAstro,GAME_NOT_WORKING|GAME_NO_SOUND)

/****************************************************/
/* STERN MPU-100 (almost identical to Bally MPU-17) */
/****************************************************/
/*--------------------------------
/ Stingray
/-------------------------------*/
INITGAME(stingray,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(stingray,"cpu_u2.716",CRC(1db32a33),
                          "cpu_u6.716",CRC(432e9b9e))
BY35_ROMEND
#define input_ports_stingray input_ports_st
CORE_GAMEDEFNV(stingray,"Stingray",1977,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Pinball
/-------------------------------*/
INITGAME(pinball,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(pinball,"cpu_u2.716",CRC(1db32a33),
                         "cpu_u6.716",CRC(432e9b9e))
BY35_ROMEND
#define input_ports_pinball input_ports_st
CORE_GAMEDEFNV(pinball,"Pinball",1977,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Stars
/-------------------------------*/
INITGAME(stars,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(stars, "cpu_u2.716",CRC(630d05df),
                        "cpu_u6.716",CRC(57e63d42))
BY35_ROMEND
#define input_ports_stars input_ports_st
CORE_GAMEDEFNV(stars,"Stars",1978,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Memory Lane
/-------------------------------*/
INITGAME(memlane,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(memlane, "cpu_u2.716",CRC(aff1859d),
                          "cpu_u6.716",CRC(3e236e3c))
BY35_ROMEND
#define input_ports_memlane input_ports_st
CORE_GAMEDEFNV(memlane,"Memory Lane",1978,"Stern",by35_mST100,GAME_USES_CHIMES)

/* ---------------------------------------------------*/
/* All games below used MPU-100 - Sound Board: SB-100 */
/* ---------------------------------------------------*/

/*--------------------------------
/ Lectronamo
/-------------------------------*/
INITGAME(lectrono,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(lectrono,"cpu_u2.716",CRC(79e918ff),
                          "cpu_u6.716",CRC(7c6e5fb5))
BY35_ROMEND
#define input_ports_lectrono input_ports_st
CORE_GAMEDEFNV(lectrono,"Lectronamo",1978,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Wildfyre
/-------------------------------*/
INITGAME(wildfyre,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(wildfyre,"cpu_u2.716",CRC(063f8b5e),
                          "cpu_u6.716",CRC(00336fbc))
BY35_ROMEND
#define input_ports_wildfyre input_ports_st
CORE_GAMEDEFNV(wildfyre,"Wildfyre",1978,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Nugent
/-------------------------------*/
INITGAME(nugent,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(nugent,"cpu_u2.716",CRC(79e918ff),
                        "cpu_u6.716",CRC(7c6e5fb5))
BY35_ROMEND
#define input_ports_nugent input_ports_st
CORE_GAMEDEFNV(nugent,"Nugent",1978,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Dracula
/-------------------------------*/
INITGAME(dracula,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(dracula,"cpu_u2.716",CRC(063f8b5e),
                         "cpu_u6.716",CRC(00336fbc))
BY35_ROMEND
#define input_ports_dracula input_ports_st
CORE_GAMEDEFNV(dracula,"Dracula",1979,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Trident - uses MPU-200 inports
/-------------------------------*/
INITGAME(trident,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(trident,"cpu_u2.716",CRC(934e49dd),
                         "cpu_u6.716",CRC(540bce56))
BY35_ROMEND
#define input_ports_trident input_ports_st
CORE_GAMEDEFNV(trident,"Trident",1979,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Hot Hand - uses MPU-200 inports
/-------------------------------*/
INITGAME(hothand,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(hothand,"cpu_u2.716",CRC(5e79ea2e),
                         "cpu_u6.716",CRC(fb955a6f))
BY35_ROMEND
#define input_ports_hothand input_ports_st
CORE_GAMEDEFNV(hothand,"Hot Hand",1979,"Stern",by35_mST100,GAME_NO_SOUND)

/*--------------------------------
/ Magic - uses MPU-200 inports
/-------------------------------*/
INITGAME(magic,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(magic,"cpu_u2.716",CRC(8838091f),
                       "cpu_u6.716",CRC(fb955a6f))
BY35_ROMEND
#define input_ports_magic input_ports_st
CORE_GAMEDEFNV(magic,"Magic",1979,"Stern",by35_mST100,GAME_NO_SOUND)

/*-------------------------------------
/ Cosmic Princess - same ROMs as Magic
/-------------------------------------*/
INITGAME(princess,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(princess,"cpu_u2.716",CRC(8838091f),
                          "cpu_u6.716",CRC(fb955a6f))
BY35_ROMEND
#define input_ports_princess input_ports_st
CORE_GAMEDEFNV(princess,"Cosmic Princess",1979,"Stern",by35_mST100,GAME_NO_SOUND)

/****************************************************/
/* STERN MPU-200 (almost identical to Bally MPU-35) */
/****************************************************/
/* ---------------------------------------------------*/
/* All games below used MPU-200 - Sound Board: SB-300 */
/* ---------------------------------------------------*/

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteor,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(meteor,"cpu_u1.716",CRC(e0fd8452),
                          "cpu_u5.716",CRC(43a46997),
                          "cpu_u2.716",CRC(fd396792),
                          "cpu_u6.716",CRC(03fa346c))
BY35_ROMEND
#define input_ports_meteor input_ports_st
CORE_GAMEDEFNV(meteor,"Meteor",1979,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Galaxy
/-------------------------------*/
INITGAME(galaxy,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(galaxy,"cpu_u1.716",CRC(35656b67),
                          "cpu_u5.716",CRC(12be0601),
                          "cpu_u2.716",CRC(08bdb285),
                          "cpu_u6.716",CRC(ad846a42))
BY35_ROMEND
#define input_ports_galaxy input_ports_st
CORE_GAMEDEFNV(galaxy,"Galaxy",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Ali
/-------------------------------*/
INITGAME(ali,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(ali,"cpu_u1.716",CRC(92e75b40),
                       "cpu_u5.716",CRC(119a4300),
                       "cpu_u2.716",CRC(9c91d08f),
                       "cpu_u6.716",CRC(7629db56))
BY35_ROMEND
#define input_ports_ali input_ports_st
CORE_GAMEDEFNV(ali,"Ali",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Big Game
/-------------------------------*/
INITGAME(biggame,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(biggame,"cpu_u1.716",CRC(f59c7514),
                           "cpu_u5.716",CRC(57df1dc5),
                           "cpu_u2.716",CRC(0251039b),
                           "cpu_u6.716",CRC(801e9a66))
BY35_ROMEND
#define input_ports_biggame input_ports_st
CORE_GAMEDEFNV(biggame,"Big Game",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Seawitch
/-------------------------------*/
INITGAME(seawitch,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(seawitch,"cpu_u1.716",CRC(c214140b),
                            "cpu_u5.716",CRC(ab2eab3a),
                            "cpu_u2.716",CRC(b8844174),
                            "cpu_u6.716",CRC(6c296d8f))
BY35_ROMEND
#define input_ports_seawitch input_ports_st
CORE_GAMEDEFNV(seawitch,"Seawitch",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Cheetah
/-------------------------------*/
INITGAME(cheetah,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(cheetah,"cpu_u1.716",CRC(6a845d94),
                           "cpu_u5.716",CRC(e7bdbe6c),
                           "cpu_u2.716",CRC(a827a1a1),
                           "cpu_u6.716",CRC(ed33c227))
BY35_ROMEND
#define input_ports_cheetah input_ports_st
CORE_GAMEDEFNV(cheetah,"Cheetah",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Quicksilver
/-------------------------------*/
INITGAME(quicksil,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(quicksil,"cpu_u1.716",CRC(fc1bd20a),
                            "cpu_u5.716",CRC(0bcaceb4),
                            "cpu_u2.716",CRC(8cb01165),
                            "cpu_u6.716",CRC(8c0e336a))
BY35_ROMEND
#define input_ports_quicksil input_ports_st
CORE_GAMEDEFNV(quicksil,"Quicksilver",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Nine Ball
/-------------------------------*/
INITGAME(nineball,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(nineball,"cpu_u1.716",CRC(fcb58f97),
                            "cpu_u5.716",CRC(c7c62161),
                            "cpu_u2.716",CRC(bdd7f258),
                            "cpu_u6.716",CRC(7e831499))
BY35_ROMEND
#define input_ports_nineball input_ports_st
CORE_GAMEDEFNV(nineball,"Nine Ball",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Free Fall
/-------------------------------*/
INITGAME(freefall,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(freefall,"cpu_u1.716",CRC(d13891ad),
                            "cpu_u5.716",CRC(77bc7759),
                            "cpu_u2.716",CRC(82bda054),
                            "cpu_u6.716",CRC(68168b97))
BY35_ROMEND
#define input_ports_freefall input_ports_st
CORE_GAMEDEFNV(freefall,"Free Fall",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Split Second
/-------------------------------*/
INITGAME(splitsec,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(splitsec,"cpu_u1.716",CRC(c6ff9aa9),
                            "cpu_u5.716",CRC(fda74efc),
                            "cpu_u2.716",CRC(81b9f784),
                            "cpu_u6.716",CRC(ecbedb0a))
BY35_ROMEND
#define input_ports_splitsec input_ports_st
CORE_GAMEDEFNV(splitsec,"Split Second",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Catacomb
/-------------------------------*/
INITGAME(catacomb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(catacomb,"cpu_u1.716",CRC(d445dd40),
                            "cpu_u5.716",CRC(d717a545),
                            "cpu_u2.716",CRC(bc504409),
                            "cpu_u6.716",CRC(da61b5a2))
BY35_ROMEND
#define input_ports_catacomb input_ports_st
CORE_GAMEDEFNV(catacomb,"Catacomb",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Iron Maiden
/-------------------------------*/
INITGAME(ironmaid,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(ironmaid,"cpu_u1.716",CRC(e15371a4),
                            "cpu_u5.716",CRC(84a29c01),
                            "cpu_u2.716",CRC(981ac0dd),
                            "cpu_u6.716",CRC(4e6f9c25))
BY35_ROMEND
#define input_ports_ironmaid input_ports_st
CORE_GAMEDEFNV(ironmaid,"Iron Maiden",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Viper
/-------------------------------*/
INITGAME(viper,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(viper, "cpu_u1.716",CRC(d0ea0aeb),
                          "cpu_u5.716",CRC(d26c7273),
                          "cpu_u2.716",CRC(d03f1612),
                          "cpu_u6.716",CRC(96ff5f60))
BY35_ROMEND
#define input_ports_viper input_ports_st
CORE_GAMEDEFNV(viper,"Viper",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Dragonfist
/-------------------------------*/
static core_tLCDLayout dispDragfist[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,36,2,CORE_SEG87}, {4,10,38,2,CORE_SEG87}, {4,20,34,2,CORE_SEG87},{0}
};
INITGAME(dragfist,GEN_STMPU200,dispDragfist,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(dragfist,"cpu_u1.716",CRC(4cbd1a38),
                            "cpu_u5.716",CRC(1783269a),
                            "cpu_u2.716",CRC(9ac8292b),
                            "cpu_u6.716",CRC(a374c8f9))
BY35_ROMEND
#define input_ports_dragfist input_ports_st
CORE_GAMEDEFNV(dragfist,"Dragonfist",1982,"Stern",by35_mST200,GAME_NO_SOUND)

/* -----------------------------------------------------------*/
/* All games below used MPU-200 - Sound Board: SB-300, VS-100 */
/* -----------------------------------------------------------*/

/*--------------------------------
/ Flight 2000
/-------------------------------*/
INITGAME(flight2k,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(flight2k,"cpu_u1.716",CRC(df9efed9),
                            "cpu_u5.716",CRC(38c13649),
                            "cpu_u2.716",CRC(425fae6a),
                            "cpu_u6.716",CRC(dc243186))
BY35_ROMEND
#define input_ports_flight2k input_ports_st
CORE_GAMEDEFNV(flight2k,"Flight 2000",1979,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Stargazer
/-------------------------------*/
INITGAME(stargzr,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(stargzr,"cpu_u1.716",CRC(83606fd4),
                           "cpu_u5.716",CRC(c54ae389)      ,
                           "cpu_u2.716",CRC(1a4c7dcb),
                           "cpu_u6.716",CRC(4e1f4dc6))
BY35_ROMEND
#define input_ports_stargzr input_ports_st
CORE_GAMEDEFNV(stargzr,"Stargazer",1980,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Lightning
/-------------------------------*/
static core_tLCDLayout dispLightnin[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,34,2,CORE_SEG87}, {4,10,38,2,CORE_SEG87}, {4,20,36,2,CORE_SEG87},{0}
};
INITGAME(lightnin,GEN_STMPU200,dispLightnin,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(lightnin,"cpu_u1.716",CRC(d3469d0a),
                            "cpu_u5.716",CRC(cd52262d),
                            "cpu_u2.716",CRC(e0933419),
                            "cpu_u6.716",CRC(df221c6b))
BY35_ROMEND
#define input_ports_lightnin input_ports_st
CORE_GAMEDEFNV(lightnin,"Lightning",1981,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Orbitor 1
/-------------------------------*/
static core_tLCDLayout dispOrbitor[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,36,2,CORE_SEG87}, {4,10,38,2,CORE_SEG87}, {4,20,34,2,CORE_SEG87},{0}
};
INITGAME(orbitor1,GEN_STMPU200,dispOrbitor,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(orbitor1, "cpu_u1.716",CRC(575520e3),
                             "cpu_u5.716",CRC(d31f27a8),
                             "cpu_u2.716",CRC(4421d827),
                             "cpu_u6.716",CRC(8861155a))
BY35_ROMEND
#define input_ports_orbitor1 input_ports_st
CORE_GAMEDEFNV(orbitor1,"Orbitor 1",1982,"Stern",by35_mST200,GAME_NO_SOUND)

/*--------------------------------
/ Cue (Proto - Never released)
/-------------------------------*/
INITGAME(cue,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(cue,"cpu_u1.716",NO_DUMP,
                       "cpu_u5.716",NO_DUMP,
                       "cpu_u2.716",NO_DUMP,
                       "cpu_u6.716",NO_DUMP)
BY35_ROMEND
#define input_ports_cue input_ports_st
CORE_GAMEDEFNV(cue,"Cue",1982,"Stern",by35_mST200,GAME_NO_SOUND)

/*----------------------------------------
/ Lazer Lord (Proto - Never released)
/---------------------------------------*/
INITGAME(lazrlord,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(lazrlord,"cpu_u1.716",CRC(32a6f341),
                            "cpu_u5.716",CRC(17583ba4),
                            "cpu_u2.716",CRC(669f3a8e),
                            "cpu_u6.716",CRC(395327a3))
BY35_ROMEND
#define input_ports_lazrlord input_ports_st
CORE_GAMEDEFNV(lazrlord,"Lazer Lord",1984,"Stern",by35_mST200,GAME_NO_SOUND)
