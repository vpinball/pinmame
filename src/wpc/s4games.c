// license:BSD-3-Clause

#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s4.h"
#include "sndbrd.h"

#define INITGAMEFULL(name, disp, sb, lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S4, disp, {FLIP_SWNO(lflip,rflip),0,0,0,sb}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S4_INPUT_PORTS_START(s4, 1) S4_INPUT_PORTS_END

/*--------------------------------
/ Phoenix - Sys.4 (Game #485)
/-------------------------------*/
INITGAMEFULL(phnix,s4_disp,SNDBRD_S3S,0,0,18,17,16,30,46,0)
S4_ROMSTART(phnix,l1,"gamerom.716", CRC(3aba6eac) SHA1(3a9f669216b3214bc42a1501aa2b10cfbcc36315),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8(     "485_s0_phoenix.716",CRC(1c3dea6e) SHA1(04bfe952be2eab66f023b204c21a1bd461ea572f))
S4_ROMEND
#define input_ports_phnix input_ports_s4
CORE_GAMEDEF(phnix,l1,"Phoenix (L-1)",1978,"Williams",s4_mS3S,0)

/*--------------------------------
/ Flash - Sys.4&6 (Game #486)
/-------------------------------*/
// tournament MODs exist (allentownpinball)

// Sys.4 L-1 also exists

// The later ROM revisions are in s6games.c

INITGAMEFULL(flash,s4_disp,SNDBRD_S67S,0,0,19,18,20,42,41,0)
S4_ROMSTART(flash,l2,"gamerom2.716",CRC(b7c2e4c7) SHA1(00ea34900af679b1b7e2698f4aa2fc9703d54cf2),
                     "yellow1.716", CRC(d251738c) SHA1(65ddbf5c36e429243331a4c5d2339df87a8a7f64),
                     "yellow2.716", CRC(5049326d) SHA1(3b2f4ea054962bf4ba41d46663b7d3d9a77590ef))
S67S_SOUNDROMS8(     "sound1.716",  CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_flash input_ports_s4
CORE_CLONEDEF(flash,l2,l1,"Flash (Sys.4 L-2)",1978,"Williams",s4_mS4S,0) // Yellow Flippers // 0486 2

/*--------------------------------
/ Pokerino - Sys.4 (Game #488)
/-------------------------------*/
INITGAMEFULL(pkrno,s4_disp,SNDBRD_S3S,0,0,38,39,40,30,0,28)
S4_ROMSTART(pkrno,l1,"gamerom.716",   CRC(9b4d01a8) SHA1(1bd51745f38381ffc66fde4b28b76aab33b573ca),
                     "white1.716",    CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716",    CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8("488_s0_pokerino.716",CRC(5de02e62) SHA1(f838439a731511a264e508a576ae7193d9fed1af))
S4_ROMEND
#define input_ports_pkrno input_ports_s4
CORE_GAMEDEF(pkrno,l1,"Pokerino (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Stellar Wars - Sys.4 (Game #490)
/-------------------------------*/
INITGAMEFULL(stlwr,s4_disp,SNDBRD_S67S,0,0,14,13,45,46,40,44)
S4_ROMSTART(stlwr,l2,"gamerom.716", CRC(874e7ef7) SHA1(271aeac2a0e61cb195811ae2e8d908cb1ab45874),
                     "yellow1.716", CRC(d251738c) SHA1(65ddbf5c36e429243331a4c5d2339df87a8a7f64),
                     "yellow2.716", CRC(5049326d) SHA1(3b2f4ea054962bf4ba41d46663b7d3d9a77590ef))
S67S_SOUNDROMS8(     "sound1.716",  CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_stlwr input_ports_s4
CORE_GAMEDEF(stlwr,l2,"Stellar Wars (L-2)",1979,"Williams",s4_mS4S,0)
