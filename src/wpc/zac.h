#ifndef INC_ZAC
#define INC_ZAC

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-- Common Inports for ZAC Games --*/
#define ZAC_COMPORTS_OLD \
  PORT_START /* 0 */ \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x1000, "Programming", KEYCODE_8) \
    COREPORT_BIT   (0x2000, "Sound Diag", KEYCODE_9) \
    /* These are put in switch column 1 */ \
    COREPORT_BIT   (0x0008, "Start", KEYCODE_1) \
    COREPORT_BIT   (0x0010, "Coin 1", KEYCODE_3) \
    COREPORT_BIT   (0x0020, "Coin 2", KEYCODE_4) \
    COREPORT_BIT   (0x0040, "Coin 3", KEYCODE_5) \
    COREPORT_BIT   (0x0001, "Diagnostics", KEYCODE_7) \
    COREPORT_BIT   (0x0002, "Ball Tilt", KEYCODE_INSERT) \
    COREPORT_BIT   (0x0004, "Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT   (0x0080, "Printer Log", KEYCODE_0) \
    /* These are put in switch column 2 */ \
    COREPORT_BIT   (0x8000, "Test Mode / H.S.Reset", KEYCODE_END) \
    COREPORT_BIT   (0x4000, "High Score Adjust", KEYCODE_PGUP) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Sound 1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Sound 2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    /* Dips A1-8 and B1-8 are only used on Gen1-games sshtlzac, ewf and locomotn */ \
    /* to define CMOS defaults which can later be adjusted in programming mode.  */ \
    COREPORT_DIPNAME( 0x0100, 0x0000, "A1 coin L credits") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "A2    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "A3 coin R value") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "A4    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "A5 coin R credits") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "A6    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "A7    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "A8 3 balls") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "B1 coin C value") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "B2    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "B3 coin C credits") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "B4    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "B5    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "B6 match disable") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "B7 save high score") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "B8 replay") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \

#define ZAC_COMPORTS \
  PORT_START /* 0 */ \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x1000, "Sound Diag", KEYCODE_9) \
    /* These are put in switch column 1 */ \
    COREPORT_BIT   (0x0010, "Coin 1", KEYCODE_3) \
    COREPORT_BIT   (0x0020, "Coin 2", KEYCODE_4) \
    COREPORT_BIT   (0x0040, "Coin 3", KEYCODE_5) \
    COREPORT_BIT   (0x0008, "Service Coin", KEYCODE_6) \
    COREPORT_BIT   (0x0001, "Diag. Up", KEYCODE_7) \
    COREPORT_BIT   (0x0002, "Diag. Down", KEYCODE_8) \
    COREPORT_BIT   (0x0004, "Slam Tilt", KEYCODE_HOME) \
    /* These are put in switch column 2 */ \
    COREPORT_BIT   (0x0200, "Start", KEYCODE_1) \
    COREPORT_BIT   (0x0400, "Ball Tilt", KEYCODE_INSERT) \
    COREPORT_BIT   (0x8000, "Printer Log", KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S1 cmos defaults") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S2    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3    - \" -") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4 prog enable") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0060, "Speech clock speed") \
      COREPORT_DIPSET(0x0000, "620 kHz" ) \
      COREPORT_DIPSET(0x0010, "626 kHz" ) \
      COREPORT_DIPSET(0x0020, "634 kHz" ) \
      COREPORT_DIPSET(0x0030, "640 kHz" ) \
      COREPORT_DIPSET(0x0040, "646 kHz" ) \
      COREPORT_DIPSET(0x0050, "654 kHz" ) \
      COREPORT_DIPSET(0x0060, "660 kHz" ) \
      COREPORT_DIPSET(0x0070, "666 kHz" ) \
      COREPORT_DIPSET(0x0080, "674 kHz" ) \
      COREPORT_DIPSET(0x0090, "680 kHz" ) \
      COREPORT_DIPSET(0x00a0, "686 kHz" ) \
      COREPORT_DIPSET(0x00b0, "694 kHz" ) \
      COREPORT_DIPSET(0x00c0, "700 kHz" ) \
      COREPORT_DIPSET(0x00d0, "706 kHz" ) \
      COREPORT_DIPSET(0x00e0, "714 kHz" ) \
      COREPORT_DIPSET(0x00f0, "720 kHz" )

/*-- Standard input ports --*/
#define ZACOLD_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    ZAC_COMPORTS_OLD

#define ZAC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    ZAC_COMPORTS

#define ZAC_INPUT_PORTS_END INPUT_PORTS_END

#define ZAC_COMINPORT       CORE_COREINPORT

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define ZAC_CPUNO   0
#define ZAC_SCPU1NO 1

/*-- Memory regions --*/
#define ZAC_MEMREG_CPU		REGION_CPU1
#define ZAC_MEMREG_SROM		REGION_SOUND1

/*-- Main CPU regions and ROM --*/

/* 4 X 2716 ROMS */
#define ZAC_ROMSTART8888(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4) \
  ROM_START(name) \
    NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD ( n2, 0x0800, 0x0800, chk2) \
      ROM_LOAD ( n3, 0x1000, 0x0800, chk3) \
      ROM_LOAD ( n4, 0x2000, 0x0800, chk4)

/* 5 X 2708 ROMS */
#define ZAC_ROMSTART44444(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0400, chk1) \
      ROM_LOAD ( n2, 0x0400, 0x0400, chk2) \
      ROM_LOAD ( n3, 0x0800, 0x0400, chk3) \
      ROM_LOAD ( n4, 0x0c00, 0x0400, chk4) \
      ROM_LOAD ( n5, 0x1000, 0x0400, chk5)

/* 1 X 2716, 4 X 2708 ROMS */
#define ZAC_ROMSTART84444(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD ( n2, 0x1c00, 0x0400, chk2) \
      ROM_LOAD ( n3, 0x0800, 0x0400, chk3) \
      ROM_LOAD ( n4, 0x0c00, 0x0400, chk4) \
      ROM_LOAD ( n5, 0x1000, 0x0400, chk5)

/* 2 X 2716, 3 X 2708 ROMS */
#define ZAC_ROMSTART84844(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
      ROM_LOAD ( n2, 0x1c00, 0x0400, chk2) \
      ROM_LOAD ( n3, 0x0800, 0x0400, chk3) \
        ROM_CONTINUE(0x1400, 0x0400) \
      ROM_LOAD ( n4, 0x0c00, 0x0400, chk4) \
      ROM_LOAD ( n5, 0x1000, 0x0400, chk5)

/* 3 X 2532 ROMS */
/*
	  setup_rom(1,1,0x0000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x0800);	  //U2 - 1st 2K
	  setup_rom(3,1,0x1000);	  //U3 - 1st 2K
	  setup_rom(1,2,0x2000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x2800);	  //U2 - 2nd 2K
	  setup_rom(3,2,0x3000);	  //U3 - 2nd 2K
setup mirrors
	  setup_rom(1,1,0x4000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x4800);	  //U2 - 1st 2K
	  setup_rom(3,1,0x5000);	  //U3 - 1st 2K
	  setup_rom(1,2,0x6000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x6800);	  //U2 - 2nd 2K
	  setup_rom(3,2,0x7000);	  //U3 - 2nd 2K
*/
#define ZAC_ROMSTART000(name,n1,chk1,n2,chk2,n3,chk3) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
        ROM_CONTINUE(0x2000, 0x0800) \
       ROM_RELOAD   (0x4000, 0x0800) \
        ROM_CONTINUE(0x6000, 0x0800) \
      ROM_LOAD ( n2, 0x0800, 0x0800, chk2) \
        ROM_CONTINUE(0x2800, 0x0800) \
       ROM_RELOAD   (0x4800, 0x0800) \
        ROM_CONTINUE(0x6800, 0x0800) \
      ROM_LOAD ( n3, 0x1000, 0x0800, chk3) \
        ROM_CONTINUE(0x3000, 0x0800) \
       ROM_RELOAD   (0x5000, 0x0800) \
        ROM_CONTINUE(0x7000, 0x0800)

/* 2 X 2764 ROMS */
/*
	  setup_rom(1,1,0x0000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x0800);	  //U2 - 1st 2K
	  setup_rom(2,3,0x1000);	  //U2 - 3rd 2K
	  setup_rom(1,2,0x2000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x2800);	  //U2 - 2nd 2K
	  setup_rom(2,4,0x3000);	  //U2 - 4th 2K
???	  setup_rom(1,3,0x4000);	  //U1 - 3rd 2K
???	  setup_rom(1,4,0x6000);	  //U1 - 4th 2K
setup mirrors
	  setup_rom(2,1,0x4800);	  //U2 - 1st 2K
	  setup_rom(2,3,0x5000);	  //U2 - 3rd 2K
	  setup_rom(2,2,0x6800);	  //U2 - 2nd 2K
	  setup_rom(2,4,0x7000);	  //U2 - 4th 2K
*/
#if 1
#define ZAC_ROMSTART1820(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
        ROM_CONTINUE(0x2000, 0x0800) \
        ROM_CONTINUE(0x4000, 0x0800) \
        ROM_CONTINUE(0x6000, 0x0800) \
      ROM_LOAD ( n2, 0x0800, 0x0800, chk2) \
        ROM_CONTINUE(0x2800, 0x0800) \
        ROM_CONTINUE(0x1000, 0x0800) \
        ROM_CONTINUE(0x3000, 0x0800) \
       ROM_RELOAD   (0x4800, 0x0800) \
        ROM_CONTINUE(0x6800, 0x0800) \
        ROM_CONTINUE(0x5000, 0x0800) \
        ROM_CONTINUE(0x7000, 0x0800)
#else
#define ZAC_ROMSTART1820(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
        ROM_CONTINUE(0x2000, 0x0800) \
        ROM_CONTINUE(0x1800, 0x0800) \
        ROM_CONTINUE(0x3800, 0x0800) \
       ROM_RELOAD   (0x4000, 0x0800) \
        ROM_CONTINUE(0x6000, 0x0800) \
        ROM_CONTINUE(0x5800, 0x0800) \
        ROM_CONTINUE(0x7800, 0x0800) \
      ROM_LOAD ( n2, 0x0800, 0x0800, chk2) \
        ROM_CONTINUE(0x2800, 0x0800) \
        ROM_CONTINUE(0x1000, 0x0800) \
        ROM_CONTINUE(0x3000, 0x0800) \
       ROM_RELOAD   (0x4800, 0x0800) \
        ROM_CONTINUE(0x6800, 0x0800) \
        ROM_CONTINUE(0x5000, 0x0800) \
        ROM_CONTINUE(0x7000, 0x0800)
#endif

/* 1 X 2532, 1 X 2564 ROMS */
#define ZAC_ROMSTART020(name,n1,chk1,n2,chk2) \
  ROM_START(name) \
	NORMALREGION(0x8000, ZAC_MEMREG_CPU) \
      ROM_LOAD ( n1, 0x0000, 0x0800, chk1) \
        ROM_CONTINUE(0x2000, 0x0800) \
       ROM_RELOAD   (0x4000, 0x0800) \
        ROM_CONTINUE(0x6000, 0x0800) \
      ROM_LOAD ( n2, 0x0800, 0x0800, chk2) \
        ROM_CONTINUE(0x2800, 0x0800) \
        ROM_CONTINUE(0x1000, 0x0800) \
        ROM_CONTINUE(0x3000, 0x0800) \
       ROM_RELOAD   (0x4800, 0x0800) \
        ROM_CONTINUE(0x6800, 0x0800) \
        ROM_CONTINUE(0x5000, 0x0800) \
        ROM_CONTINUE(0x7000, 0x0800)

#define ZAC_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(ZAC0);
extern MACHINE_DRIVER_EXTERN(ZAC1311);
extern MACHINE_DRIVER_EXTERN(ZAC1125);
extern MACHINE_DRIVER_EXTERN(ZAC1144);
extern MACHINE_DRIVER_EXTERN(ZAC1346);
extern MACHINE_DRIVER_EXTERN(ZAC1146);
extern MACHINE_DRIVER_EXTERN(ZAC2A);
extern MACHINE_DRIVER_EXTERN(ZAC2X);
extern MACHINE_DRIVER_EXTERN(ZAC2XS);
extern MACHINE_DRIVER_EXTERN(ZAC2XS2);
extern MACHINE_DRIVER_EXTERN(ZAC2XS2A);
extern MACHINE_DRIVER_EXTERN(ZAC2XS3);
extern MACHINE_DRIVER_EXTERN(TECHNO);

#define mZAC0     ZAC0
#define mZAC1311  ZAC1311
#define mZAC1125  ZAC1125
#define mZAC1144  ZAC1144
#define mZAC1346  ZAC1346
#define mZAC1146  ZAC1146
#define mZAC2A    ZAC2A
#define mZAC2X    ZAC2X
#define mZAC2XS   ZAC2XS
#define mZAC2XS2  ZAC2XS2
#define mZAC2XS2A ZAC2XS2A
#define mZAC2XS3  ZAC2XS3
#define mTECHNO   TECHNO

#endif /* INC_ZAC */
