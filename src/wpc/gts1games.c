#include "driver.h"
#include "sim.h"
#include "gts1.h"
#include "sndbrd.h"
#include "gts80.h"
#include "gts80s.h"

static core_tLCDLayout sys1_disp[] = {
  {0, 0,12,6,CORE_SEG9}, {0,16,18,6,CORE_SEG9},
  {2, 0, 0,6,CORE_SEG9}, {2,16, 6,6,CORE_SEG9},
  {4,16,40,2,CORE_SEG7}, {4, 8,32,2,CORE_SEG7},
  {0}
};

#define INIT_S1(name, dsp) \
GTS1_INPUT_PORTS_START(name, 1) GTS1_INPUT_PORTS_END \
static core_tGameData name##GameData = {0,dsp,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_NONE}}; \
static void init_##name(void) { \
	core_gameData = &name##GameData; \
} \
GTS1_2_ROMSTART(name, "u5_cf.bin", CRC(e0d4b405) SHA1(17aadd79c0dcbb336aadd5d203bc6ca866492345), \
                      "u4_ce.bin", CRC(4cd312dd) SHA1(31245daa9972ef8652caee69986585bb8239e86e))

#define INIT_S1S(name, dsp) \
GTS1S_INPUT_PORTS_START(name, 1) GTS1_INPUT_PORTS_END \
static core_tGameData name##GameData = {0,dsp,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_GTS80S}}; \
static void init_##name(void) { \
	core_gameData = &name##GameData; \
} \
GTS1_2_ROMSTART(name, "u5_cf.bin", CRC(e0d4b405) SHA1(17aadd79c0dcbb336aadd5d203bc6ca866492345), \
                      "u4_ce.bin", CRC(4cd312dd) SHA1(31245daa9972ef8652caee69986585bb8239e86e))

INIT_S1(gts1, sys1_disp) GTS1_ROMEND
GAMEX(1977,gts1,0,GTS1C,gts1,gts1,ROT0,"Gottlieb","System 1",NOT_A_DRIVER)

INIT_S1S(gts1s, sys1_disp) GTS1_ROMEND
GAMEX(1979,gts1s,gts1,GTS1S80,gts1,gts1,ROT0,"Gottlieb","System 1 with sound board",NOT_A_DRIVER)

/*-------------------------------------------------------------------
/ Cleopatra (11/1977)
/-------------------------------------------------------------------*/
INIT_S1(cleoptra, sys1_disp)
GTS1_1_ROMSTART(cleoptra,"409.cpu", CRC(8063ff71) SHA1(205f09f067bf79544d2ce2a48d23259901f935dd))
GTS1_ROMEND
CORE_CLONEDEFNV(cleoptra,gts1,"Cleopatra",1977,"Gottlieb",gl_mGTS1C,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Sinbad (05/1978)
/-------------------------------------------------------------------*/
INIT_S1(sinbad, sys1_disp)
GTS1_1_ROMSTART(sinbad,"412.cpu", CRC(84a86b83) SHA1(f331f2ffd7d1b279b4ffbb939aa8649e723f5fac))
GTS1_ROMEND
CORE_CLONEDEFNV(sinbad,gts1,"Sinbad",1978,"Gottlieb",gl_mGTS1C,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Sinbad (Norway)
/-------------------------------------------------------------------*/
INIT_S1(sinbadn, sys1_disp)
GTS1_1_ROMSTART(sinbadn,"412no1.cpu", CRC(f5373f5f) SHA1(027840501416ff01b2adf07188c7d667adf3ad5f))
GTS1_ROMEND
CORE_CLONEDEFNV(sinbadn,sinbad,"Sinbad (Norway)",1978,"Gottlieb",gl_mGTS1C,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Joker Poker (08/1978)
/-------------------------------------------------------------------*/
INIT_S1(jokrpokr, sys1_disp)
GTS1_1_ROMSTART(jokrpokr,"417.cpu", CRC(33dade08) SHA1(23b8dbd7b6c84b806fc0d2da95478235cbf9f80a))
GTS1_ROMEND
CORE_CLONEDEFNV(jokrpokr,gts1,"Joker Poker",1978,"Gottlieb",gl_mGTS1C,GAME_USES_CHIMES)

/*-------------------------------------------------------------------
/ Dragon (10/1978)
/-------------------------------------------------------------------*/
INIT_S1(dragon, sys1_disp)
GTS1_1_ROMSTART(dragon,"419.cpu", CRC(018d9b3a) SHA1(da37ef5017c71bc41bdb1f30d3fd7ac3b7e1ee7e))
GTS1_ROMEND
CORE_CLONEDEFNV(dragon,gts1,"Dragon",1978,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Close Encounters of the Third Kind (10/1978)
/-------------------------------------------------------------------*/
INIT_S1(closeenc, sys1_disp)
GTS1_1_ROMSTART(closeenc,"424.cpu", CRC(a7a5dd13) SHA1(223c67b9484baa719c91de52b363ff22813db160))
GTS1_ROMEND
CORE_CLONEDEFNV(closeenc,gts1,"Close Encounters of the Third Kind",1978,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Charlie's Angels (11/1978)
/-------------------------------------------------------------------*/
INIT_S1(charlies, sys1_disp)
GTS1_1_ROMSTART(charlies,"425.cpu", CRC(928b4279) SHA1(51096d45e880d6a8263eaeaa0cdab0f61ad2f58d))
GTS1_ROMEND
CORE_CLONEDEFNV(charlies,gts1,"Charlie's Angels",1978,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Solar Ride (02/1979)
/-------------------------------------------------------------------*/
INIT_S1(solaride, sys1_disp)
GTS1_1_ROMSTART(solaride,"421.cpu", CRC(6b5c5da6) SHA1(a09b7009473be53586f53f48b7bfed9a0c5ecd55))
GTS1_ROMEND
CORE_CLONEDEFNV(solaride,gts1,"Solar Ride",1979,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Count-Down (05/1979)
/-------------------------------------------------------------------*/
INIT_S1(countdwn, sys1_disp)
GTS1_1_ROMSTART(countdwn,"422.cpu", CRC(51bc2df0) SHA1(d4b555d106c6b4e420b0fcd1df8871f869476c22))
GTS1_ROMEND
CORE_CLONEDEFNV(countdwn,gts1,"Count-Down",1979,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Pinball Pool (08/1979)
/-------------------------------------------------------------------*/
INIT_S1(pinpool, sys1_disp)
GTS1_1_ROMSTART(pinpool,"427.cpu", CRC(c496393d) SHA1(e91d9596aacdb4277fa200a3f8f9da099c278f32))
GTS1_ROMEND
CORE_CLONEDEFNV(pinpool,gts1,"Pinball Pool",1979,"Gottlieb",gl_mGTS1T,0)

/*-------------------------------------------------------------------
/ Totem (10/1979)
/-------------------------------------------------------------------*/
INIT_S1S(totem, sys1_disp)
GTS1_1_ROMSTART(totem,"429.cpu", CRC(7885a384) SHA1(1770662af7d48ad8297097a9877c5c497119978d))
GTS80S1K_ROMSTART(    "429.snd", CRC(5d1b7ed4) SHA1(4a584f880e907fb21da78f3b3a0617f20599688f),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(totem,gts1s,"Totem",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ The Incredible Hulk (10/1979)
/-------------------------------------------------------------------*/
INIT_S1S(hulk, sys1_disp)
GTS1_1_ROMSTART(hulk,"433.cpu", CRC(c05d2b52) SHA1(393fe063b029246317c90ee384db95a84d61dbb7))
GTS80S1K_ROMSTART(   "433.snd", CRC(20cd1dff) SHA1(93e7c47ff7051c3c0dc9f8f95aa33ba094e7cf25),
                "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(hulk,gts1s,"Incredible Hulk, The",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Genie (11/1979)
/-------------------------------------------------------------------*/
INIT_S1S(genie, sys1_disp)
GTS1_1_ROMSTART(genie,"435.cpu", CRC(7749fd92) SHA1(9cd3e799842392e3939877bf295759c27f199e58))
GTS80S1K_ROMSTART(    "435.snd", CRC(4a98ceed) SHA1(f1d7548e03107033c39953ee04b043b5301dbb47),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(genie,gts1s,"Genie",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Buck Rogers (01/1980)
/-------------------------------------------------------------------*/
INIT_S1S(buckrgrs, sys1_disp)
GTS1_1_ROMSTART(buckrgrs,"437.cpu", CRC(e57d9278) SHA1(dfc4ebff1e14b9a074468671a8e5ac7948d5b352))
GTS80S1K_ROMSTART(       "437.snd", CRC(732b5a27) SHA1(7860ea54e75152246c3ac3205122d750b243b40c),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(buckrgrs,gts1s,"Buck Rogers",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Torch (02/1980)
/-------------------------------------------------------------------*/
INIT_S1S(torch, sys1_disp)
GTS1_1_ROMSTART(torch,"438.cpu", CRC(2d396a64) SHA1(38a1862771500faa471071db08dfbadc6e8759e8))
GTS80S1K_ROMSTART(    "438.snd", CRC(a9619b48) SHA1(1906bc1b059bf31082e3b4546f5a30159479ad3c),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(torch,gts1s,"Torch",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Roller Disco (02/1980)
/-------------------------------------------------------------------*/
INIT_S1S(roldisco, sys1_disp)
GTS1_1_ROMSTART(roldisco,"440.cpu", CRC(bc50631f) SHA1(6aa3124d09fc4e369d087a5ad6dd1737ace55e41))
GTS80S1K_ROMSTART(       "440.snd", CRC(4a0a05ae) SHA1(88f21b5638494d8e78dc0b6b7d69873b76b5f75d),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(roldisco,gts1s,"Roller Disco",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Asteroid Annie and the Aliens (12/1980)
/-------------------------------------------------------------------*/
static core_tLCDLayout aa_disp[] = {
  {0, 0,12,6,CORE_SEG9},
  {2, 8,40,2,CORE_SEG7}, {2, 0,32,2,CORE_SEG7},
  {0}
};
INIT_S1S(astannie, aa_disp)
GTS1_1_ROMSTART(astannie,"442.cpu", CRC(579521e0) SHA1(b1b19473e1ca3373955ee96104b87f586c4c311c))
GTS80S1K_ROMSTART(       "442.snd", CRC(c70195b4) SHA1(ff06197f07111d6a4b8942dcfe8d2279bda6f281),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_CLONEDEFNV(astannie,gts1s,"Asteroid Annie and the Aliens",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ System 1 Test prom
/-------------------------------------------------------------------*/
INIT_S1(sys1test, sys1_disp)
GTS1_1_ROMSTART(sys1test,"test.cpu", CRC(8b0704bb) SHA1(5f0eb8d5af867b815b6012c9d078927398efe6d8))
GTS1_ROMEND
CORE_CLONEDEFNV(sys1test,gts1,"System 1 Test prom",19??,"Gottlieb",gl_mGTS1C,GAME_USES_CHIMES)
