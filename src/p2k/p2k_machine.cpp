// license:BSD-3-Clause

// PinMAME P2K subsystem - machine assembly and run loop. See p2k_machine.h

#include "p2k_machine.h"

extern machine_config *p2k_active_config;

p2k_machine::p2k_machine()
{
	device_t::s_machine = &m_machine;
	p2k_active_config = &m_config;
}

p2k_machine::~p2k_machine()
{
	if (p2k_active_config == &m_config) p2k_active_config = nullptr;
	if (device_t::s_machine == &m_machine) device_t::s_machine = nullptr;
}

void p2k_machine::start()
{
	// resolve first: devices look up sub-devices and callbacks before anyone starts
	for (auto &dev : m_config.devices) dev->p2k_resolve();
	for (auto &dev : m_config.devices) dev->p2k_start();
}

void p2k_machine::reset()
{
	for (auto &dev : m_config.devices) dev->p2k_reset();
}

// interleaving granularity: the CPU never runs longer than 1/QUANTUM_HZ of a second at a time
static constexpr u32 QUANTUM_HZ = 10000;   // 100 us

void p2k_machine::run_cycles(cpu_device &cpu, u32 cpu_hz, u64 cycles)
{
	u64 remaining = cycles;
	while (remaining > 0)
	{
		// run no further than the next timer expiry, so timer callbacks land on time
		attotime now = m_machine.time();
		attotime next = m_machine.scheduler().next_expiry();
		// never hand the CPU an unbounded slice: without a quantum a device that is only
		// programmed during the slice could not get its timer serviced until after it
		u64 slice = remaining;
		u64 quantum = cpu_hz / QUANTUM_HZ;
		if (quantum < 1) quantum = 1;
		if (slice > quantum) slice = quantum;
		if (!next.is_never() && next > now)
		{
			u64 until = (next - now).as_ticks(cpu_hz);
			if (until < slice) slice = until ? until : 1;
		}

		int used = cpu.run(int(slice));
		if (used <= 0) used = int(slice);       // a halted core still consumes its slice
		remaining -= (u64(used) > remaining) ? remaining : u64(used);

		m_machine.scheduler().advance_to(now + attotime::from_ticks(used, cpu_hz));
	}
}
