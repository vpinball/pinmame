#include "driver.h"
#include "sndbrd.h"
#include "core.h"
#include "sim.h"
#include "taito.h"
#include "taitos.h"

static const core_tLCDLayout dispTaito[] = {
  { 0, 0,  0, 6, CORE_SEG7 },
  { 3, 0,  6, 6, CORE_SEG7 },
  { 6, 0, 12, 6, CORE_SEG7 },
  { 9, 0, 18, 6, CORE_SEG7 },
  {13, 1, 24, 1, CORE_SEG7 }, {13, 9, 25, 1, CORE_SEG7 },
  {0}
};

// all display, including two usually hidden ones (match and player)
static const core_tLCDLayout dispTaito_all[] = {
  { 0, 0,  0, 6, CORE_SEG7 },
  { 3, 0,  6, 6, CORE_SEG7 },
  { 6, 0, 12, 6, CORE_SEG7 },
  { 9, 0, 18, 6, CORE_SEG7 },
  {13, 0, 26, 2, CORE_SEG7 }, {13, 8, 24, 2, CORE_SEG7 },
  {0}
};

TAITO_INPUT_PORTS_START(taito,1)        TAITO_INPUT_PORTS_END

#define INITGAME(name,sb) \
static core_tGameData name##GameData = {0,dispTaito,{FLIP_SW(FLIP_L),0,0,0,sb,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteort,SNDBRD_TAITO_SINTEVOX)
// The third rom seems badly dumped, so it's marked as NOT AVAILABLE.
TAITO_ROMSTART2222(meteort,"meteor1.bin",CRC(301a9f94),
                           "meteor2.bin",CRC(6d136853),
                           "meteor4.bin",CRC(c818e889),
                           "meteor4.bin",CRC(c818e889))
// NOT AVAILABLE
TAITO_SOUNDROMS22("meteo_s1.bin", CRC(23971d1e),
                  "meteo_s1.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_meteort input_ports_taito
CORE_GAMEDEFNV(meteort,"Meteor (Taito)",198?,"Taito",taito_sintevox,GAME_NOT_WORKING)

/*--------------------------------
/ Zarza
/-------------------------------*/
INITGAME(zarza,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(zarza,"zarza1.bin",CRC(81a35f85),
                         "zarza2.bin",CRC(cbf88eee),
                         "zarza3.bin",CRC(a5faf4d5),
                         "zarza4.bin",CRC(ddfcdd20))
TAITO_SOUNDROMS22("zarza_s1.bin", CRC(f076c2a8),
                  "zarza_s2.bin", CRC(a98e13b7))
TAITO_ROMEND
#define input_ports_zarza input_ports_taito
CORE_GAMEDEFNV(zarza,"Zarza",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Gemini 2000
/-------------------------------*/
INITGAME(gemini,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(gemini,"gemini1.bin",CRC(4f952799),
                          "gemini2.bin",CRC(8903ee53),
                          "gemini3.bin",CRC(1f11b5e5),
                          "gemini4.bin",CRC(cac64ea6))
TAITO_SOUNDROMS22("gemin_s1.bin", CRC(b9a80ab2),
                  "gemin_s2.bin", CRC(312a5c35))
TAITO_ROMEND
#define input_ports_gemini input_ports_taito
CORE_GAMEDEFNV(gemini,"Gemini 2000",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Cosmic
/-------------------------------*/
INITGAME(cosmic,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(cosmic,"cosmic1.bin",CRC(1864f295),
                          "cosmic2.bin",CRC(818e8621),
                          "cosmic3.bin",CRC(c3e0cf5d),
                          "cosmic4.bin",CRC(09ed5ecd))
TAITO_SOUNDROMS22("cosmc_s1.bin", CRC(09f082c1),
                  "cosmc_s2.bin", CRC(84b98b95))
TAITO_ROMEND
#define input_ports_cosmic input_ports_taito
CORE_GAMEDEFNV(cosmic,"Cosmic",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Hawkman
/-------------------------------*/
INITGAME(hawkman,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(hawkman,"hawk1.bin",CRC(cf991a68),
                           "hawk2.bin",CRC(568ac529),
                           "hawk3.bin",CRC(14be7e31),
                           "hawk4.bin",CRC(e6df08a5))
TAITO_SOUNDROMS22("hawk_s1.bin", CRC(47549394),
                  "hawk_s2.bin", CRC(29bef82f))
TAITO_ROMEND
#define input_ports_hawkman input_ports_taito
CORE_GAMEDEFNV(hawkman,"Hawkman",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Rally
/-------------------------------*/
INITGAME(rally,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(rally,"rally1.bin",CRC(d0d6b32e),
                         "rally2.bin",CRC(e7611e06),
                         "rally3.bin",CRC(45d28cd3),
                         "rally4.bin",CRC(7fb471ee))
TAITO_SOUNDROMS22("rally_s1.bin", CRC(0c7ca1bc),
                  "rally_s2.bin", CRC(a409d9d1))
TAITO_ROMEND
#define input_ports_rally input_ports_taito
CORE_GAMEDEFNV(rally,"Rally",1983,"Taito",taito_sintetizador,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Shark (Taito)
/-------------------------------*/
INITGAME(sharkt,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(sharkt,"shark1.bin",CRC(efe19b88),
                          "shark2.bin",CRC(ab11c287),
                          "shark3.bin",CRC(7ccf945b),
                          "shark4.bin",CRC(8ca33f37))
// NOT AVAILABLE
TAITO_SOUNDROMS22("shark_s1.bin", NO_DUMP,
                  "shark_s2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_sharkt input_ports_taito
CORE_GAMEDEFNV(sharkt,"Shark (Taito)",1982,"Taito",taito_sintevox,0)

/*--------------------------------
/ Snake Machine
/-------------------------------*/
INITGAME(snake,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(snake,"snake1.bin",CRC(7bb79585),
                         "snake2.bin",CRC(55c946f7),
                         "snake3.bin",CRC(6f054bc0),
                         "snake4.bin",CRC(ed231064))
// NOT AVAILABLE
TAITO_SOUNDROMS22("snake_s1.bin", NO_DUMP,
                  "snake_s2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_snake input_ports_taito
CORE_GAMEDEFNV(snake,"Snake Machine",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Titan
/-------------------------------*/
INITGAME(titan,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(titan,"titan1.bin",CRC(625f58fb),
                         "titan2.bin",CRC(f2e5a7d0),
                         "titan3.bin",CRC(e0827a82),
                         "titan4.bin",CRC(fb3d0282))
TAITO_SOUNDROMS22("titan_s1.bin", CRC(36b5c196),
                  "titan_s2.bin", CRC(3bd0e6ab))
TAITO_ROMEND
#define input_ports_titan input_ports_taito
CORE_GAMEDEFNV(titan,"Titan",198?,"Taito",taito_sintetizador,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Vortex
/-------------------------------*/
INITGAME(vortex,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(vortex,"vortex1.bin",CRC(abe193e7),
                          "vortex2.bin",CRC(0dd68604),
                          "vortex3.bin",CRC(a46e3722),
                          "vortex4.bin",CRC(39ef8112))
TAITO_SOUNDROMS22("vrtex_s1.bin", CRC(740bdd3e),
                  "vrtex_s2.bin", CRC(4250e02e))
TAITO_ROMEND
#define input_ports_vortex input_ports_taito
CORE_GAMEDEFNV(vortex,"Vortex",198?,"Taito",taito_sintevox,0)

/*--------------------------------
/ Speed Test
/-------------------------------*/
INITGAME(stest,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(stest,"stest1.bin",CRC(e13ed60c),
                         "stest2.bin",CRC(584d683d),
                         "stest3.bin",CRC(271129a2),
                         "stest4.bin",CRC(1cdd4e08))
TAITO_SOUNDROMS22("stest_s1.bin", CRC(dc71d4b2),
                  "stest_s2.bin", CRC(d7ac9369))
TAITO_ROMEND
#define input_ports_stest input_ports_taito
CORE_GAMEDEFNV(stest,"Speed Test",198?,"Taito",taito_sintetizador,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Space Shuttle
/-------------------------------*/
// The 4th rom isn't available, so it's marked as NOT AVAILABLE
INITGAME(sshuttle,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(sshuttle,"sshutle1.bin",CRC(ab67ed50),
                            "sshutle2.bin",CRC(ed5130a4),
                            "sshutle3.bin",CRC(b1ddb78b),
                            "sshutle4.bin",NO_DUMP)
TAITO_SOUNDROMS22("sshtl_s1.bin", CRC(0c7ca1bc),
                  "sshtl_s2.bin", CRC(a409d9d1))
TAITO_ROMEND
#define input_ports_sshuttle input_ports_taito
CORE_GAMEDEFNV(sshuttle,"Space Shuttle (Taito)",1985,"Taito",taito_sintetizador,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Drakor
/-------------------------------*/
// The 4th rom isn't available, so it's marked as NOT AVAILABLE
INITGAME(drakor,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(drakor,"drakor1.bin",CRC(b0ba866e),
                          "drakor2.bin",CRC(91dbb199),
                          "drakor3.bin",CRC(7ecf377b),
                          "drakor4.bin",NO_DUMP)
TAITO_SOUNDROMS22("drakors1.bin", NO_DUMP,
                  "drakors2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_drakor input_ports_taito
CORE_GAMEDEFNV(drakor,"Drakor (Taito)",1985,"Taito",taito_sintetizador,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Polar Explorer
/-------------------------------*/
INITGAME(polar,SNDBRD_TAITO_SINTEVOXPP)
TAITO_ROMSTART2222(polar,"polar1.bin",CRC(f92944b6),
                         "polar2.bin",CRC(e6391071),
                         "polar3.bin",CRC(318d0702),
                         "polar4.bin",CRC(1c02f0c9))
TAITO_SOUNDROMS444("polar_s1.bin", CRC(baff1a67),
                   "polar_s2.bin", CRC(84fe1dc8),
				   "polar_s3.bin", CRC(d574bc94))
TAITO_ROMEND
#define input_ports_polar input_ports_taito
CORE_GAMEDEFNV(polar,"Polar Explorer",198?,"Taito",taito_sintevoxpp,GAME_IMPERFECT_SOUND)
