#include "driver.h"
#include "sim.h"
#include "alvg.h"
#include "alvgs.h"
#include "sndbrd.h"

//#define DMD       alvg_dispDMD
#define DMD			alvg_NoDMD

#define FLIP78   FLIP_SWNO(7,8)

//Here for testing, until we code up the DMD section (VPM doesn't like it, if we leave the structure empty, so we use alpha display)
static struct core_dispLayout alvg_NoDMD[] = { /* 2 X 16 AlphaNumeric Rows */
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

#if 0
/* Dot-Matrix display */
static struct core_dispLayout alvg_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)alvg_dmd128x32}, {0}
};
#endif

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
CORE_GAMEDEFNV(wtgband,"World Tour Garage Band",1992,"Alvin G",mALVGS,GAME_IMPERFECT_SOUND)

#if 0
"dot27c.512"   , CRC(c8bd48e7) SHA1(e2dc513dd42c05c2018e6d8c0b6f0b2c56e6e059)
"romdef1.c20"  , CRC(045b21c1) SHA1(134b7eb0f71506d12d9ded24999d530126c558fc)
"romdef2.c20"  , CRC(23c32ee5) SHA1(429b3b069251bb8b681bbc6382ceb6b85125eb79)
#endif

/*-------------------------------------------------------------------
/ Mystery Castle
/-------------------------------------------------------------------*/
//Need Roms

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
CORE_GAMEDEFNV(pstlpkr,"Pistol Poker",1993,"Alvin G",mALVGS,GAME_IMPERFECT_SOUND)


#if 0
"p_peteu4.512" , CRC(caa0cabd) SHA1(caff6ca4a9cce4e3d846502696c8838805673261)
"p_peteu5.c20" , CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7)
"p_peteu6.c20" , CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50)
#endif
