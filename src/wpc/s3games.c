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
S4_ROMSTART(httip,l1,"gamerom.716",CRC(b1d4fd9b) SHA1(e55ecf1328a55979c4cf8f3fb4e6761747e0abc4),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S4_ROMEND
#define input_ports_httip input_ports_s3
CORE_GAMEDEF(httip,l1,"Hot Tip (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*---------------------------------
/ Lucky Seven - Sys.3 (Game #480) - No Sound board
/---------------------------------*/
INITGAMEFULL(lucky, GEN_S3C, s4_disp,0,0,34,33,36,24,0,0)
S4_ROMSTART(lucky,l1,"gamerom.716",CRC(7cfbd4c7) SHA1(825e2245fd1615e932973f5e2b5ed5f2da9309e7),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S4_ROMEND
#define input_ports_lucky input_ports_s3
CORE_GAMEDEF(lucky,l1,"Lucky Seven (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*-------------------------------------
/ World Cup Soccer - Sys.3 (Game #481)
/-------------------------------------*/
INITGAMEFULL(wldcp, GEN_S3,s4_disp,0,0,38,37,0,0,0,0)
S4_ROMSTART(wldcp,l1,"gamerom.716", CRC(c8071956) SHA1(0452aaf2ec1bcc5717fe52a6c541d79402bebb17),
                     "white1.716",  CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2wc.716",CRC(618d15b5) SHA1(527387893eeb2cd4aa563a4cfb1948a15d2ed741))
S67S_SOUNDROMS8("sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_wldcp input_ports_s3
CORE_GAMEDEF(wldcp,l1,"World Cup Soccer (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Contact - Sys.3 (Game #482)
/-------------------------------------*/
INITGAMEFULL(cntct, GEN_S3, s4_disp,0,0,33,34,35,0,0,0)
S4_ROMSTART(cntct,l1,"gamerom.716",CRC(35359b60) SHA1(ab4c3328d93bdb4c952090b327c91b0ded36152c),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8("sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_cntct input_ports_s3
CORE_GAMEDEF(cntct,l1,"Contact (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Disco Fever - Sys.3 (Game #483)
/-------------------------------------*/
INITGAMEFULL(disco, GEN_S3,s4_disp,0,0,33,17,23,21,0,0)
S4_ROMSTART(disco,l1,"gamerom.716", CRC(831d8adb) SHA1(99a9c3d5c8cbcdf3bb9c210ad9d05c34905b272e),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8("sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_disco input_ports_s3
CORE_GAMEDEF(disco,l1,"Disco Fever (L-1)",1978,"Williams",s3_mS3S,0)
