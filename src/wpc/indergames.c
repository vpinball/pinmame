#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "inder.h"
#include "spinb.h"
#include "sndbrd.h"

#define GEN_INDER 0

#define INITGAME(name, disptype, lamps, inv1,inv2,inv3,inv4,inv5) \
	INDER_INPUT_PORTS_START(name, 1) INDER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_INDER,disptype,{FLIP_SW(FLIP_L), 0,lamps,0, SNDBRD_SPINB},NULL,{"",{0,inv1,inv2,inv3,inv4,inv5}}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout inderDisp6[] = {
  {0, 0, 2,6,CORE_SEG7}, {2, 0,10,6,CORE_SEG7},
  {7, 0,18,6,CORE_SEG7}, {9, 0,26,6,CORE_SEG7},
  {5, 3,34,2,CORE_SEG7S},{5, 9,38,2,CORE_SEG7S},
  {0}
};

static core_tLCDLayout inderDisp7[] = {
  {0, 0, 1,7,CORE_SEG7}, {2, 0, 9,7,CORE_SEG7},
  {7, 0,17,7,CORE_SEG7}, {9, 0,25,7,CORE_SEG7},
  {5, 4,34,2,CORE_SEG7S},{5,10,38,2,CORE_SEG7S},
  {0}
};

static core_tLCDLayout inderDisp7a[] = {
  {0, 0, 1,7,CORE_SEG7}, {3, 0, 9,7,CORE_SEG7},
  {0,22,17,7,CORE_SEG7}, {3,22,25,7,CORE_SEG7},
  {1,20,34,2,CORE_SEG7S},{1,25,38,2,CORE_SEG7S},
  {3,20,41,1,CORE_SEG7S},
  {0}
};

/*-------------------------------------------------------------------
/ Brave Team (1985)
/-------------------------------------------------------------------*/
INITGAME(brvteam, inderDisp6, 0, 0,0,0,0,0)
INDER_ROMSTART(brvteam,	"brv-tea.m0", CRC(1fa72160) SHA1(0fa779ce2604599adff1e124d0b161b69094a614),
						"brv-tea.m1", CRC(4f02ca47) SHA1(68ec7d48c335a1ddd808feaeccac04a4f63d1a33))
INDER_ROMEND
CORE_GAMEDEFNV(brvteam,"Brave Team",1985,"Inder (Spain)",gl_mINDER0,0)

#define init_brvteafp init_brvteam
#define input_ports_brvteafp input_ports_brvteam
INDER_ROMSTART(brvteafp,	"brvteam.fp0", CRC(e10a5800) SHA1(48311b354089531a50de3ee8a2f0f507ade5509e),
						"brvteam.fp1", CRC(368b5a28) SHA1(8545897ced58a81b0bee097445535e01c50b14e6))
INDER_ROMEND
CORE_CLONEDEFNV(brvteafp,brvteam,"Brave Team (Free Play)",1985,"Inder (Spain)",gl_mINDER0,0)

/*-------------------------------------------------------------------
/ Canasta '86' (1986)
/-------------------------------------------------------------------*/
INITGAME(canasta, inderDisp7, 0, 0,0,0x08,0,0)
INDER_ROMSTART(canasta,	"c860.bin", CRC(b1f79e52) SHA1(8e9c616f9be19d056da2f86778539d62c0885bac),
						"c861.bin", CRC(25ae3994) SHA1(86dcda3278fbe0e57b8ff4858b955d067af414ce))
INDER_ROMEND
CORE_GAMEDEFNV(canasta,"Canasta '86'",1986,"Inder (Spain)",gl_mINDER1,0)

#define init_canastfp init_canasta
#define input_ports_canastfp input_ports_canasta
INDER_ROMSTART(canastfp,	"c860.bin", CRC(b1f79e52) SHA1(8e9c616f9be19d056da2f86778539d62c0885bac),
						"c861_fp.bin", CRC(673c2592) SHA1(f1cc931e5c54e01f41025fafb87e91acc0828adf))
INDER_ROMEND
CORE_CLONEDEFNV(canastfp,canasta,"Canasta '86' (Free Play)",1986,"Inder (Spain)",gl_mINDER1,0)

/*-------------------------------------------------------------------
/ Lap By Lap (1986)
/-------------------------------------------------------------------*/
static core_tLCDLayout lblDisp[] = {
  DISP_SEG_IMPORT(inderDisp7),
  {12,4,48,2,CORE_SEG7S},{12,10,50,2,CORE_SEG7S},
  {0}
};
INITGAME(lapbylap, lblDisp, 0, 0,0,0x0c,0,0)
INDER_ROMSTART(lapbylap,"lblr0.bin", CRC(2970f31a) SHA1(01fb774de19944bb3a19577921f84ab5b6746afb),
						"lblr1.bin", CRC(94787c10) SHA1(f2a5b07e57222ee811982eb220c239e34a358d6f))
INDER_SNDROM(			"lblsr0.bin",CRC(cbaddf02) SHA1(8207eebc414d90328bfd521190d508b88bb870a2))
INDER_ROMEND
CORE_GAMEDEFNV(lapbylap,"Lap By Lap",1986,"Inder (Spain)",gl_mINDER2,0)

#define init_lapbylfp init_lapbylap
#define input_ports_lapbylfp input_ports_lapbylap
INDER_ROMSTART(lapbylfp,"lblr0.bin", CRC(2970f31a) SHA1(01fb774de19944bb3a19577921f84ab5b6746afb),
						"lblr1_fp.bin", CRC(633f6348) SHA1(7577ecf185ae67fcb24b0008635be135177443b6))
INDER_SNDROM(			"lblsr0.bin",CRC(cbaddf02) SHA1(8207eebc414d90328bfd521190d508b88bb870a2))
INDER_ROMEND
CORE_CLONEDEFNV(lapbylfp,lapbylap,"Lap By Lap (Free Play)",1986,"Inder (Spain)",gl_mINDER2,0)

/*-------------------------------------------------------------------
/ Moon Light (1987)
/-------------------------------------------------------------------*/
INITGAME(moonlght, lblDisp, 0, 0,0,0x0c,0,0)
INDER_ROMSTART1(moonlght,"ci-3.bin", CRC(56b901ae) SHA1(7269d1a100c378b21454f9f80f5bd9fbb736c222))
INDER_SNDROM4(			"ci-11.bin", CRC(a0732fe4) SHA1(54f62cd81bdb7e1924acb67ddbe43eb3d0a4eab0),
						"ci-24.bin", CRC(6406bd18) SHA1(ae45ed9e8b1fd278a36a68b780352dbbb6ee781e),
						"ci-23.bin", CRC(eac346da) SHA1(7c4c26ae089dda0dcd7300fd1ecabf5a91099c41),
						"ci-22.bin", CRC(379740da) SHA1(83ad13ab7f1f37c78397d8e830bd74c5a7aea758),
						"ci-21.bin", CRC(0febb4a7) SHA1(e6cc1b26ddfe9cd58da29de2a50a83ce50afe323))
INDER_ROMEND
CORE_GAMEDEFNV(moonlght,"Moon Light",1987,"Inder (Spain)",gl_mINDERS0,0)

#define init_moonlifp init_moonlght
#define input_ports_moonlifp input_ports_moonlght
INDER_ROMSTART1(moonlifp,"ci-3-fp.bin", CRC(7affab1e) SHA1(e31b35a7abd2921468ebcb4057e4e91c6731c1e3))
INDER_SNDROM4(			"ci-11.bin", CRC(a0732fe4) SHA1(54f62cd81bdb7e1924acb67ddbe43eb3d0a4eab0),
						"ci-24.bin", CRC(6406bd18) SHA1(ae45ed9e8b1fd278a36a68b780352dbbb6ee781e),
						"ci-23.bin", CRC(eac346da) SHA1(7c4c26ae089dda0dcd7300fd1ecabf5a91099c41),
						"ci-22.bin", CRC(379740da) SHA1(83ad13ab7f1f37c78397d8e830bd74c5a7aea758),
						"ci-21.bin", CRC(0febb4a7) SHA1(e6cc1b26ddfe9cd58da29de2a50a83ce50afe323))
INDER_ROMEND
CORE_CLONEDEFNV(moonlifp,moonlght,"Moon Light (Free Play)",1987,"Inder (Spain)",gl_mINDERS0,0)

/*-------------------------------------------------------------------
/ Clown (1988)
/-------------------------------------------------------------------*/
INITGAME(pinclown, inderDisp7, 0, 0,0,0,0,0)
INDER_ROMSTART1(pinclown,"clown_a.bin", CRC(b7c3f9ab) SHA1(89ede10d9e108089da501b28f53cd7849f791a00))
INDER_SNDROM4(			"clown_b.bin", CRC(c223c961) SHA1(ed5180505b6ebbfb9451f67a44d07df3555c8f8d),
						"clown_c.bin", CRC(dff89319) SHA1(3745a02c3755d11ea7fb552f7a5df2e8bbee2c29),
						"clown_d.bin", CRC(cce4e1dc) SHA1(561c9331d2d110d34cf250cd7b25be16a72a1d79),
						"clown_e.bin", CRC(98263526) SHA1(509764e65847637824ba93f7e6ce926501c431ce),
						"clown_f.bin", CRC(5f01b531) SHA1(116b1670ef4d5c054bb09dc55aa7d5d3ca047079))
INDER_ROMEND
CORE_GAMEDEFNV(pinclown,"Clown (Inder)",1988,"Inder (Spain)",gl_mINDERS1,0)

#define init_pinclofp init_pinclown
#define input_ports_pinclofp input_ports_pinclown
INDER_ROMSTART1(pinclofp,"clown_fp.bin", CRC(f020a0e3) SHA1(5a4abc58d5e21b602082bcd2d67947266cae41b6))
INDER_SNDROM4(			"clown_b.bin", CRC(c223c961) SHA1(ed5180505b6ebbfb9451f67a44d07df3555c8f8d),
						"clown_c.bin", CRC(dff89319) SHA1(3745a02c3755d11ea7fb552f7a5df2e8bbee2c29),
						"clown_d.bin", CRC(cce4e1dc) SHA1(561c9331d2d110d34cf250cd7b25be16a72a1d79),
						"clown_e.bin", CRC(98263526) SHA1(509764e65847637824ba93f7e6ce926501c431ce),
						"clown_f.bin", CRC(5f01b531) SHA1(116b1670ef4d5c054bb09dc55aa7d5d3ca047079))
INDER_ROMEND
CORE_CLONEDEFNV(pinclofp,pinclown,"Clown (Inder, Free Play)",1988,"Inder (Spain)",gl_mINDERS1,0)

/*-------------------------------------------------------------------
/ Corsario (1989)
/-------------------------------------------------------------------*/
INITGAME(corsario, inderDisp7a, 0, 0,0x10,0,0,0)
INDER_ROMSTART1(corsario,"0-corsar.bin", CRC(800f6895) SHA1(a222e7ea959629202686815646fc917ffc5a646c))
INDER_SNDROM4(			"a-corsar.bin", CRC(e14b7918) SHA1(5a5fc308b0b70fe041b81071ba4820782b6ff988),
						"b-corsar.bin", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"c-corsar.bin", CRC(047fd722) SHA1(2385507459f85c68141adc7084cb51dfa02462f6),
						"d-corsar.bin", CRC(10d8b448) SHA1(ed1918e6c55eba07dde31b9755c9403e073cad98),
						"e-corsar.bin", CRC(918ee349) SHA1(17cded8b5626c91e400d26332a160704f2fd2b55))
INDER_ROMEND
CORE_GAMEDEFNV(corsario,"Corsario",1989,"Inder (Spain)",gl_mINDERS1,0)

#define init_corsarfp init_corsario
#define input_ports_corsarfp input_ports_corsario
INDER_ROMSTART1(corsarfp,"fpcorsar.bin", CRC(d7df80f0) SHA1(073eb24137d3510e89b08e42716c955d81de0ce1))
INDER_SNDROM4(			"a-corsar.bin", CRC(e14b7918) SHA1(5a5fc308b0b70fe041b81071ba4820782b6ff988),
						"b-corsar.bin", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"c-corsar.bin", CRC(047fd722) SHA1(2385507459f85c68141adc7084cb51dfa02462f6),
						"d-corsar.bin", CRC(10d8b448) SHA1(ed1918e6c55eba07dde31b9755c9403e073cad98),
						"e-corsar.bin", CRC(918ee349) SHA1(17cded8b5626c91e400d26332a160704f2fd2b55))
INDER_ROMEND
CORE_CLONEDEFNV(corsarfp,corsario,"Corsario (Free Play)",1989,"Inder (Spain)",gl_mINDERS1,0)

/*-------------------------------------------------------------------
/ Mundial 90 (1990)
/-------------------------------------------------------------------*/
INITGAME(mundial, inderDisp7a, 0, 0,0x10,0,0,0)
INDER_ROMSTART1(mundial,"mundial.cpu", CRC(b615e69b) SHA1(d129eb6f2943af40ddffd0da1e7a711b58f65b3c))
INDER_SNDROM4(			"snd11.bin", CRC(2cebc1a5) SHA1(e0dae2b1ce31ff436b55ceb1ec71d39fc56694da),
						"snd24.bin", CRC(603bfc3c) SHA1(8badd9731243270ce5b8003373ed09ec7eac6ca6),
						"snd23.bin", CRC(2868ce6f) SHA1(317457763f764be08cbe6a5dd4008ba2257c9d78),
						"snd22.bin", CRC(2559f874) SHA1(cbf57f29e394d5dc320e7dcbd2625f6c96412a06),
						"snd21.bin", CRC(7a8f7402) SHA1(39666ba2634fe9c720c2c9bcc9ccc73874ed85e7))
INDER_ROMEND
CORE_GAMEDEFNV(mundial,"Mundial 90",1990,"Inder (Spain)",gl_mINDERS1,0)

#define init_mundiafp init_mundial
#define input_ports_mundiafp input_ports_mundial
INDER_ROMSTART1(mundiafp,"mundiafp.cpu", CRC(f335411c) SHA1(65d75c7f53f4793d5b5bbf16984e20950c6eae7c))
INDER_SNDROM4(			"snd11.bin", CRC(2cebc1a5) SHA1(e0dae2b1ce31ff436b55ceb1ec71d39fc56694da),
						"snd24.bin", CRC(603bfc3c) SHA1(8badd9731243270ce5b8003373ed09ec7eac6ca6),
						"snd23.bin", CRC(2868ce6f) SHA1(317457763f764be08cbe6a5dd4008ba2257c9d78),
						"snd22.bin", CRC(2559f874) SHA1(cbf57f29e394d5dc320e7dcbd2625f6c96412a06),
						"snd21.bin", CRC(7a8f7402) SHA1(39666ba2634fe9c720c2c9bcc9ccc73874ed85e7))
INDER_ROMEND
CORE_CLONEDEFNV(mundiafp,mundial,"Mundial 90 (Free Play)",1990,"Inder (Spain)",gl_mINDERS1,0)

/*-------------------------------------------------------------------
/ Atleta (1991)
/-------------------------------------------------------------------*/
INITGAME(atleta, inderDisp7a, 0, 0,0,0,0,0x10)
INDER_ROMSTART2(atleta,"atleta0.cpu", CRC(5f27240f) SHA1(8b77862fa311d703b3af8a1db17e13b17dca7ec6),
						"atleta1.cpu", CRC(12bef582) SHA1(45e1da318141d9228bc91a4e09fff6bf6f194235))
INDER_SNDROM4(			"atletaa.snd", CRC(051c5329) SHA1(339115af4a2e3f1f2c31073cbed1842518d5916e),
						"atletab.snd", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"atletac.snd", CRC(20456363) SHA1(b226400dac35dedc039a7e03cb525c6033b24ebc),
						"atletad.snd", CRC(6518e3a4) SHA1(6b1d852005dabb76c7c65b87ecc9ee1422f16737),
						"atletae.snd", CRC(1ef7b099) SHA1(08400db3e238baf1673a2da604c999db6be30ffe))
INDER_ROMEND
CORE_GAMEDEFNV(atleta,"Atleta",1991,"Inder (Spain)",gl_mINDERS1,0)

/*-------------------------------------------------------------------
/ 250 c.c. (1992)
/-------------------------------------------------------------------*/
INITGAME(ind250cc, inderDisp7a, 0, 0,0,0,0,0)
INDER_ROMSTART1(ind250cc,"0-250cc.bin", CRC(753d82ec) SHA1(61950336ba571f9f75f2fc31ccb7beaf4e05dddc))
INDER_SNDROM4(			"a-250cc.bin", CRC(b64bdafb) SHA1(eab6d54d34b44187d454c1999e4bcf455183d5a0),
						"b-250cc.bin", CRC(884c31c8) SHA1(23a838f1f0cb4905fa8552579b5452134f0fc9cc),
						"c-250cc.bin", CRC(5a1dfa1d) SHA1(4957431d87be0bb6d27910b718f7b7edcd405fff),
						"d-250cc.bin", CRC(a0940387) SHA1(0e06483e3e823bf4673d8e0bd120b0a6b802035d),
						"e-250cc.bin", CRC(538b3274) SHA1(eb76c41a60199bb94aec4666222e405bbcc33494))
INDER_ROMEND
CORE_GAMEDEFNV(ind250cc,"250 c.c.",1992,"Inder (Spain)",gl_mINDERS1,0)

/*-------------------------------------------------------------------
/ Metal Man (1992)
/-------------------------------------------------------------------*/
INITGAME(metalman, inderDisp7a, 8, 0,0,0,0,0)
INDER_ROMSTART2(metalman,	"cpu_0.bin", CRC(7fe4335b) SHA1(52ef2efa29337eebd8c2c9a8aec864356a6829b6),
						"cpu_1.bin", CRC(2cca735e) SHA1(6a76017dfbcac0d57fcec8f07f92d5e04dd3e00b))
NORMALREGION(0x10000, REGION_CPU2)
  ROM_LOAD("sound_e1.bin", 0x0000, 0x2000, CRC(55e889e8) SHA1(0a240868c1b17762588c0ed9a14f568a6e50f409))
NORMALREGION(0x80000, REGION_USER1)
  ROM_LOAD("sound_e2.bin", 0x00000, 0x20000, CRC(5ac61535) SHA1(75b9a805f8639554251192e3777073c29952c78f))
NORMALREGION(0x10000, REGION_CPU3)
  ROM_LOAD("sound_m1.bin", 0x0000, 0x2000, CRC(21a9ee1d) SHA1(d906ac7d6e741f05e81076a5be33fc763f0de9c1))
NORMALREGION(0x80000, REGION_USER2)
  ROM_LOAD("sound_m2.bin", 0x00000, 0x20000, CRC(349df1fe) SHA1(47e7ddbdc398396e40bb5340e5edcb8baf06c255))
  ROM_LOAD("sound_m3.bin", 0x40000, 0x20000, CRC(15ef1866) SHA1(4ffa3b29bf3c30a9a5bc622adde16a1a13833b22))
INDER_ROMEND
CORE_GAMEDEFNV(metalman,"Metal Man",1992,"Inder (Spain)",gl_mINDERS2,0)
