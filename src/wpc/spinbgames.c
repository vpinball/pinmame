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
SPINB_ROMEND
CORE_GAMEDEFNV(jolypark,"Jolly Park",1996,"Spinball (Spain)",mSPINBS,0)

