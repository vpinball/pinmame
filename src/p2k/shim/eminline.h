// license:BSD-3-Clause

// PinMAME P2K subsystem - the handful of MAME inline math helpers that attotime.cpp uses

#pragma once

#include "emucore.h"

#if defined(_MSC_VER)
 #include <intrin.h>
#endif

static ATTR_FORCE_INLINE s64 mul_32x32(s32 a, s32 b)   { return s64(a) * s64(b); }
static ATTR_FORCE_INLINE u64 mulu_32x32(u32 a, u32 b)  { return u64(a) * u64(b); }
static ATTR_FORCE_INLINE s32 mul_32x32_hi(s32 a, s32 b)  { return s32((s64(a) * s64(b)) >> 32); }
static ATTR_FORCE_INLINE u32 mulu_32x32_hi(u32 a, u32 b) { return u32((u64(a) * u64(b)) >> 32); }

static ATTR_FORCE_INLINE s32 div_64x32(s64 a, s32 b)  { return s32(a / b); }
static ATTR_FORCE_INLINE u32 divu_64x32(u64 a, u32 b) { return u32(a / b); }

static ATTR_FORCE_INLINE s32 div_64x32_rem(s64 a, s32 b, s32 &remainder)
{ s32 res = s32(a / b); remainder = s32(a - (s64(res) * b)); return res; }

static ATTR_FORCE_INLINE u32 divu_64x32_rem(u64 a, u32 b, u32 &remainder)
{ u32 res = u32(a / b); remainder = u32(a - (u64(res) * b)); return res; }

static ATTR_FORCE_INLINE s32 div_32x32_shift(s32 a, s32 b, u8 shift) { return s32((s64(a) << shift) / s64(b)); }
static ATTR_FORCE_INLINE u32 divu_32x32_shift(u32 a, u32 b, u8 shift) { return u32((u64(a) << shift) / u64(b)); }

static ATTR_FORCE_INLINE s64 mod_64x32(s64 a, s32 b)  { return a - (s64(a / b) * b); }
static ATTR_FORCE_INLINE u64 modu_64x32(u64 a, u32 b) { return a - (u64(a / b) * b); }

static ATTR_FORCE_INLINE u8 count_leading_zeros_32(u32 v)
{
#if defined(_MSC_VER)
	unsigned long i;
	return _BitScanReverse(&i, v) ? u8(31 - i) : u8(32);
#elif defined(__GNUC__) || defined(__clang__)
	return v ? u8(__builtin_clz(v)) : u8(32);
#else //!! could unroll the following
	u8 n = 0; if (!v) return 32; while (!(v & 0x80000000u)) { v <<= 1; n++; } return n;
#endif
}
static ATTR_FORCE_INLINE u8 count_leading_ones_32(u32 v) { return count_leading_zeros_32(~v); }

// used by the Spike 1 arm7 import (src/spike1/mame/cpu/arm7)
static ATTR_FORCE_INLINE s32 mul_32x32_shift(s32 a, s32 b, u8 shift) { return s32((s64(a) * s64(b)) >> shift); }
static ATTR_FORCE_INLINE u32 rotr_32(u32 val, int shift)
{
#if defined(_MSC_VER)
	return _rotr(val, shift); // one ror: MSVC does not recognize the C rotate idioms
#else
	return (val >> (shift & 31)) | (val << (-shift & 31)); // gcc/clang: one ror
#endif
}
static ATTR_FORCE_INLINE unsigned population_count_32(u32 val)
{
#if (defined(__GNUC__) || defined(__clang__)) && (defined(__POPCNT__) || defined(__aarch64__))
	return unsigned(__builtin_popcount(val));
#else // POPCNT is not in the SSE2 baseline these builds target (as popcount16 in src/cpu/arm7/arm7core.c)
	val -= (val >> 1) & 0x55555555u;
	val = (val & 0x33333333u) + ((val >> 2) & 0x33333333u);
	val = (val + (val >> 4)) & 0x0f0f0f0fu;
	return (val * 0x01010101u) >> 24;
#endif
}
