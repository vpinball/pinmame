#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "wmssnd.h"
#include "by6803.h"

/* 4 x 7 digit 7 Segment Display (6 Scoring Digits, & 1 Comma Digit)
   AND 1 x 6 digit 7 Segment for Ball & Credit (But appears as 2 x 2 digit*/
static const core_tLCDLayout dispBy7C[] = {
  {0, 0, 1,7,CORE_SEG7}, {0,16, 9,7,CORE_SEG7},
  {2, 0,17,7,CORE_SEG7}, {2,16,25,7,CORE_SEG7},
  {4, 4,35,2,CORE_SEG7}, {4,10,38,2,CORE_SEG7},{0}
};

/* 4 x 7 digit 9 Segment Display */
static const core_tLCDLayout dispBy104[] = {
  {0, 0, 9, 7,CORE_SEG9}, {0,16, 2, 7,CORE_SEG9},
  {0,32,28, 7,CORE_SEG9}, {0,48,35, 1,CORE_SEG9}, {0,50,22, 6,CORE_SEG9}, {0}
};
BY6803_INPUT_PORTS_START(by6803, 1)   BY6803_INPUT_PORTS_END
BY6803A_INPUT_PORTS_START(by6803a, 1) BY6803_INPUT_PORTS_END

#define INITGAME6803(name, gen, disp, flip, lamps, sb, db) \
static core_tGameData name##GameData = {gen,disp,{flip,0,lamps,0,sb, db}}; \
static void init_##name(void) { core_gameData = &name##GameData; }
#define FLIP6803 (FLIP_SWNO(5,7))

/****************************************************/
/* BALLY MPU-6803*/
/****************************************************/
//Games below use Squalk & Talk Sound Hardware
/*------------------------------------
/ Eight Ball Champ (6803-0B38: 09/85) - Manual says can work with Cheap Squeek also via operator setting
/------------------------------------*/
//CPU Works
INITGAME6803(eballchp,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY61, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(eballchp,"u3_cpu.128",0x025f3008)
BY61_SOUNDROMx000(         "u3_snd.532",0x4836d70d,
                           "u4_snd.532",0x4b49d94d,
                           "u5_snd.532",0x655441df)
BY6803_ROMEND
#define input_ports_eballchp input_ports_by6803
CORE_GAMEDEFNV(eballchp,"Eight Ball Champ",1985,"Bally",by_mBY6803_61S,0)

INITGAME6803(eballch2,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY45, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(eballch2,"u3_cpu.128",0x025f3008)
BY45_SOUNDROM22(           "ebcu3.snd", 0x00000000,
                           "ebcu4.snd", 0x00000000)
BY6803_ROMEND
#define input_ports_eballch2 input_ports_by6803
CORE_CLONEDEFNV(eballch2,eballchp,"Eight Ball Champ (Cheap Squeek)",1985,"Bally",by_mBY6803_45S,0)

/*------------------------------------
/ Beat the Clock (6803-0C70: 11/85) - ??
/------------------------------------*/
//CPU Works
INITGAME6803(beatclck,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY61, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(beatclck,"btc_u3.cpu",0x9ba822ab)
BY61_SOUNDROM0000(         "btc_u2.snd",0xfd22fd2a,
                           "btc_u3.snd",0x22311a4a,
                           "btc_u4.snd",0xaf1cf23b,
                           "btc_u5.snd",0x230cf329)
BY6803_ROMEND
#define input_ports_beatclck input_ports_by6803
CORE_GAMEDEFNV(beatclck,"Beat the Clock",1985,"Bally",by_mBY6803_61S,0)

/*------------------------------------
/ Lady Luck (6803-0E34: 02/86) - Uses Cheap Squeek (Same as Last MPU-35 Line of games)
/------------------------------------*/
//CPU Works
INITGAME6803(ladyluck,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY45, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(ladyluck,"u3.cpu",    0x129f41f5)
BY45_SOUNDROM22(           "u3_snd.532",0x1bdd6e2b,
                           "u4_snd.532",0xe9ef01e6)
BY6803_ROMEND
#define input_ports_ladyluck input_ports_by6803
CORE_GAMEDEFNV(ladyluck,"Lady Luck",1986,"Bally",by_mBY6803_45S,0)

// Games below use Turbo Cheap Squalk Sound Hardware

/*--------------------------------
/ MotorDome (6803-0E14: 05/86)
/-------------------------------*/
//CPU & Sound Works?
INITGAME6803(motrdome,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(motrdome,"modm_u2.dat",0x820ca073,
                           "modm_u3.dat",0xaae7c418)
BYTCS_SOUNDROM8(           "modm_u7.snd",0x29ce4679)
BY6803_ROMEND
#define input_ports_motrdome input_ports_by6803
CORE_GAMEDEFNV(motrdome,"MotorDome",1986,"Bally",by_mBY6803_TCSS,0)

/*------------------------------------
/ Karate Fight (6803-????: 06/86) - European version of Black Belt
/------------------------------------*/

/*------------------------------------
/ Black Belt (6803-0E52: 07/86)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(blackblt,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(blackblt,"u2.cpu",     0x7c771910,
                           "u3.cpu",     0xbad0f4c3)
BYTCS_SOUNDROM8(           "blck_u7.snd",0xdb8bce07)
BY6803_ROMEND
#define input_ports_blackblt input_ports_by6803
CORE_GAMEDEFNV(blackblt,"Black Belt",1986,"Bally",by_mBY6803_TCSS,0)

// 1st Game to use Sounds Deluxe Sound Hardware

/*------------------------------------
/ Special Force (6803-0E47: 08/86)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(specforc,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(specforc,"u2_revc.128",0xd042af04,
                           "u3_revc.128",0xd48a5eaf)
BYSD_SOUNDROM0000(         "u12_snd.512",0x4f48a490,
                           "u11_snd.512",0xb16eb713,
                           "u14_snd.512",0x6911fa51,
                           "u13_snd.512",0x3edda92d)
BY6803_ROMEND
#define input_ports_specforc input_ports_by6803
CORE_GAMEDEFNV(specforc,"Special Force",1986,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Strange Science (6803-0E35: 10/86)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(strngsci,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(strngsci, "cpu_u2.128",  0x2ffcf284,
                            "cpu_u3.128",  0x35257931)
BYTCS_SOUNDROM8(            "sound_u7.256",0xbc33901e)
BY6803_ROMEND
#define input_ports_strngsci input_ports_by6803
CORE_GAMEDEFNV(strngsci,"Strange Science",1986,"Bally",by_mBY6803_TCSS,0)

/*------------------------------------
/ City Slicker (6803-0E79: 03/87)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(cityslck,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(cityslck, "u2.128",    0x94bcf162,
                            "u3.128",    0x97cb2bca)
BYTCS_SOUNDROM0(            "u7_snd.512",0x6941d68a)
BY6803_ROMEND
#define input_ports_cityslck input_ports_by6803
CORE_GAMEDEFNV(cityslck,"City Slicker",1987,"Bally",by_mBY6803_TCSS,0)

/*------------------------------------
/ Hardbody (6803-0E94: 03/87)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(hardbody,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(hardbody,"cpu_u2.128",  0xc9248b47,
                           "cpu_u3.128",  0x31c255d0)
BYTCS_SOUNDROM0(           "sound_u7.512",0xc96f91af)
BY6803_ROMEND
#define input_ports_hardbody input_ports_by6803
CORE_GAMEDEFNV(hardbody,"Hardbody",1987,"Bally",by_mBY6803_TCSS,0)

// Games below use Sounds Deluxe Sound Hardware

/*--------------------------------
/ Party Animal (6803-0H01: 05/87)
/-------------------------------*/
//CPU & Sound Works?
INITGAME6803(prtyanim,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(prtyanim,"cpu_u2.128", 0xabdc0b2d,
                           "cpu_u3.128", 0xe48b2d63)
BYSD_SOUNDROM0000(         "snd_u12.512",0x265a9494,
                           "snd_u11.512",0x20be998f,
                           "snd_u14.512",0x639b3db1,
                           "snd_u13.512",0xb652597b)
BY6803_ROMEND
#define input_ports_prtyanim input_ports_by6803
CORE_GAMEDEFNV(prtyanim,"Party Animal",1987,"Bally",by_mBY6803_SDS,0)

/*-----------------------------------------
/ Heavy Metal Meltdown (6803-0H03: 08/87)
/-----------------------------------------*/
//CPU & Sound Works?
//
//3 Different Sources claim that this games only uses U11&U12..
//Must be correct, as it DOES pass the start up test.
INITGAME6803(hvymetal,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(hvymetal,"u2.rom", 0x53466e4e,
                           "u3.rom", 0x0a08ae7e)
BYSD_SOUNDROM00xx(         "u12.rom",0x77933258,
                           "u11.rom",0xb7e4de7d)
BY6803_ROMEND
#define input_ports_hvymetal input_ports_by6803
CORE_GAMEDEFNV(hvymetal,"Heavy Metal Meltdown",1987,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Dungeons & Dragons (6803-0H06: 10/87)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(dungdrag,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(dungdrag,"cpu_u2.128", 0xcefd4330,
                           "cpu_u3.128", 0x4bacc7f5)
BYSD_SOUNDROM0000(         "snd_u12.512",0xdd95f851,
                           "snd_u11.512",0xdcd461b3,
                           "snd_u14.512",0xdd9e61eb,
                           "snd_u13.512",0x1e2d9211)
BY6803_ROMEND
#define input_ports_dungdrag input_ports_by6803
CORE_GAMEDEFNV(dungdrag,"Dungeons & Dragons",1987,"Bally",by_mBY6803_SDS,0)

// Games below don't use a keypad anymore
/*------------------------------------------------
/ Escape from the Lost World (6803-0H05: 12/87)
/-----------------------------------------------*/
//CPU & Sound Works?
INITGAME6803(esclwrld,GEN_BY6803A,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(esclwrld,"u2.128", 0xb11a97ea,
                           "u3.128", 0x5385a562)
BYSD_SOUNDROM0000(         "u12.512",0x0c003473,
                           "u11.512",0x360f6658,
                           "u14.512",0x0b92afff,
                           "u13.512",0xb056842e)
BY6803_ROMEND
#define input_ports_esclwrld input_ports_by6803a
CORE_GAMEDEFNV(esclwrld,"Escape from the Lost World",1987,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Blackwater 100 (6803-0H07: 03/88)
/------------------------------------*/
//CPU & Sound Works?
INITGAME6803(black100,GEN_BY6803A,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(black100, "u2.cpu", 0x411fa773,
                            "u3.cpu", 0xd6f6f890)
BYSD_SOUNDROM0000(          "u12.bin",0xa0ecb282,
                            "u11.bin",0x3f117ba3,
                            "u14.bin",0xb45bf5c4,
                            "u13.bin",0xf5890443)
BY6803_ROMEND
#define input_ports_black100 input_ports_by6803a
CORE_GAMEDEFNV(black100,"Blackwater 100",1988,"Bally",by_mBY6803_SDS,0)

//Games below use 6803 MPU & Williams System 11C Sound Hardware
/*-------------------------------------------------------------
/ Truck Stop (6803-2001: 12/88) - These are ProtoType ROMS?
/-------------------------------------------------------------*/
//CPU & Sound Works?
INITGAME6803(truckstp,GEN_BY6803A,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_S11CS, BY6803_DISPALPHA)
BY6803_ROMSTART44(truckstp,"u2_p2.128",   0x3c397dec,
                           "u3_p2.128",   0xd7ac519a)
S11CS_SOUNDROM888(         "u4sndp1.256", 0x120a386f,
                           "u19sndp1.256",0x5cd43dda,
                           "u20sndp1.256",0x93ac5c33)
BY6803_ROMEND
#define input_ports_truckstp input_ports_by6803a
CORE_GAMEDEFNV(truckstp,"Truck Stop",1988,"Bally",by_mBY6803_S11CS,0)

/*-----------------------------------------------------------
/ Atlantis (6803-2006: 03/89)
/-----------------------------------------------------------*/
//CPU & Sound Works?
INITGAME6803(atlantis,GEN_BY6803A,dispBy104,FLIP6803,4,SNDBRD_S11CS, BY6803_DISPALPHA)
BY6803_ROMSTART44(atlantis, "u26_cpu.rom",0xb98491e1,
                            "u27_cpu.rom",0x8ea2b4db)
S11CS_SOUNDROM888(          "u4_snd.rom", 0x6a48b588,
                            "u19_snd.rom",0x1387467c,
                            "u20_snd.rom",0xd5a6a773)
BY6803_ROMEND
#define input_ports_atlantis input_ports_by6803a
CORE_GAMEDEFNV(atlantis,"Atlantis",1989,"Bally",by_mBY6803_S11CS,0)
