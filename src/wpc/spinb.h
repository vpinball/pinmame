#pragma once

#include "core.h"
#include "sim.h"

#define SPINB_CPU_GAME 0
#define SPINB_CPU_DMD  1
#define SPINB_CPU_SND1 2
#define SPINB_CPU_SND2 3

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SPINB_ROMEND	ROM_END

#define GEN_SPINB 0

// DMD frames per picture
#define SPINB_DSP1 1
#define SPINB_DSP2 2
#define SPINB_DSP3 3

#define SPINB_GS1(solInvert0,solInvert1,solInvert2,lamp16isGameOn) \
(((UINT32)(UINT8)(solInvert0)<<24) | ((UINT32)(UINT8)(solInvert1)<<16) | ((UINT32)(UINT8)(solInvert2)<<8) | ((UINT32)(UINT8)(lamp16isGameOn)))

/*-- Common Inports for Spinball Games --*/
#define SPINB_COMPORTS SPINB_BUTTONPORTS SPINB_DIPS_GENERIC

#define SPINB_BUTTONPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,          KEYCODE_3)\
/*  COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_4)\	commented out as it interferes with Bushido's trough switches,
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_5)\	and it doesn't seem to be connected on Jolly Park either!?
*/  COREPORT_BIT(     0x0008, "Ball Tilt",        KEYCODE_INSERT)\
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)\
    COREPORT_BIT(     0x0020, "Puesta A Cero",    KEYCODE_HOME)\
    COREPORT_BIT(     0x0040, "Test Economico",   KEYCODE_9)\
    COREPORT_BIT(     0x0080, "Test Tecnico",     KEYCODE_8)\
    COREPORT_BIT(     0x0100, "Test Contacto",    KEYCODE_7)

/* DIP switches: port 1 = SL1 (low byte) + SL2 (high byte), port 2 = SL3 (low byte).
   On every bank switches 1-4 are wired in reverse to bits 3-0, switches 5-8 go to bits 4-7
   (confirmed from the game code of Verne's World, Jolly Park and Mach 2).
   Generic set, used by Bushido */
#define SPINB_DIPS_GENERIC \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coins Per Game") \
      COREPORT_DIPSET(0x0000, "25 ptas 2 Games" ) \
      COREPORT_DIPSET(0x0008, "25 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0004, "25 ptas 1 Game, 100 ptas 5 Games" ) \
      COREPORT_DIPSET(0x000c, "50 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0002, "50 ptas 1 Game, 200 ptas 5 Games" ) \
      COREPORT_DIPSET(0x000a, "50 ptas 1 Game, 100 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0006, "100 ptas 1 Game" ) \
      COREPORT_DIPSET(0x000e, "100 ptas 1 Game, 500 ptas 6 Games" ) \
      COREPORT_DIPSET(0x0001, "100 ptas 1 Game, 200 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0009, "50 esc 1 Game" ) \
      COREPORT_DIPSET(0x0005, "50 esc 1 Game, 100 esc 3 Games" ) \
      COREPORT_DIPSET(0x000d, "100 esc 1 Game" ) \
      COREPORT_DIPSET(0x0003, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000b, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x0007, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000f, "100 esc 2 Games, 200 esc 5 Games" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "First Replay Score") \
      COREPORT_DIPSET(0x0000, "300 Million" ) \
      COREPORT_DIPSET(0x0010, "400 Million" ) \
      COREPORT_DIPSET(0x0020, "500 Million" ) \
      COREPORT_DIPSET(0x0030, "600 Million" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Reserved") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Game Type") \
      COREPORT_DIPSET(0x0000, "Normal" ) \
      COREPORT_DIPSET(0x0080, "Tournament" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "Balls")\
      COREPORT_DIPSET(0x0000, "3" )\
      COREPORT_DIPSET(0x0100, "5" )\
    COREPORT_DIPNAME( 0x0200, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0200, "1" )\
    COREPORT_DIPNAME( 0x0c00, 0x0000, "Value of Handicap")\
      COREPORT_DIPSET(0x0000, "700 Million" )\
      COREPORT_DIPSET(0x0800, "750 Million" )\
      COREPORT_DIPSET(0x0400, "800 Million" )\
      COREPORT_DIPSET(0x0c00, "850 Million" )\
    COREPORT_DIPNAME( 0x3000, 0x0000, "Time of New Ticket")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x1000, "Short" )\
      COREPORT_DIPSET(0x2000, "Medium" )\
      COREPORT_DIPSET(0x3000, "Long" )\
    COREPORT_DIPNAME( 0x4000, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x4000, "1" )\
    COREPORT_DIPNAME( 0x8000, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x8000, "1" )\
  PORT_START /* 2 */\
    COREPORT_DIPNAME( 0x0001, 0x0000, "AutoPlunger")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x0001, "Enabled" )\
    COREPORT_DIPNAME( 0x0002, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0002, "1" )\
    COREPORT_DIPNAME( 0x000c, 0x0000, "Number of loops for Extra Ball")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0008, "1" )\
      COREPORT_DIPSET(0x0004, "2" )\
      COREPORT_DIPSET(0x000C, "3" )\
    COREPORT_DIPNAME( 0x0030, 0x0000, "Multiball Ballsaver or Restart")\
      COREPORT_DIPSET(0x0000, "Disabled" )\
      COREPORT_DIPSET(0x0010, "Short" )\
      COREPORT_DIPSET(0x0020, "Medium" )\
      COREPORT_DIPSET(0x0030, "Long" )\
    COREPORT_DIPNAME( 0x0040, 0x0000, "Special Difficulty")\
      COREPORT_DIPSET(0x0000, "Easy" )\
      COREPORT_DIPSET(0x0040, "Hard" )\
    COREPORT_DIPNAME( 0x0080, 0x0000, "Reserved")\
      COREPORT_DIPSET(0x0000, "0" )\
      COREPORT_DIPSET(0x0080, "1" )

/* Verne's World, manual pages 13-14 */
#define SPINB_DIPS_VRNWRLD() \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coins Per Game") \
      COREPORT_DIPSET(0x0000, "25 ptas 2 Games" ) \
      COREPORT_DIPSET(0x0001, "25 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0002, "25 ptas 1 Game, 100 ptas 5 Games" ) \
      COREPORT_DIPSET(0x0003, "50 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0004, "50 ptas 1 Game, 200 ptas 5 Games" ) \
      COREPORT_DIPSET(0x0005, "50 ptas 1 Game, 100 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0006, "100 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0007, "100 ptas 1 Game, 500 ptas 6 Games" ) \
      COREPORT_DIPSET(0x0008, "100 ptas 1 Game, 200 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0009, "50 esc 1 Game" ) \
      COREPORT_DIPSET(0x000a, "50 esc 1 Game, 100 esc 3 Games" ) \
      COREPORT_DIPSET(0x000b, "100 esc 1 Game" ) \
      COREPORT_DIPSET(0x000c, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000d, "100 esc 1 Game, 200 esc 3 Games (2)" ) \
      COREPORT_DIPSET(0x000e, "100 esc 1 Game, 200 esc 3 Games (3)" ) \
      COREPORT_DIPSET(0x000f, "100 esc 2 Games, 200 esc 5 Games" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "Replay Score") \
      COREPORT_DIPSET(0x0000, "300 Million" ) \
      COREPORT_DIPSET(0x0010, "400 Million" ) \
      COREPORT_DIPSET(0x0020, "500 Million" ) \
      COREPORT_DIPSET(0x0030, "600 Million" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Test Mode Language") \
      COREPORT_DIPSET(0x0000, "Spanish" ) \
      COREPORT_DIPSET(0x0040, "Portuguese" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Game Mode") \
      COREPORT_DIPSET(0x0000, "Normal" ) \
      COREPORT_DIPSET(0x0080, "Exhibition (Free Play)" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Balls Per Game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0800, "5" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Reserved (SL2-2)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0400, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Handicap Score") \
      COREPORT_DIPSET(0x0000, "700 Million" ) \
      COREPORT_DIPSET(0x0100, "750 Million" ) \
      COREPORT_DIPSET(0x0200, "800 Million" ) \
      COREPORT_DIPSET(0x0300, "850 Million" ) \
    COREPORT_DIPNAME( 0x3000, 0x0000, "New Travel Time") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x1000, "Short" ) \
      COREPORT_DIPSET(0x2000, "Medium" ) \
      COREPORT_DIPSET(0x3000, "Long" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Reserved (SL2-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x4000, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Reserved (SL2-8)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x8000, DEF_STR( On ) ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Automatic Replay Score") \
      COREPORT_DIPSET(0x0000, DEF_STR( No ) ) \
      COREPORT_DIPSET(0x0008, DEF_STR( Yes ) ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Reserved (SL3-2)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0004, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0003, 0x0000, "Extra Ball and Special Difficulty") \
      COREPORT_DIPSET(0x0000, "Very Easy" ) \
      COREPORT_DIPSET(0x0001, "Easy" ) \
      COREPORT_DIPSET(0x0002, "Hard" ) \
      COREPORT_DIPSET(0x0003, "Very Hard" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "3-Ball Multiball Repeat Time") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x0010, "Short" ) \
      COREPORT_DIPSET(0x0020, "Medium" ) \
      COREPORT_DIPSET(0x0030, "Long" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Reserved (SL3-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0040, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Reserved (SL3-8)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0080, DEF_STR( On ) )

/* Jolly Park, manual pages 13-14 */
#define SPINB_DIPS_JOLYPARK() \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0002, "Coins Per Game") \
      COREPORT_DIPSET(0x0000, "25 ptas 2 Games" ) \
      COREPORT_DIPSET(0x0001, "25 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0002, "25 ptas 1 Game, 100 ptas 5 Games" ) \
      COREPORT_DIPSET(0x0003, "50 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0004, "50 ptas 1 Game, 200 ptas 5 Games" ) \
      COREPORT_DIPSET(0x0005, "50 ptas 1 Game, 100 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0006, "100 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0007, "100 ptas 1 Game, 500 ptas 6 Games" ) \
      COREPORT_DIPSET(0x0008, "100 ptas 1 Game, 200 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0009, "50 esc 1 Game" ) \
      COREPORT_DIPSET(0x000a, "50 esc 1 Game, 100 esc 3 Games" ) \
      COREPORT_DIPSET(0x000b, "100 esc 1 Game" ) \
      COREPORT_DIPSET(0x000c, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000d, "100 esc 1 Game, 200 esc 3 Games (2)" ) \
      COREPORT_DIPSET(0x000e, "100 esc 1 Game, 200 esc 3 Games (3)" ) \
      COREPORT_DIPSET(0x000f, "100 esc 2 Games, 200 esc 5 Games" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "Replay Score") \
      COREPORT_DIPSET(0x0000, "300 Million" ) \
      COREPORT_DIPSET(0x0010, "400 Million" ) \
      COREPORT_DIPSET(0x0020, "500 Million" ) \
      COREPORT_DIPSET(0x0030, "600 Million" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Reserved (SL1-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0040, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Game Mode") \
      COREPORT_DIPSET(0x0000, "Normal" ) \
      COREPORT_DIPSET(0x0080, "Exhibition (Free Play)" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Balls Per Game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0800, "5" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Reserved (SL2-2)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0400, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Handicap Score") \
      COREPORT_DIPSET(0x0000, "700 Million" ) \
      COREPORT_DIPSET(0x0100, "750 Million" ) \
      COREPORT_DIPSET(0x0200, "800 Million" ) \
      COREPORT_DIPSET(0x0300, "850 Million" ) \
    COREPORT_DIPNAME( 0x3000, 0x0000, "New Ticket Time") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x1000, "Short" ) \
      COREPORT_DIPSET(0x2000, "Medium" ) \
      COREPORT_DIPSET(0x3000, "Long" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Reserved (SL2-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x4000, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Reserved (SL2-8)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x8000, DEF_STR( On ) ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Automatic Replay Score") \
      COREPORT_DIPSET(0x0000, DEF_STR( No ) ) \
      COREPORT_DIPSET(0x0008, DEF_STR( Yes ) ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Reserved (SL3-2)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0004, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0003, 0x0000, "Loops For Extra Ball") \
      COREPORT_DIPSET(0x0000, "Captive Ball On Bridge" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
      COREPORT_DIPSET(0x0002, "2" ) \
      COREPORT_DIPSET(0x0003, "3" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "3-Ball Multiball Repeat Time") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x0010, "Short" ) \
      COREPORT_DIPSET(0x0020, "Medium" ) \
      COREPORT_DIPSET(0x0030, "Long" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Specials Difficulty") \
      COREPORT_DIPSET(0x0000, "Easy" ) \
      COREPORT_DIPSET(0x0040, "Hard" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Reserved (SL3-8)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0080, DEF_STR( On ) )

/* Mach 2, manual pages 11-12. Exhibition mode is not in the manual (SL1-8 "reserved"), but the
   game code skips the credit decrement with it on, as on the later games. The manual lists the
   ramp loops as SL2-8 ON = 2 loops; the code does loops = (SL2 >> 6) + 1, so it is SL2-7.
   Coins Per Game defaults to its first setting that is not reserved */
#define SPINB_DIPS_MACH2() \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x000f, 0x0004, "Coins Per Game") \
      COREPORT_DIPSET(0x0000, "Reserved (0)" ) \
      COREPORT_DIPSET(0x0001, "Reserved (1)" ) \
      COREPORT_DIPSET(0x0002, "Reserved (2)" ) \
      COREPORT_DIPSET(0x0003, "Reserved (3)" ) \
      COREPORT_DIPSET(0x0004, "50 ptas 1 Game, 200 ptas 5 Games" ) \
      COREPORT_DIPSET(0x0005, "50 ptas 1 Game, 100 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0006, "100 ptas 1 Game" ) \
      COREPORT_DIPSET(0x0007, "100 ptas 1 Game, 500 ptas 6 Games" ) \
      COREPORT_DIPSET(0x0008, "100 ptas 1 Game, 200 ptas 3 Games" ) \
      COREPORT_DIPSET(0x0009, "50 esc 1 Game" ) \
      COREPORT_DIPSET(0x000a, "50 esc 1 Game, 100 esc 3 Games" ) \
      COREPORT_DIPSET(0x000b, "100 esc 1 Game" ) \
      COREPORT_DIPSET(0x000c, "100 esc 1 Game, 200 esc 3 Games" ) \
      COREPORT_DIPSET(0x000d, "100 esc 1 Game, 200 esc 3 Games (2)" ) \
      COREPORT_DIPSET(0x000e, "100 esc 1 Game, 200 esc 3 Games (3)" ) \
      COREPORT_DIPSET(0x000f, "100 esc 1 Game, 200 esc 3 Games (4)" ) \
    COREPORT_DIPNAME( 0x0030, 0x0000, "Replay Score") \
      COREPORT_DIPSET(0x0000, "100 Million" ) \
      COREPORT_DIPSET(0x0010, "150 Million" ) \
      COREPORT_DIPSET(0x0020, "200 Million" ) \
      COREPORT_DIPSET(0x0030, "250 Million" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Reserved (SL1-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0040, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Game Mode") \
      COREPORT_DIPSET(0x0000, "Normal" ) \
      COREPORT_DIPSET(0x0080, "Exhibition (Free Play)" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Balls Per Game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0800, "5" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Unknown (SL2-2)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0400, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Handicap Score") \
      COREPORT_DIPSET(0x0000, "250 Million" ) \
      COREPORT_DIPSET(0x0100, "300 Million" ) \
      COREPORT_DIPSET(0x0200, "350 Million" ) \
      COREPORT_DIPSET(0x0300, "400 Million" ) \
    COREPORT_DIPNAME( 0x3000, 0x0000, "New Shot Time") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x1000, "10 Seconds" ) \
      COREPORT_DIPSET(0x2000, "12 Seconds" ) \
      COREPORT_DIPSET(0x3000, "15 Seconds" ) \
    COREPORT_DIPNAME( 0xc000, 0x0000, "Ramp Loops To Move Ramp") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x4000, "2" ) \
      COREPORT_DIPSET(0x8000, "3" ) \
      COREPORT_DIPSET(0xc000, "4" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Automatic Replay Score") \
      COREPORT_DIPSET(0x0000, DEF_STR( No ) ) \
      COREPORT_DIPSET(0x0008, DEF_STR( Yes ) ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Special On Plane") \
      COREPORT_DIPSET(0x0000, DEF_STR( No ) ) \
      COREPORT_DIPSET(0x0004, DEF_STR( Yes ) ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "Reserved (SL3-3)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0002, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0001, 0x0000, "Lock Difficulty") \
      COREPORT_DIPSET(0x0000, "Easy" ) \
      COREPORT_DIPSET(0x0001, "Hard" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Reserved (SL3-5)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0010, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Unknown (SL3-6)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0020, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Unknown (SL3-7)") \
      COREPORT_DIPSET(0x0000, DEF_STR( Off ) ) \
      COREPORT_DIPSET(0x0040, DEF_STR( On ) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Rescue Difficulty") \
      COREPORT_DIPSET(0x0000, "Easy" ) \
      COREPORT_DIPSET(0x0080, "Hard" )

/*-- Standard input ports --*/
#define SPINB_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SPINB_COMPORTS

/*-- Input ports with a game specific DIP switch set --*/
#define SPINB_INPUT_PORTS_DIPS_START(name,balls,dips) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SPINB_BUTTONPORTS \
    dips()

#define SPINB_INPUT_PORTS_END INPUT_PORTS_END

#define SPINB_COMINPORT       CORE_COREINPORT

#define SPINB_SOLSMOOTH       1 /* Smooth the Solenoids over this number of VBLANKS */
#define SPINB_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define SPINB_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define SPINB_CSOL(x) ((x)+24)

/*-- switch numbers --*/
#define SPINB_SWTEST     -8

/*-- Memory regions --*/
#define SPINB_MEMREG_CPU        REGION_CPU1
#define SPINB_MEMREG_DMD        REGION_CPU2
#define SPINB_MEMREG_SND1       REGION_CPU3
#define SPINB_MEMREG_SND2       REGION_CPU4

/*-- Main CPU regions and ROM --*/

/*-- 64K CPU ROM --*/
#define SPINB_ROMSTART(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
   NORMALREGION(0x10000, SPINB_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
       ROM_LOAD(n2, 0x2000, 0x2000, chk2)

#define SPINB_ROMSTART2(name, n1, chk1, n2, chk2) \
   ROM_START(name) \
   NORMALREGION(0x10000, SPINB_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x4000, chk1) \
       ROM_LOAD(n2, 0x4000, 0x4000, chk2)

/*-- DMD ROMS --*/
//NOTE: DMD board requires a trick to make them all fit into the 8051 memory region
//      ROM0 lives in 1st 64K space - 0 to 0xffff
//      RAM sits in next 16383 bytes - 0x10000 to 0x13ffe
//      DMD Command is stored as byte 0x13fff
//other ROMs live in the remaining space - 0x14000 to 0xfffff

/* 1 X 64K ROM */
#define SPINB_DMDROM1(n1, chk1) \
  NORMALREGION(0x14000, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1)

/* 1 X 64K ROM, 1 x 128K ROM */
#define SPINB_DMDROM2(n1, chk1, n2, chk2) \
  NORMALREGION(0x34000, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
   ROM_LOAD(n2, 0x14000, 0x20000, chk2)

/* 1 X 64K ROM, 2 x 512K ROM */
#define SPINB_DMDROM3(n1, chk1, n2, chk2, n3, chk3) \
  NORMALREGION(0x114000, SPINB_MEMREG_DMD) \
   ROM_LOAD(n1, 0x00000, 0x10000, chk1) \
   ROM_LOAD(n2, 0x14000, 0x80000, chk2) \
   ROM_LOAD(n3, 0x94000, 0x80000, chk3)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM23(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7) \
  NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
   ROM_LOAD(n3, 0x80000, 0x80000, chk3) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n4, 0x00000, 0x2000, chk4) \
  NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n5, 0x0000, 0x80000, chk5) \
   ROM_LOAD(n6, 0x80000, 0x80000, chk6) \
   ROM_LOAD(n7, 0x100000, 0x80000, chk7)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM22(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6) \
  NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
   ROM_LOAD(n3, 0x80000, 0x80000, chk3) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n4, 0x00000, 0x2000, chk4) \
  NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n5, 0x0000, 0x80000, chk5) \
   ROM_LOAD(n6, 0x80000, 0x80000, chk6)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM12(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n3, 0x00000, 0x2000, chk3) \
NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n4, 0x0000, 0x80000, chk4) \
   ROM_LOAD(n5, 0x80000, 0x80000, chk5)

/*-- SOUND ROMS --*/
#define SPINB_SNDROM13(n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6) \
  NORMALREGION(0x10000, SPINB_MEMREG_SND1) \
   ROM_LOAD(n1, 0x00000, 0x2000, chk1) \
NORMALREGION(0x180000, REGION_USER1) \
   ROM_LOAD(n2, 0x0000, 0x80000, chk2) \
NORMALREGION(0x10000, SPINB_MEMREG_SND2) \
   ROM_LOAD(n3, 0x00000, 0x2000, chk3) \
  NORMALREGION(0x180000, REGION_USER2) \
   ROM_LOAD(n4, 0x0000, 0x80000, chk4) \
   ROM_LOAD(n5, 0x80000, 0x80000, chk5) \
   ROM_LOAD(n6, 0x100000, 0x80000, chk6)

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(spinb);
extern MACHINE_DRIVER_EXTERN(spinbs1);
extern MACHINE_DRIVER_EXTERN(spinbs1n);
extern MACHINE_DRIVER_EXTERN(spinbs1n2);

#define mSPINB        spinb
#define mSPINBS       spinbs1
#define mSPINBSNMI    spinbs1n
#define mSPINBSNMI2   spinbs1n2
