#include "driver.h"
#include "sim.h"
#include "inder.h"

#define INITGAME(name, disptype, balls, inv1,inv2,inv3,inv4,inv5) \
	INDER_INPUT_PORTS_START(name, balls) INDER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{FLIP_SW(FLIP_L)},NULL,{"",{0,inv1,inv2,inv3,inv4,inv5}}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout inderDisp6[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 8,34,2,CORE_SEG7}, {4,14,38,2,CORE_SEG7},
  {0}
};

static core_tLCDLayout inderDisp7[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4,10,34,2,CORE_SEG7}, {4,16,38,2,CORE_SEG7},
  {0}
};

/*-------------------------------------------------------------------
/ Brave Team (1985)
/-------------------------------------------------------------------*/
INITGAME(brvteam, inderDisp6, 1, 0,0,0,0,0)
INDER_ROMSTART(brvteam,	"brv-tea.m0", CRC(1fa72160) SHA1(0fa779ce2604599adff1e124d0b161b69094a614),
						"brv-tea.m1", CRC(4f02ca47) SHA1(68ec7d48c335a1ddd808feaeccac04a4f63d1a33))
INDER_ROMEND
CORE_GAMEDEFNV(brvteam,"Brave Team",1985,"Inder (Spain)",gl_mINDER0,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Canasta '86' (1986)
/-------------------------------------------------------------------*/
INITGAME(canasta, inderDisp7, 1, 0,0,0x08,0,0)
INDER_ROMSTART(canasta,	"c860.bin", CRC(b1f79e52) SHA1(8e9c616f9be19d056da2f86778539d62c0885bac),
						"c861.bin", CRC(25ae3994) SHA1(86dcda3278fbe0e57b8ff4858b955d067af414ce))
INDER_ROMEND
CORE_GAMEDEFNV(canasta,"Canasta '86'",1986,"Inder (Spain)",gl_mINDER1,0)
