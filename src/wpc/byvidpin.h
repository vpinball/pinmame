#ifndef INC_BYVP
#define INC_BYVP

/* DIP SWITCH SETTINGS: 

 05 04 03 02 01 (Right Side Coin Chute)		Credits Per Coin
 13 12 11 10 09 (Left Side Coin Chute)
 --------------
  0 0 0 0 0  1/1 Coin
  ....
  1 1 1 1 1  2/5 Coins

  01 - 05 (See Coin Settings Above)
  06 = Memory for Center Arrow (ON = YES, OFF = NO)
  07 = Show Fruit @ Start of Game (ON = YES, OFF = NO)
  08 = Side Tunnels Open @ Start of Game (ON = YES, OFF = NO)
  09 - 13 (See Coin Settings Above)
  14 = Outhole Ball Adjustmeng:
       (ON = After 3 Ball ejects and no score, ball still in play!)
	   (OFF = After 3 Ball ejects and no score, maze will lite)

  15 = Energizer Stays On From Pac To Pac (ON = YES, OFF = NO)
  16 = Operate Pinball w/o Video Portion (for troubleshooting)
  17 = ??
  18 = ??
  19 = ??
  20 = ??
  21 = ??
  23 22 Special Given When Completing X # of mazes
  -----
   x  0 Complete 3 Mazes for a Special
   0  1 Complete 4 Mazes for a Special
   1  1 Complete 5 Mazes for a Special

  24 = Playfield Drain Keeps Center Arrows
  25 = ??
  26 = ??
  27 = Display # of Credits
  28 = ??
  29 = ??
  30 = Free Play? (ON = YES, OFF = NO)

  32 31	# of Pacman (Lives) per game
  -----
  0   0  3
  0   1  5
  1   0  4
  1   1  2
*/

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define BYVP_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define BYVP_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define BYVP_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for BY_VP Games --*/
#define BYVP_COMPORTS \
  PORT_START /* 0 */ \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0001, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(     0x0002, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0004, "Sound Diagnostic", KEYCODE_0) \
    COREPORT_BIT(     0x0008, "Video Diagnostic", KEYCODE_8) \
	COREPORT_BIT(	  0x0010, "Joystick Right",	  KEYCODE_RIGHT) \
    COREPORT_BIT(	  0x0020, "Joystick Left",	  KEYCODE_LEFT) \
	COREPORT_BIT(	  0x0040, "Joystick Down",	  KEYCODE_DOWN) \
	COREPORT_BIT(	  0x0080, "Joystick Up",	  KEYCODE_UP) \
    /* Switch Column 1 (Switches #3 & #6)*/ \
    COREPORT_BITDEF(  0x0100, IPT_START2,        KEYCODE_2)  \
    COREPORT_BITDEF(  0x0200, IPT_START1,        KEYCODE_1)  \
    /* Switch Column 2 (Switches #1,#2,#7,#8)*/ \
    COREPORT_BITDEF(  0x0400, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0800, IPT_COIN2,          IP_KEY_DEFAULT) \
	COREPORT_BIT(     0x1000, "Ball Tilt",        KEYCODE_DEL)  \
    COREPORT_BIT(     0x2000, "Slam Tilt",        KEYCODE_HOME)  \
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
    COREPORT_DIPNAME( 0x0040, 0x0040, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )


/*-- Standard input ports --*/
#define BYVP_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BYVP_COMPORTS

#define BYVP_INPUT_PORTS_END INPUT_PORTS_END

#define BYVP_COMINPORT       CORE_COREINPORT

/*-- BYVP switches are numbered from 1-64 (not column,row as WPC) --*/
#define BYVP_SWNO(x) (x)

/*-- BYVP switch numbers --*/
#define BYVP_SWSELFTEST   -7	//SW Col 0 - Switch #1
#define BYVP_SWCPUDIAG    -6	//SW Col 0 - Switch #2
#define BYVP_SWSOUNDDIAG  -5	//SW Col 0 - Switch #3
#define BYVP_SWVIDEODIAG  -4	//SW Col 0 - Switch #4
#define BYVP_SWJOYRIGHT   -3	//SW Col 0 - Switch #5
#define BYVP_SWJOYLEFT    -2	//SW Col 0 - Switch #6
#define BYVP_SWJOYDOWN	  -1	//SW Col 0 - Switch #7
#define BYVP_SWJOYUP	  -0	//SW Col 0 - Switch #8

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define BYVP_CPUNO   0
#define BYVP_SCPU1NO 1
#define BYVP_SCPU2NO 2

/*-- Memory regions --*/
#define BYVP_MEMREG_CPU		REGION_CPU1
#define BYVP_MEMREG_VCPU	REGION_CPU2
#define BYVP_MEMREG_SCPU	REGION_CPU3
#define BYVP_MEMREG_SROM    REGION_SOUND1

/*-- Main CPU regions and ROM --*/
#define BYVP_ROMSTARTx00(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7)\
  ROM_START(name) \
    NORMALREGION(0x10000, BYVP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_CONTINUE( 0x5000, 0x0800) /* ?? */ \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2 ) \
      ROM_CONTINUE( 0x5800, 0x0800) \
      ROM_RELOAD(   0xf000, 0x1000) \
    NORMALREGION(0x10000, BYVP_MEMREG_VCPU) \
	  ROM_LOAD( n3, 0x8000, 0x2000, chk3) \
	  ROM_LOAD( n4, 0xa000, 0x2000, chk4) \
	  ROM_LOAD( n5, 0xc000, 0x2000, chk5) \
	  ROM_LOAD( n6, 0xe000, 0x2000, chk6) \
    NORMALREGION(0x10000, BYVP_MEMREG_SCPU) \
	  ROM_LOAD( n7, 0xe000, 0x2000, chk7)  

#define BYVP_ROMSTART100(name,n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8,n9,chk9)\
  ROM_START(name) \
    NORMALREGION(0x10000, BYVP_MEMREG_CPU) \
      ROM_LOAD( n1, 0x1000, 0x0800, chk1) \
      ROM_CONTINUE( 0x5000, 0x0800) /* ?? */ \
      ROM_LOAD( n2, 0x1800, 0x0800, chk2 ) \
      ROM_CONTINUE( 0x5800, 0x0800) \
      ROM_RELOAD(   0xf000, 0x1000) \
    NORMALREGION(0x10000, BYVP_MEMREG_VCPU) \
	  ROM_LOAD( n3, 0x4000, 0x2000, chk3) \
	  ROM_LOAD( n4, 0x6000, 0x2000, chk4) \
	  ROM_LOAD( n5, 0x8000, 0x2000, chk5) \
	  ROM_LOAD( n6, 0xa000, 0x2000, chk6) \
	  ROM_LOAD( n7, 0xc000, 0x2000, chk7) \
	  ROM_LOAD( n8, 0xe000, 0x2000, chk8) \
    NORMALREGION(0x10000, BYVP_MEMREG_SCPU) \
	  ROM_LOAD( n9, 0xe000, 0x2000, chk9)  

#define BYVP_ROMEND ROM_END


/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_byVP;
extern struct MachineDriver machine_driver_byVP2;
#define byVP_mVP1		byVP
#define byVP_mVP2		byVP2

#endif /* INC_BYVP */

