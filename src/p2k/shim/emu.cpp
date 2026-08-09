// license:BSD-3-Clause

// PinMAME P2K subsystem - definitions for the shim's global state

#include "emu.h"

running_machine *device_t::s_machine = nullptr;

device_t *p2k_config_owner()
{
	extern machine_config *p2k_active_config;
	return p2k_active_config ? p2k_active_config->current_owner : nullptr;
}
machine_config *p2k_active_config = nullptr;

// set by the cpuintrf bridge when PinMAME's debugger is present
void (*p2k_instruction_hook)(unsigned pc) = nullptr;
void (*p2k_exception_hook)(int vector) = nullptr;
// XTAL::validate() lives in MAME's xtal.cpp, which needs the full emu core; the checks only
// warn about clock values not in MAME's known-crystal table.
void XTAL::validate(const char *message) const {}
void XTAL::validate(const std::string &message) const {}
