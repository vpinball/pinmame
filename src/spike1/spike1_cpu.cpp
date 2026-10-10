// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the game CPU. See spike1_cpu.h

#include "spike1_cpu.h"
#include "arm7core.h"

DEFINE_DEVICE_TYPE(SPIKE1_CPU, spike1_cpu_device, "spike1_cpu", "Spike 1 ARM926 (user mode)")

spike1_cpu_device::spike1_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: arm9_cpu_device(mconfig, SPIKE1_CPU, tag, owner, clock, 5, ARCHFLAG_T | ARCHFLAG_E, ENDIANNESS_LITTLE)
{
}

void spike1_cpu_device::enter_user_mode(const context &ctx)
{
	SwitchMode(eARM7_MODE_USER);
	load_context(ctx);
}

// In user mode the register table maps r0-r15 straight onto m_r[eR0..eR15]
void spike1_cpu_device::save_context(context &ctx) const
{
	for (int i = 0; i < 16; i++) ctx.r[i] = m_r[eR0 + i];
	ctx.cpsr = m_r[eCPSR];
}

void spike1_cpu_device::load_context(const context &ctx)
{
	for (int i = 0; i < 16; i++) m_r[eR0 + i] = ctx.r[i];
	// only the flags and the Thumb bit belong to the thread; the mode stays user, interrupts stay off
	m_r[eCPSR] = (ctx.cpsr & (N_MASK | Z_MASK | C_MASK | V_MASK | Q_MASK | T_MASK)) | I_MASK | F_MASK | 0x10;
}

bool spike1_cpu_device::thumb() const
{
	return (m_r[eCPSR] & T_MASK) != 0;
}

int spike1_cpu_device::run(int cycles)
{
	m_icount = cycles;
	execute_run();
	return cycles - m_icount;
}
