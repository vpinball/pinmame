#include "driver.h"
#include "sim.h"
#include "alvg.h"
#include "alvgs.h"
#include "alvgdmd.h"
#include "sndbrd.h"

#define DMD       alvg_dispDMD
#define DMD2      alvg_dispDMD2
#define DMD3      alvg_dispDMD3

#define FLIP78    FLIP_SWNO(7,8)

/* Dot-Matrix display (128 x 32) */
static struct core_dispLayout alvg_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(genf *)alvgdmd_update,NULL}, {0}
};
/* Dot-Matrix display (128 x 32) with 16 possible shades */
static struct core_dispLayout alvg_dispDMD2[] = {
  {0,0,32,128,CORE_DMD|CORE_DMDNOAA,(genf *)alvgdmd_update2,NULL}, {0}
};

/* Dot-Matrix display (128 x 32) with 16 possible shades inverted frame order*/
static struct core_dispLayout alvg_dispDMD3[] = {
  {0,0,32,128,CORE_DMD|CORE_DMDNOAA,(genf *)alvgdmd_update3,NULL}, {0}
};

/* Alpha Numeric Display (2 X 20 Alpha-Numeric) */
static const core_tLCDLayout alvg_alpha[] = {
  {0,0, 0,20,CORE_SEG16},
  {3,0,20,20,CORE_SEG16},
  {0}
};

/* Alpha Numeric Display (1 X 20 Alpha-Numeric) */
static const core_tLCDLayout alvg_alpha1[] = {
  {0,0, 0,20,CORE_SEG16},
  {0}
};

#define INITGAME(name, disptype, flippers, balls, sb, db, lamps) \
	ALVG_INPUT_PORTS_START(name, balls) ALVG_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_ALVG,disptype,{flippers,4,4,0,sb,db,lamps}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME2(name, disptype, flippers, balls, sb, db, lamps) \
	ALVG_INPUT_PORTS_START(name, balls) ALVG_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_ALVG_DMD2,disptype,{flippers,4,4,0,sb,db,lamps}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ A.G. Soccer-Ball
/-------------------------------------------------------------------*/
INITGAME(agsoccer, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(agsoccer,	"agscpu1r.18u", CRC(37affcf4) SHA1(017d47f54d5b34a4b71c2f5b84ba9bdb1c924299))
ALVGS_SOUNDROM11(		"ags_snd.v21",  CRC(aa30bfe4) SHA1(518f7019639a0284461e83ad849bee0be5371580),
						"ags_voic.v12", CRC(bac70b18) SHA1(0a699eb95d7d6b071b2cd9d0bf73df355e2ffce8))
ALVG_ROMEND
CORE_GAMEDEFNV(agsoccer,"A.G. Soccer-Ball (R18u)",1993,"Alvin G",mALVGS1,0)

INITGAME(agsocc07, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(agsocc07, "ags_cpu_r07u", CRC(009ef717) SHA1(d770ce8fd032f4f1d96b9792509cceebbfaebbd9))
ALVGS_SOUNDROM11(      "ags_snd.v14",  CRC(2544e468) SHA1(d49e2fc91cbb80fdf96f436c614c6f305efafb6f),
                       "ags_voic.v12", CRC(bac70b18) SHA1(0a699eb95d7d6b071b2cd9d0bf73df355e2ffce8))
ALVG_ROMEND
CORE_CLONEDEFNV(agsocc07,agsoccer,"A.G. Soccer-Ball (R07u)",1992,"Alvin G",mALVGS1,0)

/*-------------------------------------------------------------------
/ A.G. Football (actually the same ROMs as Soccer-Ball sold in Europe)
/-------------------------------------------------------------------*/
INITGAME(agfootbl, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(agfootbl,	"agscpu1r.18u", CRC(37affcf4) SHA1(017d47f54d5b34a4b71c2f5b84ba9bdb1c924299))
ALVGS_SOUNDROM11(		"ags_snd.v21",  CRC(aa30bfe4) SHA1(518f7019639a0284461e83ad849bee0be5371580),
						"ags_voic.v12", CRC(bac70b18) SHA1(0a699eb95d7d6b071b2cd9d0bf73df355e2ffce8))
ALVG_ROMEND
CORE_CLONEDEFNV(agfootbl,agsoccer,"A.G. Football (R18u)",1993,"Alvin G",mALVGS1,0)

INITGAME(agfoot07, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(agfoot07, "ags_cpu_r07u", CRC(009ef717) SHA1(d770ce8fd032f4f1d96b9792509cceebbfaebbd9))
ALVGS_SOUNDROM11(      "ags_snd.v14",  CRC(2544e468) SHA1(d49e2fc91cbb80fdf96f436c614c6f305efafb6f),
                       "ags_voic.v12", CRC(bac70b18) SHA1(0a699eb95d7d6b071b2cd9d0bf73df355e2ffce8))
ALVG_ROMEND
CORE_CLONEDEFNV(agfoot07,agsoccer,"A.G. Football (R07u)",1992,"Alvin G",mALVGS1,0)

/*-------------------------------------------------------------------
/ Al's Garage Band Goes On a World Tour
/-------------------------------------------------------------------*/
INITGAME(wrldtour, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 0)
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
CORE_GAMEDEFNV(wrldtour,"Al's Garage Band Goes On a World Tour (R01c)",1993,"Alvin G",mALVGS2DMD1,0) // release 03/30/93

INITGAME(wrldtou2, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 0)
ALVGROMSTART(wrldtou2,	"cpu02b.512", CRC(1658bf40) SHA1(7af9eedab4e7d0cedaf8bfdbc1f27b989a7171cd))
ALVGS_SOUNDROM(			"soundc.512", CRC(b44bee01) SHA1(795d8500e5bd73ce23756bf1f5c96db1a3621a70),
						"samp_0.c21", CRC(37beb831) SHA1(2b90d2be0a1bd7c59469846631d2b44bdf9f5f9d),
						"samp_1.c21", CRC(621533c6) SHA1(ca0ed9e89c340cb3b08f9a9002af9997372c1cbf),
						"samp_2.c21", CRC(454a5cca) SHA1(66b1a5832134365fd762fcba4cf4d666f60ebd65),
						"samp_3.c21", CRC(1f4928f4) SHA1(9949ab96644984fab8037224f52ec28d7d7cc967))
ALVGDMD_SPLIT_ROM(		"dot02b.512", CRC(50e3d59d) SHA1(db6df3482fc485af6bde341750bf8072a296b8da),
						"romdef1.c20",CRC(045b21c1) SHA1(134b7eb0f71506d12d9ded24999d530126c558fc),
						"romdef2.c20",CRC(23c32ee5) SHA1(429b3b069251bb8b681bbc6382ceb6b85125eb79))
ALVG_ROMEND
CORE_CLONEDEFNV(wrldtou2,wrldtour,"Al's Garage Band Goes On a World Tour (R02b)",1993,"Alvin G",mALVGS2DMD1,0) // beta 02/19/93

INITGAME(wrldtou3, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 0)
ALVGROMSTART(wrldtou3,	"cpu03.512",  CRC(56dee967) SHA1(f7b1f69d96c72b0cf738bdf45701502f7306a4a0))
ALVGS_SOUNDROM(			"soundc.512", CRC(b44bee01) SHA1(795d8500e5bd73ce23756bf1f5c96db1a3621a70),
						"samp_0.c21", CRC(37beb831) SHA1(2b90d2be0a1bd7c59469846631d2b44bdf9f5f9d),
						"samp_1.c21", CRC(621533c6) SHA1(ca0ed9e89c340cb3b08f9a9002af9997372c1cbf),
						"samp_2.c21", CRC(454a5cca) SHA1(66b1a5832134365fd762fcba4cf4d666f60ebd65),
						"samp_3.c21", CRC(1f4928f4) SHA1(9949ab96644984fab8037224f52ec28d7d7cc967))
ALVGDMD_SPLIT_ROM(		"dot03.512",  CRC(f8a084bb) SHA1(30eb344ad96b5605693d3a7c703c9ed5c1770ca4),
						"romdef1.c20",CRC(045b21c1) SHA1(134b7eb0f71506d12d9ded24999d530126c558fc),
						"romdef2.c20",CRC(23c32ee5) SHA1(429b3b069251bb8b681bbc6382ceb6b85125eb79))
ALVG_ROMEND
CORE_CLONEDEFNV(wrldtou3,wrldtour,"Al's Garage Band Goes On a World Tour (R06a Prototype)",1993,"Alvin G",mALVGS2DMD1,0) // alpha 02/05/93, supports an additional VUK/top lock at the end of the horseshoe ramp

// even earlier alpha AG03/R01A 1/15/93 exists

/*-------------------------------------------------------------------
/ U.S.A. Football
/-------------------------------------------------------------------*/
INITGAME(usafootb, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(usafootb,	"usa_cpu.bin", CRC(53b00873) SHA1(96812c4722026554a830c62eca64f09d25a0de82))
ALVGS_SOUNDROM11(		"usa_snd.bin", CRC(9d509cbc) SHA1(0be629945b5102adf75e88661e0f956e32ca77da),
						"usa_vox.bin", CRC(baae0aa3) SHA1(7933bffcf1509ceeea58a4449268c10c9fac554c))
ALVG_ROMEND
CORE_GAMEDEFNV(usafootb,"U.S.A. Football (R06u)",1993,"Alvin G",mALVGS1,0)

INITGAME(usafoota, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(usafoota,	"usa_cpu1.bin",CRC(3b64a6e9) SHA1(65535bc17395416181bafddc61c0fac177eeba2f))
ALVGS_SOUNDROM11(		"usa_snd.bin", CRC(9d509cbc) SHA1(0be629945b5102adf75e88661e0f956e32ca77da),
						"usa_vox.bin", CRC(baae0aa3) SHA1(7933bffcf1509ceeea58a4449268c10c9fac554c))
ALVG_ROMEND
CORE_CLONEDEFNV(usafoota,usafootb,"U.S.A. Football (R01u)",1992,"Alvin G",mALVGS1,0)

/*-------------------------------------------------------------------
/ Mystery Castle
/-------------------------------------------------------------------*/
INITGAME2(mystcast, DMD3, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 0)
ALVGROMSTART(mystcast,	"mcastle.cpu", CRC(936e6799) SHA1(aa29fb5f12f34c695d1556232744f65cd576a2b1))
ALVGS_SOUNDROM(			"mcastle.102", CRC(752822d0) SHA1(36461ef03cac5aefa0c03dfdc63c3d294a3b9c09),
						"mcastle.sr0", CRC(0855cc73) SHA1(c46e08432bcff24594c33171f20669ba63828931),
						"mcastle.sr1", CRC(3b5d76e0) SHA1(b2e1bca3c596eba89feda868fa56c71a6b22414c),
						"mcastle.sr2", CRC(c3ffd277) SHA1(d16d1b22089b89bbf0db7d2b66c9745a56034322),
						"mcastle.sr3", CRC(740858bb) SHA1(d2e9a0a178977dcc873368b042cea7052578df66))
ALVGDMD_ROM2R(			"mcastle.du4", CRC(686e253a) SHA1(28aff34c120c61e231e2111dc396df515bcbbb89),
						"mcastle.du5", CRC(9095c367) SHA1(9d3e9416f662ee2aad891eef059278c530448fcc))
ALVG_ROMEND
CORE_GAMEDEFNV(mystcast,"Mystery Castle (R02)",1993,"Alvin G",mALVGS2DMD3,0)

INITGAME2(mystcasa, DMD3, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 0)
ALVGROMSTART(mystcasa,	"cpu_103.bin", CRC(70ab8ece) SHA1(2bf8cd042450968b7500552419a9af5df2589c13))
ALVGS_SOUNDROM(			"mcastle.103", CRC(bd4849ac) SHA1(f477ea369539a65c0960be1f1c3b4c5503dd6b75),
						"mcastle.sr0", CRC(0855cc73) SHA1(c46e08432bcff24594c33171f20669ba63828931),
						"mcastle.sr1", CRC(3b5d76e0) SHA1(b2e1bca3c596eba89feda868fa56c71a6b22414c),
						"mcastle.sr2", CRC(c3ffd277) SHA1(d16d1b22089b89bbf0db7d2b66c9745a56034322),
						"mcastle.sr3", CRC(740858bb) SHA1(d2e9a0a178977dcc873368b042cea7052578df66))
ALVGDMD_ROM(			"u4.bin", CRC(a6969efc) SHA1(82da976cb3d30d6fb1576e4c67febd7235f73f51),
						"u5.bin", CRC(e5126980) SHA1(2c6d412c87bf27098dae4351958d84e8f9348423),
						"u6.bin", CRC(eb241633) SHA1(8e5db75b32ed2ea74088615bbe1403d4c8feafbd))
ALVG_ROMEND
CORE_CLONEDEFNV(mystcasa,mystcast,"Mystery Castle (R03)",199?,"Alvin G",mALVGS2DMD3,0)

/*-------------------------------------------------------------------
/ Pistol Poker
/-------------------------------------------------------------------*/
INITGAME2(pstlpkr, DMD2, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 1)
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
CORE_GAMEDEFNV(pstlpkr,"Pistol Poker (R02)",1993,"Alvin G",mALVGS2DMD2,0)

INITGAME2(pstlpkr1, DMD2, FLIP78, 3/*?*/, SNDBRD_ALVGS2, SNDBRD_ALVGDMD, 1)
ALVGROMSTART(pstlpkr1,	"u2-ddff.512",  CRC(83fa0595) SHA1(d6ebb0e63fd964ccaee3979a7fc13b6adf7b837c))
ALVGS_SOUNDROM(			"p_pu102.512" , CRC(b8fb806e) SHA1(c2dc19820ea22bbcf5808db2fb4be76a4033d6ea),
						"p_parom0.c20", CRC(99986af2) SHA1(52fa7d2979f7f2d6d65ab6d4f7bbfbed16303991),
						"p_parom1.c20", CRC(ae2af238) SHA1(221d3a0e3fb1daad261d723e873ef0727b88889e),
						"p_parom2.c20", CRC(f39560a4) SHA1(cdfdf7b44ff4c3f9f4d39fbd8ecbf141d8568088),
						"p_parom3.c20", CRC(19d5e4de) SHA1(fb59166ebf992e81b92a42898e351d8443adb1c3))
ALVGDMD_ROM(			"p_peteu4.512", CRC(caa0cabd) SHA1(caff6ca4a9cce4e3d846502696c8838805673261),
						"p_peteu5.c20", CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7),
						"p_peteu6.c20", CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50))
ALVG_ROMEND
CORE_CLONEDEFNV(pstlpkr1,pstlpkr,"Pistol Poker (R01)",1993,"Alvin G",mALVGS2DMD2,0)

/*-------------------------------------------------------------------
/ Punchy The Clown
/-------------------------------------------------------------------*/
INITGAME(punchy, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(punchy,	"epc061.r02", CRC(732fca88) SHA1(dff0aa4b856bafb95b08dae675dd2ad59e1860e1))
ALVGS_SOUNDROM11(		"eps061.r02", CRC(cfde1b9a) SHA1(cbf9e67df6a6762843272493c2caa1413f70fb27),
						"eps062.r02", CRC(7462a5cd) SHA1(05141bcc91b1a786444bff7fa8ba2a785dc0d376))
ALVG_ROMEND
CORE_GAMEDEFNV(punchy,"Punchy The Clown (R02)",1993,"Alvin G",mALVGS1,0)

INITGAME(punchy3, alvg_alpha, FLIP78, 3/*?*/, SNDBRD_ALVGS1, 0, 0)
ALVGROMSTART(punchy3,	"epc061.r03", CRC(8e91131c) SHA1(1bf1408e4e512b764048f4847cf8e4b7a0bf824d))
ALVGS_SOUNDROM11(		"eps061.r02", CRC(cfde1b9a) SHA1(cbf9e67df6a6762843272493c2caa1413f70fb27),
						"eps062.r02", CRC(7462a5cd) SHA1(05141bcc91b1a786444bff7fa8ba2a785dc0d376))
ALVG_ROMEND
CORE_CLONEDEFNV(punchy3,punchy,"Punchy The Clown (R03)",1993,"Alvin G",mALVGS1,0)

/*-------------------------------------------------------------------
/ Dinosaur Eggs
/-------------------------------------------------------------------*/
INITGAME(dinoeggs, alvg_alpha1, FLIP78, 3/*?*/, SNDBRD_ALVGS2, 0, 0)
ALVGROMSTART(dinoeggs,	"dinoeggs.512", CRC(4712f97f) SHA1(593351dcfd475e685c1e5eb2c1006769d3325c8b))
ALVGS_SOUNDROM11(		"eps071.r02",   CRC(288f116c) SHA1(5d03ce66bffe39ec02173525078ff07c5005ef18),
						"eps072.r02",   CRC(780a4364) SHA1(d8a972debee669f0fe66c7407fbed5ef9cd2ce01))
ALVG_ROMEND
CORE_GAMEDEFNV(dinoeggs,"Dinosaur Eggs (R02)",1993,"Alvin G",mALVGS2,0)


//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
/*-------------------------------------------------------------------
/ Test 8031 CPU Core (uses either Pistol Poker dmd roms or World Tour)
/-------------------------------------------------------------------*/
INITGAME(test8031, DMD, FLIP78, 3/*?*/, SNDBRD_ALVGS2, 0, 1)
ROM_START(test8031)

TEST8031_ROM(			"addsub.bin", NO_DUMP,
						"p_peteu5.c20", CRC(1d2cecd8) SHA1(6072a0f744fb9eef728fe7cf5e17d0007edbddd7),
						"p_peteu6.c20", CRC(3a56376c) SHA1(69febc17b8416c03a58e651447bbe1e14ff27e50))
ALVG_ROMEND
CORE_GAMEDEFNV(test8031,"Test 8031 CPU Core",2003,"Steve Ellenoff",mTEST8031,0)
#endif
