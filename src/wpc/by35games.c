#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "wmssnd.h"
#include "by35.h"

//#define DISPLAYALL
#ifdef DISPLAYALL
static const core_tLCDLayout dispBy6[] = {
  { 0, 0, 0, 16, CORE_SEG87 }, { 2, 0,16, 16, CORE_SEG87 },
  { 4, 0,32, 16, CORE_SEG87 }, {0}
};
#define dispBy7 dispBy6
#else /* DISPLAYALL */
static core_tLCDLayout dispBy6[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 2,35,2,CORE_SEG7}, {4, 8,38,2,CORE_SEG7},{0}
};
static core_tLCDLayout dispBy7[] = {
  {0, 0, 1,7,CORE_SEG87F}, {0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F}, {2,16,25,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};
#endif /* DISPLAYALL */

BY35_INPUT_PORTS_START(by35,1)        BY35_INPUT_PORTS_END
BY35PROTO_INPUT_PORTS_START(by35p,1)  BY35_INPUT_PORTS_END

#define INITGAME(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME2(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,1}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/* -------------------------------------------------------------*/
/* All games below use CHIMES for sound - ie, no sound hardware */
/* -------------------------------------------------------------*/

/*--------------------------------
/ Bow & Arrow (Prototype game)
/-------------------------------*/
static const core_tLCDLayout dispBA[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 2,34,2,CORE_SEG7}, {4, 8,38,2,CORE_SEG7},{0}
};
INITGAME(bowarrow,GEN_BYPROTO,dispBA,FLIP_SW(FLIP_L),7,0,0)
BYPROTO_ROMSTART(bowarrow,"b14.bin",CRC(d4d0f92a),
                          "b16.bin",CRC(ad2102e7),
                          "b18.bin",CRC(5d84656b),
                          "b1a.bin",CRC(6f083ce6),
                          "b1c.bin",CRC(6ed4d39e),
                          "b1e.bin",CRC(ff2f97de))
BY17_ROMEND
#define input_ports_bowarrow input_ports_by35p
CORE_GAMEDEFNV(bowarrow,"Bow & Arrow (Prototype)",1976,"Bally",by35_mBYPROTO,GAME_USES_CHIMES)

/*--------------------------------
/ Freedom
/-------------------------------*/
INITGAME(freedom,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTART228(freedom,"720-08_1.474",CRC(b78bceeb),
                         "720-10_2.474",CRC(ca90c8a7),
                         "720-07_6.716",CRC(0f4e8b83))
BY17_ROMEND
#define input_ports_freedom input_ports_by35
CORE_GAMEDEFNV(freedom,"Freedom",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Night Rider
/-------------------------------*/
INITGAME(nightrdr,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTART8x8(nightrdr,"721-21_1.716",CRC(237c4060),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_nightrdr input_ports_by35
CORE_GAMEDEFNV(nightrdr,"Night Rider",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ EVEL KNIEVEL
/-------------------------------*/
INITGAME(evelknie,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(evelknie,"722-17_2.716",CRC(b6d9a3fa),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_evelknie input_ports_by35
CORE_GAMEDEFNV(evelknie,"Evel Knievel",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Eight Ball
/-------------------------------*/
INITGAME(eightbll,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(eightbll,"723-20_2.716",CRC(33559e7b),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_eightbll input_ports_by35
CORE_GAMEDEFNV(eightbll,"Eight Ball",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Power Play
/-------------------------------*/
INITGAME(pwerplay,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(pwerplay,"724-25_2.716",CRC(43012f35),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_pwerplay input_ports_by35
CORE_GAMEDEFNV(pwerplay,"Power Play",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Mata Hari
/-------------------------------*/
INITGAME(matahari,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(matahari,"725-21_2.716",CRC(63acd9b0),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_matahari input_ports_by35
CORE_GAMEDEFNV(matahari,"Mata Hari",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Strikes and Spares
/-------------------------------*/
INITGAME(stk_sprs,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(stk_sprs,"740-16_2.716",CRC(2be27024),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_stk_sprs input_ports_by35
CORE_GAMEDEFNV(stk_sprs,"Strikes and Spares",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Black Jack
/-------------------------------*/
INITGAME(blackjck,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(blackjck,"728-32_2.716",CRC(1333c9d1),
                          "720-20_6.716",CRC(0c17aa4d))
BY17_ROMEND
#define input_ports_blackjck input_ports_by35
CORE_GAMEDEFNV(blackjck,"Black Jack",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)

/* -------------------------------------*/
/* All games below use Sound Module -32 */
/* -------------------------------------*/
/*--------------------------------
/ Lost World
/-------------------------------*/
INITGAME(lostwrld,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(lostwrld,"729-33_1.716",CRC(4ca40b95),
                          "729-48_2.716",CRC(963bffd8),
                          "720-28_6.716",CRC(f24cce3e))
BY32_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_lostwrld input_ports_by35
CORE_GAMEDEFNV(lostwrld,"Lost World",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ 6million$man
/-------------------------------*/
static core_tLCDLayout smmanDisp[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 0,50,6,CORE_SEG7}, {4,14,42,6,CORE_SEG7},
  {6, 2,35,2,CORE_SEG7}, {6, 8,38,2,CORE_SEG7}, {0}
};
INITGAME(smman,GEN_BY35,smmanDisp,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0x81)
BY35_ROMSTART888(smman,"742-20_1.716",CRC(33e55a75),
                       "742-18_2.716",CRC(5365d36c),
                       "720-30_6.716",CRC(4be8aab0))
BY32_SOUNDROM(         "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_smman input_ports_by35
CORE_GAMEDEFNV(smman,"Six Million Dollar Man",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Playboy
/-------------------------------*/
INITGAME(playboy,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(playboy,"743-14_1.716",CRC(5c40984a),
                         "743-12_2.716",CRC(6fa66664),
                         "720-30_6.716",CRC(4be8aab0))
BY32_SOUNDROM(           "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_playboy input_ports_by35
CORE_GAMEDEFNV(playboy,"Playboy",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Voltan Escapes Cosmic Doom
/-------------------------------*/
INITGAME(voltan,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(voltan,"744-03_1.716",CRC(ad2467ae),
                        "744-04_2.716",CRC(dbf58b83),
                        "720-30_6.716",CRC(4be8aab0))
BY32_SOUNDROM(          "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_voltan input_ports_by35
CORE_GAMEDEFNV(voltan,"Voltan Escapes Cosmic Doom",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Supersonic
/-------------------------------*/
INITGAME(sst,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(sst,"741-10_1.716",CRC(5e4cd81a),
                     "741-08_2.716",CRC(2789cbe6),
                     "720-30_6.716",CRC(4be8aab0))
BY32_SOUNDROM(       "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_sst input_ports_by35
CORE_GAMEDEFNV(sst,"Supersonic",1979,"Bally",by35_mBY35_32S,0)

/* -------------------------------------*/
/* All games below use Sound Module -50 */
/* -------------------------------------*/
/*--------------------------------
/ Star Trek
/-------------------------------*/
INITGAME(startrek,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(startrek,"745-11_1.716",CRC(a077efca),
                          "745-12_2.716",CRC(f683210a),
                          "720-30_6.716",CRC(4be8aab0))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_startrek input_ports_by35
CORE_GAMEDEFNV(startrek, "Star Trek",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Paragon
/-------------------------------*/
INITGAME(paragon,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(paragon,"748-17_1.716",CRC(08dbdf32),
                         "748-15_2.716",CRC(26cc05c1),
                         "720-30_6.716",CRC(4be8aab0))
BY50_SOUNDROM(           "729-51_3.123",CRC(6e7d3e8b))
BY35_ROMEND
#define input_ports_paragon input_ports_by35
CORE_GAMEDEFNV(paragon,"Paragon",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Harlem Globetrotters
/-------------------------------*/
INITGAME(hglbtrtr,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(hglbtrtr,"750-07_1.716",CRC(da594719),
                          "750-08_2.716",CRC(3c783931),
                          "720-35_6.716",CRC(78d6d289))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b))
BY35_ROMEND
#define input_ports_hglbtrtr input_ports_by35
CORE_GAMEDEFNV(hglbtrtr,"Harlem Globetrotters",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Harlem Globetrotters (7-digit bootleg)
/-------------------------------*/
INITGAME(hglbtrtb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(hglbtrtb,"harl2732.u2", CRC(f70a2981),
                          "720-3532.u6b",CRC(cb4243e7))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b))
BY35_ROMEND
#define input_ports_hglbtrtb input_ports_hglbtrtr
CORE_CLONEDEFNV(hglbtrtb,hglbtrtr,"Harlem Globetrotters (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_50S,0)
/*--------------------------------
/ Dolly Parton
/-------------------------------*/
INITGAME(dollyptn,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(dollyptn,"777-10_1.716",CRC(ca88cb9a),
                          "777-13_2.716",CRC(7fc93ea3),
                          "720-35_6.716",CRC(78d6d289))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b))
BY35_ROMEND
#define input_ports_dollyptn input_ports_by35
CORE_GAMEDEFNV(dollyptn,"Dolly Parton",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Dolly Parton (7-digit bootleg)
/-------------------------------*/
INITGAME(dollyptb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(dollyptb,"doll2732.u2", CRC(cd649da3),
                          "720-3532.u6b",CRC(cb4243e7))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b))
BY35_ROMEND
#define input_ports_dollyptb input_ports_dollyptn
CORE_CLONEDEFNV(dollyptb,dollyptn,"Dolly Parton (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_50S,0)
/*--------------------------------
/ Kiss
/-------------------------------*/
INITGAME(kiss,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY50,0)
BY35_ROMSTART888(kiss,"746-11_1.716",CRC(78ec7fad),
                      "746-14_2.716",CRC(0fc8922d),
                      "720-30_6.716",CRC(4be8aab0))
BY50_SOUNDROM(        "729-18_3.123",CRC(7b6b7d45))
BY35_ROMEND
#define input_ports_kiss input_ports_by35
CORE_GAMEDEFNV(kiss,"Kiss",1979,"Bally",by35_mBY35_50S,0)

/* -------------------------------------*/
/* All games below use Sound Module -51 */
/* -------------------------------------*/
/*--------------------------------
/ Future Spa
/-------------------------------*/
INITGAME(futurspa,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTART888(futurspa,"781-07_1.716",CRC(4c716a6a),
                          "781-09_2.716",CRC(316617ed),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "781-02_4.716",CRC(364f7c9a))
BY35_ROMEND
#define input_ports_futurspa input_ports_by35
CORE_GAMEDEFNV(futurspa,"Future Spa",1979,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Future Spa (7-digit bootleg)
/-------------------------------*/
INITGAME(futurspb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(futurspb,"fspa2732.u2", CRC(8c4bf58f),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "781-02_4.716",CRC(364f7c9a))
BY35_ROMEND
#define input_ports_futurspb input_ports_futurspa
CORE_CLONEDEFNV(futurspb,futurspa,"Future Spa (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Space invaders
/-------------------------------*/
INITGAME(spaceinv,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTART888(spaceinv,"792-10_1.716",CRC(075eba5a),
                          "792-13_2.716",CRC(b87b9e6b),
                          "720-37_6.716",CRC(ceff6993))
BY51_SOUNDROM8(           "792-07_4.716",CRC(787ffd5e))
BY35_ROMEND
#define input_ports_spaceinv input_ports_by35
CORE_GAMEDEFNV(spaceinv,"Space Invaders",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Space invaders (7-digit bootleg)
/-------------------------------*/
INITGAME(spaceinb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(spaceinb,"inva2732.u2", CRC(3d21bb0c),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "792-07_4.716",CRC(787ffd5e))
BY35_ROMEND
#define input_ports_spaceinb input_ports_spaceinv
CORE_CLONEDEFNV(spaceinb,spaceinv,"Space Invaders (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Nitro Groundshaker
/-------------------------------*/
INITGAME(ngndshkr,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(ngndshkr,"776-17_1.716",CRC(f2d44235),
                          "776-11_2.716",CRC(b0396b55),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "776-15_4.716",CRC(63c80c52))
BY35_ROMEND
#define input_ports_ngndshkr input_ports_by35
CORE_GAMEDEFNV(ngndshkr,"Nitro Groundshaker",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Nitro Groundshaker (7-digit bootleg)
/-------------------------------*/
INITGAME(ngndshkb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(ngndshkb,"nitr2732.u2", CRC(3b8d62ef),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "776-15_4.716",CRC(63c80c52))
BY35_ROMEND
#define input_ports_ngndshkb input_ports_ngndshkr
CORE_CLONEDEFNV(ngndshkb,ngndshkr,"Nitro Groundshaker (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Silverball Mania
/-------------------------------*/
INITGAME(slbmania,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(slbmania,"786-16_1.716",CRC(c054733f),
                          "786-17_2.716",CRC(94af0298),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "786-11_4.716",CRC(2a3641e6))
BY35_ROMEND
#define input_ports_slbmania input_ports_by35
CORE_GAMEDEFNV(slbmania,"Silverball Mania",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Silverball Mania (7-digit bootleg)
/-------------------------------*/
INITGAME(slbmanib,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(slbmanib,"silv2732.u2", CRC(2687a4bc),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "786-11_4.716",CRC(2a3641e6))
BY35_ROMEND
#define input_ports_slbmanib input_ports_slbmania
CORE_CLONEDEFNV(slbmanib,slbmania,"Silverball Mania (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Rolling Stones
/-------------------------------*/
INITGAME(rollston,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(rollston,"796-17_1.716",CRC(51a826d7),
                          "796-18_2.716",CRC(08c75b1a),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "796-19_4.716",CRC(b740d047))
BY35_ROMEND
#define input_ports_rollston input_ports_by35
CORE_GAMEDEFNV(rollston,"Rolling Stones",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Rolling Stones (7-digit bootleg)
/-------------------------------*/
INITGAME(rollstob,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(rollstob,"roll2732.u2", CRC(28c48275),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "796-19_4.716",CRC(b740d047))
BY35_ROMEND
#define input_ports_rollstob input_ports_rollston
CORE_CLONEDEFNV(rollstob,rollston,"Rolling Stones (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Mystic
/-------------------------------*/
INITGAME(mystic  ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(mystic  ,"798-03_1.716",CRC(f9c91e3b),
                          "798-04_2.716",CRC(f54e5785),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "798-05_4.716",CRC(e759e093))
BY35_ROMEND
#define input_ports_mystic input_ports_by35
CORE_GAMEDEFNV(mystic  ,"Mystic",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Mystic (7-digit bootleg)
/-------------------------------*/
INITGAME(mysticb ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(mysticb, "myst2732.u2", CRC(ee743ef9),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "798-05_4.716",CRC(e759e093))
BY35_ROMEND
#define input_ports_mysticb input_ports_mystic
CORE_CLONEDEFNV(mysticb,mystic,"Mystic (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Hot Doggin
/-------------------------------*/
INITGAME(hotdoggn,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(hotdoggn,"809-05_1.716",CRC(2744abcb),
                          "809-06_2.716",CRC(03db3d4d),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "809-07_4.716",CRC(43f28d7f))
BY35_ROMEND
#define input_ports_hotdoggn input_ports_by35
CORE_GAMEDEFNV(hotdoggn,"Hot Doggin",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Hot Doggin (7-digit bootleg)
/-------------------------------*/
INITGAME(hotdoggb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(hotdoggb,"hotd2732.u2", CRC(709305ee),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "809-07_4.716",CRC(43f28d7f))
BY35_ROMEND
#define input_ports_hotdoggb input_ports_hotdoggn
CORE_CLONEDEFNV(hotdoggb,hotdoggn,"Hot Doggin (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Viking
/-------------------------------*/
INITGAME(viking  ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(viking  ,"802-05_1.716",CRC(a5db0574),
                          "802-06_2.716",CRC(40410760),
                          "720-35_6.716",CRC(78d6d289))
BY51_SOUNDROM8(           "802-07-4.716",CRC(62bc5030))
BY35_ROMEND
#define input_ports_viking input_ports_by35
CORE_GAMEDEFNV(viking  ,"Viking",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Viking (7-digit bootleg)
/-------------------------------*/
INITGAME(vikingb ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(vikingb, "vikg2732.u2", CRC(7a5c24e6),
                          "720-3532.u6b",CRC(cb4243e7))
BY51_SOUNDROM8(           "802-07-4.716",CRC(62bc5030))
BY35_ROMEND
#define input_ports_vikingb input_ports_viking
CORE_CLONEDEFNV(vikingb,viking,"Viking (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Skateball
/-------------------------------*/
INITGAME2(skatebll,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART880(skatebll,"823-24_1.716",CRC(46e797d1),
                          "823-25_2.716",CRC(960cb8c3),
                          "720-40_6.732",CRC(d7aaaa03))
BY51_SOUNDROM8(           "823-02_4.716",CRC(d1037b20))
BY35_ROMEND
#define input_ports_skatebll input_ports_by35
CORE_GAMEDEFNV(skatebll,"Skateball",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Frontier
/-------------------------------*/
INITGAME2(frontier,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART880(frontier,"819-08_1.716",CRC(e2f8ce9d),
                          "819-07_2.716",CRC(af023a85),
                          "720-40_6.732",CRC(d7aaaa03))
BY51_SOUNDROM8(           "819-09_4.716",CRC(a62059ca))
BY35_ROMEND
#define input_ports_frontier input_ports_by35
CORE_GAMEDEFNV(frontier,"Frontier",1980,"Bally",by35_mBY35_51S,0)

/* -------------------------------------*/
/* All games below use Sound Module -56 */
/* -------------------------------------*/
/*--------------------------------
/ Xenon
/-------------------------------*/
INITGAME2(xenon,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY56,0)
BY35_ROMSTART880(xenon,"811-40_1.716",CRC(0fba871b),
                       "811-41_2.716",CRC(1ea0d891),
                       "720-40_6.732",CRC(d7aaaa03))
BY56_SOUNDROM(         "811-35_4.532",CRC(e9caccbb))
BY57_SOUNDROM(         "811-22_1.532",CRC(c49a968e),
                       "811-23_2.532",CRC(41043996),
                       "811-24_3.532",CRC(53d65542),
                       "811-25_4.532",CRC(2c678631),
                       "811-26_5.532",CRC(b8e7febc),
                       "811-27_6.532",CRC(1e2a2afa),
                       "811-28_7.532",CRC(cebb4cd8))
BY35_ROMEND
#define input_ports_xenon input_ports_by35
CORE_GAMEDEFNV(xenon,"Xenon",1980,"Bally",by35_mBY35_56S,0)
/*--------------------------------
/ Xenon (French)
/-------------------------------*/
#define input_ports_xenonf input_ports_xenon
#define init_xenonf        init_xenon
BY35_ROMSTART880(xenonf,"811-40_1.716",CRC(0fba871b),
                        "811-41_2.716",CRC(1ea0d891),
                        "720-40_6.732",CRC(d7aaaa03))
BY56_SOUNDROM(          "811-36_4.532",CRC(73156c6e))
BY57_SOUNDROM(          "811-22_1.532",CRC(c49a968e),
                        "811-23_2.532",CRC(41043996),
                        "811-24_3.532",CRC(53d65542),
                        "811-29_4.532",CRC(e586ec31),
                        "811-30_5.532",CRC(e48d98e3),
                        "811-31_6.532",CRC(0a2336e5),
                        "811-32_7.532",CRC(987e6118))
BY35_ROMEND
CORE_CLONEDEFNV(xenonf,xenon,"Xenon (French)",1980,"Bally",by35_mBY35_56S,0)

/* -----------------------------------------------------------*/
/* All games below use Squalk N Talk -61 (except where noted) */
/* -----------------------------------------------------------*/
/*--------------------------------
/ Flash Gordon
/-------------------------------*/
INITGAME2(flashgdn,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(flashgdn,"834-23_2.732",CRC(0c7a0d91),
                          "720-52_6.732",CRC(2a43d9fb))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e),
                          "834-18_5.532",CRC(8799e80e))
BY35_ROMEND
#define input_ports_flashgdn input_ports_by35
CORE_GAMEDEFNV(flashgdn,"Flash Gordon",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Flash Gordon (french)
/-------------------------------*/
#define input_ports_flashgdf input_ports_flashgdn
#define init_flashgdf        init_flashgdn
BY35_ROMSTARTx00(flashgdf,"834-23_2.732",CRC(0c7a0d91),
                          "720-52_6.732",CRC(2a43d9fb))
BY61_SOUNDROM0xx0(        "834-35_2.532",CRC(dff3f711),
                          "834-36_5.532",CRC(18691897))
BY35_ROMEND
CORE_CLONEDEFNV(flashgdf,flashgdn,"Flash Gordon (French)",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Eight Ball Deluxe
/-------------------------------*/
INITGAME2(eballdlx,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(eballdlx,"838-15_2.732",CRC(68d92acc),
                          "720-52_6.732",CRC(2a43d9fb))
BY61_SOUNDROMx080(        "838-08_3.532",CRC(c39478d7),
                          "838-09_4.716",CRC(518ea89e),
                          "838-10_5.532",CRC(9c63925d))
BY35_ROMEND
#define input_ports_eballdlx input_ports_by35
CORE_GAMEDEFNV(eballdlx,"Eight Ball Deluxe",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Fireball II
/-------------------------------*/
INITGAME2(fball_ii,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(fball_ii,"839-12_2.732",CRC(45e768ad),
                          "720-52_6.732",CRC(2a43d9fb))
BY61_SOUNDROM0xx0(        "839-01_2.532",CRC(4aa473bd),
                          "839-02_5.532",CRC(8bf904ff))
BY35_ROMEND
#define input_ports_fball_ii input_ports_by35
CORE_GAMEDEFNV(fball_ii,"Fireball II",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Embryon
/-------------------------------*/
INITGAME2(embryon ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(embryon,"841-06_2.732",CRC(80ab18e7),
                         "720-52_6.732",CRC(2a43d9fb))
BY61_SOUNDROMxx80(       "841-01_4.716",CRC(e8b234e3),
                         "841-02_5.532",CRC(9cd8c04e))
BY35_ROMEND
#define input_ports_embryon input_ports_by35
CORE_GAMEDEFNV(embryon ,"Embryon",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Embryon (7-digit bootleg)
/-------------------------------*/
INITGAME2(embryonb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(embryonb,"embd71u2.bin",CRC(8a25d7e9),
                          "embd71u6.bin",CRC(20a3b3ce))
BY61_SOUNDROMxx80(        "841-01_4.716",CRC(e8b234e3),
                          "841-02_5.532",CRC(9cd8c04e))
BY35_ROMEND
#define input_ports_embryonb input_ports_embryon
CORE_CLONEDEFNV(embryonb,embryon ,"Embryon (7-digit bootleg)",2002,"Bally / Oliver",by35_mBY35_61BS,0)
/*--------------------------------
/ Fathom
/-------------------------------*/
INITGAME2(fathom  ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(fathom,"842-08_2.732",CRC(1180f284),
                        "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMxx00(      "842-01_4.532",CRC(2ac02093),
                        "842-02_5.532",CRC(736800bc))
BY35_ROMEND
#define input_ports_fathom input_ports_by35
CORE_GAMEDEFNV(fathom  ,"Fathom",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Medusa
/-------------------------------*/
static core_tLCDLayout medusaDisp[] = {
  DISP_SEG_IMPORT(dispBy7),
  {4,16,42,2,CORE_SEG7}, {4,21,44,2,CORE_SEG7}, {4,26,46,2,CORE_SEG7}, {0}
};
INITGAME2(medusa  ,GEN_BY35,medusaDisp,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0x10)
BY35_ROMSTARTx00(medusa,"845-16_2.732",CRC(b0fbd1ac),
                        "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMx008(      "845-01_3.532",CRC(32200e02),
                        "845-02_4.532",CRC(ab95885a),
                        "845-05_5.716",CRC(3792a812))
BY35_ROMEND
#define input_ports_medusa input_ports_by35
CORE_GAMEDEFNV(medusa  ,"Medusa",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Centaur
/-------------------------------*/
INITGAME2(centaur ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(centaur,"848-08_2.732",CRC(8bdcd32b),
                         "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMx008(       "848-01_3.532",CRC(88322c8a),
                         "848-02_4.532",CRC(d6dbd0e4),
                         "848-05_5.716",CRC(cbd765ba))
BY35_ROMEND
#define input_ports_centaur input_ports_by35
CORE_GAMEDEFNV(centaur,"Centaur",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Elektra
/-------------------------------*/
static core_tLCDLayout elektraDisp[] = {
  DISP_SEG_IMPORT(dispBy7),
  {4,21,44,2,CORE_SEG7}, {0}
};
INITGAME2(elektra ,GEN_BY35,elektraDisp,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0x80)
BY35_ROMSTARTx00(elektra,"857-04_2.732",CRC(d2476720),
                         "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMx008(       "857-01_3.532",CRC(031548cc),
                         "857-02_4.532",CRC(efc870d9),
                         "857-03_5.716",CRC(eae2c6a6))
BY35_ROMEND
#define input_ports_elektra input_ports_by35
CORE_GAMEDEFNV(elektra,"Elektra",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Vector
/-------------------------------*/
static core_tLCDLayout vectorDisp[] = {
  DISP_SEG_IMPORT(dispBy7),
  {4,17,42,3,CORE_SEG7}, {4,24,45,3,CORE_SEG7}, {0}
};
INITGAME2(vector  ,GEN_BY35,vectorDisp,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0x80)
BY35_ROMSTARTx00(vector,"858-11_2.732",CRC(323e286b),
                        "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROM0000(      "858-01_2.532",CRC(bd2edef9),
                        "858-02_3.532",CRC(c592fb35),
                        "858-03_4.532",CRC(8661d312),
                        "858-06_5.532",CRC(3050edf6))
BY35_ROMEND
#define input_ports_vector input_ports_by35
CORE_GAMEDEFNV(vector ,"Vector",1982,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Spectrum
/-------------------------------*/
INITGAME2(spectrum,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(spectrum,"868-00_2.732",NO_DUMP,
                          "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMx008(        "868-01_3.532",CRC(c3a16c66),
                          "868-02_4.532",CRC(6b441399),
                          "868-03_5.716",CRC(4a5ac3b8))
BY35_ROMEND
#define input_ports_spectrum input_ports_by35
CORE_GAMEDEFNV(spectrum,"Spectrum",1982,"Bally",by35_mBY35_61BS,0)
#define input_ports_spectru4 input_ports_spectrum
#define init_spectru4        init_spectrum
BY35_ROMSTARTx00(spectru4,"868-04_2.732",CRC(b377f5f1),
                          "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMx008(        "868-01_3.532",CRC(c3a16c66),
                          "868-02_4.532",CRC(6b441399),
                          "868-03_5.716",CRC(4a5ac3b8))
BY35_ROMEND
CORE_CLONEDEFNV(spectru4,spectrum,"Spectrum (Ver. 4)",1982,"Bally",by35_mBY35_61BS,0)

/*--------------------------------------------------
/ Speak Easy 2 Player - Uses AS2518-51 Sound Board
/--------------------------------------------------*/
INITGAME2(speakesy,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(speakesy,"877-03_2.732",CRC(34b28bbc),
                          "720-53_6.732",CRC(c2e92f80))
BY51_SOUNDROM8(           "877-01_4.716",CRC(6534e826))
BY35_ROMEND
#define input_ports_speakesy input_ports_by35
CORE_GAMEDEFNV(speakesy,"Speakeasy",1982,"Bally",by35_mBY35_51S,0)

/*-------------------------------------------------
/ Speak Easy 4 Player - Uses AS2518-51 Sound Board
/-------------------------------------------------*/
#define input_ports_speakes4 input_ports_speakesy
#define init_speakes4        init_speakesy
BY35_ROMSTARTx00(speakes4,"877-04_2.732",CRC(8926f2bb),
                          "720-53_6.732",CRC(c2e92f80))
BY51_SOUNDROM8(           "877-01_4.716",CRC(6534e826))
BY35_ROMEND
CORE_CLONEDEFNV(speakes4,speakesy,"Speakeasy 4 Player",1982,"Bally",by35_mBY35_51S,0)

/*--------------------------------
/ Rapid Fire
/-------------------------------*/
static core_tLCDLayout dispRapid[] = {
  {0, 0, 1,7,CORE_SEG87F}, {0,16, 9,7,CORE_SEG87F},
  {2, 4,33,2,CORE_SEG7},   {2,10,36,2,CORE_SEG7},
  {2,16,41,7,CORE_SEG87F}, {0}
};
INITGAME2(rapidfir,GEN_BY35,dispRapid,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(rapidfir,"869-04_2.732",CRC(26fdf048),
                          "869-03_6.732",CRC(f6af5e8d))
BY61_SOUNDROMxxx0(        "869-02_5.532",CRC(5a74cb86))
BY35_ROMEND
#define input_ports_rapidfir input_ports_by35
CORE_GAMEDEFNV(rapidfir,"Rapid Fire",1982,"Bally",by35_mBY35_61BS,0)

/*--------------------------------------
/ Mr. and Mrs. Pacman (BY35-872: 05/82)
/--------------------------------------*/
static core_tLCDLayout m_mpacDisp[] = {
  DISP_SEG_IMPORT(dispBy7),
  {4,16,42,2,CORE_SEG7}, {4,21,44,2,CORE_SEG7}, {4,26,46,2,CORE_SEG7}, {0}
};
INITGAME2(m_mpac,GEN_BY35,m_mpacDisp,FLIP_SWNO(33,37),8,SNDBRD_BY61,0x80)
BY35_ROMSTARTx00(m_mpac,"872-04_2.732",CRC(5e542882),
                        "720-53_6.732",CRC(c2e92f80))
BY61_SOUNDROMxx00(      "872-01_4.532",CRC(d21ce16d),
                        "872-03_5.532",CRC(8fcdf853))
BY35_ROMEND
#define input_ports_m_mpac input_ports_by35
CORE_GAMEDEFNV(m_mpac  ,"Mr. and Mrs. PacMan",1982,"Bally",by35_mBY35_61S,0)

/*---------------------------------------------------
/ BMX (BY35-888: 11/82) - Uses AS2518-51 Sound Board
/----------------------------------------------------*/
INITGAME2(bmx,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(bmx,"888-03_2.732",CRC(038cf1be),
                     "720-53_6.732",CRC(c2e92f80))
BY51_SOUNDROM0(      "888-02_4.532",CRC(5692c679))
BY35_ROMEND
#define input_ports_bmx input_ports_by35
CORE_GAMEDEFNV(bmx,"BMX",1983,"Bally",by35_mBY35_51S,0)

/*-----------------------------------------------------------
/ Grand Slam (BY35-???: 01/83) - Uses AS2888-51 Sound Board
/-----------------------------------------------------------*/
static core_tLCDLayout granslamDisp[] = {
  DISP_SEG_IMPORT(dispBy6),
  {4,14,42,2,CORE_SEG7}, {4,19,44,2,CORE_SEG7}, {4,24,46,2,CORE_SEG7}, {0}
};
INITGAME2(granslam ,GEN_BY35,granslamDisp,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0x80)
BY35_ROMSTARTx00(granslam, "grndslam.u2",CRC(66aea9dc),
                           "grndslam.u6",CRC(9e6ccea1))
BY51_SOUNDROM0(            "grndslam.u4",CRC(ac34bc38))
BY35_ROMEND
#define input_ports_granslam input_ports_by35
CORE_GAMEDEFNV(granslam,"Grand Slam",1983,"Bally",by35_mBY35_51S,0)

/*----------------------------------------------------------
/ Gold Ball (BY35-???: 10/83)  - Uses AS2518-51 Sound Board
/----------------------------------------------------------*/
INITGAME2(goldball,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(goldball,"gold2732.u2",CRC(3169493c),
                          "720-5332.u6",CRC(c2e92f80))
BY51_SOUNDROM0(           "gb_u4.532",  CRC(2dcb0315))
BY35_ROMEND
#define input_ports_goldball input_ports_by35
CORE_GAMEDEFNV(goldball,"Gold Ball",1983,"Bally",by35_mBY35_51S,GAME_NOT_WORKING)

/********************************************************/
/******* Games Below use Cheap Squeak Sound Board *******/
/********************************************************/
/*--------------------------------
/ X's & O's (BY35-???: 12/83)
/-------------------------------*/
INITGAME2(xsandos ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(xsandos ,"x&os2732.u2",CRC(068dfe5a),
                          "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROMx2(          "720_u3.snd", CRC(5d8e2adb))
BY35_ROMEND
#define input_ports_xsandos input_ports_by35
CORE_GAMEDEFNV(xsandos ,"X's & O's",1983,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Kings of Steel (BY35-???: 05/84)
/-------------------------------*/
INITGAME2(kosteel ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(kosteel ,"kngs2732.u2",CRC(f876d8f2),
                          "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROM11(          "kngsu3.snd", CRC(11b02dca),
                          "kngsu4.snd", CRC(f3e4d2f6))
BY35_ROMEND
#define input_ports_kosteel input_ports_by35
CORE_GAMEDEFNV(kosteel ,"Kings of Steel",1984,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Black Pyramid (BY35-???: 07/84)
/-------------------------------*/
INITGAME2(blakpyra,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(blakpyra,"blkp2732.u2",CRC(600535b0),
                          "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROM11(          "bp_u3.532",  CRC(a5005067),
                          "bp_u4.532",  CRC(57978b4a))
BY35_ROMEND
#define input_ports_blakpyra input_ports_by35
CORE_GAMEDEFNV(blakpyra,"Black Pyramid",1984,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Spy Hunter (BY35-???: 10/84)
/-------------------------------*/
INITGAME2(spyhuntr,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(spyhuntr, "spy-2732.u2",CRC(9e930f2d),
                           "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROM11(           "spy_u3.532", CRC(95ffc1b8),
                           "spy_u4.532", CRC(a43887d0))
BY35_ROMEND
#define input_ports_spyhuntr input_ports_by35
CORE_GAMEDEFNV(spyhuntr,"Spy Hunter",1984,"Bally",by35_mBY35_45S,0)

/*-------------------------------------
/ Fireball Classic (BY35-???: 02/85)
/------------------------------------*/
INITGAME2(fbclass ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(fbclass ,"fb-class.u2",CRC(32faac6c),
                          "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROM11(          "fbcu3.snd",  CRC(1ad71775),
                          "fbcu4.snd",  CRC(697ab16f))
BY35_ROMEND
#define input_ports_fbclass input_ports_by35
CORE_GAMEDEFNV(fbclass ,"Fireball Classic",1985,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Cybernaut (BY35-???: 05/85)
/-------------------------------*/
INITGAME2(cybrnaut,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(cybrnaut,"cybe2732.u2",CRC(0610b0e0),
                          "720-5332.u6",CRC(c2e92f80))
BY45_SOUNDROMx2(          "cybu3.snd",  CRC(a3c1f6e7))
BY35_ROMEND
#define input_ports_cybrnaut input_ports_by35
CORE_GAMEDEFNV(cybrnaut,"Cybernaut",1985,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Mystic Star (Zaccaria game, 1984)
/-------------------------------*/
INITGAME(myststar,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(myststar,"rom1.bin",CRC(9a12dc91),
                          "rom2.bin",CRC(888ee5ae),
                          "rom3.bin",CRC(9e0a4619))
BY50_SOUNDROM(            "snd.123", NO_DUMP)
BY35_ROMEND
#define input_ports_myststar input_ports_by35
CORE_GAMEDEFNV(myststar,"Mystic Star",1984,"Zaccaria",by35_mBY35_50S,0)
