#ifndef INC_DE
#define INC_DE

/**************************************************************************************
  Data East Pinball -
  Hardware from 1987-1994
  CPU hardware is very similar to Williams System 11 Hardware!

  CPU Boards:
	1) CPU Rev 1 : Ram size = 2k (0x0800)	(Early Laser War Only)
	2) CPU Rev 2 : Ram size = 8k (0x2000)	(Later Laser War to Phantom of the Opera)
	3) CPU Rev 3 : CPU Controlled solenoids	(Back to the Future to Jurassic Park)
	4) CPU Rev 3b: Printer option			(Last Action Hero to Batman Forever)

  Display Boards:
	1) 520-5004-00: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Numeric), 1 X 4 Digit (7 Seg. Numeric)
	   (Used in Laser War Only)

	2) 520-5014-01: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Alphanumeric)
	   (Secret Service to Playboy)
	
	3) 520-5030-00: 2 X 16 Digit (16 Seg Alphanumeric)
		(MNF to Simpsons)
	
	4) 520-5042-00: 128X16 DMD - z80 CPU + integrated controller.
	   (Checkpoint to Hook)

	5) 520-5505 Series: 128X32 DMD - m6809 CPU + separate controller board
		a) -00 generation: (Lethal Weapon to Last Action Hero)
		b) -01 generation: (Tales From the Crypt to Guns N Roses)

	6) 520-5092-01: 192X64 DMD - 68000 CPU + separate controller board
	   (Maveric to Batman Forever)

   Sound Board Revisions: 
	1) 520-5002 Series: M6809 cpu, YM2151, MSM5205, hc4020 for stereo decoding.
		a) -00 generation, used 27256 eproms (only Laser War)
	    b) -02 generation, used 27256 & 27512 eproms (Laser War - Back to the Future)
		c) -03 generation, used 27010 voice eproms (Simpsons - Checkpoint)

	2) 520-5050-01 Series:	M6809 cpu, BSMT2000 16 bit stereo synth+dac, 2 custom PALS
		a) -01 generation,	used 27020 voice eproms (Batman - Lethal Weapon 3)
		b) -02 generation,	used 27040 voice eproms (Star Wars - J.Park)
		c) -03 generation,	similar to 02, no more info known (LAH - Maverick)
	3) 520-5077-00 Series:	??  (Tommy to Frankenstein)
	4) 520-5126-xx Series:	??	(Baywatch to Batman Forever)
*************************************************************************************/

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
