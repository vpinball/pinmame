#include "driver.h"
#include "sndbrd.h"
#include "core.h"
#include "sim.h"
#include "taito.h"
#include "taitos.h"

static const core_tLCDLayout dispTaito[] = {
  { 0, 0,  0, 6, CORE_SEG7 },
  { 3, 0,  6, 6, CORE_SEG7 },
  { 6, 0, 12, 6, CORE_SEG7 },
  { 9, 0, 18, 6, CORE_SEG7 },
  {13, 0, 24, 1, CORE_SEG7 }, {13,10, 25, 1, CORE_SEG7 },
  {0}
};

static const core_tLCDLayout dispTaito2[] = {
  { 0, 0,  0, 6, CORE_SEG7 },
  { 3, 0,  6, 6, CORE_SEG7 },
  { 6, 0, 12, 6, CORE_SEG7 },
  { 9, 0, 18, 6, CORE_SEG7 },
  {13, 0, 24, 1, CORE_SEG7 }, {13,10, 27, 1, CORE_SEG7 },
  {0}
};

TAITO_INPUT_PORTS_START(taito,1)        TAITO_INPUT_PORTS_END

#define INITGAME1(name,sb) \
static core_tGameData name##GameData = {0,dispTaito2,{FLIP_SW(FLIP_L),0,0,0,sb,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAME(name,sb) \
static core_tGameData name##GameData = {0,dispTaito,{FLIP_SW(FLIP_L),0,0,0,sb,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

//??/?? Criterium 77
//??/?? Voley Ball
//??/?? Apache!
//??/?? Football
//??/79 Hot Ball (B Eight Ball, 01/77)
//??/79 Shock (W Flash, 01/79)
//??/?? Sultan (G Sinbad, 05/78)

/*--------------------------------
/ Oba-Oba
/-------------------------------*/
INITGAME1(obaoba,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(obaoba,"ob1.bin",CRC(85cddf4f) SHA1(25c7146b0ec79740704d62878f113dd43918021b),
                          "ob2.bin",CRC(7a110b82) SHA1(67cb34603de689438ecae8877674f01273bc711f),
                          "ob3.bin",CRC(8f32a7c0) SHA1(378a5434d3f4fe1b07f0116f2558bda030d2258c))
TAITO_SOUNDROMS22("ob_s1.bin", CRC(812a362b) SHA1(22b5f5f2d467ca1b0ab55db2e01ef6579f8ee390),
                  "ob_s2.bin", CRC(f7dbb715) SHA1(70d1331612fe497f48520726c5f39accdcbdb205))
TAITO_ROMEND
#define input_ports_obaoba input_ports_taito
CORE_GAMEDEFNV(obaoba,"Oba-Oba",1980,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Drakor
/-------------------------------*/
INITGAME(drakor,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(drakor,"drakor1.bin",CRC(7ecf377b) SHA1(b55b0ae3b591768621553a2b0afd1a795b4d592b),
                          "drakor2.bin",CRC(91dbb199) SHA1(fa351462c5616f591b7705259dfe96e97eda5548),
                          "drakor3.bin",CRC(b0ba866e) SHA1(dfea60523578b8def310922d17f442a8a031bba1))
// NOT AVAILABLE
TAITO_SOUNDROMS22("drakors1.bin", NO_DUMP,
                  "drakors2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_drakor input_ports_taito
CORE_GAMEDEFNV(drakor,"Drakor",1980,"Taito",taito_sintetizador,0)

//??/?? Roman Victory
//??/?? Space Patrol

/*--------------------------------
/ Meteor
/-------------------------------*/
INITGAME(meteort,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART22_2(meteort,"meteor1.bin",CRC(301a9f94) SHA1(7619b975c13c65e8c57ca50e77dc6385c5c5be49),
                           "meteor2.bin",CRC(6d136853) SHA1(f8fa555570b877c37457d84c41b1efca08ead612),
                           "meteor3.bin",CRC(c818e889) SHA1(40350e168c0e19edd5a8d11f11d76ed6cc5e4169))
TAITO_SOUNDROMS2("meteo_s1.bin", CRC(23971d1e) SHA1(77b5b8855e28cdd9b31b7e33f61258716738d57d))
TAITO_ROMEND
#define input_ports_meteort input_ports_taito
CORE_GAMEDEFNV(meteort,"Meteor (Taito)",1980,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Fire Action
/-------------------------------*/
INITGAME(fireact,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(fireact,"fire1.bin",CRC(3059876d) SHA1(1ea214b592adb156c8e9df7fafa59d9ed059f112),
                           "fire2.bin",CRC(7906a193) SHA1(9555233f24f044972fd7267ba970108695f52fb1),
                           "fire3.bin",CRC(92135de4) SHA1(28b3b496ae8a404542fc2b0128f3f88229d91cba),
                           "fire4.bin",CRC(68de7753) SHA1(b829ddc7e94d00b854e9290acc034038a60a8c1d))
TAITO_SOUNDROMS22("fire_s1.bin", CRC(13bdd72a) SHA1(f271bfe61617293b28b1a8ea7da9035127870d6c),
				  "fire_s2.bin", CRC(b76bda3f) SHA1(be5dfa3caa3b29a40287d535d158599587af8c05))
TAITO_ROMEND
#define input_ports_fireact input_ports_taito
CORE_GAMEDEFNV(fireact,"Fire Action",1981,"Taito",taito_sintevox,0)

/*--------------------------------
/ Cavaleiro Negro
/-------------------------------*/
INITGAME(cavnegro,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(cavnegro,"cn1.bin",CRC(6b414089) SHA1(5f6042cc85a9319b3e34bdf39fd1f7feb5db0ec2),
                            "cn2.bin",CRC(9641f2e5) SHA1(4d7e522bd1d691901868abd191010b62a9032fda),
                            "cn3.bin",CRC(e1c5afd8) SHA1(0995325444ada4aa5cd19a90230bcad58c6cd072),
                            "cn4.bin",CRC(0cf4c1fa) SHA1(f0170da2c3fb138cc9f6c076a2d3f4fbf529e923))
TAITO_SOUNDROMS22("cn_s1.bin", CRC(aec5069a) SHA1(4ec1f1f054e010caf9ffdda60071f96ba772c01a),
                  "cn_s2.bin", CRC(a0508863) SHA1(b4f343ed48960048c6b2b36c5ce0bad0fdb7ac62))
TAITO_ROMEND
#define input_ports_cavnegro input_ports_taito
CORE_GAMEDEFNV(cavnegro,"Cavaleiro Negro",1981,"Taito",taito_sintevox,0)

/*--------------------------------
/ Sure Shot
/-------------------------------*/
INITGAME(sureshot,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(sureshot,"ssh1.bin",CRC(46b96e00) SHA1(2cdbc0994bf0ff55330988a07c078dd2364a304c),
                            "ssh2.bin",CRC(655a7ff2) SHA1(f57852cd37e7fd4d054ad0f7a26e07d5932ad419),
                            "ssh3.bin",CRC(4dec25d6) SHA1(314052b0f5d750411ed597bb0461e9e847ccc2df),
                            "ssh4.bin",CRC(ced8f9df) SHA1(ba6b50df3ad2cb28885542748a61777df2010d69))
TAITO_SOUNDROMS222("ssh_s1.bin", CRC(acb7e92f) SHA1(103da5c87d0f1e0444575193e760b667d42fea73),
                   "ssh_s2.bin", CRC(c1351b31) SHA1(a306ff7abe5b032cd05195200fc56a97c1d2eef3),
				   "ssh_s3.bin", CRC(5e7f5275) SHA1(48eb1a499d2485b317ad769d876ec4cd57980285))
TAITO_ROMEND
#define input_ports_sureshot input_ports_taito
CORE_GAMEDEFNV(sureshot,"Sure Shot",1981,"Taito",taito_sintevox,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Vegas
/-------------------------------*/
INITGAME(vegast,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(vegast,"vegas1.bin",CRC(be242895) SHA1(0528e9049e44b5ae7bba4a21ca5c0a2e5ffa4ca5),
                          "vegas2.bin",CRC(48169726) SHA1(282a322178e007df1183620dfcf3411bc67d8a0a),
                          "vegas3.bin",CRC(bd1fdbc3) SHA1(e184cec644b2d5cc05c3d458a06299359322df00),
                          "vegas4.bin",CRC(61f733a9) SHA1(a86ac621d81eb69a56706f9b0d49c0816f14a016))
TAITO_SOUNDROMS22("vegas_s1.bin", CRC(740bdd3e) SHA1(ed86bd65ac4b6d43f91a95d44d48b04adb631ee3),
                  "vegas_s2.bin", CRC(4250e02e) SHA1(5a67aac55728e6661d85e31b01a5263b9d4a22db))
TAITO_ROMEND
#define input_ports_vegast input_ports_taito
CORE_GAMEDEFNV(vegast,"Vegas (Taito)",1981,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Lady Luck
/-------------------------------*/
TAITO_ROMSTART2222(ladylukt,"vegas1.bin",CRC(be242895) SHA1(0528e9049e44b5ae7bba4a21ca5c0a2e5ffa4ca5),
                            "vegas2.bin",CRC(48169726) SHA1(282a322178e007df1183620dfcf3411bc67d8a0a),
                            "lluck3.bin",CRC(f22666f6) SHA1(2b92007cc4c91a2804d9f6229fa68be35be849ce),
                            "lluck4.bin",CRC(1715ee7e) SHA1(45677053f501d687d7482e70b7902a67d277eee9))
TAITO_SOUNDROMS22("vegas_s1.bin", CRC(740bdd3e) SHA1(ed86bd65ac4b6d43f91a95d44d48b04adb631ee3),
                  "vegas_s2.bin", CRC(4250e02e) SHA1(5a67aac55728e6661d85e31b01a5263b9d4a22db))
TAITO_ROMEND
#define init_ladylukt init_vegast
#define input_ports_ladylukt input_ports_vegast
CORE_CLONEDEFNV(ladylukt,vegast,"Lady Luck (Taito)",1981,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Cosmic
/-------------------------------*/
INITGAME(cosmic,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(cosmic,"cosmic1.bin",CRC(1864f295) SHA1(f92fb88a945a946536c50e6b6ccc99ef34f5cdb9),
                          "cosmic2.bin",CRC(818e8621) SHA1(4c1dbb1504487ef5c75ddcedf92c803739490806),
                          "cosmic3.bin",CRC(c3e0cf5d) SHA1(9b0a6174a1fcb8934a91679645b64b7d9abaa705),
                          "cosmic4.bin",CRC(09ed5ecd) SHA1(182f0b01b9dad229e1a323253b32105098bdcfe7))
TAITO_SOUNDROMS22("cosmc_s1.bin", CRC(09f082c1) SHA1(653d6f9f9cc62b46aa2df2fa8dd0ad4e1e9f7c49),
                  "cosmc_s2.bin", CRC(84b98b95) SHA1(1946856de6d1ae05888826416bef9bdb25d652ed))
TAITO_ROMEND
#define input_ports_cosmic input_ports_taito
CORE_GAMEDEFNV(cosmic,"Cosmic",1981,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Gemini 2000
/-------------------------------*/
INITGAME(gemini,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(gemini,"gemini1.bin",CRC(4f952799) SHA1(8433850945d020253090d829a70fba1c9f9eaa5c),
                          "gemini2.bin",CRC(8903ee53) SHA1(81f0c02872327b2b589001265f2761666bf45ba2),
                          "gemini3.bin",CRC(1f11b5e5) SHA1(043dd68e51428e9123cb3c50c499b87478062c86),
                          "gemini4.bin",CRC(cac64ea6) SHA1(eed32defaa03394395d7b9d7bbdc205004789337))
TAITO_SOUNDROMS22("gemin_s1.bin", CRC(b9a80ab2) SHA1(9fdfeae5c9bc735e6a9ad42d925a1217c30a3386),
                  "gemin_s2.bin", CRC(312a5c35) SHA1(82be0ca6f4430e54bbf963a879b85636537146a1))
TAITO_ROMEND
#define input_ports_gemini input_ports_taito
CORE_GAMEDEFNV(gemini,"Gemini 2000",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Vortex
/-------------------------------*/
INITGAME(vortex,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(vortex,"vortex1.bin",CRC(abe193e7) SHA1(8ba7e82deb3461c0723a278596d02a6d74cfad68),
                          "vortex2.bin",CRC(0dd68604) SHA1(788e527e945d7edc8d30200ddf04f0a2cf4312ff),
                          "vortex3.bin",CRC(a46e3722) SHA1(b91ea5eb8b05a642e756fe3942ce4adc6bf75a29),
                          "vortex4.bin",CRC(39ef8112) SHA1(acde00a6c13fff1173a8fbe2ec31fdf662502032))
TAITO_SOUNDROMS22("vrtex_s1.bin", CRC(740bdd3e) SHA1(ed86bd65ac4b6d43f91a95d44d48b04adb631ee3),
                  "vrtex_s2.bin", CRC(4250e02e) SHA1(5a67aac55728e6661d85e31b01a5263b9d4a22db))
TAITO_ROMEND
#define input_ports_vortex input_ports_taito
CORE_GAMEDEFNV(vortex,"Vortex",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Titan
/-------------------------------*/
INITGAME(titan,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(titan,"titan1.bin",CRC(625f58fb) SHA1(52f884faaa109243a0091882cef6e480ea5e4bcc),
                         "titan2.bin",CRC(f2e5a7d0) SHA1(e0c6a969765e433c448d54f2307767adda1254f9),
                         "titan3.bin",CRC(e0827a82) SHA1(7245bab117234c0286aad4a5f45bbb8cb843a3f0),
                         "titan4.bin",CRC(fb3d0282) SHA1(d0f47deab82bcf15e6129c0960c94493e78a1c51))
TAITO_SOUNDROMS22("titan_s1.bin", CRC(36b5c196) SHA1(b3788ed5b53e4a8fe35e7be2b6b7b943e518f68c),
                  "titan_s2.bin", CRC(3bd0e6ab) SHA1(1a0b7ddde004020aaae5095071acc4b552ced1bf))
TAITO_ROMEND
#define input_ports_titan input_ports_taito
CORE_GAMEDEFNV(titan,"Titan",1982,"Taito",taito_sintevox,0)

/*--------------------------------
/ Zarza
/-------------------------------*/
INITGAME(zarza,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(zarza,"zarza1.bin",CRC(81a35f85) SHA1(3086f47573c683f86c371954c2be6ee51b75c83b),
                         "zarza2.bin",CRC(cbf88eee) SHA1(1ef46098259f469b6fa3af05040a7ff2ace8c865),
                         "zarza3.bin",CRC(a5faf4d5) SHA1(84bb1e89dac9008e226c5d64f62f245632fe9634),
                         "zarza4.bin",CRC(ddfcdd20) SHA1(6c7761d9b11e4e62a5bf2346d9ec8278610131ec))
TAITO_SOUNDROMS22("zarza_s1.bin", CRC(f076c2a8) SHA1(f626556e1aea7a36a801e8f0fc9a762f8eea636f),
                  "zarza_s2.bin", CRC(a98e13b7) SHA1(7416a941ee87fd456a5c4115e6933b8b7ad69681))
TAITO_ROMEND
#define input_ports_zarza input_ports_taito
CORE_GAMEDEFNV(zarza,"Zarza",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Shark (Taito)
/-------------------------------*/
INITGAME(sharkt,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(sharkt,"shark1.bin",CRC(efe19b88) SHA1(a206537aad1e27abc86eb5366bdde7da8bb03726),
                          "shark2.bin",CRC(ab11c287) SHA1(958279e0cd610fb5522eccc9764ecbaaefb6c744),
                          "shark3.bin",CRC(7ccf945b) SHA1(683d8d8e4ec9c36dcf4cad240644d54f580a8bb6),
                          "shark4.bin",CRC(8ca33f37) SHA1(ec08923fb04c92f4f01a8289f924792708869cf2))
TAITO_SOUNDROMS4("shark_s1.bin",CRC(75969a7d) SHA1(a37ec84641172ec7a7936fee10c1a36d567d33bb))
TAITO_ROMEND
#define input_ports_sharkt input_ports_taito
CORE_GAMEDEFNV(sharkt,"Shark (Taito)",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Hawkman
/-------------------------------*/
INITGAME(hawkman,SNDBRD_TAITO_SINTEVOX)
TAITO_ROMSTART2222(hawkman,"hawk1.bin",CRC(cf991a68) SHA1(491d6776685b3664fae104ff3011ca3e5b0ffd41),
                           "hawk2.bin",CRC(568ac529) SHA1(d1f8034c9980f4a525d55189f68ab2a63abcf2a5),
                           "hawk3.bin",CRC(14be7e31) SHA1(86877bedb2df6edefc436dea20fcf04bf5a31641),
                           "hawk4.bin",CRC(e6df08a5) SHA1(bc1f7042b404d01c0cc8cccf1fdf1f42f37f8e02))
TAITO_SOUNDROMS22("hawk_s1.bin", CRC(47549394) SHA1(f5731200db73e8751d2ec4a072b679127b6f0afa),
                  "hawk_s2.bin", CRC(29bef82f) SHA1(5f393cc1cb6047cba1186e332e840bce8e59509b))
TAITO_ROMEND
#define input_ports_hawkman input_ports_taito
CORE_GAMEDEFNV(hawkman,"Hawkman",1982,"Taito",taito_sintevox,0)

/*--------------------------------
/ Speed Test
/-------------------------------*/
INITGAME(stest,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(stest,"stest1.bin",CRC(e13ed60c) SHA1(f2f89f7a1e7681ac3ea17c24c89ac1bee3ffa6e9),
                         "stest2.bin",CRC(584d683d) SHA1(8e52226a85366c8aebd011df014ab01f78d7e02d),
                         "stest3.bin",CRC(271129a2) SHA1(c20755f6b661502ce43fea03fb654046ed1a747d),
                         "stest4.bin",CRC(1cdd4e08) SHA1(bc7e3efd194396efb63115186bf586439732519d))
TAITO_SOUNDROMS22("stest_s1.bin", CRC(dc71d4b2) SHA1(c2d3523019f63162aa23e0141263179b9f219609),
                  "stest_s2.bin", CRC(d7ac9369) SHA1(6085341a32bc5cc17a631aeb0d5a792a9de675be))
TAITO_ROMEND
#define input_ports_stest input_ports_taito
CORE_GAMEDEFNV(stest,"Speed Test",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Lunelle
/-------------------------------*/
INITGAME(lunelle,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(lunelle,"lunelle1.bin",CRC(d471349a) SHA1(fb43daa94035dc3abe0e0b16cbb239d7f97437ea),
                           "lunelle2.bin",CRC(83b132a3) SHA1(ab52f7ae20a823a9bc2986a32ef4e32a3ec2acd4),
                           "lunelle3.bin",CRC(69ec6079) SHA1(df36daa221d27f97f69231c19cbbb80347f51dd3),
                           "lunelle4.bin",CRC(492f5de7) SHA1(5bfa0a7b1e3612baebc4c598b43121e7846ae0ff))
TAITO_SOUNDROMS22("lunel_s1.bin", NO_DUMP,
                  "lunel_s2.bin", NO_DUMP)
TAITO_ROMEND
#define input_ports_lunelle input_ports_taito
CORE_GAMEDEFNV(lunelle,"Lunelle",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Rally
/-------------------------------*/
INITGAME(rally,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(rally,"rally1.bin",CRC(d0d6b32e) SHA1(ef144de5916b78ceabcea19465c23567473a41d5),
                         "rally2.bin",CRC(e7611e06) SHA1(5443c255eea2b3e0778d63064cf952259862170e),
                         "rally3.bin",CRC(45d28cd3) SHA1(dda00ac5aad24a359ff894a2abe0db967826165d),
                         "rally4.bin",CRC(7fb471ee) SHA1(d161836528380b3d18606aa082dfc1d7a5959147))
TAITO_SOUNDROMS22("rally_s1.bin", CRC(0c7ca1bc) SHA1(09df10b1b295b9a7f5c337eb4f1e1e4db0f3d113),
                  "rally_s2.bin", CRC(a409d9d1) SHA1(3005cfaedd6edf3d80cac539563655f3bcc342ca))
TAITO_ROMEND
#define input_ports_rally input_ports_taito
CORE_GAMEDEFNV(rally,"Rally",1982,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Snake Machine
/-------------------------------*/
INITGAME(snake,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART2222(snake,"snake1.bin",CRC(7bb79585) SHA1(6e1bb1e33733bc2c41ad9fc43540190df24adc63),
                         "snake2.bin",CRC(55c946f7) SHA1(b77549063c99ee194608abb45aa0cec958336636),
                         "snake3.bin",CRC(6f054bc0) SHA1(08ab82131888756e8178b2fe2bbc24fc4f494ef2),
                         "snake4.bin",CRC(ed231064) SHA1(42410dbbef36dea9d0163c65406bc86b35bb0bd7))
TAITO_SOUNDROMS44("snake_s1.bin", CRC(f7c1623c) SHA1(77e79ccc4b074b715008de37332baf76791d471e),
                  "snake_s2.bin", CRC(18316d73) SHA1(422a093ff245f0c8f710aeba91acd59666e2398b))
TAITO_ROMEND
#define input_ports_snake input_ports_taito
CORE_GAMEDEFNV(snake,"Snake Machine",1982,"Taito",taito_sintetizadorpp,0)

/*--------------------------------
/ Voley Ball
/-------------------------------*/
INITGAME(voleybal,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART2222(voleybal,"voley1.bin",NO_DUMP,
                            "voley2.bin",NO_DUMP,
                            "voley3.bin",NO_DUMP,
                            "voley4.bin",NO_DUMP)
TAITO_SOUNDROMS44("voley_s1.bin", CRC(9c825666) SHA1(330ecd9caccb8a1555c5e7302095ae25558c020e),
                  "voley_s2.bin", CRC(79a8228c) SHA1(e71d9347a8fc230c70703164ae0e4d44423bbb5d))
TAITO_ROMEND
#define input_ports_voleybal input_ports_taito
CORE_GAMEDEFNV(voleybal,"Voley Ball",198?,"Taito",taito_sintetizadorpp,0)

//??/82 Gork
//??/8? Ogar

/*--------------------------------
/ Mr. Black
/-------------------------------*/
INITGAME(mrblack,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART22222(mrblack,"mrb1.bin",CRC(c2a43f6f) SHA1(14a461b6416e3b024cc3d7743b75e29ca1876b64),
                            "mrb2.bin",CRC(ddf2a88e) SHA1(8de67f4032811ec3b7da1655207d05e52d4e5e01),
                            "mrb3.bin",CRC(f319f68f) SHA1(f4b408837eeab8a7cd7dedc031f0b9332363a7d4),
                            "mrb4.bin",CRC(84367699) SHA1(a9a7b21fe31f12b0888bc3bbf82d0b13cf8bad49),
                            "mrb5.bin",CRC(18d8f2cc) SHA1(e14c20440753a1996e618e407ef97f3059775c46))
TAITO_SOUNDROMS444("mrb_s1.bin", CRC(ff28b2b9) SHA1(3106811740e0206ad4ba7845e204e721b0da70e2),
                   "mrb_s2.bin", CRC(3af4707e) SHA1(b7231ede973a0c83e009333f0377b81c34826117),
                   "mrb_s3.bin", CRC(276fb897) SHA1(b1a4323a4d921e3ae4beefaa04cd95e18cc33b9d))
TAITO_ROMEND
#define input_ports_mrblack input_ports_taito
CORE_GAMEDEFNV(mrblack,"Mr. Black",1984,"Taito",taito_sintetizadorpp,0)

/*--------------------------------
/ Fire Action Deluxe
/-------------------------------*/
INITGAME(fireactd,SNDBRD_TAITO_SINTEVOXPP)
TAITO_ROMSTART2222(fireactd,"fired1.bin",CRC(2f923913) SHA1(c35dcf37e6957523f6762b95f5f6503037b607d6),
                            "fired2.bin",CRC(4d268048) SHA1(f1c4cb1c91f73e2a145725b4696b7996d311883f),
                            "fired3.bin",CRC(f5e07ed1) SHA1(3da566ea2fb56998fc56db3f373ec813b5b627e1),
                            "fired4.bin",CRC(da1a4ed5) SHA1(e39be103dfcfa004061d2249292b023bc3fac9bd))
TAITO_SOUNDROMS444("fired_s1.bin", CRC(b821d324) SHA1(db00416592467a5917dd75e437842aea822fffa8),
                   "fired_s2.bin", CRC(d427d0f6) SHA1(bcd1cf15f4ff1df30a42d8889879cff9d3f16e6e),
                   "fired_s3.bin", CRC(ecff8399) SHA1(7615da5a6952cbc0769963a9563017bd46e4a73f))
TAITO_ROMEND
#define input_ports_fireactd input_ports_taito
CORE_GAMEDEFNV(fireactd,"Fire Action Deluxe",198?,"Taito",taito_sintevoxpp,GAME_IMPERFECT_SOUND)

/*--------------------------------
/ Space Shuttle
/-------------------------------*/
INITGAME(sshuttle,SNDBRD_TAITO_SINTETIZADOR)
TAITO_ROMSTART2222(sshuttle,"sshtl1.bin",CRC(ab67ed50) SHA1(0f627b007d74b81aba6b4ad0f4cf6782e42e24c9),
                            "sshtl2.bin",CRC(ed5130a4) SHA1(3e99c151d6649c4b19d59ab2128ee3160c6462a9),
                            "sshtl3.bin",CRC(17d43a16) SHA1(dd9a503460db9af64d6e22303d8a5b5b578ff950),
                            "sshtl4.bin",CRC(2719dbac) SHA1(3519dbac6fc0314d3277d59211bad4abf844ee02))
TAITO_SOUNDROMS22("rally_s1.bin", CRC(0c7ca1bc) SHA1(09df10b1b295b9a7f5c337eb4f1e1e4db0f3d113),
                  "rally_s2.bin", CRC(a409d9d1) SHA1(3005cfaedd6edf3d80cac539563655f3bcc342ca))
TAITO_ROMEND
#define input_ports_sshuttle input_ports_taito
CORE_GAMEDEFNV(sshuttle,"Space Shuttle (Taito)",1985,"Taito",taito_sintetizador,0)

/*--------------------------------
/ Polar Explorer
/-------------------------------*/
INITGAME(polar,SNDBRD_TAITO_SINTETIZADORPP)
TAITO_ROMSTART2222(polar,"polar1.bin",CRC(f92944b6) SHA1(04ff22977a5036eee46a9e1decd2ec4d7046eb0d),
                         "polar2.bin",CRC(e6391071) SHA1(2793ad9ee3018069a93c739daca03787f7d81de7),
                         "polar3.bin",CRC(318d0702) SHA1(27c4856ea098286142c70552f07fd689e35d5288),
                         "polar4.bin",CRC(1c02f0c9) SHA1(663c1f4841cb0bd7139e4063d4e7e35a51470686))
TAITO_SOUNDROMS444("polar_s1.bin", CRC(baff1a67) SHA1(d93736b8d232034047f463b43ac51f9fd4a28536),
                   "polar_s2.bin", CRC(84fe1dc8) SHA1(96f52fc9245d0f7626da9cf41979c5a84a63f4bb),
				   "polar_s3.bin", CRC(d574bc94) SHA1(f6060b60708cebd1d546dc5b9e3cec0781454af5))
TAITO_ROMEND
#define input_ports_polar input_ports_taito
CORE_GAMEDEFNV(polar,"Polar Explorer",198?,"Taito",taito_sintetizadorpp,0)

/*-----------
/ Test Eprom
/-----------*/
INITGAME(taitest,SNDBRD_NONE)
TAITO_ROMSTART2(taitest,"ttest1.bin",CRC(a9729e2f) SHA1(2c13bc9d6eab2101316fa795a18d5c5afac936d8))
TAITO_ROMEND
#define input_ports_taitest input_ports_taito
CORE_GAMEDEFNV(taitest,"Taito Test Fixture",198?,"Taito",taito,0)
