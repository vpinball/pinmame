#ifndef INC_SE
#define INC_SE

/*-------------------------
/ Machine driver constants
/--------------------------*/
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

/*-- SE switch numbers --*/
#define SE_SWBLACK   -7
#define SE_SWGREEN   -6
#define SE_SWRED     -5

/*-- Memory regions --*/
#define SE_CPUREGION	REGION_CPU1
#define SE_ROMREGION	REGION_USER1

/********************/
/** 128K CPU ROM   **/
/********************/
/*-- Main CPU regions and ROM (Up to 512MB) --*/
#define SE128_ROMSTART(name, n1, chk1) \
  ROM_START(name) \
    NORMALREGION(0x10000, SE_CPUREGION) \
    NORMALREGION(0x80000, SE_ROMREGION) \
      ROM_LOAD(n1, 0x00000, 0x20000, chk1) \
        ROM_RELOAD(0x20000, 0x20000) \
	ROM_RELOAD(0x40000, 0x20000) \
	ROM_RELOAD(0x60000, 0x20000)

#define SE_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(se2aS);
extern MACHINE_DRIVER_EXTERN(se2bS);
extern MACHINE_DRIVER_EXTERN(se2cS);
extern PINMAME_VIDEO_UPDATE(seminidmd1_update);
extern PINMAME_VIDEO_UPDATE(seminidmd2_update);
extern PINMAME_VIDEO_UPDATE(seminidmd3_update);
extern PINMAME_VIDEO_UPDATE(seminidmd4_update);
#define de_mSES1 se2aS
#define de_mSES2 se2bS
#define de_mSES3 se2cS

/* Hardware variants */
#define SE_MINIDMD    0x01
#define SE_MINIDMD2   0x02
#define SE_LED        0x04
#endif /* INC_SE */

