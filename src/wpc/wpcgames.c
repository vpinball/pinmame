#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "wpc.h"

#define swStart     13
#define swTilt      14
#define swSlamTilt  21
#define swCoinDoor  22

#define INITGAME(name) \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
}

WPC_INPUT_PORTS_START(wpc, 0) WPC_INPUT_PORTS_END

/*-------------
/ Ticket Tac Toe
/--------------*/
static core_tGameData tttGameData = {
  GEN_WPC95, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L), 0,0,0,0,0,1 },
  NULL,
  {
    "905  123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x3f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(ttt)

WPC_ROMSTART(ttt,10,"tikt1_0.rom",0x80000,CRC(bf1d0382) SHA1(3d26413400915594e9f1cc08a551c05526b94223))
DCS_SOUNDROM2m("ttt_s2.rom",CRC(faae93eb) SHA1(672758544b260d7751ac296f5beb2e271e77c50a),
               "ttt_s3.rom",CRC(371ba9b3) SHA1(de6a8cb78e08a434f6668dd4a93cad857acba310))
WPC_ROMEND
#define input_ports_ttt input_ports_wpc
CORE_GAMEDEF(ttt,10,"Ticket Tac Toe (1.0)",1996,"Williams",wpc_m95S,0)

/*--------------
/ Test Fixture DMD generation
/---------------*/
static core_tGameData tfdmdGameData = {
  GEN_WPCDMD, wpc_dispDMD,
  { 0 },
  NULL,
  { "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(tfdmd)
WPC_ROMSTART(tfdmd,l3,"u6_l3.rom",0x20000,CRC(bd43e28c) SHA1(df0a64a9fddbc59e3edde56ae12b68f76e44ba2e)) WPC_ROMEND
#define input_ports_tfdmd input_ports_wpc
CORE_GAMEDEF(tfdmd,l3,"WPC Test Fixture: DMD (L-3)",1991,"Bally",wpc_mDMD,GAME_NO_SOUND)

/*--------------
/ Test Fixture Security generation
/---------------*/
static core_tGameData tfsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U), 0,0,0,0,0,1 },
  NULL,
  {
    "648 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(tfs)

WPC_ROMSTART(tfs,12,"u6_12.rom",0x80000,CRC(12687d19) SHA1(bcc3116328a8c6f0ed430a6d2343d01fcdf2459f))
DCS_SOUNDROM1x("u2_10.rom",CRC(d705b41e) SHA1(a7811b4bb1b2b5f7e3d1a809da3363b97dfca680))
WPC_ROMEND
#define input_ports_tfs input_ports_wpc
CORE_GAMEDEF(tfs,12,"WPC Test Fixture: Security (1.2)",1994,"Bally",wpc_mSecurityS,0)

/*--------------
/ Test Fixture WPC95
/---------------*/
static core_tGameData tf95GameData = {
  GEN_WPC95, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U), 0,0,0,0,0,1 },
  NULL,
  {
    "648 123456 12345 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(tf95)

WPC_ROMSTART(tf95,12,"g11_12.rom",0x80000,CRC(259a2b23) SHA1(16f8c15e046809e0b1587b0c981d36f4d8a750ca))
DCS_SOUNDROM1m("s2_10.rom",CRC(ceff7fe4) SHA1(ff2574f65e09d446b9e446abd58159a7d100059b))
WPC_ROMEND
#define input_ports_tf95 input_ports_wpc
CORE_GAMEDEF(tf95,12,"WPC 95 Test Fixture (1.2)",1996,"Bally",wpc_m95S,0)

/*===========
/  Test Fixture Alphanumeric
/============*/
static core_tGameData tfaGameData = {
  GEN_WPCALPHA_2, wpc_dispAlpha,
  { FLIP_SWNO(12,11) },
  NULL,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(tfa)
WPC_ROMSTART(tfa,13,"u6_l3.rom",0x020000,CRC(bf4a37b5) SHA1(91b8bba6182e818a34252a4b2a0b86a2a44d9c42)) WPC_ROMEND
#define input_ports_tfa input_ports_wpc
CORE_GAMEDEF(tfa,13,"WPC Test Fixture: Alphanumeric (1.3)",1990,"Bally",wpc_mAlpha,GAME_NO_SOUND)
