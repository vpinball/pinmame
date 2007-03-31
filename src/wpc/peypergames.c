#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "peyper.h"
#include "sndbrd.h"

#define GEN_PEYPER 0

#define INITGAME(name, disptype, balls, lamps) \
	PEYPER_INPUT_PORTS_START(name, balls) PEYPER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_PEYPER,disptype,{FLIP_SW(FLIP_L),0,lamps,0,SNDBRD_NONE}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout peyperDisp7[] = {
  {0, 0,12,1,CORE_SEG8D},{0, 2, 8,2,CORE_SEG7}, {0, 6,10,1,CORE_SEG8D},{0, 8,11,1,CORE_SEG7}, {0,10,22,2,CORE_SEG7},
  {0,22, 4,1,CORE_SEG8D},{0,24, 0,2,CORE_SEG7}, {0,28, 2,1,CORE_SEG8D},{0,30, 3,1,CORE_SEG7}, {0,32,22,2,CORE_SEG7},
  {3, 0,20,1,CORE_SEG8D},{3, 2,16,2,CORE_SEG7}, {3, 6,18,1,CORE_SEG8D},{3, 8,19,1,CORE_SEG7}, {3,10,22,2,CORE_SEG7},
  {3,22,28,1,CORE_SEG8D},{3,24,24,2,CORE_SEG7}, {3,28,26,1,CORE_SEG8D},{3,30,27,1,CORE_SEG7}, {3,32,22,2,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {0}
};

// Tally Hoo (19??)
// Fantastic World (1985)
// Odin (1985) - 6 digits (according to manual)
// Nemesis (1986) - 7 digits
// Wolfman (1987)

/*-------------------------------------------------------------------
/ Odisea Paris-Dakar (1987)
/-------------------------------------------------------------------*/
INITGAME(odisea, peyperDisp7, 1, 6)
PEYPER_ROMSTART(odisea,	"odiseaa.bin", CRC(29a40242) SHA1(321e8665df424b75112589fc630a438dc6f2f459),
						"odiseab.bin", CRC(8bdf7c17) SHA1(7202b4770646fce5b2ba9e3b8ca097a993123b14),
						"odiseac.bin", CRC(832dee5e) SHA1(9b87ffd768ab2610f2352adcf22c4a7880de47ab))
PEYPER_ROMEND
CORE_GAMEDEFNV(odisea,"Odisea Paris-Dakar",1987,"Peyper (Spain)",gl_mPEYPER,0)

// Sir Lancelot  1994 (using advanced hardware)

//---------------

// Sonic games below - using same hardware

static core_tLCDLayout sonicDisp6f[] = {
  {0, 0,32,1,CORE_SEG8D},{0, 2, 8,2,CORE_SEG7}, {0, 6,10,1,CORE_SEG8D},{0, 8,11,2,CORE_SEG7}, {0,12,22,1,CORE_SEG7},
  {0,22,33,1,CORE_SEG8D},{0,24, 0,2,CORE_SEG7}, {0,28, 2,1,CORE_SEG8D},{0,30, 3,2,CORE_SEG7}, {0,34,22,1,CORE_SEG7},
  {3, 0,34,1,CORE_SEG8D},{3, 2,16,2,CORE_SEG7}, {3, 6,18,1,CORE_SEG8D},{3, 8,19,2,CORE_SEG7}, {3,12,22,1,CORE_SEG7},
  {3,22,35,1,CORE_SEG8D},{3,24,24,2,CORE_SEG7}, {3,28,26,1,CORE_SEG8D},{3,30,27,2,CORE_SEG7}, {3,34,22,1,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {0}
};

static core_tLCDLayout sonicDisp7[] = {
  {0, 0, 8,1,CORE_SEG8D},{0, 2, 9,2,CORE_SEG7}, {0, 6,11,1,CORE_SEG8D},{0, 8,12,2,CORE_SEG7}, {0,12,22,1,CORE_SEG7},
  {0,22, 0,1,CORE_SEG8D},{0,24, 1,2,CORE_SEG7}, {0,28, 3,1,CORE_SEG8D},{0,30, 4,2,CORE_SEG7}, {0,34,22,1,CORE_SEG7},
  {3, 0,16,1,CORE_SEG8D},{3, 2,17,2,CORE_SEG7}, {3, 6,19,1,CORE_SEG8D},{3, 8,20,2,CORE_SEG7}, {3,12,22,1,CORE_SEG7},
  {3,22,24,1,CORE_SEG8D},{3,24,25,2,CORE_SEG7}, {3,28,27,1,CORE_SEG8D},{3,30,28,2,CORE_SEG7}, {3,34,22,1,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {0}
};

// Night Fever (1979)
// Storm (1979)

/*-------------------------------------------------------------------
/ Odin De Luxe (1985)
/-------------------------------------------------------------------*/
INITGAME(odin_dlx, sonicDisp6f, 1, 6)
PEYPER_ROMSTART2(odin_dlx, "1a.bin", CRC(4fca9bfc) SHA1(05dce75919375d01a306aef385bcaac042243695),
						   "2a.bin", CRC(46744695) SHA1(fdbd8a93b3e4a9697e77e7d381759829b86fe28b))
PEYPER_ROMEND
CORE_GAMEDEFNV(odin_dlx,"Odin De Luxe",1985,"Sonic (Spain)",gl_mPEYPER,0)

// Gamatron (1986)
// Solar Wars (1986)
// Star Wars (1987)

/*-------------------------------------------------------------------
/ Pole Position (1987)
/-------------------------------------------------------------------*/
INITGAME(poleposn, sonicDisp7, 1, 4)
PEYPER_ROMSTART(poleposn, "1.bin", CRC(fdd37f6d) SHA1(863fef32ab9b5f3aca51788b6be9373a01fa0698),
						  "2.bin", CRC(967cb72b) SHA1(adef17018e2caf65b64bbfef72fe159b9704c409),
						  "3.bin", CRC(461fe9ca) SHA1(01bf35550e2c55995f167293746f355cfd484af1))
PEYPER_ROMEND
CORE_GAMEDEFNV(poleposn,"Pole Position (Sonic)",1987,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Star Wars (1987)
/-------------------------------------------------------------------*/
INITGAME(sonstwar, sonicDisp7, 1, 4)
PEYPER_ROMSTART(sonstwar, "sw1.bin", CRC(a2555d92) SHA1(5c82be85bf097e94953d11c0d902763420d64de4),
						  "sw2.bin", CRC(c2ae34a7) SHA1(0f59242e3aec5da7111e670c4d7cf830d0030597),
						  "sw3.bin", CRC(aee516d9) SHA1(b50e54d4d5db59e3fb71fb000f9bc5e34ff7de9c))
PEYPER_ROMEND
CORE_GAMEDEFNV(sonstwar,"Star Wars (Sonic)",1987,"Sonic (Spain)",gl_mPEYPER,0)

// Hang-On (1988)
