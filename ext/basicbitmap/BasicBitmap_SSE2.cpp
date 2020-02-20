//=====================================================================
//
// BasicBitmap_SSE2.cpp - BasicBitmap SSE2 optimize
//
// NOTE:
// for more information, please see the readme file
//
//=====================================================================
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
 #define uint16_t unsigned short
 #define uint32_t unsigned int
 #define uint64_t unsigned long long
#else
 #include <stdint.h>
#endif

#include "BasicBitmap.h"
#include "BasicBitmap_C.h"

#if (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
	#define _COMPILE_WITH_SSE2
#endif

#if 0
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
	#if (_MSC_VER >= 1400) || defined(__SSE2__)
		#define _COMPILE_WITH_SSE2
	#endif
	#if (_MSC_VER >= 1600) || defined(__AVX__)
		#define _COMPILE_WITH_AVX
	#endif
#else
	#if defined(__SSE2__)
		#define _COMPILE_WITH_SSE2
	#endif
	#if defined(__AVX__)
		#define _COMPILE_WITH_AVX
	#endif
#endif
#endif

#ifdef _COMPILE_WITH_SSE2

#include <emmintrin.h>

#ifdef _COMPILE_WITH_AVX
	#include <immintrin.h>
#endif


//---------------------------------------------------------------------
// force inline for compilers
//---------------------------------------------------------------------
#ifndef INLINE
#ifdef __GNUC__
#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
    #define INLINE         __inline__ __attribute__((always_inline))
#else
    #define INLINE         __inline__
#endif
#elif defined(_MSC_VER)
	#define INLINE __forceinline
#elif (defined(__BORLANDC__) || defined(__WATCOMC__))
    #define INLINE __inline
#else
    #define INLINE 
#endif
#endif

#define bb_inline INLINE



//---------------------------------------------------------------------
// fast copy for different sizes
//---------------------------------------------------------------------
static bb_inline void memcpy_sse2_16(void *dst, const void *src) {
	__m128i m0 = _mm_loadu_si128(((const __m128i*)src) + 0);
	_mm_storeu_si128(((__m128i*)dst) + 0, m0);
}

static bb_inline void memcpy_sse2_32(void *dst, const void *src) {
	__m128i m0 = _mm_loadu_si128(((const __m128i*)src) + 0);
	__m128i m1 = _mm_loadu_si128(((const __m128i*)src) + 1);
	_mm_storeu_si128(((__m128i*)dst) + 0, m0);
	_mm_storeu_si128(((__m128i*)dst) + 1, m1);
}

static bb_inline void memcpy_sse2_64(void *dst, const void *src) {
	__m128i m0 = _mm_loadu_si128(((const __m128i*)src) + 0);
	__m128i m1 = _mm_loadu_si128(((const __m128i*)src) + 1);
	__m128i m2 = _mm_loadu_si128(((const __m128i*)src) + 2);
	__m128i m3 = _mm_loadu_si128(((const __m128i*)src) + 3);
	_mm_storeu_si128(((__m128i*)dst) + 0, m0);
	_mm_storeu_si128(((__m128i*)dst) + 1, m1);
	_mm_storeu_si128(((__m128i*)dst) + 2, m2);
	_mm_storeu_si128(((__m128i*)dst) + 3, m3);
}

static bb_inline void memcpy_sse2_128(void *dst, const void *src) {
	__m128i m0 = _mm_loadu_si128(((const __m128i*)src) + 0);
	__m128i m1 = _mm_loadu_si128(((const __m128i*)src) + 1);
	__m128i m2 = _mm_loadu_si128(((const __m128i*)src) + 2);
	__m128i m3 = _mm_loadu_si128(((const __m128i*)src) + 3);
	__m128i m4 = _mm_loadu_si128(((const __m128i*)src) + 4);
	__m128i m5 = _mm_loadu_si128(((const __m128i*)src) + 5);
	__m128i m6 = _mm_loadu_si128(((const __m128i*)src) + 6);
	__m128i m7 = _mm_loadu_si128(((const __m128i*)src) + 7);
	_mm_storeu_si128(((__m128i*)dst) + 0, m0);
	_mm_storeu_si128(((__m128i*)dst) + 1, m1);
	_mm_storeu_si128(((__m128i*)dst) + 2, m2);
	_mm_storeu_si128(((__m128i*)dst) + 3, m3);
	_mm_storeu_si128(((__m128i*)dst) + 4, m4);
	_mm_storeu_si128(((__m128i*)dst) + 5, m5);
	_mm_storeu_si128(((__m128i*)dst) + 6, m6);
	_mm_storeu_si128(((__m128i*)dst) + 7, m7);
}


//---------------------------------------------------------------------
// tiny memory copy with jump table optimized
//---------------------------------------------------------------------
static bb_inline void *memcpy_tiny(void *dst, const void *src, size_t size) {
	unsigned char *dd = ((unsigned char*)dst) + size;
	const unsigned char *ss = ((const unsigned char*)src) + size;

	switch (size) { 
	case 64:
		memcpy_sse2_64(dd - 64, ss - 64);
	case 0:
		break;

	case 65:
		memcpy_sse2_64(dd - 65, ss - 65);
	case 1:
		dd[-1] = ss[-1];
		break;

	case 66:
		memcpy_sse2_64(dd - 66, ss - 66);
	case 2:
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 67:
		memcpy_sse2_64(dd - 67, ss - 67);
	case 3:
		*((uint16_t*)(dd - 3)) = *((uint16_t*)(ss - 3));
		dd[-1] = ss[-1];
		break;

	case 68:
		memcpy_sse2_64(dd - 68, ss - 68);
	case 4:
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 69:
		memcpy_sse2_64(dd - 69, ss - 69);
	case 5:
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 70:
		memcpy_sse2_64(dd - 70, ss - 70);
	case 6:
		*((uint32_t*)(dd - 6)) = *((uint32_t*)(ss - 6));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 71:
		memcpy_sse2_64(dd - 71, ss - 71);
	case 7:
		*((uint32_t*)(dd - 7)) = *((uint32_t*)(ss - 7));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 72:
		memcpy_sse2_64(dd - 72, ss - 72);
	case 8:
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 73:
		memcpy_sse2_64(dd - 73, ss - 73);
	case 9:
		*((uint64_t*)(dd - 9)) = *((uint64_t*)(ss - 9));
		dd[-1] = ss[-1];
		break;

	case 74:
		memcpy_sse2_64(dd - 74, ss - 74);
	case 10:
		*((uint64_t*)(dd - 10)) = *((uint64_t*)(ss - 10));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 75:
		memcpy_sse2_64(dd - 75, ss - 75);
	case 11:
		*((uint64_t*)(dd - 11)) = *((uint64_t*)(ss - 11));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 76:
		memcpy_sse2_64(dd - 76, ss - 76);
	case 12:
		*((uint64_t*)(dd - 12)) = *((uint64_t*)(ss - 12));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 77:
		memcpy_sse2_64(dd - 77, ss - 77);
	case 13:
		*((uint64_t*)(dd - 13)) = *((uint64_t*)(ss - 13));
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 78:
		memcpy_sse2_64(dd - 78, ss - 78);
	case 14:
		*((uint64_t*)(dd - 14)) = *((uint64_t*)(ss - 14));
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 79:
		memcpy_sse2_64(dd - 79, ss - 79);
	case 15:
		*((uint64_t*)(dd - 15)) = *((uint64_t*)(ss - 15));
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 80:
		memcpy_sse2_64(dd - 80, ss - 80);
	case 16:
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 81:
		memcpy_sse2_64(dd - 81, ss - 81);
	case 17:
		memcpy_sse2_16(dd - 17, ss - 17);
		dd[-1] = ss[-1];
		break;

	case 82:
		memcpy_sse2_64(dd - 82, ss - 82);
	case 18:
		memcpy_sse2_16(dd - 18, ss - 18);
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 83:
		memcpy_sse2_64(dd - 83, ss - 83);
	case 19:
		memcpy_sse2_16(dd - 19, ss - 19);
		*((uint16_t*)(dd - 3)) = *((uint16_t*)(ss - 3));
		dd[-1] = ss[-1];
		break;

	case 84:
		memcpy_sse2_64(dd - 84, ss - 84);
	case 20:
		memcpy_sse2_16(dd - 20, ss - 20);
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 85:
		memcpy_sse2_64(dd - 85, ss - 85);
	case 21:
		memcpy_sse2_16(dd - 21, ss - 21);
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 86:
		memcpy_sse2_64(dd - 86, ss - 86);
	case 22:
		memcpy_sse2_16(dd - 22, ss - 22);
		*((uint32_t*)(dd - 6)) = *((uint32_t*)(ss - 6));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 87:
		memcpy_sse2_64(dd - 87, ss - 87);
	case 23:
		memcpy_sse2_16(dd - 23, ss - 23);
		*((uint32_t*)(dd - 7)) = *((uint32_t*)(ss - 7));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 88:
		memcpy_sse2_64(dd - 88, ss - 88);
	case 24:
		memcpy_sse2_16(dd - 24, ss - 24);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 89:
		memcpy_sse2_64(dd - 89, ss - 89);
	case 25:
		memcpy_sse2_16(dd - 25, ss - 25);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 90:
		memcpy_sse2_64(dd - 90, ss - 90);
	case 26:
		memcpy_sse2_16(dd - 26, ss - 26);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 91:
		memcpy_sse2_64(dd - 91, ss - 91);
	case 27:
		memcpy_sse2_16(dd - 27, ss - 27);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 92:
		memcpy_sse2_64(dd - 92, ss - 92);
	case 28:
		memcpy_sse2_16(dd - 28, ss - 28);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 93:
		memcpy_sse2_64(dd - 93, ss - 93);
	case 29:
		memcpy_sse2_16(dd - 29, ss - 29);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 94:
		memcpy_sse2_64(dd - 94, ss - 94);
	case 30:
		memcpy_sse2_16(dd - 30, ss - 30);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 95:
		memcpy_sse2_64(dd - 95, ss - 95);
	case 31:
		memcpy_sse2_16(dd - 31, ss - 31);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 96:
		memcpy_sse2_64(dd - 96, ss - 96);
	case 32:
		memcpy_sse2_32(dd - 32, ss - 32);
		break;

	case 97:
		memcpy_sse2_64(dd - 97, ss - 97);
	case 33:
		memcpy_sse2_32(dd - 33, ss - 33);
		dd[-1] = ss[-1];
		break;

	case 98:
		memcpy_sse2_64(dd - 98, ss - 98);
	case 34:
		memcpy_sse2_32(dd - 34, ss - 34);
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 99:
		memcpy_sse2_64(dd - 99, ss - 99);
	case 35:
		memcpy_sse2_32(dd - 35, ss - 35);
		*((uint16_t*)(dd - 3)) = *((uint16_t*)(ss - 3));
		dd[-1] = ss[-1];
		break;

	case 100:
		memcpy_sse2_64(dd - 100, ss - 100);
	case 36:
		memcpy_sse2_32(dd - 36, ss - 36);
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 101:
		memcpy_sse2_64(dd - 101, ss - 101);
	case 37:
		memcpy_sse2_32(dd - 37, ss - 37);
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 102:
		memcpy_sse2_64(dd - 102, ss - 102);
	case 38:
		memcpy_sse2_32(dd - 38, ss - 38);
		*((uint32_t*)(dd - 6)) = *((uint32_t*)(ss - 6));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 103:
		memcpy_sse2_64(dd - 103, ss - 103);
	case 39:
		memcpy_sse2_32(dd - 39, ss - 39);
		*((uint32_t*)(dd - 7)) = *((uint32_t*)(ss - 7));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 104:
		memcpy_sse2_64(dd - 104, ss - 104);
	case 40:
		memcpy_sse2_32(dd - 40, ss - 40);
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 105:
		memcpy_sse2_64(dd - 105, ss - 105);
	case 41:
		memcpy_sse2_32(dd - 41, ss - 41);
		*((uint64_t*)(dd - 9)) = *((uint64_t*)(ss - 9));
		dd[-1] = ss[-1];
		break;

	case 106:
		memcpy_sse2_64(dd - 106, ss - 106);
	case 42:
		memcpy_sse2_32(dd - 42, ss - 42);
		*((uint64_t*)(dd - 10)) = *((uint64_t*)(ss - 10));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 107:
		memcpy_sse2_64(dd - 107, ss - 107);
	case 43:
		memcpy_sse2_32(dd - 43, ss - 43);
		*((uint64_t*)(dd - 11)) = *((uint64_t*)(ss - 11));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 108:
		memcpy_sse2_64(dd - 108, ss - 108);
	case 44:
		memcpy_sse2_32(dd - 44, ss - 44);
		*((uint64_t*)(dd - 12)) = *((uint64_t*)(ss - 12));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 109:
		memcpy_sse2_64(dd - 109, ss - 109);
	case 45:
		memcpy_sse2_32(dd - 45, ss - 45);
		*((uint64_t*)(dd - 13)) = *((uint64_t*)(ss - 13));
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 110:
		memcpy_sse2_64(dd - 110, ss - 110);
	case 46:
		memcpy_sse2_32(dd - 46, ss - 46);
		*((uint64_t*)(dd - 14)) = *((uint64_t*)(ss - 14));
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 111:
		memcpy_sse2_64(dd - 111, ss - 111);
	case 47:
		memcpy_sse2_32(dd - 47, ss - 47);
		*((uint64_t*)(dd - 15)) = *((uint64_t*)(ss - 15));
		*((uint64_t*)(dd - 8)) = *((uint64_t*)(ss - 8));
		break;

	case 112:
		memcpy_sse2_64(dd - 112, ss - 112);
	case 48:
		memcpy_sse2_32(dd - 48, ss - 48);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 113:
		memcpy_sse2_64(dd - 113, ss - 113);
	case 49:
		memcpy_sse2_32(dd - 49, ss - 49);
		memcpy_sse2_16(dd - 17, ss - 17);
		dd[-1] = ss[-1];
		break;

	case 114:
		memcpy_sse2_64(dd - 114, ss - 114);
	case 50:
		memcpy_sse2_32(dd - 50, ss - 50);
		memcpy_sse2_16(dd - 18, ss - 18);
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 115:
		memcpy_sse2_64(dd - 115, ss - 115);
	case 51:
		memcpy_sse2_32(dd - 51, ss - 51);
		memcpy_sse2_16(dd - 19, ss - 19);
		*((uint16_t*)(dd - 3)) = *((uint16_t*)(ss - 3));
		dd[-1] = ss[-1];
		break;

	case 116:
		memcpy_sse2_64(dd - 116, ss - 116);
	case 52:
		memcpy_sse2_32(dd - 52, ss - 52);
		memcpy_sse2_16(dd - 20, ss - 20);
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 117:
		memcpy_sse2_64(dd - 117, ss - 117);
	case 53:
		memcpy_sse2_32(dd - 53, ss - 53);
		memcpy_sse2_16(dd - 21, ss - 21);
		*((uint32_t*)(dd - 5)) = *((uint32_t*)(ss - 5));
		dd[-1] = ss[-1];
		break;

	case 118:
		memcpy_sse2_64(dd - 118, ss - 118);
	case 54:
		memcpy_sse2_32(dd - 54, ss - 54);
		memcpy_sse2_16(dd - 22, ss - 22);
		*((uint32_t*)(dd - 6)) = *((uint32_t*)(ss - 6));
		*((uint16_t*)(dd - 2)) = *((uint16_t*)(ss - 2));
		break;

	case 119:
		memcpy_sse2_64(dd - 119, ss - 119);
	case 55:
		memcpy_sse2_32(dd - 55, ss - 55);
		memcpy_sse2_16(dd - 23, ss - 23);
		*((uint32_t*)(dd - 7)) = *((uint32_t*)(ss - 7));
		*((uint32_t*)(dd - 4)) = *((uint32_t*)(ss - 4));
		break;

	case 120:
		memcpy_sse2_64(dd - 120, ss - 120);
	case 56:
		memcpy_sse2_32(dd - 56, ss - 56);
		memcpy_sse2_16(dd - 24, ss - 24);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 121:
		memcpy_sse2_64(dd - 121, ss - 121);
	case 57:
		memcpy_sse2_32(dd - 57, ss - 57);
		memcpy_sse2_16(dd - 25, ss - 25);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 122:
		memcpy_sse2_64(dd - 122, ss - 122);
	case 58:
		memcpy_sse2_32(dd - 58, ss - 58);
		memcpy_sse2_16(dd - 26, ss - 26);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 123:
		memcpy_sse2_64(dd - 123, ss - 123);
	case 59:
		memcpy_sse2_32(dd - 59, ss - 59);
		memcpy_sse2_16(dd - 27, ss - 27);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 124:
		memcpy_sse2_64(dd - 124, ss - 124);
	case 60:
		memcpy_sse2_32(dd - 60, ss - 60);
		memcpy_sse2_16(dd - 28, ss - 28);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 125:
		memcpy_sse2_64(dd - 125, ss - 125);
	case 61:
		memcpy_sse2_32(dd - 61, ss - 61);
		memcpy_sse2_16(dd - 29, ss - 29);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 126:
		memcpy_sse2_64(dd - 126, ss - 126);
	case 62:
		memcpy_sse2_32(dd - 62, ss - 62);
		memcpy_sse2_16(dd - 30, ss - 30);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 127:
		memcpy_sse2_64(dd - 127, ss - 127);
	case 63:
		memcpy_sse2_32(dd - 63, ss - 63);
		memcpy_sse2_16(dd - 31, ss - 31);
		memcpy_sse2_16(dd - 16, ss - 16);
		break;

	case 128:
		memcpy_sse2_128(dd - 128, ss - 128);
		break;
	}

	return dst;
}


//---------------------------------------------------------------------
// fast memory copy: improve 50% of speed vs traditional memcpy
//---------------------------------------------------------------------
static void* memcpy_fast(void *destination, const void *source, size_t size)
{
	unsigned char *dst = (unsigned char*)destination;
	const unsigned char *src = (const unsigned char*)source;
	static size_t cachesize = 0x200000; // L2-cache size
	size_t padding;

	// small memory copy
	if (size <= 128) {
		return memcpy_tiny(dst, src, size);
	}

	// align destination to 16 bytes boundary
	padding = (16 - (((size_t)dst) & 15)) & 15;

	if (padding > 0) {
		__m128i head = _mm_loadu_si128((const __m128i*)src);
		_mm_storeu_si128((__m128i*)dst, head);
		dst += padding;
		src += padding;
		size -= padding;
	}

	// medium size copy
	if (size <= cachesize) {
		__m128i c0, c1, c2, c3, c4, c5, c6, c7;

		for (; size >= 128; size -= 128) {
			c0 = _mm_loadu_si128(((const __m128i*)src) + 0);
			c1 = _mm_loadu_si128(((const __m128i*)src) + 1);
			c2 = _mm_loadu_si128(((const __m128i*)src) + 2);
			c3 = _mm_loadu_si128(((const __m128i*)src) + 3);
			c4 = _mm_loadu_si128(((const __m128i*)src) + 4);
			c5 = _mm_loadu_si128(((const __m128i*)src) + 5);
			c6 = _mm_loadu_si128(((const __m128i*)src) + 6);
			c7 = _mm_loadu_si128(((const __m128i*)src) + 7);
			_mm_prefetch((const char*)(src + 256), _MM_HINT_NTA);
			src += 128;
			_mm_store_si128((((__m128i*)dst) + 0), c0);
			_mm_store_si128((((__m128i*)dst) + 1), c1);
			_mm_store_si128((((__m128i*)dst) + 2), c2);
			_mm_store_si128((((__m128i*)dst) + 3), c3);
			_mm_store_si128((((__m128i*)dst) + 4), c4);
			_mm_store_si128((((__m128i*)dst) + 5), c5);
			_mm_store_si128((((__m128i*)dst) + 6), c6);
			_mm_store_si128((((__m128i*)dst) + 7), c7);
			dst += 128;
		}
	}
	else {		// big memory copy
		__m128i c0, c1, c2, c3, c4, c5, c6, c7;

		_mm_prefetch((const char*)(src), _MM_HINT_NTA);

		if ((((size_t)src) & 15) == 0) {	// source aligned
			for (; size >= 128; size -= 128) {
				c0 = _mm_load_si128(((const __m128i*)src) + 0);
				c1 = _mm_load_si128(((const __m128i*)src) + 1);
				c2 = _mm_load_si128(((const __m128i*)src) + 2);
				c3 = _mm_load_si128(((const __m128i*)src) + 3);
				c4 = _mm_load_si128(((const __m128i*)src) + 4);
				c5 = _mm_load_si128(((const __m128i*)src) + 5);
				c6 = _mm_load_si128(((const __m128i*)src) + 6);
				c7 = _mm_load_si128(((const __m128i*)src) + 7);
				_mm_prefetch((const char*)(src + 256), _MM_HINT_NTA);
				src += 128;
				_mm_stream_si128((((__m128i*)dst) + 0), c0);
				_mm_stream_si128((((__m128i*)dst) + 1), c1);
				_mm_stream_si128((((__m128i*)dst) + 2), c2);
				_mm_stream_si128((((__m128i*)dst) + 3), c3);
				_mm_stream_si128((((__m128i*)dst) + 4), c4);
				_mm_stream_si128((((__m128i*)dst) + 5), c5);
				_mm_stream_si128((((__m128i*)dst) + 6), c6);
				_mm_stream_si128((((__m128i*)dst) + 7), c7);
				dst += 128;
			}
		}
		else {							// source unaligned
			for (; size >= 128; size -= 128) {
				c0 = _mm_loadu_si128(((const __m128i*)src) + 0);
				c1 = _mm_loadu_si128(((const __m128i*)src) + 1);
				c2 = _mm_loadu_si128(((const __m128i*)src) + 2);
				c3 = _mm_loadu_si128(((const __m128i*)src) + 3);
				c4 = _mm_loadu_si128(((const __m128i*)src) + 4);
				c5 = _mm_loadu_si128(((const __m128i*)src) + 5);
				c6 = _mm_loadu_si128(((const __m128i*)src) + 6);
				c7 = _mm_loadu_si128(((const __m128i*)src) + 7);
				_mm_prefetch((const char*)(src + 256), _MM_HINT_NTA);
				src += 128;
				_mm_stream_si128((((__m128i*)dst) + 0), c0);
				_mm_stream_si128((((__m128i*)dst) + 1), c1);
				_mm_stream_si128((((__m128i*)dst) + 2), c2);
				_mm_stream_si128((((__m128i*)dst) + 3), c3);
				_mm_stream_si128((((__m128i*)dst) + 4), c4);
				_mm_stream_si128((((__m128i*)dst) + 5), c5);
				_mm_stream_si128((((__m128i*)dst) + 6), c6);
				_mm_stream_si128((((__m128i*)dst) + 7), c7);
				dst += 128;
			}
		}
		_mm_sfence();
	}

	memcpy_tiny(dst, src, size);

	return destination;
}


//---------------------------------------------------------------------
// fast blit using sse2
//---------------------------------------------------------------------
static void NormalBlit_0(void *dst, long dstpitch, 
	const void *src, long srcpitch, int width, int height) {
	if (dstpitch == srcpitch && (long)width == srcpitch) {
		memcpy_fast(dst, src, (size_t)width * height);
		return;
	}
	if (width <= 128) {
		for (; height > 0; height--) {
			memcpy_tiny(dst, src, width);
			dst = (char*)dst + dstpitch;
			src = (const char*)src + srcpitch;
		}
	}
#if 1
	else if (width <= 1600) {
		if ((width & 31) == 0) {
			int count = width >> 5;
			for (; height > 0; height--) {
				unsigned char *dd = (unsigned char*)dst;
				const unsigned char *ss = (const unsigned char*)src;
				for (int x = count; x > 0; x--) {
					__m128i c1 = _mm_loadu_si128((const __m128i*)ss);
					__m128i c2 = _mm_loadu_si128((const __m128i*)ss + 1);
					ss += 32;
					_mm_storeu_si128((__m128i*)dd, c1);
					_mm_storeu_si128((__m128i*)dd + 1, c2);
					dd += 32;
				}
				dst = (char*)dst + dstpitch;
				src = (const char*)src + srcpitch;
			}
		}	else {
			for (; height > 0; height--) {
				unsigned char *dd = (unsigned char*)dst;
				const unsigned char *ss = (const unsigned char*)src;
				int x = width;
				for (; x >= 32; x -= 32) {
					__m128i c1 = _mm_loadu_si128((const __m128i*)ss);
					__m128i c2 = _mm_loadu_si128((const __m128i*)ss + 1);
					ss += 32;
					_mm_storeu_si128((__m128i*)dd, c1);
					_mm_storeu_si128((__m128i*)dd + 1, c2);
					dd += 32;
				}
				memcpy_tiny(dd, ss, x);
				dst = (char*)dst + dstpitch;
				src = (const char*)src + srcpitch;
			}
		}
	}
#endif
	else {
		for (; height > 0; height--) {
			memcpy_fast(dst, src, width);
			dst = (char*)dst + dstpitch;
			src = (const char*)src + srcpitch;
		}
	}
}


//---------------------------------------------------------------------
// fast blit using sse2
//---------------------------------------------------------------------
static int NormalBlit_8(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	if (flip != 0) return -1;
	NormalBlit_0((char*)dst + dx, dpitch, (const char*)src + sx, spitch, w, h);
	return 0;
}

//---------------------------------------------------------------------
// fast blit using sse2
//---------------------------------------------------------------------
static int NormalBlit_16(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	if (flip != 0) return -1;
	NormalBlit_0((short*)dst + dx, dpitch, (const short*)src + sx, spitch, w * 2, h);
	return 0;
}

//---------------------------------------------------------------------
// fast blit using sse2
//---------------------------------------------------------------------
static int NormalBlit_24(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	if (flip != 0) return -1;
	NormalBlit_0((char*)dst + dx * 3, dpitch, (const char*)src + sx * 3, spitch, w * 3, h);
	return 0;
}

//---------------------------------------------------------------------
// fast blit using sse2
//---------------------------------------------------------------------
static int NormalBlit_32(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	if (flip != 0) return -1;
	NormalBlit_0((IUINT32*)dst + dx, dpitch, (const IUINT32*)src + sx, spitch, w * 4, h);
	return 0;
}


//---------------------------------------------------------------------
// speed-up over three-times faster than original C routine
//---------------------------------------------------------------------
static int TransparentBlit_8(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	__m128i c1, c2, c3, c4, m1, m2, m3, m4, mask, ffff;
	IUINT8 cmask = (IUINT8)transparent;

	if (flip != 0) return -1;

	mask = _mm_set1_epi8(transparent & 0xff);
	ffff = _mm_set1_epi32(0xffffffff);

	for (int j = 0; j < h; j++) {
		const IUINT8 *ss = (const IUINT8*)src + sx;
		IUINT8 *dd = (IUINT8*)dst + dx;
		int width = w;

		_mm_prefetch((const char*)(ss), _MM_HINT_NTA);

		for (; width >= 64; width -= 64) {
			_mm_prefetch((const char*)(ss + 128), _MM_HINT_NTA);
			c1 = _mm_loadu_si128(((const __m128i*)ss) + 0);
			c2 = _mm_loadu_si128(((const __m128i*)ss) + 1);
			c3 = _mm_loadu_si128(((const __m128i*)ss) + 2);
			c4 = _mm_loadu_si128(((const __m128i*)ss) + 3);
			m1 = _mm_cmpeq_epi8(c1, mask);
			m2 = _mm_cmpeq_epi8(c2, mask);
			m3 = _mm_cmpeq_epi8(c3, mask);
			m4 = _mm_cmpeq_epi8(c4, mask);
			m1 = _mm_xor_si128(m1, ffff);
			m2 = _mm_xor_si128(m2, ffff);
			m3 = _mm_xor_si128(m3, ffff);
			m4 = _mm_xor_si128(m4, ffff);
			_mm_maskmoveu_si128(c1, m1, (char*)(((__m128i*)dd) + 0));
			_mm_maskmoveu_si128(c2, m2, (char*)(((__m128i*)dd) + 1));
			_mm_maskmoveu_si128(c3, m3, (char*)(((__m128i*)dd) + 2));
			_mm_maskmoveu_si128(c4, m4, (char*)(((__m128i*)dd) + 3));
			ss += 4 * 16;
			dd += 4 * 16;
		}

		for (; width >= 16; width -= 16) {
			_mm_prefetch((const char*)(ss + 128), _MM_HINT_NTA);
			c1 = _mm_loadu_si128(((const __m128i*)ss) + 0);
			m1 = _mm_cmpeq_epi8(c1, mask);
			m1 = _mm_xor_si128(m1, ffff);
			_mm_maskmoveu_si128(c1, m1, (char*)(((__m128i*)dd) + 0));
			ss += 16;
			dd += 16;
		}

		_mm_sfence();
		
		for (; width > 0; ss++, dd++, width--) {
			if (ss[0] != cmask) dd[0] = ss[0];
		}

		dst = (char*)dst + dpitch;
		src = (const char*)src + spitch;
	}
	return 0;
}


//---------------------------------------------------------------------
// speed-up over twice faster than original C routine
//---------------------------------------------------------------------
static int TransparentBlit_16(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	__m128i c1, c2, c3, c4, m1, m2, m3, m4, mask, ffff;
	IUINT16 cmask = (IUINT16)transparent;

	if (flip != 0) return -1;

	mask = _mm_set1_epi16(transparent & 0xffff);
	ffff = _mm_set1_epi32(0xffffffff);

	for (int j = 0; j < h; j++) {
		const IUINT16 *ss = (const IUINT16*)src + sx;
		IUINT16 *dd = (IUINT16*)dst + dx;
		int width = w;

		_mm_prefetch((const char*)(ss + 32), _MM_HINT_NTA);

		for (; width >= 32; width -= 32) {
			_mm_prefetch((const char*)(ss + 128), _MM_HINT_NTA);
			c1 = _mm_loadu_si128(((const __m128i*)ss) + 0);
			c2 = _mm_loadu_si128(((const __m128i*)ss) + 1);
			c3 = _mm_loadu_si128(((const __m128i*)ss) + 2);
			c4 = _mm_loadu_si128(((const __m128i*)ss) + 3);
			m1 = _mm_cmpeq_epi16(c1, mask);
			m2 = _mm_cmpeq_epi16(c2, mask);
			m3 = _mm_cmpeq_epi16(c3, mask);
			m4 = _mm_cmpeq_epi16(c4, mask);
			m1 = _mm_xor_si128(m1, ffff);
			m2 = _mm_xor_si128(m2, ffff);
			m3 = _mm_xor_si128(m3, ffff);
			m4 = _mm_xor_si128(m4, ffff);
			_mm_maskmoveu_si128(c1, m1, (char*)(((__m128i*)dd) + 0));
			_mm_maskmoveu_si128(c2, m2, (char*)(((__m128i*)dd) + 1));
			_mm_maskmoveu_si128(c3, m3, (char*)(((__m128i*)dd) + 2));
			_mm_maskmoveu_si128(c4, m4, (char*)(((__m128i*)dd) + 3));
			ss += 32;
			dd += 32;
		}

		for (; width >= 8; width -= 8) {
			_mm_prefetch((const char*)(ss + 64), _MM_HINT_NTA);
			c1 = _mm_loadu_si128(((const __m128i*)ss) + 0);
			m1 = _mm_cmpeq_epi16(c1, mask);
			m1 = _mm_xor_si128(m1, ffff);
			_mm_maskmoveu_si128(c1, m1, (char*)(((__m128i*)dd) + 0));
			ss += 8;
			dd += 8;
		}

		_mm_sfence();

		for (; width > 0; ss++, dd++, width--) {
			if (ss[0] != cmask) dd[0] = ss[0];
		}
		
		dst = (char*)dst + dpitch;
		src = (const char*)src + spitch;
	}
	return 0;
}


//---------------------------------------------------------------------
// speed-up over twice faster than original C routine
//---------------------------------------------------------------------
static int TransparentBlit_32(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	__m128i c1, c2, c3, c4, m1, m2, m3, m4, mask, ffff;

	if (flip != 0) return -1;

	mask = _mm_set1_epi32(transparent);
	ffff = _mm_set1_epi32(0xffffffff);

	for (int j = 0; j < h; j++) {
		const IUINT32 *ss = (const IUINT32*)src + sx;
		IUINT32 *dd = (IUINT32*)dst + dx;
		int width = w;

		_mm_prefetch((const char*)(ss), _MM_HINT_NTA);

		for (; width >= 16; width -= 16) {
			_mm_prefetch((const char*)(ss + 128), _MM_HINT_NTA);
			c1 = _mm_loadu_si128(((const __m128i*)ss) + 0);
			c2 = _mm_loadu_si128(((const __m128i*)ss) + 1);
			c3 = _mm_loadu_si128(((const __m128i*)ss) + 2);
			c4 = _mm_loadu_si128(((const __m128i*)ss) + 3);
			m1 = _mm_cmpeq_epi32(c1, mask);
			m2 = _mm_cmpeq_epi32(c2, mask);
			m3 = _mm_cmpeq_epi32(c3, mask);
			m4 = _mm_cmpeq_epi32(c4, mask);
			m1 = _mm_xor_si128(m1, ffff);
			m2 = _mm_xor_si128(m2, ffff);
			m3 = _mm_xor_si128(m3, ffff);
			m4 = _mm_xor_si128(m4, ffff);
			_mm_maskmoveu_si128(c1, m1, (char*)(((__m128i*)dd) + 0));
			_mm_maskmoveu_si128(c2, m2, (char*)(((__m128i*)dd) + 1));
			_mm_maskmoveu_si128(c3, m3, (char*)(((__m128i*)dd) + 2));
			_mm_maskmoveu_si128(c4, m4, (char*)(((__m128i*)dd) + 3));
			ss += 16;
			dd += 16;
		}

		_mm_sfence();
		
		for (; width > 0; ss++, dd++, width--) {
			if (ss[0] != transparent) dd[0] = ss[0];
		}

		dst = (char*)dst + dpitch;
		src = (const char*)src + spitch;
	}
	return 0;
}


//---------------------------------------------------------------------
// internal const 
//---------------------------------------------------------------------
static const __m128i _pixel_mask_8x0101 = _mm_set1_epi16(0x0101);
static const __m128i _pixel_mask_8x00ff = _mm_set1_epi16(0x00ff);
static const __m128i _pixel_mask_4x000000ff = _mm_set1_epi32(0xff);

//---------------------------------------------------------------------
// internal inline 
//---------------------------------------------------------------------

// (x + ((x + 257) >> 8)) >> 8
static bb_inline __m128i _mm_fast_div_255_epu16(__m128i x) {
	return _mm_srli_epi16(_mm_adds_epu16(x, 
		_mm_srli_epi16(_mm_adds_epu16(x, _pixel_mask_8x0101), 8)), 8);
}

// alpha = 00 a1 00 a1 00 a2 00 a2 00 a3 00 a3 00 a4 00 a4
static bb_inline __m128i _mm_expand_alpha(__m128i color) {
	__m128i alpha = _mm_srli_epi32(color, 24);
	return _mm_or_si128(_mm_slli_epi32(alpha, 16), alpha);
}

// 255 - alpha
static bb_inline __m128i _mm_expand_inverse_alpha(__m128i color) {
	__m128i alpha = _mm_subs_epu8(_pixel_mask_4x000000ff, _mm_srli_epi32(color, 24));
	return _mm_or_si128(_mm_slli_epi32(alpha, 16), alpha);
}

// 00 rr 00 bb
static bb_inline __m128i _mm_pixel_rb(__m128i color) {
	return _mm_and_si128(color, _pixel_mask_8x00ff);
}

// 00 aa 00 gg
static bb_inline __m128i _mm_pixel_ag(__m128i color) {
	return _mm_srli_epi16(color, 8);
}

// color = (src * sa + dst * (255 - sa)) / 255
static bb_inline __m128i _mm_blend_with_quad(__m128i src, __m128i dst) {
	__m128i as = _mm_expand_alpha(src);
	__m128i ad = _mm_subs_epu8(_pixel_mask_8x00ff, as);
	__m128i rb_src = _mm_mullo_epi16(_mm_pixel_rb(src), as);
	__m128i ag_src = _mm_mullo_epi16(_mm_pixel_ag(src), as);
	__m128i rb_dst = _mm_mullo_epi16(_mm_pixel_rb(dst), ad);
	__m128i ag_dst = _mm_mullo_epi16(_mm_pixel_ag(dst), ad);
	rb_src = _mm_fast_div_255_epu16(_mm_adds_epu16(rb_src, rb_dst));
	ag_src = _mm_fast_div_255_epu16(_mm_adds_epu16(ag_src, ag_dst));
	return _mm_or_si128(_mm_slli_epi32(ag_src, 8), rb_src);
}

// color = src + dst * (255 - sa) / 255
static bb_inline __m128i _mm_blend_over_quad(__m128i src, __m128i dst) {
	__m128i alpha = _mm_expand_inverse_alpha(src);
	__m128i rb = _mm_pixel_rb(dst);
	__m128i ag = _mm_pixel_ag(dst);
	rb = _mm_fast_div_255_epu16(_mm_mullo_epi16(rb, alpha));
	ag = _mm_fast_div_255_epu16(_mm_mullo_epi16(ag, alpha));
	dst = _mm_or_si128(_mm_slli_epi32(ag, 8), rb);
	return _mm_adds_epu8(src, dst);
}


//---------------------------------------------------------------------
// Drawing routines
//---------------------------------------------------------------------

// color = (src * sa + dst * (255 - sa)) / 255
static int PixelDraw_SSE2_BLEND_X8R8G8B8(void *bits, int offset, int w, const IUINT32 *card)
{
	IUINT32 *dst = (IUINT32*)bits + offset;
	__m128i c1, c2, c3, c4;
	for (; w >= 8; card += 8, dst += 8, w -= 8) {
		c1 = _mm_loadu_si128(((const __m128i*)card) + 0);
		c2 = _mm_loadu_si128(((const __m128i*)card) + 1);
		c3 = _mm_loadu_si128(((__m128i*)dst) + 0);
		c4 = _mm_loadu_si128(((__m128i*)dst) + 1);
		c3 = _mm_blend_with_quad(c1, c3);
		c4 = _mm_blend_with_quad(c2, c4);
		_mm_storeu_si128(((__m128i*)dst) + 0, c3);
		_mm_storeu_si128(((__m128i*)dst) + 1, c4);
	}
	if (w & 4) {
		c1 = _mm_loadu_si128(((const __m128i*)card) + 0);
		c3 = _mm_loadu_si128(((__m128i*)dst) + 0);
		c1 = _mm_blend_with_quad(c1, c3);
		_mm_storeu_si128(((__m128i*)dst) + 0, c1);
		card += 4;
		dst += 4;
		w -= 4;
	}
	for (; w > 0; card++, dst++, w--) {
		c1 = _mm_cvtsi32_si128(card[0]);
		c3 = _mm_cvtsi32_si128(dst[0]);
		c1 = _mm_blend_with_quad(c1, c3);
		dst[0] = _mm_cvtsi128_si32(c1);
	}
	return 0;
}


// color = src + dst * (255 - sa) / 255
static int PixelDraw_SSE2_SRCOVER_A8R8G8B8(void *bits, int offset, int w, const IUINT32 *card)
{
	IUINT32 *dst = (IUINT32*)bits + offset;
	__m128i c1, c2, c3, c4;
	for (; w >= 8; card += 8, dst += 8, w -= 8) {
		c1 = _mm_loadu_si128(((const __m128i*)card) + 0);
		c2 = _mm_loadu_si128(((const __m128i*)card) + 1);
		c3 = _mm_loadu_si128(((__m128i*)dst) + 0);
		c4 = _mm_loadu_si128(((__m128i*)dst) + 1);
		c3 = _mm_blend_over_quad(c1, c3);
		c4 = _mm_blend_over_quad(c2, c4);
		_mm_storeu_si128(((__m128i*)dst) + 0, c3);
		_mm_storeu_si128(((__m128i*)dst) + 1, c4);
	}
	if (w & 4) {
		c1 = _mm_loadu_si128(((const __m128i*)card) + 0);
		c3 = _mm_loadu_si128(((__m128i*)dst) + 0);
		c1 = _mm_blend_over_quad(c1, c3);
		_mm_storeu_si128(((__m128i*)dst) + 0, c1);
		card += 4;
		dst += 4;
		w -= 4;
	}
	for (; w > 0; card++, dst++, w--) {
		c1 = _mm_cvtsi32_si128(card[0]);
		c3 = _mm_cvtsi32_si128(dst[0]);
		c1 = _mm_blend_over_quad(c1, c3);
		dst[0] = _mm_cvtsi128_si32(c1);
	}
	return 0;
}



//---------------------------------------------------------------------
// interpolation routines
//---------------------------------------------------------------------

// color = (c1 * f1 + c2 * f2) >> 8
static bb_inline __m128i _mm_interp_row_quad(__m128i c1, __m128i c2, __m128i &f1, __m128i &f2)
{
	__m128i rb1 = _mm_pixel_rb(c1);
	__m128i rb2 = _mm_pixel_rb(c2);
	__m128i ag1 = _mm_pixel_ag(c1);
	__m128i ag2 = _mm_pixel_ag(c2);
	rb1 = _mm_mullo_epi16(rb1, f1);
	rb2 = _mm_mullo_epi16(rb2, f2);
	ag1 = _mm_mullo_epi16(ag1, f1);
	ag2 = _mm_mullo_epi16(ag2, f2);
	rb1 = _mm_srli_epi16(_mm_adds_epu16(rb1, rb2), 8);
	ag1 = _mm_srli_epi16(_mm_adds_epu16(ag1, ag2), 8);
	return _mm_or_si128(_mm_slli_epi32(ag1, 8), rb1);
}

static bb_inline __m128i _mm_interp_row_half(__m128i c1, __m128i c2)
{
	__m128i rb1 = _mm_pixel_rb(c1);
	__m128i rb2 = _mm_pixel_rb(c2);
	__m128i ag1 = _mm_pixel_ag(c1);
	__m128i ag2 = _mm_pixel_ag(c2);
	rb1 = _mm_srli_epi16(_mm_adds_epu16(rb1, rb2), 1);
	ag1 = _mm_srli_epi16(_mm_adds_epu16(ag1, ag2), 1);
	return _mm_or_si128(_mm_slli_epi32(ag1, 8), rb1);
}


// interpolate two rows
static int InterpRow_SSE(IUINT32 *card, int w, const IUINT32 *row1, 
		const IUINT32 *row2, IINT32 fraction)
{
	int _fraction = (fraction >> 8) & 0xff;
	__m128i f2 = _mm_set1_epi16(_fraction);
	__m128i f1 = _mm_set1_epi16(256 - _fraction);
	__m128i c1, c2, c3, c4;

	if (fraction == 0) {
		memcpy_fast(card, row1, w * 4);
		return 0;
	}

	if (_fraction == 0x80) {
		for (; w >= 8; card += 8, row1 += 8, row2 += 8, w -= 8) {
			c1 = _mm_loadu_si128(((const __m128i*)row1) + 0);
			c2 = _mm_loadu_si128(((const __m128i*)row2) + 0);
			c3 = _mm_loadu_si128(((const __m128i*)row1) + 1);
			c4 = _mm_loadu_si128(((const __m128i*)row2) + 1);
			c1 = _mm_interp_row_half(c1, c2);
			c3 = _mm_interp_row_half(c3, c4);
			_mm_storeu_si128(((__m128i*)card) + 0, c1);
			_mm_storeu_si128(((__m128i*)card) + 1, c3);
		}
		if (w & 4) {
			c1 = _mm_loadu_si128(((const __m128i*)row1) + 0);
			c2 = _mm_loadu_si128(((const __m128i*)row2) + 0);
			c1 = _mm_interp_row_half(c1, c2);
			_mm_storeu_si128(((__m128i*)card) + 0, c1);
			card += 4;
			row1 += 4;
			row2 += 4;
			w -= 4;
		}
		for (; w > 0; row1++, row2++, card++, w--) {
			c1 = _mm_cvtsi32_si128(row1[0]);
			c2 = _mm_cvtsi32_si128(row2[0]);
			c1 = _mm_interp_row_half(c1, c2);
			card[0] = _mm_cvtsi128_si32(c1);
		}
		return 0;
	}

	for (; w >= 8; card += 8, row1 += 8, row2 += 8, w -= 8) {
		c1 = _mm_loadu_si128(((const __m128i*)row1) + 0);
		c2 = _mm_loadu_si128(((const __m128i*)row2) + 0);
		c3 = _mm_loadu_si128(((const __m128i*)row1) + 1);
		c4 = _mm_loadu_si128(((const __m128i*)row2) + 1);
		c1 = _mm_interp_row_quad(c1, c2, f1, f2);
		c3 = _mm_interp_row_quad(c3, c4, f1, f2);
		_mm_storeu_si128(((__m128i*)card) + 0, c1);
		_mm_storeu_si128(((__m128i*)card) + 1, c3);
	}
	if (w & 4) {
		c1 = _mm_loadu_si128(((const __m128i*)row1) + 0);
		c2 = _mm_loadu_si128(((const __m128i*)row2) + 0);
		c1 = _mm_interp_row_quad(c1, c2, f1, f2);
		_mm_storeu_si128(((__m128i*)card) + 0, c1);
		card += 4;
		row1 += 4;
		row2 += 4;
		w -= 4;
	}
	for (; w > 0; row1++, row2++, card++, w--) {
		c1 = _mm_cvtsi32_si128(row1[0]);
		c2 = _mm_cvtsi32_si128(row2[0]);
		c1 = _mm_interp_row_quad(c1, c2, f1, f2);
		card[0] = _mm_cvtsi128_si32(c1);
	}
	return 0;
}


//---------------------------------------------------------------------
// AVX
//---------------------------------------------------------------------
#ifdef _COMPILE_WITH_AVX
static int TransparentBlit_Avx_32(void *dst, long dpitch, int dx, const void *src,
	long spitch, int sx, int w, int h, IUINT32 transparent, int flip)
{
	__m256 c1, c2, c3, c4, m1, m2, m3, m4, mask;

	if (flip != 0) return -1;

	mask = _mm256_castsi256_ps(_mm256_set1_epi32(transparent));

	for (int j = 0; j < h; j++) {
		const IUINT32 *ss = (const IUINT32*)src + sx;
		IUINT32 *dd = (IUINT32*)dst + dx;
		int width = w;

		_mm_prefetch((const char*)(ss), _MM_HINT_NTA);

		for (; width >= 32; width -= 32) {
			c1 = _mm256_castsi256_ps(_mm256_loadu_si256(((const __m256i*)ss) + 0));
			c2 = _mm256_castsi256_ps(_mm256_loadu_si256(((const __m256i*)ss) + 1));
			c3 = _mm256_castsi256_ps(_mm256_loadu_si256(((const __m256i*)ss) + 2));
			c4 = _mm256_castsi256_ps(_mm256_loadu_si256(((const __m256i*)ss) + 3));
			_mm_prefetch((const char*)(ss + 128), _MM_HINT_NTA);
			ss += 32;
			m1 = _mm256_cmp_ps(c1, mask, _CMP_NEQ_UQ);
			m2 = _mm256_cmp_ps(c2, mask, _CMP_NEQ_UQ);
			m3 = _mm256_cmp_ps(c3, mask, _CMP_NEQ_UQ);
			m4 = _mm256_cmp_ps(c4, mask, _CMP_NEQ_UQ);
			_mm256_maskstore_ps(((float*)dd) + 0, _mm256_castps_si256(m1), c1);
			_mm256_maskstore_ps(((float*)dd) + 8, _mm256_castps_si256(m2), c2);
			_mm256_maskstore_ps(((float*)dd) + 16, _mm256_castps_si256(m3), c3);
			_mm256_maskstore_ps(((float*)dd) + 24, _mm256_castps_si256(m4), c4);
			dd += 32;
		}

		for (; width >= 8; width -= 8) {
			c1 = _mm256_castsi256_ps(_mm256_loadu_si256(((const __m256i*)ss) + 0));
			ss += 8;
			m1 = _mm256_cmp_ps(c1, mask, _CMP_NEQ_UQ);
			_mm256_maskstore_ps(((float*)dd) + 0, _mm256_castps_si256(m1), c1);
			dd += 8;
		}

		_mm_sfence();
		
		for (; width > 0; ss++, dd++, width--) {
			if (ss[0] != transparent) dd[0] = ss[0];
		}

		dst = (char*)dst + dpitch;
		src = (const char*)src + spitch;
	}
	return 0;
}

#endif



//---------------------------------------------------------------------
// Resample
//---------------------------------------------------------------------
static int Resample_ShrinkX_SSE2(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth, void *workmem)
{
    IINT32 srcdiff = srcpitch - (srcwidth * 4);
    IINT32 dstdiff = dstpitch - (dstwidth * 4);

    IINT32 xspace = 0x04000 * srcwidth / dstwidth; /* must be > 1 */
    IINT32 xrecip = 0x40000000 / xspace;

	__m128i zero = _mm_setzero_si128();			// pxor mm0, mm0
	__m128i one = _mm_set1_epi32(0x40004000);	// mm6 = one64
	__m128i xmm7 = _mm_cvtsi32_si128(xrecip);	// mm7 = xrecip
	xmm7 = _mm_shufflelo_epi16(xmm7, 0);			
	for (int y = 0; y < height; y++) {
		IINT32 counter = xspace;				// ecx = counter
		__m128i accumulator = _mm_setzero_si128();	// accumulator(mm1)=0
		__m128i xmm2, xmm3, xmm4;
		for (int x = srcwidth; x > 0; x--) {
			if (counter > 0x4000) {
				xmm2 = _mm_cvtsi32_si128(*(IUINT32*)srcpix);
				srcpix += 4;
				xmm2 = _mm_unpacklo_epi8(xmm2, zero);		// punpcklbw mm2, mm0
				accumulator = _mm_add_epi16(accumulator, xmm2);	// paddw mm1, mm2
				counter -= 0x4000;				// sub ecx, 0x4000
			}
			else {
				xmm2 = _mm_cvtsi32_si128(counter);	// mm2 = ecx
				xmm2 = _mm_shufflelo_epi16(xmm2, 0);		// pshufw mm2, mm2, 0
				xmm4 = _mm_cvtsi32_si128(*(IUINT32*)srcpix);	// movd mm4, [srcpix]
				srcpix += 4;
				xmm4 = _mm_unpacklo_epi8(xmm4, zero);	// punpcklbw mm4, mm0
				xmm3 = _mm_sub_epi16(one, xmm2);		
				xmm4 = _mm_slli_epi16(xmm4, 2);
				xmm2 = _mm_mulhi_epu16(xmm2, xmm4);
				xmm3 = _mm_mulhi_epu16(xmm3, xmm4);
				xmm2 = _mm_add_epi16(xmm2, accumulator);
				accumulator = xmm3;
				xmm2 = _mm_mulhi_epu16(xmm2, xmm7);
				xmm2 = _mm_packus_epi16(xmm2, zero);
				*((IINT32*)dstpix) = _mm_cvtsi128_si32(xmm2);
				counter += xspace;
				dstpix += 4;
				counter -= 0x4000;
			}
		}
		srcpix += srcdiff;
		dstpix += dstdiff;
	}

	return 0;
}


static int Resample_ShrinkY_SSE2(IUINT8 *dstpix, const IUINT8 *srcpix,
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight,
	void *workmem)
{
    IUINT16 *templine;
    IINT32 srcdiff = srcpitch - (width * 4);
    IINT32 dstdiff = dstpitch - (width * 4);
    IINT32 yspace = 0x4000 * srcheight / dstheight; /* must be > 1 */
    IINT32 yrecip = 0x40000000 / yspace;

    templine = (IUINT16*)workmem;
    memset(templine, 0, width * 8);

	__m128i one = _mm_set1_epi32(0x40004000);	// mm0
	__m128i zero = _mm_setzero_si128();			// mm0=0
	IINT32 counter = yspace;
	__m128i xmm7 = _mm_cvtsi32_si128(yrecip);	// mm7=yrecip
	xmm7 = _mm_shufflelo_epi16(xmm7, 0);
	xmm7 = _mm_or_si128(_mm_slli_si128(xmm7, 8), xmm7);
	int x, y;

	for (y = srcheight; y > 0; y--) {
		IUINT16 *accumulate = templine;	// eax = accumulate
		if (counter > 0x4000) {
			__m128i xmm1, xmm2;
			for (x = width; x >= 2; x -= 2) {
				xmm1 = _mm_loadl_epi64((__m128i*)srcpix);
				srcpix += 8;
				xmm2 = _mm_loadu_si128((__m128i*)accumulate);
				xmm1 = _mm_unpacklo_epi8(xmm1, zero);
				xmm2 = _mm_add_epi16(xmm2, xmm1);
				_mm_storeu_si128((__m128i*)accumulate, xmm2);
				accumulate += 8;
			}
			for (; x > 0; x--) {
				xmm1 = _mm_cvtsi32_si128(*(IUINT32*)srcpix);
				srcpix += 4;
				xmm2 = _mm_loadl_epi64((__m128i*)accumulate);
				xmm1 = _mm_unpacklo_epi8(xmm1, zero);
				xmm2 = _mm_add_epi16(xmm2, xmm1);
				_mm_storel_epi64((__m128i*)accumulate, xmm2);
				accumulate += 4;
			}
			counter -= 0x4000;
		}
		else {
			__m128i xmm1 = _mm_cvtsi32_si128(counter);
			xmm1 = _mm_shufflelo_epi16(xmm1, 0);
			xmm1 = _mm_or_si128(_mm_slli_si128(xmm1, 8), xmm1);
			__m128i xmm6 = _mm_sub_epi16(one, xmm1);
			__m128i xmm3, xmm5, xmm4;
			for (x = width; x >= 2; x -= 2) {
				xmm4 = _mm_loadl_epi64((__m128i*)srcpix);
				srcpix += 8;
				xmm4 = _mm_unpacklo_epi8(xmm4, zero);
				xmm5 = _mm_loadu_si128((__m128i*)accumulate);
				xmm3 = xmm6;
				xmm4 = _mm_slli_epi16(xmm4, 2);
				xmm3 = _mm_mulhi_epu16(xmm3, xmm4);
				xmm4 = _mm_mulhi_epu16(xmm4, xmm1);
				_mm_storeu_si128((__m128i*)accumulate, xmm3);
				xmm4 = _mm_add_epi16(xmm4, xmm5);
				accumulate += 8;
				xmm4 = _mm_mulhi_epu16(xmm4, xmm7);
				xmm4 = _mm_packus_epi16(xmm4, zero);
				_mm_storel_epi64((__m128i*)dstpix, xmm4);
				dstpix += 8;
			}
			for (; x > 0; x--) {
				xmm4 = _mm_cvtsi32_si128(*(IUINT32*)srcpix);
				srcpix += 4;
				xmm4 = _mm_unpacklo_epi8(xmm4, zero);
				xmm5 = _mm_loadl_epi64((__m128i*)accumulate);
				xmm3 = xmm6;
				xmm4 = _mm_slli_epi16(xmm4, 2);
				xmm3 = _mm_mulhi_epu16(xmm3, xmm4);
				xmm4 = _mm_mulhi_epu16(xmm4, xmm1);
				_mm_storel_epi64((__m128i*)accumulate, xmm3);
				xmm4 = _mm_add_epi16(xmm4, xmm5);
				accumulate += 4;
				xmm4 = _mm_mulhi_epu16(xmm4, xmm7);
				xmm4 = _mm_packus_epi16(xmm4, zero);
				*((IINT32*)dstpix) = _mm_cvtsi128_si32(xmm4);
				dstpix += 4;
			}
			dstpix += dstdiff;
			counter += yspace - 0x4000;
		}
		srcpix += srcdiff;
	}

	return 0;
}

static int Resample_ExpandX_SSE2(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth,
	void *workmem)
{
    IINT32 *xidx0, *xmult0, *xmult1;
    IINT32 x, y;

	xidx0 = (IINT32*)workmem;
	xmult0 = xidx0 + dstwidth;
	xmult1 = xmult0 + dstwidth * 2;

    for (x = 0; x < dstwidth; x++) {
        IINT32 xm1 = 0x100 * ((x * (srcwidth - 1)) % dstwidth) / dstwidth;
        IINT32 xm0 = 0x100 - xm1;
        xidx0[x] = x * (srcwidth - 1) / dstwidth;
        xmult1[x * 2]   = xm1 | (xm1 << 16);
        xmult1[x * 2 + 1] = xm1 | (xm1 << 16);
        xmult0[x * 2]   = xm0 | (xm0 << 16);
        xmult0[x * 2 + 1] = xm0 | (xm0 << 16);
    }

    for (y = 0; y < height; y++) {
        const IUINT8 *srcrow = srcpix + y * srcpitch;
        IUINT8 *dstrow = dstpix + y * dstpitch;
        IINT32 *xm0 = xmult0;
        IINT32 *x0 = xidx0;
		__m128i zero = _mm_setzero_si128();
		__m128i one = _mm_set1_epi32(0x01000100);	// xmm7 = one
		__m128i xmm1, xmm2, xmm4, xmm5;
		for (x = dstwidth; x > 0; x--) {
			IINT32 index = *x0++;
			xmm2 = one;
			xmm1 = _mm_loadl_epi64((__m128i*)xm0);
			xm0 += 2;
			xmm2 = _mm_sub_epi16(xmm2, xmm1);
			xmm4 = _mm_cvtsi32_si128(*((IUINT32*)(srcrow + index * 4)));
			xmm5 = _mm_cvtsi32_si128(*((IUINT32*)(srcrow + index * 4 + 4)));
			xmm4 = _mm_unpacklo_epi8(xmm4, zero);
			xmm5 = _mm_unpacklo_epi8(xmm5, zero);
			xmm4 = _mm_mullo_epi16(xmm4, xmm1);
			xmm5 = _mm_mullo_epi16(xmm5, xmm2);
			xmm5 = _mm_add_epi16(xmm5, xmm4);
			xmm5 = _mm_packus_epi16(_mm_srli_epi16(xmm5, 8), zero);
			*((IUINT32*)dstrow) = _mm_cvtsi128_si32(xmm5);
			dstrow += 4;
		}
    }

	return 0;
}

static int Resample_ExpandY_SSE2(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight,
	void *workmem)
{
    IINT32 x, y;

    for (y = 0; y < dstheight; y++) {
        IINT32 yidx0 = y * (srcheight - 1) / dstheight;
        const IUINT8 *srcrow0 = srcpix + yidx0 * srcpitch;
        const IUINT8 *srcrow1 = srcrow0 + srcpitch;
        IINT32 ymult1 = 0x0100 * ((y * (srcheight - 1)) % dstheight) / dstheight;
        IINT32 ymult0 = 0x0100 - ymult1;
        IUINT8 *dstrow = dstpix + y * dstpitch;

		__m128i zero = _mm_setzero_si128();		// xmm0
		__m128i xmm1 = _mm_cvtsi32_si128(ymult0);
		__m128i xmm2 = _mm_cvtsi32_si128(ymult1);

		xmm1 = _mm_shufflelo_epi16(xmm1, 0);
		xmm2 = _mm_shufflelo_epi16(xmm2, 0);
		xmm1 = _mm_or_si128(_mm_slli_si128(xmm1, 8), xmm1);
		xmm2 = _mm_or_si128(_mm_slli_si128(xmm2, 8), xmm2);

		__m128i xmm4, xmm5;

		for (x = width; x >= 2; x -= 2) {
			xmm4 = _mm_loadl_epi64((__m128i*)srcrow0);
			xmm5 = _mm_loadl_epi64((__m128i*)srcrow1);
			srcrow0 += 8;
			srcrow1 += 8;
			xmm4 = _mm_unpacklo_epi8(xmm4, zero);
			xmm5 = _mm_unpacklo_epi8(xmm5, zero);
			xmm4 = _mm_mullo_epi16(xmm4, xmm1);
			xmm5 = _mm_mullo_epi16(xmm5, xmm2);
			xmm5 = _mm_add_epi16(xmm5, xmm4);
			xmm5 = _mm_packus_epi16(_mm_srli_epi16(xmm5, 8), zero);
			_mm_storel_epi64((__m128i*)dstrow, xmm5);
			dstrow += 8;
		}

		for (; x > 0; x--) {
			xmm4 = _mm_cvtsi32_si128(*(IUINT32*)srcrow0);
			xmm5 = _mm_cvtsi32_si128(*(IUINT32*)srcrow1);
			srcrow0 += 4;
			srcrow1 += 4;
			xmm4 = _mm_unpacklo_epi8(xmm4, zero);
			xmm5 = _mm_unpacklo_epi8(xmm5, zero);
			xmm4 = _mm_mullo_epi16(xmm4, xmm1);
			xmm5 = _mm_mullo_epi16(xmm5, xmm2);
			xmm5 = _mm_add_epi16(xmm5, xmm4);
			xmm5 = _mm_packus_epi16(_mm_srli_epi16(xmm5, 8), zero);
			*(IUINT32*)dstrow = _mm_cvtsi128_si32(xmm5);
			dstrow += 4;
		}
    }

	return 0;
}


//---------------------------------------------------------------------
// CPU Feature
//---------------------------------------------------------------------
#ifdef __GNUC__
#define __x86_cpuid(level, a, b, c, d)			\
	__asm__("\tcpuid;\n" :"=a"(a), "=b"(b), "=c"(c), "=d"(d) :"0"(level))
#define __x86_cpuid_count(level, count, a, b, c, d)		\
	__asm__("\tcpuid;\n" :"=a"(a), "=b"(b), "=c"(c), "=d"(d) :"0"(level), "2"(count))

static unsigned int _x86_cpuid_max(unsigned int __ext, unsigned int *__sig) 
{
	unsigned int __eax, __ebx, __ecx, __edx;
#if (!defined(__x86_64__)) && (!defined(__amd64__)) && (!defined(_M_X64))
	#if __GNUC__ >= 3
	__asm__ ("pushf{l|d}\n\t"
		"pushf{l|d}\n\t"
		"pop{l}\t%0\n\t"
		"mov{l}\t{%0, %1|%1, %0}\n\t"
		"xor{l}\t{%2, %0|%0, %2}\n\t"
		"push{l}\t%0\n\t"
		"popf{l|d}\n\t"
		"pushf{l|d}\n\t"
		"pop{l}\t%0\n\t"
		"popf{l|d}\n\t"
		: "=&r" (__eax), "=&r" (__ebx)
		: "i" (0x00200000));
	#else
	__asm__ ("pushfl\n\t"
		"pushfl\n\t"
		"popl\t%0\n\t"
		"movl\t%0, %1\n\t"
		"xorl\t%2, %0\n\t"
		"pushl\t%0\n\t"
		"popfl\n\t"
		"pushfl\n\t"
		"popl\t%0\n\t"
		"popfl\n\t"
		: "=&r" (__eax), "=&r" (__ebx)
		: "i" (0x00200000));
	#endif
	if (!((__eax ^ __ebx) & 0x00200000)) return 0;
#endif
	__x86_cpuid (__ext, __eax, __ebx, __ecx, __edx);
	if (__sig) *__sig = __ebx;
	return __eax;
}

static int _x86_cpuid(int op, int *eax, int *ebx, int *ecx, int *edx) 
{
	unsigned int __ext = op & 0x80000000;
	if (_x86_cpuid_max(__ext, 0) < (unsigned int)op) return -1;
	__x86_cpuid(op, *eax, *ebx, *ecx, *edx);
	return 0;
}

#else
#include <intrin.h>
static int _x86_cpuid(int op, int *eax, int *ebx, int *ecx, int *edx)
{
	int regs[4];
	regs[0] = *eax;
	regs[1] = *ebx;
	regs[2] = *ecx;
	regs[3] = *edx;
	__cpuid(regs, op);
	*eax = regs[0];
	*ebx = regs[1];
	*ecx = regs[2];
	*edx = regs[3];
	return 0;
}
#endif

// detect feature
static int _x86_cpu_detect_feature(unsigned int *features)
{
	int eax = 0, ebx = 0, ecx = 0, edx = 0;
	int level = 0;
	features[0] = features[1] = features[2] = features[3] = 0;
	features[4] = features[5] = features[6] = features[7] = 0;
	if (_x86_cpuid(0, &eax, &ebx, &ecx, &edx) != 0) return -1;
	level = eax;
#if (!defined(__x86_64__)) && (!defined(__amd64__)) && (!defined(_M_X64))
	if (level < 1) level = 1;
#endif
	if (level >= 1) {
		eax = 1;
		ebx = ecx = edx = 0;
		_x86_cpuid(1, &eax, &ebx, &ecx, &edx);
		features[0] = (unsigned int)edx;
		eax = 0x80000000uL;
		_x86_cpuid(0x80000000uL, &eax, &ebx, &ecx, &edx);
		if ((unsigned int)eax != 0x80000000ul) {
			eax = 0x80000001uL;
			ebx = ecx = edx = 0;
			_x86_cpuid(0x80000001uL, &eax, &ebx, &ecx, &edx);
			features[1] = (unsigned int)edx;
		}
	}
#if (!defined(__x86_64__)) && (!defined(__amd64__)) && (!defined(_M_X64))
	features[0] |= (1 << 23) | (1 << 25);
#endif
	return level;
}


// detect bits
static unsigned int _x86_cpu_feature(int nbit)
{
	static unsigned int features[8];
	static int inited = 0;
	if (inited == 0) {
		_x86_cpu_detect_feature(features);
		inited = 1;
	}
	return features[nbit / 32] & (1ul << (nbit & 31));
}


//---------------------------------------------------------------------
// external interfaces
//---------------------------------------------------------------------
extern void *(*_internal_hook_memcpy)(void *dst, const void *src, size_t n);
extern void BasicBitmap_ResampleDriver(int id, void *ptr);


//---------------------------------------------------------------------
// sse2 initialize
//---------------------------------------------------------------------
int BasicBitmap_SSE2_AVX_Enable()
{
	static int initialized = 0;
	if (initialized != 0)
		return initialized;

	int result = 0;
	if (_x86_cpu_feature(26)) {
		_internal_hook_memcpy = memcpy_fast;

		BasicBitmap::SetDriver(32, NormalBlit_32, false);
		BasicBitmap::SetDriver(24, NormalBlit_24, false);
		BasicBitmap::SetDriver(16, NormalBlit_16, false);
		BasicBitmap::SetDriver(8, NormalBlit_8, false);
		BasicBitmap::SetDriver(32, TransparentBlit_32, true);
		BasicBitmap::SetDriver(16, TransparentBlit_16, true);
		BasicBitmap::SetDriver(8, TransparentBlit_8, true);

		BasicBitmap::SetDriver(BasicBitmap::X8R8G8B8, PixelDraw_SSE2_BLEND_X8R8G8B8, 0);

		BasicBitmap::SetDriver(BasicBitmap::A8R8G8B8, PixelDraw_SSE2_SRCOVER_A8R8G8B8, 1);
		BasicBitmap::SetDriver(BasicBitmap::X8R8G8B8, PixelDraw_SSE2_SRCOVER_A8R8G8B8, 1);

		BasicBitmap::SetDriver(InterpRow_SSE);

		BasicBitmap_ResampleDriver(0, (void*)Resample_ShrinkX_SSE2);
		BasicBitmap_ResampleDriver(1, (void*)Resample_ShrinkY_SSE2);
		BasicBitmap_ResampleDriver(3, (void*)Resample_ExpandX_SSE2);
		BasicBitmap_ResampleDriver(4, (void*)Resample_ExpandY_SSE2);

		result |= 1;
	}

#ifdef _COMPILE_WITH_AVX
	if (_x86_cpu_feature(28)) {
		BasicBitmap::SetDriver(32, TransparentBlit_Avx_32, true);
		result |= 2;
	}
#endif

	initialized = result;
	return result;
}

#else

int BasicBitmap_SSE2_AVX_Enable()
{
	return 0;
}

#endif
