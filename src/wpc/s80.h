#ifndef INC_S80
#define INC_S80

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define S80_CPUNO	0

#define S80_ROMEND	ROM_END

/*-- Common Inports for S80Games --*/
#define S80_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 8 */ \
    COREPORT_BIT(     0x0001, "Test",             KEYCODE_8)  \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN3,          KEYCODE_3)  \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0020, IPT_TILT,           KEYCODE_INSERT)  \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Slam Tilt",        KEYCODE_HOME)  \

/*   COREPORT_BITTOG(  0x0040, "Outhole",          KEYCODE_9)  */

/*-- Standard input ports --*/
#define S80_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    S80_COMPORTS

#define S80_INPUT_PORTS_END INPUT_PORTS_END

#define S80_COMINPORT       CORE_COREINPORT

#define S80_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define S80_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S80_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- S80 switches are numbers 0-7/0-7 but row and column reversed--*/
#define S80_SWNO(x) (x/10+1)+(x%10+1)*10

/*-- To access C-side multiplexed solenoid/flasher --*/
#define S80_CSOL(x) ((x)+24)

/*-- S80 switch numbers --*/
#define S80_SWSLAMTILT	  1

/*-- Memory regions --*/
#define S80_MEMREG_CPU		REGION_CPU1
#define S80_MEMREG_SCPU1	REGION_CPU2

/*-- S80/S80A Main CPU regions and ROM, 2 game PROM version --*/
#define S80_2_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
   ROM_START(name) \
     NORMALREGION(0x10000, S80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0200, chk1) \
	   ROM_RELOAD(  0x1400, 0x0200)       \
       ROM_LOAD(n2, 0x1200, 0x0200, chk2) \
	   ROM_RELOAD(  0x1600, 0x0200)       \
       ROM_LOAD(n3, 0x2000, 0x1000, chk3) \
       ROM_LOAD(n4, 0x3000, 0x1000, chk4)

/*-- S80/S80A Main CPU regions and ROM, 2 game PROM version --*/
#define S80_1_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3) \
   ROM_START(name) \
     NORMALREGION(0x10000, S80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x2000, 0x1000, chk2) \
       ROM_LOAD(n3, 0x3000, 0x1000, chk3)

/*-- S80B Main CPU regions and ROM, 8K single game PROM --*/
#define S80B_8K_ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, S80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x2000, 0x2000, chk1)

/*-- S80B Main CPU regions and ROM, 8K & 2K game PROM --*/
#define S80B_2K_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, S80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- S80B Main CPU regions and ROM, 8K & 4K game PROM --*/
/*-- the second half of PROM2 is later copied to the right location */
#define S80B_4K_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
     NORMALREGION(0x10000, S80_MEMREG_CPU) \
       ROM_LOAD(n1, 0x1000, 0x1000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

/*-- TheS80 are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_S80;
extern struct MachineDriver machine_driver_S80SS;
extern struct MachineDriver machine_driver_S80B;

#define gl_mS80		S80
#define gl_mS80SS	S80SS
#define gl_mS80B	S80B

#endif /* INC_S80 */
