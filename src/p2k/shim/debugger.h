// license:BSD-3-Clause

// PinMAME P2K subsystem - debugger hooks for the imported MAME code
//
// MAME's CPU cores call debugger_instruction_hook() once per instruction. That is exactly where
// PinMAME wants to check breakpoints, so the macro forwards to a function pointer the bridge
// fills in (see p2k_cpuintrf.cpp).
//
// The null test is not a "no debugger attached" fast path - it is the arming mechanism, and it is
// load-bearing for speed. This macro sits in the i386 core's execute loop, so whatever it expands
// to is paid tens of millions of times a second; the bridge leaves the pointer null whenever the
// hook has nothing to do. See arm_instruction_hook() in p2k_cpuintrf.cpp for when it is armed and why that is safe

#pragma once

extern void (*p2k_instruction_hook)(unsigned pc);
extern void (*p2k_exception_hook)(int vector);

#define debugger_instruction_hook(pc) \
	do { if (p2k_instruction_hook) p2k_instruction_hook((unsigned)(pc)); } while (0)
// the i386 core calls this from i386_trap() with the vector it is about to dispatch
#define debugger_exception_hook(e) \
	do { if (p2k_exception_hook) p2k_exception_hook((int)(e)); } while (0)
#define debugger_privilege_hook()     do { } while (0)
