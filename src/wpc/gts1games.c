#include "driver.h"
#include "sim.h"
#include "gts1.h"
#include "sndbrd.h"
#include "gts80.h"
#include "gts80s.h"

static core_tLCDLayout sys1_disp[] = {
  {0, 0, 0,16,CORE_SEG9},
  {2, 0,16,16,CORE_SEG9},
  {0}
};

#define INITGAME(name, sb) \
	GTS1_INPUT_PORTS_START(name, 1) GTS1_INPUT_PORTS_END \
	static core_tGameData name##GameData = {0,sys1_disp,{FLIP_SW(FLIP_L),0,2,0,sb}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/*-------------------------------------------------------------------
/ Cleopatra (11/1977)
/-------------------------------------------------------------------*/
INITGAME(cleoptra, SNDBRD_NONE)
GTS1_1_ROMSTART(cleoptra,"409.cpu", CRC(8063ff71) SHA1(205f09f067bf79544d2ce2a48d23259901f935dd))
GTS1_ROMEND
CORE_GAMEDEFNV(cleoptra,"Cleopatra",1977,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Sinbad (05/1978)
/-------------------------------------------------------------------*/
INITGAME(sinbad, SNDBRD_NONE)
GTS1_1_ROMSTART(sinbad,"412.cpu", CRC(84a86b83) SHA1(f331f2ffd7d1b279b4ffbb939aa8649e723f5fac))
GTS1_ROMEND
CORE_GAMEDEFNV(sinbad,"Sinbad",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Sinbad (Norway)
/-------------------------------------------------------------------*/
INITGAME(sinbadn, SNDBRD_NONE)
GTS1_1_ROMSTART(sinbadn,"412no1.cpu", CRC(f5373f5f) SHA1(027840501416ff01b2adf07188c7d667adf3ad5f))
GTS1_ROMEND
CORE_CLONEDEFNV(sinbadn,sinbad,"Sinbad (Norway)",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Joker Poker (08/1978)
/-------------------------------------------------------------------*/
INITGAME(jokrpokr, SNDBRD_NONE)
GTS1_1_ROMSTART(jokrpokr,"417.cpu", CRC(33dade08) SHA1(23b8dbd7b6c84b806fc0d2da95478235cbf9f80a))
GTS1_ROMEND
CORE_GAMEDEFNV(jokrpokr,"Joker Poker",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Dragon (10/1978)
/-------------------------------------------------------------------*/
INITGAME(dragon, SNDBRD_NONE)
GTS1_1_ROMSTART(dragon,"419.cpu", CRC(018d9b3a) SHA1(da37ef5017c71bc41bdb1f30d3fd7ac3b7e1ee7e))
GTS1_ROMEND
CORE_GAMEDEFNV(dragon,"Dragon",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Close Encounters of the Third Kind (10/1978)
/-------------------------------------------------------------------*/
INITGAME(closeenc, SNDBRD_NONE)
GTS1_1_ROMSTART(closeenc,"424.cpu", CRC(a7a5dd13) SHA1(223c67b9484baa719c91de52b363ff22813db160))
GTS1_ROMEND
CORE_GAMEDEFNV(closeenc,"Close Encounters of the Third Kind",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Charlie's Angels (11/1978)
/-------------------------------------------------------------------*/
INITGAME(charlies, SNDBRD_NONE)
GTS1_1_ROMSTART(charlies,"425.cpu", CRC(928b4279) SHA1(51096d45e880d6a8263eaeaa0cdab0f61ad2f58d))
GTS1_ROMEND
CORE_GAMEDEFNV(charlies,"Charlie's Angels",1978,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Solar Ride (02/1979)
/-------------------------------------------------------------------*/
INITGAME(solaride, SNDBRD_NONE)
GTS1_1_ROMSTART(solaride,"421.cpu", CRC(6b5c5da6) SHA1(a09b7009473be53586f53f48b7bfed9a0c5ecd55))
GTS1_ROMEND
CORE_GAMEDEFNV(solaride,"Solar Ride",1979,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Count-Down (05/1979)
/-------------------------------------------------------------------*/
INITGAME(countdwn, SNDBRD_NONE)
GTS1_1_ROMSTART(countdwn,"422.cpu", CRC(51bc2df0) SHA1(d4b555d106c6b4e420b0fcd1df8871f869476c22))
GTS1_ROMEND
CORE_GAMEDEFNV(countdwn,"Count-Down",1979,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Pinball Pool (08/1979)
/-------------------------------------------------------------------*/
INITGAME(pinpool, SNDBRD_NONE)
GTS1_1_ROMSTART(pinpool,"427.cpu", CRC(c496393d) SHA1(e91d9596aacdb4277fa200a3f8f9da099c278f32))
GTS1_ROMEND
CORE_GAMEDEFNV(pinpool,"Pinball Pool",1979,"Gottlieb",gl_mGTS1,0)

/*-------------------------------------------------------------------
/ Totem (10/1979)
/-------------------------------------------------------------------*/
INITGAME(totem, SNDBRD_GTS80S)
GTS1_1_ROMSTART(totem,"429.cpu", CRC(7885a384) SHA1(1770662af7d48ad8297097a9877c5c497119978d))
GTS80S1K_ROMSTART(    "429.snd", CRC(5d1b7ed4) SHA1(4a584f880e907fb21da78f3b3a0617f20599688f),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(totem,"Totem",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ The Incredible Hulk (10/1979)
/-------------------------------------------------------------------*/
INITGAME(hulk, SNDBRD_GTS80S)
GTS1_1_ROMSTART(hulk,"433.cpu", CRC(c05d2b52) SHA1(393fe063b029246317c90ee384db95a84d61dbb7))
GTS80S1K_ROMSTART(   "433.snd", CRC(20cd1dff) SHA1(93e7c47ff7051c3c0dc9f8f95aa33ba094e7cf25),
                "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(hulk,"Incredible Hulk, The",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Genie (11/1979)
/-------------------------------------------------------------------*/
INITGAME(genie, SNDBRD_GTS80S)
GTS1_1_ROMSTART(genie,"435.cpu", CRC(7749fd92) SHA1(9cd3e799842392e3939877bf295759c27f199e58))
GTS80S1K_ROMSTART(    "435.snd", CRC(4a98ceed) SHA1(f1d7548e03107033c39953ee04b043b5301dbb47),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(genie,"Genie",1979,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Buck Rogers (01/1980)
/-------------------------------------------------------------------*/
INITGAME(buckrgrs, SNDBRD_GTS80S)
GTS1_1_ROMSTART(buckrgrs,"437.cpu", CRC(e57d9278) SHA1(dfc4ebff1e14b9a074468671a8e5ac7948d5b352))
GTS80S1K_ROMSTART(       "437.snd", CRC(732b5a27) SHA1(7860ea54e75152246c3ac3205122d750b243b40c),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(buckrgrs,"Buck Rogers",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Torch (02/1980)
/-------------------------------------------------------------------*/
INITGAME(torch, SNDBRD_GTS80S)
GTS1_1_ROMSTART(torch,"438.cpu", CRC(2d396a64) SHA1(38a1862771500faa471071db08dfbadc6e8759e8))
GTS80S1K_ROMSTART(    "438.snd", CRC(a9619b48) SHA1(1906bc1b059bf31082e3b4546f5a30159479ad3c),
                 "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(torch,"Torch",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Roller Disco (02/1980)
/-------------------------------------------------------------------*/
INITGAME(roldisco, SNDBRD_GTS80S)
GTS1_1_ROMSTART(roldisco,"440.cpu", CRC(bc50631f) SHA1(6aa3124d09fc4e369d087a5ad6dd1737ace55e41))
GTS80S1K_ROMSTART(       "440.snd", CRC(4a0a05ae) SHA1(88f21b5638494d8e78dc0b6b7d69873b76b5f75d),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(roldisco,"Roller Disco",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ Asteroid Annie and the Aliens (12/1980)
/-------------------------------------------------------------------*/
INITGAME(astannie, SNDBRD_GTS80S)
GTS1_1_ROMSTART(astannie,"442.cpu", CRC(579521e0) SHA1(b1b19473e1ca3373955ee96104b87f586c4c311c))
GTS80S1K_ROMSTART(       "442.snd", CRC(c70195b4) SHA1(ff06197f07111d6a4b8942dcfe8d2279bda6f281),
                    "6530sys1.bin", CRC(b7831321) SHA1(c94f4bee97854d0373653a6867016e27d3fc1340))
GTS1_ROMEND
CORE_GAMEDEFNV(astannie,"Asteroid Annie and the Aliens",1980,"Gottlieb",gl_mGTS1S,0)

/*-------------------------------------------------------------------
/ System 1 Test prom
/-------------------------------------------------------------------*/
INITGAME(sys1test, SNDBRD_NONE)
GTS1_1_ROMSTART(sys1test,"test.cpu", CRC(8b0704bb) SHA1(5f0eb8d5af867b815b6012c9d078927398efe6d8))
GTS1_ROMEND
CORE_GAMEDEFNV(sys1test,"System 1 Test prom",19??,"Gottlieb",gl_mGTS1,0)
