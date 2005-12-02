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
#include "gts3.h"
#include "gts80s.h"

#define INITGAME(name, gen, disp) \
static core_tGameData name##GameData = { gen, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME_S10(name, gen, disp, mux, flip, db, inv) \
static core_tGameData name##GameData = { \
  gen, disp, {flip,0,0,0,0,db}, NULL, {{0}, {0,0,0,0,inv}}, {mux} }; \
static void init_##name(void) { core_gameData = &name##GameData; }

S4_INPUT_PORTS_START(bowl, 1) S4_INPUT_PORTS_END

static const core_tLCDLayout dispS5[] = {
  { 0, 0, 0, 4, CORE_SEG7 }, { 0,12, 4, 4, CORE_SEG7 },
  { 2, 0, 8, 4, CORE_SEG7 }, { 2,12,20, 4, CORE_SEG7 },
  { 4, 0,24, 4, CORE_SEG7 }, { 4,12,28, 4, CORE_SEG7 },
  { 6, 4,12, 2, CORE_SEG7 }, { 6,12,14, 2, CORE_SEG7 }, {0}
};

static const core_tLCDLayout dispS10[] = {
  { 0, 0,22, 4, CORE_SEG7 }, { 0,12,30, 4, CORE_SEG7 },
  { 2, 0, 2, 4, CORE_SEG7 }, { 2,12,10, 4, CORE_SEG7 },
  { 4, 0,26, 2, CORE_SEG7 }, { 4, 4,34, 2, CORE_SEG7 }, { 4,12, 6, 2, CORE_SEG7 }, { 4,16,14, 2, CORE_SEG7 },
  { 6, 4, 0, 1, CORE_SEG7 }, { 6, 6, 8, 1, CORE_SEG7 }, { 6,12,20, 1, CORE_SEG7 }, { 6,14,28, 1, CORE_SEG7 }, {0}
};

static const core_tLCDLayout dispBowl[] = {
  { 0, 0, 4, 4, CORE_SEG7 }, { 0,12,12, 4, CORE_SEG7 },
  { 2, 0,20, 4, CORE_SEG7 }, { 2,12,28, 4, CORE_SEG7 },
  { 4, 0,36, 4, CORE_SEG7 }, { 4,12,44, 4, CORE_SEG7 },
  { 6, 4,54, 2, CORE_SEG7 }, { 6,12,52, 2, CORE_SEG7 }, {0}
};

/*----------------------------
/ Topaz
/----------------------------*/
INITGAME(topaz, GEN_S3, dispS5)
S4_ROMSTART(topaz,l1,"gamerom.716",CRC(cb287b10) SHA1(7fb6b6a26237cf85d5e02cf35271231267f90fc1),
                     "b_ic20.716", CRC(c6f8e3b1) SHA1(cb78d42e1265162132a1ab2320148b6857106b0e),
                     "b_ic17.716", CRC(cfc2518a) SHA1(5e99e40dcb7e178137db8d7d7d6da82ba87130fa))
S67S_SOUNDROMS8("sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_topaz input_ports_bowl
CORE_GAMEDEF(topaz,l1,"Topaz (Shuffle) (L-1)",1978,"Williams",s4_mS4S,0)

/*----------------------------
/ Taurus
/----------------------------*/
INITGAME(taurs, GEN_S4, dispS5)
S4_ROMSTART(taurs,l1,"gamerom.716",CRC(3246e285) SHA1(4f76784ecb5063a49c24795ae61db043a51e2c89),
                     "b_ic20.716", CRC(c6f8e3b1) SHA1(cb78d42e1265162132a1ab2320148b6857106b0e),
                     "b_ic17.716", CRC(cfc2518a) SHA1(5e99e40dcb7e178137db8d7d7d6da82ba87130fa))
S67S_SOUNDROMS8("taurus.snd", CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_taurs input_ports_bowl
CORE_GAMEDEF(taurs,l1,"Taurus (Shuffle) (L-1)",1979,"Williams",s4_mS4S,0)

/*----------------------------
/ Omni
/----------------------------*/
INITGAME(omni, GEN_S3, dispS5)
S4_ROMSTART(omni,l1,"omni-1a.u21",CRC(443bd170) SHA1(cc1ebd72d77ec2014cbd84534380e5ea1f12c022),
                     "5a-9140.u20", CRC(c6f8e3b1) SHA1(cb78d42e1265162132a1ab2320148b6857106b0e),
                     "5a-9141.u17", CRC(cfc2518a) SHA1(5e99e40dcb7e178137db8d7d7d6da82ba87130fa))
S67S_SOUNDROMS8("sound.716", CRC(db085cbb) SHA1(9a57abbad183ba16b3dba16d16923c3bfc46a0c3))
S4_ROMEND
#define input_ports_omni input_ports_bowl
CORE_GAMEDEF(omni,l1,"Omni (Shuffle) (L-1)",1980,"Williams",s4_mS4S,0)

/*--------------------------------
/ Big Ball Bowling (United game?)
/-------------------------------*/
static core_tGameData bbbowlinGameData = {GEN_BOWLING,dispBowl,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_ST100B,0}};
static void init_bbbowlin(void) { core_gameData = &bbbowlinGameData; }
BY17_ROMSTARTx88(bbbowlin,"cpu_u2.716",CRC(179e0c69) SHA1(7921839d2014a00b99ce7c44b325ea4403df9eea),
                          "cpu_u6.716",CRC(7b48e45b) SHA1(ac32292ef593bf8350e8bbc41113b6c1cb78a79e))
BY35_ROMEND
BY35_INPUT_PORTS_START(bbbowlin,1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(bbbowlin,"Big Ball Bowling",19??,"United(?)",by35_mBowling,0)

/*----------------------------
/ Stars & Strikes
/----------------------------*/
static core_tGameData monrobwlGameData = {GEN_BOWLING,dispBowl,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_ST100B,0}};
static void init_monrobwl(void) { core_gameData = &monrobwlGameData; }
ST200_ROMSTART8888(monrobwl,"cpu_u1.716",CRC(42592cc9) SHA1(22452072199c4b82a413065f8dfe235a39fe3825),
                            "cpu_u5.716",CRC(78e2dcd2) SHA1(7fbe9f7adc69af5afa489d9fd953640f3466de3f),
                            "cpu_u2.716",CRC(73534680) SHA1(d5233a9d4600fa28b767ee1a251ed1a1ffbaf9c4),
                            "cpu_u6.716",CRC(ad77d719) SHA1(f8f8d0d183d639d19fea552d35a7be3aa7f07c17))
BY35_ROMEND
BY35_INPUT_PORTS_START(monrobwl, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(monrobwl,"Stars & Strikes (Bowler)",198?,"Monroe Bowling Co.",by35_mBowling2,0)

/*----------------------------
/ Black Beauty
/----------------------------*/
static core_tGameData blbeautyGameData = {GEN_BOWLING,dispBowl,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_ST100B,0}};
static void init_blbeauty(void) { core_gameData = &blbeautyGameData; }
ST200_ROMSTART8888(blbeauty,"cpu_u1.716",CRC(e2550957) SHA1(e445548b650fec5d593ca7da587300799ef94991),
                            "cpu_u5.716",CRC(70fcd9f7) SHA1(ca5c2ea09f45f5ba50526880c158aaac61f007d5),
                            "cpu_u2.716",CRC(3f55d17f) SHA1(e6333e53570fb05a841a7f141872c8bd14143f9c),
                            "cpu_u6.716",CRC(842cd307) SHA1(8429d84e8bc4343b437801d0236150e04de79b75))
BY35_ROMEND
BY35_INPUT_PORTS_START(blbeauty, 1) BY35_INPUT_PORTS_END
CORE_GAMEDEFNV(blbeauty,"Black Beauty (Bowler)",198?,"Stern",by35_mBowling2,0)

/*----------------------------
/ Big Strike
/----------------------------*/
INITGAME(bstrk, GEN_S4, dispS5)
S4_ROMSTART(bstrk,l1,"gamerom.716",CRC(323dbcde) SHA1(a75cbb5de97cb9afc1d36e9b6ff593bb482fcf8b),
                     "b_ic20.716", CRC(c6f8e3b1) SHA1(cb78d42e1265162132a1ab2320148b6857106b0e),
                     "b_ic17.716", CRC(cfc2518a) SHA1(5e99e40dcb7e178137db8d7d7d6da82ba87130fa))
S4_ROMEND
#define input_ports_bstrk input_ports_bowl
CORE_GAMEDEF(bstrk,l1,"Big Strike (Bowler) (L-1)",1983,"Williams",s4_mS4,GAME_USES_CHIMES)

/*----------------------------
/ Triple Strike
/----------------------------*/
INITGAME(tstrk, GEN_S4, dispS5)
S4_ROMSTART(tstrk,l1,"gamerom.716",CRC(b034c059) SHA1(76b3926b87b3c137fcaf33021a586827e3c030af),
                     "ic20.716",   CRC(f163fc88) SHA1(988b60626f3d4dc8f4a1dbd0c99282418bc53aae),
                     "b_ic17.716", CRC(cfc2518a) SHA1(5e99e40dcb7e178137db8d7d7d6da82ba87130fa))
S4_ROMEND
#define input_ports_tstrk input_ports_bowl
CORE_GAMEDEF(tstrk,l1,"Triple Strike (Bowler) (L-1)",1983,"Williams",s4_mS4,GAME_USES_CHIMES)

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
S9_ROMSTART12(pfevr,l2,"pf-rom1.u19", CRC(00be42bd) SHA1(72ca21c96e3ffa3c43499165f3339b669c8e94a5),
                       "pf-rom2.u20", CRC(7b101534) SHA1(21e886d5872104d71bb528b9affb12230268597a))
S9S_SOUNDROM4("cpu_u49.128", CRC(b0161712) SHA1(5850f1f1f11e3ac9b9629cff2b26c4ad32436b55))
S9_ROMEND
S11_INPUT_PORTS_START(pfevr, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(pfevr, l2, "Pennant Fever Baseball (L-2)", 1984, "Williams", s9_mS9PS, 0)

S9_ROMSTART12(pfevr,p3,"cpu_u19.732", CRC(03796c6d) SHA1(38c95fcce9d0f357a74f041f0df006b9c6f6efc7),
                       "cpu_u20.764", CRC(3a3acb39) SHA1(7844cc30a9486f718a556850fc9cef3be82f26b7))
S9S_SOUNDROM4("cpu_u49.128", CRC(b0161712) SHA1(5850f1f1f11e3ac9b9629cff2b26c4ad32436b55))
S9_ROMEND
CORE_CLONEDEF(pfevr, p3, l2, "Pennant Fever Baseball (P-3)", 1984, "Williams", s9_mS9PS, 0)

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
S9_ROMSTART12(szone,l5,"sz_u19r5.732", CRC(c79c46cb) SHA1(422ba74ae67bebbe02f85a9a8df0e3072f3cebc0),
                       "sz_u20r5.764", CRC(9b5b3be2) SHA1(fce051a60b6eecd9bc07273892b14046b251b372))
S9S_SOUNDROM4("szs_u49.128", CRC(144c3c07) SHA1(57be6f336f200079cd698b13f8fa4755cf694274))
S9_ROMEND
S11_INPUT_PORTS_START(szone, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(szone, l5, "Strike Zone (Shuffle) (L-5)", 1984, "Williams", s9_mS9S,0)

S9_ROMSTART12(szone,l2,"sz_u19r2.732", CRC(c0e4238b) SHA1(eae60ccd5b5001671cd6d2685fd588494d052d1e),
                       "sz_u20r2.764", CRC(91c08137) SHA1(86da08f346f85810fceceaa7b9824ab76a68da54))
S9S_SOUNDROM4("szs_u49.128", CRC(144c3c07) SHA1(57be6f336f200079cd698b13f8fa4755cf694274))
S9_ROMEND
CORE_CLONEDEF(szone,l2,l5,"Strike Zone (Shuffle) (L-2)", 1984, "Williams", s9_mS9S,0)

/*--------------------
/ Alley Cat (#918)
/--------------------*/
INITGAME_S10(alcat, GEN_S9, dispS10, 0, FLIP_SW(FLIP_L), S11_BCDDIAG|S11_BCDDISP, 0)
S9_ROMSTART12(alcat,l7,"u26_rev7.rom", CRC(4d274dd3) SHA1(80d72bd0f85ce2cac04f6d9f59dc1fcccc86d402),
                       "u27_rev7.rom", CRC(9c7faf8a) SHA1(dc1a561948b9a303f7924d7bebcd972db766827b))
S9S_SOUNDROM4("alct_u49.128", NO_DUMP)
S9_ROMEND
S11_INPUT_PORTS_START(alcat, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(alcat, l7, "Alley Cat (Shuffle) (L-7)", 1986, "Williams", s11_s9S,0)

/*--------------------
/ Gold Mine (#920)
/--------------------*/
INITGAME_S10(gmine, GEN_S11, dispS10, 0, FLIP_SW(FLIP_L), S11_BCDDISP, 0)
S9_ROMSTARTx4(gmine,l2,"u27.128",CRC(99c6e049) SHA1(356faec0598a54892050a28857e9eb5cdbf35833))
S11S_SOUNDROM88(       "u21.256",CRC(3b801570) SHA1(50b50ff826dcb031a30940fa3099bd3a8d773831),
                       "u22.256",CRC(08352101) SHA1(a7437847a71cf037a80686292f9616b1e08922df))
S11_ROMEND
S11_INPUT_PORTS_START(gmine, 1) S11_INPUT_PORTS_END
CORE_GAMEDEF(gmine, l2, "Gold Mine (L-2)", 1988, "Williams", s11_mS11S,0)

/*--------------------
/ Top Dawg (#921)
/--------------------*/
INITGAME_S10(tdawg, GEN_S11, dispS10, 0, FLIP_SW(FLIP_L), S11_BCDDISP, 0)
S9_ROMSTARTx4(tdawg,l1,"tdu27r1.128",CRC(0b4bb586) SHA1(a927ebf7167609cc84b38c22aa35d0c4d259dd8b))
S11S_SOUNDROM88(      "tdsu21r1.256",CRC(6a323227) SHA1(7c7263754e5672c654a2ee9582f0b278e637a909),
                      "tdsu22r1.256",CRC(58407eb4) SHA1(6bd9b304c88d9470eae5afb6621187f4a8313573))
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
WPC_ROMSTART(sf,l1,"sf_u6.l1",0x40000,CRC(ada93967) SHA1(90094d207dafdacfaf7d259c6cc3dc2b552c8588))
WPCS_SOUNDROM222("sf_u18.l1",CRC(78092c83) SHA1(7c922dfd8be4bb5e23d4c86b6eb18a29cc034338),
                 "sf_u15.l1",CRC(adcaeaa1) SHA1(27aa9526c628634c395161f4966d9943bdf1f120),
                 "sf_u14.l1",CRC(b830b419) SHA1(c59980a78d8cb1d979de21dfc5ad3d671d8486e7))
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
WPC_ROMSTART(strik,l4,"strik_l4.rom",0x40000,CRC(c99ea24c) SHA1(f8b083adcbabdc70a1bf7c87c9b488eca7b1c788))
WPCS_SOUNDROM222("lc_u18.l1",CRC(beb84fd9) SHA1(b1d5472af5e3c0f5c67e7d636122eb79e02494ba),
                 "lc_u15.l1",CRC(25fe0be3) SHA1(a784b99daab1255487d4cb05d008a8aee0c39b30),
                 "lc_u14.l1",CRC(7b6dd395) SHA1(fdb01f70bb5f1a4ada9805770e544c4a191ddf26))
WPC_ROMEND
WPC_INPUT_PORTS_START(strik, 0) WPC_INPUT_PORTS_END
CORE_GAMEDEF(strik,l4,"Strike Master (L-4)",1992,"Williams",wpc_mFliptronS,0)

/*-------------------------------------------------------------------
/ Strikes n' Spares (#N111)
/-------------------------------------------------------------------*/
static struct core_dispLayout GTS3_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)gts3_dmd128x32},
  {34,0,32,128,CORE_DMD,(void *)gts3_dmd128x32a},
  {0}
};
static core_tGameData snsparesGameData = {GEN_GTS3,GTS3_dispDMD,{FLIP_SWNO(21,22),4,4,0,SNDBRD_NONE,0}};
static void init_snspares(void) { core_gameData = &snsparesGameData; }
GTS3ROMSTART(snspares,	"gprom.bin", CRC(9e018496) SHA1(a4995f153ba2179198cfc56b7011707328e4ec89))
GTS3_DMD256_ROMSTART2(	"dsprom.bin",CRC(5c901899) SHA1(d106561b2e382afdb16e938072c9c8f1d1ccdae6))
NORMALREGION(0x100000, REGION_USER3) \
  GTS3S_ROMLOAD4(0x00000, "arom1.bin", CRC(e248574a) SHA1(d2bdc2b9a330bb81556d25d464f617e0934995eb)) \
GTS3_ROMEND
GTS32_INPUT_PORTS_START(snspares, 4) GTS3_INPUT_PORTS_END
CORE_GAMEDEFNV(snspares,"Strikes n' Spares",1995,"Gottlieb",mGTS3DMDS2, 0)

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
WPC_ROMSTART(lc,11,"lchmp1_1.rom",0x80000,CRC(60ab944c) SHA1(d2369b0a864e864b269de1765121e4534fa8fa59))
WPCS_SOUNDROM222("lc_u18.l1",CRC(beb84fd9) SHA1(b1d5472af5e3c0f5c67e7d636122eb79e02494ba),
                 "lc_u15.l1",CRC(25fe0be3) SHA1(a784b99daab1255487d4cb05d008a8aee0c39b30),
                 "lc_u14.l1",CRC(7b6dd395) SHA1(fdb01f70bb5f1a4ada9805770e544c4a191ddf26))
WPC_ROMEND
WPC_INPUT_PORTS_START(lc, 0) WPC_INPUT_PORTS_END
CORE_GAMEDEF(lc,11,"League Champ (1.1)",1996,"Bally",wpc_mFliptronS,0)
