#ifndef INC_SE
#define INC_SE

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SE_ROMEND	ROM_END

/*-- Common Inports for SEGames --*/
#define SE_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 0 */ \
    COREPORT_BIT(     0x0008, "Black Button", KEYCODE_0) \
    COREPORT_BIT(     0x0004, "Green Button", KEYCODE_9) \
    COREPORT_BIT(     0x0002, "Red Button",   KEYCODE_8) \
    COREPORT_BITTOG(  0x0001, "N/A",          KEYCODE_7) \
    /* Switch Column 6 - but adjusted to begin at 6th Switch */\
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0020, IPT_TILT,           KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0040, "Slam Tilt",        KEYCODE_HOME)  \
    /* Switch Column 1 - but adjusted to begin at 4th Switch */ \
    COREPORT_BITDEF(  0x0100, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0200, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0400, IPT_COIN2,          IP_KEY_DEFAULT) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0000, "Dip SW300") \
      COREPORT_DIPSET(0x0000, "USA" ) \
      COREPORT_DIPSET(0x0001, "Austria" ) \
      COREPORT_DIPSET(0x0002, "Belgium" ) \
      COREPORT_DIPSET(0x000d, "Brazil" ) \
      COREPORT_DIPSET(0x0003, "Canada" ) \
      COREPORT_DIPSET(0x0006, "France" ) \
      COREPORT_DIPSET(0x0007, "Germany" ) \
      COREPORT_DIPSET(0x0008, "Italy" ) \
      COREPORT_DIPSET(0x0009, "Japan" ) \
      COREPORT_DIPSET(0x0004, "Netherlands" ) \
      COREPORT_DIPSET(0x000a, "Norway" ) \
      COREPORT_DIPSET(0x000b, "Sweden" ) \
      COREPORT_DIPSET(0x000c, "Switzerland" ) \
      COREPORT_DIPSET(0x0005, "UK" ) \
      COREPORT_DIPSET(0x000e, "UK (New)" )

/*-- Standard input ports --*/
#define SE_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SE_COMPORTS

#define SE_INPUT_PORTS_END INPUT_PORTS_END

#define SE_COMINPORT       CORE_COREINPORT

#define SE_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define SE_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define SE_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- SE switches are numbered from 1-64 (not column,row as WPC) --*/
#define SE_SWNO(x) ((((x)+7)/8)*10+(((x)-1)%8)+1)

/*-- SE switch numbers --*/
#define SE_SWRED      3
#define SE_SWGREEN    2
#define SE_SWBLACK    1


/*-- Memory regions --*/
#define SE_MEMREG_ROM	REGION_USER1
#define SE_MEMREG_DROM1	REGION_USER2

#define SE_MEMREG_CPU	REGION_CPU1
#define SE_MEMREG_DCPU1 REGION_CPU2
#define SE_MEMREG_SCPU1	REGION_CPU3
#define SE_MEMREG_SROM1	REGION_SOUND1


/********************/
/** 128K CPU ROM   **/
/********************/
/*-- Main CPU regions and ROM (Up to 512MB) --*/
#define SE128_ROMSTART(name, n1, chk1) \
  ROM_START(name) \
    NORMALREGION(0x10000, SE_MEMREG_CPU) \
    NORMALREGIONE(0x80000, SE_MEMREG_ROM) \
      ROM_LOAD(n1, 0x00000, 0x20000, chk1) \
        ROM_RELOAD(0x20000, 0x20000) \
	ROM_RELOAD(0x40000, 0x20000) \
	ROM_RELOAD(0x60000, 0x20000)

/**************************************/
/** DMD (128x32) ROM 512K            **/
/**************************************/
#define SE_DMD524_ROMSTART(n1,chk1) \
  NORMALREGION(0x10000, SE_MEMREG_DCPU1)\
  NORMALREGIONE(0x80000, SE_MEMREG_DROM1) \
    ROM_LOAD(n1, 0x00000, 0x80000, chk1)

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_SE_1S;
extern struct MachineDriver machine_driver_SE_2S;
extern struct MachineDriver machine_driver_SE_3S;

#define de_mSES1        SE_1S
#define de_mSES2        SE_2S
#define de_mSES3        SE_3S
#endif /* INC_SE */

