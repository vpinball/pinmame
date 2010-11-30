#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "play.h"

#define GEN_PLAYMATIC 0

#ifdef MAME_DEBUG
#define GAME_STATUS 0
#else
#define GAME_STATUS GAME_IMPERFECT_SOUND
#endif

#define INITGAME1(name, disptype, balls) \
    PLAYMATIC1_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,-1}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME2(name, disptype, balls, sndEn) \
    PLAYMATIC_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
    static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,0,0,sndEn}}; \
    static void init_##name(void) { \
        core_gameData = &name##GameData; \
    }

#define INITGAME3(name, disptype, balls, sndEn) \
  PLAYMATIC3_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
  static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L),0,0,0,0,0,sndEn}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
  }

#define INITGAME4(name, disptype, balls) \
  PLAYMATIC4_INPUT_PORTS_START(name, balls) PLAYMATIC_INPUT_PORTS_END \
  static core_tGameData name##GameData = {GEN_PLAYMATIC,disptype,{FLIP_SW(FLIP_L)}}; \
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

/*-------------------------------------------------------------------
/ 03/78 Space Gambler
/-------------------------------------------------------------------*/
INITGAME1(spcgambl, play_dispOld, 1)
PLAYMATIC_ROMSTART88(spcgambl, "spcgamba.bin", CRC(da312699) SHA1(b1fabfe23d4e5d512623f9b0f3fa8dd38bdbba43),
            "spcgambb.bin", NO_DUMP)
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
/ 09/78 Last Lap
/-------------------------------------------------------------------*/
INITGAME1(lastlap, play_dispOld, 1)
PLAYMATIC_ROMSTART88(lastlap, "lastlapa.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "lastlapb.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
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
/ 05/79 Party
/-------------------------------------------------------------------*/
INITGAME1(party, play_dispOld, 1)
PLAYMATIC_ROMSTART88(party, "party_a.bin", CRC(253f1b93) SHA1(7ff5267d0dfe6ae19ec6b0412902f4ce83f23ed1),
            "party_b.bin", CRC(5e2ba9c0) SHA1(abd285aa5702c7fb84257b4341f64ff83c1fc0ce))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(party,"Party",1979,"Playmatic",gl_mPLAYMATIC1,0)

/*-------------------------------------------------------------------
/ 11/79 Antar
/-------------------------------------------------------------------*/
INITGAME2(antar, play_disp6, 1, 0)
PLAYMATIC_ROMSTART8888(antar, "antar08.bin", CRC(f6207f77) SHA1(f68ce967c6189457bd0ce8638e9c477f16e65763),
            "antar09.bin", CRC(2c954f1a) SHA1(fa83a5f1c269ea28d4eeff181f493cbb4dc9bc47),
            "antar10.bin", CRC(a6ce5667) SHA1(85ecd4fce94dc419e4c210262f867310b0889cd3),
            "antar11.bin", CRC(6474b17f) SHA1(e4325ceff820393b06eb2e8e4a85412b0d01a385))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(antar,"Antar",1979,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

INITGAME2(antar2, play_disp6, 1, 0)
PLAYMATIC_ROMSTART8888(antar2,  "antar08.bin", CRC(f6207f77) SHA1(f68ce967c6189457bd0ce8638e9c477f16e65763),
            "antar09.bin", CRC(2c954f1a) SHA1(fa83a5f1c269ea28d4eeff181f493cbb4dc9bc47),
            "antar10a.bin", CRC(520eb401) SHA1(1d5e3f829a7e7f38c7c519c488e6b7e1a4d34321),
            "antar11a.bin", CRC(17ad38bf) SHA1(e2c9472ed8fbe9d5965a5c79515a1b7ea9edaa79))
PLAYMATIC_ROMEND
CORE_CLONEDEFNV(antar2,antar,"Antar (alternate set)",1979,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

/*-------------------------------------------------------------------
/ 03/80 Evil Fight
/-------------------------------------------------------------------*/
INITGAME2(evlfight, play_disp6, 1, 0)
PLAYMATIC_ROMSTART8888(evlfight,  "evfg08.bin", CRC(2cc2e79a) SHA1(17440512c419b3bb2012539666a5f052f3cd8c1d),
            "evfg09.bin", CRC(5232dc4c) SHA1(6f95a578e9f09688e6ce8b0a622bcee887936c82),
            "evfg10.bin", CRC(de2f754d) SHA1(0287a9975095bcbf03ddb2b374ff25c080c8020f),
            "evfg11.bin", CRC(5eb8ac02) SHA1(31c80e74a4272becf7014aa96eaf7de555e26cd6))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(evlfight,"Evil Fight",1980,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

/*-------------------------------------------------------------------
/ 10/80 Attack
/-------------------------------------------------------------------*/
INITGAME2(attack, play_disp6, 1, 0)
PLAYMATIC_ROMSTART8888(attack,  "attack8.bin", CRC(a5204b58) SHA1(afb4b81720f8d56e88f47fc842b23313824a1085),
            "attack9.bin", CRC(bbd086b4) SHA1(6fc94b94beea482d8c8f5b3c69d3f218e2b2dfc4),
            "attack10.bin", CRC(764925e4) SHA1(2f207ef87786d27d0d856c5816a570a59d89b718),
            "attack11.bin", CRC(972157b4) SHA1(23c90f23a34b34acfe445496a133b6022a749ccc))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(attack,"Attack",1980,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

/*-------------------------------------------------------------------
/ 12/80 Black Fever
/-------------------------------------------------------------------*/
INITGAME2(blkfever, play_disp6, 1, 0)
PLAYMATIC_ROMSTART8888(blkfever,  "blackf8.bin", CRC(916b8ed8) SHA1(ddc7e09b68e3e1a033af5dc5ec32ab5b0922a833),
            "blackf9.bin", CRC(ecb72fdc) SHA1(d3598031b7170fab39727b3402b7053d4f9e1ca7),
            "blackf10.bin", CRC(b3fae788) SHA1(e14e09cc7da1098abf2f60f26a8ec507e123ff7c),
            "blackf11.bin", CRC(5a97c1b4) SHA1(b9d7eb0dd55ef6d959c0fab48f710e4b1c8d8003))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(blkfever,"Black Fever",1980,"Playmatic",gl_mPLAYMATIC2,GAME_STATUS)

// ??/80 Zira

/*-------------------------------------------------------------------
/ 03/82 Cerberus
/-------------------------------------------------------------------*/
INITGAME2(cerberus, play_disp6, 1, 1)
PLAYMATIC_ROMSTART000(cerberus, "cerb8.cpu", CRC(021d0452) SHA1(496010e6892311b1cabcdac62296cd6aa0782c5d),
                "cerb9.cpu", CRC(0fd41156) SHA1(95d1bf42c82f480825e3d907ae3c87b5f994fd2a),
                "cerb10.cpu", CRC(785602e0) SHA1(f38df3156cd14ab21752dbc849c654802079eb33))
PLAYMATIC_SOUNDROM64("cerb.snd", CRC(8af53a23) SHA1(a80b57576a1eb1b4544b718b9abba100531e3942))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(cerberus,"Cerberus",1982,"Playmatic",gl_mPLAYMATIC2S,GAME_STATUS)

/*-------------------------------------------------------------------
/ Mad Race
/-------------------------------------------------------------------*/
INITGAME2(madrace, play_disp6, 1, 1)
PLAYMATIC_ROMSTART000(madrace,  "madrace.2a0", CRC(ab487c79) SHA1(a5df29b2af4c9d94d8bf54c5c91d1e9b5ca4d065),
                "madrace.2b0", CRC(dcb54b39) SHA1(8e2ca7180f5ea3a28feb34b01f3387b523dbfa3b),
                "madrace.2c0", CRC(b24ea245) SHA1(3f868ccbc4bfb77c40c4cc05dcd8eeca85ecd76f))
PLAYMATIC_SOUNDROM6416( "madrace1.snd", CRC(49e956a5) SHA1(8790cc27a0fda7b8e07bee65109874140b4018a2),
            "madrace2.snd", CRC(c19283d3) SHA1(42f9770c46030ef20a80cc94fdbe6548772aa525))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(madrace,"Mad Race",198?,"Playmatic",gl_mPLAYMATIC2S,GAME_STATUS)

/*-------------------------------------------------------------------
/ 10/82 Spain '82
/-------------------------------------------------------------------*/
INITGAME3(spain82, play_disp6, 1, 1)
PLAYMATIC_ROMSTART320(spain82,  "spaic12.bin", CRC(cd37ecdc) SHA1(ff2d406b6ac150daef868121e5857a956aabf005),
                "spaic11.bin", CRC(c86c0801) SHA1(1b52539538dae883f9c8fe5bc6454f9224780d11))
PLAYMATIC_SOUNDROM64("spasnd.bin", CRC(62412e2e) SHA1(9e48dc3295e78e1024f726906be6e8c3fe3e61b1))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(spain82,"Spain '82",1982,"Playmatic",gl_mPLAYMATIC3S,GAME_STATUS)

/*-------------------------------------------------------------------
/ 04/84 Meg-Aaton
/-------------------------------------------------------------------*/
INITGAME3(megaaton, play_disp7, 1, 0)
PLAYMATIC_ROMSTART64(megaaton,  "cpumegat.bin", CRC(7e7a4ede) SHA1(3194b367cbbf6e0cb2629cd5d82ddee6fe36985a))
PLAYMATIC_SOUNDROM6432( "smogot.bin", CRC(92fa0742) SHA1(ef3100a53323fd67e23b47fc3e72fdb4671e9b0a),
            "smegat.bin", CRC(910ab7fe) SHA1(0ddfd15c9c25f43b8fcfc4e11bc8ea180f6bd761))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(megaaton,"Meg-Aaton",1984,"Playmatic",gl_mPLAYMATIC3S,GAME_STATUS)

// ??/84 Nautilus
// ??/84 The Raid

/*-------------------------------------------------------------------
/ 11/84 UFO-X
/-------------------------------------------------------------------*/
INITGAME4(ufo_x, play_disp7, 1)
PLAYMATIC_ROMSTART64(ufo_x,"ufoxcpu.rom", CRC(cf0f7c52) SHA1(ce52da05b310ac84bdd57609e21b0401ee3a2564))
PLAYMATIC_SOUNDROM6416("ufoxu3.rom", CRC(6ebd8ee1) SHA1(83522b76a755556fd38d7b292273b4c68bfc0ddf),
            "ufoxu4.rom", CRC(aa54ede6) SHA1(7dd7e2852d42aa0f971936dbb84c7708727ce0e7))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(ufo_x,"UFO-X",1984,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/85 KZ-26
/-------------------------------------------------------------------*/
INITGAME4(kz26, play_disp7, 1)
PLAYMATIC_ROMSTART64(kz26,  "kz26.cpu", CRC(8030a699) SHA1(4f86b325801d8ce16011f7b6ba2f3633e2f2af35))
PLAYMATIC_SOUNDROM6416( "sound1.su3", CRC(f9550ab4) SHA1(7186158f515fd9fbe5a7a09c6b7d2e8dfc3b4bb2),
            "sound2.su4", CRC(355dc9ad) SHA1(eac8bc27157afd908f9bc5b5a7c40be5b9427269))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(kz26,"KZ-26",1985,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/85 Rock 2500
/-------------------------------------------------------------------*/
INITGAME4(rock2500, play_disp7, 1)
PLAYMATIC_ROMSTART64(rock2500,"r2500cpu.rom", CRC(9c07e373) SHA1(5bd4e69d11e69fdb911a6e65b3d0a7192075abc8))
PLAYMATIC_SOUNDROM64("r2500snd.rom", CRC(24fbaeae) SHA1(20ff35ed689291f321e483287a977c02e84d4524))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(rock2500,"Rock 2500",1985,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/85 Star Fire
/-------------------------------------------------------------------*/
INITGAME4(starfire, play_disp7, 1)
PLAYMATIC_ROMSTART64(starfire,"starfcpu.rom", CRC(450ddf20) SHA1(c63c4e3833ffc1f69fcec39bafecae9c80befb2a))
PLAYMATIC_SOUNDROM6416("starfu3.rom", CRC(5d602d80) SHA1(19d21adbcbd0067c051f3033468eda8c5af57be1),
            "starfu4.rom", CRC(9af8be9a) SHA1(da6db3716db73baf8e1493aba91d4d85c5d613b4))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(starfire,"Star Fire",1985,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)

/*-------------------------------------------------------------------
/ ??/85 Trailer
/-------------------------------------------------------------------*/
INITGAME4(trailer, play_disp7, 1)
PLAYMATIC_ROMSTART64(trailer,"trcpu.rom", CRC(cc81f84d) SHA1(7a3282a47de271fde84cfddbaceb118add0df116))
PLAYMATIC_SOUNDROM6416("trsndu3.rom", CRC(05975c29) SHA1(e54d3a5613c3e39fc0338a53dbadc2e91c09ffe3),
            "trsndu4.rom", CRC(bda2a735) SHA1(134b5abb813ed8bf2eeac0861b4c88c7176582d8))
PLAYMATIC_ROMEND
CORE_GAMEDEFNV(trailer,"Trailer",1985,"Playmatic",gl_mPLAYMATIC4S,GAME_STATUS)

// ??/85 Stop Ship
// ??/86 Flash Dragon
// ??/87 Phantom Ship
// ??/87 Skill Flight
