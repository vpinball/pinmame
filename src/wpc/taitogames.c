#include "driver.h"
#include "sndbrd.h"
#include "core.h"
#include "sim.h"
#include "taito.h"
#include "taitos.h"

static const core_tLCDLayout dispTaito[] = {
  { 0, 0,  0, 3, CORE_SEG87 }, { 0, 6,  3, 3, CORE_SEG87 },
  { 3, 0,  6, 3, CORE_SEG87 }, { 3, 6,  9, 3, CORE_SEG87 },
  { 6, 0, 12, 3, CORE_SEG87 }, { 6, 6, 15, 3, CORE_SEG87 },
  { 9, 0, 18, 3, CORE_SEG87 }, { 9, 6, 21, 3, CORE_SEG87 },
  {13, 0, 26, 2, CORE_SEG87 }, {13, 8, 24, 2, CORE_SEG87 },
  {0}
};

TAITO_INPUT_PORTS_START(taito,1)        TAITO_INPUT_PORTS_END

#define INITGAME(name) \
static core_tGameData name##GameData = {0,dispTaito,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_TAITO,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteort)
// The third rom seems badly dumped, so I marked it as NO GOOD DUMP KNOWN.
TAITO_ROMSTART4444(meteort,"meteor1.bin",0x301a9f94,
                           "meteor2.bin",0x6d136853,
                           "meteor3.bin",0x0,
                           "meteor4.bin",0xc818e889)
TAITO_ROMEND
#define input_ports_meteort input_ports_taito
CORE_GAMEDEFNV(meteort,"Meteor (Taito)",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Zarza
/-------------------------------*/
INITGAME(zarza)
TAITO_ROMSTART4444(zarza,"zarza1.bin",0x81a35f85,
                         "zarza2.bin",0xcbf88eee,
                         "zarza3.bin",0xa5faf4d5,
                         "zarza4.bin",0xddfcdd20)
TAITO_SOUNDROMS("zarza_s1.bin", 0xf076c2a8,
                "zarza_s2.bin", 0xa98e13b7)
TAITO_ROMEND
#define input_ports_zarza input_ports_taito
CORE_GAMEDEFNV(zarza,"Zarza",198?,"Taito",taito,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Gemini 2000
/-------------------------------*/
INITGAME(gemini)
TAITO_ROMSTART4444(gemini,"gemini1.bin",0x4f952799,
                          "gemini2.bin",0x8903ee53,
                          "gemini3.bin",0x1f11b5e5,
                          "gemini4.bin",0xcac64ea6)
TAITO_SOUNDROMS("gemin_s1.bin", 0xb9a80ab2,
                "gemin_s2.bin", 0x312a5c35)
TAITO_ROMEND
#define input_ports_gemini input_ports_taito
CORE_GAMEDEFNV(gemini,"Gemini 2000",198?,"Taito",taito,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Cosmic
/-------------------------------*/
INITGAME(cosmic)
TAITO_ROMSTART4444(cosmic,"cosmic1.bin",0x1864f295,
                          "cosmic2.bin",0x818e8621,
                          "cosmic3.bin",0xc3e0cf5d,
                          "cosmic4.bin",0x09ed5ecd)
TAITO_ROMEND
#define input_ports_cosmic input_ports_taito
CORE_GAMEDEFNV(cosmic,"Cosmic",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Hawkman
/-------------------------------*/
INITGAME(hawkman)
TAITO_ROMSTART4444(hawkman,"hawk1.bin",0xcf991a68,
                           "hawk2.bin",0x568ac529,
                           "hawk3.bin",0x14be7e31,
                           "hawk4.bin",0xe6df08a5)
TAITO_ROMEND
#define input_ports_hawkman input_ports_taito
CORE_GAMEDEFNV(hawkman,"Hawkman",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Polar Explorer
/-------------------------------*/
INITGAME(polar)
TAITO_ROMSTART4444(polar,"polar1.bin",0xf92944b6,
                         "polar2.bin",0xe6391071,
                         "polar3.bin",0x318d0702,
                         "polar4.bin",0x1c02f0c9)
TAITO_ROMEND
#define input_ports_polar input_ports_taito
CORE_GAMEDEFNV(polar,"Polar Explorer",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Rally
/-------------------------------*/
INITGAME(rally)
TAITO_ROMSTART4444(rally,"rally1.bin",0xd0d6b32e,
                         "rally2.bin",0xe7611e06,
                         "rally3.bin",0x45d28cd3,
                         "rally4.bin",0x7fb471ee)
TAITO_ROMEND
#define input_ports_rally input_ports_taito
CORE_GAMEDEFNV(rally,"Rally",1983,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Shark
/-------------------------------*/
INITGAME(sharkt)
TAITO_ROMSTART4444(sharkt,"shark1.bin",0xefe19b88,
                          "shark2.bin",0xab11c287,
                          "shark3.bin",0x7ccf945b,
                          "shark4.bin",0x8ca33f37)
TAITO_ROMEND
#define input_ports_sharkt input_ports_taito
CORE_GAMEDEFNV(sharkt,"Shark (Taito)",1982,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Snake Machine
/-------------------------------*/
INITGAME(snake)
TAITO_ROMSTART4444(snake,"snake1.bin",0x7bb79585,
                         "snake2.bin",0x55c946f7,
                         "snake3.bin",0x6f054bc0,
                         "snake4.bin",0xed231064)
TAITO_ROMEND
#define input_ports_snake input_ports_taito
CORE_GAMEDEFNV(snake,"Snake Machine",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Titan
/-------------------------------*/
INITGAME(titan)
TAITO_ROMSTART4444(titan,"titan1.bin",0x625f58fb,
                         "titan2.bin",0xf2e5a7d0,
                         "titan3.bin",0xe0827a82,
                         "titan4.bin",0xfb3d0282)
TAITO_ROMEND
#define input_ports_titan input_ports_taito
CORE_GAMEDEFNV(titan,"Titan",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Vortex
/-------------------------------*/
INITGAME(vortex)
TAITO_ROMSTART4444(vortex,"vortex1.bin",0xabe193e7,
                          "vortex2.bin",0x0dd68604,
                          "vortex3.bin",0xa46e3722,
                          "vortex4.bin",0x39ef8112)
TAITO_ROMEND
#define input_ports_vortex input_ports_taito
CORE_GAMEDEFNV(vortex,"Vortex",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Speed Test
/-------------------------------*/
INITGAME(stest)
TAITO_ROMSTART4444(stest,"stest1.bin",0xe13ed60c,
                         "stest2.bin",0x584d683d,
                         "stest3.bin",0x271129a2,
                         "stest4.bin",0x1cdd4e08)
TAITO_ROMEND
#define input_ports_stest input_ports_taito
CORE_GAMEDEFNV(stest,"Speed Test",198?,"Taito",taito,GAME_NOT_WORKING)

/*--------------------------------
/ Space Shuttle
/-------------------------------*/
// I've only three instead of four controller ROMS, can't say if that's ok
INITGAME(sshuttle)
TAITO_ROMSTART444(sshuttle,"sshutle1.bin",0xab67ed50,
                           "sshutle2.bin",0xed5130a4,
                           "sshutle3.bin",0xb1ddb78b)
TAITO_ROMEND
#define input_ports_sshuttle input_ports_taito
CORE_GAMEDEFNV(sshuttle,"Space Shuttle (Taito)",1985,"Taito",taito,GAME_NOT_WORKING)

