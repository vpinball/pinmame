// license:BSD-3-Clause

// PinMAME P2K subsystem - boot harness.
//
// Loads a game's ROMs, builds the machine and runs the CPU from the reset vector, reporting
// what the boot code touches. A development tool for bringing the driver up, not a test

#include "p2k_driver.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Under PinMAME these arrive as ROM regions, already interleaved by ROM_LOAD32_WORD. This harness
// runs without PinMAME, so it reads the same files and lays them out the same way: four 16 MB
// banks, the even-numbered chip of each pair in the low half of every dword
static bool read_file(const std::string &path, std::vector<u8> &dest, size_t expected)
{
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) return false;
	dest.resize(expected);
	const size_t got = fread(dest.data(), 1, expected, f);
	fclose(f);
	return got == expected;
}

static bool build_prism_region(const char *dir, const char *prefix, std::vector<u8> &region)
{
	region.assign(4 * 0x1000000, 0);
	for (int bank = 0; bank < 4; bank++)
	{
		std::vector<u8> lo, hi;
		char a[64], b[64];
		snprintf(a, sizeof a, "%s_u%d.rom", prefix, 100 + bank * 2);
		snprintf(b, sizeof b, "%s_u%d.rom", prefix, 101 + bank * 2);
		if (!read_file(std::string(dir) + "/" + a, lo, 0x800000)) { printf("missing %s\n", a); return false; }
		if (!read_file(std::string(dir) + "/" + b, hi, 0x800000)) { printf("missing %s\n", b); return false; }
		u8 *out = region.data() + size_t(bank) * 0x1000000;
		for (size_t i = 0; i < 0x800000 / 2; i++)
		{
			out[i * 4 + 0] = lo[i * 2 + 0];
			out[i * 4 + 1] = lo[i * 2 + 1];
			out[i * 4 + 2] = hi[i * 2 + 0];
			out[i * 4 + 3] = hi[i * 2 + 1];
		}
	}
	return true;
}

extern "C" unsigned mediagx_get_reg(int regnum);
extern "C" unsigned mediagx_dasm(char *buffer, unsigned pc);

int main(int argc, char **argv)
{
	const char *romdir = (argc > 1) ? argv[1] : "roms";
	const char *prefix = (argc > 2) ? argv[2] : "rfm";
	u64 cycles = (argc > 3) ? strtoull(argv[3], nullptr, 0) : 1000000;

	p2k_state state;
	printf("loading %s ROMs from %s\n", prefix, romdir);
	std::vector<u8> prism;
	if (!build_prism_region(romdir, prefix, prism) ||
	    !state.set_prism_roms(prism.data(), prism.size(), prefix))
	{
		printf("ROM loading failed\n");
		return 1;
	}

	// optional: an assembled 8 MB update flash image
	if (const char *nv = getenv("P2K_NVRAM_UPDATES"))
	{
		std::vector<u8> upd;
		const bool ok = read_file(nv, upd, 0x800000) && state.set_nvram_updates(upd.data(), upd.size());
		printf("update flash %s: %s\n", nv, ok ? "loaded" : "FAILED");
	}

	state.build_machine(20000000);
	state.reset();
	state.set_trace(true);

	printf("running %llu cycles from the reset vector\n", (unsigned long long)cycles);
	state.run_cycles(cycles);

	// exercise the cpuintrf bridge the way PinMAME's debugger will: registers and disassembly
	{
		printf("\n--- bridge: disassembly at the boot loader entry ---\n");
		unsigned pc = 0xc0003;
		for (int i = 0; i < 6; i++)
		{
			char text[160] = "";
			unsigned len = mediagx_dasm(text, pc);
			printf("  %08x  %s\n", pc, text);
			pc += len ? len : 1;
		}
		printf("  EAX=%08x ESP=%08x\n", mediagx_get_reg(0x0b /*I386_EAX*/), mediagx_get_reg(-3 /*REG_SP*/));
	}

	printf("\n--- COM1 console (%zu bytes) ---\n%s\n--- end ---\n",
		state.console_log().size(), state.console_log().c_str());
	printf("\nemulated time: %.3f ms\n", state.machine().machine().time().as_double() * 1000.0);
	printf("unmapped accesses: %llu reads, %llu writes\n",
		(unsigned long long)state.unmapped_reads(), (unsigned long long)state.unmapped_writes());
	return 0;
}
