#include "driver.h"
#include "sim.h"
#include "zac.h"
#include "zacsnd.h"
#include "sndbrd.h"

//Display: 5 X 6 Digit, 2 X 2 Digit, 7 Segment Displays without commas
static core_tLCDLayout dispZAC1[] = {
  {0, 0,42,6,CORE_SEG7}, {0,16,34,6,CORE_SEG7},
  {2, 0,26,6,CORE_SEG7}, {2,16,18,6,CORE_SEG7},
  {5, 0, 2,2,CORE_SEG7}, {5, 8, 6,2,CORE_SEG7}, {5,16,10,6,CORE_SEG7},
  {0}
};

//Display: 5 X 7 Digit, 7 Segment Displays with 2 commas
static core_tLCDLayout dispZAC2[] = {
  {0, 0,33,7,CORE_SEG87F}, {0,16,25,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F}, {2,16, 9,7,CORE_SEG87F},
  {5, 0, 1,7,CORE_SEG87F}, {0}
};

//Display: 5 X 8 Digit, 7 Segment Displays with 2 commas
static core_tLCDLayout dispZAC3[] = {
  {0, 0,32,8,CORE_SEG87F}, {0,18,24,8,CORE_SEG87F},
  {2, 0,16,8,CORE_SEG87F}, {2,18, 8,8,CORE_SEG87F},
  {5, 0, 0,8,CORE_SEG87F}, {0}
};

#ifdef MAME_DEBUG
#define SOUNDFLAG 0
#else
#define SOUNDFLAG GAME_IMPERFECT_SOUND
#endif

#define INITGAME1(name, gen, disp, sb) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,0,0,sb}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZACOLD_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

#define INITGAME(name, gen, disp, sb) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,2,0,sb}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZAC_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

//Games in rough production order

//10/77 Combat

/*--------------------------------
/ Winter Sports (01/78)
/-------------------------------*/
INITGAME1(wsports,0,dispZAC1,SNDBRD_ZAC1311)
ZAC_ROMSTART44444(wsports,	"ws1.bin",CRC(58feb058),
							"ws2.bin",CRC(ece702cb),
							"ws3.bin",CRC(ff7f6824),
							"ws4.bin",CRC(74460cf2),
							"ws5.bin",CRC(5ef51ced))
ZAC_ROMEND
CORE_GAMEDEFNV(wsports,"Winter Sports",1978,"Zaccaria",mZAC1311,SOUNDFLAG)

//07/78 House of Diamonds
//09/78 Strike
//10/78 Ski Jump

/*--------------------------------
/ Future World (10/78)
/-------------------------------*/
// Game ROMs #2 and #3 have to be swapped!
INITGAME1(futurwld,0,dispZAC1,SNDBRD_ZAC1311)
ZAC_ROMSTART44444(futurwld,	"futwld_1.lgc",CRC(d83b8793),
							"futwld_3.lgc",CRC(bdcb7e1d),
							"futwld_2.lgc",CRC(48e3d293),
							"futwld_4.lgc",CRC(b1de2120),
							"futwld_5.lgc",CRC(6b7965f2))
ZAC_ROMEND
CORE_GAMEDEFNV(futurwld,"Future World",1978,"Zaccaria",mZAC1311,SOUNDFLAG)

/*--------------------------------
/ Shooting the Rapids (04/79)
/-------------------------------*/
INITGAME1(strapids,0,dispZAC1,SNDBRD_ZAC1125)
ZAC_ROMSTART44444(strapids,	"rapids_1.lgc",CRC(2a30cef3),
							"rapids_2.lgc",CRC(04adaa14),
							"rapids_3.lgc",CRC(397992fb),
							"rapids_4.lgc",CRC(3319fa21),
							"rapids_5.lgc",CRC(0dd67110))
ZAC_ROMEND
CORE_GAMEDEFNV(strapids,"Shooting the Rapids",1979,"Zaccaria",mZAC1125,SOUNDFLAG)

/*--------------------------------
/ Hot Wheels (09/79)
/-------------------------------*/
INITGAME1(hotwheel,0,dispZAC1,SNDBRD_ZAC1125)
ZAC_ROMSTART84444(hotwheel,	"zac_boot.lgc",CRC(62a3da59),
							"htwhls_2.lgc",CRC(7ff870ae),
							"htwhls_3.lgc",CRC(7c1fba91),
							"htwhls_4.lgc",CRC(974804ba),
							"htwhls_5.lgc",CRC(e28f3c60))
ZAC_ROMEND
CORE_GAMEDEFNV(hotwheel,"Hot Wheels",1979,"Zaccaria",mZAC1125,SOUNDFLAG)

//09/79 Space City

/*--------------------------------
/ Fire Mountain (01/80)
/-------------------------------*/
INITGAME1(firemntn,0,dispZAC1,SNDBRD_ZAC1125)
ZAC_ROMSTART84444(firemntn,	"zac_boot.lgc",CRC(62a3da59),
							"firemt_2.lgc",CRC(d146253f),
							"firemt_3.lgc",CRC(d9faae07),
							"firemt_4.lgc",CRC(b5cac3da),
							"firemt_5.lgc",CRC(13f11d84))
ZAC_ROMEND
CORE_GAMEDEFNV(firemntn,"Fire Mountain",1980,"Zaccaria",mZAC1125,SOUNDFLAG)

/*--------------------------------
/ Star God (05/80)
/-------------------------------*/
// After game start, no key will respond anymore?
INITGAME1(stargod,0,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(stargod,	"zac_boot.lgc",CRC(62a3da59),
							"stargod2.lgc",CRC(7a784b03),
							"stargod3.lgc",CRC(95492ac0),
							"stargod4.lgc",CRC(09e5682a),
							"stargod5.lgc",CRC(03cd4e24))
ZAC_SOUNDROM_0(				"stargod.snd", CRC(5079e493))
ZAC_ROMEND
CORE_GAMEDEFNV(stargod,"Star God",1980,"Zaccaria",mZAC1346,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Space Shuttle (09/80)
/-------------------------------*/
INITGAME1(sshtlzac,0,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(sshtlzac,	"zac_boot.lgc",CRC(62a3da59),
							"spcshtl2.lgc",CRC(0e06771b),
							"spcshtl3.lgc",CRC(a302e5a9),
							"spcshtl4.lgc",CRC(a02ee0b5),
							"spcshtl5.lgc",CRC(d1dabd9b))
ZAC_SOUNDROM_0(				"spcshtl.snd", CRC(9a61781c))
ZAC_ROMEND
CORE_GAMEDEFNV(sshtlzac,"Space Shuttle (Zaccaria)",1980,"Zaccaria",mZAC1346,SOUNDFLAG)

/*--------------------------------
/ Earth, Wind & Fire (04/81)
/-------------------------------*/
// Something's wrong (with the rom loading maybe?)
INITGAME1(ewf,0,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(ewf,	"zac_boot.lgc",CRC(62a3da59),
						"ewf_2.lgc",   CRC(aa67e0b4),
						"ewf_3.lgc",   CRC(b21bf015),
						"ewf_4.lgc",   CRC(d110da3f),
						"ewf_5.lgc",   CRC(686c4a4b))
ZAC_SOUNDROM_0(			"stargod.snd", CRC(5079e493))
ZAC_ROMEND
CORE_GAMEDEFNV(ewf,"Earth, Wind & Fire",1981,"Zaccaria",mZAC1346,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Locomotion (09/81)
/-------------------------------*/
INITGAME1(locomotn,0,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84844(locomotn,	"loc-1.fil",  CRC(8d0252a2),
							"loc-2.fil",  CRC(9dbd8601),
							"loc-3.fil",  CRC(8cadea7b),
							"loc-4.fil",  CRC(177c89b6),
							"loc-5.fil",  CRC(cad4122a))
ZAC_SOUNDROM_0(				"loc-snd.fil",CRC(51ea9d2a))
ZAC_ROMEND
CORE_GAMEDEFNV(locomotn,"Locomotion",1981,"Zaccaria",mZAC1,SOUNDFLAG)

/*--------------------------------
/ Pinball Champ '82 (04/82) (Is this really different from the '83?)
/-------------------------------*/
INITGAME(pinchp82,1,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(pinchp82,	"pinchamp.ic1",CRC(1412ec33),
							"pinchamp.ic2",CRC(a24ba4c6),
							"pinchp82.ic3",CRC(ab7a92ac))
ZAC_SOUNDROM_cefg0(			"pbc_1c.snd",  CRC(6e2defe5),
							"pbc_1e.snd",  CRC(703b3cae),
							"pbc_1f.snd",  CRC(f3f4b950),
							"pbc_1g.snd",  CRC(44adae13))
ZAC_ROMEND
CORE_CLONEDEFNV(pinchp82,pinchamp,"Pinball Champ '82 (german speech)",1982,"Zaccaria",mZAC2B,SOUNDFLAG)

/*--------------------------------
/ Soccer King (09/82)
/-------------------------------*/
INITGAME(socrking,1,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(socrking,	"soccer.ic1",CRC(3fbd7c32),
							"soccer.ic2",CRC(0cc0df1f),
							"soccer.ic3",CRC(5da6ea20))
ZAC_SOUNDROM_cefgh(			"sound1.c",  CRC(ca1765bb),
							"sound2.e",  CRC(4e60f05f),
							"sound3.f",  CRC(c393305a),
							"sound4.g",  CRC(ebf990f8),
							"sound5.h",  CRC(1f047bd7))
ZAC_ROMEND
CORE_GAMEDEFNV(socrking,"Soccer King",1982,"Zaccaria",mZAC2A,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Pinball Champ (??/83)
/-------------------------------*/
INITGAME(pinchamp,1,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(pinchamp,	"pinchamp.ic1",CRC(1412ec33),
							"pinchamp.ic2",CRC(a24ba4c6),
							"pinchamp.ic3",CRC(df5f4f88))
ZAC_SOUNDROM_cefg0(			"sound1.c",    CRC(f739fcba),
							"sound2.e",    CRC(24d83e74),
							"sound3.f",    CRC(d055e8c6),
							"sound4.g",    CRC(39b68215))
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC2B,SOUNDFLAG)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,1,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(tmachzac,	"timemach.ic1",CRC(d88f424b),
							"timemach.ic2",CRC(3c313487))
ZAC_SOUNDROM_de1g(			"sound1.d",    CRC(efc1d724),
							"sound2.e",    CRC(41881a1d),
							"sound3.g",    CRC(b7b872da))
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2C,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,1,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(farfalla,	"cpurom1.bin",CRC(ac249150),
							"cpurom2.bin",CRC(6edc823f))
ZAC_SOUNDROM_de1g(			"rom1.snd",   CRC(aca09674),
							"rom2.snd",   CRC(76da384d),
							"rom3.snd",   CRC(d0584952))
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2F,SOUNDFLAG)

/*--------------------------------
/ Devil Riders (04/84)
/-------------------------------*/
INITGAME(dvlrider,1,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(dvlrider,	"cpu.ic1",CRC(5874ab12),
							"cpu.ic2",CRC(09829446))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(5d48462c),
						"gb01snd2.1e",CRC(1127be59),
						"gb01snd3.1g",CRC(1ae91ae8))
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

INITGAME(dvlrideg,1,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(dvlrideg,	"cpu.ic1",CRC(5874ab12),
							"cpu.ic2",CRC(09829446))
ZAC_SOUNDROM_de2g(	"g_snd_1.bin",CRC(77d042dc),
					"g_snd_2.bin",CRC(31e35fd4),
					"g_snd_3.bin",CRC(2e64a401))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrideg,dvlrider,"Devil Riders (german speech)",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

/*--------------------------------
/ Magic Castle (09/84)
/-------------------------------*/
INITGAME(mcastle,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(mcastle,	"cpu.ic1",CRC(50dd8209),
							"cpu.ic2",CRC(11372bec))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(cd6a4a07),
						"gb01snd2.1e",CRC(d289952d),
						"gb01snd3.1g",CRC(8b4342eb))
ZAC_ROMEND
CORE_GAMEDEFNV(mcastle,"Magic Castle",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

INITGAME(mcastleg,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(mcastleg,	"cpu.ic1",CRC(50dd8209),
							"cpu.ic2",CRC(11372bec))
ZAC_SOUNDROM_de2g(	"magic1d.snd",CRC(1f1a1140),
					"magic1e.snd",CRC(a8787011),
					"magic1g.snd",CRC(313fb216))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastleg,mcastle,"Magic Castle (german speech)",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

/*--------------------------------
/ Robot (01/85)
/-------------------------------*/
// There might be a bad rom on this one.
INITGAME(robot,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(robot,	"robot_1.lgc",CRC(96a87432),
						"robot_2.lgc",CRC(28ba9687))
ZAC_SOUNDROM_de2g(		"robot_d.snd",CRC(88685b1e),
						"robot_e.snd",CRC(e326a851),
						"robot_g.snd",CRC(7ed5da55))
ZAC_ROMEND
CORE_GAMEDEFNV(robot,"Robot",1985,"Zaccaria",mZAC2F,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Clown (07/85)
/-------------------------------*/
INITGAME(clown,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(clown,	"clown_1.lgc",CRC(16f09833),
						"clown_2.lgc",CRC(697e6b5b))
ZAC_SOUNDROM_e2f2(		"clown_e.snd",CRC(04a34cc1),
						"clown_f.snd",CRC(e35a4f72))
ZAC_ROMEND
CORE_GAMEDEFNV(clown,"Clown",1985,"Zaccaria",mZAC2X,SOUNDFLAG)

/*--------------------------------
/ Pool Champion (12/85)
/-------------------------------*/
INITGAME(poolcham,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(poolcham,	"poolcham.ic1",CRC(fca2a2b2),
							"poolcham.ic2",CRC(267a2a02))
ZAC_SOUNDROM_f(				"poolcham.1f", CRC(efe33926))
// ZAC_SOUNDROM_f(			"sound1.f",    CRC(b4b4e31e)) // different language?
ZAC_ROMEND
CORE_GAMEDEFNV(poolcham,"Pool Champion",1985,"Zaccaria",mZAC2D,SOUNDFLAG)

/*--------------------------------
/ Black Belt (03/86)
/-------------------------------*/
INITGAME(bbeltzac,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(bbeltzac,	"bbz-1.fil",CRC(2e7e1575),
							"bbz-2.fil",CRC(dbec92ae))
ZAC_SOUNDROM_e2f4(			"bbz-e.snd",CRC(1fe045d2),
							"bbz-f.snd",CRC(9f58f369))
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Black Belt (Zaccaria)",1986,"Zaccaria",mZAC2D,SOUNDFLAG)

/*--------------------------------
/ Mexico '86 (07/86)
/-------------------------------*/
INITGAME(mexico,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(mexico,	"mex86_1.lgc",CRC(60d559b1),
							"mex86_2.lgc",CRC(5c984c15))
ZAC_SOUNDROM_e2f4(			"mex86_e.snd",CRC(a985e8db),
							"mex86_f.snd",CRC(301c2b63))
ZAC_ROMEND
CORE_GAMEDEFNV(mexico,"Mexico '86",1986,"Zaccaria",mZAC2F,SOUNDFLAG)

/*--------------------------------
/ Zankor (12/86)
/-------------------------------*/
INITGAME(zankor,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(zankor,	"zan_ic1.764",CRC(e7ba5acf),
							"zan_ic2.764",CRC(5804ff10))
ZAC_SOUNDROM_e4f4(			"zan_1e.128", CRC(d467000f),
							"zan_1f.128", CRC(74fcadc9))
ZAC_SOUNDROM_456(			"zan_ic4.128",CRC(f34a2aaa),
							"zan_ic5.128",CRC(bf61aab0),
							"zan_ic6.128",CRC(13a5b8d4))
ZAC_ROMEND
CORE_GAMEDEFNV(zankor,"Zankor",1986,"Zaccaria",mZAC2F,SOUNDFLAG)

/*--------------------------------
/ Spooky (04/87)
/-------------------------------*/
INITGAME(spooky,1,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(spooky,	"spook_1.lgc",CRC(377b347d),
							"spook_2.lgc",CRC(ae0598b0))
ZAC_SOUNDROM_e2f4(			"spook_e.snd",CRC(3d632c93),
							"spook_f.snd",CRC(cc04a448))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4),
							"spook_6.snd",CRC(d4320bc7))
ZAC_ROMEND
CORE_GAMEDEFNV(spooky,"Spooky",1987,"Zaccaria",mZAC2X,SOUNDFLAG)

//??/87 Star's Phoenix
//??/86 New Star's Phoenix
