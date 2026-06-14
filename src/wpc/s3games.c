// license:BSD-3-Clause

/*
Note on the original/shipped PROM configuration, via https://github.com/vpinball/pinmame/issues/583 :
All S3 games have two files in common:
ROM_LOAD("white1.716", 0x1000, 0x0800, CRC(9bbbf14f)
ROM_LOAD("white2.716", 0x1800, 0x0800, CRC(4d4010dd)

But they are distinguished by the file
ROM_LOAD("gamerom.716", 0x0000, 0x0800, CRC(..)
which is unique for each game.

According to the schematics, however, the physical configuration of the memory chips (PROM and EPROM) on the original boards was slightly different
(using Hot Tip as an example, but the same applies to other games):
The schematic (page 7 for Hot Tip) clearly indicates that IC17 and IC20 are two 2Kx8 EPROMs (17=upper, 20=lower), i.e., 2716 size=0x0800
However, there is NO other EPROM 2716 size=0x0800 on the board which would be corresponding to gamerom.716!
Instead, there are two 512x8 PROMs (type 7641/6341, compatible with the 74S474 and 82S141) called IC21 and IC22, and the correct dumps of these two 512x8 PROMs have been available on IPDB for years.

Comparing these two files with gamerom.716 yields the following:
prom1.474   [0x200] CRC(ebfc71cb) SHA1(6b6204c1e78873f22e9c12a40cd2eea73d9272c8) SUM(6aaf)
prom2.474   [0x200] CRC(2d0a402f) SHA1(93c33da7e0f329cddb2eb149df98316f7525d24e) SUM(87a7)
gamerom.716 [0x800] CRC(b1d4fd9b) SHA1(e55ecf1328a55979c4cf8f3fb4e6761747e0abc4) SUM(f256)
1xxxxxxxxxx = 0x00
prom1.474 gamerom.716 [1/4] IDENTICAL
prom2.474 gamerom.716 [2/4] IDENTICAL

So, gamerom.716 has the second half completely empty (0x00) and the first half of it is the concatenation of these two PROMs.
This can be explained by the fact that if a PROM breaks, it was virtually impossible for the arcade manager to repair/replace it,
but these could have been easily replaced with a simple 2716, reprogrammed as "gamerom.716".
*/

#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "sndbrd.h"
#include "s4.h"         // System 3 Is Nearly Identical to 4, so we share functions

#define s3_mS3  s4_mS4  //                        ""

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = { gen, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, gen, disp, sb, lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { gen, disp, {FLIP_SWNO(lflip,rflip),0,0,0,sb}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S3_INPUT_PORTS_START(s3, 1) S3_INPUT_PORTS_END

/*----------------------------
/ Hot Tip - Sys.3 (Game #477) - No Sound board
/----------------------------*/
INITGAMEFULL(httip, GEN_S3C, s4_disp, SNDBRD_NONE, 0,0,31,13,35,0,0,0)
S4_ROMSTART(httip,l1,"gamerom.716",CRC(b1d4fd9b) SHA1(e55ecf1328a55979c4cf8f3fb4e6761747e0abc4),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S4_ROMEND
#define input_ports_httip input_ports_s3
CORE_GAMEDEF(httip,l1,"Hot Tip (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

// https://zacaj.com/p.php?id=66 MOD exists

/*---------------------------------
/ Lucky Seven - Sys.3 (Game #480) - No Sound board
/---------------------------------*/
INITGAMEFULL(lucky, GEN_S3C, s4_disp, SNDBRD_NONE, 0,0,34,33,36,24,0,0)
S4_ROMSTART(lucky,l1,"gamerom.716",CRC(7cfbd4c7) SHA1(825e2245fd1615e932973f5e2b5ed5f2da9309e7),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S4_ROMEND
#define input_ports_lucky input_ports_s3
CORE_GAMEDEF(lucky,l1,"Lucky Seven (L-1)",1977,"Williams",s3_mS3C,GAME_USES_CHIMES)

/*-------------------------------------
/ World Cup - Sys.3 (Game #481)
/-------------------------------------*/
INITGAMEFULL(wldcp, GEN_S3, s4_disp, SNDBRD_S3WCS, 0,0,38,37,0,0,0,0)
S4_ROMSTART(wldcp,l1,"gamerom.716", CRC(c8071956) SHA1(0452aaf2ec1bcc5717fe52a6c541d79402bebb17),
                     "white1.716",  CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2wc.716",CRC(618d15b5) SHA1(527387893eeb2cd4aa563a4cfb1948a15d2ed741))
S67S_SOUNDROMS8("481_s0_world_cup.716",CRC(cf012812) SHA1(26074f6a44075a94e6f91de1dbf92f8ec3ff8ca4)) // same as 481-50-world_cup.716
S4_ROMEND
#define input_ports_wldcp input_ports_s3
CORE_GAMEDEF(wldcp,l1,"World Cup (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Contact - Sys.3 (Game #482)
/-------------------------------------*/
INITGAMEFULL(cntct, GEN_S3, s4_disp, SNDBRD_S3S, 0,0,33,34,35,0,0,0)
S4_ROMSTART(cntct,l1,"gamerom.716",CRC(35359b60) SHA1(ab4c3328d93bdb4c952090b327c91b0ded36152c),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8("482_s0_contact.716",CRC(d3c713da) SHA1(1fc4a8fadf472e9a04b3a86f60a9d625d07764e1))
S4_ROMEND
#define input_ports_cntct input_ports_s3
CORE_GAMEDEF(cntct,l1,"Contact (L-1)",1978,"Williams",s3_mS3S,0)

/*-------------------------------------
/ Disco Fever - Sys.3 (Game #483)
/-------------------------------------*/
INITGAMEFULL(disco, GEN_S3, s4_disp, SNDBRD_S3DFS, 0,0,33,17,23,21,0,0)
S4_ROMSTART(disco,l1,"gamerom.716",CRC(831d8adb) SHA1(99a9c3d5c8cbcdf3bb9c210ad9d05c34905b272e),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8("483_s0_disco_fever.716",CRC(d1cb5047) SHA1(7f36296975df19feecc6456ffb91f4a23bcad037)) // also a PROM version exists, but its the same as this one
S4_ROMEND
#define input_ports_disco input_ports_s3
CORE_GAMEDEF(disco,l1,"Disco Fever (L-1)",1978,"Williams",s3_mS3S,0)
