//============================================================
//
//	blit.c - Win32 blit handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "driver.h"
#include "blit.h"
#include "video.h"
#include "window.h"



//============================================================
//	IMPORTS
//============================================================

// from winmain.c
extern int verbose;



//============================================================
//	PARAMETERS
//============================================================

#define DEBUG_BLITTERS		0
#define MAX_BLITTER_SIZE	8192
#define MAX_RGB_ROWS		8

#undef RGB
#define RGB					0
#define RGb					1
#define RgB					2
#define Rgb					3
#define rGB					4
#define rGb					5
#define rgB					6
#define rgb					7


//============================================================
//	TYPE DEFINITIONS
//============================================================

typedef void (*blitter_func)(void);

struct rgb_descriptor
{
	int rows;
	UINT8 data[MAX_RGB_ROWS][16];
};


//============================================================
//	IMPORTS
//============================================================

extern void asmblit1_16_to_16_x1(void);
extern void asmblit1_16_to_16_x2(void);
extern void asmblit1_16_to_16_x3(void);
extern void asmblit1_16_to_24_x1(void);
extern void asmblit1_16_to_24_x2(void);
extern void asmblit1_16_to_24_x3(void);
extern void asmblit1_16_to_32_x1(void);
extern void asmblit1_16_to_32_x2(void);
extern void asmblit1_16_to_32_x3(void);

extern void asmblit1_24_to_16_x1(void);
extern void asmblit1_24_to_16_x2(void);
extern void asmblit1_24_to_16_x3(void);
extern void asmblit1_24_to_24_x1(void);
extern void asmblit1_24_to_24_x2(void);
extern void asmblit1_24_to_24_x3(void);
extern void asmblit1_24_to_32_x1(void);
extern void asmblit1_24_to_32_x2(void);
extern void asmblit1_24_to_32_x3(void);

extern void asmblit1_32_to_16_x1(void);
extern void asmblit1_32_to_16_x2(void);
extern void asmblit1_32_to_16_x3(void);
extern void asmblit1_32_to_24_x1(void);
extern void asmblit1_32_to_24_x2(void);
extern void asmblit1_32_to_24_x3(void);
extern void asmblit1_32_to_32_x1(void);
extern void asmblit1_32_to_32_x2(void);
extern void asmblit1_32_to_32_x3(void);

extern void asmblit16_16_to_16_x1(void);
extern void asmblit16_16_to_16_x1_mmx(void);
extern void asmblit16_16_to_16_x2(void);
extern void asmblit16_16_to_16_x2_mmx(void);
extern void asmblit16_16_to_16_x3(void);
extern void asmblit16_16_to_16_x3_mmx(void);
extern void asmblit16_16_to_24_x1(void);
extern void asmblit16_16_to_24_x2(void);
extern void asmblit16_16_to_24_x3(void);
extern void asmblit16_16_to_32_x1(void);
extern void asmblit16_16_to_32_x1_mmx(void);
extern void asmblit16_16_to_32_x2(void);
extern void asmblit16_16_to_32_x2_mmx(void);
extern void asmblit16_16_to_32_x3(void);
extern void asmblit16_16_to_32_x3_mmx(void);

extern void asmblit16_24_to_16_x1(void);
extern void asmblit16_24_to_16_x2(void);
extern void asmblit16_24_to_16_x3(void);
extern void asmblit16_24_to_24_x1(void);
extern void asmblit16_24_to_24_x2(void);
extern void asmblit16_24_to_24_x3(void);
extern void asmblit16_24_to_32_x1(void);
extern void asmblit16_24_to_32_x2(void);
extern void asmblit16_24_to_32_x3(void);

extern void asmblit16_32_to_16_x1(void);
extern void asmblit16_32_to_16_x2(void);
extern void asmblit16_32_to_16_x3(void);
extern void asmblit16_32_to_24_x1(void);
extern void asmblit16_32_to_24_x2(void);
extern void asmblit16_32_to_24_x3(void);
extern void asmblit16_32_to_32_x1(void);
extern void asmblit16_32_to_32_x2(void);
extern void asmblit16_32_to_32_x3(void);

extern void asmblit1_16_to_16_rgb(void);
extern void asmblit16_16_to_16_rgb(void);
extern void asmblit1_16_to_32_rgb(void);
extern void asmblit16_16_to_32_rgb(void);

extern void asmblit_header(void);
extern void asmblit_header_dirty(void);
extern void asmblit_yloop_top(void);
extern void asmblit_yloop_top_dirty(void);
extern void asmblit_middlexloop_header(void);
extern void asmblit_middlexloop_header_dirty(void);
extern void asmblit_middlexloop_top(void);
extern void asmblit_middlexloop_top_dirty(void);
extern void asmblit_middlexloop_bottom(void);
extern void asmblit_middlexloop_bottom_dirty(void);
extern void asmblit_lastxloop_header(void);
extern void asmblit_lastxloop_header_dirty(void);
extern void asmblit_lastxloop_top(void);
extern void asmblit_lastxloop_top_dirty(void);
extern void asmblit_lastxloop_bottom(void);
extern void asmblit_lastxloop_bottom_dirty(void);
extern void asmblit_yloop_bottom_dirty(void);
extern void asmblit_yloop_bottom(void);
extern void asmblit_footer(void);
extern void asmblit_footer_dirty(void);
extern void asmblit_footer_mmx(void);
extern void asmblit_footer_mmx_dirty(void);

extern int asmblit_has_mmx(void);
extern int asmblit_has_xmm(void);


//============================================================
//	GLOBAL VARIABLES
//============================================================

void *asmblit_srcdata;
UINT32 asmblit_srcheight;
void *asmblit_srclookup;

void *asmblit_dstdata;
UINT32 asmblit_dstpitch;

void *asmblit_dirtydata;

UINT32 asmblit_mmxmask[4];
UINT32 asmblit_rgbmask[MAX_VIDEO_HEIGHT * 2 * 16];


//============================================================
//	LOCAL VARIABLES
//============================================================

// blitter cache
static UINT8				active_fast_blitter[MAX_BLITTER_SIZE];
static UINT8				active_update_blitter[MAX_BLITTER_SIZE];

// current parameters
static struct win_blit_params	active_blitter_params;

// MMX/XMM supported?
static int					use_mmx = -1;
static int					use_xmm = -1;

// register index to size table
static const UINT8 regoffset_regsize[32] =
{
	4,4,4,4, 2,2,2,2, 1,1,1,1, 1,1,1,1,
	8,8,8,8, 8,8,8,8, 16,16,16,16, 16,16,16,16
};

// register index to bits table
static const UINT8 regoffset_regbits[32] =
{
	0,1,2,3, 0,1,2,3, 0,1,2,3, 4,5,6,7,
	0,1,2,3, 4,5,6,7, 0,1,2,3, 4,5,6,7
};

// register index to effective modified size table
static const UINT8 regoffset_modsize[32] =
{
	4,4,4,4, 2,2,2,2, 1,1,1,1, 2,2,2,2,
	8,8,8,8, 8,8,8,8, 16,16,16,16, 16,16,16,16
};

// table for 16-pixel RGB pattern
static struct rgb_descriptor rgb16_desc =
{
	8,
	{
		{ rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb, rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb },
		{ rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb, rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb },
		{ rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb, rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb },
		{ rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb, rGb,rgB,Rgb,rgb,rGb,rgB,Rgb,rgb },
		{ rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb, rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb },
		{ rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb, rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb },
		{ rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb, rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb },
		{ rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb, rgb,rGb,rgB,Rgb,rgb,rGb,rgB,Rgb }
	}
};

// table for 6-pixel RGB pattern
static struct rgb_descriptor rgb6_desc =
{
	6,
	{
		{ rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb, rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb },
		{ rgB,rGb,rgB,rGb,rgB,rGb,rgB,rGb, rgB,rGb,rgB,rGb,rgB,rGb,rgB,rGb },
		{ Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb, Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb },
		{ Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB, Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB },
		{ rGb,rgB,rGb,rgB,rGb,rgB,rGb,rgB, rGb,rgB,rGb,rgB,rGb,rgB,rGb,rgB },
		{ rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb, rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb }
	}
};

// table for 4-pixel RGB pattern
static struct rgb_descriptor rgb4_desc =
{
	4,
	{
		{ rgb,rgb,rgb,rgb,RgB,RGb,rGB,rgb, rgb,rgb,rgb,rgb,RgB,RGb,rGB,rgb },
		{ RgB,RGb,rGB,rgb,RgB,RGb,rGB,rgb, RgB,RGb,rGB,rgb,RgB,RGb,rGB,rgb },
		{ RgB,RGb,rGB,rgb,rgb,rgb,rgb,rgb, RgB,RGb,rGB,rgb,rgb,rgb,rgb,rgb },
		{ RgB,RGb,rGB,rgb,RgB,RGb,rGB,rgb, RgB,RGb,rGB,rgb,RgB,RGb,rGB,rgb }
	}
};

// table for 3-pixel RGB pattern
static struct rgb_descriptor rgb3_desc =
{
	3,
	{
		{ Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB, Rgb,rgB,Rgb,rgB,Rgb,rgB,Rgb,rgB },
		{ rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb, rGb,Rgb,rGb,Rgb,rGb,Rgb,rGb,Rgb },
		{ rgB,rGb,rgB,rGb,rgB,rGb,rgB,rGb, rgB,rGb,rgB,rGb,rgB,rGb,rgB,rGb }
	}
};

// table for tiny mesh pattern
static struct rgb_descriptor rgbtiny_desc =
{
	4,
	{
		{ rGb,rgB,Rgb,RGB,rGb,rgB,Rgb,RGB, rGb,rgB,Rgb,RGB,rGb,rgB,Rgb,RGB },
		{ rgb,RGB,rgb,RGB,rgb,RGB,rgb,RGB, rgb,RGB,rgb,RGB,rgb,RGB,rgb,RGB },
		{ Rgb,RGB,rGb,rgB,Rgb,RGB,rGb,rgB, Rgb,RGB,rGb,rgB,Rgb,RGB,rGb,rgB },
		{ rgb,RGB,rgb,RGB,rgb,RGB,rgb,RGB, rgb,RGB,rgb,RGB,rgb,RGB,rgb,RGB }
	}
};

// table for 4-pixel vertical (Now 8-pixel) RGB pattern
static struct rgb_descriptor rgb4v_desc =
{
	8,
	{
		{ rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb, rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb },
		{ rGB,rGB,rgb,rGB,rGB,rGB,rgb,rGB, rGB,rGB,rgb,rGB,rGB,rGB,rgb,rGB },
		{ RGb,RGb,rgb,RGb,RGb,RGb,rgb,RGb, RGb,RGb,rgb,RGb,RGb,RGb,rgb,RGb },
		{ RgB,RgB,rgb,RgB,RgB,RgB,rgb,RgB, RgB,RgB,rgb,RgB,RgB,RgB,rgb,RgB },
		{ rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb, rgb,rgb,rgb,rgb,rgb,rgb,rgb,rgb },
		{ rgb,rGB,rGB,rGB,rgb,rGB,rGB,rGB, rgb,rGB,rGB,rGB,rgb,rGB,rGB,rGB },
		{ rgb,RGb,RGb,RGb,rgb,RGb,RGb,RGb, rgb,RGb,RGb,RGb,rgb,RGb,RGb,RGb },
		{ rgb,RgB,RgB,RgB,rgb,RgB,RgB,RgB, rgb,RgB,RgB,RgB,rgb,RgB,RgB,RgB }
	}
};

// table for 75% vertical win_old_scanlines RGB pattern
static struct rgb_descriptor scan75v_desc =
{
	1,
	{
		{ RGB,rgb,RGB,rgb,RGB,rgb,RGB,rgb, RGB,rgb,RGB,rgb,RGB,rgb,RGB,rgb }
	}
};

// table for "sharp" pattern, i.e., no change, just pixel double
static struct rgb_descriptor sharp_desc =
{
	1,
	{
		{ RGB,RGB,RGB,RGB,RGB,RGB,RGB,RGB, RGB,RGB,RGB,RGB,RGB,RGB,RGB,RGB }
	}
};


//============================================================
//	PROTOTYPES
//============================================================

static void generate_blitter(const struct win_blit_params *blit);



//============================================================
//	BLITTER CORE TABLES
//============================================================

static void (*blit1_core[4][4][3])(void) =
{
	{
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
	},
	{
		{ NULL,                 NULL,                 NULL },
		{ asmblit1_16_to_16_x1, asmblit1_16_to_16_x2, asmblit1_16_to_16_x3 },
		{ asmblit1_16_to_24_x1, asmblit1_16_to_24_x2, asmblit1_16_to_24_x3 },
		{ asmblit1_16_to_32_x1, asmblit1_16_to_32_x2, asmblit1_16_to_32_x3 }
	},
	{
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
	},
	{
		{ NULL,                 NULL,                 NULL },
		{ asmblit1_32_to_16_x1, asmblit1_32_to_16_x2, asmblit1_32_to_16_x3 },
		{ asmblit1_32_to_24_x1, asmblit1_32_to_24_x2, asmblit1_32_to_24_x3 },
		{ asmblit1_32_to_32_x1, asmblit1_32_to_32_x2, asmblit1_32_to_32_x3 }
	}
};

static void (*blit16_core[4][4][3])(void) =
{
	{
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
	},
	{
		{ NULL,                  NULL,                  NULL },
		{ asmblit16_16_to_16_x1, asmblit16_16_to_16_x2, asmblit16_16_to_16_x3 },
		{ asmblit16_16_to_24_x1, asmblit16_16_to_24_x2, asmblit16_16_to_24_x3 },
		{ asmblit16_16_to_32_x1, asmblit16_16_to_32_x2, asmblit16_16_to_32_x3 }
	},
	{
		{ NULL,                  NULL,                  NULL },
		{ NULL,                  NULL,                  NULL },
		{ NULL,                  NULL,                  NULL },
		{ NULL,                  NULL,                  NULL },
	},
	{
		{ NULL,                  NULL,                  NULL },
		{ asmblit16_32_to_16_x1, asmblit16_32_to_16_x2, asmblit16_32_to_16_x3 },
		{ asmblit16_32_to_24_x1, asmblit16_32_to_24_x2, asmblit16_32_to_24_x3 },
		{ asmblit16_32_to_32_x1, asmblit16_32_to_32_x2, asmblit16_32_to_32_x3 }
	}
};

static void (*blit16_core_mmx[4][4][3])(void) =
{
	{
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
		{ NULL,                 NULL,                 NULL },
	},
	{
		{ NULL,                      NULL,                      NULL },
		{ asmblit16_16_to_16_x1_mmx, asmblit16_16_to_16_x2_mmx, asmblit16_16_to_16_x3_mmx },
		{ NULL,                      NULL,                      NULL },
		{ asmblit16_16_to_32_x1_mmx, asmblit16_16_to_32_x2_mmx, asmblit16_16_to_32_x3_mmx },
	},
	{
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
	},
	{
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
		{ NULL,                      NULL,                      NULL },
	}
};

static void (*blit1_core_rgb[4][4])(void) =
{
	{ NULL, NULL, NULL, NULL },
	{ NULL, asmblit1_16_to_16_rgb, NULL, asmblit1_16_to_32_rgb },
	{ NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

static void (*blit16_core_rgb[4][4])(void) =
{
	{ NULL, NULL, NULL, NULL },
	{ NULL, asmblit16_16_to_16_rgb, NULL, asmblit16_16_to_32_rgb },
	{ NULL, NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};



//============================================================
//	win_perform_blit
//============================================================

int win_perform_blit(const struct win_blit_params *blit, int update)
{
	int srcdepth = (blit->srcdepth + 7) / 8;
	int dstdepth = (blit->dstdepth + 7) / 8;
	blitter_func blitter;

	// if anything important has changed, fix it
	if (blit->srcwidth != active_blitter_params.srcwidth ||
		blit->srcdepth != active_blitter_params.srcdepth ||
		blit->srcpitch != active_blitter_params.srcpitch ||
		blit->dstdepth != active_blitter_params.dstdepth ||
		blit->dstpitch != active_blitter_params.dstpitch ||
		blit->dstyskip != active_blitter_params.dstyskip ||
		blit->dstxscale != active_blitter_params.dstxscale ||
		blit->dstyscale != active_blitter_params.dstyscale ||
		blit->dsteffect != active_blitter_params.dsteffect ||
		(blit->dirtydata != NULL) != (active_blitter_params.dirtydata != NULL))
	{
		generate_blitter(blit);
		active_blitter_params = *blit;
	}

	// copy data to the globals
	asmblit_srcdata = (UINT8 *)blit->srcdata + blit->srcpitch * blit->srcyoffs + srcdepth * blit->srcxoffs;
	asmblit_srcheight = blit->srcheight;
	asmblit_srclookup = blit->srclookup;

	asmblit_dstdata = (UINT8 *)blit->dstdata + blit->dstpitch * blit->dstyoffs + dstdepth * blit->dstxoffs;
	asmblit_dstpitch = blit->dstpitch;

	asmblit_dirtydata = blit->dirtydata;

	if (((UINT32)asmblit_dstdata & 7) != 0)
		fprintf(stderr, "Misaligned blit to: %08x\n", (UINT32)asmblit_dstdata);

	// pick the blitter
	blitter = update ? (blitter_func)active_update_blitter : (blitter_func)active_fast_blitter;
	(*blitter)();
	return 1;
}



//============================================================
//	snippet_length
//============================================================

static int snippet_length(void *snippet)
{
	UINT8 *current = snippet;

	// determine the length of a code snippet
	while (current[0] != 0xcc || current[1] != 0xcc || current[2] != 0xcc || current[3] != 0xf0)
		current++;
	return current - (UINT8 *)snippet;
}



//============================================================
//	emit_snippet_pair
//============================================================

static void emit_snippet(void *snippet, UINT8 **dest)
{
	// emit a snippet as-is
	int length = snippet_length(snippet);
	memcpy(*dest, snippet, length);
	*dest += length;
}



//============================================================
//	emit_mov_edi_reg
//============================================================

static void emit_mov_edi_reg(int reg, int offs, UINT8 **dest)
{
	int regsize = regoffset_regsize[reg];
	int regbits = regoffset_regbits[reg];

	// first byte depends on register size
	if (regsize == 1)
		*(*dest)++ = 0x88;
	else if (regsize == 2)
		*(*dest)++ = 0x66, *(*dest)++ = 0x89;
	else if (regsize == 4)
		*(*dest)++ = 0x89;
	else if (regsize == 8)
		*(*dest)++ = 0x0f, *(*dest)++ = 0x7f;
	else if (regsize == 16)
		*(*dest)++ = 0xf3, *(*dest)++ = 0x0f, *(*dest)++ = 0x7f;

	// second byte is mod r/m byte; offset follows
	if (offs == 0)
		*(*dest)++ = 0x07 | (regbits << 3);
	else if (offs >= -128 && offs <= 127)
		*(*dest)++ = 0x47 | (regbits << 3), *(*dest)++ = offs & 0xff;
	else
		*(*dest)++ = 0x87 | (regbits << 3), *(UINT32 *)*dest = offs, *dest += 4;
}



//============================================================
//	emit_mov_edi_0
//============================================================

static void emit_mov_edi_0(int reg, int offs, UINT8 **dest)
{
	// register parameter is only used to determine the size
	int regsize = regoffset_regsize[reg];

	// first byte depends on register size
	if (regsize == 1)
		*(*dest)++ = 0xc6;
	else if (regsize == 2)
		*(*dest)++ = 0x66, *(*dest)++ = 0xc7;
	else if (regsize == 4)
		*(*dest)++ = 0xc7;
	else if (regsize == 8)
		*(*dest)++ = 0x0f, *(*dest)++ = 0x7f;
	else if (regsize == 16)
		*(*dest)++ = 0xf3, *(*dest)++ = 0x0f, *(*dest)++ = 0x7f;

	// second byte is mod r/m byte; offset follows (for non-MMX registers)
	if (regsize < 8)
	{
		if (offs == 0)
			*(*dest)++ = 0x07 | (0 << 3);
		else if (offs >= -128 && offs <= 127)
			*(*dest)++ = 0x47 | (0 << 3), *(*dest)++ = offs & 0xff;
		else
			*(*dest)++ = 0x87 | (0 << 3), *(UINT32 *)*dest = offs, *dest += 4;

		// immediate follows that
		if (regsize == 1)
			*(*dest)++ = 0;
		else if (regsize == 2)
			*(UINT16 *)*dest = 0, *dest += 2;
		else
			*(UINT32 *)*dest = 0, *dest += 4;
	}

	// for MMX registers, we assume that (X)MM7 has been zeroed
	else
	{
		if (offs == 0)
			*(*dest)++ = 0x07 | (7 << 3);
		else if (offs >= -128 && offs <= 127)
			*(*dest)++ = 0x47 | (7 << 3), *(*dest)++ = offs & 0xff;
		else
			*(*dest)++ = 0x87 | (7 << 3), *(UINT32 *)*dest = offs, *dest += 4;
	}
}



//============================================================
//	emit_reduce_brightness
//============================================================

static void emit_reduce_brightness(int count, const UINT8 *reglist, const struct win_blit_params *blit, UINT8 **dest)
{
	int regsize[4] = { 0 };
	UINT32 mask;
	int shift, i;

	// do nothing for less than 16bpp
	if (blit->dstdepth < 16)
		return;

	// find all the registers we need to tweak
	for (i = 0; i < count; i++)
	{
		int size = regoffset_modsize[reglist[i]];
		int idx = reglist[i] & 3;
		if (size > regsize[idx])
			regsize[idx] = size;
	}

	// for 25/75% we need to shift right by 2
	if (blit->dsteffect == EFFECT_SCANLINE_25 || blit->dsteffect == EFFECT_SCANLINE_75)
	{
		for (i = 0; i < 4; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 1)
					*(*dest)++ = 0xc0;
				else if (regsize[i] == 2)
					*(*dest)++ = 0x66, *(*dest)++ = 0xc1;
				else if (regsize[i] == 4)
					*(*dest)++ = 0xc1;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (5 << 3) | (i & 7);
				*(*dest)++ = 2;
			}
		shift = 2;
	}

	// for 50% we need to shift right by 1
	else
	{
		for (i = 0; i < 4; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 1)
					*(*dest)++ = 0xd0;
				else if (regsize[i] == 2)
					*(*dest)++ = 0x66, *(*dest)++ = 0xd1;
				else if (regsize[i] == 4)
					*(*dest)++ = 0xd1;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (5 << 3) | (i & 7);
			}
		shift = 1;
	}

	// now determine the mask to use
	if (blit->dstdepth == 16)
	{
		mask = (0xff >> (win_color16_rsrc_shift + shift)) << win_color16_rdst_shift;
		mask |= (0xff >> (win_color16_gsrc_shift + shift)) << win_color16_gdst_shift;
		mask |= (0xff >> (win_color16_bsrc_shift + shift)) << win_color16_bdst_shift;
		mask |= mask << 16;
	}
	else
	{
		mask = (0xff >> shift) << win_color32_rdst_shift;
		mask |= (0xff >> shift) << win_color32_gdst_shift;
		mask |= (0xff >> shift) << win_color32_bdst_shift;
	}

	// generate an AND with that mask
	// emit opcode
	for (i = 0; i < 4; i++)
		if (regsize[i])
		{
			if (regsize[i] == 1)
				*(*dest)++ = 0x80;
			else if (regsize[i] == 2)
				*(*dest)++ = 0x66, *(*dest)++ = 0x81;
			else if (regsize[i] == 4)
				*(*dest)++ = 0x81;

			// emit modrm
			*(*dest)++ = 0xc0 | (4 << 3) | i;

			// emit mask
			if (regsize[i] == 1)
				*(*dest)++ = mask;
			else if (regsize[i] == 2)
				*(UINT16 *)*dest = mask, *dest += 2;
			else if (regsize[i] == 4)
				*(UINT32 *)*dest = mask, *dest += 4;
		}

	// for the 75% case, we need to multiply by 3 (lea reg,[reg+reg*2])
	if (blit->dsteffect == EFFECT_SCANLINE_75)
	{
		for (i = 0; i < 4; i++)
			if (regsize[i] > 1 && regsize[i] < 8)
			{
				// emit opcode
				if (regsize[i] == 2)
					*(*dest)++ = 0x66, *(*dest)++ = 0x8d;
				else if (regsize[i] == 4)
					*(*dest)++ = 0x8d;

				// emit modrm + sib
				*(*dest)++ = 0x00 | (i << 3) | 4;
				*(*dest)++ = 0x40 | (i << 3) | i;
			}
	}
}



//============================================================
//	emit_reduce_brightness_mmx
//============================================================

static void emit_reduce_brightness_mmx(int count, const UINT8 *reglist, const struct win_blit_params *blit, UINT8 **dest)
{
	int freelist[16], tempfree[16];
	int regsize[16] = { 0 };
	int i, j, shift;
	UINT32 mask;

	// do nothing for less than 16bpp
	if (blit->dstdepth < 16)
		return;

	// find all the registers we need to tweak
	for (i = 0; i < count; i++)
	{
		int size = regoffset_modsize[reglist[i]];
		int idx = reglist[i] & 15;
		if (size > regsize[idx])
			regsize[idx] = size;
	}

	// now find some free registers
	for (i = j = 0; i < 8; i++)
		if (!regsize[i])
			tempfree[j++] = i;
	for (i = j = 0; i < 8; i++)
		if (!regsize[8 + i])
			tempfree[8 + j++] = i;

	// associate a free register with each used one
	for (i = j = 0; i < 8; i++)
		if (regsize[i])
			freelist[i] = tempfree[j++];
	for (i = j = 0; i < 8; i++)
		if (regsize[8 + i])
			freelist[8 + i] = tempfree[8 + j++];

	// for 75% (MMX) we need to copy to a free reg temporarily
	if (blit->dsteffect == EFFECT_SCANLINE_75)
	{
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0x6f;

				// emit modrm
				*(*dest)++ = 0xc0 | (freelist[i] << 3) | (i & 7);
			}
	}

	// for 25% we need to shift right by 2
	if (blit->dsteffect == EFFECT_SCANLINE_25)
	{
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0x73;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (2 << 3) | (i & 7);
				*(*dest)++ = 2;
			}
		shift = 2;
	}

	// for 50% we need to shift right by 1
	else if (blit->dsteffect == EFFECT_SCANLINE_50)
	{
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0x73;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (2 << 3) | (i & 7);
				*(*dest)++ = 1;
			}
		shift = 1;
	}

	// for 75% we need to shift the temp right by 2
	else
	{
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0x73;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (2 << 3) | freelist[i];
				*(*dest)++ = 2;
			}
		shift = 2;
	}

	// now determine the masks to use
	if (blit->dstdepth == 16)
	{
		mask = (0xff >> (win_color16_rsrc_shift + shift)) << win_color16_rdst_shift;
		mask |= (0xff >> (win_color16_gsrc_shift + shift)) << win_color16_gdst_shift;
		mask |= (0xff >> (win_color16_bsrc_shift + shift)) << win_color16_bdst_shift;
		mask |= mask << 16;
	}
	else
	{
		mask = (0xff >> shift) << win_color32_rdst_shift;
		mask |= (0xff >> shift) << win_color32_gdst_shift;
		mask |= (0xff >> shift) << win_color32_bdst_shift;
	}
	asmblit_mmxmask[0] = asmblit_mmxmask[1] = asmblit_mmxmask[2] = asmblit_mmxmask[3] = mask;

	// generate an AND with that mask
	if (blit->dsteffect != EFFECT_SCANLINE_75)
	{
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0xdb;

				// emit modrm
				*(*dest)++ = 0x00 | ((i & 7) << 3) | 5;

				// emit address
				*(UINT32 *)*dest = (UINT32)&asmblit_mmxmask;
				*dest += 4;
			}
	}

	// for 75% we and the temporary instead, then subtract
	else
	{
		// and
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0xdb;

				// emit modrm
				*(*dest)++ = 0x00 | (freelist[i] << 3) | 5;

				// emit address
				*(UINT32 *)*dest = (UINT32)&asmblit_mmxmask;
				*dest += 4;
			}

		// sub
		for (i = 0; i < 16; i++)
			if (regsize[i])
			{
				// emit opcode
				if (regsize[i] == 16)
					*(*dest)++ = 0x66;
				*(*dest)++ = 0x0f, *(*dest)++ = 0xfa;

				// emit modrm and count
				*(*dest)++ = 0xc0 | (i << 3) | freelist[i];
			}
	}
}



//============================================================
//	generate_rgb_masks
//============================================================

static void generate_rgb_masks(const struct rgb_descriptor *desc, const struct win_blit_params *blit)
{
	int i;

	// generate an entry for each row of the destination
	for (i = 0; i < MAX_VIDEO_HEIGHT * 2; i++)
	{
		const UINT8 *src = &desc->data[i % desc->rows][0];
		UINT32 *dst = &asmblit_rgbmask[i * 16];
		int j;

		// loop over all 16 pixels
		for (j = 0; j < 16; j++)
		{
			int rmask = (src[j] & 4) ? 0x3f : 0;
			int gmask = (src[j] & 2) ? 0x3f : 0;
			int bmask = (src[j] & 1) ? 0x3f : 0;
			UINT32 mask;

			// now determine the masks to use
			if (blit->dstdepth == 16)
			{
				mask = (rmask >> win_color16_rsrc_shift) << win_color16_rdst_shift;
				mask |= (gmask >> win_color16_gsrc_shift) << win_color16_gdst_shift;
				mask |= (bmask >> win_color16_bsrc_shift) << win_color16_bdst_shift;
				*(UINT16 *)dst = mask;
				dst = (UINT32 *)((UINT16 *)dst + 1);
			}
			else
			{
				mask = rmask << win_color32_rdst_shift;
				mask |= gmask << win_color32_gdst_shift;
				mask |= bmask << win_color32_bdst_shift;
				*dst++ = mask;
			}
		}
	}
}


//============================================================
//	emit_expansion
//============================================================

static void emit_expansion(int count, const UINT8 *reglist, const UINT32 *offslist, const struct win_blit_params *blit, UINT8 **dest, int update)
{
	int row, i, rowoffs = 0;
	int has_mmx = 0;

	// determine if we have MMX registers in the list
	for (i = 0; i < count; i++)
		if (reglist[i] >= 16)
			has_mmx = 1;

	// loop over copied lines and blit them
	for (row = 0; row < blit->dstyscale; row++)
	{
		// handle dimmed scanlines
		if (blit->dsteffect >= EFFECT_SCANLINE_25 && blit->dsteffect <= EFFECT_SCANLINE_75 && row != 0 && row == blit->dstyscale - 1)
		{
			if (has_mmx)
				emit_reduce_brightness_mmx(count, reglist, blit, dest);
			else
				emit_reduce_brightness(count, reglist, blit, dest);
		}

		// store the results
		for (i = 0; i < count; i++)
			emit_mov_edi_reg(reglist[i], offslist[i] + rowoffs, dest);
		rowoffs += blit->dstpitch;
	}

	// if updating, and generating scanlines, store a 0
	if (update && blit->dstyskip)
	{
		// if we have any MMX, generate a PXOR MM7,MM7
		if (has_mmx)
		{
			*(*dest)++ = 0x0f, *(*dest)++ = 0xef;
			*(*dest)++ = 0xc0 | (7 << 3) | 7;
		}

		// generate the moves
		for (row = 0; row < blit->dstyskip; row++)
		{
			for (i = 0; i < count; i++)
				emit_mov_edi_0(reglist[i], offslist[i] + rowoffs, dest);
			rowoffs += blit->dstpitch;
		}
	}
}



//============================================================
//	check_for_mmx
//============================================================

static void check_for_mmx(void)
{
	use_mmx = asmblit_has_mmx();
	use_xmm = asmblit_has_xmm();
	if (use_xmm && verbose)
		fprintf(stderr, "SSE2 supported\n");
	else if (use_mmx && verbose)
		fprintf(stderr, "MMX supported\n");
	else if (verbose)
		fprintf(stderr, "MMX not supported\n");
}



//============================================================
//	expand_blitter
//============================================================

static void expand_blitter(int which, const struct win_blit_params *blit, UINT8 **dest, int update)
{
	int srcdepth_index = (blit->srcdepth + 7) / 8 - 1;
	int dstdepth_index = (blit->dstdepth + 7) / 8 - 1;
	int xscale_index = blit->dstxscale - 1;
	UINT8 *blitter = NULL;
	int i;

	// determine MMX/XMM support
	if (use_mmx == -1)
		check_for_mmx();

	// find the blitter -- custom case
	switch (blit->dsteffect)
	{
		case EFFECT_RGB16:
		case EFFECT_RGB6:
		case EFFECT_RGB4:
		case EFFECT_RGB4V:
		case EFFECT_RGB3:
		case EFFECT_RGB_TINY:
		case EFFECT_SCANLINE_75V:
			if (use_mmx)
			{
				if (blit->dsteffect == EFFECT_RGB16)
					generate_rgb_masks(&rgb16_desc, blit);
				else if (blit->dsteffect == EFFECT_RGB6)
					generate_rgb_masks(&rgb6_desc, blit);
				else if (blit->dsteffect == EFFECT_RGB4)
					generate_rgb_masks(&rgb4_desc, blit);
				else if (blit->dsteffect == EFFECT_RGB4V)
					generate_rgb_masks(&rgb4v_desc, blit);
				else if (blit->dsteffect == EFFECT_RGB3)
					generate_rgb_masks(&rgb3_desc, blit);
				else if (blit->dsteffect == EFFECT_RGB_TINY)
					generate_rgb_masks(&rgbtiny_desc, blit);
				else if (blit->dsteffect == EFFECT_SCANLINE_75V)
					generate_rgb_masks(&scan75v_desc, blit);
				else if (blit->dsteffect == EFFECT_SHARP)
					generate_rgb_masks(&sharp_desc, blit);

				if (which == 1)
					blitter = (UINT8 *)(blit1_core_rgb[srcdepth_index][dstdepth_index]);
				else
					blitter = (UINT8 *)(blit16_core_rgb[srcdepth_index][dstdepth_index]);
			}
			break;
	}

	// find the blitter -- standard case
	if (blitter == NULL)
	{
		if (which == 1)
			blitter = (UINT8 *)(blit1_core[srcdepth_index][dstdepth_index][xscale_index]);
		else
		{
			blitter = (UINT8 *)(blit16_core_mmx[srcdepth_index][dstdepth_index][xscale_index]);
			if (!use_mmx || blitter == NULL)
				blitter = (UINT8 *)(blit16_core[srcdepth_index][dstdepth_index][xscale_index]);
		}
	}

	// copy until the end
	while (blitter[0] != 0xcc || blitter[1] != 0xcc || blitter[2] != 0xcc || blitter[3] != 0xf0)
	{
		UINT8 reglist[8];
		UINT32 offslist[8];
		int count;

		// if we're not at a special tag, just copy and continue
		if (blitter[0] != 0xcc || blitter[1] != 0xcc || blitter[2] != 0xcc)
		{
			*(*dest)++ = *blitter++;
			continue;
		}

		// if we're at a shift tag, process it
		if ((blitter[3] & 0xf0) == 0x20)
		{
			int dstreg = 0, rshift1 = 0, rshift2 = 0, lshift = 0;
			int shifttype = blitter[3] & 15;
			blitter += 4;

			// determine parameters
			switch (shifttype & 3)
			{
				case 0:	rshift1 = win_color32_rdst_shift; rshift2 = win_color16_rsrc_shift; lshift = win_color16_rdst_shift; break;
				case 1:	rshift1 = win_color32_gdst_shift; rshift2 = win_color16_gsrc_shift; lshift = win_color16_gdst_shift; break;
				case 2:	rshift1 = win_color32_bdst_shift; rshift2 = win_color16_bsrc_shift; lshift = win_color16_bdst_shift; break;
			}

			// determine registers
			switch (shifttype >> 2)
			{
				case 0: dstreg = 3;	break;	// ebx
				case 1: dstreg = 1; break;	// ecx
			}

			// emit the right shift instruction
			*(*dest)++ = 0xc1;
			*(*dest)++ = 0xc0 | (5 << 3) | dstreg;
			*(*dest)++ = rshift1 + rshift2;

			// emit the AND instruction
			*(*dest)++ = 0x83;
			*(*dest)++ = 0xc0 | (4 << 3) | dstreg;
			*(*dest)++ = 0xff >> rshift2;

			// emit the left shift instruction
			*(*dest)++ = 0xc1;
			*(*dest)++ = 0xc0 | (4 << 3) | dstreg;
			*(*dest)++ = lshift;
		}

		// if we're at an expansion tag, process it
		else if ((blitter[3] & 0xf0) == 0x30 || (blitter[3] & 0xf0) == 0x40)
		{
			// store the first one
			count = 1;
			reglist[0] = blitter[3] - 0x30;
			blitter += 4;

			// loop over all the remaining
			while (blitter[0] == 0xcc && blitter[1] == 0xcc && blitter[2] == 0xcc &&
					((blitter[3] & 0xf0) == 0x30 || (blitter[3] & 0xf0) == 0x40))
			{
				reglist[count++] = blitter[3] - 0x30;
				blitter += 4;
			}

			// now get the offsets
			for (i = 0; i < count; i++)
			{
				offslist[i] = *(UINT32 *)blitter;
				blitter += 4;
			}

			// now generate the expansion code
			emit_expansion(count, reglist, offslist, blit, dest, update);
		}

		// otherwise, copy as-is
		else
			*(*dest)++ = *blitter++;
	}
}



//============================================================
//	fixup_addresses
//============================================================

static void fixup_addresses(UINT8 **fixups, UINT8 *start, UINT8 *end)
{
	// loop over the final blitter
	for ( ; start < end; start++)
	{
		// if this is an address fixup, do it
		if (start[0] == 0xcc && start[1] == 0xcc && start[2] == 0xcc && (start[3] & 0xf0) == 0x00)
		{
			int idx = start[3] & 0x0f;
			*(UINT32 *)start = fixups[idx] - (start + 4);
		}
	}
}



//============================================================
//	fixup_values
//============================================================

static void fixup_values(UINT32 *fixups, UINT8 *start, UINT8 *end)
{
	// loop over the final blitter
	for ( ; start < end; start++)
	{
		// if this is an address fixup, do it
		if (start[0] == 0xcc && start[1] == 0xcc && start[2] == 0xcc && (start[3] & 0xf0) == 0x10)
		{
			int idx = start[3] & 0x0f;
			*(UINT32 *)start = fixups[idx];
		}
	}
}



//============================================================
//	generate_blitter
//============================================================

#define EMIT_SNIPPET_PAIR(snipname) \
	emit_snippet(asmblit_##snipname, &fastptr); \
	if (blit->dirtydata) emit_snippet(asmblit_##snipname##_dirty, &fastptr); \
	emit_snippet(asmblit_##snipname, &updateptr); \

#define EMIT_SNIPPET_PAIR_REVERSE(snipname) \
	if (blit->dirtydata) emit_snippet(asmblit_##snipname##_dirty, &fastptr); \
	emit_snippet(asmblit_##snipname, &fastptr); \
	emit_snippet(asmblit_##snipname, &updateptr); \

#define EXPAND_BLITTER_PAIR(count) \
	expand_blitter(count, blit, &fastptr, 0); \
	expand_blitter(count, blit, &updateptr, 1); \

#define SET_FIXUPS(index) \
	addrfixups[0][index] = fastptr; \
	addrfixups[1][index] = updateptr;

static void generate_blitter(const struct win_blit_params *blit)
{
	UINT8 *fastptr = active_fast_blitter;
	UINT8 *updateptr = active_update_blitter;

	UINT8 *addrfixups[2][16];
	UINT32 valuefixups[16];
	int middle, last;

#if DEBUG_BLITTERS
	fprintf(stderr, "Generating blitter\n");
#endif

	// determine how many pixels to do at the middle, and end
	middle = blit->srcwidth / 16;
	last = blit->srcwidth % 16;

	// generate blitter loop

	// function header
	EMIT_SNIPPET_PAIR(header);

	// top of outer (Y) loop
	SET_FIXUPS(0);
	EMIT_SNIPPET_PAIR(yloop_top);

		// top of middle (X) loop
		if (middle)
		{
			EMIT_SNIPPET_PAIR(middlexloop_header);
			SET_FIXUPS(1);
			EMIT_SNIPPET_PAIR(middlexloop_top);
			EXPAND_BLITTER_PAIR(16);
			SET_FIXUPS(2);
			EMIT_SNIPPET_PAIR(middlexloop_bottom);
		}

		// top of last (X) loop
		if (last)
		{
			EMIT_SNIPPET_PAIR(lastxloop_header);
			SET_FIXUPS(3);
			EMIT_SNIPPET_PAIR(lastxloop_top);
			EXPAND_BLITTER_PAIR(1);
			EMIT_SNIPPET_PAIR(lastxloop_bottom);
		}

	SET_FIXUPS(4);
	EMIT_SNIPPET_PAIR_REVERSE(yloop_bottom);

	// function footer
	if (use_mmx)
	{
		EMIT_SNIPPET_PAIR(footer_mmx);
	}
	EMIT_SNIPPET_PAIR(footer);

	// fixup local jmps
	fixup_addresses(&addrfixups[0][0], active_fast_blitter, fastptr);
	fixup_addresses(&addrfixups[1][0], active_update_blitter, updateptr);

	// fixup data values
	valuefixups[0] = (blit->srcdepth + 7) / 8;
	valuefixups[1] = ((blit->dstdepth + 7) / 8) * blit->dstxscale;
	valuefixups[2] = valuefixups[0] * 16;
	valuefixups[3] = valuefixups[1] * 16;
	valuefixups[4] = blit->srcpitch;
	valuefixups[5] = blit->dstpitch * (blit->dstyscale + blit->dstyskip);
	valuefixups[6] = blit->dirtypitch;
	valuefixups[7] = middle;
	valuefixups[8] = last;
	fixup_values(valuefixups, active_fast_blitter, fastptr);
	fixup_values(valuefixups, active_update_blitter, updateptr);

#if DEBUG_BLITTERS
	// generate files with the results; use ndisasmw to disassemble them
	{
		FILE *out;

		out = fopen("fast.com", "wb");
		fwrite(active_fast_blitter, 1, fastptr - active_fast_blitter, out);
		fclose(out);

		out = fopen("update.com", "wb");
		fwrite(active_update_blitter, 1, updateptr - active_update_blitter, out);
		fclose(out);
	}
#endif
}
