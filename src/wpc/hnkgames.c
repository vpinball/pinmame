#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "hnks.h"
#include "by35.h"

//Display: 5 X 9 Segment, 6 Digit Displays. Backglass covers the ball/credit display.
/*
(02)(03)(04)(05)(06)(07)xx(10)(11)(12)(13)(14)(15)xx
(18)(19)(20)(21)(22)(23)xx(26)(27)(28)(29)(30)(31)xx
(34)(35)(36)(37)(39)(40)
*/
#if 1
static const core_tLCDLayout dispHNK[] = {
  {0, 0, 2,6,CORE_SEG9}, {0,16,10,6,CORE_SEG9},
  {2, 0,18,6,CORE_SEG9}, {2,16,26,6,CORE_SEG9},
  {4, 4,35,2,CORE_SEG9}, {4,10,38,2,CORE_SEG9},{0}
};
#else
static core_tLCDLayout dispHNK[] = {
  {0, 0, 0,6,CORE_SEG9}, {0,14,8,6,CORE_SEG9},
  {2, 0,16,6,CORE_SEG9}, {2,14,24,6,CORE_SEG9},
  {4, 4,33,2,CORE_SEG9}, {4,10,36,2,CORE_SEG9},{0}
};
#endif
HNK_INPUT_PORTS_START(hnk, 1) HNK_INPUT_PORTS_END

#define INITGAME(name, gen, disp, flip, lamps, sb) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,0}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
}

//Production Order and Date of Manufacture not yet known for these games
/*--------------------------------
/ FJ Holden
/-------------------------------*/
INITGAME(fjholden,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(fjholden,  "fj_ic2.mpu",CRC(b47bc2c7),
                        "fj_ic3.mpu",CRC(ceaeb7d3))
HNK_SOUNDROMS("fj_ic14.snd", CRC(34fe3587),
              "fj_ic3.snd",  CRC(09d3f020))
HNK_ROMEND
#define input_ports_fjholden input_ports_hnk
CORE_GAMEDEFNV(fjholden,"FJ Holden",1978,"Hankin",by35_mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Orbit 1
/-------------------------------*/
INITGAME(orbit1,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(orbit1,    "o1_ic2.mpu",CRC(b47bc2c7),
                        "o1_ic3.mpu",CRC(fe7b61be))
HNK_SOUNDROMS("o1_ic14.snd", CRC(323bfbd5),
              "o1_ic3.snd",  CRC(dfc57606))
HNK_ROMEND
#define input_ports_orbit1 input_ports_hnk
CORE_GAMEDEFNV(orbit1,"Orbit 1",1978,"Hankin",by35_mHNK,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Howzat
/-------------------------------*/
INITGAME(howzat,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(howzat,    "hz_ic2.mpu",CRC(b47bc2c7),
                        "hz_ic3.mpu",CRC(d13df4bc))
HNK_SOUNDROMS("hz_ic14.snd", CRC(0e3fdb59),
              "hz_ic3.snd",  CRC(dfc57606))
HNK_ROMEND
#define input_ports_howzat input_ports_hnk
CORE_GAMEDEFNV(howzat,"Howzat",1980,"Hankin",by35_mHNK,0)

/*--------------------------------
/ Shark
/-------------------------------*/
INITGAME(shark,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(shark,     "shk_ic2.mpu",CRC(b47bc2c7),
                        "shk_ic3.mpu",CRC(c3ef936c))
HNK_SOUNDROMS("shk_ic14.snd", CRC(8f8b0e48),
              "shk_ic3.snd",  CRC(dfc57606))
HNK_ROMEND
#define input_ports_shark input_ports_hnk
CORE_GAMEDEFNV(shark,"Shark",1980,"Hankin",by35_mHNK,0)

/*--------------------------------
/ The Empire Strike Back
/-------------------------------*/
INITGAME(empsback,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(empsback,  "sw_ic2.mpu",CRC(b47bc2c7),
                        "sw_ic3.mpu",CRC(837ffe32))
HNK_SOUNDROMS("sw_ic14.snd", CRC(c1eeb53b),
              "sw_ic3.snd",  CRC(db214f65))
HNK_ROMEND
#define input_ports_empsback input_ports_hnk
CORE_GAMEDEFNV(empsback,"The Empire Strike Back",1981,"Hankin",by35_mHNK,0)

