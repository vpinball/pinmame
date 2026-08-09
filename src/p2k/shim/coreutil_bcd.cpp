// license:BSD-3-Clause

// PinMAME P2K subsystem - the BCD helpers from MAME's coreutil
//
// MAME's coreutil.cpp also contains core_crc32(), which pulls in zlib. Nothing in the P2K
// subsystem needs it, so only the parts the RTC uses are provided here. The implementations
// match MAME's

#include "emucore.h"

int bcd_adjust(int value)
{
	if ((value & 0xf) >= 0xa)
		value = value + 0x10 - 0xa;
	if ((value & 0xf0) >= 0xa0)
		value = value - 0xa0 + 0x100;
	return value;
}

u32 dec_2_bcd(u32 a)
{
	u32 result = 0;
	int shift = 0;
	while (a != 0)
	{
		result |= (a % 10) << shift;
		a /= 10;
		shift += 4;
	}
	return result;
}

u32 bcd_2_dec(u32 a)
{
	u32 result = 0, scale = 1;
	while (a != 0)
	{
		result += (a & 0x0f) * scale;
		a >>= 4;
		scale *= 10;
	}
	return result;
}
