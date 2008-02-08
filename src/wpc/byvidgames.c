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
/ Baby Pacman (Video/Pinball Combo) (BY133-891:  10/82)
/-----------------------------------------------------*/
INITGAMEVP(babypac,byVP_dispBabyPac,FLIP_SWNO(0,1),SNDBRD_BY45BP,0)
BYVP_ROMSTARTx00(babypac, "891-u2.732", CRC(7f7242d1) SHA1(213a697bb7fc69f93ea04621f0fcfdd796f35196),
                          "891-u6.732", CRC(6136d636) SHA1(c01a0a2fcad3bdabd649128e012ab558b1c90cd3),
                          "891-u9.764", CRC(7fa570f3) SHA1(423ad9266b1ded00fa52ce4180d518874142a203),
                          "891-u10.764",CRC(28f4df8b) SHA1(bd6a3598c2c90b5a3a59327616d2f5b9940d98bc),
                          "891-u11.764",CRC(0a5967a4) SHA1(26d56ddea3f39d41e382449007bf7ba113c0285f),
                          "891-u12.764",CRC(58cfe542) SHA1(e024d14019866bd460d1da6b901f9b786a76a181),
                          "891-u29.764",CRC(0b57fd5d) SHA1(43a03e6d16c87c3305adb04722484f992f23a1bd))
BYVP_ROMEND
CORE_GAMEDEFNVR90(babypac,"Baby Pacman (Video/Pinball Combo)",1982,"Bally",byVP_mVP1,0)

BYVP_ROMSTARTx00(babypacn,"pacmann.u2", CRC(734710cb) SHA1(040943be6e1f3a750f3b2c705833699ec5845a80),
                          "891-u6.732", CRC(6136d636) SHA1(c01a0a2fcad3bdabd649128e012ab558b1c90cd3),
                          "babyvidn.u9",CRC(e937eb02) SHA1(391c4e2c2cd22bb06e2b4a4cef1ddc08040b8c43),
                          "891-u10.764",CRC(28f4df8b) SHA1(bd6a3598c2c90b5a3a59327616d2f5b9940d98bc),
                          "891-u11.764",CRC(0a5967a4) SHA1(26d56ddea3f39d41e382449007bf7ba113c0285f),
                          "891-u12.764",CRC(58cfe542) SHA1(e024d14019866bd460d1da6b901f9b786a76a181),
                          "891-u29.764",CRC(0b57fd5d) SHA1(43a03e6d16c87c3305adb04722484f992f23a1bd))
BYVP_ROMEND
#define init_babypacn init_babypac
#define input_ports_babypacn input_ports_babypac
GAMEX(2008,babypacn,babypac,byVP1,babypac,babypac,ROT90,"Bally / Oliver","Baby Pacman (Video/Pinball Combo, home roms)",0)

/*-----------------------------------------------------------------
/ Granny and the Gators (Video/Pinball Combo) - (BY35-???: 01/84)
/----------------------------------------------------------------*/
INITGAMEVP(granny,byVP_dispGranny,FLIP_SW(FLIP_L),SNDBRD_BY45BP,1)
BYVP_ROMSTART100(granny,"cpu_u2.532",CRC(d45bb956) SHA1(86a6942ff9fe38fa109ecde40dc2dd19adf938a9),
                        "cpu_u6.532",CRC(306aa673) SHA1(422c3d9decf9214a18edb536c2077bf52b272e7d),
                        "vid_u4.764",CRC(3a3d4c6b) SHA1(a6c27eee178a4bde67004e11f6ddf3b6414571dd),
                        "vid_u5.764",CRC(78bcb0fb) SHA1(d9dc1cc1bef063d5fbdbf2d1daf793234a9c55a0),
                        "vid_u6.764",CRC(8d8220a6) SHA1(64aa7d6ef2702c1b9afc61528434caf56cb91396),
                        "vid_u7.764",CRC(aa71cf29) SHA1(b69cd4060f5d4d2a7f85d901552cdc987013fde2),
                        "vid_u8.764",CRC(a442bc01) SHA1(2c01123dc5799561ae9e7c5d6db588b82b5ae59c),
                        "vid_u9.764",CRC(6b67a1f7) SHA1(251c2b941898363bbd6ee1a94710e2b2938ec851),
                        "cs_u3.764", CRC(0a39a51d) SHA1(98342ba38e48578ce9870f2ee85b553d46c0e35f))
BYVP_ROMEND
CORE_GAMEDEFNV(granny,"Granny and the Gators (Video/Pinball Combo)",1984,"Bally",byVP_mVP2,0)
