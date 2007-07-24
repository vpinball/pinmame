#include "driver.h"
#include "sim.h"
#include "gp.h"
#include "gpsnd.h"
#include "sndbrd.h"

//DDU-1/2: 2 X (2 x 7 Segment, 6 Digit Displays, 2 x 2 Digit 7 Segment)
//BDU-1:   4 X 7 Segment, 6 Digit Displays, 2 x 2 Digit 7 Segment
//BDU-2:   4 X 7 Segment, 7 Digit Displays, 2 x 2 Digit 7 Segment

#if 0
//Use for testing segments only
static core_tLCDLayout dispGP_TEST[] = {
 {0, 0, 00,16,CORE_SEG87},
 {2, 0, 16,16,CORE_SEG87},
 {4, 0, 32,16,CORE_SEG87}, {0}
};
#endif

/*Ordering for DDU-1:
  --------
  Player 1/2 (or 3/4)
  (06)(05)(04)(03)(02)(01)
							(00)
  (06)(05)(04)(03)(02)(01)

  For balls/credits - left side displays [bp], right side [cc]
	b = which ball is in play, p = # of players in game, combined for match
	cc = # of credits, up to 99
*/
static core_tLCDLayout dispGP_DDU1[] = {
 {0, 0, 1,6,CORE_SEG7}, {0,16, 9,6,CORE_SEG7},
 {4, 0,17,6,CORE_SEG7}, {4,16,25,6,CORE_SEG7},
 {2, 4, 8,1,CORE_SEG7}, {2, 6, 0,1,CORE_SEG7},
 {2,20,24,1,CORE_SEG7}, {2,22,16,1,CORE_SEG7}, {0}
};

/* DDU-2 is the same as DDU-1, but all starting panels shifted by 8 */
static core_tLCDLayout dispGP_DDU2[] = {
 {0, 0, 9,6,CORE_SEG7},{0,16,17,6,CORE_SEG7},
 {4, 0,25,6,CORE_SEG7},{4,16,33,6,CORE_SEG7},
 {2, 4,16,1,CORE_SEG7},{2, 6, 8,1,CORE_SEG7},
 {2,20,32,1,CORE_SEG7},{2,22,24,1,CORE_SEG7}, {0}
};

/*BDU-1*/
static core_tLCDLayout dispGP_BDU1[] = {
 {0, 0, 1,6,CORE_SEG7}, {0,24, 9,6,CORE_SEG7},
 {4, 0,17,6,CORE_SEG7}, {4,24,25,6,CORE_SEG7},
 {4,13,33,2,CORE_SEG7}, {4,19,37,2,CORE_SEG7}, {0}
};

/*BDU-2*/
//NOTE: 7th Digit order comes after digits 0-6, so we tack it onto front!
static core_tLCDLayout dispGP_BDU2[] = {
 {0, 0, 0,7,CORE_SEG7}, {0,22, 8,7,CORE_SEG7},
 {3, 0,16,7,CORE_SEG7}, {3,22,24,7,CORE_SEG7},
 {5,19,33,2,CORE_SEG7S},{5,25,37,2,CORE_SEG7S},{0}
};

#define INITGAME(name, gen, disp, flip, sb, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GP_INPUT_PORTS_START(name, 1) GP_INPUT_PORTS_END

#define INIT_110(name) \
INITGAME(name, 0,dispGP_DDU1,FLIP_SW(FLIP_L),0,-2) \
GP_ROMSTART88(name, "a-110.u12", CRC(ed0d518b) SHA1(8f3ca8792ad907c660d9149a1aa3a3528c7573e3), \
                    "b1-110.u13",CRC(a223f2e8) SHA1(767e15e19e11399935c890c1d1034dccf1ad7f92)) \
GP_ROMEND

INIT_110(gp_110) GAMEX(1978,gp_110,0,GP1,gp_110,gp_110,ROT0,"Game Plan","Model 110",NOT_A_DRIVER)

//Games in rough production order

/*-------------------------------------------------------------------
/ Foxy Lady (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(foxylady) CORE_CLONEDEFNV(foxylady,gp_110,"Foxy Lady",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Black Velvet (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(blvelvet) CORE_CLONEDEFNV(blvelvet,gp_110,"Black Velvet",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Camel Lights (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(camlight) CORE_CLONEDEFNV(camlight,gp_110,"Camel Lights",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Real (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(real)     CORE_CLONEDEFNV(real,gp_110,"Real",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Rio (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(rio)      CORE_CLONEDEFNV(rio,gp_110,"Rio",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Chuck-A-Luck (October 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INIT_110(chucklck) CORE_CLONEDEFNV(chucklck,gp_110,"Chuck-A-Luck",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Star Trip (April 1979) - Model: Cocktail #120
/-------------------------------------------------------------------*/
INITGAME(startrip, 0,dispGP_DDU1,FLIP_SW(FLIP_L),SNDBRD_GPSSU1,-2)
GP_ROMSTART88(startrip,	"startrip.u12",CRC(98f27fdf) SHA1(8bcff1e13b9b978f91110f1e83a3288723930a1d),
						"startrip.u13",CRC(b941a1a8) SHA1(a43f8acadb3db3e2274162d5305e30006f912339))
GP_ROMEND
CORE_GAMEDEFNV(startrip,"Star Trip",1979,"Game Plan",mGP1S1,0)

/*-------------------------------------------------------------------
/ Family Fun! (April 1979) - Model: Cocktail #120
/-------------------------------------------------------------------*/
INITGAME(famlyfun, 0,dispGP_DDU1,FLIP_SW(FLIP_L),SNDBRD_GPSSU1,-2)
GP_ROMSTART88(famlyfun,	"family.u12",CRC(98f27fdf) SHA1(8bcff1e13b9b978f91110f1e83a3288723930a1d),
						"family.u13",CRC(b941a1a8) SHA1(a43f8acadb3db3e2274162d5305e30006f912339))
GP_ROMEND
CORE_GAMEDEFNV(famlyfun,"Family Fun!",1979,"Game Plan",mGP1S1,0)

/***************************************************************************
 *Games below are regular standup pinball games (except where noted)
 *                   -all games below use MPU-2!
 ***************************************************************************/

/*-------------------------------------------------------------------
/ Sharpshooter (May 1979) - Model #130
/-------------------------------------------------------------------*/
INITGAME(sshooter, 0,dispGP_BDU1,FLIP_SW(FLIP_L),SNDBRD_GPSSU2,0)
GP_ROMSTART888(sshooter,"130a.716",CRC(dc402b37) SHA1(90c46391a1e5f000f3b235d580463bf96b45bd3e),
						"130b.716",CRC(19a86f5e) SHA1(bc4a87314fc9c4e74e492c3f6e44d5d6cae72939),
						"130c.716",CRC(b956f67b) SHA1(ff64383d7f59e9bbec588553e35a21fb94c7203b))
GP_ROMEND
CORE_GAMEDEFNV(sshooter,"Sharpshooter",1979,"Game Plan",mGP2S2,0)

/*-------------------------------------------------------------------
/ Vegas (August 1979) - Cocktail Model #140
/-------------------------------------------------------------------*/
INITGAME(vegasgp, 0,dispGP_DDU2,FLIP_SW(FLIP_L),SNDBRD_GPSSU1,-2)
GP_ROMSTART88(vegasgp, "140a.12",CRC(2c00bc19) SHA1(521d4b44f46dea0a08e90cd3aea5799462215863),
					   "140b.13",CRC(cf26d67b) SHA1(05481e880e23a7bc1d1716b52ac1effc0db437f2))
GP_ROMEND
CORE_GAMEDEFNV(vegasgp,"Vegas (Game Plan)",1979,"Game Plan",mGP2S1,0)

/*-------------------------------------------------------------------
/ Coney Island! (December 1979) - Model #180
/-------------------------------------------------------------------*/
INITGAME(coneyis, 0,dispGP_BDU1,FLIP_SW(FLIP_L),SNDBRD_GPSSU3,0)
GP_ROMSTART888(coneyis,	"130a.716",CRC(dc402b37) SHA1(90c46391a1e5f000f3b235d580463bf96b45bd3e),
						"130b.716",CRC(19a86f5e) SHA1(bc4a87314fc9c4e74e492c3f6e44d5d6cae72939),
						"130c.716",CRC(b956f67b) SHA1(ff64383d7f59e9bbec588553e35a21fb94c7203b))
GP_ROMEND
CORE_GAMEDEFNV(coneyis,"Coney Island!",1979,"Game Plan",mGP2S2,0)

//Challenger I (?? / 1980)

/*-------------------------------------------------------------------
/ (Pinball) Lizard (June / July 1980) - Model #210
/-------------------------------------------------------------------*/
INITGAME(lizard, 0,dispGP_BDU1,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(lizard,	"130a.716",CRC(dc402b37) SHA1(90c46391a1e5f000f3b235d580463bf96b45bd3e),
						"130b.716",CRC(19a86f5e) SHA1(bc4a87314fc9c4e74e492c3f6e44d5d6cae72939),
						"130c.716",CRC(b956f67b) SHA1(ff64383d7f59e9bbec588553e35a21fb94c7203b))
GP_SOUNDROM88("lizard.u9", CRC(2d121b24) SHA1(55c16951538229571165c35a353da53e22d11f81),
              "lizard.u10",CRC(28b8f1f0) SHA1(db6d816366e0bca59376f6f8bf87e6a2d849aa72))
GP_ROMEND
CORE_GAMEDEFNV(lizard,"(Pinball) Lizard",1980,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Global Warfare (June 1981)  - Model #240
/-------------------------------------------------------------------*/
INITGAME(gwarfare, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(gwarfare,"240a.716",CRC(30206428) SHA1(7a9029e4fd4c4c00da3256ed06464c0bd8022168),
						"240b.716",CRC(a54eb15d) SHA1(b9235bd188c1251eb213789800b7686b5e3c557f),
						"240c.716",CRC(60d115a8) SHA1(e970fdd7cbbb2c81ab8c8209edfb681798c683b9))
GP_SOUNDROM88("gw240bot.rom", CRC(3245a206) SHA1(b321b2d276fbd74199eff2d8c0d1b8a2f5c93604),
              "gw240top.rom",CRC(faaf3de1) SHA1(9c984d1ac696eb16f7bf35463a69a470344314a7))
GP_ROMEND
CORE_GAMEDEFNV(gwarfare,"Global Warfare",1981,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Mike Bossy (January 1982) - Model #???
/-------------------------------------------------------------------*/
INITGAME(mbossy, 0,dispGP_BDU1,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(mbossy,	"mb_a.716",NO_DUMP,
						"mb_b.716",NO_DUMP,
						"mb_c.716",NO_DUMP)
GP_SOUNDROM88("mb.u9", CRC(dfa98db5) SHA1(65361630f530383e67837c428050bcdb15373c0b),
              "mb.u10",CRC(2d3c91f9) SHA1(7e1f067af29d9e484da234382d7dc821ca07b6c4))
GP_ROMEND
CORE_GAMEDEFNV(mbossy,"Mike Bossy",1982,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Super Nova (May 1982) - Model #150
/-------------------------------------------------------------------*/
//Flyer suggests 6 digits for scoring??
INITGAME(suprnova, 0,dispGP_BDU1,FLIP_SW(FLIP_L),SNDBRD_GPSSU4,0)
GP_ROMSTART888(suprnova,"130a.716",CRC(dc402b37) SHA1(90c46391a1e5f000f3b235d580463bf96b45bd3e),
						"150b.716",CRC(8980a8bb) SHA1(129816fe85681b760307a713c667737a750b0c04),
						"150c.716",CRC(6fe08f96) SHA1(1309619a2400674fa1d05dc9214fdb85419fd1c3))
GP_ROMEND
CORE_GAMEDEFNV(suprnova,"Super Nova",1982,"Game Plan",mGP2S4,0)

/*-------------------------------------------------------------------
/ Sharp Shooter II (November 1983) - Model #730
/-------------------------------------------------------------------*/
INITGAME(sshootr2, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(sshootr2,"130a.716",CRC(dc402b37) SHA1(90c46391a1e5f000f3b235d580463bf96b45bd3e),
						"130b.716",CRC(19a86f5e) SHA1(bc4a87314fc9c4e74e492c3f6e44d5d6cae72939),
						"730c",CRC(d1af712b) SHA1(9dce2ec1c2d9630a29dd21f4685c09019e59b147))
GP_SOUNDROM88("730u9.snd", CRC(dfa98db5) SHA1(65361630f530383e67837c428050bcdb15373c0b),
              "730u10.snd",CRC(6d3dcf44) SHA1(3703313d4172ebfec1dcacca949076541ee35cb7))
GP_ROMEND
CORE_GAMEDEFNV(sshootr2,"Sharp Shooter II",1983,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Attila the Hun (April 1984) - Model #260
/-------------------------------------------------------------------*/
INITGAME(attila, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(attila,	"260.a",CRC(b31c11d8) SHA1(d3f2ad84cc28e99acb54349b232dbf8abdf15b21),
						"260.b",CRC(e8cca86d) SHA1(ed0797175a573537be2d5119ad68b1847e49e578),
						"260.c",CRC(206605c3) SHA1(14f61a2f43c29370bcb6db29969e8dfcfe3da1ab))
GP_SOUNDROM0("260.snd",CRC(21e6b188) SHA1(84148942e6007d49bb4085ec3678954d48e4439e))
GP_ROMEND
CORE_GAMEDEFNV(attila,"Attila The Hun",1984,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Agents 777 (November 1984) - Model #770
/-------------------------------------------------------------------*/
INITGAME(agent777, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(agent777,"770a",CRC(fc4eebcd) SHA1(742a201e89c1357d2a1f24b0acf3b78ffec96c74),
						"770b",CRC(ea62aece) SHA1(32be10bc76a59e03c3fd3294daefc8d28c20386a),
						"770c",CRC(59280db7) SHA1(8f199be7bfbc01466541c07dc4c365e20055a66c))
GP_SOUNDROM0("770snd",CRC(e4e66c9f) SHA1(f373facefb18c64377da47308a8bbd5fc80e9c2d))
GP_ROMEND
CORE_GAMEDEFNV(agent777,"Agents 777",1984,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Captain Hook (April 1985) - Model #780
/-------------------------------------------------------------------*/
INITGAME(cpthook, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(cpthook,	"780.a",CRC(6bd5a495) SHA1(8462e0c68176daee6b23dce9091f5aee99e62631),
						"780.b",CRC(3d1c5555) SHA1(ecb0d40f5e6e37acfc8589816e24b26525273393),
						"780.c",CRC(e54bc51f) SHA1(3480e0cdd43f9ac3fda8cd466b2f039210525e8b))
GP_SOUNDROM0("780.snd",CRC(95af3392) SHA1(73a2b583b7fc423c2e4390667aebc90ad41f4f93))
GP_ROMEND
CORE_GAMEDEFNV(cpthook,"Captain Hook",1985,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Lady Sharpshooter (May 1985) - Cocktail Model #830
/-------------------------------------------------------------------*/
INITGAME(ladyshot, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU1,0)
GP_ROMSTART888(ladyshot,"830a.716",CRC(c055b993) SHA1(a9a7156e5ec0a32db1ffe36b3c6280953a2606ff),
						"830b.716",CRC(1e3308ea) SHA1(a5955a6a15b33c4cf35105ab524a8e7e03d748b6),
						"830c.716",CRC(f5e1db15) SHA1(e8168ab37ba30211045fc96b23dad5f06592b38d))
GP_SOUNDROM0("830.snd",NO_DUMP)
GP_ROMEND
CORE_GAMEDEFNV(ladyshot,"Lady Sharpshooter",1985,"Game Plan",mGP2SM,0)

/*-------------------------------------------------------------------
/ Andromeda (August 1985) - Model #850
/-------------------------------------------------------------------*/
INITGAME(andromed, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU3,0)
GP_ROMSTART00(andromed,	"850.a",CRC(67ed03ee) SHA1(efe7c495766ffb73545a77ab24f02925ac0395f1),
						"850.b",CRC(37c244e8) SHA1(5cef0a1a6f2c34f2d01bdd12ce11da40c8be4296))
GP_SOUNDROM8("850.snd",CRC(18e084a6) SHA1(56efbabe60305f168ca479295577bff7f3a4dace))
GP_ROMEND
CORE_GAMEDEFNV(andromed,"Andromeda",1985,"Game Plan",mGP2SM3,0)

GP_ROMSTART00(andromea,	"850.a",CRC(67ed03ee) SHA1(efe7c495766ffb73545a77ab24f02925ac0395f1),
						"850b.rom",CRC(fc1829a5) SHA1(9761543d17c0a5c08b0fec45c35648ce769a3463))
GP_SOUNDROM8("850.snd",CRC(18e084a6) SHA1(56efbabe60305f168ca479295577bff7f3a4dace))
GP_ROMEND
#define init_andromea init_andromed
GP_INPUT_PORTS_START(andromea, 1) GP_INPUT_PORTS_END
CORE_CLONEDEFNV(andromea,andromed,"Andromeda (alternate set)",1985,"Game Plan",mGP2SM3,0)

/*-------------------------------------------------------------------
/ Cyclopes (November 1985) - Model #800
/-------------------------------------------------------------------*/
INITGAME(cyclopes, 0,dispGP_BDU2,FLIP_SW(FLIP_L),SNDBRD_GPMSU3,0)
GP_ROMSTART000(cyclopes,"800.a",CRC(3e9628e5) SHA1(4dad9e082a9f4140162bc155f2b0f0a948ba012f),
						"800.b",CRC(3f945c46) SHA1(25eb543e0b0edcd0a0dcf8e4aa1405cda55ebe2e),
						"800.c",CRC(7ea18e65) SHA1(e86d82e3ba659499dfbf14920b196252784724f7))
GP_SOUNDROM0("800.snd",CRC(290db3d2) SHA1(a236594f7a89969981bd5707d6dfbb5120fb8f46))
GP_ROMEND
CORE_GAMEDEFNV(cyclopes,"Cyclopes",1985,"Game Plan",mGP2SM3,0)

//Loch Ness Monster (November 1985) - Model #???
