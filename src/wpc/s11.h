#ifndef INC_S11
#define INC_S11

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-- Common Inports for S11Games --*/
#define S11_COMPORTS \
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
    COREPORT_BIT(     0x0100, "Advance",          KEYCODE_8) \
    COREPORT_BITTOG(  0x0200, "Up/Down",      KEYCODE_7) \
    COREPORT_BIT(     0x0400, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0800, "Sound Diagnostic", KEYCODE_0) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Country") \
      COREPORT_DIPSET(0x0001, "Germany" ) \
      COREPORT_DIPSET(0x0000, "USA" )

/*-- Standard input ports --*/
#define S11_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    S11_COMPORTS

#define S11_INPUT_PORTS_END INPUT_PORTS_END

#define S11_COMINPORT       CORE_COREINPORT

#define S11_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define S11_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define S11_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

/*-- S11 switches are numbered from 1-64 (not column,row as WPC) --*/
#define S11_SWNO(x) ((((x)+7)/8)*10+(((x)-1)%8)+1)

/*-- To access C-side multiplexed solenoid/flasher --*/
#define S11_CSOL(x) ((x)+(WPC_FIRSTFLIPPERSOL-1))

/*-- GameOn solenoids --*/
#define S11_GAMEONSOL 23
/*-- S11 switch numbers --*/
#define S11_SWADVANCE     -7
#define S11_SWUPDN        -6
#define S11_SWCPUDIAG     -5
#define S11_SWSOUNDDIAG   -4

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define S11_CPUNO   0
#define S11_SCPU1NO 1
#define S11_SCPU2NO 2

/*-- Memory regions --*/
#define S11_MEMREG_CPU		REGION_CPU1
#define S11_MEMREG_SCPU1	REGION_CPU2
#define S11_MEMREG_SCPU2	REGION_CPU3
#define S11_MEMREG_SROM1	REGION_SOUND1
#define S11_MEMREG_SROM2	REGION_SOUND2

/*-- standard display layouts --*/
extern core_tLCDLayout s11_dispS9[], s11_dispS11[], s11_dispS11a[], s11_dispS11b_2[];
#define s11_dispS11b_1 s11_dispS11a
#define s11_dispS11b_3 s11_dispS11b_2
#define s11_dispS11c   s11_dispS11b_2

/*-- Main CPU regions and ROM --*/
#define S9_ROMSTARTx4(name, ver, n1, chk1) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S11_MEMREG_CPU) \
      ROM_LOAD(n1, 0x8000, 0x4000, chk1) \
      ROM_RELOAD(  0xc000, 0x4000)

#define S9_ROMSTART12(name, ver, n1, chk1,n2,chk2) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S11_MEMREG_CPU) \
      ROM_LOAD(n1, 0x5000, 0x1000, chk1) \
      ROM_RELOAD(  0xd000, 0x1000) \
      ROM_LOAD(n2, 0x6000, 0x2000, chk2) \
      ROM_RELOAD(  0xe000, 0x2000)

#define S11_ROMSTART48(name, ver, n1, chk1, n2, chk2) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S11_MEMREG_CPU) \
      ROM_LOAD(n1, 0x4000, 0x4000, chk1) \
      ROM_LOAD(n2, 0x8000, 0x8000, chk2)

#define S11_ROMSTART28(name, ver, n1, chk1, n2, chk2) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S11_MEMREG_CPU) \
      ROM_LOAD(n1, 0x4000, 0x2000, chk1) \
        ROM_RELOAD(  0x6000, 0x2000) \
      ROM_LOAD(n2, 0x8000, 0x8000, chk2)

#define S11_ROMSTART24(name, ver, n1, chk1, n2, chk2) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S11_MEMREG_CPU) \
      ROM_LOAD(n1, 0x4000, 0x2000, chk1) \
        ROM_RELOAD(  0x6000, 0x2000) \
      ROM_LOAD(n2, 0x8000, 0x4000, chk2) \
        ROM_RELOAD(  0xc000, 0x4000)

/*-- Sound on CPU board --*/
#define S11S_STDREG \
  SOUNDREGION(0x10000, S11_MEMREG_SCPU2) \
  SOUNDREGION(0x10000, S11_MEMREG_SROM2)

#define S11S_SOUNDROM44(n1, chk1, n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n1, 0x4000, 0x4000, chk1) \
      ROM_RELOAD(  0x8000, 0x4000) \
    ROM_LOAD(n2, 0xc000, 0x4000, chk2) \
      ROM_RELOAD(  0x8000, 0x4000)

#define S11S_SOUNDROMx8(n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n2, 0x4000, 0x4000, chk2) \
      ROM_CONTINUE(0xc000, 0x4000)

#define S11S_SOUNDROM88(n1, chk1, n2, chk2) \
  S11S_STDREG \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
    ROM_LOAD(n2, 0x8000, 0x8000, chk2)

/*-- S9 Sound on CPU board --*/
#define S9S_STDREG SOUNDREGION(0x10000, S11_MEMREG_SCPU1)

#define S9S_SOUNDROM41111(u49,chk49, u4,chk4, u5,chk5, u6,chk6, u7,chk7) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49)  \
     ROM_LOAD(u7,  0x8000, 0x1000, chk7)  \
     ROM_LOAD(u5,  0x9000, 0x1000, chk5)  \
     ROM_LOAD(u6,  0xa000, 0x1000, chk6)  \
     ROM_LOAD(u4,  0xb000, 0x1000, chk4)

#define S9S_SOUNDROM4111(u49,chk49, u4,chk4, u5,chk5, u6,chk6) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49)  \
	 ROM_LOAD(u5,  0x9000, 0x1000, chk5) \
     ROM_LOAD(u6,  0xa000, 0x1000, chk6) \
     ROM_LOAD(u4,  0xb000, 0x1000, chk4)

#define S9S_SOUNDROM4(u49,chk49) \
   S9S_STDREG \
     ROM_LOAD(u49, 0xc000, 0x4000, chk49) \
     ROM_RELOAD(0x8000, 0x4000) \
     ROM_RELOAD(0xc000, 0x4000) \
     ROM_RELOAD(0x0000, 0x4000)

/*-- Jokerz sound on CPU board --*/
#define S11B3S_STDREG \
  SOUNDREGION(0x10000, S11_MEMREG_SCPU1) \
  SOUNDREGION(0x10000, S11_MEMREG_SROM1)

#define S11B3S_SOUNDROM881(n1, chk1, n2, chk2, n3, chk3) \
  S11B3S_STDREG \
    ROM_LOAD(n1, 0x0000, 0x8000, chk1) \
    ROM_LOAD(n2, 0x8000, 0x8000, chk2)

#define S11_ROMEND ROM_END
#define S9_ROMEND ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern struct MachineDriver machine_driver_s9;
extern struct MachineDriver machine_driver_s11a_2;
#define s9_mS9           s9
#define s11_mS11         s11a_2
#define s11_mS11A        s11a_2
#define s11_mS11B_1      s11a_2
#define s11_mS11B_2      s11a_2
#define s11_mS11B_3      s11a_2
#define s11_mS11C        s11a_2

extern struct MachineDriver machine_driver_s9_s;
extern struct MachineDriver machine_driver_s11a_2_s;
extern struct MachineDriver machine_driver_s11b_3_s;
extern struct MachineDriver machine_driver_s11c_s;
#define s9_mS9S          s9_s
#define s11_mS11S        s11a_2_s
#define s11_mS11AS       s11a_2_s
#define s11_mS11B_1S     s11a_2_s
#define s11_mS11B_2S     s11a_2_s
#define s11_mS11B_3S     s11b_3_s
#define s11_mS11CS       s11c_s

#endif /* INC_S11 */

