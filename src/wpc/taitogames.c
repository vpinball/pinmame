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
  {13, 0, 24, 1, CORE_SEG7 }, {13,10, 25, 1, CORE_SEG7 },
  {0}
};

static const core_tLCDLayout dispTaito2[] = {
  { 0, 0,  0, 6, CORE_SEG7 },
  { 3, 0,  6, 6, CORE_SEG7 },
  { 6, 0, 12, 6, CORE_SEG7 },
  { 9, 0, 18, 6, CORE_SEG7 },
  {13, 0, 24, 1, CORE_SEG7 }, {13,10, 27, 1, CORE_SEG7 },
  {0}
};

TAITO_INPUT_PORTS_START(taito,1)        TAITO_INPUT_PORTS_END

#define INITGAME1(name,sb) \
static core_tGameData name##GameData = {0,dispTaito2,{FLIP_SW(FLIP_L),0,0,0,sb,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME(name,sb) \
static core_tGameData name##GameData = {0,dispTaito,{FLIP_SW(FLIP_L),0,0,0,sb,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

//??/?? Criterium 77
//??/?? Voley Ball
//??/?? Apache!
//??/?? Football
//??/79 Hot Ball (B Eight Ball, 01/77)
//??/79 Shock (W Flash, 01/79)
//??/?? Sultan (G Sinbad, 05/78)

/*--------------------------------
/ Oba-Oba
/-------------------------------*/
INITGAME1(obaoba,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(obaoba,"ob1.bin",CRC(85cddf4f),
                          "ob2.bin",CRC(7a110b82),
                          "ob3.bin",CRC(8f32a7c0))
TAITO_SOUNDROMS22("ob_s1.bin", CRC(812a362b),
                  "ob_s2.bin", CRC(f7dbb715))
TAITO_ROMEND
#define input_ports_obaoba input_ports_taito
CORE_GAMEDEFNV(obaoba,"Oba-Oba",1980,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Drakor
/-------------------------------*/
INITGAME(drakor,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(drakor,"drakor1.bin",CRC(7ecf377b),
                          "drakor2.bin",CRC(91dbb199),
                          "drakor3.bin",CRC(b0ba866e))
// NOT AVAILABLE
TAITO_SOUNDROMS22("drakors1.bin", NO_DUMP,
                  "drakors2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_drakor input_ports_taito
CORE_GAMEDEFNV(drakor,"Drakor",1980,"Taito",taito_sintetizador,0)

//??/?? Roman Victory
//??/?? Space Patrol

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteort,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(meteort,"meteor1.bin",CRC(301a9f94),
                           "meteor2.bin",CRC(6d136853),
                           "meteor3.bin",CRC(c818e889))
TAITO_SOUNDROMS2("meteo_s1.bin", CRC(23971d1e))
TAITO_ROMEND
#define input_ports_meteort input_ports_taito
CORE_GAMEDEFNV(meteort,"Meteor (Taito)",1980,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Fire Action
/-------------------------------*/
INITGAME(fireact,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(fireact,"fire1.bin",CRC(3059876d),
                           "fire2.bin",CRC(7906a193),
                           "fire3.bin",CRC(92135de4),
                           "fire4.bin",CRC(68de7753))
// not available
TAITO_SOUNDROMS22("fire_s1.bin", NO_DUMP,
				  "fire_s2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_fireact input_ports_taito
CORE_GAMEDEFNV(fireact,"Fire Action",1981,"Taito",taito_sintevox,0)

/*--------------------------------
/ Cavaleiro Negro
/-------------------------------*/
INITGAME(cavnegro,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(cavnegro,"cn1.bin",CRC(6b414089),
                            "cn2.bin",CRC(9641f2e5),
                            "cn3.bin",CRC(e1c5afd8),
                            "cn4.bin",CRC(0cf4c1fa))
TAITO_SOUNDROMS22("cn_s1.bin", CRC(aec5069a),
                  "cn_s2.bin", CRC(a0508863))
TAITO_ROMEND
#define input_ports_cavnegro input_ports_taito
CORE_GAMEDEFNV(cavnegro,"Cavaleiro Negro",1981,"Taito",taito_sintevox,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Sure Shot
/-------------------------------*/
INITGAME(sureshot,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(sureshot,"ssh1.bin",CRC(46b96e00),
                            "ssh2.bin",CRC(655a7ff2),
                            "ssh3.bin",CRC(4dec25d6),
                            "ssh4.bin",CRC(ced8f9df))
TAITO_SOUNDROMS222("ssh_s1.bin", CRC(acb7e92f),
                   "ssh_s2.bin", CRC(c1351b31),
				   "ssh_s3.bin", CRC(5e7f5275))
TAITO_ROMEND
#define input_ports_sureshot input_ports_taito
CORE_GAMEDEFNV(sureshot,"Sure Shot",1981,"Taito",taito_sintevox,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Vegas
/-------------------------------*/
INITGAME(vegast,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(vegast,"vegas1.bin",CRC(be242895),
                          "vegas2.bin",CRC(48169726),
                          "vegas3.bin",CRC(bd1fdbc3),
                          "vegas4.bin",CRC(61f733a9))
TAITO_SOUNDROMS22("vegas_s1.bin", CRC(740bdd3e),
                  "vegas_s2.bin", CRC(4250e02e))
TAITO_ROMEND
#define input_ports_vegast input_ports_taito
CORE_GAMEDEFNV(vegast,"Vegas",1981,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Cosmic
/-------------------------------*/
INITGAME(cosmic,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(cosmic,"cosmic1.bin",CRC(1864f295),
                          "cosmic2.bin",CRC(818e8621),
                          "cosmic3.bin",CRC(c3e0cf5d),
                          "cosmic4.bin",CRC(09ed5ecd))
TAITO_SOUNDROMS22("cosmc_s1.bin", CRC(09f082c1),
                  "cosmc_s2.bin", CRC(84b98b95))
TAITO_ROMEND
#define input_ports_cosmic input_ports_taito
CORE_GAMEDEFNV(cosmic,"Cosmic",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Gemini 2000
/-------------------------------*/
INITGAME(gemini,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(gemini,"gemini1.bin",CRC(4f952799),
                          "gemini2.bin",CRC(8903ee53),
                          "gemini3.bin",CRC(1f11b5e5),
                          "gemini4.bin",CRC(cac64ea6))
TAITO_SOUNDROMS22("gemin_s1.bin", CRC(b9a80ab2),
                  "gemin_s2.bin", CRC(312a5c35))
TAITO_ROMEND
#define input_ports_gemini input_ports_taito
CORE_GAMEDEFNV(gemini,"Gemini 2000",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Vortex
/-------------------------------*/
INITGAME(vortex,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(vortex,"vortex1.bin",CRC(abe193e7),
                          "vortex2.bin",CRC(0dd68604),
                          "vortex3.bin",CRC(a46e3722),
                          "vortex4.bin",CRC(39ef8112))
TAITO_SOUNDROMS22("vrtex_s1.bin", CRC(740bdd3e),
                  "vrtex_s2.bin", CRC(4250e02e))
TAITO_ROMEND
#define input_ports_vortex input_ports_taito
CORE_GAMEDEFNV(vortex,"Vortex",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Titan
/-------------------------------*/
INITGAME(titan,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(titan,"titan1.bin",CRC(625f58fb),
                         "titan2.bin",CRC(f2e5a7d0),
                         "titan3.bin",CRC(e0827a82),
                         "titan4.bin",CRC(fb3d0282))
TAITO_SOUNDROMS22("titan_s1.bin", CRC(36b5c196),
                  "titan_s2.bin", CRC(3bd0e6ab))
TAITO_ROMEND
#define input_ports_titan input_ports_taito
CORE_GAMEDEFNV(titan,"Titan",1982,"Taito",taito_sintevox,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Zarza
/-------------------------------*/
INITGAME(zarza,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(zarza,"zarza1.bin",CRC(81a35f85),
                         "zarza2.bin",CRC(cbf88eee),
                         "zarza3.bin",CRC(a5faf4d5),
                         "zarza4.bin",CRC(ddfcdd20))
TAITO_SOUNDROMS22("zarza_s1.bin", CRC(f076c2a8),
                  "zarza_s2.bin", CRC(a98e13b7))
TAITO_ROMEND
#define input_ports_zarza input_ports_taito
CORE_GAMEDEFNV(zarza,"Zarza",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Shark (Taito)
/-------------------------------*/
INITGAME(sharkt,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(sharkt,"shark1.bin",CRC(efe19b88),
                          "shark2.bin",CRC(ab11c287),
                          "shark3.bin",CRC(7ccf945b),
                          "shark4.bin",CRC(8ca33f37))
TAITO_SOUNDROMS4("shark_s1.bin",CRC(75969a7d))
TAITO_ROMEND
#define input_ports_sharkt input_ports_taito
CORE_GAMEDEFNV(sharkt,"Shark (Taito)",1982,"Taito",taito_sintetizador,0)

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
CORE_GAMEDEFNV(hawkman,"Hawkman",1982,"Taito",taito_sintevox,GAME_IMPERFECT_SOUND)

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
CORE_GAMEDEFNV(stest,"Speed Test",1982,"Taito",taito_sintetizador,0)

//??/82 Lunelle (W Alien Poker, 10/80)

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
CORE_GAMEDEFNV(rally,"Rally",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Snake Machine
/-------------------------------*/
INITGAME(snake,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART2222(snake,"snake1.bin",CRC(7bb79585),
                         "snake2.bin",CRC(55c946f7),
                         "snake3.bin",CRC(6f054bc0),
                         "snake4.bin",CRC(ed231064))
// NOT AVAILABLE
TAITO_SOUNDROMS444("snake_s1.bin", NO_DUMP,
                   "snake_s2.bin", NO_DUMP,
                   "snake_s3.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_snake input_ports_taito
CORE_GAMEDEFNV(snake,"Snake Machine",1982,"Taito",taito_sintetizadorpp,GAME_IMPERFECT_SOUND)

//??/82 Gork
//??/8? Ogar

/*--------------------------------
/ Mr. Black
/-------------------------------*/
INITGAME(mrblack,SNDBRD_TAITO_SINTEVOXPP)
TAITO_ROMSTART22222(mrblack,"mrb1.bin",CRC(c2a43f6f),
                            "mrb2.bin",CRC(ddf2a88e),
                            "mrb3.bin",CRC(f319f68f),
                            "mrb4.bin",CRC(84367699),
                            "mrb5.bin",CRC(18d8f2cc))
// NOT AVAILABLE
TAITO_SOUNDROMS444("mrb_s1.bin", NO_DUMP,
                   "mrb_s2.bin", NO_DUMP,
                   "mrb_s3.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_mrblack input_ports_taito
CORE_GAMEDEFNV(mrblack,"Mr. Black",1984,"Taito",taito_sintevoxpp,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Fire Action Deluxe
/-------------------------------*/
INITGAME(fireactd,SNDBRD_TAITO_SINTEVOXPP)
TAITO_ROMSTART2222(fireactd,"fired1.bin",CRC(2f923913),
                            "fired2.bin",CRC(4d268048),
                            "fired3.bin",CRC(f5e07ed1),
                            "fired4.bin",CRC(da1a4ed5))
// NOT AVAILABLE
TAITO_SOUNDROMS444("fired_s1.bin", NO_DUMP,
                   "fired_s2.bin", NO_DUMP,
                   "fired_s3.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_fireactd input_ports_taito
CORE_GAMEDEFNV(fireactd,"Fire Action Deluxe",198?,"Taito",taito_sintevoxpp,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Space Shuttle
/-------------------------------*/
INITGAME(sshuttle,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART22222(sshuttle,"sshutle1.bin",CRC(ab67ed50),
                             "sshutle2.bin",CRC(ed5130a4),
                             "sshutle3.bin",CRC(b1ddb78b),
                             "sshutle4.bin",CRC(a409d9d1),
                             "sshutle5.bin",CRC(0c7ca1bc))
TAITO_SOUNDROMS444("sshtl_s1.bin", NO_DUMP,
                   "sshtl_s2.bin", NO_DUMP,
                   "sshtl_s3.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_sshuttle input_ports_taito
CORE_GAMEDEFNV(sshuttle,"Space Shuttle (Taito)",1985,"Taito",taito_sintetizadorpp,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Polar Explorer
/-------------------------------*/
INITGAME(polar,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART2222(polar,"polar1.bin",CRC(f92944b6),
                         "polar2.bin",CRC(e6391071),
                         "polar3.bin",CRC(318d0702),
                         "polar4.bin",CRC(1c02f0c9))
TAITO_SOUNDROMS444("polar_s1.bin", CRC(baff1a67),
                   "polar_s2.bin", CRC(84fe1dc8),
				   "polar_s3.bin", CRC(d574bc94))
TAITO_ROMEND
#define input_ports_polar input_ports_taito
CORE_GAMEDEFNV(polar,"Polar Explorer",198?,"Taito",taito_sintetizadorpp,GAME_IMPERFECT_SOUND)
