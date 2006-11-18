#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s7.h"

const core_tLCDLayout s7_dispS7[] = {
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_BALLS(0,8,CORE_SEG7S),DISP_SEG_CREDIT(20,28,CORE_SEG7S),{0}
};
#define INITGAME(name, disp) \
static core_tGameData name##GameData = { GEN_S7, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S7, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S7_INPUT_PORTS_START(s7, 1) S7_INPUT_PORTS_END

/*-------------------------------
/ Firepower - Sys.7 7-Digit conversion
/------------------------------*/
static const struct core_dispLayout fp_7digit_disp[] = {
  // Player 1            Player 2
  {0, 0, 0,7,CORE_SEG7}, {0,18, 7,7,CORE_SEG7},
  // Player 3            Player 4
  {2, 0,20,7,CORE_SEG7}, {2,18,27,7,CORE_SEG7},
  // Left Side           Right Side
  {4, 9,34,2,CORE_SEG7}, {4,14,14,2,CORE_SEG7}, {0}
};
INITGAMEFULL(frpwr,fp_7digit_disp,0,45,26,25,27,28,42,12)
S7_ROMSTART808x(frpwr,b7,"f7ic14pr.716",CRC(c7a60d8a) SHA1(1921f2ddec96963d7f990cd6b1cdd6b4a6e42810),
                         "f7ic17gr.532",CRC(a042201a) SHA1(1421e1dbbcb322d83838d68ac0909f4804249815),
                         "f7ic20ga.716",CRC(584d7b45) SHA1(9ea01eda36dab77dec78c2d1207983c505e406b2))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S7_ROMEND
#define input_ports_frpwr input_ports_s7
CORE_CLONEDEF(frpwr,b7,l6,"Firepower (Sys.7 7-digit conversion)",2003,"Williams / Oliver",s7_mS7S6,0)
/*-------------------------------
/ Firepower - Sys.7 7-Digit conversion, , rev. 38
/------------------------------*/
S7_ROMSTART8088(frpwr,c7,"f7ic14pr.716",CRC(4cd22956) SHA1(86380754b9cbb0a81b4fa4d26cff71b9a70d2a96),
                         "f7ic17gr.532",CRC(3a1b7cc7) SHA1(1f32ef6d66040a53b04a0cddb1ffbf197a29c940),
                         "f7ic20ga.716",CRC(5ed61e41) SHA1(b73a05b336f7bb8ee528205612bd0744e86498f5),
                         "f7ic26.716"  ,CRC(aaf82d89) SHA1(c481a49c7e7d4734c0eeab31b9970ca62a3995f0))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS0000(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(35db11d2) SHA1(001df0d5245230b960ff69c30ee2b305b3a5e4b4),
                         "v_ic4.532",   CRC(8d4ff909) SHA1(b82666fe96bdf174bc4f347d7139da9ab7dadee1))


S7_ROMEND
#define input_ports_frpwr input_ports_s7
CORE_CLONEDEF(frpwr,c7,l6,"Firepower (Sys.7 7-digit conversion, new version)",2005,"Williams / Oliver",s7_mS7S6,0)

/*-----------------------------------
/ Cosmic Gunfight - Sys.7 (Game #502)
/-----------------------------------*/
INITGAMEFULL(csmic,s7_dispS7,52,49,36,37,21,22,24,23)
S7_ROMSTART8088(csmic,l1, "ic14.716",   CRC(ac66c0dc) SHA1(9e2ac0e956008c2d56ffd564c983e127bc4af7ae),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(a259eba0) SHA1(0c5acae3beacb8abb0160dd8a580d3514ca557fe))
S67S_SOUNDROMS8(          "sound12.716",CRC(af41737b) SHA1(8be4e7cebe5a821e859550c0350f0fc9cc00b2a9))
S7_ROMEND
#define input_ports_csmic input_ports_s7
CORE_GAMEDEF(csmic,l1,"Cosmic Gunfight (L-1)",1980,"Williams",s7_mS7S,0)


/*--------------------------------
/ Jungle Lord - Sys.7 (Game #503)
/--------------------------------*/
INITGAMEFULL(jngld,s7_dispS7,0,0,12,28,40,0,0,0)
S7_ROMSTART8088(jngld,l2, "ic14.716",   CRC(6e5a6374) SHA1(738ecef807de9fee6fd1e832b35511c11173914c),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(4714b1f1) SHA1(01f8593a926df69fb8ae79260f11c5f6b868cd51))
S67S_SOUNDROMS8(          "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "speech7.532",CRC(83ffb695) SHA1(f9151bdfdefd5c178ca7eb5122f62b700d64f41a),
                          "speech5.532",CRC(754bd474) SHA1(c05f48bb07085683de469603880eafd28dffd9f5),
                          "speech6.532",CRC(f2ac6a52) SHA1(5b3e743eac382d571fd049f92ea9955342b9ffa0))
S7_ROMEND
#define input_ports_jngld input_ports_s7
CORE_GAMEDEF(jngld,l2,"Jungle Lord (L-2)",1981,"Williams",s7_mS7S,0)

/*--------------------------------
/ Pharaoh - Sys.7 (Game #504)
/--------------------------------*/
INITGAMEFULL(pharo,s7_dispS7,0,0,20,24,44,0,0,0)
S7_ROMSTART8088(pharo,l2, "ic14.716",   CRC(cef00088) SHA1(e0c6b776eddc060c42a483de6cc96a1c9f2afcf7),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(2afbcd1f) SHA1(98bb3a74548b7d9c5d7b8432369658ed32e8be07))
S67S_SOUNDROMS8(          "sound12.716",CRC(b0e3a04b) SHA1(eac54376fe77acf46e485ab561a01220910c1fd6))
S67S_SPEECHROMS0000(      "speech7.532",CRC(e087f8a1) SHA1(49c2ad60d82d02f0529329f7cb4b57339d6546c6),
                          "speech5.532",CRC(d72863dc) SHA1(e24ad970ed202165230fab999be42bea0f861fdd),
                          "speech6.532",CRC(d29830bd) SHA1(88f6c508f2a7000bbf6c9c26e1029cf9a241d5ca),
                          "speech4.532",CRC(9ecc23fd) SHA1(bf5947d186141504fd182065533d4efbfd27441d))
S7_ROMEND
#define input_ports_pharo input_ports_s7
CORE_GAMEDEF(pharo,l2,"Pharaoh (L-2)",1981,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Solar Fire - Sys.7 (Game #507)
/-----------------------------------*/
INITGAMEFULL(solar,s7_dispS7,0,0,11,12,0,0,0,0)
S7_ROMSTART8088(solar,l2, "ic14.716",   CRC(cec19a55) SHA1(a1c0f7cc36e5fc7be4e8bcc80896f77eb4c23b1a),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(b667ee32) SHA1(bb4b5270d9cd36207b68e8c6883538d08aae1778))
S67S_SOUNDROMS8(          "sound12.716",CRC(05a2230c) SHA1(c57cd7628310aa8f68ca24217aad1ead066a1a82))
S7_ROMEND
#define input_ports_solar input_ports_s7
CORE_GAMEDEF(solar,l2,"Solar Fire (L-2)",1981,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Thunderball - Sys.7 (Game #508)
/-----------------------------------*/
INITGAMEFULL(thund,s7_dispS7,0,0,0,0,0,0,0,0)
S7_ROMSTART000x(thund,p1, "ic14.532",   CRC(1cd34f1f) SHA1(3f5b5a319570c26a3d34d640fef2ac6c04b83b70),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.532",   CRC(aa3f07dc) SHA1(f31662972046f9a874380a8dcd1bc9259de5f6ba))
S67S_SOUNDROMS0(          "sound12.532",CRC(cc70af52) SHA1(d9c2840acdcd69aab39fc647dd4819eccc06af33))
S67S_SPEECHROMS0000(      "speech7.532",CRC(33e1b041) SHA1(f50c0311bde69fa6e8071e297a81cc3ef3dcf44f),
                          "speech5.532",CRC(11780c80) SHA1(bcc5efcd69b4f776feef32484a872863847d64cd),
                          "speech6.532",CRC(ab688698) SHA1(e0cbac44a6fe30a49da478c32500a0b43903cc2b),
                          "speech4.532",CRC(2a4d6f4b) SHA1(e6f8a1a6e6abc81f980a4938d98abb250e8e1e3b))
S7_ROMEND
#define input_ports_thund input_ports_s7
CORE_GAMEDEF(thund,p1,"Thunderball (P-1)",1982,"Williams",s7_mS7SND,0)

/*-------------------------------
/ Hyperball - Sys.7 - (Game #509)
/-------------------------------*/
static const core_tLCDLayout dispHypbl[] = {
  { 0, 0, 1, 7,CORE_SEG87 }, { 0,16, 9, 7,CORE_SEG87 },
  { 2,10,20, 1,CORE_SEG87 }, { 2,12,28, 1,CORE_SEG87 },
  { 2,16, 0, 1,CORE_SEG87 }, { 2,18, 8, 1,CORE_SEG87 },
  { 4, 3,43, 5,CORE_SEG16 }, { 4,13,49, 7,CORE_SEG16 }, {0}
};
static core_tGameData hypblGameData = { GEN_S7, dispHypbl, {FLIP_SWNO(33,34), 0, 4}, NULL, {"", {0,0,0,0xf8,0x0f}, {0} } };
static void init_hypbl(void) { core_gameData = &hypblGameData; }
#define input_ports_hypbl input_ports_s7

S7_ROMSTART000x(hypbl,l4, "ic14.532",    CRC(8090fe71) SHA1(0f1f40c0ee8da5b2fd51efeb8be7c20d6465239e),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20.532",    CRC(d13962e8) SHA1(e23310be100060c9803682680066b965aa5efb16))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_GAMEDEF(hypbl,l4,"HyperBall (L-4)",1981,"Williams",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l5, "ic14.532",    CRC(8090fe71) SHA1(0f1f40c0ee8da5b2fd51efeb8be7c20d6465239e),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20_fix.532",    CRC(48958d77) SHA1(ddfec991ef99606b866ced08b59e205a0b2cadd1))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l5,l4,"HyperBall (L-5)",1998,"Jess M. Askey (High score bootleg)",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l6, "ic14.532",    CRC(8090fe71) SHA1(0f1f40c0ee8da5b2fd51efeb8be7c20d6465239e),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20_l6.532",    CRC(e40c27cd) SHA1(337bd8450be305796e52c1b7d3ae3cc8cc972525))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l6,l4,"HyperBall (L-6)",2006,"Jess M. Askey (High score bootleg)",s7_mS7S,0)

/*----------------------------
/ Barracora- Sys.7 (Game #510)
/----------------------------*/
INITGAMEFULL(barra,s7_dispS7,34,35,0,20,21,22,29,30)
S7_ROMSTART8088(barra,l1, "ic14.716",   CRC(522e944e) SHA1(0fa17b7912f8129e40de5fed8c3ccccc0a2a9366),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(2a0e0171) SHA1(f1f2d4c1baed698d3b7cf2e88a2c28056e859920))
S67S_SOUNDROMS8(          "sound4.716", CRC(67ea12e7) SHA1(f81e97183442736d5766a7e5e074bc6539e8ced0))
S7_ROMEND
#define input_ports_barra input_ports_s7
CORE_GAMEDEF(barra,l1,"Barracora (L-1)",1981,"Williams",s7_mS7S,0)

/*----------------------------
/ Varkon- Sys.7 (Game #512)
/----------------------------*/
INITGAMEFULL(vrkon,s7_dispS7,40,39,33,41,0,0,0,0)
S7_ROMSTART8088(vrkon,l1, "ic14.716",   CRC(3baba324) SHA1(522654e0d81458d8b31150dcb0cb53c29b334358),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(df20330c) SHA1(22157c6480ad38b9c53c390f5e7bfa63a8abd0e8))
S67S_SOUNDROMS8(          "sound12.716",CRC(d13db2bb) SHA1(862546bbdd1476906948f7324b7434c29df79baa))
S7_ROMEND
#define input_ports_vrkon input_ports_s7
CORE_GAMEDEF(vrkon,l1,"Varkon (L-1)",1982,"Williams",s7_mS7S,0)

/*----------------------------------
/Time Fantasy- Sys.7 (Game #515)
/----------------------------------*/
// MOVED TO tmfnt.c

/*----------------------------
/ Warlok- Sys.7 (Game #516)
/----------------------------*/
INITGAMEFULL(wrlok,s7_dispS7,0,36,13,14,15,20,21,0)
S7_ROMSTART8088(wrlok,l3, "ic14.716",   CRC(291be241) SHA1(77fffa878f760583ef152a7939867621a61d58dc),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(44f8b507) SHA1(cdd8455c1e34584e8f1b75d430b8b37d4dd7dff0))
S67S_SOUNDROMS8(          "sound12.716",CRC(5d8e46d6) SHA1(68f8760ad85b8ada81f6ed00eadb9daf37191c53))
S7_ROMEND
#define input_ports_wrlok input_ports_s7
CORE_GAMEDEF(wrlok,l3,"Warlok (L-3)",1982,"Williams",s7_mS7S,0)

/*----------------------------
/ Defender - Sys.7 (Game #517)
/----------------------------*/
// Multiplex solenoid requires custom solenoid handler.
extern int dfndrCustSol(int);
static core_tGameData dfndrGameData = {
 GEN_S7, s7_dispS7, {FLIP_SWNO(0,59),0,0,10,0,0,0,0x3FF,dfndrCustSol},
 NULL,{{0}},{11,{55,56,57,58,0,0}}
};
static void init_dfndr(void) { core_gameData = &dfndrGameData; }

S7_ROMSTART000x(dfndr,l4, "ic14.532",   CRC(959ec419) SHA1(f400d3a1feba0e149d24f4e1a8d240fe900b3f0b),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.532",   CRC(e99e64a2) SHA1(a6cde9cb771063778cae706c740b73ce9bce9aa5))
S67S_SOUNDROMS8(          "sound12.716",CRC(cabaec58) SHA1(9605a1c299ed109a4ebcfa7ed6985ecc815c9e0c))
S7_ROMEND
#define input_ports_dfndr input_ports_s7
CORE_GAMEDEF(dfndr,l4,"Defender (L-4)",1982,"Williams",s7_mS7S,0)

/*---------------------------
/ Joust - Sys.7 (Game #519)
/--------------------------*/
INITGAMEFULL(jst,s7_dispS7,0,0,0,0,29,53,30,54) // 4 flippers,buttons need special
S7_ROMSTART8088(jst,l2, "ic14.716",   CRC(c4cae4bf) SHA1(ff6e48364561402b16e40a41fa1b89e7723dd38a),
                        "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                        "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                        "ic26.716",   CRC(63eea5d8) SHA1(55c26ee94809f087bd886575a5e47efc93160190))
S67S_SOUNDROMS0(        "sound12.532",CRC(3bbc90bf) SHA1(82154e719ceca5c72d1ab034bc4ff5e3ebb36832))
S7_ROMEND
#define input_ports_jst input_ports_s7
CORE_GAMEDEF(jst,l2,"Joust (L-2)",1983,"Williams",s7_mS7S,0)


/*---------------------------
/ Laser Cue - Sys.7 (Game #520)
/--------------------------*/
INITGAMEFULL(lsrcu,s7_dispS7,0,43,41,19,18,17,0,0)
S7_ROMSTART8088(lsrcu,l2, "ic14.716",   CRC(39fc350d) SHA1(46e95f4016907c21c69472e6ef4a68a9adc3be77),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(db4a09e7) SHA1(5ea454c852303e12cc606c2c1e403b72e0a99f25))
S67S_SOUNDROMS8(          "sound12.716",CRC(1888c635) SHA1(5dcdaee437a69c6027c24310f0cd2cae4e89fa05))
S7_ROMEND
#define input_ports_lsrcu input_ports_s7
CORE_GAMEDEF(lsrcu,l2,"Laser Cue (L-2)",1983,"Williams",s7_mS7S,0)

/*--------------------------------
/ Firepower II- Sys.7 (Game #521)
/-------------------------------*/
INITGAMEFULL(fpwr2,s7_dispS7,0,13,47,48,41,42,43,44)
S7_ROMSTART8088(fpwr2,l2, "ic14.716",   CRC(a29688dd) SHA1(83815154bbaf51dd789112664d772a876efee3da),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(1068939d) SHA1(f15c3a149bafee6d74e359399de88fd122b93441))
S67S_SOUNDROMS8(          "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S7_ROMEND
#define input_ports_fpwr2 input_ports_s7
CORE_GAMEDEF(fpwr2,l2,"Firepower II (L-2)",1983,"Williams",s7_mS7S,0)

/*-----------------------------
/ Rat Race - Sys.7 (Game #5??)
/-----------------------------*/
static core_tGameData ratrcGameData = { GEN_S7, s7_dispS7, {FLIP_SWNO(0,0)}, NULL, {"",{0,0,0xff,0xff,0,0xf0,0xff,0x3f}},{0,{0,0,0,0,0,0}}};
static void init_ratrc(void) { core_gameData = &ratrcGameData; }
S7_ROMSTART000x(ratrc,l1,"ic14.532",   CRC(c6f4bcf4) SHA1(d71c86299139abe3dd376a324315a039be82875c),
                         "ic17.532",   CRC(0800c214) SHA1(3343c07fd550bb0759032628e01bb750135dab15),
                         "ic20.532",   CRC(0c5c7c09) SHA1(c93b39ba1460feee5850fcd3ca7cacb72c4c8ff3))
S9RR_SOUNDROM(           "b486.bin",   CRC(c54b9402) SHA1(c56fc5f105fc2c1166e3b22bb09b72af79e0aec1))
S7_ROMEND
#define input_ports_ratrc input_ports_s7
CORE_GAMEDEF(ratrc,l1,"Rat Race (L-1)",1983,"Williams",s7_mS7RR,0)

/*-----------------------------
/ Star Light - Sys.7 (Game #530)
/-----------------------------*/
INITGAMEFULL(strlt,s7_dispS7,0,0,50,51,41,42,43,0)
S7_ROMSTART000x(strlt,l1,"ic14.532",   CRC(292f1c4a) SHA1(0b5d50331364655672be16236d38d72b28f6dec2),
                         "ic17.532",   CRC(a43d8518) SHA1(fb2289bb7380838d0d817e78c39e5bcb2709373f),
                         "ic20.532",   CRC(66876b56) SHA1(6fab43fbb67c7b602ca595c20a41fc1553afdb65))
S67S_SOUNDROMS8(         "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S7_ROMEND
#define input_ports_strlt input_ports_s7
CORE_GAMEDEF(strlt,l1,"Star Light (L-1)",1984,"Williams",s7_mS7S,0)

