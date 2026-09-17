#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "recel.h"

/* Lite box layout. System III Operation and Maintenance manual §7.4.

   coreGlobals.segments keeps the 10788's scan-time index: group A at 0-15,
   group B at 16-31. The display runs the other way -- the write pointer
   starts at time 15 and walks down -- so the higher scan time is a field's
   leftmost digit, and each field is spelled out descending because PinMAME
   layouts only run left to right.

     row 1   player 1  A7..A3   free play A0, extra ball A1 (two 1-digit units)
     row 2   player 2  AF..AB   match number A9
     row 3   player 3  BF..BB   --
     row 4   player 4  B7..B3   credit: tens B0, units B1 (one 2-digit unit)

   Columns 2 and A (player status LEDs) and A8 (ball/tilt/game over) are
   latched lamps rather than digits and are not laid out here; see
   gpkd_kind() in recel.c. B8/B9 drive no indicator on a real machine. */
#define RECEL_D(row, col, pos) {row, col, pos, 1, CORE_SEG7},
/* One counter: the five GPKD-multiplexed digits, MSD first, then the x1
   digit. base = the x10 digit's GPKD position. The x1 is not multiplexed --
   the 095-105 unit's sixth 7448 position is wired to a permanent 0 -- so it
   comes from RECEL_SEG_UNITS */
#define RECEL_COUNTER(row, col, base) \
  RECEL_D(row, col,    (base)+4) RECEL_D(row, (col)+2,  (base)+3) \
  RECEL_D(row, (col)+4,(base)+2) RECEL_D(row, (col)+6,  (base)+1) \
  RECEL_D(row, (col)+8,(base))   RECEL_D(row, (col)+10, RECEL_SEG_UNITS)

static core_tLCDLayout recel_disp[] = {
  RECEL_COUNTER(0, 0,  3) RECEL_D(0, 14, 0) RECEL_D(0, 18, 1)
  RECEL_COUNTER(2, 0, 11) RECEL_D(2, 14, 9)
  RECEL_COUNTER(4, 0, 27)
  RECEL_COUNTER(6, 0, 19) RECEL_D(6, 14, 16) RECEL_D(6, 16, 17)
  {0}
};

#define INIT_RECEL(name, dsp, hwver) \
RECEL_INPUT_PORTS_START(name, 1) RECEL_INPUT_PORTS_END \
static core_tGameData name##GameData = { \
  GEN_RECEL, dsp, {FLIP_SW(FLIP_L),0,RECEL_LAMPCOLS,0,SNDBRD_NONE,0,hwver}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*-------------------------------------------------------------------
/ Recel System III shared program (not a game)
/-------------------------------------------------------------------*/
INIT_RECEL(recel, recel_disp, 1)
RECEL_BIOS_ROMSTART(recel)
RECEL_ROMEND
GAMEX(1978,recel,0,RECEL,recel,recel,ROT0,"Recel","System III",NOT_A_DRIVER)

/*-------------------------------------------------------------------
/ Fair Fight (1978) - model 1.053, doc 035-623
/-------------------------------------------------------------------*/
INIT_RECEL(r_fairfght, recel_disp, 1)
RECEL_ROMSTART(r_fairfght, "fa.c5", 0x0100, CRC(5d3694da) SHA1(4d0a8033acb6ef2e2af107f76540fd19b4a39b12))
RECEL_ROMEND
CORE_CLONEDEFNV(r_fairfght,recel,"Fair Fight",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Mr. Evil (1978) - model 1.055, doc 035-626
/-------------------------------------------------------------------*/
INIT_RECEL(r_mrevil, recel_disp, 1)
RECEL_ROMSTART(r_mrevil, "me.c5", 0x0100, CRC(53ce24a0) SHA1(42d376e3e7a4e94a09db2f974af8d4869579d0f5))
RECEL_ROMEND
CORE_CLONEDEFNV(r_mrevil,recel,"Mr. Evil",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Mr. Doom (1979) - model 1.060, doc 035-635. Mr. Evil re-themed: same
/ switch matrix and driver table, different ROM and rules
/-------------------------------------------------------------------*/
INIT_RECEL(r_mrdoom, recel_disp, 1)
RECEL_ROMSTART(r_mrdoom, "md.c5", 0x0100, CRC(ca679a69) SHA1(f08f0cfe646f08882473dcd5d23889fffe4a03c8))
RECEL_ROMEND
CORE_CLONEDEFNV(r_mrdoom,recel,"Mr. Doom",1979,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Screech (1978) - INDER model 2004, unit 52-000. INDER's board set is
/ the Petaco one renumbered
/-------------------------------------------------------------------*/
INIT_RECEL(r_screech, recel_disp, 1)
RECEL_ROMSTART(r_screech, "sc_1_1702.bin", 0x0100, CRC(c9185ef3) SHA1(3ace6cccc96375c5eab3d43f86f52bf52124334e))
RECEL_ROMEND
CORE_CLONEDEFNV(r_screech,recel,"Screech",1978,"Inder",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Poker Plus (1978) - model 1.051-E. The widely distributed po.c5 dump is
/ bad -- it never renders a score -- so this is the collector dump recorded
/ as good by the garzol/RECEL collection
/-------------------------------------------------------------------*/
INIT_RECEL(r_pokrplus, recel_disp, 1)
RECEL_ROMSTART(r_pokrplus, "ba65.c5", 0x0100, CRC(571ee27b) SHA1(482a3ba18eff05bce4cab073b1f13fc2f145bb2b))
RECEL_ROMEND
CORE_CLONEDEFNV(r_pokrplus,recel,"Poker Plus",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Alaska (1978) - Recreativos Franco, sold abroad as Interflip
/-------------------------------------------------------------------*/
INIT_RECEL(r_alaska, recel_disp, 1)
RECEL_ROMSTART(r_alaska, "al.c5", 0x0100, CRC(905ef624) SHA1(ab0bb2e7262650b670524ce9f88bd1f14ffd749a))
RECEL_ROMEND
CORE_CLONEDEFNV(r_alaska,recel,"Alaska",1978,"Interflip",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Hot & Cold (1978) - Inder
/-------------------------------------------------------------------*/
INIT_RECEL(r_hotcold, recel_disp, 1)
RECEL_ROMSTART(r_hotcold, "hc.c5", 0x0100, CRC(f58d0c05) SHA1(54ecf9f67ce3a5264bfd9c063353705f9202d524))
RECEL_ROMEND
CORE_CLONEDEFNV(r_hotcold,recel,"Hot & Cold",1978,"Inder",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ SwashBuckler (1979) - model 1.061-E, doc 035-639
/-------------------------------------------------------------------*/
INIT_RECEL(r_swash, recel_disp, 1)
RECEL_ROMSTART(r_swash, "sw.c5", 0x0100, CRC(69326f5f) SHA1(f0bb4251f579ccf97c1cabb63254ba466ccd141e))
RECEL_ROMEND
CORE_CLONEDEFNV(r_swash,recel,"SwashBuckler",1979,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Cavalier (1979) - model 1.062
/-------------------------------------------------------------------*/
INIT_RECEL(r_cavalier, recel_disp, 1)
RECEL_ROMSTART(r_cavalier, "ca.c5", 0x0100, CRC(dc2e865f) SHA1(3f15f90dafa9d5e42381605044b6c9b529afd3af))
RECEL_ROMEND
CORE_CLONEDEFNV(r_cavalier,recel,"Cavalier",1979,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Don Quijote (1979) - model 1.063
/-------------------------------------------------------------------*/
INIT_RECEL(r_quijote, recel_disp, 1)
RECEL_ROMSTART(r_quijote, "qu.c5", 0x0100, CRC(1fd535d0) SHA1(a9c9a72881d195a0de751f10fa54fb181523a33f))
RECEL_ROMEND
CORE_CLONEDEFNV(r_quijote,recel,"Don Quijote",1979,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Crazy Race (1978) - model 1.054-E. Hardware version 2: the 2 KB EPROM
/ carries a 1 KB game area plus a patched replacement for the A1762 ROM
/-------------------------------------------------------------------*/
INIT_RECEL(r_crzyrace, recel_disp, 2)
RECEL_ROMSTART(r_crzyrace, "cr.c5", 0x0800, CRC(60088804) SHA1(a73a7f8a0583a79588f9823a5e65ed28edad96a3))
RECEL_ROMEND
CORE_CLONEDEFNV(r_crzyrace,recel,"Crazy Race",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ The Flipper Game (1980)
/-------------------------------------------------------------------*/
INIT_RECEL(r_flipper, recel_disp, 2)
RECEL_ROMSTART(r_flipper, "fl.c5", 0x0800, CRC(76ee0370) SHA1(f2a835a0b76f7258d5e65390c239f5456e30e87a))
RECEL_ROMEND
CORE_CLONEDEFNV(r_flipper,recel,"The Flipper Game",1980,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Black Magic (1980) - model 1.065, doc December 1979. One player
/-------------------------------------------------------------------*/
INIT_RECEL(r_blackmag, recel_disp, 2)
RECEL_ROMSTART(r_blackmag, "bm_1065_1.bin", 0x0800, CRC(a917718c) SHA1(0b4fdf270560df902e95b34c25cca20e91f1071c))
RECEL_ROMEND
CORE_CLONEDEFNV(r_blackmag,recel,"Black Magic",1980,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Black Magic 4 (1980) - model 1.066. Four players
/-------------------------------------------------------------------*/
INIT_RECEL(r_blackm4, recel_disp, 2)
RECEL_ROMSTART(r_blackm4, "b4.c5", 0x0800, CRC(cd383f5b) SHA1(c38acaae46e5fd2660efbd0e2d35e295892e60a5))
RECEL_ROMEND
CORE_CLONEDEFNV(r_blackm4,recel,"Black Magic 4",1980,"Recel",gl_mRECEL,0)
