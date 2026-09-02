// license:BSD-3-Clause

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

/* Io Moon only: same cabinet block, but the real SW40 DIP block instead of S1..S8,
   because it is the one machine here whose Z80 ROM was traced for it (sleic.h) */
#define INITGAME2(name, disptype, balls) \
	SLEIC2_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Dot-Matrix display (128 x 32) */
static core_tLCDLayout sleic_dispDMD[] = {
  {0,0,32,128,CORE_DMD,NULL,NULL}, {0}
};

/*-------------------------------------------------------------------
/ Bike Race (1992)
/-------------------------------------------------------------------*/
INITGAME(bikerace, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerace,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "bkcpu04.bin", CRC(ce745e89) SHA1(04ba97a9ef1e60a7609c87cf6d8fcae2d0e32621),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bkio07.bin",  CRC(b52a9d4f) SHA1(726a4d9b354729d7390d2a4f877dc480701ec795))
SLEIC_ROMEND
CORE_GAMEDEFNV(bikerace,"Bike Race",1992,"Sleic (Spain)",gl_mSLEIC3,0)

INITGAME(bikerac2, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerac2,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "04.bin",      CRC(aaaa4a8a) SHA1(ff579041575da4060615da2ff634f3aa91537751),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "07.bin",      CRC(0b763a89) SHA1(8952d7b13674e1599e53cce96e57c2783899a90a))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac2,bikerace,"Bike Race (2-ball play)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* V4.1 -- the newest of the three known Bike Race sets (dumped by Joerg Amann).
/  ROM 01 was not dumped; 02 and 05 are byte-identical to the parent set and are
/  inherited from it, so only 03/04/06/07 are listed here.  Relative to bikerace,
/  this set carries the same F000 code revision as bikerac2 (including the 4-opto
/  trough handler that can report "FALTA 2 BOLAS"), and adds changes of its own to
/  the OKI sample ROM (03), the character/graphics ROM (06) and the game data (04).
/
/  NOT WORKING: it boots and runs, but the artwork comes out as garbage, and the
/  reason looks like a missing or differently-mapped graphics ROM rather than a
/  driver bug.  The graphics descriptor table lives in the first 4KB of ROM 06 --
/  the only part of that chip this revision changed -- and where the 1992 sets
/  fetch their artwork from 0x40000 (ROM 05), V4.1's table sends the fetch to
/  0x24000, i.e. into ROM 06 itself, whose contents from 0x1104 on are byte-for-byte
/  the 1992 data.  Drawing from there produces the garbage.  Swapping the 05/06 bank
/  order, and relocating ROM 05's artwork to 0x24000, both fail, so the set as dumped
/  does not contain what its own table points at.  Resolving this needs the V4.1
/  board's ROM complement confirmed (ROM 01 was never dumped) or its PAL. */
INITGAME(bikerac3, sleic_dispDMD, 2)
SLEIC_ROMSTART7(bikerac3,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04.bin",    CRC(33fd212e) SHA1(9471e34fc4280741816d65f88590febc9e8629a7),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bk06.bin",    CRC(ad48a30a) SHA1(183b04699b038811de950bba6b8a067689bdb883),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac3,bikerace,"Bike Race (V4.1)",1992,"Sleic (Spain)",gl_mSLEIC3,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ Sleic Pin-Ball (1993)
/-------------------------------------------------------------------*/
INITGAME(sleicpin, sleic_dispDMD, 1)
SLEIC_ROMSTART4(sleicpin,"sp01-1_1.rom", CRC(240015bb) SHA1(0e647718173ad59dafbf3b5bc84bef3c33886e23),
						 "sp02-1_1.rom", CRC(0e4851a0) SHA1(0692ee2df0b560e2013db9c03fd27c6eb12e618d),
						 "sp03-1_1.rom", CRC(261b0ae4) SHA1(e7d9d1c2cab7776afb732701b0b8697b62a8d990),
						 "sp04-1_1.rom", CRC(84514cfa) SHA1(6aa87b86892afa534cf963821f08286c126b4245))
SLEIC_ROMEND
CORE_GAMEDEFNV(sleicpin,"Sleic Pin-Ball",1993,"Sleic (Spain)",gl_mSLEIC1,0)

/*-------------------------------------------------------------------
/ Io Moon (1994)
/-------------------------------------------------------------------*/
INITGAME2(iomoon, sleic_dispDMD, 1)
SLEIC_ROMSTART5(iomoon, "v1_3_01.bin", CRC(df80bf4f) SHA1(29547b444cad116c9dc925d6b3112f584df37250),
						"v1_3_02.bin", CRC(2bd589cd) SHA1(87354c76cbef8185d563266230c72a618ce6fcd7),
						"v1_3_03.bin", CRC(334d0e20) SHA1(06b38cc7fcee633c45a9000187fcde8d7e03a51f),
						"v1_3_04.bin", CRC(f3a950bf) SHA1(e0410f8fe9b4efe7d21052c0a19894a563f90a27),
						"v1_3_05.bin", CRC(6bb5e101) SHA1(125412953bbee7ee171c0bd34f7848fde37ace67))
SLEIC_ROMEND
CORE_GAMEDEFNV(iomoon,"Io Moon",1994,"Sleic (Spain)",gl_mSLEIC2,GAME_NOT_WORKING)
