#ifndef INC_S9
#define INC_S9

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-- Common Inports for S9Games --*/
#define S9_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_TILT,           KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_2)  \
    COREPORT_BITDEF(  0x0004, IPT_START1,         IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0010, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BIT(     0x0040, "Slam Tilt",        KEYCODE_HOME)  \
    COREPORT_BIT(     0x0080, "Hiscore Reset",    KEYCODE_4) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x0100, "Advance",          KEYCODE_7) \
    COREPORT_BITTOG(  0x0200, "Up/Down",          KEYCODE_8) \
    COREPORT_BIT(     0x0400, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0800, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* must fill up 2 ports */


/*-- Standard input ports --*/
#define S9_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    S9_COMPORTS

#define S9_INPUT_PORTS_END INPUT_PORTS_END

#define S9_COMINPORT       CORE_COREINPORT

#define S9_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define S9_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S9_DISPLAYSMOOTH   4 /* Smooth the display over this number of VBLANKS */

/*-- S9 switches are numbered from 1-64 (not column,row as WPC) --*/
#define S9_SWNO(x) ((((x)+7)/8)*10+(((x)-1)%8)+1)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define S9_CSOL(x) ((x)+24)

/*-- S9 switch numbers --*/
#define S9_SWADVANCE     1
#define S9_SWUPDN        2
#define S9_SWCPUDIAG     3
#define S9_SWSOUNDDIAG   4

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define S9_CPUNO   0
#define S9_SCPU1NO 1
#define S9_SCPU2NO 2

/*-- Memory regions --*/
#define S9_MEMREG_CPU		REGION_CPU1
#define S9_MEMREG_S1CPU		REGION_CPU2
#define S9_MEMREG_S2CPU		REGION_CPU3
#define S9_MEMREG_SROM		REGION_SOUND1

/**************************/
/* GAMES WITH 1 ROM (16K) */
/**************************/

/*-- Main CPU regions and ROM --*/
#define S91_ROMSTART(name, ver, n1, chk1) \
   ROM_START(name##_##ver) \
     NORMAL_REGION(0x10000, S9_MEMREG_CPU) \
       ROM_LOAD(n1, 0xc000, 0x4000, chk1)
#define S91_ROMEND ROM_END

/*******************************/
/* GAMES WITH 2 ROMS (4K & 8K) */
/*******************************/

/*-- Main CPU regions and ROM --*/
#define S92_ROMSTART(name, ver, n1, chk1, n2, chk2) \
   ROM_START(name##_##ver) \
     NORMAL_REGION(0x10000, S9_MEMREG_CPU) \
       ROM_LOAD(n1, 0xd000, 0x1000, chk1) \
       ROM_LOAD(n2, 0xe000, 0x2000, chk2)
#define S92_ROMEND ROM_END


/*Normal 5 Sound Roms*/
#define S9_SOUNDROMS(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5)\
    SOUNDREGION(0x10000, S9_MEMREG_S1CPU) \
	ROM_LOAD( n1, 0xc000, 0x4000, chk1 ) \
	ROM_LOAD( n2, 0x8000, 0x1000, chk2 ) \
	ROM_LOAD( n3, 0x9000, 0x1000, chk3 ) \
	ROM_LOAD( n4, 0xa000, 0x1000, chk4 ) \
	ROM_LOAD( n5, 0xb000, 0x1000, chk5 )

/*Only 4 Sound Roms*/
#define S92_SOUNDROMS(n1, chk1, n2, chk2, n3, chk3, n4, chk4)\
    SOUNDREGION(0x10000, S9_MEMREG_S1CPU) \
	ROM_LOAD( n1, 0xc000, 0x4000, chk1 ) \
	ROM_LOAD( n2, 0x8000, 0x1000, chk2 ) \
	ROM_LOAD( n3, 0x9000, 0x1000, chk3 ) \
	ROM_LOAD( n4, 0xa000, 0x1000, chk4 )

/* Only 1 Sound Rom */
#define S91_SOUNDROMS(n1, chk1)\
    SOUNDREGION(0x10000, S9_MEMREG_S1CPU) \
	ROM_LOAD( n1, 0xc000, 0x4000, chk1 )

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_s9a_2;
#define s9_mS9         s9a_2
#define s9_mS9A_1      s9a_2
#define s9_mS9A_2      s9a_2
#define s9_mS9B_1      s9a_2
#define s9_mS9B_2      s9a_2
#define s9_mS9B_3      s9a_2
#define s9_mS9C        s9a_2

extern struct MachineDriver machine_driver_s9a_2_s;
extern struct MachineDriver machine_driver_s9c_s;
#define s9_mS9S        s9a_2_s
#define s9_mS9A_1S     s9a_2_s
#define s9_mS9A_2S     s9a_2_s
#define s9_mS9B_1S     s9a_2_s
#define s9_mS9B_2S     s9a_2_s
#define s9_mS9B_3S     s9a_2_s
#define s9_mS9CS       s9c_s

#endif /* INC_S9 */

