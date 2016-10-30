//=====================================================================
//
// BasicBitmap.cpp - Simple Bitmap Library
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
#include "BasicBitmap.h"
#include "BasicBitmap_C.h"

#ifndef PIXEL_NO_SYSTEM
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#include <windows.h>
#ifndef _WIN32
#define _WIN32 
#endif
#endif
#endif

#ifndef PIXEL_NO_STDIO
#include <stdio.h>
#endif

#ifdef __WATCOMC__
#pragma warning 555 10	// turn off while is always false
#endif

#if defined(__BORLANDC__) && !defined(__MSDOS__)
#pragma warn -8004  
#pragma warn -8057
#endif


//=====================================================================
// Internal Macros
//=====================================================================
#define pixel_round(x, n) (((x) + (n) - 1) & (~((n) - 1)))

#define PIXEL_ALIGN		16
#define PIXEL_STACK_BUFFER	2048


//---------------------------------------------------------------------
// MACRO: Pixel Assembly & Disassembly 
//---------------------------------------------------------------------
/* lookup table for scaling 1 bit colors up to 8 bits */
const IUINT32 _pixel_scale_1[2] = { 0, 255 };

/* lookup table for scaling 2 bit colors up to 8 bits */
const IUINT32 _pixel_scale_2[4] = { 0, 85, 170, 255 };

/* lookup table for scaling 3 bit colors up to 8 bits */
const IUINT32 _pixel_scale_3[8] = { 0, 36, 72, 109, 145, 182, 218, 255 };

/* lookup table for scaling 4 bit colors up to 8 bits */
const IUINT32 _pixel_scale_4[16] = {
    0, 16, 32, 49, 65, 82, 98, 115, 
    139, 156, 172, 189, 205, 222, 238, 255
};

/* lookup table for scaling 5 bit colors up to 8 bits */
const IUINT32 _pixel_scale_5[32] = {
   0,   8,   16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98,  106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
const IUINT32 _pixel_scale_6[64] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   64,  68,  72,  76,  80,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   129, 133, 137, 141, 145, 149, 153, 157,
   161, 165, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 214, 218, 222,
   226, 230, 234, 238, 242, 246, 250, 255
};


static unsigned char pixel_blend_lut[2048 * 2];

static unsigned char pixel_clip_vector[256 * 3];
static unsigned char *pixel_clip_256 = &pixel_clip_vector[256];

#define PIXEL_CLIP_256(x) pixel_clip_256[x]


//=====================================================================
// Memory Management
//=====================================================================
void *(*_internal_hook_malloc)(size_t size) = NULL;
void (*_internal_hook_free)(void *ptr) = NULL;
void *(*_internal_hook_memcpy)(void *dst, const void *src, size_t n) = NULL;

// allocate aligned memory
static void *internal_align_malloc(size_t size, size_t n) {
	size_t need = size + n + sizeof(void*);
	char *ptr = NULL;
	char *dst;
	if (_internal_hook_malloc) {
		ptr = (char*)_internal_hook_malloc(need);
	}	else {
		ptr = (char*)malloc(need);
	}
	if (ptr == NULL) return NULL;
	dst = (char*)(((size_t)ptr + sizeof(void*) + n - 1) & (~(n - 1)));
	*(char**)(dst - sizeof(char*)) = ptr;
	return dst;
}

// free aligned memory
static void internal_align_free(void *ptr) {
	char *dst = (char*)ptr;
	ptr = *(char**)(dst - sizeof(char*));
	assert(ptr);
	*(char**)(dst - sizeof(char*)) = NULL;
	if (_internal_hook_free) {
		_internal_hook_free(ptr);
	}	else {
		free(ptr);
	}
}

// copy from destination to source
static void *internal_memcpy(void *dst, const void *src, size_t size)
{
	if (_internal_hook_memcpy) {
		return _internal_hook_memcpy(dst, src, size);
	}

#ifndef PIXEL_NO_MEMCPY
	return memcpy(dst, src, size);
#else
	unsigned char *dd = (unsigned char*)dst;
	const unsigned char *ss = (const unsigned char*)src;
	for (; size >= 8; size -= 8) {
		*(IUINT32*)(dd + 0) = *(const IUINT32*)(ss + 0);
		*(IUINT32*)(dd + 4) = *(const IUINT32*)(ss + 4);
		dd += 8;
		ss += 8;
	}
	for (; size > 0; size--) *dd++ = *ss++;
	return dst;
#endif
}


//---------------------------------------------------------------------
// memory encode / decode
//---------------------------------------------------------------------

/* encode 8 bits unsigned int */
static inline char *basic_encode_8u(char *p, IUINT8 c)
{
	*(unsigned char*)p++ = c;
	return p;
}

/* decode 8 bits unsigned int */
static inline const char *basic_decode_8u(const char *p, IUINT8 *c)
{
	*c = *(unsigned char*)p++;
	return p;
}

/* encode 16 bits unsigned int (lsb) */
static inline char *basic_encode_16u(char *p, IUINT16 w)
{
#if IWORDS_BIG_ENDIAN
	*(unsigned char*)(p + 0) = (w & 255);
	*(unsigned char*)(p + 1) = (w >> 8);
#else
	*(unsigned short*)(p) = w;
#endif
	p += 2;
	return p;
}

/* decode 16 bits unsigned int (lsb) */
static inline const char *basic_decode_16u(const char *p, IUINT16 *w)
{
#if IWORDS_BIG_ENDIAN
	*w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
	*w = *(const unsigned short*)p;
#endif
	p += 2;
	return p;
}

/* encode 32 bits unsigned int (lsb) */
static inline char *basic_encode_32u(char *p, IUINT32 l)
{
#if IWORDS_BIG_ENDIAN
	*(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
	*(IUINT32*)p = l;
#endif
	p += 4;
	return p;
}

/* decode 32 bits unsigned int (lsb) */
static inline const char *basic_decode_32u(const char *p, IUINT32 *l)
{
#if IWORDS_BIG_ENDIAN
	*l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else 
	*l = *(const IUINT32*)p;
#endif
	p += 4;
	return p;
}


//---------------------------------------------------------------------
// initialize tables
//---------------------------------------------------------------------
static void PixelInitLut()
{
	static int inited = 0;
	if (inited) {
		return;
	}
	else {
		int i;
		for (i = 0; i < 256; i++) {
			pixel_clip_256[i] = (IUINT8)i;
			pixel_clip_256[i - 256] = 0;
			pixel_clip_256[i + 256] = 255;
		}

		for (i = 0; i < 2048; i++) {
			IUINT32 da = _pixel_scale_5[i >> 6];
			IUINT32 sa = _pixel_scale_6[i & 63];
			IUINT32 FA = da + ((255 - da) * sa) / 255;
			IUINT32 SA = (FA != 0)? ((sa * 255) / FA) : 0;
			pixel_blend_lut[i * 2 + 0] = (unsigned char)SA;
			pixel_blend_lut[i * 2 + 1] = (unsigned char)FA;
		}
	}
	inited = 1;
}


//---------------------------------------------------------------------
// Internal Macros
//---------------------------------------------------------------------
#define RGBA_FROM_A8R8G8B8(c, r, g, b, a) _pixel_disasm_8888(c, a, r, g, b)
#define RGBA_FROM_X8R8G8B8(c, r, g, b, a) do { \
        _pixel_disasm_X888(c, r, g, b); (a) = 255; } while (0)
#define RGBA_FROM_P8R8G8B8(c, r, g, b, a) do { \
        _pixel_from_P8R8G8B8(c, r, g, b, a); } while (0)
#define RGBA_FROM_A8B8G8R8(c, r, g, b, a) _pixel_disasm_8888(c, a, b, g, r)
#define RGBA_FROM_R8G8B8(c, r, g, b, a) do { \
        _pixel_disasm_888(c, r, g, b); (a) = 255; } while (0)
#define RGBA_FROM_R5G6B5(c, r, g, b, a) do { \
        _pixel_disasm_565(c, r, g, b); (a) = 255; } while (0)
#define RGBA_FROM_X1R5G5B5(c, r, g, b, a) do { \
        _pixel_disasm_X555(c, r, g, b); (a) = 255; } while (0)
#define RGBA_FROM_A1R5G5B5(c, r, g, b, a) _pixel_disasm_1555(c, a, r, g, b)
#define RGBA_FROM_A4R4G4B4(c, r, g, b, a) _pixel_disasm_4444(c, a, r, g, b)
#define RGBA_FROM_G8(c, r, g, b, a) do { \
		(r) = (g) = (b) = (c); (a) = 255; } while (0)

#define RGBA_TO_A8R8G8B8(r, g, b, a)  _pixel_asm_8888(a, r, g, b)
#define RGBA_TO_A8B8G8R8(r, g, b, a)  _pixel_asm_8888(a, b, g, r)
#define RGBA_TO_X8R8G8B8(r, g, b, a)  _pixel_asm_8888(0, r, g, b)
#define RGBA_TO_P8R8G8B8(r, g, b, a)  _pixel_RGBA_to_P8R8G8B8(r, g, b, a)
#define RGBA_TO_R8G8B8(r, g, b, a)    _pixel_asm_888(r, g, b)
#define RGBA_TO_R5G6B5(r, g, b, a)    _pixel_asm_565(r, g, b)
#define RGBA_TO_X1R5G5B5(r, g, b, a)  _pixel_asm_1555(0, r, g, b)
#define RGBA_TO_A1R5G5B5(r, g, b, a)  _pixel_asm_1555(a, r, g, b)
#define RGBA_TO_A4R4G4B4(r, g, b, a)  _pixel_asm_4444(a, r, g, b)
#define RGBA_TO_G8(r, g, b, a)        _pixel_to_gray(r, g, b)


#define _pixel_RGBA_to_P8R8G8B8(r, g, b, a) ( \
        (((a)) << 24) | \
        ((((r) * _pixel_norm(a)) >> 8) << 16) | \
        ((((g) * _pixel_norm(a)) >> 8) <<  8) | \
        ((((b) * _pixel_norm(a)) >> 8) <<  0))

#define _pixel_from_P8R8G8B8(c, r, g, b, a) do { \
            IUINT32 __SA = ((c) >> 24); \
            IUINT32 __FA = (__SA); \
            (a) = __SA; \
            if (__FA > 0) { \
                (r) = ((((c) >> 16) & 0xff) * 255) / __FA; \
                (g) = ((((c) >>  8) & 0xff) * 255) / __FA; \
                (b) = ((((c) >>  0) & 0xff) * 255) / __FA; \
            }    else { \
                (r) = 0; (g) = 0; (b) = 0; \
            }    \
        }   while (0)

#define RGBA_TO_PIXEL(fmt, r, g, b, a) RGBA_TO_##fmt(r, g, b, a)
#define RGBA_FROM_PIXEL(fmt, c, r, g, b, a) RGBA_FROM_##fmt(c, r, g, b, a)


//---------------------------------------------------------------------
// BLEND
//---------------------------------------------------------------------

/* blend onto a static surface (no alpha channel) */
#define BLEND_STATIC(sr, sg, sb, sa, dr, dg, db, da) do { \
		IINT32 SA = _pixel_norm(sa); \
		(dr) = (((((IINT32)(sr)) - ((IINT32)(dr))) * SA) >> 8) + (dr); \
		(dg) = (((((IINT32)(sg)) - ((IINT32)(dg))) * SA) >> 8) + (dg); \
		(db) = (((((IINT32)(sb)) - ((IINT32)(db))) * SA) >> 8) + (db); \
		(da) = 255; \
	}	while (0)

/* blend onto a normal surface (with alpha channel) */
#define BLEND_NORMAL(sr, sg, sb, sa, dr, dg, db, da) do { \
		IINT32 SA = _pixel_norm(sa); \
		IINT32 DA = _pixel_norm(da); \
		IINT32 FA = DA + (((256 - DA) * SA) >> 8); \
		SA = (FA != 0)? ((SA << 8) / FA) : (0); \
		(da) = _pixel_unnorm(FA); \
		(dr) = (((((IINT32)(sr)) - ((IINT32)(dr))) * SA) >> 8) + (dr); \
		(dg) = (((((IINT32)(sg)) - ((IINT32)(dg))) * SA) >> 8) + (dg); \
		(db) = (((((IINT32)(sb)) - ((IINT32)(db))) * SA) >> 8) + (db); \
	}	while (0)


/* blend onto a normal surface (with alpha channel) in a fast way */
/* looking up alpha values from lut instead of div. calculation */
/* lut must be inited by ipixel_lut_init() */
#define BLEND_NORMAL_FAST(sr, sg, sb, sa, dr, dg, db, da) do { \
		IUINT32 __lutpos = (((da) & 0xf8) << 4) | (((sa) & 0xfc) >> 1); \
		IINT32 SA = pixel_blend_lut[(__lutpos) + 0]; \
		IINT32 FA = pixel_blend_lut[(__lutpos) + 1]; \
		SA = _pixel_norm((SA)); \
		(da) = FA; \
		(dr) = (((((IINT32)(sr)) - ((IINT32)(dr))) * SA) >> 8) + (dr); \
		(dg) = (((((IINT32)(sg)) - ((IINT32)(dg))) * SA) >> 8) + (dg); \
		(db) = (((((IINT32)(sb)) - ((IINT32)(db))) * SA) >> 8) + (db); \
	}	while (0)

/* equation used in sdl */
#define BLEND_NORMAL_FLAT(sr, sg, sb, sa, dr, dg, db, da) do { \
		(dr) = ((((sr) - (dr)) * (sa)) / 255) + (dr);  \
		(dg) = ((((sg) - (dg)) * (sa)) / 255) + (dg);  \
		(db) = ((((sb) - (db)) * (sa)) / 255) + (db);  \
		(da) = (sa) + (da)- ((sa) * (da)) / 255;  \
	}	while (0)


/* additive blend */
#define BLEND_ADDITIVE(sr, sg, sb, sa, dr, dg, db, da) do { \
		IINT32 XA = _pixel_norm(sa); \
		IINT32 XR = (sr) * XA; \
		IINT32 XG = (sg) * XA; \
		IINT32 XB = (sb) * XA; \
		XR = XR >> 8; \
		XG = XG >> 8; \
		XB = XB >> 8; \
		XA = (sa) + (da); \
		XR += (dr); \
		XG += (dg); \
		XB += (db); \
		(dr) = PIXEL_CLIP_256(XR); \
		(dg) = PIXEL_CLIP_256(XG); \
		(db) = PIXEL_CLIP_256(XB); \
		(da) = PIXEL_CLIP_256(XA); \
	}	while (0)

/* premultiplied src over */
#define BLEND_SRCOVER(sr, sg, sb, sa, dr, dg, db, da) do { \
		IUINT32 SA = 255 - (sa); \
		(dr) = (dr) * SA; \
		(dg) = (dg) * SA; \
		(db) = (db) * SA; \
		(da) = (da) * SA; \
		(dr) = _pixel_fast_div_255(dr) + (sr); \
		(dg) = _pixel_fast_div_255(dg) + (sg); \
		(db) = _pixel_fast_div_255(db) + (sb); \
		(da) = _pixel_fast_div_255(da) + (sa); \
	}	while (0)


/* premutiplied 32bits blending: 
   dst = src + (255 - src.alpha) * dst / 255 */
#define BLEND_PARGB(color_dst, color_src) do { \
		IUINT32 __A = 255 - ((color_src) >> 24); \
		IUINT32 __DST_RB = (color_dst) & 0xff00ff; \
		IUINT32 __DST_AG = ((color_dst) >> 8) & 0xff00ff; \
		__DST_RB *= __A; \
		__DST_AG *= __A; \
		__DST_RB += ((__DST_RB + 0x01010101) >> 8) & 0x00ff00ff; \
		__DST_AG += ((__DST_AG + 0x01010101) >> 8) & 0xff00ff00; \
		__DST_RB >>= 8; \
		__DST_AG &= 0xff00ff00; \
		__A = (__DST_RB & 0xff00ff) | __DST_AG; \
		(color_dst) = __A + (color_src); \
	}	while (0)


/* premutiplied 32bits blending (with coverage): 
   tmp = src * coverage / 255,
   dst = tmp + (255 - tmp.alpha) * dst / 255 */
#define BLEND_PARGB_COVER(color_dst, color_src, coverage) do { \
		IUINT32 __r1 = (color_src) & 0xff00ff; \
		IUINT32 __r2 = ((color_src) >> 8) & 0xff00ff; \
		IUINT32 __r3 = _pixel_norm(coverage); \
		IUINT32 __r4; \
		__r1 *= __r3; \
		__r2 *= __r3; \
		__r3 = (color_dst) & 0xff00ff; \
		__r4 = ((color_dst) >> 8) & 0xff00ff; \
		__r1 = ((__r1) >> 8) & 0xff00ff; \
		__r2 = (__r2) & 0xff00ff00; \
		__r1 = __r1 | __r2; \
		__r2 = 255 - (__r2 >> 24); \
		__r3 *= __r2; \
		__r4 *= __r2; \
		__r3 = ((__r3 + (__r3 >> 8)) >> 8) & 0xff00ff; \
		__r4 = (__r4 + (__r4 >> 8)) & 0xff00ff00; \
		(color_dst) = (__r3 | __r4) + (__r1); \
	}	while (0)



//=====================================================================
// STATIC DEFINITION
//=====================================================================
BasicBitmap::PixelBlit BasicBitmap::PixelBlitNormal1 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitNormal2 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitNormal3 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitNormal4 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitMask1 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitMask2 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitMask3 = NULL;
BasicBitmap::PixelBlit BasicBitmap::PixelBlitMask4 = NULL;


//=====================================================================
// BasicBitmap
//=====================================================================

//---------------------------------------------------------------------
// Constructor: create new bitmap
// pixel format for bpp = 32 is A8R8G8B8
// for bpp = 24 is R8G8B8, for bpp = 16 is R5G6B5, 
// for bpp = 15 is A1R5G5B5 and for bpp = 8 is G8.
//---------------------------------------------------------------------
BasicBitmap::BasicBitmap(int width, int height, PixelFmt fmt)
{
	_bits = NULL;
	_lines = NULL;
	_w = _h = _bpp = 0;
	_pitch = 0;
	_borrow = false;
	int hr = Initialize(width, height, fmt, NULL, -1);
	if (hr != 0) {
	#ifndef PIXEL_USE_EXCEPTION
		assert(hr == 0);
	#else
		throw BasicError("Initialize bitmap error", hr);
	#endif
	}
}


//---------------------------------------------------------------------
// Constructor: create new bitmap with external bit buffer
//---------------------------------------------------------------------
BasicBitmap::BasicBitmap(int width, int height, PixelFmt fmt, void *mem, long pitch)
{
	_bits = NULL;
	_lines = NULL;
	_w = _h = _bpp = 0;
	_pitch = 0;
	_borrow = false;
	int hr = Initialize(width, height, fmt, mem, pitch);
	if (hr != 0) {
	#ifndef PIXEL_USE_EXCEPTION
		assert(hr == 0);
	#else
		throw BasicError("Initialize bitmap error", hr);
	#endif
	}
}


//---------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------
BasicBitmap::~BasicBitmap()
{
	Destroy();
}


//---------------------------------------------------------------------
// Initialize allocate buffer
// pixel format for bpp = 32 is A8R8G8B8,
// for bpp = 24 is R8G8B8, for bpp = 16 is R5G6B5, 
// for bpp = 15 is A1R5G5B5 and for bpp = 8 is G8.
//---------------------------------------------------------------------
int BasicBitmap::Initialize(int width, int height, PixelFmt fmt, void *mem, long pitch)
{
	int bpp = 0;

	switch (fmt) {
	case A8R8G8B8: bpp = 32; break;
	case A8B8G8R8: bpp = 32; break;
	case X8R8G8B8: bpp = 32; break;
	case R8G8B8: bpp = 24; break;
	case R5G6B5: bpp = 16; break;
	case A1R5G5B5: bpp = 16; break;
	case X1R5G5B5: bpp = 16; break;
	case A4R4G4B4: bpp = 16; break;
	case G8: bpp = 8; break;
	case UNKNOW: 
		return -1;
	}

	int pixelsize = (bpp + 1) / 8;

	if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) 
		return -2;

	Destroy();

	_w = width;
	_h = height;
	_bpp = bpp;
	_fmt = fmt;

	if (mem == NULL) {
		_pitch = pixel_round(width * pixelsize, 4);
		_bits = (IUINT8*)internal_align_malloc(_pitch * height, PIXEL_ALIGN);
		if (_bits == NULL) {
			_w = _h = _bpp = 0;
			return -3;
		}
		_borrow = false;
	}	else {
		_pitch = pitch;
		_bits = (unsigned char*)mem;
		_borrow = true;
	}

	long size_lines = sizeof(char*) * height;

	char *ptr = (char*)malloc(size_lines);
	_lines = (unsigned char**)ptr;

	if (_lines == NULL) {
		if (_borrow == false) {
			internal_align_free(_bits);
			_bits = NULL;
			_w = _h = _bpp = 0;
			return -4;
		}
	}

	for (int j = 0; j < height; j++) {
		_lines[j] = _bits + j * _pitch;
	}

	_pixelsize = pixelsize;

	_mask = 0;

	return 0;
}


//---------------------------------------------------------------------
// dispose memory
//---------------------------------------------------------------------
int BasicBitmap::Destroy()
{
	if (_bits) {
		if (_borrow == false) {
			internal_align_free(_bits);
		}
		_bits = NULL;
	}
	if (_lines) {
		free(_lines); 
	}
	_w = _h = _bpp = 0;
	_pitch = 0;
	return 0;
}


//---------------------------------------------------------------------
// fill rectangle with given color
//---------------------------------------------------------------------
void BasicBitmap::Fill(int x, int y, int w, int h, IUINT32 color)
{
	int i, j;

	if (x < 0) w += x, x = 0;
	if (y < 0) h += y, y = 0;
	if (w < 0 || h < 0) return;
	if (x + w >= _w) w = _w - x;
	if (y + h >= _h) h = _h - y;

	switch (_pixelsize) {
	case 1:
		for (j = 0; j < h; j++) {
			IUINT8 *dst = Line(y + j) + x;
			IUINT8 cc = (IUINT8)(color & 0xff);
			for (i = w; i > 0; i--) {
				*dst++ = cc;
			}
		}
		break;
	case 2:
		for (j = 0; j < h; j++) {
			IUINT16 *dst = ((IUINT16*)Line(y + j)) + x;
			IUINT16 cc = (IUINT16)(color & 0xffff);
			for (i = w; i > 0; i--) {
				*dst++ = cc;
			}
		}
		break;
	case 3:
		for (j = 0; j < h; j++) {
			IUINT8 *dst = Line(y + j);
			int l = x;
			int r = x + w;
			for (; l < r; l++) {
				_pixel_store(24, dst, l, color);
			}
		}
		break;
	case 4:
		for (j = 0; j < h; j++) {
			IUINT32 *dst = ((IUINT32*)Line(y + j)) + x;
			for (i = w; i > 0; i--) {
				*dst++ = color;
			}
		}
		break;
	}
}


//---------------------------------------------------------------------
// Constructor: create new bitmap
//---------------------------------------------------------------------
void BasicBitmap::Clear(IUINT32 color)
{
	Fill(0, 0, _w, _h, color);
}


//---------------------------------------------------------------------
// blit normal
//---------------------------------------------------------------------
int BasicBitmap::BlitNormal(int bpp, void *dbits, long dpitch, int dx, const
	void *sbits, long spitch, int sx, int w, int h, IUINT32 mask, int flip)
{
	int pixelsize = (bpp + 7) >> 3;
	int y, x, x1, x2, sx0, sxd, endx;

	if (flip & PIXEL_FLAG_VFLIP) { 
		sbits = (const IUINT8*)sbits + spitch * (h - 1); 
		spitch = -spitch; 
	} 

	switch (pixelsize) {
	case 1:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				internal_memcpy((IUINT8*)dbits + dx, 
					(const IUINT8*)sbits + sx, size); 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx + w - 1; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 2:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				internal_memcpy((IUINT16*)dbits + dx, 
					(const IUINT16*)sbits + sx, size); 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx + w - 1; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 3:
		if (flip & PIXEL_FLAG_HFLIP) { 
			sx0 = sx + w - 1; 
			sxd = -1; 
		}	else { 
			sx0 = sx; 
			sxd = 1; 
		} 
		endx = dx + w; 
		for (y = 0; y < h; y++) { 
			IUINT32 cc; 
			for (x1 = dx, x2 = sx0; x1 < endx; x1++, x2 += sxd) { 
				cc = _pixel_fetch(24, sbits, x2); 
				_pixel_store(24, dbits, x1, cc); 
			} 
			dbits = (IUINT8*)dbits + dpitch; 
			sbits = (const IUINT8*)sbits + spitch; 
		} 
		break;

	case 4:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			long size = w * pixelsize; 
			for (y = 0; y < h; y++) { 
				internal_memcpy((IUINT32*)dbits + dx, 
					(const IUINT32*)sbits + sx, size);
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx + w - 1; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				for (x = w; x > 0; x--) *dst++ = *src--; 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;
	}

	return mask - mask;
}


//---------------------------------------------------------------------
// blit mask
//---------------------------------------------------------------------
int BasicBitmap::BlitMask(int bpp, void *dbits, long dpitch, int dx, const
	void *sbits, long spitch, int sx, int w, int h, IUINT32 mask, int flip)
{
	int pixelsize = (bpp + 7) >> 3;
	int y, x1, x2, sx0, sxd, endx;

	if (flip & PIXEL_FLAG_VFLIP) { 
		sbits = (const IUINT8*)sbits + spitch * (h - 1); 
		spitch = -spitch; 
	} 

	switch (pixelsize) {
	case 1:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			IUINT8 cmask = (IUINT8)(mask & 0xff);
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				IUINT8 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT8 cmask = (IUINT8)(mask & 0xff);
			for (y = 0; y < h; y++) { 
				const IUINT8 *src = (const IUINT8*)sbits + sx + w - 1; 
				IUINT8 *dst = (IUINT8*)dbits + dx; 
				IUINT8 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 2:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			IUINT16 cmask = (IUINT16)(mask & 0xffff);
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				IUINT16 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT16 cmask = (IUINT16)(mask & 0xffff);
			for (y = 0; y < h; y++) { 
				const IUINT16 *src = (const IUINT16*)sbits + sx + w - 1; 
				IUINT16 *dst = (IUINT16*)dbits + dx; 
				IUINT16 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;

	case 3:
		if (flip & PIXEL_FLAG_HFLIP) { 
			sx0 = sx + w - 1; 
			sxd = -1; 
		}	else { 
			sx0 = sx; 
			sxd = 1; 
		} 
		endx = dx + w; 
		for (y = 0; y < h; y++) { 
			IUINT32 cc; 
			for (x1 = dx, x2 = sx0; x1 < endx; x1++, x2 += sxd) { 
				cc = _pixel_fetch(24, sbits, x2); 
				if (cc != mask) _pixel_store(24, dbits, x1, cc); 
			} 
			dbits = (IUINT8*)dbits + dpitch; 
			sbits = (const IUINT8*)sbits + spitch; 
		} 
		break;

	case 4:
		if ((flip & PIXEL_FLAG_HFLIP) == 0) { 
			IUINT32 cmask = (IUINT32)mask;
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				IUINT32 *dstend = dst + w; 
				for (; dst < dstend; src++, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		}	else { 
			IUINT32 cmask = (IUINT32)mask;
			for (y = 0; y < h; y++) { 
				const IUINT32 *src = (const IUINT32*)sbits + sx + w - 1; 
				IUINT32 *dst = (IUINT32*)dbits + dx; 
				IUINT32 *dstend = dst + w; 
				for (; dst < dstend; src--, dst++) { 
					if (src[0] != cmask) dst[0] = src[0]; 
				} 
				dbits = (IUINT8*)dbits + dpitch; 
				sbits = (const IUINT8*)sbits + spitch; 
			} 
		} 
		break;
	}

	return 0;
}


//---------------------------------------------------------------------
// ClipRect - clip the rectangle from the src clip and dst clip then
// caculate a new rectangle shared between dst and src cliprect:
// clipdst  - dest clip array (left, top, right, bottom)
// clipsrc  - source clip array (left, top, right, bottom)
// (x, y)   - dest position
// rectsrc  - source rect
// mode     - check IBLIT_HFLIP or IBLIT_VFLIP
// return zero for successful, return non-zero if there is no shared part
//---------------------------------------------------------------------
int BasicBitmap::ClipRect(const int *clipdst, const int *clipsrc, 
	int *x, int *y, int *rectsrc, int mode)
{
	int dcl = clipdst[0];       /* dest clip: left     */
	int dct = clipdst[1];       /* dest clip: top      */
	int dcr = clipdst[2];       /* dest clip: right    */
	int dcb = clipdst[3];       /* dest clip: bottom   */
	int scl = clipsrc[0];       /* source clip: left   */
	int sct = clipsrc[1];       /* source clip: top    */
	int scr = clipsrc[2];       /* source clip: right  */
	int scb = clipsrc[3];       /* source clip: bottom */
	int dx = *x;                /* dest x position     */
	int dy = *y;                /* dest y position     */
	int sl = rectsrc[0];        /* source rectangle: left   */
	int st = rectsrc[1];        /* source rectangle: top    */
	int sr = rectsrc[2];        /* source rectangle: right  */
	int sb = rectsrc[3];        /* source rectangle: bottom */
	int hflip, vflip;
	int w, h, d;

	hflip = (mode & PIXEL_FLAG_HFLIP)? 1 : 0;
	vflip = (mode & PIXEL_FLAG_VFLIP)? 1 : 0;

	if (dcr <= dcl || dcb <= dct || scr <= scl || scb <= sct) 
		return -1;

	if (sr <= scl || sb <= sct || sl >= scr || st >= scb) 
		return -2;

	/* check dest clip: left */
	if (dx < dcl) {
		d = dcl - dx;
		dx = dcl;
		if (!hflip) sl += d;
		else sr -= d;
	}

	/* check dest clip: top */
	if (dy < dct) {
		d = dct - dy;
		dy = dct;
		if (!vflip) st += d;
		else sb -= d;
	}

	w = sr - sl;
	h = sb - st;

	if (w < 0 || h < 0) 
		return -3;

	/* check dest clip: right */
	if (dx + w > dcr) {
		d = dx + w - dcr;
		if (!hflip) sr -= d;
		else sl += d;
	}

	/* check dest clip: bottom */
	if (dy + h > dcb) {
		d = dy + h - dcb;
		if (!vflip) sb -= d;
		else st += d;
	}

	if (sl >= sr || st >= sb) 
		return -4;

	/* check source clip: left */
	if (sl < scl) {
		d = scl - sl;
		sl = scl;
		if (!hflip) dx += d;
	}

	/* check source clip: top */
	if (st < sct) {
		d = sct - st;
		st = sct;
		if (!vflip) dy += d;
	}

	if (sl >= sr || st >= sb) 
		return -5;

	/* check source clip: right */
	if (sr > scr) {
		d = sr - scr;
		sr = scr;
		if (hflip) dx += d;
	}

	/* check source clip: bottom */
	if (sb > scb) {
		d = sb - scb;
		sb = scb;
		if (vflip) dy += d;
	}

	if (sl >= sr || st >= sb) 
		return -6;

	*x = dx;
	*y = dy;

	rectsrc[0] = sl;
	rectsrc[1] = st;
	rectsrc[2] = sr;
	rectsrc[3] = sb;

	return 0;
}


//---------------------------------------------------------------------
// clip rectangle with scale
//---------------------------------------------------------------------
int BasicBitmap::ClipScale(const BasicRect *clipdst, const 
	BasicRect *clipsrc, BasicRect *bound_dst, BasicRect *bound_src, int mode)
{
	int dcl = clipdst->left;
	int dct = clipdst->top;
	int dcr = clipdst->right;
	int dcb = clipdst->bottom;
	int scl = clipsrc->left;
	int sct = clipsrc->top;
	int scr = clipsrc->right;
	int scb = clipsrc->bottom;
	int dl = bound_dst->left;
	int dt = bound_dst->top;
	int dr = bound_dst->right;
	int db = bound_dst->bottom;
	int sl = bound_src->left;
	int st = bound_src->top;
	int sr = bound_src->right;
	int sb = bound_src->bottom;
	int dw = dr - dl;
	int dh = db - dt;
	int sw = sr - sl;
	int sh = sb - st;
	int hflip, vflip;
	float fx, fy;
	float ix, iy;
	int d;

    hflip = (mode & PIXEL_FLAG_HFLIP)? 1 : 0;
    vflip = (mode & PIXEL_FLAG_VFLIP)? 1 : 0;

	if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0) 
		return -1;

	if (dl >= dcl && dt >= dct && dr <= dcr && db <= dcb &&
		sl >= scl && st >= sct && sr <= scr && sb <= scb)
		return 0;

	fx = ((float)dw) / sw;
	fy = ((float)dh) / sh;
	ix = ((float)sw) / dw;
	iy = ((float)sh) / dh;

	// check dest clip: left
	if (dl < dcl) {
		d = dcl - dl;
		dl = dcl;
		if (!hflip) sl += (int)(d * ix);
		else sr -= (int)(d * ix);
	}

	// check dest clip: top
	if (dt < dct) {
		d = dct - dt;
		dt = dct;
		if (!vflip) st += (int)(d * iy);
		else sb -= (int)(d * iy);
	}

	if (dl >= dr || dt >= db || sl >= sr || st >= sb)
		return -2;

	// check dest clip: right
	if (dr > dcr) {
		d = dr - dcr;
		dr = dcr;
		if (!hflip) sr -= (int)(d * ix);
		else sl += (int)(d * ix);
	}

	// check dest clip: bottom
	if (db > dcb) {
		d = db - dcb;
		db = dcb;
		if (!vflip) sb -= (int)(d * iy);
		else st += (int)(d * iy);
	}

	if (dl >= dr || dt >= db || sl >= sr || st >= sb)
		return -3;
	
	// check source clip: left
	if (sl < scl) {
		d = scl - sl;
		sl = scl;
		if (!hflip) dl += (int)(d * fx);
		else dr -= (int)(d * fx);
	}

	// check source clip: top
	if (st < sct) {
		d = sct - st;
		st = sct;
		if (!vflip) dt += (int)(d * fy);
		else db -= (int)(d * fy);
	}

	if (dl >= dr || dt >= db || sl >= sr || st >= sb)
		return -4;

	// check source clip: right
	if (sr > scr) {
		d = sr - scr;
		sr = scr;
		if (!hflip) dr -= (int)(d * fx);
		else dl += (int)(d * fx);
	}

	// check source clip: bottom
	if (sb > scb) {
		d = sb - scb;
		sb = scb;
		if (!vflip) db -= (int)(d * fy);
		else dt += (int)(d * fy);
	}

	if (dl >= dr || dt >= db || sl >= sr || st >= sb)
		return -5;

	bound_dst->left = dl;
	bound_dst->top = dt;
	bound_dst->right = dr;
	bound_dst->bottom = db;
	bound_src->left = sl;
	bound_src->top = st;
	bound_src->right = sr;
	bound_src->bottom = sb;

	return 0;
}


//---------------------------------------------------------------------
// blit from source bitmap with same bpp
// (mode & PIXEL_FLAG_HFLIP) != 0, for horizontal flip
// (mode & PIXEL_FLAG_VFLIP) != 0, for vertical flip
// (mode & PIXEL_FLAG_MASK)  != 0, for transparent bliting 
// returns zero for successful, others for error
//---------------------------------------------------------------------
int BasicBitmap::Blit(int x, int y, const BasicBitmap *src,
	int sx, int sy, int w, int h, int mode)
{
	if (_bpp != src->_bpp) 
		return -1;

	if ((mode & PIXEL_FLAG_NOCLIP) == 0) {
		int clipdst[4], clipsrc[4], rect[4];
		clipdst[0] = 0;
		clipdst[1] = 0;
		clipdst[2] = (int)_w;
		clipdst[3] = (int)_h;
		clipsrc[0] = 0;
		clipsrc[1] = 0;
		clipsrc[2] = (int)src->_w;
		clipsrc[3] = (int)src->_h;
		rect[0] = sx;
		rect[1] = sy;
		rect[2] = sx + (int)w;
		rect[3] = sy + (int)h;
		int hr = ClipRect(clipdst, clipsrc, &x, &y, rect, mode);
		if (hr != 0) 
			return 0;
		sx = rect[0];
		sy = rect[1];
		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	const void *sbits = src->Line(sy);
	void *dbits = this->Line(y);
	IUINT32 mask = src->GetMask();

	bool opacity = (mode & PIXEL_FLAG_MASK)? false : true;
	int flip = (PIXEL_FLAG_HFLIP | PIXEL_FLAG_VFLIP) & mode;
	PixelBlit hook = NULL;

	switch (_pixelsize) {
	case 1: hook = opacity? PixelBlitNormal1 : PixelBlitMask1; break;
	case 2: hook = opacity? PixelBlitNormal2 : PixelBlitMask2; break;
	case 3: hook = opacity? PixelBlitNormal3 : PixelBlitMask3; break;
	case 4: hook = opacity? PixelBlitNormal4 : PixelBlitMask4; break;
	}

	if (hook != NULL) {
		int retval = hook(dbits, _pitch, x, sbits, src->_pitch, 
							sx, w, h, mask, flip);
		if (retval == 0) 
			return 0;
	}

	if (opacity) {
		BlitNormal(_bpp, dbits, _pitch, x, sbits, src->_pitch, 
			sx, w, h, mask, flip);
	}	
	else {
		BlitMask(_bpp, dbits, _pitch, x, sbits, src->_pitch, 
			sx, w, h, mask, flip);
	}

	return 0;
}


//---------------------------------------------------------------------
// draw pixel
//---------------------------------------------------------------------
int BasicBitmap::Blit(int x, int y, const BasicBitmap *src, 
	const BasicRect *rect, int mode)
{
	int sx, sy, w, h;
	if (rect) {
		sx = rect->left;
		sy = rect->top;
		w = rect->right - rect->left;
		h = rect->bottom - rect->top;
	}	else {
		sx = sy = 0;
		w = src->Width();
		h = src->Height();
	}
	return Blit(x, y, src, sx, sy, w, h, mode);
}


//---------------------------------------------------------------------
// draw pixel
//---------------------------------------------------------------------
void BasicBitmap::SetDriver(int bpp, PixelBlit proc, bool transparent)
{
	switch (bpp) {
	case 8:
		if (transparent) PixelBlitMask1 = proc;
		else PixelBlitNormal1 = proc;
		break;
	case 15:
	case 16:
		if (transparent) PixelBlitMask2 = proc;
		else PixelBlitNormal2 = proc;
		break;
	case 24:
		if (transparent) PixelBlitMask3 = proc;
		else PixelBlitNormal3 = proc;
		break;
	case 32:
		if (transparent) PixelBlitMask4 = proc;
		else PixelBlitNormal4 = proc;
		break;
	}
}


//---------------------------------------------------------------------
// draw pixel
//---------------------------------------------------------------------
void BasicBitmap::SetPixel(int x, int y, IUINT32 color)
{
	if ((unsigned int)x >= (unsigned int)_w) return;
	if ((unsigned int)y >= (unsigned int)_h) return;
	IUINT8 *bits = Line(y);
	switch (_pixelsize) {
	case 1: _pixel_store(8, bits, x, color); break;
	case 2: _pixel_store(16, bits, x, color); break;
	case 3: _pixel_store(24, bits, x, color); break;
	case 4: _pixel_store(32, bits, x, color); break;
	}
}


//---------------------------------------------------------------------
// read pixel
//---------------------------------------------------------------------
IUINT32 BasicBitmap::GetPixel(int x, int y) const
{
	if ((unsigned int)x >= (unsigned int)_w) return 0;
	if ((unsigned int)y >= (unsigned int)_h) return 0;
	const IUINT8 *bits = (const IUINT8*)Line(y);
	IUINT32 cc = 0;
	switch (_pixelsize) {
	case 1: cc = _pixel_fetch(8, bits, x); break;
	case 2: cc = _pixel_fetch(16, bits, x); break;
	case 3: cc = _pixel_fetch(24, bits, x); break;
	case 4: cc = _pixel_fetch(32, bits, x); break;
	}
	return cc;
}


//---------------------------------------------------------------------
// Get A8R8G8B8 from position, different from GetPixel, GetPixel 
// returns raw pixel GetColor will convert raw pixel to A8R8G8B8
//---------------------------------------------------------------------
IUINT32 BasicBitmap::GetColor(int x, int y) const
{
	if ((unsigned int)x >= (unsigned int)_w) return 0;
	if ((unsigned int)y >= (unsigned int)_h) return 0;
	const IUINT8 *bits = (const IUINT8*)Line(y);
	IUINT32 cc = 0, r = 0, g = 0, b = 0, a = 0;
	switch (_fmt) {
	case G8:
		cc = _pixel_fetch(8, bits, x);
		cc = RGBA_TO_A8R8G8B8(255, cc, cc, cc);
		break;
	case X1R5G5B5:
		cc = _pixel_fetch(16, bits, x);
		RGBA_FROM_X1R5G5B5(cc, r, g, b, a);
		cc = RGBA_TO_A8R8G8B8(r, g, b, a);
		break;
	case A1R5G5B5:
		cc = _pixel_fetch(16, bits, x);
		RGBA_FROM_A1R5G5B5(cc, r, g, b, a);
		cc = RGBA_TO_A8R8G8B8(r, g, b, a);
		break;
	case A4R4G4B4:
		cc = _pixel_fetch(16, bits, x);
		RGBA_FROM_A4R4G4B4(cc, r, g, b, a);
		cc = RGBA_TO_A8R8G8B8(r, g, b, a);
		break;
	case R5G6B5:
		cc = _pixel_fetch(16, bits, x);
		RGBA_FROM_R5G6B5(cc, r, g, b, a);
		cc = RGBA_TO_A8R8G8B8(r, g, b, a);
		break;
	case R8G8B8:
		cc = _pixel_fetch(24, bits, x) | 0xff000000;
		break;
	case X8R8G8B8:
		cc = _pixel_fetch(32, bits, x) | 0xff000000;
		break;
	case A8B8G8R8:
		cc = _pixel_fetch(32, bits, x);
		cc = (cc & 0xff00ff00) | ((cc & 0xff) << 16) | ((cc & 0xff0000) >> 16);
		break;
	case UNKNOW:
	case A8R8G8B8:
		cc = _pixel_fetch(32, bits, x);
		break;
	}
	return cc;
}


//---------------------------------------------------------------------
// Set A8R8G8B8 to position, SetPixel set raw pixel, SetColor will
// convert A8R8G8B8 to raw pixel format then set to position
//---------------------------------------------------------------------
void BasicBitmap::SetColor(int x, int y, IUINT32 RGBA)
{
	if ((unsigned int)x >= (unsigned int)_w) return;
	if ((unsigned int)y >= (unsigned int)_h) return;
	IUINT8 *bits = (IUINT8*)Line(y);
	IUINT32 r, g, b, a, c;
	switch (_fmt) {
	case G8:
		RGBA_FROM_A8R8G8B8(RGBA, r, g, b, a);
		c = _pixel_to_gray(r, g, b);
		_pixel_store(8, bits, x, c); 
		break;
	case X1R5G5B5:
		RGBA_FROM_A8R8G8B8(RGBA, r, g, b, a);
		c = RGBA_TO_X1R5G5B5(r, g, b, a);
		_pixel_store(16, bits, x, c); 
		break;
	case A1R5G5B5:
		RGBA_FROM_A8R8G8B8(RGBA, r, g, b, a);
		c = RGBA_TO_A1R5G5B5(r, g, b, a);
		_pixel_store(16, bits, x, c); 
		break;
	case R5G6B5:
		RGBA_FROM_A8R8G8B8(RGBA, r, g, b, a);
		c = RGBA_TO_R5G6B5(r, g, b, a);
		_pixel_store(16, bits, x, c); 
		break;
	case A4R4G4B4:
		RGBA_FROM_A8R8G8B8(RGBA, r, g, b, a);
		c = RGBA_TO_A4R4G4B4(r, g, b, a);
		_pixel_store(16, bits, x, c); 
		break;
	case R8G8B8:
		c = RGBA & 0xffffff;
		_pixel_store(24, bits, x, c); 
		break;
	case X8R8G8B8:
		_pixel_store(32, bits, x, (RGBA & 0xffffff)); 
		break;
	case A8B8G8R8:
		RGBA = (RGBA & 0xff00ff00) | ((RGBA & 0xff) << 16) | ((RGBA & 0xff0000) >> 16);
		_pixel_store(32, bits, x, RGBA);
		break;
	case A8R8G8B8:
		_pixel_store(32, bits, x, RGBA);
		break;
	case UNKNOW:
		break;
	}
}


//---------------------------------------------------------------------
// set pixel batch
//---------------------------------------------------------------------
void BasicBitmap::SetPixels(const short *xylist, int count, IUINT32 color)
{
	unsigned short w = (unsigned short)((_w > 32767)? 32767 : _w);
	unsigned short h = (unsigned short)((_h > 32767)? 32767 : _h);
	unsigned char **lines = _lines;
	int i;
	switch (_pixelsize) {
	case 1:
		for (i = count; i > 0; xylist += 2, i--) {
			unsigned short x = (unsigned short)xylist[0];
			unsigned short y = (unsigned short)xylist[1];
			if (x < w && y < h) {
				_pixel_store(8, lines[y], x, color);
			}
		}
		break;
	case 2:
		for (i = count; i > 0; xylist += 2, i--) {
			unsigned short x = (unsigned short)xylist[0];
			unsigned short y = (unsigned short)xylist[1];
			if (x < w && y < h) {
				_pixel_store(16, lines[y], x, color);
			}
		}
		break;
	case 3:
		for (i = count; i > 0; xylist += 2, i--) {
			unsigned short x = (unsigned short)xylist[0];
			unsigned short y = (unsigned short)xylist[1];
			if (x < w && y < h) {
				_pixel_store(24, lines[y], x, color);
			}
		}
		break;
	case 4:
		for (i = count; i > 0; xylist += 2, i--) {
			unsigned short x = (unsigned short)xylist[0];
			unsigned short y = (unsigned short)xylist[1];
			if (x < w && y < h) {
				_pixel_store(32, lines[y], x, color);
			}
		}
		break;
	}
}


//---------------------------------------------------------------------
// ARGB from native
//---------------------------------------------------------------------
IUINT32 BasicBitmap::Raw2ARGB(IUINT32 color)
{
	IUINT32 a = 255, r = 0, g = 0, b = 0;
	switch (_fmt) {
	case G8:
		r = g = b = color;
		break;
	case X1R5G5B5:
		RGBA_FROM_X1R5G5B5(color, r, g, b, a);
		break;
	case A1R5G5B5:
		RGBA_FROM_A1R5G5B5(color, r, g, b, a);
		break;
	case A4R4G4B4:
		RGBA_FROM_A4R4G4B4(color, r, g, b, a);
		break;
	case R5G6B5:
		RGBA_FROM_R5G6B5(color, r, g, b, a);
		break;
	case R8G8B8:
	case X8R8G8B8:
		return 0xff000000 | color;
	case A8B8G8R8:
		return (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color & 0xff0000) >> 16);
	case UNKNOW:
	case A8R8G8B8:
		return color;
	}

	return _pixel_asm_8888(a, r, g, b);
}


//---------------------------------------------------------------------
// ARGB to native
//---------------------------------------------------------------------
IUINT32 BasicBitmap::ARGB2Raw(IUINT32 argb)
{
	IUINT32 a = 255, r = 0, g = 0, b = 0, c = 0;
	_pixel_disasm_8888(argb, a, r, g, b);
	switch (_fmt) {
	case G8:
		c = _pixel_to_gray(r, g, b);
		break;
	case X1R5G5B5:
		c = RGBA_TO_X1R5G5B5(r, g, b, a);
		break;
	case A1R5G5B5:
		c = RGBA_TO_A1R5G5B5(r, g, b, a);
		break;
	case R5G6B5:
		c = RGBA_TO_R5G6B5(r, g, b, a);
		break;
	case A4R4G4B4:
		c = RGBA_TO_A4R4G4B4(r, g, b, a);
		break;
	case R8G8B8:
		return argb & 0xffffff;
	case X8R8G8B8:
		return argb & 0xffffff;
	case A8B8G8R8:
		return _pixel_asm_8888(a, b, g, r);
	case A8R8G8B8:
		return argb;
	case UNKNOW:
		return argb;
	}
	return c;
}


//---------------------------------------------------------------------
// fetch pixel
//---------------------------------------------------------------------
void BasicBitmap::Fetch(PixelFmt fmt, const void *bits, int x, int w, IUINT32 *buffer)
{
	IUINT32 r, g, b, a;
	switch (fmt) {
	case G8: {
			const IUINT8 *src = (const IUINT8*)bits + x;
			for (; w > 0; w--) {
				IUINT32 cc = *src++; 
				*buffer++ = _pixel_asm_8888(255, cc, cc, cc);
			}
		}
		break;
	case A1R5G5B5: {
			const IUINT16 *src = (const IUINT16*)bits + x;
			for (; w > 0; w--) {
				IUINT32 cc = *src++; 
				_pixel_disasm_1555(cc, a, r, g, b);
				*buffer++ = _pixel_asm_8888(a, r, g, b);
			}
		}
		break;
	case A4R4G4B4: {
			const IUINT16 *src = (const IUINT16*)bits + x;
			for (; w > 0; w--) {
				IUINT32 cc = *src++; 
				_pixel_disasm_4444(cc, a, r, g, b);
				*buffer++ = _pixel_asm_8888(a, r, g, b);
			}
		}
		break;
	case R5G6B5: {
			const IUINT16 *src = (const IUINT16*)bits + x;
			for (; w > 0; w--) {
				IUINT32 cc = *src++; 
				_pixel_disasm_565(cc, r, g, b);
				*buffer++ = _pixel_asm_8888(255, r, g, b);
			}
		}
		break;
	case X1R5G5B5: {
			const IUINT16 *src = (const IUINT16*)bits + x;
			for (; w > 0; w--) {
				IUINT32 cc = *src++; 
				_pixel_disasm_X555(cc, r, g, b);
				*buffer++ = _pixel_asm_8888(255, r, g, b);
			}
		}
		break;
	case R8G8B8: {
			const IUINT8 *src = (const IUINT8*)bits + x * 3;
			IUINT32 c1, c2;
			ILINS_LOOP_DOUBLE(
					{ 
						c1 = _pixel_read_24(src) | 0xff000000; 
						src += 3; 
						*buffer++ = c1; 
					},
					{
						c1 = _pixel_read_24(src + 0) | 0xff000000;
						c2 = _pixel_read_24(src + 3) | 0xff000000;
						src += 6;
						*buffer++ = c1;
						*buffer++ = c2;
					},
					w
				);
		}
		break;
	case A8R8G8B8: {
			internal_memcpy(buffer, (const IUINT32*)bits + x, w * sizeof(IUINT32));
		}
		break;
	case A8B8G8R8: {
			const IUINT32 *pixel = (const IUINT32*)bits + x;
			ILINS_LOOP_DOUBLE( 
				{
					*buffer++ = ((*pixel & 0xff00ff00) |
						((*pixel & 0xff0000) >> 16) | ((*pixel & 0xff) << 16));
					pixel++;
				},
				{
					*buffer++ = ((*pixel & 0xff00ff00) |
						((*pixel & 0xff0000) >> 16) | ((*pixel & 0xff) << 16));
					pixel++;
					*buffer++ = ((*pixel & 0xff00ff00) |
						((*pixel & 0xff0000) >> 16) | ((*pixel & 0xff) << 16));
					pixel++;
				},
				w);
		}
		break;
	case X8R8G8B8: {
			const IUINT32 *src = (const IUINT32*)bits + x;
			ILINS_LOOP_DOUBLE( 
				{
					*buffer++ = *src++ | 0xff000000;
				},
				{
					*buffer++ = *src++ | 0xff000000;
					*buffer++ = *src++ | 0xff000000;
				},
				w);
		}
		break;
	case UNKNOW:
		break;
	}
}


//---------------------------------------------------------------------
// store pixel
//---------------------------------------------------------------------
void BasicBitmap::Store(PixelFmt fmt, void *bits, int x, int w, const IUINT32 *buffer)
{
	IUINT32 r, g, b, a;
	switch (fmt) {
	case G8: {
			IUINT8 *dst = (IUINT8*)bits + x;
			for (; w > 0; w--) {
				_pixel_load_card(buffer, r, g, b, a);
				buffer++;
				*dst++ = (IUINT8)_pixel_to_gray(r, g, b);
			}
		}
		break;
	case A1R5G5B5: {
			IUINT16 *dst = (IUINT16*)bits + x;
			for (; w > 0; w--) {
				_pixel_load_card(buffer, r, g, b, a);
				buffer++;
				*dst++ = (IUINT16)_pixel_asm_1555(a, r, g, b);
			}
		}
		break;
	case A4R4G4B4: {
			IUINT16 *dst = (IUINT16*)bits + x;
			for (; w > 0; w--) {
				_pixel_load_card(buffer, r, g, b, a);
				buffer++;
				*dst++ = (IUINT16)_pixel_asm_4444(a, r, g, b);
			}
		}
		break;
	case R5G6B5: {
			IUINT16 *dst = (IUINT16*)bits + x;
			for (; w > 0; w--) {
				_pixel_load_card(buffer, r, g, b, a);
				buffer++;
				*dst++ = (IUINT16)_pixel_asm_565(r, g, b);
			}
		}
		break;
	case X1R5G5B5: {
			IUINT16 *dst = (IUINT16*)bits + x;
			for (; w > 0; w--) {
				_pixel_load_card(buffer, r, g, b, a);
				buffer++;
				*dst++ = (IUINT16)_pixel_asm_1555(0, r, g, b);
			}
		}
		break;
	case R8G8B8: {
			IUINT8 *dst = (IUINT8*)bits + x * 3;
			IUINT32 c1, c2;
			ILINS_LOOP_DOUBLE(
					{ 
						c1 = *buffer++;
						_pixel_write_24(dst, c1);
						dst += 3; 
					},
					{
						c1 = *buffer++;
						c2 = *buffer++;
						_pixel_write_24(dst + 0, c1);
						_pixel_write_24(dst + 3, c2);
						dst += 6;
					},
					w
				);			
		}
		break;
	case A8R8G8B8: {
			IUINT32 *dst = (IUINT32*)bits + x;
			internal_memcpy(dst, buffer, sizeof(IUINT32) * w);
		}
		break;
	case A8B8G8R8: {
			IUINT32 *dst = (IUINT32*)bits + x;
			ILINS_LOOP_DOUBLE( 
				{
					*dst++ = (buffer[0] & 0xff00ff00) |
						((buffer[0] & 0xff0000) >> 16) | ((buffer[0] & 0xff) << 16);
					buffer++;
				},
				{
					*dst++ = (buffer[0] & 0xff00ff00) |
						((buffer[0] & 0xff0000) >> 16) | ((buffer[0] & 0xff) << 16);
					buffer++;
					*dst++ = (buffer[0] & 0xff00ff00) |
						((buffer[0] & 0xff0000) >> 16) | ((buffer[0] & 0xff) << 16);
					buffer++;
				},
				w);			
		}
		break;
	case X8R8G8B8: {
			IUINT32 *dst = (IUINT32*)bits + x;
			ILINS_LOOP_DOUBLE( 
				{
					*dst++ = buffer[0] & 0xffffff;
					buffer++;
				},
				{
					*dst++ = buffer[0] & 0xffffff;
					buffer++;
					*dst++ = buffer[0] & 0xffffff;
					buffer++;
				},
				w);
		}
		break;
	case UNKNOW: 
		break;
	}
}


//---------------------------------------------------------------------
// bpp -> fmt
//---------------------------------------------------------------------
BasicBitmap::PixelFmt BasicBitmap::Bpp2Fmt(int bpp)
{
	switch (bpp) {
	case 8:
		return G8;
	case 15:
		return A1R5G5B5;
	case 16:
		return R5G6B5;
	case 24:
		return R8G8B8;
	case 32:
		return A8R8G8B8;
	}
	return A8R8G8B8;
}

int BasicBitmap::Fmt2Bpp(PixelFmt fmt)
{
	int bpp = 0;
	switch (fmt) {
	case A8R8G8B8: bpp = 32; break;
	case A8B8G8R8: bpp = 32; break;
	case X8R8G8B8: bpp = 32; break;
	case R8G8B8: bpp = 24; break;
	case R5G6B5: bpp = 16; break;
	case A1R5G5B5: bpp = 16; break;
	case X1R5G5B5: bpp = 16; break;
	case A4R4G4B4: bpp = 16; break;
	case G8: bpp = 8; break;
	case UNKNOW: bpp = 8; break;
	}
	return bpp;
}


//---------------------------------------------------------------------
// reverse A8R8G8B8
//---------------------------------------------------------------------
void BasicBitmap::CardReverse(IUINT32 *card, int size)
{
	IUINT32 *p1, *p2;
	IUINT32 value;
	for (p1 = card, p2 = card + size - 1; p1 < p2; p1++, p2--) {
		value = *p1;
		*p1 = *p2;
		*p2 = value;
	}
}


void BasicBitmap::CardMultiply(IUINT32 *card, int size, IUINT32 color) 
{
	IUINT32 r1, g1, b1, a1, r2, g2, b2, a2, f;
	RGBA_FROM_A8R8G8B8(color, r1, g1, b1, a1);
	if ((color & 0xffffff) == 0xffffff) f = 1;
	else f = 0;
	if (color == 0xffffffff) {
		return;
	}
	else if (color == 0) {
		memset(card, 0, sizeof(IUINT32) * size);
	}
	else if (f) {
		IUINT8 *src = (IUINT8*)card;
		if (a1 == 0) {
			for (; size > 0; size--) {
			#if IWORDS_BIG_ENDIAN
				src[0] = 0;
			#else
				src[3] = 0;
			#endif
				src += sizeof(IUINT32);
			}
			return;
		}
		a1 = _pixel_norm(a1);
		for (; size > 0; size--) {
		#if IWORDS_BIG_ENDIAN
			a2 = src[0];
			src[0] = (IUINT8)((a2 * a1) >> 8);
		#else
			a2 = src[3];
			src[3] = (IUINT8)((a2 * a1) >> 8);
		#endif
			src += sizeof(IUINT32);
		}
	}
	else {
		IUINT8 *src = (IUINT8*)card;
		a1 = _pixel_norm(a1);
		r1 = _pixel_norm(r1);
		g1 = _pixel_norm(g1);
		b1 = _pixel_norm(b1);
		for (; size > 0; src += sizeof(IUINT32), size--) {
			_pixel_load_card(src, r2, g2, b2, a2);
			r2 = (r1 * r2) >> 8;
			g2 = (g1 * g2) >> 8;
			b2 = (b1 * b2) >> 8;
			a2 = (a1 * a2) >> 8;
			*((IUINT32*)src) = RGBA_TO_A8R8G8B8(r2, g2, b2, a2);
		}
	}
}


void BasicBitmap::CardSetAlpha(IUINT32 *card, int size, IUINT32 alpha)
{
	unsigned char *dd = (unsigned char*)card;
	IUINT8 aa = (IUINT8)(alpha & 0xff);

#if IWORDS_BIG_ENDIAN
	ILINS_LOOP_DOUBLE(
		{
			dd[0] = aa;
			dd += 4;
		},
		{
			dd[0] = aa;
			dd[4] = aa;
			dd += 8;
		},
		size);
#else
	ILINS_LOOP_DOUBLE(
		{
			dd[3] = aa;
			dd += 4;
		},
		{
			dd[3] = aa;
			dd[7] = aa;
			dd += 8;
		},
		size);
#endif
}


void BasicBitmap::CardPremultiply(IUINT32 *card, int size, bool reverse)
{
	if (reverse == false) {
		for (; size > 0; card++, size--) {
			IUINT32 r, g, b, a;
			_pixel_load_card(card, r, g, b, a);
			r = r * a;
			g = g * a;
			b = b * a;
			r = _pixel_fast_div_255(r);
			g = _pixel_fast_div_255(g);
			b = _pixel_fast_div_255(b);
			_pixel_save_card(card, r, g, b, a);
		}
	}	
	else {
		for (; size > 0; card++, size--) {
			IUINT32 r, g, b, a;
			_pixel_load_card(card, r, g, b, a);
			if (a > 0) {
				r = r * 255 / a;
				g = g * 255 / a;
				b = b * 255 / a;
			}	
			else {
				r = g = b = 0;
			}
			_pixel_save_card(card, r, g, b, a);
		}
	}
}


//---------------------------------------------------------------------
// convert from different bpp 
//---------------------------------------------------------------------
int BasicBitmap::Convert(int x, int y, const BasicBitmap *src, int sx, int sy,
	int w, int h, int mode)
{
	if (_fmt == src->_fmt) {
		return Blit(x, y, src, sx, sy, w, h, mode);
	}

	if ((mode & PIXEL_FLAG_NOCLIP) == 0) {
		int clipdst[4], clipsrc[4], rect[4];
		clipdst[0] = 0;
		clipdst[1] = 0;
		clipdst[2] = (int)_w;
		clipdst[3] = (int)_h;
		clipsrc[0] = 0;
		clipsrc[1] = 0;
		clipsrc[2] = (int)src->_w;
		clipsrc[3] = (int)src->_h;
		rect[0] = sx;
		rect[1] = sy;
		rect[2] = sx + (int)w;
		rect[3] = sy + (int)h;
		int hr = ClipRect(clipdst, clipsrc, &x, &y, rect, mode);
		if (hr != 0) 
			return 0;
		sx = rect[0];
		sy = rect[1];
		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	PixelFmt sfmt = src->Format();
	PixelFmt dfmt = Format();

	bool hflip = (mode & PIXEL_FLAG_HFLIP)? true : false;
	bool vflip = (mode & PIXEL_FLAG_VFLIP)? true : false;

	int srcy = sy, incy = 1;
	if (vflip) srcy = sy + h - 1, incy = -1;

	if (dfmt == A8R8G8B8) {
		for (int j = 0; j < h; srcy += incy, j++) {
			IUINT32 *dd = (IUINT32*)Line(y + j) + x;
			const void *ss = src->Line(srcy);
			Fetch(sfmt, ss, sx, w, dd);
			if (hflip) {
				CardReverse(dd, w);
			}
		}
	}
	else if (sfmt == A8R8G8B8 && hflip == false) {
		for (int j = 0; j < h; srcy += incy, j++) {
			IUINT32 *ss = (IUINT32*)src->Line(srcy) + x;
			void *dd = Line(y + j);
			Store(dfmt, dd, x, w, ss);
		}
	}
	else {
		unsigned char _buffer[PIXEL_STACK_BUFFER];
		unsigned char *buffer = _buffer;
		if (w * 4 > PIXEL_STACK_BUFFER) {
			buffer = (unsigned char*)internal_align_malloc(w * 4, 16);
			if (buffer == NULL) return -1;
		}
		for (int j = 0; j < h; j++) {
			const void *ss = src->Line(srcy);
			void *dd = Line(y + j);
			Fetch(sfmt, ss, sx, w, (IUINT32*)buffer);
			if (hflip) {
				CardReverse((IUINT32*)buffer, w);			
			}
			Store(dfmt, dd, x, w, (IUINT32*)buffer);
			srcy += incy;
		}
		if (buffer != _buffer) {
			internal_align_free(buffer);
		}
	}

	return 0;
}


//---------------------------------------------------------------------
// Draw Span
//---------------------------------------------------------------------
#define PIXEL_SPAN_DRAW_PROC_N(fmt, bpp, nbytes, mode) \
static int pixel_span_draw_proc_##fmt##_0(void *bits, \
	int offset, int w, const IUINT32 *card) \
{ \
	unsigned char *dst = ((unsigned char*)bits) + offset * nbytes; \
	IUINT32 cc, r1, g1, b1, a1 = 0, r2, g2, b2, a2 = 0, inc; \
	for (inc = w; inc > 0; inc--) { \
		_pixel_load_card(card, r1, g1, b1, a1); \
		if (a1 == 255) { \
			cc = RGBA_TO_PIXEL(fmt, r1, g1, b1, 255); \
			_pixel_store(bpp, dst, 0, cc); \
		} \
		else if (a1 > 0) { \
			cc = _pixel_fetch(bpp, dst, 0); \
			RGBA_FROM_PIXEL(fmt, cc, r2, g2, b2, a2); \
			BLEND_##mode(r1, g1, b1, a1, r2, g2, b2, a2); \
			cc = RGBA_TO_PIXEL(fmt, r2, g2, b2, a2); \
			_pixel_store(bpp, dst, 0, cc); \
		} \
		card++; \
		dst += nbytes; \
	} \
	return a1 + a2; \
} \
static int pixel_span_draw_proc_##fmt##_1(void *bits, \
	int offset, int w, const IUINT32 *card) \
{ \
	unsigned char *dst = ((unsigned char*)bits) + offset * nbytes; \
	IUINT32 cc, r1, g1, b1, a1 = 0, r2, g2, b2, a2 = 0, inc; \
	for (inc = w; inc > 0; inc--) { \
		_pixel_load_card(card, r1, g1, b1, a1); \
		if (a1 == 255) { \
			cc = RGBA_TO_PIXEL(fmt, r1, g1, b1, 255); \
			_pixel_store(bpp, dst, 0, cc); \
		} \
		else if (a1 > 0) { \
			cc = _pixel_fetch(bpp, dst, 0); \
			RGBA_FROM_PIXEL(fmt, cc, r2, g2, b2, a2); \
			BLEND_SRCOVER(r1, g1, b1, a1, r2, g2, b2, a2); \
			cc = RGBA_TO_PIXEL(fmt, r2, g2, b2, a2); \
			_pixel_store(bpp, dst, 0, cc); \
		} \
		card++; \
		dst += nbytes; \
	} \
	return a1 + a2; \
} \
static int pixel_span_draw_proc_##fmt##_2(void *bits, \
	int offset, int w, const IUINT32 *card) \
{ \
	unsigned char *dst = ((unsigned char*)bits) + offset * nbytes; \
	IUINT32 cc, r1, g1, b1, a1 = 0, r2, g2, b2, a2 = 0, inc; \
	for (inc = w; inc > 0; inc--) { \
		_pixel_load_card(card, r1, g1, b1, a1); \
		if (a1 > 0) { \
			cc = _pixel_fetch(bpp, dst, 0); \
			RGBA_FROM_PIXEL(fmt, cc, r2, g2, b2, a2); \
			BLEND_ADDITIVE(r1, g1, b1, a1, r2, g2, b2, a2); \
			cc = RGBA_TO_PIXEL(fmt, r2, g2, b2, a2); \
			_pixel_store(bpp, dst, 0, cc); \
		} \
		card++; \
		dst += nbytes; \
	} \
	return a2 + a1; \
} 

PIXEL_SPAN_DRAW_PROC_N(A8R8G8B8, 32, 4, NORMAL_FAST)
PIXEL_SPAN_DRAW_PROC_N(A8B8G8R8, 32, 4, NORMAL_FAST)
PIXEL_SPAN_DRAW_PROC_N(X8R8G8B8, 32, 4, STATIC)

PIXEL_SPAN_DRAW_PROC_N(R8G8B8, 24, 3, STATIC)
PIXEL_SPAN_DRAW_PROC_N(A1R5G5B5, 16, 2, NORMAL_FAST)
PIXEL_SPAN_DRAW_PROC_N(A4R4G4B4, 16, 2, NORMAL_FAST)
PIXEL_SPAN_DRAW_PROC_N(X1R5G5B5, 16, 2, STATIC)
PIXEL_SPAN_DRAW_PROC_N(R5G6B5, 16, 2, STATIC)
PIXEL_SPAN_DRAW_PROC_N(G8, 8, 1, STATIC)


static BasicBitmap::PixelDraw BitmapSpanFn[12][3] = {
	{ NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL },
	{ NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL },
	{ NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL },
	{ NULL, NULL, NULL }, { NULL, NULL, NULL }, { NULL, NULL, NULL },
};


BasicBitmap::PixelDraw BasicBitmap::GetDriver(BasicBitmap::PixelFmt fmt, int mode, bool builtin)
{
	static int inited = 0;
	PixelDraw proc = NULL;

	if (inited == 0) {
		PixelInitLut();
		inited = 1;
	}

	switch (fmt) {
	case A8R8G8B8: 
		if (mode == 0) proc = pixel_span_draw_proc_A8R8G8B8_0;
		else if (mode == 1) proc = pixel_span_draw_proc_A8R8G8B8_1;
		else proc = pixel_span_draw_proc_A8R8G8B8_2;
		break;
	case A8B8G8R8: 
		if (mode == 0) proc = pixel_span_draw_proc_A8B8G8R8_0;
		else if (mode == 1) proc = pixel_span_draw_proc_A8B8G8R8_1;
		else proc = pixel_span_draw_proc_A8B8G8R8_2;
		break;
	case X8R8G8B8:
		if (mode == 0) proc = pixel_span_draw_proc_X8R8G8B8_0;
		else if (mode == 1) proc = pixel_span_draw_proc_X8R8G8B8_1;
		else proc = pixel_span_draw_proc_X8R8G8B8_2;
		break;
	case R8G8B8: 
		if (mode == 0) proc = pixel_span_draw_proc_R8G8B8_0;
		else if (mode == 1) proc = pixel_span_draw_proc_R8G8B8_1;
		else proc = pixel_span_draw_proc_R8G8B8_2;
		break;
	case R5G6B5:
		if (mode == 0) proc = pixel_span_draw_proc_R5G6B5_0;
		else if (mode == 1) proc = pixel_span_draw_proc_R5G6B5_1;
		else proc = pixel_span_draw_proc_R5G6B5_2;
		break;
	case A1R5G5B5: 
		if (mode == 0) proc = pixel_span_draw_proc_A1R5G5B5_0;
		else if (mode == 1) proc = pixel_span_draw_proc_A1R5G5B5_1;
		else proc = pixel_span_draw_proc_A1R5G5B5_2;
		break;
	case A4R4G4B4:
		if (mode == 0) proc = pixel_span_draw_proc_A4R4G4B4_0;
		else if (mode == 1) proc = pixel_span_draw_proc_A4R4G4B4_1;
		else proc = pixel_span_draw_proc_A4R4G4B4_2;
		break;
	case X1R5G5B5:
		if (mode == 0) proc = pixel_span_draw_proc_X1R5G5B5_0;
		else if (mode == 1) proc = pixel_span_draw_proc_X1R5G5B5_1;
		else proc = pixel_span_draw_proc_X1R5G5B5_2;
		break;
	case G8: 
		if (mode == 0) proc = pixel_span_draw_proc_G8_0;
		else if (mode == 1) proc = pixel_span_draw_proc_G8_1;
		else proc = pixel_span_draw_proc_G8_2;
		break;
	case UNKNOW:
		proc = NULL;
		break;
	}

	if (builtin == false) {
		PixelDraw user = BitmapSpanFn[(int)fmt][mode & 3];
		if (user != NULL) return user;
	}

	return proc;
}

void BasicBitmap::SetDriver(PixelFmt fmt, BasicBitmap::PixelDraw proc, int mode)
{
	BitmapSpanFn[(int)fmt][mode] = proc;
}


//---------------------------------------------------------------------
// Alpha Blend
//---------------------------------------------------------------------
int BasicBitmap::Blend(int x, int y, const BasicBitmap *src, int sx, int sy,
	int w, int h, int mode, IUINT32 color)
{
	if ((mode & PIXEL_FLAG_NOCLIP) == 0) {
		int clipdst[4], clipsrc[4], rect[4];
		clipdst[0] = 0;
		clipdst[1] = 0;
		clipdst[2] = (int)_w;
		clipdst[3] = (int)_h;
		clipsrc[0] = 0;
		clipsrc[1] = 0;
		clipsrc[2] = (int)src->_w;
		clipsrc[3] = (int)src->_h;
		rect[0] = sx;
		rect[1] = sy;
		rect[2] = sx + (int)w;
		rect[3] = sy + (int)h;
		int hr = ClipRect(clipdst, clipsrc, &x, &y, rect, mode);
		if (hr != 0) 
			return 0;
		sx = rect[0];
		sy = rect[1];
		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	const void *sbits = src->Line(sy);
	void *dbits = this->Line(y);
	long spitch = src->Pitch();

	PixelFmt sfmt = src->Format();
	PixelFmt dfmt = Format();

	if (mode & PIXEL_FLAG_VFLIP) {
		sbits = src->Line(sy + h - 1);
		spitch = -src->Pitch();
	}

	int op = 0;

	if (mode & PIXEL_FLAG_SRCOVER) op = 1;
	else if (mode & PIXEL_FLAG_ADDITIVE) op = 2;

	PixelDraw draw = GetDriver(dfmt, op, false);

	if ((mode & PIXEL_FLAG_SRCCOPY) != 0 || draw == NULL) {
		return Convert(x, y, src, sx, sy, w, h, mode);
	}

	if (sfmt == A8R8G8B8 && (mode & PIXEL_FLAG_HFLIP) == 0 && color == 0xffffffff) {
		for (int j = 0; j < h; j++) {
			draw(dbits, x, w, (const IUINT32*)sbits + sx);
			dbits = (char*)dbits + _pitch;
			sbits = (const char*)sbits + spitch;
		}
	}
	else {
		unsigned char _buffer[PIXEL_STACK_BUFFER];
		unsigned char *buffer = _buffer;
		if (w * 4 > PIXEL_STACK_BUFFER) {
			buffer = (unsigned char*)internal_align_malloc(w * 4, 16);
			if (buffer == NULL) return -1;
		}
		IUINT32 *card = (IUINT32*)buffer;
		for (int j = 0; j < h; j++) {
			Fetch(sfmt, sbits, sx, w, card);
			if (mode & PIXEL_FLAG_HFLIP) {
				CardReverse(card, w);
			}
			if (color != 0xffffffff) {
				CardMultiply(card, w, color);
			}
			draw(dbits, x, w, card);
			dbits = (char*)dbits + _pitch;
			sbits = (const char*)sbits + spitch;
		}
		if (buffer != _buffer) {
			internal_align_free(buffer);
		}
	}

	return 0;
}


//---------------------------------------------------------------------
// set pixel batch
//---------------------------------------------------------------------
void BasicBitmap::DrawLine(int x1, int y1, int x2, int y2, IUINT32 color)
{
	int x, y;
	if (x1 == x2 && y1 == y2) {
		SetPixel(x1, y1, color);
		return;
	}	else if (x1 == x2) {
		int inc = (y1 <= y2)? 1 : -1;
		for (y = y1; y != y2; y += inc) SetPixel(x1, y, color);
		SetPixel(x2, y2, color);
	}	else if (y1 == y2) {
		int inc = (x1 <= x2)? 1 : -1;
		for (x = x1; x != x2; x += inc) SetPixel(x, y1, color);
		SetPixel(x2, y2, color);
	}	else {
		int dx = (x1 < x2)? x2 - x1 : x1 - x2;
		int dy = (y1 < y2)? y2 - y1 : y1 - y2;
		int rem = 0;
		if (dx >= dy) {
			if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; x <= x2; x++) {
				SetPixel(x, y, color);
				rem += dy;
				if (rem >= dx) {
					rem -= dx;
					y += (y2 >= y1)? 1 : -1;
					SetPixel(x, y, color);
				}
			}
			SetPixel(x2, y2, color);
		}	else {
			if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
			for (x = x1, y = y1; y <= y2; y++) {
				SetPixel(x, y, color);
				rem += dx;
				if (rem >= dy) {
					rem -= dy;
					x += (x2 >= x1)? 1 : -1;
					SetPixel(x, y, color);
				}
			}
			SetPixel(x2, y2, color);
		}
	}
}


//---------------------------------------------------------------------
// draw 8x16 glyph
//---------------------------------------------------------------------
void BasicBitmap::DrawGlyph(int x, int y, const void *glyph, int w, int h, IUINT32 color)
{
	const unsigned char *data = (const unsigned char*)glyph;
	short xylist[16 * 16 * 2];
	int n = 0, i, j;
	if (h > 16 || w > 16) return;
	if (w == 8) {
		for (j = 0; j < h; j++) {
			unsigned char mask;
			for (i = 0, mask = 0x80; i < 8; mask >>= 1, i++) {
				if (mask & data[0]) {
					xylist[n + 0] = (short)(x + i);
					xylist[n + 1] = (short)(y + j);
					n += 2;
				}
			}
			data++;
		}
	}	
	else if (w == 16) {
		for (j = 0; j < h; j++) {
			unsigned char mask;
			for (i = 0, mask = 0x80; i < 8; mask >>= 1, i++) {
				if (mask & data[0]) {
					xylist[n + 0] = (short)(x + i);
					xylist[n + 1] = (short)(y + j);
					n += 2;
				}
			}
			data++;
			for (i = 0, mask = 0x80; i < 8; mask >>= 1, i++) {
				if (mask & data[0]) {
					xylist[n + 0] = (short)(x + i + 8);
					xylist[n + 1] = (short)(y + j);
					n += 2;
				}
			}
			data++;
		}
	}
	if (n > 0) {
		SetPixels(xylist, n >> 1, color);
	}
}


//---------------------------------------------------------------------
// split r, g, b, a channels
//---------------------------------------------------------------------
int BasicBitmap::SplitChannel(BasicBitmap *R, BasicBitmap *G, BasicBitmap *B, BasicBitmap *A)
{
	BasicBitmap *vector[4];
	int w = _w;
	int h = _h;
	if (R) {
		if (R->Bpp() != 8 || R->Width() < w || R->Height() < h) 
			return -1;
	}
	if (G) {
		if (G->Bpp() != 8 || G->Width() < w || G->Height() < h) 
			return -1;
	}
	if (B) {
		if (B->Bpp() != 8 || B->Width() < w || B->Height() < h) 
			return -1;
	}
	if (A) {
		if (A->Bpp() != 8 || A->Width() < w || A->Height() < h) 
			return -1;
	}

	IUINT32 *buffer = new IUINT32[w];
	PixelFmt fmt = Format();

#if IWORDS_BIG_ENDIAN
	vector[3] = B;
	vector[2] = G;
	vector[1] = R;
	vector[0] = A;
#else
	vector[0] = B;
	vector[1] = G;
	vector[2] = R;
	vector[3] = A;
#endif

	for (int j = 0; j < h; j++) {
		IUINT8 *ptr = (IUINT8*)buffer;
		Fetch(fmt, Line(j), 0, w, buffer);
		for (int i = 0; i < 4; i++) {
			BasicBitmap *bmp = vector[i];
			if (bmp != NULL) {
				IUINT8 *src = ptr + i;
				IUINT8 *dst = bmp->Line(j);
				for (int x = w; x > 0; x++) {
					*dst = *src;
					src += 4;
					dst += 1;
				}
			}
		}
	}

	delete []buffer;

	return 0;
}


//---------------------------------------------------------------------
// merge channel
//---------------------------------------------------------------------
int BasicBitmap::MergeChannel(const BasicBitmap *R, const BasicBitmap *G, 
	const BasicBitmap *B, const BasicBitmap *A)
{
	const BasicBitmap *vector[4];
	int w = _w;
	int h = _h;
	if (R) {
		if (R->Bpp() != 8 || R->Width() < w || R->Height() < h) 
			return -1;
	}
	if (G) {
		if (G->Bpp() != 8 || G->Width() < w || G->Height() < h) 
			return -1;
	}
	if (B) {
		if (B->Bpp() != 8 || B->Width() < w || B->Height() < h) 
			return -1;
	}
	if (A) {
		if (A->Bpp() != 8 || A->Width() < w || A->Height() < h) 
			return -1;
	}

	IUINT32 *buffer = new IUINT32[w];
	PixelFmt fmt = Format();

#if IWORDS_BIG_ENDIAN
	vector[3] = B;
	vector[2] = G;
	vector[1] = R;
	vector[0] = A;
#else
	vector[0] = B;
	vector[1] = G;
	vector[2] = R;
	vector[3] = A;
#endif

	for (int j = 0; j < h; j++) {
		IUINT8 *ptr = (IUINT8*)buffer;
		Fetch(fmt, Line(j), 0, w, buffer);
		for (int i = 0; i < 4; i++) {
			const BasicBitmap *bmp = vector[i];
			if (bmp != NULL) {
				IUINT8 *dst = ptr + i;
				const IUINT8 *src = bmp->Line(j);
				for (int x = w; x > 0; x++) {
					*dst = *src;
					dst += 4;
					src += 1;
				}
			}
		}
	}

	delete []buffer;

	return 0;
}


//---------------------------------------------------------------------
// load file content
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::Chop(int x, int y, int w, int h, PixelFmt fmt)
{
	BasicBitmap *bmp = new BasicBitmap(w, h, fmt);
	bmp->Convert(0, 0, this, x, y, w, h);
	return bmp;
}


//---------------------------------------------------------------------
// Flip 
//---------------------------------------------------------------------
void BasicBitmap::FlipHorizontal()
{
	int x1, x2;
	for (int y = 0; y < _h; y++) {
		switch (_pixelsize) {
		case 1: {
				IUINT8 *ptr = (IUINT8*)Line(y);
				IUINT8 cc;
				for (x1 = 0, x2 = _w - 1; x1 < x2; x1++, x2--) {
					cc = ptr[x1];
					ptr[x1] = ptr[x2];
					ptr[x2] = cc;
				}
			}
			break;
		case 2: {
				IUINT16 *ptr = (IUINT16*)Line(y);
				IUINT16 cc;
				for (x1 = 0, x2 = _w - 1; x1 < x2; x1++, x2--) {
					cc = ptr[x1];
					ptr[x1] = ptr[x2];
					ptr[x2] = cc;
				}
			}
			break;
		case 3: {
				IUINT8 *ptr = (IUINT8*)Line(y);
				IUINT32 c1, c2;
				for (x1 = 0, x2 = _w - 1; x1 < x2; x1++, x2--) {
					c1 = _pixel_fetch(24, ptr, x1);
					c2 = _pixel_fetch(24, ptr, x2);
					_pixel_store(24, ptr, x1, c2);
					_pixel_store(24, ptr, x2, c1);
				}
			}
			break;
		case 4: {
				IUINT32 *ptr = (IUINT32*)Line(y);
				IUINT32 cc;
				for (x1 = 0, x2 = _w - 1; x1 < x2; x1++, x2--) {
					cc = ptr[x1];
					ptr[x1] = ptr[x2];
					ptr[x2] = cc;
				}
			}
			break;
		}
	}
}


//---------------------------------------------------------------------
// row fetch
//---------------------------------------------------------------------
void BasicBitmap::RowFetch(int x, int y, IUINT32 *card, int w) const
{
	if (y >= 0 && y < _h) {
		const IUINT8 *row = _lines[y];
		if (x < 0) {
			w += x;
			card -= x;
			x = 0;
			if (w < 0) return;
		}
		if (x + w >= _w) {
			w = _w - x;
			if (w < 0) return;
		}
		Fetch(_fmt, row, x, w, card);
	}
}


//---------------------------------------------------------------------
// row store
//---------------------------------------------------------------------
void BasicBitmap::RowStore(int x, int y, const IUINT32 *card, int w)
{
	if (y >= 0 && y < _h) {
		IUINT8 *row = _lines[y];
		if (x < 0) {
			w += x;
			card -= x;
			x = 0;
			if (w < 0) return;
		}
		if (x + w >= _w) {
			w = _w - x;
			if (w < 0) return;
		}
		Store(_fmt, row, x, w, card);
	}
}


//---------------------------------------------------------------------
// use for 256 palette color format
//---------------------------------------------------------------------
void BasicBitmap::RowFetchWithPalette(int x, int y, IUINT32 *card, int w, 
	const BasicColor *pal) const
{
	if (_bpp != 8) return;
	if (y >= 0 && y < _h) {
		const IUINT8 *ptr = _lines[y];
		if (x < 0) {
			w += x;
			card -= x;
			x = 0;
			if (w < 0) return;
		}
		if (x + w >= _w) {
			w = _w - x;
			if (w < 0) return;
		}
		for (ptr = ptr + x; w > 0; ptr++, w--) {
			const BasicColor *cc = pal + ptr[0];
			*card++ = _pixel_asm_8888(255, cc->r, cc->g, cc->b);
		}
	}
}


//---------------------------------------------------------------------
// use for 256 palette color format
//---------------------------------------------------------------------
void BasicBitmap::RowStoreWithPalette(int x, int y, IUINT32 *card, int w, 
	const BasicColor *pal)
{
	IUINT32 r, g, b, a = 0;
	if (_bpp != 8) return;
	if (y >= 0 && y < _h) {
		IUINT8 *ptr = _lines[y];
		if (x < 0) {
			w += x;
			card -= x;
			x = 0;
			if (w < 0) return;
		}
		if (x + w >= _w) {
			w = _w - x;
			if (w < 0) return;
		}
		for (ptr = ptr + x; w > 0; card++, ptr++, w--) {
			_pixel_load_card(card, r, g, b, a);
			ptr[0] = (IUINT8)BestfitColor(pal, r, g, b, 256);
		}
	}
	r = a;
}


//---------------------------------------------------------------------
// Search Palette
//---------------------------------------------------------------------
int BasicBitmap::BestfitColor(const BasicColor *pal, int r, int g, int b, int size)
{
	static IUINT32 diff_lookup[512 * 3] = { 0 };
	long lowest = 0x7FFFFFFF, bestfit = 0;
	long coldiff, i;
	const BasicColor *rgb;

	if (diff_lookup[0] == 0) {
		for (i = 0; i < 256; i++) {
			int k = i * i;
			diff_lookup[ 256 + i] = diff_lookup[ 256 - i] = k * 30 * 30;
			diff_lookup[ 768 + i] = diff_lookup[ 768 - i] = k * 59 * 59;
			diff_lookup[1280 + i] = diff_lookup[1280 - i] = k * 11 * 11;
		}
		diff_lookup[0] = 1;
	}

	/* range correction */
	r = r & 255;
	g = g & 255;
	b = b & 255;

	for (i = size, rgb = pal; i > 0; rgb++, i--) {
		coldiff  = diff_lookup[ 768 + rgb->g - g];
		if (coldiff >= lowest) continue;

		coldiff += diff_lookup[ 256 + rgb->r - r];
		if (coldiff >= lowest) continue;

		coldiff += diff_lookup[1280 + rgb->b - b];
		if (coldiff >= lowest) continue;

		bestfit = (int)(rgb - pal); 
		if (coldiff == 0) return bestfit;
		lowest = coldiff;
	}

	return bestfit;
}


//---------------------------------------------------------------------
// Flip
//---------------------------------------------------------------------
void BasicBitmap::FlipVertical()
{
	IUINT8 *buffer = new IUINT8[_pitch];
	for (int y1 = 0, y2 = _h - 1; y1 < y2; y1++, y2--) {
		internal_memcpy(buffer, Line(y1), _pitch);
		internal_memcpy(Line(y1), Line(y2), _pitch);
		internal_memcpy(Line(y2), buffer, _pitch);
	}
	delete buffer;
}


//---------------------------------------------------------------------
// load file content
//---------------------------------------------------------------------
void BasicBitmap::BresenhamStretch(int dx, int dy, int dw, int dh, 
	const BasicBitmap *src, int sx, int sy, int sw, int sh, int mode)
{
	int dstwidth, dstheight, dstwidth2, dstheight2, srcwidth2, srcheight2;
	int werr, herr, incx, incy, i, j, nbytes; 
	IUINT32 mask;

	if (src->Bpp() != Bpp())
		return;

	if (dw == sw && dh == sh) {
		mode &= ~PIXEL_FLAG_NOCLIP;
		Blit(dx, dy, src, sx, sy, sw, sh, mode);
		return;
	}

	if ((mode & PIXEL_FLAG_NOCLIP) == 0) {
		BasicRect clipdst(0, 0, Width(), Height());
		BasicRect clipsrc(0, 0, src->Width(), src->Height());
		BasicRect bound_dst(dx, dy, dx + dw, dy + dh);
		BasicRect bound_src(sx, sy, sx + sw, sy + sh);
		if (ClipScale(&clipdst, &clipsrc, &bound_dst, &bound_src, mode) != 0) {
			return;
		}
		dx = bound_dst.left;
		dy = bound_dst.top;
		dw = bound_dst.right - bound_dst.left;
		dh = bound_dst.bottom - bound_dst.top;
		sx = bound_src.left;
		sy = bound_src.top;
		sw = bound_src.right - bound_src.left;
		sh = bound_src.bottom - bound_src.top;
	}	
	else {
		if (dx < 0 || dx + dw > (int)_w || dy < 0 || dy + dh > (int)_h ||
			sx < 0 || sx + sw > (int)src->_w || sy < 0 || sy + sh > (int)src->_h)
			return;
	}

	if (sh <= 0 || sw <= 0 || dh <= 0 || dw <= 0) 
		return;

	dstwidth = dw;
	dstheight = dh;
	dstwidth2 = dw * 2;
	dstheight2 = dh * 2;
	srcwidth2 = sw * 2;
	srcheight2 = sh * 2;

	if (mode & PIXEL_FLAG_VFLIP) sy = sy + sh - 1, incy = -1;
	else incy = 1;

	herr = srcheight2 - dstheight2;
	nbytes = _pixelsize;
	mask = src->GetMask();

	for (j = 0; j < dstheight; j++) {
		const unsigned char *srcrow = (const unsigned char*)src->Line(sy);
		unsigned char *dstrow = (unsigned char*)this->Line(dy);
		const unsigned char *srcpix = srcrow + nbytes * sx;
		unsigned char *dstpix = dstrow + nbytes * dx;
		incx = nbytes;
		if (mode & PIXEL_FLAG_HFLIP) {
			srcpix += (sw - 1) * nbytes;
			incx = -nbytes;
		}
		werr = srcwidth2 - dstwidth2;

		switch (nbytes)
		{
		case 1:
			{
				unsigned char mask8;
				if ((mode & PIXEL_FLAG_MASK) == 0) {
					for (i = dstwidth; i > 0; i--) {
						*dstpix++ = *srcpix;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}   else {
					mask8 = (unsigned char)(src->_mask & 0xff);
					for (i = dstwidth; i > 0; i--) {
						if (*srcpix != mask8) *dstpix = *srcpix;
						dstpix++;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}
			}
			break;

		case 2:
			{
				unsigned short mask16;
				if ((mode & PIXEL_FLAG_MASK) == 0) {
					for (i = dstwidth; i > 0; i--) {
						*((unsigned short*)dstpix) = 
											*((unsigned short*)srcpix);
						dstpix += 2;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}   else {
					mask16 = (unsigned short)(src->_mask & 0xffff);
					for (i = dstwidth; i > 0; i--) {
						if (*((unsigned short*)srcpix) != mask16) 
							*((unsigned short*)dstpix) = 
											*((unsigned short*)srcpix);
						dstpix += 2;
						while (werr >= 0) {
							srcpix += incx, werr -= dstwidth2;
						}
						werr += srcwidth2;
					}
				}
			}
			break;
		
		case 3:
			if ((mode & PIXEL_FLAG_MASK) == 0) {
				for (i = dstwidth; i > 0; i--) {
					dstpix[0] = srcpix[0];
					dstpix[1] = srcpix[1];
					dstpix[2] = srcpix[2];
					dstpix += 3;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			else {
				IUINT32 k;
				for (i = dstwidth; i > 0; i--) {
					k = _pixel_read_24(srcpix);
					if (k != mask) {
						dstpix[0] = srcpix[0];
						dstpix[1] = srcpix[1];
						dstpix[2] = srcpix[2];
					}
					dstpix += 3;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			break;

		case 4:
			if ((mode & PIXEL_FLAG_MASK) == 0) {
				for (i = dstwidth; i > 0; i--) {
					*((IUINT32*)dstpix) = *((IUINT32*)srcpix);
					dstpix += 4;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}   else {
				for (i = dstwidth; i > 0; i--) {
					if (*((IUINT32*)srcpix) != mask) 
						*((IUINT32*)dstpix) = *((IUINT32*)srcpix);
					dstpix += 4;
					while (werr >= 0) {
						srcpix += incx, werr -= dstwidth2;
					}
					werr += srcwidth2;
				}
			}
			break;
		}

		while (herr >= 0) {
			sy += incy, herr -= dstheight2;
		}

		herr += srcheight2; 
		dy++;
	}

	return;
}


//---------------------------------------------------------------------
// premultiply with alpha
//---------------------------------------------------------------------
void BasicBitmap::Premultiply(bool reverse)
{
	PixelFmt fmt = _fmt;
	for (int j = 0; j < _h; j++) {
		IUINT8 *bits = (IUINT8*)Line(j);
		if (fmt == X8R8G8B8) {
			IUINT32 *card = (IUINT32*)bits;
			for (int i = _w; i > 0; card++, i--) {
				card[0] |= 0xff000000;
			}
			continue;
		}
		if (reverse == false) {
			switch (fmt) {
			case A8R8G8B8:
				{
					IUINT32 *card = (IUINT32*)bits;
					for (int i = _w; i > 0; card++, i--) {
						IUINT32 r, g, b, a;
						_pixel_load_card(card, r, g, b, a);
						r = r * a;
						g = g * a;
						b = b * a;
						r = _pixel_fast_div_255(r);
						g = _pixel_fast_div_255(g);
						b = _pixel_fast_div_255(b);
						_pixel_save_card(card, r, g, b, a);
					}
				}
				break;
			case A1R5G5B5:
				{
					IUINT16 *ptr = (IUINT16*)bits;
					for (int i = _w; i > 0; ptr++, i--) {
						if ((ptr[0] & 0x8000) == 0) {
							ptr[0] = 0;
						}
					}
				}
				break;
			case A4R4G4B4:
				{
					IUINT16 *ptr = (IUINT16*)bits;
					for (int i = _w; i > 0; ptr++, i--) {
						IUINT32 r, g, b, a;
						_pixel_disasm_4444(ptr[0], a, r, g, b);
						r = r * a;
						g = g * a;
						b = b * a;
						r = _pixel_fast_div_255(r);
						g = _pixel_fast_div_255(g);
						b = _pixel_fast_div_255(b);
						ptr[0] = _pixel_asm_4444(a, r, g, b);
					}
				}
				break;
			default:
				break;
			}
		}
		else {
			switch (fmt) {
			case A8R8G8B8:
				{
					IUINT32 *card = (IUINT32*)bits;
					for (int i = _w; i > 0; card++, i--) {
						IUINT32 r, g, b, a;
						_pixel_load_card(card, r, g, b, a);
						if (a > 0) {
							r = r * 255 / a;
							g = g * 255 / a;
							b = b * 255 / a;
						}	else {
							r = g = b = 0;
						}
						_pixel_save_card(card, r, g, b, a);
					}
				}
			case A1R5G5B5:
				// nothing to do with A1R5G5B5
				break;
			case A4R4G4B4:
				{
					IUINT16 *ptr = (IUINT16*)bits;
					for (int i = _w; i > 0; ptr++, i--) {
						IUINT32 r, g, b, a;
						_pixel_disasm_4444(ptr[0], a, r, g, b);
						if (a > 0) {
							r = r * 255 / a;
							g = g * 255 / a;
							b = b * 255 / a;
						}	else {
							r = g = b = 0;
						}
						ptr[0] = _pixel_asm_4444(a, r, g, b);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}


//---------------------------------------------------------------------
// interpolate
//---------------------------------------------------------------------
BasicBitmap::InterpRow BasicBitmap::InterpolateRowPtr = NULL;
BasicBitmap::InterpCol BasicBitmap::InterpolateColPtr = NULL;

void BasicBitmap::SetDriver(InterpRow fn) 
{
	InterpolateRowPtr = fn;
}

void BasicBitmap::SetDriver(InterpCol fn)
{
	InterpolateColPtr = fn;
}


// interpolate row
int BasicBitmap::InterpolateRow(IUINT32 *card, int w, const IUINT32 *row1, 
	const IUINT32 *row2, IINT32 fraction)
{
	IINT32 f2 = (fraction & 0xff00) >> 8;
	IINT32 f1 = 256 - f2;

	if (f2 == 0 || row1 == row2) {
		internal_memcpy(card, row1, w * sizeof(IUINT32));
		return 0;
	}

	if (f1 == 0x80) {
		const unsigned char *s1 = (const unsigned char*)row1;
		const unsigned char *s2 = (const unsigned char*)row2;
		unsigned char *dd = (unsigned char*)card;
		ILINS_LOOP_DOUBLE(
			{
				dd[0] = (IUINT8)((((int)s1[0]) + ((int)s2[0])) >> 1);
				dd[1] = (IUINT8)((((int)s1[1]) + ((int)s2[1])) >> 1);
				dd[2] = (IUINT8)((((int)s1[2]) + ((int)s2[2])) >> 1);
				dd[3] = (IUINT8)((((int)s1[3]) + ((int)s2[3])) >> 1);
				s1 += 4;
				s2 += 4;
				dd += 4;
			},
			{
				dd[0] = (IUINT8)((((int)s1[0]) + ((int)s2[0])) >> 1);
				dd[1] = (IUINT8)((((int)s1[1]) + ((int)s2[1])) >> 1);
				dd[2] = (IUINT8)((((int)s1[2]) + ((int)s2[2])) >> 1);
				dd[3] = (IUINT8)((((int)s1[3]) + ((int)s2[3])) >> 1);
				dd[4] = (IUINT8)((((int)s1[4]) + ((int)s2[4])) >> 1);
				dd[5] = (IUINT8)((((int)s1[5]) + ((int)s2[5])) >> 1);
				dd[6] = (IUINT8)((((int)s1[6]) + ((int)s2[6])) >> 1);
				dd[7] = (IUINT8)((((int)s1[7]) + ((int)s2[7])) >> 1);
				s1 += 8;
				s2 += 8;
				dd += 8;
			},
			w);
		return 0;
	}

	IUINT32 c1, c2, c3, c4;

	ILINS_LOOP_DOUBLE(
		{
			c1 = row1[0] & 0xff00ff;
			c2 = row2[0] & 0xff00ff;
			c3 = (row1[0] >> 8) & 0xff00ff;
			c4 = (row2[0] >> 8) & 0xff00ff;
			c1 = (c1 * f1 + c2 * f2) >> 8;
			c3 = (c3 * f1 + c4 * f2) & 0xff00ff00;
			*card++ = c3 | (c1 & 0xff00ff);
			row1++;
			row2++;
		},
		{
			c1 = row1[0] & 0xff00ff;
			c2 = row2[0] & 0xff00ff;
			c3 = (row1[0] >> 8) & 0xff00ff;
			c4 = (row2[0] >> 8) & 0xff00ff;
			c1 = (c1 * f1 + c2 * f2) >> 8;
			c3 = (c3 * f1 + c4 * f2) & 0xff00ff00;
			card[0] = c3 | (c1 & 0xff00ff);

			c1 = row1[1] & 0xff00ff;
			c2 = row2[1] & 0xff00ff;
			c3 = (row1[1] >> 8) & 0xff00ff;
			c4 = (row2[1] >> 8) & 0xff00ff;
			c1 = (c1 * f1 + c2 * f2) >> 8;
			c3 = (c3 * f1 + c4 * f2) & 0xff00ff00;
			card[1] = c3 | (c1 & 0xff00ff);
			
			card += 2;
			row1 += 2;
			row2 += 2;
		},
		w);

	return 0;
}

int BasicBitmap::InterpolateCol(IUINT32 *card, int w, const IUINT32 *src, 
	IINT32 x, IINT32 dx)
{
	IUINT32 c1, c2, rb, ag;
	IINT32 xi, fx, fz;

	if (dx == 0x10000 && (x & 0xffff) == 0) {
		internal_memcpy(card, src + (x >> 16), w * sizeof(IUINT32));
		return 0;
	}

	ILINS_LOOP_DOUBLE(
		{
			xi = x >> 16;
			c1 = src[xi];
			c2 = src[xi + 1];
			fx = (x & 0xffff) >> 8;
			fz = 256 - fx;
			x += dx;
			rb = ((c1 & 0xff00ff) * fz + (c2 & 0xff00ff) * fx) >> 8;
			ag = ((c1 >> 8) & 0xff00ff) * fz + ((c2 >> 8) & 0xff00ff) * fx;
			*card++ = (rb & 0xff00ff) | (ag & 0xff00ff00);
		},
		{
			xi = x >> 16;
			c1 = src[xi];
			c2 = src[xi + 1];
			fx = (x & 0xffff) >> 8;
			fz = 256 - fx;
			x += dx;
			rb = ((c1 & 0xff00ff) * fz + (c2 & 0xff00ff) * fx) >> 8;
			ag = ((c1 >> 8) & 0xff00ff) * fz + ((c2 >> 8) & 0xff00ff) * fx;
			*card++ = (rb & 0xff00ff) | (ag & 0xff00ff00);

			xi = x >> 16;
			c1 = src[xi];
			c2 = src[xi + 1];
			fx = (x & 0xffff) >> 8;
			fz = 256 - fx;
			x += dx;
			rb = ((c1 & 0xff00ff) * fz + (c2 & 0xff00ff) * fx) >> 8;
			ag = ((c1 >> 8) & 0xff00ff) * fz + ((c2 >> 8) & 0xff00ff) * fx;
			*card++ = (rb & 0xff00ff) | (ag & 0xff00ff00);
		},
		w);
	return 0;
}

int BasicBitmap::InterpolateRowNearest(IUINT32 *card, int w, const IUINT32 *row1, 
	const IUINT32 *row2, IINT32 fraction)
{
	internal_memcpy(card, row1, sizeof(IUINT32) * w);
	return 0;
}

int BasicBitmap::InterpolateColNearest(IUINT32 *card, int w, const IUINT32 *src,
	IINT32 x, IINT32 dx)
{
	int i;
	if (dx == 0x10000) {
		internal_memcpy(card, src + (x >> 16), sizeof(IUINT32) * w);
		return 0;
	}
	for (i = w; i > 0; i--) {
		*card++ = src[x >> 16];
		x += dx;
	}
	return 0;
}


//---------------------------------------------------------------------
// Draw scale from src using different filter and blend op
// (mode & PIXEL_FLAG_HFLIP) != 0, for horizontal flip
// (mode & PIXEL_FLAG_VFLIP) != 0, for vertical flip
// (mode & PIXEL_FLAG_SRCOVER) == 0, for pure alpha blending
// (mode & PIXEL_FLAG_SRCOVER) != 0, for premultiplied src-over blending
// (mode & PIXEL_FLAG_ADDITIVE) != 0, for color plus
// (mode & PIXEL_FLAG_SRCCOPY) != 0, for color copy (no blending)
// (mode & PIXEL_FLAG_LINEAR) != 0, for linear interpolation
// (mode & PIXEL_FLAG_BILINEAR) != 0, for bi-linear interpolation
//---------------------------------------------------------------------
void BasicBitmap::Scale(int dx, int dy, int dw, int dh, const BasicBitmap *src,
	int sx, int sy, int sw, int sh, int mode, IUINT32 color)
{
	if ((_w | _h | src->_w | src->_h) >= 0x7fff) 
		return;

	if (dw == sw && dh == sh) {
		Blend(dx, dy, src, sx, sy, sw, sh, mode, color);
		return;
	}

	if ((mode & PIXEL_FLAG_NOCLIP) == 0) {
		BasicRect clipdst(0, 0, Width(), Height());
		BasicRect clipsrc(0, 0, src->Width(), src->Height());
		BasicRect bound_dst(dx, dy, dx + dw, dy + dh);
		BasicRect bound_src(sx, sy, sx + sw, sy + sh);
		if (ClipScale(&clipdst, &clipsrc, &bound_dst, &bound_src, mode) != 0) {
			return;
		}
		dx = bound_dst.left;
		dy = bound_dst.top;
		dw = bound_dst.right - bound_dst.left;
		dh = bound_dst.bottom - bound_dst.top;
		sx = bound_src.left;
		sy = bound_src.top;
		sw = bound_src.right - bound_src.left;
		sh = bound_src.bottom - bound_src.top;
	}	
	else {
		if (dx < 0 || dx + dw > (int)_w || dy < 0 || dy + dh > (int)_h ||
			sx < 0 || sx + sw > (int)src->_w || sy < 0 || sy + sh > (int)src->_h)
			return;
	}

	if (sh <= 0 || sw <= 0 || dh <= 0 || dw <= 0) 
		return;

	PixelFmt sfmt = src->_fmt;
	PixelFmt dfmt = this->_fmt;

	PixelDraw draw = GetDriver(dfmt, 0, false);
	if (mode & PIXEL_FLAG_SRCOVER) draw = GetDriver(dfmt, 1, false);
	else if (mode & PIXEL_FLAG_ADDITIVE) draw = GetDriver(dfmt, 2, false);
	else if (mode & PIXEL_FLAG_SRCCOPY) draw = NULL;

	IINT32 offx = (sw == dw)? 0 : 0x8000;
	IINT32 offy = (sh == dh)? 0 : 0x8000;
	IINT32 incx = (sw << 16) / dw;
	IINT32 incy = (sh << 16) / dh;
	IINT32 starty = (sy << 16) + offy;

	if (mode & PIXEL_FLAG_VFLIP) {
		starty = ((sy + sh - 1) << 16) - offy;
	}

	InterpRow interprow = InterpolateRowNearest;
	InterpCol interpcol = InterpolateColNearest;

	if (mode & PIXEL_FLAG_LINEAR) {
		interprow = (InterpolateRowPtr)? InterpolateRowPtr : InterpolateRow;
		interpcol = InterpolateColNearest;
	}

	if (mode & PIXEL_FLAG_BILINEAR) {
		interprow = (InterpolateRowPtr)? InterpolateRowPtr : InterpolateRow;
		interpcol = (InterpolateColPtr)? InterpolateColPtr : InterpolateCol;
	}

	int depth = src->Bpp();
	int need = (depth != 32)? (sw * 2) : 0;

	IUINT32 *buffer = (IUINT32*)internal_align_malloc((sw + 4 + dw + need) * 4, 16);
	IUINT32 *srcrow = buffer;
	IUINT32 *cache = buffer + sw + 4;
	IUINT32 *scanline1 = cache + dw;
	IUINT32 *scanline2 = scanline1 + sw;
	IUINT32 *srcline = NULL;

	for (int j = 0; j < dh; j++) {
		IUINT8 *dstrow = (IUINT8*)Line(dy + j);
		const IUINT32 *row1;
		const IUINT32 *row2;
		IINT32 fraction;

		if ((mode & PIXEL_FLAG_VFLIP) == 0) {
			int y1 = starty >> 16;
			int y2 = y1 + 1;
			int my = src->_h - 1;
			y1 = (y1 < 0)? 0 : ((y1 > my)? my : y1);
			y2 = (y2 < 0)? 0 : ((y2 > my)? my : y2);
			if (depth == 32) {
				row1 = ((const IUINT32*)src->Line(y1)) + sx;
				row2 = ((const IUINT32*)src->Line(y2)) + sx;
			}	else {
				row1 = scanline1;
				row2 = scanline2;
				src->RowFetch(sx, y1, scanline1, sw);
				src->RowFetch(sx, y2, scanline2, sw);
			}
			fraction = starty & 0xffff;
			starty += incy;
		}	else {
			int y1 = (starty + 0xffff) >> 16;
			int y2 = y1 - 1;
			int my = src->_h - 1;
			y1 = (y1 < 0)? 0 : ((y1 > my)? my : y1);
			y2 = (y2 < 0)? 0 : ((y2 > my)? my : y2);
			if (depth == 32) {
				row1 = ((const IUINT32*)src->Line(y1)) + sx;
				row2 = ((const IUINT32*)src->Line(y2)) + sx;
			}	else {
				row1 = scanline1;
				row2 = scanline2;
				src->RowFetch(sx, y1, scanline1, sw);
				src->RowFetch(sx, y2, scanline2, sw);
			}
			fraction = 0x10000 - (starty & 0xffff);
			starty -= incy;
		}

		// interpolate from row1 and row2 to srcrow
		interprow(srcrow, sw, row1, row2, fraction);

		if ((mode & PIXEL_FLAG_HFLIP) != 0) {
			CardReverse(srcrow, sw);
		}

		if (sfmt == X8R8G8B8) {
			CardSetAlpha(srcrow, sw, 0xff);
		}

		// repeat right edge for linear interpolation
		srcrow[sw] = srcrow[sw - 1];
		srcrow[sw + 1] = srcrow[sw - 1];

		if (color != 0xffffffff) {
			CardMultiply(srcrow, sw, color);
		}
		
		if (draw == NULL) {		// copy src directly
			if (dw == sw) {
				Store(dfmt, dstrow, dx, dw, srcrow);
			}	
			else if (_bpp == 32) {
				interpcol(((IUINT32*)dstrow) + dx, dw, srcrow, offx, incx);
			}
			else {
				interpcol(cache, dw, srcrow, offx, incx);
				Store(dfmt, dstrow, dx, dw, cache);
			}
		}
		else {					// draw with compositor
			if (dw == sw) {
				srcline = srcrow;
			}	else {
				interpcol(cache, dw, srcrow, offx, incx);
				srcline = cache;
			}
			draw(dstrow, dx, dw, srcline);
		}
	}

	internal_align_free(buffer);
}


// scale using rectangle
void BasicBitmap::Scale(const BasicRect *rect, const BasicBitmap *src, 
	const BasicRect *bound, int mode, IUINT32 color)
{
	const BasicRect *rs = bound;
	const BasicRect *rd = rect;
	BasicRect t1, t2;
	if (rd == NULL) {
		rd = &t1;
		t1.left = 0;
		t1.top = 0;
		t1.right = Width();
		t1.bottom = Height();
	}
	if (rs == NULL) {
		rs = &t2;
		t2.left = 0;
		t2.top = 0;
		t2.right = src->Width();
		t2.bottom = src->Height();
	}
	Scale(rd->left, rd->top, rd->right - rd->left, rd->bottom - rd->top,
		src, rs->left, rs->top, rs->right - rs->left, rs->bottom - rs->top, 
		mode, color);
}


//---------------------------------------------------------------------
// Shuffle 32 bits color
//---------------------------------------------------------------------
void BasicBitmap::Shuffle(int b0, int b1, int b2, int b3)
{
	if (_bpp != 32) return;
	for (int j = 0; j < _h; j++) {
		IUINT8 *src = (IUINT8*)Line(j);
		IUINT8 QUAD[4];
		for (int i = _w; i > 0; src += 4, i--) {
			*(IUINT32*)QUAD = *(IUINT32*)src;
			src[0] = QUAD[b0];
			src[1] = QUAD[b1];
			src[2] = QUAD[b2];
			src[3] = QUAD[b3];
		}
	}
}


//---------------------------------------------------------------------
// load file content
//---------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

void *BasicBitmap::LoadContent(const char *filename, long *size)
{
#ifndef PIXEL_NO_STDIO
	size_t length, remain;
	char *data, *lptr;
	FILE *fp;

	fp = fopen(filename, "rb");

	if (fp == NULL) {
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = (char*)malloc(length);

	if (data == NULL) {
		fclose(fp);
		return NULL;
	}

	for (lptr = data, remain = length; ; ) {
		size_t readed;
		if (feof(fp) || remain == 0) break;
		readed = fread(lptr, 1, remain, fp);
		remain -= readed;
		lptr += readed;
	}

	fclose(fp);

	if (size) size[0] = (long)length;

	return data;
#else
	if (size) size[0] = -1;
	return NULL;
#endif
}

#ifndef PIXEL_NO_STDIO
static void Pixel_FilePutByte(FILE *fp, IUINT8 c) {
	fputc(c, fp);
}

static void Pixel_FilePutWord(FILE *fp, IUINT16 w) {
	fputc(w & 0xff, fp);
	fputc((w >> 8) & 0xff, fp);
}

static void Pixel_FilePutLong(FILE *fp, IUINT32 x) {
	fputc(x & 0xff, fp);
	fputc((x >> 8) & 0xff, fp);
	fputc((x >> 16) & 0xff, fp);
	fputc((x >> 24) & 0xff, fp);
}
#endif


//---------------------------------------------------------------------
// Load bmp from memory
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::LoadBmpFromMemory(const void *buffer, long size, BasicColor *pal)
{
	struct MyBITMAPINFOHEADER{ // bmih  
		IUINT32	biSize; 
		IUINT32	biWidth; 
		IINT32	biHeight; 
		IUINT16 biPlanes; 
		IUINT16 biBitCount;
		IUINT32	biCompression; 
		IUINT32	biSizeImage; 
		IUINT32	biXPelsPerMeter; 
		IUINT32	biYPelsPerMeter; 
		IUINT32	biClrUsed; 
		IUINT32	biClrImportant; 
	}	InfoHeader; 

	char FileHeader[14];
	const char *ptr = (const char *)buffer;

	internal_memcpy(FileHeader, ptr, 14);
	ptr += 14;

	if (FileHeader[0] != 0x42 || FileHeader[1] != 0x4d) return NULL;

	ptr = basic_decode_32u(ptr, &InfoHeader.biSize);
	ptr = basic_decode_32u(ptr, &InfoHeader.biWidth);
	ptr = basic_decode_32u(ptr, (IUINT32*)&InfoHeader.biHeight);
	ptr = basic_decode_16u(ptr, &InfoHeader.biPlanes);
	ptr = basic_decode_16u(ptr, &InfoHeader.biBitCount);
	ptr = basic_decode_32u(ptr, &InfoHeader.biCompression);
	ptr = basic_decode_32u(ptr, &InfoHeader.biSizeImage);
	ptr = basic_decode_32u(ptr, &InfoHeader.biXPelsPerMeter);
	ptr = basic_decode_32u(ptr, &InfoHeader.biYPelsPerMeter);
	ptr = basic_decode_32u(ptr, &InfoHeader.biClrUsed);
	ptr = basic_decode_32u(ptr, &InfoHeader.biClrImportant);

	IUINT32 offset;
	basic_decode_32u(FileHeader + 10, &offset);

	if (pal != NULL && InfoHeader.biBitCount == 8) {
		for (IUINT32 i = 0; i < 256; i++) {
			if (ptr >= (const char*)buffer + offset) break;
			pal[i].b = *(const unsigned char*)ptr++;
			pal[i].g = *(const unsigned char*)ptr++;
			pal[i].r = *(const unsigned char*)ptr++;
			pal[i].a = 255;
			ptr++;
		}
	}

	ptr = (const char*)buffer + offset;

	int width = InfoHeader.biWidth;
	int height = InfoHeader.biHeight;
	int depth = InfoHeader.biBitCount;
	long pixelsize = ((depth + 7) / 8);
	long pitch = (pixelsize * width + 3) & (~3l);
	PixelFmt fmt = Bpp2Fmt(depth);

	BasicBitmap *bmp = new BasicBitmap(width, height, fmt);
	int y;

	if (bmp == NULL) return NULL;

	switch (depth) {
	case 8:
	case 15:
	case 16:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + pitch * y;
			IUINT8 *dst = bmp->Line(height - 1 - y);
			internal_memcpy(dst, src, pitch);
		}
		break;
	case 24:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + pitch * y;
			IUINT8 *dst = bmp->Line(height - 1 - y);
			internal_memcpy(dst, src, pitch);
		}
		break;
	case 32:
		for (y = 0; y < height; y++) {
			const IUINT8 *src = (const IUINT8*)ptr + pitch * y;
			IUINT8 *dst = bmp->Line(height - 1 - y);
			internal_memcpy(dst, src, pitch);
		}
		break;
	}

	return bmp;
}


//---------------------------------------------------------------------
// load bmp from file
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::LoadBmp(const char *filename, PixelFmt fmt, BasicColor *pal)
{
	void *content;
	long size;
	content = LoadContent(filename, &size);
	if (content == NULL) return NULL;
	BasicBitmap *bmp = LoadBmpFromMemory(content, size, pal);
	free(content);

	if (bmp == NULL) 
		return NULL;

	if (fmt == UNKNOW || fmt == bmp->Format()) 
		return bmp;

	BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), fmt);

	if (cvt) {
		cvt->Convert(0, 0, bmp, 0, 0, bmp->Width(), bmp->Height());
	}

	delete bmp;

	return cvt;
}

int BasicBitmap::SaveBmp(const char *filename, const BasicColor *pal) const
{
#ifndef PIXEL_NO_STDIO
	IUINT32 c, p;
	long i, j;
	BasicColor tmppal[256];
	int bfSize;
	int biSizeImage;
	int depth;
	int bpp;
	int filler;
	FILE *fp;

	if ((fp = fopen(filename, "wb")) == NULL) 
		return -1;

	depth = Bpp();
	bpp = (depth == 15 || depth == 16)? 24 : depth;
	filler = 3 - ((Width() * (bpp / 8) - 1) & 3);

	if (!pal) {
		for (i = 0; i < 256; i++) {
			tmppal[i].r = (unsigned char)i;
			tmppal[i].g = (unsigned char)i;
			tmppal[i].b = (unsigned char)i;
			tmppal[i].a = 255;
		}
		pal = tmppal;
	}

	if (bpp == 8) {
		biSizeImage = (Width() + filler) * Height();
		bfSize = (54 + 256 * 4 + biSizeImage);
	}	
	else if (bpp == 24) {
		long pitch = (Width() * 3 + 3) & (~3);
		biSizeImage = pitch * Height();
		bfSize = 54 + biSizeImage;
	}
	else if (bpp == 32) {
		long pitch = (Width() * 4 + 3) & (~3);
		biSizeImage = pitch * Height();
		bfSize = 54 + biSizeImage;
	}
	else {
		fclose(fp);
		return -2;
	}

	/* file_header */
	Pixel_FilePutWord(fp, 0x4D42);              /* bfType ("BM") */
	Pixel_FilePutLong(fp, bfSize);              /* bfSize */
	Pixel_FilePutWord(fp, 0);                   /* bfReserved1 */
	Pixel_FilePutWord(fp, 0);                   /* bfReserved2 */

	if (bpp == 8) {
		Pixel_FilePutLong(fp, 54 + 256 * 4); 
	}
	else {
		Pixel_FilePutLong(fp, 54); 
	}

	/* info_header */
	Pixel_FilePutLong(fp, 40);           /* biSize */
	Pixel_FilePutLong(fp, Width());      /* biWidth */
	Pixel_FilePutLong(fp, Height());     /* biHeight */
	Pixel_FilePutWord(fp, 1);            /* biPlanes */
	Pixel_FilePutWord(fp, bpp);          /* biBitCount */
	Pixel_FilePutLong(fp, 0);            /* biCompression */
	Pixel_FilePutLong(fp, biSizeImage);  /* biSizeImage */
	Pixel_FilePutLong(fp, 0xB12);        /* (0xB12 = 72 dpi) */
	Pixel_FilePutLong(fp, 0xB12);        /* biYPelsPerMeter */

	if (bpp == 8) {
		Pixel_FilePutLong(fp, 256);      /* biClrUsed */
		Pixel_FilePutLong(fp, 256);      /* biClrImportant */
		for (i = 0; i < 256; i++) {
			Pixel_FilePutByte(fp, pal[i].b);
			Pixel_FilePutByte(fp, pal[i].g);
			Pixel_FilePutByte(fp, pal[i].r);
			Pixel_FilePutByte(fp, 0);
		}
	}	else {
		Pixel_FilePutLong(fp, 0);        /* biClrUsed */
		Pixel_FilePutLong(fp, 0);        /* biClrImportant */
	}

	/* image data */
	for (j = (long)Height() - 1; j >= 0; j--) {
		const IUINT8 *source = Line(j);
		int width = Width();
		IUINT32 r, g, b, a;
		for (i = 0; i < (long)width; i++) {
			c = p = 0;
			switch (_bpp) {
			case 8:
				c = _pixel_fetch(8, source, i);
				p = c;
				break;
			case 15:
				c = _pixel_fetch(16, source, i);
				_pixel_disasm_1555(c, a, r, g, b);
				p = _pixel_asm_8888(a, r, g, b);
				break;
			case 16:
				c = _pixel_fetch(16, source, i);
				_pixel_disasm_565(c, r, g, b);
				p = _pixel_asm_8888(255, r, g, b);
				break;
			case 24:
				p = _pixel_fetch(24, source, i);
				break;
			case 32:
				p = _pixel_fetch(32, source, i);
				break;
			}
			if (bpp == 8) {
				Pixel_FilePutByte(fp, (IUINT8)c);
			}	else {
				Pixel_FilePutByte(fp, (int)((p >>  0) & 0xFF));
				Pixel_FilePutByte(fp, (int)((p >>  8) & 0xFF));
				Pixel_FilePutByte(fp, (int)((p >> 16) & 0xFF));
				if (bpp == 32) {
					Pixel_FilePutByte(fp, (int)((p >> 24) & 0xFF));
				}
			}
		}
		for (i = 0; i < (long)filler; i++) 
			Pixel_FilePutByte(fp, 0);
	}
	
	fclose(fp);
#endif
	return 0;
}


//---------------------------------------------------------------------
// Save to PPM File
//---------------------------------------------------------------------
int BasicBitmap::SavePPM(const char *filename) const
{
#ifndef PIXEL_NO_STDIO
	FILE *fp = fopen(filename, "wb");
	if (fp == NULL) return -1;
	fprintf(fp, "P6\n%d %d\n255\n", _w, _h);
	PixelFmt fmt = _fmt;
	const BasicBitmap *dst = this;
	BasicBitmap *tmp = NULL;

	if (fmt != A8R8G8B8 || fmt != X8R8G8B8) {
		tmp = new BasicBitmap(_w, _h, A8R8G8B8);
		if (tmp == NULL) {
			fclose(fp);
			return -1;
		}
		tmp->Convert(0, 0, this, 0, 0, _w, _h, 0);
		dst = tmp;
	}

	for (int j = 0; j < _h; j++) {
		const IUINT32 *src = dst->Address32(0, j);
		for (int i = _w; i > 0; src++, i--) {
			unsigned char color[3];
			color[0] = (src[0] >> 16) & 0xff;	// R
			color[1] = (src[0] >>  8) & 0xff;	// G
			color[2] = (src[0] >>  0) & 0xff;	// B
			fwrite(color, 3, 1, fp);
		}
	}

	if (tmp != NULL) {
		delete tmp;
	}
	fclose(fp);
#endif
	return 0;
}


//---------------------------------------------------------------------
// load tga from memory
//---------------------------------------------------------------------
static const char *PixelTgaReadSingle(const char *ptr, int bpp, IUINT32 *cc) 
{
	IUINT32 r, g, b, a, c;

	if (bpp == 8) {
		*cc = *(const unsigned char*)ptr;
		return ptr + 1;
	}
	if (bpp == 15 || bpp == 16) {
		IUINT16 w;
		ptr = basic_decode_16u(ptr, &w);
		*cc = w;
		return ptr;
	}
	if (bpp != 24 && bpp != 32) {
		*cc = 0;
		return ptr;
	}
	b = *(const unsigned char*)ptr++;
	g = *(const unsigned char*)ptr++;
	r = *(const unsigned char*)ptr++;
	if (bpp == 32) {
		a = *(const unsigned char*)ptr++;
	}	else {
		a = 0;
	}
	c = _pixel_asm_8888(a, r, g, b);
	if (bpp == 24) c &= 0xffffff;
	*cc = c;
	return ptr;
}


static const char *PixelTgaReadn(const char *ptr, void *buffer, int w, int bpp)
{
	unsigned char *lptr = (unsigned char*)buffer;
	int n = (bpp + 7) >> 3;
	for (; w > 0; w--, lptr += n) {
		IUINT32 c;
		ptr = PixelTgaReadSingle(ptr, bpp, &c);
		if (n == 1) _pixel_store_8(lptr, 0, c);
		else if (n == 2) _pixel_store_16(lptr, 0, c);
		else if (n == 3) _pixel_store_24(lptr, 0, c);
		else if (n == 4) _pixel_store_32(lptr, 0, c);
	}
	return ptr;
}


BasicBitmap *BasicBitmap::LoadTgaFromMemory(const void *buffer, long size, BasicColor *pal)
{
	unsigned char image_palette[256][4];
	unsigned char id_length, palette_type, image_type, palette_entry_size;
	unsigned char bpp, descriptor_bits;
	IUINT32 c = 0, i, y, yc, n, x;
	IUINT16 palette_colors;
	IUINT16 image_width, image_height;
	unsigned char *lptr;
	BasicBitmap *bmp;
	int compressed, count;
	BasicColor tmppal[256];

	if (pal == NULL) {
		pal = tmppal;
	}

	const char *ptr = (const char*)buffer;

	id_length = *(const unsigned char*)ptr++;
	palette_type = *(const unsigned char*)ptr++;
	image_type = *(const unsigned char*)ptr++;
	ptr += 2;
	ptr = basic_decode_16u(ptr, &palette_colors);
	palette_entry_size = *(const unsigned char*)ptr++;
	ptr += 4;
	ptr = basic_decode_16u(ptr, &image_width);
	ptr = basic_decode_16u(ptr, &image_height);
	bpp = *(const unsigned char*)ptr++;
	descriptor_bits = *(const unsigned char*)ptr++;

	ptr = ptr + id_length;

	if ((image_type < 1 || image_type > 3) && (image_type < 9 || image_type > 11))
		return NULL;

	if (palette_type == 1) {
		for (i = 0; i < (IUINT32)palette_colors; i++) {
			if (palette_entry_size == 16) {
				IUINT16 word;
				ptr = basic_decode_16u(ptr, &word);
				c = word;
				image_palette[i][0] = (IUINT8)_pixel_scale_5[(c >>  0) & 31];
				image_palette[i][1] = (IUINT8)_pixel_scale_5[(c >>  5) & 31];
				image_palette[i][2] = (IUINT8)_pixel_scale_5[(c >> 10) & 31];
			}
			else if (palette_entry_size >= 24) {
				image_palette[i][0] = *(const unsigned char*)ptr++;
				image_palette[i][1] = *(const unsigned char*)ptr++;
				image_palette[i][2] = *(const unsigned char*)ptr++;
				if (palette_entry_size == 32) {
					image_palette[i][3] = *(const unsigned char*)ptr++;
				}
			}
		}
	}
	else if (palette_type != 0) {
		return NULL;
	}

	/* 
	 * (image_type & 7): 1/colormapped, 2/truecolor, 3/grayscale
	 * (image_type & 8): 0/uncompressed, 8/compressed
	 */
	compressed = (image_type & 8);
	image_type = (image_type & 7);

	if ((image_type < 1) || (image_type > 3)) {
		return NULL;
	}

	if (image_type == 1) {				/* paletted image */
		if ((palette_type != 1) || (bpp != 8)) {
			return NULL;
		}
		for(i = 0; i < (IUINT32)palette_colors; i++) {
			pal[i].r = image_palette[i][2];
			pal[i].g = image_palette[i][1];
			pal[i].b = image_palette[i][0];
			pal[i].a = image_palette[i][3];
		}
	}
	else if (image_type == 2) {			/* truecolor image */
		if (palette_type != 0) {
			return NULL;
		}
		if (bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32) {
			return NULL;
		}
		if (bpp == 16) bpp = 15;
	}
	else if (image_type == 3) {			/* grayscale image */
		if ((palette_type != 0) || (bpp != 8)) {
			return NULL;
		}
		for (i = 0; i < 256; i++) {
			pal[i].r = (unsigned char)(i);
			pal[i].g = (unsigned char)(i);
			pal[i].b = (unsigned char)(i);
			pal[i].a = 255;
		}
	}

	n = (bpp + 7) >> 3;
	PixelFmt fmt = Bpp2Fmt(bpp);
	bmp = new BasicBitmap(image_width, image_height, fmt);

	if (!bmp) {
		return NULL;
	}

	for (y = 0; y < (IUINT32)bmp->Height(); y++) {
		memset(bmp->Line(y), 0, bmp->Pitch());
	}

	for (y = image_height; y; y--) {
		yc = (descriptor_bits & 0x20) ? image_height - y : y - 1;
		lptr = (unsigned char*)(bmp->Line(yc));
		if (compressed == 0) {
			ptr = PixelTgaReadn(ptr, lptr, image_width, bpp);
		}	else {
			for (x = 0; x < (IUINT32)image_width; x += count) {
				count = *(const unsigned char*)ptr++;
				if (count & 0x80) {
					count = (count & 0x7F) + 1;
					ptr = PixelTgaReadSingle(ptr, bpp, &c);
					for (i = 0; i < (IUINT32)count; i++, lptr += n) {
						if (n == 1) _pixel_store_8(lptr, 0, c);
						else if (n == 2) _pixel_store_16(lptr, 0, c);
						else if (n == 3) _pixel_store_24(lptr, 0, c);
						else if (n == 4) _pixel_store_32(lptr, 0, c);
					}
				}	else {
					count = count + 1;
					ptr = PixelTgaReadn(ptr, lptr, count, bpp);
					lptr = lptr + n * count;
				}
			}
		}
	}

	return bmp;
}



//---------------------------------------------------------------------
// load tga file
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::LoadTga(const char *filename, PixelFmt fmt, BasicColor *pal)
{
	void *content;
	long size;
	content = LoadContent(filename, &size);
	if (content == NULL) return NULL;
	BasicBitmap *bmp = LoadTgaFromMemory(content, size, pal);
	free(content);

	if (bmp == NULL)
		return NULL;

	if (fmt == UNKNOW || fmt == bmp->Format()) 
		return bmp;

	BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), fmt);

	if (cvt) {
		cvt->Convert(0, 0, bmp, 0, 0, bmp->Width(), bmp->Height());
	}

	delete bmp;

	return cvt;
}



//=====================================================================
// Win32 special routines
//=====================================================================
#if defined(_WIN32) && (!defined(PIXEL_NO_SYSTEM))

#define PIXEL_DIB_INFO_SIZE sizeof(BITMAPINFO) + (256 + 4) * sizeof(RGBQUAD)

// setup dib info from bitmap, buffsize of info must not exceed
// sizeof(BITMAPINFO) + (256 + 4) * sizeof(RGBQUAD)
void BasicBitmap::InitDIBInfo(void *ptr, int width, int height, PixelFmt fmt)
{
	BITMAPINFO *info = (BITMAPINFO*)ptr;
	int bitfield = 0;
	int palsize = 0;
	int bpp = Fmt2Bpp(fmt);
	int pixelsize = (bpp + 7) >> 3;
	int pitch = (width * pixelsize) & (~3);
	LPRGBQUAD palette = NULL;

	info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info->bmiHeader.biWidth = width;
	info->bmiHeader.biHeight = -height;
	info->bmiHeader.biPlanes = 1;
	info->bmiHeader.biBitCount = bpp;
	info->bmiHeader.biCompression = BI_RGB;
	info->bmiHeader.biSizeImage = pitch * height;
	info->bmiHeader.biXPelsPerMeter = 0;
	info->bmiHeader.biYPelsPerMeter = 0;
	info->bmiHeader.biClrUsed = 0;
	info->bmiHeader.biClrImportant = 0;

	if (bpp < 15) {
		palsize = (1 << bpp) * sizeof(PALETTEENTRY);
	}
	else if (bpp == 16 || bpp == 32 || bpp == 24) {
		bitfield = 3 * sizeof(RGBQUAD);
	}

	if (bpp == 24) {
		info->bmiHeader.biCompression = BI_RGB;
	}
	else if (bpp >= 15) {
		IUINT32 *data = (IUINT32*)info;
		switch (fmt) {
		case X1R5G5B5:
		case A1R5G5B5:
			data[10] = 0x7c00; data[11] = 0x3e0;
			data[12] = 0x1f; data[13] = 0;
			break;
		case R5G6B5:
			data[10] = 0xf800; data[11] = 0x7e0;
			data[12] = 0x1f; data[13] = 0;
			break;
		case A4R4G4B4:
			data[10] = 0xf00; data[11] = 0xf0;
			data[12] = 0xf; data[13] = 0xf000;
			break;
		case R8G8B8:
			data[10] = 0xff0000; data[11] = 0xff00; 
			data[12] = 0xff; data[13] = 0;
			break;
		case X8R8G8B8:
		case A8R8G8B8:
			data[10] = 0xff0000; data[11] = 0xff00; 
			data[12] = 0xff; data[13] = 0xff000000;
			break;
		case A8B8G8R8:
			data[10] = 0xff; data[11] = 0xff00; 
			data[12] = 0xff0000; data[13] = 0xff000000;
			break;
		case G8:
		case UNKNOW:
			break;
		}
		info->bmiHeader.biCompression = BI_BITFIELDS;
	}

	if (palsize > 0) {
		long offset = sizeof(BITMAPINFOHEADER) + bitfield;
		palette = (LPRGBQUAD)((char*)info + offset);
	}	else {
		palette = NULL;
	}

	if (palette) {
		int i;
		if (palette != NULL) {
			for (i = 0; i < 256; i++) {
				palette[i].rgbBlue = i;
				palette[i].rgbGreen = i;
				palette[i].rgbRed = i;
				palette[i].rgbReserved = 0;
			}
		}
	}
}


// create dib section
void *BasicBitmap::CreateDIB(void *hDC, int w, int h, PixelFmt fmt, void **bits)
{
	unsigned char buffer[4 * 4 + sizeof(BITMAPINFOHEADER) + 1024];
	BITMAPINFO *info = (BITMAPINFO*)buffer;
	HBITMAP hBitmap;
	LPVOID ptr;
	int mode = (Fmt2Bpp(fmt) < 15)? DIB_PAL_COLORS: DIB_RGB_COLORS;
	InitDIBInfo(info, w, h, fmt);
	hBitmap = ::CreateDIBSection((HDC)hDC, (LPBITMAPINFO)info, mode, &ptr, NULL, 0);
	if (bits) bits[0] = ptr;
	return hBitmap;
}


// draw to hdc
int BasicBitmap::SetDIBitsToDevice(void *hDC, int x, int y, int sx, int sy,
	int sw, int sh)
{
	char _buffer[PIXEL_DIB_INFO_SIZE];
	BITMAPINFO *info = (BITMAPINFO*)_buffer;
	int width = (int)this->_w;
	int height = (int)this->_h;
	RECT rect;
	int hr;

	InitDIBInfo(info, width, height, Format());

	rect.left = sx;
	rect.top = sy;
	rect.right = sx + sw;
	rect.bottom = sy + sh;

	if (rect.left < 0) x -= rect.left, rect.left = 0;
	if (rect.top < 0) y -= rect.top, rect.top = 0;
	if (rect.right > width) rect.right = width;
	if (rect.bottom > height) rect.bottom = height;

	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	if (width <= 0 || height <= 0) return 0;

	hr = ::SetDIBitsToDevice((HDC)hDC, x, y, width, height, 
						rect.left, rect.top, 
						0, (int)this->_h, this->_bits,
						(LPBITMAPINFO)info, DIB_RGB_COLORS);

	if (hr <= 0) return -1;

	return 0;
}

// get dib bits
int BasicBitmap::GetDIBits(void *hDC, void *hBitmap)
{
	char _buffer[PIXEL_DIB_INFO_SIZE];
	BITMAPINFO *info = (BITMAPINFO*)_buffer;

	InitDIBInfo(info, Width(), Height(), Format());

	int hr = ::GetDIBits((HDC)hDC, (HBITMAP)hBitmap,
		0, (int)this->_h, this->_bits, 
		(LPBITMAPINFO)info, DIB_RGB_COLORS);

	if (hr <= 0) return -1;

	return 0;
}

// GdiStartup
int BasicBitmap::GdiPlusInit(int startup)
{
	static HINSTANCE hDLL = NULL;
	static ULONG token = 0;
	static int inited = 0;
	int retval;

	if (hDLL == NULL) {
		hDLL = LoadLibraryA("gdiplus.dll");
		if (hDLL == NULL) return -1;
	}

	if (startup) {
		struct {
			unsigned int version;
			void *callback;
			BOOL SuppressBackgroundThread;
			BOOL SuppressExternalCodecs;
		}	GStartupInput;
		struct {
			void *hook;
			void *unhook;
		}	GStartupOutput;

		typedef int (WINAPI *GdiPlusStartup_t)(ULONG*, LPVOID, LPVOID);
		static GdiPlusStartup_t GdiPlusStartup_o = NULL;

		if (inited) return 0;

		if (GdiPlusStartup_o == NULL) {
			GdiPlusStartup_o = (GdiPlusStartup_t)GetProcAddress(
				hDLL, "GdiplusStartup");
			if (GdiPlusStartup_o == NULL) {
				return -2;
			}
		}

		GStartupInput.version = 1;
		GStartupInput.callback = NULL;
		GStartupInput.SuppressBackgroundThread = 0;
		GStartupInput.SuppressExternalCodecs = 0;

		retval = GdiPlusStartup_o(&token, &GStartupInput, &GStartupOutput);

		if (retval != 0) {
			return -3;
		}

		inited = 1;
	}
	else {
		typedef int (WINAPI *GdiPlusShutdown_t)(ULONG*);
		static GdiPlusShutdown_t GdiPlusShutdown_o = NULL;

		if (inited == 0) return 0;

		if (GdiPlusShutdown_o == NULL) {
			GdiPlusShutdown_o = (GdiPlusShutdown_t)GetProcAddress(
				hDLL, "GdiplusShutdown");
			if (GdiPlusShutdown_o == NULL) {
				return -4;
			}
		}

		GdiPlusShutdown_o(&token);

		inited = 0;
	}

	return 0;
}


// core routine for loading png/jpg from memory using gdi+
void *BasicBitmap::GdiPlusLoadCore(const void *data, long size, int *cx, int *cy, 
	long *pitch, int *pfmt, int *bpp, int *errcode)
{
	typedef HRESULT (WINAPI *CreateStream_t)(HGLOBAL, BOOL, LPSTREAM*);
	typedef int (WINAPI *GdipCreateBitmap_t)(LPSTREAM, LPVOID*); 
	typedef int (WINAPI *GdipGetPixelFormat_t)(LPVOID, int*);
	typedef int (WINAPI *GdipDisposeImage_t)(LPVOID);
	typedef int (WINAPI *GdipGetImageWidth_t)(LPVOID, UINT*);
	typedef int (WINAPI *GdipGetImageHeight_t)(LPVOID, UINT*);
	typedef int (WINAPI *GdipLockBits_t)(LPVOID, LPVOID, UINT, int, LPVOID);
	typedef int (WINAPI *GdipUnlockBits_t)(LPVOID, LPVOID);

	static CreateStream_t CreateStream_o = NULL;
	static GdipCreateBitmap_t GdipCreateBitmap_o = NULL;
	static GdipGetPixelFormat_t GdipGetPixelFormat_o = NULL;
	static GdipDisposeImage_t GdipDisposeImage_o = NULL;
	static GdipGetImageWidth_t GdipGetImageWidth_o = NULL;
	static GdipGetImageHeight_t GdipGetImageHeight_o = NULL;
	static GdipLockBits_t GdipLockBits_o = NULL;
	static GdipUnlockBits_t GdipUnlockBits_o = NULL;

	static HINSTANCE hGdiPlusDLL = NULL;
	LPSTREAM ppstm = NULL;
	HGLOBAL hg = NULL;
	LPVOID pg = NULL;
	LPVOID bitmap = NULL;
	void *bits = NULL;
	int retval = 0;
	int gpixfmt = 0;
	int fmt = 0;
	int nbytes = 0;
	UINT width;
	UINT height;
	long stride;
	int GRECT[4];
	int i;

	struct { 
		unsigned int width;
		unsigned int height;
		int pitch;
		int format;
		void *scan0;
		int *reserved;
	}	GBitmapData;

	GBitmapData.width = 0;
	GBitmapData.height = 0;
	GBitmapData.pitch = 0;
	GBitmapData.scan0 = NULL;

	if (CreateStream_o == NULL) {
		static HINSTANCE hOleDLL = NULL;
		if (hOleDLL == NULL) {
			hOleDLL = LoadLibraryA("ole32.dll");
			if (hOleDLL == NULL) {
				if (errcode) errcode[0] = -20;
				return NULL;
			}
		}
		CreateStream_o = (CreateStream_t)GetProcAddress(
			hOleDLL, "CreateStreamOnHGlobal");
		if (CreateStream_o == NULL) {
			if (errcode) errcode[0] = -21;
			return NULL;
		}
	}

	if (hGdiPlusDLL == NULL) {
		hGdiPlusDLL = LoadLibraryA("gdiplus.dll");
		if (hGdiPlusDLL == NULL) {
			if (errcode) errcode[0] = -22;
			return NULL;
		}
	}

	if (GdipCreateBitmap_o == NULL) {
		GdipCreateBitmap_o = (GdipCreateBitmap_t)GetProcAddress(
			hGdiPlusDLL, "GdipCreateBitmapFromStream");
		if (GdipCreateBitmap_o == NULL) {
			if (errcode) errcode[0] = -23;
			return NULL;
		}
	}

	if (GdipGetPixelFormat_o == NULL) {
		GdipGetPixelFormat_o = (GdipGetPixelFormat_t)GetProcAddress(
			hGdiPlusDLL, "GdipGetImagePixelFormat");
		if (GdipGetPixelFormat_o == NULL) {
			if (errcode) errcode[0] = -24;
			return NULL;
		}
	}

	if (GdipDisposeImage_o == NULL) {
		GdipDisposeImage_o = (GdipDisposeImage_t)GetProcAddress(
			hGdiPlusDLL, "GdipDisposeImage");
		if (GdipDisposeImage_o == NULL) {
			if (errcode) errcode[0] = -25;
			return NULL;
		}
	}

	if (GdipGetImageWidth_o == NULL) {
		GdipGetImageWidth_o = (GdipGetImageWidth_t)GetProcAddress(
			hGdiPlusDLL, "GdipGetImageWidth");
		if (GdipGetImageWidth_o == NULL) {
			if (errcode) errcode[0] = -26;
			return NULL;
		}
	}

	if (GdipGetImageHeight_o == NULL) {
		GdipGetImageHeight_o = (GdipGetImageHeight_t)GetProcAddress(
			hGdiPlusDLL, "GdipGetImageHeight");
		if (GdipGetImageHeight_o == NULL) {
			if (errcode) errcode[0] = -27;
			return NULL;
		}
	}

	if (GdipLockBits_o == NULL) {
		GdipLockBits_o = (GdipLockBits_t)GetProcAddress(
			hGdiPlusDLL, "GdipBitmapLockBits");
		if (GdipLockBits_o == NULL) {
			if (errcode) errcode[0] = -28;
			return NULL;
		}
	}

	if (GdipUnlockBits_o == NULL) {
		GdipUnlockBits_o = (GdipUnlockBits_t)GetProcAddress(
			hGdiPlusDLL, "GdipBitmapUnlockBits");
		if (GdipUnlockBits_o == NULL) {
			if (errcode) errcode[0] = -29;
			return NULL;
		}
	}

	if (errcode) errcode[0] = 0;

	hg = GlobalAlloc(GMEM_MOVEABLE, size);

	if (hg == NULL) {
		if (errcode) errcode[0] = -1;
		return NULL;
	}

	pg = GlobalLock(hg);

	if (pg == NULL) {
		GlobalFree(hg);
		if (errcode) errcode[0] = -2;
		return NULL;
	}

	internal_memcpy(pg, data, size);

	GlobalUnlock(hg);

	if (CreateStream_o(hg, 0, &ppstm) != S_OK) {
		GlobalFree(hg);
		if (errcode) errcode[0] = -3;
		return NULL;
	}

	retval = GdipCreateBitmap_o(ppstm, &bitmap);

	if (retval != 0) {
		retval = -4;
		bitmap = NULL;
		goto finalizing;
	}


	#define GPixelFormat1bppIndexed     196865
	#define GPixelFormat4bppIndexed     197634
	#define GPixelFormat8bppIndexed     198659
	#define GPixelFormat16bppGrayScale  1052676
	#define GPixelFormat16bppRGB555     135173
	#define GPixelFormat16bppRGB565     135174
	#define GPixelFormat16bppARGB1555   397319
	#define GPixelFormat24bppRGB        137224
	#define GPixelFormat32bppRGB        139273
	#define GPixelFormat32bppARGB       2498570
	#define GPixelFormat32bppPARGB      925707
	#define GPixelFormat48bppRGB        1060876
	#define GPixelFormat64bppARGB       3424269
	#define GPixelFormat64bppPARGB      29622286
	#define GPixelFormatMax             15
	
	if (GdipGetPixelFormat_o(bitmap, &gpixfmt) != 0) {
		retval = -5;
		goto finalizing;
	}
	
	if (gpixfmt == GPixelFormat8bppIndexed)
		gpixfmt = GPixelFormat8bppIndexed,
		fmt = 8, 
		nbytes = 1;
	else if (gpixfmt == GPixelFormat16bppRGB555)
		gpixfmt = GPixelFormat16bppRGB555,
		fmt = 555,
		nbytes = 2;
	else if (gpixfmt == GPixelFormat16bppRGB565)
		gpixfmt = GPixelFormat16bppRGB565,
		fmt = 565,
		nbytes = 2;
	else if (gpixfmt == GPixelFormat16bppARGB1555)
		gpixfmt = GPixelFormat16bppARGB1555,
		fmt = 1555,
		nbytes = 2;
	else if (gpixfmt == GPixelFormat24bppRGB)
		gpixfmt = GPixelFormat24bppRGB,
		fmt = 888,
		nbytes = 3;
	else if (gpixfmt == GPixelFormat32bppRGB)
		gpixfmt = GPixelFormat32bppRGB,
		fmt = 888,
		nbytes = 4;
	else if (gpixfmt == GPixelFormat32bppARGB)
		gpixfmt = GPixelFormat32bppARGB,
		fmt = 8888,
		nbytes = 4;
	else if (gpixfmt == GPixelFormat64bppARGB) 
		gpixfmt = GPixelFormat32bppARGB,
		fmt = 8888,
		nbytes = 4;
	else if (gpixfmt == GPixelFormat64bppPARGB)
		gpixfmt = GPixelFormat32bppARGB,
		fmt = 8888,
		nbytes = 4;
	else if (gpixfmt == GPixelFormat32bppPARGB)
		gpixfmt = GPixelFormat32bppARGB,
		fmt = 8888,
		nbytes = 4;
	else 
		gpixfmt = GPixelFormat32bppARGB,
		fmt = 8888,
		nbytes = 4;

	if (bpp) bpp[0] = nbytes * 8;
	if (pfmt) pfmt[0] = fmt;

	if (GdipGetImageWidth_o(bitmap, &width) != 0) {
		retval = -6;
		goto finalizing;
	}

	if (cx) cx[0] = (int)width;

	if (GdipGetImageHeight_o(bitmap, &height) != 0) {
		retval = -7;
		goto finalizing;
	}

	if (cy) cy[0] = (int)height;

	stride = (nbytes * width + 3) & ~3;
	if (pitch) pitch[0] = stride;

	GRECT[0] = 0;
	GRECT[1] = 0;
	GRECT[2] = (int)width;
	GRECT[3] = (int)height;

	if (GdipLockBits_o(bitmap, GRECT, 1, gpixfmt, &GBitmapData) != 0) {
		GBitmapData.scan0 = NULL;
		retval = -8;
		goto finalizing;
	}

	if (GBitmapData.format != gpixfmt) {
		retval = -9;
		goto finalizing;
	}

	bits = (char*)malloc(stride * height);

	if (bits == NULL) {
		retval = -10;
		goto finalizing;
	}

	for (i = 0; i < (int)height; i++) {
		char *src = (char*)GBitmapData.scan0 + GBitmapData.pitch * i;
		char *dst = (char*)bits + stride * i;
		internal_memcpy(dst, src, stride);
	}

	retval = 0;
	
finalizing:
	if (GBitmapData.scan0 != NULL) {
		GdipUnlockBits_o(bitmap, &GBitmapData);
		GBitmapData.scan0 = NULL;
	}

	if (bitmap) {
		GdipDisposeImage_o(bitmap);
		bitmap = NULL;
	}

	if (ppstm) {
		#ifndef __cplusplus
		ppstm->lpVtbl->Release(ppstm);
		#else
		ppstm->Release();
		#endif
		ppstm = NULL;
	}

	if (hg) {
		GlobalFree(hg);
		hg = NULL;
	}

	if (errcode) errcode[0] = retval;

	return bits;
}


// use GdiPlus to load JPEG/PNG file
BasicBitmap *BasicBitmap::GdiPlusLoadImageFromMemory(const void *data, long size, int *err)
{
	void *bits = NULL;
	int cx = -1;
	int cy = -1;
	long pitch = -1;
	int pfmt = -1;
	int bpp = -1;
	int errorcode = 0;
	PixelFmt fmt = UNKNOW;

	bits = GdiPlusLoadCore(data, size, &cx, &cy, &pitch, &pfmt, &bpp, &errorcode);

	if (bits == NULL) {
		if (err) err[0] = errorcode;
		return NULL;
	}

	switch (pfmt) {
	case 8: fmt = G8; break;
	case 555: fmt = X1R5G5B5; break;
	case 565: fmt = R5G6B5; break;
	case 888: fmt = R8G8B8; break;
	case 8888: fmt = A8R8G8B8; break;
	default:
		fmt = UNKNOW;
		break;
	}

	if (fmt == UNKNOW) {
		if (err) err[0] = -20;
		free(bits);
		return NULL;
	}

	BasicBitmap *bmp = new BasicBitmap(cx, cy, fmt);

	if (bmp == NULL) {
		if (err) err[0] = -21;
		free(bits);
		return NULL;
	}

	for (int j = 0; j < bmp->Height(); j++) {
		void *dst = bmp->Line(j);
		void *src = (char*)bits + pitch * j;
		internal_memcpy(dst, src, bmp->_pixelsize * cx);
	}

	free(bits);

	return bmp;
}


// Load JPG/PNG From file using gdi+
BasicBitmap *BasicBitmap::GdiPlusLoadImage(const char *filename, PixelFmt fmt)
{
	void *content;
	long size;
	content = LoadContent(filename, &size);
	if (content == NULL) return NULL;
	BasicBitmap *bmp = GdiPlusLoadImageFromMemory(content, size, NULL);
	free(content);

	if (bmp == NULL) 
		return NULL;

	if (fmt == UNKNOW || fmt == bmp->Format()) 
		return bmp;

	BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), fmt);

	if (cvt) {
		cvt->Convert(0, 0, bmp, 0, 0, bmp->Width(), bmp->Height());
	}

	delete bmp;

	return cvt;
}


// call AlphaBlend from msimg32.dll, BlendFunction must be passed as pointer
bool BasicBitmap::AlphaBlend(void *hdcDst, int nXOriginDest, int nYOriginDest,
	int nWidthDest, int nHeightDest, void *hdcSrc, int nXOriginSrc, 
	int nYOriginSrc, int nWidthSrc, int nHeightSrc, int SCA)
{
	typedef BOOL (WINAPI *AlphaBlend_t)(HDC, int, int, int, int, HDC,
		int, int, int, int, DWORD);
	static AlphaBlend_t AlphaBlend_o = NULL;
	static HINSTANCE hDLL = NULL;
	DWORD blend = 0x01000000;
	if (AlphaBlend_o == NULL) {
		hDLL = LoadLibraryA("msimg32.dll");
		if (hDLL == NULL) return false;
		AlphaBlend_o = (AlphaBlend_t)GetProcAddress(hDLL, "AlphaBlend");
		if (AlphaBlend_o == NULL) return false;
	}
	if (SCA < 0) SCA = 0;
	else if (SCA > 255) SCA = 255;
	blend = blend | (((DWORD)SCA) << 16);
	BOOL hr = AlphaBlend_o((HDC)hdcDst, nXOriginDest, nYOriginDest, nWidthDest,
		nHeightDest, (HDC)hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, 
		nHeightSrc, blend);
	return (hr)? true : false;
}


// call TransparentBlt from msimg32.dll
bool BasicBitmap::TransparentBlt(void *hdcDst, int nXOriginDest, int nYOriginDest,
	int nWidthDest, int nHeightDest, void *hdcSrc, int nXOriginSrc, 
	int nYOriginSrc, int nWidthSrc, int nHeightSrc, IUINT32 crTransparent)
{
	typedef BOOL (WINAPI *TransparentBlt_t)(HDC, int, int, int, int, HDC,
		int, int, int, int, UINT);
	static TransparentBlt_t TransparentBlt_o = NULL;
	static HINSTANCE hDLL = NULL;
	if (TransparentBlt_o == NULL) {
		hDLL = LoadLibraryA("msimg32.dll");
		if (hDLL == NULL) return false;
		TransparentBlt_o = (TransparentBlt_t)
			GetProcAddress(hDLL, "TransparentBlt");
		if (TransparentBlt_o == NULL) return false;
	}
	BOOL hr = TransparentBlt_o((HDC)hdcDst, nXOriginDest, nYOriginDest, nWidthDest,
		nHeightDest, (HDC)hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, 
		nHeightSrc, (UINT)crTransparent);
	return (hr)? true : false;
}


// CreateDIBSection and then create BasicBitmap in the DIB (borror pixel memory)
// DeleteObject((HBITMAP)hbitmap) must be invoked to dispose DIB after bitmap destruction
BasicBitmap *BasicBitmap::CreateBitmapInDIB(void *hDC, int w, int h, PixelFmt fmt, void **hbitmap)
{
	BasicBitmap *bmp = NULL;
	HBITMAP hBitmap = NULL;
	void *bits = NULL;
	if (hbitmap) hbitmap[0] = NULL;
	hBitmap = (HBITMAP)CreateDIB(hDC, w, h, fmt, &bits);
	if (hBitmap == NULL) {
		return NULL;
	}
	if (bits == NULL) {
		DeleteObject(hBitmap);
		return NULL;
	}
	int pixelsize = (Fmt2Bpp(fmt) + 7) / 8;
	int pitch = (pixelsize * w + 3) & (~3);
	bmp = new BasicBitmap(w, h, fmt, bits, pitch);
	if (bmp == NULL) {
		DeleteObject(hBitmap);
		return NULL;
	}
	if (hbitmap) hbitmap[0] = (void*)hBitmap;
	return bmp;
}


#endif


//---------------------------------------------------------------------
// detect file type and load
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::LoadImageFromMemory(const void *data, long size, BasicColor *pal)
{
	const unsigned char *ptr = (const unsigned char*)data;

	if (ptr[0] == 0x42 && ptr[1] == 0x4d) {
		return LoadBmpFromMemory(data, size, pal);
	}

#if (defined(_WIN32) || defined(WIN32)) && (!defined(PIXEL_NO_SYSTEM))
	if (ptr[0] == 0x89 && ptr[1] == 0x50 && ptr[2] == 0x4E && ptr[3] == 0x47) {
		return GdiPlusLoadImageFromMemory(data, size, NULL);	// PNG
	}

	if (ptr[0] == 0xff && ptr[1] == 0xd8) {
		return GdiPlusLoadImageFromMemory(data, size, NULL);	// JPEG
	}

#endif

	if ((ptr[2] >= 1 && ptr[2] <= 3) || (ptr[2] >= 9 && ptr[2] <= 11)) {
		return LoadTgaFromMemory(data, size, pal);
	}

	return NULL;
}


//---------------------------------------------------------------------
// detect file type and load 
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::LoadFile(const char *filename, PixelFmt fmt, BasicColor *pal)
{
	void *content;
	long size;
	content = LoadContent(filename, &size);
	if (content == NULL) {
		return NULL;
	}

	BasicBitmap *bmp = LoadImageFromMemory(content, size, pal);
	free(content);

	if (bmp == NULL) 
		return NULL;

	if (fmt == UNKNOW || fmt == bmp->Format()) 
		return bmp;

	BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), fmt);

	if (cvt) {
		cvt->Convert(0, 0, bmp, 0, 0, bmp->Width(), bmp->Height());
	}

	delete bmp;

	return cvt;
}


//---------------------------------------------------------------------
// quick-and-dirty font drawing for show frame rate, debug info, etc.
//---------------------------------------------------------------------
void BasicBitmap::QuickText(int x, int y, const char *text, IUINT32 color)
{
	static IUINT32 font[] = {
	0,0,813201456,3145776,7105644,0,1828613228,7105790,2025880624,3209228,
	416073216,13002288,1983409208,7785692,12607584,0,1616916504,1585248,404238432,
	6303768,4282148352ul,26172,4231016448ul,12336,0,1613770752,4227858432ul,0,0,
	3158016,806882310,8437856,3738093180ul,8185590,808480816,16527408,940362872,
	16567392,940362872,7916556,3429645340ul,1969406,217628924,7916556,
	4173357112ul,7916748,403492092,3158064,2026687608,7916748,2093796472,7346188,
	3158016,3158016,3158016,1613770752,3227529240ul,1585248,16515072,64512,
	202911840,6303768,403491960,3145776,3739141756ul,7913694,3435952176ul,
	13421820,2087085820,16541286,3233834556ul,3958464,1717988600,16280678,
	2020107006,16671336,2020107006,15753320,3233834556ul,4089550,4241280204ul,
	13421772,808464504,7876656,202116126,7916748,2020370150,15099500,1616929008,
	16672354,4278120134ul,13027030,3740722886ul,13027022,3334892600ul,3697862,
	2087085820,15753312,3435973752ul,1865948,2087085820,15099500,1893780600,
	7916572,808498428,7876656,3435973836ul,16567500,3435973836ul,3176652,
	3603351238ul,13037310,946652870,13003832,2026687692,7876656,411879166,
	16672306,1616928888,7888992,405823680,132620,404232312,7870488,3328981008ul,0,
	0,4278190080ul };

	const unsigned char *ascii = (const unsigned char*)font;
	int savex = x;
	for (; text[0]; text++) {
		int c = (int)text[0];
		if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
		if (c == '{') c = '[';
		if (c == '}') c = ']';
		if (c == '\r') x = savex;
		else if (c == '\n') x = savex, y = y + 8;
		else if (c < 32 || c > 127) { x += 8; return; }
		else {
		#if IWORDS_BIG_ENDIAN
			const unsigned char *p = ascii + 8 * (c - 32);
			unsigned char t[8];
			t[0] = p[3], t[1] = p[2], t[2] = p[3], t[3] = p[0];
			t[4] = p[7], t[5] = p[6], t[6] = p[5], t[7] = p[4];
			DrawGlyph(x, y, t, 8, 8, color);
		#else
			DrawGlyph(x, y, ascii + 8 * ((int)c - 32), 8, 8, color);
		#endif
			x += 8;
		}
	}
}


//---------------------------------------------------------------------
// DownSample 2x2 pixels into 1 pixel, only support 32/8 bits
//---------------------------------------------------------------------
int BasicBitmap::DownSampleBy2(int x, int y, const BasicBitmap *src, int sx, int sy, int sw, int sh)
{
	if (sx < 0 || sy < 0 || sx + sw > src->Width() || sy + sh > src->Height())
		return -1;
	if (x < 0 || y < 0 || x + sw / 2 > Width() || y + sh / 2 > Height())
		return -2;
	if (Bpp() != src->Bpp())
		return -3;

	int w = sw / 2;
	int h = sh / 2;

	if (Bpp() == 32) {
		for (int j = 0; j < h; j++) {
			const IUINT32 *row1 = src->Address32(sx, sy + j * 2 + 0);
			const IUINT32 *row2 = src->Address32(sx, sy + j * 2 + 1);
			IUINT32 *dst = Address32(x, y + j);
			for (int i = w; i > 0; row1 += 2, row2 += 2, dst++, i--) {
				IUINT32 rb = (row1[0] & 0xff00ff) + (row1[1] & 0xff00ff) +
					(row2[0] & 0xff00ff) + (row2[1] & 0xff00ff);
				IUINT32 ag = ((row1[0] >> 8) & 0xff00ff) + ((row1[1] >> 8) & 0xff00ff) +
					((row2[0] >> 8) & 0xff00ff) + ((row2[1] >> 8) & 0xff00ff);
				rb = (rb >> 2) & 0xff00ff;
				ag = (ag >> 2) & 0xff00ff;
				dst[0] = rb | (ag << 8);
			}
		}
		return 0;
	}
	if (Bpp() == 8) {
		for (int j = 0; j < h; j++) {
			const IUINT8 *row1 = src->Address8(sx, sy + j * 2 + 0);
			const IUINT8 *row2 = src->Address8(sx, sy + j * 2 + 1);
			IUINT8 *dst = Address8(x, y + j);
			for (int i = w; i > 0; row1 += 2, row2 += 2, dst++, i--) {
				int cc = (int)row1[0] + (int)row1[1] + (int)row2[0] + (int)row2[1];
				dst[0] = (cc >> 2) & 0xff;
			}
		}
		return 0;
	}
	if (Bpp() == 24) {
		for (int j = 0; j < h; j++) {
			const IUINT8 *row1 = src->Address8(sx, sy + j * 2 + 0);
			const IUINT8 *row2 = src->Address8(sx, sy + j * 2 + 1);
			IUINT8 *dst = Address8(x, y + j);
			for (int i = w; i > 0; row1 += 6, row2 += 6, dst += 3, i--) {
				IUINT32 c1 = _pixel_fetch_24(row1, 0);
				IUINT32 c2 = _pixel_fetch_24(row1, 1);
				IUINT32 c3 = _pixel_fetch_24(row2, 0);
				IUINT32 c4 = _pixel_fetch_24(row2, 1);
				IUINT32 rb = (c1 & 0xff00ff) + (c2 & 0xff00ff) + (c3 & 0xff00ff) + 
					(c4 & 0xff00ff);
				IUINT32 ag = ((c1 >> 8) & 0xff) + ((c2 >> 8) & 0xff) +
					((c3 >> 8) & 0xff) + ((c4 >> 8) & 0xff);
				rb = (rb >> 2) & 0xff00ff;
				ag = (ag >> 2) & 0x0000ff;
				c1 = rb | (ag << 8);
				_pixel_write_24(dst, c1);
			}
		}
		return 0;
	}
	if (Bpp() == 16) {
		for (int j = 0; j < h; j++) {
			const IUINT16 *row1 = src->Address16(sx, sy + j * 2 + 0);
			const IUINT16 *row2 = src->Address16(sx, sy + j * 2 + 1);
			IUINT16 *dst = Address16(x, y + j);
			IUINT32 c1, c2, c3, c4, r, g, b, a;
			int i;
			switch (_fmt) {
			case R5G6B5:
				for (i = w; i > 0; row1 += 2, row2 += 2, dst++, i--) {
					RGBA_FROM_R5G6B5(((IUINT32)row1[0]), c1, c2, c3, c4);
					RGBA_FROM_R5G6B5(((IUINT32)row1[1]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					RGBA_FROM_R5G6B5(((IUINT32)row2[0]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					RGBA_FROM_R5G6B5(((IUINT32)row2[1]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					c1 >>= 2, c2 >>= 2, c3 >>= 2, c4 >>= 2;
					dst[0] = (IUINT16)RGBA_TO_R5G6B5(c1, c2, c3, c4);
				}
				break;
			case A1R5G5B5:
			case X1R5G5B5:
				for (i = w; i > 0; row1 += 2, row2 += 2, dst++, i--) {
					RGBA_FROM_A1R5G5B5(((IUINT32)row1[0]), c1, c2, c3, c4);
					RGBA_FROM_A1R5G5B5(((IUINT32)row1[1]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					RGBA_FROM_A1R5G5B5(((IUINT32)row2[0]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					RGBA_FROM_A1R5G5B5(((IUINT32)row2[1]), r, g, b, a);
					c1 += r, c2 += g, c3 += b, c4 += a;
					c1 >>= 2, c2 >>= 2, c3 >>= 2, c4 >>= 2;
					dst[0] = (IUINT16)RGBA_TO_A1R5G5B5(c1, c2, c3, c4);
				}
				break;
			case A4R4G4B4:
				for (i = w; i > 0; row1 += 2, row2 += 2, dst++, i--) {
					c1 = (row1[0] & 0x0f0f) + (row1[1] & 0x0f0f) + 
						(row2[0] & 0x0f0f) + (row2[1] & 0x0f0f);
					c2 = ((row1[0] >> 4) & 0x0f0f) + ((row1[1] >> 4) & 0x0f0f) +
						((row2[0] >> 4) & 0x0f0f) + ((row2[1] >> 4) & 0x0f0f);
					c3 = (c1 >> 2) & 0x0f0f;
					c4 = (c2 >> 2) & 0x0f0f;
					dst[0] = (IUINT16)((c4 << 4) | c3);
				}
				break;
			default:
				break;
			}
		}
		return 0;
	}

	return 0;
}


//---------------------------------------------------------------------
// GetBlock only works with 8 bpp
//---------------------------------------------------------------------
int BasicBitmap::GetBlock(int x, int y, int *block, int w, int h) const
{
	if (_bpp != 8) return -1;
	if (x >= 0 && y >= 0 && x + w <= Width() && y + h <= Height()) {
		for (int j = 0; j < h; j++) {
			const IUINT8 *src = Address8(x, y + j);
			for (int i = w; i > 0; src++, i--) {
				*block++ = src[0];
			}
		}
	}
	else {
		for (int j = 0; j < h; j++) {
			if (((unsigned int)(y + j)) < (unsigned int)_h) {
				const IUINT8 *src = Address8(0, y + j);
				int k = x + w;
				for (int i = x; i < k; i++) {
					if (((unsigned int)i) < (unsigned int)_w) {
						*block++ = src[i];
					}	else {
						*block++ = 0;
					}
				}
			}	else {
				for (int i = 0; i < w; i++) *block++ = 0;
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// SetBlock only works with 8 bpp
//---------------------------------------------------------------------
int BasicBitmap::SetBlock(int x, int y, const int *block, int w, int h)
{
	if (_bpp != 8) return -1;

	if (x >= 0 && y >= 0 && x + w <= Width() && y + h <= Height()) {
		for (int j = 0; j < h; j++) {
			IUINT8 *src = Address8(x, y + j);
			for (int i = w; i > 0; src++, i--) {
				src[0] = (unsigned char)block[0];
				block++;
			}
		}			
	}
	else {
		for (int j = 0; j < h; j++) {
			if (((unsigned int)(y + j)) < (unsigned int)_h) {
				IUINT8 *src = Address8(0, y + j);
				int k = x + w;
				for (int i = x; i < k; i++) {
					if (((unsigned int)i) < (unsigned int)_w) {
						src[i] = (unsigned char)block[0];
					}
					block++;
				}
			}	else {
				block += w; 
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// SetAlphaForAllPixel
//---------------------------------------------------------------------
void BasicBitmap::SetAlphaForAllPixel(int alpha)
{
	if (_bpp != 32) return;
	for (int i = 0; i < _h; i++) {
		CardSetAlpha(Address32(0, i), _w, alpha);
	}
}


//---------------------------------------------------------------------
// BilinearSampler
//---------------------------------------------------------------------
static inline IUINT32 _pixel_biline_interp (IUINT32 tl, IUINT32 tr,
	IUINT32 bl, IUINT32 br, IINT32 distx, IINT32 disty)
{
    IINT32 distxy, distxiy, distixy, distixiy;
    IUINT32 f, r;

    distxy = distx * disty;
    distxiy = (distx << 8) - distxy;	/* distx * (256 - disty) */
    distixy = (disty << 8) - distxy;	/* disty * (256 - distx) */
    distixiy =
	256 * 256 - (disty << 8) -
	(distx << 8) + distxy;		/* (256 - distx) * (256 - disty) */

    /* Blue */
    r = (tl & 0x000000ff) * distixiy + (tr & 0x000000ff) * distxiy
      + (bl & 0x000000ff) * distixy  + (br & 0x000000ff) * distxy;

    /* Green */
    f = (tl & 0x0000ff00) * distixiy + (tr & 0x0000ff00) * distxiy
      + (bl & 0x0000ff00) * distixy  + (br & 0x0000ff00) * distxy;
    r |= f & 0xff000000;

    tl >>= 16;
    tr >>= 16;
    bl >>= 16;
    br >>= 16;
    r >>= 16;

    /* Red */
    f = (tl & 0x000000ff) * distixiy + (tr & 0x000000ff) * distxiy
      + (bl & 0x000000ff) * distixy  + (br & 0x000000ff) * distxy;
    r |= f & 0x00ff0000;

    /* Alpha */
    f = (tl & 0x0000ff00) * distixiy + (tr & 0x0000ff00) * distxiy
      + (bl & 0x0000ff00) * distixy  + (br & 0x0000ff00) * distxy;
    r |= f & 0xff000000;

    return r;
}

// calculate bilinear pixel
IUINT32 BasicBitmap::SampleBilinear(float x, float y, bool repeat) const
{
	IUINT32 c00, c01, c10, c11;
	IINT32 fx = (IINT32)(x * 0x10000);
	IINT32 fy = (IINT32)(y * 0x10000);
	int x1 = fx >> 16;
	int y1 = fy >> 16;
	int x2 = x1 + 1;
	int y2 = y1 + 1;
	int dx = (fx >> 8) & 0xff;
	int dy = (fy >> 8) & 0xff;
	if (repeat) {
		if (_w <= 0 || _h <= 0) return 0;
		if (x1 < 0) x1 = 0;
		if (y1 < 0) y1 = 0;
		if (x2 < 0) x2 = 0;
		if (y2 < 0) y2 = 0;
		if (x1 >= _w) x1 = _w - 1;
		if (y1 >= _h) y1 = _h - 1;
		if (x2 >= _w) x2 = _w - 1;
		if (y2 >= _h) y2 = _h - 1;
	}
	c00 = GetColor(x1, y1);
	c01 = GetColor(x2, y1);
	c10 = GetColor(x1, y2);
	c11 = GetColor(x2, y2);
	return _pixel_biline_interp(c00, c01, c10, c11, dx, dy);
}


//---------------------------------------------------------------------
// BicubicSampler
//---------------------------------------------------------------------
static inline int _pixel_middle(int x, int xmin, int xmax) {
	if (x < xmin) return xmin;
	if (x > xmax) return xmax;
	return x;
}

inline int _pixel_abs(int x) {
	return (x < 0)? (-x) : x;
}

static inline float _pixel_bicubic(float x) {
	if (x == 0.0f) return 1.0f;
	if (x < 0.0f) x = -x;
	float a = -0.5f;
	float x2 = x * x;
	float x3 = x2 * x;
	if (x <= 1.0f) return x3 * (a + 2.0f) - x2 * (a + 3.0f) + 1.0f;
	if (x <= 2.0f) return x3 * a - x2 * a * 5.0f + 8.0f * a * x - 4.0f * a;
	return 0.0f;
}

IUINT32 BasicBitmap::SampleBicubic(float x, float y, bool repeat) const
{
	float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	IINT32 fx = (IINT32)(x * 0x10000);
	IINT32 fy = (IINT32)(y * 0x10000);
	int xi = (fx >> 16);
	int yi = (fy >> 16);
	float x0 = (float)(xi - 1);
	float y0 = (float)(yi - 1);
	int sx[4];
	int sy[4];
	float wx[4];
	float wy[4];
	sx[0] = xi - 1;
	sx[1] = xi;
	sx[2] = xi + 1;
	sx[3] = xi + 2;
	sy[0] = yi - 1;
	sy[1] = yi;
	sy[2] = yi + 1;
	sy[3] = yi + 2;
	if (repeat) {
		sx[0] = _pixel_middle(sx[0], 0, _w - 1);
		sx[1] = _pixel_middle(sx[1], 0, _w - 1);
		sx[2] = _pixel_middle(sx[2], 0, _w - 1);
		sx[3] = _pixel_middle(sx[3], 0, _w - 1);
		sy[0] = _pixel_middle(sy[0], 0, _h - 1);
		sy[1] = _pixel_middle(sy[1], 0, _h - 1);
		sy[2] = _pixel_middle(sy[2], 0, _h - 1);
		sy[3] = _pixel_middle(sy[3], 0, _h - 1);
	}
	wx[0] = _pixel_bicubic(x - (x0 + 0.0f));
	wx[1] = _pixel_bicubic(x - (x0 + 1.0f));
	wx[2] = _pixel_bicubic(x - (x0 + 2.0f));
	wx[3] = _pixel_bicubic(x - (x0 + 3.0f));
	wy[0] = _pixel_bicubic(y - (y0 + 0.0f));
	wy[1] = _pixel_bicubic(y - (y0 + 1.0f));
	wy[2] = _pixel_bicubic(y - (y0 + 2.0f));
	wy[3] = _pixel_bicubic(y - (y0 + 3.0f));
	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			float weight = wx[i] * wy[j];
			IUINT32 color = GetColor(sx[i], sy[j]);
			r = r + weight * ((color & 0xff0000) >> 16);
			g = g + weight * ((color & 0xff00) >> 8);
			b = b + weight * ((color & 0xff) >> 0);
			a = a + weight * ((color & 0xff000000) >> 24);
		}
	}
	IUINT32 r1 = (int)r;
	IUINT32 g1 = (int)g;
	IUINT32 b1 = (int)b;
	IUINT32 a1 = (int)a;
	r1 = _pixel_middle(r1, 0, 255);
	g1 = _pixel_middle(g1, 0, 255);
	b1 = _pixel_middle(b1, 0, 255);
	a1 = _pixel_middle(a1, 0, 255);
#if 0
	IUINT32 r2, g2, b2, a2;
	IUINT32 ci = GetColor(xi, yi);
	RGBA_FROM_A8R8G8B8(ci, r2, g2, b2, a2);
	if (_pixel_abs(r1 - r2) > 198 || _pixel_abs(g1 - g2) > 198 ||
		_pixel_abs(b1 - b2) > 198 || _pixel_abs(a1 - a2) > 198)
		return ci;
#endif
	return RGBA_TO_A8R8G8B8(r1, g1, b1, a1);
}


//---------------------------------------------------------------------
// return an new resampled bitmap
//---------------------------------------------------------------------
BasicBitmap *BasicBitmap::Resample(int NewWidth, int NewHeight, ResampleFilter filter) const
{
	if (NewWidth <= 0 || NewHeight <= 0) return NULL;
	BasicBitmap *bmp = new BasicBitmap(NewWidth, NewHeight, _fmt);
	if (bmp == NULL) return NULL;
	bmp->Resample(0, 0, NewWidth, NewHeight, this, 0, 0, _w, _h, filter);
	return bmp;
}



//---------------------------------------------------------------------
// Smooth Resample
//---------------------------------------------------------------------

// shrink x type
typedef int (*BasicBitmap_ResampleShrinkX)(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth, void *workmem);

// shrink y type
typedef int (*BasicBitmap_ResampleShrinkY)(IUINT8 *dstpix, const IUINT8 *srcpix,
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight, void *workmem);

// expand x type
typedef int (*BasicBitmap_ResampleExpandX)(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth, void *workmem);

// expand y type
typedef int (*BasicBitmap_ResampleExpandY)(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight, void *workmem);


// c implementation
static int BasicBitmap_ResampleShrinkX_C(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth, void *workmem)
{
	IINT32 srcdiff = srcpitch - (srcwidth * 4);
	IINT32 dstdiff = dstpitch - (dstwidth * 4);
	IINT32 x, y;
	IINT32 xspace = 0x10000 * srcwidth / dstwidth;
	IINT32 xrecip = 0;
	IINT64 zrecip = 1;

	zrecip <<= 32;
	xrecip = (int)(zrecip / xspace);

	for (y = 0; y < height; y++) {
		IUINT32 accumulate[4] = { 0, 0, 0, 0 };
		int xcounter = xspace;
		for (x = 0; x < srcwidth; x++) {
			if (xcounter > 0x10000) {
				accumulate[0] += (IUINT32) *srcpix++;
				accumulate[1] += (IUINT32) *srcpix++;
				accumulate[2] += (IUINT32) *srcpix++;
				accumulate[3] += (IUINT32) *srcpix++;
				xcounter -= 0x10000;
			}	else {
				int xfrac = 0x10000 - xcounter;
				#define ismooth_putpix_x(n) { \
						*dstpix++ = (IUINT8)(((accumulate[n] + ((srcpix[n] \
							* xcounter) >> 16)) * xrecip) >> 16); \
					}
				ismooth_putpix_x(0);
				ismooth_putpix_x(1);
				ismooth_putpix_x(2);
				ismooth_putpix_x(3);
				#undef ismooth_putpix_x
				accumulate[0] = (IUINT32)((*srcpix++ * xfrac) >> 16);
				accumulate[1] = (IUINT32)((*srcpix++ * xfrac) >> 16);
				accumulate[2] = (IUINT32)((*srcpix++ * xfrac) >> 16);
				accumulate[3] = (IUINT32)((*srcpix++ * xfrac) >> 16);
				xcounter = xspace - xfrac;
			}
		}
		srcpix += srcdiff;
		dstpix += dstdiff;
	}
	return 0;
}


// c implementation
static int BasicBitmap_ResampleShrinkY_C(IUINT8 *dstpix, const IUINT8 *srcpix,
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight,
	void *workmem)
{
	IINT32 srcdiff = srcpitch - (width * 4);
	IINT32 dstdiff = dstpitch - (width * 4);
	IINT32 x, y;
	IINT32 yspace = 0x10000 * srcheight / dstheight;
	IINT32 yrecip = 0;
	IINT32 ycounter = yspace;
	IUINT32 *templine;

	IINT64 zrecip = 1;
	zrecip <<= 32;
	yrecip = (IINT32)(zrecip / yspace);

	// size = width * 4 * 4
	templine = (IUINT32*)workmem;
	if (templine == NULL) return -1;

	memset(templine, 0, width * 4 * 4);

	for (y = 0; y < srcheight; y++) {
		IUINT32 *accumulate = templine;
		if (ycounter > 0x10000) {
			for (x = 0; x < width; srcpix += 4, accumulate += 4, x++) {
				accumulate[0] += (IUINT32)srcpix[0];
				accumulate[1] += (IUINT32)srcpix[1];
				accumulate[2] += (IUINT32)srcpix[2];
				accumulate[3] += (IUINT32)srcpix[3];
			}
			ycounter -= 0x10000;
		}	else {
			IINT32 yfrac = 0x10000 - ycounter;
			IINT32 yc = ycounter;
			IINT32 yr = yrecip;
			for (x = 0; x < width; dstpix += 4, srcpix += 4, accumulate += 4, x++) {
				dstpix[0] = (IUINT8)(((accumulate[0] + ((srcpix[0] * yc) >> 16)) * yr) >> 16);
				dstpix[1] = (IUINT8)(((accumulate[1] + ((srcpix[1] * yc) >> 16)) * yr) >> 16);
				dstpix[2] = (IUINT8)(((accumulate[2] + ((srcpix[2] * yc) >> 16)) * yr) >> 16);
				dstpix[3] = (IUINT8)(((accumulate[3] + ((srcpix[3] * yc) >> 16)) * yr) >> 16);
			}
			dstpix += dstdiff;
			accumulate = templine;
			srcpix -= 4 * width;
			for (x = 0; x < width; accumulate += 4, srcpix += 4, x++) {
				accumulate[0] = (IUINT32)((srcpix[0] * yfrac) >> 16);
				accumulate[1] = (IUINT32)((srcpix[1] * yfrac) >> 16);
				accumulate[2] = (IUINT32)((srcpix[2] * yfrac) >> 16);
				accumulate[3] = (IUINT32)((srcpix[3] * yfrac) >> 16);
			}
			ycounter = yspace - yfrac;
		}
		srcpix += srcdiff;
	}

	return 0;
}


// c implementation
static int BasicBitmap_ResampleExpandX_C(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int height, long dstpitch, long srcpitch, int dstwidth, int srcwidth,
	void *workmem)
{
	IINT32 dstdiff = dstpitch - (dstwidth * 4);
	IINT32 *xidx0, *xmult0, *xmult1;
	IINT32 x, y;

	if (workmem == NULL) return -1;

	xidx0 = (IINT32*)workmem;		// size = 3 * dstwidth * 4
	xmult0 = xidx0 + dstwidth;
	xmult1 = xidx0 + dstwidth * 2;

	for (x = 0; x < dstwidth; x++) {
		xidx0[x] = x * (srcwidth - 1) / dstwidth;
		xmult1[x] = 0x10000 * ((x * (srcwidth - 1)) % dstwidth) / dstwidth;
		xmult0[x] = 0x10000 - xmult1[x];
	}

	for (y = 0; y < height; y++) {
		const IUINT8 *srcrow0 = srcpix + y * srcpitch;
		for (x = 0; x < dstwidth; x++) {
			const IUINT8 *src = srcrow0 + xidx0[x] * 4;
			IINT32 xm0 = xmult0[x];
			IINT32 xm1 = xmult1[x];
			*dstpix++ = (IUINT8)(((src[0] * xm0) + (src[4] * xm1)) >> 16);
			*dstpix++ = (IUINT8)(((src[1] * xm0) + (src[5] * xm1)) >> 16);
			*dstpix++ = (IUINT8)(((src[2] * xm0) + (src[6] * xm1)) >> 16);
			*dstpix++ = (IUINT8)(((src[3] * xm0) + (src[7] * xm1)) >> 16);
		}
		dstpix += dstdiff;
	}

	return 0;
}


// c implementation
static int BasicBitmap_ResampleExpandY_C(IUINT8 *dstpix, const IUINT8 *srcpix, 
	int width, long dstpitch, long srcpitch, int dstheight, int srcheight,
	void *workmem)
{
	IINT32 x, y;
	for (y = 0; y < dstheight; y++) {
		int yidx0 = y * (srcheight - 1) / dstheight;
		const IUINT8 *s0 = srcpix + yidx0 * srcpitch;
		const IUINT8 *s1 = s0 + srcpitch;
		int ym1 = 0x10000 * ((y * (srcheight - 1)) % dstheight) / dstheight;
		int ym0 = 0x10000 - ym1;
		for (x = 0; x < width; x++) {
			*dstpix++ = (IUINT8)(((*s0++ * ym0) + (*s1++ * ym1)) >> 16);
			*dstpix++ = (IUINT8)(((*s0++ * ym0) + (*s1++ * ym1)) >> 16);
			*dstpix++ = (IUINT8)(((*s0++ * ym0) + (*s1++ * ym1)) >> 16);
			*dstpix++ = (IUINT8)(((*s0++ * ym0) + (*s1++ * ym1)) >> 16);
		}
		dstpix += dstpitch - 4 * width;
	}

	return 0;
}


//---------------------------------------------------------------------
// resample drivers
//---------------------------------------------------------------------
static BasicBitmap_ResampleShrinkX BasicBitmap_ResampleShrinkX_Ptr = 
	BasicBitmap_ResampleShrinkX_C;

static BasicBitmap_ResampleShrinkY BasicBitmap_ResampleShrinkY_Ptr = 
	BasicBitmap_ResampleShrinkY_C;

static BasicBitmap_ResampleExpandX BasicBitmap_ResampleExpandX_Ptr = 
	BasicBitmap_ResampleExpandX_C;

static BasicBitmap_ResampleExpandY BasicBitmap_ResampleExpandY_Ptr = 
	BasicBitmap_ResampleExpandY_C;


//---------------------------------------------------------------------
// resample driver setup
//---------------------------------------------------------------------
void BasicBitmap_ResampleDriver(int id, void *ptr)
{
	switch (id) {
	case 0:
		BasicBitmap_ResampleShrinkX_Ptr = (ptr == NULL)? 
			BasicBitmap_ResampleShrinkX_C :
			(BasicBitmap_ResampleShrinkX)ptr;
		break;
	case 1:
		BasicBitmap_ResampleShrinkY_Ptr = (ptr == NULL)? 
			BasicBitmap_ResampleShrinkY_C :
			(BasicBitmap_ResampleShrinkY)ptr;
		break;
	case 3:
		BasicBitmap_ResampleExpandX_Ptr = (ptr == NULL)? 
			BasicBitmap_ResampleExpandX_C :
			(BasicBitmap_ResampleExpandX)ptr;
		break;
	case 4:
		BasicBitmap_ResampleExpandY_Ptr = (ptr == NULL)? 
			BasicBitmap_ResampleExpandY_C :
			(BasicBitmap_ResampleExpandY)ptr;
		break;
	}
}


//---------------------------------------------------------------------
// ResampleSmooth
//---------------------------------------------------------------------
int BasicBitmap_ResampleSmooth(IUINT8 *dstpix, const IUINT8 *srcpix, int dstwidth,
	int srcwidth, int dstheight, int srcheight, long dstpitch, long srcpitch)
{
	if (srcwidth == dstwidth && srcheight == dstheight) {
		long size = srcwidth * 4;
		for (int y = 0; y < dstheight; y++) {
			internal_memcpy(dstpix + y * dstpitch, srcpix + y * srcpitch, size);
		}
		return 0;
	}

	long needsrc = (srcwidth > srcheight)? srcwidth : srcheight;
	long needdst = (dstwidth > dstheight)? dstwidth : dstheight;
	long worksize = ((needsrc > needdst)? needsrc : needdst) * 32;
	long imagesize = ((long)srcwidth) * dstheight * 4;

	IUINT8 *temp = new IUINT8[imagesize + worksize];
	if (temp == NULL) return -1;

	IUINT8 *workmem = temp + imagesize;

	if (dstwidth == srcwidth) {
		if (dstheight < srcheight) {
			if (BasicBitmap_ResampleShrinkY_Ptr(dstpix, srcpix, srcwidth, dstpitch, 
				srcpitch, dstheight, srcheight, workmem) != 0) {
				delete temp;
				return -2;
			}
		}
		else if (dstheight > srcheight) {
			if (BasicBitmap_ResampleExpandY_Ptr(dstpix, srcpix, srcwidth, dstpitch,
				srcpitch, dstheight, srcheight, workmem) != 0) {
				delete temp;
				return -3;
			}
		}
		else {
			assert(0);
		}
		delete temp;
		return 0;
	}

	if (dstheight < srcheight) {
		if (BasicBitmap_ResampleShrinkY_Ptr(temp, srcpix, srcwidth, srcwidth * 4, 
			srcpitch, dstheight, srcheight, workmem) != 0) {
			delete temp;
			return -4;
		}
	}
	else if (dstheight > srcheight) {
		if (BasicBitmap_ResampleExpandY_Ptr(temp, srcpix, srcwidth, srcwidth * 4,
			srcpitch, dstheight, srcheight, workmem) != 0) {
			delete temp;
			return -5;
		}
	}
	else {
		if (dstwidth < srcwidth) {
			if (BasicBitmap_ResampleShrinkX_Ptr(dstpix, srcpix, dstheight, dstpitch, 
				srcpitch, dstwidth, srcwidth, workmem) != 0) {
				delete temp;
				return -6;
			}
		}
		else if (dstwidth > srcwidth) {
			if (BasicBitmap_ResampleExpandX_Ptr(dstpix, srcpix, dstheight, dstpitch, 
				srcpitch, dstwidth, srcwidth, workmem) != 0) {
				delete temp;
				return -7;
			}
		}
		else {
			assert(0);
		}
		delete temp;
		return 0;	
	}

	if (dstwidth < srcwidth) {
		if (BasicBitmap_ResampleShrinkX_Ptr(dstpix, temp, dstheight, dstpitch, 
			srcwidth * 4, dstwidth, srcwidth, workmem) != 0) {
			delete temp;
			return -8;
		}
	}
	else if (dstwidth > srcwidth) {
		if (BasicBitmap_ResampleExpandX_Ptr(dstpix, temp, dstheight, dstpitch, 
			srcwidth * 4, dstwidth, srcwidth, workmem) != 0) {
			delete temp;
			return -9;
		}
	}
	else {
		long size = srcwidth * 4;
		for (int y = 0; y < dstheight; y++) {
			internal_memcpy(dstpix + y * dstpitch, temp + y * size, size);
		}
	}

	delete temp;

	return 0;
}


//---------------------------------------------------------------------
// ResampleSmooth
//---------------------------------------------------------------------
int BasicBitmap_ResampleAvg(BasicBitmap *dst, int dx, int dy, int dw, int dh,
	const BasicBitmap *src, int sx, int sy, int sw, int sh)
{
	BasicBitmap::PixelFmt sfmt = src->Format();
	BasicBitmap::PixelFmt dfmt = dst->Format();
	int srcwidth = sw;
	int srcheight = sh;
	int dstwidth = dw;
	int dstheight = dh;
	const IUINT8 *ss;
	IUINT8 *dd;

	if (src->Bpp() == 32 && dst->Bpp() == 32 && sfmt == dfmt) {
		dd = (IUINT8*)dst->Address32(dx, dy);
		ss = (const IUINT8*)src->Address32(sx, sy);
		return BasicBitmap_ResampleSmooth(dd, ss, dstwidth, srcwidth, dstheight,
			srcheight, dst->Pitch(), src->Pitch());
	}

	if (src->Bpp() == 32) {
		BasicBitmap *bmp = new BasicBitmap(dstwidth, dstheight, BasicBitmap::A8R8G8B8);
		if (bmp == NULL) return -1;
		int hr = BasicBitmap_ResampleSmooth(bmp->Address8(0, 0), 
					(const IUINT8*)src->Address32(sx, sy),
					dstwidth, srcwidth, dstheight, srcheight, 
					bmp->Pitch(), src->Pitch());
		if (hr != 0) {
			delete bmp;
			return -2;
		}
		if (sfmt == BasicBitmap::X8R8G8B8) {
			bmp->SetAlphaForAllPixel(255);
		}
		dst->Convert(dx, dy, bmp, 0, 0, bmp->Width(), bmp->Height());
		delete bmp;
		return 0;
	}

	if (dst->Bpp() == 32) {
		BasicBitmap *bmp = new BasicBitmap(srcwidth, srcheight, BasicBitmap::A8R8G8B8);
		if (bmp == NULL) return -3;
		bmp->Convert(0, 0, src, 0, 0, srcwidth, srcheight);
		int hr = BasicBitmap_ResampleSmooth((IUINT8*)dst->Address32(dx, dy),
			(const IUINT8*)bmp->Address32(0, 0),
			dstwidth, srcwidth, dstheight, srcheight,
			dst->Pitch(), bmp->Pitch());
		delete bmp;
		if (hr != 0) {
			return -4;
		}
		return 0;
	}

	BasicBitmap *bs = new BasicBitmap(sw, sh, BasicBitmap::A8R8G8B8);
	BasicBitmap *bd = new BasicBitmap(dw, dh, BasicBitmap::A8R8G8B8);

	if (bs == NULL || bd == NULL) {
		if (bs) delete bs;
		if (bd) delete bd;
		return -5;
	}

	bs->Convert(0, 0, src, 0, 0, srcwidth, srcheight);

	int hr = BasicBitmap_ResampleSmooth(bs->Address8(0, 0), bd->Address8(0, 0),
		dstwidth, srcwidth, dstheight, srcheight, bd->Pitch(), bs->Pitch());

	if (hr == 0) {
		dst->Convert(dx, dy, bd, 0, 0, dstwidth, dstheight);
	}

	delete bs;
	delete bd;

	return (hr == 0)? 0 : -6;
}


//---------------------------------------------------------------------
// Resample
//---------------------------------------------------------------------
int BasicBitmap::Resample(int dx, int dy, int dw, int dh, const BasicBitmap *src, 
	int sx, int sy, int sw, int sh, ResampleFilter filter)
{
	if ((_w | _h | src->_w | src->_h) >= 0x7fff) 
		return -1;

	if (dw == sw && dh == sh) {
		Convert(dx, dy, src, sx, sy, sw, sh, 0);
		return 0;
	}
	else {
		BasicRect clipdst(0, 0, Width(), Height());
		BasicRect clipsrc(0, 0, src->Width(), src->Height());
		BasicRect bound_dst(dx, dy, dx + dw, dy + dh);
		BasicRect bound_src(sx, sy, sx + sw, sy + sh);
		if (ClipScale(&clipdst, &clipsrc, &bound_dst, &bound_src, 0) != 0) {
			return 0;
		}
		dx = bound_dst.left;
		dy = bound_dst.top;
		dw = bound_dst.right - bound_dst.left;
		dh = bound_dst.bottom - bound_dst.top;
		sx = bound_src.left;
		sy = bound_src.top;
		sw = bound_src.right - bound_src.left;
		sh = bound_src.bottom - bound_src.top;
	}	

	if (dx < 0 || dx + dw > (int)_w || dy < 0 || dy + dh > (int)_h ||
		sx < 0 || sx + sw > (int)src->_w || sy < 0 || sy + sh > (int)src->_h ||
		dh <= 0 || dw <= 0)
		return 0;

	if (sw == dw && sh == dh) {
		Convert(dx, dy, src, sx, sy, sw, sh, 0);
		return 0;
	}

	int mode = PIXEL_FLAG_SRCCOPY | PIXEL_FLAG_NOCLIP;

	switch (filter) {
	case NONE:
		Scale(dx, dy, dw, dh, src, sx, sy, sw, sh, mode, 0xffffffff);
		break;
	case LINEAR:
		Scale(dx, dy, dw, dh, src, sx, sy, sw, sh, mode | PIXEL_FLAG_LINEAR, 0xffffffff);
		break;
	case BILINEAR:
		Scale(dx, dy, dw, dh, src, sx, sy, sw, sh, mode | PIXEL_FLAG_BILINEAR, 0xffffffff);
		break;
	case AVERAGE: 
		BasicBitmap_ResampleAvg(this, dx, dy, dw, dh, src, sx, sy, sw, sh);
		break;
	case BICUBIC: {
			float incx = ((float)sw) / ((float)dw);
			float incy = ((float)sh) / ((float)dh);
			for (int j = 0; j < dh; j++) {
				float y = (float)sy + j * incy + 0.5f;
				float x = (float)sx + 0.5f;
				for (int i = 0; i < dw; i++) {
					IUINT32 color = 0;
					color = src->SampleBicubic(x, y, true);
					SetColor(dx + i, dy + j, color);
					x += incx;
				}
			}
		}
	}

	return 0;
}

///

void ResampleA1R5G5B5(unsigned short *dst, int dx, int dy, unsigned short *src, int sx, int sy)
{
	BasicBitmap bdst(dx, dy, BasicBitmap::A1R5G5B5, dst, dx * 2);
	BasicBitmap bsrc(sx, sy, BasicBitmap::A1R5G5B5, src, sx * 2);

	bdst.Resample(0, 0, dx, dy, &bsrc, 0, 0, sx, sy, BasicBitmap::ResampleFilter::BILINEAR);
}
