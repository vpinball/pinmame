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

#define INITGAME_SONIC(name, disptype, balls, lamps) \
	PEYPER_INPUT_PORTS_START(name, balls) PEYPER_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_PEYPER,disptype,{FLIP_SW(FLIP_L),0,lamps,0,SNDBRD_NONE,0,1}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout disp6f[] = {
  {0, 0,32,1,CORE_SEG8D},{0, 2, 8,2,CORE_SEG7}, {0, 6,10,1,CORE_SEG8D},{0, 8,11,2,CORE_SEG7}, {0,12,36,1,CORE_SEG7},
  {0,22,33,1,CORE_SEG8D},{0,24, 0,2,CORE_SEG7}, {0,28, 2,1,CORE_SEG8D},{0,30, 3,2,CORE_SEG7}, {0,34,36,1,CORE_SEG7},
  {3, 0,34,1,CORE_SEG8D},{3, 2,16,2,CORE_SEG7}, {3, 6,18,1,CORE_SEG8D},{3, 8,19,2,CORE_SEG7}, {3,12,36,1,CORE_SEG7},
  {3,22,35,1,CORE_SEG8D},{3,24,24,2,CORE_SEG7}, {3,28,26,1,CORE_SEG8D},{3,30,27,2,CORE_SEG7}, {3,34,36,1,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {0}
};

static core_tLCDLayout peyperDisp7[] = {
  {0, 0,12,1,CORE_SEG8D},{0, 2, 8,2,CORE_SEG7}, {0, 6,10,1,CORE_SEG8D},{0, 8,11,1,CORE_SEG7}, {0,10,36,1,CORE_SEG7}, {0,12,36,1,CORE_SEG7},
  {0,22, 4,1,CORE_SEG8D},{0,24, 0,2,CORE_SEG7}, {0,28, 2,1,CORE_SEG8D},{0,30, 3,1,CORE_SEG7}, {0,32,36,1,CORE_SEG7}, {0,34,36,1,CORE_SEG7},
  {3, 0,20,1,CORE_SEG8D},{3, 2,16,2,CORE_SEG7}, {3, 6,18,1,CORE_SEG8D},{3, 8,19,1,CORE_SEG7}, {3,10,36,1,CORE_SEG7}, {3,12,36,1,CORE_SEG7},
  {3,22,28,1,CORE_SEG8D},{3,24,24,2,CORE_SEG7}, {3,28,26,1,CORE_SEG8D},{3,30,27,1,CORE_SEG7}, {3,32,36,1,CORE_SEG7}, {3,34,36,1,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {6, 0,41,1,CORE_SEG7S},{6, 2,47,1,CORE_SEG7S},{6, 4,36,1,CORE_SEG7S},{6, 6,36,1,CORE_SEG7S},{6, 8,36,1,CORE_SEG7S},{6,10,36,1,CORE_SEG7S},{6,12,36,1,CORE_SEG7S},
  {8, 0,43,2,CORE_SEG7S},{8, 4,36,1,CORE_SEG7S},{8, 6,36,1,CORE_SEG7S},{8, 8,36,1,CORE_SEG7S},{8,10,36,1,CORE_SEG7S},{8,12,36,1,CORE_SEG7S},
  {10, 0,45,2,CORE_SEG7S},{10, 4,36,1,CORE_SEG7S},{10, 6,36,1,CORE_SEG7S},{10, 8,36,1,CORE_SEG7S},{10,10,36,1,CORE_SEG7S},{10,12,36,1,CORE_SEG7S},
  {0}
};

/*-------------------------------------------------------------------
/ Odin (1985)
/-------------------------------------------------------------------*/
INITGAME(odin, disp6f, 1, 4)
PEYPER_ROMSTART2(odin, "odin_a.bin", CRC(ac3a7770) SHA1(2409629d3adbae0d7e6e5f9fe6f137c1e5a1bb86),
						   "odin_b.bin", CRC(46744695) SHA1(fdbd8a93b3e4a9697e77e7d381759829b86fe28b))
PEYPER_ROMEND
CORE_GAMEDEFNV(odin,"Odin",1985,"Peyper (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Nemesis (1986)
/-------------------------------------------------------------------*/
INITGAME(nemesis, peyperDisp7, 1, 4)
PEYPER_ROMSTART(nemesis,	"nemesisa.bin", CRC(56f13350) SHA1(30907c362f88b48d634e8aaa1e1161852886645c),
						"nemesisb.bin", CRC(a8f3e6c7) SHA1(c25b2271c4de6f4b57c3c850d28a0878ea081c26),
						"memoriac.bin", CRC(468f16f0) SHA1(66ce0464d82331cfc0ac1f6fbd871066e4e57262))
PEYPER_ROMEND
CORE_GAMEDEFNV(nemesis,"Nemesis",1986,"Peyper (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Wolf Man (1987)
/-------------------------------------------------------------------*/
WOLFMAN_INPUT_PORTS_START PEYPER_INPUT_PORTS_END
static core_tGameData wolfmanGameData = {GEN_PEYPER,peyperDisp7,{FLIP_SW(FLIP_L),0,4,0,SNDBRD_NONE}};
static void init_wolfman(void) { core_gameData = &wolfmanGameData; }
PEYPER_ROMSTART(wolfman,	"memoriaa.bin", CRC(1fec83fe) SHA1(5dc887d0fa00129ae31451c03bfe442f87dd2f54),
						"memoriab.bin", CRC(62a1e3ec) SHA1(dc472c7c9d223820f8f1031c92e36890c1fcba7d),
						"memoriac.bin", CRC(468f16f0) SHA1(66ce0464d82331cfc0ac1f6fbd871066e4e57262))
PEYPER_ROMEND
CORE_GAMEDEFNV(wolfman,"Wolf Man",1987,"Peyper (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Odisea Paris-Dakar (1987)
/-------------------------------------------------------------------*/
ODISEA_INPUT_PORTS_START PEYPER_INPUT_PORTS_END
static core_tGameData odiseaGameData = {GEN_PEYPER,peyperDisp7,{FLIP_SW(FLIP_L),0,4,0,SNDBRD_NONE}};
static void init_odisea(void) { core_gameData = &odiseaGameData; }
PEYPER_ROMSTART(odisea,	"odiseaa.bin", CRC(29a40242) SHA1(321e8665df424b75112589fc630a438dc6f2f459),
						"odiseab.bin", CRC(8bdf7c17) SHA1(7202b4770646fce5b2ba9e3b8ca097a993123b14),
						"odiseac.bin", CRC(832dee5e) SHA1(9b87ffd768ab2610f2352adcf22c4a7880de47ab))
PEYPER_ROMEND
CORE_GAMEDEFNV(odisea,"Odisea Paris-Dakar",1987,"Peyper (Spain)",gl_mPEYPER,0)

// Sir Lancelot  1994 (using advanced hardware)

//---------------

// Sonic games below - using same hardware

static core_tLCDLayout sonicDisp7[] = {
  {0, 0, 8,1,CORE_SEG8D},{0, 2, 9,2,CORE_SEG7}, {0, 6,11,1,CORE_SEG8D},{0, 8,12,2,CORE_SEG7}, {0,12,36,1,CORE_SEG7},
  {0,22, 0,1,CORE_SEG8D},{0,24, 1,2,CORE_SEG7}, {0,28, 3,1,CORE_SEG8D},{0,30, 4,2,CORE_SEG7}, {0,34,36,1,CORE_SEG7},
  {3, 0,16,1,CORE_SEG8D},{3, 2,17,2,CORE_SEG7}, {3, 6,19,1,CORE_SEG8D},{3, 8,20,2,CORE_SEG7}, {3,12,36,1,CORE_SEG7},
  {3,22,24,1,CORE_SEG8D},{3,24,25,2,CORE_SEG7}, {3,28,27,1,CORE_SEG8D},{3,30,28,2,CORE_SEG7}, {3,34,36,1,CORE_SEG7},
  {1,19,31,1,CORE_SEG7S},{1,21,30,1,CORE_SEG7S},{1,24,15,1,CORE_SEG7S},{1,27,14,1,CORE_SEG7S},
  {3,17, 6,1,CORE_SEG7},
  {0}
};

// Third World (1978) - using Playmatic hardware
// Night Fever (1979) - using Playmatic hardware
// Storm (1979)

/*-------------------------------------------------------------------
/ Odin De Luxe (1985)
/-------------------------------------------------------------------*/
INITGAME_SONIC(odin_dlx, disp6f, 1, 4)
PEYPER_ROMSTART2(odin_dlx, "1a.bin", CRC(4fca9bfc) SHA1(05dce75919375d01a306aef385bcaac042243695),
						   "odin_b.bin", CRC(46744695) SHA1(fdbd8a93b3e4a9697e77e7d381759829b86fe28b))
PEYPER_ROMEND
CORE_CLONEDEFNV(odin_dlx,odin,"Odin De Luxe",1985,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Gamatron (1986)
/-------------------------------------------------------------------*/
INITGAME_SONIC(gamatros, sonicDisp7, 1, 4)
PEYPER_ROMSTART2(gamatros, "gama_a.bin", CRC(1dc2841c) SHA1(27c6a07b1f8bd5e73b425e7dbdcfb1d5233c18b2),
						  "gama_b.bin", CRC(56125890) SHA1(8b30a2282df264d798df1b031ecade999d135f81))
PEYPER_ROMEND
CORE_GAMEDEFNV(gamatros,"Gamatron (Sonic)",1986,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Solar Wars (1986)
/-------------------------------------------------------------------*/
INITGAME_SONIC(solarwar, sonicDisp7, 1, 4)
PEYPER_ROMSTART2(solarwar, "solarw1c.bin", CRC(aa6bf0cd) SHA1(7332a4b1679841283d846f3e4f1792cb8e9529bf),
						  "solarw2.bin", CRC(95e2cbb1) SHA1(f9ab3222ca0b9e0796030a7a618847a4e8f77957))
PEYPER_ROMEND
CORE_GAMEDEFNV(solarwar,"Solar Wars (Sonic)",1986,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Pole Position (1987)
/-------------------------------------------------------------------*/
INITGAME_SONIC(poleposn, sonicDisp7, 1, 4)
PEYPER_ROMSTART(poleposn, "1.bin", CRC(fdd37f6d) SHA1(863fef32ab9b5f3aca51788b6be9373a01fa0698),
						  "2.bin", CRC(967cb72b) SHA1(adef17018e2caf65b64bbfef72fe159b9704c409),
						  "3.bin", CRC(461fe9ca) SHA1(01bf35550e2c55995f167293746f355cfd484af1))
PEYPER_ROMEND
CORE_GAMEDEFNV(poleposn,"Pole Position (Sonic)",1987,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Star Wars (1987)
/-------------------------------------------------------------------*/
INITGAME_SONIC(sonstwar, sonicDisp7, 1, 4)
PEYPER_ROMSTART(sonstwar, "sw1.bin", CRC(a2555d92) SHA1(5c82be85bf097e94953d11c0d902763420d64de4),
						  "sw2.bin", CRC(c2ae34a7) SHA1(0f59242e3aec5da7111e670c4d7cf830d0030597),
						  "sw3.bin", CRC(aee516d9) SHA1(b50e54d4d5db59e3fb71fb000f9bc5e34ff7de9c))
PEYPER_ROMEND
CORE_GAMEDEFNV(sonstwar,"Star Wars (Sonic)",1987,"Sonic (Spain)",gl_mPEYPER,0)

#define init_sonstwr2 init_sonstwar
#define input_ports_sonstwr2 input_ports_sonstwar
PEYPER_ROMSTART(sonstwr2, "stw1i.bin", CRC(416e2a0c) SHA1(74ca550ee9eb83d9762ffab0f085dffae569d4a9),
						  "stw2i.bin", CRC(ccbbec46) SHA1(4fd0e48916e8761a7e70300d3ede166f5f04f8ae),
						  "sw3.bin", CRC(aee516d9) SHA1(b50e54d4d5db59e3fb71fb000f9bc5e34ff7de9c))
PEYPER_ROMEND
CORE_CLONEDEFNV(sonstwr2,sonstwar,"Star Wars (Sonic, alternate set)",1987,"Sonic (Spain)",gl_mPEYPER,0)

/*-------------------------------------------------------------------
/ Hang-On (1988)
/-------------------------------------------------------------------*/
INITGAME_SONIC(hangon, sonicDisp7, 1, 4)
PEYPER_ROMSTART(hangon, "hangon1.bin", CRC(b0672137) SHA1(e0bd0808a3a8c6df200b0edc7b5e8cf293a659b7),
						"hangon2.bin", CRC(6e1e55c0) SHA1(473c882a0eb68807969894b82be2b86d7c463c93),
						"hangon3.bin", CRC(26949f2f) SHA1(e3e1a436ce59c7f1c2904cd8f50f2ba4a4e37638))
PEYPER_ROMEND
CORE_GAMEDEFNV(hangon,"Hang-On",1988,"Sonic (Spain)",gl_mPEYPER,0)


// Games by other manufacturers below

/*-------------------------------------------------------------------
/ Video Dens: Ator (1985)
/-------------------------------------------------------------------*/
INITGAME(ator, disp6f, 1, 4)
PEYPER_ROMSTART2(ator, "ator_1.bin", NO_DUMP,
                       "ator_2.bin", CRC(21aad5c4) SHA1(e78da5d80682710db34cbbfeae5af54241c73371))
PEYPER_ROMEND
CORE_GAMEDEFNV(ator,"Ator",1985,"Video Dens",gl_mPEYPER_VD,0)

INITGAME(ator2, disp6f, 1, 4)
PEYPER_ROMSTART2(ator2, "ator1.bin", CRC(87967577) SHA1(6586401a4b1a837bacd61d156b44eedaab271479),
                        "ator2.bin", CRC(832b06ea) SHA1(c70c613fd29d1d560951890bce072cbf45525526))
PEYPER_ROMEND
CORE_CLONEDEFNV(ator2,ator,"Ator (2 bumpers)",1985,"Video Dens",gl_mPEYPER_VD,0)
