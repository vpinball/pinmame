#include "driver.h"
#include "sndbrd.h"
#include "sim.h"
#include "hnk.h"
#include "hnks.h"

//Display: 5 X 9 Segment, 6 Digit Displays. Backglass covers the ball/credit display.
/*
(02)(03)(04)(05)(06)(07)xx(10)(11)(12)(13)(14)(15)xx
(18)(19)(20)(21)(22)(23)xx(26)(27)(28)(29)(30)(31)xx
(34)(35)(36)(37)(39)(40)
*/
static core_tLCDLayout dispHNK[] = {
  {0, 0, 2,6,CORE_SEG9}, {0,14,10,6,CORE_SEG9},
  {2, 0,18,6,CORE_SEG9}, {2,14,26,6,CORE_SEG9},
  {4, 4,34,2,CORE_SEG9}, {4,10,36,2,CORE_SEG9},{0}
};

#define INITGAME(name, gen, disp, flip, lamps, sb) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,0}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
HNK_INPUT_PORTS_START(name, 1) HNK_INPUT_PORTS_END

//Production Order and Date of Manufacture not yet known for these games


/*--------------------------------
/ FJ Holden
/-------------------------------*/
INITGAME(fjholden,0,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(fjholden,	"fj_ic2.mpu",0xb47bc2c7,
						"fj_ic3.mpu",0xceaeb7d3)
HNK_SOUNDROMS("fj_ic14.snd", 0x34fe3587,
              "fj_ic3.snd",  0x09d3f020)
HNK_ROMEND
CORE_GAMEDEFNV(fjholden,"FJ Holden",1978,"Hankin",mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Orbit 1
/-------------------------------*/
INITGAME(orbit1,0,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(orbit1,	"o1_ic2.mpu",0xb47bc2c7,
						"o1_ic3.mpu",0xfe7b61be)
HNK_SOUNDROMS("o1_ic14.snd", 0x323bfbd5,
              "o1_ic3.snd",  0xdfc57606)
HNK_ROMEND
CORE_GAMEDEFNV(orbit1,"Orbit 1",1978,"Hankin",mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Howzat
/-------------------------------*/
INITGAME(howzat,0,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(howzat,	"hz_ic2.mpu",0xb47bc2c7,
						"hz_ic3.mpu",0xd13df4bc)
HNK_SOUNDROMS("hz_ic14.snd", 0x0e3fdb59,
              "hz_ic3.snd",  0xdfc57606)
HNK_ROMEND
CORE_GAMEDEFNV(howzat,"Howzat",1980,"Hankin",mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Shark
/-------------------------------*/
INITGAME(shark,0,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(shark,		"shk_ic2.mpu",0xb47bc2c7,
						"shk_ic3.mpu",0xc3ef936c)
HNK_SOUNDROMS("shk_ic14.snd", 0x8f8b0e48,
              "shk_ic3.snd",  0xdfc57606)
HNK_ROMEND
CORE_GAMEDEFNV(shark,"Shark",1980,"Hankin",mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ The Empire Strike Back
/-------------------------------*/
INITGAME(empsback,0,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(empsback,	"sw_ic2.mpu",0xb47bc2c7,
						"sw_ic3.mpu",0x837ffe32)
HNK_SOUNDROMS("sw_ic14.snd", 0xc1eeb53b,
              "sw_ic3.snd",  0xdfc57606)
HNK_ROMEND
CORE_GAMEDEFNV(empsback,"The Empire Strike Back",1981,"Hankin",mHNK,GAME_IMPERFECT_SOUND)

