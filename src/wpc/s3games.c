#include "driver.h"
#include "sim.h"
#include "s67s.h"
#include "s6.h"
#include "s4.h"			//*System 3 Is Nearly Identical to 4, so we share functions*/

#define s3_mS3	s4_mS4	//                        ""
#define s3_mS3S	s4_mS4S	//                        ""


#define INITGAME(name, balls) \
	static core_tGameData name##GameData = { GEN_S3, s4_disp }; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S3_INPUT_PORTS_START(name, balls) S3_INPUT_PORTS_END

/*----------------------------
/ Hot Tip - Sys.3 (Game #477) - No Sound board?
/----------------------------*/
INITGAME(httip,0/*?*/)
S6_ROMSTART(httip,l1,"gamerom.716", 0xb1d4fd9b,
					 "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S6_ROMEND
CORE_GAMEDEF(httip,l1,"Hot Tip (L-1)",1977,"Williams",s3_mS3,GAME_NO_SOUND)

/*---------------------------------
/ Lucky Seven - Sys.3 (Game #480) - No Sound board?
/---------------------------------*/
INITGAME(lucky,0/*?*/)
S6_ROMSTART(lucky,l1,"gamerom.716", 0x7cfbd4c7,
					 "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S6_ROMEND
CORE_GAMEDEF(lucky,l1,"Lucky Seven (L-1)",1977,"Williams",s3_mS3,GAME_NO_SOUND)

/*-------------------------------------
/ World Cup Soccer - Sys.3 (Game #481)
/-------------------------------------*/
INITGAME(wldcp,0/*?*/)
S6_ROMSTART(wldcp,l1,"gamerom.716", 0xc8071956,
			         "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(wldcp,l1,"World Cup Soccer (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Contact - Sys.3 (Game #482)
/-------------------------------------*/
INITGAME(cntct,0/*?*/)
S6_ROMSTART(cntct,l1,"gamerom.716", 0x35359b60,
			         "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(cntct,l1,"Contact (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Disco Fever - Sys.3 (Game #483)
/-------------------------------------*/
INITGAME(disco,0/*?*/)
S6_ROMSTART(disco,l1,"gamerom.716", 0x831d8adb,
			         "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(disco,l1,"Disco Fever (L-1)",1978,"Williams",s3_mS3S,0)
