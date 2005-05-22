#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "sleic.h"

#define GEN_SLEIC 0

#define INITGAME(name, disptype, balls) \
	SLEIC_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Dot-Matrix display (128 x 32) */
static struct core_dispLayout sleic_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)sleic_dmd_update}, {0}
};

/*-------------------------------------------------------------------
/ Pin-Ball (1993)
/-------------------------------------------------------------------*/
INITGAME(sleicpin, sleic_dispDMD, 1)
SLEIC_ROMSTART4(sleicpin,	"sp01-1_1.rom", CRC(240015bb) SHA1(0e647718173ad59dafbf3b5bc84bef3c33886e23),
						"sp02-1_1.rom", CRC(0e4851a0) SHA1(0692ee2df0b560e2013db9c03fd27c6eb12e618d),
						"sp03-1_1.rom", CRC(261b0ae4) SHA1(e7d9d1c2cab7776afb732701b0b8697b62a8d990),
						"sp04-1_1.rom", CRC(84514cfa) SHA1(6aa87b86892afa534cf963821f08286c126b4245))
SLEIC_ROMEND
CORE_GAMEDEFNV(sleicpin,"Pin-Ball",1993,"Sleic (Spain)",gl_mSLEIC,0)
