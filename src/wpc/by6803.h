#ifndef INC_BY6803
#define INC_BY6803

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

#define BY6803_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define BY6803_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */
#define BY6803_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- Common Inports for BY6803 Games --*/
#define BY6803_COMPORTS \
  PORT_START /* 2 */ \
    /* Switch Column 1 */ \
	COREPORT_BIT   (0x0020, "Credit",			KEYCODE_1) \
    /* Switch Column 2 */ \
    COREPORT_BIT   (0x0040, "KP: 3/Right Coin",	KEYCODE_5) \
    COREPORT_BIT   (0x0080, "KP: 2/Left Coin",	KEYCODE_3) \
	COREPORT_BIT   (0x0100, "KP: 1/Middle Coin",KEYCODE_4) \
	COREPORT_BIT   (0x0800, "Slam Tilt",		KEYCODE_HOME) \
	COREPORT_BIT   (0x1000, "Ball Tilt",		KEYCODE_INSERT) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x2000, "Self Test",		KEYCODE_7) \
    COREPORT_BIT   (0x4000, "Sound Diagnostic",	KEYCODE_0) \
  PORT_START /* 3 */ \
	/* Switch Column 1 */ \
    COREPORT_BIT(	0x0001, "KP: Enter",    KEYCODE_8) \
    COREPORT_BIT(	0x0002, "KP: 0",        KEYCODE_0_PAD) \
	COREPORT_BIT(	0x0004, "KP: Clear",	KEYCODE_9) \
	COREPORT_BIT(	0x0008, "KP: Cancel",	KEYCODE_6) \
	/* Switch Column 2 */ \
    COREPORT_BIT(	0x0010, "KP: 3",        KEYCODE_3_PAD) \
    COREPORT_BIT(	0x0020, "KP: 2",        KEYCODE_2_PAD) \
	COREPORT_BIT(	0x0040, "KP: 1",		KEYCODE_1_PAD) \
	COREPORT_BIT(	0x0080, "KP: A",		KEYCODE_V) \
	/* Switch Column 3 */ \
    COREPORT_BIT(	0x0100, "KP: 6",        KEYCODE_6_PAD) \
    COREPORT_BIT(	0x0200, "KP: 5",        KEYCODE_5_PAD) \
	COREPORT_BIT(	0x0400, "KP: 4",		KEYCODE_4_PAD) \
	COREPORT_BIT(	0x0800, "KP: B",		KEYCODE_B) \
	/* Switch Column 4 */ \
    COREPORT_BIT(	0x1000, "KP: 9",        KEYCODE_9_PAD) \
    COREPORT_BIT(	0x2000, "KP: 8",        KEYCODE_8_PAD) \
	COREPORT_BIT(	0x4000, "KP: 7",		KEYCODE_7_PAD) \
	COREPORT_BIT(	0x8000, "KP: C",		KEYCODE_C)

#define BY6803A_COMPORTS \
  PORT_START /* 2 */ \
    /* Switch Column 1 */ \
	COREPORT_BIT   (0x0020, "Credit",			KEYCODE_1) \
    /* Switch Column 2 */ \
    COREPORT_BIT   (0x0040, "Right Coin",		KEYCODE_5) \
    COREPORT_BIT   (0x0080, "Left Coin",		KEYCODE_3) \
	COREPORT_BIT   (0x0100, "Middle Coin",		KEYCODE_4) \
	COREPORT_BIT   (0x0800, "Slam Tilt",		KEYCODE_HOME) \
	COREPORT_BIT   (0x1000, "Ball Tilt",		KEYCODE_INSERT) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x2000, "Self Test",		KEYCODE_7) \
    COREPORT_BIT   (0x4000, "Sound Diagnostic",	KEYCODE_0)

/*-- Standard input ports --*/
#define BY6803_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BY6803_COMPORTS

#define BY6803A_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    BY6803A_COMPORTS

#define BY6803_INPUT_PORTS_END INPUT_PORTS_END

#define BY6803_COMINPORT       CORE_COREINPORT

/*-- BY6803 switches are numbered from 1-64 (not column,row as WPC) --*/
#define BY6803_SWNO(x) (x)

/*-- By6803 switch numbers --*/
#define BY6803_SWSELFTEST   -7
#define BY6803_SWSOUNDDIAG  -6
//#define BY6803_SWCPUDIAG    -6
//#define BY6803_SWSOUNDDIAG  -5
#define BY6803_SWVIDEODIAG  -4

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define BY6803_CPUNO   0
#define BY6803_SCPU1NO 1
#define BY6803_SCPU2NO 2

/*-- Memory regions --*/
#define BY6803_MEMREG_CPU	REGION_CPU1
#define BY6803_MEMREG_SCPU	REGION_CPU2
#define BY6803_MEMREG_SROM  REGION_SOUND1

/*-- Main CPU regions and ROM --*/
#define BY6803_ROMSTARTx4(name, n1, chk1)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY6803_MEMREG_CPU) \
      ROM_LOAD( n1, 0xc000, 0x4000, chk1 )

#define BY6803_ROMSTART44(name,n1,chk1,n2,chk2)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY6803_MEMREG_CPU) \
      ROM_LOAD( n1, 0x8000, 0x4000, chk1) \
      ROM_LOAD( n2, 0xc000, 0x4000, chk2 )
#define BY6803_ROMEND ROM_END


/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_by6803S1;
extern struct MachineDriver machine_driver_by6803S1a;
extern struct MachineDriver machine_driver_by6803S2;
extern struct MachineDriver machine_driver_by6803S2a;
extern struct MachineDriver machine_driver_by6803S3;
extern struct MachineDriver machine_driver_by6803S3a;
extern struct MachineDriver machine_driver_by6803S4;
extern void BY6803_UpdateSoundLED(int data);

#define by_mBY6803S1	by6803S1
#define by_mBY6803S1A	by6803S1a
#define by_mBY6803S2	by6803S2
#define by_mBY6803S2A	by6803S2a
#define by_mBY6803S3	by6803S3
#define by_mBY6803S3A	by6803S3a
#define by_mBY6803S4	by6803S4


#endif /* INC_BY6803 */


#if 0
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(	0x0001, "KP: Enter",        KEYCODE_ENTER_PAD)  \
    COREPORT_BIT(	0x0004, "KP: Clear",        KEYCODE_DEL_PAD)  \
	COREPORT_BIT(	0x0008, "KP: Game",			KEYCODE_PLUS_PAD)  \
	COREPORT_BIT(	0x0020, "Credit",			KEYCODE_3) \
    /* Switch Column 2 */ \
    COREPORT_BIT(	0x0040, "KP: 3/Coin 3",     KEYCODE_3_PAD)  \
    COREPORT_BIT(	0x0080, "KP: 2/Coin 1",     KEYCODE_2_PAD)  \
	COREPORT_BIT(	0x0100, "KP: 1/Coin 2",		KEYCODE_1_PAD)  \
	COREPORT_BIT(	0x0200, "KP: A",			KEYCODE_A)  \
	COREPORT_BIT(	0x0800, "KP: 1/Coin 2",		KEYCODE_1_PAD)  \
	COREPORT_BIT(	0x1000, "Slam Tilt",		KEYCODE_HOME)  \
	COREPORT_BIT(	0x2000, "Ball Tilt",		KEYCODE_PGDN)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(	0x4000, "Self Test",        KEYCODE_7) \
    COREPORT_BIT(	0x8000, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 1 */\
	/* Switch Column 3 */ \
    COREPORT_BIT(	0x0001, "KP: 6",        KEYCODE_6_PAD)  \
    COREPORT_BIT(	0x0002, "KP: 5",        KEYCODE_5_PAD)  \
	COREPORT_BIT(	0x0004, "KP: 4",		KEYCODE_4_PAD)  \
	COREPORT_BIT(	0x0008, "KP: B",		KEYCODE_B)  \
	/* Switch Column 4 */ \
    COREPORT_BIT(	0x0010, "KP: 9",        KEYCODE_9_PAD)  \
    COREPORT_BIT(	0x0020, "KP: 8",        KEYCODE_8_PAD)  \
	COREPORT_BIT(	0x0040, "KP: 7",		KEYCODE_7_PAD)  \
	COREPORT_BIT(	0x0080, "KP: C",		KEYCODE_C)
#endif

