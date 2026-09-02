// license:BSD-3-Clause
#pragma once

#include "core.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define SLEIC_ROMEND    ROM_END

/*-- Common Inports for SLEIC Games --*/
#define SLEIC_COMPORTS \
  PORT_START /* 0 */ \
    /* Switch Column 1 / direct switches port 0x00 on Io Moon */ \
    COREPORT_BIT(     0x0001, "Key 1 / L Flipper", KEYCODE_LSHIFT) \
    COREPORT_BIT(     0x0002, "Key 2 / R Flipper", KEYCODE_RSHIFT) \
    COREPORT_BIT(     0x0004, "Key 3",     KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Key 4",     KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Key 5",     KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Key 6",     KEYCODE_6) \
    COREPORT_BIT(     0x0040, "Key 7",     KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Key 8",     KEYCODE_8) \
    /* Cabinet buttons.  Every SLEIC machine here reads them on Z80 port 0x03 and every \
     * SWITCH_UPDATE handler below maps them into swMatrix[9], one bit per button, in the \
     * same bit order: 0 = TILT, 1 = TEST, 2 = right flipper, 3 = left flipper, 4 = START, \
     * 5 = COIN.  The port-0x03 CODE each bit produces differs per machine (Io Moon F5: \
     * 0x3E/0x3F/0x42/0x41/0x40/0x32), so the per-driver handler is the place that names \
     * them; see SWITCH_UPDATE(SLEIC2) for what the Io Moon firmware does with each. \
     * Nothing is mapped into swMatrix[10] (Io Moon's port-0x04 config byte). */ \
    COREPORT_BITDEF(  0x0100, IPT_START1, IP_KEY_DEFAULT) /* START -> swMatrix[9].4 */ \
    COREPORT_BITDEF(  0x0200, IPT_COIN1,  IP_KEY_DEFAULT) /* COIN  -> swMatrix[9].5 */ \
    COREPORT_BITDEF(  0x0400, IPT_TILT,   IP_KEY_DEFAULT) /* TILT  -> swMatrix[9].0 */ \
    COREPORT_BIT(     0x0800, "Test / Service Menu", KEYCODE_END) /* -> swMatrix[9].1; service-menu enter: Bike Race C4 Test code 0x33, Io Moon code 0x3F (F14). F2 is grabbed by MAME UI, End avoids stale-cfg unbinding */ \
  /* DIP block SW40 on the Z80 I/O board, read as PinMAME DIP bank 0 (core_getDip(0)). \
   * The FUNCTIONS are the Io Moon service manual's, section 7.2.2.3: SW1 VDB solenoid \
   * watchdog, SW2-SW4 country code, SW5 "no balls dispensed", SW6 solenoid test, SW7 \
   * lamp test, SW8 board self-test.  The switch -> BIT assignment is only established \
   * for SW2-SW4, and it is established from the ROM rather than the manual, which gives \
   * no bit numbers: Z80 command 0xF9 -> handler 2D9D reads port 0x04 and sends the low \
   * nibble back as 0xF0|nibble, and the 80188 turns bits 1-3 of it into a country number \
   * 0..7 (D5CA3-D5CC2, seven-way table at D5D01).  Only SLEIC2 reads any of this; the \
   * sister machines' Z80 ROMs were not traced for it, so the rest stay plain switches. \
   * \
   * COUNTRY is what the manual calls the coin-value setting, and it is more than that: \
   * it picks the pricing preset (sub_D69CC -> one of eight, saved to NVRAM 0x1C4-0x1CF) \
   * AND the display LANGUAGE, because value 5 selects the Spanish string and menu-record \
   * tables at three sites (D3277 attract, D8048 pricing, DD406 menu records) while every \
   * other value gets the English ones.  Two independent checks anchor the manual's row \
   * order to the value: value 0 (all three ON) loads exactly the manual's UK column \
   * (30p/1, 50p/2, GBP1/5 -> divisors 3/5/10, credits 1/2/5 at D6D36), and the manual's \
   * Spain row is SW2 OFF / SW3 ON / SW4 OFF, which with ON = 0 and SW2 as the low bit is \
   * value 5 -- the one value the ROM special-cases.  ON = 0 is the ordinary closed-switch \
   * convention and the Z80 reads port 0x04 without inverting it. */ \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "SW40-1 (VDB watchdog)") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0001, "Off" ) \
    COREPORT_DIPNAME( 0x000e, 0x000e, "SW40-2/3/4 Country") \
      COREPORT_DIPSET(0x0000, "United Kingdom" ) \
      COREPORT_DIPSET(0x0002, "France" ) \
      COREPORT_DIPSET(0x0004, "Germany" ) \
      COREPORT_DIPSET(0x0006, "Italy" ) \
      COREPORT_DIPSET(0x0008, "Netherlands" ) \
      COREPORT_DIPSET(0x000a, "Spain (Spanish text)" ) \
      COREPORT_DIPSET(0x000c, "Belgium" ) \
      COREPORT_DIPSET(0x000e, "Portugal" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "SW40-5") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0010, "Off" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "SW40-6") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0020, "Off" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "SW40-7") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0040, "Off" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "SW40-8") \
      COREPORT_DIPSET(0x0000, "On" ) \
      COREPORT_DIPSET(0x0080, "Off" )

/*-- Standard input ports --*/
#define SLEIC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SLEIC_COMPORTS

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
