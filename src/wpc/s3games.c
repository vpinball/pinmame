#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s4.h"                 //*System 3 Is Nearly Identical to 4, so we share functions*/

#define s3_mS3  s4_mS4  //                        ""
#define s3_mS3S s4_mS4S //                        ""

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = { gen, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, gen, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { gen, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S3_INPUT_PORTS_START(s3, 1) S3_INPUT_PORTS_END

/*----------------------------
/ Hot Tip - Sys.3 (Game #477) - No Sound board
/----------------------------*/
INITGAMEFULL(httip, GEN_S3C, s4_disp, 0,0,31,13,35,0,0,0)
S4_ROMSTART(httip,l1,"gamerom.716",0xb1d4fd9b,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S4_ROMEND
#define input_ports_httip input_ports_s3
CORE_GAMEDEF(httip,l1,"Hot Tip (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*---------------------------------
/ Lucky Seven - Sys.3 (Game #480) - No Sound board
/---------------------------------*/
INITGAMEFULL(lucky, GEN_S3C, s4_disp,0,0,34,33,36,24,0,0)
S4_ROMSTART(lucky,l1,"gamerom.716",0x7cfbd4c7,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S4_ROMEND
#define input_ports_lucky input_ports_s3
CORE_GAMEDEF(lucky,l1,"Lucky Seven (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*-------------------------------------
/ World Cup Soccer - Sys.3 (Game #481)
/-------------------------------------*/
INITGAME(wldcp, GEN_S3,s4_disp)
S4_ROMSTART(wldcp,l1,"gamerom.716", 0xc8071956,
                     "white1.716",  0x9bbbf14f,
                     "white2wc.716",0x618d15b5)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S4_ROMEND
#define input_ports_wldcp input_ports_s3
CORE_GAMEDEF(wldcp,l1,"World Cup Soccer (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Contact - Sys.3 (Game #482)
/-------------------------------------*/
INITGAME(cntct, GEN_S3,s4_disp)
S4_ROMSTART(cntct,l1,"gamerom.716",0x35359b60,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S4_ROMEND
#define input_ports_cntct input_ports_s3
CORE_GAMEDEF(cntct,l1,"Contact (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Disco Fever - Sys.3 (Game #483)
/-------------------------------------*/
INITGAMEFULL(disco, GEN_S3,s4_disp,0,0,33,17,23,21,0,0)
S4_ROMSTART(disco,l1,"gamerom.716", 0x831d8adb,
                     "white1.716", 0x9bbbf14f,
                     "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S4_ROMEND
#define input_ports_disco input_ports_s3
CORE_GAMEDEF(disco,l1,"Disco Fever (L-1)",1978,"Williams",s3_mS3S,0)
