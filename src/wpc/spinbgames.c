#include "driver.h"
#include "sim.h"
#include "spinb.h"
#include "sndbrd.h"

#define GEN_SPINB GEN_ALVG

/* Dot-Matrix display */
static struct core_dispLayout spinb_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)SPINBdmd_update}, {0}
};

#define INITGAME(name, disptype, flippers, balls, sb, db) \
	SPINB_INPUT_PORTS_START(name, balls) SPINB_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SPINB,disptype,{flippers,0,3,0,sb,db}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Bushido (1993) - ( Last game by Inder - before becomming Spinball - but same hardware)
/-------------------------------------------------------------------*/
INITGAME(bushido, spinb_dispDMD, FLIP_SWNO(47,57), 4/*?*/, SNDBRD_SPINB, 0)
SPINB_ROMSTART(bushido,	"0-z80.bin", CRC(3ea1eb1d) SHA1(cceb6c68e481f36a5646ff4f38d3dfc4275b0c79),
						"1-z80.bin", CRC(648da72b) SHA1(1005a13b4746e302d979c8b1da300e943cdcab3d))
SPINB_DMDROM1(			"g-disply.bin", CRC(9a1df82f) SHA1(4ad6a12ae36ec898b8ac5243da6dec3abcd9dc33))
SPINB_SNDROM22(			"a-sonido.bin", CRC(cf7d5399) SHA1(c79145826cfa6be2487e3add477d9b452c553762),
						"b-sonido.bin", CRC(cb4fc885) SHA1(569f389fa8f91f886b58f44f701d2752ef01f3fa),	//Sound Effects 1
						"c-sonido.bin", CRC(35a43dd8) SHA1(f2b1994f67f749c65a88c95d970b655990d85b96),	//Sound Effects 2
						"d-musica.bin", CRC(2263d41b) SHA1(7c6631df12726ca5cf326c5856d51098635b9c73),
						"e-musica.bin", CRC(1414b921) SHA1(5df9e538ee109df28953ec8f162c60cb8c6e4d96),	//Music 1
						"f-musica.bin", CRC(80f3a6df) SHA1(e09ad4660e511779c6e55559fa0c2c0b0c6600c8))	//Music 2
SPINB_ROMEND
CORE_GAMEDEFNV(bushido,"Bushido",1993,"Inder/Spinball (Spain)",mSPINBS,0)

/*-------------------------------------------------------------------
/ Mach 2 (1995)
/-------------------------------------------------------------------*/
INITGAME(mach2, spinb_dispDMD, FLIP_SWNO(47,57), 4/*?*/, SNDBRD_SPINB, 0)
SPINB_ROMSTART(mach2,	"jpcpu0.rom", NO_DUMP,
						"jpcpu1.rom", NO_DUMP)
SPINB_DMDROM1(			"jpdmd0.rom", NO_DUMP)
SPINB_SNDROM22(			"jpsndc1.rom", NO_DUMP,
						"jpsndm4.rom", NO_DUMP,	//Sound Effects 1
						"jpsndm5.rom", NO_DUMP,	//Sound Effects 2
						"jpsndc0.rom", NO_DUMP,
						"jpsndm1.rom", NO_DUMP,	//Music 1
						"jpsndm2.rom", NO_DUMP)	//Music 2
SPINB_ROMEND
CORE_GAMEDEFNV(mach2,"Mach 2",1995,"Spinball (Spain)",mSPINBSNMI,0)

/*-------------------------------------------------------------------
/ Jolly Park (1996)
/-------------------------------------------------------------------*/
INITGAME(jolypark, spinb_dispDMD, FLIP_SWNO(47,57), 4/*?*/, SNDBRD_SPINB, 0)
SPINB_ROMSTART(jolypark,	"jpcpu0.rom", CRC(061967af) SHA1(45048e1d9f17efa3382460fd474a5aeb4191d617),
							"jpcpu1.rom", CRC(ea99202f) SHA1(e04825e73fd25f6469b3315f063f598ea1ab44c7))
SPINB_DMDROM2(			    "jpdmd0.rom", CRC(b57565cb) SHA1(3fef66d298893029de78fdb6ecdb562c33d76180),
						    "jpdmd1.rom", CRC(40d1563f) SHA1(90dbea742202340da6fa950eedc2bceec5a2af7e))
SPINB_SNDROM23(				"jpsndc1.rom", CRC(0475318f) SHA1(7154bd5ca5b28019eb0ff598ec99bbe49260932b),
							"jpsndm4.rom", CRC(735f3db7) SHA1(81dc893f5194d6ac1af54b262555a40c5c3e0292),	//Sound Effects 1
							"jpsndm5.rom", CRC(769374bd) SHA1(8121369714c55cc06c493b15e5c2ca79b13aff52),	//Sound Effects 2
							"jpsndc0.rom", CRC(a97259dc) SHA1(58dea3f36b760112cfc32d306077da8cf6cdec5a),
							"jpsndm1.rom", CRC(fc91d2f1) SHA1(c838a0b31bbec9dbc96b46d692c8d6f1286fe46a),	//Music 1
							"jpsndm2.rom", CRC(fb2d1882) SHA1(fb0ef9def54d9163a46354a0df0757fac6cbd57c),	//Music 2
							"jpsndm3.rom", CRC(77e515ba) SHA1(17b635d107c437bfc809f8cc1a6cd063cef12691))	//Music 3

SPINB_ROMEND
CORE_GAMEDEFNV(jolypark,"Jolly Park",1996,"Spinball (Spain)",mSPINBSNMI,0)

/*-------------------------------------------------------------------
/ Verne's World (1996)
/-------------------------------------------------------------------*/
INITGAME(vrnwrld, spinb_dispDMD, FLIP_SWNO(47,57), 4/*?*/, SNDBRD_SPINB, 0)
SPINB_ROMSTART(vrnwrld,	"jpcpu0.rom", NO_DUMP,
						"jpcpu1.rom", NO_DUMP)
SPINB_DMDROM2(			"jpdmd0.rom", NO_DUMP,
						"jpdmd1.rom", NO_DUMP)
SPINB_SNDROM23(			"jpsndc1.rom", NO_DUMP,
						"jpsndm4.rom", NO_DUMP,	//Sound Effects 1
						"jpsndm5.rom", NO_DUMP,	//Sound Effects 2
						"jpsndc0.rom", NO_DUMP,
						"jpsndm1.rom", NO_DUMP,	//Music 1
						"jpsndm2.rom", NO_DUMP,	//Music 2
						"jpsndm3.rom", NO_DUMP)	//Music 3
SPINB_ROMEND
CORE_GAMEDEFNV(vrnwrld,"Verne's World",1996,"Spinball (Spain)",mSPINBSNMI,0)

