// license:BSD-3-Clause

// PinMAME P2K subsystem - machine assembly and run loop.
//
// MAME builds a machine from a machine_config and lets its scheduler drive everything. Here the
// machine is assembled explicitly and driven by this small runner: the CPU executes a slice of
// cycles, then emulated time advances by the same amount and any timers that came due fire.
// This is what PinMAME's own timer/execution loop calls into

#pragma once

#include "emu.h"

class p2k_machine final
{
public:
	p2k_machine();
	~p2k_machine();

	// create a device and hang it under the machine root
	template <class DeviceClass, typename... Params>
	DeviceClass &add(const device_type_impl<DeviceClass> &type, const char *tag, Params &&... args)
	{
		return type(m_config, tag, std::forward<Params>(args)...);
	}

	// resolve callbacks and start every device, then reset them
	void start();
	void reset();

	// run the CPU for `cycles` at `cpu_hz`, interleaved with the timer queue
	void run_cycles(class cpu_device &cpu, u32 cpu_hz, u64 cycles);

	running_machine &machine() { return m_machine; }
	machine_config &config() { return m_config; }

private:
	running_machine m_machine;
	machine_config m_config;
};
