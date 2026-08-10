// license:BSD-3-Clause

// PinMAME P2K subsystem - the handful of definitions shared with PinMAME's half of the build
//
// The subsystem deliberately includes no PinMAME headers: those belong to the other half, built
// with different settings (see cmake/p2k.cmake). This header is the one crossing in the other
// direction - owned by the subsystem, included by src/wpc/p2k.c

#pragma once

// Capacity of the frame buffer the two halves pass between them: src/wpc/p2k.c sizes its
// destination with it, p2k_pinmame_frame()/frame_rgb() take it as the 'capacity' argument and
// refuse any geometry that would not fit. 1024x768 is the most a MediaGX outputs at 15/16/32bit color
#define P2K_MAX_WIDTH  1024
#define P2K_MAX_HEIGHT 768
#define P2K_MAX_PIXELS (P2K_MAX_WIDTH * P2K_MAX_HEIGHT)

// The battery-backed blocks handed across p2k_pinmame_nvram_set()/_get(): src/wpc/p2k.c sizes the
// arrays it saves through core_nvram() with these, and the subsystem allocates the blocks behind
// nvram_block_ptr() with the same. Both sides must agree, and the API cannot tell you when they
// do not - it clamps to the smaller of the two, so a divergence silently truncates a player's saved settings rather than failing
#define P2K_NV_CMOS_SIZE   0x30000
#define P2K_NV_EEPROM_SIZE 0x80

// The 'which' argument of the same two calls. p2k_state::nvram_block mirrors these; the
// static_asserts in p2k_driver.cpp keep the two spellings from drifting. The subsystem has a third
// block (NVRAM_UPDATES) that PinMAME does not save, so it has no name here
#define P2K_NV_BLOCK_CMOS   0
#define P2K_NV_BLOCK_EEPROM 1

// The display controller hands out 240 lines and the monitor shows 480: the picture is
// line-doubled on the way to the CRT, which is why the machine's pixels are twice as tall as
// they are wide. "Source is 640x240 RGB555 ... Output is 640x480 ARGB8888, line-doubled and Y-flipped".
// Doubling on PinMAME's side rather than in the subsystem keeps the machine's own frame_rgb()
// honest about what its framebuffer holds, and gives the doubled picture to everything
// downstream at once - the window, libpinmame, and any frontend behind it
#define P2K_LINE_DOUBLE 2

// How the second line of each pair is interpolated
//
//  0 - line doubling: each of the machine's 240 lines written out twice
//  1 - interpolation: the odd output lines become the average of the machine's line above and the
//      one below. The even lines stay exact copies, so no original scanline is blurred, only the invented line is.
//      The bottom line of the frame is doubled as before.
//
// use -DP2K_LINE_INTERPOLATE=1 or by changing it here
#ifndef P2K_LINE_INTERPOLATE
#define P2K_LINE_INTERPOLATE 0
#endif

#if P2K_LINE_INTERPOLATE && (P2K_LINE_DOUBLE != 2)
#error "P2K_LINE_INTERPOLATE lerps a new line between two real ones and needs P2K_LINE_DOUBLE == 2"
#endif
