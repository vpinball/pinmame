#ifndef INC_DE
#define INC_DE

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define DE_CPUNO	0
#define DE_CPUREV1	0x00000080
#define DE_CPUREV2	0x00000100
#define DE_CPUREV3	0x00000200

#define DE_ALPHA1	0x00000400
#define DE_ALPHA2	0x00000800
#define DE_ALPHA3	0x00001000

#define DE_ROMEND	ROM_END

/*-- Common Inports for DEGames --*/
#define DE_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_TILT,           KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    COREPORT_BITDEF(  0x0004, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN3,          KEYCODE_3) \
	COREPORT_BITDEF(  0x0020, IPT_COIN4,          KEYCODE_3) \
    COREPORT_BIT(     0x0040, "Slam Tilt",        KEYCODE_HOME)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Black Button",     KEYCODE_8) \
    COREPORT_BITTOG(  0x0200, "Green Button",     KEYCODE_7) \

/*-- Standard input ports --*/
#define DE_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    DE_COMPORTS

#define DE_INPUT_PORTS_END INPUT_PORTS_END

#define DE_COMINPORT       CORE_COREINPORT

#define DE_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define DE_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define DE_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- DE switches are numbered from 1-64 (not column,row as WPC) --*/
#define DE_SWNO(x) ((((x)+7)/8)*10+(((x)-1)%8)+1)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define DE_CSOL(x) ((x)+24)

/*-- DE switch numbers --*/
#define DE_SWADVANCE     -7
#define DE_SWUPDN        -6
#define DE_SWCPUDIAG     -5
#define DE_SWSOUNDDIAG   -8


/*-- Memory regions --*/
#define DE_MEMREG_CPU		REGION_CPU1

#define DE_MEMREG_SCPU1		REGION_CPU2	/*Sound is CPU #2 for alpha games*/
#define DE_MEMREG_DCPU1		REGION_CPU2 /*DMD is CPU #2 for DMD games*/
#define DE_MEMREG_SDCPU1	REGION_CPU3 /*Sound is CPU #3 for DMD games*/

#define DE_MEMREG_SROM1		REGION_USER1 /*Sound rom loaded into Region #1*/
#define DE_MEMREG_DROM1		REGION_USER1 /*DMD rom loaded into Region #1*/
#define DE_MEMREG_SDROM1	REGION_USER2 /*Sound rom on DMD games loaded into Region #2*/

/**************************************/
/******* GAMES USING 2 ROMS     *******/
/**************************************/

/********************/
/** 16K & 32K ROMS **/
/********************/

/*-- Main CPU regions and ROM --*/
#define DE1632_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, DE_MEMREG_CPU) \
       ROM_LOAD(n1, 0x4000, 0x4000, chk1) \
       ROM_LOAD(n2, 0x8000, 0x8000, chk2)

/***********************************************/
/** 2 X 32K(1st 8K of B5 Chip is Blank) ROMS  **/
/***********************************************/

/*-- Main CPU regions and ROM --*/
#define DE3232_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, DE_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
       ROM_LOAD(n2, 0x8000, 0x8000, chk2)

/**************************************/
/******* GAMES USING ONLY 1 ROM *******/
/**************************************/

/***********************/
/** 32K in 1 Rom Only **/
/***********************/

/*-- Main CPU regions and ROM --*/
#define DE32_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, DE_MEMREG_CPU) \
		ROM_LOAD(n1, 0x8000, 0x8000, chk1)

/**************************************/
/** 64K(1st 8K of Chip is Blank) ROM **/
/**************************************/

/*-- Main CPU regions and ROM --*/
#define DE64_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
       NORMALREGION(0x10000, DE_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x10000, chk1) 

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_DE_AS;
extern struct MachineDriver machine_driver_DE_NO;

#define de_mDE_AS     DE_AS
#define de_mDE_NO     DE_NO

#endif /* INC_DE */
