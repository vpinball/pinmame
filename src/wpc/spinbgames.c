#include "driver.h"
#include "sim.h"
#include "spinb.h"
//#include "spinbs.h"
//#include "sndbrd.h"

#define GEN_SPINB GEN_ALVG

#define DMD       spinb_dispDMD

#define FLIP78    FLIP_SWNO(16,24)

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
/ Jolly Park
/-------------------------------------------------------------------*/
INITGAME(jolypark, DMD, FLIP78, 3/*?*/, 0, 0)
SPINB_ROMSTART(jolypark,	"jpcpu0.rom", CRC(061967af) SHA1(45048e1d9f17efa3382460fd474a5aeb4191d617),
							"jpcpu1.rom", CRC(ea99202f) SHA1(e04825e73fd25f6469b3315f063f598ea1ab44c7))
SPINB_DMDROM(			    "jpdmd0.rom", CRC(b57565cb) SHA1(3fef66d298893029de78fdb6ecdb562c33d76180),
						    "jpdmd1.rom", CRC(40d1563f) SHA1(90dbea742202340da6fa950eedc2bceec5a2af7e))
SPINB_SNDROM(				"jpsndc1.rom", CRC(0475318f) SHA1(7154bd5ca5b28019eb0ff598ec99bbe49260932b),
							"jpsndm4.rom", CRC(735f3db7) SHA1(81dc893f5194d6ac1af54b262555a40c5c3e0292),	//Sound Effects 1
							"jpsndm5.rom", CRC(769374bd) SHA1(8121369714c55cc06c493b15e5c2ca79b13aff52),	//Sound Effects 2
							"jpsndc0.rom", CRC(a97259dc) SHA1(58dea3f36b760112cfc32d306077da8cf6cdec5a),	
							"jpsndm1.rom", CRC(fc91d2f1) SHA1(c838a0b31bbec9dbc96b46d692c8d6f1286fe46a),	//Music 1
							"jpsndm2.rom", CRC(fb2d1882) SHA1(fb0ef9def54d9163a46354a0df0757fac6cbd57c),	//Music 2
							"jpsndm3.rom", CRC(77e515ba) SHA1(17b635d107c437bfc809f8cc1a6cd063cef12691))	//Music 3

SPINB_ROMEND
CORE_GAMEDEFNV(jolypark,"Jolly Park",1996,"Spinball (Spain)",mSPINBS,0)

