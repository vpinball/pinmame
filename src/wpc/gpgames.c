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

//Real to Real (May 1978)
//Rio (??,1978)
//Camel Lights (May 1978)
//Foxy Lady (May 1978)

/*Games below are Cocktail #120 Model*/
//Family Fun! (April 1979)
//Star Trip (April 1979)

/*Games below are regular standup pinball games*/
//Sharpshooter (May 1979)

/*-------------------------------------------------------------------
/ Vegas (August 1979)
/-------------------------------------------------------------------*/
INITGAME(vegasgp, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART88(vegasgp,	"vegas.u12",0x98f27fdf,
						"vegas.u13",0xb941a1a8)
GP_ROMEND
CORE_GAMEDEFNV(vegasgp,"Vegas (Game Plan)",1979,"Game Plan",mGP1,TEST)

// (December 1979)
/*-------------------------------------------------------------------
/ Coney Island! (August 1979)
/-------------------------------------------------------------------*/
INITGAME(coneyis, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART888(coneyis,	"180a.716",0xdc402b37,
						"180b.716",0x19a86f5e,
						"180c.716",0xb956f67b)
GP_ROMEND
CORE_GAMEDEFNV(coneyis,"Coney Island!",1979,"Game Plan",mGP1,TEST)


//Lizard (July 1980)
//Global Warfare (June 1981)
//Mike Bossy - The Scoring Machine (January 1982)
//Super Nova (May 1982)
//Sharp Shooter II (November 1983)
//Attila the Hun (April 1984)
//Agents 777 (November 1984)
//Captain Hook (April 1985)
//Lady Sharpshooter (May 1985)
//Andromeda (September 1985)
//
/*-------------------------------------------------------------------
/ Cyclopes (November 1985)
/-------------------------------------------------------------------*/
INITGAME(cyclopes, 0,dispGP,FLIP_SW(FLIP_L),0)
GP_ROMSTART00(cyclopes,	"850a",0x67ed03ee,
						"850b",0x37c244e8)
GP_ROMEND
CORE_GAMEDEFNV(cyclopes,"Cyclopes",1985,"Game Plan",mGP1,TEST)

//Loch-Ness Monster (November 1985)

