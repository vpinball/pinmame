#include "driver.h"
#include "sim.h"
#include "gp.h"

//Display: 4 X 7 Segment, 6 Digit Displays, 2 x 2 Digit 7 Segment
//Display: 4 X 7 Segment, 7 Digit Displays, 2 x 2 Digit 7 Segment
static core_tLCDLayout dispGP[] = {
  {0, 0, 2,7,CORE_SEG87}, {0,16,10,7,CORE_SEG87},
  {2, 0,18,7,CORE_SEG87}, {2,16,26,7,CORE_SEG87},
  {4, 4,34,7,CORE_SEG87}, {0}
};

#define INITGAME(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GP_INPUT_PORTS_START(name, 1) GP_INPUT_PORTS_END

//Games in rough production order

#define TEST 0
//#define TEST GAME_NOT_WORKING

/*Games below are Cocktail #110 Model*/

/*-------------------------------------------------------------------
/ Foxy Lady (May 1978)
/-------------------------------------------------------------------*/
INITGAME(foxylady, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART88(foxylady,	"a-110.u12",0xed0d518b,
						"b1-110.u13",0xa223f2e8)
GP_ROMEND
CORE_GAMEDEFNV(foxylady,"Foxy Lady",1978,"Game Plan",mGP1,TEST)

//Black Velvet (May 1978) (Same as Foxy Lady)
//Camel Lights (May 1978) (Same as Foxy Lady)
//Real to Real (May 1978) (Same as Foxy Lady)
//Rio (?? / 1978) (Same as Foxy Lady)

//Chuck-A-Luck (October 1978)

/*Games below are Cocktail #120 Model*/

/*-------------------------------------------------------------------
/ Star Trip (April 1979)
/-------------------------------------------------------------------*/
INITGAME(startrip, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART88(startrip,	"startrip.u12",0x98f27fdf,
						"startrip.u13",0xb941a1a8)
GP_ROMEND
CORE_GAMEDEFNV(startrip,"Star Trip",1979,"Game Plan",mGP1,TEST)

//Family Fun! (April 1979) (Same as Star Trip)

/*Games below are regular standup pinball games*/

/*-------------------------------------------------------------------
/ Sharpshooter (May 1979)
/-------------------------------------------------------------------*/
INITGAME(sshooter, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(sshooter,"130a.716",0xdc402b37,
						"130b.716",0x19a86f5e,
						"130c.716",0xb956f67b)
GP_ROMEND
CORE_GAMEDEFNV(sshooter,"Sharpshooter",1979,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Vegas (August 1979)
/-------------------------------------------------------------------*/
INITGAME(vegasgp, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART88(vegasgp,	"vegas.u12",0x98f27fdf,
						"vegas.u13",0xb941a1a8)
GP_ROMEND
CORE_GAMEDEFNV(vegasgp,"Vegas (Game Plan)",1979,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Coney Island! (December 1979)
/-------------------------------------------------------------------*/
INITGAME(coneyis, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(coneyis,	"180a.716",0xdc402b37,
						"180b.716",0x19a86f5e,
						"180c.716",0xb956f67b)
GP_ROMEND
CORE_GAMEDEFNV(coneyis,"Coney Island!",1979,"Game Plan",mGP1,TEST)

//Challenger I (?? / 1980)

/*-------------------------------------------------------------------
/ (Pinball) Lizard (June / July 1980)
/-------------------------------------------------------------------*/
INITGAME(lizard, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(lizard,	"lizard.u12",0xdc402b37,
						"lizard.u13",0x19a86f5e,
						"lizard.u26",0xb956f67b)
//GP_SOUNDROM88("lizard.u9",0x2d121b24,"lizard.u10",0x28b8f1f0)
GP_ROMEND
CORE_GAMEDEFNV(lizard,"(Pinball) Lizard",1980,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Global Warfare (June 1981)
/-------------------------------------------------------------------*/
INITGAME(gwarfare, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(gwarfare,"240a.716",0x30206428,
						"240b.716",0xa54eb15d,
						"240c.716",0x60d115a8)
GP_ROMEND
CORE_GAMEDEFNV(gwarfare,"Global Warfare",1981,"Game Plan",mGP1,TEST)

//Mike Bossy (January 1982)
//The Scoring Machine (January 1982)

/*-------------------------------------------------------------------
/ Super Nova (May 1982)
/-------------------------------------------------------------------*/
INITGAME(suprnova, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(suprnova,"150a.716",0xdc402b37,
						"150b.716",0x8980a8bb,
						"150c.716",0x6fe08f96)
GP_ROMEND
CORE_GAMEDEFNV(suprnova,"Super Nova",1982,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Sharp Shooter II (November 1983)
/-------------------------------------------------------------------*/
INITGAME(sshootr2, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(sshootr2,"730a",0xdc402b37,
						"730b",0x19a86f5e,
						"730c",0xd1af712b)
//GP_SOUNDROM8("730snd",0x6d3dcf44)
GP_ROMEND
CORE_GAMEDEFNV(sshootr2,"Sharp Shooter II",1983,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Attila the Hun (April 1984)
/-------------------------------------------------------------------*/
INITGAME(atilla, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(atilla,	"260.a",0xb31c11d8,
						"260.b",0xe8cca86d,
						"260.c",0x206605c3)
//GP_SOUNDROM0("260.snd",0x21e6b188)
GP_ROMEND
CORE_GAMEDEFNV(atilla,"Atilla The Hun",1984,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Agents 777 (November 1984)
/-------------------------------------------------------------------*/
INITGAME(agent777, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(agent777,"770a",0xfc4eebcd,
						"770b",0xea62aece,
						"770c",0x59280db7)
//GP_SOUNDROM0("770snd",0xe4e66c9f)
GP_ROMEND
CORE_GAMEDEFNV(agent777,"Agents 777",1984,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Captain Hook (April 1985)
/-------------------------------------------------------------------*/
INITGAME(cpthook, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(cpthook,	"780.a",0x6bd5a495,
						"780.b",0x3d1c5555,
						"780.c",0xe54bc51f)
//GP_SOUNDROM0("780.snd",0x95af3392)
GP_ROMEND
CORE_GAMEDEFNV(cpthook,"Captain Hook",1985,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Lady Sharpshooter (May 1985) (Cocktail Machine)
/-------------------------------------------------------------------*/
INITGAME(ladyshot, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(ladyshot,"830a.716",0xc055b993,
						"830b.716",0x1e3308ea,
						"830c.716",0xf5e1db15)
GP_ROMEND
CORE_GAMEDEFNV(ladyshot,"Lady Sharpshooter",1985,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Andromeda (September 1985)
/-------------------------------------------------------------------*/
INITGAME(andromed, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(andromed,"850.a",0xa811f936,
						"850.b",0x75ec7247,
						"850.c",0x75dc73c4)
//GP_SOUNDROM8("850.snd",0x18e084a6)
GP_ROMEND
CORE_GAMEDEFNV(andromed,"Andromeda",1985,"Game Plan",mGP1,TEST)

/*-------------------------------------------------------------------
/ Cyclopes (November 1985)
/-------------------------------------------------------------------*/
INITGAME(cyclopes, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART00(cyclopes,	"850a",0x67ed03ee,
						"850b",0x37c244e8)
//GP_SOUNDROM8("850snd",0x18e084a6)
GP_ROMEND
CORE_GAMEDEFNV(cyclopes,"Cyclopes",1985,"Game Plan",mGP1,TEST)

//Loch Ness Monster (November 1985)
