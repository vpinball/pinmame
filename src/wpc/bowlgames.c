/*
 * In this file we'll collect any bowler, shuffle alley, pitch & bat game,
 * and other non-pinball stuff that's running on pinball machine hardware.
 */

#include "driver.h"
#include "core.h"
#include "by35.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "wpc.h"
#include "s11.h"
#include "s4.h"

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = { gen, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME_S10(name, gen, disp, mux, flip, db, inv) \
static core_tGameData name##GameData = { \
  gen, disp, {flip,0,0,0,0,db}, NULL, {{0}, {0,0,0,0,inv}}, {mux} }; \
static void init_##name(void) { core_gameData = &name##GameData; }

S4_INPUT_PORTS_START(bowl, 1) S4_INPUT_PORTS_END

static const core_tLCDLayout bowl_disp[] = {
  { 0, 0, 0, 4, CORE_SEG7 }, { 0,12, 4, 4, CORE_SEG7 },
  { 2, 0, 8, 4, CORE_SEG7 }, { 2,12,20, 4, CORE_SEG7 },
  { 4, 0,24, 4, CORE_SEG7 }, { 4,12,28, 4, CORE_SEG7 },
  { 6, 4,12, 2, CORE_SEG7 }, { 6,12,14, 2, CORE_SEG7 }, {0}
};

/*----------------------------
/ Topaz
/----------------------------*/
INITGAME(topaz, GEN_S3, bowl_disp)
S4_ROMSTART(topaz,l1,"gamerom.716",CRC(cb287b10),
                     "b_ic20.716", CRC(c6f8e3b1),
                     "b_ic17.716", CRC(cfc2518a))
S67S_SOUNDROMS8("sound1.716",CRC(f4190ca3))
S4_ROMEND
#define input_ports_topaz input_ports_bowl
CORE_GAMEDEF(topaz,l1,"Topaz (Shuffle) (L-1)",1978,"Williams",s4_mS4S,0)

/*----------------------------
/ Taurus
/----------------------------*/
INITGAME(taurs, GEN_S4, bowl_disp)
S4_ROMSTART(taurs,l1,"gamerom.716",CRC(3246e285),
                     "b_ic20.716", CRC(c6f8e3b1),
                     "b_ic17.716", CRC(cfc2518a))
S67S_SOUNDROMS8("sound.716",0x0)
S4_ROMEND
#define input_ports_taurs input_ports_bowl
CORE_GAMEDEF(taurs,l1,"Taurus (Shuffle) (L-1)",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Big Ball Bowling (United game?)
/-------------------------------*/
static const core_tLCDLayout dispBowl[] = {
  { 0, 0, 4, 4, CORE_SEG7 }, { 0,12,12, 4, CORE_SEG7 },
  { 2, 0,20, 4, CORE_SEG7 }, { 2,12,28, 4, CORE_SEG7 },
  { 4, 0,36, 4, CORE_SEG7 }, { 4,12,44, 4, CORE_SEG7 },
  { 6, 4,54, 2, CORE_SEG7 }, { 6,12,52, 2, CORE_SEG7 }, {0}
};
static core_tGameData bbbowlinGameData = {GEN_BOWLING,dispBowl,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_ST100B,0}};
static void init_bbbowlin(void) { core_gameData = &bbbowlinGameData; }
BY17_ROMSTARTx88(bbbowlin,"cpu_u2.716",CRC(179e0c69),
                          "cpu_u6.716",CRC(7b48e45b))
BY35_ROMEND
BY35_INPUT_PORTS_START(bbbowlin,1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(bbbowlin,"Big Ball Bowling",19??,"United(?)",by35_mBowling,GAME_NO_SOUND)

/*----------------------------
/ Big Strike
/----------------------------*/
INITGAME(bstrk, GEN_S4, bowl_disp)
S4_ROMSTART(bstrk,l1,"gamerom.716",CRC(323dbcde),
                     "b_ic20.716", CRC(c6f8e3b1),
                     "b_ic17.716", CRC(cfc2518a))
S67S_SOUNDROMS8("sound.716",0x0)
S4_ROMEND
#define input_ports_bstrk input_ports_bowl
CORE_GAMEDEF(bstrk,l1,"Big Strike (Bowler) (L-1)",1983,"Williams",s4_mS4S,0)

/*----------------------------
/ Triple Strike
/----------------------------*/
INITGAME(tstrk, GEN_S4, bowl_disp)
S4_ROMSTART(tstrk,l1,"gamerom.716",CRC(b034c059),
                     "ic20.716",   CRC(f163fc88),
                     "b_ic17.716", CRC(cfc2518a))
S67S_SOUNDROMS8("sound.716",0x0)
S4_ROMEND
#define input_ports_tstrk input_ports_bowl
CORE_GAMEDEF(tstrk,l1,"Triple Strike (Bowler) (L-1)",1983,"Williams",s4_mS4S,0)

/*--------------------
/ Pennant Fever (#526)
/--------------------*/
static core_tLCDLayout dispPfevr[] = {
  { 0, 0,21, 3, CORE_SEG7 }, { 0, 8,25, 3, CORE_SEG7 }, { 0,16,12, 4, CORE_SEG7 },
  { 2, 2, 0, 1, CORE_SEG7 }, { 2, 4, 8, 1, CORE_SEG7 },
  { 2, 8,20, 1, CORE_SEG7 }, { 2,10,28, 1, CORE_SEG7 },
  { 4, 4,33, 1, CORE_SEG7 }, { 4, 8,35, 1, CORE_SEG7 }, {0}
};
INITGAME_S10(pfevr, GEN_S9, dispPfevr, 0, FLIP_SW(FLIP_L), S11_BCDDIAG|S11_BCDDISP, 0x08)
S9_ROMSTART12(pfevr,p3,"cpu_u19.732", CRC(03796c6d),
                       "cpu_u20.764", CRC(3a3acb39))
S9S_SOUNDROM4("cpu_u49.128", CRC(b0161712))
S9_ROMEND
S11_INPUT_PORTS_START(pfevr, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(pfevr, p3, "Pennant Fever Baseball (P-3)", 1984, "Williams", s9_mS9S,GAME_IMPERFECT_SOUND)

/*--------------------
/ Strike Zone (#916)
/--------------------*/
static core_tLCDLayout dispSzone[] = {
  { 0, 0,20, 4, CORE_SEG7 }, { 0,12,24, 4, CORE_SEG7 },
  { 2, 0,28, 4, CORE_SEG7 }, { 2,12, 0, 4, CORE_SEG7 },
  { 4, 0, 4, 4, CORE_SEG7 }, { 4,12, 8, 4, CORE_SEG7 },
  { 6, 4,33, 1, CORE_SEG7 }, { 6, 6,32, 1, CORE_SEG7 }, { 6,12,34, 2, CORE_SEG7 }, {0}
};
INITGAME_S10(szone, GEN_S9, dispSzone, 0, FLIP_SW(FLIP_L), S11_BCDDIAG|S11_BCDDISP, 0)
S9_ROMSTART12(szone,l5,"sz_u19r5.732", CRC(c79c46cb),
                       "sz_u20r5.764", CRC(9b5b3be2))
S9S_SOUNDROM4("szs_u49.128", CRC(144c3c07))
S9_ROMEND
S11_INPUT_PORTS_START(szone, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(szone, l5, "Strike Zone (Shuffle) (L-5)", 198?, "Williams", s9_mS9S,0)

static core_tLCDLayout dispS10[] = {
  { 0, 0,22, 4, CORE_SEG7 }, { 0,12,30, 4, CORE_SEG7 },
  { 2, 0, 2, 4, CORE_SEG7 }, { 2,12,10, 4, CORE_SEG7 },
  { 4, 0,26, 2, CORE_SEG7 }, { 4, 4,34, 2, CORE_SEG7 }, { 4,12, 6, 2, CORE_SEG7 }, { 4,16,14, 2, CORE_SEG7 },
  { 6, 4, 0, 1, CORE_SEG7 }, { 6, 6, 8, 1, CORE_SEG7 }, { 6,12,20, 1, CORE_SEG7 }, { 6,14,28, 1, CORE_SEG7 }, {0}
};
/*--------------------
/ Gold Mine (#920)
/--------------------*/
INITGAME_S10(gmine, GEN_S11, dispS10, 0, FLIP_SW(FLIP_L), S11_BCDDISP, 0)
S9_ROMSTARTx4(gmine,l2,"u27.128",CRC(99c6e049))
S11S_SOUNDROM88(       "u21.256",CRC(3b801570),
                       "u22.256",CRC(08352101))
S11_ROMEND
S11_INPUT_PORTS_START(gmine, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(gmine, l2, "Gold Mine (L-2)", 1988, "Williams", s11_mS11S,0)

/*--------------------
/ Top Dawg (#921)
/--------------------*/
INITGAME_S10(tdawg, GEN_S11, dispS10, 0, FLIP_SW(FLIP_L), S11_BCDDISP, 0)
S9_ROMSTARTx4(tdawg,l1,"tdu27r1.128",CRC(0b4bb586))
S11S_SOUNDROM88(      "tdsu21r1.256",CRC(6a323227),
                      "tdsu22r1.256",CRC(58407eb4))
S11_ROMEND
S11_INPUT_PORTS_START(tdawg, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(tdawg, l1, "Top Dawg (L-1)", 1988, "Williams", s11_mS11S,0)

/*--------------------
/ Slugfest baseball
/--------------------*/
static core_tGameData sfGameData = {
  GEN_WPCDMD, wpc_dispDMD,
  { 0 },
  NULL,
  { "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 13, 14, 21, 22, 0 }
  }
};
static void init_sf(void) { core_gameData = &sfGameData; }
WPC_ROMSTART(sf,l1,"sf_u6.l1",0x40000,CRC(ada93967))
WPCS_SOUNDROM222("sf_u18.l1",CRC(78092c83),
                 "sf_u15.l1",CRC(adcaeaa1),
                 "sf_u14.l1",CRC(b830b419))
WPC_ROMEND
WPC_INPUT_PORTS_START(sf, 0) WPC_INPUT_PORTS_END
CORE_GAMEDEF(sf,l1,"Slugfest (L-1)",1991,"Williams",wpc_mDMDS,0)

/*-------------------
/ Strike Master
/--------------------*/
static core_tGameData strikGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  { 0 },
  NULL,
  { "",
    { 0 }, /* No inverted switches */
    { 13, 14, 21, 22, 0 }
  }
};
static void init_strik(void) { core_gameData = &strikGameData; }
WPC_ROMSTART(strik,l4,"strik_l4.rom",0x40000,CRC(c99ea24c))
WPC_ROMEND
WPC_INPUT_PORTS_START(strik, 0) WPC_INPUT_PORTS_END
CORE_GAMEDEF(strik,l4,"Strike Master (L-4)",1992,"Williams",wpc_mFliptron,GAME_NO_SOUND)

/*-----------------------------
/ League Champ (Shuffle Alley)
/------------------------------*/
static core_tGameData lcGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  { 0 },
  NULL,
  {
    "",
    { 0 }, /* No inverted switches */
    { 13, 14, 21, 22, 0 }
  }
};
static void init_lc(void) { core_gameData = &lcGameData; }
WPC_ROMSTART(lc,11,"lchmp1_1.rom",0x80000,CRC(60ab944c))
WPC_ROMEND
WPC_INPUT_PORTS_START(lc, 0) WPC_INPUT_PORTS_END
CORE_GAMEDEF(lc,11,"League Champ (1.1)",1996,"Bally",wpc_mFliptron,GAME_NO_SOUND)
