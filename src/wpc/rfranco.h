// license:BSD-3-Clause

#pragma once

#include "core.h"
#include "sim.h"

/*-- CPUs --*/
#define RFRANCO_CPU     0
#define RFRANCO_SCPU    1

/*-- Memory regions --*/
#define RFRANCO_MEMREG_CPU  REGION_CPU1
#define RFRANCO_MEMREG_SCPU REGION_CPU2

#define RFRANCO_DISPLAYSMOOTH 2 /* Smooth the display over this number of VBLANKS */

/*-- Standard input ports --
   The playfield switches arrive as two bytes shifted in serially on SID (see
   rfranco.c); these ports carry the cabinet inputs and the operator switches
   that sit on the door, described in the manual under "INSTRUCCIONES PARA
   AJUSTES, TEST Y VISUALIZACION DE RAM". */
#define RFRANCO_COMPORTS \
  PORT_START /* 0 */ \
    /* These land in swMatrix[2], which the driver hands back to the sound CPU
       as AY-3-8910 IC2 port A when the game asks with sound command 0x99. That
       is the only route a coin can reach the game. */ \
    COREPORT_BITDEF(  0x0010, IPT_COIN1,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_COIN2,   KEYCODE_3) \
    /* Not KEYCODE_HOME: that is Slam Tilt in some 35 other drivers, and a drain
       key is unique to this driver anyway (no other PinMAME game has one - they
       expect the outhole to be toggled as an ordinary matrix switch). BACKSPACE
       is used by no driver and bound to no MAME UI function, so it carries no
       expectation to contradict. */ \
    COREPORT_BIT(     0x0040, "Drain (caida de bolas)", KEYCODE_BACKSPACE) \
    COREPORT_BITDEF(  0x0080, IPT_START1,  IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0100, "Falta (Tilt)",   KEYCODE_INSERT) \
    /* The two operator switches on the door, done the way Williams System
       4-11 does its coin door: real toggles rather than DIPs, because that is
       what they are on the machine - each one stays where it is put and can be
       moved while the game runs. See the comment on RFRANCO_SWAJUSTE below for
       why that second part is not optional here.
       These two do NOT land in swMatrix[2] with everything above them; they go
       to the pseudo column 0, which is why they are grouped apart. */ \
    COREPORT_BITTOG(  0x0200, "Interruptor de ajuste", KEYCODE_7) \
    COREPORT_BITTOG(  0x0400, "Interruptor de test",   KEYCODE_8) \
    /* Port 1 is deliberately empty. The machine has no DIP switches at all -
       the two door switches used to be modelled as a pair of them, and the
       operator's real settings live in NVRAM, reached through the ajustes
       menu. The port itself stays because core reads one regardless of what
       MDRV_DIPS says: core_updateSw fills CORE_COREINPORT+(coreDips+31)/16
       inports, which is one even at zero. */ \
  PORT_START /* 1 */

#define RFRANCO_INPUT_PORTS_START(name, balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    RFRANCO_COMPORTS

#define RFRANCO_INPUT_PORTS_END INPUT_PORTS_END

#define RFRANCO_COMINPORT CORE_COREINPORT

/*-- The two operator door switches --
   Switch numbers, not DIP bits. They live in swMatrix[0], the pseudo column
   that no hardware byte of this machine occupies - the four real bytes are
   rows 1-4, the flipper buttons are core's column 11 and take no switch
   number here, and core's own manual switch chords only reach columns 1-8. It
   is the same place Williams System 4-11 keeps Advance and Auto/Manual
   (s11.c:784), reached through this driver's own col*10+row+1 numbering
   instead of the -7/-6 constants those drivers use: those only resolve
   through core's default sw2m, and this driver installs its own.

   Closed means the switch is UP, out of its resting position, so both open is
   juego and that is what a machine with nobody at the door boots into.

   They have to be switches rather than a setting because the ROM does not
   only read them at boot. Inside the ajustes menu it re-reads the pair on
   every pass and decides from it what the start button does: both still up
   steps the current zone's VALUE, either one put back down steps to the NEXT
   ZONE. Walking the menu means moving them on a running machine. */
#define RFRANCO_SWAJUSTE  1   /* interruptor de ajuste - AY IC2 port B bit 7 */
#define RFRANCO_SWTEST    2   /* interruptor de test   - AY IC2 port B bit 6 */

/*-- ROM loading --
   IC19 holds the 8085 game program (16K 27128); IC14, the second program
   socket, is unpopulated on every known board. IC4 holds the 8035 sound
   program (4K 2532). */
#define RFRANCO_ROMSTART(name, n1, chk1, n2, chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, RFRANCO_MEMREG_CPU) \
      ROM_LOAD(n1, 0x0000, 0x4000, chk1) \
    NORMALREGION(0x1000, RFRANCO_MEMREG_SCPU) \
      ROM_LOAD(n2, 0x0000, 0x1000, chk2)

#define RFRANCO_ROMEND ROM_END

/* The 2532 at IC4 has its data pins wired to the 8035's AD0-AD7 in reverse
   order. Undo that once per game start, from each set's DRIVER_INIT. */
extern void rfranco_unscramble_sound_rom(void);

extern MACHINE_DRIVER_EXTERN(RFRANCO);

#define gl_mRFRANCO RFRANCO
