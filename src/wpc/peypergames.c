#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "peyper.h"
#include "sndbrd.h"

#define GEN_PEYPER 0

#define INITGAME(name, disptype, balls) \
	PEYPER_INPUT_PORTS_START(name, balls) PEYPER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_PEYPER,disptype,{FLIP_SW(FLIP_L), 0,10,0, SNDBRD_NONE}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout peyperDisp7[] = {
  {0, 0,12,1,CORE_SEG7}, {0, 2, 8,4,CORE_SEG7}, {0,10,33,1,CORE_SEG7},
  {0,20, 4,1,CORE_SEG7}, {0,22, 0,4,CORE_SEG7}, {0,30,32,1,CORE_SEG7},
  {3, 0,20,1,CORE_SEG7}, {3, 2,16,4,CORE_SEG7}, {3,10,34,1,CORE_SEG7},
  {3,20,28,1,CORE_SEG7}, {3,22,24,4,CORE_SEG7}, {3,30,35,1,CORE_SEG7},
  {1,20,31,1,CORE_SEG7S},{1,22,30,1,CORE_SEG7S},{1,25,14,1,CORE_SEG7S},{1,28,15,1,CORE_SEG7S},
  {3,15, 6,1,CORE_SEG7},
  {0}
};

/*-------------------------------------------------------------------
/ Odisea Paris-Dakar (198?)
/-------------------------------------------------------------------*/
INITGAME(odisea, peyperDisp7, 1)
PEYPER_ROMSTART(odisea,	"odiseaa.bin", CRC(29a40242) SHA1(321e8665df424b75112589fc630a438dc6f2f459),
						"odiseab.bin", CRC(8bdf7c17) SHA1(7202b4770646fce5b2ba9e3b8ca097a993123b14),
						"odiseac.bin", CRC(832dee5e) SHA1(9b87ffd768ab2610f2352adcf22c4a7880de47ab))
PEYPER_ROMEND
CORE_GAMEDEFNV(odisea,"Odisea Paris-Dakar",198?,"Peyper (Spain)",gl_mPEYPER,0)
