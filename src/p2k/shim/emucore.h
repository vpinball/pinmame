// license:BSD-3-Clause

// PinMAME P2K subsystem - stand-in for MAME's emucore.h
//
// Imported MAME headers (attotime.h, xtal.h, ...) include "emucore.h" for the basic types and
// helpers. This supplies just those, without pulling in MAME's core

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <type_traits>
#include <memory>
#include <functional>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <ctime>

// MAME's platform basics (byte-order helpers, PRI* macros)
#include "osdcomm.h"

using u8  = std::uint8_t;   using s8  = std::int8_t;
using u16 = std::uint16_t;  using s16 = std::int16_t;
using u32 = std::uint32_t;  using s32 = std::int32_t;
using u64 = std::uint64_t;  using s64 = std::int64_t;
using offs_t = u32;

#define ATTR_UNUSED
#define ATTR_PRINTF(x,y)
#if defined(__GNUC__) || defined(__clang__)
#define ATTR_CONST              __attribute__((const))
#define ATTR_FORCE_INLINE       __attribute__((always_inline))
#else
#define ATTR_CONST
#define ATTR_FORCE_INLINE       __forceinline
#endif
#define NOINLINE
#define ATTR_HOT
#define ATTR_COLD

template <typename T> constexpr T BIT(T x, unsigned n) { return (x >> n) & T(1); }
template <typename T> constexpr T BIT(T x, unsigned n, unsigned w) { return (x >> n) & ((T(1) << w) - 1); }

template <typename T> ATTR_FORCE_INLINE T make_bitmask(unsigned n) { return n ? (T(-1) >> (8 * sizeof(T) - n)) : 0; }

// MAME's zero-initialising array allocator
// called as make_unique_clear<u8[]>(n), like MAME's
template <typename T, typename E = std::remove_extent_t<T>>
ATTR_FORCE_INLINE std::unique_ptr<E[]> make_unique_clear(std::size_t n, u8 fill = 0)
{
	auto p = std::make_unique<E[]>(n);
	std::memset(p.get(), fill, n * sizeof(E));
	return p;
}

// MAME's checked downcast; unchecked here, as in a release build
template <class Dest, class Source> ATTR_FORCE_INLINE Dest downcast(Source *src) { return static_cast<Dest>(src); }
template <class Dest, class Source> ATTR_FORCE_INLINE Dest downcast(Source &src) { return static_cast<Dest>(src); }

// MAME throws this; here it aborts with the message, like fatalerror
class emu_fatalerror final
{
public:
	template <typename... T> emu_fatalerror(const char *fmt, T &&... args)
	{ fprintf(stderr, "fatal: "); fprintf(stderr, fmt, args...); fprintf(stderr, "\n"); abort(); }
};

#define fatalerror(...) do { fprintf(stderr, __VA_ARGS__); abort(); } while (0)
#define popmessage(...) do {} while (0)

// save-state type registration - the shim does not implement save states
#define ALLOW_SAVE_TYPE(TYPE)
#define ALLOW_SAVE_TYPE_AND_ARRAY(TYPE)

namespace util {
template <typename... T> inline std::string string_format(const char *fmt, T &&... args)
{
	char buf[1024]; snprintf(buf, sizeof(buf), fmt, args...); return std::string(buf);
}
template <typename... T> inline std::string string_format(const std::string &fmt, T &&... args)
{
	return string_format(fmt.c_str(), std::forward<T>(args)...);
}
template <typename... T> inline void stream_format(std::ostream &os, const char *fmt, T &&... args)
{
	char buf[1024]; snprintf(buf, sizeof(buf), fmt, args...); os << buf;
}
}
using util::string_format;
