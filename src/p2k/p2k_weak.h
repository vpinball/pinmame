// license:BSD-3-Clause

// P2K optional link-time symbols
//
// Two things this subsystem calls may or may not be in the link:
//
//   p2k_dcs_read / p2k_dcs_write    live in src/wpc/p2k.c, i.e. on PinMAME's side. The standalone
//                                   harnesses built by "make p2kboot" and "make p2ktest" link the
//                                   subsystem alone and do not have them.
//   remote_debug_breakpoint_hook    lives in src/remote_debug/, built only with REMOTE_DEBUG=1.
//                                   The subsystem is compiled without PinMAME's defines (see
//                                   P2K_CXXFLAGS in p2k.mak), so it cannot test for that itself.
//
// GCC and Clang answer both with a weak declaration: the symbol resolves to null when nothing
// provides it, and the call sites test the pointer. MSVC has no weak symbols, so each case is
// decided at compile time instead - which is possible because the Windows projects are not the
// ones with a missing symbol:
//
//   * every Windows build that compiles this subsystem also compiles src/wpc/p2k.c - the .props
//     and the CMake lists add the two together - so the DCS entry points are always there;
//   * no Windows project defines REMOTE_DEBUG, so the hook is not. If one ever does, defining
//     REMOTE_DEBUG for these files as well is all it takes

#pragma once

#include <ctime>

#ifdef _MSC_VER
  #define P2K_WEAK
  // A strong symbol's address is never null, and MSVC warns (C4551) about testing it
  #define P2K_HAVE_WEAK(sym) 1
#else
  #define P2K_WEAK __attribute__((weak))
  #define P2K_HAVE_WEAK(sym) ((sym) != nullptr)
#endif

// -rtc's fixed clock lives in src/timer.c, which the standalone harnesses do not link; there these
// resolve to null and the host clock is used.
extern "C" {
	int    rtc_is_fixed(void) P2K_WEAK;
	time_t rtc_now(void) P2K_WEAK;
	void   rtc_now_tm(struct tm *out) P2K_WEAK;
}

#define P2K_RTC_FIXED() (P2K_HAVE_WEAK(rtc_is_fixed) && rtc_is_fixed())
