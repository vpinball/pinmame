#ifndef INC_S6
#define INC_S6

/*-- Common Inports for S6 Games --*/
#define S6_COMPORTS \
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
    COREPORT_BITTOG(  0x0200, "Auto/Manual",          KEYCODE_7) \
    COREPORT_BIT(     0x0400, "CPU Diagnostic",   KEYCODE_9) \
    COREPORT_BIT(     0x0800, "Sound Diagnostic", KEYCODE_0) \
    COREPORT_BIT(     0x1000, "Master Command Enter", KEYCODE_PGUP) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Sound Dip 1") \
      COREPORT_DIPSET(0x0001, "1" ) \
      COREPORT_DIPSET(0x0000, "0" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "Sound Dip 2") \
      COREPORT_DIPSET(0x0002, "1" ) \
      COREPORT_DIPSET(0x0000, "0" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "F1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "F2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "F3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "F4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "F5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "F6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "F7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "F8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "D1 Clear Audits") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "D2 Reset Defaults") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "D3 Auto-Cycle Mode") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "D4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "D5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "D6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "D7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "D8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" )


/*-- Standard input ports --*/
#define S6_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    S6_COMPORTS

#define S6_INPUT_PORTS_END INPUT_PORTS_END

#define S6_COMINPORT       CORE_COREINPORT

/*-- S6 switch numbers --*/
#define S6_SWADVANCE    -7
#define S6_SWUPDN       -6
#define S6_SWCPUDIAG    -5
#define S6_SWSOUNDDIAG  -4
#define S6_ENTER        -3

#define S6_CPUREGION	REGION_CPU1

/*-- Main CPU regions and ROM --*/
#define S6_ROMSTART(name, ver, ic14,chk14, ic20,chk20, ic17,chk17) \
  ROM_START(name##_##ver) \
    NORMALREGION(0x10000, S6_CPUREGION) \
      ROM_LOAD(ic14, 0x6000, 0x0800, chk14) \
        ROM_RELOAD(  0xe000, 0x0800) \
      ROM_LOAD(ic17, 0x7800, 0x0800, chk17) \
        ROM_RELOAD(  0xf800, 0x0800) \
      ROM_LOAD(ic20, 0x7000, 0x0800, chk20) \
        ROM_RELOAD(  0xf000, 0x0800)

#define S6_ROMSTARTMOD(name, ic14,chk14, ic20,chk20, ic17,chk17) \
  ROM_START(name) \
    NORMALREGION(0x10000, S6_CPUREGION) \
      ROM_LOAD(ic14, 0x6000, 0x1000, chk14) \
        ROM_RELOAD(  0xe000, 0x1000) \
      ROM_LOAD(ic17, 0x7800, 0x0800, chk17) \
        ROM_RELOAD(  0xf800, 0x0800) \
      ROM_LOAD(ic20, 0x7000, 0x0800, chk20) \
        ROM_RELOAD(  0xf000, 0x0800)

#define S6_ROMSTARTPROM(name, ic14,chk14, ic20,chk20, ic17,chk17, ic21,chk21, ic22,chk22, ic26,chk26) \
  ROM_START(name) \
    NORMALREGION(0x10000, S6_CPUREGION) \
      ROM_LOAD(ic14, 0x6800, 0x0800, chk14) \
        ROM_RELOAD(  0xe800, 0x0800) \
      ROM_LOAD(ic17, 0x7800, 0x0800, chk17) \
        ROM_RELOAD(  0xf800, 0x0800) \
      ROM_LOAD(ic20, 0x7000, 0x0800, chk20) \
        ROM_RELOAD(  0xf000, 0x0800) \
      ROM_LOAD(ic21, 0x6000, 0x0200, chk21) \
        ROM_RELOAD(  0xe000, 0x0200) \
      ROM_LOAD(ic22, 0x6200, 0x0200, chk22) \
        ROM_RELOAD(  0xe000, 0x0200) \
      ROM_LOAD(ic26, 0x6400, 0x0200, chk26) \
        ROM_RELOAD(  0xe000, 0x0200)

#define S6_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(s6);
extern MACHINE_DRIVER_EXTERN(s6S);
#define s6_mS6      s6
#define s6_mS6S     s6S
/* standard 6 digit display layout */
extern const core_tLCDLayout s6_6digit_disp[],s6_7digit_disp[];

#endif /* INC_S6 */

