#include "driver.h"
#include "sim.h"
#include "gp.h"

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
 {0, 0, 0,6,CORE_SEG7}, {0,16, 8,6,CORE_SEG7},
 {4, 0,16,6,CORE_SEG7}, {4,16,24,6,CORE_SEG7},
 {2, 4,14,1,CORE_SEG7}, {2, 6, 6,1,CORE_SEG7},
 {2,20,30,1,CORE_SEG7}, {2,22,22,1,CORE_SEG7}, {0}
};

/* DDU-2 is the same as DDU-1, but all starting panels shifted by 8 */
static core_tLCDLayout dispGP_DDU2[] = {
	{0, 0, 8,6,CORE_SEG7},{0,16,16,6,CORE_SEG7},
	{4, 0,24,6,CORE_SEG7},{4,16,32,6,CORE_SEG7},
	{2, 4,22,1,CORE_SEG7},{2, 6,14,1,CORE_SEG7},
	{2,20,38,1,CORE_SEG7},{2,22,30,1,CORE_SEG7}, {0}
};

/*BDU-1*/
static core_tLCDLayout dispGP_BDU1[] = {
 {0, 0, 0,6,CORE_SEG7}, {0,24, 8,6,CORE_SEG7},
 {4, 0,16,6,CORE_SEG7}, {4,24,24,6,CORE_SEG7},
 {4,13,32,2,CORE_SEG7}, {4,19,36,2,CORE_SEG7}, {0}
};

/*BDU-2*/
//NOTE: 7th Digit order comes after digits 0-6, so we tack it onto front!
static core_tLCDLayout dispGP_BDU2[] = {
 {0, 0, 6,1,CORE_SEG7}, {0,22,14,1,CORE_SEG7},
 {4, 0,22,1,CORE_SEG7}, {4,22,30,1,CORE_SEG7},
 {0, 2, 0,6,CORE_SEG7}, {0,24, 8,6,CORE_SEG7},
 {4, 2,16,6,CORE_SEG7}, {4,24,24,6,CORE_SEG7},
 {4,20,32,2,CORE_SEG7S},{4,26,36,2,CORE_SEG7S},{0}
};

#define INITGAME(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GP_INPUT_PORTS_START(name, 1) GP_INPUT_PORTS_END

//Games in rough production order

/*-------------------------------------------------------------------
/ Foxy Lady (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INITGAME(foxylady, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(foxylady,	"a-110.u12",0xed0d518b,
						"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(foxylady,"Foxy Lady",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Black Velvet (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INITGAME(blvelvet, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(blvelvet,	"a-110.u12",0xed0d518b,
						"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(blvelvet,"Black Velvet",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Camel Lights (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INITGAME(camlight, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(camlight,	"a-110.u12",0xed0d518b,
						"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(camlight,"Camel Lights",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Real (to Real) (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INITGAME(real, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(real,	"a-110.u12",0xed0d518b,
					"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(real,"Real (to Real)",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Rio (May 1978) - Model: Cocktail #110
/-------------------------------------------------------------------*/
INITGAME(rio, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(rio,	"a-110.u12",0xed0d518b,
					"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(rio,"Rio",1978,"Game Plan",mGP1,GAME_USES_CHIMES)

//Chuck-A-Luck (October 1978)

/*-------------------------------------------------------------------
/ Star Trip (April 1979) - Model: Cocktail #120
/-------------------------------------------------------------------*/
INITGAME(startrip, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(startrip,	"startrip.u12",0x98f27fdf,
						"startrip.u13",0xb941a1a8)
GP_ROMEND
CORE_GAMEDEFNV(startrip,"Star Trip",1979,"Game Plan",mGP1,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Family Fun! (April 1979) - Model: Cocktail #120
/-------------------------------------------------------------------*/
INITGAME(famlyfun, 0,dispGP_DDU1,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(famlyfun,	"family.u12",0x98f27fdf,
						"family.u13",0xb941a1a8)
GP_ROMEND
CORE_GAMEDEFNV(famlyfun,"Family Fun!",1979,"Game Plan",mGP1,GAME_NO_SOUND)

/***************************************************************************
 *Games below are regular standup pinball games (except where noted)
 *                   -all games below use MPU-2!
 ***************************************************************************/

/*-------------------------------------------------------------------
/ Sharpshooter (May 1979) - Model #130
/-------------------------------------------------------------------*/
INITGAME(sshooter, 0,dispGP_BDU1,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(sshooter,"130a.716",0xdc402b37,
						"130b.716",0x19a86f5e,
						"130c.716",0xb956f67b)
GP_ROMEND
CORE_GAMEDEFNV(sshooter,"Sharpshooter",1979,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Vegas (August 1979) - Cocktail Model #140
/-------------------------------------------------------------------*/
INITGAME(vegasgp, 0,dispGP_DDU2,FLIP_SW(FLIP_L),-2)
GP_ROMSTART88(vegasgp, "140a.12",0x2c00bc19,
					   "140b.13",0xcf26d67b)
GP_ROMEND
CORE_GAMEDEFNV(vegasgp,"Vegas (Game Plan)",1979,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Coney Island! (December 1979) - Model #180
/-------------------------------------------------------------------*/
INITGAME(coneyis, 0,dispGP_BDU1,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(coneyis,	"180a.716",0xdc402b37,
						"180b.716",0x19a86f5e,
						"180c.716",0xb956f67b)
GP_ROMEND
CORE_GAMEDEFNV(coneyis,"Coney Island!",1979,"Game Plan",mGP2,GAME_NO_SOUND)

//Challenger I (?? / 1980)

/*-------------------------------------------------------------------
/ (Pinball) Lizard (June / July 1980) - Model #210
/-------------------------------------------------------------------*/
INITGAME(lizard, 0,dispGP_BDU1,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(lizard,	"lizard.u12",0xdc402b37,
						"lizard.u13",0x19a86f5e,
						"lizard.u26",0xb956f67b)
//GP_SOUNDROM88("lizard.u9",0x2d121b24,"lizard.u10",0x28b8f1f0)
GP_ROMEND
CORE_GAMEDEFNV(lizard,"(Pinball) Lizard",1980,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Global Warfare (June 1981)  - Model #???
/-------------------------------------------------------------------*/
INITGAME(gwarfare, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(gwarfare,"240a.716",0x30206428,
						"240b.716",0xa54eb15d,
						"240c.716",0x60d115a8)
GP_ROMEND
CORE_GAMEDEFNV(gwarfare,"Global Warfare",1981,"Game Plan",mGP2,GAME_NO_SOUND)

//Mike Bossy (January 1982) - Model #???

/*-------------------------------------------------------------------
/ Super Nova (May 1982) - Model #150
/-------------------------------------------------------------------*/
//Flyer suggests 6 digits for scoring??
INITGAME(suprnova, 0,dispGP_BDU1,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(suprnova,"130a.716",0xdc402b37,
						"150b.716",0x8980a8bb,
						"150c.716",0x6fe08f96)
GP_ROMEND
CORE_CLONEDEFNV(suprnova,sshooter,"Super Nova",1982,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Sharp Shooter II (November 1983) - Model #730
/-------------------------------------------------------------------*/
INITGAME(sshootr2, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(sshootr2,"130a.716",0xdc402b37,
						"130b.716",0x19a86f5e,
						"730c",0xd1af712b)
//GP_SOUNDROM8("730snd",0x6d3dcf44)
GP_ROMEND
CORE_CLONEDEFNV(sshootr2,sshooter,"Sharp Shooter II",1983,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Attila the Hun (April 1984) - Model #260
/-------------------------------------------------------------------*/
INITGAME(atilla, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(atilla,	"260.a",0xb31c11d8,
						"260.b",0xe8cca86d,
						"260.c",0x206605c3)
//GP_SOUNDROM0("260.snd",0x21e6b188)
GP_ROMEND
CORE_GAMEDEFNV(atilla,"Atilla The Hun",1984,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Agents 777 (November 1984) - Model #770
/-------------------------------------------------------------------*/
INITGAME(agent777, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(agent777,"770a",0xfc4eebcd,
						"770b",0xea62aece,
						"770c",0x59280db7)
//GP_SOUNDROM0("770snd",0xe4e66c9f)
GP_ROMEND
CORE_GAMEDEFNV(agent777,"Agents 777",1984,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Captain Hook (April 1985) - Model #780
/-------------------------------------------------------------------*/
INITGAME(cpthook, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(cpthook,	"780.a",0x6bd5a495,
						"780.b",0x3d1c5555,
						"780.c",0xe54bc51f)
//GP_SOUNDROM0("780.snd",0x95af3392)
GP_ROMEND
CORE_GAMEDEFNV(cpthook,"Captain Hook",1985,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Lady Sharpshooter (May 1985) - Cocktail Model #???
/-------------------------------------------------------------------*/
INITGAME(ladyshot, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(ladyshot,"830a.716",0xc055b993,
						"830b.716",0x1e3308ea,
						"830c.716",0xf5e1db15)
GP_ROMEND
CORE_GAMEDEFNV(ladyshot,"Lady Sharpshooter",1985,"Game Plan",mGP2,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Andromeda (September 1985) - Model #850
/-------------------------------------------------------------------*/
/*Something is wrong with the rom loading here.. but not sure how to fix it!
 * - The Interrupt Vector address is at 0x1ff0, which is not a valid address here*/
INITGAME(andromed, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(andromed,"850.a",0xa811f936,
						"850.b",0x75ec7247,
						"850.c",0x75dc73c4)
//GP_SOUNDROM8("850.snd",0x18e084a6)
GP_ROMEND
CORE_GAMEDEFNV(andromed,"Andromeda",1985,"Game Plan",mGP2,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Cyclopes (November 1985) - Model #800
/-------------------------------------------------------------------*/
INITGAME(cyclopes, 0,dispGP_BDU2,FLIP_SW(FLIP_L),0)
GP_ROMSTART00(cyclopes,	"850a",0x67ed03ee,
						"850b",0x37c244e8)
//GP_SOUNDROM8("850snd",0x18e084a6)
GP_ROMEND
CORE_GAMEDEFNV(cyclopes,"Cyclopes",1985,"Game Plan",mGP2,GAME_NO_SOUND)

//Loch Ness Monster (November 1985) - Model #???
