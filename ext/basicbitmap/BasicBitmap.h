//=====================================================================
//
// BasicBitmap.h - Simple Bitmap Library
// https://github.com/skywind3000/BasicBitmap
//
// Created by: skywind, 2011, 2012, 2015, 2018
// Last Modified: 2018/03/15 14:54
//
// As the purpose of providing a simple, high-performance and platform
// independent Bitmap library, this file is created based on a subset
// of my vector graphic library: https://code.google.com/p/pixellib
// 
// FEATURES:
//  - common pixel format supported (from A8R8G8B8 to A4R4G4B4)
//  - blitting with or without a transparent color
//  - converting between different pixel formats
//  - loading bmp/tga from memory or file and saving bmp to file
//  - loading png/jpg with gdiplus (only in windows xp or above)
//  - blending with different compositor
//  - scaling with different filters (nearest, linear, bilinear)
//
// As a platform independent implementation, this class is written 
// in pure C/C++. But all the core routines can be replaced by
// external implementations (sse2 eg.) using SetDriver/SetFunction.
//
// HISTORY:
// 2011.2.9   skywind  create this file based on a subset of pixellib
// 2011.2.11  skywind  immigrate blitting/blending/convertion/scaling
// 2011.2.13  skywind  immigrate tga/bmp loader
//
//=====================================================================
#ifndef __BASIC_BITMAP_H__
#define __BASIC_BITMAP_H__

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef __cplusplus
#error This file must be compiled in C++ mode !!
#endif


//=====================================================================
// DEFINITION OF INTEGERS
//=====================================================================
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(__UINT32_TYPE__) && defined(__UINT32_TYPE__)
	typedef __UINT32_TYPE__ ISTDUINT32;
	typedef __INT32_TYPE__ ISTDINT32;
#elif defined(__UINT_FAST32_TYPE__) && defined(__INT_FAST32_TYPE__)
	typedef __UINT_FAST32_TYPE__ ISTDUINT32;
	typedef __INT_FAST32_TYPE__ ISTDINT32;
#elif defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
	defined(_M_AMD64)
	typedef unsigned int ISTDUINT32;
	typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
	defined(__i386) || defined(_M_X86)
	typedef unsigned long ISTDUINT32;
	typedef long ISTDINT32;
#elif defined(__MACOS__)
	typedef UInt32 ISTDUINT32;
	typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
	#include <sys/types.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
	#include <sys/inttypes.h>
	typedef u_int32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
	typedef unsigned __int32 ISTDUINT32;
	typedef __int32 ISTDINT32;
#elif defined(__GNUC__) && (__GNUC__ > 3)
	#include <stdint.h>
	typedef uint32_t ISTDUINT32;
	typedef int32_t ISTDINT32;
#else 
	typedef unsigned long ISTDUINT32; 
	typedef long ISTDINT32;
#endif
#endif

#ifndef __IINT8_DEFINED
    #define __IINT8_DEFINED
    typedef signed char IINT8;
#endif

#ifndef __IUINT8_DEFINED
    #define __IUINT8_DEFINED
    typedef unsigned char IUINT8;
#endif

#ifndef __IUINT16_DEFINED
    #define __IUINT16_DEFINED
    typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
    #define __IINT16_DEFINED
    typedef signed short IINT16;
#endif

#ifndef __IINT32_DEFINED
    #define __IINT32_DEFINED
    typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
    #define __IUINT32_DEFINED
    typedef ISTDUINT32 IUINT32;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 IUINT64;
#else
typedef unsigned long long IUINT64;
#endif
#endif


//=====================================================================
// DETECTION WORD ORDER
//=====================================================================
#ifndef IWORDS_BIG_ENDIAN
    #ifdef _BIG_ENDIAN_
        #if _BIG_ENDIAN_
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MISPEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
            #define IWORDS_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef IWORDS_BIG_ENDIAN
        #define IWORDS_BIG_ENDIAN  0
    #endif
#endif

#ifdef __MSDOS__
	#error This file cannot be compiled in 16 bits (only 32, 64 bits)
#endif


//---------------------------------------------------------------------
// inline definition
//---------------------------------------------------------------------
#ifndef INLINE
#if defined(__GNUC__)

#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#if (!defined(bb_inline))
#define bb_inline INLINE
#endif


//=====================================================================
// Pixel Access
//=====================================================================
#define _pixel_fetch_8(ptr, offset)  (((const IUINT8 *)(ptr))[offset])
#define _pixel_fetch_16(ptr, offset) (((const IUINT16*)(ptr))[offset])
#define _pixel_fetch_32(ptr, offset) (((const IUINT32*)(ptr))[offset])

#define _pixel_fetch_24_lsb(ptr, offset) \
    ( (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[0]) /*<<  0*/ ) | \
      (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[1]) <<  8 ) | \
      (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[2]) << 16 ))

#define _pixel_fetch_24_msb(ptr, offset) \
    ( (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[0]) << 16 ) | \
      (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[1]) <<  8 ) | \
      (((IUINT32)(((const IUINT8*)(ptr)) + (offset) * 3)[2]) /*<<  0*/ ))

#define _pixel_store_8(ptr, off, c)  (((IUINT8 *)(ptr))[off] = (IUINT8)(c))
#define _pixel_store_16(ptr, off, c) (((IUINT16*)(ptr))[off] = (IUINT16)(c))
#define _pixel_store_32(ptr, off, c) (((IUINT32*)(ptr))[off] = (IUINT32)(c))

#define _pixel_store_24_lsb(ptr, off, c) do { \
        ((IUINT8*)(ptr))[(off) * 3 + 0] = (IUINT8) (((c) /*>>  0*/) & 0xff); \
        ((IUINT8*)(ptr))[(off) * 3 + 1] = (IUINT8) (((c) >>  8) & 0xff); \
        ((IUINT8*)(ptr))[(off) * 3 + 2] = (IUINT8) (((c) >> 16) & 0xff); \
    }   while (0)

#define _pixel_store_24_msb(ptr, off, c)  do { \
        ((IUINT8*)(ptr))[(off) * 3 + 0] = (IUINT8) (((c) >> 16) & 0xff); \
        ((IUINT8*)(ptr))[(off) * 3 + 1] = (IUINT8) (((c) >>  8) & 0xff); \
        ((IUINT8*)(ptr))[(off) * 3 + 2] = (IUINT8) (((c) /*>>  0*/) & 0xff); \
    }   while (0)

#define _pixel_load_card_lsb(ptr, r, g, b, a) do { \
			(a) = ((IUINT8*)(ptr))[3]; \
			(r) = ((IUINT8*)(ptr))[2]; \
			(g) = ((IUINT8*)(ptr))[1]; \
			(b) = ((IUINT8*)(ptr))[0]; \
		} while (0)

#define _pixel_load_card_msb(ptr, r, g, b, a) do { \
			(a) = ((IUINT8*)(ptr))[0]; \
			(r) = ((IUINT8*)(ptr))[1]; \
			(g) = ((IUINT8*)(ptr))[2]; \
			(b) = ((IUINT8*)(ptr))[3]; \
		} while (0)

#define _pixel_save_card_lsb(ptr, r, g, b, a) do { \
			((IUINT8*)(ptr))[3] = (IUINT8)(a); \
			((IUINT8*)(ptr))[2] = (IUINT8)(r); \
			((IUINT8*)(ptr))[1] = (IUINT8)(g); \
			((IUINT8*)(ptr))[0] = (IUINT8)(b); \
		} while (0)

#define _pixel_save_card_msb(ptr, r, g, b, a) do { \
			((IUINT8*)(ptr))[0] = (IUINT8)(a); \
			((IUINT8*)(ptr))[1] = (IUINT8)(r); \
			((IUINT8*)(ptr))[2] = (IUINT8)(g); \
			((IUINT8*)(ptr))[3] = (IUINT8)(b); \
		} while (0)

#define _pixel_read_24_lsb(a) ( \
	(((IUINT32)(((const unsigned char*)(a))[0]))      ) | \
	(((IUINT32)(((const unsigned char*)(a))[1])) <<  8) | \
	(((IUINT32)(((const unsigned char*)(a))[2])) << 16) )

#define _pixel_read_24_msb(a) ( \
	(((IUINT32)(((const unsigned char*)(a))[0])) << 16) | \
	(((IUINT32)(((const unsigned char*)(a))[1])) <<  8) | \
	(((IUINT32)(((const unsigned char*)(a))[2]))      ) )

#define _pixel_write_24_lsb(a, c) do { \
		((unsigned char*)(a))[0] = (IUINT8)(((c)      ) & 0xff); \
		((unsigned char*)(a))[1] = (IUINT8)(((c) >>  8) & 0xff); \
		((unsigned char*)(a))[2] = (IUINT8)(((c) >> 16) & 0xff); \
	}	while (0)

#define _pixel_write_24_msb(a, c) do { \
		((unsigned char*)(a))[0] = (IUINT8)(((c) >> 16) & 0xff); \
		((unsigned char*)(a))[1] = (IUINT8)(((c) >>  8) & 0xff); \
		((unsigned char*)(a))[2] = (IUINT8)(((c)      ) & 0xff); \
	}	while (0)


#if IWORDS_BIG_ENDIAN
#define _pixel_fetch_24 _pixel_fetch_24_msb
#define _pixel_store_24 _pixel_store_24_msb
#define _pixel_load_card _pixel_load_card_msb
#define _pixel_save_card _pixel_save_card_msb
#define _pixel_read_24 _pixel_read_24_msb
#define _pixel_write_24 _pixel_write_24_msb
#define _pixel_card_alpha	0
#else
#define _pixel_fetch_24 _pixel_fetch_24_lsb
#define _pixel_store_24 _pixel_store_24_lsb
#define _pixel_load_card _pixel_load_card_lsb
#define _pixel_save_card _pixel_save_card_lsb
#define _pixel_read_24 _pixel_read_24_lsb
#define _pixel_write_24 _pixel_write_24_lsb
#define _pixel_card_alpha	3
#endif

#define _pixel_fetch(nbits, ptr, off) _pixel_fetch_##nbits(ptr, off)
#define _pixel_store(nbits, ptr, off, c) _pixel_store_##nbits(ptr, off, c)

#define _pixel_norm(color) (((color) >> 7) + (color))
#define _pixel_unnorm(color) ((((color) << 8) - (color)) >> 8)
#define _pixel_y_div_255(x, y) (((x) * _pixel_norm(y)) >> 8)
#define _pixel_fast_div_255(x) (((x) + (((x) + 257) >> 8)) >> 8)

#define _pixel_to_gray(r, g, b) \
        ((19595 * (r) + 38469 * (g) + 7472 * (b)) >> 16)


//---------------------------------------------------------------------
// MACRO: Pixel Assembly & Disassembly 
//---------------------------------------------------------------------
extern const IUINT32 _pixel_scale_1[2];
extern const IUINT32 _pixel_scale_2[4];
extern const IUINT32 _pixel_scale_3[8];
extern const IUINT32 _pixel_scale_4[16];
extern const IUINT32 _pixel_scale_5[32];
extern const IUINT32 _pixel_scale_6[64];

/* assembly 32 bits */
#define _pixel_asm_8888(a, b, c, d) ((IUINT32)( \
            ((IUINT32)(a) << 24) | \
            ((IUINT32)(b) << 16) | \
            ((IUINT32)(c) <<  8) | \
            ((IUINT32)(d) /*<<  0*/)))

/* assembly 24 bits */
#define _pixel_asm_888(a, b, c) ((IUINT32)( \
            ((IUINT32)(a) << 16) | \
            ((IUINT32)(b) <<  8) | \
            ((IUINT32)(c) /*<<  0*/)))

/* assembly 16 bits */
#define _pixel_asm_1555(a, b, c, d) ((IUINT16)( \
            ((IUINT16)((a) & 0x80) << 8) | \
            ((IUINT16)((b) & 0xf8) << 7) | \
            ((IUINT16)((c) & 0xf8) << 2) | \
            ((IUINT16)((d) & 0xf8) >> 3)))

#define _pixel_asm_5551(a, b, c, d) ((IUINT16)( \
            ((IUINT16)((a) & 0xf8) << 8) | \
            ((IUINT16)((b) & 0xf8) << 3) | \
            ((IUINT16)((c) & 0xf8) >> 2) | \
            ((IUINT16)((d) & 0x80) >> 7)))

#define _pixel_asm_565(a, b, c)  ((IUINT16)( \
            ((IUINT16)((a) & 0xf8) << 8) | \
            ((IUINT16)((b) & 0xfc) << 3) | \
            ((IUINT16)((c) & 0xf8) >> 3)))

#define _pixel_asm_4444(a, b, c, d) ((IUINT16)( \
            ((IUINT16)((a) & 0xf0) << 8) | \
            ((IUINT16)((b) & 0xf0) << 4) | \
            ((IUINT16)((c) & 0xf0) /*<< 0*/) | \
            ((IUINT16)((d) & 0xf0) >> 4)))

/* disassembly 32 bits */
#define _pixel_disasm_8888(x, a, b, c, d) do { \
            (a) = ((x) >> 24) & 0xff; \
            (b) = ((x) >> 16) & 0xff; \
            (c) = ((x) >>  8) & 0xff; \
            (d) = ((x) /*>>  0*/) & 0xff; \
        }   while (0)

#define _pixel_disasm_888X(x, a, b, c) do { \
            (a) = ((x) >> 24) & 0xff; \
            (b) = ((x) >> 16) & 0xff; \
            (c) = ((x) >>  8) & 0xff; \
        }   while (0)

#define _pixel_disasm_X888(x, a, b, c) do { \
            (a) = ((x) >> 16) & 0xff; \
            (b) = ((x) >>  8) & 0xff; \
            (c) = ((x) /*>>  0*/) & 0xff; \
        }   while (0)

/* disassembly 24 bits */
#define _pixel_disasm_888(x, a, b, c) do { \
            (a) = ((x) >> 16) & 0xff; \
            (b) = ((x) >>  8) & 0xff; \
            (c) = ((x) /*>>  0*/) & 0xff; \
        }   while (0)

/* disassembly 16 bits */
#define _pixel_disasm_1555(x, a, b, c, d) do { \
            (a) = _pixel_scale_1[(x) >> 15]; \
            (b) = _pixel_scale_5[((x) >> 10) & 0x1f]; \
            (c) = _pixel_scale_5[((x) >>  5) & 0x1f]; \
            (d) = _pixel_scale_5[((x) /*>>  0*/) & 0x1f]; \
        }   while (0)

#define _pixel_disasm_X555(x, a, b, c) do { \
            (a) = _pixel_scale_5[((x) >> 10) & 0x1f]; \
            (b) = _pixel_scale_5[((x) >>  5) & 0x1f]; \
            (c) = _pixel_scale_5[((x) /*>>  0*/) & 0x1f]; \
        }   while (0)

#define _pixel_disasm_5551(x, a, b, c, d) do { \
            (a) = _pixel_scale_5[((x) >> 11) & 0x1f]; \
            (b) = _pixel_scale_5[((x) >>  6) & 0x1f]; \
            (c) = _pixel_scale_5[((x) >>  1) & 0x1f]; \
            (d) = _pixel_scale_1[((x) /*>>  0*/) & 0x01]; \
        }   while (0)

#define _pixel_disasm_555X(x, a, b, c) do { \
            (a) = _pixel_scale_5[((x) >> 11) & 0x1f]; \
            (b) = _pixel_scale_5[((x) >>  6) & 0x1f]; \
            (c) = _pixel_scale_5[((x) >>  1) & 0x1f]; \
        }   while (0)

#define _pixel_disasm_565(x, a, b, c) do { \
            (a) = _pixel_scale_5[((x) >> 11) & 0x1f]; \
            (b) = _pixel_scale_6[((x) >>  5) & 0x3f]; \
            (c) = _pixel_scale_5[((x) /*>>  0*/) & 0x1f]; \
        }   while (0)

#define _pixel_disasm_4444(x, a, b, c, d) do { \
            (a) = _pixel_scale_4[((x) >> 12) & 0xf]; \
            (b) = _pixel_scale_4[((x) >>  8) & 0xf]; \
            (c) = _pixel_scale_4[((x) >>  4) & 0xf]; \
            (d) = _pixel_scale_4[((x) /*>>  0*/) & 0xf]; \
        }   while (0)

#define _pixel_disasm_X444(x, a, b, c) do { \
            (a) = _pixel_scale_4[((x) >>  8) & 0xf]; \
            (b) = _pixel_scale_4[((x) >>  4) & 0xf]; \
            (c) = _pixel_scale_4[((x) /*>>  0*/) & 0xf]; \
        }   while (0)

#define _pixel_disasm_444X(x, a, b, c) do { \
            (a) = _pixel_scale_4[((x) >> 12) & 0xf]; \
            (b) = _pixel_scale_4[((x) >>  8) & 0xf]; \
            (c) = _pixel_scale_4[((x) >>  4) & 0xf]; \
        }   while (0)


//---------------------------------------------------------------------
// loop unroll double 
//---------------------------------------------------------------------
#ifndef ILINS_LOOP_DOUBLE
#define ILINS_LOOP_DOUBLE(actionx1, actionx2, width) do { \
	const unsigned long __width = (unsigned long)(width); \
	unsigned long __increment = __width >> 2; \
	for (; __increment > 0; __increment--) { actionx2; actionx2; } \
	if (__width & 2) { actionx2; } \
	if (__width & 1) { actionx1; }  \
}	while (0)
#endif


//=====================================================================
// Global Definition
//=====================================================================
#define PIXEL_FLAG_MASK		  1		// draw with transparent color
#define PIXEL_FLAG_HFLIP	  2		// horizontal flip
#define PIXEL_FLAG_VFLIP	  4		// vertical flip
#define PIXEL_FLAG_NOCLIP	  8		// no clip (must be careful)
#define PIXEL_FLAG_SRCOVER   16		// only used in blend & scale
#define PIXEL_FLAG_ADDITIVE  32		// only used in blend & scale
#define PIXEL_FLAG_SRCCOPY   64		// only used in blend & scale
#define PIXEL_FLAG_LINEAR   128		// only used in scale
#define PIXEL_FLAG_BILINEAR 256		// only used in scale


//---------------------------------------------------------------------
// Rectangle
//---------------------------------------------------------------------
struct BasicRect
{
	int left;
	int top;
	int right;
	int bottom;

	bb_inline BasicRect(int l, int t, int r, int b):
		left(l), top(t), right(r), bottom(b) {}

	bb_inline BasicRect():
		left(0), top(0), right(0), bottom(0) {}
};


//---------------------------------------------------------------------
// Color
//---------------------------------------------------------------------
struct BasicColor
{
#if IWORDS_BIG_ENDIAN
	IUINT8 a, r, g, b;
#else
	IUINT8 b, g, r, a;
#endif
};


//---------------------------------------------------------------------
// Exception
//---------------------------------------------------------------------
struct BasicError
{ 
	BasicError(const char *m, int c) : msg(m), code(c) {} 
	const char *msg; 
	int code; 
};


//=====================================================================
// BasicBitmap
//=====================================================================
class BasicBitmap
{
public:
	virtual ~BasicBitmap();

	// format of color elements
	enum PixelFmt { 
		A8R8G8B8 = 0, 
		A8B8G8R8 = 1,
		X8R8G8B8 = 2, 
		R8G8B8 = 3,
		R5G6B5 = 4, 
		A1R5G5B5 = 5, 
		X1R5G5B5 = 6,
		A4R4G4B4 = 7,
		G8 = 8,
		UNKNOW = 9,
	};
	
	// create new bitmap, default pixel format is A8R8G8B8 
	BasicBitmap(int width, int height, PixelFmt fmt = A8R8G8B8);

	// create new bitmap with external bit buffer, 
	// you must free external mem manually after destructor
	BasicBitmap(int width, int height, PixelFmt fmt, void *mem, long pitch);

	// copy constructor
	BasicBitmap(const BasicBitmap &src);

	// move constructor
	#if __cplusplus >= 201103 || (defined(_MSC_VER) && _MSC_VER >= 1900)
	BasicBitmap(BasicBitmap &&src);
	#endif

private:
	// copy assignment is not allowed here
	BasicBitmap& operator=(const BasicBitmap &src);

public:

	void Fill(int x, int y, int w, int h, IUINT32 color);
	void Clear(IUINT32 color = 0);

	// blit from source bitmap with same bpp
	// (mode & PIXEL_FLAG_HFLIP) != 0, for horizontal flip
	// (mode & PIXEL_FLAG_VFLIP) != 0, for vertical flip
	// (mode & PIXEL_FLAG_MASK) != 0, for transparent bliting (using SetMask to set color key)
	// returns zero for successful, others for error
	int Blit(int x, int y, const BasicBitmap *src, int sx, int sy, int w, int h, int mode = 0);
	int Blit(int x, int y, const BasicBitmap *src, const BasicRect *rect = NULL, int mode = 0);

	void SetPixel(int x, int y, IUINT32 color);
	IUINT32 GetPixel(int x, int y) const;

	IUINT32 Raw2ARGB(IUINT32 color);
	IUINT32 ARGB2Raw(IUINT32 argb);

	// draw pixel batch
	void SetPixels(const short *xylist, int count, IUINT32 color);

	// convert from different bpp, do not support transparent color (mask), 
	int Convert(int x, int y, const BasicBitmap *src, int sx, int sy, int w, int h, int mode = 0);

	// AlphaBlend with cover color
	// (mode & PIXEL_FLAG_HFLIP) != 0, for horizontal flip
	// (mode & PIXEL_FLAG_VFLIP) != 0, for vertical flip
	// (mode & PIXEL_FLAG_SRCOVER) == 0, for pure alpha blending
	// (mode & PIXEL_FLAG_SRCOVER) != 0, for premultiplied src-over blending
	// (mode & PIXEL_FLAG_ADDITIVE) != 0, for color plus
	// (mode & PIXEL_FLAG_SRCCOPY) != 0, for color copy (no blending)
	int Blend(int x, int y, const BasicBitmap *src, int sx, int sy, int w, int h, 
		int mode = 0, IUINT32 color = 0xffffffff);

	// split r, g, b, a channels
	int SplitChannel(BasicBitmap *R, BasicBitmap *G, BasicBitmap *B, BasicBitmap *A);

	// composite r, g, b, a channels 
	int MergeChannel(const BasicBitmap *R, const BasicBitmap *G, 
		const BasicBitmap *B, const BasicBitmap *A);

	// chop a rectangle and make a new bitmap,
	BasicBitmap *Chop(int x, int y, int w, int h, PixelFmt fmt = A8R8G8B8);

	// flip around y axis 
	void FlipHorizontal();

	// flip around x axis
	void FlipVertical();

	// Bresenham stretch, bpp must be same, it can stretch with a transparent
	// color (when using PIXEL_FLAG_MASK), mode can be PIXEL_FLAG_MASK / HFLIP / VFLIP
	void BresenhamStretch(int dx, int dy, int dw, int dh, const BasicBitmap *src, 
		int sx, int sy, int sw, int sh, int mode);

	// Get A8R8G8B8 from position, different from GetPixel, GetPixel returns raw pixel
	// GetColor will convert raw pixel to A8R8G8B8
	IUINT32 GetColor(int x, int y) const;

	// Set A8R8G8B8 to position, SetPixel set raw pixel, GetColor will convert A8R8G8B8
	// to raw pixel format then set to position
	void SetColor(int x, int y, IUINT32 RGBA);

	// Shuffle 32/24 bits, b3 is unused in 24 bits
	void Shuffle(int b0, int b1, int b2, int b3);

public:
	bb_inline int Width() const { return _w; }
	bb_inline int Height() const { return _h; }
	bb_inline int Bpp() const { return _bpp; }
	bb_inline long Pitch() const { return _pitch; }
	bb_inline PixelFmt Format() const { return _fmt; }

	bb_inline unsigned char *Bits() { return _bits; }
	bb_inline const unsigned char *Bits() const { return _bits; }

	bb_inline unsigned char *Line(int n) { return _lines[n]; }
	bb_inline const unsigned char *Line(int n) const { return _lines[n]; }

	bb_inline IUINT32 GetMask() const { return _mask; }

	// set the transparent color, work in Transparent Blit
	// with flag PIXEL_FLAG_MASK to turn it on (in Blit & BresenhamStretch)
	bb_inline void SetMask(IUINT32 transparent) { _mask = transparent; }

	bb_inline IUINT8 *Address8(int x, int y) { return Line(y) + x; }
	bb_inline IUINT16 *Address16(int x, int y) { return ((IUINT16*)Line(y)) + x; }
	bb_inline IUINT32 *Address32(int x, int y) { return ((IUINT32*)Line(y)) + x; }
	bb_inline IUINT8 *Address24(int x, int y) { return Line(y) + x * 3; }
	bb_inline const IUINT8 *Address8(int x, int y) const { return Line(y) + x; }
	bb_inline const IUINT16 *Address16(int x, int y) const { return ((const IUINT16*)Line(y)) + x; }
	bb_inline const IUINT32 *Address32(int x, int y) const { return ((const IUINT32*)Line(y)) + x; }
	bb_inline const IUINT8 *Address24(int x, int y) const { return Line(y) + x * 3; }

public:

	// Draw scale from src using different filter and blend op
	// (mode & PIXEL_FLAG_HFLIP) != 0, for horizontal flip
	// (mode & PIXEL_FLAG_VFLIP) != 0, for vertical flip
	// (mode & PIXEL_FLAG_SRCOVER) == 0, for pure alpha blending
	// (mode & PIXEL_FLAG_SRCOVER) != 0, for premultiplied src-over blending
	// (mode & PIXEL_FLAG_ADDITIVE) != 0, for color plus
	// (mode & PIXEL_FLAG_SRCCOPY) != 0, for color copy (no blending)
	// (mode & PIXEL_FLAG_LINEAR) != 0, for linear interpolation
	// (mode & PIXEL_FLAG_BILINEAR) != 0, for bi-linear interpolation
	void Scale(int dx, int dy, int dw, int dh, const BasicBitmap *src,
		int sx, int sy, int sw, int sh, int mode = 0, IUINT32 color = 0xffffffff);

	void Scale(const BasicRect *rect, const BasicBitmap *src, const BasicRect *bound, 
		int mode = 0, IUINT32 color = 0xffffffff);

	// premultiply with alpha
	void Premultiply(bool reverse = false);

	// DrawLine
	void DrawLine(int x1, int y1, int x2, int y2, IUINT32 color);

	// draw 8x8/8x16/16x16 glyph
	void DrawGlyph(int x, int y, const void *glyph, int w, int h, IUINT32 color);

	// Quick-and-dirty font drawing using builtin mini-8x8 ascii font,
	// for show frame rate, debug info, etc.
	void QuickText(int x, int y, const char *text, IUINT32 color);

	// BilinearSampler, (x, y) is float point position
	IUINT32 SampleBilinear(float x, float y, bool repeat = true) const;

	// BicubicSampler, (x, y) is float point position
	IUINT32 SampleBicubic(float x, float y, bool repeat = true) const;


	// resample 
	enum ResampleFilter {
		NONE = 0,		// no interpolate at all
		LINEAR = 1,		// interpolate between rows not columns.
		BILINEAR = 2,	// interpolate between rows and columns
		AVERAGE = 3,	// better than bilinear, similar to bicubic, but much faster
		BICUBIC = 4,	// unoptimized, slow, but high quality, TODO: optimize it
	};

	// resample using different filter, 
	int Resample(int dx, int dy, int dw, int dh, const BasicBitmap *src, 
		int sx, int sy, int sw, int sh, ResampleFilter filter);

	// return an new resampled bitmap
	BasicBitmap *Resample(int NewWidth, int NewHeight, ResampleFilter filter) const;


public:

	typedef int (*PixelBlit)(void *dst, long dpitch, int dx, const void *src,
		long spitch, int sx, int w, int h, IUINT32 mask, int flip);

	typedef int (*PixelDraw)(void *dst, int offset, int w, const IUINT32 *card);

	// Replace Bliter for different bpps and transparent mode
	static void SetDriver(int bpp, PixelBlit proc, bool transparent = false);

	// Set and Get drawer for each format, mode 0/normal, 1/srcover, 2/plus
	static PixelDraw GetDriver(PixelFmt fmt, int mode, bool builtin = false);
	static void SetDriver(PixelFmt fmt, PixelDraw proc, int mode);

	// Row Fetch Pixel
	void RowFetch(int x, int y, IUINT32 *card, int w) const;

	void RowStore(int x, int y, const IUINT32 *card, int w);

	// use for 256 palette color format
	void RowFetchWithPalette(int x, int y, IUINT32 *card, int w, 
		const BasicColor *pal) const;

	// use for 256 palette color format
	void RowStoreWithPalette(int x, int y, IUINT32 *card, int w, 
		const BasicColor *pal);

	typedef int (*InterpRow)(IUINT32 *card, int w, const IUINT32 *row1, 
		const IUINT32 *row2, IINT32 fraction);		// 15.16 fixed point

	typedef int (*InterpCol)(IUINT32 *card, int w, const IUINT32 *src, 
		IINT32 x, IINT32 dx);		// 15.16 fixed point

	// interpolate row callback
	static void SetDriver(InterpRow row);

	// interpolate col callback
	static void SetDriver(InterpCol col);

public:

	static BasicBitmap *LoadBmpFromMemory(const void *buffer, long size, BasicColor *pal);

	static BasicBitmap *LoadTgaFromMemory(const void *buffer, long size, BasicColor *pal);

	static BasicBitmap *LoadBmp(const char *filename, PixelFmt fmt = UNKNOW, BasicColor *pal = NULL);

	static BasicBitmap *LoadTga(const char *filename, PixelFmt fmt = UNKNOW, BasicColor *pal = NULL);

	int SaveBmp(const char *filename, const BasicColor *pal = NULL) const;

	int SavePPM(const char *filename) const;

	// Search Palette
	static int BestfitColor(const BasicColor *pal, int r, int g, int b, int size);

	// DownSample 2x2 pixels into 1 pixel, only support 8/32 bits
	int DownSampleBy2(int x, int y, const BasicBitmap *src, int sx, int sy, int sw, int sh);

	// GetBlock only works with 8 bpp
	int GetBlock(int x, int y, int *block, int w, int h) const;

	// SetBlock only works with 8 bpp
	int SetBlock(int x, int y, const int *block, int w, int h);

	// Set new alpha to all pixel, works for 32bits
	void SetAlphaForAllPixel(int alpha);


public:
#ifndef PIXEL_NO_SYSTEM

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
	// setup dib info from bitmap, buffsize of info must not exceed
	// sizeof(BITMAPINFO) + (256 + 4) * sizeof(RGBQUAD)
	static void InitDIBInfo(void *ptr, int width, int height, PixelFmt fmt);

	// call CreateDIBSection with given pixel format, return HBITMAP
	static void *CreateDIB(void *hDC, int w, int h, PixelFmt fmt, void **bits);

	// draw to hdc
	int SetDIBitsToDevice(void *hDC, int x, int y, int sx, int sy, int sw, int sh);

	// get bits from given HBITMAP
	int GetDIBits(void *hDC, void *hBitmap);

	// startup=1 for start gdi+, startup=0 for shutdown gdi+
	static int GdiPlusInit(int startup);

	// GdiPlus Load Image from memory
	static void *GdiPlusLoadCore(const void *data, long size, int *cx, int *cy, 
		long *pitch, int *pfmt, int *bpp, int *errcode);

	// load image using gdi plus, make sure gdi+ has been initialized
	// call GdiPlusInit first if you need
	static BasicBitmap *GdiPlusLoadImageFromMemory(const void *data, long size, int *err);

	// load image using gdi plus, make sure gdi+ has been initialized
	// call GdiPlusInit first if you need
	static BasicBitmap *GdiPlusLoadImage(const char *filename, PixelFmt fmt = UNKNOW);

	// CreateDIBSection and then create BasicBitmap in the DIB (borror pixel memory)
	// DeleteObject((HBITMAP)hbitmap) must be invoked to dispose DIB after bitmap destruction
	static BasicBitmap *CreateBitmapInDIB(void *hDC, int w, int h, PixelFmt fmt, void **hbitmap);

	// call AlphaBlend from msimg32.dll, BlendFunction must be passed as pointer
	static bool AlphaBlend(void *hdcDst, int nXOriginDest, int nYOriginDest,
		int nWidthDest, int nHeightDest, void *hdcSrc, int nXOriginSrc, 
		int nYOriginSrc, int nWidthSrc, int nHeightSrc, int SCA = 255);

	// call TransparentBlt from msimg32.dll
	static bool TransparentBlt(void *hdcDst, int nXOriginDest, int nYOriginDest,
		int nWidthDest, int nHeightDest, void *hdcSrc, int nXOriginSrc, 
		int nYOriginSrc, int nWidthSrc, int nHeightSrc, IUINT32 crTransparent);

#elif defined(__ANDROID__)
#endif

#endif

	// detect file type, and using LoadBmpFromMemory, GdiPlusLoadImageFromMemory, ..
	static BasicBitmap *LoadImageFromMemory(const void *data, long size, BasicColor *pal);

	// detect file type and load 
	static BasicBitmap *LoadFile(const char *filename, PixelFmt fmt = UNKNOW, BasicColor *pal = NULL);

protected:
	static int BlitNormal(int bpp, void *dbits, long dpitch, int dx, 
		const void *sbits, long spitch, int sx, int w, int h, 
		IUINT32 mask, int flip);

	static int BlitMask(int bpp, void *dbits, long dpitch, int dx, 
		const void *sbits, long spitch, int sx, int w, int h, 
		IUINT32 mask, int flip);

	static int ClipRect(const int *clipdst, const int *clipsrc,
		int *x, int *y, int *rectsrc, int mode);

	static int ClipScale(const BasicRect *clipdst, const BasicRect *clipsrc,
		BasicRect *bound_dst, BasicRect *bound_src, int mode);


	static PixelFmt Bpp2Fmt(int bpp);
	static int Fmt2Bpp(PixelFmt fmt);

	static void CardReverse(IUINT32 *card, int size);
	static void CardMultiply(IUINT32 *card, int size, IUINT32 color);
	static void CardSetAlpha(IUINT32 *card, int size, IUINT32 alpha);
	static void CardPremultiply(IUINT32 *card, int size, bool reverse = false);

	static void Fetch(PixelFmt fmt, const void * const __restrict bits, int x, int w, IUINT32 * __restrict buffer);
	static void Store(PixelFmt fmt, void * const __restrict bits, int x, int w, const IUINT32 * __restrict buffer);

	static void *LoadContent(const char *filename, long *size);

	static int InterpolateRow(IUINT32 *card, int w, const IUINT32 *row1, 
		const IUINT32 *row2, IINT32 fraction);

	static int InterpolateCol(IUINT32 * __restrict card, const int w, const IUINT32 * const __restrict src, 
		IINT32 x, const IINT32 dx);

	static int InterpolateRowNearest(IUINT32 *card, int w, const IUINT32 *row1, 
		const IUINT32 *row2, IINT32 fraction);

	static int InterpolateColNearest(IUINT32 * __restrict card, int w, const IUINT32 * const __restrict src,
		IINT32 x, const IINT32 dx);

	static InterpRow InterpolateRowPtr;
	static InterpCol InterpolateColPtr;


protected:
	int Initialize(int width, int height, PixelFmt fmt, void *mem, long pitch);
	int Destroy();

protected:
	static PixelBlit PixelBlitNormal1;
	static PixelBlit PixelBlitNormal2;
	static PixelBlit PixelBlitNormal3;
	static PixelBlit PixelBlitNormal4;
	static PixelBlit PixelBlitMask1;
	static PixelBlit PixelBlitMask2;
	static PixelBlit PixelBlitMask3;
	static PixelBlit PixelBlitMask4;

protected:
	int _w;
	int _h;
	int _bpp;
	int _pixelsize;
	long _pitch;
	bool _borrow;
	PixelFmt _fmt;
	IUINT32 _mask;
	unsigned char *_bits;
	unsigned char **_lines;
};

#endif
