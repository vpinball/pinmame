#include "driver.h"
#include "sim.h"
#include "zac.h"
#include "zacsnd.h"

//Display: 5 X 6 Segment, 2 X 2 Segment, 7 Digit Displays without commas
static core_tLCDLayout dispZAC1[] = {
  {0, 0,42,6,CORE_SEG7}, {0,16,34,6,CORE_SEG7},
  {2, 0,26,6,CORE_SEG7}, {2,16,18,6,CORE_SEG7},
  {5, 0, 2,2,CORE_SEG7}, {5, 8, 6,2,CORE_SEG7}, {5,16,10,6,CORE_SEG7},
  {0}
};

//Display: 5 X 7 Segment, 7 Digit Displays with 2 commas
static core_tLCDLayout dispZAC2[] = {
  {0, 0,33,7,CORE_SEG87F}, {0,16,25,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F}, {2,16, 9,7,CORE_SEG87F},
  {5, 0, 1,7,CORE_SEG87},  {0}
};

//Display: 5 X 8 Segment, 7 Digit Displays with 2 commas
static core_tLCDLayout dispZAC3[] = {
  {0, 0,32,8,CORE_SEG87F}, {0,18,24,8,CORE_SEG87F},
  {2, 0,16,8,CORE_SEG87F}, {2,18, 8,8,CORE_SEG87F},
  {5, 0, 0,8,CORE_SEG87},  {0}
};

#define INITGAME1(name, gen, disp) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,0}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZACOLD_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,2}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZAC_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

//Games in rough production order

//10/77 Combat

/*--------------------------------
/ Winter Sports (01/78)
/-------------------------------*/
INITGAME1(wsports,0,dispZAC1)
ZAC_ROMSTART44444(wsports,	"ws1.bin",0x58feb058,
							"ws2.bin",0xece702cb,
							"ws3.bin",0xff7f6824,
							"ws4.bin",0x74460cf2,
							"ws5.bin",0x5ef51ced)
ZAC_ROMEND
CORE_GAMEDEFNV(wsports,"Winter Sports",1978,"Zaccaria",mZAC0,GAME_NO_SOUND)

//07/78 House of Diamonds
//09/78 Strike
//10/78 Ski Jump

/*--------------------------------
/ Future World (10/78)
/-------------------------------*/
// Game ROMs #2 and #3 have to be swapped!
INITGAME1(futurwld,0,dispZAC1)
ZAC_ROMSTART44444(futurwld,	"futwld_1.lgc",0xd83b8793,
							"futwld_3.lgc",0xbdcb7e1d,
							"futwld_2.lgc",0x48e3d293,
							"futwld_4.lgc",0xb1de2120,
							"futwld_5.lgc",0x6b7965f2)
ZAC_ROMEND
CORE_GAMEDEFNV(futurwld,"Future World",1978,"Zaccaria",mZAC0,GAME_NO_SOUND)

/*--------------------------------
/ Shooting the Rapids (04/79)
/-------------------------------*/
INITGAME1(strapids,0,dispZAC1)
ZAC_ROMSTART44444(strapids,	"rapids_1.lgc",0x2a30cef3,
							"rapids_2.lgc",0x04adaa14,
							"rapids_3.lgc",0x397992fb,
							"rapids_4.lgc",0x3319fa21,
							"rapids_5.lgc",0x0dd67110)
ZAC_ROMEND
CORE_GAMEDEFNV(strapids,"Shooting the Rapids",1979,"Zaccaria",mZAC0,GAME_NO_SOUND)

//09/79 Hot Wheels
//09/79 Space City

/*--------------------------------
/ Fire Mountain (01/80)
/-------------------------------*/
INITGAME1(firemntn,0,dispZAC1)
ZAC_ROMSTART84444(firemntn,	"zac_boot.lgc",0x62a3da59,
							"firemt_2.lgc",0xd146253f,
							"firemt_3.lgc",0xd9faae07,
							"firemt_4.lgc",0xb5cac3da,
							"firemt_5.lgc",0x13f11d84)
ZAC_ROMEND
CORE_GAMEDEFNV(firemntn,"Fire Mountain",1980,"Zaccaria",mZAC0,GAME_NO_SOUND)

/*--------------------------------
/ Star God (05/80)
/-------------------------------*/
// After game start, no key will respond anymore?
INITGAME1(stargod,0,dispZAC1)
ZAC_ROMSTART84444(stargod,	"zac_boot.lgc",0x62a3da59,
							"stargod2.lgc",0x7a784b03,
							"stargod3.lgc",0x95492ac0,
							"stargod4.lgc",0x09e5682a,
							"stargod5.lgc",0x03cd4e24)
ZAC_SOUNDROM_0(				"stargod.snd", 0x5079e493)
ZAC_ROMEND
CORE_GAMEDEFNV(stargod,"Star God",1980,"Zaccaria",mZAC0,GAME_NOT_WORKING)

/*--------------------------------
/ Space Shuttle (09/80)
/-------------------------------*/
INITGAME1(sshtlzac,0,dispZAC1)
ZAC_ROMSTART84444(sshtlzac,	"zac_boot.lgc",0x62a3da59,
							"spcshtl2.lgc",0x0e06771b,
							"spcshtl3.lgc",0xa302e5a9,
							"spcshtl4.lgc",0xa02ee0b5,
							"spcshtl5.lgc",0xd1dabd9b)
ZAC_SOUNDROM_0(				"spcshtl.snd", 0x9a61781c)
ZAC_ROMEND
CORE_GAMEDEFNV(sshtlzac,"Space Shuttle (Zaccaria)",1980,"Zaccaria",mZAC0,GAME_NO_SOUND)

/*--------------------------------
/ Earth, Wind & Fire (04/81)
/-------------------------------*/
// Something's wrong (with the rom loading maybe?)
INITGAME1(ewf,0,dispZAC1)
ZAC_ROMSTART84444(ewf,	"zac_boot.lgc",0x62a3da59,
						"ewf_2.lgc",   0xaa67e0b4,
						"ewf_3.lgc",   0xb21bf015,
						"ewf_4.lgc",   0xd110da3f,
						"ewf_5.lgc",   0x686c4a4b)
ZAC_SOUNDROM_0(			"stargod.snd", 0x5079e493)
ZAC_ROMEND
CORE_GAMEDEFNV(ewf,"Earth, Wind & Fire",1981,"Zaccaria",mZAC0,GAME_NOT_WORKING)

/*--------------------------------
/ Locomotion (09/81)
/-------------------------------*/
INITGAME1(locomotn,0,dispZAC1)
ZAC_ROMSTART84844(locomotn,	"loc-1.fil",  0x8d0252a2,
							"loc-2.fil",  0x9dbd8601,
							"loc-3.fil",  0x8cadea7b,
							"loc-4.fil",  0x177c89b6,
							"loc-5.fil",  0xcad4122a)
ZAC_SOUNDROM_0(				"loc-snd.fil",0x51ea9d2a)
ZAC_ROMEND
CORE_GAMEDEFNV(locomotn,"Locomotion",1981,"Zaccaria",mZAC1,GAME_NO_SOUND)

/*--------------------------------
/ Pinball Champ '82 (04/82) (Is this really different from the '83?)
/-------------------------------*/
INITGAME(pinchp82,1,dispZAC3)
ZAC_ROMSTART000(pinchp82,	"pinchamp.ic1",0x1412ec33,
							"pinchamp.ic2",0xa24ba4c6,
							"pinchp82.ic3",0xab7a92ac)
ZAC_SOUNDROM_cefg0(			"pbc_1c.snd",  0x6e2defe5,
							"pbc_1e.snd",  0x703b3cae,
							"pbc_1f.snd",  0xf3f4b950,
							"pbc_1g.snd",  0x44adae13)
ZAC_ROMEND
CORE_CLONEDEFNV(pinchp82,pinchamp,"Pinball Champ '82",1982,"Zaccaria",mZAC2B,GAME_NOT_WORKING)

/*--------------------------------
/ Soccer King (09/82)
/-------------------------------*/
INITGAME(socrking,1,dispZAC3)
ZAC_ROMSTART000(socrking,	"soccer.ic1",0x3fbd7c32,
							"soccer.ic2",0x0cc0df1f,
							"soccer.ic3",0x5da6ea20)
ZAC_SOUNDROM_cefg1(			"sound1.c",  0x9c28b291,
							"sound2.e",  0x6aff2325,
							"sound3.f",  0xd26b43b1,
							"sound4.g",  0x5de7fc8c)
ZAC_ROMEND
CORE_GAMEDEFNV(socrking,"Soccer King",1982,"Zaccaria",mZAC2A,GAME_NOT_WORKING)

/*--------------------------------
/ Pinball Champ (??/83)
/-------------------------------*/
INITGAME(pinchamp,1,dispZAC3)
ZAC_ROMSTART000(pinchamp,	"pinchamp.ic1",0x1412ec33,
							"pinchamp.ic2",0xa24ba4c6,
							"pinchamp.ic3",0xdf5f4f88)
ZAC_SOUNDROM_cefg0(			"sound1.c",    0xf739fcba,
							"sound2.e",    0x24d83e74,
							"sound3.f",    0xd055e8c6,
							"sound4.g",    0x39b68215)
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC2B,GAME_NOT_WORKING)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,1,dispZAC2)
ZAC_ROMSTART1820(tmachzac,	"timemach.ic1",0xd88f424b,
							"timemach.ic2",0x3c313487)
ZAC_SOUNDROM_de1g(			"sound1.d",    0xefc1d724,
							"sound2.e",    0x41881a1d,
							"sound3.g",    0xb7b872da)
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2C,GAME_NOT_WORKING)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,1,dispZAC2)
ZAC_ROMSTART1820(farfalla,	"cpurom1.bin",0xac249150,
							"cpurom2.bin",0x6edc823f)
ZAC_SOUNDROM_de1g(			"rom1.snd",   0xaca09674,
							"rom2.snd",   0x76da384d,
							"rom3.snd",   0xd0584952)
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2F,GAME_NO_SOUND)

/*--------------------------------
/ Devil Riders (04/84)
/-------------------------------*/
INITGAME(dvlrider,1,dispZAC2)
ZAC_ROMSTART1820(dvlrider,	"cpu.ic1",0x5874ab12,
							"cpu.ic2",0x09829446)
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",0x5d48462c,
						"gb01snd2.1e",0x1127be59,
						"gb01snd3.1g",0x1ae91ae8)
// ZAC_SOUNDROM_de2g(	"g_snd_1.bin",0x77d042dc, // german speech
//						"g_snd_2.bin",0x31e35fd4,
//						"g_snd_3.bin",0x2e64a401)
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2,GAME_NO_SOUND)

/*--------------------------------
/ Magic Castle (09/84)
/-------------------------------*/
INITGAME(mcastle,1,dispZAC3)
ZAC_ROMSTART020(mcastle,	"cpu.ic1",0x50dd8209,
							"cpu.ic2",0x11372bec)
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",0xcd6a4a07,
						"gb01snd2.1e",0xd289952d,
						"gb01snd3.1g",0x8b4342eb)
// ZAC_SOUNDROM_de2g(	"magic1d.snd",0x1f1a1140, // german speech
//						"magic1e.snd",0xa8787011,
//						"magic1g.snd",0x313fb216)
ZAC_ROMEND
CORE_GAMEDEFNV(mcastle,"Magic Castle",1984,"Zaccaria",mZAC2,GAME_NO_SOUND)

/*--------------------------------
/ Robot (01/85)
/-------------------------------*/
// There might be a bad rom on this one.
INITGAME(robot,1,dispZAC3)
ZAC_ROMSTART020(robot,	"robot_1.lgc",0x96a87432,
						"robot_2.lgc",0x28ba9687)
ZAC_SOUNDROM_de2g(		"robot_d.snd",0x88685b1e,
						"robot_e.snd",0xe326a851,
						"robot_g.snd",0x7ed5da55)
ZAC_ROMEND
CORE_GAMEDEFNV(robot,"Robot",1985,"Zaccaria",mZAC2F,GAME_NOT_WORKING)

/*--------------------------------
/ Clown (07/85)
/-------------------------------*/
INITGAME(clown,1,dispZAC3)
ZAC_ROMSTART020(clown,	"clown_1.lgc",0x16f09833,
						"clown_2.lgc",0x697e6b5b)
ZAC_SOUNDROM_e2f2(		"clown_e.snd",0x04a34cc1,
						"clown_f.snd",0xe35a4f72)
ZAC_ROMEND
CORE_GAMEDEFNV(clown,"Clown",1985,"Zaccaria",mZAC2,GAME_NO_SOUND)

/*--------------------------------
/ Pool Champion (12/85)
/-------------------------------*/
INITGAME(poolcham,1,dispZAC3)
ZAC_ROMSTART020(poolcham,	"poolcham.ic1",0xfca2a2b2,
							"poolcham.ic2",0x267a2a02)
ZAC_SOUNDROM_f(				"poolcham.1f", 0xefe33926)
// ZAC_SOUNDROM_f(			"sound1.f",    0xb4b4e31e) // different language?
ZAC_ROMEND
CORE_GAMEDEFNV(poolcham,"Pool Champion",1985,"Zaccaria",mZAC2F,GAME_NO_SOUND)

/*--------------------------------
/ Black Belt (03/86)
/-------------------------------*/
INITGAME(bbeltzac,1,dispZAC3)
ZAC_ROMSTART1820(bbeltzac,	"bbz-1.fil",0x2e7e1575,
							"bbz-2.fil",0xdbec92ae)
ZAC_SOUNDROM_e2f4(			"bbz-e.snd",0x1fe045d2,
							"bbz-f.snd",0x9f58f369)
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Black Belt (Zaccaria)",1986,"Zaccaria",mZAC2F,GAME_NO_SOUND)

/*--------------------------------
/ Mexico '86 (07/86)
/-------------------------------*/
INITGAME(mexico,1,dispZAC3)
ZAC_ROMSTART1820(mexico,	"mex86_1.lgc",0x60d559b1,
							"mex86_2.lgc",0x5c984c15)
ZAC_SOUNDROM_e2f4(			"mex86_e.snd",0xa985e8db,
							"mex86_f.snd",0x301c2b63)
ZAC_ROMEND
CORE_GAMEDEFNV(mexico,"Mexico '86",1986,"Zaccaria",mZAC2F,GAME_NO_SOUND)

/*--------------------------------
/ Zankor (12/86)
/-------------------------------*/
INITGAME(zankor,1,dispZAC3)
ZAC_ROMSTART1820(zankor,	"zan_ic1.764",0xe7ba5acf,
							"zan_ic2.764",0x5804ff10)
ZAC_SOUNDROM_e4f4(			"zan_1e.128", 0xd467000f,
							"zan_1f.128", 0x74fcadc9)
ZAC_SOUNDROM_456(			"zan_ic4.128",0xf34a2aaa,
							"zan_ic5.128",0xbf61aab0,
							"zan_ic6.128",0x13a5b8d4)
ZAC_ROMEND
CORE_GAMEDEFNV(zankor,"Zankor",1986,"Zaccaria",mZAC2F,GAME_NO_SOUND)

//??/86 Mystic Star (conversion kit with different hardware)

/*--------------------------------
/ Spooky (04/87)
/-------------------------------*/
INITGAME(spooky,1,dispZAC3)
ZAC_ROMSTART1820(spooky,	"spook_1.lgc",0x377b347d,
							"spook_2.lgc",0xae0598b0)
ZAC_SOUNDROM_e2f4(			"spook_e.snd",0x3d632c93,
							"spook_f.snd",0xcc04a448)
ZAC_SOUNDROM_46(			"spook_4.snd",0x3ab517a4,
							"spook_6.snd",0xd4320bc7)
ZAC_ROMEND
CORE_GAMEDEFNV(spooky,"Spooky",1987,"Zaccaria",mZAC2,GAME_NO_SOUND)

//??/87 Star's Phoenix
//??/86 New Star's Phoenix
