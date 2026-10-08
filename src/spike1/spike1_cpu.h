// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the game CPU
//
// Spike 1 runs its game as an ARMv5TE (ARM926EJ-S class) Linux user-mode program. The imported MAME
// ARM9 core executes it in user mode with the MMU off; its SWI goes to the system-call layer
// instead of the exception vector (see set_swi_handler() in mame/cpu/arm7/arm7.h)

#pragma once

#include "emu.h"
#include "arm7.h"

#include <functional>

class spike1_cpu_device : public arm9_cpu_device
{
public:
	// A guest thread's user-mode registers, for switching threads on the one CPU
	struct context
	{
		uint32_t r[16];
		uint32_t cpsr;
	};

	spike1_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// Puts the CPU in user mode with the given registers; call after reset
	void enter_user_mode(const context &ctx);
	void save_context(context &ctx) const;
	void load_context(const context &ctx);

	uint32_t reg(int n) const { return m_r[n]; }
	void set_reg(int n, uint32_t value) { m_r[n] = value; }
	uint32_t pc() const { return m_r[15]; }
	bool thumb() const;

	// Runs up to `cycles` cycles and returns how many were used
	int run(int cycles);
	// Ends the slice that is running at the next instruction boundary
	void end_slice() { m_icount = 0; }
	// Takes `cycles` from the slice that is running, e.g. for a system call that costs time
	void eat(int cycles) { m_icount -= cycles; }

	void set_swi(std::function<void (uint32_t)> handler) { set_swi_handler(std::move(handler)); }
};

DECLARE_DEVICE_TYPE(SPIKE1_CPU, spike1_cpu_device)
