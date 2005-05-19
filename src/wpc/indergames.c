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
/ Clown (1988)
/-------------------------------------------------------------------*/
INITGAME(pinclown, inderDisp7, 1, 0,0,0,0,0)
INDER_ROMSTART1(pinclown,"clown_a.bin", CRC(b7c3f9ab) SHA1(89ede10d9e108089da501b28f53cd7849f791a00))
INDER_SNDROM4(			"clown_b.bin", CRC(acb16108) SHA1(b9003bac44b8bac406950ffb6696eef9a07f16c7),
						"clown_c.bin", CRC(dff89319) SHA1(3745a02c3755d11ea7fb552f7a5df2e8bbee2c29),
						"clown_d.bin", CRC(cce4e1dc) SHA1(561c9331d2d110d34cf250cd7b25be16a72a1d79),
						"clown_e.bin", CRC(98263526) SHA1(509764e65847637824ba93f7e6ce926501c431ce),
						"clown_f.bin", CRC(5f01b531) SHA1(116b1670ef4d5c054bb09dc55aa7d5d3ca047079))
INDER_ROMEND
CORE_GAMEDEFNV(pinclown,"Clown (Inder)",1988,"Inder (Spain)",gl_mINDERS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Corsario (1989)
/-------------------------------------------------------------------*/
INITGAME(corsario, inderDisp7, 1, 0,0,0,0,0)
INDER_ROMSTART1(corsario,"0-corsar.bin", CRC(800f6895) SHA1(a222e7ea959629202686815646fc917ffc5a646c))
INDER_SNDROM4(			"a-corsar.bin", CRC(e14b7918) SHA1(5a5fc308b0b70fe041b81071ba4820782b6ff988),
						"b-corsar.bin", CRC(7f155828) SHA1(e459c81b2c2e47d4276344d8d6a08c2c6242f941),
						"c-corsar.bin", CRC(047fd722) SHA1(2385507459f85c68141adc7084cb51dfa02462f6),
						"d-corsar.bin", CRC(10d8b448) SHA1(ed1918e6c55eba07dde31b9755c9403e073cad98),
						"e-corsar.bin", CRC(918ee349) SHA1(17cded8b5626c91e400d26332a160704f2fd2b55))
INDER_ROMEND
CORE_GAMEDEFNV(corsario,"Corsario",1989,"Inder (Spain)",gl_mINDERS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ 250 CC (1992)
/-------------------------------------------------------------------*/
INITGAME(ind250cc, inderDisp7, 1, 0,0,0,0,0)
INDER_ROMSTART1(ind250cc,"0-250cc.bin", CRC(753d82ec) SHA1(61950336ba571f9f75f2fc31ccb7beaf4e05dddc))
INDER_SNDROM4(			"a-250cc.bin", CRC(b64bdafb) SHA1(eab6d54d34b44187d454c1999e4bcf455183d5a0),
						"b-250cc.bin", CRC(884c31c8) SHA1(23a838f1f0cb4905fa8552579b5452134f0fc9cc),
						"c-250cc.bin", CRC(5a1dfa1d) SHA1(4957431d87be0bb6d27910b718f7b7edcd405fff),
						"d-250cc.bin", CRC(a0940387) SHA1(0e06483e3e823bf4673d8e0bd120b0a6b802035d),
						"e-250cc.bin", CRC(538b3274) SHA1(eb76c41a60199bb94aec4666222e405bbcc33494))
INDER_ROMEND
CORE_GAMEDEFNV(ind250cc,"250 CC",1992,"Inder (Spain)",gl_mINDERS,GAME_IMPERFECT_SOUND)

