#include "driver.h"
#include "wpc.h"
#include "sim.h"
#include "dcs.h"
#include "wpcsound.h"

/* 251200 Moved all the rest (except non-pin and the test fixtures) to own
          Simulator files (ML) */
/* 031200 Moved Indiana Jones to Own Simulator File (SJE) */
/* 021200 Moved Judge Dredd & WW to Own Simulator File (SJE)*/
/* 011200 Added Judge Dredd Sound Support (SJE) */
/* 011200 Moved Dr. Dude to Own Simulator File (SJE) */
/* 291100 Moved Dracula to Own Simulator File (SJE) */
/* 271100 Added Dracula Sound Support (SJE) */
/* 261100 Moved Black Rose & BOP to Own Simulator File (SJE) */
/* 251100 Moved Roadshow & Hurricane to Own Simulator File (SJE) */
/* 231100 Added Black Rose Sound Support (SJE) */
/* 201100 Added Indiana Jones, No Good Golphers Sound Support (SJE) */
/* 201100 Moved Gilligan's Island to Own Simulator File (SJE) */
/* 191100 Moved Getaway & Fish Tales to Own Simulator File (SJE) */
/* 191100 Updated Party Zone Support for good working romset (SJE) */
/* 191100 Added Cirqus Voltaire, Corvette, No Fear,
	  Harley Davidson, Getaway, Indy500 Sound Support (SJE) */
/* 151100 Moved CFTBL,STTNG,WC94,PZ to Own Simulator Files (SJE) */
/* 071100 Added Roadhshow Sound Support (SJE) */
/* 061100 Added Popeye, BOP, and Safe Cracker Sound Support (SJE) */
/* 051100 Added TAF Sound Support (SJE) */
/* 021100 Added Party Zone Sound Support (SJE) */
/* 011100 Moved Funhouse and T2 To Own Simulator Files (SJE) */
/* 301000 Added WCS94 Sound Support (SJE) */
/* 291000 Added T2 and WW Sound Support (SJE) */
/* 171000 Added FT sound, NBA S3.0 (WPCMAME) */
/* 181000 Corrected games year and names (WPCMAME)*/

#define swStart     13
#define swTilt      14
#define swSlamTilt  21
#define swCoinDoor  22

#define INITGAME(name, balls) \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
WPC_INPUT_PORTS_START(name, balls) WPC_INPUT_PORTS_END

/*--------------------
/ Slugfest baseball
/--------------------*/
static core_tGameData sfGameData = {
  GEN_WPCDMD, NULL,
  { 0 },
  NULL,
  { "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /* Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor, 0}
  }
};
INITGAME(sf,0/*?*/)

WPC_ROMSTART(sf,l1,"sf_u6.l1",0x40000,0xada93967)
WPCS_SOUNDROM222("sf_u18.l1",0x78092c83,
                 "sf_u15.l1",0xadcaeaa1,
                 "sf_u14.l1",0xb830b419)
WPC_ROMEND

CORE_GAMEDEF(sf,l1,"Slugfest (L-1)",1991,"Williams",wpc_mDMDS,0)

/*-------------
/ Ticket Tac Toe
/--------------*/
static core_tGameData tttGameData = {
  GEN_WPC95, NULL,
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
INITGAME(ttt,0/*?*/)

WPC_ROMSTART(ttt,10,"tikt1_0.rom",0x80000,0xbf1d0382) WPC_ROMEND

CORE_GAMEDEF(ttt,10,"Ticket Tac Toe (1.0)",1996,"Williams",wpc_m95,GAME_NO_SOUND)

/*-----------------------------
/ League Champ (Shuffle Alley)
/------------------------------*/
static core_tGameData lcGameData = {
  GEN_WPCFLIPTRON, NULL,
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
INITGAME(lc,0/*?*/)

WPC_ROMSTART(lc,11,"lchmp1_1.rom",0x80000,0x60ab944c) WPC_ROMEND

CORE_GAMEDEF(lc,11,"League Champ (1.1)",1996,"Bally",wpc_mFliptron,GAME_NO_SOUND)

/*--------------
/ Test Fixture DMD generation
/---------------*/
static core_tGameData tfdmdGameData = {
  GEN_WPCDMD, NULL,
  { 0 },
  NULL,
  { "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /*Start    Tilt    SlamTilt    CoinDoor    Shooter */
    { swStart, swTilt, swSlamTilt, swCoinDoor}
  }
};
INITGAME(tfdmd,0)

WPC_ROMSTART(tfdmd,l3,"u6_l3.rom",0x20000,0xbd43e28c) WPC_ROMEND

CORE_GAMEDEF(tfdmd,l3,"WPC Test Fixture: DMD (L-3)",1991,"Bally",wpc_mDMD,GAME_NO_SOUND)

/*--------------
/ Test Fixture Security generation
/---------------*/
static core_tGameData tfsGameData = {
  GEN_WPCSECURITY, NULL,
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
INITGAME(tfs,0)

WPC_ROMSTART(tfs,12,"u6_12.rom",0x80000,0x12687d19)
DCS_SOUNDROM1x("u2_10.rom",0xd705b41e)
WPC_ROMEND

CORE_GAMEDEF(tfs,12,"WPC Test Fixture: Security (1.2)",1994,"Bally",wpc_mSecurityS,0)

/*--------------
/ Test Fixture WPC95
/---------------*/
static core_tGameData tf95GameData = {
  GEN_WPC95, NULL,
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
INITGAME(tf95,0)

WPC_ROMSTART(tf95,12,"g11_12.rom",0x80000,0x259a2b23)
DCS_SOUNDROM1m("s2_10.rom",0xceff7fe4)
WPC_ROMEND

CORE_GAMEDEF(tf95,12,"WPC 95 Test Fixture (1.2)",1996,"Bally",wpc_m95S,0)

/*===========
/  Test Ficture Alphanumeric
/============*/
static core_tGameData tfaGameData = {
  GEN_WPCALPHA_2, NULL,
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
INITGAME(tfa,0)

WPC_ROMSTART(tfa,13,"u6_l3.rom",0x020000,0xbf4a37b5) WPC_ROMEND

CORE_GAMEDEF(tfa,13,"WPC Test Fixture: Alphanumeric (1.3)",1990,"Bally",wpc_mAlpha,GAME_NO_SOUND)
