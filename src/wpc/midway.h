#ifndef INC_MIDWAY
#define INC_MIDWAY

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define MIDWAY_ROMEND	ROM_END

/*-- Common Inports for MIDWAY Games --*/
#define MIDWAY_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 3 */ \
    COREPORT_BITDEF(  0x0400, IPT_COIN2, KEYCODE_3) \
    /* switch column 4 */ \
    COREPORT_BITDEF(  0x4000, IPT_COIN1, IP_KEY_DEFAULT) \
    /* switch column 0 */ \
    COREPORT_BIT(     0x0001, "Key 1", KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Key 2", KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Key 4", KEYCODE_4) \
    COREPORT_BIT(     0x0080, "Key 6", KEYCODE_6) \
    COREPORT_BIT(     0x0008, "Enter Diagnostic", KEYCODE_7) \
    COREPORT_BIT(     0x0010, "Switch Test", KEYCODE_8) \
    COREPORT_BIT(     0x0020, "Lamp/Display Test", KEYCODE_9) \
    COREPORT_BIT(     0x0040, "Reset", KEYCODE_0) \

/*-- Standard input ports --*/
#define MIDWAY_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    MIDWAY_COMPORTS

#define MIDWAY_INPUT_PORTS_END INPUT_PORTS_END

#define MIDWAY_COMINPORT       CORE_COREINPORT

#define MIDWAY_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
#define MIDWAY_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define MIDWAY_DISPLAYSMOOTH   3 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define MIDWAY_CSOL(x) ((x)+24)

/*-- MIDWAY switch numbers --*/

/*-- Memory regions --*/
#define MIDWAY_MEMREG_CPU	REGION_CPU1

/* CPUs */
#define MIDWAY_CPU	0

/*-- MIDWAY CPU regions and ROM --*/
#define MIDWAY_1_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, MIDWAY_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x0400, chk1)

#define MIDWAY_3_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3) \
   ROM_START(name) \
     NORMALREGION(0x10000, MIDWAY_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x0800, 0x0800, chk2) \
       ROM_LOAD(n3, 0x1000, 0x0800, chk3)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(MIDWAY);
extern MACHINE_DRIVER_EXTERN(MIDWAYProto);

#define gl_mMIDWAY		MIDWAY
#define gl_mMIDWAYP		MIDWAYProto

#endif /* INC_MIDWAY */
