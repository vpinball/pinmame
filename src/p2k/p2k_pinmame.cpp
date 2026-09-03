// license:BSD-3-Clause

// PinMAME P2K subsystem - the PinMAME-facing side of the machine
//
// src/wpc/p2k.c is compiled as part of PinMAME and knows nothing about the subsystem's C++
// types; the functions below are the whole contract between them

#include "p2k_driver.h"
#include "p2k_debug.h"
#include <cstdlib>
#include <cstring>

using UINT32 = u32;

namespace { std::unique_ptr<p2k_state> g_machine; }

extern "C" {

// The ROMs arrive as PinMAME memory regions, which src/wpc/p2k.c passes in: the subsystem does not
// know about PinMAME's ROM system and does not open files. Everything a machine needs is in its
// ROM set, so a version is selected, audited and zipped exactly like any other game's
void p2k_pinmame_start(const unsigned char *prism, unsigned prismLen,
                       const unsigned char *updates, unsigned updatesLen)
{
	g_machine = std::make_unique<p2k_state>();


	// A failure here means the ROM region is short or absent, which PinMAME's own audit should
	// have caught first - but the machine would otherwise run with empty ROM banks and draw
	// nothing, and a black screen with no message is indistinguishable from an emulation bug
	if (!g_machine->set_prism_roms(prism, prismLen))
		fprintf(stderr, "[p2k] the MediaGX ROM region is missing or short (%u bytes, need %u) - "
		                "this machine will show nothing\n", prismLen, 4u * 0x1000000u);
#if P2K_DEBUG
	// P2K_NO_UPDATE=1 hides a set's update flash, so a shipped game boots the way the prototypes do
	if (getenv("P2K_NO_UPDATE")) { updates = nullptr; updatesLen = 0; }
#endif
	// No update region at all is a machine that has no update flash: the game is in the Prism ROMs
	// and the loader starts the copy there. The flash stays erased, which is what such a board
	// reads, and there is nothing to report. Present but short is the error case - that set is damaged
	if (updatesLen != 0 && !g_machine->set_nvram_updates(updates, updatesLen))
		fprintf(stderr, "[p2k] the update flash region is short (%u bytes, need %u) - "
		                "the machine does not boot without it\n", updatesLen, 0x800000u);

	// We could run the MediaGX at 233MHz by now (but gameplay also ran okayish when it was e.g. 233/4), real life modders go even higher to prevent stutter.
	// But it requires a pretty modern CPU, as its all single threaded, so we compromise on 233/3 ~= 77.7MHz.
	// A potential downclock is not free: the firmware programs the PIT as a rate generator with divisor 298,
	// so a tick arrives every e.g. ~6400 CPU cycles if it would run at 20 MHz, and the operating system's tick handler
	// would not even fit in that
	u32 cpu_hz = 233000000/3; //!! sync with p2k.c
#if P2K_DEBUG
	// P2K_CPU_HZ raises it, which is how that was measured
	if (const char *s = getenv("P2K_CPU_HZ")) { const long v = strtol(s, nullptr, 0); if (v > 0) cpu_hz = u32(v); }
#endif
	g_machine->build_machine(cpu_hz);
	g_machine->reset();
}

void p2k_pinmame_stop(void) { g_machine.reset(); }

// Persistent blocks: 0 = CMOS, 1 = PLX EEPROM, 2 = the update flash. PinMAME reads its NVRAM file
// before the machine is built and writes it after the machine is gone, so the driver buffers the
// bytes and these two move them in and out once it exists
void p2k_pinmame_nvram_set(int which, const unsigned char *data, unsigned size)
{
	size_t n = 0;
	unsigned char *p = /*g_machine ?*/ g_machine->nvram_block_ptr(p2k_state::nvram_block(which), &n) /*: nullptr*/;
	if (p && data) memcpy(p, data, size < n ? size : n);
	// and back the other way: into the device, with the years slept through folded into register 9
	if (which == P2K_NV_BLOCK_RTC) g_machine->rtc_restore();
}

unsigned p2k_pinmame_nvram_get(int which, unsigned char *data, unsigned size)
{
	size_t n = 0;
	// The clock lives in the device, not in a buffer, so it is copied out and stamped with the host
	// time first - that stamp is how the next start knows how long the machine was off
	if (which == P2K_NV_BLOCK_RTC) g_machine->rtc_save();
	const unsigned char *p = /*g_machine ?*/ g_machine->nvram_block_ptr(p2k_state::nvram_block(which), &n) /*: nullptr*/;
	if (!p || !data) return 0;
	if (size > n) size = unsigned(n);
	memcpy(data, p, size);
	return size;
}

UINT32 p2k_pinmame_read(offs_t address, UINT32 mem_mask)
{
	return /*g_machine ?*/ g_machine->mem_r(address, mem_mask) /*: 0xffffffff*/;
}

void p2k_pinmame_write(offs_t address, UINT32 data, UINT32 mem_mask)
{
	/*if (g_machine)*/ g_machine->mem_w(address, data, mem_mask);
}

// Put the host's date and time into the machine's real-time clock, on every start, so it shows real
// time rather than the clock it had when it was last switched off. keep_year must be set for a
// machine that has a clock of its own: the firmware treats the year register as a count of years to
// add rather than a date, so handing it the host year again makes the displayed year climb. See p2k_state::clock_from_host()
void p2k_pinmame_clock_from_host(int keep_year)
{
	if (g_machine) g_machine->clock_from_host(keep_year != 0);
}

// The power driver board's DIP switch byte, register 0x02: the country code, which is what the
// pricing tables key off. Pushed every frame, but the machine only reads it during startup, so a
// change takes effect on the next boot - exactly as moving the physical switches would
void p2k_pinmame_set_dips(unsigned char dips)
{
	if (g_machine) g_machine->set_dips(dips);
}

// The pinball I/O. PinMAME's core model owns the switch matrix and wants the lamp and coil state
// back; the driver board in the subsystem is the other end of that. Switches go in live, and the
// outputs come back as what the board did since the last pull rather than what it is doing at this
// instant - it chops its coils far faster than anything here is called, so a sample would alias.
// See p2k_state::pull_outputs and the note on its accumulators

// The array to read the matrix from. The caller keeps ownership and must outlive the machine - src/wpc/p2k.c passes coreGlobals.swMatrix, which is static
void p2k_pinmame_set_switch_source(const volatile unsigned char *matrix)
{
	if (g_machine) g_machine->set_switch_source(matrix);
}

// Where to report a coil edge to. src/wpc/p2k.c passes a function that hands it to PinMAME's PWM
// integrator; a standalone build passes nothing and the machine runs exactly as before
void p2k_pinmame_set_solenoid_notify(void (*fn)(UINT32 solenoids, UINT32 solenoids2))
{
	if (g_machine) g_machine->set_solenoid_notify(fn);
}

// Where to report a lamp column strobe to, for the same reason as the coil edge above
void p2k_pinmame_set_lamp_notify(void (*fn)(unsigned char columns, unsigned char row_a, unsigned char row_b))
{
	if (g_machine) g_machine->set_lamp_notify(fn);
}

// With a live source set this only refreshes the fallback copy, but it also carries the debug hooks
void p2k_pinmame_push_switches(const unsigned char *matrix, unsigned count)
{
	/*if (g_machine)*/ g_machine->push_switches(matrix, count);
}

void p2k_pinmame_pull_outputs(unsigned char *lamps, unsigned lamp_columns, UINT32 *solenoids, UINT32 *solenoids2, UINT32 *solNow, UINT32 *sol2Now)
{
	/*if (g_machine)*/ g_machine->pull_outputs(lamps, lamp_columns, solenoids, solenoids2, solNow, sol2Now);
}

// The current picture, as 0x00RRGGBB pixels, for PinMAME's video path. The caller passes a
// buffer and its capacity; the return value is 1 on success, 0 if the machine
// has no sane geometry yet. Pinball 2000 renders mirrored - the cabinet reflects the monitor
// into the playfield - so this hands back what the hardware holds and leaves flipping to
// whoever shows it
unsigned p2k_pinmame_frame(UINT32 *dest, unsigned capacity, unsigned *width, unsigned *height, const unsigned fast_15bpp_path, unsigned *fast_15bpp_path_success)
{
	if (!g_machine || !dest || !width || !height) return 0;
	bool tmp_success = false;
	const bool result = g_machine->frame_rgb(dest, capacity, *width, *height, fast_15bpp_path, tmp_success);
	*fast_15bpp_path_success = tmp_success;
	return result;
}

}
