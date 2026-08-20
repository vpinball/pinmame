// license:BSD-3-Clause

// PinMAME P2K subsystem - debugger hooks for the imported MAME code
//
// MAME's CPU cores call debugger_instruction_hook() once per instruction and
// debugger_exception_hook() from i386_trap(). With P2K_DEBUG they forward to function pointers the
// bridge fills in (see p2k_cpuintrf.cpp); without it they expand to nothing, which matters because
// the instruction one sits in the hot path (i386 execute loop).
//
// Removing them is safe because g_clkint_gate is `constexpr false` without P2K_DEBUG: the frame
// tracking they drive never starts, and their counters are read only by report_progress(), an
// empty inline in that build. p2k_cpuintrf.cpp static_asserts on the gate so this cannot rot

#pragma once

#include "../p2k_debug.h"

#if P2K_DEBUG

extern void (*p2k_instruction_hook)(unsigned pc);
extern void (*p2k_exception_hook)(int vector);

// The null test is the arming mechanism, not a "no debugger attached" fast path - the bridge
// leaves the pointer null whenever the hook has nothing to do. See arm_instruction_hook()
#define debugger_instruction_hook(pc) \
	do { if (p2k_instruction_hook) p2k_instruction_hook((unsigned)(pc)); } while (0)
// the i386 core calls this from i386_trap() with the vector it is about to dispatch
#define debugger_exception_hook(e) \
	do { if (p2k_exception_hook) p2k_exception_hook((int)(e)); } while (0)

#else

#define debugger_instruction_hook(pc) ((void)0)
#define debugger_exception_hook(e)    ((void)0)

#endif // P2K_DEBUG

#define debugger_privilege_hook()     ((void)0)
