#include "driver.h"
#include "sim.h"
#include "GTS3.h"
#include "gts3sound.h"

/* ROM STATUS
N = No Lead, L = Have a Lead on Rom

(N*)Lights, Camera, Action 1989
(N*)Silver Slugger 1990
(L)Vegas 1990
(L)Deadly Weapon 1990
(N*)Title Fight 1990
(N*)Bell Ringer/Nudge It 1990
(L)Car Hop 1991
(N*)Hoops 1991
Cactus Jacks 1991
Class of 1812 1991
Surf'n Safari 1991
Operation: Thunderstorm (OT) /No. 1721/ Premier Technology , March 1992, 4 players 
Super Mario Bros. (SMB) /No. 2435/ Premier Technology , April 25, 1992, 4 players 
(N*)Super Mario Bros. Mushroom World (MW) /No. 3427/ Premier Technology , June 1992, 4 players 
Cue Ball Wizard (CBW) /No. 610/ Premier Technology , October 1992, 4 players 
Street Fighter II (SF2, SFII) /No. 2403/ Premier Technology , March 1993, 4 players 
Tee'd Off (TO) /No. 2508/ Premier Technology , May 1993, 4 players 
Wipe Out (WO) /No. 2799/ Premier Technology , October 1993, 4 players 
Gladiators /No. 1011/ Premier Technology , November 1993, 4 players 
World Challenge Soccer (WCS) /No. 2808/ Premier Technology , February 1994, 4 players 
Rescue 911 (R911) /No. 1951/ Premier Technology , May 1994, 4 players 
Freddy: a Nightmare on Elm Street (Freddy) /No. 948/ Premier Technology , October 1994, 4 
Shaq Attaq (SA) /No. 2874/ Premier Technology , February 1995, 4 players 
Stargate (SG) /No. 2847/ Premier Technology , March 1995, 4 players 
Big Hurt /No. 3591/ Premier Technology , June 1995, 4 players 
Strikes 'N Spares 1995 (????????)
Waterworld (WW) /No. 3793/ Premier Technology , October 1995, 4 players 
Mario Andretti (MA) /No. 3794/ Premier Technology , December 1995, 4 players 
Barb Wire /No. 3795/ Premier Technology , April 1996, 4 players 
*/

#define ALPHA	 GTS3_dispAlpha
#define DMD		 0
#define FLIP67   FLIP_SWNO(GTS3_SWNO(6),GTS3_SWNO(7))
#define FLIP4142 FLIP_SWNO(GTS3_SWNO(41),GTS3_SWNO(42))
#define FLIP4243 FLIP_SWNO(GTS3_SWNO(42),GTS3_SWNO(43))
#define FLIP4547 FLIP_SWNO(GTS3_SWNO(45),GTS3_SWNO(47))
#define FLIP8182 FLIP_SWNO(GTS3_SWNO(81),GTS3_SWNO(82))
#define FLIP8283 FLIP_SWNO(GTS3_SWNO(82),GTS3_SWNO(83))
#define GDISP_SEG_20(row,type)    {2*row, 0, 20*row, 20, type}

/* 2 X 20 AlphaNumeric Rows */
core_tLCDLayout GTS3_dispAlpha[] = {
	GDISP_SEG_20(0,CORE_SEG16),GDISP_SEG_20(1,CORE_SEG16),{0}
};

/* 2 X 20 AlphaNumeric Rows & LED Board with 4x2 7 Segments*/
core_tLCDLayout GTS3_dispAlpha1[] = {
	GDISP_SEG_20(0,CORE_SEG16),GDISP_SEG_20(1,CORE_SEG16),{4,0,1,8,CORE_SEG7},{0}
};


#define INITGAME(name, disptype, flippers, balls) \
	static core_tGameData name##GameData = {0,disptype,{flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GTS3_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

#define INITGAME2(name, disptype, flippers, balls) \
	static core_tGameData name##GameData = {0,disptype,{flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GTS32_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

#define INITGAME3(name, disptype, flippers, balls) \
	static core_tGameData name##GameData = {0,disptype,{flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
GTS33_INPUT_PORTS_START(name, balls) GTS3_INPUT_PORTS_END

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Lights, Camera, Action
/-------------------------------------------------------------------*/
INITGAME(lca, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(lca,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(lca,"Lights, Camera, Action",1989,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Silver Slugger
/-------------------------------------------------------------------*/
INITGAME(silvslug, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(silvslug,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(silvslug,"Silver Slugger",1990,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Vegas
/-------------------------------------------------------------------*/
INITGAME(vegas, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(vegas,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(vegas,"Vegas",1990,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Deadly Weapon
/-------------------------------------------------------------------*/
INITGAME(deadweap, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(deadweap,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(deadweap,"Deadly Weapon",1990,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Title Fight
/-------------------------------------------------------------------*/
INITGAME(tfight, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(tfight,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(tfight,"Title Fight",1990,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Bell Ringer/Nudge It
/-------------------------------------------------------------------*/
INITGAME(bellring, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(bellring,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(bellring,"Bell Ringer/Nudge It",1990,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Car Hop
/-------------------------------------------------------------------*/
INITGAME(carhop, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(carhop,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(carhop,"Car Hop",1991,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Hoops
/-------------------------------------------------------------------*/
INITGAME(hoops, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(hoops,	"gprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(hoops,"Hoops",1991,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Cactus Jacks
/-------------------------------------------------------------------*/
INITGAME(cactjack, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(cactjack,	"gprom.bin",0x5661ab06)
GTS3_ROMEND
CORE_GAMEDEFNV(cactjack,"Cactus Jacks",1991,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Class of 1812
/-------------------------------------------------------------------*/
INITGAME(clas1812, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(clas1812,	"gprom.bin",0x564349bf)
GTS3_ROMEND
CORE_GAMEDEFNV(clas1812,"Class of 1812",1991,"Gottlieb",mGTS3,GAME_NOT_WORKING)
/*-------------------------------------------------------------------
/ Surf'n Safari
/-------------------------------------------------------------------*/
INITGAME(surfnsaf, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(surfnsaf,	"gprom.bin",0xac3393bd)
GTS3_ROMEND
CORE_GAMEDEFNV(surfnsaf,"Surf'n Safari",1991,"Gottlieb",mGTS3,GAME_NO_SOUND)


//ALL GAMES LISTED BELOW HERE - FLIPPER SWITCHES ARE CONFIRMED CORRECT!!!!
//ALL GAMES LISTED ABOVE HERE - FLIPPER SWITCHES NOT CORRECT OR CONFIRMED!

/*-------------------------------------------------------------------
/ Operation Thunder
/-------------------------------------------------------------------*/
INITGAME3(opthund, ALPHA, FLIP67, 3/*?*/)
GTS3ROMSTART(opthund,	"gprom.bin",0x96a128c2)
GTS3_ROMEND
CORE_GAMEDEFNV(opthund,"Operation Thunder",1992,"Gottlieb",mGTS3,GAME_NO_SOUND)

/*************************
 ***Start of DMD 128x32***
 *************************/

/*-------------------------------------------------------------------
/ Super Mario Brothers
/-------------------------------------------------------------------*/
INITGAME2(smb, DMD, FLIP4547, 3/*?*/)
GTS3ROMSTART(smb,		"gprom.bin", 0xfa1f6e52)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0x59639112)
GTS3_ROMEND
CORE_GAMEDEFNV(smb,"Super Mario Brothers",1992,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Super Mario Brothers Mushroom World (NEED ROM)
/-------------------------------------------------------------------*/
INITGAME2(smbmush, DMD, FLIP4547, 3/*?*/)
GTS3ROMSTART(smbmush,	"gprom.bin", 0x0)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0x0)
GTS3_ROMEND
CORE_GAMEDEFNV(smbmush,"Super Mario Brothers Mushroom World",1992,"Gottlieb",mGTS3DMD,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Cue Ball Wizard
/-------------------------------------------------------------------*/
INITGAME2(cueball, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(cueball,	"gprom.bin",0x3437fdd8)
GTS3_DMD256_ROMSTART(	"dsprom.bin",0x3cc7f470)
GTS3SOUND32256(			"yrom1.bin",0xc22f5cc5,
						"drom1.bin",0x9fd04109,
						"arom1.bin",0x476bb11c,
						"arom2.bin",0x23708ad9)
GTS3_ROMEND
CORE_GAMEDEFNV(cueball,"Cue Ball Wizard",1992,"Gottlieb",mGTS3DMDS, 0)

/************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST */
/************************************************/

/*-------------------------------------------------------------------
/ Street Fighter 2
/-------------------------------------------------------------------*/
INITGAME2(sf2, DMD, FLIP8283, 3/*?*/)
GTS3ROMSTART(sf2,		"gprom.bin",0x299ad173)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xe565e5e9)
GTS3_ROMEND
CORE_GAMEDEFNV(sf2,"Street Fighter 2",1993,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Teed Off
/-------------------------------------------------------------------*/
INITGAME2(teedoff, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(teedoff,	"gprom.bin",0xd7008579)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x24f10ad2)
GTS3_ROMEND
CORE_GAMEDEFNV(teedoff,"Teed Off",1993,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Wipeout
/-------------------------------------------------------------------*/
INITGAME2(wipeout, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(wipeout,	"gprom.bin",0x1161cdb7)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xcbdec3ab)
GTS3_ROMEND
CORE_GAMEDEFNV(wipeout,"Wipeout",1993,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Gladiators 
/-------------------------------------------------------------------*/
INITGAME2(gladiatr, DMD, FLIP8283, 3/*?*/)
GTS3ROMSTART(gladiatr,	"gprom.bin", 0x40386cf5)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xfdc8baed)
GTS3_ROMEND
CORE_GAMEDEFNV(gladiatr,"Gladiators",1993,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ World Challenge Soccer
/-------------------------------------------------------------------*/
INITGAME2(wcsoccer, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(wcsoccer,	"gprom.bin", 0x6382c32e)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x71ba5263)
GTS3_ROMEND
CORE_GAMEDEFNV(wcsoccer,"World Challenge Soccer",1994,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Rescue 911
/-------------------------------------------------------------------*/
INITGAME2(rescu911, DMD, FLIP8283, 3/*?*/)
GTS3ROMSTART(rescu911,	"gprom.bin", 0x943a7597)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x9657ebd5)
GTS3_ROMEND
CORE_GAMEDEFNV(rescu911,"Rescue 911",1994,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Freddy: A Nightmare on Elm Street
/-------------------------------------------------------------------*/
INITGAME2(freddy, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(freddy,	"gprom.bin", 0xf0a6f3e6)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xd78d0fa3)
GTS3_ROMEND
CORE_GAMEDEFNV(freddy,"Freddy: A Nightmare on Elm Street",1994,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Shaq Attaq
/-------------------------------------------------------------------*/
INITGAME2(shaqattq, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(shaqattq,	"gprom.bin", 0x7a967fd1)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xd6cca842)
GTS3_ROMEND
CORE_GAMEDEFNV(shaqattq,"Shaq Attaq",1995,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/************************************************************/
/* ALL GAMES BELOW HAD IMPROVED DIAGNOSTIC TEST & UTILITIES */
/************************************************************/

/*-------------------------------------------------------------------
/ Stargate
/-------------------------------------------------------------------*/
INITGAME2(stargate, DMD, FLIP8182, 3/*?*/)
GTS3ROMSTART(stargate,	"gprom.bin",0x567ecd88)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x91c1b01a)
GTS3_ROMEND
CORE_GAMEDEFNV(stargate,"Stargate",1995,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Big Hurt
/-------------------------------------------------------------------*/
INITGAME2(bighurt, DMD, FLIP8283, 3/*?*/)
GTS3ROMSTART(bighurt,	"gprom.bin", 0x92ce9353)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0xbbe96c5e)
GTS3_ROMEND
CORE_GAMEDEFNV(bighurt,"Big Hurt",1995,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Waterworld
/-------------------------------------------------------------------*/
INITGAME2(waterwld, DMD, FLIP4142, 3/*?*/)
GTS3ROMSTART(waterwld,	"gprom.bin", 0xdb1fd197)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x79164099)
GTS3_ROMEND
CORE_GAMEDEFNV(waterwld,"Waterworld",1995,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Mario Andretti
/-------------------------------------------------------------------*/
INITGAME2(andretti, DMD, FLIP8283, 3/*?*/)
GTS3ROMSTART(andretti,	"gprom.bin", 0xcffa788d)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x1f70baae)
GTS3_ROMEND
CORE_GAMEDEFNV(andretti,"Mario Andretti",1995,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Barb Wire 
/-------------------------------------------------------------------*/
INITGAME2(barbwire, DMD, FLIP4243, 3/*?*/)
GTS3ROMSTART(barbwire,	"gprom.bin", 0x2e130835)
GTS3_DMD512_ROMSTART(	"dsprom.bin",0x2b9533cd)
GTS3_ROMEND
CORE_GAMEDEFNV(barbwire,"Barb Wire",1996,"Gottlieb",mGTS3DMD, GAME_NO_SOUND)
