#include "driver.h"
#include "sim.h"
#include "by35.h"
#include "by35snd.h"
#include "byvidpin.h"
#include "by6803.h"
#include "by6803snd.h"

//#define DISPLAYALL
#ifdef DISPLAYALL
static core_tLCDLayout dispBy6[] = {
  { 0, 0, 0, 16, CORE_SEG7 }, { 2, 0,16, 16, CORE_SEG7 },
  { 4, 0,32, 16, CORE_SEG7 }, {0}
};
#define dispBy7 dispBy6
#else /* DISPLAYALL */
static core_tLCDLayout dispBy6[] = {
  {0, 0, 2,6,CORE_SEG7}, {0,14,10,6,CORE_SEG7},
  {2, 0,18,6,CORE_SEG7}, {2,14,26,6,CORE_SEG7},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};
static core_tLCDLayout dispBy7[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};
#endif /* DISPLAYALL */

static core_tLCDLayout by_NoOutput[] = {{0}};

#define INITGAME(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
BY35_INPUT_PORTS_START(name, 1) BY35_INPUT_PORTS_END

#define INITGAMEVP(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
BYVP_INPUT_PORTS_START(name, 1) BYVP_INPUT_PORTS_END

#define INITGAME6803(name, gen, disp, flip, lamps) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
BY6803_INPUT_PORTS_START(name, 1) BY6803_INPUT_PORTS_END

/* -------------------------------------------------------------*/
/* All games below use CHIMES for sound - ie, no sound hardware */
/* -------------------------------------------------------------*/

/*--------------------------------
/ Freedom
/-------------------------------*/
INITGAME(freedom,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTART228(freedom,"720-08_1.474",0xb78bceeb,
			 "720-10_2.474",0xca90c8a7,
			 "720-07_6.716",0x0f4e8b83)
BY17_ROMEND
CORE_GAMEDEFNV(freedom,"Freedom",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Night Rider
/-------------------------------*/
INITGAME(nightrdr,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTART8x8(nightrdr,"721-21_1.716",0x237c4060,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(nightrdr,"Night Rider",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ EVEL KNIEVEL
/-------------------------------*/
INITGAME(evelknie,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(evelknie,"722-17_2.716",0xb6d9a3fa,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(evelknie,"Evel Knievel",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Eight Ball
/-------------------------------*/
INITGAME(eightbll,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(eightbll,"723-20_2.716",0x33559e7b,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(eightbll,"Eight Ball",1977,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Power Play
/-------------------------------*/
INITGAME(pwerplay,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(pwerplay,"724-25_2.716",0x43012f35,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(pwerplay,"Power Play",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Mata Hari
/-------------------------------*/
INITGAME(matahari,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(matahari,"725-21_2.716",0x63acd9b0,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(matahari,"Mata Hari",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Strikes and Spares
/-------------------------------*/
INITGAME(stk_sprs,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(stk_sprs,"740-16_2.716",0x2be27024,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(stk_sprs,"Strikes and Spares",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)
/*--------------------------------
/ Black Jack
/-------------------------------*/
INITGAME(blackjck,GEN_BY17,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(blackjck,"728-32_2.716",0x1333c9d1,
			  "720-20_6.716",0x0c17aa4d)
BY17_ROMEND
CORE_GAMEDEFNV(blackjck,"Black Jack",1978,"Bally",by35_mBY17,GAME_USES_CHIMES)

/* -------------------------------------*/
/* All games below use Sound Module -32 */
/* -------------------------------------*/

/*--------------------------------
/ Lost World
/-------------------------------*/
INITGAME(lostwrld,GEN_BY35_32,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(lostwrld,"729-33_1.716",0x4ca40b95,
			  "729-48_2.716",0x963bffd8,
			  "720-28_6.716",0xf24cce3e)
BY35_SOUND32ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(lostwrld,"Lost World",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ 6million$man
/-------------------------------*/
INITGAME(smman,GEN_BY35_32,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(smman,"742-20_1.716", 0x33e55a75,
		       "742-18_2.716", 0x5365d36c,
		       "720-30_6.716", 0x4be8aab0)
BY35_SOUND32ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(smman,"Six Million Dollar Man",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Playboy
/-------------------------------*/
INITGAME(playboy,GEN_BY35_32,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(playboy,"743-14_1.716",0x5c40984a,
			 "743-12_2.716",0x6fa66664,
			 "720-30_6.716",0x4be8aab0)
BY35_SOUND32ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(playboy,"Playboy",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Voltan Escapes Cosmic Doom
/-------------------------------*/
INITGAME(voltan,GEN_BY35_32,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(voltan,"744-03_1.716",  0xad2467ae,
		                "744-04_2.716",0xdbf58b83,
		                "720-30_6.716",0x4be8aab0)
BY35_SOUND32ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(voltan,"Voltan Escapes Cosmic Doom",1978,"Bally",by35_mBY35_32S,0)
/*--------------------------------
/ Supersonic
/-------------------------------*/
INITGAME(sst,GEN_BY35_32,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(sst,"741-10_1.716",0x5e4cd81a,
		     "741-08_2.716",0x2789cbe6,
		     "720-30_6.716",0x4be8aab0)
BY35_SOUND32ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(sst,"Supersonic",1979,"Bally",by35_mBY35_32S,0)

/* -------------------------------------*/
/* All games below use Sound Module -50 */
/* -------------------------------------*/

/*--------------------------------
/ Star Trek
/-------------------------------*/
INITGAME(startrek,GEN_BY35_50,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(startrek,"745-11_1.716",0xa077efca,
			  "745-12_2.716",0xf683210a,
 			  "720-30_6.716",0x4be8aab0)
BY35_SOUND50ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(startrek, "Star Trek",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Paragon
/-------------------------------*/
INITGAME(paragon,GEN_BY35_50,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(paragon,"748-17_1.716",0x08dbdf32,
			 "748-15_2.716",0x26cc05c1,
			 "720-30_6.716",0x4be8aab0)
BY35_SOUND50ROM("729-51_3.123",0x6e7d3e8b)
BY35_ROMEND
CORE_GAMEDEFNV(paragon,"Paragon",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Harlem Globetrotters
/-------------------------------*/
INITGAME(hglbtrtr,GEN_BY35_50,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(hglbtrtr,"750-07_1.716",0xda594719,
			  "750-08_2.716",0x3c783931,
			  "720-35_6.716",0x78d6d289)
BY35_SOUND50ROM("729-51_3.123",0x6e7d3e8b)
BY35_ROMEND
CORE_GAMEDEFNV(hglbtrtr,"Harlem Globetrotters",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Dolly Parton
/-------------------------------*/
INITGAME(dollyptn,GEN_BY35_50,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(dollyptn,"777-10_1.716",0xca88cb9a,
		 	  "777-13_2.716",0x7fc93ea3,
		 	  "720-35_6.716",0x78d6d289)
BY35_SOUND50ROM("729-51_3.123",0x6e7d3e8b)
BY35_ROMEND
CORE_GAMEDEFNV(dollyptn,"Dolly Parton",1979,"Bally",by35_mBY35_50S,0)
/*--------------------------------
/ Kiss
/-------------------------------*/
INITGAME(kiss,GEN_BY35_50,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(kiss,"746-11_1.716",0x78ec7fad,
		      "746-14_2.716",0x0fc8922d,
		      "720-30_6.716",0x4be8aab0)
BY35_SOUND50ROM("729-18_3.123",0x7b6b7d45)
BY35_ROMEND
CORE_GAMEDEFNV(kiss,"Kiss",1979,"Bally",by35_mBY35_50S,0)

/* -------------------------------------*/
/* All games below use Sound Module -51 */
/* -------------------------------------*/

/*--------------------------------
/ Future Spa
/-------------------------------*/
INITGAME(futurspa,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(futurspa,"781-07_1.716",0x4c716a6a,
		          "781-09_2.716",0x316617ed,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("781-02_4.716",0x364f7c9a)
BY35_ROMEND
CORE_GAMEDEFNV(futurspa,"Future Spa",1979,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Space invaders
/-------------------------------*/
INITGAME(spaceinv,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(spaceinv,"792-10_1.716",0x075eba5a,
		          "792-13_2.716",0xb87b9e6b,
		          "720-37_6.716",0xceff6993)
BY35_SOUND51ROM("792-07_4.716",0x787ffd5e)
BY35_ROMEND
CORE_GAMEDEFNV(spaceinv,"Space Invaders",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Nitro Groundshaker
/-------------------------------*/
INITGAME(ngndshkr,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(ngndshkr,"776-17_1.716",0xf2d44235,
		          "776-11_2.716",0xb0396b55,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("776-15_4.716",0x63c80c52)
BY35_ROMEND
CORE_GAMEDEFNV(ngndshkr,"Nitro Groundshaker",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Silverball Mania
/-------------------------------*/
INITGAME(slbmania,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(slbmania,"786-16_1.716",0xc054733f,
		          "786-17_2.716",0x94af0298,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("786-11_4.716",0x2a3641e6)
BY35_ROMEND
CORE_GAMEDEFNV(slbmania,"Silverball Mania",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Rolling Stones
/-------------------------------*/
INITGAME(rollston,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(rollston,"796-17_1.716",0x51a826d7,
		          "796-18_2.716",0x08c75b1a,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("796-19_4.716",0xb740d047)
BY35_ROMEND
CORE_GAMEDEFNV(rollston,"Rolling Stones",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Mystic
/-------------------------------*/
INITGAME(mystic  ,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(mystic  ,"798-03_1.716",0xf9c91e3b,
		          "798-04_2.716",0xf54e5785,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("798-05_4.716",0xe759e093)
BY35_ROMEND
CORE_GAMEDEFNV(mystic  ,"Mystic",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Hot Doggin
/-------------------------------*/
INITGAME(hotdoggn,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(hotdoggn,"809-05_1.716",0x2744abcb,
		          "809-06_2.716",0x03db3d4d,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("809-07_4.716",0x43f28d7f)
BY35_ROMEND
CORE_GAMEDEFNV(hotdoggn,"Hot Doggin",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Viking
/-------------------------------*/
INITGAME(viking  ,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART888(viking  ,"802-05_1.716",0xa5db0574,
		          "802-06_2.716",0x40410760,
		          "720-35_6.716",0x78d6d289)
BY35_SOUND51ROM("802-07-4.716",0x62bc5030)
BY35_ROMEND
CORE_GAMEDEFNV(viking  ,"Viking",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Skateball
/-------------------------------*/
INITGAME(skatebll,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART880(skatebll,"823-24_1.716",0x46e797d1,
		          "823-25_2.716",0x960cb8c3,
		          "720-40_6.732",0xd7aaaa03)
BY35_SOUND51ROM("823-02_4.716",0xd1037b20)
BY35_ROMEND
CORE_GAMEDEFNV(skatebll,"Skateball",1980,"Bally",by35_mBY35_51S,0)
/*--------------------------------
/ Frontier
/-------------------------------*/
INITGAME(frontier,GEN_BY35_51,dispBy6,FLIP_SW(FLIP_L),0)
BY35_ROMSTART880(frontier,"819-08_1.716",0xe2f8ce9d,
		          "819-07_2.716",0xaf023a85,
		          "720-40_6.732",0xd7aaaa03)
BY35_SOUND51ROM("819-09_4.716",0xa62059ca)
BY35_ROMEND
CORE_GAMEDEFNV(frontier,"Frontier",1980,"Bally",by35_mBY35_51S,0)

/* -------------------------------------*/
/* All games below use Sound Module -56 */
/* -------------------------------------*/

/*--------------------------------
/ Xenon
/-------------------------------*/
INITGAME(xenon,GEN_BY35_56,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTART880(xenon,"811-40_1.716", 0x0fba871b,
		       "811-41_2.716", 0x1ea0d891,
		       "720-40_6.732", 0xd7aaaa03)
BY35_SOUND56ROM("811-35_4.532",0xe9caccbb)
BY35_SOUND57ROM("811-22_1.532",0xc49a968e,
                "811-23_2.532",0x41043996,
                "811-24_3.532",0x53d65542,
                "811-25_4.532",0x2c678631,
                "811-26_5.532",0xb8e7febc,
                "811-27_6.532",0x1e2a2afa,
                "811-28_7.532",0xcebb4cd8)
BY35_ROMEND
CORE_GAMEDEFNV(xenon,"Xenon",1980,"Bally",by35_mBY35_56S,0)
/*--------------------------------
/ Xenon (French)
/-------------------------------*/
#define input_ports_xenonf input_ports_xenon
#define init_xenonf        init_xenon
BY35_ROMSTART880(xenonf,"811-40_1.716", 0x0fba871b,
		        "811-41_2.716", 0x1ea0d891,
		        "720-40_6.732", 0xd7aaaa03)
BY35_SOUND56ROM("811-36_4.532",0x73156c6e)
BY35_SOUND57ROM("811-22_1.532",0xc49a968e,
                "811-23_2.532",0x41043996,
                "811-24_3.532",0x53d65542,
                "811-29_4.532",0xe586ec31,
                "811-30_5.532",0xe48d98e3,
                "811-31_6.532",0x0a2336e5,
                "811-32_7.532",0x987e6118)
BY35_ROMEND
CORE_CLONEDEFNV(xenonf,xenon,"Xenon (French)",1980,"Bally",by35_mBY35_56S,0)

/* -----------------------------------------------------------*/
/* All games below use Squalk N Talk -61 (except where noted) */
/* -----------------------------------------------------------*/

/*--------------------------------
/ Flash Gordon
/-------------------------------*/
INITGAME(flashgdn,GEN_BY35_61,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(flashgdn,"834-23_2.732", 0x0c7a0d91,
		          "720-52_6.732", 0x2a43d9fb)
BY35_SOUND61ROM0xx0("834-20_2.532",0x2f8ced3e,
                    "834-18_5.532",0x8799e80e)
BY35_ROMEND
CORE_GAMEDEFNV(flashgdn,"Flash Gordon",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Flash Gordon (french)
/-------------------------------*/
#define input_ports_flashgdf input_ports_flashgdn
#define init_flashgdf        init_flashgdn
BY35_ROMSTARTx00(flashgdf,"834-23_2.732", 0x0c7a0d91,
		          "720-52_6.732", 0x2a43d9fb)
BY35_SOUND61ROM0xx0("834-35_2.532",0xdff3f711,
                    "834-36_5.532",0x18691897)
BY35_ROMEND
CORE_CLONEDEFNV(flashgdf,flashgdn,"Flash Gordon (French)",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Eight Ball Deluxe
/-------------------------------*/
INITGAME(eballdlx,GEN_BY35_61,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(eballdlx,"838-15_2.732", 0x68d92acc,
			  "720-52_6.732", 0x2a43d9fb)
BY35_SOUND61ROMx080("838-08_3.532",0xc39478d7,
                    "838-09_4.716",0x518ea89e,
                    "838-10_5.532",0x9c63925d)
BY35_ROMEND
CORE_GAMEDEFNV(eballdlx,"Eight Ball Deluxe",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Fireball II
/-------------------------------*/
INITGAME(fball_ii,GEN_BY35_61,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(fball_ii,"839-12_2.732", 0x45e768ad,
			  "720-52_6.732", 0x2a43d9fb)
BY35_SOUND61ROM0xx0("839-01_2.532",0x4aa473bd,
                    "839-02_5.532",0x8bf904ff)
BY35_ROMEND
CORE_GAMEDEFNV(fball_ii,"Fireball II",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Embryon
/-------------------------------*/
INITGAME(embryon ,GEN_BY35_61B,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(embryon ,"841-06_2.732", 0x80ab18e7,
			  "720-52_6.732", 0x2a43d9fb)
BY35_SOUND61ROMxx80("841-01_4.716",0xe8b234e3,
                    "841-02_5.532",0x9cd8c04e)
BY35_ROMEND
CORE_GAMEDEFNV(embryon ,"Embryon",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Fathom
/-------------------------------*/
INITGAME(fathom  ,GEN_BY35_61B,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(fathom  ,"842-08_2.732", 0x1180f284,
			  "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMxx00("842-01_4.532",0x2ac02093,
                    "842-02_5.532",0x736800bc)
BY35_ROMEND
CORE_GAMEDEFNV(fathom  ,"Fathom",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Medusa
/-------------------------------*/
INITGAME(medusa  ,GEN_BY35_61B,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(medusa  ,	"845-16_2.732", 0xb0fbd1ac,
							"720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMx008("845-01_3.532",0x32200e02,
                    "845-02_4.532",0xab95885a,
                    "845-05_5.716",0x3792a812)
BY35_ROMEND
CORE_GAMEDEFNV(medusa  ,"Medusa",1981,"Bally",by35_mBY35_61BS,GAME_NOT_WORKING)
/*--------------------------------
/ Centaur
/-------------------------------*/
INITGAME(centaur ,GEN_BY35_61B,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(centaur ,"848-08_2.732", 0x8bdcd32b,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMx008("848-01_3.532",0x88322c8a,
                    "848-02_4.532",0xd6dbd0e4,
                    "848-05_5.716",0xcbd765ba)
BY35_ROMEND
CORE_GAMEDEFNV(centaur,"Centaur",1981,"Bally",by35_mBY35_61BS,0)
/*--------------------------------
/ Elektra
/-------------------------------*/
INITGAME(elektra ,GEN_BY35_61,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(elektra ,"857-04_2.732", 0xd2476720,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMx008("857-01_3.532",0x031548cc,
                    "857-02_4.532",0xefc870d9,
                    "857-03_5.716",0xeae2c6a6)
BY35_ROMEND
CORE_GAMEDEFNV(elektra,"Elektra",1981,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Vector
/-------------------------------*/
INITGAME(vector  ,GEN_BY35_61,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(vector  ,"858-11_2.732", 0x323e286b,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROM0000("858-01_2.532",0xbd2edef9,
                    "858-02_3.532",0xc592fb35,
                    "858-03_4.532",0x8661d312,
                    "858-06_5.532",0x3050edf6)
BY35_ROMEND
CORE_GAMEDEFNV(vector ,"Vector",1982,"Bally",by35_mBY35_61S,0)
/*--------------------------------
/ Spectrum
/-------------------------------*/
INITGAME(spectrum,GEN_BY35_61B,dispBy7,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(spectrum,"868-00_2.732", 0x13f15156,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMx008("868-01_3.532",0xc3a16c66,
                    "868-02_4.532",0x6b441399,
                    "868-03_5.716",0x4a5ac3b8)
BY35_ROMEND
CORE_GAMEDEFNV(spectrum,"Spectrum",1982,"Bally",by35_mBY35_61BS,0)
#define input_ports_spectru4 input_ports_spectrum
#define init_spectru4        init_spectrum
BY35_ROMSTARTx00(spectru4,"868-04_2.732", 0xb377f5f1,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMx008("868-01_3.532",0xc3a16c66,
                    "868-02_4.532",0x6b441399,
                    "868-03_5.716",0x4a5ac3b8)
BY35_ROMEND
CORE_CLONEDEFNV(spectru4,spectrum, "Spectrum 4 Player",1982,"Bally",by35_mBY35_61BS,0)

/*--------------------------------------------------
/ Speak Easy 2 Player - Uses AS2518-51 Sound Board
/--------------------------------------------------*/
INITGAME(speakesy,GEN_BY35_51,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(speakesy,"877-03_2.732", 0x34b28bbc,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND51ROM("877-01_4.716",0x6534e826)
BY35_ROMEND
CORE_GAMEDEFNV(speakesy,"Speakeasy",1982,"Bally",by35_mBY35_51S,0)

/*-------------------------------------------------
/ Speak Easy 4 Player - Uses AS2518-51 Sound Board
/-------------------------------------------------*/
#define input_ports_speakes4 input_ports_speakesy
#define init_speakes4        init_speakesy
BY35_ROMSTARTx00(speakes4,"877-04_2.732", 0x8926f2bb,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND51ROM("877-01_4.716",0x6534e826)
BY35_ROMEND
CORE_CLONEDEFNV(speakes4,speakesy,"Speakeasy 4 Player",1982,"Bally",by35_mBY35_51S,0)

/*--------------------------------
/ Rapid Fire
/-------------------------------*/
static core_tLCDLayout dispRapid[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 4,33,2,CORE_SEG7}, {2,10,36,2,CORE_SEG7},
  {2,16,41,7,CORE_SEG7}, {0}
};
INITGAME(rapidfir,GEN_BY35_61B,dispRapid,FLIP_SW(FLIP_L),8)
BY35_ROMSTARTx00(rapidfir,"869-04_2.732", 0x26fdf048,
                          "869-03_6.732", 0xf6af5e8d)
BY35_SOUND61ROMxxx0("869-02_5.532",0x5a74cb86)
BY35_ROMEND
CORE_GAMEDEFNV(rapidfir,"Rapid Fire",1982,"Bally",by35_mBY35_61BS,0)

/*--------------------------------------
/ Mr. and Mrs. Pacman (BY35-872: 05/82)
/--------------------------------------*/
static core_tLCDLayout m_mpacDisp[] = {
{ 0, 0, 1,7,CORE_SEG87},{ 0,27,17,7,CORE_SEG87},
{ 3, 0, 9,7,CORE_SEG87},{ 3,27,25,7,CORE_SEG87},
{ 2,18,34,6,CORE_SEG7}, /* credit/ball */
{ 5,17,42,2,CORE_SEG7}, { 5,22,44,2,CORE_SEG7}, { 5,27,46,2,CORE_SEG7},
{0}
};

INITGAME(m_mpac,GEN_BY35_61,m_mpacDisp,FLIP_SWNO(BY35_SWNO(33),BY35_SWNO(37)),8)
BY35_ROMSTARTx00(m_mpac  ,"872-04_2.732", 0x5e542882,
                          "720-53_6.732", 0xc2e92f80)
BY35_SOUND61ROMxx00("872-01_4.532",0xd21ce16d,
                    "872-03_5.532",0x8fcdf853)
BY35_ROMEND
CORE_GAMEDEFNV(m_mpac  ,"Mr. and Mrs. PacMan",1982,"Bally",by35_mBY35_61S,0)

/*-----------------------------------------------------
/ Baby Pacman (Video/Pinball Combo) (BY133-???:  10/82)
/-----------------------------------------------------*/
INITGAMEVP(babypac,0,by_NoOutput,FLIP_SW(FLIP_L),0)
BYVP_ROMSTARTx00(babypac,	"891-u2.732", 0x7f7242d1,
							"891-u6.732", 0x6136d636,
							"891-u9.764",  0x7fa570f3,
							"891-u10.764", 0x28f4df8b,
							"891-u11.764", 0x0a5967a4,
							"891-u12.764", 0x58cfe542,
							"891-u29.764", 0x0b57fd5d)
BYVP_ROMEND
CORE_GAMEDEFNV(babypac,"Baby Pacman (Video/Pinball Combo)",1982,"Bally",byVP_mVP1,GAME_NOT_WORKING)
//CORE_GAMEDEFNVR90(babypac,"Baby Pacman (Video/Pinball Combo)",1982,"Bally",byVP_mVP1,GAME_NOT_WORKING)

/*same as eballdlx*/    //BY35      10/82 Eight Ball Deluxe Limited Edition

/*---------------------------------------------------
/ BMX (BY35-888: 11/82) - Uses AS2518-51 Sound Board
/----------------------------------------------------*/
INITGAME(bmx,GEN_BY35_51,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(bmx,"888-03_2.732", 0x038cf1be,
                     "720-53_6.732", 0xc2e92f80)
BY35_SOUND51ROM0("888-02_4.532",0x5692c679)
BY35_ROMEND
CORE_GAMEDEFNV(bmx,"BMX",1983,"Bally",by35_mBY35_51S,0)

/*-----------------------------------------------------------
/ Grand Slam (BY35-???: 01/83) - Uses AS2888-51 Sound Board 
/-----------------------------------------------------------*/
INITGAME(granslam ,GEN_BY35_51,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(granslam,	"grndslam.u2", 0x66aea9dc,
							"grndslam.u6", 0x9e6ccea1)
BY35_SOUND51ROM0(			"grndslam.u4", 0x5692c679)
BY35_ROMEND
CORE_GAMEDEFNV(granslam,"Grand Slam",1983,"Bally",by35_mBY35_51S,0)

/*same as cenatur*/     //BY35      06/83 Centaur II

/*----------------------------------------------------------
/ Gold Ball (BY35-???: 10/83)  - Uses AS2518-51 Sound Board
/----------------------------------------------------------*/
INITGAME(goldball,GEN_BY35_51,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(goldball,	"gold2732.u2", 0x3169493c,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND51ROM0(			"gb_u4.532",0x2dcb0315)
BY35_ROMEND
CORE_GAMEDEFNV(goldball,"Gold Ball",1983,"Bally",by35_mBY35_51S,GAME_NOT_WORKING)

/********************************************************/
/******* Games Below use Cheap Squeak Sound Board *******/
/********************************************************/

/*--------------------------------
/ X's & O's (BY35-???: 12/83)
/-------------------------------*/
INITGAME(xsandos ,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(xsandos ,	"x&os2732.u2", 0x068dfe5a,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROMx2(			"720_u3.snd",0x5d8e2adb)
BY35_ROMEND
CORE_GAMEDEFNV(xsandos ,"X's & O's",1983,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

//??        ??/84 Mysterian

/*-----------------------------------------------------------------
/ Granny and the Gators (Video/Pinball Combo) - (BY35-???: 01/84)
/----------------------------------------------------------------*/
INITGAMEVP(granny,0,by_NoOutput,FLIP_SW(FLIP_L),0)
BYVP_ROMSTART100(granny,	"cpu_u2.532", 0xd45bb956,
							"cpu_u6.532", 0x306aa673,
							"vid_u4.764", 0x3a3d4c6b,
							"vid_u5.764", 0x78bcb0fb,
							"vid_u6.764", 0x8d8220a6,
							"vid_u7.764", 0xaa71cf29,
							"vid_u8.764", 0xa442bc01,
							"vid_u9.764", 0x6b67a1f7,							
							"cs_u3.764", 0x0a39a51d)
BYVP_ROMEND
CORE_GAMEDEFNV(granny,"Granny and the Gators (Video/Pinball Combo)",1984,"Bally",byVP_mVP2,GAME_NOT_WORKING)
//CORE_GAMEDEFNVR90(granny,"Granny and the Gators (Video/Pinball Combo)",1984,"Bally",byVP_mVP2,GAME_NOT_WORKING)

/*--------------------------------
/ Kings of Steel (BY35-???: 05/84)
/-------------------------------*/
INITGAME(kosteel ,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(kosteel ,	"kngs2732.u2", 0xf876d8f2,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROM11(			"kngsu3.snd",0x11b02dca,
							"kngsu4.snd",0xf3e4d2f6)
BY35_ROMEND
CORE_GAMEDEFNV(kosteel ,"Kings of Steel",1984,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Black Pyramid (BY35-???: 07/84)
/-------------------------------*/
INITGAME(blakpyra,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(blakpyra,	"blkp2732.u2", 0x600535b0,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROM11(			"bp_u3.532",0xa5005067,
							"bp_u4.532",0x57978b4a)
BY35_ROMEND
CORE_GAMEDEFNV(blakpyra,"Black Pyramid",1984,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Spy Hunter (BY35-???: 10/84)
/-------------------------------*/
INITGAME(spyhuntr,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(spyhuntr,	"spy-2732.u2", 0x9e930f2d,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROM11(			"spy_u3.532",0x95ffc1b8,
							"spy_u4.532",0xa43887d0)
BY35_ROMEND
CORE_GAMEDEFNV(spyhuntr,"Spy Hunter",1984,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

//??        ??/85 Hot Shotz

/*-------------------------------------
/ Fireball Classic (BY35-???: 02/85)
/------------------------------------*/
INITGAME(fbclass ,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(fbclass ,	"fb-class.u2", 0x32faac6c,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROM11(			"fbcu3.snd",0x1ad71775,
							"fbcu4.snd",0x697ab16f)
BY35_ROMEND
CORE_GAMEDEFNV(fbclass ,"Fireball Classic",1985,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Cybernaut (BY35-???: 05/85)
/-------------------------------*/
INITGAME(cybrnaut,GEN_BY35_45,dispBy7,FLIP_SW(FLIP_L),0)
BY35_ROMSTARTx00(cybrnaut,	"cybe2732.u2", 0x0610b0e0,
							"720-5332.u6", 0xc2e92f80)
BY35_SOUND45ROMx2(			"cybu3.snd",0xa3c1f6e7)
BY35_ROMEND
CORE_GAMEDEFNV(cybrnaut,"Cybernaut",1985,"Bally",by35_mBY35_45S,GAME_IMPERFECT_SOUND)

/****************************************************/
/* BALLY MPU-6803*/
/****************************************************/

//Games below use Squalk & Talk Sound Hardware

/*------------------------------------
/ Eight Ball Champ (6803-0B38: 09/85) - Manual says can work with Cheap Squeek also via operator setting
/------------------------------------*/
//CPU Works
INITGAME6803(eballchp,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTARTx4(eballchp, "u3_cpu.128", 0x025f3008)
BY6803_SOUND_S1_x111(		"u3_snd.532", 0x4836d70d,
							"u4_snd.532", 0x4b49d94d,
							"u5_snd.532", 0x655441df)
BY6803_ROMEND
CORE_GAMEDEFNV(eballchp,"Eight Ball Champ",1985,"Bally",by_mBY6803S1,0)

/*------------------------------------
/ Beat the Clock (6803-0C70: 11/85) - ??
/------------------------------------*/
//CPU Works
INITGAME6803(beatclck,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTARTx4(beatclck,	"btc_u3.cpu", 0x9ba822ab)
BY6803_SOUND_S1_1111(		"btc_u2.snd", 0xfd22fd2a,
							"btc_u3.snd", 0x22311a4a,
							"btc_u4.snd", 0xaf1cf23b,
							"btc_u5.snd", 0x230cf329)
BY6803_ROMEND
CORE_GAMEDEFNV(beatclck,"Beat the Clock",1985,"Bally",by_mBY6803S1,0)

/*------------------------------------
/ Lady Luck (6803-0E34: 02/86) - Uses Cheap Squeek (Same as Last MPU-35 Line of games)
/------------------------------------*/
//CPU Works
INITGAME6803(ladyluck,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTARTx4(ladyluck,	"u3.cpu", 0x129f41f5)
BY35_SOUND45ROM11(			"u3_snd.532", 0x1bdd6e2b,
							"u4_snd.532", 0xe9ef01e6)
BY6803_ROMEND
CORE_GAMEDEFNV(ladyluck,"Lady Luck",1986,"Bally",by_mBY6803S1,GAME_NO_SOUND)

// Games below use Turbo Cheap Squalk Sound Hardware

/*--------------------------------
/ MotorDome (6803-0E14: 05/86)
/-------------------------------*/
//CPU & Sound Works?
INITGAME6803(motrdome,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(motrdome,	"modm_u2.dat", 0x820ca073,
							"modm_u3.dat", 0xaae7c418)
BY6803_SOUND_S2_8(			"modm_u7.snd", 0x29ce4679)
BY6803_ROMEND
CORE_GAMEDEFNV(motrdome,"MotorDome",1986,"Bally",by_mBY6803S2,0)

/*------------------------------------
/ Karate Fight (6803-????: 06/86) - Proto for Black Belt?
/------------------------------------*/

/*------------------------------------
/ Black Belt (6803-0E52: 07/86)
/------------------------------------*/
//CPU & Sound DOES NOT WORK! - Bad rom?
INITGAME6803(blackblt,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(blackblt,	"u2.cpu", 0x7c771910,
							"u3.cpu", 0xbad0f4c3)
BY6803_SOUND_S2_8(			"blck_u7.snd",0xdb8bce07)
BY6803_ROMEND
CORE_GAMEDEFNV(blackblt,"Black Belt",1986,"Bally",by_mBY6803S2,0)

// 1st Game to use Sounds Deluxe Sound Hardware

/*------------------------------------
/ Special Force (6803-0E47: 08/86)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(specforc,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(specforc,	"u2_revc.128", 0xd042af04,
							"u3_revc.128", 0xd48a5eaf)
BY6803_SOUND_S3_0000(		"u12_snd.512", 0x4f48a490,
							"u11_snd.512", 0xb16eb713,
							"u14_snd.512", 0x6911fa51,
							"u13_snd.512", 0x3edda92d)
BY6803_ROMEND
CORE_GAMEDEFNV(specforc,"Special Force",1986,"Bally",by_mBY6803S3,0)

/*------------------------------------
/ Strange Science (6803-0E35: 10/86)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(strngsci,0,dispBy7,FLIP_SWNO(5,8),0)
BY6803_ROMSTART44(strngsci,	"cpu_u2.128", 0x2ffcf284,
							"cpu_u3.128", 0x35257931)
BY6803_SOUND_S2_8(			"sound_u7.256",0xbc33901e)
BY6803_ROMEND
CORE_GAMEDEFNV(strngsci,"Strange Science",1986,"Bally",by_mBY6803S2,0)

/*------------------------------------
/ City Slicker (6803-0E79: 03/87) 
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(cityslck,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(cityslck,	"u2.128", 0x94bcf162,
							"u3.128", 0x97cb2bca)
BY6803_SOUND_S2_0(			"u7_snd.512",0x6941d68a)
BY6803_ROMEND
CORE_GAMEDEFNV(cityslck,"City Slicker",1987,"Bally",by_mBY6803S2A,0)

/*------------------------------------
/ Hardbody (6803-0E94: 03/87)
/------------------------------------*/
//CPU & Sound Works?
//Switchs 8 in column 5 & Entire Column 6 must be closed for lamps to operate
//(Might be less than switches listed, but haven't figured out which)
INITGAME6803(hardbody,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(hardbody,	"cpu_u2.128", 0xc9248b47,
							"cpu_u3.128", 0x31c255d0)
BY6803_SOUND_S2_0(			"sound_u7.512",0xc96f91af)
BY6803_ROMEND
CORE_GAMEDEFNV(hardbody,"Hardbody",1987,"Bally",by_mBY6803S2A,0)

// Games below use Sounds Deluxe Sound Hardware

/*--------------------------------
/ Party Animal (6803-0H01: 05/87)
/-------------------------------*/
//CPU & Sound Works?
INITGAME6803(prtyanim,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(prtyanim,	"cpu_u2.128", 0xabdc0b2d,
							"cpu_u3.128", 0xe48b2d63)
BY6803_SOUND_S3_0000(		"snd_u12.512", 0x265a9494,
							"snd_u11.512", 0x20be998f,
							"snd_u14.512", 0x639b3db1,
							"snd_u13.512", 0xb652597b)
BY6803_ROMEND
CORE_GAMEDEFNV(prtyanim,"Party Animal",1987,"Bally",by_mBY6803S3,0)

/*-----------------------------------------
/ Heavy Metal Meltdown (6803-0H03: 08/87)
/-----------------------------------------*/
//CPU Works & Sound Does NOT Work!
//
//3 Different Sources claim that this games only uses U11&U12..
//Must be correct, as it DOES pass the start up test.
INITGAME6803(hvymetal,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(hvymetal,	"u2.rom", 0x53466e4e,
							"u3.rom", 0x0a08ae7e)
BY6803_SOUND_S3_00xx(		"u12.rom",0x77933258,
							"u11.rom",0xb7e4de7d)
BY6803_ROMEND
CORE_GAMEDEFNV(hvymetal,"Heavy Metal Meltdown",1987,"Bally",by_mBY6803S3,0)

/*------------------------------------
/ Dungeons & Dragons (6803-0H06: 10/87)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(dungdrag,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(dungdrag,	"cpu_u2.128", 0xcefd4330,
							"cpu_u3.128", 0x4bacc7f5)
BY6803_SOUND_S3_0000(		"snd_u12.512", 0xdd95f851,
							"snd_u11.512", 0xdcd461b3,
							"snd_u14.512", 0xdd9e61eb,
							"snd_u13.512", 0x1e2d9211)
BY6803_ROMEND
CORE_GAMEDEFNV(dungdrag,"Dungeons & Dragons",1987,"Bally",by_mBY6803S3,0)

/*------------------------------------------------
/ Escape from the Lost World (6803-0H05: 01/88)
/-----------------------------------------------*/
//CPU & Sound Works?
INITGAME6803(esclwrld,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(esclwrld,	"u2.128", 0xb11a97ea,
							"u3.128", 0x5385a562)
BY6803_SOUND_S3_0000(		"u12.512", 0x0c003473,
							"u11.512", 0x360f6658,
							"u14.512", 0x0b92afff,
							"u13.512", 0xb056842e)
BY6803_ROMEND
CORE_GAMEDEFNV(esclwrld,"Escape from the Lost World",1988,"Bally",by_mBY6803S3,0)

/*------------------------------------
/ Blackwater 100 (6803-0H07: 03/88)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(black100,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(black100,	"u2.cpu", 0x411fa773,
							"u3.cpu", 0xd6f6f890)
BY6803_SOUND_S3_0000(		"u12.bin", 0xa0ecb282,
							"u11.bin", 0x3f117ba3,
							"u14.bin", 0xb45bf5c4,
							"u13.bin", 0xf5890443)
BY6803_ROMEND
CORE_GAMEDEFNV(black100,"Blackwater 100",1988,"Bally",by_mBY6803S3,0)

//Games below use 6803 MPU & Williams System 11C Sound Hardware

/*-------------------------------------------------------------
/ Truck Stop (6803-2001: 12/88) - These are ProtoType ROMS?
/-------------------------------------------------------------*/
//CPU & Sound Works?
INITGAME6803(truckstp,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(truckstp,	"u2_p2.128", 0x3c397dec,
							"u3_p2.128", 0xd7ac519a)
BY6803_SOUND_S4_888(		"u4sndp1.256",0x120a386f,
							"u19sndp1.256",0x5cd43dda,
							"u20sndp1.256",0x93ac5c33)
BY6803_ROMEND
CORE_GAMEDEFNV(truckstp,"Truck Stop",1988,"Bally",by_mBY6803S4,0)

/*-----------------------------------------------------------
/ Atlantis (6803-2006: 03/89)
/-----------------------------------------------------------*/
//CPU (No lights) & Sound Works?
//Does NOT appear to be switch problem, but not sure what it is!
INITGAME6803(atlantis,0,dispBy7,FLIP_SW(FLIP_L),0)
BY6803_ROMSTART44(atlantis,	"u26_cpu.rom", 0xb98491e1,
							"u27_cpu.rom", 0x8ea2b4db)
BY6803_SOUND_S4_888(		"u4_snd.rom",0x6a48b588,
							"u19_snd.rom",0x1387467c,
							"u20_snd.rom",0xd5a6a773)
BY6803_ROMEND
CORE_GAMEDEFNV(atlantis,"Atlantis",1989,"Bally",by_mBY6803S4,GAME_NOT_WORKING)

/****************************************************/
/* STERN MPU-100 (almost identicawl to Bally MPU-17) */
/****************************************************/

/*--------------------------------
/ Stingray
/-------------------------------*/
INITGAME(stingray,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(stingray,  "cpu_u2.716",0x1db32a33,
							"cpu_u6.716",0x432e9b9e)
BY35_ROMEND
CORE_GAMEDEFNV(stingray,"Stingray",1977,"Stern",by35_mBY17,GAME_USES_CHIMES)

/*--------------------------------
/ Pinball
/-------------------------------*/
INITGAME(pinball,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(pinball,   "cpu_u2.716",0x1db32a33,
							"cpu_u6.716",0x432e9b9e)
BY35_ROMEND
CORE_GAMEDEFNV(pinball,"Pinball",1977,"Stern",by35_mBY17,GAME_USES_CHIMES)

/*--------------------------------
/ Stars
/-------------------------------*/
INITGAME(stars,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(stars,     "cpu_u2.716",0x630d05df,
							"cpu_u6.716",0x57e63d42)
BY35_ROMEND
CORE_GAMEDEFNV(stars,"Stars",1978,"Stern",by35_mBY17,GAME_USES_CHIMES)

/*--------------------------------
/ Memory Lane
/-------------------------------*/
INITGAME(memlane,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(memlane,   "cpu_u2.716",0xaff1859d,
							"cpu_u6.716",0x3e236e3c)
BY35_ROMEND
CORE_GAMEDEFNV(memlane,"Memory Lane",1978,"Stern",by35_mBY17,GAME_USES_CHIMES)

/* ---------------------------------------------------*/
/* All games below used MPU-100 - Sound Board: SB-100 */
/* ---------------------------------------------------*/

/*--------------------------------
/ Lectronamo
/-------------------------------*/
INITGAME(lectrono,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(lectrono,  "cpu_u2.716",0x79e918ff,
							"cpu_u6.716",0x7c6e5fb5)
BY35_ROMEND
CORE_GAMEDEFNV(lectrono,"Lectronamo",1978,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Wildfyre
/-------------------------------*/
INITGAME(wildfyre,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(wildfyre,  "cpu_u2.716",0x063f8b5e,
							"cpu_u6.716",0x00336fbc)
BY35_ROMEND
CORE_GAMEDEFNV(wildfyre,"Wildfyre",1978,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Nugent
/-------------------------------*/
INITGAME(nugent,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(nugent,    "cpu_u2.716",0x79e918ff,
							"cpu_u6.716",0x7c6e5fb5)
BY35_ROMEND
CORE_GAMEDEFNV(nugent,"Nugent",1978,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Dracula
/-------------------------------*/
INITGAME(dracula,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(dracula,   "cpu_u2.716",0x63f8b5e,
							"cpu_u6.716",0x336fbc)
BY35_ROMEND
CORE_GAMEDEFNV(dracula,"Dracula",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Trident - Don't have!
/-------------------------------*/
INITGAME(trident,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(trident,   "cpu_u2.716",0x934e49dd,
							"cpu_u6.716",0x540bce56)
BY35_ROMEND
CORE_GAMEDEFNV(trident,"Trident",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Hot Hand
/-------------------------------*/
INITGAME(hothand,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(hothand,   "cpu_u2.716",0x5e79ea2e,
							"cpu_u6.716",0xfb955a6f)
BY35_ROMEND
CORE_GAMEDEFNV(hothand,"Hot Hand",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Magic
/-------------------------------*/
INITGAME(magic,GEN_STMPU100,dispBy6,FLIP_SW(FLIP_L),0)
BY17_ROMSTARTx88(magic,     "cpu_u2.716",0x8838091f,
							"cpu_u6.716",0xfb955a6f)
BY35_ROMEND
CORE_GAMEDEFNV(magic,"Magic",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)


/****************************************************/
/* STERN MPU-200 (almost identical to Bally MPU-35) */
/****************************************************/

/* ---------------------------------------------------*/
/* All games below used MPU-200 - Sound Board: SB-300 */
/* ---------------------------------------------------*/

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteor,GEN_STMPU200,dispBy6,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(meteor,  "cpu_u1.716",0xe0fd8452,
							"cpu_u5.716",0x43a46997,
							"cpu_u2.716",0xfd396792,
							"cpu_u6.716",0x03fa346c)
BY35_ROMEND
CORE_GAMEDEFNV(meteor,"Meteor",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Galaxy
/-------------------------------*/
INITGAME(galaxy,GEN_STMPU200,dispBy6,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(galaxy,  "cpu_u1.716",0x35656b67,
							"cpu_u5.716",0x12be0601,
							"cpu_u2.716",0x08bdb285,
							"cpu_u6.716",0xad846a42)
BY35_ROMEND
CORE_GAMEDEFNV(galaxy,"Galaxy",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Ali
/-------------------------------*/
INITGAME(ali,GEN_STMPU200,dispBy6,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(ali,		"cpu_u1.716",0x92e75b40,
							"cpu_u5.716",0x119a4300,
							"cpu_u2.716",0x9c91d08f,
							"cpu_u6.716",0x7629db56)
BY35_ROMEND
CORE_GAMEDEFNV(ali,"Ali",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Big Game
/-------------------------------*/
INITGAME(biggame,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(biggame, "cpu_u1.716",0xf59c7514,
							"cpu_u5.716",0x57df1dc5,
							"cpu_u2.716",0x0251039b,
							"cpu_u6.716",0x801e9a66)
BY35_ROMEND
CORE_GAMEDEFNV(biggame,"Big Game",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Seawitch
/-------------------------------*/
INITGAME(seawitch,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(seawitch,"cpu_u1.716",0xc214140b,
							"cpu_u5.716",0xab2eab3a,
							"cpu_u2.716",0xb8844174,
							"cpu_u6.716",0x6c296d8f)
BY35_ROMEND
CORE_GAMEDEFNV(seawitch,"Seawitch",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Cheetah
/-------------------------------*/
INITGAME(cheetah,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(cheetah, "cpu_u1.716",0x6a845d94,
							"cpu_u5.716",0xe7bdbe6c,
							"cpu_u2.716",0xa827a1a1,
							"cpu_u6.716",0xed33c227)
BY35_ROMEND
CORE_GAMEDEFNV(cheetah,"Cheetah",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Quicksilver
/-------------------------------*/
INITGAME(quicksil,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(quicksil,"cpu_u1.716",0x0bf508d1,
							"cpu_u5.716",0xe2634491,
							"cpu_u2.716",0x8cb01165,
							"cpu_u6.716",0x8c0e336a)
BY35_ROMEND
CORE_GAMEDEFNV(quicksil,"Quicksilver",1980,"Stern",by35_mBY35_50,GAME_NOT_WORKING)

/*--------------------------------
/ Nine Ball
/-------------------------------*/
INITGAME(nineball,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(nineball,"cpu_u1.716",0xfcb58f97,
							"cpu_u5.716",0xc7c62161,
							"cpu_u2.716",0xbdd7f258,
							"cpu_u6.716",0x7e831499)
BY35_ROMEND
CORE_GAMEDEFNV(nineball,"Nine Ball",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Free Fall
/-------------------------------*/
INITGAME(freefall,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(freefall,"cpu_u1.716",0xd13891ad,
							"cpu_u5.716",0x77bc7759,
							"cpu_u2.716",0x82bda054,
							"cpu_u6.716",0x68168b97)
BY35_ROMEND
CORE_GAMEDEFNV(freefall,"Free Fall",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Split Second
/-------------------------------*/
INITGAME(splitsec,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(splitsec,"cpu_u1.716",0xc6ff9aa9,
							"cpu_u5.716",0xfda74efc,
							"cpu_u2.716",0x81b9f784,
							"cpu_u6.716",0xecbedb0a)
BY35_ROMEND
CORE_GAMEDEFNV(splitsec,"Split Second",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Catacomb
/-------------------------------*/
INITGAME(catacomb,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(catacomb,"cpu_u1.716",0xd445dd40,
							"cpu_u5.716",0xd717a545,
							"cpu_u2.716",0xbc504409,
							"cpu_u6.716",0xda61b5a2)
BY35_ROMEND
CORE_GAMEDEFNV(catacomb,"Catacomb",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Iron Maiden
/-------------------------------*/
INITGAME(ironmaid,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(ironmaid,"cpu_u1.716",0xe15371a4,
							"cpu_u5.716",0x84a29c01,
							"cpu_u2.716",0x981ac0dd,
							"cpu_u6.716",0x4e6f9c25)
BY35_ROMEND
CORE_GAMEDEFNV(ironmaid,"Iron Maiden",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Viper
/-------------------------------*/
INITGAME(viper,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(viper,   "cpu_u1.716",0xd0ea0aeb,
							"cpu_u5.716",0xd26c7273,
							"cpu_u2.716",0xd03f1612,
							"cpu_u6.716",0x96ff5f60)
BY35_ROMEND
CORE_GAMEDEFNV(viper,"Viper",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Dragonfist
/-------------------------------*/
static core_tLCDLayout dispDragfist[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4, 4,36,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,34,2,CORE_SEG7},{0}
};
INITGAME(dragfist,GEN_STMPU200,dispDragfist,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(dragfist,"cpu_u1.716",0x4cbd1a38,
							"cpu_u5.716",0x1783269a,
							"cpu_u2.716",0x9ac8292b,
							"cpu_u6.716",0xa374c8f9)
BY35_ROMEND
CORE_GAMEDEFNV(dragfist,"Dragonfist",1982,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/* -----------------------------------------------------------*/
/* All games below used MPU-200 - Sound Board: SB-300, VS-100 */
/* -----------------------------------------------------------*/

/*--------------------------------
/ Flight 2000
/-------------------------------*/
INITGAME(flight2k,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(flight2k,"cpu_u1.716",0xdf9efed9,
							"cpu_u5.716",0x38c13649,
							"cpu_u2.716",0x425fae6a,
							"cpu_u6.716",0xdc243186)
BY35_ROMEND
CORE_GAMEDEFNV(flight2k,"Flight 2000",1979,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Stargazer
/-------------------------------*/
INITGAME(stargzr,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(stargzr, "cpu_u1.716",0x83606fd4,
							"cpu_u5.716",0xc54ae389	,
							"cpu_u2.716",0x1a4c7dcb,
							"cpu_u6.716",0x4e1f4dc6)
BY35_ROMEND
CORE_GAMEDEFNV(stargzr,"Stargazer",1980,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Lightning
/-------------------------------*/
static core_tLCDLayout dispLightnin[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4, 4,34,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,36,2,CORE_SEG7},{0}
};
INITGAME(lightnin,GEN_STMPU200,dispLightnin,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(lightnin,"cpu_u1.716",0xd3469d0a,
							"cpu_u5.716",0xcd52262d,
							"cpu_u2.716",0xe0933419,
							"cpu_u6.716",0xdf221c6b)
BY35_ROMEND
CORE_GAMEDEFNV(lightnin,"Lightning",1981,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Orbitor 1
/-------------------------------*/
static core_tLCDLayout dispOrbitor[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4, 4,36,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7}, {4,20,34,2,CORE_SEG7},{0}
};
INITGAME(orbitor1,GEN_STMPU200,dispOrbitor,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(orbitor1, "cpu_u1.716",0x575520e3,
							"cpu_u5.716",0xd31f27a8,
							"cpu_u2.716",0x4421d827,
							"cpu_u6.716",0x8861155a)
BY35_ROMEND
CORE_GAMEDEFNV(orbitor1,"Orbitor 1",1982,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*--------------------------------
/ Cue (Proto - Never released)
/-------------------------------*/
INITGAME(cue,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(cue,"cpu_u1.716",0x00000000,
							"cpu_u5.716",0x00000000,
							"cpu_u2.716",0x00000000,
							"cpu_u6.716",0x00000000)
BY35_ROMEND
CORE_GAMEDEFNV(cue,"Cue",1982,"Stern",by35_mBY35_50,GAME_NO_SOUND)

/*----------------------------------------
/ Lazer Lord (Proto - Never released)
/---------------------------------------*/
INITGAME(lazrlord,GEN_STMPU200,dispBy7,FLIP_SW(FLIP_L),0)
ST200_ROMSTART8888(lazrlord,"cpu_u1.716",0x32a6f341,
							"cpu_u5.716",0x17583ba4,
							"cpu_u2.716",0x669f3a8e,
							"cpu_u6.716",0x395327a3)
BY35_ROMEND
CORE_GAMEDEFNV(lazrlord,"Lazer Lord",1984,"Stern",by35_mBY35_50,GAME_NO_SOUND)
