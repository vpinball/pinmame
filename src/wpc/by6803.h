#ifndef INC_BY6803
#define INC_BY6803

/*-- Common Inports for BY6803 Games --*/
#define BY6803_COMPORTS \
  PORT_START /* 2 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT   (0x0020, "Credit",           KEYCODE_1) \
    /* Switch Column 2 */ \
    COREPORT_BIT   (0x0040, "KP: 3/Right Coin", KEYCODE_5) \
    COREPORT_BIT   (0x0080, "KP: 2/Left Coin",  KEYCODE_3) \
    COREPORT_BIT   (0x0100, "KP: 1/Middle Coin",KEYCODE_4) \
    COREPORT_BIT   (0x0800, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BIT   (0x1000, "Ball Tilt",        KEYCODE_INSERT) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x2000, "Self Test",        KEYCODE_7) \
    COREPORT_BIT   (0x4000, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 3 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(   0x0001, "KP: Enter",    KEYCODE_8) \
    COREPORT_BIT(   0x0002, "KP: 0",        KEYCODE_0_PAD) \
    COREPORT_BIT(   0x0004, "KP: Clear",    KEYCODE_9) \
    COREPORT_BIT(   0x0008, "KP: Cancel",   KEYCODE_6) \
    /* Switch Column 2 */ \
    COREPORT_BIT(   0x0010, "KP: 3",        KEYCODE_3_PAD) \
    COREPORT_BIT(   0x0020, "KP: 2",        KEYCODE_2_PAD) \
    COREPORT_BIT(   0x0040, "KP: 1",        KEYCODE_1_PAD) \
    COREPORT_BIT(   0x0080, "KP: A",        KEYCODE_V) \
    /* Switch Column 3 */ \
    COREPORT_BIT(   0x0100, "KP: 6",        KEYCODE_6_PAD) \
    COREPORT_BIT(   0x0200, "KP: 5",        KEYCODE_5_PAD) \
    COREPORT_BIT(   0x0400, "KP: 4",        KEYCODE_4_PAD) \
    COREPORT_BIT(   0x0800, "KP: B",        KEYCODE_B) \
    /* Switch Column 4 */ \
    COREPORT_BIT(   0x1000, "KP: 9",        KEYCODE_9_PAD) \
    COREPORT_BIT(   0x2000, "KP: 8",        KEYCODE_8_PAD) \
    COREPORT_BIT(   0x4000, "KP: 7",        KEYCODE_7_PAD) \
    COREPORT_BIT(   0x8000, "KP: C",        KEYCODE_C)

#define BY6803A_COMPORTS \
  PORT_START /* 2 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT   (0x0020, "Credit",           KEYCODE_1) \
    /* Switch Column 2 */ \
    COREPORT_BIT   (0x0040, "Right Coin",       KEYCODE_5) \
    COREPORT_BIT   (0x0080, "Left Coin",        KEYCODE_3) \
    COREPORT_BIT   (0x0100, "Middle Coin",      KEYCODE_4) \
    COREPORT_BIT   (0x0800, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BIT   (0x1000, "Ball Tilt",        KEYCODE_INSERT) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT   (0x2000, "Self Test",        KEYCODE_7) \
    COREPORT_BIT   (0x4000, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 3 */ \
    /* No dips but keep an empty port so all 6803 got the same size */

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
#define BY6803_CPUREGION   REGION_CPU1

/*-- Main CPU regions and ROM --*/
#define BY6803_ROMSTARTx4(name, n1, chk1)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY6803_CPUREGION) \
      ROM_LOAD( n1, 0xc000, 0x4000, chk1 )

#define BY6803_ROMSTART44(name,n1,chk1,n2,chk2)\
  ROM_START(name) \
    NORMALREGION(0x10000, BY6803_CPUREGION) \
      ROM_LOAD( n1, 0x8000, 0x4000, chk1) \
      ROM_LOAD( n2, 0xc000, 0x4000, chk2 )
#define BY6803_ROMEND ROM_END

/* Display types */
#define BY6803_DISP7SEG  0
#define BY6803_DISPALPHA 1

extern MACHINE_DRIVER_EXTERN(by6803_61S);
extern MACHINE_DRIVER_EXTERN(by6803_45S);
extern MACHINE_DRIVER_EXTERN(by6803_TCSS);
extern MACHINE_DRIVER_EXTERN(by6803_TCS2S);
extern MACHINE_DRIVER_EXTERN(by6803_SDS);
extern MACHINE_DRIVER_EXTERN(by6803_S11CS);

#define by_mBY6803_61S   by6803_61S
#define by_mBY6803_45S   by6803_45S
#define by_mBY6803_TCSS  by6803_TCSS
#define by_mBY6803_TCS2S by6803_TCS2S
#define by_mBY6803_SDS   by6803_SDS
#define by_mBY6803_S11CS by6803_S11CS

#endif /* INC_BY6803 */
