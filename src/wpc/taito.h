#ifndef INC_TAITO
#define INC_TAITO

/*-- Common Inports for Taito Games --*/
#define TAITO_COMPORTS \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BIT(     0x0004, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BITDEF(  0x0008, IPT_TILT,           KEYCODE_INSERT) \
    COREPORT_BITTOG(  0x0010, "Reset",			  KEYCODE_DEL)  \
    COREPORT_BITTOG(  0x0020, "Coin Door",        KEYCODE_END)  \
    COREPORT_BITTOG(  0x0040, "Box  Door",        KEYCODE_PGDN)  \
    COREPORT_BITDEF(  0x0080, IPT_COIN2,          IP_KEY_DEFAULT) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \

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
#define TAITO_MEMREG_CPU REGION_CPU1

#define TAITO_CPU		0

/*-- Main CPU regions and ROM --*/
#define TAITO_ROMSTART4444(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3) \
      ROM_LOAD( n4, 0x1800, 0x0800, chk4)

#define TAITO_ROMSTART444(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
    NORMALREGION(0x10000, TAITO_MEMREG_CPU) \
      ROM_LOAD( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD( n3, 0x1000, 0x0800, chk3) \

#define TAITO_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(taito);

#endif /* INC_TAITO */

