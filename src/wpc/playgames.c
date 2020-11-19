#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "play.h"
#include "sndbrd.h"

#define GEN_PLAYMATIC 0

#ifdef MAME_DEBUG
#define GAME_STATUS 0
#else
#define GAME_STATUS GAME_IMPERFECT_SOUND
#endif

#define INITGAME1(name, disptype, balls) \
    PLAYMATIC1_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,-1,0,SNDBRD_PLAY1}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME2(name, disptype, balls, sb) \
    PLAYMATIC_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,sb}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME4(name, disptype, balls) \
  PLAYMATIC4_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
  static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_PLAY4}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
  }

#define INITGAME4FM(name, disptype, balls) \
  PLAYMATIC4_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
  static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_PLAY4,0,1}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
  }

#define INITGAME5(name, disptype, balls) \
  PLAYMATIC4_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
  static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_ZSU}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
  }

core_tLCDLayout play_dispOld[] = {
  { 0, 0, 0,2,CORE_SEG7 }, { 0, 4, 2,1,CORE_SEG8D}, { 0, 6, 3,3,CORE_SEG7 },
  { 3, 0, 6,2,CORE_SEG7 }, { 3, 4, 8,1,CORE_SEG8D}, { 3, 6, 9,1,CORE_SEG7 }, { 3, 8, 4,2,CORE_SEG7 },
  { 6, 0,10,2,CORE_SEG7 }, { 6, 4,12,1,CORE_SEG8D}, { 6, 6,13,1,CORE_SEG7 }, { 6, 8, 4,2,CORE_SEG7 },
  { 9, 0,14,2,CORE_SEG7 }, { 9, 4,16,1,CORE_SEG8D}, { 9, 6,17,1,CORE_SEG7 }, { 9, 8, 4,2,CORE_SEG7 },
  { 6,25,18,2,CORE_SEG7S}, { 4,24,20,3,CORE_SEG7S},
  {0}
};

core_tLCDLayout play_disp6[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 },
  { 3, 0,29,1,CORE_SEG7 }, { 3, 2,24,5,CORE_SEG7 },
  { 6, 0,21,1,CORE_SEG7 }, { 6, 2,16,5,CORE_SEG7 },
  { 9, 0,13,1,CORE_SEG7 }, { 9, 2, 8,5,CORE_SEG7 },
  {12, 0, 5,1,CORE_SEG7 }, {12, 2, 0,2,CORE_SEG7 }, {12, 8, 3,2,CORE_SEG7 },
  {0}
};

core_tLCDLayout play_disp7[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 }, { 0,12,52,1,CORE_SEG7 },
  { 6, 0,29,1,CORE_SEG7 }, { 6, 2,24,5,CORE_SEG7 }, { 6,12,51,1,CORE_SEG7 },
  { 0,20,21,1,CORE_SEG7 }, { 0,22,16,5,CORE_SEG7 }, { 0,32,50,1,CORE_SEG7 },
  { 6,20,13,1,CORE_SEG7 }, { 6,22, 8,5,CORE_SEG7 }, { 6,32,49,1,CORE_SEG7 },
  { 3,10, 5,1,CORE_SEG7 }, { 3,12, 0,1,CORE_SEG7 }, { 3,16, 2,1,CORE_SEG7 }, { 3,20, 4,1,CORE_SEG7 }, { 3,22,48,1,CORE_SEG7 },
  {0}
};

core_tLCDLayout play_disp7a[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 }, { 0,12,52,1,CORE_SEG7 },
  { 6, 0,29,1,CORE_SEG7 }, { 6, 2,24,5,CORE_SEG7 }, { 6,12,51,1,CORE_SEG7 },
  { 0,20,21,1,CORE_SEG7 }, { 0,22,16,5,CORE_SEG7 }, { 0,32,50,1,CORE_SEG7 },
  { 6,20,13,1,CORE_SEG7 }, { 6,22, 8,5,CORE_SEG7 }, { 6,32,49,1,CORE_SEG7 },
  { 3,10, 1,2,CORE_SEG7 }, { 3,16, 0,1,CORE_SEG7 }, { 3,20, 4,1,CORE_SEG7 }, { 3,22,48,1,CORE_SEG7 },
  {0}
};

/*-------------------------------------------------------------------
/ 03/78 Space Gambler
/-------------------------------------------------------------------*/
INITGAME1(spcgambl, play_dispOld, 1)
PLAYMATIC_ROMSTART88(spcgambl, "spcgamba.bin", CRC(3b6e5287) SHA1(4d2fae779bb4117a99a9311b96ab79799f40067b),
            "spcgambb.bin", CRC(5c61f25c) SHA1(44b2d74926bf5678146b6d2347b4147e8a29a660))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(spcgambl,"Space Gambler",1978,"Playmatic",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ 04/78 Big Town
/-------------------------------------------------------------------*/
INITGAME1(bigtown, play_dispOld, 1)
PLAYMATIC_ROMSTART88(bigtown, "bigtowna.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "bigtownb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(bigtown,"Big Town",1978,"Playmatic",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ 09/78 Last Lap // same ROMs as Big Town
/-------------------------------------------------------------------*/
INITGAME1(lastlap, play_dispOld, 1)
PLAYMATIC_ROMSTART88(lastlap, "bigtowna.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "bigtownb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(lastlap,"Last Lap",1978,"Playmatic",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ 09/78 Chance
/-------------------------------------------------------------------*/
INITGAME1(chance, play_dispOld, 1)
PLAYMATIC_ROMSTART884(chance, "chance_a.bin", CRC(3cd9d5a6) SHA1(c1d9488495a67198f7f60f70a889a9a3062c71d7),
            "chance_b.bin", CRC(a281b0f1) SHA1(1d2d26ce5f50294d5a95f688c82c3bdcec75de95),
            "chance_c.bin", CRC(369afee3) SHA1(7fa46c7f255a5ef21b0d1cc018056bc4889d68b8))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(chance,"Chance",1978,"Playmatic",gl_mPLAYMATIC1A,0)

/*-------------------------------------------------------------------
/ 05/79 Party // same ROMs as Big Town
/-------------------------------------------------------------------*/
INITGAME1(party, play_dispOld, 1)
PLAYMATIC_ROMSTART88(party, "bigtowna.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "bigtownb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(party,"Party",1979,"Playmatic",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ 11/79 Antar
/-------------------------------------------------------------------*/
INITGAME2(antar, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(antar, "antar08.bin", CRC(f6207f77) SHA1(f68ce967c6189457bd0ce8638e9c477f16e65763),
            "antar09.bin", CRC(2c954f1a) SHA1(fa83a5f1c269ea28d4eeff181f493cbb4dc9bc47),
            "antar10.bin", CRC(a6ce5667) SHA1(85ecd4fce94dc419e4c210262f867310b0889cd3),
            "antar11.bin", CRC(6474b17f) SHA1(e4325ceff820393b06eb2e8e4a85412b0d01a385))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(antar,"Antar",1979,"Playmatic",gl_mPLAYMATIC2,0)

INITGAME2(antar2, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(antar2,  "antar08.bin", CRC(f6207f77) SHA1(f68ce967c6189457bd0ce8638e9c477f16e65763),
            "antar09.bin", CRC(2c954f1a) SHA1(fa83a5f1c269ea28d4eeff181f493cbb4dc9bc47),
            "antar10a.bin", CRC(520eb401) SHA1(1d5e3f829a7e7f38c7c519c488e6b7e1a4d34321),
            "antar11a.bin", CRC(17ad38bf) SHA1(e2c9472ed8fbe9d5965a5c79515a1b7ea9edaa79))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(antar2,antar,"Antar (alternate set)",1979,"Playmatic",gl_mPLAYMATIC2,0)

/*-------------------------------------------------------------------
/ 03/80 Evil Fight
/-------------------------------------------------------------------*/
INITGAME2(evlfight, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(evlfight,  "evfg08.bin", CRC(2cc2e79a) SHA1(17440512c419b3bb2012539666a5f052f3cd8c1d),
            "evfg09.bin", CRC(5232dc4c) SHA1(6f95a578e9f09688e6ce8b0a622bcee887936c82),
            "evfg10.bin", CRC(de2f754d) SHA1(0287a9975095bcbf03ddb2b374ff25c080c8020f),
            "evfg11.bin", CRC(5eb8ac02) SHA1(31c80e74a4272becf7014aa96eaf7de555e26cd6))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(evlfight,"Evil Fight",1980,"Playmatic",gl_mPLAYMATIC2,0)

/*-------------------------------------------------------------------
/ 10/80 Attack
/-------------------------------------------------------------------*/
INITGAME2(attack, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(attack,  "attack8.bin", CRC(a5204b58) SHA1(afb4b81720f8d56e88f47fc842b23313824a1085),
            "attack9.bin", CRC(bbd086b4) SHA1(6fc94b94beea482d8c8f5b3c69d3f218e2b2dfc4),
            "attack10.bin", CRC(764925e4) SHA1(2f207ef87786d27d0d856c5816a570a59d89b718),
            "attack11.bin", CRC(972157b4) SHA1(23c90f23a34b34acfe445496a133b6022a749ccc))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(attack,"Attack",1980,"Playmatic",gl_mPLAYMATIC2,0)

/*-------------------------------------------------------------------
/ 12/80 Black Fever
/-------------------------------------------------------------------*/
INITGAME2(blkfever, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(blkfever,  "blackf8.bin", CRC(916b8ed8) SHA1(ddc7e09b68e3e1a033af5dc5ec32ab5b0922a833),
            "blackf9.bin", CRC(ecb72fdc) SHA1(d3598031b7170fab39727b3402b7053d4f9e1ca7),
            "blackf10.bin", CRC(b3fae788) SHA1(e14e09cc7da1098abf2f60f26a8ec507e123ff7c),
            "blackf11.bin", CRC(5a97c1b4) SHA1(b9d7eb0dd55ef6d959c0fab48f710e4b1c8d8003))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(blkfever,"Black Fever",1980,"Playmatic",gl_mPLAYMATIC2,0)

/*-------------------------------------------------------------------
/ ??/81 Zira
/-------------------------------------------------------------------*/
PLAYMATIC_INPUT_PORTS_START(zira, 1) PLAYMATIC_INPUT_PORTS_END
static core_tGameData ziraGameData = {GEN_PLAYMATIC,play_disp6,{FLIP_SW(FLIP_L),0,2,0,SNDBRD_PLAYZ}};
static void init_zira(void) {
  core_gameData = &ziraGameData;
}
PLAYMATIC_ROMSTART00(zira, "zira_u8.bin", CRC(53f8bf17) SHA1(5eb74f27bc65374a85dd44bbc8f6142488c226a2),
                "zira_u9.bin", CRC(d50a2419) SHA1(81b157f579a433389506817b1b6e02afaa2cf0d5))
PLAYMATIC_SOUNDROMZ("zira.snd", CRC(008cb743) SHA1(8e9677f08189638d669b265bb6943275a08ec8b4))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(zira,"Zira",1981,"Playmatic",gl_mPLAYMATIC2SZ,0)

/*-------------------------------------------------------------------
/ 03/82 Cerberus
/-------------------------------------------------------------------*/
INITGAME2(cerberus, play_disp6, 3, SNDBRD_PLAY3)
PLAYMATIC_ROMSTART000(cerberus, "cerb8.cpu", CRC(021d0452) SHA1(496010e6892311b1cabcdac62296cd6aa0782c5d),
                "cerb9.cpu", CRC(0fd41156) SHA1(95d1bf42c82f480825e3d907ae3c87b5f994fd2a),
                "cerb10.cpu", CRC(785602e0) SHA1(f38df3156cd14ab21752dbc849c654802079eb33))
PLAYMATIC_SOUNDROM64("cerb.snd", CRC(8af53a23) SHA1(a80b57576a1eb1b4544b718b9abba100531e3942))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(cerberus,"Cerberus",1982,"Playmatic",gl_mPLAYMATIC2S3,0)

/*-------------------------------------------------------------------
/ 10/82 Spain 82
/-------------------------------------------------------------------*/
INPUT_PORTS_START(spain82)
  CORE_PORTS
  SIM_PORTS(3)
  PLAYMATIC2_COMPORTS
INPUT_PORTS_END
static core_tGameData spain82GameData = {GEN_PLAYMATIC,play_disp6,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_PLAY3}};
static void init_spain82(void) {
  core_gameData = &spain82GameData;
}
PLAYMATIC_ROMSTART320(spain82,  "spaic12.bin", CRC(cd37ecdc) SHA1(ff2d406b6ac150daef868121e5857a956aabf005),
                "spaic11.bin", CRC(c86c0801) SHA1(1b52539538dae883f9c8fe5bc6454f9224780d11))
PLAYMATIC_SOUNDROM64("spasnd.bin", CRC(62412e2e) SHA1(9e48dc3295e78e1024f726906be6e8c3fe3e61b1))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(spain82,"Spain 82",1982,"Playmatic",gl_mPLAYMATIC3S3,0)

/*-------------------------------------------------------------------
/ Mad Race
/-------------------------------------------------------------------*/
INITGAME2(madrace, play_disp6, 1, SNDBRD_PLAY4)
PLAYMATIC_ROMSTART000(madrace,  "madrace.2a0", CRC(ab487c79) SHA1(a5df29b2af4c9d94d8bf54c5c91d1e9b5ca4d065),
                "madrace.2b0", CRC(dcb54b39) SHA1(8e2ca7180f5ea3a28feb34b01f3387b523dbfa3b),
                "madrace.2c0", CRC(b24ea245) SHA1(3f868ccbc4bfb77c40c4cc05dcd8eeca85ecd76f))
PLAYMATIC_SOUNDROM6416( "madrace1.snd", CRC(49e956a5) SHA1(8790cc27a0fda7b8e07bee65109874140b4018a2),
            "madrace2.snd", CRC(c19283d3) SHA1(42f9770c46030ef20a80cc94fdbe6548772aa525))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(madrace,"Mad Race",198?,"Playmatic",gl_mPLAYMATIC2S4,0)

/*-------------------------------------------------------------------
/ 04/84 Meg-Aaton
/-------------------------------------------------------------------*/
PLAYMATIC3_INPUT_PORTS_START(megaaton, 3) PLAYMATIC_INPUT_PORTS_END
static core_tGameData megaatonGameData = {GEN_PLAYMATIC,play_disp7,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_PLAY4}};
static void init_megaaton(void) {
  core_gameData = &megaatonGameData;
}
PLAYMATIC_ROMSTART64(megaaton,  "cpumegat.bin", CRC(7e7a4ede) SHA1(3194b367cbbf6e0cb2629cd5d82ddee6fe36985a))
PLAYMATIC_SOUNDROM6432( "smogot.bin", CRC(fefc3ab2) SHA1(e748d9b443a69fcdd587f22c87d41818b6c0e436),
            "smegat.bin", CRC(910ab7fe) SHA1(0ddfd15c9c25f43b8fcfc4e11bc8ea180f6bd761))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(megaaton,"Meg-Aaton",1984,"Playmatic",gl_mPLAYMATIC3S4,GAME_NOT_WORKING)

#define init_megaatoa init_megaaton
#define input_ports_megaatoa input_ports_megaaton
PLAYMATIC_ROMSTART3232(megaatoa, "mega_u12.bin", CRC(65761b02) SHA1(dd9586eaf70698ef7a80ce1be293322f64829aea),
                "mega_u11.bin", CRC(513f3683) SHA1(0f080a33426df1ffdb14e9b2e6382304e201e335))
PLAYMATIC_SOUNDROM6432( "smogot.bin", CRC(fefc3ab2) SHA1(e748d9b443a69fcdd587f22c87d41818b6c0e436),
            "smegat.bin", CRC(910ab7fe) SHA1(0ddfd15c9c25f43b8fcfc4e11bc8ea180f6bd761))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(megaatoa,megaaton,"Meg-Aaton (alternate set)",1984,"Playmatic",gl_mPLAYMATIC3S4,0)

/*-------------------------------------------------------------------
/ ??/84 Nautilus
/-------------------------------------------------------------------*/
INITGAME4(nautilus, play_disp7, 1)
PLAYMATIC_ROMSTART64(nautilus, "nautilus.cpu", CRC(197e5492) SHA1(0f83fc2e742fd0cca0bd162add4bef68c6620067))
PLAYMATIC_SOUNDROM64("nautilus.snd", CRC(413d110f) SHA1(8360f652296c46339a70861efb34c41e92b25d0e))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(nautilus,"Nautilus",1984,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/84 The Raid
/-------------------------------------------------------------------*/
INITGAME4(theraid, play_disp7, 1)
PLAYMATIC_ROMSTART64(theraid, "theraid.cpu", CRC(97aa1489) SHA1(6b691b287138cc78cfc1010f380ff8c66342c39b))
PLAYMATIC_SOUNDROM64("theraid.snd", CRC(e33f8363) SHA1(e7f251c334b15e12b1eb7e079c2e9a5f64338052))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(theraid,"Raid, The",1984,"Playmatic",gl_mPLAYMATIC4,0)

INITGAME4(theraida, play_disp7, 1)
PLAYMATIC_ROMSTART64(theraida, "ph_6_1a0.u13", CRC(cc2b1872) SHA1(e61071450cc6b0fa5e6297f75bca0391039dca10))
PLAYMATIC_SOUNDROM64("theraid.snd", CRC(e33f8363) SHA1(e7f251c334b15e12b1eb7e079c2e9a5f64338052))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(theraida,theraid,"Raid, The (alternate set)",1984,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ 11/84 UFO-X
/-------------------------------------------------------------------*/
INITGAME4(ufo_x, play_disp7, 1)
PLAYMATIC_ROMSTART64(ufo_x,"ufoxcpu.rom", CRC(cf0f7c52) SHA1(ce52da05b310ac84bdd57609e21b0401ee3a2564))
PLAYMATIC_SOUNDROM6416("ufoxu3.rom", CRC(6ebd8ee1) SHA1(83522b76a755556fd38d7b292273b4c68bfc0ddf),
            "ufoxu4.rom", CRC(aa54ede6) SHA1(7dd7e2852d42aa0f971936dbb84c7708727ce0e7))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(ufo_x,"UFO-X",1984,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/85 KZ-26
/-------------------------------------------------------------------*/
INITGAME4(kz26, play_disp7, 1)
PLAYMATIC_ROMSTART64(kz26,  "kz26.cpu", CRC(8030a699) SHA1(4f86b325801d8ce16011f7b6ba2f3633e2f2af35))
PLAYMATIC_SOUNDROM6416( "sound1.su3", CRC(8ad1a804) SHA1(6177619f09af4302ffddd8c0c1b374dab7f47e91),
            "sound2.su4", CRC(355dc9ad) SHA1(eac8bc27157afd908f9bc5b5a7c40be5b9427269))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(kz26,"KZ-26",1985,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/85 Rock 2500
/-------------------------------------------------------------------*/
INITGAME4(rock2500, play_disp7, 1)
PLAYMATIC_ROMSTART64(rock2500,"r2500cpu.rom", CRC(9c07e373) SHA1(5bd4e69d11e69fdb911a6e65b3d0a7192075abc8))
PLAYMATIC_SOUNDROM64("r2500snd.rom", CRC(24fbaeae) SHA1(20ff35ed689291f321e483287a977c02e84d4524))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(rock2500,"Rock 2500",1985,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/85 Star Fire
/-------------------------------------------------------------------*/
INITGAME4(starfire, play_disp7, 1)
PLAYMATIC_ROMSTART64(starfire,"starfcpu.rom", CRC(450ddf20) SHA1(c63c4e3833ffc1f69fcec39bafecae9c80befb2a))
PLAYMATIC_SOUNDROM6416("starfu3.rom", CRC(5d602d80) SHA1(19d21adbcbd0067c051f3033468eda8c5af57be1),
            "starfu4.rom", CRC(9af8be9a) SHA1(da6db3716db73baf8e1493aba91d4d85c5d613b4))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(starfire,"Star Fire",1985,"Playmatic",gl_mPLAYMATIC4,0)

INITGAME4(starfira, play_disp7, 1)
PLAYMATIC_ROMSTART64(starfira,"starcpua.rom", CRC(29bac350) SHA1(ab3e3ea4881be954f7fa7278800ffd791c4581da))
PLAYMATIC_SOUNDROM6416("starfu3.rom", CRC(5d602d80) SHA1(19d21adbcbd0067c051f3033468eda8c5af57be1),
            "starfu4.rom", CRC(9af8be9a) SHA1(da6db3716db73baf8e1493aba91d4d85c5d613b4))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(starfira,starfire,"Star Fire (alternate set)",1985,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/85 Trailer
/-------------------------------------------------------------------*/
INITGAME4(trailer, play_disp7, 1)
PLAYMATIC_ROMSTART64(trailer,"trcpu.rom", CRC(cc81f84d) SHA1(7a3282a47de271fde84cfddbaceb118add0df116))
PLAYMATIC_SOUNDROM6416("trsndu3.rom", CRC(05975c29) SHA1(e54d3a5613c3e39fc0338a53dbadc2e91c09ffe3),
            "trsndu4.rom", CRC(bda2a735) SHA1(134b5abb813ed8bf2eeac0861b4c88c7176582d8))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(trailer,"Trailer",1985,"Playmatic",gl_mPLAYMATIC4,0)

// ??/85 Stop Ship

/*-------------------------------------------------------------------
/ ??/86 Flash Dragon
/-------------------------------------------------------------------*/
INITGAME4(fldragon, play_disp7, 1)
PLAYMATIC_ROMSTART64_2(fldragon,"fldrcpu1.rom", CRC(e513ded0) SHA1(64ed3dcff53311fb93bd50d105a4c1186043fdd7),
            "fldrcpu2.rom", CRC(6ff2b276) SHA1(040b614f0b0587521ef5550b5587b94a7f3f178b))
PLAYMATIC_SOUNDROM6416("fdsndu3.rom", CRC(aa9c52a8) SHA1(97d5d63b14d10c70a5eb80c08ccf5a1f3df7596d),
            "fdsndu4.rom", CRC(0a7dc1d2) SHA1(32c7be5e9fbe4fa9ca661af7b7b5ea13ef250ce6))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(fldragon,"Flash Dragon",1986,"Playmatic",gl_mPLAYMATIC4,0)

INITGAME4(fldragoa, play_disp7, 1)
PLAYMATIC_ROMSTART64_2(fldragoa,"fldr_1a.cpu", CRC(21fda8e8) SHA1(feea608c2605cea1cdf9f7ed884297a95993f754),
            "fldr_2a.cpu", CRC(3592a0b7) SHA1(4c4ed7930dcbbf81ce2e5296c0b36bb615bd2270))
PLAYMATIC_SOUNDROM6416("fdsndu3.rom", CRC(aa9c52a8) SHA1(97d5d63b14d10c70a5eb80c08ccf5a1f3df7596d),
            "fdsndu4.rom", CRC(0a7dc1d2) SHA1(32c7be5e9fbe4fa9ca661af7b7b5ea13ef250ce6))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(fldragoa,fldragon,"Flash Dragon (alternate set)",1986,"Playmatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/87 Phantom Ship
/-------------------------------------------------------------------*/
INITGAME5(phntmshp, play_disp7, 1)
PLAYMATIC_ROMSTART64_2(phntmshp,"video1.bin", CRC(2b61a8d2) SHA1(1b5cabbab252b2ffb6ed12fb7e4181de7695ed9a),
            "video2.bin", CRC(50126db1) SHA1(58d89e44131554cb087c4cad62869f90366704ad))
PLAYMATIC_SOUNDROM256x4("sonido1.bin", CRC(3294611d) SHA1(5f790b41bcb6d87418c80e61ac8ae69c57864b1d),
            "sonido2.bin", CRC(c2efc826) SHA1(44ee144b902627745853011968e0d654b35b3b08),
            "sonido3.bin", CRC(13d50f39) SHA1(70624de2dd8412c83866183a83f16cc5b8bdccb8),
            "sonido4.bin", CRC(b53f73ed) SHA1(bb928cfee418e8d9698d7bee78a32426f793c6e9))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(phntmshp,"Phantom Ship",1987,"Playmatic",gl_mPLAYMATIC4SZSU,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/87 Skill Flight
/-------------------------------------------------------------------*/
INITGAME5(sklflite, play_disp7a, 1)
PLAYMATIC_ROMSTART64_2(sklflite,"skflcpu1.rom", CRC(8f833b55) SHA1(1729203582c22b51d1cc401aa8f270aa5cdadabe),
            "skflcpu2.rom", CRC(ffc497aa) SHA1(3e88539ae1688322b9268f502d8ca41cffb28df3))
PLAYMATIC_SOUNDROM256("skflsnd.rom", CRC(926a1da9) SHA1(16c762fbfe6a55597f26ff55d380192bb8647ee0))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(sklflite,"Skill Flight (Playmatic)",1987,"Playmatic",gl_mPLAYMATIC4SZSU,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/?? Miss Disco (Bingo machine)
/-------------------------------------------------------------------*/
core_tLCDLayout play_disp_bingo[] = {
  {0} // no digits
};
INPUT_PORTS_START(msdisco) CORE_PORTS SIM_PORTS(1)
  PORT_START
    COREPORT_BIT(0x0001, "Key 1", KEYCODE_1)
    COREPORT_BIT(0x0002, "Key 2", KEYCODE_2)
    COREPORT_BIT(0x0004, "Key 3", KEYCODE_3)
    COREPORT_BIT(0x0008, "Key 4", KEYCODE_4)
    COREPORT_BIT(0x0010, "Key 5", KEYCODE_5)
    COREPORT_BIT(0x0020, "Key 6", KEYCODE_6)
    COREPORT_BIT(0x0040, "Key 7", KEYCODE_7)
    COREPORT_BIT(0x0080, "Key 8", KEYCODE_8)
  PORT_START
    COREPORT_DIPNAME( 0x0001, 0x0000, "EF1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0001, "1" )
    COREPORT_DIPNAME( 0x0002, 0x0000, "EF2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0002, "1" )
    COREPORT_DIPNAME( 0x0004, 0x0000, "EF3")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0004, "1" )
    COREPORT_DIPNAME( 0x0008, 0x0000, "EF4")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0008, "1" )
INPUT_PORTS_END
static core_tGameData msdiscoGameData = {GEN_PLAYMATIC,play_disp_bingo,{FLIP_SW(FLIP_L),0,17}};
static void init_msdisco(void) {
  core_gameData = &msdiscoGameData;
}
ROM_START(msdisco)
  NORMALREGION(0x10000, PLAYMATIC_MEMREG_CPU)
    ROM_LOAD("1.bin", 0x0000, 0x1000, CRC(06fb7da9) SHA1(36c6fda166b2a07a5ed9ad5d2b6fdfe8fd707b0f))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(msdisco,"Miss Disco (Bingo)",19??,"Playmatic",gl_mPLAYMATICBINGO,GAME_NOT_WORKING)


// games by other manufacturers below

/*-------------------------------------------------------------------
/ ??/78 Third World (Sonic) // same ROMs as Big Town
/-------------------------------------------------------------------*/
INITGAME1(thrdwrld, play_dispOld, 1)
PLAYMATIC_ROMSTART88(thrdwrld, "bigtowna.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "bigtownb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(thrdwrld,"Third World",1978,"Sonic (Spain)",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ ??/79 Night Fever (Sonic) // same ROMs as Big Town
/-------------------------------------------------------------------*/
INITGAME1(ngtfever, play_dispOld, 1)
PLAYMATIC_ROMSTART88(ngtfever, "bigtowna.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "bigtownb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(ngtfever,"Night Fever",1979,"Sonic (Spain)",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ ??/79 Storm (Sonic)
/-------------------------------------------------------------------*/
INITGAME2(storm, play_disp6, 1, SNDBRD_PLAY2)
PLAYMATIC_ROMSTART8888(storm, "a-1.bin", CRC(12e37664) SHA1(d7095975cd9d4445fd1f4cd711992c7367deae89),
                              "b-1.bin", CRC(3ac3cea3) SHA1(c6197911d25661cb647ea606eee5f3f1bd9b4ba2),
                              "c-1.bin", CRC(8bedf1ea) SHA1(7633ebf8a65e3fc7afa21d50aaa441f87a86efd3),
                              "d-1.bin", CRC(f717ef3e) SHA1(cd5126360471c06539e445fecbf2f0ddeb1b156c))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(storm,"Storm",1979,"Sonic (Spain)",gl_mPLAYMATIC2,0)

/*-------------------------------------------------------------------
/ ??/84 Flashman (Sport Matic)
/-------------------------------------------------------------------*/
core_tLCDLayout dispFM[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 }, { 0,12,52,1,CORE_SEG7 },
  { 0,18,29,1,CORE_SEG7 }, { 0,20,24,5,CORE_SEG7 }, { 0,30,51,1,CORE_SEG7 },
  { 3,18,21,1,CORE_SEG7 }, { 3,20,16,5,CORE_SEG7 }, { 3,30,50,1,CORE_SEG7 },
  { 6,18,13,1,CORE_SEG7 }, { 6,20, 8,5,CORE_SEG7 }, { 6,30,49,1,CORE_SEG7 },
  {10, 0, 5,1,CORE_SEG7 }, {10, 2, 0,1,CORE_SEG7 }, {10, 5, 1,1,CORE_SEG7 }, {10, 8, 2,1,CORE_SEG7 }, {10,11, 3,2,CORE_SEG7 },
  {0}
};
INITGAME4FM(flashman, dispFM, 1)
PLAYMATIC_ROMSTART64(flashman,"pf7-1a0.u9", CRC(2cd16521) SHA1(bf9aa293e2ded3f5b1e61a10e6a8ebb8b4e9d4e1))
PLAYMATIC_SOUNDROM6416("mfm-1a0.u3", CRC(456fd555) SHA1(e91d6df15fdfc330ee9edb691ff925ad24afea35),
            "mfm-1b0.u4", CRC(90256257) SHA1(c7f2554e500c4e512999b4edc54c86f3335a2b30))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(flashman,"Flashman",1984,"Sport Matic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/86 Rider's Surf (JocMatic)
/-------------------------------------------------------------------*/
INITGAME4(ridersrf, play_disp7, 1)
PLAYMATIC_ROMSTART64(ridersrf,"cpu.bin", CRC(4941938e) SHA1(01e44054e65166d68602d6a38217eda7ea669761))
PLAYMATIC_SOUNDROM64("sound.bin", CRC(2db2ecb2) SHA1(365fcac208607acc3e134affeababd6c89dbc74d))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(ridersrf,"Rider's Surf",1986,"JocMatic",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/87 Iron Balls (Stargame)
/-------------------------------------------------------------------*/
INITGAME4(ironball, play_disp7, 1)
PLAYMATIC_ROMSTART64(ironball,"video.bin", CRC(1867ebff) SHA1(485e46c742d914febcbdd58cb5a886f1d773282a))
PLAYMATIC_SOUNDROM64("sound.bin", CRC(83165483) SHA1(5076e5e836105d69c4ba606d8b995ecb16f88504))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(ironball,"Iron Balls",1987,"Stargame",gl_mPLAYMATIC4,0)

/*-------------------------------------------------------------------
/ ??/87 Terrific Lake (Sport Matic)
/-------------------------------------------------------------------*/
core_tLCDLayout dispSM[] = {
  { 0, 0,37,1,CORE_SEG7 }, { 0, 2,32,5,CORE_SEG7 }, { 0,12,52,1,CORE_SEG7 },
  { 3, 0,29,1,CORE_SEG7 }, { 3, 2,24,5,CORE_SEG7 }, { 3,12,51,1,CORE_SEG7 },
  { 6, 0,21,1,CORE_SEG7 }, { 6, 2,16,5,CORE_SEG7 }, { 6,12,50,1,CORE_SEG7 },
  { 3,20,13,1,CORE_SEG7 }, { 3,22, 8,5,CORE_SEG7 }, { 3,32,49,1,CORE_SEG7 },
  {10, 0, 5,1,CORE_SEG7 }, {10, 2, 0,1,CORE_SEG7 }, {10, 5, 1,1,CORE_SEG7 }, {10, 8, 2,1,CORE_SEG7 }, {10,11, 3,2,CORE_SEG7 },
  {0}
};
INITGAME5(terrlake, dispSM, 1)
PLAYMATIC_ROMSTART64(terrlake,"jtl_2a3.u9", CRC(f6d3cedd) SHA1(31e0daac1e9215ad0e1557d31d520745ead0f396))
SOUNDREGION(0x10000, REGION_CPU2)
  ROM_LOAD("stl_1a0.u3", 0x0000, 0x8000, CRC(b5afdc39) SHA1(fb74de453dfc66b87f3d64508802b3de46d14631))
SOUNDREGION(0x20000, REGION_USER1)
  ROM_LOAD("stl_1b0.u4", 0x00000, 0x8000, CRC(3bbdd791) SHA1(68cd86cb96a278538d18ca0a77b372309829edf4))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(terrlake,"Terrific Lake",1987,"Sport Matic",gl_mPLAYMATIC4SZSU,GAME_STATUS)
