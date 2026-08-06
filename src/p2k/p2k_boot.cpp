// PinMAME P2K subsystem - boot harness.
//
// Loads a game's ROMs, builds the machine and runs the CPU from the reset vector, reporting
// what the boot code touches. A development tool for bringing the driver up, not a test.
#include "p2k_driver.h"
#include <cstdio>
#include <cstdlib>

extern "C" unsigned mediagx_get_reg(int regnum);
extern "C" unsigned mediagx_dasm(char *buffer, unsigned pc);

int main(int argc, char **argv)
{
	const char *romdir = (argc > 1) ? argv[1] : "roms";
	const char *prefix = (argc > 2) ? argv[2] : "rfm";
	u64 cycles = (argc > 3) ? strtoull(argv[3], nullptr, 0) : 1000000;

	p2k_state state;
	printf("loading %s ROMs from %s\n", prefix, romdir);
	if (!state.load_roms(romdir, prefix))
	{
		printf("ROM loading failed\n");
		return 1;
	}

	// optional: the update flash image built by tools/make_nvram_updates.py
	if (const char *nv = getenv("P2K_NVRAM_UPDATES"))
		printf("update flash %s: %s\n", nv, state.load_nvram_updates(nv) ? "loaded" : "FAILED");

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
