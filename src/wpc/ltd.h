#ifndef INC_LTD
#define INC_LTD

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define LTD_ROMEND	ROM_END

/*-- Common Inports for LTD Games --*/
#define LTD_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 0 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,  IP_KEY_DEFAULT) \
    /* switch column 1 */ \
    COREPORT_BIT(     0x0100,"Ball Tilt", KEYCODE_INSERT) \
    /* switch column 4 */ \
    COREPORT_BITDEF(  0x2000, IPT_START1, IP_KEY_DEFAULT)

/*-- Common Inports for LTD System 4 Games --*/
#define LTD4_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 2 */ \
    COREPORT_BIT   (  0x0001, "A7 - Diagnostics", KEYCODE_7) \
    COREPORT_BIT   (  0x0002, "A8 - Enter",       KEYCODE_8) \
    COREPORT_BITDEF(  0x0010, IPT_COIN1,          IP_KEY_DEFAULT) \

/*-- Standard input ports --*/
#define LTD_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    LTD_COMPORTS

#define LTD4_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    LTD4_COMPORTS

#define LTD_INPUT_PORTS_END INPUT_PORTS_END

#define LTD_COMINPORT       CORE_COREINPORT

#define LTD_SOLSMOOTH       6 /* Smooth the Solenoids over this numer of VBLANKS */
#define LTD4_SOLSMOOTH      2 /* Smooth the Solenoids over this numer of VBLANKS */
#define LTD_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define LTD_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- Memory regions --*/
#define LTD_MEMREG_CPU	REGION_CPU1

/* CPUs */
#define LTD_CPU	0

/*-- LTD CPU regions and ROM, 1x2K game PROM version --*/
#define LTD_2_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, LTD_MEMREG_CPU) \
       ROM_LOAD(n1, 0xc000, 0x0800, chk1) \
         ROM_RELOAD(0xc800, 0x0800) \
         ROM_RELOAD(0xd000, 0x0800) \
         ROM_RELOAD(0xd800, 0x0800) \
         ROM_RELOAD(0xe000, 0x0800) \
         ROM_RELOAD(0xe800, 0x0800) \
         ROM_RELOAD(0xf000, 0x0800) \
         ROM_RELOAD(0xf800, 0x0800) \

/*-- LTD CPU regions and ROM, 1x4K game PROM version --*/
#define LTD_4_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, LTD_MEMREG_CPU) \
       ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
         ROM_RELOAD(0xd000, 0x1000) \
         ROM_RELOAD(0xe000, 0x1000) \
         ROM_RELOAD(0xf000, 0x1000)

/*-- LTD CPU regions and ROM, 2x4K game PROM version --*/
#define LTD_44_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, LTD_MEMREG_CPU) \
       ROM_LOAD(n1, 0xc000, 0x1000, chk1) \
         ROM_RELOAD(0xe000, 0x1000) \
       ROM_LOAD(n2, 0xd000, 0x1000, chk2) \
         ROM_RELOAD(0xf000, 0x1000)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(LTD);
extern MACHINE_DRIVER_EXTERN(LTD4);

#define gl_mLTD		LTD
#define gl_mLTD4	LTD4

#endif /* INC_LTD */
