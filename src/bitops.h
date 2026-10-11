// license:BSD-3-Clause

/*********************************************************************

	bitops.h

	Bit rotations, byte swaps, bit reversals and bit counts, on the
	compiler's intrinsics where there are any.
	Standalone, so that the self-contained CPU cores can use it too.

*********************************************************************/

#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#ifdef _M_ARM64
#include <arm64intr.h>
#endif
#elif (!defined(__GNUC__) || (__GNUC__ > 3) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
#include <x86intrin.h>
#endif

#if defined(_MSC_VER)
#define BITOPS_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define BITOPS_INLINE static inline __attribute__((always_inline))
#else
#define BITOPS_INLINE static inline
#endif

/***************************************************************************

	Useful functions to deal with bit rotations/byte swaps/bit reversals

***************************************************************************/

// cyclic left shift, aka <<<
BITOPS_INLINE unsigned long long rotl_64(const unsigned long long x, const unsigned int count)
{
#ifdef _MSC_VER
    return _rotl64(x, count);
#elif !defined(__arm__) && !defined(__aarch64__) && (defined(__INTEL_COMPILER) || (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__))
    return __rolq(x, count);
#else
    return (x<<count) | (x>>( (unsigned int)(-(int)count)&63 )); // -count&63 instead of 64-count to handle count==0
#endif
}

BITOPS_INLINE unsigned int rotl_32(const unsigned int x, const unsigned int count)
{
    //assert(count < 32);
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    return _rotl(x, count);
#else
    return (x<<count) | (x>>( (unsigned int)(-(int)count)&31 )); // -count&31 instead of 32-count to handle count==0
#endif
}

// cyclic right shift, aka >>>
BITOPS_INLINE unsigned long long rotr_64(const unsigned long long x, const unsigned int count)
{
#ifdef _MSC_VER
    return _rotr64(x, count);
#elif !defined(__arm__) && !defined(__aarch64__) && (defined(__INTEL_COMPILER) || (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__))
    return __rorq(x, count);
#else
    return (x>>count) | (x<<( (unsigned int)(-(int)count)&63 )); // -count&63 instead of 64-count to handle count==0
#endif
}

BITOPS_INLINE unsigned int rotr_32(const unsigned int x, const unsigned int count)
{
    //assert(count < 32);
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    return _rotr(x, count);
#else
    return (x>>count) | (x<<( (unsigned int)(-(int)count)&31 )); // -count&31 instead of 32-count to handle count==0
#endif
}

//

BITOPS_INLINE unsigned long long swap_byteorder_64(unsigned long long x)
{
#if (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__)
    return __builtin_bswap64(x);
#elif defined(_MSC_VER)
    return _byteswap_uint64(x);
#else
    x = ((x <<  8) & 0xFF00FF00FF00FF00ull) | ((x >>  8) & 0x00FF00FF00FF00FFull);
    x = ((x << 16) & 0xFFFF0000FFFF0000ull) | ((x >> 16) & 0x0000FFFF0000FFFFull);
    return (x << 32) | (x >> 32);
#endif
}

BITOPS_INLINE unsigned int swap_byteorder_32(unsigned int x)
{
#if (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__)
    return __builtin_bswap32(x);
#elif defined(_MSC_VER)
    return _byteswap_ulong(x);
#else
    x = ((x << 8) & 0xFF00FF00u) | ((x >> 8) & 0xFF00FFu);
    return (x << 16) | (x >> 16);
#endif
}

//

// input limited to 0..15! (slower than using a lookup table)
BITOPS_INLINE unsigned char __brevnyb(unsigned char i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbit(i) >> 28;
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = ((i >> 1) & 0x5u) | ((i & 0x5u)*2);
    return (i*4 | (i >> 2)) & 0xfu;
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse32)
    return __builtin_bitreverse32(i) >> 28;
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet
    return __builtin_arm_rbit(i) >> 28;
#else
    i = ((i >> 1) & 0x5u) | ((i & 0x5u)*2);
    return (i*4 | (i >> 2)) & 0xfu;
#endif
}

// (slower than using a small lookup table)
BITOPS_INLINE unsigned char __brevc(unsigned char i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbit(i) >> 24;
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = ((i >> 1) & 0x55u) | ((i & 0x55u)*2);
    i = ((i >> 2) & 0x33u) | ((i & 0x33u)*4);
    return i*16 | (i >> 4);
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse8)
    return __builtin_bitreverse8(i);
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet
    return __builtin_arm_rbit(i) >> 24;
#else
    i = ((i >> 1) & 0x55u) | ((i & 0x55u)*2);
    i = ((i >> 2) & 0x33u) | ((i & 0x33u)*4);
    return i*16 | (i >> 4);
    //return (unsigned char)((((i * 0x80200802ull) & 0x0884422110ull) * 0x0101010101ull) >> 32);
#endif
}

// input limited to 14bits!
BITOPS_INLINE unsigned short __brev14(unsigned short i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbit(i) >> 18;
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = ((i >> 1) & 0x5555u) | ((i & 0x5555u)*2);
    i = ((i >> 2) & 0x3333u) | ((i & 0x3333u)*4);
    i = ((i >> 4) & 0x0f0fu) | ((i & 0x0f0fu)*16);
    return i*64 | (i >> 10);
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse32)
    return __builtin_bitreverse32(i) >> 18;
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet
    return __builtin_arm_rbit(i) >> 18;
#else
    i = ((i >> 1) & 0x5555u) | ((i & 0x5555u)*2);
    i = ((i >> 2) & 0x3333u) | ((i & 0x3333u)*4);
    i = ((i >> 4) & 0x0f0fu) | ((i & 0x0f0fu)*16);
    return i*64 | (i >> 10);
#endif
}

BITOPS_INLINE unsigned short __brevs(unsigned short i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbit(i) >> 16;
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = ((i >> 1) & 0x5555u) | ((i & 0x5555u)*2);
    i = ((i >> 2) & 0x3333u) | ((i & 0x3333u)*4);
    i = ((i >> 4) & 0x0f0fu) | ((i & 0x0f0fu)*16);
    return i*256 | (i >> 8);
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse16)
    return __builtin_bitreverse16(i);
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet //!! use arm_acle.h ? __rev or something?
    return __builtin_arm_rbit(i) >> 16;
#else
    i = ((i >> 1) & 0x5555u) | ((i & 0x5555u)*2);
    i = ((i >> 2) & 0x3333u) | ((i & 0x3333u)*4);
    i = ((i >> 4) & 0x0f0fu) | ((i & 0x0f0fu)*16);
    return i*256 | (i >> 8);
#endif
}

BITOPS_INLINE unsigned int __brev(unsigned int i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbit(i);
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = rotr_32(i & 0xaaaaaaaau, 2) | (i & 0x55555555u);
    i = rotr_32(i & 0x66666666u, 4) | (i & 0x99999999u);
    i = rotr_32(i & 0x1e1e1e1eu, 8) | (i & 0xe1e1e1e1u);
    i = rotl_32(i, 7);
    return swap_byteorder_32(i);
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse32)
    return __builtin_bitreverse32(i);
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet //!! use arm_acle.h ? __rev or something?
    return __builtin_arm_rbit(i);
#else
    /*i = i*65536 | (i >> 16);
    i =    ((i & 0x00ff00ffu)*256)  | ((i & 0xff00ff00u) >> 8);
    i =    ((i & 0x0f0f0f0fu)*16)   | ((i & 0xf0f0f0f0u) >> 4);
    i =    ((i & 0x33333333u)*4)    | ((i & 0xccccccccu) >> 2);
    return ((i & 0x55555555u)*2) | ((i & 0xaaaaaaaau) >> 1);*/
    /*i = ((i >> 1) & 0x55555555u) | ((i & 0x55555555u)*2);
    i = ((i >> 2) & 0x33333333u) | ((i & 0x33333333u)*4);
    i = ((i >> 4) & 0x0f0f0f0fu) | ((i & 0x0f0f0f0fu)*16);
    i = ((i >> 8) & 0x00ff00ffu) | ((i & 0x00ff00ffu)*256);
    return i*65536 | (i >> 16);*/
    i = rotr_32(i & 0xaaaaaaaau, 2) | (i & 0x55555555u);
    i = rotr_32(i & 0x66666666u, 4) | (i & 0x99999999u);
    i = rotr_32(i & 0x1e1e1e1eu, 8) | (i & 0xe1e1e1e1u);
    i = rotl_32(i, 7);
    return swap_byteorder_32(i);
#endif
}

BITOPS_INLINE unsigned long long __brevll(unsigned long long i)
{
#if defined(_M_ARM64) && defined(_MSC_VER)
    return __rbitll(i);
#elif defined(_MSC_VER) || (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ < 4))
    i = rotr_64(i & 0xaaaaaaaaaaaaaaaaull, 2) | (i & 0x5555555555555555ull);
    i = rotr_64(i & 0x6666666666666666ull, 4) | (i & 0x9999999999999999ull);
    i = rotr_64(i & 0x1e1e1e1e1e1e1e1eull, 8) | (i & 0xe1e1e1e1e1e1e1e1ull);
    i = rotl_64(i, 7);
    return swap_byteorder_64(i);
#elif defined(__has_builtin) && __has_builtin(__builtin_bitreverse64)
    return __builtin_bitreverse64(i);
#elif defined(__aarch64__) && defined(__clang__) //!! gcc does not have an intrinsic yet
    return __builtin_arm_rbit64(i);
#else
    /*i = (i << 32) | (i >> 32);
    i =    ((i & 0x0000ffff0000ffffull)*65536) | ((i & 0xffff0000ffff0000ull) >> 16);
    i =    ((i & 0x00ff00ff00ff00ffull) * 256) | ((i & 0xff00ff00ff00ff00ull) >>  8);
    i =    ((i & 0x0f0f0f0f0f0f0f0full) *  16) | ((i & 0xf0f0f0f0f0f0f0f0ull) >>  4);
    i =    ((i & 0x3333333333333333ull) *   4) | ((i & 0xccccccccccccccccull) >>  2);
    return ((i & 0x5555555555555555ull) *   2) | ((i & 0xaaaaaaaaaaaaaaaaull) >>  1);*/
    i = rotr_64(i & 0xaaaaaaaaaaaaaaaaull, 2) | (i & 0x5555555555555555ull);
    i = rotr_64(i & 0x6666666666666666ull, 4) | (i & 0x9999999999999999ull);
    i = rotr_64(i & 0x1e1e1e1e1e1e1e1eull, 8) | (i & 0xe1e1e1e1e1e1e1e1ull);
    i = rotl_64(i, 7);
    return swap_byteorder_64(i);
#endif
}

//

// arithmetic shift right, count 0..31; every supported compiler shifts a negative int arithmetically
BITOPS_INLINE unsigned int sar_32(const unsigned int x, const unsigned int count)
{
    return (unsigned int)((int)x >> count);
}

// leading zero bits, 32 for 0 (_BitScanReverse rather than __lzcnt: LZCNT runs as BSR on CPUs without it)
BITOPS_INLINE unsigned int clz_32(const unsigned int x)
{
#if defined(_MSC_VER)
    unsigned long i;
    return _BitScanReverse(&i, x) ? 31u - (unsigned int)i : 32u;
#elif (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__)
    return x ? (unsigned int)__builtin_clz(x) : 32u;
#else
    unsigned int n = 0, y = x;
    if (!y) return 32;
    if (!(y & 0xFFFF0000u)) { n += 16; y <<= 16; }
    if (!(y & 0xFF000000u)) { n +=  8; y <<=  8; }
    if (!(y & 0xF0000000u)) { n +=  4; y <<=  4; }
    if (!(y & 0xC0000000u)) { n +=  2; y <<=  2; }
    if (!(y & 0x80000000u)) n++;
    return n;
#endif
}

// 1 if an odd number of bits is set
BITOPS_INLINE unsigned int parity_32(unsigned int x)
{
#if (defined(__GNUC__) && (__GNUC__ > 3)) || defined(__clang__)
    return (unsigned int)__builtin_parity(x);
#else
    // no intrinsic short of POPCNT, which is not baseline x64: fold to a nibble, then a 16 bit table
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    return (0x6996u >> (x & 15)) & 1;
#endif
}
