#ifndef INC_DE2
#define INC_DE2

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define DE_CPUREV3b	0x0002000

//Games after Frankenstein used Non-Toggling Up/Down Switch
#define DE2_COMPORTS \
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
    COREPORT_BIT(     0x0200, "Green Button",     KEYCODE_7) \

/*-- Standard input ports --*/
#define DE2_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    DE2_COMPORTS

/******************************************/
/** DMD (128x16) ROM 64K                 **/
/******************************************/
#define DE_DMD64_ROMSTART(n1,chk1) \
     NORMALREGION(0x10000, DE_MEMREG_DCPU1)\
	 NORMALREGION(0x20000, DE_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
	   ROM_RELOAD(	0x10000, 0x10000)

/**************************************/
/** DMD (128x16) ROM 128K            **/
/**************************************/
#define DE_DMD128_ROMSTART(n1,chk1) \
	 NORMALREGION(0x10000, DE_MEMREG_DCPU1)\
     NORMALREGION(0x20000, DE_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x20000, chk1)

/**************************************/
/** DMD (128x32) ROM 2x256K          **/
/**************************************/
#define DE_DMD256_ROMSTART(n1,chk1,n2,chk2) \
	 NORMALREGION(0x10000, DE_MEMREG_DCPU1)\
     NORMALREGION(0x80000, DE_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x40000, chk1) \
	   ROM_LOAD(n2, 0x40000, 0x40000, chk2) 

/**************************************/
/** DMD (128x32) ROM 512K            **/
/**************************************/
#define DE_DMD512_ROMSTART(n1,chk1) \
	 NORMALREGION(0x10000, DE_MEMREG_DCPU1)\
     NORMALREGION(0x80000, DE_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x80000, chk1)

/**************************************/
/** DMD (194x64) ROM 2x512K          **/
/**************************************/
#if defined(MAMEVER) && (MAMEVER > 3709)
#  define DE_DMD1024_ROMSTART(n1,chk1, n2, chk2) \
    NORMALREGION(0x01000000, DE_MEMREG_DCPU1) \
    ROM_LOAD16_BYTE(n1, 0x00000001, 0x00080000, chk1) \
    ROM_LOAD16_BYTE(n2, 0x00000000, 0x00080000, chk2)
#else /* MAMEVER */
#  define DE_DMD1024_ROMSTART(n1,chk1, n2, chk2) \
    NORMALREGION(0x01000000, DE_MEMREG_DCPU1) \
      ROM_LOAD_ODD( n1, 0x0000000, 0x0080000, chk1) \
      ROM_LOAD_EVEN(n2, 0x0000000, 0x0080000, chk2)
#endif /* MAMEVER */
     //NORMALREGION(0x100000, DE_MEMREG_DROM1)

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_DE_DMD1;
extern struct MachineDriver machine_driver_DE_DMD1S1;
extern struct MachineDriver machine_driver_DE_DMD1S2;
extern struct MachineDriver machine_driver_DE_DMD2S1;
extern struct MachineDriver machine_driver_DE_DMD3S1;

#define de_mDE_DMD1S1	DE_DMD1S1
#define de_mDE_DMD1S2	DE_DMD1S2
#define de_mDE_DMD2S1	DE_DMD2S1
#define de_mDE_DMD3S1	DE_DMD3S1

#endif /* INC_DE2 */

