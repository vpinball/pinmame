#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s4.h"     //* Bowler Hardware Is Nearly Identical to Sys4, so we share functions*/

#define s3_mS3  s4_mS4  //                        ""
#define s3_mS3S s4_mS4S //                        ""

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = { gen, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, gen, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { gen, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S3_INPUT_PORTS_START(bowl, 1) S3_INPUT_PORTS_END

static const core_tLCDLayout bowl_disp[] = {
  { 0, 0, 0, 4,CORE_SEG7 }, { 0,10, 4, 4,CORE_SEG7 }, { 0,20, 8, 4,CORE_SEG7 },
  { 2, 0,20, 4,CORE_SEG7 }, { 2,10,24, 4,CORE_SEG7 }, { 2,20,28, 4,CORE_SEG7 },
  { 4, 8,12, 2,CORE_SEG7 }, { 4,14,14, 2,CORE_SEG7 }, {0}
};

/*----------------------------
/ Taurus - No Sound board?
/----------------------------*/
INITGAME(taurs, GEN_S3C, bowl_disp)
S4_ROMSTART(taurs,l1,"taurus14.716",0x3246e285,
                     "taurus20.716",0xc6f8e3b1,
                     "white2.716",  0xcfc2518a)
S4_ROMEND
#define input_ports_taurs input_ports_bowl
CORE_GAMEDEF(taurs,l1,"Taurus Shuffle Bowler (L-1)",1980,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*----------------------------
/ Triple Strike - No Sound board?
/----------------------------*/
INITGAME(tstrk, GEN_S3C, bowl_disp)
S4_ROMSTART(tstrk,l1,"gamerom.716",0xb034c059,
                     "white1.716", 0xf163fc88,
                     "white2.716", 0xcfc2518a)
S4_ROMEND
#define input_ports_tstrk input_ports_bowl
CORE_GAMEDEF(tstrk,l1,"Triple Strike Bowler (L-1)",1984,"Williams",s3_mS3C,GAME_USES_CHIMES)
