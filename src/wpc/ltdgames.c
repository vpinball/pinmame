#include "driver.h"
#include "sim.h"
#include "ltd.h"

// Sistema 3 games below

#define INITGAME(name, disptype, balls, lamps, gs1, gs2) \
	LTD_INPUT_PORTS_START(name, balls) LTD_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{FLIP_SW(FLIP_L),0,lamps,0,0,0,gs1,gs2}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout disp2p5[] = {
  { 1, 0,16,4,CORE_SEG7 }, { 1, 8,15,1,CORE_SEG7 },
  { 1,16, 6,4,CORE_SEG7 }, { 1,24,14,1,CORE_SEG7 },
  { 0,12,11,1,CORE_SEG7 }, { 2,12,10,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Arizona
/-------------------------------------------------------------------*/
INITGAME(arizona, disp2p5, 1, 5, 0x3f, 0x0c)
LTD_1_ROMSTART(arizona, "arizltd.bin", CRC(908f00d8) SHA1(98f28f1aedbad43e0e096959fdef45e038405473))
LTD_ROMEND
CORE_GAMEDEFNV(arizona,"Arizona",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ Atlantis
/-------------------------------------------------------------------*/
static core_tLCDLayout dispAtla[] = {
  DISP_SEG_IMPORT(disp2p5),
  { 5, 0, 4,1,CORE_SEG7 }, { 5, 3, 5,1,CORE_SEG7 },
  {0}
};
INITGAME(atla_ltd, dispAtla, 1, 5, 0x0f, 0x0c)
LTD_2_ROMSTART(atla_ltd, "atlantis.bin", CRC(c61be043) SHA1(e6c4463f59a5743fa34aa55beeb6f536ad9f1b56))
LTD_ROMEND
CORE_GAMEDEFNV(atla_ltd,"Atlantis (LTD)",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ Disco Dancing
/-------------------------------------------------------------------*/
static core_tLCDLayout dispDisco[] = {
  DISP_SEG_IMPORT(disp2p5),
  { 5, 0, 5,1,CORE_SEG7 },
  {0}
};
INITGAME(discodan, dispDisco, 1, 5, 0x1f, 0x0c)
LTD_2_ROMSTART(discodan, "disco.bin", CRC(83c79157) SHA1(286fd0c984870639fcd7d7b8f6a5a5ddabcddcf5))
LTD_ROMEND
CORE_GAMEDEFNV(discodan,"Disco Dancing",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ Hustler
/-------------------------------------------------------------------*/
INITGAME(hustler, disp2p5, 1, 5, 0x3f, 0x0c)
LTD_2_ROMSTART(hustler, "hustler_1.bin", CRC(43f323f5) SHA1(086b81699bea08b10b4231e398f4f689395355b0))
LTD_ROMEND
CORE_GAMEDEFNV(hustler,"Hustler",19??,"LTD",gl_mLTD3,0)

/*
Gaston: I mapped the "Hustler" switches a while ago with exegeta:

2 – 100 or 500 pts
3 – 100 or 500 pts
4 – 100 or 500 pts
5 – 100 pts + solenoid 3
6 – 100 pts + solenoid 4
9 – 10 pts
10 – 100 or 500 pts
11 – 100 or 500 pts
12 – 300 pts
13 – 1,000 pts
14 – 100 or 500 pts
17 – 100 or 500 pts
18 – 10 pts + solenoid 5
19 – 10 pts + solenoid 6
33 – 300 pts (but only once)
34 – 300 pts (but only once, as long as switch #33 STAYS on!)
35 – see above, and so on
 
So the 100 + 500 pts targets are the 7 wire rollover switches, 13 is the outlane, and 33 and above are the drop targets which need to remain closed when down (logical).
 
When the correct amount of drop targets is down, a solenoid (8, 9, 10, I don’t know exactly) will fire to push them up and release the switches!
*/

/*-------------------------------------------------------------------
/ Martian Queen
/-------------------------------------------------------------------*/
static core_tLCDLayout dispMarqn[] = {
  DISP_SEG_IMPORT(disp2p5),
  { 5, 0,13,1,CORE_SEG7 },
  {0}
};
INITGAME(marqueen, dispMarqn, 1, 5, 0x3f, 0x04)
LTD_2_ROMSTART(marqueen, "mqueen.bin", CRC(cb664001) SHA1(00152f89e58bc11567a8de32ccaaa47146dace0d) BAD_DUMP) // patched from bad dump
LTD_ROMEND
CORE_GAMEDEFNV(marqueen,"Martian Queen",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ King Kong
/-------------------------------------------------------------------*/
INITGAME(kkongltd, dispDisco, 1, 5, 0x1f, 0x0c)
LTD_2_ROMSTART(kkongltd, "kong.bin", CRC(5b2a3123) SHA1(eee417d17d3272ee63c728915af84da33f1f73a2))
LTD_ROMEND
CORE_GAMEDEFNV(kkongltd,"King Kong (LTD)",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ Viking King
/-------------------------------------------------------------------*/
INITGAME(vikngkng, disp2p5, 1, 5, 0x3f, 0x0c)
LTD_2_ROMSTART(vikngkng, "vikking.bin", CRC(aa32d158) SHA1(b24294ae4ecb2ab3119ad7fe79ef567b19ac792a))
LTD_ROMEND
CORE_GAMEDEFNV(vikngkng,"Viking King",19??,"LTD",gl_mLTD3,0)

/*-------------------------------------------------------------------
/ Force
/-------------------------------------------------------------------*/
static core_tLCDLayout dispForce[] = {
  { 1, 0,16,4,CORE_SEG7 }, { 1, 8,15,1,CORE_SEG7 },
  { 1,16, 6,4,CORE_SEG7 }, { 1,24, 5,1,CORE_SEG7 },
  { 0,12,11,1,CORE_SEG7 }, { 2,12,10,1,CORE_SEG7 },
  { 4, 0, 3,1,CORE_SEG7S },{ 4, 3, 4,1,CORE_SEG7S },{ 4, 6, 2,1,CORE_SEG7S },
  { 6, 0,12,2,CORE_SEG7S },{ 6, 5,14,1,CORE_SEG7S },
  {0}
};
INITGAME(force, dispForce, 1, 1, 0x03, 0)
LTD_2_ROMSTART(force, "forceltd.bin", CRC(48f9ebbe) SHA1(8aaab352fb21263b1b93ffefd9b5169284083beb))
LTD_ROMEND
CORE_GAMEDEFNV(force,"Force",198?,"LTD",gl_mLTD3,GAME_NO_SOUND)

static core_tLCDLayout disp2p6[] = {
  { 1, 0,14,6,CORE_SEG7 },
  { 1,18, 4,6,CORE_SEG7 },
  { 0,14,11,1,CORE_SEG7 }, { 2,14,10,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Space Poker
/-------------------------------------------------------------------*/
INITGAME(spcpoker, disp2p6, 1, 9, 0x0f, 0x0c)
LTD_2_ROMSTART(spcpoker, "spoker.bin", CRC(98918b19) SHA1(b1bb3f408dae9fc77d396894c3c4ef08bce8c345) BAD_DUMP) // patched from bad dump
LTD_ROMEND
CORE_GAMEDEFNV(spcpoker,"Space Poker",198?,"LTD",gl_mLTD3,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Black Hole
/-------------------------------------------------------------------*/
INITGAME(bhol_ltd, disp2p6, 1, 4, 0x0f, 0x0c)
LTD_2_ROMSTART(bhol_ltd, "blackhol.bin", CRC(9f6ae35e) SHA1(c17bf08a41c6cf93550671b0724c58e8ac302c33))
LTD_ROMEND
CORE_GAMEDEFNV(bhol_ltd,"Black Hole (LTD)",198?,"LTD",gl_mLTD3,GAME_NO_SOUND)

static core_tLCDLayout disp3p6[] = {
  { 0, 0,14,6,CORE_SEG7 },
  { 3, 0, 8,6,CORE_SEG7 },
  { 6, 0, 2,6,CORE_SEG7 },
  { 9, 2, 1,1,CORE_SEG7 }, { 9, 8, 0,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Cowboy Eight Ball
/-------------------------------------------------------------------*/
INITGAME(cowboy3p, disp3p6, 1, 0, 0, 0)
LTD_4_ROMSTART(cowboy3p, "cowboy3p.bin", CRC(5afa29af) SHA1(a5ccf5cd17c63d4292222b792535187b1bcfa786))
LTD_ROMEND
CORE_GAMEDEFNV(cowboy3p,"Cowboy Eight Ball",1981,"LTD",gl_mLTD3A,0)

INITGAME(cowboy3a, disp3p6, 1, 0, 0, 0)
LTD_4_ROMSTART(cowboy3a, "cowboy3a.bin", CRC(48278d77) SHA1(4102a2be10b48d0edb2b636e10696cbb2cd3a4c4))
LTD_ROMEND
CORE_CLONEDEFNV(cowboy3a,cowboy3p,"Cowboy Eight Ball (alternate set)",1981,"LTD",gl_mLTD3A,0)

/*-------------------------------------------------------------------
/ Zephy
/-------------------------------------------------------------------*/
INITGAME(zephy, disp3p6, 2, 0, 0, 0)
LTD_4_ROMSTART(zephy, "zephy.l2", CRC(8dd11287) SHA1(8133d0c797eb0fdb56d83fc55da91bfc3cddc9e3))
LTD_ROMEND
CORE_GAMEDEFNV(zephy,"Zephy",1982,"LTD",gl_mLTD3A,0)

INITGAME(zephya, disp3p6, 2, 0, 0, 0)
LTD_4_ROMSTART(zephya, "zephy1.bin", CRC(ae189c8a) SHA1(c309b436ef94cd5c266c88fe5f222261e083e4eb))
LTD_ROMEND
CORE_CLONEDEFNV(zephya,zephy,"Zephy (alternate set)",1982,"LTD",gl_mLTD3A,0)

// Sistema 4 games below

#define INITGAME4(name, disptype, balls) \
	LTD4_INPUT_PORTS_START(name, balls) LTD_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,disptype,{0x03000102,0,8}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

static core_tLCDLayout disp4p6[] = {
  { 0, 0,25,6,CORE_SEG7 },
  { 3, 0,17,6,CORE_SEG7 },
  { 6, 0, 9,6,CORE_SEG7 },
  { 9, 0, 1,6,CORE_SEG7 },
  {12, 2,15,1,CORE_SEG7 }, {12, 8, 0,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Cowboy Eight Ball 2
/-------------------------------------------------------------------*/
INITGAME4(cowboy, disp4p6, 1)
LTD_44_ROMSTART(cowboy, "cowboy_l.bin", CRC(87befe2a) SHA1(93fdf40b10e53d7d95e5dc72923b6be887411fc0),
                        "cowboy_h.bin", CRC(105e5d7b) SHA1(75edeab8c8ba19f334479133802acbc25f405763))
LTD_ROMEND
CORE_GAMEDEFNV(cowboy,"Cowboy Eight Ball 2",198?,"LTD",gl_mLTD4,0)

/*-------------------------------------------------------------------
/ Haunted Hotel
/-------------------------------------------------------------------*/
static core_tLCDLayout dispHH[] = {
  DISP_SEG_IMPORT(disp4p6),
  {15, 0, 7,2,CORE_SEG7 }, {15, 4,31,1,CORE_SEG7 }, {15, 6,16,1,CORE_SEG7 }, {15, 8,23,2,CORE_SEG7 },
  {0}
};
INITGAME4(hhotel, dispHH, 3)
LTD_44_ROMSTART(hhotel, "hh1.bin", CRC(a107a683) SHA1(5bb79d9a0a6b33f067cdd54942784c67ab557909),
                        "hh2.bin", CRC(e0c2ebc1) SHA1(131240589162c7b3f44a2bb951945c7d64f89c8d))
LTD_ROMEND
CORE_GAMEDEFNV(hhotel,"Haunted Hotel",198?,"LTD",gl_mLTD4HH,0)

/*-------------------------------------------------------------------
/ Mr. & Mrs. Pec-Men
/-------------------------------------------------------------------*/
static core_tLCDLayout dispPec[] = {
  DISP_SEG_IMPORT(disp4p6),
  {15, 0, 7,2,CORE_SEG7 }, {15, 5,31,1,CORE_SEG7 }, {15, 7,16,1,CORE_SEG7 }, {15,10,23,2,CORE_SEG7 },
  {0}
};
INITGAME4(pecmen, dispPec, 1)
LTD_44_ROMSTART(pecmen, "pecmen_l.bin", CRC(f86c724e) SHA1(635ec94a1c6e77800ef9774102cc639be86c4261),
                        "pecmen_h.bin", CRC(013abca0) SHA1(269376af92368d214c3d09ec6d3eb653841666f3))
LTD_ROMEND
CORE_GAMEDEFNV(pecmen,"Mr. & Mrs. Pec-Men",198?,"LTD",gl_mLTD4,0)

static core_tLCDLayout disp4p7[] = {
  { 0, 0,24,7,CORE_SEG7 },
  { 3, 0,16,7,CORE_SEG7 },
  { 6, 0, 8,7,CORE_SEG7 },
  { 9, 0, 0,7,CORE_SEG7 },
  {12, 2,15,1,CORE_SEG7 }, {12,10, 7,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ Al Capone
/-------------------------------------------------------------------*/
INITGAME4(alcapone, disp4p7, 1)
LTD_44_ROMSTART(alcapone, "alcapo_l.bin", CRC(c4270ba8) SHA1(f3d80af9900c94df2d43f2755341a346a0b64c87),
                          "alcapo_h.bin", CRC(279f766d) SHA1(453c58e44c4ef8f1f9eb752b6163c61ebed70b27))
LTD_ROMEND
CORE_GAMEDEFNV(alcapone,"Al Capone",198?,"LTD",gl_mLTD4,0)

/*-------------------------------------------------------------------
/ Alien Warrior
/-------------------------------------------------------------------*/
INITGAME4(alienwar, disp4p7, 1)
LTD_44_ROMSTART(alienwar, "alwar_l.bin", NO_DUMP, // existing dump was falsely trimmed to 2K
                          "alwar_h.bin", NO_DUMP) // existing dump was falsely trimmed to 2K
LTD_ROMEND
CORE_GAMEDEFNV(alienwar,"Alien Warrior",198?,"LTD",gl_mLTD4,0)

/*-------------------------------------------------------------------
/ Columbia
/-------------------------------------------------------------------*/
INITGAME4(columbia, disp4p7, 3)
LTD_44_ROMSTART(columbia, "columb_l.bin", CRC(ac345dee) SHA1(14f03fa8059de5cd69cc83638aa6533fbcead37e),
                          "columb_h.bin", CRC(acd2a85b) SHA1(30889ee4230ce05f6060f926b2137bbf5939db2d))
LTD_ROMEND
CORE_GAMEDEFNV(columbia,"Columbia",198?,"LTD",gl_mLTD4,0)

/*-------------------------------------------------------------------
/ Time Machine
/-------------------------------------------------------------------*/
INITGAME4(tmacltd4, disp4p7, 1)
LTD_44_ROMSTART(tmacltd4, "tm4_l.bin", CRC(69691662) SHA1(3d86314967075e3f5b168c8d7bf6b26bbbb957bd),
                          "tm4_h.bin", CRC(f5f97992) SHA1(ba31f71a600e7061b500e0750f50643503e52a80))
LTD_ROMEND
CORE_GAMEDEFNV(tmacltd4,"Time Machine (LTD) (4 Players)",198?,"LTD",gl_mLTD4,0)

INITGAME4(tmacltd2, disp4p7, 1)
LTD_44_ROMSTART(tmacltd2, "tm2_l.bin", NO_DUMP, // existing dump has LSB set to high for all bytes
                          "tm2_h.bin", CRC(f717c9db) SHA1(9ca5819b707fa20edfc289734e1aa189ae242aa3))
LTD_ROMEND
CORE_CLONEDEFNV(tmacltd2,tmacltd4,"Time Machine (LTD) (2 Players)",198?,"LTD",gl_mLTD4,0)

/*-------------------------------------------------------------------
/ Trick Shooter
/-------------------------------------------------------------------*/
INITGAME4(tricksht, disp4p7, 1)
LTD_44_ROMSTART(tricksht, "tricks_l.bin", CRC(951413ff) SHA1(f4a28f7b41cb077377433dc7bfb6647e5d392481),
                          "tricks_h.bin", CRC(2e4efb51) SHA1(3dd20addecf4b47bd68b05d557c378d1dbbbd892))
LTD_ROMEND
CORE_GAMEDEFNV(tricksht,"Trick Shooter",198?,"LTD",gl_mLTD4,0)
