#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "inder.h"
#include "sndbrd.h"

#define GEN_INDER 0

#define INITGAME(name, disptype, balls, inv1,inv2,inv3,inv4,inv5) \
	INDER_INPUT_PORTS_START(name, balls) INDER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_INDER,disptype,{FLIP_SW(FLIP_L), 0,0,0, SNDBRD_SPINB},NULL,{"",{0,inv1,inv2,inv3,inv4,inv5}}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout inderDisp6[] = {
  {0, 0, 2,6,CORE_SEG7}, {2, 0,10,6,CORE_SEG7},
  {6, 0,18,6,CORE_SEG7}, {8, 0,26,6,CORE_SEG7},
  {4, 1,34,2,CORE_SEG7}, {4, 7,38,2,CORE_SEG7},
  {0}
};

static core_tLCDLayout inderDisp7[] = {
  {0, 0, 1,7,CORE_SEG7}, {2, 0, 9,7,CORE_SEG7},
  {6, 0,17,7,CORE_SEG7}, {8, 0,25,7,CORE_SEG7},
  {4, 2,34,2,CORE_SEG7}, {4, 8,38,2,CORE_SEG7},
  {0}
};

static core_tLCDLayout inderDisp7a[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,22, 9,7,CORE_SEG7},
  {3, 0,17,7,CORE_SEG7}, {3,22,25,7,CORE_SEG7},
  {1,20,34,2,CORE_SEG7S},{1,25,38,2,CORE_SEG7S},
  {4,20,41,1,CORE_SEG7S},
  {0}
};

/*-------------------------------------------------------------------
/ Brave Team (1985)
/-------------------------------------------------------------------*/
INITGAME(brvteam, inderDisp6, 1, 0,0,0,0,0)
INDER_ROMSTART(brvteam,	"brv-tea.m0", CRC(1fa72160) SHA1(0fa779ce2604599adff1e124d0b161b69094a614),
						"brv-tea.m1", CRC(4f02ca47) SHA1(68ec7d48c335a1ddd808feaeccac04a4f63d1a33))
INDER_ROMEND
CORE_GAMEDEFNV(brvteam,"Brave Team",1985,"Inder (Spain)",gl_mINDER0,0)

/*-------------------------------------------------------------------
/ Canasta '86' (1986)
/-------------------------------------------------------------------*/
INITGAME(canasta, inderDisp7, 1, 0,0,0x08,0,0)
INDER_ROMSTART(canasta,	"c860.bin", CRC(b1f79e52) SHA1(8e9c616f9be19d056da2f86778539d62c0885bac),
						"c861.bin", CRC(25ae3994) SHA1(86dcda3278fbe0e57b8ff4858b955d067af414ce))
INDER_ROMEND
CORE_GAMEDEFNV(canasta,"Canasta '86'",1986,"Inder (Spain)",gl_mINDER1,0)

/*-------------------------------------------------------------------
/ Lap By Lap (1986)
/-------------------------------------------------------------------*/
static core_tLCDLayout lblDisp[] = {
  DISP_SEG_IMPORT(inderDisp7),
  {10,2,48,2,CORE_SEG7}, {10,8,50,2,CORE_SEG7},
  {0}
};
INITGAME(lapbylap, lblDisp, 1, 0,0,0x0c,0,0)
INDER_ROMSTART(lapbylap,"lblr0.bin", CRC(2970f31a) SHA1(01fb774de19944bb3a19577921f84ab5b6746afb),
						"lblr1.bin", CRC(94787c10) SHA1(f2a5b07e57222ee811982eb220c239e34a358d6f))
INDER_SNDROM(			"lblsr0.bin",CRC(cbaddf02) SHA1(8207eebc414d90328bfd521190d508b88bb870a2))
INDER_ROMEND
CORE_GAMEDEFNV(lapbylap,"Lap By Lap",1986,"Inder (Spain)",gl_mINDER2,0)

/*-------------------------------------------------------------------
/ Clown (1988)
/-------------------------------------------------------------------*/
INITGAME(pinclown, inderDisp7, 1, 0,0,0,0,0)
INDER_ROMSTART1(pinclown,"clown_a.bin", CRC(b7c3f9ab) SHA1(89ede10d9e108089da501b28f53cd7849f791a00))
INDER_SNDROM4(			"clown_b.bin", CRC(81a66302) SHA1(3d1243ae878747f20e54cd3322c5a54ded45ce21),
						"clown_c.bin", CRC(dff89319) SHA1(3745a02c3755d11ea7fb552f7a5df2e8bbee2c29),
						"clown_d.bin", CRC(cce4e1dc) SHA1(561c9331d2d110d34cf250cd7b25be16a72a1d79),
						"clown_e.bin", CRC(98263526) SHA1(509764e65847637824ba93f7e6ce926501c431ce),
						"clown_f.bin", CRC(5f01b531) SHA1(116b1670ef4d5c054bb09dc55aa7d5d3ca047079))
INDER_ROMEND
CORE_GAMEDEFNV(pinclown,"Clown (Inder)",1988,"Inder (Spain)",gl_mINDERS,0)

/*-------------------------------------------------------------------
/ Corsario (1989)
/-------------------------------------------------------------------*/
INITGAME(corsario, inderDisp7a, 1, 0,0x10,0,0,0)
INDER_ROMSTART1(corsario,"0-corsar.bin", CRC(800f6895) SHA1(a222e7ea959629202686815646fc917ffc5a646c))
INDER_SNDROM4(			"a-corsar.bin", CRC(e14b7918) SHA1(5a5fc308b0b70fe041b81071ba4820782b6ff988),
						"b-corsar.bin", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"c-corsar.bin", CRC(047fd722) SHA1(2385507459f85c68141adc7084cb51dfa02462f6),
						"d-corsar.bin", CRC(10d8b448) SHA1(ed1918e6c55eba07dde31b9755c9403e073cad98),
						"e-corsar.bin", CRC(918ee349) SHA1(17cded8b5626c91e400d26332a160704f2fd2b55))
INDER_ROMEND
CORE_GAMEDEFNV(corsario,"Corsario",1989,"Inder (Spain)",gl_mINDERS,0)

/*-------------------------------------------------------------------
/ Atleta (1991)
/-------------------------------------------------------------------*/
INITGAME(atleta, inderDisp7a, 1, 0,0,0,0,0x10)
INDER_ROMSTART2(atleta,"atleta0.cpu", CRC(5f27240f) SHA1(8b77862fa311d703b3af8a1db17e13b17dca7ec6),
						"atleta1.cpu", CRC(12bef582) SHA1(45e1da318141d9228bc91a4e09fff6bf6f194235))
INDER_SNDROM4(			"atletaa.snd", CRC(051c5329) SHA1(339115af4a2e3f1f2c31073cbed1842518d5916e),
						"atletab.snd", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"atletac.snd", CRC(20456363) SHA1(b226400dac35dedc039a7e03cb525c6033b24ebc),
						"atletad.snd", CRC(6518e3a4) SHA1(6b1d852005dabb76c7c65b87ecc9ee1422f16737),
						"atletae.snd", CRC(1ef7b099) SHA1(08400db3e238baf1673a2da604c999db6be30ffe))
INDER_ROMEND
CORE_GAMEDEFNV(atleta,"Atleta",1991,"Inder (Spain)",gl_mINDERS,0)

/*-------------------------------------------------------------------
/ 250 CC (1992)
/-------------------------------------------------------------------*/
INITGAME(ind250cc, inderDisp7a, 1, 0,0,0,0,0)
INDER_ROMSTART1(ind250cc,"0-250cc.bin", CRC(753d82ec) SHA1(61950336ba571f9f75f2fc31ccb7beaf4e05dddc))
INDER_SNDROM4(			"a-250cc.bin", CRC(b64bdafb) SHA1(eab6d54d34b44187d454c1999e4bcf455183d5a0),
						"b-250cc.bin", CRC(884c31c8) SHA1(23a838f1f0cb4905fa8552579b5452134f0fc9cc),
						"c-250cc.bin", CRC(5a1dfa1d) SHA1(4957431d87be0bb6d27910b718f7b7edcd405fff),
						"d-250cc.bin", CRC(a0940387) SHA1(0e06483e3e823bf4673d8e0bd120b0a6b802035d),
						"e-250cc.bin", CRC(538b3274) SHA1(eb76c41a60199bb94aec4666222e405bbcc33494))
INDER_ROMEND
CORE_GAMEDEFNV(ind250cc,"250 CC",1992,"Inder (Spain)",gl_mINDERS,0)
