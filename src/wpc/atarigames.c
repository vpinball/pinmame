#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "atari.h"

#define FLIPSW1920 FLIP_SWNO(19,20)
#define FLIPSW6667 FLIP_SWNO(66,67)

#define INITGAME1(name, disptype, flippers, balls) \
	ATARI1_INPUT_PORTS_START(name, balls) ATARI_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{flippers,0,9,0,SNDBRD_ATARI1,0}}; \
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

/*-------------------------------------------------------------------
/ The Atarians (11/1976)
/-------------------------------------------------------------------*/
INITGAME1(atarians, atari_disp1, FLIP_SWNO(36,37), 1)
ATARI_2_ROMSTART(atarians,	"atarian.e0",	CRC(45cb0427) SHA1(e286930ca36bdd0f79acefd142d2a5431fa8005b),
							"atarian.e00",	CRC(6066bd63) SHA1(e993497d0ca9f056e18838494089def8bdc265c9))
ATARI_ROMEND
CORE_GAMEDEFNV(atarians,"The Atarians",1976,"Atari",gl_mATARI0,GAME_NOT_WORKING)

/*-------------------------------------------------------------------
/ The Atarians (working bootleg)
/-------------------------------------------------------------------*/
INITGAME1(atarianb, atari_disp1, FLIP_SWNO(36,37), 1)
ATARI_2_ROMSTART(atarianb,	"atarian.e0",	CRC(45cb0427) SHA1(e286930ca36bdd0f79acefd142d2a5431fa8005b),
							"atarianb.e00",	CRC(74fc86e4) SHA1(135d75e5c03feae0929fa84caa3c802353cdd94e))
ATARI_ROMEND
CORE_CLONEDEFNV(atarianb,atarians,"The Atarians (working bootleg)",2002,"Atari / Gaston",gl_mATARI0,0)

/*-------------------------------------------------------------------
/ Time 2000 (06/1977)
/-------------------------------------------------------------------*/
INITGAME1(time2000, atari_disp1, FLIPSW1920, 1)
ATARI_2_ROMSTART(time2000,	"time.e0",	CRC(1e79c133) SHA1(54ce5d59a00334fcec8b12c077d70e3629549af0),
							"time.e00",	CRC(e380f35c) SHA1(f2b4c508c8b7a2ce9924da97c05fb31d5115f36f))
ATARI_ROMEND
CORE_GAMEDEFNV(time2000,"Time 2000",1977,"Atari",gl_mATARI1,0)

/*-------------------------------------------------------------------
/ Airborne Avenger (09/1977)
/-------------------------------------------------------------------*/
INITGAME1(aavenger, atari_disp1, FLIPSW1920, 1)
ATARI_2_ROMSTART(aavenger,	"airborne.e0",	CRC(44e67c54) SHA1(7f94189c12e322c41908d651cf6a3b6061426959),
							"airborne.e00",	CRC(05ac26b8) SHA1(114d587923ade9370d606e428af02a407d272c85))
ATARI_ROMEND
CORE_GAMEDEFNV(aavenger,"Airborne Avenger",1977,"Atari",gl_mATARI1,0)

/*-------------------------------------------------------------------
/ Middle Earth (02/1978)
/-------------------------------------------------------------------*/
INITGAME1(midearth, atari_disp1, FLIPSW1920, 1)
ATARI_2_ROMSTART(midearth,	"608.bin",	CRC(28b92faf) SHA1(8585770f4059049f1dcbc0c6ef5718b6ff1a5431),
							"609.bin",	CRC(589df745) SHA1(4bd3e4f177e8d86bab41f3a14c169b936eeb480a))
ATARI_ROMEND
CORE_GAMEDEFNV(midearth,"Middle Earth",1978,"Atari",gl_mATARI1A,0)

/*-------------------------------------------------------------------
/ Space Riders (09/1978)
/-------------------------------------------------------------------*/
INITGAME1(spcrider, atari_disp1, FLIPSW6667, 1)
ATARI_2_ROMSTART(spcrider,	"spacel.bin",	CRC(66ffb04e) SHA1(42d8b7fb7206b30478f631d0e947c0908dcf5419),
							"spacer.bin",	CRC(3cf1cd73) SHA1(c46044fb815b439f12fb3e21c470c8b93ebdfd55))
ATARI_ROMEND
CORE_GAMEDEFNV(spcrider,"Space Riders",1978,"Atari",gl_mATARI1,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Superman (03/1979)
/-------------------------------------------------------------------*/
INITGAME2(superman, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(superman,	"supmn_k.rom",	CRC(a28091c2) SHA1(9f5e47db408da96a31cb2f3be0fa9fb1e79f8d85),
							"atari_m.rom",	CRC(1bb6b72c) SHA1(dd24ed54de275aadf8dc0810a6af3ac97aea4026),
							"atari_j.rom",	CRC(26521779) SHA1(2cf1c66441aee99b9d01859d495c12025b5ef094))
ATARI_ROMEND
CORE_GAMEDEFNV(superman,"Superman",1979,"Atari",gl_mATARI2,0)

/*-------------------------------------------------------------------
/ Hercules (05/1979)
/-------------------------------------------------------------------*/
INITGAME2(hercules, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(hercules,	"herc_k.rom",	CRC(65e099b1) SHA1(83a06bc82e0f8f4c0655886c6a9962bb28d00c5e),
							"atari_m.rom",	CRC(1bb6b72c) SHA1(dd24ed54de275aadf8dc0810a6af3ac97aea4026),
							"atari_j.rom",	CRC(26521779) SHA1(2cf1c66441aee99b9d01859d495c12025b5ef094))
ATARI_ROMEND
CORE_GAMEDEFNV(hercules,"Hercules",1979,"Atari",gl_mATARI2,0)

/*-------------------------------------------------------------------
/ Road Runner (??/1979)
/-------------------------------------------------------------------*/
INITGAME3(roadrunr, atari_disp2, FLIPSW6667, 1)
ATARI_3_ROMSTART(roadrunr,	"0000.716",	CRC(62f5f394) SHA1(ff91066d43d788119e3337788abd86e5c0bf2d92),
							"3000.716",	CRC(2fc01359) SHA1(d3df20c764bb68a5316367bb18d34a03293e7fa6),
							"3800.716",	CRC(77262408) SHA1(3045a732c39c96002f495f64ed752279f7d43ee7))
ATARI_ROMEND
CORE_GAMEDEFNV(roadrunr,"Road Runner",1979,"Atari",gl_mATARI3,0)

//Monza (1980)
//Neutron Star (1981)
//4x4 (1983)
//Triangle (19??)
