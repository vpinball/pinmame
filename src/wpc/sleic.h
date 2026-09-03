// license:BSD-3-Clause
#pragma once

#include "core.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SLEIC_ROMEND    ROM_END

/*-- Cabinet buttons, shared by every SLEIC machine.
 *
 * KEY MAP (all three machines; Io Moon adds one more, see SLEIC2_CABPORT):
 *
 *   Left Shift   left flipper        Right Shift  right flipper
 *   1            start               5            coin
 *   T            tilt                7            test / service menu
 *
 * That is the PinMAME convention throughout: Left/Right Shift are the flippers in
 * CORE_PORTS itself, 1/5/T are MAME's own IPT_START1 / IPT_COIN1 / IPT_TILT defaults
 * (inptport.c), and 7 is the test/self-test/diagnostic button in by35, by6803, gts1,
 * gts3, atari, alvg, ltd, zac and spinb.  These machines used to put test on End
 * instead; across PinMAME End is a coin-door-class TOGGLE (Coin Door in wpc/gts3,
 * Memory Protect in se, Ticket in alvg), not a momentary test button, so it moved to 7.
 * A saved .cfg keeps whatever binding it was saved with, so delete the game's file in
 * cfg/ to pick the new default up.
 *
 * There is ONE coin key because the cabinet has ONE coin line: a single electronic
 * mechanism on the door, wired to Z80 port 0x03 bit 5, and the only credit source in
 * either ROM.  On Io Moon a press of it is a COIN, not a contact closure -- the mechanism
 * answers a coin with a pulse train whose length is the coin's value, and the firmware
 * prices pulses, so the driver emits that train (iomoon_coin_update in sleic.c).  One
 * press therefore buys at least one credit under every country setting; the three coin
 * VALUES the country switch selects are denominations on that same line, not three
 * switches, so there is no coin 2 / coin 3 key to bind.
 *
 * There is no separate 8/9/0 service cluster because these machines have no separate
 * service buttons: the service menu is walked with the flipper buttons, which is what
 * the cabinet does.  Io Moon sends codes 0x41/0x42 for them and sub_DD480's dispatch at
 * DD501 uses those as scroll and select (F14); Bike Race and Sleic Pin-Ball do the same
 * with their own codes.
 *
 * Six further entries used to sit here -- "Key 3".."Key 8" on keys 3..8, feeding
 * swMatrix[0] through the base SWITCH_UPDATE(SLEIC).  Every machine replaces that
 * handler (SLEIC1/SLEIC2/SLEIC3 each map their own cabinet block into swMatrix[9]), so
 * nothing ever read them, and two of them clashed with keys that have a job: key 5 is
 * MAME's coin key, and keys 7/8 are the service keys.  They are gone; the bits they had
 * (0x0004-0x0080) are simply unused, so every other binding keeps the mask it had and a
 * saved cfg stays valid.  The playfield contacts are NOT here either -- each machine
 * maps its own matrix with its own key table (iomoon_pf_keys, sleic1_pf_keys,
 * sleic3_pf_keys), which is what lets the contact self-test exercise every position.
 *
 * Bits 0/1 are the flipper BUTTONS, read as cabinet inputs (Io Moon swMatrix[9] bits
 * 3/2); on Io Moon port 0x00 IN is the J1 inbound byte, not a switch-matrix read.
 --*/
#define SLEIC_CABPORT \
  PORT_START /* 0 */ \
    COREPORT_BIT(     0x0001, "L Flipper", KEYCODE_LSHIFT) \
    COREPORT_BIT(     0x0002, "R Flipper", KEYCODE_RSHIFT) \
    /* Cabinet buttons.  Every SLEIC machine here reads them on Z80 port 0x03 and every \
     * SWITCH_UPDATE handler below maps them into swMatrix[9], one bit per button, in the \
     * same bit order: 0 = TILT, 1 = TEST, 2 = right flipper, 3 = left flipper, 4 = START, \
     * 5 = COIN.  The port-0x03 CODE each bit produces differs per machine (Io Moon F5: \
     * 0x3E/0x3F/0x42/0x41/0x40/0x32), so the per-driver handler is the place that names \
     * them; see SWITCH_UPDATE(SLEIC2) for what the Io Moon firmware does with each. \
     * Nothing is mapped into swMatrix[10] (Io Moon's port-0x04 config byte). */ \
    COREPORT_BITDEF(  0x0100, IPT_START1, IP_KEY_DEFAULT) /* START -> swMatrix[9].4 (key 1) */ \
    COREPORT_BITDEF(  0x0200, IPT_COIN1,  IP_KEY_DEFAULT) /* COIN  -> swMatrix[9].5 (key 5); on Io Moon one press = one coin = a pulse train */ \
    COREPORT_BITDEF(  0x0400, IPT_TILT,   IP_KEY_DEFAULT) /* TILT  -> swMatrix[9].0 (key T) */ \
    COREPORT_BIT(     0x0800, "Test / Service Menu", KEYCODE_7) /* -> swMatrix[9].1; service-menu enter: Bike Race C4 Test code 0x33, Io Moon code 0x3F (F14) */ \

/*-- Common Inports for SLEIC games whose DIP block has not been traced --*/
#define SLEIC_COMPORTS \
  SLEIC_CABPORT \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" )

/*-- Io Moon adds one cabinet input the other machines have no use for: the ball drain.
 * It belongs to the driver's OPTIONAL internal trough model (SWITCH_UPDATE(SLEIC2)), and
 * that model is OFF BY DEFAULT -- so by default this key does nothing at all.
 *
 * Why the model exists: the firmware BLOCKS on the trough contacts -- 80188 commands
 * 0xE9/0xEF and the Z80 handlers 2B03/2BC7 wait on the column-0 contacts, and with no
 * ball anywhere the Z80 never returns to its main loop.  Why it is not the default:
 * PinMAME's convention is that the FRONTEND closes switches, and swMatrix[1] bits 0-3
 * are ordinary switches a VPinMAME table script (or, standalone, the Q/W/E/R matrix test
 * keys) drives.  A driver that seeds balls invents state the frontend then fights.
 *
 * The opt-in knob is the standard simulator port's "Balls" setting, in SIM_PORTS below:
 * 0 (the default) = model off, frontend-driven; 3 = the internal three-ball model, for
 * standalone desktop play.  With it at 0 this input and "Shoot Ball" are both INERT --
 * iomoon_ball_update returns before it reads either -- so nothing is half-live.
 *
 * The bit is 0x1000, appended after the shared block's 0x0800, so every existing binding
 * keeps the bit it already had.  Backspace because the MAME UI does not claim it and it
 * is not a key the cabinet map or the PinMAME service convention wants: the digits the
 * retired "Key 3".."Key 8" entries freed are left unbound on purpose, since 8/9/0 are
 * where PinMAME puts service buttons and this machine has none. */
#define SLEIC2_CABPORT \
  SLEIC_CABPORT \
    COREPORT_BIT(     0x1000, "Drain ball (needs Balls=3)", KEYCODE_BACKSPACE)

/*-- Io Moon (SLEIC2) inports.  Same cabinet block plus the drain, but its DIP block is \
 * the real SW40. \
 * \
 * Read as PinMAME DIP bank 0 (core_getDip(0)); only SLEIC2 reads it, which is why it is \
 * a separate macro -- the sister machines' Z80 ROMs were never traced for SW40 and keep \
 * the plain S1..S8 above rather than showing labels they do not honour. \
 * \
 * The FUNCTIONS are the Io Moon service manual's, section 7.2.2.3: SW1 VDB solenoid \
 * watchdog, SW2-SW4 country code, SW5 "no balls dispensed", SW6 solenoid test, SW7 lamp \
 * test, SW8 board self-test.  Only SW2-SW4 is wired here, and only it is established \
 * from the ROM -- the manual gives no bit numbers.  Z80 command 0xF9 -> handler 2D9D \
 * reads port 0x04 and sends the low nibble back as 0xF0|nibble; the 80188 turns bits 1-3 \
 * of it into a country number 0..7 (D5CA3-D5CC2, seven-way table at D5D01). \
 * \
 * COUNTRY is what the manual calls the coin-value setting, and it is more than that: it \
 * picks the pricing preset (sub_D69CC -> one of eight, saved to NVRAM 0x1C4-0x1CF) AND \
 * the display LANGUAGE, because value 5 selects the Spanish string and menu-record \
 * tables at three sites (D3277 attract, D8048 pricing, DD406 menu records) while every \
 * other value gets the English ones. \
 * \
 * WHICH SWITCH IS THE LOW BIT is decided by the presets, not by the two obvious \
 * anchors.  Country 0 = the manual's UK row and country 5 = its Spain row are both \
 * INVARIANT under reversing SW2 and SW4 (000 and 101 are palindromes), so neither \
 * discriminates; so is Germany (010).  The rows that do are Italy, Netherlands, France \
 * and Belgium, and with SW2 as the LOW bit the presets read out of the emulator match \
 * the manual on six of the eight rows: \
 * \
 *   value  divisors (pulses)  credits      manual row     verdict \
 *     0        3 / 5 / 10      1 / 2 / 5   United Kingdom exact \
 *     1        2 / 5 / 10      1 / 3 / 7   France         DOES NOT MATCH (manual 3/5/10, 1/2/5) \
 *     2        1 / 2 / 5       1 / 3 / 8   Germany        exact \
 *     3        1 / 2 / 4       1 / 3 / 7   Italy          exact \
 *     4        2 / 5 / 10      1 / 3 / 7   Netherlands    exact \
 *     5      2 / 4 / 8 / 20  1 / 3 / 7 / 18 Spain         coins exact, 3rd credit 7 vs 8 \
 *     6        2 / 4 / 10      1 / 3 / 8   Belgium        DOES NOT MATCH (manual 2/5/10, 1/3/7) \
 *     7        1 / 2 / 4       1 / 3 / 6   Portugal       coins exact, 3rd credit 6 vs 7 \
 * \
 * Reverse SW2 and SW4 and three of those four discriminating rows break: value 3 would \
 * be Belgium (2/5/10 against the ROM's 1/2/4), value 4 France (3/5/10 against 2/5/10) \
 * and value 6 Italy (1/2/4 against 2/4/10), buying only Netherlands at value 1.  Six \
 * matching rows against three broken ones settles it: SW2 is the low bit, ON = 0, which \
 * is also the ordinary closed-switch convention and how the Z80 reads port 0x04 (no \
 * inversion).  Values 1 and 6 are labelled for their manual ROW; the coin values the \
 * ROM actually loads there are not the manual's, and value 1 is a duplicate of value 4. \
 * \
 * WHAT ONE COIN BUYS, per setting, because that is what the country switch is FOR.  The \
 * coin key inserts one coin of the smallest denomination the country accepts (SLEIC_CABPORT \
 * above, and iomoon_coin_update in sleic.c for why a key is a coin and not a pulse), so \
 * this column is what one press gives you on a fresh machine: \
 * \
 *   value  country          smallest coin    one press   two presses  three presses \
 *     0    United Kingdom   30p  (3 pulses)     1 credit    2           3 \
 *     1    France(ROM=NL)   2 pulses            1           2           3 \
 *     2    Germany          1 DM (1 pulse)      1           3           4 \
 *     3    Italy            500 L (1 pulse)     1           3           4 \
 *     4    Netherlands      1 Fl (2 pulses)     1           2           3 \
 *     5    Spain            50 pta (2 pulses)   1           3           4 \
 *     6    Belgium          2 pulses            1           3           4 \
 *     7    Portugal         50 Esc (1 pulse)    1           3           4 \
 * \
 * (No setting needs more than one press for the first credit, and none can: every preset's \
 * smallest coin is worth at least one credit -- that is what a smallest coin IS.  The \
 * later columns do not simply add up because the firmware prices the RUNNING PULSE TOTAL \
 * greedily, largest coin first, so two 1 DM coins are priced as one 2 DM coin and pay the \
 * 2 DM bonus -- that is the machine's own arithmetic (sub_DD03D), not a driver artefact. \
 * The larger denominations are not separate keys because they are not separate switches; \
 * see the coin comment in sleic.c.) \
 * \
 * DEFAULT is Netherlands -- country 4, SW2/SW3 ON and SW4 OFF, DIP value 0x0008 -- by \
 * OWNER PREFERENCE, and it is the setting the owner's own cabinet ran: guilders, one coin \
 * per game.  It is English text (country 5 is the only value that selects the Spanish \
 * tables, so every other value is English) and its preset is one of the six that match \
 * the service manual exactly -- 1 Fl/1, 2,5 Fl/3, 5 Fl/7 = 2/5/10 pulses for 1/3/7 \
 * credits -- with none of the France/Belgium coin-value discrepancies and no off-by-one \
 * credit like Spain's or Portugal's.  United Kingdom (0x0000) is equally exact and stays \
 * one click away; it is not the default only because the machine this driver is compared \
 * against is a Dutch one.  Verified end to end at the Netherlands default, from the DIP \
 * rather than an override: country byte 4, preset 2/5/10 and 1/3/7, one coin key \
 * press -> 1 credit, and START -> mode 3 -> song 1, with the DMD drawing the English \
 * ADJUSTMENT / SOUND-VIDEO / GAME / TECHNICAL menu. \
 * Note country 0 reaches its preset through two fall-through paths -- the country table \
 * at D5D01 is indexed from 2 so a nibble of 0 lands on D5CF8's "country 0", and \
 * sub_D69CC's bounds check sends 0 to the default preset sub_D6D36 -- which is by \
 * design, not an accident: D6D36 IS the UK column. \
 * Set Spain if the machine being compared against is a Spanish one; that is the market \
 * Io Moon was built for and the path the ROM exercises most (its own pricing routine \
 * sub_DCD9E and the only fourth coin value). */ \
#define SLEIC2_COMPORTS \
  SLEIC2_CABPORT \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "SW40-1 VDB watchdog (not modelled)") \
      COREPORT_DIPSET(0x0000, "On (watchdog disabled)" ) \
      COREPORT_DIPSET(0x0001, "Off (watchdog enabled)" ) \
    COREPORT_DIPNAME( 0x000e, 0x0008, "SW40-2/3/4 Country") \
      COREPORT_DIPSET(0x0000, "United Kingdom" ) \
      COREPORT_DIPSET(0x0002, "France (ROM coins differ)" ) \
      COREPORT_DIPSET(0x0004, "Germany" ) \
      COREPORT_DIPSET(0x0006, "Italy" ) \
      COREPORT_DIPSET(0x0008, "Netherlands" ) \
      COREPORT_DIPSET(0x000a, "Spain (Spanish text)" ) \
      COREPORT_DIPSET(0x000c, "Belgium (ROM coins differ)" ) \
      COREPORT_DIPSET(0x000e, "Portugal" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "SW40-5 Service: no balls dispensed") \
      COREPORT_DIPSET(0x0000, "On (0xED answers at once, no ball check)" ) \
      COREPORT_DIPSET(0x0010, "Off (normal play)" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "SW40-6 solenoid test (not modelled)") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0020, "Off" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "SW40-7 lamp test (not modelled)") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0040, "Off" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "SW40-8 board self-test (not modelled)") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0080, "Off" )

/*-- Standard input ports --*/
#define SLEIC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SLEIC_COMPORTS

/* Io Moon.  <balls> is SIM_PORTS' "Balls" default and, for this machine only, it doubles
 * as the on/off switch for the driver's internal ball-trough model: sleicgames.c passes
 * 0 for every Io Moon set, so the trough contacts are plain frontend-driven switches
 * unless the operator sets "Balls" to 3.  See SLEIC2_CABPORT above and the trough
 * comment in sleic.c */
#define SLEIC2_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SLEIC2_COMPORTS

#define SLEIC_INPUT_PORTS_END INPUT_PORTS_END

#define SLEIC_COMINPORT       CORE_COREINPORT

#define SLEIC_LAMPSMOOTH      1 /* Smooth the lamps over this number of VBLANKS */
#define SLEIC_DISPLAYSMOOTH   1 /* Smooth the display over this number of VBLANKS */
#define SLEIC_SOLSMOOTH       1 /* Smooth the Solenoids over this number of VBLANKS */

/*-- Memory regions --*/
#define SLEIC_MEMREG_CPU     REGION_CPU1
#define SLEIC_MEMREG_IO      REGION_CPU2
#define SLEIC_MEMREG_DISPLAY REGION_CPU3
/* Io Moon's graphics ROM: not CPU-addressable at a fixed address, it is paged into
 * segment 6000 a 64 KB page at a time (findings F2), so it lives in its own region */
#define SLEIC_MEMREG_GFX     REGION_USER2

/* CPUs */
#define SLEIC_MAIN_CPU    0
#define SLEIC_IO_CPU      1
#define SLEIC_DISPLAY_CPU 2

#define SLEIC_ROMSTART4(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
ROM_START(name) \
  NORMALREGION(0x100000, SLEIC_MEMREG_CPU) \
    ROM_LOAD(n3, 0xe0000, 0x20000, chk3) \
  NORMALREGION(0x10000, SLEIC_MEMREG_IO) \
    ROM_LOAD(n4, 0x0000, 0x8000, chk4) \
  NORMALREGION(0x10000, SLEIC_MEMREG_DISPLAY) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
  NORMALREGION(0x100000, REGION_USER1) \
    ROM_LOAD(n2, 0x00000, 0x80000, chk2)

/* Io Moon (SLEIC2).  The 80188 sees ROM1 (n1) through two chip selects that are not
 * adjacent in its address space, so the image is placed twice in the CPU region:
 *   LMCS 0x00000-0x3FFFF <- ROM1 file 0x00000-0x3FFFF (resident IVT + animation data)
 *   UMCS 0xC0000-0xFFFFF <- ROM1 file 0x40000-0x7FFFF (all code, reset vector at FFFF0)
 * The full image is loaded at 0x80000 (so file 0x40000 lands on 0xC0000) and its low
 * 256 KB is reloaded at 0, which leaves 0x40000-0x7FFFF empty for the mid-range RAM
 * windows to alias into.  The graphics ROM (n2) is NOT flat in the address space: it
 * is paged one 64 KB page at a time into segment 6000 by PCS0 bits 0-2, so it stays
 * in its own region and the driver banks it.  n3/n4 are the OKI sample ROMs, n5 the
 * Z80 I/O ROM.  See findings F1/F2 */
#define SLEIC_ROMSTART5(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5) \
ROM_START(name) \
  NORMALREGION(0x100000, SLEIC_MEMREG_CPU) \
    ROM_LOAD(n1, 0x80000, 0x80000, chk1) \
    ROM_RELOAD(  0x00000, 0x40000) \
  NORMALREGION(0x10000, SLEIC_MEMREG_IO) \
    ROM_LOAD(n5, 0x0000, 0x8000, chk5) \
  NORMALREGION(0x100000, REGION_USER1) \
    ROM_LOAD(n3, 0x00000, 0x80000, chk3) \
    ROM_LOAD(n4, 0x80000, 0x80000, chk4) \
  NORMALREGION(0x100000, SLEIC_MEMREG_GFX) \
    ROM_LOAD(n2, 0x00000, 0x80000, chk2)

#define SLEIC_ROMSTART7(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4, n5, chk5, n6, chk6, n7, chk7) \
ROM_START(name) \
  NORMALREGION(0x100000, SLEIC_MEMREG_CPU) \
    ROM_LOAD(n6, 0x20000, 0x20000, chk6) \
    ROM_LOAD(n5, 0x40000, 0x20000, chk5) \
    ROM_LOAD(n4, 0xe0000, 0x20000, chk4) \
  NORMALREGION(0x10000, SLEIC_MEMREG_IO) \
    ROM_LOAD(n7, 0x0000, 0x8000, chk7) \
  NORMALREGION(0x10000, SLEIC_MEMREG_DISPLAY) \
    ROM_LOAD(n1, 0x0000, 0x2000, chk1) \
  NORMALREGION(0x100000, REGION_USER1) \
    ROM_LOAD(n2, 0x00000, 0x80000, chk2) \
    ROM_LOAD(n3, 0x80000, 0x80000, chk3) \
  NORMALREGION(0x100000, REGION_USER2) \
    ROM_LOAD(n5, 0x00000, 0x20000, chk5) \
    ROM_LOAD(n6, 0x20000, 0x20000, chk6)

/*-- These are only here so the game structure can be in the game file --*/

extern MACHINE_DRIVER_EXTERN(SLEIC1);
extern MACHINE_DRIVER_EXTERN(SLEIC2);
extern MACHINE_DRIVER_EXTERN(SLEIC3);

#define gl_mSLEIC1      SLEIC1
#define gl_mSLEIC2      SLEIC2
#define gl_mSLEIC3      SLEIC3
