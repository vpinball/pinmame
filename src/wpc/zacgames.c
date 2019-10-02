#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "zac.h"
#include "zacsnd.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "s4.h"

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

#define INITGAME1(name, gen, disp, sb, irq) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,0,0,sb,0,irq}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZACOLD_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

#define INITGAME(name, gen, disp, sb, irq) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,2,0,sb,0,irq}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZAC_INPUT_PORTS_START(name, 1) ZAC_INPUT_PORTS_END

#define INITGAME2(name, gen, disp, sb, irq) \
static core_tGameData name##GameData = {gen,disp,{FLIP_SW(FLIP_L),0,2,0,sb,0,irq}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
ZAC_INPUT_PORTS_NOSNDDIAG_START(name, 1) ZAC_INPUT_PORTS_END

//Games in rough production order

// Generation 1 games below

//10/77 Combat (might use the SC/MP hardware, or not exist at all)

/*--------------------------------
/ Winter Sports (01/78)
/-------------------------------*/
INITGAME1(wsports,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1311,125)
ZAC_ROMSTART44444(wsports,	"ws1.bin",CRC(58feb058) SHA1(50216bba5be28284e63d826543297d1b6b609325),
							"ws2.bin",CRC(ece702cb) SHA1(84cf0976b33bd7cf25976de9c66cc85808f1cd50),
							"ws3.bin",CRC(ff7f6824) SHA1(0eef4aca51c0e823f7634d7fc22c96c590239269),
							"ws4.bin",CRC(74460cf2) SHA1(4afa612af1eff8eae686ceba7c117bc7962272c7),
							"ws5.bin",CRC(5ef51ced) SHA1(390579d0482ceabf87924f7718ef33e336726d92))
ZAC_ROMEND
CORE_GAMEDEFNV(wsports,"Winter Sports",1978,"Zaccaria",mZAC1311,0)

/*--------------------------------
// House of Diamonds (07/78)
/-------------------------------*/
INITGAME1(hod,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1311,125)
ZAC_ROMSTART44444(hod,		"hod_1.bin",CRC(b666af0e) SHA1(e6a96ed30733e7b011ba35d1a628cefd073f29a1),
							"hod_2.bin",CRC(956aac25) SHA1(2a59c3589d14e36ab2c61c6fbc9e8212410a385b),
							"hod_3.bin",CRC(88b05360) SHA1(44992a01eaa8f58296d6fb003da8dad528f2b937),
							"hod_4.bin",CRC(25b6be1f) SHA1(351138404865d69ccb3ad450deda0776e987fdd2),
							"hod_5.bin",CRC(81b73c40) SHA1(21b80cff132becdb028e6ee895231da635189ef4))
ZAC_ROMEND
CORE_GAMEDEFNV(hod,"House of Diamonds",1978,"Zaccaria",mZAC1311,0)

/*--------------------------------
/ Future World (10/78)
/-------------------------------*/
INITGAME1(futurwld,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1311,125)
ZAC_ROMSTART44444(futurwld,	"futwld_1.lgc",CRC(d83b8793) SHA1(3bb04d8395191ecf324b6da0bcddcf7bd8d41867),
							"futwld_2.lgc",CRC(bdcb7e1d) SHA1(e6c0c7e8188df87937f0b22dbb0639872e03e948),
							"futwld_3.lgc",CRC(48e3d293) SHA1(0029f30c4a94067e7782e22499b11db86f051934),
							"futwld_4.lgc",CRC(b1de2120) SHA1(970e1c4eadb7ace1398684accac289a434d13d84),
							"futwld_5.lgc",CRC(6b7965f2) SHA1(31314bc63f01717004c5c2448b5db7d292145b60))
ZAC_ROMEND
CORE_GAMEDEFNV(futurwld,"Future World",1978,"Zaccaria",mZAC1311,0)

/*--------------------------------
/ Shooting the Rapids (04/79)
/-------------------------------*/
INITGAME1(strapids,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125,125)
ZAC_ROMSTART44444(strapids,	"rapids_1.lgc",CRC(2a30cef3) SHA1(1af0ad08316fca565a6de1d308ed0495907656e7),
							"rapids_2.lgc",CRC(04adaa14) SHA1(7819de53cee669b7e42624cd577ed1e3b771d2a9),
							"rapids_3.lgc",CRC(397992fb) SHA1(46e4f293fc8d8094eb16030261342504694fbf8f),
							"rapids_4.lgc",CRC(3319fa21) SHA1(b384a7347e0d6ca3bec53f356312b66d66b5b03f),
							"rapids_5.lgc",CRC(0dd67110) SHA1(0c32e400ef07d7243148ae280e145a3e050313e8))
ZAC_ROMEND
CORE_GAMEDEFNV(strapids,"Shooting the Rapids",1979,"Zaccaria",mZAC1125,SOUNDFLAG)

/*--------------------------------
/ Hot Wheels (09/79)
/-------------------------------*/
INITGAME1(hotwheel,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125,125)
ZAC_ROMSTART84444(hotwheel,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"htwhls_2.lgc",CRC(7ff870ae) SHA1(274ee7c2cb92b6710c546058e7277f06720b5e37),
							"htwhls_3.lgc",CRC(7c1fba91) SHA1(d514e9b3128dfe7999e414fd9044dc20c0d76c66),
							"htwhls_4.lgc",CRC(974804ba) SHA1(f35c1b52327b2d3170a9a28dbee4d1437f1f594a),
							"htwhls_5.lgc",CRC(e28f3c60) SHA1(eb780be60b41017d105288cef71906d15474b8fa))
ZAC_ROMEND
CORE_GAMEDEFNV(hotwheel,"Hot Wheels",1979,"Zaccaria",mZAC1125,SOUNDFLAG)

/*--------------------------------
/ Fire Mountain (01/80)
/-------------------------------*/
INITGAME1(firemntn,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125,125)
ZAC_ROMSTART84444(firemntn,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"firemt_2.lgc",CRC(d146253f) SHA1(69910ddd1b7f1a0a0db689e750a0288d10e92951),
							"firemt_3.lgc",CRC(d9faae07) SHA1(9883be01e2d359a111528029407141c9792c3583),
							"firemt_4.lgc",CRC(b5cac3da) SHA1(94f1153571a099574d041a5168854056a692a03d),
							"firemt_5.lgc",CRC(13f11d84) SHA1(031f43467a4a01810297e3bfe0762ed2eed4e251))
ZAC_ROMEND
CORE_GAMEDEFNV(firemntn,"Fire Mountain",1980,"Zaccaria",mZAC1125,SOUNDFLAG)

/*--------------------------------
/ Star God (05/80)
/-------------------------------*/
INITGAME1(stargod,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125,125)
ZAC_ROMSTART84444(stargod,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"stargod2.lgc",CRC(7a784b03) SHA1(bc3490b69913f52e3e9db5c3de5617ab89efe073),
							"stargod3.lgc",CRC(95492ac0) SHA1(992ad53efc5b53020e3939dfca5431fd50b6571c),
							"stargod4.lgc",CRC(09e5682a) SHA1(c9fcad4f55ee005e204a49fa65e7d77ecfde9680),
							"stargod5.lgc",CRC(43ba2462) SHA1(6749bdceca4a1dc2bc90d7ee3b671f52219e1af4))
ZAC_ROMEND
CORE_GAMEDEFNV(stargod,"Star God",1980,"Zaccaria",mZAC1125,SOUNDFLAG)

INITGAME1(stargoda,GEN_ZAC1,dispZAC1,SNDBRD_S67S,125)
ZAC_ROMSTART84444(stargoda,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"stargod2.lgc",CRC(7a784b03) SHA1(bc3490b69913f52e3e9db5c3de5617ab89efe073),
							"stargod3.lgc",CRC(95492ac0) SHA1(992ad53efc5b53020e3939dfca5431fd50b6571c),
							"stargod4.lgc",CRC(09e5682a) SHA1(c9fcad4f55ee005e204a49fa65e7d77ecfde9680),
							"stargod5.lgc",CRC(43ba2462) SHA1(6749bdceca4a1dc2bc90d7ee3b671f52219e1af4))
S67S_SOUNDROMS8(			"stargod.snd", CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
ZAC_ROMEND
CORE_CLONEDEFNV(stargoda,stargod,"Star God (alternate sound)",1980,"Zaccaria",mZAC1144,0)

INITGAME1(stargodb,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125,125)
ZAC_ROMSTART84444(stargodb,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"stargod.ic2",CRC(a92ae202) SHA1(b5ed61f0c0c769f0bd4f5a69677d0eb5122bdb56),
							"stargod.ic3",CRC(e677cf0d) SHA1(6aff0275148ccc56f2ac1bf5e5bd5baed64bfa7c),
							"stargod.ic4",CRC(fdfbb31f) SHA1(b64a529a097a7e2589ff124998160d375153d16c),
							"stargod.ic5",CRC(536484f8) SHA1(7c40bf7e8b5b21cce44d96633581730ea9eeb176))
ZAC_ROMEND
CORE_CLONEDEFNV(stargodb,stargod,"Star God (alternate version)",1980,"Zaccaria",mZAC1125,0)

/*--------------------------------
/ Space Shuttle (09/80)
/-------------------------------*/
INITGAME1(sshtlzac,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346,125)
ZAC_ROMSTART84444(sshtlzac,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"spcshtl2.lgc",CRC(0e06771b) SHA1(f30f3727f24219e5047c871fe81c2e172a17cd38),
							"spcshtl3.lgc",CRC(a302e5a9) SHA1(1585f4000d105a7a2be5638ade9ab8668e6c8a5e),
							"spcshtl4.lgc",CRC(a02ee0b5) SHA1(50532bdc347ecfdbd4cc43403ff2cb1dcb1fe1ac),
							"spcshtl5.lgc",CRC(d1dabd9b) SHA1(0d28336764f43fa4d1b23d849b6ec0f60b2b4ecf))
ZAC_SOUNDROM_0(				"spcshtl.snd", CRC(9a61781c) SHA1(0293640653d8cc9532debd31bbb70f025b4e6d03))
ZAC_ROMEND
CORE_GAMEDEFNV(sshtlzac,"Space Shuttle (Zaccaria)",1980,"Zaccaria",mZAC1346,0)

/*--------------------------------
/ Earth, Wind & Fire (04/81)
/-------------------------------*/
INITGAME1(ewf,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346,125)
ZAC_ROMSTART84444(ewf,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
						"ewf_2.lgc",   CRC(aa67e0b4) SHA1(4491eff7081fd5e397974fac1156992ce2012d0b),
						"ewf_3.lgc",   CRC(b21bf015) SHA1(ecddfe1d6797c39e094a7f86efabf0abea0fa4af),
						"ewf_4.lgc",   CRC(d110da3f) SHA1(88e27347d209fab5be924f95b0a001476ea92c1f),
						"ewf_5.lgc",   CRC(f695dab6) SHA1(48ca60718cea40baa5052f690c8d69eb7ab32b0e))
ZAC_SOUNDROM_0(			"ewf.snd",     CRC(5079e493) SHA1(51d366cdd09ad00b8b016b0ea1c85ac95ef94d71))
ZAC_ROMEND
CORE_GAMEDEFNV(ewf,"Earth, Wind & Fire",1981,"Zaccaria",mZAC1346,0)

/*--------------------------------
/ Locomotion (09/81)
/-------------------------------*/
INITGAME1(locomotn,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346,125)
ZAC_ROMSTART84844(locomotn,	"loc-1.fil",  CRC(8d0252a2) SHA1(964dca642fb26eef2c132eca354a0ffce32e25df),
							"loc-2.fil",  CRC(9dbd8601) SHA1(10bc37d2691c7237a14e0718febed2aa7822db23),
							"loc-3.fil",  CRC(8cadea7b) SHA1(e712add828dd22a2b495f0479f949748db21fbf7),
							"loc-4.fil",  CRC(177c89b6) SHA1(23de8208dbbf141952a974514fc752ed2eb6b202),
							"loc-5.fil",  CRC(cad4122a) SHA1(df29914adeb9675abbd9f43dbef23adf2fe96c81))
ZAC_SOUNDROM_0(				"loc-snd.fil",CRC(51ea9d2a) SHA1(9a68687af2c1cad2a261f61a67a625d906c502e1))
ZAC_ROMEND
CORE_GAMEDEFNV(locomotn,"Locomotion",1981,"Zaccaria",mZAC1146,SOUNDFLAG)

// Generation 2 games below

// Pinball Champ '82 (04/82) - using the same roms as Pinball Champ

/*--------------------------------
/ Soccer Kings (09/82)
/-------------------------------*/
INITGAME(socrking,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(socrking,	"soccer.ic1",CRC(3fbd7c32) SHA1(2f56f67d1ad987638284000cca1e20ff17fcd4f9),
							"soccer.ic2",CRC(0cc0df1f) SHA1(2fd05af0ec63835a8f69fdc50e2faceb829b4df2),
							"soccer.ic3",CRC(72caac2c) SHA1(7d63e0cf699365ee1787004d6155646e715b672e))
ZAC_SOUNDROM_cefg1(			"sound1.c",  CRC(3aa95018) SHA1(5347c3aefb642fc5cabd9d5e61fe6515a2dcb2aa),
							"sound2.e",  CRC(f9b57fd6) SHA1(50e42ed349680211eedf55ae639dbae899f3c6da),
							"sound3.f",  CRC(551566e6) SHA1(350432dbc0d6f55404cae970524a0dfda15d8aa0),
							"sound4.g",  CRC(720593fb) SHA1(93aa9ae1be299548e17b4fe97a7fb4ddab76de40))
ZAC_ROMEND
CORE_GAMEDEFNV(socrking,"Soccer Kings",1982,"Zaccaria",mZAC2A,0)

INITGAME(socrkina,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(socrkina,	"soccer.ic1",CRC(3fbd7c32) SHA1(2f56f67d1ad987638284000cca1e20ff17fcd4f9),
							"soccer.ic2",CRC(0cc0df1f) SHA1(2fd05af0ec63835a8f69fdc50e2faceb829b4df2),
							"soccer.1c3",CRC(5da6ea20) SHA1(c0e740e20a968ee58e0cd12a0089bb047f5e2cbb))
ZAC_SOUNDROM_cefg1(			"sound1.c",  CRC(3aa95018) SHA1(5347c3aefb642fc5cabd9d5e61fe6515a2dcb2aa),
							"sound2.e",  CRC(f9b57fd6) SHA1(50e42ed349680211eedf55ae639dbae899f3c6da),
							"sound3.f",  CRC(551566e6) SHA1(350432dbc0d6f55404cae970524a0dfda15d8aa0),
							"sound4.g",  CRC(720593fb) SHA1(93aa9ae1be299548e17b4fe97a7fb4ddab76de40))
ZAC_ROMEND
CORE_CLONEDEFNV(socrkina,socrking,"Soccer Kings (alternate set)",1982,"Zaccaria",mZAC2A,0)

INITGAME(socrkngi,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(socrkngi,	"soccer.ic1",CRC(3fbd7c32) SHA1(2f56f67d1ad987638284000cca1e20ff17fcd4f9),
							"soccer.ic2",CRC(0cc0df1f) SHA1(2fd05af0ec63835a8f69fdc50e2faceb829b4df2),
							"soccer.ic3",CRC(72caac2c) SHA1(7d63e0cf699365ee1787004d6155646e715b672e))
ZAC_SOUNDROM_cefg1(			"sking_it.1c",CRC(2965643f) SHA1(06de48e7afe1004ad27b805ab4b5111ef5db4380),
							"sking_it.1e",CRC(f70ae48f) SHA1(c7aec7b54ae298d833f79f041dd9b08ec3e0ccb4),
							"sking_it.1f",CRC(1b817503) SHA1(6efbb2c5cfeb5286d82155a4b506a2c347aebad8),
							"sking_it.1g",CRC(853a3cbc) SHA1(26d9273bc5cddd47daf88432bf8118e94334a6c1))
ZAC_ROMEND
CORE_CLONEDEFNV(socrkngi,socrking,"Soccer Kings (Italian Speech)",1982,"Zaccaria",mZAC2A,0)

INITGAME(socrkngg,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(socrkngg,	"soccer.ic1",CRC(3fbd7c32) SHA1(2f56f67d1ad987638284000cca1e20ff17fcd4f9),
							"soccer.ic2",CRC(0cc0df1f) SHA1(2fd05af0ec63835a8f69fdc50e2faceb829b4df2),
							"soccer.ic3",CRC(72caac2c) SHA1(7d63e0cf699365ee1787004d6155646e715b672e))
ZAC_SOUNDROM_cefgh(			"sk-de1.c",  CRC(702e3e67) SHA1(ad4c02ef480d3923eebaedb12851018146740558),
							"sk-de2.e",  CRC(b60eddb5) SHA1(7e335315d0b91fc67888cda644dabafdef1afa19),
							"sk-de3.f",  CRC(2f72a94e) SHA1(912ef1e2878b61edff88e5cc1ec19d1b22d44f2d),
							"sk-de4.g",  CRC(23adcc78) SHA1(c25185c08377286c04c43fa2156245a71fc68e2e),
							"sk-de5.h",  CRC(c6f0302d) SHA1(c57d36f3bc3a7e3a056b930b8e11b4cee4af0558))
ZAC_ROMEND
CORE_CLONEDEFNV(socrkngg,socrking,"Soccer Kings (German Speech)",1982,"Zaccaria",mZAC2A,0)

INITGAME(sockfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(sockfp,	"socfp.ic1",CRC(8053055b) SHA1(5fce1179605337da767f1f26563a564f77a39cf6),
							"socfp.ic2",CRC(da22663a) SHA1(ceeb1593a91e5f8476ad11cf7998c7d239772034))
ZAC_SOUNDROM_cefg1(			"sound1.c",  CRC(3aa95018) SHA1(5347c3aefb642fc5cabd9d5e61fe6515a2dcb2aa),
							"sound2.e",  CRC(f9b57fd6) SHA1(50e42ed349680211eedf55ae639dbae899f3c6da),
							"sound3.f",  CRC(551566e6) SHA1(350432dbc0d6f55404cae970524a0dfda15d8aa0),
							"sound4.g",  CRC(720593fb) SHA1(93aa9ae1be299548e17b4fe97a7fb4ddab76de40))
ZAC_ROMEND
CORE_CLONEDEFNV(sockfp,socrking,"Soccer Kings (Free Play)",1982,"Zaccaria",mZAC2A,0)

INITGAME(sockifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(sockifp,	"socfp.ic1",CRC(8053055b) SHA1(5fce1179605337da767f1f26563a564f77a39cf6),
							"socfp.ic2",CRC(da22663a) SHA1(ceeb1593a91e5f8476ad11cf7998c7d239772034))
ZAC_SOUNDROM_cefg1(			"sking_it.1c",CRC(2965643f) SHA1(06de48e7afe1004ad27b805ab4b5111ef5db4380),
							"sking_it.1e",CRC(f70ae48f) SHA1(c7aec7b54ae298d833f79f041dd9b08ec3e0ccb4),
							"sking_it.1f",CRC(1b817503) SHA1(6efbb2c5cfeb5286d82155a4b506a2c347aebad8),
							"sking_it.1g",CRC(853a3cbc) SHA1(26d9273bc5cddd47daf88432bf8118e94334a6c1))
ZAC_ROMEND
CORE_CLONEDEFNV(sockifp,socrking,"Soccer Kings (Italian Speech, Free Play)",1982,"Zaccaria",mZAC2A,0)

INITGAME(sockgfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(sockgfp,	"socfp.ic1",CRC(8053055b) SHA1(5fce1179605337da767f1f26563a564f77a39cf6),
							"socfp.ic2",CRC(da22663a) SHA1(ceeb1593a91e5f8476ad11cf7998c7d239772034))
ZAC_SOUNDROM_cefgh(			"sk-de1.c",  CRC(702e3e67) SHA1(ad4c02ef480d3923eebaedb12851018146740558),
							"sk-de2.e",  CRC(b60eddb5) SHA1(7e335315d0b91fc67888cda644dabafdef1afa19),
							"sk-de3.f",  CRC(2f72a94e) SHA1(912ef1e2878b61edff88e5cc1ec19d1b22d44f2d),
							"sk-de4.g",  CRC(23adcc78) SHA1(c25185c08377286c04c43fa2156245a71fc68e2e),
							"sk-de5.h",  CRC(c6f0302d) SHA1(c57d36f3bc3a7e3a056b930b8e11b4cee4af0558))
ZAC_ROMEND
CORE_CLONEDEFNV(sockgfp,socrking,"Soccer Kings (German Speech, Free Play)",1982,"Zaccaria",mZAC2A,0)

/*--------------------------------
/ Pinball Champ (04/83)
/-------------------------------*/
INITGAME(pinchamp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pinchamp,	"pinchamp.ic1",CRC(1412ec33) SHA1(82c158ec0536f76cbe80e8c12e0047579439a5b7),
							"pinchamp.ic2",CRC(a24ba4c6) SHA1(4f02c4d6cd727fa96a68c72012b0b4a4484397c4),
							"pinchamp.ic3",CRC(df5f4f88) SHA1(249cf958b0998aa41fa26c617be9b6c52c2f5549))
ZAC_SOUNDROM_cefg0(			"pchmp_gb.1c", CRC(f739fcba) SHA1(7460f1da99c474601e8cec64683cbd61837a82e8),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_gb.1f", CRC(d055e8c6) SHA1(0820d941880aa8925b400c792af7ce6b80dcbc48),
							"pchmp_gb.1g", CRC(39b68215) SHA1(4d57f1f1f71f7bdbef67ca4cc62cfde80d1ab04c))
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC2A,0)

INITGAME(pinchamg,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pinchamg,	"pinchamp.ic1",CRC(1412ec33) SHA1(82c158ec0536f76cbe80e8c12e0047579439a5b7),
							"pinchamp.ic2",CRC(a24ba4c6) SHA1(4f02c4d6cd727fa96a68c72012b0b4a4484397c4),
							"pinchamp.ic3",CRC(df5f4f88) SHA1(249cf958b0998aa41fa26c617be9b6c52c2f5549))
ZAC_SOUNDROM_cefg0(			"pchmp_de.1c", CRC(6e2defe5) SHA1(fcb62da1aed23d9fb9a222862b4b772aad9792a1),
							"pchmp_de.1e", CRC(703b3cae) SHA1(c7bd021e936fb0fd4bc16d48c3ef1df69d1fe01a),
							"pchmp_de.1f", CRC(f3f4b950) SHA1(ed5c02f701530d2d6255cc72d695e24d4df40fc3),
							"pchmp_de.1g", CRC(44adae13) SHA1(0d8d538704db62b41ad5781ec53c34e482342025))
ZAC_ROMEND
CORE_CLONEDEFNV(pinchamg,pinchamp,"Pinball Champ (German Speech)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pinchami,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pinchami,	"pinchamp.ic1",CRC(1412ec33) SHA1(82c158ec0536f76cbe80e8c12e0047579439a5b7),
							"pinchamp.ic2",CRC(a24ba4c6) SHA1(4f02c4d6cd727fa96a68c72012b0b4a4484397c4),
							"pinchamp.ic3",CRC(df5f4f88) SHA1(249cf958b0998aa41fa26c617be9b6c52c2f5549))
ZAC_SOUNDROM_cefg1(			"pchmp_it.1c", CRC(a0033b90) SHA1(bca8fe29fdfcbc22fd0e8bafbd7946db5c2c4041),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_it.1f", CRC(5555f341) SHA1(8aa27d17711f4162c9d10f60afba7f823112bfe0),
							"pchmp_it.1g", CRC(2561579b) SHA1(a280cd81f58a17601adfa9ce17f225111c7d9f95))
ZAC_ROMEND
CORE_CLONEDEFNV(pinchami,pinchamp,"Pinball Champ (Italian Speech)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincham7,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pincham7,	"pblchmp7.ic1",CRC(f050b7fa) SHA1(918bdfd77e785c546202c29b1e296ca5f683ca66),
							"pblchmp7.ic2",CRC(cbcb63c7) SHA1(c15329482f02614185adcd0475a02c667cadfc98),
							"pblchmp7.ic3",CRC(54abff9c) SHA1(925c7c1fb903bd6069aee1967c75eb8e61ecf591))
ZAC_SOUNDROM_cefg0(			"pchmp_gb.1c", CRC(f739fcba) SHA1(7460f1da99c474601e8cec64683cbd61837a82e8),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_gb.1f", CRC(d055e8c6) SHA1(0820d941880aa8925b400c792af7ce6b80dcbc48),
							"pchmp_gb.1g", CRC(39b68215) SHA1(4d57f1f1f71f7bdbef67ca4cc62cfde80d1ab04c))
ZAC_ROMEND
CORE_CLONEDEFNV(pincham7,pinchamp,"Pinball Champ (7-digits)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincha7g,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pincha7g,	"pblchmp7.ic1",CRC(f050b7fa) SHA1(918bdfd77e785c546202c29b1e296ca5f683ca66),
							"pblchmp7.ic2",CRC(cbcb63c7) SHA1(c15329482f02614185adcd0475a02c667cadfc98),
							"pblchmp7.ic3",CRC(54abff9c) SHA1(925c7c1fb903bd6069aee1967c75eb8e61ecf591))
ZAC_SOUNDROM_cefg0(			"pchmp_de.1c", CRC(6e2defe5) SHA1(fcb62da1aed23d9fb9a222862b4b772aad9792a1),
							"pchmp_de.1e", CRC(703b3cae) SHA1(c7bd021e936fb0fd4bc16d48c3ef1df69d1fe01a),
							"pchmp_de.1f", CRC(f3f4b950) SHA1(ed5c02f701530d2d6255cc72d695e24d4df40fc3),
							"pchmp_de.1g", CRC(44adae13) SHA1(0d8d538704db62b41ad5781ec53c34e482342025))
ZAC_ROMEND
CORE_CLONEDEFNV(pincha7g,pinchamp,"Pinball Champ (7-digits, German Speech)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincha7i,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART000(pincha7i,	"pblchmp7.ic1",CRC(f050b7fa) SHA1(918bdfd77e785c546202c29b1e296ca5f683ca66),
							"pblchmp7.ic2",CRC(cbcb63c7) SHA1(c15329482f02614185adcd0475a02c667cadfc98),
							"pblchmp7.ic3",CRC(54abff9c) SHA1(925c7c1fb903bd6069aee1967c75eb8e61ecf591))
ZAC_SOUNDROM_cefg1(			"pchmp_it.1c", CRC(a0033b90) SHA1(bca8fe29fdfcbc22fd0e8bafbd7946db5c2c4041),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_it.1f", CRC(5555f341) SHA1(8aa27d17711f4162c9d10f60afba7f823112bfe0),
							"pchmp_it.1g", CRC(2561579b) SHA1(a280cd81f58a17601adfa9ce17f225111c7d9f95))
ZAC_ROMEND
CORE_CLONEDEFNV(pincha7i,pinchamp,"Pinball Champ (7-digits, Italian Speech)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pincfp,	"pincfp.ic1",CRC(174ff0b2) SHA1(8e4d06c140724fef96fe357bb584c3b25d22eac6),
							"pincfp.ic2",CRC(5d3ff552) SHA1(926daa3c8cf2acfd481d9fec99ef24285f9b8951))
ZAC_SOUNDROM_cefg0(			"pchmp_gb.1c", CRC(f739fcba) SHA1(7460f1da99c474601e8cec64683cbd61837a82e8),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_gb.1f", CRC(d055e8c6) SHA1(0820d941880aa8925b400c792af7ce6b80dcbc48),
							"pchmp_gb.1g", CRC(39b68215) SHA1(4d57f1f1f71f7bdbef67ca4cc62cfde80d1ab04c))
ZAC_ROMEND
CORE_CLONEDEFNV(pincfp,pinchamp,"Pinball Champ (Free Play)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincgfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pincgfp,	"pincfp.ic1",CRC(174ff0b2) SHA1(8e4d06c140724fef96fe357bb584c3b25d22eac6),
							"pincfp.ic2",CRC(5d3ff552) SHA1(926daa3c8cf2acfd481d9fec99ef24285f9b8951))
ZAC_SOUNDROM_cefg0(			"pchmp_de.1c", CRC(6e2defe5) SHA1(fcb62da1aed23d9fb9a222862b4b772aad9792a1),
							"pchmp_de.1e", CRC(703b3cae) SHA1(c7bd021e936fb0fd4bc16d48c3ef1df69d1fe01a),
							"pchmp_de.1f", CRC(f3f4b950) SHA1(ed5c02f701530d2d6255cc72d695e24d4df40fc3),
							"pchmp_de.1g", CRC(44adae13) SHA1(0d8d538704db62b41ad5781ec53c34e482342025))
ZAC_ROMEND
CORE_CLONEDEFNV(pincgfp,pinchamp,"Pinball Champ (German Speech, Free Play)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pincifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pincifp,	"pincfp.ic1",CRC(174ff0b2) SHA1(8e4d06c140724fef96fe357bb584c3b25d22eac6),
							"pincfp.ic2",CRC(5d3ff552) SHA1(926daa3c8cf2acfd481d9fec99ef24285f9b8951))
ZAC_SOUNDROM_cefg1(			"pchmp_it.1c", CRC(a0033b90) SHA1(bca8fe29fdfcbc22fd0e8bafbd7946db5c2c4041),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_it.1f", CRC(5555f341) SHA1(8aa27d17711f4162c9d10f60afba7f823112bfe0),
							"pchmp_it.1g", CRC(2561579b) SHA1(a280cd81f58a17601adfa9ce17f225111c7d9f95))
ZAC_ROMEND
CORE_CLONEDEFNV(pincifp,pinchamp,"Pinball Champ (Italian Speech, Free Play)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pinc7fp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pinc7fp,	"pinc7fp.ic1",CRC(8bff8868) SHA1(16b525995fc584e226b8499b914e21059ac80416),
							"pinc7fp.ic2",CRC(5f75b984) SHA1(276017cc54f4ac253478383642ce32929dfc8d92))
ZAC_SOUNDROM_cefg0(			"pchmp_gb.1c", CRC(f739fcba) SHA1(7460f1da99c474601e8cec64683cbd61837a82e8),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_gb.1f", CRC(d055e8c6) SHA1(0820d941880aa8925b400c792af7ce6b80dcbc48),
							"pchmp_gb.1g", CRC(39b68215) SHA1(4d57f1f1f71f7bdbef67ca4cc62cfde80d1ab04c))
ZAC_ROMEND
CORE_CLONEDEFNV(pinc7fp,pinchamp,"Pinball Champ (7-digits, Free Play)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pinc7gfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pinc7gfp,	"pinc7fp.ic1",CRC(8bff8868) SHA1(16b525995fc584e226b8499b914e21059ac80416),
							"pinc7fp.ic2",CRC(5f75b984) SHA1(276017cc54f4ac253478383642ce32929dfc8d92))
ZAC_SOUNDROM_cefg0(			"pchmp_de.1c", CRC(6e2defe5) SHA1(fcb62da1aed23d9fb9a222862b4b772aad9792a1),
							"pchmp_de.1e", CRC(703b3cae) SHA1(c7bd021e936fb0fd4bc16d48c3ef1df69d1fe01a),
							"pchmp_de.1f", CRC(f3f4b950) SHA1(ed5c02f701530d2d6255cc72d695e24d4df40fc3),
							"pchmp_de.1g", CRC(44adae13) SHA1(0d8d538704db62b41ad5781ec53c34e482342025))
ZAC_ROMEND
CORE_CLONEDEFNV(pinc7gfp,pinchamp,"Pinball Champ (7-digits, German Speech, Free Play)",1983,"Zaccaria",mZAC2A,0)

INITGAME(pinc7ifp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC1370,366)
ZAC_ROMSTART1820(pinc7ifp,	"pinc7fp.ic1",CRC(8bff8868) SHA1(16b525995fc584e226b8499b914e21059ac80416),
							"pinc7fp.ic2",CRC(5f75b984) SHA1(276017cc54f4ac253478383642ce32929dfc8d92))
ZAC_SOUNDROM_cefg1(			"pchmp_it.1c", CRC(a0033b90) SHA1(bca8fe29fdfcbc22fd0e8bafbd7946db5c2c4041),
							"pchmp_gb.1e", CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"pchmp_it.1f", CRC(5555f341) SHA1(8aa27d17711f4162c9d10f60afba7f823112bfe0),
							"pchmp_it.1g", CRC(2561579b) SHA1(a280cd81f58a17601adfa9ce17f225111c7d9f95))
ZAC_ROMEND
CORE_CLONEDEFNV(pinc7ifp,pinchamp,"Pinball Champ (7-digits, Italian Speech, Free Play)",1983,"Zaccaria",mZAC2A,0)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366) // sound board is equipped with a TMS5200 chip instead of a 5220!
ZAC_ROMSTART1820(tmachzac,	"timemach.ic1",CRC(d88f424b) SHA1(a0c51f894d604504253f66e49298a9d836e25308),
							"timemach.ic2",CRC(3c313487) SHA1(17c6c4a0c0c6dd90cf7fd9298b945305f734747d))
ZAC_SOUNDROM_de1g(			"sound1.d",    CRC(efc1d724) SHA1(f553767c053e4854fe7839f8c8f4a7f5aefe2692),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"sound3.g",    CRC(b7b872da) SHA1(dfeb48a683c6d249101488f244b26509a4c4d81d))
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2X,0)

INITGAME(tmacgzac,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(tmacgzac,	"timemach.ic1",CRC(d88f424b) SHA1(a0c51f894d604504253f66e49298a9d836e25308),
							"timemach.ic2",CRC(3c313487) SHA1(17c6c4a0c0c6dd90cf7fd9298b945305f734747d))
ZAC_SOUNDROM_de1g(			"tmach_de.1d", CRC(8e8c27a4) SHA1(2e418e509bc241c193564e926583b09582944233),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"tmach_de.1g", CRC(06cba6e4) SHA1(c6ebd9170943da9f74944ada5c7ebd0929e627d0))
ZAC_ROMEND
CORE_CLONEDEFNV(tmacgzac,tmachzac,"Time Machine (Zaccaria, German Speech)",1983,"Zaccaria",mZAC2X,0)

INITGAME(tmacfzac,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(tmacfzac,	"timemach.ic1",CRC(d88f424b) SHA1(a0c51f894d604504253f66e49298a9d836e25308),
							"timemach.ic2",CRC(3c313487) SHA1(17c6c4a0c0c6dd90cf7fd9298b945305f734747d))
ZAC_SOUNDROM_de1g(			"tmach_fr.1d", CRC(831203a4) SHA1(fc60086c2b9b83a47f30b028e7512090658c5700),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"tmach_fr.1g", CRC(4fb43fa3) SHA1(35ef929976e16abef9e70e569a6c005fd7995a6b))
ZAC_ROMEND
CORE_CLONEDEFNV(tmacfzac,tmachzac,"Time Machine (Zaccaria, French Speech)",1983,"Zaccaria",mZAC2X,0)

INITGAME(tmachfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(tmachfp,	"timemfp.ic1",CRC(8df8c995) SHA1(a29ab06ac6b8cf7d7c5bd650f16926e87e2c8fc1),
							"timemfp.ic2",CRC(4f291dc5) SHA1(f6ec713e5a737d34ca42142f3f6cad0edfd7b908))
ZAC_SOUNDROM_de1g(			"sound1.d",    CRC(efc1d724) SHA1(f553767c053e4854fe7839f8c8f4a7f5aefe2692),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"sound3.g",    CRC(b7b872da) SHA1(dfeb48a683c6d249101488f244b26509a4c4d81d))
ZAC_ROMEND
CORE_CLONEDEFNV(tmachfp,tmachzac,"Time Machine (Zaccaria, Free Play)",1983,"Zaccaria",mZAC2X,0)

INITGAME(tmacgfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(tmacgfp,	"timemfp.ic1",CRC(8df8c995) SHA1(a29ab06ac6b8cf7d7c5bd650f16926e87e2c8fc1),
							"timemfp.ic2",CRC(4f291dc5) SHA1(f6ec713e5a737d34ca42142f3f6cad0edfd7b908))
ZAC_SOUNDROM_de1g(			"tmach_de.1d", CRC(8e8c27a4) SHA1(2e418e509bc241c193564e926583b09582944233),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"tmach_de.1g", CRC(06cba6e4) SHA1(c6ebd9170943da9f74944ada5c7ebd0929e627d0))
ZAC_ROMEND
CORE_CLONEDEFNV(tmacgfp,tmachzac,"Time Machine (Zaccaria, German Speech, Free Play)",1983,"Zaccaria",mZAC2X,0)

INITGAME(tmacffp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(tmacffp,	"timemfp.ic1",CRC(8df8c995) SHA1(a29ab06ac6b8cf7d7c5bd650f16926e87e2c8fc1),
							"timemfp.ic2",CRC(4f291dc5) SHA1(f6ec713e5a737d34ca42142f3f6cad0edfd7b908))
ZAC_SOUNDROM_de1g(			"tmach_fr.1d", CRC(831203a4) SHA1(fc60086c2b9b83a47f30b028e7512090658c5700),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"tmach_fr.1g", CRC(4fb43fa3) SHA1(35ef929976e16abef9e70e569a6c005fd7995a6b))
ZAC_ROMEND
CORE_CLONEDEFNV(tmacffp,tmachzac,"Time Machine (Zaccaria, French Speech, Free Play)",1983,"Zaccaria",mZAC2X,0)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farfalla,	"cpurom1.bin",CRC(ac249150) SHA1(9eac1bf6119cd1fa6cc823faf02b9bf153519a77),
							"cpurom2.bin",CRC(6edc823f) SHA1(b10fcbc308ec06762a2eb35921a7e6a68fd5c9b1))
ZAC_SOUNDROM_de1g(			"rom1.snd",   CRC(aca09674) SHA1(8e1edc25c7fe2189215f73da8f1bec4b670bd8e6),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"rom3.snd",   CRC(d0584952) SHA1(80fe571a2e8a2a34fae03589df930b3eb3fa1f6b))
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2X,0)

INITGAME(farfalli,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farfalli,	"cpurom1.bin",CRC(ac249150) SHA1(9eac1bf6119cd1fa6cc823faf02b9bf153519a77),
							"cpurom2.bin",CRC(6edc823f) SHA1(b10fcbc308ec06762a2eb35921a7e6a68fd5c9b1))
ZAC_SOUNDROM_de1g(			"farsnd1.bin",CRC(fd80040d) SHA1(122c99627d944b253e091b56d32336367df615c1),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"farsnd3.bin",CRC(b58618c2) SHA1(89330ee928b5a5f99d50f1150c94732775907fd8))
ZAC_ROMEND
CORE_CLONEDEFNV(farfalli,farfalla,"Farfalla (Italian Speech)",1983,"Zaccaria",mZAC2X,0)

INITGAME(farfallg,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farfallg,	"cpurom1.bin",CRC(ac249150) SHA1(9eac1bf6119cd1fa6cc823faf02b9bf153519a77),
							"cpurom2.bin",CRC(6edc823f) SHA1(b10fcbc308ec06762a2eb35921a7e6a68fd5c9b1))
ZAC_SOUNDROM_de1g(			"farf_de.1d", CRC(5f64df81) SHA1(d8bd6d1fb3eec704fe31ccc1feeb5a9529c70d07),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"farf_de.1g", CRC(0500d468) SHA1(f7dfc6f52e4db1d0d42edb646d719badbcee8ef0))
ZAC_ROMEND
CORE_CLONEDEFNV(farfallg,farfalla,"Farfalla (German Speech)",1983,"Zaccaria",mZAC2X,0)

INITGAME(farffp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farffp,	"farffp.ic1",CRC(0cddd987) SHA1(3d57fe22fe6bdd02276f6385ba606bf0f2980385),
							"farffp.ic2",CRC(5f75cd13) SHA1(9a36ed3c8ada6b22ac9ae39286d339f521d64747))
ZAC_SOUNDROM_de1g(			"rom1.snd",   CRC(aca09674) SHA1(8e1edc25c7fe2189215f73da8f1bec4b670bd8e6),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"rom3.snd",   CRC(d0584952) SHA1(80fe571a2e8a2a34fae03589df930b3eb3fa1f6b))
ZAC_ROMEND
CORE_CLONEDEFNV(farffp,farfalla,"Farfalla (Free Play)",1983,"Zaccaria",mZAC2X,0)

INITGAME(farfifp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farfifp,	"farffp.ic1",CRC(0cddd987) SHA1(3d57fe22fe6bdd02276f6385ba606bf0f2980385),
							"farffp.ic2",CRC(5f75cd13) SHA1(9a36ed3c8ada6b22ac9ae39286d339f521d64747))
ZAC_SOUNDROM_de1g(			"farsnd1.bin",CRC(fd80040d) SHA1(122c99627d944b253e091b56d32336367df615c1),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"farsnd3.bin",CRC(b58618c2) SHA1(89330ee928b5a5f99d50f1150c94732775907fd8))
ZAC_ROMEND
CORE_CLONEDEFNV(farfifp,farfalla,"Farfalla (Italian Speech, Free Play)",1983,"Zaccaria",mZAC2X,0)

INITGAME(farfgfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(farfgfp,	"farffp.ic1",CRC(0cddd987) SHA1(3d57fe22fe6bdd02276f6385ba606bf0f2980385),
							"farffp.ic2",CRC(5f75cd13) SHA1(9a36ed3c8ada6b22ac9ae39286d339f521d64747))
ZAC_SOUNDROM_de1g(			"farf_de.1d", CRC(5f64df81) SHA1(d8bd6d1fb3eec704fe31ccc1feeb5a9529c70d07),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"farf_de.1g", CRC(0500d468) SHA1(f7dfc6f52e4db1d0d42edb646d719badbcee8ef0))
ZAC_ROMEND
CORE_CLONEDEFNV(farfgfp,farfalla,"Farfalla (German Speech, Free Play)",1983,"Zaccaria",mZAC2X,0)

/*--------------------------------
/ Devil Riders (04/84)
/-------------------------------*/
INITGAME(dvlrider,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlrider,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(5d48462c) SHA1(755bc259e992a9b375bd1e338775da14c15932bd),
						"gb01snd2.1e",CRC(1127be59) SHA1(be074fe3efecd0c1e10599c8981bf7c5debb4d37),
						"gb01snd3.1g",CRC(1ae91ae8) SHA1(05bcc7e509beb5fc2510bca99c39af0bc02530a7))
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlridei,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlridei,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(	"dride_it.1d",CRC(cc33b947) SHA1(1b240ed6b38a78e21c5009342c4abab8bfd9ff7e),
					"dride_it.1e",CRC(b3764fd7) SHA1(27b5332af1aaedfc36d942f78146baa85617dbbe),
					"dride_it.1g",CRC(04b6ee80) SHA1(03157af1b4c7c8e882e7a482b3313584418d2d9a))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlridei,dvlrider,"Devil Riders (Italian Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlrideg,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlrideg,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(	"g_snd_1.bin",CRC(77d042dc) SHA1(78e056468887a315e29c913803e3c36f9c7f694e),
					"g_snd_2.bin",CRC(31e35fd4) SHA1(2eeefbd831159d975fe9cac99db99dfdca04b0dc),
					"g_snd_3.bin",CRC(2e64a401) SHA1(694808963d6e6a02ddeb9228073825ff16f91d49))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrideg,dvlrider,"Devil Riders (German Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlridef,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlridef,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(	"fr01snd1.1d",CRC(ba435706) SHA1(a05fb75aa135d02e9f97024524fe780d48d92be0),
					"fr01snd2.1e",NO_DUMP,
					"fr01snd3.1g",CRC(f707ad05) SHA1(0e3254ccf901181854f2e31c7339bd9e21d7d0bd))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlridef,dvlrider,"Devil Riders (French Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlrdfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlrdfp,	"driders.ic1",CRC(e69e12d0) SHA1(6df947a73c35d33c4ca5a1c81271bdbac1a10c37),
							"driders.ic2",CRC(0de27aca) SHA1(2770c8ae4ac41b9e287162d875df393362d0ae78))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(5d48462c) SHA1(755bc259e992a9b375bd1e338775da14c15932bd),
						"gb01snd2.1e",CRC(1127be59) SHA1(be074fe3efecd0c1e10599c8981bf7c5debb4d37),
						"gb01snd3.1g",CRC(1ae91ae8) SHA1(05bcc7e509beb5fc2510bca99c39af0bc02530a7))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrdfp,dvlrider,"Devil Riders (Free Play)",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlrdifp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlrdifp,	"driders.ic1",CRC(e69e12d0) SHA1(6df947a73c35d33c4ca5a1c81271bdbac1a10c37),
							"driders.ic2",CRC(0de27aca) SHA1(2770c8ae4ac41b9e287162d875df393362d0ae78))
ZAC_SOUNDROM_de2g(	"dride_it.1d",CRC(cc33b947) SHA1(1b240ed6b38a78e21c5009342c4abab8bfd9ff7e),
					"dride_it.1e",CRC(b3764fd7) SHA1(27b5332af1aaedfc36d942f78146baa85617dbbe),
					"dride_it.1g",CRC(04b6ee80) SHA1(03157af1b4c7c8e882e7a482b3313584418d2d9a))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrdifp,dvlrider,"Devil Riders (Italian Speech, Free Play)",1984,"Zaccaria",mZAC2X,0)

INITGAME(dvlrdgfp,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(dvlrdgfp,	"driders.ic1",CRC(e69e12d0) SHA1(6df947a73c35d33c4ca5a1c81271bdbac1a10c37),
							"driders.ic2",CRC(0de27aca) SHA1(2770c8ae4ac41b9e287162d875df393362d0ae78))
ZAC_SOUNDROM_de2g(	"g_snd_1.bin",CRC(77d042dc) SHA1(78e056468887a315e29c913803e3c36f9c7f694e),
					"g_snd_2.bin",CRC(31e35fd4) SHA1(2eeefbd831159d975fe9cac99db99dfdca04b0dc),
					"g_snd_3.bin",CRC(2e64a401) SHA1(694808963d6e6a02ddeb9228073825ff16f91d49))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrdgfp,dvlrider,"Devil Riders (German Speech, Free Play)",1984,"Zaccaria",mZAC2X,0)

/*--------------------------------
/ Magic Castle (09/84)
/-------------------------------*/
INITGAME(mcastle,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(mcastle,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(cd6a4a07) SHA1(47359747f79feca9d85d8f7657325874eda0f915),
						"gb01snd2.1e",CRC(d289952d) SHA1(94052cbee4cd499fb55d59b047828c21d71ab288),
						"gb01snd3.1g",CRC(8b4342eb) SHA1(a8534cb2ebaff4d5d4101eb710c068f3b91e9e0c))
ZAC_ROMEND
CORE_GAMEDEFNV(mcastle,"Magic Castle",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastlei,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(mcastlei,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"mgic_it.1d",CRC(16911674) SHA1(8fc5b0ec48c76eac21bcab44cf2fe9635e55ef49),
					"mgic_it.1e",CRC(646f9673) SHA1(cf78029f63c8264db2d0012143981d36b5410499),
					"mgic_it.1g",CRC(ffef01b2) SHA1(0c8a549432f3aed4b17eb5e3b6917f557d3f6050))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastlei,mcastle,"Magic Castle (Italian Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastleg,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(mcastleg,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"magic1d.snd",CRC(1f1a1140) SHA1(fed351c78e4c46c05e910f1844351492faa9edcf),
					"magic1e.snd",CRC(a8787011) SHA1(16da0b40e24346f4e90d553c7c3e68daa4d4a656),
					"magic1g.snd",CRC(313fb216) SHA1(1065f057654dd41cdac0553e1c315edf141f1d19))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastleg,mcastle,"Magic Castle (German Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastlef,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(mcastlef,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"mgic_fr.1d",CRC(7d3faa3b) SHA1(4f9ab1a868f7b9900bbbde02c2e654e7f778ed9d),
					"mgic_fr.1e",CRC(0077241c) SHA1(113d9039ad14f3887533f5e655a7912ddd441e77),
					"mgic_fr.1g",CRC(12d8b4f6) SHA1(a1b428e36c9d14bfb5b258a1c10ab1d02b502b56))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastlef,mcastle,"Magic Castle (French Speech)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(mcastfp,	"mcastfp.ic1",CRC(87349186) SHA1(1dbeb54c10d4be9abbc7cb48fe4407e13f935e2e),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(cd6a4a07) SHA1(47359747f79feca9d85d8f7657325874eda0f915),
						"gb01snd2.1e",CRC(d289952d) SHA1(94052cbee4cd499fb55d59b047828c21d71ab288),
						"gb01snd3.1g",CRC(8b4342eb) SHA1(a8534cb2ebaff4d5d4101eb710c068f3b91e9e0c))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastfp,mcastle,"Magic Castle (Free Play)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(mcastifp,	"mcastfp.ic1",CRC(87349186) SHA1(1dbeb54c10d4be9abbc7cb48fe4407e13f935e2e),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"mgic_it.1d",CRC(16911674) SHA1(8fc5b0ec48c76eac21bcab44cf2fe9635e55ef49),
					"mgic_it.1e",CRC(646f9673) SHA1(cf78029f63c8264db2d0012143981d36b5410499),
					"mgic_it.1g",CRC(ffef01b2) SHA1(0c8a549432f3aed4b17eb5e3b6917f557d3f6050))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastifp,mcastle,"Magic Castle (Italian Speech, Free Play)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastgfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(mcastgfp,	"mcastfp.ic1",CRC(87349186) SHA1(1dbeb54c10d4be9abbc7cb48fe4407e13f935e2e),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"magic1d.snd",CRC(1f1a1140) SHA1(fed351c78e4c46c05e910f1844351492faa9edcf),
					"magic1e.snd",CRC(a8787011) SHA1(16da0b40e24346f4e90d553c7c3e68daa4d4a656),
					"magic1g.snd",CRC(313fb216) SHA1(1065f057654dd41cdac0553e1c315edf141f1d19))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastgfp,mcastle,"Magic Castle (German Speech, Free Play)",1984,"Zaccaria",mZAC2X,0)

INITGAME(mcastffp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(mcastffp,	"mcastfp.ic1",CRC(87349186) SHA1(1dbeb54c10d4be9abbc7cb48fe4407e13f935e2e),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"mgic_fr.1d",CRC(7d3faa3b) SHA1(4f9ab1a868f7b9900bbbde02c2e654e7f778ed9d),
					"mgic_fr.1e",CRC(0077241c) SHA1(113d9039ad14f3887533f5e655a7912ddd441e77),
					"mgic_fr.1g",CRC(12d8b4f6) SHA1(a1b428e36c9d14bfb5b258a1c10ab1d02b502b56))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastffp,mcastle,"Magic Castle (French Speech, Free Play)",1984,"Zaccaria",mZAC2X,0)

/*--------------------------------
/ Robot (01/85)
/-------------------------------*/
INITGAME(robot,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(robot,	"robot_1.lgc",CRC(5e754418) SHA1(81a25ef85147d8c043b7d243d9d0d3e8bf90f852),
						"robot_2.lgc",CRC(28ba9687) SHA1(8e99834328783361856fa9632b2c6e3a5a05d49b))
ZAC_SOUNDROM_de2g(		"robot_d.snd",CRC(ab5e5524) SHA1(9aae2560bccf64daeab0514c8934c55f77fe240d),
						"robot_e.snd",CRC(2f314e33) SHA1(1f92aff3d99c2e86820720a3290285b9f36cb15b),
						"robot_g.snd",CRC(6fb1caf5) SHA1(0bc6a6edaa9589b7d171f96fa74855a022c2b050))
ZAC_ROMEND
CORE_GAMEDEFNV(robot,"Robot",1985,"Zaccaria",mZAC2X,0)

INITGAME(roboti,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(roboti,	"robot_1.lgc", CRC(5e754418) SHA1(81a25ef85147d8c043b7d243d9d0d3e8bf90f852),
						"robot_2.lgc", CRC(28ba9687) SHA1(8e99834328783361856fa9632b2c6e3a5a05d49b))
ZAC_SOUNDROM_de2g(		"robot_it.1d", CRC(a4a20ed7) SHA1(459519e10bad59ba27cd5d5d31c5f276726c9bd0),
						"robot_it.1e", CRC(2f314e33) SHA1(1f92aff3d99c2e86820720a3290285b9f36cb15b),
						"robot_it.1g", CRC(6bce79ac) SHA1(f93871b050edebccca7f0265c3f5144e10b6cc79))
ZAC_ROMEND
CORE_CLONEDEFNV(roboti,robot,"Robot (Italian Speech)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotg,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(robotg,	"robot_1.lgc", CRC(5e754418) SHA1(81a25ef85147d8c043b7d243d9d0d3e8bf90f852),
						"robot_2.lgc", CRC(28ba9687) SHA1(8e99834328783361856fa9632b2c6e3a5a05d49b))
ZAC_SOUNDROM_de2g(		"robot_dg.snd",CRC(88685b1e) SHA1(7d49a1d42f3e07948390a00a562aeba9dd4ddeeb),
						"robot_eg.snd",CRC(e326a851) SHA1(c2bb5e329803922fa1c1ca30be6e3ae3d292135a),
						"robot_gg.snd",CRC(7ed5da55) SHA1(d70f1f470cf9d300375600352f9625b4e34f5ed3))
ZAC_ROMEND
CORE_CLONEDEFNV(robotg,robot,"Robot (German Speech)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotf,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART020(robotf,	"robot_1.lgc", CRC(5e754418) SHA1(81a25ef85147d8c043b7d243d9d0d3e8bf90f852),
						"robot_2.lgc", CRC(28ba9687) SHA1(8e99834328783361856fa9632b2c6e3a5a05d49b))
ZAC_SOUNDROM_de2g(		"robot_fr.1d", CRC(94957954) SHA1(22f729a1ca48399aa222f5037071d0482b9d59aa),
						"robot_fr.1e", CRC(fdcfff02) SHA1(1ef02ad646dfea1b9727a0a99e93db724cd38cce),
						"robot_fr.1g", CRC(ccf6413f) SHA1(71242e999985ee78ec0fb282e4de2f45c1867051))
ZAC_ROMEND
CORE_CLONEDEFNV(robotf,robot,"Robot (French Speech)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(robotfp,	"robotfp.ic1",CRC(afd47263) SHA1(bd4f2b4201a6bb1a859c27c7861532d0f007046c),
						"robotfp.ic2",CRC(8771d21d) SHA1(cdf313b7214f1f38da4698891125791af0e8f02a))
ZAC_SOUNDROM_de2g(		"robot_d.snd",CRC(ab5e5524) SHA1(9aae2560bccf64daeab0514c8934c55f77fe240d),
						"robot_e.snd",CRC(2f314e33) SHA1(1f92aff3d99c2e86820720a3290285b9f36cb15b),
						"robot_g.snd",CRC(6fb1caf5) SHA1(0bc6a6edaa9589b7d171f96fa74855a022c2b050))
ZAC_ROMEND
CORE_CLONEDEFNV(robotfp,robot,"Robot (Free Play)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(robotifp,	"robotfp.ic1",CRC(afd47263) SHA1(bd4f2b4201a6bb1a859c27c7861532d0f007046c),
						"robotfp.ic2",CRC(8771d21d) SHA1(cdf313b7214f1f38da4698891125791af0e8f02a))
ZAC_SOUNDROM_de2g(		"robot_it.1d", CRC(a4a20ed7) SHA1(459519e10bad59ba27cd5d5d31c5f276726c9bd0),
						"robot_it.1e", CRC(2f314e33) SHA1(1f92aff3d99c2e86820720a3290285b9f36cb15b),
						"robot_it.1g", CRC(6bce79ac) SHA1(f93871b050edebccca7f0265c3f5144e10b6cc79))
ZAC_ROMEND
CORE_CLONEDEFNV(robotifp,robot,"Robot (Italian Speech, Free Play)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotgfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(robotgfp,	"robotfp.ic1",CRC(afd47263) SHA1(bd4f2b4201a6bb1a859c27c7861532d0f007046c),
						"robotfp.ic2",CRC(8771d21d) SHA1(cdf313b7214f1f38da4698891125791af0e8f02a))
ZAC_SOUNDROM_de2g(		"robot_dg.snd",CRC(88685b1e) SHA1(7d49a1d42f3e07948390a00a562aeba9dd4ddeeb),
						"robot_eg.snd",CRC(e326a851) SHA1(c2bb5e329803922fa1c1ca30be6e3ae3d292135a),
						"robot_gg.snd",CRC(7ed5da55) SHA1(d70f1f470cf9d300375600352f9625b4e34f5ed3))
ZAC_ROMEND
CORE_CLONEDEFNV(robotgfp,robot,"Robot (German Speech, Free Play)",1985,"Zaccaria",mZAC2X,0)

INITGAME(robotffp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136,366)
ZAC_ROMSTART1820(robotffp,	"robotfp.ic1",CRC(afd47263) SHA1(bd4f2b4201a6bb1a859c27c7861532d0f007046c),
						"robotfp.ic2",CRC(8771d21d) SHA1(cdf313b7214f1f38da4698891125791af0e8f02a))
ZAC_SOUNDROM_de2g(		"robot_fr.1d", CRC(94957954) SHA1(22f729a1ca48399aa222f5037071d0482b9d59aa),
						"robot_fr.1e", CRC(fdcfff02) SHA1(1ef02ad646dfea1b9727a0a99e93db724cd38cce),
						"robot_fr.1g", CRC(ccf6413f) SHA1(71242e999985ee78ec0fb282e4de2f45c1867051))
ZAC_ROMEND
CORE_CLONEDEFNV(robotffp,robot,"Robot (French Speech, Free Play)",1985,"Zaccaria",mZAC2X,0)

/*--------------------------------
/ Clown (07/85)
/-------------------------------*/
INITGAME2(clown,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART020(clown,	"clown_1.lgc",CRC(16f09833) SHA1(5c9c8b9403d8b69ae7252bf904edc617784b8165),
						"clown_2.lgc",CRC(697e6b5b) SHA1(d2c459cbffec94730eb2abe3c63b4913a18085a7))
ZAC_SOUNDROM_e2f2(		"clown_e.snd",CRC(04a34cc1) SHA1(56fcc07ccab3cac27928f5c5411868bde1769603),
						"clown_f.snd",CRC(e35a4f72) SHA1(0037c1072f58798ba61af85a1b4b374b85c883ae))
ZAC_ROMEND
CORE_GAMEDEFNV(clown,"Clown",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(clownfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(clownfp,	"clownfp.ic1",CRC(b1f8743a) SHA1(732d9830d2e9c959c90d9c592ac8d72f2bc8e581),
						"clownfp.ic2",CRC(8ee37639) SHA1(9e117d2e43a3d3e9db1369b50e88f3a2a46ab7a9))
ZAC_SOUNDROM_e2f2(		"clown_e.snd",CRC(04a34cc1) SHA1(56fcc07ccab3cac27928f5c5411868bde1769603),
						"clown_f.snd",CRC(e35a4f72) SHA1(0037c1072f58798ba61af85a1b4b374b85c883ae))
ZAC_ROMEND
CORE_CLONEDEFNV(clownfp,clown,"Clown (Free Play)",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

/*--------------------------------
/ Pool Champion (12/85)
/-------------------------------*/
INITGAME2(poolcham,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART020(poolcham,	"poolcham.ic1",CRC(fca2a2b2) SHA1(9a0d9c495e38628c5e0bc10f6335100eb934f153),
							"poolcham.ic2",CRC(267a2a02) SHA1(049ada7bfcf0d8560ac03effd3fbb02ead51933c))
ZAC_SOUNDROM_f(				"poolcham.1f", CRC(efe33926) SHA1(30444a2ee7f453f46c74fff8365d80fc4f0a277f))
ZAC_ROMEND
CORE_GAMEDEFNV(poolcham,"Pool Champion",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(poolchai,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART020(poolchai,	"poolchai.ic1",CRC(fca2a2b2) SHA1(9a0d9c495e38628c5e0bc10f6335100eb934f153) BAD_DUMP,
							"poolchai.ic2",CRC(267a2a02) SHA1(049ada7bfcf0d8560ac03effd3fbb02ead51933c) BAD_DUMP)
ZAC_SOUNDROM_e2f4(			"poolc_it.1e", CRC(28a3e5ee) SHA1(c090c81c78d3296e91ce12e1170ee2c71ba07177),
							"sound1.f",    CRC(b4b4e31e) SHA1(bcd1c4c7f6f079655a9c37d0b978d997f95b93ad))
ZAC_ROMEND
CORE_CLONEDEFNV(poolchai,poolcham,"Pool Champion (Italian Speech)",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(poolcfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(poolcfp,	"poolcfp.ic1",CRC(1cc1514d) SHA1(63dd4c11856453b1aaa46144c5e631fbd17ad390),
							"poolcfp.ic2",CRC(e5a4afb7) SHA1(e9ca17239decbb951152abeb332853bc0f9503ed))
ZAC_SOUNDROM_f(				"poolcham.1f", CRC(efe33926) SHA1(30444a2ee7f453f46c74fff8365d80fc4f0a277f))
ZAC_ROMEND
CORE_CLONEDEFNV(poolcfp,poolcham,"Pool Champion (Free Play)",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

/*--------------------------------
/ Black Belt (03/86)
/-------------------------------*/
INITGAME2(bbeltzac,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltzac,	"bbz-1.fil",CRC(2e7e1575) SHA1(1b9e6e4ff461962f4c7249bd2a748444cb658c30),
							"bbz-2.fil",CRC(dbec92ae) SHA1(7a1c6e5ac81d3cfcbb135a1c8b69e55296fffcc5))
ZAC_SOUNDROM_e2f4(			"bbz-e.snd",CRC(1fe045d2) SHA1(d17d7dbcafe9f8644cbe393a56ff6b45d9d40155),
							"bbz-f.snd",CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Black Belt (Zaccaria)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltzai,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltzai,	"bbz-1.fil",CRC(2e7e1575) SHA1(1b9e6e4ff461962f4c7249bd2a748444cb658c30),
							"bbz-2.fil",CRC(dbec92ae) SHA1(7a1c6e5ac81d3cfcbb135a1c8b69e55296fffcc5))
ZAC_SOUNDROM_e2f4(			"bbelt_it.1e", CRC(fab5b89f) SHA1(9a2c2ae0a2035762b11cbd84fe3cddbde4572f18),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltzai,bbeltzac,"Black Belt (Zaccaria, Italian Speech)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltzag,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltzag,	"bbz-1.fil",CRC(2e7e1575) SHA1(1b9e6e4ff461962f4c7249bd2a748444cb658c30),
							"bbz-2.fil",CRC(dbec92ae) SHA1(7a1c6e5ac81d3cfcbb135a1c8b69e55296fffcc5))
ZAC_SOUNDROM_e2f4(			"bbelt_de.1e", CRC(f343103d) SHA1(d0ee91c873a10049f9aae6e762637d0384ff052a),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltzag,bbeltzac,"Black Belt (Zaccaria, German Speech)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltzaf,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltzaf,	"bbz-1.fil",CRC(2e7e1575) SHA1(1b9e6e4ff461962f4c7249bd2a748444cb658c30),
							"bbz-2.fil",CRC(dbec92ae) SHA1(7a1c6e5ac81d3cfcbb135a1c8b69e55296fffcc5))
ZAC_SOUNDROM_e2f4(			"bbelt_fr.1e", CRC(81e89d96) SHA1(f38610ffc2b12601b2b2f6871645bd4186d9b229),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltzaf,bbeltzac,"Black Belt (Zaccaria, French Speech)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltzfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltzfp,	"bkbeltfp.ic1",CRC(b22e3696) SHA1(b37313aa2d91ed697ba8c9316feb327560d9e8ab),
							"bkbeltfp.ic2",CRC(e058ac50) SHA1(5f0b1f3d5160c20d2a7e4c3c3b8614066121d282))
ZAC_SOUNDROM_e2f4(			"bbz-e.snd",CRC(1fe045d2) SHA1(d17d7dbcafe9f8644cbe393a56ff6b45d9d40155),
							"bbz-f.snd",CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltzfp,bbeltzac,"Black Belt (Zaccaria, Free Play)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltifp,	"bkbeltfp.ic1",CRC(b22e3696) SHA1(b37313aa2d91ed697ba8c9316feb327560d9e8ab),
							"bkbeltfp.ic2",CRC(e058ac50) SHA1(5f0b1f3d5160c20d2a7e4c3c3b8614066121d282))
ZAC_SOUNDROM_e2f4(			"bbelt_it.1e", CRC(fab5b89f) SHA1(9a2c2ae0a2035762b11cbd84fe3cddbde4572f18),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltifp,bbeltzac,"Black Belt (Zaccaria, Italian Speech, Free Play)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltgfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltgfp,	"bkbeltfp.ic1",CRC(b22e3696) SHA1(b37313aa2d91ed697ba8c9316feb327560d9e8ab),
							"bkbeltfp.ic2",CRC(e058ac50) SHA1(5f0b1f3d5160c20d2a7e4c3c3b8614066121d282))
ZAC_SOUNDROM_e2f4(			"bbelt_de.1e", CRC(f343103d) SHA1(d0ee91c873a10049f9aae6e762637d0384ff052a),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltgfp,bbeltzac,"Black Belt (Zaccaria, German Speech, Free Play)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(bbeltffp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(bbeltffp,	"bkbeltfp.ic1",CRC(b22e3696) SHA1(b37313aa2d91ed697ba8c9316feb327560d9e8ab),
							"bkbeltfp.ic2",CRC(e058ac50) SHA1(5f0b1f3d5160c20d2a7e4c3c3b8614066121d282))
ZAC_SOUNDROM_e2f4(			"bbelt_fr.1e", CRC(81e89d96) SHA1(f38610ffc2b12601b2b2f6871645bd4186d9b229),
							"bbz-f.snd",   CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_CLONEDEFNV(bbeltffp,bbeltzac,"Black Belt (Zaccaria, French Speech, Free Play)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

/*--------------------------------
/ Mexico 86 (07/86)
/-------------------------------*/
INITGAME2(mexico,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(mexico,	"mex86_1.lgc",CRC(60d559b1) SHA1(1097f32dd0c89b6e3653a620e39696d8ab1289fc),
							"mex86_2.lgc",CRC(5c984c15) SHA1(c6228568cee6a365a3c552a57e5e1e0445108bad))
ZAC_SOUNDROM_e2f4(			"mex86_e.snd",CRC(a985e8db) SHA1(11f91179fa1d46c1c83cdd4fbcf8ebdfd2a41f3f),
							"mex86_f.snd",CRC(301c2b63) SHA1(df4a4cb48d28d53c3728066d3e3fa9eac17c78c5))
ZAC_ROMEND
CORE_GAMEDEFNV(mexico,"Mexico 86 (German Speech)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

INITGAME2(mexicofp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178,366)
ZAC_ROMSTART1820(mexicofp,	"mex86fp.ic1",CRC(1efaf9bf) SHA1(efcc822aa6af3674196d501518dc4b1cb376bbba),
							"mex86fp.ic2",CRC(3c386c4e) SHA1(b867d6f4794e21c5c9afca61d66c3e7c0248e0b9))
ZAC_SOUNDROM_e2f4(			"mex86_e.snd",CRC(a985e8db) SHA1(11f91179fa1d46c1c83cdd4fbcf8ebdfd2a41f3f),
							"mex86_f.snd",CRC(301c2b63) SHA1(df4a4cb48d28d53c3728066d3e3fa9eac17c78c5))
ZAC_ROMEND
CORE_CLONEDEFNV(mexicofp,mexico,"Mexico 86 (German Speech, Free Play)",1986,"Zaccaria",mZAC2XS,SOUNDFLAG)

/*--------------------------------
/ Zankor (12/86)
/-------------------------------*/
INITGAME2(zankor,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(zankor,	"zan_ic1.764",CRC(e7ba5acf) SHA1(48b64921dd8a22c2483162db571512cad8cbb072),
							"zan_ic2.764",CRC(5804ff10) SHA1(fc3c4acb183c5c3e0a6504583c78f25a7a322cce))
ZAC_SOUNDROM_e2f4(			"1en.64", CRC(abc930cc) SHA1(6c658aae3f26db21df7b74a616cf37307dba63e3),
							"zan_1f.128", CRC(74fcadc9) SHA1(efd6fc99d7a3ed8e59fbbafbee161af6fb527028))

ZAC_SOUNDROM_456(			"zan_ic4.128",CRC(f34a2aaa) SHA1(5e415874f68586aa30dba9fff0dc8990c636cecd),
							"zan_ic5.128",CRC(bf61aab0) SHA1(939266696d0562f255f0fa5068280fe6a4cf8267),
							"zan_ic6.128",CRC(13a5b8d4) SHA1(d8c976b3f5e9c7cded0922feefa1531c59432515))
ZAC_ROMEND
CORE_GAMEDEFNV(zankor,"Zankor (Italian Speech)",1986,"Zaccaria",mZAC2XS2,SOUNDFLAG)

INITGAME2(zankorfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(zankorfp,	"zankorfp.ic1",CRC(9955ed8d) SHA1(636528e05d930b4cb680a4763a0ace4bb5e0cd0c),
							"zankorfp.ic2",CRC(3a253be7) SHA1(20d506686b811dbe0f931c86e7bcc7b689068148))
ZAC_SOUNDROM_e2f4(			"1en.64", CRC(abc930cc) SHA1(6c658aae3f26db21df7b74a616cf37307dba63e3),
							"zan_1f.128", CRC(74fcadc9) SHA1(efd6fc99d7a3ed8e59fbbafbee161af6fb527028))

ZAC_SOUNDROM_456(			"zan_ic4.128",CRC(f34a2aaa) SHA1(5e415874f68586aa30dba9fff0dc8990c636cecd),
							"zan_ic5.128",CRC(bf61aab0) SHA1(939266696d0562f255f0fa5068280fe6a4cf8267),
							"zan_ic6.128",CRC(13a5b8d4) SHA1(d8c976b3f5e9c7cded0922feefa1531c59432515))
ZAC_ROMEND
CORE_CLONEDEFNV(zankorfp,zankor,"Zankor (Italian Speech, Free Play)",1986,"Zaccaria",mZAC2XS2,SOUNDFLAG)

/*--------------------------------
/ Spooky (04/87)
/-------------------------------*/
INITGAME2(spooky,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(spooky,	"spook_1.lgc",CRC(377b347d) SHA1(c7334cf2b10b749f5f75b8feaa8ec773a576b2f1),
							"spook_2.lgc",CRC(ae0598b0) SHA1(aab725d1e386a3792100eb55c5836e6ed68cafdd))
ZAC_SOUNDROM_e2f4(			"spook_e.snd",CRC(3d632c93) SHA1(3cc127956a6df1a4fd551826068810724b32ad0e),
							"spook_f.snd",CRC(cc04a448) SHA1(e837a7d7640aa1d2c2880616bd377b64dc8fac9d))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4) SHA1(4a9dd9d571f958c270b437a1665e6d3dd3eef598),
							"spook_6.snd",CRC(d4320bc7) SHA1(30b959f5df44d097baffc2de70b12fc767f5663b))
ZAC_ROMEND
CORE_GAMEDEFNV(spooky,"Spooky",1987,"Zaccaria",mZAC2XS2A,SOUNDFLAG)

INITGAME2(spookyi,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(spookyi,	"spook_1.lgc",CRC(377b347d) SHA1(c7334cf2b10b749f5f75b8feaa8ec773a576b2f1),
							"spook_2.lgc",CRC(ae0598b0) SHA1(aab725d1e386a3792100eb55c5836e6ed68cafdd))
ZAC_SOUNDROM_e2f4(			"spook_it.1e",CRC(cdbe248e) SHA1(2337836e01622b3fc3f31272faaebf30a608a138),
							"spook_f.snd",CRC(cc04a448) SHA1(e837a7d7640aa1d2c2880616bd377b64dc8fac9d))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4) SHA1(4a9dd9d571f958c270b437a1665e6d3dd3eef598),
							"spook_6.snd",CRC(d4320bc7) SHA1(30b959f5df44d097baffc2de70b12fc767f5663b))
ZAC_ROMEND
CORE_CLONEDEFNV(spookyi,spooky,"Spooky (Italian Speech)",1987,"Zaccaria",mZAC2XS2A,SOUNDFLAG)

INITGAME2(spookyfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(spookyfp,	"spookyfp.ic1",CRC(c78e1535) SHA1(583ec5a0b999f353d6d679a83555834105ee3c1c),
							"spookyfp.ic2",CRC(b71040ea) SHA1(2b00d1f5789107ae5cff16ebb801eda486b93d8d))
ZAC_SOUNDROM_e2f4(			"spook_e.snd",CRC(3d632c93) SHA1(3cc127956a6df1a4fd551826068810724b32ad0e),
							"spook_f.snd",CRC(cc04a448) SHA1(e837a7d7640aa1d2c2880616bd377b64dc8fac9d))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4) SHA1(4a9dd9d571f958c270b437a1665e6d3dd3eef598),
							"spook_6.snd",CRC(d4320bc7) SHA1(30b959f5df44d097baffc2de70b12fc767f5663b))
ZAC_ROMEND
CORE_CLONEDEFNV(spookyfp,spooky,"Spooky (Free Play)",1987,"Zaccaria",mZAC2XS2A,SOUNDFLAG)

INITGAME2(spookifp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178_13181,366)
ZAC_ROMSTART1820(spookifp,	"spookyfp.ic1",CRC(c78e1535) SHA1(583ec5a0b999f353d6d679a83555834105ee3c1c),
							"spookyfp.ic2",CRC(b71040ea) SHA1(2b00d1f5789107ae5cff16ebb801eda486b93d8d))
ZAC_SOUNDROM_e2f4(			"spook_it.1e",CRC(cdbe248e) SHA1(2337836e01622b3fc3f31272faaebf30a608a138),
							"spook_f.snd",CRC(cc04a448) SHA1(e837a7d7640aa1d2c2880616bd377b64dc8fac9d))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4) SHA1(4a9dd9d571f958c270b437a1665e6d3dd3eef598),
							"spook_6.snd",CRC(d4320bc7) SHA1(30b959f5df44d097baffc2de70b12fc767f5663b))
ZAC_ROMEND
CORE_CLONEDEFNV(spookifp,spooky,"Spooky (Italian Speech, Free Play)",1987,"Zaccaria",mZAC2XS2A,SOUNDFLAG)

/*--------------------------------
/ Star's Phoenix (07/87)
/-------------------------------*/
INITGAME(strsphnx,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(strsphnx,	"strphnx1.cpu",CRC(2a31b7da) SHA1(05f2173783e686cc8774bed6eb59b41f7af88d11),
							"strphnx2.cpu",CRC(db830505) SHA1(55d6d6e12e2861fec81b46fb90c29aad5ad922aa))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"snd_ic40.bin",CRC(974ceb9c) SHA1(3665af9170a2afbe26f68e8f3cedb0d177f476c4))
ZAC_ROMEND
CORE_GAMEDEFNV(strsphnx,"Star's Phoenix (Italian Speech)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(strsphnf,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(strsphnf,	"strphnx1.cpu",CRC(2a31b7da) SHA1(05f2173783e686cc8774bed6eb59b41f7af88d11),
							"strphnx2.cpu",CRC(db830505) SHA1(55d6d6e12e2861fec81b46fb90c29aad5ad922aa))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"sndfic40.bin",CRC(1b40de42) SHA1(cfebab38a493edab1dfd0e5f591ccb9658da80c6))
ZAC_ROMEND
CORE_CLONEDEFNV(strsphnf,strsphnx,"Star's Phoenix (French Speech)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(strsphfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(strsphfp,	"strsphfp.ic1",CRC(8d39e104) SHA1(6f2ac378d9fa98f31e0f94487864fdcc5e4aea80),
							"strsphfp.ic2",CRC(a50df56e) SHA1(1aa11d5918bfe93a65b713b5dcfab8ee9c667f48))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"snd_ic40.bin",CRC(974ceb9c) SHA1(3665af9170a2afbe26f68e8f3cedb0d177f476c4))
ZAC_ROMEND
CORE_CLONEDEFNV(strsphfp,strsphnx,"Star's Phoenix (Italian Speech, Free Play)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(strspffp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(strspffp,	"strsphfp.ic1",CRC(8d39e104) SHA1(6f2ac378d9fa98f31e0f94487864fdcc5e4aea80),
							"strsphfp.ic2",CRC(a50df56e) SHA1(1aa11d5918bfe93a65b713b5dcfab8ee9c667f48))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"sndfic40.bin",CRC(1b40de42) SHA1(cfebab38a493edab1dfd0e5f591ccb9658da80c6))
ZAC_ROMEND
CORE_CLONEDEFNV(strspffp,strsphnx,"Star's Phoenix (French Speech, Free Play)",1987,"Zaccaria",mZAC2XS3,0)

/*--------------------------------
/ New Star's Phoenix (08/87) (same ROMs as Star's Phoenix, but slightly different playfield, etc)
/-------------------------------*/
INITGAME(nstrphnx,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(nstrphnx,	"strphnx1.cpu",CRC(2a31b7da) SHA1(05f2173783e686cc8774bed6eb59b41f7af88d11),
							"strphnx2.cpu",CRC(db830505) SHA1(55d6d6e12e2861fec81b46fb90c29aad5ad922aa))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"snd_ic40.bin",CRC(974ceb9c) SHA1(3665af9170a2afbe26f68e8f3cedb0d177f476c4))
ZAC_ROMEND
CORE_GAMEDEFNV(nstrphnx,"New Star's Phoenix (Italian Speech)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(nstrphnf,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(nstrphnf,	"strphnx1.cpu",CRC(2a31b7da) SHA1(05f2173783e686cc8774bed6eb59b41f7af88d11),
							"strphnx2.cpu",CRC(db830505) SHA1(55d6d6e12e2861fec81b46fb90c29aad5ad922aa))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"sndfic40.bin",CRC(1b40de42) SHA1(cfebab38a493edab1dfd0e5f591ccb9658da80c6))
ZAC_ROMEND
CORE_CLONEDEFNV(nstrphnf,nstrphnx,"New Star's Phoenix (French Speech)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(nstrphfp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(nstrphfp,	"strsphfp.ic1",CRC(8d39e104) SHA1(6f2ac378d9fa98f31e0f94487864fdcc5e4aea80),
							"strsphfp.ic2",CRC(a50df56e) SHA1(1aa11d5918bfe93a65b713b5dcfab8ee9c667f48))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"snd_ic40.bin",CRC(974ceb9c) SHA1(3665af9170a2afbe26f68e8f3cedb0d177f476c4))
ZAC_ROMEND
CORE_CLONEDEFNV(nstrphfp,nstrphnx,"New Star's Phoenix (Italian Speech, Free Play)",1987,"Zaccaria",mZAC2XS3,0)

INITGAME(nstrpffp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13181x3,366)
ZAC_ROMSTART1820(nstrpffp,	"strsphfp.ic1",CRC(8d39e104) SHA1(6f2ac378d9fa98f31e0f94487864fdcc5e4aea80),
							"strsphfp.ic2",CRC(a50df56e) SHA1(1aa11d5918bfe93a65b713b5dcfab8ee9c667f48))
ZAC_SOUNDROM_5x256(			"snd_ic05.bin",CRC(74cc4902) SHA1(e2f46bcf5446f98d098c49f8c2416292401265b9),
							"snd_ic06.bin",CRC(a0400411) SHA1(da9de6105639c4f6174f5bc92f44e02c339a2bc3),
							"snd_ic24.bin",CRC(158d6f83) SHA1(281e1b13be43025be1b33dcd366cec0b36f29e5c),
							"snd_ic25.bin",CRC(b1c9238e) SHA1(88c9df1fca94d32a0fa5d75312dabff257e867dd),
							"sndfic40.bin",CRC(1b40de42) SHA1(cfebab38a493edab1dfd0e5f591ccb9658da80c6))
ZAC_ROMEND
CORE_CLONEDEFNV(nstrpffp,nstrphnx,"New Star's Phoenix (French Speech, Free Play)",1987,"Zaccaria",mZAC2XS3,0)


// Tecnoplay games below

/*--------------------------------
/ Scramble (1987)
/-------------------------------*/
INITGAME(scram_tp,GEN_ZAC2,dispZAC2,SNDBRD_TECHNO,366)
ZAC_ROMSTART1820(scram_tp,	"scram_1.lgc",CRC(da565549) SHA1(d187801428824df2b506c999548a5c6d146bc59e),
							"scram_2.lgc",CRC(537e6c61) SHA1(84e0db4268d3c990c3834ebd20bf7c475a70082d))
TECHNO_SOUNDROM1("scram_1.snd",CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55),
                 "scram_2.snd",CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881))
TECHNO_SOUNDROM2("scram_3.snd",CRC(ed27cd78) SHA1(a062ee1a3ec8819acddac13a4b454f5fd95d1e29),
                 "scram_4.snd",CRC(943f279d) SHA1(52767708d706a01ea16e37c866eb5762297e1f86),
                 "scram_5.snd",CRC(3aa782ec) SHA1(7cbbd3a737239b2755c6a6651a284e83fcfa22f6))
ZAC_ROMEND
CORE_GAMEDEFNV(scram_tp,"Scramble",1987,"Tecnoplay",mTECHNO,0)


// Apple Time games below

/*--------------------------------
/ Thunder Man (1987)
/-------------------------------*/
static core_tGameData thndrmanGameData = {GEN_ZAC2,dispZAC3,{FLIP_SW(FLIP_L),0,2,0,SNDBRD_ZAC13136,0,366,1}};
static void init_thndrman(void) {
  core_gameData = &thndrmanGameData;
}
ZAC_INPUT_PORTS_START(thndrman, 1) ZAC_INPUT_PORTS_END
ROM_START(thndrman)
  NORMALREGION (0x10000, ZAC_MEMREG_CPU)
  ROM_LOAD("mpu_ic1.764",0x0000, 0x0800, CRC(d4861835) SHA1(b3518eb3126fc3f5b6f3e9a4c53df8eec21768c5))
    ROM_CONTINUE(0x2000, 0x0800)
    ROM_CONTINUE(0x0800, 0x0800)
    ROM_CONTINUE(0x2800, 0x0800)
    ROM_RELOAD  (0x4000, 0x0800)
    ROM_CONTINUE(0x6000, 0x0800)
    ROM_CONTINUE(0x4800, 0x0800)
    ROM_CONTINUE(0x6800, 0x0800)
  ROM_LOAD("mpu_ic3.764",0x1000, 0x0800, CRC(db85aec2) SHA1(ac5b321cc9d6f86a7f554af010858d2144cbdf2a))
    ROM_CONTINUE(0x3000, 0x0800)
    ROM_CONTINUE(0x1800, 0x0800)
    ROM_CONTINUE(0x3800, 0x0800)
    ROM_RELOAD  (0x5000, 0x0800)
    ROM_CONTINUE(0x7000, 0x0800)
    ROM_CONTINUE(0x5800, 0x0800)
    ROM_CONTINUE(0x7800, 0x0800)
  SOUNDREGION(0x10000, ZACSND_CPUAREGION)
  ROM_LOAD("snd_1f.764", 0xc000, 0x2000, CRC(400e8e2a) SHA1(24af3a8e11aec89ae27a5cfcce9d4624bede18f7))
  ROM_LOAD("snd_1c.764", 0xe000, 0x2000, CRC(4f18409f) SHA1(21002a147e2542caacba0392cec62511343b90c2))
ZAC_ROMEND
CORE_GAMEDEFNV(thndrman,"Thunder Man",1987,"Apple Time",mZAC2X,0)
