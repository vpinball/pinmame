// license:BSD-3-Clause

// PinMAME P2K subsystem - machine assembly and run loop. See p2k_machine.h

#include "p2k_machine.h"
#include "p2k_debug.h"

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

// An optional extra cap on how long the CPU may run before the device timers are looked at again:
// 1/QUANTUM_HZ of a second. Zero, the default, means no cap - next_expiry() below is then the only
// bound, which is the one that matters.
//
// Not MAME's quantum, despite the name. MAME's interleaves several CPUs against each other, and
// there is only one in here - the DCS sound CPU is PinMAME's and PinMAME interleaves it, in the
// ~25890-cycle chunks it calls mediagx_execute with. This one interleaves nothing. It was a
// stand-in for a mechanism this shim never ported: MAME aborts the running timeslice when a timer
// is set to expire inside it, so "a device was programmed mid-slice" is handled exactly. A fixed
// cap only approximates that, and the approximation was never fine enough to be worth its cost.
//
// It used to default to 10000, a 7766-cycle cap, and that bought little over next_expiry() alone:
// once the PIT runs, its period already bounds a slice at ~19406 cycles, so the cap only tightened
// 19406 to 7766 at roughly twice the scheduler passes. The one case it uniquely covered is early
// boot, before any timer exists, where the exposure without it is one PinMAME chunk.
//
// It could never do the thing that mattered, either. This cap is also the worst-case latency of a
// device change reaching the CPU, and the guest needs that under about 400 cycles - so 7766 was
// never going to approximate "now". pics_settle() in p2k_driver.cpp does that properly.
//
// The knob stays because it is the instrument that found it: setting it sweeps the latency
// directly, and nothing else here turns a latency bug into a number. The sweep is in README.md.
//
// A debug build's, like every other knob here - without P2K_DEBUG this is a compile-time zero, so
// the cap cannot be switched on by a stray environment variable in a shipped build, and the whole
// block below folds away rather than costing the loop a load and a branch per slice
#if P2K_DEBUG
static const u32 QUANTUM_HZ = []() -> u32 {
	if (const char *s = getenv("P2K_QUANTUM_HZ")) return u32(strtoul(s, nullptr, 0));
	return 0;   // no cap
}();
#else
static constexpr u32 QUANTUM_HZ = 0;
#endif

void p2k_machine::run_cycles(cpu_device &cpu, u32 cpu_hz, u64 cycles)
{
	// Hoisted: both operands are loop-invariant - cpu_hz is the caller's and QUANTUM_HZ is fixed at
	// load - and a 64-bit divide per iteration is not free. 0 here means no cap, which is the
	// default, and then the loop pays one register test
	u64 quantum = 0;
	if (QUANTUM_HZ)
	{
		quantum = cpu_hz / QUANTUM_HZ;
		if (quantum < 1) quantum = 1;
	}

	u64 remaining = cycles;
	while (remaining > 0)
	{
		// run no further than the next timer expiry, so timer callbacks land on time
		attotime now = m_machine.time();
		attotime next = m_machine.scheduler().next_expiry();
		u64 slice = remaining;
		if (quantum && slice > quantum) slice = quantum;
		if (!next.is_never() && next > now)
		{
			u64 until = (next - now).as_ticks(cpu_hz);
			if (until < slice) slice = until ? until : 1;
		}

		int used = cpu.run(int(slice));
		if (used <= 0) used = int(slice); // a halted core still consumes its slice
		remaining -= (u64(used) > remaining) ? remaining : u64(used);

		m_machine.scheduler().advance_to(now + attotime::from_ticks(used, cpu_hz));
	}
}
