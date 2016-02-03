#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "sleic.h"

#define INITGAME(name, disptype, balls) \
	SLEIC_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Dot-Matrix display (128 x 32) */
static struct core_dispLayout sleic_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(genf *)sleic_dmd_update,NULL}, {0}
};

/*-------------------------------------------------------------------
/ Bike Race (1992)
/-------------------------------------------------------------------*/
INITGAME(bikerace, sleic_dispDMD, 1)
SLEIC_ROMSTART7(bikerace,	"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						"bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						"bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						"bkcpu04.bin", CRC(ce745e89) SHA1(04ba97a9ef1e60a7609c87cf6d8fcae2d0e32621),
						"bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						"bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						"bkio07.bin",  CRC(b52a9d4f) SHA1(726a4d9b354729d7390d2a4f877dc480701ec795))
SLEIC_ROMEND
CORE_GAMEDEFNV(bikerace,"Bike Race",1992,"Sleic (Spain)",gl_mSLEIC3,GAME_NOT_WORKING)

INITGAME(bikerac2, sleic_dispDMD, 1)
SLEIC_ROMSTART7(bikerac2,	"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						"bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						"bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						"04.bin",      CRC(aaaa4a8a) SHA1(ff579041575da4060615da2ff634f3aa91537751),
						"bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						"bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						"07.bin",      CRC(0b763a89) SHA1(8952d7b13674e1599e53cce96e57c2783899a90a))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac2,bikerace,"Bike Race (2-ball play)",1992,"Sleic (Spain)",gl_mSLEIC3,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Pin-Ball (1993)
/-------------------------------------------------------------------*/
INITGAME(sleicpin, sleic_dispDMD, 1)
SLEIC_ROMSTART4(sleicpin,	"sp01-1_1.rom", CRC(240015bb) SHA1(0e647718173ad59dafbf3b5bc84bef3c33886e23),
						"sp02-1_1.rom", CRC(0e4851a0) SHA1(0692ee2df0b560e2013db9c03fd27c6eb12e618d),
						"sp03-1_1.rom", CRC(261b0ae4) SHA1(e7d9d1c2cab7776afb732701b0b8697b62a8d990),
						"sp04-1_1.rom", CRC(84514cfa) SHA1(6aa87b86892afa534cf963821f08286c126b4245))
SLEIC_ROMEND
CORE_GAMEDEFNV(sleicpin,"Pin-Ball",1993,"Sleic (Spain)",gl_mSLEIC1,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Io Moon (1994)
/-------------------------------------------------------------------*/
INITGAME(iomoon, sleic_dispDMD, 1)
SLEIC_ROMSTART5(iomoon,	"v1_3_01.bin", CRC(df80bf4f) SHA1(29547b444cad116c9dc925d6b3112f584df37250),
						"v1_3_02.bin", CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin", CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin", CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin", CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_GAMEDEFNV(iomoon,"Io Moon",1994,"Sleic (Spain)",gl_mSLEIC2,GAME_NOT_WORKING)
