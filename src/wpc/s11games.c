#include "driver.h"
#include "sim.h"
#include "core.h"
#include "s11csoun.h"
#include "mech.h"
#include "s11.h"

//220201: Moved Millionaire to own Sim File (SJE)
//140401: Corrected A/C solenoid for F14, PB & Fire!
//150401: Corrected Radical sound ROMs
/*
Issues:
Road Kings: Main CPU
Jokerz: No soundboard
Pool: Sound loops on startup
Police: Display mess (don't know how it should look)
*/
#define INITGAME(name, gen, disp, mux, flip, balls) \
static core_tGameData name##GameData = { \
  gen, disp, {flip}, NULL, {{0}}, {mux} }; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S11_INPUT_PORTS_START(name, balls) S11_INPUT_PORTS_END

/*--------------------
/ Space Shuttle (S9) 12/84
/--------------------*/
static core_tLCDLayout dispSshtl[] = { \
	{  0, 0,17,7, CORE_SEG87},
	{  2, 0,25,7, CORE_SEG87},
	{  4, 0, 1,7, CORE_SEG87}, { 4, 26,9,7,CORE_SEG87},
	{  5,18, 0,1, CORE_SEG7},{5,20, 8,1,CORE_SEG7},
	{  5,23,16,1, CORE_SEG7},{5,25,24,1,CORE_SEG7},{0}
};
static core_tGameData sshtlGameData = {
  GEN_S9, dispSshtl,
  {FLIP_SWNO(0,S11_SWNO(41))},
  NULL,
  {{0}},
  {0, {S11_SWNO(39),S11_SWNO(40), S11_SWNO(27), S11_SWNO(26), S11_SWNO(25), 0}}
};
static void init_sshtl(void) {
  core_gameData = &sshtlGameData;
}
S11_INPUT_PORTS_START(sshtl, 3) S11_INPUT_PORTS_END
S9_ROMSTARTx4(sshtl,l7,"cpu_u20.128", 0x848ad54c)
S9S_SOUNDROM4111(      "cpu_u49.128", 0x20e77990,
                       "spch_u4.732", 0xb0d03c5e,
                       "spch_u5.732", 0x13edd4e5,
                       "spch_u6.732", 0xcf48b2e7)
S9_ROMEND
CORE_GAMEDEF(sshtl, l7, "Space Shuttle (L-7)", 1984, "Williams", s9_mS9S,0)

/*--------------------
/ Sorcerer (S9) 03/85
/--------------------*/
INITGAME(sorcr, GEN_S9, s11_dispS9, 0, FLIP_SWNO(0,S11_SWNO(44)),3/*?*/)
S9_ROMSTART12(sorcr,l1,"cpu_u19.732", 0x88b6837d,
                       "cpu_u20.764", 0xc235b692)
S9S_SOUNDROM41111(     "cpu_u49.128", 0xa0bae1e4,
                       "spch_u4.732", 0x0c81902d,
                       "spch_u5.732", 0xd48c68ad,
                       "spch_u6.732", 0xa5c54d47,
                       "spch_u7.732", 0xbba9ed18)

S9_ROMEND
CORE_GAMEDEF(sorcr, l1, "Sorcerer (L-1)", 1985, "Williams", s9_mS9S,0)

/*--------------------
/ Pennant Fever
/--------------------*/
static core_tLCDLayout dispPfever[] = { \
  { 0, 0,17, 3, CORE_SEG7},  { 0, 8,21, 3, CORE_SEG7}, { 0,16,12, 4, CORE_SEG7},
  { 2, 2, 0, 1, CORE_SEG7},  { 2, 4, 8, 1, CORE_SEG7},
  { 2, 8,16, 1, CORE_SEG7},  { 2,10,24, 1, CORE_SEG7},
  { 4, 4,29, 1, CORE_SEG7},  { 4, 8,31, 1, CORE_SEG7},
  {0}
};
static core_tGameData PfevrGameData = {
  GEN_S9, dispPfever,
  {0},
  NULL,
  {{0}},
  {FLIP_SW(FLIP_L)}
};
static void init_pfevr(void) {
  core_gameData = &PfevrGameData;
}
S11_INPUT_PORTS_START(pfevr, 1) S11_INPUT_PORTS_END
S9_ROMSTART12(pfevr,p3,"cpu_u19.732", 0x03796c6d,
                       "cpu_u20.764", 0x3a3acb39)
S9S_SOUNDROM4("cpu_u49.128", 0xb0161712)
S9_ROMEND
CORE_GAMEDEF(pfevr, p3, "Pennant Fever Baseball (P-3)", 1984, "Williams", s9_mS9S,GAME_IMPERFECT_SOUND)

/*--------------------
/ Comet (S9) 06/85
/--------------------*/
static core_tLCDLayout dispComet[] = { \
  {  0, 0,17, 7, CORE_SEG87}, {3, 0,25,7,CORE_SEG87},
  {  0,18, 1, 7, CORE_SEG87}, {3,18, 9,7,CORE_SEG87},
  {  6, 3,16, 1, CORE_SEG7 }, {6, 5,24,1,CORE_SEG7},
  {  6, 8, 0, 1, CORE_SEG7 }, {6,10, 8,1,CORE_SEG7}, {0}
};
static core_tGameData cometGameData = {
  GEN_S9, dispComet,
  {FLIP_SWNO(0,S11_SWNO(30))},
  NULL,
  {{0}},
  {0, {S11_SWNO(47),S11_SWNO(48), S11_SWNO(40), S11_SWNO(41), S11_SWNO(42), 0}}
};
static void init_comet(void) {
  core_gameData = &cometGameData;
}
S11_INPUT_PORTS_START(comet, 1) S11_INPUT_PORTS_END
S9_ROMSTARTx4(comet,l4,"cpu_u20.128", 0x36193600)
S9S_SOUNDROM41111(     "cpu_u49.128", 0xf1db0cbe,
                       "spch_u4.732", 0xd0215c49,
                       "spch_u5.732", 0x89f7ede5,
                       "spch_u6.732", 0x6ba2aba6,
                       "spch_u7.732", 0x36545b22)
S9_ROMEND

CORE_GAMEDEF(comet, l4, "Comet (L-4)", 1985, "Williams", s9_mS9S,0)

/*--------------------
/ High Speed 01/86
/--------------------*/
static core_tGameData hsGameData = {
  GEN_S11, s11_dispS11,
  {FLIP_SWNO(S11_SWNO(37),S11_SWNO(38))},
  NULL,
  {{0}},
  {0, {S11_SWNO(49), S11_SWNO(50), S11_SWNO(35), S11_SWNO(34), S11_SWNO(33)}}
};
static void init_hs(void) {
  core_gameData = &hsGameData;
}
S11_INPUT_PORTS_START(hs, 3) S11_INPUT_PORTS_END
S11_ROMSTART28(hs,l4,"hs_u26.l4", 0x38b73830,
                     "hs_u27.l4", 0x24c6f7f0)
S11S_SOUNDROM88(     "hs_u21.l2", 0xc0580037,
                     "hs_u22.l2", 0xc03be631)
S11CS_SOUNDROM8(     "hs_u4.l1",  0x0f96e094)
S11_ROMEND

CORE_GAMEDEF(hs, l4, "High Speed (L-4)", 1986, "Williams", s11_mS11S,0)

/*--------------------
/ Grand Lizard 04/86
/--------------------*/
INITGAME(grand, GEN_S11, s11_dispS11, FLIP_SW(FLIP_L),0, 3/*?*/)
S11_ROMSTART28(grand,l4,"lzrd_u26.l4", 0x5fe50db6,
                        "lzrd_u27.l4", 0x6462ca55)
S11S_SOUNDROM44(        "lzrd_u21.l1", 0x98859d37,
                        "lzrd_u22.l1", 0x4e782eba)
S11CS_SOUNDROM8(        "lzrd_u4.l1",  0x4baafc11)
S11_ROMEND

CORE_GAMEDEF(grand, l4, "Grand Lizard (L-4)", 1986, "Williams", s11_mS11S,0)

/*--------------------
/ Road Kings 07/86
/--------------------*/
INITGAME(rdkng, GEN_S11, s11_dispS11, 0, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART28(rdkng,l5,"road_u26.l5", 0x22bcc70e,
                        "road_u27.l5", 0x3dcad794)
S11S_SOUNDROM88(        "road_u21.l1", 0xf34efbf4,
                        "road_u22.l1", 0xa9803804)
S11CS_SOUNDROM8(        "road_u4.l1",  0x4395b48f)
S11_ROMEND

CORE_GAMEDEF(rdkng, l5, "Road Kings (L-5)", 1986, "Williams", s11_mS11S,0)

/*--------------------
/ Pinbot 10/86
/--------------------*/
static core_tGameData pbGameData = {
  GEN_S11A, s11_dispS11,
  {FLIP_SWNO(S11_SWNO(10),S11_SWNO(11))},
  NULL,
  {{0}},
  {14, {S11_SWNO(53), 0, S11_SWNO(48), S11_SWNO(54), S11_SWNO(55),S11_SWNO(52)}}
};
static void init_pb(void) {
  core_gameData = &pbGameData;
}
S11_INPUT_PORTS_START(pb, 3) S11_INPUT_PORTS_END
S11_ROMSTART48(pb,l5,"pbot_u26.l5", 0xdaa0c8e4,
                     "pbot_u27.l5", 0xe625d6ce)
S11S_SOUNDROM88(     "pbot_u21.l1", 0x3eab88d9,
                     "pbot_u22.l1", 0xf8f7c965)
S11CS_SOUNDROM88(    "pbot_u4.l1",  0xde5926bd,
                     "pbot_u19.l1", 0x40eb4e9f)
S11_ROMEND

CORE_GAMEDEF(pb, l5, "Pinbot (L-5)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ F14 Tomcat 3/87
/--------------------*/
static core_tGameData f14GameData = {
  GEN_S11A, s11_dispS11a,
  {FLIP_SWNO(S11_SWNO(15),S11_SWNO(63))},
  NULL,
  {{0}},
  {14, {S11_SWNO(57), S11_SWNO(58), 0, S11_SWNO(28)}}
};
static void init_f14(void) {
  core_gameData = &f14GameData;
}
S11_INPUT_PORTS_START(f14, 3) S11_INPUT_PORTS_END
S11_ROMSTART48(f14,l1,"f14_u26.l1", 0x62c2e615,
                      "f14_u27.l1", 0xda1740f7)
S11S_SOUNDROM88(      "f14_u21.l1", 0xe412300c,
                      "f14_u22.l1", 0xc9dd7496)
S11CS_SOUNDROM88(     "f14_u4.l1",  0x43ecaabf,
                      "f14_u19.l1", 0xd0de4a7c)

S11_ROMEND

CORE_GAMEDEF(f14, l1, "F14 Tomcat (L-1)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Fire! 8/87
/--------------------*/
static core_tGameData fireGameData = {
  GEN_S11A, s11_dispS11a,
  {FLIP_SWNO(S11_SWNO(23),S11_SWNO(24))},
  NULL,
  {{0}},
  {12, {0,0, S11_SWNO(61), S11_SWNO(62), S11_SWNO(57), S11_SWNO(58)}}
};
static void init_fire(void) {
  core_gameData = &fireGameData;
}
S11_INPUT_PORTS_START(fire, 3) S11_INPUT_PORTS_END
S11_ROMSTART48(fire,l3,"fire_u26.l3", 0x48abae33,
                       "fire_u27.l3", 0x4ebf4888)
S11S_SOUNDROM88(       "fire_u21.l2", 0x2edde0a4,
                       "fire_u22.l2", 0x16145c97)
S11CS_SOUNDROM8(       "fire_u4.l1",  0x0e058918)

S11_ROMEND

CORE_GAMEDEF(fire, l3, "Fire! (L-3)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Big Guns 10/87
/--------------------*/
INITGAME(bguns, GEN_S11A,s11_dispS11a,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(bguns,l8,"guns_u26.l8", 0x792dc1e8,
                        "guns_u27.l8", 0xac4a1a51)
S11S_SOUNDROM88(        "guns_u21.l1", 0x35c6bfe4,
                        "guns_u22.l1", 0x091a5cb8)
S11CS_SOUNDROM88(       "gund_u4.l1",  0xd4a430a3,
                        "guns_u19.l1", 0xec1a6c23)

S11_ROMEND

CORE_GAMEDEF(bguns, l8, "Big Guns (L-8)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Space Station 1/88
/--------------------*/
INITGAME(spstn, GEN_S11B_1,s11_dispS11b_1,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(spstn,l5,"sstn_u26.l5", 0x614c8528,
                        "sstn_u27.l5", 0x4558d963)
S11S_SOUNDROM88(        "sstn_u21.l1", 0xa2ceccaa,
                        "sstn_u22.l1", 0x2b745994)
S11CS_SOUNDROM8(        "sstn_u4.l1",  0xad7a0511)

S11_ROMEND

CORE_GAMEDEF(spstn, l5, "Space Station (L-5)", 1988, "Williams", s11_mS11B_1S,0)

/*--------------------
/ Cyclone 2/88
/--------------------*/
INITGAME(cycln, GEN_S11B_1,s11_dispS11b_1, 12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(cycln,l5,"cycl_u26.l5", 0x9ab15e12,
                        "cycl_u27.l5", 0xc4b6aac0)
S11S_SOUNDROM88(        "cycl_u21.l1", 0xd4f69a7c,
                        "cycl_u22.l1", 0x28dc8f13)
S11CS_SOUNDROM88(       "cycl_u4.l5",  0xd04b663b,
                        "cycl_u19.l1", 0xa20f6519)

S11_ROMEND

CORE_GAMEDEF(cycln, l5, "Cyclone (L-5)", 1988, "Williams", s11_mS11B_1S,0)

/*--------------------
/ Banzai Run 7/88
/--------------------*/
INITGAME(bnzai, GEN_S11B_1,s11_dispS11b_1,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(bnzai,l3,"banz_u26.l3", 0xca578aa3,
                        "banz_u27.l3", 0xaf66fac4)
S11S_SOUNDROM88(        "banz_u21.l1", 0xcd06716e,
                        "banz_u22.l1", 0xe8159033)
S11CS_SOUNDROM888(      "banz_u4.l1",  0x8fd69c69,
                        "banz_u19.l1", 0x9104248c,
                        "banz_u20.l1", 0x26b3d15c)

S11_ROMEND

CORE_GAMEDEF(bnzai, l3, "Banzai Run (L-3)", 1988, "Williams", s11_mS11B_1S,0)

/*--------------------
/ Swords of Fury 8/88
/--------------------*/
INITGAME(swrds, GEN_S11B_1, s11_dispS11b_1,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(swrds,l2,"swrd_u26.l2", 0xc036f4ff,
                        "swrd_u27.l2", 0x33b0fb5a)
S11S_SOUNDROM88(        "swrd_u21.l1", 0xee8b0a64,
                        "swrd_u22.l1", 0x73dcdbb0)
S11CS_SOUNDROM88(       "swrd_u4.l1",  0x272b509c,
                        "swrd_u19.l1", 0xa22f84fa)

S11_ROMEND

CORE_GAMEDEF(swrds, l2, "Swords of Fury (L-2)", 1988, "Williams", s11_mS11B_1S,0)

/*--------------------
/ Taxi 10/88
/--------------------*/
static core_tLCDLayout dispTaxi[] = { \
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG8H),\
  { 2, 0,17,7,CORE_SEG7S},{0}
};

INITGAME(taxi, GEN_S11B_2,dispTaxi,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(taxi, l4,"taxi_u26.l4", 0xa70d8088,
                        "taxi_u27.l4", 0xf973f79c)
S11S_SOUNDROM88(        "taxi_u21.l1", 0x2b20e9ab,
                        "taxi_u22.l1", 0xd13055c5)
S11CS_SOUNDROM88(       "taxi_u4.l1",  0x6082ebb5,
                        "taxi_u19.l1", 0x91c64913)

S11_ROMEND
S11_ROMSTART48(taxi, l3,"taxi_u26.l4", 0xa70d8088,
                        "taxi_u27.l3", 0xe2bfb6fa)
S11S_SOUNDROM88(        "taxi_u21.l1", 0x2b20e9ab,
                        "taxi_u22.l1", 0xd13055c5)
S11CS_SOUNDROM88(       "taxi_u4.l1",  0x6082ebb5,
                        "taxi_u19.l1", 0x91c64913)

S11_ROMEND

CORE_GAMEDEF(taxi , l4, "Taxi (Lola) (L-4)", 1988, "Williams", s11_mS11B_2S,0)
CORE_CLONEDEF(taxi , l3, l4, "Taxi (Marylin) (L-3)", 1988, "Williams", s11_mS11B_2S,0)

/*--------------------
/ Jokerz 1/89
/--------------------*/
INITGAME(jokrz, GEN_S11B_3, s11_dispS11b_3,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(jokrz,l6,"jokeru26.l6", 0xc748c1ba,
                        "jokeru27.l6", 0x612d0ea7)
S11B3S_SOUNDROM881(      "jokeru21.l1", 0x9e2be4f6,
                        "jokeru22.l1", 0x2f67160c,
                        "jokeru5.l2" , 0xe9dc0095)
S11_ROMEND

CORE_GAMEDEF(jokrz, l6, "Jokerz (L-6)", 1989, "Williams", s11_mS11B_3S,GAME_IMPERFECT_SOUND)

/*--------------------
/ Earthshaker 4/89
/--------------------*/
INITGAME(eshak,GEN_S11B_2, s11_dispS11b_2,12, FLIP_SWNO(S11_SWNO(58),S11_SWNO(57)),3/*?*/)
S11_ROMSTART48(eshak,l3,"eshk_u26.l3", 0x5350d132,
                        "eshk_u27.l3", 0x91389290)
S11S_SOUNDROM88(        "eshk_u21.l1", 0xfeac68e5,
                        "eshk_u22.l1", 0x44f50fe1)
S11CS_SOUNDROM88(       "eshk_u4.l1",  0x40069f8c,
                        "eshk_u19.l1", 0xe5593075)

S11_ROMEND
S11_ROMSTART48(eshak,f1,"eshk_u26.f1", 0x15e2bfe3,
                        "eshk_u27.f1", 0xddfa8edd)
S11S_SOUNDROM88(        "eshk_u21.l1", 0xfeac68e5,
                        "eshk_u22.l1", 0x44f50fe1)
S11CS_SOUNDROM88(       "eshk_u4.l1",  0x40069f8c,
                        "eshk_u19.l1", 0xe5593075)

S11_ROMEND

CORE_GAMEDEF(eshak, f1, "Earthshaker (Family version) (F-1)", 1989, "Williams", s11_mS11B_1S,0)
CORE_CLONEDEF(eshak, l3, f1, "Earthshaker (L-3)", 1988, "Williams", s11_mS11B_1S,0)

/*-----------------------
/ Black Knight 2000 6/89
/-----------------------*/
INITGAME(bk2k, GEN_S11B_2, s11_dispS11b_2,12, FLIP_SWNO(S11_SWNO(58),S11_SWNO(57)), 3/*?*/)
S11_ROMSTART48(bk2k, l4,"bk2k_u26.l4", 0x16c7b9e7,
                        "bk2k_u27.l4", 0x5cf3ab40)
S11S_SOUNDROM88(        "bk2k_u21.l1", 0x08be36ad,
                        "bk2k_u22.l1", 0x9c8becd8)
S11CS_SOUNDROM88(       "bk2k_u4.l2",  0x1d87281b,
                        "bk2k_u19.l1", 0x58e162b2)

S11_ROMEND

CORE_GAMEDEF(bk2k, l4, "Black Knight 2000 (L-4)", 1989, "Williams", s11_mS11B_2S,0)

/*-----------------------
/ Police Force 9/89
/-----------------------*/
static core_tLCDLayout dispPolic[] = {
  { 0,8,16, 8,CORE_SEG8 },
  { 2,0, 0,16,CORE_SEG16 },
  { 4,0,16,16,CORE_SEG8H},{0}
};
INITGAME(polic,GEN_S11B_2, dispPolic, 12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(polic,l4,"pfrc_u26.l4", 0x1a1409e9,
                        "pfrc_u27.l4", 0x641ed5d4)
S11S_SOUNDROM88(        "pfrc_u21.l1", 0x7729afd3,
                        "pfrc_u22.l1", 0x40f5e6b2)
S11CS_SOUNDROM88(       "pfrc_u4.l2",  0x8f431529,
                        "pfrc_u19.l1", 0xabc4caeb)

S11_ROMEND

CORE_GAMEDEF(polic,l4, "Police Force (L-4)", 1989, "Williams", s11_mS11B_2S,0)

/*-----------------------------
/ Transporter the Rescue 6/89
/----------------------------*/
INITGAME(tsptr,GEN_S11B_2,s11_dispS11b_2,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(tsptr,l3,"tran_u26.l3", 0x2d48a108,
                        "tran_u27.l3", 0x50efb01c)
S11S_SOUNDROM88(        "tran_u21.l2", 0xb10120ee,
                        "tran_u22.l2", 0x337784b5)
S11CS_SOUNDROM888(      "tran_u4.l2",  0xa06ddd61,
                        "tran_u19.l2", 0x3cfde8b0,
                        "tran_u20.l2", 0xe9890cf1)

S11_ROMEND

CORE_GAMEDEF(tsptr,l3, "Transporter the Rescue (L-3)", 1989, "Bally", s11_mS11B_2S,0)

/*-----------------------
/ Bad Cats 12/89
/-----------------------*/
#define BCATS_WHEEL 16
static int wheelPos;
static void bcats_handleMech(int mech) {
  switch (2*(core_getPulsedSol(15)>0)+(core_getPulsedSol(16)>0)) {
    case 0:
    case 2: break;
    case 3: wheelPos += 1; if (wheelPos >= BCATS_WHEEL) wheelPos = 0; break;
    case 4: wheelPos -= 1; if (wheelPos <= 0) wheelPos = BCATS_WHEEL; break;
  }
  core_setSwSeq(44,wheelPos < 3);
}
static void bcats_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"wheel:%3d", wheelPos);
}


static core_tGameData bcatsGameData = {
  GEN_S11B_2, s11_dispS11b_2, {
    FLIP_SWNO(S11_SWNO(57),S11_SWNO(58)),
    0,0,0,
    NULL, bcats_handleMech,NULL,bcats_drawMech,
    NULL,NULL
  },
  NULL,
  {{0}},
  {12}
};
static void init_bcats(void) {
  core_gameData = &bcatsGameData;
}
S11_INPUT_PORTS_START(bcats, 1) S11_INPUT_PORTS_END
S11_ROMSTART48(bcats,l5,"cats_u26.l5", 0x32246d12,
                        "cats_u27.l5", 0xef842bbf)
S11S_SOUNDROM88(        "cats_u21.l1", 0x04110d08,
                        "cats_u22.l1", 0x7e152c78)
S11CS_SOUNDROM888(      "cats_u4.l1",  0x18c62813,
                        "cats_u19.l1", 0xf2fea68b,
                        "cats_u20.l1", 0xbf4dc35a)
S11_ROMEND

CORE_GAMEDEF(bcats,l5, "Bad Cats (L-5)", 1989, "Williams", s11_mS11B_2S,0)

/*-----------------------
/ Mousin' Around 12/89
/-----------------------*/
INITGAME(mousn,GEN_S11B_2,s11_dispS11b_2,12, FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(mousn,l4,"mous_u26.l4", 0xa540edc1,
                        "mous_u27.l4", 0xff108148)
S11S_SOUNDROM88(        "mous_u21.bin",0x59b1b0c5,
                        "mous_u22.l1", 0x00ad198c)
S11CS_SOUNDROM888(      "mous_u4.l2",  0x643add1e,
                        "mous_u19.l2", 0x7b4941f7,
                        "mous_u20.l2", 0x59b1b0c5)
S11_ROMEND

CORE_GAMEDEF(mousn,l4, "Mousin' Around (L-4)", 1989, "Bally", s11_mS11B_2S,0)

/*-----------------------
/ Whirlwind 4/90
/-----------------------*/
INITGAME(whirl,GEN_S11B_2x,s11_dispS11b_2,12, FLIP_SWNO(S11_SWNO(58),S11_SWNO(57)),3/*?*/)
S11_ROMSTART48(whirl,l3,"whir_u26.l3", 0x066b8fec,
                        "whir_u27.l3", 0x47fc033d)
S11S_SOUNDROM88(        "whir_u21.l1", 0xfa3da322,
                        "whir_u22.l1", 0xfcaf8c4e)
S11CS_SOUNDROM888(      "whir_u4.l1",  0x29952d84,
                        "whir_u19.l1", 0xc63f6fe9,
                        "whir_u20.l1", 0x713007af)
S11_ROMEND

CORE_GAMEDEF(whirl,l3, "Whirlwind (L-3)", 1990, "Williams", s11_mS11B_2S,0)

/*--------------------
/ Game Show 4/90
/--------------------*/
INITGAME(gs   ,GEN_S11C,s11_dispS11c,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(gs   ,l3,"gshw_u26.l3", 0x3419bfb2,
                        "gshw_u27.l3", 0x4f3babb6)
S11CS_SOUNDROM000(      "gshw_u4.l2",  0xe89e0116,
                        "gshw_u19.l1", 0x8bae0813,
                        "gshw_u20.l1", 0x75ccbdf7)
S11_ROMEND

CORE_GAMEDEF(gs   , l3, "Game Show (L-3)", 1990, "Bally", s11_mS11CS,0)

/*--------------------
/ Rollergames 5/90
/--------------------*/
INITGAME(rollr,GEN_S11C,s11_dispS11c,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(rollr,l2,"rolr_u26.l2", 0xcd7cad9e,
                        "rolr_u27.l2", 0xf3bac2b8)
S11CS_SOUNDROM000(      "rolr_u4.l3",  0xd366c705,
                        "rolr_u19.l3", 0x45a89e55,
                        "rolr_u20.l3", 0x77f89aff)
S11_ROMEND

CORE_GAMEDEF(rollr, l2, "Rollergames (L-2)", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Pool Sharks 6/90
/--------------------*/
INITGAME(pool ,GEN_S11C,s11_dispS11c,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(pool ,l7,"pool_u26.l7", 0xcee98aed,
                        "pool_u27.l7", 0x356d9a89)
S11CS_SOUNDROM000(      "pool_u4.l2",  0x04e95e10,
                        "pool_u19.l2", 0x0f45d02b,
                        "pool_u20.l2", 0x925f62d6)
S11_ROMEND

CORE_GAMEDEF(pool , l7, "Pool Sharks (L-7)", 1990, "Bally", s11_mS11CS,GAME_IMPERFECT_SOUND)

/*--------------------
/ Diner 8/90
/--------------------*/
INITGAME(diner,GEN_S11C,s11_dispS11c,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(diner,l4,"dinr_u26.l4", 0x6f187abf,
                        "dinr_u27.l4", 0xd69f9f74)
S11CS_SOUNDROM000(      "dinr_u4.l1",  0x3bd28368,
                        "dinr_u19.l1", 0x278b9a30,
                        "dinr_u20.l1", 0x511fb260)
S11_ROMEND

CORE_GAMEDEF(diner, l4, "Diner (L-4)", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Radical 9/90
/--------------------*/
INITGAME(radcl,GEN_S11C,s11_dispS11c,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(radcl,l1,"rad_u26.l1", 0x84b1a125,
                        "rad_u27.l1", 0x6f6ca382)
S11CS_SOUNDROM008(      "rad_u4.l1",  0x5aafc09c,
                        "rad_u19.l1", 0x7c005e1f,
                        "rad_u20.l1", 0x05b96292)
S11_ROMEND

CORE_GAMEDEF(radcl, l1, "Radical (L-1)", 1990, "Bally", s11_mS11CS,0)

/*--------------------
/ Riverboat Gambler 10/90
/--------------------*/
static core_tLCDLayout dispRvrbt[] = {
  { 0,18,17, 7, CORE_SEG87 },
  { 0, 4,28, 4, CORE_SEG7  },
  { 2, 0, 0,16, CORE_SEG16 },
  { 4, 0,16,16, CORE_SEG8H },{0}
};
#if 1
static char *wheel[] = {"Red","Green","Black","Red","Black","Red*","Black","Red",
                        "Black","Green","Red","Black","Red","Black*","Red","Black"};
#endif
static void rvrbt_drawMech(BMTYPE **line) {
  core_textOutf(50, 0,BLACK,"wheel:%3d", mech_getPos(0));
  core_textOutf(50,10,BLACK,"wheel:%-7s", wheel[mech_getPos(0)]);
}
static core_tGameData rvrbtGameData = {
  GEN_S11C, dispRvrbt, {
    FLIP_SW(FLIP_L),
    0,0,0,
    NULL, NULL,NULL,rvrbt_drawMech,
    NULL,NULL
  },
  NULL,
  {{0}},
  {12}
};
static mech_tInitData rvrbt_wheelMech = {
  14,15, MECH_LINEAR|MECH_CIRCLE|MECH_TWOSTEPSOL|MECH_FAST, 200,16,
  {{S11_SWNO(59),15,15},{S11_SWNO(59),0,6}}
};
static void init_rvrbt(void) {
  core_gameData = &rvrbtGameData;
  mech_add(0,&rvrbt_wheelMech);
}
S11_INPUT_PORTS_START(rvrbt, 3) S11_INPUT_PORTS_END

//INITGAME(rvrbt,GEN_S11C,dispRvrbt,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(rvrbt,l3,"gamb_u26.l3", 0xa65f6004,
                        "gamb_u27.l3", 0x9be0f613)
S11CS_SOUNDROM000(      "gamb_u4.l2",  0xc0cfa9be,
                        "gamb_u19.l1", 0x04a3a8c8,
                        "gamb_u20.l1", 0xa60c734d)
S11_ROMEND

CORE_GAMEDEF(rvrbt, l3, "Riverboat Gambler (L-3)", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Bugs Bunny Birthday Ball 11/90
/--------------------*/
static core_tLCDLayout dispbbb[] = { \
	{0,0,0,16,CORE_SEG16},{0,33,16,16,CORE_SEG16}, {0}
};

INITGAME(bbnny,GEN_S11C,dispbbb,12,FLIP_SW(FLIP_L),3/*?*/)
S11_ROMSTART48(bbnny,l2,"bugs_u26.l2", 0xb4358920,
                        "bugs_u27.l2", 0x8ff29439)
S11CS_SOUNDROM000(      "bugs_u4.l2",  0x04bc9aa5,
                        "bugs_u19.l1", 0xa2084702,
                        "bugs_u20.l1", 0x5df734ef)
S11_ROMEND

CORE_GAMEDEF(bbnny, l2, "Bugs Bunny Birthday Ball (L-2)", 1990, "Bally", s11_mS11CS,0)
#if 0
/*--------------------
/ Dr. Dude
/--------------------*/
INITGAME(dd,GEN_S11C,12,3/*?*/)
S11_ROMSTART48(dd,l2,"dude_u26.l2", 0xd1e19fc2,
                     "dude_u27.l2", 0x654b5d4c)
S11CS_SOUNDROM000(   "dude_u4.l1",  0x3eeef714,
                     "dude_u19.l1", 0xdc7b985b,
                     "dude_u20.l1", 0xa83d53dd)
S11_ROMEND

CORE_GAMEDEF(dd, l2, "Dr. Dude (L-2)", 1990, "Bally", s11_mS11CS,0)
#endif
