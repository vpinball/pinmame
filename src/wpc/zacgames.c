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

/*--------------------------------
/ Winter Sports (01/78)
/-------------------------------*/
INITGAME(wsports,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART44444(wsports,	"ws1.bin",0x58feb058,
							"ws2.bin",0xece702cb,
							"ws3.bin",0xff7f6824,
							"ws4.bin",0x74460cf2,
							"ws5.bin",0x5ef51ced)
ZAC_ROMEND
CORE_GAMEDEFNV(wsports,"Winter Sports",1978,"Zaccaria",mZAC0,TEST)

//07/78 House of Diamonds
//09/78 Strike
//10/78 Ski Jump
//10/78 Future World
//04/79 Shooting the Rapids
//09/79 Hot Wheels
//09/79 Space City
//01/80 Fire Mountain

/*--------------------------------
/ Star God (05/80)
/-------------------------------*/
INITGAME(stargod,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART84444(stargod,	"stargod1.lgc",0x62a3da59,
							"stargod2.lgc",0x7a784b03,
							"stargod3.lgc",0x95492ac0,
							"stargod4.lgc",0x09e5682a,
							"stargod5.lgc",0x03cd4e24)
ZAC_ROMEND
CORE_GAMEDEFNV(stargod,"Star God",1980,"Zaccaria",mZAC0,TEST)

/*--------------------------------
/ Space Shuttle (09/80)
/-------------------------------*/
INITGAME(sshtlzac,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART84444(sshtlzac,	"stargod1.lgc",0x62a3da59,
							"spcshtl2.lgc",0x0e06771b,
							"spcshtl3.lgc",0xa302e5a9,
							"spcshtl4.lgc",0xa02ee0b5,
							"spcshtl5.lgc",0xd1dabd9b)
ZAC_ROMEND
CORE_GAMEDEFNV(sshtlzac,"Space Shuttle (Zaccaria)",1980,"Zaccaria",mZAC0,TEST)

/*--------------------------------
/ Earth, Wind & Fire (04/81)
/-------------------------------*/
INITGAME(ewf,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART84444(ewf,	"ewf1.bin",0x62a3da59,
						"ewf2.bin",0x0,
						"ewf3.bin",0x0,
						"ewf4.bin",0x0,
						"ewf5.bin",0x0)
ZAC_ROMEND
CORE_GAMEDEFNV(ewf,"Earth, Wind & Fire",1981,"Zaccaria",mZAC0,TEST)

/*--------------------------------
/ Locomotion (09/81)
/-------------------------------*/
INITGAME(locomotn,0,dispZAC,FLIP_SW(FLIP_L),0)
ZAC_ROMSTART84844(locomotn,	"loc-1.fil",0x8d0252a2,
							"loc-2.fil",0x9dbd8601,
							"loc-3.fil",0x8cadea7b,
							"loc-4.fil",0x177c89b6,
							"loc-5.fil",0xcad4122a)
ZAC_ROMEND
CORE_GAMEDEFNV(locomotn,"Locomotion",1981,"Zaccaria",mZAC1,TEST)

//04/82 Pinball Champ '82 (Is this really different from the '83?)

/*--------------------------------
/ Soccer King (09/82)
/-------------------------------*/
INITGAME(socrking,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART000(socrking,	"soccer.ic1",0x3fbd7c32,
							"soccer.ic2",0x0cc0df1f,
							"soccer.ic3",0x5da6ea20)
ZAC_ROMEND
CORE_GAMEDEFNV(socrking,"Soccer King",1982,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Pinball Champ (??/83)
/-------------------------------*/
INITGAME(pinchamp,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART000(pinchamp,	"pinchamp.ic1",0x1412ec33,
							"pinchamp.ic2",0xa24ba4c6,
							"pinchamp.ic3",0xdf5f4f88)
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(tmachzac,	"timemach.ic1",0xd88f424b,
							"timemach.ic2",0x3c313487)
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(farfalla,	"cpurom1.bin",0xac249150,
							"cpurom2.bin",0x6edc823f)
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Devil Riders (04/84)
/-------------------------------*/
INITGAME(dvlrider,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(dvlrider,	"cpu.ic1",0x5874ab12,
							"cpu.ic2",0x09829446)
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Magic Castle (09/84)
/-------------------------------*/
INITGAME(mcastle,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART020(mcastle,	"cpu.ic1",0x50dd8209,
							"cpu.ic2",0x11372bec)
ZAC_ROMEND
CORE_GAMEDEFNV(mcastle,"Magic Castle",1984,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Robot (01/85)
/-------------------------------*/
INITGAME(robot,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART020(robot,	"robot_1.lgc",0x96a87432,
						"robot_2.lgc",0x28ba9687)
ZAC_ROMEND
CORE_GAMEDEFNV(robot,"Robot",1985,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Clown (07/85)
/-------------------------------*/
INITGAME(clown,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART020(clown,	"clown_1.lgc",0x16f09833,
						"clown_2.lgc",0x697e6b5b)
ZAC_ROMEND
CORE_GAMEDEFNV(clown,"Clown",1985,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Pool Champion (12/85)
/-------------------------------*/
INITGAME(poolcham,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART020(poolcham,	"poolcham.ic1",0xfca2a2b2,
							"poolcham.ic2",0x267a2a02)
ZAC_ROMEND
CORE_GAMEDEFNV(poolcham,"Pool Champion",1985,"Zaccaria",mZAC2,TEST)

/*--------------------------------
/ Blackbelt (??/86)
/-------------------------------*/
INITGAME(bbeltzac,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(bbeltzac,	"bbz-1.fil",0x2e7e1575,
							"bbz-2.fil",0xdbec92ae)
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Blackbelt (Zaccaria)",1986,"Zaccaria",mZAC2,TEST)

//??/86 Mexico

/*--------------------------------
/ Zankor (??/86)
/-------------------------------*/
INITGAME(zankor,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(zankor,	"zan_ic1.764",0xe7ba5acf,
							"zan_ic2.764",0x5804ff10)
ZAC_ROMEND
CORE_GAMEDEFNV(zankor,"Zankor",1986,"Zaccaria",mZAC2,TEST)

//??/86 Mystic Star (conversion kit with different hardware)

/*--------------------------------
/ Spooky (??/87)
/-------------------------------*/
INITGAME(spooky,0,dispZAC,FLIP_SW(FLIP_L),2)
ZAC_ROMSTART1820(spooky,	"spook_1.lgc",0x377b347d,
							"spook_2.lgc",0xae0598b0)
ZAC_ROMEND
CORE_GAMEDEFNV(spooky,"Spooky",1987,"Zaccaria",mZAC2,TEST)

//??/87 Star's Phoenix
//??/86 New Star's Phoenix
