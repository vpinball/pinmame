#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "byvidpin.h"

#define INITGAMEVP(name, disp, flip, snd, dualTMS) \
static core_tGameData name##GameData = {0,disp,{flip,0,0,0,snd,dualTMS}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
BYVP_INPUT_PORTS_START(name, 1) BYVP_INPUT_PORTS_END

/*-----------------------------------------------------
/ Baby Pacman (Video/Pinball Combo) (BY133-???:  10/82)
/-----------------------------------------------------*/
INITGAMEVP(babypac,byVP_dispBabyPac,FLIP_SWNO(0,1),SNDBRD_BY45BP,0)
BYVP_ROMSTARTx00(babypac, "891-u2.732", CRC(7f7242d1),
                          "891-u6.732", CRC(6136d636),
                          "891-u9.764", CRC(7fa570f3),
                          "891-u10.764",CRC(28f4df8b),
                          "891-u11.764",CRC(0a5967a4),
                          "891-u12.764",CRC(58cfe542),
                          "891-u29.764",CRC(0b57fd5d))
BYVP_ROMEND
CORE_GAMEDEFNVR90(babypac,"Baby Pacman (Video/Pinball Combo)",1982,"Bally",byVP_mVP1,0)

/*-----------------------------------------------------------------
/ Granny and the Gators (Video/Pinball Combo) - (BY35-???: 01/84)
/----------------------------------------------------------------*/
INITGAMEVP(granny,byVP_dispGranny,FLIP_SW(FLIP_L),SNDBRD_BY45BP,1)
BYVP_ROMSTART100(granny,"cpu_u2.532",CRC(d45bb956),
                        "cpu_u6.532",CRC(306aa673),
                        "vid_u4.764",CRC(3a3d4c6b),
                        "vid_u5.764",CRC(78bcb0fb),
                        "vid_u6.764",CRC(8d8220a6),
                        "vid_u7.764",CRC(aa71cf29),
                        "vid_u8.764",CRC(a442bc01),
                        "vid_u9.764",CRC(6b67a1f7),
                        "cs_u3.764", CRC(0a39a51d))
BYVP_ROMEND
CORE_GAMEDEFNV(granny,"Granny and the Gators (Video/Pinball Combo)",1984,"Bally",byVP_mVP2,0)
