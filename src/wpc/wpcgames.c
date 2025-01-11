// license:BSD-3-Clause

#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "wpc.h"
#include "mech.h"

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
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L), 0,0,0,0,0,0,0 },
  NULL,
  {
    "905 123456 12345 123",
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

/*-------------
/ Phantom Haus
/--------------*/
extern const core_tLCDLayout wpc_dispDMD64[];
static mech_tInitData ph_reelMech[] = {{
  38,39, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL, 48,48, {{36, 1,47}}
},{
  27,37, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL, 48,48, {{34, 1,47}}
},{
  25,26, MECH_LINEAR | MECH_CIRCLE | MECH_TWOSTEPSOL, 48,48, {{32, 1,47}}
},{0}};
static void ph_drawMech(BMTYPE **line) {
  core_textOutf(60, 0,BLACK, "Lt. Reel: %2d", mech_getPos(0));
  core_textOutf(60,10,BLACK, "Cen.Reel: %2d", mech_getPos(1));
  core_textOutf(60,20,BLACK, "Rt. Reel: %2d", mech_getPos(2));
}
static core_tGameData phGameData = {
  GEN_WPC95, wpc_dispDMD64,
  { FLIP_SWNO(0,0), 0,16,0,0,0,WPC_PH,0, NULL, NULL, mech_getPos, ph_drawMech },
  NULL,
  {
    "901 100031 64739 123",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* Cancel Red Blue Super */
    {  15,    26, 27,  14 }
  }
};
static void init_ph(void) {
  core_gameData = &phGameData;
  mech_add(0, &ph_reelMech[0]);
  mech_add(1, &ph_reelMech[1]);
  mech_add(2, &ph_reelMech[2]);
}
WPC_ROMSTART(ph,04,"g11_040.rom",0x80000,CRC(8473f464) SHA1(829f2c0c772639e56f747d4274ad98967290bd43))
DCS_SOUNDROM4xm("S2_030.rom",CRC(bbeb510c) SHA1(05088b6fa89cd203099189c31d132cd062ab8357),
               "phs3_02.rom",CRC(710568a7) SHA1(8c7c28bacc2777722a54bee32375240c9a7441d8),
               "phs4_02.rom",CRC(390df0f2) SHA1(fdf1bb96e26c0245d04be610e1a84cd4e03059ff),
               "phs5_02.rom",CRC(b524513a) SHA1(5e372e409b4d1fc5bcda55af5dc2cbf1dc66e56b))
WPC_ROMEND
INPUT_PORTS_START(ph)
  CORE_PORTS
  SIM_PORTS(0)
  PORT_START /* 0 */
    COREPORT_BIT   (  0x0001, "Coin Acceptor 1", KEYCODE_1)
    COREPORT_BIT   (  0x0002, "Coin Acceptor 2", KEYCODE_2)
    COREPORT_BIT   (  0x0004, "Coin Acceptor 3", KEYCODE_3)
    COREPORT_BIT   (  0x0010, "Coin Acceptor 4", KEYCODE_4)
    COREPORT_BIT   (  0x0008, "Coin Acceptor 5", KEYCODE_5)
    COREPORT_BITTOG(  0x0020, "Left Data Key",   KEYCODE_DEL)
    COREPORT_BITTOG(  0x0040, "Right Data Key",  KEYCODE_END)
    COREPORT_BIT   (  0x0080, "Change Language", KEYCODE_INSERT)
    COREPORT_BIT   (  0x0200, "Cancel Collect",  KEYCODE_7)
    COREPORT_BIT   (  0x0400, "Red",             KEYCODE_8)
    COREPORT_BIT   (  0x0800, "Blue",            KEYCODE_9)
    COREPORT_BIT   (  0x0100, "Super Game",      KEYCODE_0)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0001, 0x0001, "SW1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0001, "1" )
    COREPORT_DIPNAME( 0x0002, 0x0002, "SW2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0002, "1" )
    COREPORT_DIPNAME( 0x0004, 0x0000, "W20")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0004, "1" )
    COREPORT_DIPNAME( 0x0008, 0x0000, "W19")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0008, "1" )
    COREPORT_DIPNAME( 0x00f0, 0x0000, "Country")
      COREPORT_DIPSET(0x0000, "U.S.A. / Canada 1" ) \
      COREPORT_DIPSET(0x0010, "France/Coin 6-20F" ) \
      COREPORT_DIPSET(0x0020, "Germany 1" ) \
      COREPORT_DIPSET(0x0030, "France 20F Door" ) \
      COREPORT_DIPSET(0x0040, "Invalid 1" ) \
      COREPORT_DIPSET(0x0050, "Invalid 2" ) \
      COREPORT_DIPSET(0x0060, "Invalid 3" ) \
      COREPORT_DIPSET(0x0070, "Germany 2" ) \
      COREPORT_DIPSET(0x0080, "Invalid 4" ) \
      COREPORT_DIPSET(0x0090, "France Coin Setting 6" ) \
      COREPORT_DIPSET(0x00a0, "Export" ) \
      COREPORT_DIPSET(0x00b0, "France" ) \
      COREPORT_DIPSET(0x00c0, "United Kingdom" ) \
      COREPORT_DIPSET(0x00d0, "Italy" ) \
      COREPORT_DIPSET(0x00e0, "Spain" ) \
      COREPORT_DIPSET(0x00f0, "U.S.A. / Canada 2" )
INPUT_PORTS_END
CORE_GAMEDEF(ph,04,"Phantom Haus (0.4 Prototype)",1996,"Williams",wpc_m95S,0)

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
CORE_GAMEDEF(tfdmd,l3,"WPC Test Fixture: DMD (L-3)",1991,"Bally",wpc_mDMD,GAME_NO_SOUND) // Slugfest to Demolition Man

/*--------------
/ Test Fixture Security generation
/---------------*/
static core_tGameData tfsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U), 0,0,0,0,0,0,0 },
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
CORE_GAMEDEF(tfs,12,"WPC Test Fixture: Security (1.2)",1994,"Bally",wpc_mSecurityS,0) // World Cup Soccer '94 to WhoDunnit

/*--------------
/ Test Fixture WPC95
/---------------*/
static core_tGameData tf95GameData = {
  GEN_WPC95, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U), 0,0,0,0,0,0,0 },
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
CORE_GAMEDEF(tf95,12,"WPC Test Fixture: WPC-95 (1.2)",1996,"Bally",wpc_m95S,0) // Congo to the present

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
CORE_GAMEDEF(tfa,13,"WPC Test Fixture: Alphanumeric (L-3)",1990,"Bally",wpc_mAlpha,GAME_NO_SOUND) // Funhouse and The Machine:Bride of Pinbot

// games by other manufacturers

/*-------------------
/ Rush (Dave Astill)
/-------------------*/
static core_tGameData rushGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L), 0,0,0,0,0,0,0 },
  NULL,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(rush)

WPC_ROMSTART(rush,10,"rush_10.rom",0x40000,CRC(f4fb0f13) SHA1(fee2f48fa0eba35cbbe6b84d8d24710ed8454d2e))
WPC_ROMEND
#define input_ports_rush input_ports_wpc
CORE_GAMEDEF(rush,10,"Rush (1.0)",20??,"Astill Entertainment",wpc_dmd,GAME_NO_SOUND)
