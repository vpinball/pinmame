#include "driver.h"
#include "core.h"
#include "by35.h"
#include "sndbrd.h"
#include "stsnd.h"

static const core_tLCDLayout dispst6[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};
static const core_tLCDLayout dispst7[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};

BY35_INPUT_PORTS_START(st,1) BY35_INPUT_PORTS_END

#define INITGAME(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*----------------------------------
/ Black Sheep Squadron (Astro game)
/---------------------------------*/
INITGAME(blkshpsq,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
ASTRO_ROMSTART88(blkshpsq,"cpu_u2.716",CRC(23d6cd54) SHA1(301ba10f3f333109630dd8abd13a6b4063f805a9),
                          "cpu_u6.716",CRC(ea68b9f7) SHA1(ebb69f4faadf457454939e47d8ae6e79eb0e1a11))
BY17_ROMEND
#define input_ports_blkshpsq input_ports_st
CORE_GAMEDEFNV(blkshpsq,"Black Sheep Squadron",1978,"Astro",by35_mST100,GAME_USES_CHIMES)

/*----------------------------------
/ Unknown game and manufacturer
/---------------------------------*/
static const core_tLCDLayout dispUnknown[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 0,33,7,CORE_SEG87}, {0}
};
INITGAME(st_game,GEN_ASTRO,dispUnknown,FLIP_SW(FLIP_L),0,SNDBRD_ASTRO,0)
ASTRO_ROMSTART88(st_game, "cpu_u2.716",CRC(b9ac5204) SHA1(1ac4e336eb62c091e61e9b6b21a858e70ac9ab38),
                          "cpu_u6.716",CRC(e16fbde1) SHA1(f7fe2f2ef9251792af1227f82dcc95239dd8baa1))
BY35_ROMEND
#define input_ports_st_game input_ports_st
CORE_GAMEDEFNV(st_game,"Unknown Game",198?,"Unknown Manufacturer",by35_mAstro,GAME_NOT_WORKING)

/*--------------------------------
/ Gamatron (Pinstar game, 1985)
/-------------------------------*/
INITGAME(gamatron,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
PS_ROMSTART8K(gamatron,"gamatron.764",CRC(fa9f7676) SHA1(8c56868eb6af7bb8ad73523ab6583100fcadc3c1))
BY35_ROMEND
#define input_ports_gamatron input_ports_st
CORE_CLONEDEFNV(gamatron,flight2k,"Gamatron",1985,"Pinstar",by35_mST200,0)

/****************************************************/
/* STERN MPU-100 (almost identical to Bally MPU-17) */
/****************************************************/
/*--------------------------------
/ Pinball
/-------------------------------*/
INITGAME(pinball,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(pinball,"cpu_u2.716",CRC(1db32a33) SHA1(2f0a3ca36968b81f29373e4f2cf7ee28a4071882),
                         "cpu_u6.716",CRC(432e9b9e) SHA1(292e509f50bc841f6e469c198fc82c2a9095f008))
BY35_ROMEND
#define input_ports_pinball input_ports_st
CORE_GAMEDEFNV(pinball,"Pinball",1977,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Stingray
/-------------------------------*/
INITGAME(stingray,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(stingray,"cpu_u2.716",CRC(1db32a33) SHA1(2f0a3ca36968b81f29373e4f2cf7ee28a4071882),
                          "cpu_u6.716",CRC(432e9b9e) SHA1(292e509f50bc841f6e469c198fc82c2a9095f008))
BY35_ROMEND
#define input_ports_stingray input_ports_st
CORE_GAMEDEFNV(stingray,"Stingray",1977,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Stars
/-------------------------------*/
INITGAME(stars,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(stars, "cpu_u2.716",CRC(630d05df) SHA1(2baa16265d524297332fa951d9eab3e0e8d26078),
                        "cpu_u6.716",CRC(57e63d42) SHA1(619ef955553654893c3071d8b70855fee8a5e6a7))
BY35_ROMEND
#define input_ports_stars input_ports_st
CORE_GAMEDEFNV(stars,"Stars",1978,"Stern",by35_mST100,GAME_USES_CHIMES)

/*--------------------------------
/ Memory Lane
/-------------------------------*/
INITGAME(memlane,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_NONE,0)
BY17_ROMSTARTx88(memlane, "cpu_u2.716",CRC(aff1859d) SHA1(5a9801d139bf2477b6d351a2654ae07516be144a),
                          "cpu_u6.716",CRC(3e236e3c) SHA1(7f631a5fac8a1b1af3b5332ba38d52553f13531a))
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
BY17_ROMSTARTx88(lectrono,"cpu_u2.716",CRC(79e918ff) SHA1(a728eb26d941a9c7484be593a216905237d32551),
                          "cpu_u6.716",CRC(7c6e5fb5) SHA1(3aa4e0c1f377ba024e6b34bd431a188ff02d4eaa))
BY35_ROMEND
#define input_ports_lectrono input_ports_st
CORE_GAMEDEFNV(lectrono,"Lectronamo",1978,"Stern",by35_mST100s,0)

/*--------------------------------
/ Wildfyre
/-------------------------------*/
INITGAME(wildfyre,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(wildfyre,"cpu_u2.716",CRC(063f8b5e) SHA1(80434de549102bff829b474603d6736b839b8999),
                          "cpu_u6.716",CRC(00336fbc) SHA1(d2c360b8a80b209ecf4ec02ee19a5234c0364504))
BY35_ROMEND
#define input_ports_wildfyre input_ports_st
CORE_GAMEDEFNV(wildfyre,"Wildfyre",1978,"Stern",by35_mST100s,0)

/*--------------------------------
/ Nugent
/-------------------------------*/
INITGAME(nugent,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(nugent,"cpu_u2.716",CRC(79e918ff) SHA1(a728eb26d941a9c7484be593a216905237d32551),
                        "cpu_u6.716",CRC(7c6e5fb5) SHA1(3aa4e0c1f377ba024e6b34bd431a188ff02d4eaa))
BY35_ROMEND
#define input_ports_nugent input_ports_st
CORE_GAMEDEFNV(nugent,"Nugent",1978,"Stern",by35_mST100s,0)

/*--------------------------------
/ Dracula
/-------------------------------*/
INITGAME(dracula,GEN_STMPU100,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100,0)
BY17_ROMSTARTx88(dracula,"cpu_u2.716",CRC(063f8b5e) SHA1(80434de549102bff829b474603d6736b839b8999),
                         "cpu_u6.716",CRC(00336fbc) SHA1(d2c360b8a80b209ecf4ec02ee19a5234c0364504))
BY35_ROMEND
#define input_ports_dracula input_ports_st
CORE_GAMEDEFNV(dracula,"Dracula",1979,"Stern",by35_mST100s,0)

/*--------------------------------
/ Trident - uses MPU-200 inports
/-------------------------------*/
INITGAME(trident,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(trident,"cpu_u2.716",CRC(934e49dd) SHA1(cbf6ca2759166f522f651825da0c75cf7248d3da),
                         "cpu_u6.716",CRC(540bce56) SHA1(0b21385501b83e448403e0216371487ed54026b7))
BY35_ROMEND
#define input_ports_trident input_ports_st
CORE_GAMEDEFNV(trident,"Trident",1979,"Stern",by35_mST100s,0)

/*--------------------------------
/ Hot Hand - uses MPU-200 inports
/-------------------------------*/
INITGAME(hothand,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(hothand,"cpu_u2.716",CRC(5e79ea2e) SHA1(9b45c59b2076fcb3a35de1dd3ba2444ea852f149),
                         "cpu_u6.716",CRC(fb955a6f) SHA1(387080d5af318463475797fecff026d6db776a0c))
BY35_ROMEND
#define input_ports_hothand input_ports_st
CORE_GAMEDEFNV(hothand,"Hot Hand",1979,"Stern",by35_mST100s,0)

/*--------------------------------
/ Magic - uses MPU-200 inports
/-------------------------------*/
INITGAME(magic,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(magic,"cpu_u2.716",CRC(8838091f) SHA1(d2702b5e15076793b4560c77b78eed6c1da571b6),
                       "cpu_u6.716",CRC(fb955a6f) SHA1(387080d5af318463475797fecff026d6db776a0c))
BY35_ROMEND
#define input_ports_magic input_ports_st
CORE_GAMEDEFNV(magic,"Magic",1979,"Stern",by35_mST100s,0)

/*-------------------------------------
/ Cosmic Princess - same ROMs as Magic
/-------------------------------------*/
INITGAME(princess,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST100B,0)
BY17_ROMSTARTx88(princess,"cpu_u2.716",CRC(8838091f) SHA1(d2702b5e15076793b4560c77b78eed6c1da571b6),
                          "cpu_u6.716",CRC(fb955a6f) SHA1(387080d5af318463475797fecff026d6db776a0c))
BY35_ROMEND
#define input_ports_princess input_ports_st
CORE_GAMEDEFNV(princess,"Cosmic Princess",1979,"Stern",by35_mST100s,0)
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
ST200_ROMSTART8888(meteor,"cpu_u1.716",CRC(e0fd8452) SHA1(a13215378a678e26a565742d81fdadd2e161ba7a),
                          "cpu_u5.716",CRC(43a46997) SHA1(2c74ca10cf9091db10542960f499f39f3da277ee),
                          "cpu_u2.716",CRC(fd396792) SHA1(b5d051a7ce7e7c2f9c4a0d900cef4f9ef2089476),
                          "cpu_u6.716",CRC(03fa346c) SHA1(51c04123cb433e90920c241e2d1f89db4643427b))
BY35_ROMEND
#define input_ports_meteor input_ports_st
CORE_GAMEDEFNV(meteor,"Meteor",1979,"Stern",by35_mST200,0)
/*--------------------------------
/ Meteor (7-digit conversion)
/-------------------------------*/
INITGAME(meteorb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(meteorb,"cpu_u1.716",CRC(e0fd8452) SHA1(a13215378a678e26a565742d81fdadd2e161ba7a),
                          "cpu_u5b.716",CRC(fe374449) SHA1(6ed39ae54a65a37d1d9bff52a12c5e9caee90cf1),
                          "cpu_u2b.716",CRC(62cd0484) SHA1(754bb6a7c3c6024b642dba4bc148ed110ab14295),
                          "cpu_u6b.716",CRC(10cb5d60) SHA1(1d3da195fbe06b49d08e4ce2ebc5d9d811126aa6))
BY35_ROMEND
#define input_ports_meteorb input_ports_meteor
CORE_CLONEDEFNV(meteorb,meteor,"Meteor (7-digit conversion)",2003,"Stern / Oliver",by35_mST200,0)

/*--------------------------------
/ Galaxy
/-------------------------------*/
INITGAME(galaxy,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(galaxy,"cpu_u1.716",CRC(35656b67) SHA1(e1ad9456c561d19220f8607576cb505588512179),
                          "cpu_u5.716",CRC(12be0601) SHA1(d651b834348c071dda660f37b4e359bf01cbd8d3),
                          "cpu_u2.716",CRC(08bdb285) SHA1(7984835ac151e5dac05628f3d5146d20e3623c38),
                          "cpu_u6.716",CRC(ad846a42) SHA1(303c9cb933ca60d35e12793a4ac0cf7ef11bc92e))
BY35_ROMEND
#define input_ports_galaxy input_ports_st
CORE_GAMEDEFNV(galaxy,"Galaxy",1980,"Stern",by35_mST200,0)
/*--------------------------------
/ Galaxy (7-digit conversion)
/-------------------------------*/
INITGAME(galaxyb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(galaxyb,"cpu_u1b.716",CRC(53f7c0c9) SHA1(c3ee8bbdd1eca7a044c7abf4e0ba6059f523c323),
                           "cpu_u5b.716",CRC(1b1cd31b) SHA1(65a6a58d2c509419fce3142a9ae88d8ea7d25f1c),
                           "cpu_u2b.716",CRC(f0b4e60b) SHA1(e1628ec94585fbf4935e824721472cc9c91bbf89),
                           "cpu_u6b.716",CRC(be4eacc1) SHA1(3d95e8e859312ef0a7ed52356dabe35ed0bebdef))
BY35_ROMEND
#define input_ports_galaxyb input_ports_galaxy
CORE_CLONEDEFNV(galaxyb,galaxy,"Galaxy (7-digit bootleg)",2004,"Stern / Oliver",by35_mST200,0)

/*--------------------------------
/ Ali
/-------------------------------*/
INITGAME(ali,GEN_STMPU200,dispst6,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(ali,"cpu_u1.716",CRC(92e75b40) SHA1(bace68db0ea12d50a546157d11084f3b00949136),
                       "cpu_u5.716",CRC(119a4300) SHA1(e913d9bd399b90502efe110c8bf7f23ae07df276),
                       "cpu_u2.716",CRC(9c91d08f) SHA1(a3e8c8e8c2c8b03d86b36eea8c84e5c0a27b8444),
                       "cpu_u6.716",CRC(7629db56) SHA1(f922d31ec4dd1755da0a24bec4e3fa3a7a9b22fc))
BY35_ROMEND
#define input_ports_ali input_ports_st
CORE_GAMEDEFNV(ali,"Ali",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Big Game
/-------------------------------*/
INITGAME(biggame,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(biggame,"cpu_u1.716",CRC(f59c7514) SHA1(49ab034a21e70956f63327aec4cbae115cd66a66),
                           "cpu_u5.716",CRC(57df1dc5) SHA1(283f45879b76d56ba0db0fb3d9d9771f91a70d02),
                           "cpu_u2.716",CRC(0251039b) SHA1(0a0e662788cf012dfb773d200c542a2a363748a8),
                           "cpu_u6.716",CRC(801e9a66) SHA1(8634d6bd4af3e5ec3b736679393462961b76ede1))
BY35_ROMEND
#define input_ports_biggame input_ports_st
CORE_GAMEDEFNV(biggame,"Big Game",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Seawitch
/-------------------------------*/
INITGAME(seawitch,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(seawitch,"cpu_u1.716",CRC(c214140b) SHA1(4d68ddd3b0f051c5f601ea5b9d5d5195d6017304),
                            "cpu_u5.716",CRC(ab2eab3a) SHA1(80a8c1ccd554be279720a26466bd6c59e1e56df0),
                            "cpu_u2.716",CRC(b8844174) SHA1(6e01321196fd6fce7b5526efc402044c87fe96a6),
                            "cpu_u6.716",CRC(6c296d8f) SHA1(8cdb77f382ef1214ef45579213cf8f19141366ad))
BY35_ROMEND
#define input_ports_seawitch input_ports_st
CORE_GAMEDEFNV(seawitch,"Seawitch",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Cheetah
/-------------------------------*/
INITGAME(cheetah,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(cheetah,"cpu_u1.716",CRC(6a845d94) SHA1(c272d5895edf2270f5f06fc33345bb4911abbee4),
                           "cpu_u5.716",CRC(e7bdbe6c) SHA1(8b213c2271dbd5157e0d34a33672130b935d76be),
                           "cpu_u2.716",CRC(a827a1a1) SHA1(723ebf193b5ce7b19df70e83caa9bb80f2e3fa66),
                           "cpu_u6.716",CRC(ed33c227) SHA1(a96ba2814cef7663728bb5fdea2dc6ecfa219038))
BY35_ROMEND
#define input_ports_cheetah input_ports_st
CORE_GAMEDEFNV(cheetah,"Cheetah",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Quicksilver
/-------------------------------*/
INITGAME(quicksil,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(quicksil,"cpu_u1.716",CRC(fc1bd20a) SHA1(e3c547f996dfc5d1567223d234443cf31d648ef6),
                            "cpu_u5.716",CRC(0bcaceb4) SHA1(461d2fe5772a5ac84d31a4a186b9f639c683ca8a),
                            "cpu_u2.716",CRC(8cb01165) SHA1(b42e2ccce2c20ad570cdcdb63c9d12e414f9b255),
                            "cpu_u6.716",CRC(8c0e336a) SHA1(8d3a5b7c07d03c7e2945ea60c72f9181d3ee2a14))
BY35_ROMEND
#define input_ports_quicksil input_ports_st
CORE_GAMEDEFNV(quicksil,"Quicksilver",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Stargazer
/-------------------------------*/
INITGAME(stargzr,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(stargzr,"cpu_u1.716",CRC(83606fd4) SHA1(7f6448bc0dabe50de40fd47a7242c1be4a93e84d),
                           "cpu_u5.716",CRC(c54ae389)       SHA1(062e64e8ced723adb7f4040539ba6400fc4a9c9a),
                           "cpu_u2.716",CRC(1a4c7dcb) SHA1(54888a8867b8d60f215b7e683ae4966f14ddca15),
                           "cpu_u6.716",CRC(4e1f4dc6) SHA1(1f63a0b71af84fb6e1168ff77cbcbabcaa1323f3))
BY35_ROMEND
#define input_ports_stargzr input_ports_st
CORE_GAMEDEFNV(stargzr,"Stargazer",1980,"Stern",by35_mST200,0)

/*--------------------------------
/ Stargazer (modified rules rev .9)
/-------------------------------*/
INITGAME(stargzrb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(stargzrb,"cpu_u1.716",CRC(83606fd4) SHA1(7f6448bc0dabe50de40fd47a7242c1be4a93e84d),
                           "cpu_u5b.716",CRC(29682d85) SHA1(3f449270cd4098a7ed1ee9c0d801110b1b653913),
                           "cpu_u2b.716",CRC(360427cc) SHA1(ad76124b7fd088a5e2d24cf369c1620cdcc80309),
                           "cpu_u6b.716",CRC(b68b11c5) SHA1(1af6aca8ecf70d2adf588a1e856f753193c05abd))
BY35_ROMEND
#define input_ports_stargzrb input_ports_stargzr
CORE_CLONEDEFNV(stargzrb,stargzr,"Stargazer (modified rules rev.9)",2006,"Stern / Oliver",by35_mST200,0)

/*--------------------------------
/ Flight 2000
/-------------------------------*/
INITGAME(flight2k,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(flight2k,"cpu_u1.716",CRC(df9efed9) SHA1(47727664e745e77ca1c221a32bd56d936f5b31bc),
                            "cpu_u5.716",CRC(38c13649) SHA1(bcdbd17b48edd41ec7d38261595ac06eb8fc6a4d),
                            "cpu_u2.716",CRC(425fae6a) SHA1(fde8d23e6ebb176ba72f763d66c2e17e51237fa1),
                            "cpu_u6.716",CRC(dc243186) SHA1(046ce51b8a8218214088c4264548c753bd880e19))
VSU100_SOUNDROM_U9(         "snd_u9.716",CRC(d816573c) SHA1(75134a017c34abbb149159ca001d35464a3f5128))
BY35_ROMEND
#define input_ports_flight2k input_ports_st
CORE_GAMEDEFNV(flight2k,"Flight 2000",1980,"Stern",by35_mST200v,0)

/*--------------------------------
/ Nine Ball
/-------------------------------*/
INITGAME(nineball,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(nineball,"cpu_u1.716",CRC(fcb58f97) SHA1(6510a6d0b466bd27ade50992260cea716d79fda2),
                            "cpu_u5.716",CRC(c7c62161) SHA1(624eab2fdf7bafbf4af012df521bd09f9b2da8d8),
                            "cpu_u2.716",CRC(bdd7f258) SHA1(2a38de09827100cbbd4e79be50aad03a3f2b63b4),
                            "cpu_u6.716",CRC(7e831499) SHA1(8d3c148b91c21938b1b5fca85ecd8f6d7f1e76b0))
BY35_ROMEND
#define input_ports_nineball input_ports_st
CORE_GAMEDEFNV(nineball,"Nine Ball",1980,"Stern",by35_mST200,0)

INITGAME(ninebalb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ROM_START(ninebalb)
  NORMALREGION(0x10000, BY35_CPUREGION)
    ROM_LOAD("nineball.256", 0x0000, 0x8000, CRC(06cb8a63) SHA1(c901bba0b41b45c5cfa6d04181f1e035beab5a08))
    ROM_RELOAD(0x8000, 0x8000)
ST200_ROMEND
#define input_ports_ninebalb input_ports_st
CORE_CLONEDEFNV(ninebalb,nineball,"Nine Ball (modified rules rev. 85)",2007,"Stern / Oliver",by35_mST200,0)

/*--------------------------------
/ Free Fall
/-------------------------------*/
INITGAME(freefall,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(freefall,"cpu_u1.716",CRC(d13891ad) SHA1(afb40c51f2d5695c74ce9979c0a818845f95edd4),
                            "cpu_u5.716",CRC(77bc7759) SHA1(3f739757180b3dcce5426935a51e4b615f157199),
                            "cpu_u2.716",CRC(82bda054) SHA1(32772e878d2a4bba8f67e419a68a81fec2a5f6d7),
                            "cpu_u6.716",CRC(68168b97) SHA1(defa4bba465182db22debddb4070c40c048c95e2))
VSU100_SOUNDROM_U9U10(      "snd_u9.716",CRC(ea8cf062) SHA1(55c840a9bea363fd436c00a115cb61d15a9f8c47),
                           "snd_u10.716",CRC(dd681a79) SHA1(d954cae375fb0145e10536e43d1cb03902de2ea3))
BY35_ROMEND
#define input_ports_freefall input_ports_st
CORE_GAMEDEFNV(freefall,"Free Fall",1981,"Stern",by35_mST200v,0)

/*--------------------------------
/ Lightning
/-------------------------------*/
static core_tLCDLayout dispLightnin[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,34,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,36,2,CORE_SEG7},{0}
};
INITGAME(lightnin,GEN_STMPU200,dispLightnin,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(lightnin,"cpu_u1.716",CRC(d3469d0a) SHA1(18565f5c85694da8eaf850146d3d9a90a17b7816),
                            "cpu_u5.716",CRC(cd52262d) SHA1(099aeda2183822046cce907b265b42319007ac32),
                            "cpu_u2.716",CRC(e0933419) SHA1(1f7cad915496f34473dffde7e320d51838acd0fd),
                            "cpu_u6.716",CRC(df221c6b) SHA1(5935020d3a24d829fbeaa8cf764daff48a151a81))
VSU100_SOUNDROM_U9U10(      "snd_u9.716",CRC(00ffa77c) SHA1(242efd800731a7f84369c6ce54298d0a227dd8ba),
                           "snd_u10.716",CRC(80fe9158) SHA1(20fcdb4c09b25e494f02bbfb20c07ff2870d5798))
BY35_ROMEND
#define input_ports_lightnin input_ports_st
CORE_GAMEDEFNV(lightnin,"Lightning",1981,"Stern",by35_mST200v,0)

/*--------------------------------
/ Split Second
/-------------------------------*/
INITGAME(splitsec,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(splitsec,"cpu_u1.716",CRC(c6ff9aa9) SHA1(39f80faca16c869ac14df7c5fc3dfa80b47dad95),
                            "cpu_u5.716",CRC(fda74efc) SHA1(31becc243ada23e2f4d17927985772c9fcf8a3c3),
                            "cpu_u2.716",CRC(81b9f784) SHA1(43cf71b51eda70a3c126340ea658c03c438e4f18),
                            "cpu_u6.716",CRC(ecbedb0a) SHA1(8cc7281dd2bd300ab95a08761c12733d98599ebd))
VSU100_SOUNDROM_U9U10(      "snd_u9.716",CRC(e6ed5f48) SHA1(ea2bbc607acb2b816667cd54f3d07605110c252e),
                           "snd_u10.716",CRC(36e6ee70) SHA1(61bd89d69627bea89b7f31af63ff90ace6db3c85))
BY35_ROMEND
#define input_ports_splitsec input_ports_st
CORE_GAMEDEFNV(splitsec,"Split Second",1981,"Stern",by35_mST200v,0)

/*--------------------------------
/ Catacomb
/-------------------------------*/
INITGAME(catacomb,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(catacomb,"cpu_u1.716",CRC(d445dd40) SHA1(9ff5896977d7e2a0cf788c77dcfd7c010e17d2fb),
                            "cpu_u5.716",CRC(d717a545) SHA1(a183f3b1f766c3a82ae52defc38d84328fb7b31a),
                            "cpu_u2.716",CRC(bc504409) SHA1(cd3e948d34a8db71fc841261e683988c9df31ef8),
                            "cpu_u6.716",CRC(da61b5a2) SHA1(ec4a914cd57b37921578699bc427f12a3670c7eb))
VSU100_SOUNDROM_U9U10(      "snd_u9.716",CRC(a13cb591) SHA1(b64a2dc3429803095dc05cdd1718db2404b13eb8),
                           "snd_u10.716",CRC(2b31f8be) SHA1(05b394bd8b6c04e34fe2bab19cbd0f06d9e4b90d))
BY35_ROMEND
#define input_ports_catacomb input_ports_st
CORE_GAMEDEFNV(catacomb,"Catacomb",1981,"Stern",by35_mST200v,0)

/*--------------------------------
/ Iron Maiden
/-------------------------------*/
INITGAME(ironmaid,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(ironmaid,"cpu_u1.716",CRC(e15371a4) SHA1(fe441ed8abd325190d8eee6d907e17c7fc02be64),
                            "cpu_u5.716",CRC(84a29c01) SHA1(0e0ff8821c7028ce690328cd08a77bb51c0993c9),
                            "cpu_u2.716",CRC(981ac0dd) SHA1(c585907b74695812f333867cf359a01a5ea6ed81),
                            "cpu_u6.716",CRC(4e6f9c25) SHA1(9053e1d335a29f7acade7752adffe69f42032959))
BY35_ROMEND
#define input_ports_ironmaid input_ports_st
CORE_GAMEDEFNV(ironmaid,"Iron Maiden",1981,"Stern",by35_mST200,0)

/*--------------------------------
/ Viper
/-------------------------------*/
INITGAME(viper,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(viper, "cpu_u1.716",CRC(d0ea0aeb) SHA1(28f4df9f45807abd1528aa6e5a80933156e6d692),
                          "cpu_u5.716",CRC(d26c7273) SHA1(303c18861941463932fdf47e9606159936b28dc1),
                          "cpu_u2.716",CRC(d03f1612) SHA1(d390ec1e953148ac26bf218701117855c941fc65),
                          "cpu_u6.716",CRC(96ff5f60) SHA1(a9df887ca338db208a684540f6c9fc07722c3aa5))
BY35_ROMEND
#define input_ports_viper input_ports_st
CORE_GAMEDEFNV(viper,"Viper",1981,"Stern",by35_mST200,0)

/*--------------------------------
/ Dragonfist
/-------------------------------*/
static core_tLCDLayout dispDragfist[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,36,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,34,2,CORE_SEG7},{0}
};
INITGAME(dragfist,GEN_STMPU200,dispDragfist,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(dragfist,"cpu_u1.716",CRC(4cbd1a38) SHA1(73b7291f38cd0a3300107605db26d474ecfc3101),
                            "cpu_u5.716",CRC(1783269a) SHA1(75151b79844d26d9e8ecf00dec96643ee2fedc5b),
                            "cpu_u2.716",CRC(9ac8292b) SHA1(99ad3ad6e1d1b19695ce1b5b76f6bd85c9c6530d),
                            "cpu_u6.716",CRC(a374c8f9) SHA1(481116025a52353f298f3d93dfe33b3ad9f86d18))
BY35_ROMEND
#define input_ports_dragfist input_ports_st
CORE_GAMEDEFNV(dragfist,"Dragonfist",1982,"Stern",by35_mST200,0)

/*--------------------------------
/ Orbitor 1
/-------------------------------*/
static core_tLCDLayout dispOrbitor[] = {
  {0, 0, 1,7,CORE_SEG87F},{0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F},{2,16,25,7,CORE_SEG87F},
  {4, 4,36,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,33,3,CORE_SEG7},{0}
};
INITGAME(orbitor1,GEN_STMPU200,dispOrbitor,FLIP_SW(FLIP_L),0,SNDBRD_ST300V,0)
ST200_ROMSTART8888(orbitor1,"cpu_u1.716",CRC(575520e3) SHA1(9d52b065a14d4f95cebd48f60f628f2c246385fa),
                            "cpu_u5.716",CRC(d31f27a8) SHA1(0442260db42192a95f6292e6b57000c127871d28),
                            "cpu_u2.716",CRC(4421d827) SHA1(9b617215f2d92ef2c69104eb4e63a924704665aa),
                            "cpu_u6.716",CRC(8861155a) SHA1(81a1b3434d4f80dee5704454f8359200faea173d))
VSU100_SOUNDROM_U9U10(      "snd_u9.716",CRC(2ba24569) SHA1(da2f4a4eeed9ae7ff8a342f4d630e12dcb2decf5),
                           "snd_u10.716",CRC(8e5b4a38) SHA1(de3f59363553f5f0d6098401734436930e64fbbd))
BY35_ROMEND
#define input_ports_orbitor1 input_ports_st
CORE_GAMEDEFNV(orbitor1,"Orbitor 1",1982,"Stern",by35_mST200v,0)

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
CORE_GAMEDEFNV(cue,"Cue",1982,"Stern",by35_mST200,0)

/*----------------------------------------
/ Lazer Lord (Proto - Never released)
/---------------------------------------*/
INITGAME(lazrlord,GEN_STMPU200,dispst7,FLIP_SW(FLIP_L),0,SNDBRD_ST300,0)
ST200_ROMSTART8888(lazrlord,"cpu_u1.716",CRC(32a6f341) SHA1(75922c6831463d240fe057a0f72280d417899fa4),
                            "cpu_u5.716",CRC(17583ba4) SHA1(4807e3ab18c2e40a292b499fe038975bb4b9fc17),
                            "cpu_u2.716",CRC(669f3a8e) SHA1(4beb0e4c75f4e3c1788808b57081612d4774d130),
                            "cpu_u6.716",CRC(395327a3) SHA1(e2a3a8ea696bcc4b5e11b08b6c7a6d9a991aa4af))
BY35_ROMEND
#define input_ports_lazrlord input_ports_st
CORE_GAMEDEFNV(lazrlord,"Lazer Lord",1984,"Stern",by35_mST200,0)
