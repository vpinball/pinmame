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

/*--------------------
/ Slugfest baseball
/--------------------*/
static core_tGameData sfGameData = {
  GEN_WPCDMD, wpc_dispDMD,
  { 0 },
  NULL,
  { "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};
INITGAME(sf)

WPC_ROMSTART(sf,l1,"sf_u6.l1",0x40000,0xada93967)
WPCS_SOUNDROM222("sf_u18.l1",0x78092c83,
                 "sf_u15.l1",0xadcaeaa1,
                 "sf_u14.l1",0xb830b419)
WPC_ROMEND
#define input_ports_sf input_ports_wpc
CORE_GAMEDEF(sf,l1,"Slugfest (L-1)",1991,"Williams",wpc_mDMDS,0)

/*-------------------
/ Strike Master
/--------------------*/
static core_tGameData strikGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  { 0 },
  NULL,
  { "",
    { 0 }, /* No inverted switches */
    /* Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};
INITGAME(strik)
WPC_ROMSTART(strik,l4,"strik_l4.rom",0x40000,0xc99ea24c)
WPC_ROMEND
#define input_ports_strik input_ports_wpc
CORE_GAMEDEF(strik,l4,"Strike Master (L-4)",1992,"Williams",wpc_mFliptron,GAME_NO_SOUND)

/*-------------
/ Ticket Tac Toe
/--------------*/
static core_tGameData tttGameData = {
  GEN_WPC95, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L) },
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

WPC_ROMSTART(ttt,10,"tikt1_0.rom",0x80000,0xbf1d0382) WPC_ROMEND
#define input_ports_ttt input_ports_wpc
CORE_GAMEDEF(ttt,10,"Ticket Tac Toe (1.0)",1996,"Williams",wpc_m95,GAME_NO_SOUND)

/*-----------------------------
/ League Champ (Shuffle Alley)
/------------------------------*/
static core_tGameData lcGameData = {
  GEN_WPCFLIPTRON, wpc_dispDMD,
  { 0 },
  NULL,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(lc)
WPC_ROMSTART(lc,11,"lchmp1_1.rom",0x80000,0x60ab944c) WPC_ROMEND
#define input_ports_lc input_ports_wpc
CORE_GAMEDEF(lc,11,"League Champ (1.1)",1996,"Bally",wpc_mFliptron,GAME_NO_SOUND)

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
WPC_ROMSTART(tfdmd,l3,"u6_l3.rom",0x20000,0xbd43e28c) WPC_ROMEND
#define input_ports_tfdmd input_ports_wpc
CORE_GAMEDEF(tfdmd,l3,"WPC Test Fixture: DMD (L-3)",1991,"Bally",wpc_mDMD,GAME_NO_SOUND)

/*--------------
/ Test Fixture Security generation
/---------------*/
static core_tGameData tfsGameData = {
  GEN_WPCSECURITY, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U) },
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

WPC_ROMSTART(tfs,12,"u6_12.rom",0x80000,0x12687d19)
DCS_SOUNDROM1x("u2_10.rom",0xd705b41e)
WPC_ROMEND
#define input_ports_tfs input_ports_wpc
CORE_GAMEDEF(tfs,12,"WPC Test Fixture: Security (1.2)",1994,"Bally",wpc_mSecurityS,0)

/*--------------
/ Test Fixture WPC95
/---------------*/
static core_tGameData tf95GameData = {
  GEN_WPC95, wpc_dispDMD,
  { FLIP_SW(FLIP_L | FLIP_U) | FLIP_SOL(FLIP_L | FLIP_U) },
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

WPC_ROMSTART(tf95,12,"g11_12.rom",0x80000,0x259a2b23)
DCS_SOUNDROM1m("s2_10.rom",0xceff7fe4)
WPC_ROMEND
#define input_ports_tf95 input_ports_wpc
CORE_GAMEDEF(tf95,12,"WPC 95 Test Fixture (1.2)",1996,"Bally",wpc_m95S,0)

/*===========
/  Test Ficture Alphanumeric
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
WPC_ROMSTART(tfa,13,"u6_l3.rom",0x020000,0xbf4a37b5) WPC_ROMEND
#define input_ports_tfa input_ports_wpc
CORE_GAMEDEF(tfa,13,"WPC Test Fixture: Alphanumeric (1.3)",1990,"Bally",wpc_mAlpha,GAME_NO_SOUND)
