#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "by35snd.h"
#include "wmssnd.h"
#include "by6803.h"

/* 4 x 7 digit 7 Segment Display (6 Scoring Digits, & 1 Comma Digit)
   AND 1 x 6 digit 7 Segment for Ball & Credit (But appears as 2 x 2 digit*/
static const core_tLCDLayout dispBy7C[] = {
  {0, 0, 1,7,CORE_SEG87F}, {0,16, 9,7,CORE_SEG87F},
  {2, 0,17,7,CORE_SEG87F}, {2,16,25,7,CORE_SEG87F},
  {4, 4,35,2,CORE_SEG87},  {4,10,38,2,CORE_SEG87},{0}
};

/* 4 x 7 digit 9 Segment Display */
static const core_tLCDLayout dispBy104[] = {
  {0, 0, 9,7,CORE_SEG98}, {0,16, 2,7,CORE_SEG98},
  {0,32,28,7,CORE_SEG98}, {0,48,35,1,CORE_SEG10}, {0,50,22,6,CORE_SEG98}, {0}
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
INITGAME6803(eballchp,GEN_BY6803,dispBy7C,FLIP_SWNO(0,26),4,SNDBRD_BY61, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(eballchp,"u3_cpu.128",CRC(025f3008) SHA1(25d310f169b92ce6b348330816ddc3b5710e57da))
BY61_SOUNDROMx000(         "u3_snd.532",CRC(4836d70d) SHA1(a4acc64609d91a84ba4c8101186d07397b496600),
                           "u4_snd.532",CRC(4b49d94d) SHA1(52d5f4b7604601cd86f0e80ed7c4fe09d14f5976),
                           "u5_snd.532",CRC(655441df) SHA1(9da5578856ded3dcdafed67679eb4c4134dc9f81))
BY6803_ROMEND
#define input_ports_eballchp input_ports_by6803
CORE_GAMEDEFNV(eballchp,"Eight Ball Champ",1985,"Bally",by_mBY6803_61S,0)

INITGAME6803(eballch2,GEN_BY6803,dispBy7C,FLIP_SWNO(0,26),4,SNDBRD_BY45, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(eballch2,"u3_cpu.128",CRC(025f3008) SHA1(25d310f169b92ce6b348330816ddc3b5710e57da))
BY45_SOUNDROM22(           "ebcu3.snd", NO_DUMP,
                           "ebcu4.snd", NO_DUMP)
BY6803_ROMEND
#define input_ports_eballch2 input_ports_by6803
CORE_CLONEDEFNV(eballch2,eballchp,"Eight Ball Champ (Cheap Squeek)",1985,"Bally",by_mBY6803_45S,0)

/*------------------------------------
/ Beat the Clock (6803-0C70: 11/85) - ??
/------------------------------------*/
INITGAME6803(beatclck,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY61, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(beatclck,"btc_u3.cpu",CRC(9ba822ab) SHA1(f28d38411df3978bcaf24177fa1b47037a586cbb))
BY61_SOUNDROM0000(         "btc_u2.snd",CRC(fd22fd2a) SHA1(efad3b94e91d07930ada5366d389f35377dfbd99),
                           "btc_u3.snd",CRC(22311a4a) SHA1(2c22ba9228e44e68b9308b3bf8803edcd70fa5b9),
                           "btc_u4.snd",CRC(af1cf23b) SHA1(ebfa3afafd7850dfa2664d3c640fbfa631012455),
                           "btc_u5.snd",CRC(230cf329) SHA1(45b17a785b81cd5b1d7fdfb720cf1990994b52b7))
BY6803_ROMEND
#define input_ports_beatclck input_ports_by6803
CORE_GAMEDEFNV(beatclck,"Beat the Clock",1985,"Bally",by_mBY6803_61S,0)

/*------------------------------------
/ Lady Luck (6803-0E34: 02/86) - Uses Cheap Squeek (Same as Last MPU-35 Line of games)
/------------------------------------*/
INITGAME6803(ladyluck,GEN_BY6803,dispBy7C,FLIP_SW(FLIP_L),4,SNDBRD_BY45, BY6803_DISP7SEG)
BY6803_ROMSTARTx4(ladyluck,"u3.cpu",    CRC(129f41f5) SHA1(0351419814d3f4e98a4572fdec9d53e12fe6b6be))
BY45_SOUNDROM22(           "u3_snd.532",CRC(1bdd6e2b) SHA1(14fc25b5f8eefe8ffab062f83e06ec19403aa00a),
                           "u4_snd.532",CRC(e9ef01e6) SHA1(79191e776b6683b259cd1a80e9fb3183268bde56))
BY6803_ROMEND
#define input_ports_ladyluck input_ports_by6803
CORE_GAMEDEFNV(ladyluck,"Lady Luck",1986,"Bally",by_mBY6803_45S,0)

// Games below use Turbo Cheap Squalk Sound Hardware

/*--------------------------------
/ MotorDome (6803-0E14: 05/86)
/-------------------------------*/
INITGAME6803(motrdome,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(motrdome,"modm_u2.dat",CRC(820ca073) SHA1(0b50712f7d65f629af934deccc52d588f390a05b),
                           "modm_u3.dat",CRC(aae7c418) SHA1(9d3ea83ffff0b9696f5113043475c6e9b9a464ae))
BYTCS_SOUNDROM8(           "modm_u7.snd",CRC(29ce4679) SHA1(f17998198b542dd99a34abd678db7e031bde074b))
BY6803_ROMEND
#define input_ports_motrdome input_ports_by6803
CORE_GAMEDEFNV(motrdome,"MotorDome",1986,"Bally",by_mBY6803_TCSS,0)

/*------------------------------------
/ Karate Fight (6803-????: 06/86) - European version of Black Belt
/------------------------------------*/

/*------------------------------------
/ Black Belt (6803-0E52: 07/86)
/------------------------------------*/
INITGAME6803(blackblt,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(blackblt,"u2.cpu",     CRC(7c771910) SHA1(1df8ae478c3626a5200215bfca557ca42e064d2b),
                           "u3.cpu",     CRC(bad0f4c3) SHA1(5e5240fda9f7f7f15f1953f12b132ba1c4fc886e))
BYTCS_SOUNDROM8(           "blck_u7.snd",CRC(db8bce07) SHA1(6327cfbb2761f4d190e2852f3321cdd0cc1e46a8))
BY6803_ROMEND
#define input_ports_blackblt input_ports_by6803
CORE_GAMEDEFNV(blackblt,"Black Belt",1986,"Bally",by_mBY6803_TCSS,0)

// 1st Game to use Sounds Deluxe Sound Hardware

/*------------------------------------
/ Special Force (6803-0E47: 08/86)
/------------------------------------*/
INITGAME6803(specforc,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(specforc,"u2_revc.128",CRC(d042af04) SHA1(0a73ee6d3ce603899fd89de70f90e9efc58b8b42),
                           "u3_revc.128",CRC(d48a5eaf) SHA1(90a5d5e928abfec699bae9d0087e90316339058f))
BYSD_SOUNDROM0000(         "u12_snd.512",CRC(4f48a490) SHA1(6c9a594ecc68adf3b1eda315c4704e1d025a3442),
                           "u11_snd.512",CRC(b16eb713) SHA1(461e5ed82891d17849984137536bc6d1ab2907c2),
                           "u14_snd.512",CRC(6911fa51) SHA1(a75f75bb4493b0ea3a423bc033d49022228d79c1),
                           "u13_snd.512",CRC(3edda92d) SHA1(dbd95bb1c534779f56cc9e30efef159feaf22712))
BY6803_ROMEND
#define input_ports_specforc input_ports_by6803
CORE_GAMEDEFNV(specforc,"Special Force",1986,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Strange Science (6803-0E35: 10/86)
/------------------------------------*/
INITGAME6803(strngsci,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(strngsci, "cpu_u2.128",  CRC(2ffcf284) SHA1(27d66806708c983092bab4ed6965c2e91e69acdc),
                            "cpu_u3.128",  CRC(35257931) SHA1(d3d6b84e50677a4c5f9d5c13c9522ad6d3a1358d))
BYTCS_SOUNDROM8(            "sound_u7.256",CRC(bc33901e) SHA1(5231d8f01a107742acee2d13580a461063018a11))
BY6803_ROMEND
#define input_ports_strngsci input_ports_by6803
CORE_GAMEDEFNV(strngsci,"Strange Science",1986,"Bally",by_mBY6803_TCSS,0)

/*------------------------------------
/ City Slicker (6803-0E79: 03/87)
/------------------------------------*/
INITGAME6803(cityslck,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(cityslck, "u2.128",    CRC(94bcf162) SHA1(1d83592ad2441fc5e4c6fd3ab2373614dfe78b34),
                            "u3.128",    CRC(97cb2bca) SHA1(0cbd49bbce2ce26c720d8a52bd4d1256f0ac61b3))
BYTCS_SOUNDROM0(            "u7_snd.512",CRC(6941d68a) SHA1(28de4327f328d16ec4cab59642c185777535efb2))
BY6803_ROMEND
#define input_ports_cityslck input_ports_by6803
CORE_GAMEDEFNV(cityslck,"City Slicker",1987,"Bally",by_mBY6803_TCS2S,0)

/*------------------------------------
/ Hardbody (6803-0E94: 03/87)
/------------------------------------*/
INITGAME6803(hardbody,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYTCS, BY6803_DISPALPHA)
BY6803_ROMSTART44(hardbody,"cpu_u2.128",  CRC(c9248b47) SHA1(54239bd7d15574ebbb70ed306a804b7b32ed516a),
                           "cpu_u3.128",  CRC(31c255d0) SHA1(b6ffa2616ae9a4a121585cc402080ec6f26f8472))
BYTCS_SOUNDROM0(           "sound_u7.512",CRC(c96f91af) SHA1(9602a8991ca0cf9a7c68710f55c245d9c675b06f))
BY6803_ROMEND
#define input_ports_hardbody input_ports_by6803
CORE_GAMEDEFNV(hardbody,"Hardbody",1987,"Bally",by_mBY6803_TCS2S,0)

BY6803_ROMSTART44(hardbdyg,"hrdbdy-g.u2", CRC(fce357cc) SHA1(f7d13c12aabcb3c5bb5826b1911817bd359f1941),
                           "hrdbdy-g.u3", CRC(ccac74b5) SHA1(d55cfc8ee866a9af4567d56890f5a9ecb9c3c02f))
BYTCS_SOUNDROM0(           "sound_u7.512",CRC(c96f91af) SHA1(9602a8991ca0cf9a7c68710f55c245d9c675b06f))
BY6803_ROMEND
#define init_hardbdyg init_hardbody
#define input_ports_hardbdyg input_ports_hardbody
CORE_CLONEDEFNV(hardbdyg,hardbody,"Hardbody (German)",1987,"Bally",by_mBY6803_TCS2S,0)

// Games below use Sounds Deluxe Sound Hardware

/*--------------------------------
/ Party Animal (6803-0H01: 05/87)
/-------------------------------*/
INITGAME6803(prtyanim,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(prtyanim,"cpu_u2.128", CRC(abdc0b2d) SHA1(b93c7248ea83461101383023bd4e4a50292d8570),
                           "cpu_u3.128", CRC(e48b2d63) SHA1(190fc5a805bda9617c08a29c0bde4d94a77279e9))
BYSD_SOUNDROM0000(         "snd_u12.512",CRC(265a9494) SHA1(3b631f2b1c8c685aef32fb6c5289cd792711ff7e),
                           "snd_u11.512",CRC(20be998f) SHA1(7f98073d0f559e081b2d6dc8c1f3462e3fe9a713),
                           "snd_u14.512",CRC(639b3db1) SHA1(e07669c3186c963f2fea29bcf5675ac86eb07c86),
                           "snd_u13.512",CRC(b652597b) SHA1(8b4074a545d420319712a1fdd77a3bfb282ed9cd))
BY6803_ROMEND
#define input_ports_prtyanim input_ports_by6803
CORE_GAMEDEFNV(prtyanim,"Party Animal",1987,"Bally",by_mBY6803_SDS,0)

/*-----------------------------------------
/ Heavy Metal Meltdown (6803-0H03: 08/87)
/-----------------------------------------*/
//3 Different Sources claim that this games only uses U11&U12..
//Must be correct, as it DOES pass the start up test.
INITGAME6803(hvymetal,GEN_BY6803,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(hvymetal,"u2.rom", CRC(53466e4e) SHA1(af6d0e15821ff707f24bb99b8d9dfb9f929906db),
                           "u3.rom", CRC(0a08ae7e) SHA1(04f295fbe3a7bd7b929556338914c0ed94a77d62))
BYSD_SOUNDROM00xx(         "u12.rom",CRC(77933258) SHA1(42a01e97440dbb7d3da92dbfbad2516f4b553a5f),
                           "u11.rom",CRC(b7e4de7d) SHA1(bcc89e10c368cdbc5137d8f585e109c0be25522d))
BY6803_ROMEND
#define input_ports_hvymetal input_ports_by6803
CORE_GAMEDEFNV(hvymetal,"Heavy Metal Meltdown",1987,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Dungeons & Dragons (6803-0H06: 10/87)
/------------------------------------*/
INITGAME6803(dungdrag,GEN_BY6803,dispBy104,FLIP_SW(FLIP_L),4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(dungdrag,"cpu_u2.128", CRC(cefd4330) SHA1(0bffb2b73229e9908a018e06daeceb736896e5f0),
                           "cpu_u3.128", CRC(4bacc7f5) SHA1(71dd898924e0e968c4f3ba8a261e6b382d8ae0f1))
BYSD_SOUNDROM0000(         "snd_u12.512",CRC(dd95f851) SHA1(6fa46b512bced0d1862b2621e195ef0dfd24f928),
                           "snd_u11.512",CRC(dcd461b3) SHA1(834000cfb6c6acf5c296db58971251819971f4de),
                           "snd_u14.512",CRC(dd9e61eb) SHA1(fd1ec58f5708d5abf3d7424954ce054454514283),
                           "snd_u13.512",CRC(1e2d9211) SHA1(f5fcf1c07f01e7f1a7abff9ac3c481b84471d3a6))
BY6803_ROMEND
#define input_ports_dungdrag input_ports_by6803
CORE_GAMEDEFNV(dungdrag,"Dungeons & Dragons",1987,"Bally",by_mBY6803_SDS,0)

// Games below don't use a keypad anymore
/*------------------------------------------------
/ Escape from the Lost World (6803-0H05: 12/87)
/-----------------------------------------------*/
INITGAME6803(esclwrld,GEN_BY6803A,dispBy104,FLIP6803,4,SNDBRD_BYSD, BY6803_DISPALPHA)
BY6803_ROMSTART44(esclwrld,"u2.128", CRC(b11a97ea) SHA1(29339785a67ed7dc9eb39ddc7bb7e6baaf731210),
                           "u3.128", CRC(5385a562) SHA1(a6c39532d01db556e4bdf90020a9d9905238e8ef))
BYSD_SOUNDROM0000(         "u12.512",CRC(0c003473) SHA1(8ada2aa546a6499c5e2b5eb45a1975b8285d25f9),
                           "u11.512",CRC(360f6658) SHA1(c0346952dcd33bbcf4c43c51cde5433a099a7a5d),
                           "u14.512",CRC(0b92afff) SHA1(78f51989e74ced9e0a81c4e18d5abad71de01faf),
                           "u13.512",CRC(b056842e) SHA1(7c67e5d69235a784b9c38cb31302d206278a3814))
BY6803_ROMEND
#define input_ports_esclwrld input_ports_by6803a
CORE_GAMEDEFNV(esclwrld,"Escape from the Lost World",1987,"Bally",by_mBY6803_SDS,0)

BY6803_ROMSTART44(esclwrlg,"u2_ger.128", CRC(0a6ab137) SHA1(0627b7c67d13f305f2287f3cfa023c8dd7721250),
                           "u3_ger.128", CRC(26d8bfbb) SHA1(3b81fb0e736d14004bbbbb2edd682fdfc1b2c832))
BYSD_SOUNDROM0000(         "u12.512",CRC(0c003473) SHA1(8ada2aa546a6499c5e2b5eb45a1975b8285d25f9),
                           "u11.512",CRC(360f6658) SHA1(c0346952dcd33bbcf4c43c51cde5433a099a7a5d),
                           "u14.512",CRC(0b92afff) SHA1(78f51989e74ced9e0a81c4e18d5abad71de01faf),
                           "u13.512",CRC(b056842e) SHA1(7c67e5d69235a784b9c38cb31302d206278a3814))
BY6803_ROMEND
#define init_esclwrlg init_esclwrld
#define input_ports_esclwrlg input_ports_esclwrld
CORE_CLONEDEFNV(esclwrlg,esclwrld,"Escape from the Lost World (German)",1987,"Bally",by_mBY6803_SDS,0)

/*------------------------------------
/ Blackwater 100 (6803-0H07: 03/88)
/------------------------------------*/
static core_tGameData black100GameData = {GEN_BY6803A,dispBy104,{FLIP6803,0,4,0,SNDBRD_BYSD, BY6803_DISPALPHA, 1}};
static void init_black100(void) { core_gameData = &black100GameData; }
BY6803_ROMSTART44(black100, "u2.cpu", CRC(411fa773) SHA1(9756c7eee0f78792823a0b0379d2baac28cb03e8),
                            "u3.cpu", CRC(d6f6f890) SHA1(8fe4dae471f4c89f2fd72c6e647ead5206881c63))
BYSD_SOUNDROM0000(          "u12.bin",CRC(a0ecb282) SHA1(4655e0b85f7e8af8dda853279696718d3adbf7e3),
                            "u11.bin",CRC(3f117ba3) SHA1(b4cded8fdd90ca030c6ff12c817701402c94baba),
                            "u14.bin",CRC(b45bf5c4) SHA1(396ddf346e8ebd8cb91777521d93564d029f40b1),
                            "u13.bin",CRC(f5890443) SHA1(77cd18cf5541ae9f7e2dd1c060a9bf29b242d05d))
BY6803_ROMEND
#define input_ports_black100 input_ports_by6803a
CORE_GAMEDEFNV(black100,"Blackwater 100",1988,"Bally",by_mBY6803_SDS,0)

BY6803_ROMSTART44(black10s, "sb2.cpu", CRC(b6fdbb0f) SHA1(5b36a725db3a1e023bbb54b8f85300fe99174b6e),
                            "sb3.cpu", CRC(ae9930b8) SHA1(1b6c63ce98939ecded300639d872df62548157a4))
BYSD_SOUNDROM0000(          "u12.bin",CRC(a0ecb282) SHA1(4655e0b85f7e8af8dda853279696718d3adbf7e3),
                            "u11.bin",CRC(3f117ba3) SHA1(b4cded8fdd90ca030c6ff12c817701402c94baba),
                            "u14.bin",CRC(b45bf5c4) SHA1(396ddf346e8ebd8cb91777521d93564d029f40b1),
                            "u13.bin",CRC(f5890443) SHA1(77cd18cf5541ae9f7e2dd1c060a9bf29b242d05d))
BY6803_ROMEND
#define init_black10s init_black100
#define input_ports_black10s input_ports_by6803a
CORE_CLONEDEFNV(black10s,black100,"Blackwater 100 (Single Ball Play)",1988,"Bally",by_mBY6803_SDS,0)

//Games below use 6803 MPU & Williams System 11C Sound Hardware
/*-------------------------------------------------------------
/ Truck Stop (6803-2001: 12/88) - These are ProtoType ROMS?
/-------------------------------------------------------------*/
INITGAME6803(trucksp3,GEN_BY6803A,dispBy104,FLIP6803,4,SNDBRD_S11CS, BY6803_DISPALPHA)
BY6803_ROMSTART44(trucksp3,"u2_p3.128",   CRC(79b2a5b1) SHA1(d3de91bfadc9684302b2367cfcb30ed0d6faa020),
                           "u3_p3.128",   CRC(2993373c) SHA1(26490f1dd8a5329a88a2ceb1e6044711a29f1445))
S11CS_SOUNDROM888(         "u4sndp1.256", CRC(120a386f) SHA1(51b3b45eb7ea63758b21aad404ba12a9607fec44),
                           "u19sndp1.256",CRC(5cd43dda) SHA1(23dd8a52ea1340fc239a246af0d94da905464efb),
                           "u20sndp1.256",CRC(93ac5c33) SHA1(f6dc84eca4678188a58ba3c8ef18975164dd29b0))
BY6803_ROMEND
#define input_ports_trucksp3 input_ports_by6803a
CORE_GAMEDEFNV(trucksp3,"Truck Stop (P-3)",1988,"Bally",by_mBY6803_S11CS,0)

BY6803_ROMSTART44(trucksp2,"u2_p2.128",   CRC(3c397dec) SHA1(2fc86ad39c935ce8615eafd67e571ac94c938cd7),
                           "u3_p2.128",   CRC(d7ac519a) SHA1(612bf9fee0d54e8b1215508bd6c1ea61dcb99951))
S11CS_SOUNDROM888(         "u4sndp1.256", CRC(120a386f) SHA1(51b3b45eb7ea63758b21aad404ba12a9607fec44),
                           "u19sndp1.256",CRC(5cd43dda) SHA1(23dd8a52ea1340fc239a246af0d94da905464efb),
                           "u20sndp1.256",CRC(93ac5c33) SHA1(f6dc84eca4678188a58ba3c8ef18975164dd29b0))
BY6803_ROMEND
#define init_trucksp2 init_trucksp3
#define input_ports_trucksp2 input_ports_trucksp3
CORE_CLONEDEFNV(trucksp2,trucksp3,"Truck Stop (P-2)",1988,"Bally",by_mBY6803_S11CS,0)

/*-----------------------------------------------------------
/ Atlantis (6803-2006: 03/89)
/-----------------------------------------------------------*/
INITGAME6803(atlantis,GEN_BY6803A,dispBy104,FLIP6803,4,SNDBRD_S11CS, BY6803_DISPALPHA)
BY6803_ROMSTART44(atlantis, "u26_cpu.rom",CRC(b98491e1) SHA1(b867e2b24e93c4ee19169fe93c0ebfe0c1e2fc25),
                            "u27_cpu.rom",CRC(8ea2b4db) SHA1(df55a9fb70d1cabad51dc2b089af7904a823e1d8))
S11CS_SOUNDROM888(          "u4_snd.rom", CRC(6a48b588) SHA1(c58dbfd920c279d7b9d2de8558d73c687b29ce9c),
                            "u19_snd.rom",CRC(1387467c) SHA1(8b3dd6c2fc94cfebc1879795532c651cda202846),
                            "u20_snd.rom",CRC(d5a6a773) SHA1(30807e03655d2249c801007350bfb228a2e8a0a4))
BY6803_ROMEND
#define input_ports_atlantis input_ports_by6803a
CORE_GAMEDEFNV(atlantis,"Atlantis",1989,"Bally",by_mBY6803_S11CS,0)
