// license:BSD-3-Clause

// PinMAME P2K subsystem - self-tests for the imported MAME code
//
// Two checks, both independent of PinMAME state:
//   1. CPU: the MediaGX core boots at its reset vector and executes real-mode code.
//   2. Machine: 8254 channel 0 drives 8259 IR0, the CPU takes the interrupt, runs the handler
//      and acknowledges it - i.e. timers, callbacks and interrupt routing all work

#include "emu.h"
#include "i386.h"
#include "i386priv.h"   // register indices for the state interface
#include "p2k_machine.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include <cstdio>

namespace {

u8 g_ram[0x110000]; // 1 MB + the top-of-memory alias
bool g_io_hit = false;
u8 g_io_val = 0;

pic8259_device *g_pic = nullptr;
pit8253_device *g_pit = nullptr;

offs_t map_addr(offs_t a)
{
	if (a >= 0xffff0000) return 0x100000 + (a & 0xffff); // top-of-memory alias (BIOS)
	return a & 0xfffff;
}

u32 mem_r(void *, offs_t a, u32 mask)
{
	offs_t o = map_addr(a);
	u32 v = g_ram[o] | (g_ram[o+1] << 8) | (g_ram[o+2] << 16) | (u32(g_ram[o+3]) << 24);
	return v & mask;
}
void mem_w(void *, offs_t a, u32 d, u32 mask)
{
	offs_t o = map_addr(a);
	u32 v = g_ram[o] | (g_ram[o+1] << 8) | (g_ram[o+2] << 16) | (u32(g_ram[o+3]) << 24);
	v = (v & ~mask) | (d & mask);
	g_ram[o] = u8(v); g_ram[o+1] = u8(v >> 8); g_ram[o+2] = u8(v >> 16); g_ram[o+3] = u8(v >> 24);
}

// split a dword access into the byte ports it actually touches
template <typename F> u32 io_read_lanes(offs_t a, u32 mask, F port_r)
{
	u32 result = 0;
	for (unsigned lane = 0; lane < 4; lane++)
		if (mask & (0xffu << (lane * 8)))
			result |= u32(port_r(a + lane)) << (lane * 8);
	return result;
}
template <typename F> void io_write_lanes(offs_t a, u32 d, u32 mask, F port_w)
{
	for (unsigned lane = 0; lane < 4; lane++)
		if (mask & (0xffu << (lane * 8)))
			port_w(a + lane, u8(d >> (lane * 8)));
}

// --- test 1: bare CPU, no peripherals ---------------------------------------------------

u32 io_r_bare(void *, offs_t, u32 mask) { return 0xffffffff & mask; }
void io_w_bare(void *, offs_t a, u32 d, u32 mask)
{
	io_write_lanes(a, d, mask, [](offs_t port, u8 val)
	{
		printf("  [io] out 0x%04x, 0x%02x\n", port, val);
		if (port == 0x80) { g_io_hit = true; g_io_val = val; }
	});
}

// --- test 2: PC-style I/O dispatch to the 8259 and 8254 ---------------------------------

u8 port_r_pc(offs_t a)
{
	if (a >= 0x20 && a <= 0x21) return g_pic->read(a & 1);
	if (a >= 0x40 && a <= 0x43) return g_pit->read(a & 3);
	return 0xff;
}
void port_w_pc(offs_t a, u8 d)
{
	if (a >= 0x20 && a <= 0x21) { g_pic->write(a & 1, d); return; }
	if (a >= 0x40 && a <= 0x43) { g_pit->write(a & 3, d); return; }
	if (a == 0x80) { g_io_hit = true; g_io_val = d; }
}
u32 io_r_pc(void *, offs_t a, u32 mask) { return io_read_lanes(a, mask, port_r_pc); }
void io_w_pc(void *, offs_t a, u32 d, u32 mask) { io_write_lanes(a, d, mask, port_w_pc); }

} // anonymous namespace

// ---------------------------------------------------------------- test 1
extern "C" int p2k_cpu_selftest()
{
	memset(g_ram, 0, sizeof(g_ram));
	g_io_hit = false;

	p2k_machine mach;
	p2k_bus_callbacks mem_cb { mem_r, mem_w, nullptr };
	p2k_bus_callbacks io_cb  { io_r_bare, io_w_bare, nullptr };
	address_space program(mem_cb), io(io_cb);

	mediagx_device &cpu = mach.add(MEDIAGX, "mediagx", 20000000u);
	cpu.p2k_set_space(AS_PROGRAM, &program);
	cpu.p2k_set_space(AS_IO, &io);

	// real-mode code at the reset vector: mov al,0x42 / out 0x80,al / jmp $
	const u8 code[] = { 0xb0, 0x42, 0xe6, 0x80, 0xeb, 0xfe };
	memcpy(&g_ram[0x100000 + 0xfff0], code, sizeof(code));

	mach.start();
	mach.reset();
	printf("test 1: CPU at reset vector, 200 cycles\n");
	mach.run_cycles(cpu, 20000000u, 200);

	bool ok = g_io_hit && g_io_val == 0x42;
	printf("  -> I/O write %s\n", ok ? "seen, value 0x42" : "MISSING or wrong");

	// the register the code loaded must be readable through the state interface - this is the
	// path a debugger (and PinMAME's cpuintrf bridge) uses
	u64 eax = cpu.state_int(I386_EAX);
	u64 eip = cpu.state_int(I386_EIP);
	bool state_ok = (eax & 0xff) == 0x42;
	printf("  -> registers via state interface: EAX=%08x EIP=%08x %s\n",
		u32(eax), u32(eip), state_ok ? "(AL as executed)" : "(WRONG)");

	// and writing back through the same path must stick
	cpu.set_state_int(I386_EAX, 0x12345678);
	bool write_ok = u32(cpu.state_int(I386_EAX)) == 0x12345678;
	printf("  -> register write-back %s\n", write_ok ? "ok" : "FAILED");

	return (ok && state_ok && write_ok) ? 0 : 1;
}

// ---------------------------------------------------------------- test 2
extern "C" int p2k_machine_selftest()
{
	memset(g_ram, 0, sizeof(g_ram));
	g_io_hit = false;

	p2k_machine mach;
	p2k_bus_callbacks mem_cb { mem_r, mem_w, nullptr };
	p2k_bus_callbacks io_cb  { io_r_pc, io_w_pc, nullptr };
	address_space program(mem_cb), io(io_cb);

	mediagx_device &cpu = mach.add(MEDIAGX, "mediagx", 20000000u);
	pic8259_device &pic = mach.add(PIC8259, "pic", 0u);
	pit8254_device &pit = mach.add(PIT8254, "pit", 0u);
	g_pic = &pic;
	g_pit = &pit;

	cpu.p2k_set_space(AS_PROGRAM, &program);
	cpu.p2k_set_space(AS_IO, &io);

	// 8254 channel 0 runs at the PC rate and drives the master 8259's IR0
	pit.set_clk<0>(1193182.0);
	pit.out_handler<0>().set([&pic](int state) { pic.ir0_w(state); });

	// the 8259's INT output drives the CPU's interrupt line; the CPU asks it for the vector
	pic.out_int_callback().set([&cpu](int state) { cpu.set_input_line(INPUT_LINE_IRQ0, state); });
	cpu.set_irq_acknowledge_callback([&pic](int) { return int(pic.acknowledge()); });

	// Set up the IVT, program the 8259 and the 8254, enable interrupts, then idle.
	// Assembled with GNU as (.code16); the mnemonics are kept alongside the bytes.
	const u8 setup[] = {
		0xfa,                                     // cli
		0x31, 0xc0,                               // xor ax,ax
		0x8e, 0xd8,                               // mov ds,ax
		0xc7, 0x06, 0x20, 0x00, 0x00, 0x06,       // movw $0x0600, 0x20   IVT vec 8 offset
		0xc7, 0x06, 0x22, 0x00, 0x00, 0x00,       // movw $0x0000, 0x22   IVT vec 8 segment
		0xb0, 0x11, 0xe6, 0x20,                   // mov al,0x11 ; out 0x20,al   ICW1
		0xb0, 0x08, 0xe6, 0x21,                   // mov al,0x08 ; out 0x21,al   ICW2 base 0x08
		0xb0, 0x00, 0xe6, 0x21,                   // mov al,0x00 ; out 0x21,al   ICW3
		0xb0, 0x01, 0xe6, 0x21,                   // mov al,0x01 ; out 0x21,al   ICW4 8086 mode
		0xb0, 0xfe, 0xe6, 0x21,                   // mov al,0xfe ; out 0x21,al   unmask IR0
		0xb0, 0x36, 0xe6, 0x43,                   // mov al,0x36 ; out 0x43,al   ch0 mode 3
		0xb0, 0x64, 0xe6, 0x40,                   // mov al,0x64 ; out 0x40,al   count lo = 100
		0xb0, 0x00, 0xe6, 0x40,                   // mov al,0x00 ; out 0x40,al   count hi
		0xfb,                                     // sti
		0x90, 0xeb, 0xfd,                         // idle: nop ; jmp idle
	};
	const u8 isr[] = {
		0xb0, 0x99, 0xe6, 0x80,                   // mov al,0x99 ; out 0x80,al   signal
		0xb0, 0x20, 0xe6, 0x20,                   // mov al,0x20 ; out 0x20,al   EOI
		0xcf,                                     // iret
	};
	memcpy(&g_ram[0x500], setup, sizeof(setup));
	memcpy(&g_ram[0x600], isr, sizeof(isr));
	const u8 entry[] = { 0xea, 0x00, 0x05, 0x00, 0x00 };   // jmp far 0000:0500
	memcpy(&g_ram[0x100000 + 0xfff0], entry, sizeof(entry));

	mach.start();
	mach.reset();

	printf("test 2: 8254 ch0 -> 8259 IR0 -> CPU interrupt\n");
	mach.run_cycles(cpu, 20000000u, 2000000);      // 100 ms of emulated time

	bool ok = g_io_hit && g_io_val == 0x99;
	printf("  -> emulated time %.3f ms, interrupt handler %s\n",
		mach.machine().time().as_double() * 1000.0, ok ? "ran" : "DID NOT RUN");

	g_pic = nullptr; g_pit = nullptr;
	return ok ? 0 : 1;
}

#ifdef P2K_SELFTEST_MAIN
int main()
{
	int fails = 0;
	fails += p2k_cpu_selftest();
	fails += p2k_machine_selftest();
	printf("\n%s\n", fails ? "SELFTEST FAILED" : "selftest ok");
	return fails ? 1 : 0;
}
#endif
