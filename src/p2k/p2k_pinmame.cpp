// PinMAME P2K subsystem - the PinMAME-facing side of the machine.
//
// src/wpc/p2k.c is compiled as part of PinMAME and knows nothing about the subsystem's C++
// types; these four functions are the whole contract between them.
#include "p2k_driver.h"
#include <cstdlib>
#include <cstring>

using UINT32 = u32;

namespace { std::unique_ptr<p2k_state> g_machine; }

extern "C" {

void p2k_pinmame_start(const char *game)
{
	g_machine = std::make_unique<p2k_state>();

	// The real ROM set is loaded by the subsystem; point it somewhere with P2K_ROMS. The driver
	// says which game it is, so the file prefix follows the driver; P2K_GAME still overrides it
	// for experiments.
	const char *romdir = getenv("P2K_ROMS");
	const char *prefix = getenv("P2K_GAME");
	if (!prefix) prefix = (game && *game) ? game : "rfm";
	if (romdir) g_machine->load_roms(romdir, prefix);
	if (const char *nv = getenv("P2K_NVRAM_UPDATES")) g_machine->load_nvram_updates(nv);

	// The MAME driver runs the MediaGX at 20 MHz with the comment "should be 233MHz". That
	// downclock is not free: the firmware programs the PIT as a rate generator with divisor 298,
	// so a tick arrives every ~6400 CPU cycles at 20 MHz, and the operating system's tick handler
	// does not fit in that. P2K_CPU_HZ raises it for experiments.
	u32 cpu_hz = 20000000;
	if (const char *s = getenv("P2K_CPU_HZ")) { const long v = strtol(s, nullptr, 0); if (v > 0) cpu_hz = u32(v); }
	g_machine->build_machine(cpu_hz);
	g_machine->reset();
}

void p2k_pinmame_stop(void) { g_machine.reset(); }

// Persistent blocks: 0 = CMOS, 1 = PLX EEPROM, 2 = the update flash. PinMAME reads its NVRAM file
// before the machine is built and writes it after the machine is gone, so the driver buffers the
// bytes and these two move them in and out once it exists.
void p2k_pinmame_nvram_set(int which, const unsigned char *data, unsigned size)
{
	size_t n = 0;
	unsigned char *p = g_machine ? g_machine->nvram_block_ptr(p2k_state::nvram_block(which), &n) : nullptr;
	if (p && data) memcpy(p, data, size < n ? size : n);
}

unsigned p2k_pinmame_nvram_get(int which, unsigned char *data, unsigned size)
{
	size_t n = 0;
	const unsigned char *p = g_machine ? g_machine->nvram_block_ptr(p2k_state::nvram_block(which), &n) : nullptr;
	if (!p || !data) return 0;
	if (size > n) size = unsigned(n);
	memcpy(data, p, size);
	return size;
}

UINT32 p2k_pinmame_read(offs_t address, UINT32 mem_mask)
{
	return g_machine ? g_machine->mem_r(address, mem_mask) : 0xffffffff;
}

void p2k_pinmame_write(offs_t address, UINT32 data, UINT32 mem_mask)
{
	if (g_machine) g_machine->mem_w(address, data, mem_mask);
}

// The pinball I/O. PinMAME's core model owns the switch matrix and wants the lamp and coil
// state back; the driver board in the subsystem is the other end of that. Called from
// src/wpc/p2k.c once per frame for now - fast enough for lamps, and the point at which a faster
// sync would go if coils need it.
void p2k_pinmame_push_switches(const unsigned char *matrix, unsigned count)
{
	if (g_machine) g_machine->push_switches(matrix, count);
}

void p2k_pinmame_pull_outputs(unsigned char *lamps, unsigned lamp_columns, UINT32 *solenoids, UINT32 *solenoids2)
{
	if (g_machine) g_machine->pull_outputs(lamps, lamp_columns, solenoids, solenoids2);
}

// The current picture, as 0x00RRGGBB pixels, for PinMAME's video path. The caller passes a
// buffer and its capacity; the return value is the number of pixels written, 0 if the machine
// has no sane geometry yet. Pinball 2000 renders mirrored - the cabinet reflects the monitor
// into the playfield - so this hands back what the hardware holds and leaves flipping to
// whoever shows it.
unsigned p2k_pinmame_frame(UINT32 *dest, unsigned capacity, unsigned *width, unsigned *height)
{
	if (!g_machine || !dest) return 0;
	static std::vector<u32> rgb;
	unsigned w = 0, h = 0;
	if (!g_machine->frame_rgb(rgb, w, h)) return 0;
	const size_t n = rgb.size() < capacity ? rgb.size() : capacity;
	for (size_t i = 0; i < n; i++) dest[i] = rgb[i];
	if (width) *width = w;
	if (height) *height = h;
	return unsigned(n);
}

}
