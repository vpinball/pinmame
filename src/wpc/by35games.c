#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "wmssnd.h"
#include "by35.h"
#include "machine/6821pia.h"

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

#define INITGAME(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME2(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_NOSOUNDE}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME2B(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_NOSOUNDE | BY35GD_SWVECTOR}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME3(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb,db,BY35GD_PHASE}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/* -------------------------------------------------------------*/
/* All games below use CHIMES for sound - ie, no sound hardware */
/* -------------------------------------------------------------*/

/*--------------------------------
/ Freedom
/-------------------------------*/
INITGAME(freedom,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTART228(freedom,"720-08_1.474",CRC(b78bceeb) SHA1(acf6f1a497ada344211f12dbf4be619bee559950),
                         "720-10_2.474",CRC(ca90c8a7) SHA1(d9b5e95247e846e50a2a43c85ad5eb1fc761ab67),
                         "720-07_6.716",CRC(0f4e8b83) SHA1(faa05dde24eb60be0cdc4456ae2e660a15ed85ac))
BY17_ROMEND
#define input_ports_freedom input_ports_by35
CORE_GAMEDEFNV(freedom,"Freedom",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Night Rider
/-------------------------------*/
INITGAME(nightrdr,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTART8x8(nightrdr,"721-21_1.716",CRC(237c4060) SHA1(4ce3dba9189fe7666fc76a2c8ee7fff9b12d4c00),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_nightrdr input_ports_by35
CORE_GAMEDEFNV(nightrdr,"Night Rider",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Evel Knievel
/-------------------------------*/
INITGAME(evelknie,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(evelknie,"722-17_2.716",CRC(b6d9a3fa) SHA1(1939e13f73a324e3d2fd269a54446f48cf530f50),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_evelknie input_ports_by35
CORE_GAMEDEFNV(evelknie,"Evel Knievel",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Eight Ball
/-------------------------------*/
INITGAME(eightbll,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(eightbll,"723-20_2.716",CRC(33559e7b) SHA1(49008db95c8f012e7e3b613e6eee811512207fa9),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_eightbll input_ports_by35
CORE_GAMEDEFNV(eightbll,"Eight Ball",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Power Play
/-------------------------------*/
INITGAME(pwerplay,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(pwerplay,"724-25_2.716",CRC(43012f35) SHA1(f90d582e3394d949a637a09882ffdad7664c44c0),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_pwerplay input_ports_by35
CORE_GAMEDEFNV(pwerplay,"Power Play",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Mata Hari
/-------------------------------*/
INITGAME(matahari,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(matahari,"725-21_2.716",CRC(63acd9b0) SHA1(2347342f1281c097ea39c79236d85b00a1dfc7b2),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_matahari input_ports_by35
CORE_GAMEDEFNV(matahari,"Mata Hari",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)

INITGAME(matatest,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY35_ROMSTARTx00(matatest,"matat0n.u2",CRC(64a6bb3c) SHA1(ced5fcd18009106ac3c7b42e36cdc10ce410eeeb),
                          "ptestn.u6", CRC(ccb213ec) SHA1(8defbe8e11a75d26daf1351f439dd409f1efc608))
BY35_ROMEND
#define input_ports_matatest input_ports_matahari
CORE_CLONEDEFNV(matatest,matahari,"Mata Hari (New game rules)",2005,"Oliver",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Strikes and Spares
/-------------------------------*/
INITGAME(stk_sprs,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(stk_sprs,"740-16_2.716",CRC(2be27024) SHA1(266dee3a5c4c115acc20543df2eb172f1e85dacb),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
BY17_ROMEND
#define input_ports_stk_sprs input_ports_by35
CORE_GAMEDEFNV(stk_sprs,"Strikes and Spares",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Black Jack
/-------------------------------*/
INITGAME(blackjck,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0,0,0)
BY17_ROMSTARTx88(blackjck,"728-32_2.716",CRC(1333c9d1) SHA1(1fbb60d84db47ffaf7f291575b2705783a110678),
                          "720-20_6.716",CRC(0c17aa4d) SHA1(729e61a29691857112579efcdb96a35e8e5b1279))
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
BY35_ROMSTART888(lostwrld,"729-33_1.716",CRC(4ca40b95) SHA1(4b4a3fbffb0aa99dab6330e24f93605eee35ac54),
                          "729-48_2.716",CRC(963bffd8) SHA1(5144092d019132946b396fd7134866a878b3ca62),
                          "720-28_6.716",CRC(f24cce3e) SHA1(0dfeaeb5b1cf4c950ff530ee56966ac0f2257111))
BY32_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_lostwrld input_ports_by35
CORE_GAMEDEFNV(lostwrld,"Lost World",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Six Million Dollar Man
/-------------------------------*/
static core_tLCDLayout smmanDisp[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 0,50,6,CORE_SEG7}, {4,14,42,6,CORE_SEG7},
  {6, 2,35,2,CORE_SEG7}, {6, 8,38,2,CORE_SEG7}, {0}
};
INITGAME(smman,GEN_BY35,smmanDisp,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0x81)
BY35_ROMSTART888(smman,"742-20_1.716",CRC(33e55a75) SHA1(98fbec07c9d03557654e5b67e29738c66156ec62),
                       "742-18_2.716",CRC(5365d36c) SHA1(1db651d31e28cf3fda00bef5289bb14d3b37b3c1),
                       "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY32_SOUNDROM(         "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_smman input_ports_by35
CORE_GAMEDEFNV(smman,"Six Million Dollar Man",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Playboy
/-------------------------------*/
INITGAME(playboy,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(playboy,"743-14_1.716",CRC(5c40984a) SHA1(dea104242fcb6d604faa0f01f087bc58bd43cd9d),
                         "743-12_2.716",CRC(6fa66664) SHA1(4943220942ce74d4620eb5fbbab8f8a763f65a2e),
                         "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY32_SOUNDROM(           "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_playboy input_ports_by35
CORE_GAMEDEFNV(playboy,"Playboy",1978,"Bally",by35_mBY35_32S,0)

INITGAME(playboyb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(playboyb,"play2732.u2", CRC(da49e8ce) SHA1(fa2b7731e6ade119b1c18e85e15bdc21ea9e46af),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_playboyb input_ports_by35
CORE_CLONEDEFNV(playboyb,playboy, "Playboy (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)


/*--------------------------------
/ Voltan Escapes Cosmic Doom
/-------------------------------*/
INITGAME(voltan,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(voltan,"744-03_1.716",CRC(ad2467ae) SHA1(58c4de1ea696372bce9146a4c48a296ebcb2c431),
                        "744-04_2.716",CRC(dbf58b83) SHA1(2d5e1c42fb8987eec81d89a4fe758ff0b88a1889),
                        "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY32_SOUNDROM(          "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_voltan input_ports_by35
CORE_GAMEDEFNV(voltan,"Voltan Escapes Cosmic Doom",1978,"Bally",by35_mBY35_32S,0)


INITGAME(voltanb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(voltanb,"volt2732.u2", CRC(a4670a54) SHA1(7bb792e388d52bd350e38c02fcde2f8ed9993dc1),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_voltanb input_ports_by35
CORE_CLONEDEFNV(voltanb,voltan, "Voltan Escapes Cosmic Doom (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)




/*--------------------------------
/ Supersonic
/-------------------------------*/
INITGAME(sst,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY32,0)
BY35_ROMSTART888(sst,"741-10_1.716",CRC(5e4cd81a) SHA1(d2a4a3599ad7271cd0ddc376c31c9b2e8defa379),
                     "741-08_2.716",CRC(2789cbe6) SHA1(8230657cb5ee793354a5d4a80a9348639ec9af8f),
                     "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY32_SOUNDROM(       "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_sst input_ports_by35
CORE_GAMEDEFNV(sst,"Supersonic",1979,"Bally",by35_mBY35_32S,0)

INITGAME(sstb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(sstb,"surp2732.u2", CRC(4987f46e) SHA1(a32984f29ba41c8c03883cb80797c55b20d1ce42),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_sstb input_ports_by35
CORE_CLONEDEFNV(sstb,sst, "Supersonic (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)


/* -------------------------------------*/
/* All games below use Sound Module -50 */
/* -------------------------------------*/
/*--------------------------------
/ Star Trek
/-------------------------------*/
INITGAME(startrek,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(startrek,"745-11_1.716",CRC(a077efca) SHA1(6f78d9a43db0b99c3818a73a04d15aa300194a6d),
                          "745-12_2.716",CRC(f683210a) SHA1(6120909d97269d9abfcc34eef2c79b56a9cf53bc),
                          "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_startrek input_ports_by35
CORE_GAMEDEFNV(startrek, "Star Trek",1979,"Bally",by35_mBY35_50S,0)

INITGAME(startreb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(startreb,"star2732.u2", CRC(34dd99c3) SHA1(86dd5b46873c1910311504bdbfcd340317109be6),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(            "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_startreb input_ports_by35
CORE_CLONEDEFNV(startreb,startrek, "Star Trek (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)

/*--------------------------------
/ Paragon
/-------------------------------*/
INITGAME(paragon,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(paragon,"748-17_1.716",CRC(08dbdf32) SHA1(43d1380d809683e74d67b6cf57c6eb0ad248a813),
                         "748-15_2.716",CRC(26cc05c1) SHA1(6e11a0f2327dbf15f6c149ddd873d9af96597d9d),
                         "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY50_SOUNDROM(           "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_paragon input_ports_by35
CORE_GAMEDEFNV(paragon,"Paragon",1979,"Bally",by35_mBY35_50S,0)

INITGAME(paragonb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(paragonb,"para2732.u2", CRC(b3c990a1) SHA1(cb90c5fa52fefc29574a86d0f39fd29b2a70b8f2),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(           "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_paragonb input_ports_by35
CORE_CLONEDEFNV(paragonb,paragon, "Paragon (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)



/*--------------------------------
/ Harlem Globetrotters
/-------------------------------*/
INITGAME(hglbtrtr,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(hglbtrtr,"750-07_1.716",CRC(da594719) SHA1(0aaa50e7d62da64f88d82b00cf0747945be88818),
                          "750-08_2.716",CRC(3c783931) SHA1(ee260511063aff1b72e18b3bc5a5be81aecf10c9),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_hglbtrtr input_ports_by35
CORE_GAMEDEFNV(hglbtrtr,"Harlem Globetrotters",1979,"Bally",by35_mBY35_50S,0)

INITGAME(hglbtrtb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(hglbtrtb,"harl2732.u2", CRC(f70a2981) SHA1(dd3e6448efa0dff49ed84c1f586d3b817598fa31),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_hglbtrtb input_ports_hglbtrtr
CORE_CLONEDEFNV(hglbtrtb,hglbtrtr,"Harlem Globetrotters (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_50S,0)
/*--------------------------------
/ Dolly Parton
/-------------------------------*/
INITGAME(dollyptn,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(dollyptn,"777-10_1.716",CRC(ca88cb9a) SHA1(0deac1c02b2121635af4bd76a6695d8abc09d694),
                          "777-13_2.716",CRC(7fc93ea3) SHA1(534ac5ed34397fe622dcf7cc90eaf38a311fa871),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_dollyptn input_ports_by35
CORE_GAMEDEFNV(dollyptn,"Dolly Parton",1979,"Bally",by35_mBY35_50S,0)

INITGAME(dollyptb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(dollyptb,"doll2732.u2", CRC(cd649da3) SHA1(10fbffa0dc620d8bc35b8236b1d55fbf3338b6b7),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY50_SOUNDROM(            "729-51_3.123",CRC(6e7d3e8b) SHA1(7a93d82a05213ffa6eacfa318051414f872a701d))
BY35_ROMEND
#define input_ports_dollyptb input_ports_dollyptn
CORE_CLONEDEFNV(dollyptb,dollyptn,"Dolly Parton (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_50S,0)
/*--------------------------------
/ Kiss
/-------------------------------*/
INITGAME(kiss,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY50,0)
BY35_ROMSTART888(kiss,"746-11_1.716",CRC(78ec7fad) SHA1(b7e47ed14be08571b620de71cd5006faaddc88d5),
                      "746-14_2.716",CRC(0fc8922d) SHA1(dc6bd4d2d744df69b33ec69896cf71ac10c14a35),
                      "720-30_6.716",CRC(4be8aab0) SHA1(b6ae0c4f27b7dd7fb13c0632617a2559f86f29ae))
BY50_SOUNDROM(        "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_kiss input_ports_by35
CORE_GAMEDEFNV(kiss,"Kiss",1979,"Bally",by35_mBY35_50S,0)
INITGAME(kissb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTARTx00(kissb,"kiss2732.u2", CRC(716adcfd) SHA1(048e3142cfa307ea4552e6af3812b0d7301b62ad),
                          "3032d7.bin",CRC(c0fc5342) SHA1(0511162ac54e1c630c7460cec7311bc928baf656))
BY50_SOUNDROM(        "729-18_3.123",CRC(7b6b7d45) SHA1(22f791bac0baab71754b2f6c00c217a342c92df5))
BY35_ROMEND
#define input_ports_kissb input_ports_by35
CORE_CLONEDEFNV(kissb,kiss,"Kiss (7-digit conversion)",2005,"Bally / Oliver",by35_mBY35_50S,0)
/* -------------------------------------*/
/* All games below use Sound Module -51 */
/* -------------------------------------*/
/*--------------------------------
/ Future Spa
/-------------------------------*/
INITGAME(futurspa,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTART888(futurspa,"781-07_1.716",CRC(4c716a6a) SHA1(a19ff17079b7ef0b9e6933ffc718dee0236bae10),
                          "781-09_2.716",CRC(316617ed) SHA1(749d63cefe9541885b51db89302ad8a23e8f5b0a),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "781-02_4.716",CRC(364f7c9a) SHA1(e6a3d425317eaeba4109712c6949f11c50b82892))
BY35_ROMEND
#define input_ports_futurspa input_ports_by35
CORE_GAMEDEFNV(futurspa,"Future Spa",1979,"Bally",by35_mBY35_51S,0)

INITGAME(futurspb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(futurspb,"fspa2732.u2", CRC(8c4bf58f) SHA1(7235b205b9817d6e227683b26779d3fd4f1df11b),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "781-02_4.716",CRC(364f7c9a) SHA1(e6a3d425317eaeba4109712c6949f11c50b82892))
BY35_ROMEND
#define input_ports_futurspb input_ports_futurspa
CORE_CLONEDEFNV(futurspb,futurspa,"Future Spa (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Space Invaders
/-------------------------------*/
INITGAME(spaceinv,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTART888(spaceinv,"792-10_1.716",CRC(075eba5a) SHA1(7147c2dfb6af1c39bbfb9e98f409baae10d09628),
                          "792-13_2.716",CRC(b87b9e6b) SHA1(eab787ea81409ba88e30a342564944e1fade8124),
                          "720-37_6.716",CRC(ceff6993) SHA1(bc91e7afdfc441ff47a37031f2d6caeb9ab64143))
BY51_SOUNDROM8(           "792-07_4.716",CRC(787ffd5e) SHA1(4dadad7095de27622c2120311a84555dacdc3364))
BY35_ROMEND
#define input_ports_spaceinv input_ports_by35
CORE_GAMEDEFNV(spaceinv,"Space Invaders",1980,"Bally",by35_mBY35_51S,0)

INITGAME(spaceinb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(spaceinb,"inva2732.u2", CRC(3d21bb0c) SHA1(b361adc3f0d83b873101ff5354d2d3b876128f4a),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "792-07_4.716",CRC(787ffd5e) SHA1(4dadad7095de27622c2120311a84555dacdc3364))
BY35_ROMEND
#define input_ports_spaceinb input_ports_spaceinv
CORE_CLONEDEFNV(spaceinb,spaceinv,"Space Invaders (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Nitro Groundshaker
/-------------------------------*/
INITGAME(ngndshkr,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(ngndshkr,"776-17_1.716",CRC(f2d44235) SHA1(282106767b5ec5180fa8e7eb2eb5b4766849c920),
                          "776-11_2.716",CRC(b0396b55) SHA1(2d10c4af7ecfa23b64ffb640111b582f44256fd5),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "776-15_4.716",CRC(63c80c52) SHA1(3350919fce237b308b8f960948f70d01d312e9c0))
BY35_ROMEND
#define input_ports_ngndshkr input_ports_by35
CORE_GAMEDEFNV(ngndshkr,"Nitro Groundshaker",1980,"Bally",by35_mBY35_51S,0)

INITGAME(ngndshkb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(ngndshkb,"nitr2732.u2", CRC(3b8d62ef) SHA1(da9652de8930bdd092928775a886252798ce6bf8),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "776-15_4.716",CRC(63c80c52) SHA1(3350919fce237b308b8f960948f70d01d312e9c0))
BY35_ROMEND
#define input_ports_ngndshkb input_ports_ngndshkr
CORE_CLONEDEFNV(ngndshkb,ngndshkr,"Nitro Groundshaker (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Silverball Mania
/-------------------------------*/
INITGAME(slbmania,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(slbmania,"786-16_1.716",CRC(c054733f) SHA1(2699cf940ce40012e2d7554b0b130adcb2bec6d1),
                          "786-17_2.716",CRC(94af0298) SHA1(579eb0290283194d92b172f787d8a9ff54f16a07),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "786-11_4.716",CRC(2a3641e6) SHA1(64693d424277e2aaf5fd4af33b2d348a8a455448))
BY35_ROMEND
#define input_ports_slbmania input_ports_by35
CORE_GAMEDEFNV(slbmania,"Silverball Mania",1980,"Bally",by35_mBY35_51S,0)

INITGAME(slbmanib,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(slbmanib,"silv2732.u2", CRC(2687a4bc) SHA1(88eb8f793d44b820a0e79789db8cd48eec451c73),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "786-11_4.716",CRC(2a3641e6) SHA1(64693d424277e2aaf5fd4af33b2d348a8a455448))
BY35_ROMEND
#define input_ports_slbmanib input_ports_slbmania
CORE_CLONEDEFNV(slbmanib,slbmania,"Silverball Mania (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Rolling Stones
/-------------------------------*/
INITGAME(rollston,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(rollston,"796-17_1.716",CRC(51a826d7) SHA1(6811149c8948066b85b4018802afd409dbe8c2e1),
                          "796-18_2.716",CRC(08c75b1a) SHA1(792a535514fe4d9476914f7f61c696a7a1bdb549),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "796-19_4.716",CRC(b740d047) SHA1(710edb6bbba0a03e4f516b501f019493a3a4033e))
BY35_ROMEND
#define input_ports_rollston input_ports_by35
CORE_GAMEDEFNV(rollston,"Rolling Stones",1980,"Bally",by35_mBY35_51S,0)

INITGAME(rollstob,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(rollstob,"roll2732.u2", CRC(28c48275) SHA1(578f774bbeca228ef381531563d1d56dc9b612e5),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "796-19_4.716",CRC(b740d047) SHA1(710edb6bbba0a03e4f516b501f019493a3a4033e))
BY35_ROMEND
#define input_ports_rollstob input_ports_rollston
CORE_CLONEDEFNV(rollstob,rollston,"Rolling Stones (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Mystic
/-------------------------------*/
INITGAME(mystic  ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(mystic  ,"798-03_1.716",CRC(f9c91e3b) SHA1(a3e6600b7b809cdd51a2d61b679f4f45ecf16e99),
                          "798-04_2.716",CRC(f54e5785) SHA1(425304512b70ef0f17ca9854af96cbb63c5ee33e),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "798-05_4.716",CRC(e759e093) SHA1(e635dac4aa925804ec658e856f7830290bfbc7b8))
BY35_ROMEND
#define input_ports_mystic input_ports_by35
CORE_GAMEDEFNV(mystic  ,"Mystic",1980,"Bally",by35_mBY35_51S,0)

INITGAME(mysticb ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(mysticb, "myst2732.u2", CRC(ee743ef9) SHA1(121d3d912983c70d967e9f9fc2230faa59fd37cd),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "798-05_4.716",CRC(e759e093) SHA1(e635dac4aa925804ec658e856f7830290bfbc7b8))
BY35_ROMEND
#define input_ports_mysticb input_ports_mystic
CORE_CLONEDEFNV(mysticb,mystic,"Mystic (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Hot Doggin
/-------------------------------*/
INITGAME(hotdoggn,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(hotdoggn,"809-05_1.716",CRC(2744abcb) SHA1(b45bd58c365785d12f9bec381574058e29f33fd2),
                          "809-06_2.716",CRC(03db3d4d) SHA1(b8eed2d22474d2b0a1667eef2fdd4ecfa5fd35f3),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "809-07_4.716",CRC(43f28d7f) SHA1(01fca0ee0137a0715421eaa3582ff8d324340ecf))
BY35_ROMEND
#define input_ports_hotdoggn input_ports_by35
CORE_GAMEDEFNV(hotdoggn,"Hot Doggin",1980,"Bally",by35_mBY35_51S,0)

INITGAME(hotdoggb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(hotdoggb,"hotd2732.u2", CRC(709305ee) SHA1(37d5e681a1a2b8b2782dae3007db3e5036003e00),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "809-07_4.716",CRC(43f28d7f) SHA1(01fca0ee0137a0715421eaa3582ff8d324340ecf))
BY35_ROMEND
#define input_ports_hotdoggb input_ports_hotdoggn
CORE_CLONEDEFNV(hotdoggb,hotdoggn,"Hot Doggin (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Viking
/-------------------------------*/
INITGAME(viking  ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART888(viking  ,"802-05_1.716",CRC(a5db0574) SHA1(d9836679ed797b649f2c1e22bc24e8a9fe1c3000),
                          "802-06_2.716",CRC(40410760) SHA1(b0b87d8600a03de7090e42f6ebdeeb5feccf87f6),
                          "720-35_6.716",CRC(78d6d289) SHA1(47c3005790119294309f12ea68b7e573f360b9ef))
BY51_SOUNDROM8(           "802-07-4.716",CRC(62bc5030) SHA1(5a696f784a415d5b16ee23cd72a905264a2bbeac))
BY35_ROMEND
#define input_ports_viking input_ports_by35
CORE_GAMEDEFNV(viking  ,"Viking",1980,"Bally",by35_mBY35_51S,0)

INITGAME(vikingb ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(vikingb, "vikg2732.u2", CRC(7a5c24e6) SHA1(073b16eb6d77684ba99e3bf340a3bc9dfb537560),
                          "720-3532.u6b",CRC(b5e6a3d5) SHA1(fa1593eeed449dbac87965e613b501108a015eb2))
BY51_SOUNDROM8(           "802-07-4.716",CRC(62bc5030) SHA1(5a696f784a415d5b16ee23cd72a905264a2bbeac))
BY35_ROMEND
#define input_ports_vikingb input_ports_viking
CORE_CLONEDEFNV(vikingb,viking,"Viking (7-digit conversion)",2002,"Bally / Oliver",by35_mBY35_51S,0)
/*--------------------------------
/ Skateball
/-------------------------------*/
INITGAME2(skatebll,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART880(skatebll,"823-24_1.716",CRC(46e797d1) SHA1(7ddbf6047b8d95af8727c32b056bee1c4aa228e4),
                          "823-25_2.716",CRC(960cb8c3) SHA1(3a4499cab85d3563961b0a01c78fa1f3ba2188fe),
                          "720-40_6.732",CRC(d7aaaa03) SHA1(4e0b901645e509bcb59bf81a6ffc1612b4fb16ee))
BY51_SOUNDROM8(           "823-02_4.716",CRC(d1037b20) SHA1(8784728540573be5e8ebb940ec0046b778f9413b))
BY35_ROMEND
#define input_ports_skatebll input_ports_by35
CORE_GAMEDEFNV(skatebll,"Skateball",1980,"Bally",by35_mBY35_51S,0)


/*--------------------------------
/ Skateball custom rom rev 3
/-------------------------------*/
INITGAME2(skateblb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(skateblb,"skate02.u2",CRC(91607e8a) SHA1(00d4ff84acc594037b8b90504af11ff648f8d746),
                          "skate02.u6",CRC(b93e27c7) SHA1(86feb778571cb507c2059acc08101e1eb1c7a26e))
BY51_SOUNDROM8(           "823-02_4.716",CRC(d1037b20) SHA1(8784728540573be5e8ebb940ec0046b778f9413b))
BY35_ROMEND
#define input_ports_skateblb input_ports_skatebll
CORE_CLONEDEFNV(skateblb,skatebll,"Skateball (alternate set rev. 3)",2005,"Bally / Oliver",by35_mBY35_51S,0)


/*--------------------------------
/ Frontier
/-------------------------------*/
INITGAME2(frontier,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTART880(frontier,"819-08_1.716",CRC(e2f8ce9d) SHA1(03b38486e12f1677dcabcd0f14d194c59b3bd214),
                          "819-07_2.716",CRC(af023a85) SHA1(95df232ba654293066beccbad158146259a764b7),
                          "720-40_6.732",CRC(d7aaaa03) SHA1(4e0b901645e509bcb59bf81a6ffc1612b4fb16ee))
BY51_SOUNDROM8(           "819-09_4.716",CRC(a62059ca) SHA1(75e139ea2573a8c3b666c9a1024d9308da9875c7))
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
BY35_ROMSTART880(xenon,"811-40_1.716",CRC(0fba871b) SHA1(52bc0ef65507f0f7422c319d0dc2059e12deab6d),
                       "811-41_2.716",CRC(1ea0d891) SHA1(98cd8cfed5c0f437d2b9423b31205f1e8b7436f9),
                       "720-40_6.732",CRC(d7aaaa03) SHA1(4e0b901645e509bcb59bf81a6ffc1612b4fb16ee))
BY56_SOUNDROM(         "811-35_4.532",CRC(e9caccbb) SHA1(e2e09ac738c48342212bf38687299876b40cecbb))
BY57_SOUNDROM(         "811-22_1.532",CRC(c49a968e) SHA1(86680e8cbb82e69c232313e5fdd7a0058b7eef13),
                       "811-23_2.532",CRC(41043996) SHA1(78fa3782ee9f32d14cf41a96a60f708087e97bb9),
                       "811-24_3.532",CRC(53d65542) SHA1(edb63b6d36524ae17ec40cfc02d5cf9985f0477f),
                       "811-25_4.532",CRC(2c678631) SHA1(a1f9a732fdb498a71caf61ec8cf3d105cf7e114e),
                       "811-26_5.532",CRC(b8e7febc) SHA1(e557b1bbbc68a6884edebe779df4529116031e00),
                       "811-27_6.532",CRC(1e2a2afa) SHA1(3f4d4a562e46c162b80660eec8d9af6efe165dd6),
                       "811-28_7.532",CRC(cebb4cd8) SHA1(2678ffb5e8e2fcff07f029f14a9e0bf1fb95f7bc))
BY35_ROMEND
#define input_ports_xenon input_ports_by35
CORE_GAMEDEFNV(xenon,"Xenon",1980,"Bally",by35_mBY35_56S,0)
/*--------------------------------
/ Xenon (French)
/-------------------------------*/
#define input_ports_xenonf input_ports_xenon
#define init_xenonf        init_xenon
BY35_ROMSTART880(xenonf,"811-40_1.716",CRC(0fba871b) SHA1(52bc0ef65507f0f7422c319d0dc2059e12deab6d),
                        "811-41_2.716",CRC(1ea0d891) SHA1(98cd8cfed5c0f437d2b9423b31205f1e8b7436f9),
                        "720-40_6.732",CRC(d7aaaa03) SHA1(4e0b901645e509bcb59bf81a6ffc1612b4fb16ee))
BY56_SOUNDROM(          "811-36_4.532",CRC(73156c6e) SHA1(b0b3ecb44428c01849189adf6c86be3e95a99012))
BY57_SOUNDROM(          "811-22_1.532",CRC(c49a968e) SHA1(86680e8cbb82e69c232313e5fdd7a0058b7eef13),
                        "811-23_2.532",CRC(41043996) SHA1(78fa3782ee9f32d14cf41a96a60f708087e97bb9),
                        "811-24_3.532",CRC(53d65542) SHA1(edb63b6d36524ae17ec40cfc02d5cf9985f0477f),
                        "811-29_4.532",CRC(e586ec31) SHA1(080d43e9a5895e95533ae73cffa4948f747ce510),
                        "811-30_5.532",CRC(e48d98e3) SHA1(bb32ab96501dcd21525540a61bd5e478a35b1fef),
                        "811-31_6.532",CRC(0a2336e5) SHA1(28eeb00b03b8d9eb0e6966be00dfbf3a1e13e04c),
                        "811-32_7.532",CRC(987e6118) SHA1(4cded4ff715494f762d043dbcb0298111f327311))
BY35_ROMEND
CORE_CLONEDEFNV(xenonf,xenon,"Xenon (French)",1980,"Bally",by35_mBY35_56S,0)
/*--------------------------------
/ Flash Gordon (Vocalizer sound)
/-------------------------------*/
INITGAME2(flashgdv,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY56,0)
BY35_ROMSTARTx00(flashgdv,"834-23_2.732",CRC(0c7a0d91) SHA1(1f79be15817975acbc35cb08591e2289e2eca938),
                          "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY56_SOUNDROM(            "834-02_4.532",CRC(f1eb0a12) SHA1(a58567665547aacf9a1b2c39295d963527ef8696))
BY57_SOUNDROM(            "834-03_1.532",CRC(88bef6f4) SHA1(561e0bde04661b700552e4fbb6141c39f2789c99),
                          "834-04_2.532",CRC(bce91475) SHA1(482b424977d73b36e2014617e3bd3deb51091c28),
                          "834-05_3.532",CRC(1a4dbd99) SHA1(fa9ae0bde118a40ba9a0e9a085b30298cac0ea93),
                          "834-06_4.532",CRC(983c9e9d) SHA1(aae323a39b0ec987e6b9b98e5d9b2c58b1eea1a4),
                          "834-07_5.532",CRC(697f5333) SHA1(39bbff8790e394a20ef5ba3239fb1d9359be0fe5),
                          "834-08_6.532",CRC(75dd195f) SHA1(fdb6f7a15cd42e1326bf6baf8fa69f6266653cef),
                          "834-09_7.532",CRC(19ceabd1) SHA1(37e7780f2ba3e06462e775547278dcba1b6d2ac8))
BY35_ROMEND
#define input_ports_flashgdv input_ports_by35
CORE_CLONEDEFNV(flashgdv,flashgdn,"Flash Gordon (Vocalizer sound)",1981,"Bally",by35_mBY35_56S,0)

/* -----------------------------------------------------------*/
/* All games below use Squalk N Talk -61 (except where noted) */
/* -----------------------------------------------------------*/

/*--------------------------------
/ Flash Gordon
/-------------------------------*/
INITGAME2(flashgdn,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(flashgdn,"834-23_2.732",CRC(0c7a0d91) SHA1(1f79be15817975acbc35cb08591e2289e2eca938),
                          "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY61_SOUNDROM0xx0(        "834-20_2.532",CRC(2f8ced3e) SHA1(ecdeb07c31c22ec313b55774f4358a9923c5e9e7),
                          "834-18_5.532",CRC(8799e80e) SHA1(f255b4e7964967c82cfc2de20ebe4b8d501e3cb0))
BY35_ROMEND
#define input_ports_flashgdn input_ports_by35
CORE_GAMEDEFNV(flashgdn,"Flash Gordon",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Flash Gordon (french)
/-------------------------------*/
#define input_ports_flashgdf input_ports_flashgdn
#define init_flashgdf        init_flashgdn
BY35_ROMSTARTx00(flashgdf,"834-23_2.732",CRC(0c7a0d91) SHA1(1f79be15817975acbc35cb08591e2289e2eca938),
                          "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY61_SOUNDROM0xx0(        "834-35_2.532",CRC(dff3f711) SHA1(254a5670775ecb6c347f33af8ba7c350e4cfa550),
                          "834-36_5.532",CRC(18691897) SHA1(3b445e0756c07d80f14c01af5a7f87744474ae15))
BY35_ROMEND
CORE_CLONEDEFNV(flashgdf,flashgdn,"Flash Gordon (French)",1981,"Bally",by35_mBY35_61S,0)

/*--------------------------------
/ Eight Ball Deluxe
/-------------------------------*/
INITGAME2(eballdlx,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(eballdlx,"838-15_2.732",CRC(68d92acc) SHA1(f37b16d2953677cd779073bc3eac4b586d62fad8),
                          "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY61_SOUNDROMx080(        "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                          "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                          "838-10_5.532",CRC(9c63925d) SHA1(abd1fa6308d3569e16ee10bfabce269a124d8f26))
BY35_ROMEND
#define input_ports_eballdlx input_ports_by35
CORE_GAMEDEFNV(eballdlx,"Eight Ball Deluxe",1981,"Bally",by35_mBY35_61S,0)

#define init_eballdlb init_eballdlx
BY35_ROMSTARTx00(eballdlb,"8bd029.u2",CRC(ebc65a0d) SHA1(cb0bdb8750362e4e822dd6b41ad45ee8c9fb1452),
                          "8bd029.u6",CRC(6977de87) SHA1(5986337b7a75f8212f16d3d704ab8c1a8a828d0c))
BY61_SOUNDROMx080(        "838-08_3.532",CRC(c39478d7) SHA1(8148aca7c4113921ab882da32d6d88e66abb22cc),
                          "838-09_4.716",CRC(518ea89e) SHA1(a387274ef530bb57f31819733b35615a39260126),
                          "838-10_5.532",CRC(9c63925d) SHA1(abd1fa6308d3569e16ee10bfabce269a124d8f26))
BY35_ROMEND
#define input_ports_eballdlb input_ports_by35
CORE_CLONEDEFNV(eballdlb,eballdlx,"Eight Ball Deluxe (modified rules rev.29)",2007,"Bally / Oliver",by35_mBY35_61S,0)

/*--------------------------------
/ Fireball II
/-------------------------------*/
INITGAME2(fball_ii,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0)
BY35_ROMSTARTx00(fball_ii,"839-12_2.732",CRC(45e768ad) SHA1(b706cb5f3dcfa2db54d8d15de180fcbf36b3768f),
                          "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY61_SOUNDROM0xx0(        "839-01_2.532",CRC(4aa473bd) SHA1(eaa12ded76f9999d33ce0fe6198df1708e007e12),
                          "839-02_5.532",CRC(8bf904ff) SHA1(de78d08bddd546abac65c2f95f1d52797e716362))
BY35_ROMEND
#define input_ports_fball_ii input_ports_by35
CORE_GAMEDEFNV(fball_ii,"Fireball II",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Embryon
/-------------------------------*/
INITGAME2(embryon ,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(embryon,"841-06_2.732",CRC(80ab18e7) SHA1(52e5b1709e6f21919fc9efed67f51934d883dbb7),
                         "720-52_6.732",CRC(2a43d9fb) SHA1(9ff903c32b80780383578a9abaa3ef9d3bcecbc7))
BY61_SOUNDROMxx80(       "841-01_4.716",CRC(e8b234e3) SHA1(584e553748b1c6571491150e346d815005948b68),
                         "841-02_5.532",CRC(9cd8c04e) SHA1(7d74d8f33a98c9832fda1054187eb7300dbf5f5e))
BY35_ROMEND
#define input_ports_embryon input_ports_by35
CORE_GAMEDEFNV(embryon ,"Embryon",1981,"Bally",by35_mBY35_61BS,0)

INITGAME2(embryonb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(embryonb,"embd71u2.bin",CRC(8a25d7e9) SHA1(3f60007fae4f911b88d0642f50604c74dc45bed4),
                          "embd71u6.bin",CRC(20a3b3ce) SHA1(2304620d56a8ce5769e541aa29c2c6a3e81661c2))
BY61_SOUNDROMxx80(        "841-01_4.716",CRC(e8b234e3) SHA1(584e553748b1c6571491150e346d815005948b68),
                          "841-02_5.532",CRC(9cd8c04e) SHA1(7d74d8f33a98c9832fda1054187eb7300dbf5f5e))
BY35_ROMEND
#define input_ports_embryonb input_ports_embryon
CORE_CLONEDEFNV(embryonb,embryon ,"Embryon (7-digit conversion rev.1)",2002,"Bally / Oliver",by35_mBY35_61BS,0)

BY35_ROMSTARTx00(embryonc,"embd78u2.bin",CRC(7b399975) SHA1(8e8c30884af61ca2003d9d38871d5c4d48bcf177),
                          "embd78u6.bin",CRC(aa623412) SHA1(a6d386bd867ef01179d9e8695ce5e630727551df))
BY61_SOUNDROMxx80(        "841-01_4.716",CRC(e8b234e3) SHA1(584e553748b1c6571491150e346d815005948b68),
                          "841-02_5.532",CRC(9cd8c04e) SHA1(7d74d8f33a98c9832fda1054187eb7300dbf5f5e))
BY35_ROMEND
#define input_ports_embryonc input_ports_embryon
#define init_embryonc init_embryonb
CORE_CLONEDEFNV(embryonc,embryon ,"Embryon (7-digit conversion rev.8)",2004,"Bally / Oliver",by35_mBY35_61BS,0)

BY35_ROMSTARTx00(embryond,"embd78u2.bin",CRC(7b399975) SHA1(8e8c30884af61ca2003d9d38871d5c4d48bcf177),
                          "embd79u6.bin",CRC(5742794c) SHA1(f56b85a2370876f38144c8aceb8d513801fd40e1))
BY61_SOUNDROMxx80(        "841-01_4.716",CRC(e8b234e3) SHA1(584e553748b1c6571491150e346d815005948b68),
                          "841-02_5.532",CRC(9cd8c04e) SHA1(7d74d8f33a98c9832fda1054187eb7300dbf5f5e))
BY35_ROMEND
#define input_ports_embryond input_ports_embryon
#define init_embryond init_embryonb
CORE_CLONEDEFNV(embryond,embryon ,"Embryon (7-digit conversion rev.9)",2004,"Bally / Oliver",by35_mBY35_61BS,0)
/*--------------------------------
/ Fathom
/-------------------------------*/
INITGAME2(fathom  ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(fathom,"842-08_2.732",CRC(1180f284) SHA1(78be1fa54faba5c5b14f580e41546be685846391),
                        "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMxx00(      "842-01_4.532",CRC(2ac02093) SHA1(a89c1d24f4f3e1f58ca4e476f408835efb368a90),
                        "842-02_5.532",CRC(736800bc) SHA1(2679d4d76e7258ad18ffe05cf333f21c35adfe0e))
BY35_ROMEND
#define input_ports_fathom input_ports_by35
CORE_GAMEDEFNV(fathom  ,"Fathom",1981,"Bally",by35_mBY35_61BS,0)

#define init_fathomb init_fathom
BY35_ROMSTARTx00(fathomb,"fathomu2.732",CRC(e964af0d) SHA1(f7282767db1eba9b5cf12e1c064376c0fdda5505),
                         "fathomu6.732",CRC(0c169a11) SHA1(cbcfe3ce84a1b049706f54b6d606ea54e9a0a06c))
BY61_SOUNDROMxx00(       "842-01_4.532",CRC(2ac02093) SHA1(a89c1d24f4f3e1f58ca4e476f408835efb368a90),
                         "842-02_5.532",CRC(736800bc) SHA1(2679d4d76e7258ad18ffe05cf333f21c35adfe0e))
BY35_ROMEND
#define input_ports_fathomb input_ports_fathom
CORE_CLONEDEFNV(fathomb,fathom,"Fathom (modified rules)",2004,"Bally / Oliver",by35_mBY35_61BS,0)
/*--------------------------------
/ Medusa
/-------------------------------*/
static core_tLCDLayout medusaDisp[] = {
  DISP_SEG_IMPORT(dispBy7),
  {4,16,42,2,CORE_SEG7}, {4,21,44,2,CORE_SEG7}, {4,26,46,2,CORE_SEG7}, {0}
};
INITGAME2(medusa  ,GEN_BY35,medusaDisp,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0x10)
BY35_ROMSTARTx00(medusa,"845-16_2.732",CRC(b0fbd1ac) SHA1(e876eced0c02a2b4b3c308494e8c453074d0e561),
                        "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(      "845-01_3.532",CRC(32200e02) SHA1(e75356a20f81a68e6b27d2fa04b8cc9b17f3976a),
                        "845-02_4.532",CRC(ab95885a) SHA1(fa91cef2a244d25d408585d1e14e1ed8fdc8c845),
                        "845-05_5.716",CRC(3792a812) SHA1(5c7cc43e57d8e8ded1cc109aa65c4f08052899b9))
BY35_ROMEND
#define input_ports_medusa input_ports_by35
CORE_GAMEDEFNV(medusa  ,"Medusa",1981,"Bally",by35_mBY35_61BS,0)

#ifdef MAME_DEBUG
#define init_medusaf init_medusa
BY35_ROMSTARTx00(medusaf,"845-16_2.732",CRC(b0fbd1ac) SHA1(e876eced0c02a2b4b3c308494e8c453074d0e561),
                         "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(       "845-01_3.532",CRC(32200e02) SHA1(e75356a20f81a68e6b27d2fa04b8cc9b17f3976a),
                         "845-02_4.532",CRC(ab95885a) SHA1(fa91cef2a244d25d408585d1e14e1ed8fdc8c845),
                         "845-05_5.716",CRC(3792a812) SHA1(5c7cc43e57d8e8ded1cc109aa65c4f08052899b9))
BY35_ROMEND
#define input_ports_medusaf input_ports_medusa
CORE_CLONEDEFNV(medusaf,medusa,"Medusa (6802 board)",2004,"Bally / Oliver",by35_m6802_61S,0)
#endif /* MAME_DEBUG */
/*--------------------------------
/ Centaur
/-------------------------------*/
INITGAME2(centaur ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(centaur,"848-08_2.732",CRC(8bdcd32b) SHA1(39f64393d3a39a8172b3d80d196253aac1342f40),
                         "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(       "848-01_3.532",CRC(88322c8a) SHA1(424fd2b107f5fbc3ab8b58e3fa8c285170b1f09a),
                         "848-02_4.532",CRC(d6dbd0e4) SHA1(62e4c8c1a747c5f6a3a4bf4d0bc80b06a1f70d13),
                         "848-05_5.716",CRC(cbd765ba) SHA1(bdfae28af46c805f253f02d449dd81575aa9305b))
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
BY35_ROMSTARTx00(elektra,"857-04_2.732",CRC(d2476720) SHA1(372c210c4f19302ffe25722bba6bcaaa85c4b90d),
                         "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(       "857-01_3.532",CRC(031548cc) SHA1(1f0204afd32dc07a301f404b4b064e34a83bd783),
                         "857-02_4.532",CRC(efc870d9) SHA1(45132c123b3191d616e2e9372948ab66ff221228),
                         "857-03_5.716",CRC(eae2c6a6) SHA1(ee3a9b01fa07e2df4eb6d2ab26da5f7f0e12475b))
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
INITGAME2B(vector  ,GEN_BY35,vectorDisp,FLIP_SW(FLIP_L),8,SNDBRD_BY61,0x80)
BY35_ROMSTARTx00(vector,"858-11_2.732",CRC(323e286b) SHA1(998387900363fd46d392a931c1f092c886a23c69),
                        "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROM0000(      "858-01_2.532",CRC(bd2edef9) SHA1(8f129016440bad5e78d4b073268e76e542b61684),
                        "858-02_3.532",CRC(c592fb35) SHA1(5201824f129812c907e7d8a4600de23d95fd1eb0),
                        "858-03_4.532",CRC(8661d312) SHA1(36d04d875382ff5387991d660d031c662b414698),
                        "858-06_5.532",CRC(3050edf6) SHA1(e028192d9a8c17123b07566c6d73302cec07b440))
BY35_ROMEND
#define input_ports_vector input_ports_by35
CORE_GAMEDEFNV(vector ,"Vector",1982,"Bally",by35_mBY35_61S,0)

#define init_vectorb init_vector
BY35_ROMSTARTx00(vectorb,"vectoru2.732",CRC(6b2cbd42) SHA1(f778d19f8ff7a6228ccd3b49af9d60bc9eeffe7b),
                         "vectoru6.732",CRC(fe504d05) SHA1(27c72358ea53fd051e64c0179019116356f543d5))
BY61_SOUNDROM0000(       "858-01_2.532",CRC(bd2edef9) SHA1(8f129016440bad5e78d4b073268e76e542b61684),
                         "858-02_3.532",CRC(c592fb35) SHA1(5201824f129812c907e7d8a4600de23d95fd1eb0),
                         "858-03_4.532",CRC(8661d312) SHA1(36d04d875382ff5387991d660d031c662b414698),
                         "858-06_5.532",CRC(3050edf6) SHA1(e028192d9a8c17123b07566c6d73302cec07b440))
BY35_ROMEND
#define input_ports_vectorb input_ports_vector
CORE_CLONEDEFNV(vectorb,vector,"Vector (modified rules)",2004,"Bally / Oliver",by35_mBY35_61S,0)
/*--------------------------------
/ Spectrum
/-------------------------------*/
INITGAME2(spectrum,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(spectrum,"868-00_2.732",NO_DUMP,
                          "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(        "868-01_3.532",CRC(c3a16c66) SHA1(8c0a8b50fac0e218515b471621e80000ae475296),
                          "868-02_4.532",CRC(6b441399) SHA1(aae9e805f76cd6bc264bf69dd2d57629ee58bfc2),
                          "868-03_5.716",CRC(4a5ac3b8) SHA1(288feba40efd65f4eec5c0b2fcf013904e3dc24e))
BY35_ROMEND
#define input_ports_spectrum input_ports_by35
CORE_GAMEDEFNV(spectrum,"Spectrum",1982,"Bally",by35_mBY35_61BS,0)
#define input_ports_spectru4 input_ports_spectrum
#define init_spectru4        init_spectrum
BY35_ROMSTARTx00(spectru4,"868-04_2.732",CRC(b377f5f1) SHA1(adc40204da90ef1a4470a478520b949c6ded07b5),
                          "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMx008(        "868-01_3.532",CRC(c3a16c66) SHA1(8c0a8b50fac0e218515b471621e80000ae475296),
                          "868-02_4.532",CRC(6b441399) SHA1(aae9e805f76cd6bc264bf69dd2d57629ee58bfc2),
                          "868-03_5.716",CRC(4a5ac3b8) SHA1(288feba40efd65f4eec5c0b2fcf013904e3dc24e))
BY35_ROMEND
CORE_CLONEDEFNV(spectru4,spectrum,"Spectrum (Ver. 4)",1982,"Bally",by35_mBY35_61BS,0)

/*--------------------------------------------------
/ Speakeasy 2 Player - Uses AS2518-51 Sound Board
/--------------------------------------------------*/
INITGAME2(speakesy,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY51,0)
BY35_ROMSTARTx00(speakesy,"877-03_2.732",CRC(34b28bbc) SHA1(c649a04664e694cfbd6b4d496bf76f5e802d492a),
                          "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY51_SOUNDROM8(           "877-01_4.716",CRC(6534e826) SHA1(580653636f8d33e758e6631c9ce495f42fe3747a))
BY35_ROMEND
#define input_ports_speakesy input_ports_by35
CORE_GAMEDEFNV(speakesy,"Speakeasy",1982,"Bally",by35_mBY35_51S,0)

/*-------------------------------------------------
/ Speakeasy 4 Player - Uses AS2518-51 Sound Board
/-------------------------------------------------*/
#define input_ports_speakes4 input_ports_speakesy
#define init_speakes4        init_speakesy
BY35_ROMSTARTx00(speakes4,"877-04_2.732",CRC(8926f2bb) SHA1(617c032ce949007d6bcb52268f17bec5a02f8651),
                          "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY51_SOUNDROM8(           "877-01_4.716",CRC(6534e826) SHA1(580653636f8d33e758e6631c9ce495f42fe3747a))
BY35_ROMEND
CORE_CLONEDEFNV(speakes4,speakesy,"Speakeasy 4 Player",1982,"Bally",by35_mBY35_51S,0)

/*--------------------------------
/ Rapid Fire
/-------------------------------*/
static core_tLCDLayout dispRapid[] = {
  {0, 0, 1,7,CORE_SEG87F}, {4, 0, 9,7,CORE_SEG87F},
  {2,16,33,7,CORE_SEG7}, {0}
};
INITGAME2(rapidfir,GEN_BY35,dispRapid,FLIP_SW(FLIP_L),8,SNDBRD_BY61B,0)
BY35_ROMSTARTx00(rapidfir,"869-04_2.732",CRC(26fdf048) SHA1(15787345e7162a530334bff98d877e525d4a1295),
                          "869-03_6.732",CRC(f6af5e8d) SHA1(3cf782d4a0ca38e3953a20d23d0eb01af87ba445))
BY61_SOUNDROMxxx0(        "869-02_5.532",CRC(5a74cb86) SHA1(4fd09b0bc4257cb7b48cd8087b8b15fe768f7ddf))
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
BY35_ROMSTARTx00(m_mpac,"872-04_2.732",CRC(5e542882) SHA1(bec5f56cd5192e0a12ea1226a49a2b7d8eaaa5cf),
                        "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY61_SOUNDROMxx00(      "872-01_4.532",CRC(d21ce16d) SHA1(3ee6e2629530e7e6e4d7eac713d34c48297a1047),
                        "872-03_5.532",CRC(8fcdf853) SHA1(7c6bffcd974d2684e7f2c69d926f6cabb53e2f90))
BY35_ROMEND
#define input_ports_m_mpac input_ports_by35
CORE_GAMEDEFNV(m_mpac  ,"Mr. and Mrs. PacMan",1982,"Bally",by35_mBY35_61S,0)

/*---------------------------------------------------
/ BMX (BY35-888: 11/82) - Uses AS2518-51 Sound Board
/----------------------------------------------------*/
INITGAME2(bmx,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(bmx,"888-03_2.732",CRC(038cf1be) SHA1(b000a3d84623db6a7644551e5e2f0d7b533acb13),
                     "720-53_6.732",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY51_SOUNDROM0(      "888-02_4.532",CRC(5692c679) SHA1(7eef074d16cde589cde7500c4dc76c9a902c7fe3))
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
INITGAME3(granslam ,GEN_BY35,granslamDisp,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0x80)
BY35_ROMSTARTx00(granslam, "grndslam.u2",CRC(66aea9dc) SHA1(76c017dc83a63b7f1e6035e228370219eb9c0678),
                           "grndslam.u6",CRC(9e6ccea1) SHA1(5e158e021e0f3eed063577ae22cf5f1bc9655065))
BY51_SOUNDROM0(            "grndslam.u4",CRC(ac34bc38) SHA1(376ceb53cb51d250b5bc222001291b0c85e42e8a))
BY35_ROMEND
#define input_ports_granslam input_ports_by35
CORE_GAMEDEFNV(granslam,"Grand Slam",1983,"Bally",by35_mBY35_51S,0)

BY35_ROMSTARTx00(gransla4, "gr_slam.u2b",CRC(552d9423) SHA1(16b86d5b7539fd803f458f1633ecc249ef15243d),
                           "grndslam.u6",CRC(9e6ccea1) SHA1(5e158e021e0f3eed063577ae22cf5f1bc9655065))
BY51_SOUNDROM0(            "grndslam.u4",CRC(ac34bc38) SHA1(376ceb53cb51d250b5bc222001291b0c85e42e8a))
BY35_ROMEND
#define init_gransla4 init_granslam
#define input_ports_gransla4 input_ports_granslam
CORE_CLONEDEFNV(gransla4,granslam,"Grand Slam (4 Players)",1983,"Bally",by35_mBY35_51S,0)

/*----------------------------------------------------------
/ Gold Ball (BY35-???: 10/83)  - Uses AS2518-51 Sound Board
/----------------------------------------------------------*/
INITGAME3(goldball,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(goldball,"gold2732.u2",CRC(3169493c) SHA1(1335fcdfb2d6970d78c636748ff419baf85ef78b),
                          "goldball.u6",CRC(9b6e79d0) SHA1(4fcda91bbe930e6131d94964a08459e395f841af))
BY51_SOUNDROM0(           "gb_u4.532",  CRC(2dcb0315) SHA1(8cb9c9f627f0c8420d3b3d9f0d10d77a82c8be56))
BY35_ROMEND
#define input_ports_goldball input_ports_by35
CORE_GAMEDEFNV(goldball,"Gold Ball",1983,"Bally",by35_mBY35_51S,0)

INITGAME3(goldbalb,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(goldbalb,"gold2732.u2", CRC(3169493c) SHA1(1335fcdfb2d6970d78c636748ff419baf85ef78b),
                          "goldd7u6.bin",CRC(a34d3d8c) SHA1(ccd416d6453936982be52abefdd462f0cf6cf05e))
BY51_SOUNDROM0(           "gb_u4.532",   CRC(2dcb0315) SHA1(8cb9c9f627f0c8420d3b3d9f0d10d77a82c8be56))
BY35_ROMEND
#define input_ports_goldbalb input_ports_goldball
CORE_CLONEDEFNV(goldbalb,goldball,"Gold Ball (7-digit conversion)",2004,"Bally / Oliver",by35_mBY35_51S,0)


INITGAME3(goldbalc,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),0,SNDBRD_BY51,0)
BY35_ROMSTARTx00(goldbalc,"go102732.u2", CRC(1eda67dc) SHA1(8b3a15d2cc33a23e1292f7cd34fa998ae8c105ed),
                          "gold10u6.bin",CRC(780ff734) SHA1(506022a5c6b435f15c37e193d194871673a15b68))
BY51_SOUNDROM0(           "gb_u4.532",   CRC(2dcb0315) SHA1(8cb9c9f627f0c8420d3b3d9f0d10d77a82c8be56))
BY35_ROMEND
#define input_ports_goldbalc input_ports_goldball
CORE_CLONEDEFNV(goldbalc,goldball,"Gold Ball (6/7-digit alternate set rev.12)",2005,"Bally / Oliver",by35_mBY35_51S,0)

#define init_goldbaln init_goldball
BY35_ROMSTARTx00(goldbaln,"u2.532",CRC(aa6eb9d6) SHA1(a73cc832450e718d9b8484e409a1f8093d91d786),
                          "goldball.u6",CRC(9b6e79d0) SHA1(4fcda91bbe930e6131d94964a08459e395f841af))
BY51_SOUNDROM0(           "gb_u4.532",  CRC(2dcb0315) SHA1(8cb9c9f627f0c8420d3b3d9f0d10d77a82c8be56))
BY35_ROMEND
#define input_ports_goldbaln input_ports_goldball
CORE_CLONEDEFNV(goldbaln,goldball ,"Gold Ball (alternate set)",1983,"Bally",by35_mBY35_51S,0)

/********************************************************/
/******* Games Below use Cheap Squeak Sound Board *******/
/********************************************************/
/*--------------------------------
/ X's & O's (BY35-???: 12/83)
/-------------------------------*/
INITGAME2(xsandos ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(xsandos ,"x&os2732.u2",CRC(068dfe5a) SHA1(028baf79852b14cac51a7cdc8e751a8173beeccb),
                          "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROMx2(          "720_u3.snd", CRC(5d8e2adb) SHA1(901a26f5e598386295a1298ee3a634941bd58b3e))
BY35_ROMEND
#define input_ports_xsandos input_ports_by35
CORE_GAMEDEFNV(xsandos ,"X's & O's",1983,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Kings of Steel (BY35-???: 05/84)
/-------------------------------*/
INITGAME2(kosteel ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(kosteel ,"kngs2732.u2",CRC(f876d8f2) SHA1(581f4b98e0a69f4ae879caeafdbf2eb979514ad1),
                          "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROM11(          "kngsu3.snd", CRC(11b02dca) SHA1(464eee1aa1fd9b6e26d4ba635777fffad0222106),
                          "kngsu4.snd", CRC(f3e4d2f6) SHA1(93f4e9e1348b1225bc02db38c994e3338afb175c))
BY35_ROMEND
#define input_ports_kosteel input_ports_by35
CORE_GAMEDEFNV(kosteel ,"Kings of Steel",1984,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Black Pyramid (BY35-???: 07/84)
/-------------------------------*/
INITGAME2(blakpyra,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(blakpyra,"blkp2732.u2",CRC(600535b0) SHA1(33d080f4430ad9c33ee9de1bfbb5cfde50f0776e),
                          "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROM11(          "bp_u3.532",  CRC(a5005067) SHA1(bd460a20a6e8f33746880d72241d6776b85126cf),
                          "bp_u4.532",  CRC(57978b4a) SHA1(4995837790d81b02325d39b548fb882a591769c5))
BY35_ROMEND
#define input_ports_blakpyra input_ports_by35
CORE_GAMEDEFNV(blakpyra,"Black Pyramid",1984,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Spy Hunter (BY35-???: 10/84)
/-------------------------------*/
INITGAME2(spyhuntr,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(spyhuntr, "spy-2732.u2",CRC(9e930f2d) SHA1(fb48ce0d8d8f8a695827c0eea57510b53daa7c39),
                           "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROM11(           "spy_u3.532", CRC(95ffc1b8) SHA1(28f058f74abbbee120dca06f7321bcb588bef3c6),
                           "spy_u4.532", CRC(a43887d0) SHA1(6bbc55943fa9f0cd97f946767f21652e19d85265))
BY35_ROMEND
#define input_ports_spyhuntr input_ports_by35
CORE_GAMEDEFNV(spyhuntr,"Spy Hunter",1984,"Bally",by35_mBY35_45S,0)

/*-------------------------------------
/ Fireball Classic (BY35-???: 02/85)
/------------------------------------*/
INITGAME2(fbclass ,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(fbclass ,"fb-class.u2",CRC(32faac6c) SHA1(589020d09f26326dab266bc7c74ca0e10de565e6),
                          "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROM11(          "fbcu3.snd",  CRC(1ad71775) SHA1(ddb885730deaf315fe7f3c1803628c06eedc8350),
                          "fbcu4.snd",  CRC(697ab16f) SHA1(7beed02e6cb042f90d2048778408b1f744ffe242))
BY35_ROMEND
#define input_ports_fbclass input_ports_by35
CORE_GAMEDEFNV(fbclass ,"Fireball Classic",1984,"Bally",by35_mBY35_45S,0)

/*--------------------------------
/ Cybernaut (BY35-???: 05/85)
/-------------------------------*/
INITGAME2(cybrnaut,GEN_BY35,dispBy7,FLIP_SW(FLIP_L),8,SNDBRD_BY45,0)
BY35_ROMSTARTx00(cybrnaut,"cybe2732.u2",CRC(0610b0e0) SHA1(92f5e8a83240ad03ecc16ece4824b047b77816f7),
                          "720-5332.u6",CRC(c2e92f80) SHA1(61de956a4b6e9fb9ef2b25c01bff1fb5972284ad))
BY45_SOUNDROMx2(          "cybu3.snd",  CRC(a3c1f6e7) SHA1(35a5e828a6f2dd9009e165328a005fa079bad6cb))
BY35_ROMEND
#define input_ports_cybrnaut input_ports_by35
CORE_GAMEDEFNV(cybrnaut,"Cybernaut",1985,"Bally",by35_mBY35_45S,0)

/******************************************************/
/******* Games below by different manufacturers *******/
/******************************************************/

/*--------------------------------
/ Mystic Star (Zaccaria game, 1984)
/-------------------------------*/
INITGAME(myststar,GEN_BY35,dispBy6,FLIP_SW(FLIP_L),0,SNDBRD_BY50,0)
BY35_ROMSTART888(myststar,"rom1.bin",CRC(9a12dc91) SHA1(8961c22b2aeabac04d36d124f283409e11faee8a),
                          "rom2.bin",CRC(888ee5ae) SHA1(d99746c7c9a9a0a83b4bc15473fe9ebd3b02ffe4),
                          "rom3.bin",CRC(9e0a4619) SHA1(82065b74d39ba932704514e83d432262d360f1e1))
BY50_SOUNDROM(            "snd.123", NO_DUMP)
BY35_ROMEND
#define input_ports_myststar input_ports_by35
CORE_GAMEDEFNV(myststar,"Mystic Star",1984,"Zaccaria",by35_mBY35_50S,0)

/*------------------------------------
/ 301/Bullseye (Grand Products, 1986)
/------------------------------------*/
static MEMORY_READ_START(gpsnd_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x6000, 0x6003, pia_r(2) },
  { 0x8000, 0x8fff, MRA_ROM },
  { 0xfff0, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(gpsnd_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x6000, 0x6003, pia_w(2) },
MEMORY_END

static struct DACinterface grand_dacInt = { 1, { 50 }};

MACHINE_DRIVER_START(by35_GP)
  MDRV_IMPORT_FROM(by35)
  MDRV_DIPS(33)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579545/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(gpsnd_readmem, gpsnd_writemem)
  MDRV_SOUND_ADD(DAC, grand_dacInt)
MACHINE_DRIVER_END

static struct {
  struct sndbrdData brdData;
  UINT8 sndCmd;
} gplocals;

static READ_HANDLER(pia_b_r) { cpu_set_irq_line(gplocals.brdData.cpuNo, 0, CLEAR_LINE); return gplocals.sndCmd; }

static const struct pia6821_interface grand_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0,pia_b_r, 0,0, 0,0,
  /*o: A/B,CA/B2       */ DAC_0_data_w,0, 0,0,
  /*irq: A/B           */ 0, 0
};

static void grand_init(struct sndbrdData *brdData) {
  memset(&gplocals, 0x00, sizeof(gplocals));
  gplocals.brdData = *brdData;
  pia_config(2, PIA_STANDARD_ORDERING, &grand_pia);
}
static void grand_diag(int button) {
  cpu_set_nmi_line(gplocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(grand_man_w) {
  gplocals.sndCmd = data;
  cpu_set_irq_line(gplocals.brdData.cpuNo, 0, ASSERT_LINE);
}
static WRITE_HANDLER(grand_data_w) {
  grand_man_w(0, data | (core_getDip(4) ? 0x80 : 0x00));
}

const struct sndbrdIntf grandIntf = {
  "GRAND", grand_init, NULL, grand_diag, grand_man_w, grand_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

INITGAME(bullseye,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),8,SNDBRD_GRAND,0)
ROM_START(bullseye) \
  NORMALREGION(0x10000, BY35_CPUREGION) \
    ROM_LOAD("bull.u2", 0x2000, 0x0800, CRC(a2951aa2) SHA1(f9c0826c5d1d6d904286678ed90de3850a13b5f4)) \
    ROM_CONTINUE( 0x2800, 0x0800) \
    ROM_LOAD("bull.u6", 0x3000, 0x0800, CRC(64d4b9c4) SHA1(bf4d0671372fd3a445c4c7330b9849171ca8048c)) \
    ROM_CONTINUE( 0x3800, 0x0800) \
    ROM_RELOAD( 0xf000, 0x1000)
  SOUNDREGION(0x10000, BY51_CPUREGION) \
    ROM_LOAD("bull.snd", 0x8000, 0x0800, CRC(c0482a2f) SHA1(a6aa698ad517cdc078129d702ee936af576260ed)) \
      ROM_RELOAD(0x8800, 0x0800) \
      ROM_RELOAD(0xf800, 0x0800)
BY35_ROMEND
BY35_INPUT_PORTS_START(bullseye,1) \
  PORT_START /* 3 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "Sound mode") \
      COREPORT_DIPSET(0x0000, "chimes" ) \
      COREPORT_DIPSET(0x0001, "effects" ) \
INPUT_PORTS_END
CORE_GAMEDEFNV(bullseye,"301/Bullseye",1986,"Grand Products Inc.",by35_GP,0)
