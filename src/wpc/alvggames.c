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

#define INITGAME(name, disptype, flippers, balls, sb, lamps) \
	ALVG_INPUT_PORTS_START(name, balls) ALVG_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_ALVG,disptype,{flippers,4,lamps,0,sb,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ World Tour Garage Band
/-------------------------------------------------------------------*/
INITGAME(wtgband, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, 5)
ALVGROMSTART(wtgband,	"cpu27c.512", CRC(c9572fb5) SHA1(47a3e8943ef4207011a33f4a03a6e722c937cc48))
ALVGS_SOUNDROM(			"soundc.512", CRC(b44bee01) SHA1(795d8500e5bd73ce23756bf1f5c96db1a3621a70),
						"samp_0.c21", CRC(37beb831) SHA1(2b90d2be0a1bd7c59469846631d2b44bdf9f5f9d),
						"samp_1.c21", CRC(621533c6) SHA1(ca0ed9e89c340cb3b08f9a9002af9997372c1cbf),
						"samp_2.c21", CRC(454a5cca) SHA1(66b1a5832134365fd762fcba4cf4d666f60ebd65),
						"samp_3.c21", CRC(1f4928f4) SHA1(9949ab96644984fab8037224f52ec28d7d7cc967))
ALVG_ROMEND
CORE_GAMEDEFNV(wtgband,"World Tour Garage Band",1992,"Alvin G",mALVGS,GAME_IMPERFECT_GRAPHICS)

//DMD
#if 0
"dot27c.512"   , CRC(c8bd48e7) SHA1(e2dc513dd42c05c2018e6d8c0b6f0b2c56e6e059)
"romdef1.c20"  , CRC(045b21c1) SHA1(134b7eb0f71506d12d9ded24999d530126c558fc)
"romdef2.c20"  , CRC(23c32ee5) SHA1(429b3b069251bb8b681bbc6382ceb6b85125eb79)
#endif

/*-------------------------------------------------------------------
/ Mystery Castle
/-------------------------------------------------------------------*/
//Need CPU & DMD Roms
INITGAME(mystcast, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, 5)
ALVGROMSTART(mystcast,	"mcast_c0.rom", NO_DUMP)
ALVGS_SOUNDROM(		"s081r03_.rom" , CRC(bd4849ac) SHA1(f477ea369539a65c0960be1f1c3b4c5503dd6b75),
					"mcast_s0.rom" , CRC(0855cc73) SHA1(c46e08432bcff24594c33171f20669ba63828931),
					"mcast_s1.rom" , CRC(3b5d76e0) SHA1(b2e1bca3c596eba89feda868fa56c71a6b22414c),
					"mcast_s2.rom" , CRC(c3ffd277) SHA1(d16d1b22089b89bbf0db7d2b66c9745a56034322),
					"mcast_s3.rom" , CRC(1fa20ff1) SHA1(2bad7cddb4c8fc08780740b077f74cdf47fc2e5c))
ALVG_ROMEND
CORE_GAMEDEFNV(mystcast,"Mystery Castle",1993,"Alvin G",mALVGS,GAME_IMPERFECT_GRAPHICS)

/*-------------------------------------------------------------------
/ Pistol Poker
/-------------------------------------------------------------------*/
INITGAME(pstlpkr, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS, 5)
ALVGROMSTART(pstlpkr,	"p_peteu2.512", CRC(490a1e2d) SHA1(907dd858ed948681e7366a64a0e7537ebe301d6b))
ALVGS_SOUNDROM(			"p_pu102.512" , CRC(b8fb806e) SHA1(c2dc19820ea22bbcf5808db2fb4be76a4033d6ea),
						"p_parom0.c20", CRC(99986af2) SHA1(52fa7d2979f7f2d6d65ab6d4f7bbfbed16303991),
						"p_parom1.c20", CRC(ae2af238) SHA1(221d3a0e3fb1daad261d723e873ef0727b88889e),
						"p_parom2.c20", CRC(f39560a4) SHA1(cdfdf7b44ff4c3f9f4d39fbd8ecbf141d8568088),
						"p_parom3.c20", CRC(19d5e4de) SHA1(fb59166ebf992e81b92a42898e351d8443adb1c3))
ALVG_ROMEND
CORE_GAMEDEFNV(pstlpkr,"Pistol Poker",1993,"Alvin G",mALVGS,GAME_IMPERFECT_GRAPHICS)

//DMD
#if 0
"p_peteu4.512" , CRC(caa0cabd) SHA1(caff6ca4a9cce4e3d846502696c8838805673261)
"p_peteu5.c20" , CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7)
"p_peteu6.c20" , CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50)
#endif
