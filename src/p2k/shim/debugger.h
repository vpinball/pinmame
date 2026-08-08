// PinMAME P2K subsystem - debugger hooks for the imported MAME code.
//
// MAME's CPU cores call debugger_instruction_hook() once per instruction. That is exactly where
// PinMAME wants to check breakpoints, so the macro forwards to a function pointer the bridge
// fills in (see p2k_cpuintrf.cpp). Nothing is called when no debugger is attached.
#pragma once

extern void (*p2k_instruction_hook)(unsigned pc);
extern void (*p2k_exception_hook)(int vector);

#define debugger_instruction_hook(pc) \
	do { if (p2k_instruction_hook) p2k_instruction_hook((unsigned)(pc)); } while (0)
// the i386 core calls this from i386_trap() with the vector it is about to dispatch
#define debugger_exception_hook(e) \
	do { if (p2k_exception_hook) p2k_exception_hook((int)(e)); } while (0)
#define debugger_privilege_hook()     do { } while (0)
