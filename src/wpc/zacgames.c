#include "driver.h"
#include "sim.h"
#include "zac.h"

//Display: 5 X 7 Segment, 7 Digit Displays with 2 commas
static core_tLCDLayout dispZAC[] = {
  {0, 0, 2,7,CORE_SEG87}, {0,16,10,7,CORE_SEG87},
  {2, 0,18,7,CORE_SEG87}, {2,16,26,7,CORE_SEG87},
  {4, 4,34,7,CORE_SEG87}, {0}
};

#define INITGAME(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZAC_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

//Games in rough production order

#define TEST 0
//#define TEST GAME_NOT_WORKING

//10/77 Combat
//01/78 Winter Sports
//07/78 House of Diamonds
//09/78 Strike
//10/78 Ski Jump					
//10/78 Future World
//04/79 Shooting the Rapids
//09/79 Hot Wheels
//09/79 Space City
//01/80 Fire Mountain
//05/80 Star God
//09/80 Space Shuttle
//04/81 Earth, Wind & Fire
//09/81 Locomotion
//04/82 Pinball Champ '82 (Is this really different than the '83?)
//09/82 Soccer King
/*--------------------------------
/ Pinball Champ (??/83)
/-------------------------------*/
INITGAME(pinchamp,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART(pinchamp,	"pinchamp.ic1",0x1412ec33,
						"pinchamp.ic2",0xa24ba4c6,
						"pinchamp.ic3",0xdf5f4f88)
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC1,TEST)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART2(tmachzac,	"timemach.ic1",0xd88f424b,
						"timemach.ic2",0x3c313487)
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART2(farfalla,	"cpurom1.bin",0xac249150,
						"cpurom2.bin",0x6edc823f)
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Devil's Dare (04/84)
/-------------------------------*/
INITGAME(dvlrider,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART2(dvlrider,	"cpu.ic1",0x5874ab12,
						"cpu.ic2",0x09829446)
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2,TEST)

//09/84 Magic Castle
//01/85 Robot
//07/85 Clown
//12/85 Pool Champion


/*--------------------------------
/ Blackbelt (??/86)
/-------------------------------*/
INITGAME(bbeltzac,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART2(bbeltzac,	"bbz-1.fil",0x2e7e1575,
						"bbz-2.fil",0xdbec92ae)
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Blackbelt (Zaccaria)",1986,"Zaccaria",mZAC2,TEST)

//??/86 Mexico
//??/86 Zankor
//??/86 Mystic Star
//??/87 Spooky
//??/87 Star's Phoenix
//??/86 New Star's Phoenix
