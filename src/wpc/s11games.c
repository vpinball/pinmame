#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s11.h"

/*
Issues:
Pool: Sound loops on startup, stops after one minute?
Jokerz: Startup sound does not stop
*/
#define INITGAME(name, gen, disp, mux, flip, db, gs1) \
static core_tGameData name##GameData = { \
  gen, disp, {flip,0,0,0,0,db,gs1}, NULL, {{0}}, {mux} }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, gen, disp, mux, flip, db, gs1, gs2,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { \
  gen, disp, {flip,0,0,0,0,db,gs1,gs2}, NULL, {{0}}, {mux,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S11_INPUT_PORTS_START(s11, 1) S11_INPUT_PORTS_END

static core_tLCDLayout disp16oneline[] = { \
  {0,0,0,16,CORE_SEG16},{0,33,20,16,CORE_SEG16}, {0}
};

static MACHINE_DRIVER_START(s11a_one)
  MDRV_IMPORT_FROM(s11_s11aS)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(s11c_one)
  MDRV_IMPORT_FROM(s11_s11cS)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)
MACHINE_DRIVER_END

/*--------------------
/ Space Shuttle (S9) 12/84
/--------------------*/
static core_tLCDLayout dispSshtl[] = { \
  { 0, 0,21,7, CORE_SEG87},
  { 2, 0,29,7, CORE_SEG87},
  { 4, 0, 1,7, CORE_SEG87}, { 4,26, 9,7,CORE_SEG87},
  { 5,16, 0,1, CORE_SEG7},  { 5,18, 8,1,CORE_SEG7},
  { 5,21,20,1, CORE_SEG7},  { 5,23,28,1,CORE_SEG7},{0}
};
INITGAMEFULL(sshtl, GEN_S9, dispSshtl, 0, FLIP_SWNO(0,41),
             S11_BCDDIAG|S11_BCDDISP,0,3750, 39, 40, 27, 26, 25, 0)
S9_ROMSTARTx4(sshtl,l7,"cpu_u20.128", CRC(848ad54c) SHA1(4e4ce5fb970da37706472f94a27fd912e1ecb1a0))
S9S_SOUNDROM4111(      "cpu_u49.128", CRC(20e77990) SHA1(b9ec143526d7d152b653c7119e4d07945b6813eb),
                       "spch_u4.732", CRC(b0d03c5e) SHA1(46b952f71a7ecc03e22e427875f6e16a9d124067),
                       "spch_u5.732", CRC(13edd4e5) SHA1(46c4052c31ddc20bb87445636f8fe3b6f7bff856),
                       "spch_u6.732", CRC(cf48b2e7) SHA1(fe55419a5d40b3a4e8c02a92746b25a075b8efd3))
S9_ROMEND
#define input_ports_sshtl input_ports_s11
CORE_GAMEDEF(sshtl, l7, "Space Shuttle (L-7)", 1984, "Williams", s9_mS9S,0)

/*--------------------
/ Sorcerer (S9) 03/85
/--------------------*/
INITGAMEFULL(sorcr, GEN_S9, s11_dispS9, 0, FLIP_SWNO(0,44),
             S11_BCDDIAG|S11_BCDDISP,0,0,32,33,21,22,23,0)
S9_ROMSTART12(sorcr,l1,"cpu_u19.732", CRC(88b6837d) SHA1(d26b06342741443406a72ba48a70e82df62bb26e),
                       "cpu_u20.764", CRC(c235b692) SHA1(d3b97fad2d501c894570601b387933c7644f64e6))
S9S_SOUNDROM41111(     "cpu_u49.128", CRC(a0bae1e4) SHA1(dc5172aa1d59191d4119da20757cb2c2469f8fe3),
                       "spch_u4.732", CRC(0c81902d) SHA1(6d8f703327e5c73a321fc4aa3a67ce68fff82d70),
                       "spch_u5.732", CRC(d48c68ad) SHA1(b1391b87519ad47be3dcce7f8581f871e6a3669f),
                       "spch_u6.732", CRC(a5c54d47) SHA1(4e1206412ecf52ae61c9df2055e0715749a6325d),
                       "spch_u7.732", CRC(bba9ed18) SHA1(8e37ba8cb6bbc1e0afeef230088beda4513adddb))
S9_ROMEND
#define input_ports_sorcr input_ports_s11
CORE_CLONEDEF(sorcr,l1,l2, "Sorcerer (L-1)", 1985, "Williams", s9_mS9S,0)

S9_ROMSTART12(sorcr,l2,"cpu_u19.l2", CRC(faf738db) SHA1(a3b3f4160dc837ddf5379e1edb0eafeefcc11e3d),
                       "cpu_u20.l2", CRC(74fc8117) SHA1(c228c76ade670603f77bb324e6794ec6dd358285))
S9S_SOUNDROM41111(     "cpu_u49.128", CRC(a0bae1e4) SHA1(dc5172aa1d59191d4119da20757cb2c2469f8fe3),
                       "spch_u4.732", CRC(0c81902d) SHA1(6d8f703327e5c73a321fc4aa3a67ce68fff82d70),
                       "spch_u5.732", CRC(d48c68ad) SHA1(b1391b87519ad47be3dcce7f8581f871e6a3669f),
                       "spch_u6.732", CRC(a5c54d47) SHA1(4e1206412ecf52ae61c9df2055e0715749a6325d),
                       "spch_u7.732", CRC(bba9ed18) SHA1(8e37ba8cb6bbc1e0afeef230088beda4513adddb))
S9_ROMEND
CORE_GAMEDEF(sorcr, l2, "Sorcerer (L-2)", 1985, "Williams", s9_mS9S,0)

/*--------------------
/ Comet (S9) 06/85
/--------------------*/
INITGAMEFULL(comet, GEN_S9, s11_dispS9, 0, FLIP_SWNO(0,30),
             S11_BCDDIAG|S11_BCDDISP,0,0,47,48,40,41,42,0)
S9_ROMSTARTx4(comet,l4,"cpu_u20.128", CRC(36193600) SHA1(efdc44ef26c2def8f860a0296e27b2c3dac55ec8))
S9S_SOUNDROM41111(     "cpu_u49.128", CRC(f1db0cbe) SHA1(59b7f36fb2003b90b288abeff56df62ce50f10c6),
                       "spch_u4.732", CRC(d0215c49) SHA1(4f0925a826199b6e8baa5e7fbff5cde9e31d505b),
                       "spch_u5.732", CRC(89f7ede5) SHA1(bbfbd991c9e005c2fa36d8458803b121f4933618),
                       "spch_u6.732", CRC(6ba2aba6) SHA1(783b4e9b38db8677d91f86cb4805f0fa1ae8f856),
                       "spch_u7.732", CRC(36545b22) SHA1(f4a026f3fa58dce81b439d76120a6769f4632955))
S9_ROMEND
#define input_ports_comet input_ports_s11
CORE_CLONEDEF(comet,l4,l5,"Comet (L-4)", 1985, "Williams", s9_mS9S,0)

S9_ROMSTARTx4(comet,l5,"cpu_u20.l5",  CRC(d153d9ab) SHA1(0b97591b8ba35207b1427900486d69078ae122bc))
S9S_SOUNDROM41111(     "cpu_u49.128", CRC(f1db0cbe) SHA1(59b7f36fb2003b90b288abeff56df62ce50f10c6),
                       "spch_u4.732", CRC(d0215c49) SHA1(4f0925a826199b6e8baa5e7fbff5cde9e31d505b),
                       "spch_u5.732", CRC(89f7ede5) SHA1(bbfbd991c9e005c2fa36d8458803b121f4933618),
                       "spch_u6.732", CRC(6ba2aba6) SHA1(783b4e9b38db8677d91f86cb4805f0fa1ae8f856),
                       "spch_u7.732", CRC(36545b22) SHA1(f4a026f3fa58dce81b439d76120a6769f4632955))
S9_ROMEND
CORE_GAMEDEF(comet, l5, "Comet (L-5)", 1985, "Williams", s9_mS9S,0)

/*--------------------
/ High Speed 01/86
/--------------------*/
INITGAMEFULL(hs, GEN_S11X, s11_dispS11, 0, FLIP_SWNO(37,38),
             S11_BCDDIAG,0,0,49,50,35,34,33,0)
S11_ROMSTART28(hs,l4,"hs_u26.l4", CRC(38b73830) SHA1(df89670f3df2b657dcf1f8ee08e506e54e016028),
                     "hs_u27.l4", CRC(24c6f7f0) SHA1(bb0058650ec0908f88b6a202df79e971b46f8594))
S11XS_SOUNDROM88(    "hs_u21.l2", CRC(c0580037) SHA1(675ca65a6a20f8607232c532b4d127641f77d837),
                     "hs_u22.l2", CRC(c03be631) SHA1(53823e0f55377a45aa181882c310dd307cf368f5))
S11CS_SOUNDROM8(     "hs_u4.l1",  CRC(0f96e094) SHA1(58650705a02a71ced85f5c2a243722a35282cbf7))
S11_ROMEND
#define input_ports_hs input_ports_s11

S11_ROMSTART28(hs,l3,"u26-l3.rom", CRC(fd587959) SHA1(20fe6d7bd617b1fa886362ce520393a25be9a632),
                     "hs_u27.l4", CRC(24c6f7f0) SHA1(bb0058650ec0908f88b6a202df79e971b46f8594))
S11XS_SOUNDROM88(    "hs_u21.l2", CRC(c0580037) SHA1(675ca65a6a20f8607232c532b4d127641f77d837),
                     "hs_u22.l2", CRC(c03be631) SHA1(53823e0f55377a45aa181882c310dd307cf368f5))
S11CS_SOUNDROM8(     "hs_u4.l1",  CRC(0f96e094) SHA1(58650705a02a71ced85f5c2a243722a35282cbf7))
S11_ROMEND

CORE_GAMEDEF(hs, l4, "High Speed (L-4)", 1986, "Williams", s11_mS11XS,0)
CORE_CLONEDEF(hs,l3,l4, "High Speed (L-3)", 1986, "Williams", s11_mS11XS,0)

/*--------------------
/ Grand Lizard 04/86
/--------------------*/
INITGAMEFULL(grand, GEN_S11X, s11_dispS11, 0, FLIP_SWNO(0,48),
             S11_BCDDIAG,0,0,43,44,0,0,0,0)
S11_ROMSTART28(grand,l4,"lzrd_u26.l4", CRC(5fe50db6) SHA1(7e2adfefce5c33ad605606574dbdfb2642aa0e85),
                        "lzrd_u27.l4", CRC(6462ca55) SHA1(0ebfa998d3cefc213ada9ed815d44977120e5d6d))
S11XS_SOUNDROM44(       "lzrd_u21.l1", CRC(98859d37) SHA1(08429b9e6a3b3007815373dc280b985e3441aa9f),
                        "lzrd_u22.l1", CRC(4e782eba) SHA1(b44ab499128300175bdb57f07ffe2992c82e47e4))
S11CS_SOUNDROM8(        "lzrd_u4.l1",  CRC(4baafc11) SHA1(3507f5f37e02688fa56cf5bb303eaccdcedede06))
S11_ROMEND
#define input_ports_grand input_ports_s11
CORE_GAMEDEF(grand, l4, "Grand Lizard (L-4)", 1986, "Williams", s11_mS11XS,0)

/*--------------------
/ Road Kings 07/86
/--------------------*/
INITGAMEFULL(rdkng, GEN_S11X, s11_dispS11,12, FLIP_SWNO(47,48),
             S11_BCDDIAG,S11_RKMUX,0,43,44,24,25,26,27)
S11_ROMSTART48(rdkng,l4,"road_u26.l4", CRC(4ea27d67) SHA1(cf46e8c5e417999150403d6d40adf8c36b1c0347),
                        "road_u27.l4", CRC(5b88e755) SHA1(6438505bb335f670e0892126764819a48eec9b88))
S11XS_SOUNDROM88(       "road_u21.l1", CRC(f34efbf4) SHA1(cb5ffe9818994f4681e3492a5cd46f410d2e5353),
                        "road_u22.l1", CRC(a9803804) SHA1(a400d4621c3f7a6e47546b2f33dc4920183a5a74))
S11CS_SOUNDROM8(        "road_u4.l1",  CRC(4395b48f) SHA1(2325ce6ba7f6f92f884c302e6f053c31229dc774))
S11_ROMEND
#define input_ports_rdkng input_ports_s11
CORE_GAMEDEF(rdkng, l4, "Road Kings (L-4)", 1986, "Williams", s11_mS11XS,0)

S11_ROMSTART48(rdkng,l1,"road_u26.l1", CRC(19abe96b) SHA1(d6c3b6dab328f23cc4506e4f56cd0beeb06fb3cb),
                        "road_u27.l1", CRC(3dcad794) SHA1(0cf06f8e16d738f0bc0111e2e12351a26e2f02c6))
S11XS_SOUNDROM88(       "road_u21.l1", CRC(f34efbf4) SHA1(cb5ffe9818994f4681e3492a5cd46f410d2e5353),
                        "road_u22.l1", CRC(a9803804) SHA1(a400d4621c3f7a6e47546b2f33dc4920183a5a74))
S11CS_SOUNDROM8(        "road_u4.l1",  CRC(4395b48f) SHA1(2325ce6ba7f6f92f884c302e6f053c31229dc774))
S11_ROMEND
CORE_CLONEDEF(rdkng,l1,l4,"Road Kings (L-1)", 1986, "Williams", s11_mS11XS,0)

S11_ROMSTART48(rdkng,l2,"road_u26.l1", CRC(19abe96b) SHA1(d6c3b6dab328f23cc4506e4f56cd0beeb06fb3cb),
                        "road_u27.l2", CRC(aff45e2b) SHA1(c52aca20639f519a940951ef04c2bd179a596b30))
S11XS_SOUNDROM88(       "road_u21.l1", CRC(f34efbf4) SHA1(cb5ffe9818994f4681e3492a5cd46f410d2e5353),
                        "road_u22.l1", CRC(a9803804) SHA1(a400d4621c3f7a6e47546b2f33dc4920183a5a74))
S11CS_SOUNDROM8(        "road_u4.l1",  CRC(4395b48f) SHA1(2325ce6ba7f6f92f884c302e6f053c31229dc774))
S11_ROMEND
CORE_CLONEDEF(rdkng,l2,l4,"Road Kings (L-2)", 1986, "Williams", s11_mS11XS,0)

S11_ROMSTART48(rdkng,l3,"road_u26.l3", CRC(9bade45d) SHA1(c1791724761cdd1d863e12b02655c5fed8936162),
                        "road_u27.l3", CRC(97b599dc) SHA1(18524d22a75b0569bb480d847cef8047ee51f91e))
S11XS_SOUNDROM88(       "road_u21.l1", CRC(f34efbf4) SHA1(cb5ffe9818994f4681e3492a5cd46f410d2e5353),
                        "road_u22.l1", CRC(a9803804) SHA1(a400d4621c3f7a6e47546b2f33dc4920183a5a74))
S11CS_SOUNDROM8(        "road_u4.l1",  CRC(4395b48f) SHA1(2325ce6ba7f6f92f884c302e6f053c31229dc774))
S11_ROMEND
CORE_CLONEDEF(rdkng,l3,l4,"Road Kings (L-3)", 1986, "Williams", s11_mS11XS,0)

/*--------------------
/ Pinbot 10/86
/--------------------*/
INITGAMEFULL(pb, GEN_S11X, s11_dispS11, 14, FLIP_SWNO(10,11),
             0,0,0, 53, 0, 48, 54, 55,52)
S11_ROMSTART48(pb,l5,"pbot_u26.l5", CRC(daa0c8e4) SHA1(47289b350eb0d84aa0d37e53383e18625451bbe8),
                     "pbot_u27.l5", CRC(e625d6ce) SHA1(1858dc2183954342b8e2e5eb9a14edcaa8dad5ae))
S11XS_SOUNDROM88(    "pbot_u21.l1", CRC(3eab88d9) SHA1(667e3b675e2ae8fec6a6faddb9b0dd5531d64f8f),
                     "pbot_u22.l1", CRC(a2d2c9cb) SHA1(46437dc54538f1626caf41a2818ddcf8000c44e4))
S11CS_SOUNDROM88(    "pbot_u4.l1",  CRC(de5926bd) SHA1(3d111e27c5f0c8c0afc5fe5cc45bf77c12b69228),
                     "pbot_u19.l1", CRC(40eb4e9f) SHA1(07b0557b35599a2dd5aa66a306fbbe8f50eed998))
S11_ROMEND
#define input_ports_pb input_ports_s11

S11_ROMSTART48(pb,l2,"u26-l2.rom", CRC(e3b94ca4) SHA1(1db2acb025941cc165cc7ec70a160e07ab1eeb2e),
                     "u27-l2.rom", CRC(0a334fc5) SHA1(d08afe6ddc141e37f97ea588d184a316ff7f6db7))
S11XS_SOUNDROM88(    "pbot_u21.l1", CRC(3eab88d9) SHA1(667e3b675e2ae8fec6a6faddb9b0dd5531d64f8f),
                     "pbot_u22.l1", CRC(a2d2c9cb) SHA1(46437dc54538f1626caf41a2818ddcf8000c44e4))
S11CS_SOUNDROM88(    "pbot_u4.l1",  CRC(de5926bd) SHA1(3d111e27c5f0c8c0afc5fe5cc45bf77c12b69228),
                     "pbot_u19.l1", CRC(40eb4e9f) SHA1(07b0557b35599a2dd5aa66a306fbbe8f50eed998))
S11_ROMEND

S11_ROMSTART48(pb,l3,"u26-l2.rom", CRC(e3b94ca4) SHA1(1db2acb025941cc165cc7ec70a160e07ab1eeb2e),
                     "u27-l3.rom", CRC(6f40ee84) SHA1(85453137e3fdb1e422e3903dd053e04c9f2b9607))
S11XS_SOUNDROM88(    "pbot_u21.l1", CRC(3eab88d9) SHA1(667e3b675e2ae8fec6a6faddb9b0dd5531d64f8f),
                     "pbot_u22.l1", CRC(a2d2c9cb) SHA1(46437dc54538f1626caf41a2818ddcf8000c44e4))
S11CS_SOUNDROM88(    "pbot_u4.l1",  CRC(de5926bd) SHA1(3d111e27c5f0c8c0afc5fe5cc45bf77c12b69228),
                     "pbot_u19.l1", CRC(40eb4e9f) SHA1(07b0557b35599a2dd5aa66a306fbbe8f50eed998))
S11_ROMEND

CORE_GAMEDEF(pb, l5, "Pinbot (L-5)", 1987, "Williams", s11_mS11XSL,0)
CORE_CLONEDEF(pb,l2,l5, "Pinbot (L-2)", 1987, "Williams", s11_mS11XSL,0)
CORE_CLONEDEF(pb,l3,l5, "Pinbot (L-3)", 1987, "Williams", s11_mS11XSL,0)

/*--------------------
/ F14 Tomcat 3/87
/--------------------*/
INITGAMEFULL(f14, GEN_S11A, s11_dispS11a, 14, FLIP_SWNO(15,63),
             0,0,0, 57, 58, 0, 28, 0, 0)
S11_ROMSTART48(f14,l1,"f14_u26.l1", CRC(62c2e615) SHA1(456ce0d1f74fa5e619c272880ba8ac6819848ddc),
                      "f14_u27.l1", CRC(da1740f7) SHA1(1395a4f3891a043cfedc5426ec88af35eab8d4ea))
S11XS_SOUNDROM88(     "f14_u21.l1", CRC(e412300c) SHA1(382d0cfa47abea295f0c7501bc0a010473e9d73b),
                      "f14_u22.l1", CRC(c9dd7496) SHA1(de3cb855d87033274cc912578b02d1593d2d69f9))
S11CS_SOUNDROM88(     "f14_u4.l1",  CRC(43ecaabf) SHA1(64b50dbff03cd556130d0cff47b951fdf37d397d),
                      "f14_u19.l1", CRC(d0de4a7c) SHA1(46ecd5786653add47751cc56b38d9db7c4622377))
S11_ROMEND
#define input_ports_f14 input_ports_s11
CORE_GAMEDEF(f14, l1, "F14 Tomcat (L-1)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Fire! 8/87
/--------------------*/
INITGAMEFULL(fire, GEN_S11A, s11_dispS11a, 12, FLIP_SWNO(23,24),
             0,0,0, 0,0, 61, 62, 57, 58)
S11_ROMSTART48(fire,l3,"fire_u26.l3", CRC(48abae33) SHA1(00ce24316aa007eec090ae74818003e11a141214),
                       "fire_u27.l3", CRC(4ebf4888) SHA1(45dc0231404ed70be2ab5d599a673aac6271550e))
S11XS_SOUNDROM88(      "fire_u21.l2", CRC(2edde0a4) SHA1(de292a340a3a06b0b996fc69fee73eb7bbfbbe64),
                       "fire_u22.l2", CRC(16145c97) SHA1(523e99df3907a2c843c6e27df4d16799c4136a46))
S11CS_SOUNDROM8(       "fire_u4.l1",  CRC(0e058918) SHA1(4d6bf2290141119174787f8dd653c47ea4c73693))

S11_ROMEND
#define input_ports_fire input_ports_s11
CORE_GAMEDEF(fire, l3, "Fire! (L-3)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Big Guns 10/87
/--------------------*/
INITGAME(bguns, GEN_S11A,s11_dispS11a,12, FLIP_SWNO(52,53) ,0,0)
S11_ROMSTART48(bguns,l8,"guns_u26.l8", CRC(792dc1e8) SHA1(34586585bbaf579cb522569238e24d9ab891b471),
                        "guns_u27.l8", CRC(ac4a1a51) SHA1(d48b5e5b550107df8c6edc2d5f78777d7d408959))
S11XS_SOUNDROM88(        "guns_u21.l1", CRC(35c6bfe4) SHA1(83dbd10311add75f56046de58d315f8a87389703),
                        "guns_u22.l1", CRC(091a5cb8) SHA1(db77314241eb6ed7f4385f99312a49b7caad1283))
S11CS_SOUNDROM88(       "gund_u4.l1",  CRC(d4a430a3) SHA1(5b44e3f313cc7cb75f51c239013d46e5eb986f9d),
                        "guns_u19.l1", CRC(ec1a6c23) SHA1(45bb4f78b89de9e690b5f9741d17f97766e702d6))
S11_ROMEND
#define input_ports_bguns input_ports_s11
CORE_GAMEDEF(bguns, l8, "Big Guns (L-8)", 1987, "Williams", s11_mS11AS,0)

S11_ROMSTART48(bguns,l7,"guns_u26.l8", CRC(792dc1e8) SHA1(34586585bbaf579cb522569238e24d9ab891b471),
                        "guns_u27.l7", CRC(8ff26d24) SHA1(eab732b401144ad7efc80d336299beae85ca7d24))
S11XS_SOUNDROM88(        "guns_u21.l1", CRC(35c6bfe4) SHA1(83dbd10311add75f56046de58d315f8a87389703),
                        "guns_u22.l1", CRC(091a5cb8) SHA1(db77314241eb6ed7f4385f99312a49b7caad1283))
S11CS_SOUNDROM88(       "gund_u4.l1",  CRC(d4a430a3) SHA1(5b44e3f313cc7cb75f51c239013d46e5eb986f9d),
                        "guns_u19.l1", CRC(ec1a6c23) SHA1(45bb4f78b89de9e690b5f9741d17f97766e702d6))
S11_ROMEND
CORE_CLONEDEF(bguns,l7,l8,"Big Guns (L-7)", 1987, "Williams", s11_mS11AS,0)

S11_ROMSTART48(bguns,la,"u26-l-a.rom", CRC(613b4d5c) SHA1(7eed4ddb661cd03839a9a89ca695de9cbd1c4d45),
                        "u27-l-a.rom", CRC(eee9e1cc) SHA1(32fbade5cbc9047a61d4ce0ec1e616d5324d507f))
S11XS_SOUNDROM88(        "guns_u21.l1", CRC(35c6bfe4) SHA1(83dbd10311add75f56046de58d315f8a87389703),
                        "guns_u22.l1", CRC(091a5cb8) SHA1(db77314241eb6ed7f4385f99312a49b7caad1283))
S11CS_SOUNDROM88(       "gund_u4.l1",  CRC(d4a430a3) SHA1(5b44e3f313cc7cb75f51c239013d46e5eb986f9d),
                        "guns_u19.l1", CRC(ec1a6c23) SHA1(45bb4f78b89de9e690b5f9741d17f97766e702d6))
S11_ROMEND
CORE_CLONEDEF(bguns,la,l8,"Big Guns (L-A)", 1987, "Williams", s11_mS11AS,0)

S11_ROMSTART48(bguns,p1,"u26-p-1.rom", CRC(26b8d58f) SHA1(678d4f706b862f3168d6d15859dba6288912e462),
                        "u27-p-1.rom", CRC(2fba9a0d) SHA1(16629a5f009865825207378118a147e3135c51cf))
S11XS_SOUNDROM88(        "guns_u21.l1", CRC(35c6bfe4) SHA1(83dbd10311add75f56046de58d315f8a87389703),
                        "guns_u22.l1", CRC(091a5cb8) SHA1(db77314241eb6ed7f4385f99312a49b7caad1283))
S11CS_SOUNDROM88(       "gund_u4.l1",  CRC(d4a430a3) SHA1(5b44e3f313cc7cb75f51c239013d46e5eb986f9d),
                        "guns_u19.l1", CRC(ec1a6c23) SHA1(45bb4f78b89de9e690b5f9741d17f97766e702d6))
S11_ROMEND
CORE_CLONEDEF(bguns,p1,l8,"Big Guns (P-1)", 1987, "Williams", s11_mS11AS,0)

/*--------------------
/ Space Station 1/88
/--------------------*/
INITGAMEFULL(spstn, GEN_S11B,s11_dispS11b1,12, FLIP_SWNO(55,56),
             0,0,0,0,63,61,64,60,62)
S11_ROMSTART48(spstn,l5,"sstn_u26.l5", CRC(614c8528) SHA1(4f177e3d72a5cc302c62c756ec778ae2a98c8f2e),
                        "sstn_u27.l5", CRC(4558d963) SHA1(be317310978cca4ddd616d76fe892dcf7c980473))
S11XS_SOUNDROM88(       "sstn_u21.l1", CRC(a2ceccaa) SHA1(4c23713543e06458e49e3f2d472543c4a4246a93),
                        "sstn_u22.l1", CRC(2b745994) SHA1(67ebfe13db6670237496b033611bf9d4ba8d5c30))
S11CS_SOUNDROM8(        "sstn_u4.l1",  CRC(ad7a0511) SHA1(9aa6412de12599fd0d10faef8fffb5d535f49015))
S11_ROMEND
#define input_ports_spstn input_ports_s11
CORE_GAMEDEF(spstn, l5, "Space Station (L-5)", 1988, "Williams", s11_mS11BS,0)

/*--------------------
/ Cyclone 2/88
/--------------------*/
INITGAME(cycln, GEN_S11B,s11_dispS11b1, 12, FLIP_SWNO(57,58), 0,0)
S11_ROMSTART48(cycln,l5,"cycl_u26.l5", CRC(9ab15e12) SHA1(406f3212269dc42de1f3fabcf179958adbd4b5e8),
                        "cycl_u27.l5", CRC(c4b6aac0) SHA1(9058e450dbf9d198b1746c258b0e437d7ee844e9))
S11XS_SOUNDROM88(       "cycl_u21.l1", CRC(d4f69a7c) SHA1(da0ce27d92b22583be54a41fc8083cee803c987a),
                        "cycl_u22.l1", CRC(28dc8f13) SHA1(bccce3a9b6b2f52da919c6df8db07e5e3de12657))
S11CS_SOUNDROM88(       "cycl_u4.l5",  CRC(d04b663b) SHA1(f54c6df08ec73b733cfeb2a989e44e5c04da3d9e),
                        "cycl_u19.l1", CRC(a20f6519) SHA1(63ded5f76133340fa31d4fe65420f4465866fb85))
S11_ROMEND
#define input_ports_cycln input_ports_s11
CORE_GAMEDEF(cycln, l5, "Cyclone (L-5)", 1988, "Williams", s11_mS11BS,0)

S11_ROMSTART48(cycln,l4,"cycl_u26.l4", CRC(7da30995) SHA1(3774004df22ddce508fe0604c0349df3edd513b4),
                        "cycl_u27.l4", CRC(8874d65f) SHA1(600e2e8cd21faf8999ebef06fb08c43a1eb2ffd7))
S11XS_SOUNDROM88(       "cycl_u21.l1", CRC(d4f69a7c) SHA1(da0ce27d92b22583be54a41fc8083cee803c987a),
                        "cycl_u22.l1", CRC(28dc8f13) SHA1(bccce3a9b6b2f52da919c6df8db07e5e3de12657))
S11CS_SOUNDROM88(       "cycl_u4.l5",  CRC(d04b663b) SHA1(f54c6df08ec73b733cfeb2a989e44e5c04da3d9e),
                        "cycl_u19.l1", CRC(a20f6519) SHA1(63ded5f76133340fa31d4fe65420f4465866fb85))
S11_ROMEND
CORE_CLONEDEF(cycln, l4, l5, "Cyclone (L-4)", 1988, "Williams", s11_mS11BS,0)

/*--------------------
/ Banzai Run 7/88
/--------------------*/
INITGAME(bnzai, GEN_S11B,s11_dispS11b1,12, FLIP_SWNO(25,34),0,0)
S11_ROMSTART48(bnzai,l3,"banz_u26.l3", CRC(ca578aa3) SHA1(32c03178cc9d9514f76e084e56f6cf6f82754331),
                        "banz_u27.l3", CRC(af66fac4) SHA1(84929aaad8a8e4a312a230b73f206d3b43a04dc3))
S11XS_SOUNDROM88(       "banz_u21.l1", CRC(cd06716e) SHA1(b61a0dc017dd4a09296a43a855461c5cee07517b),
                        "banz_u22.l1", CRC(e8159033) SHA1(e8f15801feefeb30768e88d685c208108aa134e8))
S11CS_SOUNDROM888(      "banz_u4.l1",  CRC(8fd69c69) SHA1(c024cda85c6616943c3a12ab5943a7be8709bfe3),
                        "banz_u19.l1", CRC(9104248c) SHA1(48a8c41f3a4127f4fb4de37e876c8380e3511e1f),
                        "banz_u20.l1", CRC(26b3d15c) SHA1(528084b6c62394f8ed9fc0f90b91d844060fc904))
S11_ROMEND
#define input_ports_bnzai input_ports_s11

S11_ROMSTART48(bnzai,g3,"banz_u26.l3g",CRC(744b8758) SHA1(0bcd5dfd872656d0261a819e3dbd222754585ec0),
                        "banz_u27.l3", CRC(af66fac4) SHA1(84929aaad8a8e4a312a230b73f206d3b43a04dc3))
S11XS_SOUNDROM88(       "banz_u21.l1", CRC(cd06716e) SHA1(b61a0dc017dd4a09296a43a855461c5cee07517b),
                        "banz_u22.l1", CRC(e8159033) SHA1(e8f15801feefeb30768e88d685c208108aa134e8))
S11CS_SOUNDROM888(      "banz_u4.l1",  CRC(8fd69c69) SHA1(c024cda85c6616943c3a12ab5943a7be8709bfe3),
                        "banz_u19.l1", CRC(9104248c) SHA1(48a8c41f3a4127f4fb4de37e876c8380e3511e1f),
                        "banz_u20.l1", CRC(26b3d15c) SHA1(528084b6c62394f8ed9fc0f90b91d844060fc904))
S11_ROMEND

S11_ROMSTART48(bnzai,l1,"u26-l1.rom",  CRC(556abdc0) SHA1(6de78345e5839a4ae9ff97273b6edb2635e0e8b4),
                        "u27-l1.rom",  CRC(7fc6de2e) SHA1(a7b42c2cd8c1e3810a319c755e52273454d5ca41))
S11XS_SOUNDROM88(       "banz_u21.l1", CRC(cd06716e) SHA1(b61a0dc017dd4a09296a43a855461c5cee07517b),
                        "banz_u22.l1", CRC(e8159033) SHA1(e8f15801feefeb30768e88d685c208108aa134e8))
S11CS_SOUNDROM888(      "banz_u4.l1",  CRC(8fd69c69) SHA1(c024cda85c6616943c3a12ab5943a7be8709bfe3),
                        "banz_u19.l1", CRC(9104248c) SHA1(48a8c41f3a4127f4fb4de37e876c8380e3511e1f),
                        "banz_u20.l1", CRC(26b3d15c) SHA1(528084b6c62394f8ed9fc0f90b91d844060fc904))
S11_ROMEND

S11_ROMSTART48(bnzai,pa,"u26-pa.rom",  CRC(65a73e31) SHA1(0332b51ecfc548f72eaca402d83a5ad6dd223272),
                        "u27-pa.rom",  CRC(c64e2898) SHA1(b2291e9e65f8392f2f05f116dc47fcaf37500e60))
S11XS_SOUNDROM88(       "banz_u21.l1", CRC(cd06716e) SHA1(b61a0dc017dd4a09296a43a855461c5cee07517b),
                        "banz_u22.l1", CRC(e8159033) SHA1(e8f15801feefeb30768e88d685c208108aa134e8))
S11CS_SOUNDROM888(      "u4-p7.rom",   CRC(630d1ce9) SHA1(fb7f6004b94bf20281216519f18b53949eef4405),
                        "banz_u19.l1", CRC(9104248c) SHA1(48a8c41f3a4127f4fb4de37e876c8380e3511e1f),
                        "banz_u20.l1", CRC(26b3d15c) SHA1(528084b6c62394f8ed9fc0f90b91d844060fc904))
S11_ROMEND

CORE_GAMEDEF(bnzai, l3, "Banzai Run (L-3)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(bnzai,g3,l3,"Banzai Run (L-3) Germany", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(bnzai,l1,l3,"Banzai Run (L-1)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(bnzai,pa,l3,"Banzai Run (P-A)", 1988, "Williams", s11_mS11BS,0)

/*--------------------
/ Swords of Fury 8/88
/--------------------*/
INITGAME(swrds, GEN_S11B, s11_dispS11b1,12, FLIP_SWNO(58,60),0,0)
S11_ROMSTART48(swrds,l2,"swrd_u26.l2", CRC(c036f4ff) SHA1(a86840dbc117774aeca695ded1ab3ec76e134325),
                        "swrd_u27.l2", CRC(33b0fb5a) SHA1(a55bdfe20b1c869eae52d3be75df1c550d0b20f5))
S11XS_SOUNDROM88(       "swrd_u21.l1", CRC(ee8b0a64) SHA1(c2c52059a9a5f7c0abcfdd76cfc6d5b5451f7d1e),
                        "swrd_u22.l1", CRC(73dcdbb0) SHA1(66f5b3804442a1742b6fb3cccf539c4df956b3f2))
S11CS_SOUNDROM88(       "swrd_u4.l1",  CRC(272b509c) SHA1(756d3783f664ca1c41dd1d12032330b74c3f89ea),
                        "swrd_u19.l1", CRC(a22f84fa) SHA1(1731e86e85cca2d283512d5048c787df3970c9c5))
S11_ROMEND
#define input_ports_swrds input_ports_s11
CORE_GAMEDEF(swrds, l2, "Swords of Fury (L-2)", 1988, "Williams", s11_mS11BS, 0)

/*--------------------
/ Taxi 10/88
/--------------------*/
static core_tLCDLayout dispTaxi[] = { \
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG8),\
  { 2, 0,21,7,CORE_SEG7SCH},{0}
};

INITGAME(taxi, GEN_S11B,dispTaxi,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(taxi, l4,"taxi_u26.l4", CRC(a70d8088) SHA1(0986035436e2b1199571248dac8eb7a903b5015c),
                        "taxi_u27.l4", CRC(f973f79c) SHA1(a33ab04451d8a5b2354e4d174c238878e962f228))
S11XS_SOUNDROM88(       "taxi_u21.l1", CRC(2b20e9ab) SHA1(d785667ae0fd237dd8343bb1ecfbacf050ec2c6f),
                        "taxi_u22.l1", CRC(d13055c5) SHA1(8c2959bde03567b83db425ebc9e7309d9601c2b2))
S11CS_SOUNDROM88(       "taxi_u4.l1",  CRC(6082ebb5) SHA1(37e19ad27fe05b4c8e572f6598d2d574e4ac5a7d),
                        "taxi_u19.l1", CRC(91c64913) SHA1(10e48977f925f6bc1be0c56854aafa99283b4047))
S11_ROMEND
S11_ROMSTART48(taxi, l3,"taxi_u26.l4", CRC(a70d8088) SHA1(0986035436e2b1199571248dac8eb7a903b5015c),
                        "taxi_u27.l3", CRC(e2bfb6fa) SHA1(ba1bddffe4d4e8f04131dd6f5a0380765fbcdfc5))
S11XS_SOUNDROM88(       "taxi_u21.l1", CRC(2b20e9ab) SHA1(d785667ae0fd237dd8343bb1ecfbacf050ec2c6f),
                        "taxi_u22.l1", CRC(d13055c5) SHA1(8c2959bde03567b83db425ebc9e7309d9601c2b2))
S11CS_SOUNDROM88(       "taxi_u4.l1",  CRC(6082ebb5) SHA1(37e19ad27fe05b4c8e572f6598d2d574e4ac5a7d),
                        "taxi_u19.l1", CRC(91c64913) SHA1(10e48977f925f6bc1be0c56854aafa99283b4047))
S11_ROMEND
S11_ROMSTART48(taxi, lg1,"u26-lg1m.rom", CRC(40a2f33c) SHA1(815910b36a5df6c63862590c42b6a41286f38236),
                        "u27-lg1m.rom", CRC(955dcbab) SHA1(e66e0da6366885ceed7618b09cf66fe11ae27627))
S11XS_SOUNDROM88(       "taxi_u21.l1", CRC(2b20e9ab) SHA1(d785667ae0fd237dd8343bb1ecfbacf050ec2c6f),
                        "taxi_u22.l1", CRC(d13055c5) SHA1(8c2959bde03567b83db425ebc9e7309d9601c2b2))
S11CS_SOUNDROM88(       "taxi_u4.l1",  CRC(6082ebb5) SHA1(37e19ad27fe05b4c8e572f6598d2d574e4ac5a7d),
                        "taxi_u19.l1", CRC(91c64913) SHA1(10e48977f925f6bc1be0c56854aafa99283b4047))
S11_ROMEND
#define input_ports_taxi input_ports_s11
CORE_GAMEDEF(taxi , l4, "Taxi (Lola) (L-4)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(taxi , l3, l4, "Taxi (Marilyn) (L-3)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(taxi , lg1, l4, "Taxi (Marilyn) (L-1) Germany", 1988, "Williams", s11_mS11BS,0)

/*--------------------
/ Jokerz 1/89
/--------------------*/
INITGAME(jokrz, GEN_S11B2, s11_dispS11b2,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(jokrz,l6,"jokeru26.l6", CRC(c748c1ba) SHA1(e74b3be2c5d3e81ff29bc4444384f456846111b3),
                        "jokeru27.l6", CRC(612d0ea7) SHA1(35d88de615a15442689e13414117b7dfca6a4614))
S11XS_SOUNDROM88(       "jokeru21.l1", CRC(9e2be4f6) SHA1(6e26b55935d0c8138176b54a11c1a9ab58366628),
                        "jokeru22.l1", CRC(2f67160c) SHA1(f1e179fde41f9bf8226069c24b0bd5152a13e518))
S11JS_SOUNDROM(         "jokeru5.l2" , CRC(e9dc0095) SHA1(23a99555e50461ccc8e67de01796642c080294c2))
S11_ROMEND
#define input_ports_jokrz input_ports_s11
CORE_GAMEDEF(jokrz, l6, "Jokerz (L-6)", 1989, "Williams", s11_mS11B2S,GAME_IMPERFECT_SOUND)

S11_ROMSTART48(jokrz,l3,"u26-l3.rom", CRC(3bf963df) SHA1(9f7757d96deca8638dbc1fe3669eee78dc222ebb),
                        "u27-l3.rom", CRC(32526aff) SHA1(c4ee4b58e90f214012addada114fc9333d2d274c))
S11XS_SOUNDROM88(       "jokeru21.l1", CRC(9e2be4f6) SHA1(6e26b55935d0c8138176b54a11c1a9ab58366628),
                        "jokeru22.l1", CRC(2f67160c) SHA1(f1e179fde41f9bf8226069c24b0bd5152a13e518))
S11JS_SOUNDROM(         "jokeru5.l2" , CRC(e9dc0095) SHA1(23a99555e50461ccc8e67de01796642c080294c2))
S11_ROMEND
CORE_CLONEDEF(jokrz,l3,l6,"Jokerz (L-3)", 1989, "Williams", s11_mS11B2S,GAME_IMPERFECT_SOUND)

/*--------------------
/ Earthshaker 4/89
/--------------------*/
INITGAME(esha,GEN_S11B, s11_dispS11b2,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(esha,pr4,"eshk_u26.f1", CRC(15e2bfe3) SHA1(57ce7f017a6f9ab88f221870efde91e97efbc8a6),
                        "eshk_u27.f1", CRC(ddfa8edd) SHA1(e59ba6c1e8a0087abda218a8922d83ebefd84666))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "eshk_u4.l1",  CRC(40069f8c) SHA1(aafdc189259fa9c8dc49e60e978b84775e16c64e),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))

S11_ROMEND
S11_ROMSTART48(esha,la3,"eshk_u26.l3", CRC(5350d132) SHA1(fbc671c89f85375c34c49610943c87336123fdc8),
                        "eshk_u27.l3", CRC(91389290) SHA1(3f80b77aa0b7db2409bc6b197feb7a4d289b6ec8))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "eshk_u4.l1",  CRC(40069f8c) SHA1(aafdc189259fa9c8dc49e60e978b84775e16c64e),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))

S11_ROMEND
S11_ROMSTART48(esha,lg1,"u26-lg1.rom", CRC(6b1c4d12) SHA1(8e90878ab3b6319e4b81967b4cb8c47e1b6b936c),
                        "u27-lg1.rom", CRC(6ee69cda) SHA1(227a4b311b9fa5f34d38bee2b5063572a06809cf))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "eshk_u4.l1",  CRC(40069f8c) SHA1(aafdc189259fa9c8dc49e60e978b84775e16c64e),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))

S11_ROMEND
S11_ROMSTART48(esha,lg2,"u26-lg2.rom", CRC(e30361c6) SHA1(f5626aaf36348b3aad6b04901c5d84eee1153f51),
                        "u27-lg1.rom", CRC(6ee69cda) SHA1(227a4b311b9fa5f34d38bee2b5063572a06809cf))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "eshk_u4.l1",  CRC(40069f8c) SHA1(aafdc189259fa9c8dc49e60e978b84775e16c64e),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))

S11_ROMEND
S11_ROMSTART48(esha,la1,"u26-la1.rom", CRC(c9c9a32d) SHA1(cd273198e777b644535836ea5785b0dfe5c792c5),
                        "u27-la1.rom", CRC(3433b516) SHA1(5aff6bc72f2d6c0fd00f125ed5b4b6d8035d54bc))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "eshk_u4.l1",  CRC(40069f8c) SHA1(aafdc189259fa9c8dc49e60e978b84775e16c64e),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))
S11_ROMEND
S11_ROMSTART48(esha,pa1,"u26-pa1.rom", CRC(08c0b0d6) SHA1(36c23655e1ae07a3a5c91f68fdb27a78ca272683),
                        "u27-pa1.rom", CRC(ddd6e8bb) SHA1(b46da424f9c4ac70e65af3ee7b4e08df38ffdb26))
S11XS_SOUNDROM88(       "eshk_u21.l1", CRC(feac68e5) SHA1(2f12a78398bc3a468e3e0656da91260d45b0663b),
                        "eshk_u22.l1", CRC(44f50fe1) SHA1(a8e24dbb0f5cf300118e1ebdcd2bb6b274d87936))
S11CS_SOUNDROM88(       "u4-p1.rom",   CRC(7219ffc2) SHA1(b8585b7d12f401d8ba4d95a5e2f20d35ff0ac26a),
                        "eshk_u19.l1", CRC(e5593075) SHA1(549b03402e5639b449e35325eb52e78f8810b07a))
S11_ROMEND
#define input_ports_esha input_ports_s11
CORE_GAMEDEF (esha, la3, "Earthshaker (LA-3)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(esha, pr4, la3, "Earthshaker (Family version) (PR-4)", 1989, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(esha, lg1, la3, "Earthshaker (German) (LG-1)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(esha, lg2, la3, "Earthshaker (German) (LG-2)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(esha, la1, la3, "Earthshaker (LA-1)", 1988, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(esha, pa1, la3, "Earthshaker (Prototype) (PA-1)", 1988, "Williams", s11_mS11BS,0)

/*-----------------------
/ Black Knight 2000 6/89
/-----------------------*/
INITGAME(bk2k, GEN_S11B, s11_dispS11b2,12, FLIP_SWNO(58,57), S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(bk2k, l4,"bk2k_u26.l4", CRC(16c7b9e7) SHA1(b6d5edb5ac2b58da699702ece00534d18c1a9fd7),
                        "bk2k_u27.l4", CRC(5cf3ab40) SHA1(ee8cb554d10478b028da4a761476d6ec8c56a042))
S11XS_SOUNDROM88(       "bk2k_u21.l1", CRC(08be36ad) SHA1(0f4c448e003df54ed8ccf0e0c57f6123ce1e2027),
                        "bk2k_u22.l1", CRC(9c8becd8) SHA1(9090e8104dad63f14246caabafec428d94d5e18d))
S11CS_SOUNDROM88(       "bk2k_u4.l2",  CRC(1d87281b) SHA1(609288b017aac6ce6da8717a35fdf87013adeb3c),
                        "bk2k_u19.l1", CRC(58e162b2) SHA1(891f810ae18b46593f570d719f0290a1d08a1a10))
S11_ROMEND
#define input_ports_bk2k input_ports_s11
CORE_GAMEDEF(bk2k, l4, "Black Knight 2000 (L-4)", 1989, "Williams", s11_mS11BS,0)

S11_ROMSTART48(bk2k,lg1,"bk2kgu26.lg1", CRC(f916d163) SHA1(bd8cbac9345a8debd01c8c68110652f591ad9d51),
                        "bk2kgu27.lg1", CRC(4132ac5c) SHA1(5636d4e8fb9bf5a5f4ccafe4ef035ab0e8964e8b))
S11XS_SOUNDROM88(       "bk2k_u21.l1", CRC(08be36ad) SHA1(0f4c448e003df54ed8ccf0e0c57f6123ce1e2027),
                        "bk2k_u22.l1", CRC(9c8becd8) SHA1(9090e8104dad63f14246caabafec428d94d5e18d))
S11CS_SOUNDROM88(       "bk2k_u4.l2",  CRC(1d87281b) SHA1(609288b017aac6ce6da8717a35fdf87013adeb3c),
                        "bk2k_u19.l1", CRC(58e162b2) SHA1(891f810ae18b46593f570d719f0290a1d08a1a10))
S11_ROMEND
CORE_CLONEDEF(bk2k,lg1,l4, "Black Knight 2000 (LG-1)", 1989, "Williams", s11_mS11BS,0)

S11_ROMSTART48(bk2k,lg3,"u26-lg3.rom", CRC(6f468c85) SHA1(b919b436559a29c43911bd2839c5ae7c03e9b06f),
                        "u27-lg3.rom", CRC(27707522) SHA1(37844e2f3c70430ee169e1c369aa8e9d47b2c8f2))
S11XS_SOUNDROM88(       "bk2k_u21.l1", CRC(08be36ad) SHA1(0f4c448e003df54ed8ccf0e0c57f6123ce1e2027),
                        "bk2k_u22.l1", CRC(9c8becd8) SHA1(9090e8104dad63f14246caabafec428d94d5e18d))
S11CS_SOUNDROM88(       "bk2k_u4.l2",  CRC(1d87281b) SHA1(609288b017aac6ce6da8717a35fdf87013adeb3c),
                        "bk2k_u19.l1", CRC(58e162b2) SHA1(891f810ae18b46593f570d719f0290a1d08a1a10))
S11_ROMEND
CORE_CLONEDEF(bk2k,lg3,l4,"Black Knight 2000 (LG-3)", 1989, "Williams", s11_mS11BS,0)

S11_ROMSTART48(bk2k,pu1,"u26-pu1.rom", CRC(2da07403) SHA1(4b48c5d7b0a03aa4593dc6053dc5e94df22d2a64),
                        "u27-pu1.rom", CRC(245efbae) SHA1(e6354a6f5029f21aab2343cd90daf6cbfb51e556))
S11XS_SOUNDROM88(       "bk2k_u21.l1", CRC(08be36ad) SHA1(0f4c448e003df54ed8ccf0e0c57f6123ce1e2027),
                        "bk2k_u22.l1", CRC(9c8becd8) SHA1(9090e8104dad63f14246caabafec428d94d5e18d))
S11CS_SOUNDROM88(       "bk2k_u4.l2",  CRC(1d87281b) SHA1(609288b017aac6ce6da8717a35fdf87013adeb3c),
                        "bk2k_u19.l1", CRC(58e162b2) SHA1(891f810ae18b46593f570d719f0290a1d08a1a10))
S11_ROMEND
CORE_CLONEDEF(bk2k,pu1,l4,"Black Knight 2000 (PU-1)", 1989, "Williams", s11_mS11BS,0)

/*-----------------------
/ Police Force 9/89
/-----------------------*/
static core_tLCDLayout dispPolic[] = {
  { 0,8,20, 8,CORE_SEG8H },
  { 2,0, 0,16,CORE_SEG16 },
  { 4,0,20,16,CORE_SEG8},{0}
};
INITGAME(polic,GEN_S11B, dispPolic, 12, FLIP_SWNO(58,57), S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(polic,l4,"pfrc_u26.l4", CRC(1a1409e9) SHA1(775d35a22483bcf8c4b03841e0aca22b6504a48f),
                        "pfrc_u27.l4", CRC(641ed5d4) SHA1(f98b8bb64184aba062715555bd1de679d6382ac3))
S11XS_SOUNDROM88(       "pfrc_u21.l1", CRC(7729afd3) SHA1(9cd2898a7a4203cf3b2dcd203e25cde5dd582ee7),
                        "pfrc_u22.l1", CRC(40f5e6b2) SHA1(4af2e2658720b08d03d24c9d314a6e5074b2c747))
S11CS_SOUNDROM88(       "pfrc_u4.l2",  CRC(8f431529) SHA1(0f479990715a31fd860c000a066cffb70da502c2),
                        "pfrc_u19.l1", CRC(abc4caeb) SHA1(6faef2de9a49a1015b4038ab18849de2f25dbded))
S11_ROMEND
#define input_ports_polic input_ports_s11
CORE_GAMEDEF(polic,l4, "Police Force (LA-4)", 1989, "Williams", s11_mS11BS,0)

S11_ROMSTART48(polic,l3,"pfrc_u26.l4", CRC(1a1409e9) SHA1(775d35a22483bcf8c4b03841e0aca22b6504a48f),
                        "pfrc_u27.lx3",CRC(ef5d4808) SHA1(89cf62640e39397899776ab1d132645a5eab9e0e))
S11XS_SOUNDROM88(       "pfrc_u21.l1", CRC(7729afd3) SHA1(9cd2898a7a4203cf3b2dcd203e25cde5dd582ee7),
                        "pfrc_u22.l1", CRC(40f5e6b2) SHA1(4af2e2658720b08d03d24c9d314a6e5074b2c747))
S11CS_SOUNDROM88(       "pfrc_u4.l2",  CRC(8f431529) SHA1(0f479990715a31fd860c000a066cffb70da502c2),
                        "pfrc_u19.l1", CRC(abc4caeb) SHA1(6faef2de9a49a1015b4038ab18849de2f25dbded))
S11_ROMEND
CORE_CLONEDEF(polic,l3,l4,"Police Force (LA-3)", 1989, "Williams", s11_mS11BS,0)

S11_ROMSTART48(polic,l2,"pfrc_u26.l2", CRC(4bc972dc) SHA1(7d6e421945832bd2c95a7b8e27d5573a42109379),
                        "pfrc_u27.l2", CRC(46ae36f2) SHA1(6685efa858a14b21fae5e3192ab714750ff51341))
S11XS_SOUNDROM88(       "pfrc_u21.l1", CRC(7729afd3) SHA1(9cd2898a7a4203cf3b2dcd203e25cde5dd582ee7),
                        "pfrc_u22.l1", CRC(40f5e6b2) SHA1(4af2e2658720b08d03d24c9d314a6e5074b2c747))
S11CS_SOUNDROM88(       "pfrc_u4.l2",  CRC(8f431529) SHA1(0f479990715a31fd860c000a066cffb70da502c2),
                        "pfrc_u19.l1", CRC(abc4caeb) SHA1(6faef2de9a49a1015b4038ab18849de2f25dbded))
S11_ROMEND
CORE_CLONEDEF(polic,l2,l4,"Police Force (LA-2)", 1989, "Williams", s11_mS11BS,0)

/*-----------------------------
/ Transporter the Rescue 6/89
/----------------------------*/
INITGAME(tsptr,GEN_S11B,disp16oneline,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(tsptr,l3,"tran_u26.l3", CRC(2d48a108) SHA1(d41bf077aab1201b08ea14725d4a0d841ee6b919),
                        "tran_u27.l3", CRC(50efb01c) SHA1(941f18d51bf8a5d209ed90e0865b7fa638a6eab3))
S11XS_SOUNDROM88(       "tran_u21.l2", CRC(b10120ee) SHA1(305a898a8b762c27dba26921ef169556bf96e518),
                        "tran_u22.l2", CRC(337784b5) SHA1(30c17afd8f76118940982db946cd3a2a29445d10))
S11CS_SOUNDROM888(      "tran_u4.l2",  CRC(a06ddd61) SHA1(630fe7ab94516930c4876a95f822024a44371170),
                        "tran_u19.l2", CRC(3cfde8b0) SHA1(7bdc71ba1ba4fd337f052354323c86fd97b2b881),
                        "tran_u20.l2", CRC(e9890cf1) SHA1(0ae37504c704401101c79ce49df11044f8d8caa9))

S11_ROMEND
#define input_ports_tsptr input_ports_s11
CORE_GAMEDEF(tsptr,l3, "Transporter the Rescue (L-3)", 1989, "Bally", s11a_one,0)

/*-----------------------
/ Bad Cats 12/89
/-----------------------*/
INITGAME(bcats,GEN_S11B,s11_dispS11b2,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(bcats,l5,"cats_u26.l5", CRC(32246d12) SHA1(b8aa89d197a6b992501904f5072a10ab1a31db87),
                        "cats_u27.l5", CRC(ef842bbf) SHA1(854860db428795d5de5c075aa78496f0c18a380f))
S11XS_SOUNDROM88(       "cats_u21.l1", CRC(04110d08) SHA1(4b44b26983cb5d14a93c16a19dc2bdbaa665dc69),
                        "cats_u22.l1", CRC(7e152c78) SHA1(b4ab770fdd9420a5d35e55bf8fb84c99ac544b8b))
S11CS_SOUNDROM888(      "cats_u4.l1",  CRC(18c62813) SHA1(a4fb69cfedd0b92c22b599913df3cdf8b3eef42c),
                        "cats_u19.l1", CRC(f2fea68b) SHA1(9a41823e71342b7a162420378f122bba34ce0636),
                        "cats_u20.l1", CRC(bf4dc35a) SHA1(9920ce90d93fb6ecf98792c35bb6eb8862a969f3))
S11_ROMEND
#define input_ports_bcats input_ports_s11

S11_ROMSTART48(bcats,l2,"bcgu26.la2", CRC(206c7cf8) SHA1(34eb128d46a0e1ba943f4e37aa95fa6d81aefb0e),
                        "bcgu27.la2", CRC(911981c6) SHA1(0d5b5c6d8399c6337300c789a0466242f91eaf94))
S11XS_SOUNDROM88(       "cats_u21.l1", CRC(04110d08) SHA1(4b44b26983cb5d14a93c16a19dc2bdbaa665dc69),
                        "cats_u22.l1", CRC(7e152c78) SHA1(b4ab770fdd9420a5d35e55bf8fb84c99ac544b8b))
S11CS_SOUNDROM888(      "cats_u4.l1",  CRC(18c62813) SHA1(a4fb69cfedd0b92c22b599913df3cdf8b3eef42c),
                        "cats_u19.l1", CRC(f2fea68b) SHA1(9a41823e71342b7a162420378f122bba34ce0636),
                        "cats_u20.l1", CRC(bf4dc35a) SHA1(9920ce90d93fb6ecf98792c35bb6eb8862a969f3))
S11_ROMEND

CORE_GAMEDEF(bcats,l5, "Bad Cats (L-5)", 1989, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(bcats,l2,l5,"Bad Cats (LA-2)", 1989, "Williams", s11_mS11BS,0)

/*-----------------------
/ Mousin' Around 12/89
/-----------------------*/
INITGAME(mousn,GEN_S11B,disp16oneline,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(mousn,l4,"mous_u26.l4", CRC(a540edc1) SHA1(c0b208369ac770f0d4cd7decfce5f8401ded082f),
                        "mous_u27.l4", CRC(ff108148) SHA1(32b44286d43a39d5677c6582c5b09fc3b9833806))
S11XS_SOUNDROM88(       "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7),
                        "mous_u22.l1", CRC(00ad198c) SHA1(4f15696909e1f3574ad20b28e31da2c155ed129f))
S11CS_SOUNDROM888(      "mous_u4.l2",  CRC(643add1e) SHA1(45dea0f4c6f24d17e6f7dda75afaa7caefdc6b96),
                        "mous_u19.l2", CRC(7b4941f7) SHA1(2b2fc8e7634b1885b020b2115126d6341172cc91),
                        "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7))
S11_ROMEND
#define input_ports_mousn input_ports_s11

S11_ROMSTART48(mousn,l1,"u26-la1.rom", CRC(0fff7946) SHA1(53bd68fd21218128f9311047ac911cff7eea8b23),
                        "u27-la1.rom", CRC(a440192b) SHA1(837a9eb290f46d792f7307c569dfc627507420b8))
S11XS_SOUNDROM88(       "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7),
                        "mous_u22.l1", CRC(00ad198c) SHA1(4f15696909e1f3574ad20b28e31da2c155ed129f))
S11CS_SOUNDROM888(      "mous_u4.l2",  CRC(643add1e) SHA1(45dea0f4c6f24d17e6f7dda75afaa7caefdc6b96),
                        "mous_u19.l2", CRC(7b4941f7) SHA1(2b2fc8e7634b1885b020b2115126d6341172cc91),
                        "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7))
S11_ROMEND

S11_ROMSTART48(mousn,lu,"u26-la1.rom", CRC(0fff7946) SHA1(53bd68fd21218128f9311047ac911cff7eea8b23),
                        "u27-lu1.rom", CRC(6e5b692c) SHA1(20c4b8d105d5df6e1b540c02c1c54bca08ec42e8))
S11XS_SOUNDROM88(       "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7),
                        "mous_u22.l1", CRC(00ad198c) SHA1(4f15696909e1f3574ad20b28e31da2c155ed129f))
S11CS_SOUNDROM888(      "mous_u4.l2",  CRC(643add1e) SHA1(45dea0f4c6f24d17e6f7dda75afaa7caefdc6b96),
                        "mous_u19.l2", CRC(7b4941f7) SHA1(2b2fc8e7634b1885b020b2115126d6341172cc91),
                        "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7))
S11_ROMEND

S11_ROMSTART48(mousn,lx,"mous_u26.l4", CRC(a540edc1) SHA1(c0b208369ac770f0d4cd7decfce5f8401ded082f),
                        "mous_u27.l4", CRC(ff108148) SHA1(32b44286d43a39d5677c6582c5b09fc3b9833806))
S11XS_SOUNDROM88(       "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7),
                        "mous_u22.l1", CRC(00ad198c) SHA1(4f15696909e1f3574ad20b28e31da2c155ed129f))
S11CS_SOUNDROM888(      "mous_u4.lx",  CRC(d311db4a) SHA1(d9d20921eb42c19c5074c976608bfec0d3130204),
                        "mous_u19.lx", CRC(c7a6f494) SHA1(272f0bd3885bb81da13ee6ed3d66f9424ccf4b0d),
                        "mous_u20.l2", CRC(59b1b0c5) SHA1(443426be41c1413f22b137145dbc3bcf84d9ccc7))
S11_ROMEND

CORE_GAMEDEF(mousn,l4,"Mousin' Around (LA-4)", 1989, "Bally", s11a_one,0)
CORE_CLONEDEF(mousn,l1,l4,"Mousin' Around (LA-1)", 1989, "Bally", s11a_one,0)
CORE_CLONEDEF(mousn,lu,l4,"Mousin' Around (LU-1)", 1989, "Bally", s11a_one,0)
CORE_CLONEDEF(mousn,lx,l4,"Mousin' Around (LX-1)", 1989, "Bally", s11a_one,0)

/*-----------------------
/ Whirlwind 4/90
/-----------------------*/
INITGAME(whirl,GEN_S11B,s11_dispS11b2,12, FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2|S11_SNDOVERLAY)
S11_ROMSTART48(whirl,l3,"whir_u26.l3", CRC(066b8fec) SHA1(017ca12ef5ebd9bb70690b0e096064be5144a512),
                        "whir_u27.l3", CRC(47fc033d) SHA1(42518650ecb538323bc33ee193bc229d89ca1936))
S11XS_SOUNDROM88(       "whir_u21.l1", CRC(fa3da322) SHA1(732107eace9eecdb97eff4abb4420a2febef7425),
                        "whir_u22.l1", CRC(fcaf8c4e) SHA1(8e8cab1923a56bcef4671dce28aef1e39303c04a))
S11CS_SOUNDROM888(      "whir_u4.l1",  CRC(29952d84) SHA1(26479a341b0552c5f9d9bf9dd013855e51a7b857),
                        "whir_u19.l1", CRC(c63f6fe9) SHA1(947bbccb5eeae414770254d42d0a95425e2dca8c),
                        "whir_u20.l1", CRC(713007af) SHA1(3ac88bb905ccf8e227bbf3c102c74e3d2446cc88))
S11_ROMEND
#define input_ports_whirl input_ports_s11

S11_ROMSTART48(whirl,l2,"whir_u26.l3", CRC(066b8fec) SHA1(017ca12ef5ebd9bb70690b0e096064be5144a512),
                        "wwdgu27.l2", CRC(d8fb48f3) SHA1(8c64f94cca51abd6f4a7e53ac59a6f623bd2cfd7))
S11XS_SOUNDROM88(       "whir_u21.l1", CRC(fa3da322) SHA1(732107eace9eecdb97eff4abb4420a2febef7425),
                        "whir_u22.l1", CRC(fcaf8c4e) SHA1(8e8cab1923a56bcef4671dce28aef1e39303c04a))
S11CS_SOUNDROM888(      "whir_u4.l1",  CRC(29952d84) SHA1(26479a341b0552c5f9d9bf9dd013855e51a7b857),
                        "whir_u19.l1", CRC(c63f6fe9) SHA1(947bbccb5eeae414770254d42d0a95425e2dca8c),
                        "whir_u20.l1", CRC(713007af) SHA1(3ac88bb905ccf8e227bbf3c102c74e3d2446cc88))
S11_ROMEND

CORE_GAMEDEF(whirl,l3, "Whirlwind (L-3)", 1990, "Williams", s11_mS11BS,0)
CORE_CLONEDEF(whirl,l2,l3,"Whirlwind (L-2)", 1990, "Williams", s11_mS11BS,0)

/*--------------------
/ Game Show 4/90
/--------------------*/
INITGAME(gs   ,GEN_S11C,disp16oneline,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(gs,l3,"gshw_u26.l3", CRC(3419bfb2) SHA1(7ce294a3118d20c7cdc3d5cd946e4c43090c5151),
                        "gshw_u27.l3", CRC(4f3babb6) SHA1(87091a6786fc6817529cfed7f60396babe153d8d))
S11CS_SOUNDROM000(      "gshw_u4.l2",  CRC(e89e0116) SHA1(e96bee143d1662d078f21531f405d838fdace693),
                        "gshw_u19.l1", CRC(8bae0813) SHA1(a2b1beca13796892d8ee1533e395cabdbbb11f88),
                        "gshw_u20.l1", CRC(75ccbdf7) SHA1(7dce8ae427a621919caad8d8b08b06bb0adad850))
S11_ROMEND
#define input_ports_gs input_ports_s11

S11_ROMSTART48(gs,l4,"gshw_u26.l3", CRC(3419bfb2) SHA1(7ce294a3118d20c7cdc3d5cd946e4c43090c5151),
                        "u27-lu4.rom", CRC(ba265978) SHA1(66ac8e83e35cdfd72f1d3aa8ce6d92c2c833f304))
S11CS_SOUNDROM000(      "gshw_u4.l2",  CRC(e89e0116) SHA1(e96bee143d1662d078f21531f405d838fdace693),
                        "gshw_u19.l1", CRC(8bae0813) SHA1(a2b1beca13796892d8ee1533e395cabdbbb11f88),
                        "gshw_u20.l1", CRC(75ccbdf7) SHA1(7dce8ae427a621919caad8d8b08b06bb0adad850))
S11_ROMEND

CORE_CLONEDEF(gs,l3,l4,"Game Show (L-3)", 1990, "Bally", s11c_one,0)
CORE_GAMEDEF(gs,l4,"Game Show (L-4)", 1990, "Bally", s11c_one,0)

/*--------------------
/ Rollergames 5/90
/--------------------*/
INITGAME(rollr,GEN_S11C,s11_dispS11c,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(rollr,l2,"rolr_u26.l2", CRC(cd7cad9e) SHA1(e381fa73895c307a0b3b4b699cfec2a68908f6f7),
                        "rolr_u27.l2", CRC(f3bac2b8) SHA1(9f0ff32ea83e43097de42065909137a362b29d49))
S11CS_SOUNDROM000(      "rolr_u4.l3",  CRC(d366c705) SHA1(76018305b5040b2e5d8c45cc81a18f13e1a8f8da),
                        "rolr_u19.l3", CRC(45a89e55) SHA1(3aff897514d242c83a8e7575d430d594a873736e),
                        "rolr_u20.l3", CRC(77f89aff) SHA1(dcd9fe233f33ef8f97cdeaaa365532e485a28944))
S11_ROMEND
#define input_ports_rollr input_ports_s11
CORE_GAMEDEF(rollr, l2, "Rollergames (L-2)", 1990, "Williams", s11_mS11CS,0)

S11_ROMSTART48(rollr,ex,"rolr-u26.ea3", CRC(78c3c1ad) SHA1(04e4370548b3ba85c49634402a0ea166e3643f68),
                        "rolr_u27.ea3", CRC(18685158) SHA1(d1a79fbe1185fb9e1ae1d9e2b2751429f487bb4c))
S11CS_SOUNDROM000(      "rolr_u4.l3",  CRC(d366c705) SHA1(76018305b5040b2e5d8c45cc81a18f13e1a8f8da),
                        "rolr_u19.l3", CRC(45a89e55) SHA1(3aff897514d242c83a8e7575d430d594a873736e),
                        "rolr_u20.l3", CRC(77f89aff) SHA1(dcd9fe233f33ef8f97cdeaaa365532e485a28944))
S11_ROMEND
CORE_CLONEDEF(rollr,ex,l2, "Rollergames (EXPERIMENTAL)", 1991, "Williams", s11_mS11CS,0)

S11_ROMSTART48(rollr,e1,"rolr_u26.pe1", CRC(56620505) SHA1(2df9097e52178f246148a40e0ad4a6e6a5cdb5d4),
                        "rolr_u27.pe1", CRC(724d0af2) SHA1(5de5596f4e594c0e6b8448817de6ff46ffc7194b))
S11CS_SOUNDROM000(      "rolr_u4.pe1",  CRC(8c383b24) SHA1(5c738e5ec566f7fa5706cd4c33e5d706fa76c72d),
                        "rolr_u19.pe1", CRC(c6880cff) SHA1(c8ce23d68297d36ef62e508855a478434ff9a592),
                        "rolr_u20.pe1", CRC(4220812b) SHA1(7071565f1087020d1e1738e801dafb509ea37622))
S11_ROMEND
CORE_CLONEDEF(rollr,e1,l2, "Rollergames (PU-1)", 1991, "Williams", s11_mS11CS,0)

S11_ROMSTART48(rollr,p2,"rolr_u26.pa2", CRC(11d96b1c) SHA1(e96991bdef8b14043285feeb4cacc182a6e9dcbd),
                        "rolr_u27.pa2", CRC(ee547bd5) SHA1(db45bf7a25321ac041f58404f7512bded9ebf11e))
S11CS_SOUNDROM000(      "rolr_u4.pa1",  CRC(324df946) SHA1(e7ba2b9434baea20a0cf38540fdab1668c058539),
                        "rolr_u19.pa1", CRC(45a89e55) SHA1(3aff897514d242c83a8e7575d430d594a873736e),
                        "rolr_u20.pa1", CRC(8ddaaad1) SHA1(33f58c6a9b0e509b7c9a460a687d6e2c388b4b54))
S11_ROMEND
CORE_CLONEDEF(rollr,p2,l2, "Rollergames (PA-2, PA-1 Sound)", 1991, "Williams", s11_mS11CS,0)

S11_ROMSTART48(rollr,l3,"rolr-u26.lu3", CRC(7d71ed50) SHA1(092aa13706a7fe58ad80e88c1c4a5c1d7d712546),
                        "rolr_u27.l2", CRC(f3bac2b8) SHA1(9f0ff32ea83e43097de42065909137a362b29d49))
S11CS_SOUNDROM000(      "rolr_u4.l3",  CRC(d366c705) SHA1(76018305b5040b2e5d8c45cc81a18f13e1a8f8da),
                        "rolr_u19.l3", CRC(45a89e55) SHA1(3aff897514d242c83a8e7575d430d594a873736e),
                        "rolr_u20.l3", CRC(77f89aff) SHA1(dcd9fe233f33ef8f97cdeaaa365532e485a28944))
S11_ROMEND
CORE_CLONEDEF(rollr,l3,l2, "Rollergames (LU-3) Europe", 1990, "Williams", s11_mS11CS,0)

S11_ROMSTART48(rollr,g3,"rolr-u26.lg3", CRC(438d2b94) SHA1(f507a06794563701b6d4fc51ff90a42a6d21d060),
                        "rolr_u27.l2", CRC(f3bac2b8) SHA1(9f0ff32ea83e43097de42065909137a362b29d49))
S11CS_SOUNDROM000(      "rolr_u4.l3",  CRC(d366c705) SHA1(76018305b5040b2e5d8c45cc81a18f13e1a8f8da),
                        "rolr_u19.l3", CRC(45a89e55) SHA1(3aff897514d242c83a8e7575d430d594a873736e),
                        "rolr_u20.l3", CRC(77f89aff) SHA1(dcd9fe233f33ef8f97cdeaaa365532e485a28944))
S11_ROMEND
CORE_CLONEDEF(rollr,g3,l2, "Rollergames (LG-3) Germany", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Pool Sharks 6/90
/--------------------*/
INITGAME(pool ,GEN_S11C,disp16oneline,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2|S11_SNDDELAY)
S11_ROMSTART48(pool ,l7,"pool_u26.l7", CRC(cee98aed) SHA1(5b652684c10ab4945783089d848b2f663d3b2547),
                        "pool_u27.l7", CRC(356d9a89) SHA1(ce795c535d03a14d28fb3f2071cae48ccdb1a856))
S11CS_SOUNDROM000(      "pool_u4.l2",  CRC(04e95e10) SHA1(3873b3cd6c2961b3f2f28a1e17f8a63c6db808d2),
                        "pool_u19.l2", CRC(0f45d02b) SHA1(58bbfdb3b98c43b66e11808cec7cd65a7f2dce6d),
                        "pool_u20.l2", CRC(925f62d6) SHA1(21b8d6f9a8b98fce8a3cdf7f5f2d40200544a898))
S11_ROMEND
#define input_ports_pool input_ports_s11
CORE_GAMEDEF(pool , l7, "Pool Sharks (L-7)", 1990, "Bally", s11c_one,0)

/*--------------------
/ Diner 8/90
/--------------------*/
INITGAME(diner,GEN_S11C,s11_dispS11c,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(diner,l4,"dinr_u26.l4", CRC(6f187abf) SHA1(8acabbccdf3528a9c5e60cc8939ab960bf4c5512),
                        "dinr_u27.l4", CRC(d69f9f74) SHA1(88d9b42c2313a90e5d6f50220d3b44331595d86b))
S11CS_SOUNDROM000(      "dinr_u4.l1",  CRC(3bd28368) SHA1(41eec2f5f863039deaabfae8aece4b1cf15e4b78),
                        "dinr_u19.l1", CRC(278b9a30) SHA1(41e59adb8b6c08caee46c3dd73256480b4041619),
                        "dinr_u20.l1", CRC(511fb260) SHA1(e6e25b464c5c38f3c0492436f1e8aa2be33dd278))
S11_ROMEND
#define input_ports_diner input_ports_s11

S11_ROMSTART48(diner,l3,"u26-la3.rom", CRC(8b6aa22e) SHA1(6b802a85fc2babf5a183fb434df11597363c1c9d),
                        "u27-la3.rom", CRC(4171451a) SHA1(818e330245691d9ef3181b885c9342880f89d912))
S11CS_SOUNDROM000(      "dinr_u4.l1",  CRC(3bd28368) SHA1(41eec2f5f863039deaabfae8aece4b1cf15e4b78),
                        "dinr_u19.l1", CRC(278b9a30) SHA1(41e59adb8b6c08caee46c3dd73256480b4041619),
                        "dinr_u20.l1", CRC(511fb260) SHA1(e6e25b464c5c38f3c0492436f1e8aa2be33dd278))
S11_ROMEND

S11_ROMSTART48(diner,l1,"u26-lu1.rom", CRC(259b302f) SHA1(d7e19c2d2ad7805d9158178c24d180d158a59b0c),
                        "u27-lu1.rom", CRC(35fafbb3) SHA1(0db3d0c9421f4fdcf4d376d543626559e1bf2daa))
S11CS_SOUNDROM000(      "dinr_u4.l1",  CRC(3bd28368) SHA1(41eec2f5f863039deaabfae8aece4b1cf15e4b78),
                        "dinr_u19.l1", CRC(278b9a30) SHA1(41e59adb8b6c08caee46c3dd73256480b4041619),
                        "dinr_u20.l1", CRC(511fb260) SHA1(e6e25b464c5c38f3c0492436f1e8aa2be33dd278))
S11_ROMEND

CORE_GAMEDEF(diner, l4, "Diner (L-4)", 1990, "Williams", s11_mS11CS,0)
CORE_CLONEDEF(diner,l3,l4, "Diner (L-3)", 1990, "Williams", s11_mS11CS,0)
CORE_CLONEDEF(diner,l1,l4, "Diner (L-1) Europe", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Radical 9/90
/--------------------*/
INITGAME(radcl,GEN_S11C,disp16oneline,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(radcl,l1,"rad_u26.l1", CRC(84b1a125) SHA1(dd01fb9189acd2620c57149921aadb051f7a2412),
                        "rad_u27.l1", CRC(6f6ca382) SHA1(a61055aab97d3fe2ecd0ed4281a9681b1d910269))
S11CS_SOUNDROM008(      "rad_u4.l1",  CRC(5aafc09c) SHA1(27984bbc91dc7593e6a5b42f74dd6ddf58189bec),
                        "rad_u19.l1", CRC(7c005e1f) SHA1(bdeea7517f2adf72b4b642bffb25ba5b98453127),
                        "rad_u20.l1", CRC(05b96292) SHA1(7da0289cf0a0c93768c0706fdedfc3a5f2101e77))
S11_ROMEND
#define input_ports_radcl input_ports_s11
CORE_GAMEDEF(radcl, l1, "Radical (L-1)", 1990, "Bally", s11c_one,0)

S11_ROMSTART48(radcl,g1,"rad_u26.l1", CRC(84b1a125) SHA1(dd01fb9189acd2620c57149921aadb051f7a2412),
                        "u27-lg1.rom", CRC(4f2eca4b) SHA1(ff44deded1686cfa0351c4499485d6eb4561cbc1))
S11CS_SOUNDROM008(      "rad_u4.l1",  CRC(5aafc09c) SHA1(27984bbc91dc7593e6a5b42f74dd6ddf58189bec),
                        "rad_u19.l1", CRC(7c005e1f) SHA1(bdeea7517f2adf72b4b642bffb25ba5b98453127),
                        "rad_u20.l1", CRC(05b96292) SHA1(7da0289cf0a0c93768c0706fdedfc3a5f2101e77))
S11_ROMEND
CORE_CLONEDEF(radcl,g1,l1, "Radical (G-1)", 1990, "Bally", s11c_one,0)

S11_ROMSTART48(radcl,p3,"rad_u26.p1", CRC(7d736ae9) SHA1(4ea6945fa5cfbd33fcdf780814b0bf5cb3faa388),
                        "u27-p1.rom", CRC(83b1d928) SHA1(b1bd5d8a93f1ab9fb9bf5c268d8530be438448e6))
S11CS_SOUNDROM008(      "rad_u4.p3",  CRC(d31b7744) SHA1(7ebcc1503fc322909d32c7c8bda8c0b6505919b3),
                        "rad_u19.l1", CRC(7c005e1f) SHA1(bdeea7517f2adf72b4b642bffb25ba5b98453127),
                        "rad_u20.p3", CRC(82f8369c) SHA1(0691a80672fc11d46359f710bd211de7a59de346))
S11_ROMEND
CORE_CLONEDEF(radcl,p3,l1, "Radical (P-3)", 1990, "Bally", s11c_one,0)

/*-----------------------
/ Star Trax 9/90
/-----------------------*/
INITGAME(strax,GEN_S11B, s11_dispS11b2, 12, FLIP_SW(FLIP_L),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(strax,p7,"strx_u26.p7", CRC(0d2a401c) SHA1(b0a0899dcde04dc42e4fd5d6baf39bb0e81dbb34),
                        "strx_u27.p7", CRC(6e9c0632) SHA1(5c0ea2b60dd9001b802d2ecdb5c381ab05f08ec9))
S11XS_SOUNDROM88(       "strx_u21.l1", CRC(6a323227) SHA1(7c7263754e5672c654a2ee9582f0b278e637a909),
                        "strx_u22.l1", CRC(58407eb4) SHA1(6bd9b304c88d9470eae5afb6621187f4a8313573))
S11CS_SOUNDROM88(       "pfrc_u4.l2",  CRC(8f431529) SHA1(0f479990715a31fd860c000a066cffb70da502c2),
                        "pfrc_u19.l1", CRC(abc4caeb) SHA1(6faef2de9a49a1015b4038ab18849de2f25dbded))
S11_ROMEND
#define input_ports_strax input_ports_s11
CORE_GAMEDEF(strax,p7, "Star Trax (domestic prototype)", 1990, "Williams", s11_mS11BS,0)

/*--------------------
/ Riverboat Gambler 10/90
/--------------------*/
static core_tLCDLayout dispRvrbt[] = {
  { 0,18,21, 7, CORE_SEG87H },
  { 0, 4,32, 4, CORE_SEG7H },
  { 2, 0, 0,16, CORE_SEG16 },
  { 4, 0,20,16, CORE_SEG8 },{0}
};
INITGAME(rvrbt,GEN_S11C,dispRvrbt,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(rvrbt,l3,"gamb_u26.l3", CRC(a65f6004) SHA1(ea44bb7f8f2ec9e5989be63ba41f674b14d19b8a),
                        "gamb_u27.l3", CRC(9be0f613) SHA1(1c2b442bc3daef212fe23ff03f5409c354e79989))
S11CS_SOUNDROM000(      "gamb_u4.l2",  CRC(c0cfa9be) SHA1(352df9a4dcbc131ae249416e9e517137a04627ba),
                        "gamb_u19.l1", CRC(04a3a8c8) SHA1(e72ef767f13282d2335cda3288037610d9bfedf2),
                        "gamb_u20.l1", CRC(a60c734d) SHA1(76cfcf96276ca4f6b5eee0e0402fab5ee9685366))
S11_ROMEND
#define input_ports_rvrbt input_ports_s11
CORE_GAMEDEF(rvrbt, l3, "Riverboat Gambler (L-3)", 1990, "Williams", s11_mS11CS,0)

/*--------------------
/ Bugs Bunny Birthday Ball 11/90
/--------------------*/
INITGAME(bbnny,GEN_S11C,disp16oneline,12,FLIP_SWNO(58,57),S11_LOWALPHA|S11_DISPINV,S11_MUXSW2)
S11_ROMSTART48(bbnny,l2,"bugs_u26.l2", CRC(b4358920) SHA1(93af1cf5dc2b5442f428a621c0f73b27c197a3df),
                        "bugs_u27.l2", CRC(8ff29439) SHA1(8fcdcea556e9e01ea8cb7c1548f98af2467c8a5f))
S11CS_SOUNDROM000(      "bugs_u4.l2",  CRC(04bc9aa5) SHA1(c3da2dc3e26b88a0ebc6f87e61fc71bec45330c3),
                        "bugs_u19.l1", CRC(a2084702) SHA1(ffd749387e7b52bad1e98c6a8939fb87bc67524c),
                        "bugs_u20.l1", CRC(5df734ef) SHA1(c8d153444dd6171c3ebddc8100ab06fde3373cc6))
S11_ROMEND
#define input_ports_bbnny input_ports_s11
CORE_GAMEDEF(bbnny, l2, "Bugs Bunny Birthday Ball (L-2)", 1990, "Bally", s11c_one,0)

S11_ROMSTART48(bbnny,lu,"bugs_u26.l2", CRC(b4358920) SHA1(93af1cf5dc2b5442f428a621c0f73b27c197a3df),
                        "u27-lu2.rom", CRC(aaa2c82d) SHA1(b279c87cb2ac90a818eeb1afa6115b8cdab1b0df))
S11CS_SOUNDROM000(      "bugs_u4.l2",  CRC(04bc9aa5) SHA1(c3da2dc3e26b88a0ebc6f87e61fc71bec45330c3),
                        "bugs_u19.l1", CRC(a2084702) SHA1(ffd749387e7b52bad1e98c6a8939fb87bc67524c),
                        "bugs_u20.l1", CRC(5df734ef) SHA1(c8d153444dd6171c3ebddc8100ab06fde3373cc6))
S11_ROMEND
CORE_CLONEDEF(bbnny,lu,l2,"Bugs Bunny Birthday Ball (LU-2) European", 1990, "Bally", s11c_one,0)

/*--------------------
/ Dr. Dude - moved to own simulator
/--------------------*/
