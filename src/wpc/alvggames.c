#include "driver.h"
#include "sim.h"
#include "alvg.h"
#include "alvgs.h"
#include "alvgdmd.h"
#include "sndbrd.h"

#define DMD       alvg_dispDMD

#define FLIP78   FLIP_SWNO(6,7)		//really 7,8 in the matrix, but for some reason isn't showing properly

/* Dot-Matrix display */
static struct core_dispLayout alvg_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)alvgdmd_update}, {0}
};

#define INITGAME(name, disptype, flippers, balls, sb, db, lamps) \
	ALVG_INPUT_PORTS_START(name, balls) ALVG_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_ALVG,disptype,{flippers,4,lamps,0,sb,db,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Al's Garage Band Goes On A World Tour
/-------------------------------------------------------------------*/
INITGAME(wrldtour, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, SNDBRD_ALVGDMD, 5)
ALVGROMSTART(wrldtour,	"cpu27c.512", CRC(c9572fb5) SHA1(47a3e8943ef4207011a33f4a03a6e722c937cc48))
ALVGS_SOUNDROM(			"soundc.512", CRC(b44bee01) SHA1(795d8500e5bd73ce23756bf1f5c96db1a3621a70),
						"samp_0.c21", CRC(37beb831) SHA1(2b90d2be0a1bd7c59469846631d2b44bdf9f5f9d),
						"samp_1.c21", CRC(621533c6) SHA1(ca0ed9e89c340cb3b08f9a9002af9997372c1cbf),
						"samp_2.c21", CRC(454a5cca) SHA1(66b1a5832134365fd762fcba4cf4d666f60ebd65),
						"samp_3.c21", CRC(1f4928f4) SHA1(9949ab96644984fab8037224f52ec28d7d7cc967))
ALVGDMD_SPLIT_ROM(		"dot27c.512", CRC(c8bd48e7) SHA1(e2dc513dd42c05c2018e6d8c0b6f0b2c56e6e059),
						"romdef1.c20",CRC(045b21c1) SHA1(134b7eb0f71506d12d9ded24999d530126c558fc),
						"romdef2.c20",CRC(23c32ee5) SHA1(429b3b069251bb8b681bbc6382ceb6b85125eb79))
ALVG_ROMEND
CORE_GAMEDEFNV(wrldtour,"Al's Garage Band Goes On A World Tour",1992,"Alvin G",mALVGS,0)

/*-------------------------------------------------------------------
/ Mystery Castle
/-------------------------------------------------------------------*/
INITGAME(mystcast, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, SNDBRD_ALVGDMD, 5)
ALVGROMSTART(mystcast,	"mcastle.cpu", CRC(936e6799) SHA1(aa29fb5f12f34c695d1556232744f65cd576a2b1))
ALVGS_SOUNDROM(			"mcastle.102", CRC(752822d0) SHA1(36461ef03cac5aefa0c03dfdc63c3d294a3b9c09),
						"mcastle.sr0", CRC(0855cc73) SHA1(c46e08432bcff24594c33171f20669ba63828931),
						"mcastle.sr1", CRC(3b5d76e0) SHA1(b2e1bca3c596eba89feda868fa56c71a6b22414c),			   
						"mcastle.sr2", CRC(c3ffd277) SHA1(d16d1b22089b89bbf0db7d2b66c9745a56034322),
						"mcastle.sr3", CRC(740858bb) SHA1(d2e9a0a178977dcc873368b042cea7052578df66))
ALVGDMD_ROM2R(			"mcastle.du4", CRC(686e253a) SHA1(28aff34c120c61e231e2111dc396df515bcbbb89),
						"mcastle.du5", CRC(9095c367) SHA1(9d3e9416f662ee2aad891eef059278c530448fcc))
ALVG_ROMEND
CORE_GAMEDEFNV(mystcast,"Mystery Castle",1993,"Alvin G",mALVGS,0)

/*-------------------------------------------------------------------
/ Pistol Poker
/-------------------------------------------------------------------*/
INITGAME(pstlpkr, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, SNDBRD_ALVGDMD, 5)
ALVGROMSTART(pstlpkr,	"p_peteu2.512", CRC(490a1e2d) SHA1(907dd858ed948681e7366a64a0e7537ebe301d6b))
ALVGS_SOUNDROM(			"p_pu102.512" , CRC(b8fb806e) SHA1(c2dc19820ea22bbcf5808db2fb4be76a4033d6ea),
						"p_parom0.c20", CRC(99986af2) SHA1(52fa7d2979f7f2d6d65ab6d4f7bbfbed16303991),
						"p_parom1.c20", CRC(ae2af238) SHA1(221d3a0e3fb1daad261d723e873ef0727b88889e),
						"p_parom2.c20", CRC(f39560a4) SHA1(cdfdf7b44ff4c3f9f4d39fbd8ecbf141d8568088),
						"p_parom3.c20", CRC(19d5e4de) SHA1(fb59166ebf992e81b92a42898e351d8443adb1c3))
ALVGDMD_ROM(			"p_peteu4.512", CRC(caa0cabd) SHA1(caff6ca4a9cce4e3d846502696c8838805673261),
						"p_peteu5.c20", CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7),
						"p_peteu6.c20", CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50))
ALVG_ROMEND
CORE_GAMEDEFNV(pstlpkr,"Pistol Poker",1993,"Alvin G",mALVGS,0)






//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
/*-------------------------------------------------------------------
/ Test 8031 CPU Core (uses either Pistol Poker dmd roms or World Tour)
/-------------------------------------------------------------------*/
INITGAME(test8031, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, 0, 5)
ROM_START(test8031)

TEST8031_ROM(			"addsub.bin", NO_DUMP,
						"p_peteu5.c20", CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7),
						"p_peteu6.c20", CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50))
ALVG_ROMEND
CORE_GAMEDEFNV(test8031,"Test 8031 CPU Core",2003,"Steve Ellenoff",mTEST8031,0)
#endif
