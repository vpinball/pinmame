// license:BSD-3-Clause

/*
System 7 games have the software revision identified with an "L" to signify the Level of software release.
To see what version your game is running, set your test switch to AUTO/UP and press ADVANCE while in Game Over mode.
You will see "2GGG" in the player 1 display, where GGG is the game number.
The player 2 display will show the number of the installed game revision.
*/

#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s7.h"

const core_tLCDLayout s7_dispS7[] = {
  DISP_SEG_7(1,0,CORE_SEG87),DISP_SEG_7(1,1,CORE_SEG87),
  DISP_SEG_7(0,0,CORE_SEG87),DISP_SEG_7(0,1,CORE_SEG87),
  DISP_SEG_BALLS(0,8,CORE_SEG7S),DISP_SEG_CREDIT(20,28,CORE_SEG7S),{0}
};
const struct core_dispLayout s7_6digit_disp[] = {
  // Player 1            Player 2
  {0, 0, 0,6,CORE_SEG7}, {0,18, 8,6,CORE_SEG7},
  // Player 3            Player 4
  {2, 0,20,6,CORE_SEG7}, {2,18,28,6,CORE_SEG7},
  // Right Side          Left Side
  {4, 9,14,2,CORE_SEG7}, {4,14, 6,2,CORE_SEG7}, {0}
};

#define INITGAMEFULL(name, disp,lflip,rflip,sc, ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S7, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; hc55516_set_sample_clock(0, sc); }

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

INITGAMEFULL(frpwr,fp_7digit_disp,0,45,11774, 26,25,27,28,42,12)
S7_ROMSTART808x(frpwr,b7,"f7ic14pr.716",CRC(c7a60d8a) SHA1(1921f2ddec96963d7f990cd6b1cdd6b4a6e42810),
                         "f7ic17gr.532",CRC(a042201a) SHA1(1421e1dbbcb322d83838d68ac0909f4804249815),
                         "f7ic20ga.716",CRC(584d7b45) SHA1(9ea01eda36dab77dec78c2d1207983c505e406b2))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S7_ROMEND
#define input_ports_frpwr input_ports_s7
CORE_CLONEDEF(frpwr,b7,l6,"Firepower (Sys.7/7-digit conversion)",2003,"Williams / Oliver",s7_mS7S6,0)

/*-------------------------------
/ Firepower - Sys.7 6-Digit conversion, rev. 31
/------------------------------*/
INITGAMEFULL(frpwr_e7,s7_6digit_disp,0,45,11774, 26,25,27,28,42,12)
S7_ROMSTART808x(frpwr,e7,"fir6d714.716",CRC(61606c43) SHA1(89d49ddde4a2a06b23b52dd3e4dc64a086bd5c10),
                         "fir6d717.532",CRC(b1a13f4d) SHA1(8895ed6a6d5a8ee1fde24bb161d966d7f0d0e8bc),
                         "fir6d720.716",CRC(3202451a) SHA1(35b3d50488db5e168483f7e5c4180e8ad30ba238))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S7_ROMEND
#define input_ports_frpwr_e7 input_ports_s7
CORE_GAMEDEFNV(frpwr_e7,"Firepower (Sys.7/6-digit /10 Scoring rev. 31)",2005,"Williams / Oliver",s7_mS7S6,0)

/*-------------------------------
/ Firepower - Sys.7 6-Digit conversion, rev. 31
/------------------------------*/
INITGAMEFULL(frpwr_a7,s7_6digit_disp,0,45,11774, 26,25,27,28,42,12)
S7_ROMSTART808x(frpwr,a7,"fire6714.716",CRC(e6e15b03) SHA1(b78cbdaa83c96983c3ca294f2412f70adf75da95),
                         "fire6717.532",CRC(9d3e8720) SHA1(6fbf01a9c9e4dd1f0f22fe0011975e272b1ecdc0),
                         "fire6720.716",CRC(5c6943fd) SHA1(aba085285d10c306389572dd96fe38fa332f0a0e))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S7_ROMEND
#define input_ports_frpwr_a7 input_ports_s7
CORE_GAMEDEFNV(frpwr_a7,"Firepower (Sys.7/6-digit conversion rev. 31)",2005,"Williams / Oliver",s7_mS7S6,0)

/*-------------------------------
/ Firepower - Sys.7 7-Digit conversion, rev. 31
/------------------------------*/
INITGAMEFULL(frpwr_d7,fp_7digit_disp,0,45,11774, 26,25,27,28,42,12)
S7_ROMSTART808x(frpwr,d7,"fire7714.716",CRC(61606c43) SHA1(89d49ddde4a2a06b23b52dd3e4dc64a086bd5c10),
                         "fire7717.532",CRC(13d29ea8) SHA1(dd176a45ccb508f5f19db94e80cc65414c9a01e8),
                         "fire7720.716",CRC(d7d7fb17) SHA1(6d4fc5a6b60a866a13dfc98c348aff89613a572e))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6.532",   CRC(e56f7aa2) SHA1(cb922c3f4d91285dda4ccae880c2d798a82fd51b))
S7_ROMEND
#define input_ports_frpwr_d7 input_ports_s7
CORE_GAMEDEFNV(frpwr_d7,"Firepower (Sys.7/7-digit conversion rev. 31)",2005,"Williams / Oliver",s7_mS7S6,0)

/*-------------------------------
/ Firepower - Sys.7 7-Digit conversion, rev. 38
/------------------------------*/
S7_ROMSTART8088(frpwr,c7,"f7ic14pr_38.716",CRC(4cd22956) SHA1(86380754b9cbb0a81b4fa4d26cff71b9a70d2a96),
                         "f7ic17gr_38.532",CRC(3a1b7cc7) SHA1(1f32ef6d66040a53b04a0cddb1ffbf197a29c940),
                         "f7ic20ga_38.716",CRC(5ed61e41) SHA1(b73a05b336f7bb8ee528205612bd0744e86498f5),
                         "f7ic26.716"  ,CRC(aaf82d89) SHA1(c481a49c7e7d4734c0eeab31b9970ca62a3995f0))
S67S_SOUNDROMS8(         "sound3.716",  CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS0000(     "v_ic7.532",   CRC(94c5c0a7) SHA1(ff7c618d1666c1d5c3319fdd72c1af2846659290),
                         "v_ic5.532",   CRC(1737fdd2) SHA1(6307e0ae715e97294ee8aaaeb2e2bebb0cb590c2),
                         "v_ic6_38.532",CRC(35db11d2) SHA1(001df0d5245230b960ff69c30ee2b305b3a5e4b4),
                         "v_ic4.532",   CRC(8d4ff909) SHA1(b82666fe96bdf174bc4f347d7139da9ab7dadee1))
S7_ROMEND
CORE_CLONEDEF(frpwr,c7,l6,"Firepower (Sys.7/7-digit conversion rev. 38)",2006,"Williams / Oliver",s7_mS7S6,0)

/*-----------------------------------
/ Cosmic Gunfight - Sys.7 (Game #502)
/-----------------------------------*/
INITGAMEFULL(csmic,s7_dispS7,52,49,0, 36,37,21,22,24,23)
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
INITGAMEFULL(jngld,s7_dispS7,0,0,13982, 12,28,40,0,0,0)
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

S7_ROMSTART8088(jngld,l1, "ic14-l1.716", CRC(0144af0d) SHA1(2e5b6e35613decbac10f9b99c7a8cbe7f63b6b07),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26-l1.716", CRC(df37bb45) SHA1(60a0670e73f2370d6269ef241b581f5b0ade6ea0))
S67S_SOUNDROMS8(          "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "speech7.532",CRC(83ffb695) SHA1(f9151bdfdefd5c178ca7eb5122f62b700d64f41a),
                          "speech5.532",CRC(754bd474) SHA1(c05f48bb07085683de469603880eafd28dffd9f5),
                          "speech6.532",CRC(f2ac6a52) SHA1(5b3e743eac382d571fd049f92ea9955342b9ffa0))
S7_ROMEND
CORE_CLONEDEF(jngld,l1,l2,"Jungle Lord (L-1)",1981,"Williams",s7_mS7S,0)

/*
Jungle Lord new tricks (JLnt) is a set of Home ROMs for the 1980 Williams Jungle Lord pinball machine. In addition to all original game rules, with minor sound and scoring tweaks, the following new tricks have been implemented (*operator adjustable feature):

Skill Shots:
Quickly locking a new ball in the lower or upper saucer awards 100k or Instant Multiball*, except during Stampede Mode.

Extra Ball:
Complete 1-2-3-4-5-sequence three times* to light Extra Ball in lower lock; progress is indicated by bonus lamps during completion and at each ball start.

MB Bonus Frenzy:
Each bonus advance during multiball sounds trumpet and instantly scores 20k*.

Combo Shots:
Quick 2-/3-way combos ([left inlane,] turnaround, lower lock) ring bell and score 100/250k, respectively, except during multiball.

Magna Whirl:
Whirl L-O-R-D letters with Magna-Save buttons to improve odds with the mini ball.

These new tricks are intended to add fun and to encourage lower playfield play and risk taking. Scoring potential of the new Combo Shots and MB Bonus Frenzy are similar to those of Double Trouble and maxed out bonus in the original game.
*/
S7_ROMSTART8088(jngld,nt, "ic14.bin",   CRC(714bfdaa) SHA1(6d566578d1f6e445bf66e917699c99e91f4f9aca),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.bin",   CRC(6bcb8a17) SHA1(91778503a9402b073b4066c386b00dd447a3e740))
S67S_SOUNDROMS8(          "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S67S_SPEECHROMS000x(      "speech7.532",CRC(83ffb695) SHA1(f9151bdfdefd5c178ca7eb5122f62b700d64f41a),
                          "speech5.532",CRC(754bd474) SHA1(c05f48bb07085683de469603880eafd28dffd9f5),
                          "speech6.532",CRC(f2ac6a52) SHA1(5b3e743eac382d571fd049f92ea9955342b9ffa0))
S7_ROMEND
CORE_CLONEDEF(jngld,nt,l2,"Jungle Lord (New Tricks)",2013,"A.M. Thurnherr",s7_mS7S,0)

// A L1 version of JLNT exists, but idealjoker wants to redo some things in addition (so rerelease by early 2021?)

/*--------------------------------
/ Pharaoh - Sys.7 (Game #504)
/--------------------------------*/
INITGAMEFULL(pharo,s7_dispS7,0,0,16571, 20,24,44,0,0,0)
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

S7_ROMSTART8088(pharo,l2b,"ic14.716",   CRC(cef00088) SHA1(e0c6b776eddc060c42a483de6cc96a1c9f2afcf7),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(2afbcd1f) SHA1(98bb3a74548b7d9c5d7b8432369658ed32e8be07))
S67S_SOUNDROMS8(          "sound12.716",CRC(b0e3a04b) SHA1(eac54376fe77acf46e485ab561a01220910c1fd6))
S67S_SPEECHROMS0000(      "speech7f.532",CRC(3472255e) SHA1(fb7d9613eead268af43f4c8066e9f4e5d60cf49f),
                          "speech5.532",CRC(d72863dc) SHA1(e24ad970ed202165230fab999be42bea0f861fdd),
                          "speech6.532",CRC(d29830bd) SHA1(88f6c508f2a7000bbf6c9c26e1029cf9a241d5ca),
                          "speech4.532",CRC(9ecc23fd) SHA1(bf5947d186141504fd182065533d4efbfd27441d))
S7_ROMEND
CORE_CLONEDEF(pharo,l2b,l2,"Pharaoh (L-2 Tomb Sound Fix MOD)",2019,"Williams",s7_mS7S,0)

/*-----------------------------------
/ Solar Fire - Sys.7 (Game #507)
/-----------------------------------*/
INITGAMEFULL(solar,s7_dispS7,0,0,0, 11,12,0,0,0,0)
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
INITGAMEFULL(thund,s7_dispS7,0,0,16051, 0,0,0,0,0,0)
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
CORE_GAMEDEF(thund,p1,"Thunderball (P-1 Prototype)",1982,"Williams",s7_mS7SND,0) // dated 6/22

S7_ROMSTART000x(thund,p2, "ic14_831.532", CRC(873ccf24) SHA1(2723aa7d059a111374d8145391fbef0c81043e4b),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20_831.532", CRC(91ed089b) SHA1(0e47f5a87cb227a6ee8645931bfa807219b388ef))
S67S_SOUNDROMS0(          "sound12.532",CRC(cc70af52) SHA1(d9c2840acdcd69aab39fc647dd4819eccc06af33))
S67S_SPEECHROMS0000(      "speech7.532",CRC(33e1b041) SHA1(f50c0311bde69fa6e8071e297a81cc3ef3dcf44f),
                          "speech5.532",CRC(11780c80) SHA1(bcc5efcd69b4f776feef32484a872863847d64cd),
                          "speech6.532",CRC(ab688698) SHA1(e0cbac44a6fe30a49da478c32500a0b43903cc2b),
                          "speech4.532",CRC(2a4d6f4b) SHA1(e6f8a1a6e6abc81f980a4938d98abb250e8e1e3b))
S7_ROMEND
CORE_CLONEDEF(thund,p2,p1,"Thunderball (P-2 Prototype)",1982,"Williams",s7_mS7SND,0) // dated 8/31

S7_ROMSTART000x(thund,p3, "ic14_908.532", CRC(099e798e) SHA1(38d79622b4d68c69308ee109f47509e0733828ba),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20_908.532", CRC(21f87917) SHA1(6cfdd5aadafb2d137f2e959fa047ffbe5c09ac2c))
S67S_SOUNDROMS0(          "sound12.532",CRC(cc70af52) SHA1(d9c2840acdcd69aab39fc647dd4819eccc06af33))
S67S_SPEECHROMS0000(      "speech7.532",CRC(33e1b041) SHA1(f50c0311bde69fa6e8071e297a81cc3ef3dcf44f),
                          "speech5.532",CRC(11780c80) SHA1(bcc5efcd69b4f776feef32484a872863847d64cd),
                          "speech6.532",CRC(ab688698) SHA1(e0cbac44a6fe30a49da478c32500a0b43903cc2b),
                          "speech4.532",CRC(2a4d6f4b) SHA1(e6f8a1a6e6abc81f980a4938d98abb250e8e1e3b))
S7_ROMEND
CORE_CLONEDEF(thund,p3,p1,"Thunderball (P-3 Prototype)",1982,"Williams",s7_mS7SND,0) // dated 9/08

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

S7_ROMSTART000x(hypbl,l3, "ic14-l3.532", CRC(e233bbed) SHA1(bb29acc3e48d6b40b3df2e7702f8a8ff4357c15c),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
						  "ic20-l3.532", CRC(4a37d6e8) SHA1(8c26dd5652ace431a6ff0faf0bb9db37489c4fec))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l3,l4,"HyperBall (L-3)",1981,"Williams",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l2, "ic14-l2.532", CRC(8eb82df4) SHA1(854b3f1fa2112fbdba19f4c843f67989c0572d8c),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20-l2.532", CRC(f5f66cf1) SHA1(885b4961b6ec712b7445001d448d881245be1234))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l2,l4,"HyperBall (L-2)",1981,"Williams",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l5, "ic14.532",    CRC(8090fe71) SHA1(0f1f40c0ee8da5b2fd51efeb8be7c20d6465239e),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20_fix.532",    CRC(48958d77) SHA1(ddfec991ef99606b866ced08b59e205a0b2cadd1))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l5,l4,"HyperBall (L-5 MOD High score save)",1998,"Jess M. Askey",s7_mS7S,0)

S7_ROMSTART000x(hypbl,l6, "ic14.532",    CRC(8090fe71) SHA1(0f1f40c0ee8da5b2fd51efeb8be7c20d6465239e),
                          "ic17.532",    CRC(6f4c0c4c) SHA1(1036067e2c85da867983e6e51ee2a7b5135000df),
                          "ic20_l6.532",    CRC(e40c27cd) SHA1(337bd8450be305796e52c1b7d3ae3cc8cc972525))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_CLONEDEF(hypbl,l6,l4,"HyperBall (L-6 MOD High score save)",2006,"Jess M. Askey",s7_mS7S,0)

/*----------------------------
/ Barracora - Sys.7 (Game #510)
/----------------------------*/
// Tournament MOD exists: Bara_20.mod.716 & bara_26.mod.716 and/or ic14_l2_multiplier.716
INITGAMEFULL(barra,s7_dispS7,34,35,0, 0,20,21,22,29,30)
S7_ROMSTART8088(barra,l1, "ic14.716",   CRC(522e944e) SHA1(0fa17b7912f8129e40de5fed8c3ccccc0a2a9366),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(2a0e0171) SHA1(f1f2d4c1baed698d3b7cf2e88a2c28056e859920))
S67S_SOUNDROMS8(          "sound4.716", CRC(67ea12e7) SHA1(f81e97183442736d5766a7e5e074bc6539e8ced0))
S7_ROMEND
#define input_ports_barra input_ports_s7
CORE_GAMEDEF(barra,l1,"Barracora (L-1)",1981,"Williams",s7_mS7S,0)

/*----------------------------
/ Varkon - Sys.7 (Game #512)
/----------------------------*/
INITGAMEFULL(vrkon,s7_dispS7,40,39,0, 33,41,0,0,0,0)
S7_ROMSTART8088(vrkon,l1, "ic14.716",   CRC(3baba324) SHA1(522654e0d81458d8b31150dcb0cb53c29b334358),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(df20330c) SHA1(22157c6480ad38b9c53c390f5e7bfa63a8abd0e8))
S67S_SOUNDROMS8(          "sound12.716",CRC(d13db2bb) SHA1(862546bbdd1476906948f7324b7434c29df79baa))
S7_ROMEND
#define input_ports_vrkon input_ports_s7
CORE_GAMEDEF(vrkon,l1,"Varkon (L-1)",1982,"Williams",s7_mS7S,0)

/*-------------------------------
/ Spellbinder - Sys.7 - (Game #513) - NOT Released - Developed by Jess Askey see https://github.com/jessaskey/wms_perc
/-------------------------------*/
static core_tGameData splbnGameData = { GEN_S7, dispHypbl, {FLIP_SWNO(33,34), 0, 4}, NULL, {"", {0,0,0,0xf8,0x0f}, {0} } };
static void init_splbn(void) { core_gameData = &splbnGameData; }
#define input_ports_splbn input_ports_s7

S7_ROMSTART000x(splbn,l0, "ic14.532",    CRC(940a817a) SHA1(2583ff6f6b6985d3ac85b4f120ebb002a10b65af),
                          "ic17.532",    CRC(b38fde72) SHA1(17ef3ca354431307b6a79992c50cb2491b8a7631),
                          "ic20.532",    CRC(ff765ebf) SHA1(14d42735291c3f6e112f19bd49a39e00059cf907))
S67S_SOUNDROMS0(          "sound12.532", CRC(06051e5e) SHA1(f0ab4be812ceaf771829dd549f2a612156102a93))
S7_ROMEND
CORE_GAMEDEF(splbn,l0,"Spellbinder (L-0 BETA)",1982,"Williams / Jess M. Askey",s7_mS7S,0)

/*----------------------------------
/Time Fantasy - Sys.7 (Game #515)
/----------------------------------*/
// MOVED TO tmfnt.c

/*----------------------------
/ Warlok - Sys.7 (Game #516)
/----------------------------*/
INITGAMEFULL(wrlok,s7_dispS7,0,36,0, 13,14,15,20,21,0)
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
INITGAMEFULL(jst,s7_dispS7,0,0,0, 0,0,29,53,30,54) // 4 flippers,buttons need special
S7_ROMSTART8088(jst,l2, "ic14.716",   CRC(c4cae4bf) SHA1(ff6e48364561402b16e40a41fa1b89e7723dd38a),
                        "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                        "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                        "ic26.716",   CRC(63eea5d8) SHA1(55c26ee94809f087bd886575a5e47efc93160190))
S67S_SOUNDROMS0(        "sound12.532",CRC(3bbc90bf) SHA1(82154e719ceca5c72d1ab034bc4ff5e3ebb36832))
S7_ROMEND
#define input_ports_jst input_ports_s7
CORE_GAMEDEF(jst,l2,"Joust (L-2)",1983,"Williams",s7_mS7S,0)

S7_ROMSTART8088(jst,l1, "ic14-l1.716", CRC(9871ebb2) SHA1(75c639a26d3bf7e05de7b5be063742f7448284ac),
                        "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                        "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                        "ic26-l1.716", CRC(123d8ffc) SHA1(c227a53653525269ea77203d4d1b14132058c073))
S67S_SOUNDROMS0(        "sound12.532",CRC(3bbc90bf) SHA1(82154e719ceca5c72d1ab034bc4ff5e3ebb36832))
S7_ROMEND
CORE_CLONEDEF(jst,l1,l2,"Joust (L-1)",1983,"Williams",s7_mS7S,0)

/*---------------------------
/ Laser Cue - Sys.7 (Game #520)
/--------------------------*/
INITGAMEFULL(lsrcu,s7_dispS7,0,43,0, 41,19,18,17,0,0)
S7_ROMSTART8088(lsrcu,l2, "ic14.716",   CRC(39fc350d) SHA1(46e95f4016907c21c69472e6ef4a68a9adc3be77),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(db4a09e7) SHA1(5ea454c852303e12cc606c2c1e403b72e0a99f25))
S67S_SOUNDROMS8(          "sound12.716",CRC(1888c635) SHA1(5dcdaee437a69c6027c24310f0cd2cae4e89fa05))
S7_ROMEND
#define input_ports_lsrcu input_ports_s7
CORE_GAMEDEF(lsrcu,l2,"Laser Cue (L-2)",1983,"Williams",s7_mS7S,0)

S7_ROMSTART8088(lsrcu,l3, "ic14-l3.716",CRC(6b0c8368) SHA1(ee98108696996d24e0d059b6fd5343aeee77e583),
                          "ic17.532",   CRC(bb571a17) SHA1(fb0b7f247673dae0744d4188e1a03749a2237165),
                          "ic20.716",   CRC(dfb4b75a) SHA1(bcf017b01236f755cee419e398bbd8955ae3576a),
                          "ic26.716",   CRC(db4a09e7) SHA1(5ea454c852303e12cc606c2c1e403b72e0a99f25))
S67S_SOUNDROMS8(          "sound12.716",CRC(1888c635) SHA1(5dcdaee437a69c6027c24310f0cd2cae4e89fa05))
S7_ROMEND
CORE_CLONEDEF(lsrcu,l3,l2,"Laser Cue (L-3)",2016,"Timmo (Neverending bell fix)",s7_mS7S,0)

/*--------------------------------
/ Firepower II - Sys.7 (Game #521)
/-------------------------------*/
INITGAMEFULL(fpwr2,s7_dispS7,0,13,0, 47,48,41,42,43,44)
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
INITGAMEFULL(strlt,s7_dispS7,0,0,0, 50,51,41,42,43,0)
S7_ROMSTART000x(strlt,l1,"ic14.532",   CRC(292f1c4a) SHA1(0b5d50331364655672be16236d38d72b28f6dec2),
                         "ic17.532",   CRC(a43d8518) SHA1(fb2289bb7380838d0d817e78c39e5bcb2709373f),
                         "ic20.532",   CRC(66876b56) SHA1(6fab43fbb67c7b602ca595c20a41fc1553afdb65))
S67S_SOUNDROMS8(         "sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S7_ROMEND
#define input_ports_strlt input_ports_s7
CORE_GAMEDEF(strlt,l1,"Star Light (L-1)",1984,"Williams",s7_mS7S,0)


// Games below are from different manufacturers

/*--------------------------------
/ Wild Texas - Sys.7 (identifies as #521 L-1 which makes it a Firepower II clone)
/-------------------------------*/
INITGAMEFULL(wldtexas,s7_dispS7,0,13,0, 47,48,41,42,43,44)
ROM_START(wldtexas)
  NORMALREGION(0x10000, S7_CPUREGION)
    ROM_LOAD("wldtexas.prg", 0x4000, 0x4000, CRC(243e7116) SHA1(c13c261632b3e8693a500d922f151296102e0169))
    ROM_RELOAD(0x8000, 0x4000)
    ROM_RELOAD(0xc000, 0x4000)
S67S_SOUNDROMS8("sound3.716", CRC(55a10d13) SHA1(521d4cdfb0ed8178b3594cedceae93b772a951a4))
S7_ROMEND
#define input_ports_wldtexas input_ports_fpwr2
CORE_CLONEDEFNV(wldtexas,fpwr2_l2,"Wild Texas",????,"Unknown Manufacturer",s7_mS7S,0)
