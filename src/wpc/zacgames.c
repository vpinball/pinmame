#include "driver.h"
#include "gen.h"
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
INITGAME1(wsports,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1311)
ZAC_ROMSTART44444(wsports,	"ws1.bin",CRC(58feb058) SHA1(50216bba5be28284e63d826543297d1b6b609325),
							"ws2.bin",CRC(ece702cb) SHA1(84cf0976b33bd7cf25976de9c66cc85808f1cd50),
							"ws3.bin",CRC(ff7f6824) SHA1(0eef4aca51c0e823f7634d7fc22c96c590239269),
							"ws4.bin",CRC(74460cf2) SHA1(4afa612af1eff8eae686ceba7c117bc7962272c7),
							"ws5.bin",CRC(5ef51ced) SHA1(390579d0482ceabf87924f7718ef33e336726d92))
ZAC_ROMEND
CORE_GAMEDEFNV(wsports,"Winter Sports",1978,"Zaccaria",mZAC1311,SOUNDFLAG)

//07/78 House of Diamonds
//09/78 Strike
//10/78 Ski Jump

/*--------------------------------
/ Future World (10/78)
/-------------------------------*/
// Game ROMs #2 and #3 have to be swapped!
INITGAME1(futurwld,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1311)
ZAC_ROMSTART44444(futurwld,	"futwld_1.lgc",CRC(d83b8793) SHA1(3bb04d8395191ecf324b6da0bcddcf7bd8d41867),
							"futwld_3.lgc",CRC(bdcb7e1d) SHA1(e6c0c7e8188df87937f0b22dbb0639872e03e948),
							"futwld_2.lgc",CRC(48e3d293) SHA1(0029f30c4a94067e7782e22499b11db86f051934),
							"futwld_4.lgc",CRC(b1de2120) SHA1(970e1c4eadb7ace1398684accac289a434d13d84),
							"futwld_5.lgc",CRC(6b7965f2) SHA1(31314bc63f01717004c5c2448b5db7d292145b60))
ZAC_ROMEND
CORE_GAMEDEFNV(futurwld,"Future World",1978,"Zaccaria",mZAC1311,SOUNDFLAG)

/*--------------------------------
/ Shooting the Rapids (04/79)
/-------------------------------*/
INITGAME1(strapids,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125)
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
INITGAME1(hotwheel,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125)
ZAC_ROMSTART84444(hotwheel,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"htwhls_2.lgc",CRC(7ff870ae) SHA1(274ee7c2cb92b6710c546058e7277f06720b5e37),
							"htwhls_3.lgc",CRC(7c1fba91) SHA1(d514e9b3128dfe7999e414fd9044dc20c0d76c66),
							"htwhls_4.lgc",CRC(974804ba) SHA1(f35c1b52327b2d3170a9a28dbee4d1437f1f594a),
							"htwhls_5.lgc",CRC(e28f3c60) SHA1(eb780be60b41017d105288cef71906d15474b8fa))
ZAC_ROMEND
CORE_GAMEDEFNV(hotwheel,"Hot Wheels",1979,"Zaccaria",mZAC1125,SOUNDFLAG)

//09/79 Space City

/*--------------------------------
/ Fire Mountain (01/80)
/-------------------------------*/
INITGAME1(firemntn,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1125)
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
// After game start, no key will respond anymore?
INITGAME1(stargod,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(stargod,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"stargod2.lgc",CRC(7a784b03) SHA1(bc3490b69913f52e3e9db5c3de5617ab89efe073),
							"stargod3.lgc",CRC(95492ac0) SHA1(992ad53efc5b53020e3939dfca5431fd50b6571c),
							"stargod4.lgc",CRC(09e5682a) SHA1(c9fcad4f55ee005e204a49fa65e7d77ecfde9680),
							"stargod5.lgc",CRC(03cd4e24) SHA1(b73d383dc71e44277de9116a702b899a54ce32b9))
ZAC_SOUNDROM_0(				"stargod.snd", CRC(5079e493) SHA1(51d366cdd09ad00b8b016b0ea1c85ac95ef94d71))
ZAC_ROMEND
CORE_GAMEDEFNV(stargod,"Star God",1980,"Zaccaria",mZAC1346,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Space Shuttle (09/80)
/-------------------------------*/
INITGAME1(sshtlzac,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(sshtlzac,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
							"spcshtl2.lgc",CRC(0e06771b) SHA1(f30f3727f24219e5047c871fe81c2e172a17cd38),
							"spcshtl3.lgc",CRC(a302e5a9) SHA1(1585f4000d105a7a2be5638ade9ab8668e6c8a5e),
							"spcshtl4.lgc",CRC(a02ee0b5) SHA1(50532bdc347ecfdbd4cc43403ff2cb1dcb1fe1ac),
							"spcshtl5.lgc",CRC(d1dabd9b) SHA1(0d28336764f43fa4d1b23d849b6ec0f60b2b4ecf))
ZAC_SOUNDROM_0(				"spcshtl.snd", CRC(9a61781c) SHA1(0293640653d8cc9532debd31bbb70f025b4e6d03))
ZAC_ROMEND
CORE_GAMEDEFNV(sshtlzac,"Space Shuttle (Zaccaria)",1980,"Zaccaria",mZAC1346,SOUNDFLAG)

/*--------------------------------
/ Earth, Wind & Fire (04/81)
/-------------------------------*/
// Something's wrong (with the rom loading maybe?)
INITGAME1(ewf,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84444(ewf,	"zac_boot.lgc",CRC(62a3da59) SHA1(db571139aff61757f6c0fda6fa0d1fea8257cb15),
						"ewf_2.lgc",   CRC(aa67e0b4) SHA1(4491eff7081fd5e397974fac1156992ce2012d0b),
						"ewf_3.lgc",   CRC(b21bf015) SHA1(ecddfe1d6797c39e094a7f86efabf0abea0fa4af),
						"ewf_4.lgc",   CRC(d110da3f) SHA1(88e27347d209fab5be924f95b0a001476ea92c1f),
						"ewf_5.lgc",   CRC(686c4a4b) SHA1(5a6dcabb162a6a1aa00be954e16bf0904d091aff))
ZAC_SOUNDROM_0(			"stargod.snd", CRC(5079e493) SHA1(51d366cdd09ad00b8b016b0ea1c85ac95ef94d71))
ZAC_ROMEND
CORE_GAMEDEFNV(ewf,"Earth, Wind & Fire",1981,"Zaccaria",mZAC1346,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Locomotion (09/81)
/-------------------------------*/
INITGAME1(locomotn,GEN_ZAC1,dispZAC1,SNDBRD_ZAC1346)
ZAC_ROMSTART84844(locomotn,	"loc-1.fil",  CRC(8d0252a2) SHA1(964dca642fb26eef2c132eca354a0ffce32e25df),
							"loc-2.fil",  CRC(9dbd8601) SHA1(10bc37d2691c7237a14e0718febed2aa7822db23),
							"loc-3.fil",  CRC(8cadea7b) SHA1(e712add828dd22a2b495f0479f949748db21fbf7),
							"loc-4.fil",  CRC(177c89b6) SHA1(23de8208dbbf141952a974514fc752ed2eb6b202),
							"loc-5.fil",  CRC(cad4122a) SHA1(df29914adeb9675abbd9f43dbef23adf2fe96c81))
ZAC_SOUNDROM_0(				"loc-snd.fil",CRC(51ea9d2a) SHA1(9a68687af2c1cad2a261f61a67a625d906c502e1))
ZAC_ROMEND
CORE_GAMEDEFNV(locomotn,"Locomotion",1981,"Zaccaria",mZAC1,SOUNDFLAG)

/*--------------------------------
/ Pinball Champ '82 (04/82) (Is this really different from the '83?)
/-------------------------------*/
INITGAME(pinchp82,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(pinchp82,	"pinchamp.ic1",CRC(1412ec33) SHA1(82c158ec0536f76cbe80e8c12e0047579439a5b7),
							"pinchamp.ic2",CRC(a24ba4c6) SHA1(4f02c4d6cd727fa96a68c72012b0b4a4484397c4),
							"pinchp82.ic3",CRC(ab7a92ac) SHA1(6e810bd6f5934b05b39e34e09a0940f7f5d8fc0c))
ZAC_SOUNDROM_cefg0(			"pbc_1c.snd",  CRC(6e2defe5) SHA1(fcb62da1aed23d9fb9a222862b4b772aad9792a1),
							"pbc_1e.snd",  CRC(703b3cae) SHA1(c7bd021e936fb0fd4bc16d48c3ef1df69d1fe01a),
							"pbc_1f.snd",  CRC(f3f4b950) SHA1(ed5c02f701530d2d6255cc72d695e24d4df40fc3),
							"pbc_1g.snd",  CRC(44adae13) SHA1(0d8d538704db62b41ad5781ec53c34e482342025))
ZAC_ROMEND
CORE_CLONEDEFNV(pinchp82,pinchamp,"Pinball Champ '82 (german speech)",1982,"Zaccaria",mZAC2B,SOUNDFLAG)

/*--------------------------------
/ Soccer Kings (09/82)
/-------------------------------*/
INITGAME(socrking,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(socrking,	"soccer.ic1",CRC(3fbd7c32) SHA1(2f56f67d1ad987638284000cca1e20ff17fcd4f9),
							"soccer.ic2",CRC(0cc0df1f) SHA1(2fd05af0ec63835a8f69fdc50e2faceb829b4df2),
							"soccer.ic3",CRC(5da6ea20) SHA1(c0e740e20a968ee58e0cd12a0089bb047f5e2cbb))
ZAC_SOUNDROM_cefg1(			"sound1.c",  CRC(3aa95018) SHA1(5347c3aefb642fc5cabd9d5e61fe6515a2dcb2aa),
							"sound2.e",  CRC(f9b57fd6) SHA1(50e42ed349680211eedf55ae639dbae899f3c6da),
							"sound3.f",  CRC(d162b022) SHA1(3087554f254e2a8e59ed87feb2770a013d90b53d),
							"sound4.g",  CRC(720593fb) SHA1(93aa9ae1be299548e17b4fe97a7fb4ddab76de40))
ZAC_ROMEND
CORE_GAMEDEFNV(socrking,"Soccer Kings",1982,"Zaccaria",mZAC2A,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Pinball Champ (??/83)
/-------------------------------*/
INITGAME(pinchamp,GEN_ZAC2,dispZAC3,SNDBRD_ZAC1370)
ZAC_ROMSTART000(pinchamp,	"pinchamp.ic1",CRC(1412ec33) SHA1(82c158ec0536f76cbe80e8c12e0047579439a5b7),
							"pinchamp.ic2",CRC(a24ba4c6) SHA1(4f02c4d6cd727fa96a68c72012b0b4a4484397c4),
							"pinchamp.ic3",CRC(df5f4f88) SHA1(249cf958b0998aa41fa26c617be9b6c52c2f5549))
ZAC_SOUNDROM_cefg0(			"sound1.c",    CRC(f739fcba) SHA1(7460f1da99c474601e8cec64683cbd61837a82e8),
							"sound2.e",    CRC(24d83e74) SHA1(f78e151c9885b965cd5209777580414522362ebf),
							"sound3.f",    CRC(d055e8c6) SHA1(0820d941880aa8925b400c792af7ce6b80dcbc48),
							"sound4.g",    CRC(39b68215) SHA1(4d57f1f1f71f7bdbef67ca4cc62cfde80d1ab04c))
ZAC_ROMEND
CORE_GAMEDEFNV(pinchamp,"Pinball Champ",1983,"Zaccaria",mZAC2B,SOUNDFLAG)

/*--------------------------------
/ Time Machine (04/83)
/-------------------------------*/
INITGAME(tmachzac,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(tmachzac,	"timemach.ic1",CRC(d88f424b) SHA1(a0c51f894d604504253f66e49298a9d836e25308),
							"timemach.ic2",CRC(3c313487) SHA1(17c6c4a0c0c6dd90cf7fd9298b945305f734747d))
ZAC_SOUNDROM_de1g(			"sound1.d",    CRC(efc1d724) SHA1(f553767c053e4854fe7839f8c8f4a7f5aefe2692),
							"sound2.e",    CRC(41881a1d) SHA1(42f8dd13c38e11c0dd3cf59c64751baaacb00ac1),
							"sound3.g",    CRC(b7b872da) SHA1(dfeb48a683c6d249101488f244b26509a4c4d81d))
ZAC_ROMEND
CORE_GAMEDEFNV(tmachzac,"Time Machine (Zaccaria)",1983,"Zaccaria",mZAC2C,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Farfalla (09/83)
/-------------------------------*/
INITGAME(farfalla,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(farfalla,	"cpurom1.bin",CRC(ac249150) SHA1(9eac1bf6119cd1fa6cc823faf02b9bf153519a77),
							"cpurom2.bin",CRC(6edc823f) SHA1(b10fcbc308ec06762a2eb35921a7e6a68fd5c9b1))
ZAC_SOUNDROM_de1g(			"rom1.snd",   CRC(aca09674) SHA1(8e1edc25c7fe2189215f73da8f1bec4b670bd8e6),
							"rom2.snd",   CRC(76da384d) SHA1(0e4616bf2fb2c21270aecfc04ad9e68ce9390bfb),
							"rom3.snd",   CRC(d0584952) SHA1(80fe571a2e8a2a34fae03589df930b3eb3fa1f6b))
ZAC_ROMEND
CORE_GAMEDEFNV(farfalla,"Farfalla",1983,"Zaccaria",mZAC2F,SOUNDFLAG)

/*--------------------------------
/ Devil Riders (04/84)
/-------------------------------*/
INITGAME(dvlrider,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(dvlrider,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(5d48462c) SHA1(755bc259e992a9b375bd1e338775da14c15932bd),
						"gb01snd2.1e",CRC(1127be59) SHA1(be074fe3efecd0c1e10599c8981bf7c5debb4d37),
						"gb01snd3.1g",CRC(1ae91ae8) SHA1(05bcc7e509beb5fc2510bca99c39af0bc02530a7))
ZAC_ROMEND
CORE_GAMEDEFNV(dvlrider,"Devil Riders",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

INITGAME(dvlrideg,GEN_ZAC2,dispZAC2,SNDBRD_ZAC13136)
ZAC_ROMSTART1820(dvlrideg,	"cpu.ic1",CRC(5874ab12) SHA1(e616193943797d91e5cf2abfcc052821d24336b4),
							"cpu.ic2",CRC(09829446) SHA1(dc82135eae544f8eb1a3227bc6de0bd9a464e778))
ZAC_SOUNDROM_de2g(	"g_snd_1.bin",CRC(77d042dc) SHA1(78e056468887a315e29c913803e3c36f9c7f694e),
					"g_snd_2.bin",CRC(31e35fd4) SHA1(2eeefbd831159d975fe9cac99db99dfdca04b0dc),
					"g_snd_3.bin",CRC(2e64a401) SHA1(694808963d6e6a02ddeb9228073825ff16f91d49))
ZAC_ROMEND
CORE_CLONEDEFNV(dvlrideg,dvlrider,"Devil Riders (german speech)",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

/*--------------------------------
/ Magic Castle (09/84)
/-------------------------------*/
INITGAME(mcastle,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(mcastle,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(		"gb01snd1.1d",CRC(cd6a4a07) SHA1(47359747f79feca9d85d8f7657325874eda0f915),
						"gb01snd2.1e",CRC(d289952d) SHA1(94052cbee4cd499fb55d59b047828c21d71ab288),
						"gb01snd3.1g",CRC(8b4342eb) SHA1(a8534cb2ebaff4d5d4101eb710c068f3b91e9e0c))
ZAC_ROMEND
CORE_GAMEDEFNV(mcastle,"Magic Castle",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

INITGAME(mcastleg,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(mcastleg,	"cpu.ic1",CRC(50dd8209) SHA1(c1df8ea16d8a8ae1d6f524fa25c94c4664f314c1),
							"cpu.ic2",CRC(11372bec) SHA1(bd822c0ee455941630cdade83335c84224d351e4))
ZAC_SOUNDROM_de2g(	"magic1d.snd",CRC(1f1a1140) SHA1(fed351c78e4c46c05e910f1844351492faa9edcf),
					"magic1e.snd",CRC(a8787011) SHA1(16da0b40e24346f4e90d553c7c3e68daa4d4a656),
					"magic1g.snd",CRC(313fb216) SHA1(1065f057654dd41cdac0553e1c315edf141f1d19))
ZAC_ROMEND
CORE_CLONEDEFNV(mcastleg,mcastle,"Magic Castle (german speech)",1984,"Zaccaria",mZAC2X,SOUNDFLAG)

/*--------------------------------
/ Robot (01/85)
/-------------------------------*/
// There might be a bad rom on this one.
INITGAME(robot,GEN_ZAC2,dispZAC3,SNDBRD_ZAC13136)
ZAC_ROMSTART020(robot,	"robot_1.lgc",CRC(96a87432) SHA1(d6a7e4fa6221bea0ab3bb6e7dc46d1732b29d53e),
						"robot_2.lgc",CRC(28ba9687) SHA1(8e99834328783361856fa9632b2c6e3a5a05d49b))
ZAC_SOUNDROM_de2g(		"robot_d.snd",CRC(88685b1e) SHA1(7d49a1d42f3e07948390a00a562aeba9dd4ddeeb),
						"robot_e.snd",CRC(e326a851) SHA1(c2bb5e329803922fa1c1ca30be6e3ae3d292135a),
						"robot_g.snd",CRC(7ed5da55) SHA1(d70f1f470cf9d300375600352f9625b4e34f5ed3))
ZAC_ROMEND
CORE_GAMEDEFNV(robot,"Robot",1985,"Zaccaria",mZAC2F,GAME_NOT_WORKING|SOUNDFLAG)

/*--------------------------------
/ Clown (07/85)
/-------------------------------*/
INITGAME(clown,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART020(clown,	"clown_1.lgc",CRC(16f09833) SHA1(5c9c8b9403d8b69ae7252bf904edc617784b8165),
						"clown_2.lgc",CRC(697e6b5b) SHA1(d2c459cbffec94730eb2abe3c63b4913a18085a7))
ZAC_SOUNDROM_e2f2(		"clown_e.snd",CRC(04a34cc1) SHA1(56fcc07ccab3cac27928f5c5411868bde1769603),
						"clown_f.snd",CRC(e35a4f72) SHA1(0037c1072f58798ba61af85a1b4b374b85c883ae))
ZAC_ROMEND
CORE_GAMEDEFNV(clown,"Clown",1985,"Zaccaria",mZAC2XS,SOUNDFLAG)

/*--------------------------------
/ Pool Champion (12/85)
/-------------------------------*/
INITGAME(poolcham,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART020(poolcham,	"poolcham.ic1",CRC(fca2a2b2) SHA1(9a0d9c495e38628c5e0bc10f6335100eb934f153),
							"poolcham.ic2",CRC(267a2a02) SHA1(049ada7bfcf0d8560ac03effd3fbb02ead51933c))
ZAC_SOUNDROM_f(				"poolcham.1f", CRC(efe33926) SHA1(30444a2ee7f453f46c74fff8365d80fc4f0a277f))
// ZAC_SOUNDROM_f(			"sound1.f",    CRC(b4b4e31e) SHA1(bcd1c4c7f6f079655a9c37d0b978d997f95b93ad)) // different language?
ZAC_ROMEND
CORE_GAMEDEFNV(poolcham,"Pool Champion",1985,"Zaccaria",mZAC2DS,SOUNDFLAG)

/*--------------------------------
/ Black Belt (03/86)
/-------------------------------*/
INITGAME(bbeltzac,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART1820(bbeltzac,	"bbz-1.fil",CRC(2e7e1575) SHA1(1b9e6e4ff461962f4c7249bd2a748444cb658c30),
							"bbz-2.fil",CRC(dbec92ae) SHA1(7a1c6e5ac81d3cfcbb135a1c8b69e55296fffcc5))
ZAC_SOUNDROM_e2f4(			"bbz-e.snd",CRC(1fe045d2) SHA1(d17d7dbcafe9f8644cbe393a56ff6b45d9d40155),
							"bbz-f.snd",CRC(9f58f369) SHA1(32472d93284c0f1fc2875714b40428406dcf6325))
ZAC_ROMEND
CORE_GAMEDEFNV(bbeltzac,"Black Belt (Zaccaria)",1986,"Zaccaria",mZAC2DS,SOUNDFLAG)

/*--------------------------------
/ Mexico '86 (07/86)
/-------------------------------*/
INITGAME(mexico,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART1820(mexico,	"mex86_1.lgc",CRC(60d559b1) SHA1(1097f32dd0c89b6e3653a620e39696d8ab1289fc),
							"mex86_2.lgc",CRC(5c984c15) SHA1(c6228568cee6a365a3c552a57e5e1e0445108bad))
ZAC_SOUNDROM_e2f4(			"mex86_e.snd",CRC(a985e8db) SHA1(11f91179fa1d46c1c83cdd4fbcf8ebdfd2a41f3f),
							"mex86_f.snd",CRC(301c2b63) SHA1(df4a4cb48d28d53c3728066d3e3fa9eac17c78c5))
ZAC_ROMEND
CORE_GAMEDEFNV(mexico,"Mexico '86",1986,"Zaccaria",mZAC2FS,SOUNDFLAG)

/*--------------------------------
/ Zankor (12/86)
/-------------------------------*/
INITGAME(zankor,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART1820(zankor,	"zan_ic1.764",CRC(e7ba5acf) SHA1(48b64921dd8a22c2483162db571512cad8cbb072),
							"zan_ic2.764",CRC(5804ff10) SHA1(fc3c4acb183c5c3e0a6504583c78f25a7a322cce))
ZAC_SOUNDROM_e4f4(			"zan_1e.128", CRC(d467000f) SHA1(ee806c485520042755078c49a020b5bc06913fa3),
							"zan_1f.128", CRC(74fcadc9) SHA1(efd6fc99d7a3ed8e59fbbafbee161af6fb527028))
ZAC_SOUNDROM_456(			"zan_ic4.128",CRC(f34a2aaa) SHA1(5e415874f68586aa30dba9fff0dc8990c636cecd),
							"zan_ic5.128",CRC(bf61aab0) SHA1(939266696d0562f255f0fa5068280fe6a4cf8267),
							"zan_ic6.128",CRC(13a5b8d4) SHA1(d8c976b3f5e9c7cded0922feefa1531c59432515))
ZAC_ROMEND
CORE_GAMEDEFNV(zankor,"Zankor",1986,"Zaccaria",mZAC2FS,SOUNDFLAG)

/*--------------------------------
/ Spooky (04/87)
/-------------------------------*/
INITGAME(spooky,GEN_ZAC2,dispZAC3,SNDBRD_ZAC11178)
ZAC_ROMSTART1820(spooky,	"spook_1.lgc",CRC(377b347d) SHA1(c7334cf2b10b749f5f75b8feaa8ec773a576b2f1),
							"spook_2.lgc",CRC(ae0598b0) SHA1(aab725d1e386a3792100eb55c5836e6ed68cafdd))
ZAC_SOUNDROM_e2f4(			"spook_e.snd",CRC(3d632c93) SHA1(3cc127956a6df1a4fd551826068810724b32ad0e),
							"spook_f.snd",CRC(cc04a448) SHA1(e837a7d7640aa1d2c2880616bd377b64dc8fac9d))
ZAC_SOUNDROM_46(			"spook_4.snd",CRC(3ab517a4) SHA1(4a9dd9d571f958c270b437a1665e6d3dd3eef598),
							"spook_6.snd",CRC(d4320bc7) SHA1(30b959f5df44d097baffc2de70b12fc767f5663b))
ZAC_ROMEND
CORE_GAMEDEFNV(spooky,"Spooky",1987,"Zaccaria",mZAC2XS,SOUNDFLAG)

//??/87 Star's Phoenix
//??/86 New Star's Phoenix

/*--------------------------------
/ Scramble (Technoplay 1987)
/-------------------------------*/
INITGAME(scram_tp,GEN_ZAC2,dispZAC2,SNDBRD_TECHNO)
ZAC_ROMSTART1820(scram_tp,	"scram_1.lgc",CRC(da565549) SHA1(d187801428824df2b506c999548a5c6d146bc59e),
							"scram_2.lgc",CRC(537e6c61) SHA1(84e0db4268d3c990c3834ebd20bf7c475a70082d))
TECHNO_SOUNDROM1("scram_1.snd",CRC(ee5f868b) SHA1(23ef4112b94109ad4d4a6b9bb5215acec20e5e55),
                 "scram_2.snd",CRC(a04bf7d0) SHA1(5be5d445b199e7dc9d42e7ee5e9b31c18dec3881))
TECHNO_SOUNDROM2("scram_3.snd",CRC(ed27cd78) SHA1(a062ee1a3ec8819acddac13a4b454f5fd95d1e29),
                 "scram_4.snd",CRC(943f279d) SHA1(52767708d706a01ea16e37c866eb5762297e1f86),
                 "scram_5.snd",CRC(3aa782ec) SHA1(7cbbd3a737239b2755c6a6651a284e83fcfa22f6))
ZAC_ROMEND
CORE_GAMEDEFNV(scram_tp,"Scramble",1987,"Technoplay",mTECHNO,0)
