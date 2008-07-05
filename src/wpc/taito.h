#ifndef INC_TAITO
#define INC_TAITO

/*-- Common Inports for Taito Games --*/
#define TAITO_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 ( 0 in game) */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0004, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BITDEF(  0x0008, IPT_TILT,           KEYCODE_INSERT) \
    COREPORT_BITTOG(  0x0010, "Reset",			  KEYCODE_DEL)  \
    COREPORT_BITTOG(  0x0020, "Coin Door",        KEYCODE_END)  \
    COREPORT_BITTOG(  0x0040, "Box  Door",        KEYCODE_PGDN)  \
    COREPORT_BITDEF(  0x0080, IPT_COIN2,          IP_KEY_DEFAULT) \
    /* Switch Column 8 (7 in game) */ \
    COREPORT_BIT(     0x0100, "Diagnostics",	  KEYCODE_6) \
    COREPORT_BIT(     0x0200, "Statistics",	      KEYCODE_7) \
    COREPORT_BIT(     0x0400, "Adjustments",	  KEYCODE_8) \
    COREPORT_BIT(     0x0800, "Configurations",   KEYCODE_9) \
    COREPORT_BIT(     0x1000, "Enter",			  KEYCODE_0) \
    COREPORT_BIT(     0x8000, "Sound Test",		  KEYCODE_PGUP) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Adjust 1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "Adjust 2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Adjust 3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Adjust 4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Adjust 5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Adjust 6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Adjust 7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Adjust 8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Sound 1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "Sound 2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Sound 3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Sound 4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Sound 5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Sound 6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Sound 7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Sound 8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )

/*-- Standard input ports --*/
#define TAITO_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    TAITO_COMPORTS

#define TAITO_INPUT_PORTS_END INPUT_PORTS_END

#define TAITO_COMINPORT       CORE_COREINPORT

/*-- Taito switch numbers --*/
#define TAITO_SWSELFTEST   -7
#define TAITO_SWCPUDIAG    -6
#define TAITO_SWSOUNDDIAG  -5

/*-------------------------
/ Machine driver constants
/--------------------------*/
/*-- Memory regions --*/
#define TAITO_MEMREG_CPU  REGION_CPU1
#define TAITO_MEMREG_SCPU REGION_CPU2

#define TAITO_CPU		0
#define TAITO_SCPU		1

/*-- Main CPU regions and ROM --*/
#define TAITO_ROMSTART11111(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0400, chk1) \
      ROM_LOAD( n2, 0x0400, 0x0400, chk2) \
      ROM_LOAD( n3, 0x0800, 0x0400, chk3) \
      ROM_LOAD( n4, 0x0c00, 0x0400, chk4) \
      ROM_LOAD( n5, 0x1800, 0x0400, chk5)

/*-- Main CPU regions and ROM --*/
#define TAITO_ROMSTART22222(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3) \
      ROM_LOAD( n4, 0x1800, 0x0800, chk4) \
      ROM_LOAD( n5, 0x2000, 0x0800, chk5) \
        ROM_RELOAD( 0x4800, 0x0800)

/*-- Main CPU regions and ROM --*/
#define TAITO_ROMSTART2222(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3) \
      ROM_LOAD( n4, 0x1800, 0x0800, chk4)

#define TAITO_ROMSTART22_2(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3) \
        ROM_RELOAD( 0x1800, 0x0800)

#define TAITO_ROMSTART2(name,n1,chk1) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
        ROM_RELOAD( 0x1800, 0x0800)

#define TAITO_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(taito);
extern MACHINE_DRIVER_EXTERN(taito_old);
extern MACHINE_DRIVER_EXTERN(taito_sintetizador);
extern MACHINE_DRIVER_EXTERN(taito_sintetizadorpp);
extern MACHINE_DRIVER_EXTERN(taito_sintevox);
extern MACHINE_DRIVER_EXTERN(taito_sintevoxpp);

#endif /* INC_TAITO */

