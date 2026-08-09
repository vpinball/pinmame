// license:BSD-3-Clause

// PinMAME P2K subsystem - stand-in for MAME's PC keyboard device
//
// The 8042 keyboard controller requires an at_keyboard_device, but MAME's implementation pulls
// in the input port and natural-keyboard subsystems. Pinball 2000 reads its cabinet switches
// through the pinball driver board, not through the keyboard, so this device never produces a
// scancode.
//
// It does answer commands, though, and it has to: the game's 8042 initialisation writes 0xFF
// (keyboard reset) to port 0x60 and then spins on the status port waiting for the output buffer
// to fill. A keyboard that stays silent hangs the machine there. The replies below are the ones
// MAME's at_keyboard_device gives (src/devices/machine/pckeybrd.cpp, MAME 0.239); the 8042 picks
// them up by polling read()

#pragma once

#include "emu.h"

class pc_keyboard_device : public device_t
{
public:
	enum class KEYBOARD_TYPE { PC, MF2, AT_84, AT };

	pc_keyboard_device(const machine_config &mconfig, device_type type, const char *tag,
			device_t *owner, u32 clock)
		: device_t(mconfig, type, tag, owner, clock)
		, m_out_keypress_func(*this)
	{}

	auto keypress() { return m_out_keypress_func.bind(); }

	// the controller polls this; 0 means "no data"
	u8 read()
	{
		if (m_head == m_tail) return 0;
		const u8 data = m_queue[m_head];
		m_head = (m_head + 1) % QUEUE_SIZE;
		return data;
	}

	void write(u8 data)
	{
		if (m_expect_parameter)       // second byte of 0xED / 0xF0 / 0xF3
		{
			m_expect_parameter = false;
			queue(0xfa);
			return;
		}

		switch (data)
		{
			case 0xed:  case 0xf0:  case 0xf3:                 // LEDs / scan code set / rates
				m_expect_parameter = true;
				queue(0xfa);
				break;
			case 0xee:                                          // echo, not acknowledged
				queue(0xee);
				break;
			case 0xf2:                                          // identify
				queue(0xfa); queue(0xab); queue(0x83);
				break;
			case 0xf4:  case 0xf5:  case 0xf6:                  // enable / disable / defaults
				m_head = m_tail = 0;
				queue(0xfa);
				break;
			case 0xfe:                                          // resend
				queue(0xfa);
				break;
			case 0xff:                                          // reset: acknowledge, then BAT ok
				m_head = m_tail = 0;
				m_expect_parameter = false;
				queue(0xfa); queue(0xaa);
				break;
			default:
				queue(0xfa);
				break;
		}
	}

	void set_type(KEYBOARD_TYPE type, int default_set) {}

protected:
	devcb_write_line m_out_keypress_func;

private:
	static constexpr unsigned QUEUE_SIZE = 8;

	void queue(u8 data)
	{
		const unsigned next = (m_tail + 1) % QUEUE_SIZE;
		if (next == m_head) return;      // full: drop, as a real keyboard would
		m_queue[m_tail] = data;
		m_tail = next;
	}

	u8 m_queue[QUEUE_SIZE] = {};
	unsigned m_head = 0, m_tail = 0;
	bool m_expect_parameter = false;
};

class at_keyboard_device final : public pc_keyboard_device
{
public:
	at_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock = 0);
	at_keyboard_device(const machine_config &mconfig, const char *tag, device_t *owner,
			KEYBOARD_TYPE type, int default_set)
		: at_keyboard_device(mconfig, tag, owner, 0)
	{ set_type(type, default_set); }
};

DECLARE_DEVICE_TYPE(AT_KEYB, at_keyboard_device)
