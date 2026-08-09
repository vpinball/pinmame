// license:BSD-3-Clause

// PinMAME P2K subsystem - stub for MAME's PC keyboard device. See pckeybrd.h

#include "machine/pckeybrd.h"

DEFINE_DEVICE_TYPE(AT_KEYB, at_keyboard_device, "at_keyb", "PC/AT Keyboard (stub)")

at_keyboard_device::at_keyboard_device(const machine_config &mconfig, const char *tag,
		device_t *owner, u32 clock)
	: pc_keyboard_device(mconfig, AT_KEYB, tag, owner, clock)
{
}
