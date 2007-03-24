#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s6.h"

#define INITGAME(name, disp) \
static core_tGameData name##GameData = { GEN_S6, disp }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S6, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S6_INPUT_PORTS_START(s6, 1) S6_INPUT_PORTS_END

/*--------------------------------
/ Laser Ball - Sys.6 (Game #493)
/-------------------------------*/
INITGAMEFULL(lzbal,s6_6digit_disp,0,0,45,46,47,44,0,13)
S6_ROMSTART(lzbal,l2, "gamerom.716", CRC(9c5ffe2f) SHA1(f0db627abaeb8c023a3ccc75262e236c998a5d6f),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(	  "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S6_ROMEND
#define input_ports_lzbal input_ports_s6
CORE_GAMEDEF(lzbal,l2,"Laser Ball (L-2)",1979,"Williams",s6_mS6S,0)

S6_ROMSTART(lzbal,t2, "gamerom.716", CRC(9c5ffe2f) SHA1(f0db627abaeb8c023a3ccc75262e236c998a5d6f),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(	  "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S6_ROMEND
#define input_ports_lzbal_t2 input_ports_s6
CORE_CLONEDEF(lzbal,t2,l2,"Laser Ball (T-2) Ted Estes",1980,"Williams",s6_mS6S,0)

/*-----------------------------
/ Scorpion - Sys.6 (Game #494)
/----------------------------*/
INITGAMEFULL(scrpn,s6_6digit_disp,0,17,23,24,22,0,39,40)
S6_ROMSTART(scrpn,l1,  "gamerom.716", CRC(881109a9) SHA1(53d4275c76b47b68a74209fe660d943a51e90eca),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(       "sound1.716",  CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S6_ROMEND
#define input_ports_scrpn input_ports_s6
CORE_GAMEDEF(scrpn,l1,"Scorpion (L-1)",1980,"Williams",s6_mS6S,0)

S6_ROMSTART(scrpn,t1,  "gamerom.716", CRC(881109a9) SHA1(53d4275c76b47b68a74209fe660d943a51e90eca),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(       "sound1.716",  CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S6_ROMEND
#define input_ports_scrpn_t1 input_ports_s6
CORE_CLONEDEF(scrpn,t1,l1,"Scorpion (T-1) Ted Estes",1980,"Williams",s6_mS6S,0)

/*----------------------------
/ Blackout - Sys.6 (Game #495)
/---------------------------*/
INITGAMEFULL(blkou,s6_6digit_disp,0,46,22,21,23,44,43,0)
S6_ROMSTART(blkou,l1,  "gamerom.716", CRC(4b407ae2) SHA1(46a2afcfc2d969c5acae18b57a678265257a6102),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(       "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S67S_SPEECHROMS000x(   "v_ic7.532" ,  CRC(87864071) SHA1(d03c71efc0431f30a07c8194c0614c96fb683710),
                       "v_ic5.532" ,  CRC(046a96d8) SHA1(879127a88b3640bbb202c64cbf8678869c964177),
                       "v_ic6.532" ,  CRC(0104e5c4) SHA1(c073cb4bdea189085ae074e9c16872752b6ffba0))
S6_ROMEND
#define input_ports_blkou input_ports_s6
CORE_GAMEDEF(blkou,l1,"Blackout (L-1)",1979,"Williams",s6_mS6S,0)

S6_ROMSTART(blkou,t1,  "gamerom.716", CRC(4b407ae2) SHA1(46a2afcfc2d969c5acae18b57a678265257a6102),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(       "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S67S_SPEECHROMS000x(   "v_ic7.532" ,  CRC(87864071) SHA1(d03c71efc0431f30a07c8194c0614c96fb683710),
                       "v_ic5.532" ,  CRC(046a96d8) SHA1(879127a88b3640bbb202c64cbf8678869c964177),
                       "v_ic6.532" ,  CRC(0104e5c4) SHA1(c073cb4bdea189085ae074e9c16872752b6ffba0))
S6_ROMEND
#define input_ports_blkou_t1 input_ports_s6
CORE_CLONEDEF(blkou,t1,l1,"Blackout (T-1) Ted Estes",1979,"Williams",s6_mS6S,0)

/*--------------------------
/ Gorgar - Sys.6 (Game #496)
/-------------------------*/
INITGAMEFULL(grgar,s6_6digit_disp,0,0,38,37,39,12,36,0)
S6_ROMSTART(grgar,l1,  "gamerom.716", CRC(1c6f3e48) SHA1(ba5536e6fbcaf3709277fe27827d7f75c1889ba3),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(       "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S67S_SPEECHROMS000x(   "v_ic7.532" ,  CRC(0b1879e3) SHA1(2c34a815f598b4413e9229e8eb1322ec9e7cc9d6),
                       "v_ic5.532" ,  CRC(0ceaef37) SHA1(33b5f5286b8588162d56dbc5c9a8ccb70d3b9090),
                       "v_ic6.532" ,  CRC(218290b9) SHA1(6afeff1413895489e92a4bb1c05f6de5773dbb6a))
S6_ROMEND
#define input_ports_grgar input_ports_s6
CORE_GAMEDEF(grgar,l1,"Gorgar (L-1)",1979,"Williams",s6_mS6S,0)

S6_ROMSTART(grgar,t1,  "gamerom.716", CRC(1c6f3e48) SHA1(ba5536e6fbcaf3709277fe27827d7f75c1889ba3),
                       "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                       "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(       "sound2.716",  CRC(c9103a68) SHA1(cc77af54fdb192f0b334d9d1028210618c3f1d95))
S67S_SPEECHROMS000x(   "v_ic7.532" ,  CRC(0b1879e3) SHA1(2c34a815f598b4413e9229e8eb1322ec9e7cc9d6),
                       "v_ic5.532" ,  CRC(0ceaef37) SHA1(33b5f5286b8588162d56dbc5c9a8ccb70d3b9090),
                       "v_ic6.532" ,  CRC(218290b9) SHA1(6afeff1413895489e92a4bb1c05f6de5773dbb6a))
S6_ROMEND
#define input_ports_grgar_t1 input_ports_s6
CORE_CLONEDEF(grgar,t1,l1,"Gorgar (T-1) Ted Estes",1979,"Williams",s6_mS6S,0)

/*-------------------------------
/ Firepower - Sys.6 (Game #497)
/------------------------------*/
INITGAMEFULL(frpwr_l6,s6_6digit_disp,0,45,26,25,27,28,42,12)
S6_ROMSTARTPROM(frpwr_l6, "gamerom.716", CRC(fdd3b983) SHA1(fb5d1eb01589311cf4b2ef16e25db03d40bca2f7),
                          "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                          "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2),
                          "prom1_6.474", CRC(af6eb0b9) SHA1(28f8366737e09ffd60cb5ea55a5734143cdb9663),
                          "prom2.474",   CRC(f75ade1a) SHA1(a5572c5c721dbcb82988b709f4ef2119118e37c2),
                          "prom3.474",   CRC(242ec687) SHA1(c3366c898a66c78034687e6a6000193d52be4141))
S67S_SOUNDROMS8(          "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                          "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                          "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S6_ROMEND
#define input_ports_frpwr_l6 input_ports_s6
CORE_GAMEDEFNV(frpwr_l6,"Firepower (L-6)",1980,"Williams",s6_mS6S,0)

INITGAMEFULL(frpwr_t6,s6_6digit_disp,0,45,26,25,27,28,42,12)
S6_ROMSTARTPROM(frpwr_t6, "gamerom.716", CRC(fdd3b983) SHA1(fb5d1eb01589311cf4b2ef16e25db03d40bca2f7),
                          "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
		                  "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98),
                          "prom1_6.474", CRC(af6eb0b9) SHA1(28f8366737e09ffd60cb5ea55a5734143cdb9663),
                          "prom2.474",   CRC(f75ade1a) SHA1(a5572c5c721dbcb82988b709f4ef2119118e37c2),
                          "prom3.474",   CRC(242ec687) SHA1(c3366c898a66c78034687e6a6000193d52be4141))
S67S_SOUNDROMS8(          "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                          "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                          "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S6_ROMEND
#define input_ports_frpwr_t6 input_ports_frpwr_l6
CORE_CLONEDEFNV(frpwr_t6,frpwr_l6,"Firepower (T-6) Ted Estes",1980,"Williams",s6_mS6S,0)

INITGAMEFULL(frpwr_l2,s6_6digit_disp,0,45,26,25,27,28,42,12)
S6_ROMSTARTPROM(frpwr_l2, "gamerom.716", CRC(fdd3b983) SHA1(fb5d1eb01589311cf4b2ef16e25db03d40bca2f7),
                          "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                          "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2),
                          "prom1.474",   CRC(fbb7299f) SHA1(0ae9dbdc6ed8315596bf755ece34691671dc8d44),
                          "prom2.474",   CRC(f75ade1a) SHA1(a5572c5c721dbcb82988b709f4ef2119118e37c2),
                          "prom3.474",   CRC(242ec687) SHA1(c3366c898a66c78034687e6a6000193d52be4141))
S67S_SOUNDROMS8(          "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                          "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                          "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S6_ROMEND
#define input_ports_frpwr_l2 input_ports_frpwr_l6
CORE_CLONEDEFNV(frpwr_l2,frpwr_l6,"Firepower (L-2)",1980,"Williams",s6_mS6S,0)

static const struct core_dispLayout fp_7digit_disp[] = {
  // Player 1            Player 2
  {0, 0, 0,7,CORE_SEG7}, {0,18, 7,7,CORE_SEG7},
  // Player 3            Player 4
  {2, 0,20,7,CORE_SEG7}, {2,18,27,7,CORE_SEG7},
  // Left Side           Right Side
  {4, 9,34,2,CORE_SEG7}, {4,14,14,2,CORE_SEG7}, {0}
};
INITGAMEFULL(frpwr_b6,fp_7digit_disp,0,45,26,25,27,28,42,12)
S6_ROMSTARTMOD(frpwr_b6, "f6p7ic14.732",CRC(e3ecd6f8) SHA1(6fb7cf515804c5c552ef2b9868c561540adb70b4),
                         "f6p7ic20.716",CRC(5f373730) SHA1(57262ed3d05cee100cd091fc068f09f63218d7e4),
                         "f6p7ic17.716",CRC(8731ba39) SHA1(3adfde92fc27cc0bb1a21e287773a74365e68214))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S6_ROMEND
#define input_ports_frpwr_b6 input_ports_frpwr_l2
CORE_CLONEDEFNV(frpwr_b6,frpwr_l6,"Firepower (Sys.6 7-digit conversion)",2003,"Williams / Oliver",s6_mS6S,0)

/* Following games used a 7 segment display */

/*--------------------------
/ Algar - Sys.6 (Game #499)
/-------------------------*/
INITGAMEFULL(algar,s6_7digit_disp,0,50,47,48,49,10,38,36)
S6_ROMSTART(algar,l1, "gamerom.716", CRC(6711da23) SHA1(80a46f5a2630977bc1c6e17466e8865083eb9a18),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(      "sound4.716",  CRC(67ea12e7) SHA1(f81e97183442736d5766a7e5e074bc6539e8ced0))
S6_ROMEND
#define input_ports_algar input_ports_s6
CORE_GAMEDEF(algar,l1,"Algar (L-1)",1980,"Williams",s6_mS6S,0)

/*-------------------------------
/ Alien Poker - Sys.6 (Game #501)
/-------------------------------*/
INITGAMEFULL(alpok,s6_7digit_disp,0,43,35,37,36,38,0,13)
S6_ROMSTART(alpok,l6, "gamerom6.716",CRC(20538a4a) SHA1(6cdd6b7ded76b3cbd954d371e126e1bbd95a6219),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(      "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(  "v_ic7.532" ,  CRC(a66c7ca6) SHA1(6e90081f853fcf66bfeac0a8ee1c762b3760b90b),
                      "v_ic5.532" ,  CRC(f16a237a) SHA1(a904138fad5cbc19946bcf0de824e27537dcd621),
                      "v_ic6.532" ,  CRC(15a3cc85) SHA1(86002ac78189415ae912e8bc23c92b3b67610d87))
S6_ROMEND
#define input_ports_alpok input_ports_s6
CORE_GAMEDEF(alpok,l6,"Alien Poker (L-6)",1980,"Williams",s6_mS6S,0)

S6_ROMSTART(alpok,l2, "gamerom.716", CRC(79c07603) SHA1(526a45b139394e475fc052636e98d880a8908168),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(      "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(  "v_ic7.532" ,  CRC(a66c7ca6) SHA1(6e90081f853fcf66bfeac0a8ee1c762b3760b90b),
                      "v_ic5.532" ,  CRC(f16a237a) SHA1(a904138fad5cbc19946bcf0de824e27537dcd621),
                      "v_ic6.532" ,  CRC(15a3cc85) SHA1(86002ac78189415ae912e8bc23c92b3b67610d87))
S6_ROMEND
CORE_CLONEDEF(alpok,l2,l6,"Alien Poker (L-2)",1980,"Williams",s6_mS6S,0)

S6_ROMSTART(alpok,f6, "gamerom6.716",CRC(20538a4a) SHA1(6cdd6b7ded76b3cbd954d371e126e1bbd95a6219),
                      "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                      "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(      "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS0000(  "5t5014fr.dat",CRC(1d961517) SHA1(c71ee324becfc8cdbecabd1e64b11b5a39ff2483),
                      "5t5015fr.dat",CRC(8d065f80) SHA1(0ab22c9b20ab6fe41abab620435ad03652db7a8e),
                      "5t5016fr.dat",CRC(0ddf91e9) SHA1(48f5fdfc0c5a66dd318fecb7c90e5f5a684a3876),
                      "5t5017fr.dat",CRC(7e546dc1) SHA1(58f8286403978b0d929987189089881d754a9a83))
S6_ROMEND
CORE_CLONEDEF(alpok,f6,l6,"Alien Poker (L-6, French speech)",1980,"Williams",s6_mS6S,0)

S6_ROMSTARTMOD(alpok_b6,"al32ic14.732",CRC(7e950f6e) SHA1(5f9b7c6bc2b9e6ee7b6d6dc66f0d3608f7b467f3),
                        "al32ic20.716",CRC(0058551c) SHA1(6b4bd2bd329ecac18d7a848ff4614cd61f5b37ad),
                        "al32ic17.716",CRC(29ee23d8) SHA1(9d5a143c4567a44bdbba00f16c4713dbb88d2e93))
S67S_SOUNDROMS8(        "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(    "v_ic7.532" ,  CRC(a66c7ca6) SHA1(6e90081f853fcf66bfeac0a8ee1c762b3760b90b),
                        "v_ic5.532" ,  CRC(f16a237a) SHA1(a904138fad5cbc19946bcf0de824e27537dcd621),
                        "v_ic6.532" ,  CRC(15a3cc85) SHA1(86002ac78189415ae912e8bc23c92b3b67610d87))
S6_ROMEND
CORE_CLONEDEF(alpok,b6,l6,"Alien Poker (L-6, Multiball conversion)",2006,"Williams / Oliver",s6_mS6S,0)

