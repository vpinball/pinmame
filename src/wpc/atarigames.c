#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "atari.h"

#define FLIPSW1920 FLIP_SWNO(19,20)
#define FLIPSW6667 FLIP_SWNO(66,67)

#define INITGAME1(name, disptype, flippers, balls, sndType) \
	ATARI1_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,9,0,SNDBRD_ATARI1,0,sndType}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME2(name, disptype, flippers, balls) \
	ATARI2_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,0,0,SNDBRD_ATARI2,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

#define INITGAME3(name, disptype, flippers, balls) \
	ATARI3_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,0,0,SNDBRD_ATARI2,0}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* 4 X 6 Segments, 2 X 2 Segments */
core_tLCDLayout atari_disp1[] = {
  { 0, 0, 2, 3, CORE_SEG87 }, { 0, 6, 5, 3, CORE_SEG87 },
  { 3, 0,10, 3, CORE_SEG87 }, { 3, 6,13, 3, CORE_SEG87 },
  { 6, 0,18, 3, CORE_SEG87 }, { 6, 6,21, 3, CORE_SEG87 },
  { 9, 0,26, 3, CORE_SEG87 }, { 9, 6,29, 3, CORE_SEG87 },
  { 9,16,32, 2, CORE_SEG87 }, { 9,22,34, 2, CORE_SEG87 },
  {0}
};

#define DISP_SEG_6(row,col,type) {4*row,16*col,row*20+col*8+2,6,type}

core_tLCDLayout atari_disp2[] = {
  DISP_SEG_6(0,0,CORE_SEG7), DISP_SEG_6(0,1,CORE_SEG7),
  DISP_SEG_6(1,0,CORE_SEG7), DISP_SEG_6(1,1,CORE_SEG7),
  DISP_SEG_CREDIT(42,43,CORE_SEG7), DISP_SEG_BALLS(46,47,CORE_SEG7), {0}
};

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

//!! potential bad dump(s)?
// 07028-01.bin : last 256 bytes zero // apparently okay though, as 'the games don't use the waveforms from the upper half'
// 20252-01.bin : first 256 bytes exactly the same as 07028-01.bin

//!! 82s130.bin (Pascal/dumped from a Superman) and 20967-01.bin (nuatari) differ in 4 bits "only"! (note: the upper 4bits of each byte in these ROMs must be ignored before comparing them!)
//   So we need to find out which one is really proper or if both are, and are just different revisions
//   ATARI_SNDSTART("20967-01.bin", CRC(08a1c881) SHA1(9422add065aab8a2edc01f11db0916200903f960))
//!! Comparing the waves from 82s130.bin and 20967-01.bin, one can find differences in 3 curves, BUT only one of these differences looks odd (weird spike found in wave 7 of 20967-01.bin), maybe that one was a bug?
//   So maybe 20967-01.bin is an earlier version and 82s130.bin the final?
//   Note though for the differences in wave 3:
//    82s130.bin and 20252-01.bin show the same curve there, which may be another indication that 20967-01.bin is just a bad dump, as in there wave 3 has 2 slightly differing values

//Triangle (1976?) never existed as a complete machine, only parts

/*-------------------------------------------------------------------
/ The Atarians (11/1976)
/-------------------------------------------------------------------*/
INITGAME1(atarians, atari_disp1, FLIP_SWNO(36,37), 1, 0)
ATARI_2_ROMSTART(atarians,	"atarian.e0",	CRC(45cb0427) SHA1(e286930ca36bdd0f79acefd142d2a5431fa8005b), // nuatari has exact same roms on webpage
							"atarian.e00",	CRC(6066bd63) SHA1(e993497d0ca9f056e18838494089def8bdc265c9))
ATARI_SNDSTART("07028-01.bin", CRC(e8034b5b) SHA1(6959912c530efcc4a0c690800867fb0d1f33627f))
ATARI_ROMEND
CORE_GAMEDEFNV(atarians,"Atarians, The",1976,"Atari",gl_mATARI0,0)

/*-------------------------------------------------------------------
/ Time 2000 (06/1977)
/-------------------------------------------------------------------*/
INITGAME1(time2000, atari_disp1, FLIPSW1920, 1, 0)
ATARI_2_ROMSTART(time2000,	"time.e0",	CRC(1e79c133) SHA1(54ce5d59a00334fcec8b12c077d70e3629549af0), // nuatari has exact same roms on webpage
							"time.e00",	CRC(e380f35c) SHA1(f2b4c508c8b7a2ce9924da97c05fb31d5115f36f))
ATARI_SNDSTART("07028-01.bin", CRC(e8034b5b) SHA1(6959912c530efcc4a0c690800867fb0d1f33627f))
ATARI_ROMEND
CORE_GAMEDEFNV(time2000,"Time 2000",1977,"Atari",gl_mATARI1,0)

/*-------------------------------------------------------------------
/ Airborne Avenger (09/1977)
/-------------------------------------------------------------------*/
INITGAME1(aavenger, atari_disp1, FLIPSW1920, 1, 1)
ATARI_2_ROMSTART(aavenger,	"airborne.e0",	CRC(44e67c54) SHA1(7f94189c12e322c41908d651cf6a3b6061426959),
							"airborne.e00",	CRC(05ac26b8) SHA1(114d587923ade9370d606e428af02a407d272c85))
ATARI_SNDSTART("20252-01.bin", CRC(3D44551D) SHA1(926100F8169AB20230AD2168F94E6AD65FB1A7DC))
ATARI_ROMEND
CORE_GAMEDEFNV(aavenger,"Airborne Avenger",1977,"Atari",gl_mATARI1,0)

/*-------------------------------------------------------------------
/ Middle Earth (02/1978)
/-------------------------------------------------------------------*/
INITGAME1(midearth, atari_disp1, FLIPSW1920, 1, 2)
ATARI_2_ROMSTART(midearth,	"608.bin", CRC(28b92faf) SHA1(8585770f4059049f1dcbc0c6ef5718b6ff1a5431), // nuatari has exact same roms on webpage
							"609.bin", CRC(589df745) SHA1(4bd3e4f177e8d86bab41f3a14c169b936eeb480a))
ATARI_SNDSTART("20252-01.bin", CRC(3D44551D) SHA1(926100F8169AB20230AD2168F94E6AD65FB1A7DC))
ATARI_ROMEND
CORE_GAMEDEFNV(midearth,"Middle Earth",1978,"Atari",gl_mATARI1A,0)

/*-------------------------------------------------------------------
/ Middle Earth (alternate)
/-------------------------------------------------------------------*/
INITGAME1(midearta, atari_disp1, FLIPSW1920, 1, 2)
ATARI_2_ROMSTART(midearta,	"20856-01.BIN",	CRC(8f119e37) SHA1(5a4d63605865f3ceca4c09dbdcd888498c615b89),
							"20855-01.BIN", CRC(4a9d47ca) SHA1(57c4458822109c3ba2fa53ac1c1cd6e169e51b24))
ATARI_SNDSTART("20252-01.bin", CRC(3D44551D) SHA1(926100F8169AB20230AD2168F94E6AD65FB1A7DC))
ATARI_ROMEND
CORE_CLONEDEFNV(midearta,midearth,"Middle Earth (alternate)",197?,"Atari",gl_mATARI1A,0)

// pre-production / white main cabinet has different CPU PROMs
// also a german CPU-PROMs variant exist (which seems to be the same as the pre-production according to nuatari?!)

/*-------------------------------------------------------------------
/ Middle Earth (pre-production and/or german) //!! this is the best we got from nuatari
/-------------------------------------------------------------------*/
INITGAME1(mideartp, atari_disp1, FLIPSW1920, 1, 2)
ATARI_2_ROMSTART(mideartp,	"c.e0",	CRC(553044c1) SHA1(4ad328eff48b82b32721684d181a339eae304d92) BAD_DUMP,
							"c.e00",CRC(288cd68d) SHA1(d97d31f59525b785bfa4c3fc8445eb294591bde2) BAD_DUMP)
ATARI_SNDSTART("20252-01.bin", CRC(3D44551D) SHA1(926100F8169AB20230AD2168F94E6AD65FB1A7DC)) // confirmed by nuatari
ATARI_ROMEND
CORE_CLONEDEFNV(mideartp,midearth,"Middle Earth (Prototype or German)",197?,"Atari",gl_mATARI1A,0)

/*-------------------------------------------------------------------
/ Space Riders (09/1978)
/-------------------------------------------------------------------*/
INITGAME1(spcrider, atari_disp1, FLIPSW6667, 1, 2)
ATARI_2_ROMSTART(spcrider,	"spacel.bin",	CRC(66ffb04e) SHA1(42d8b7fb7206b30478f631d0e947c0908dcf5419),
							"spacer.bin",	CRC(3cf1cd73) SHA1(c46044fb815b439f12fb3e21c470c8b93ebdfd55))
ATARI_SNDSTART("82s130.bin", CRC(da1f77b4) SHA1(b21fdc1c6f196c320ec5404013d672c35f95890b)) // 20967-01 // nuatari lists 20967-01 (and claims that all the SR boards (5) he has feature that one), manual schematics and parts list 20252-01 though
ATARI_ROMEND
CORE_GAMEDEFNV(spcrider,"Space Riders",1978,"Atari",gl_mATARI1,0)

/*-------------------------------------------------------------------
/ Superman (03/1979)
/-------------------------------------------------------------------*/
INITGAME2(superman, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(superman,	"supmn_k.rom",	CRC(a28091c2) SHA1(9f5e47db408da96a31cb2f3be0fa9fb1e79f8d85), // nuatari has exact same 3 roms on webpage
							"atari_m.rom",	CRC(1bb6b72c) SHA1(dd24ed54de275aadf8dc0810a6af3ac97aea4026),
							"atari_j.rom",	CRC(26521779) SHA1(2cf1c66441aee99b9d01859d495c12025b5ef094))
ATARI_SNDSTART("82s130.bin", CRC(da1f77b4) SHA1(b21fdc1c6f196c320ec5404013d672c35f95890b)) // 20967-01 //!! 82s130.bin has 4 bits set to 0 instead of 1 like the nuatari dump, so which one is "more correct"?
ATARI_ROMEND
CORE_GAMEDEFNV(superman,"Superman",1979,"Atari",gl_mATARI2,0)

// also a Add-A-Ball officially modified ROM exists https://pinwiki.com/wiki/images/d/da/Startech-atari-superman-prom-mod-add-a-ball.jpg

/*-------------------------------------------------------------------
/ Hercules (05/1979)
/-------------------------------------------------------------------*/
INITGAME2(hercules, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(hercules,	"herc_k.rom",	CRC(65e099b1) SHA1(83a06bc82e0f8f4c0655886c6a9962bb28d00c5e), // nuatari has exact same 3 roms on webpage
							"atari_m.rom",	CRC(1bb6b72c) SHA1(dd24ed54de275aadf8dc0810a6af3ac97aea4026),
							"atari_j.rom",	CRC(26521779) SHA1(2cf1c66441aee99b9d01859d495c12025b5ef094))
ATARI_SNDSTART("82s130.bin", CRC(da1f77b4) SHA1(b21fdc1c6f196c320ec5404013d672c35f95890b)) // 20967-01 //!! 82s130.bin has 4 bits set to 0 instead of 1 like the nuatari dump, so which one is "more correct"?
ATARI_ROMEND
CORE_GAMEDEFNV(hercules,"Hercules",1979,"Atari",gl_mATARI2,0)

/*-------------------------------------------------------------------
/ Road Runner (??/1979)
/-------------------------------------------------------------------*/
INITGAME3(roadrunr, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(roadrunr,	"0000.716",	CRC(62f5f394) SHA1(ff91066d43d788119e3337788abd86e5c0bf2d92),
							"3000.716",	CRC(2fc01359) SHA1(d3df20c764bb68a5316367bb18d34a03293e7fa6),
							"3800.716",	CRC(77262408) SHA1(3045a732c39c96002f495f64ed752279f7d43ee7))
ATARI_SNDSTART("82s130.bin", CRC(da1f77b4) SHA1(b21fdc1c6f196c320ec5404013d672c35f95890b)) // 20967-01 //!! 82s130.bin has 4 bits set to 0 instead of 1 like the nuatari dump, so which one is "more correct"? //!! unknown so far if using the 20967-01 is correct for Road Runner, but sounds good
ATARI_ROMEND
CORE_GAMEDEFNV(roadrunr,"Road Runner (Prototype)",1979,"Atari",gl_mATARI3,0)

//Pipeline (19??) backglass/art/parts only
//Wonder Woman (19??) backglass only
//Monza (1980) table-top, prototype, but no longer exists
//Neutron Star (1981) prototype

/*-------------------------------------------------------------------
/ 4x4 (10/1982)
/-------------------------------------------------------------------*/
core_tLCDLayout atari_disp4[] = {
  { 0, 0, 0,16, CORE_SEG87 },
  { 2, 0,16,16, CORE_SEG87 },
  { 0 }
};
ATARI2_INPUT_PORTS_START(fourx4, 3) ATARI_INPUT_PORTS_END
static core_tGameData fourx4GameData = {0,atari_disp4,{FLIP_SWNO(0,1),0,0,0,SNDBRD_NONE,0}};
static void init_fourx4(void) {
  core_gameData = &fourx4GameData;
}
ATARI_4x4_ROMSTART(fourx4,	"8000ce65.bin",	CRC(27341155) SHA1(c0da1fbf64f93ab163b2ea6bfbfc7b778cea819f),
							"a0004c37.bin",	CRC(6f93102f) SHA1(d6520987ed5805b0e6b5da5653fc7cb063e86dda),
							"c000a70c.bin",	CRC(c31ca8d3) SHA1(53f20eff0084771dc61d19db7ddae52e4423e75e))
ATARI_ROMEND
CORE_GAMEDEFNV(fourx4,"4x4 (Prototype)",1982,"Atari",gl_mATARI4,GAME_NOT_WORKING)
