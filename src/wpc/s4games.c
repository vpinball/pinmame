#include "driver.h"
#include "sim.h"
#include "s67s.h"
#include "s6.h"			//*System 4 Is Nearly Identical to 6, so we share functions*/
#include "s4.h"

#define INITGAME(name, balls) \
static core_tGameData name##GameData = { GEN_S4, s4_disp }; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S4_INPUT_PORTS_START(name, balls) S4_INPUT_PORTS_END


/*--------------------------------
/ Phoenix - Sys.4 (Game #485)
/-------------------------------*/
INITGAME(phnix,3/*?*/)
S6_ROMSTART(phnix,l1,"gamerom.716", 0x3aba6eac,
					 "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(phnix,l1,"Phoenix (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Flash - Sys.4 (Game #486)
/-------------------------------*/
INITGAME(flash,3/*?*/)
S6_ROMSTART(flash,l1,"gamerom.716", 0x287f12d6,
					 "green1.716", 0x2145f8ab,
					 "green2.716", 0x1c978a4a)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(flash,l1,"Flash (L-1)",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Tri Zone - Sys.4 (Game #487)
/-------------------------------*/
INITGAME(trizn,3/*?*/)
S6_ROMSTART(trizn,l1,"gamerom.716", 0x757091c5,
					 "green1.716", 0x2145f8ab,
					 "green2.716", 0x1c978a4a)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(trizn,l1,"Tri Zone (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Pokerino - Sys.4 (Game #488)
/-------------------------------*/
INITGAME(pkrno,3/*?*/)
S6_ROMSTART(pkrno,l1,"gamerom.716", 0x9b4d01a8,
					 "white1.716", 0x9bbbf14f,
					 "white2.716", 0x4d4010dd)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(pkrno,l1,"Pokerino (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Time Warp - Sys.4 (Game #489)
/-------------------------------*/
INITGAME(tmwrp,3/*?*/)
S6_ROMSTART(tmwrp,l2,"gamerom.716", 0xb168df09,
					 "green1.716", 0x2145f8ab,
					 "green2.716", 0x1c978a4a)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(tmwrp,l2,"Time Warp (L-2)",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Stellar Wars - Sys.4 (Game #490)
/-------------------------------*/
INITGAME(stlwr,3/*?*/)
S6_ROMSTART(stlwr,l2,"gamerom.716", 0x874e7ef7,
					 "yellow1.716", 0xd251738c,
					 "yellow2.716", 0x5049326d)
S67S_SOUNDROMS8("sound1.716",0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(stlwr,l2,"Stellar Wars (L-2)",1979,"Williams",s4_mS4S,0)



