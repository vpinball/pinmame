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
HNK_ROMSTART(fjholden,  "fj_ic2.mpu",CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052),
                        "fj_ic3.mpu",CRC(ceaeb7d3) SHA1(9e479b985f8500983e71d6ff33ee94160e99650d))
HNK_SOUNDROMS("fj_ic14.snd", CRC(34fe3587) SHA1(132714675a23c101ceb5a4d544818650ae5ccea2),
              "fj_ic3.snd",  CRC(09d3f020) SHA1(274be0b94d341ee43357011691da82e83a7c4a00))
HNK_ROMEND
#define input_ports_fjholden input_ports_hnk
CORE_GAMEDEFNV(fjholden,"FJ Holden",1978,"Hankin",by35_mHNK,0)

/*--------------------------------
/ Orbit 1
/-------------------------------*/
INITGAME(orbit1,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(orbit1,    "o1_ic2.mpu",CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052),
                        "o1_ic3.mpu",CRC(fe7b61be) SHA1(c086b0433bb9ab3f2139c705d4372beb1656b27f))
HNK_SOUNDROMS("o1_ic14.snd", CRC(323bfbd5) SHA1(2e89aa4fcd33f9bfeea5c310ffb0a5be45fb70a9),
              "o1_ic3.snd",  CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
HNK_ROMEND
#define input_ports_orbit1 input_ports_hnk
CORE_GAMEDEFNV(orbit1,"Orbit 1",1978,"Hankin",by35_mHNK,0)

/*--------------------------------
/ Howzat
/-------------------------------*/
INITGAME(howzat,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(howzat,    "hz_ic2.mpu",CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052),
                        "hz_ic3.mpu",CRC(d13df4bc) SHA1(27a70260698d3eaa7cf7a56edc5dd9a4af3f4103))
HNK_SOUNDROMS("hz_ic14.snd", CRC(0e3fdb59) SHA1(cae3c85b2c32a0889785f770ece66b959bcf21e1),
              "hz_ic3.snd",  CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
HNK_ROMEND
#define input_ports_howzat input_ports_hnk
CORE_GAMEDEFNV(howzat,"Howzat",1980,"Hankin",by35_mHNK,0)

/*--------------------------------
/ Shark
/-------------------------------*/
INITGAME(shark,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(shark,     "shk_ic2.mpu",CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052),
                        "shk_ic3.mpu",CRC(c3ef936c) SHA1(14668496d162a77e03c1142bef2956d5b76afc99))
HNK_SOUNDROMS("shk_ic14.snd", CRC(8f8b0e48) SHA1(72d94aa9b32c603b1ca681b0ab3bf8ddbf5c9afe),
              "shk_ic3.snd",  CRC(dfc57606) SHA1(638853c8e46bf461f2ecde02b8b2aa68c2d414b8))
HNK_ROMEND
#define input_ports_shark input_ports_hnk
CORE_GAMEDEFNV(shark,"Shark",1980,"Hankin",by35_mHNK,0)

/*--------------------------------
/ The Empire Strike Back
/-------------------------------*/
INITGAME(empsback,GEN_HNK,dispHNK,FLIP_SW(FLIP_L),0,SNDBRD_HANKIN)
HNK_ROMSTART(empsback,  "sw_ic2.mpu",CRC(b47bc2c7) SHA1(42c985d83a9454fcd08b87e572e5563ebea0d052),
                        "sw_ic3.mpu",CRC(837ffe32) SHA1(9affc5d9345ce15394553d3204e5234cc6348d2e))
HNK_SOUNDROMS("sw_ic14.snd", CRC(c1eeb53b) SHA1(7a800dd0a8ae392e14639e1819198d4215cc2251),
              "sw_ic3.snd",  CRC(db214f65) SHA1(1a499cf2059a5c0d860d5a4251a89a5735937ef8))
HNK_ROMEND
#define input_ports_empsback input_ports_hnk
CORE_GAMEDEFNV(empsback,"The Empire Strike Back",1981,"Hankin",by35_mHNK,0)

