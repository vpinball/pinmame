// license:BSD-3-Clause

#include "driver.h"
#include "sim.h"
#include "spinb.h"
#include "sndbrd.h"

#define GEN_SPINB 0

/* Dot-Matrix display */
static struct core_dispLayout spinb_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(genf *)SPINBdmd_update,NULL}, {0}
};

// DMD frames per picture
#define SPINB_DSP1 1
#define SPINB_DSP2 2
#define SPINB_DSP3 3

#define SPINB_GS1(solInvert0,solInvert1,solInvert2,lamp16isGameOn) \
(((UINT8)(solInvert0)<<24) | ((UINT8)(solInvert1)<<16) | ((UINT8)(solInvert2)<<8) | ((UINT8)(lamp16isGameOn)))

#define INITGAME(name, disptype, flippers, balls, sb, db, gs1, gs2) \
	SPINB_INPUT_PORTS_START(name, balls) SPINB_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SPINB,disptype,{flippers,0,4,0,sb,db,gs1,gs2}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

/*-------------------------------------------------------------------
/ Bushido (1993) - (Last game by Inder - before becomming Spinball - but same hardware)
/-------------------------------------------------------------------*/
// Note that bushido has (at least) different Trough-Switches compared to bushidoa and bushidob
INITGAME(bushido, spinb_dispDMD, FLIP_SWNO(0,0), 3, SNDBRD_SPINB, SPINB_DSP1, SPINB_GS1(0x11,0x0C,0,FALSE), 175)
SPINB_ROMSTART(bushido,	"0-z80.bin",    CRC(3ea1eb1d) SHA1(cceb6c68e481f36a5646ff4f38d3dfc4275b0c79),
						"1-z80.old",    CRC(648da72b) SHA1(1005a13b4746e302d979c8b1da300e943cdcab3d))   // more code in it than alternate set A and B
SPINB_DMDROM1(			"g-disply.bin", CRC(9a1df82f) SHA1(4ad6a12ae36ec898b8ac5243da6dec3abcd9dc33))
SPINB_SNDROM22(			"a-sonido.bin", CRC(cf7d5399) SHA1(c79145826cfa6be2487e3add477d9b452c553762),
						"b-sonido.bin", CRC(cb4fc885) SHA1(569f389fa8f91f886b58f44f701d2752ef01f3fa),	//Sound Effects 1
						"c-sonido.bin", CRC(35a43dd8) SHA1(f2b1994f67f749c65a88c95d970b655990d85b96),	//Sound Effects 2
						"d-musica.bin", CRC(2cb9697c) SHA1(d5c66d616ccd5e299832704e494743429dafd569),
						"e-musica.bin", CRC(1414b921) SHA1(5df9e538ee109df28953ec8f162c60cb8c6e4d96),	//Music 1
						"f-musica.bin", CRC(80f3a6df) SHA1(e09ad4660e511779c6e55559fa0c2c0b0c6600c8))	//Music 2
SPINB_ROMEND
CORE_GAMEDEFNV(bushido,"Bushido",1993,"Inder/Spinball (Spain)",mSPINBS,0)

SPINB_ROMSTART(bushidoa,"0-cpu.bin",    CRC(7f7e6642) SHA1(6872397eed7525f384b79cdea13531d273d8cf14),
						"1-cpu.bin",    CRC(a538d37f) SHA1(d2878ad0d31b4221b823812485c7faaf666ce185))
SPINB_DMDROM1(			"g-disply.bin", CRC(9a1df82f) SHA1(4ad6a12ae36ec898b8ac5243da6dec3abcd9dc33))
SPINB_SNDROM22(			"a-sonido.bin", CRC(cf7d5399) SHA1(c79145826cfa6be2487e3add477d9b452c553762),
						"b-sonido.bin", CRC(cb4fc885) SHA1(569f389fa8f91f886b58f44f701d2752ef01f3fa),	//Sound Effects 1
						"c-sonido.bin", CRC(35a43dd8) SHA1(f2b1994f67f749c65a88c95d970b655990d85b96),	//Sound Effects 2
						"d-musica.bin", CRC(2cb9697c) SHA1(d5c66d616ccd5e299832704e494743429dafd569),
						"e-musica.bin", CRC(1414b921) SHA1(5df9e538ee109df28953ec8f162c60cb8c6e4d96),	//Music 1
						"f-musica.bin", CRC(80f3a6df) SHA1(e09ad4660e511779c6e55559fa0c2c0b0c6600c8))	//Music 2
SPINB_ROMEND
#define init_bushidoa init_bushido
#define input_ports_bushidoa input_ports_bushido
CORE_CLONEDEFNV(bushidoa,bushido,"Bushido (alternate set A)",1993,"Inder/Spinball (Spain)",mSPINBS,0)

// "On the real pinball, bushidob locks when you lost the final match sequence after a game,
//  but works fine if we use the older MAME romset (we've been playing it for longer before returning it back)."
// -> Bad dump? Bitrot? But works fine in PinMAME as-is. So maybe a machine issue?!
SPINB_ROMSTART(bushidob,"inder_sa_m-17_pb-in_a-1954-93_b4_cpu rom_0_06-a.bin", CRC(7c8561f7) SHA1(d81338160f6eb8898a7b779f57e327d2db3f7dab),
						"inder_sa_m-17_pb-in_a-1954-93_b4_cpu rom_1_06-a.bin", CRC(4a41865c) SHA1(f597d5dd42b804ab637408b90d05bdb234559a48)) // more code in it than alternate set A
SPINB_DMDROM1(			"g-disply.bin", CRC(9a1df82f) SHA1(4ad6a12ae36ec898b8ac5243da6dec3abcd9dc33))
SPINB_SNDROM22(			"a-sonido.bin", CRC(cf7d5399) SHA1(c79145826cfa6be2487e3add477d9b452c553762),
						"b-sonido.bin", CRC(cb4fc885) SHA1(569f389fa8f91f886b58f44f701d2752ef01f3fa),	//Sound Effects 1
						"c-sonido.bin", CRC(35a43dd8) SHA1(f2b1994f67f749c65a88c95d970b655990d85b96),	//Sound Effects 2
						"d-musica.bin", CRC(2cb9697c) SHA1(d5c66d616ccd5e299832704e494743429dafd569),
						"e-musica.bin", CRC(1414b921) SHA1(5df9e538ee109df28953ec8f162c60cb8c6e4d96),	//Music 1
						"f-musica.bin", CRC(80f3a6df) SHA1(e09ad4660e511779c6e55559fa0c2c0b0c6600c8))	//Music 2
SPINB_ROMEND
#define init_bushidob init_bushido
#define input_ports_bushidob input_ports_bushido
CORE_CLONEDEFNV(bushidob,bushido,"Bushido (alternate set B)",1993,"Inder/Spinball (Spain)",mSPINBS,0)

/*-------------------------------------------------------------------
/ Mach 2 (1995) // or Mach 2.0 Two ?
/-------------------------------------------------------------------*/
INITGAME(mach2, spinb_dispDMD, FLIP_SWNO(0,0), 3, SNDBRD_SPINB, SPINB_DSP2, SPINB_GS1(0x11,0,0,TRUE), 175)
SPINB_ROMSTART(mach2,	"m2cpu0.19", CRC(274c8040) SHA1(6b039b79b7e08f2bf2045bc4f1cbba790c999fed),
						"m2cpu1.19", CRC(c445df0b) SHA1(1f346c1df8df0a3c4e8cb1186280d2f34959b3f8))
SPINB_DMDROM1(			"m2dmdf.01", CRC(c45ccc74) SHA1(8362e799a76536a16dd2d5dde500ad3db273180f))
SPINB_SNDROM12(			"m2sndd.01", CRC(e789f22d) SHA1(36aa7eac1dd37a02c982d109462dddbd85a305cc),
						"m2snde.01", CRC(f5721119) SHA1(9082198e8d875b67323266c4bf8c2c378b63dfbb),	//Sound Effects
						"m2musa.01", CRC(2d92a882) SHA1(cead22e434445e5c25414646b1e9ae2b9457439d),
						"m2musb.01", CRC(6689cd19) SHA1(430092d51704dfda8bd8264875f1c1f4461c56e5),	//Music 1
						"m2musc.01", CRC(88851b82) SHA1(d0c9fa391ca213a69b7c8ae7ca52063503b5656e))	//Music 2
SPINB_ROMEND
CORE_GAMEDEFNV(mach2,"Mach 2",1995,"Spinball (Spain)",mSPINBS,0)

SPINB_ROMSTART(mach2a,	"mach_cpu_0.bin", CRC(4eee3d63) SHA1(dc7c919dd909a134ec434ce665c421aa03aaa637),
						"mach_cpu_1.bin", CRC(c115b900) SHA1(712ae2a38528c05f28e3743cb49e072fc05bf908))
SPINB_DMDROM1(			"m2dmdf.01", CRC(c45ccc74) SHA1(8362e799a76536a16dd2d5dde500ad3db273180f))
SPINB_SNDROM12(			"m2sndd.01", CRC(e789f22d) SHA1(36aa7eac1dd37a02c982d109462dddbd85a305cc),
						"m2snde.01", CRC(f5721119) SHA1(9082198e8d875b67323266c4bf8c2c378b63dfbb),	//Sound Effects
						"m2musa.01", CRC(2d92a882) SHA1(cead22e434445e5c25414646b1e9ae2b9457439d),
						"m2musb.01", CRC(6689cd19) SHA1(430092d51704dfda8bd8264875f1c1f4461c56e5),	//Music 1
						"m2musc.01", CRC(88851b82) SHA1(d0c9fa391ca213a69b7c8ae7ca52063503b5656e))	//Music 2
SPINB_ROMEND
#define init_mach2a init_mach2
#define input_ports_mach2a input_ports_mach2
CORE_CLONEDEFNV(mach2a,mach2,"Mach 2 (alternate set)",1995,"Spinball (Spain)",mSPINBS,0)

/*-------------------------------------------------------------------
/ Jolly Park (1996)
/-------------------------------------------------------------------*/
INITGAME(jolypark, spinb_dispDMD, FLIP_SWNO(0,0), 4, SNDBRD_SPINB, SPINB_DSP3, SPINB_GS1(0x11,0,0xff,TRUE), 175)
SPINB_ROMSTART(jolypark,"jpcpu0.rom", CRC(061967af) SHA1(45048e1d9f17efa3382460fd474a5aeb4191d617),
						"jpcpu1.rom", CRC(ea99202f) SHA1(e04825e73fd25f6469b3315f063f598ea1ab44c7))
SPINB_DMDROM2(			"jpdmd0.rom", CRC(b57565cb) SHA1(3fef66d298893029de78fdb6ecdb562c33d76180),
						"jpdmd1.rom", CRC(40d1563f) SHA1(90dbea742202340da6fa950eedc2bceec5a2af7e))
SPINB_SNDROM23(			"jpsndc1.rom", CRC(0475318f) SHA1(7154bd5ca5b28019eb0ff598ec99bbe49260932b),
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
INITGAME(vrnwrld, spinb_dispDMD, FLIP_SWNO(0,0), 4, SNDBRD_SPINB, SPINB_DSP3/*?*/, SPINB_GS1(0xff,0xff,0xff,TRUE), 175)
SPINB_ROMSTART2(vrnwrld,"vwcpu0.rom", CRC(89c980e7) SHA1(09eeed0232255875cf119d59143d839ff40e30dd),
						"vwcpu1.rom", CRC(a4db4e64) SHA1(fc55781295fc723741de24ad60311b7e33551830))
SPINB_DMDROM3(			"vwdmd0.rom", CRC(40600060) SHA1(7ad619bcb5e5e50325360f4e946b5bfa072caead),
						"vwdmd1.rom", CRC(de4a1060) SHA1(6b848dfd8aafdbcf7e1593f98bd1c3d69306aa11),
						"vwdmd2.rom", CRC(29fc8da7) SHA1(2704f14a3338a63abda3bcbc56e9f984a679eb38))
SPINB_SNDROM13(			"vws2ic9.rom", CRC(ab8cb4c5) SHA1(92a702c11e2cef703992244529ba86079d5ab9b0),
						"vws3ic15.rom", CRC(d62c9443) SHA1(7c6b8662d88ba6592da8b83af11087647105e8dd),	//Sound Effects 1
						"vws4ic30.rom", CRC(ecd18a19) SHA1(558e687e0429d31fafe8db05954d9a8ad90d6aeb),
						"vws5ic25.rom", CRC(56d349f0) SHA1(e71d2d03c3e978c552e272de8850cc265255fbd1),	//Music 1
						"vws6ic26.rom", CRC(bee399c1) SHA1(b2c6e4830641ed32b9643dc8c1fa08a2da5a7e9b),	//Music 2
						"vws7ic27.rom", CRC(7335b29c) SHA1(4de6de09f069feecbad2e5ef50032e8d381ff9b1))	//Music 3
SPINB_ROMEND
CORE_GAMEDEFNV(vrnwrld,"Verne's World",1996,"Spinball (Spain)",mSPINBSNMI2,0)
