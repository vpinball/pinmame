/* Video Effect Functions
 *
 * Ben Saylor - bsaylor@macalester.edu
 *
 * Each of these functions copies one line of source pixels, applying an effect.
 * They are called through pointers from blit_core.h.
 * There's one for every bitmap depth / display depth combination,
 * and for direct/non-pallettized where needed.
 *
 *  FIXME: 24-bit, video drivers
 *
 * HISTORY:
 *
 *  2001-10-06:
 *   - minor changes to make -effect option trump -arbheight option <adam@gimp.org>
 *
 *  2001-10-01:
 *   - should now compile with all video drivers,
 *     though effects won't work except with
 *     aforementioned drivers
 *
 *  2001-09-21:
 *   - scan3 effect (deluxe scanlines)
 *   - small fixes & changes
 *
 *  2001-09-19:
 *   - ported to xmame-0.55.1
 *   - fixed some DGA bugs
 *   - confirmed to work with SDL
 *   - added ggi and svgalib support (untested)
 *   - added two basic RGB effects (rgbstripe3x2 and rgbscan2x3)
 *   - removed 8-bit support
 *
 *  2001-09-13:
 *   - added scan2 effect (light scanlines)
 *   - automatically scale as required by effect
 *   - works with x11_window and dga2, and hopefully dga1 and SDL (untested)
 *     (windowed mode is fastest)
 *   - use doublebuffering where requested by video driver
 *   - works on 16-bit and 32-bit displays, others untested
 *     (16-bit recommended - it's faster)
 *   - scale2x smooth scaling effect
 */

#define __EFFECT_C_
#include "xmame.h"
#include "osd_cpu.h"
#include "effect.h"

/* divide R, G, and B to darken pixels */
#define SHADE16_HALF(P)   (((P)>>1) & 0x7bef)
#define SHADE16_FOURTH(P) (((P)>>2) & 0x39e7)
#define SHADE32_HALF(P)   (((P)>>1) & 0x007f7f7f)
#define SHADE32_FOURTH(P) (((P)>>2) & 0x003f3f3f)

/* straight RGB masks */
#define RMASK16(P) ((P) & 0xf800)
#define GMASK16(P) ((P) & 0x07e0)
#define BMASK16(P) ((P) & 0x001f)
#define RMASK32(P) ((P) & 0x00ff0000)
#define GMASK32(P) ((P) & 0x0000ff00)
#define BMASK32(P) ((P) & 0x000000ff)

/* inverse RGB masks */
#define RMASK16_INV(P) ((P) & 0x07ff)
#define GMASK16_INV(P) ((P) & 0xf81f)
#define BMASK16_INV(P) ((P) & 0xffe0)
#define RMASK32_INV(P) ((P) & 0x0000ffff)
#define GMASK32_INV(P) ((P) & 0x00ff00ff)
#define BMASK32_INV(P) ((P) & 0x00ffff00)

/* inverse RGB masks, darkened*/
#define RMASK16_INV_HALF(P) (((P)>>1) & 0x03ef)
#define GMASK16_INV_HALF(P) (((P)>>1) & 0x780f)
#define BMASK16_INV_HALF(P) (((P)>>1) & 0xebe0)
#define RMASK32_INV_HALF(P) (((P)>>1) & 0x00007f7f)
#define GMASK32_INV_HALF(P) (((P)>>1) & 0x007f007f)
#define BMASK32_INV_HALF(P) (((P)>>1) & 0x007f7f00)

/* RGB semi-masks */
#define RMASK16_SEMI(P) ( RMASK16(P) | RMASK16_INV_HALF(P) )
#define GMASK16_SEMI(P) ( GMASK16(P) | GMASK16_INV_HALF(P) )
#define BMASK16_SEMI(P) ( BMASK16(P) | BMASK16_INV_HALF(P) )
#define RMASK32_SEMI(P) ( RMASK32(P) | RMASK32_INV_HALF(P) )
#define GMASK32_SEMI(P) ( GMASK32(P) | GMASK32_INV_HALF(P) )
#define BMASK32_SEMI(P) ( BMASK32(P) | BMASK32_INV_HALF(P) )

/* average two pixels */
#define MEAN16(P,Q) ( RMASK16((RMASK16(P)+RMASK16(Q))/2) | GMASK16((GMASK16(P)+GMASK16(Q))/2) | BMASK16((BMASK16(P)+BMASK16(Q))/2) )
#define MEAN32(P,Q) ( RMASK32((RMASK32(P)+RMASK32(Q))/2) | GMASK32((GMASK32(P)+GMASK32(Q))/2) | BMASK32((BMASK32(P)+BMASK32(Q))/2) )

#ifdef USE_HWSCALE
#define FOURCC_YUY2 0x32595559
#define FOURCC_YV12 0x32315659
#endif

/* called from config.c to set scale parameters */
void effect_init1()
{
        int disable_arbscale = 0;

	switch (effect) {
		case EFFECT_SCALE2X:
		case EFFECT_SCAN2:
			normal_widthscale = 2;
			normal_heightscale = 2;
                        disable_arbscale = 1;
			break;
		case EFFECT_RGBSTRIPE:
			normal_widthscale = 3;
			normal_heightscale = 2;
                        disable_arbscale = 1;
			break;
		case EFFECT_RGBSCAN:
			normal_widthscale = 2;
			normal_heightscale = 3;
                        disable_arbscale = 1;
			break;
		case EFFECT_SCAN3:
			normal_widthscale = 3;
			normal_heightscale = 3;
                        disable_arbscale = 1;
			break;
	}

	if (yarbsize && disable_arbscale) {
	  printf("Using effects -- disabling arbitrary scaling\n");
	  yarbsize = 0;
	}
}

/* called from <driver>_create_display by each video driver;
 * initializes function pointers to correct depths
 * and allocates buffer for doublebuffering */
void effect_init2(int src_depth, int dst_depth, int dst_width)
{
	if (effect) {
		int i,rddepth;

		switch(dst_depth) {
#ifdef USE_HWSCALE
			case FOURCC_YUY2:
			case FOURCC_YV12:
				rddepth=16;
				break;
#endif
			default:
				rddepth=dst_depth;
				break;
		}

		printf("Initializing video effect %d: bitmap depth = %d, display depth = %d\n", effect, src_depth, rddepth);
		effect_dbbuf = malloc(dst_width*normal_heightscale*rddepth/8);
		for (i=0; i<dst_width*normal_heightscale*rddepth/8; i++)
			effect_dbbuf[i] = 0;
		switch (dst_depth) {
			case 15:
			case 16:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_16;
						effect_scale2x_direct_func	= effect_scale2x_16_16_direct;
						effect_scan2_func		= effect_scan2_16_16;
						effect_scan2_direct_func	= effect_scan2_16_16_direct;
						effect_rgbstripe_func		= effect_rgbstripe_16_16;
						effect_rgbstripe_direct_func	= effect_rgbstripe_16_16_direct;
						effect_rgbscan_func		= effect_rgbscan_16_16;
						effect_rgbscan_direct_func	= effect_rgbscan_16_16_direct;
						effect_scan3_func		= effect_scan3_16_16;
						effect_scan3_direct_func	= effect_scan3_16_16_direct;
						break;
					case 32:
						break;
				}
				break;
			case 24:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_24;
						effect_scan2_func		= effect_scan2_16_24;
						effect_rgbstripe_func		= effect_rgbstripe_16_24;
						effect_rgbscan_func		= effect_rgbscan_16_24;
						effect_scan3_func		= effect_scan3_16_24;
						break;
					case 32:
						break;
				}
				break;
			case 32:
				switch (src_depth) {
					case 16:
						effect_scale2x_func		= effect_scale2x_16_32;
						effect_scan2_func		= effect_scan2_16_32;
						effect_rgbstripe_func		= effect_rgbstripe_16_32;
						effect_rgbscan_func		= effect_rgbscan_16_32;
						effect_scan3_func		= effect_scan3_16_32;
						break;
					case 32:
						effect_scale2x_direct_func	= effect_scale2x_32_32_direct;
						effect_scan2_direct_func	= effect_scan2_32_32_direct;
						effect_rgbstripe_direct_func	= effect_rgbstripe_32_32_direct;
						effect_rgbscan_direct_func	= effect_rgbscan_32_32_direct;
						effect_scan3_direct_func	= effect_scan3_32_32_direct;
						break;
				}
				break;
#ifdef USE_HWSCALE
			case FOURCC_YUY2:
				switch(src_depth) {
					case 16:
						effect_scale2x_func = effect_scale2x_16_YUY2;
						effect_scan2_func		= effect_scan2_16_YUY2;
						effect_rgbstripe_func		= effect_rgbstripe_16_YUY2;
						effect_rgbscan_func		= effect_rgbscan_16_YUY2;
						effect_scan3_func		= effect_scan3_16_YUY2;
						break;
					case 32:
						effect_scale2x_direct_func = effect_scale2x_32_YUY2_direct;
						effect_scan2_direct_func	= effect_scan2_32_YUY2_direct;
						effect_rgbstripe_direct_func	= effect_rgbstripe_32_YUY2_direct;
						effect_rgbscan_direct_func	= effect_rgbscan_32_YUY2_direct;
						effect_scan3_direct_func	= effect_scan3_32_YUY2_direct;
						break;
       				}
				break;
#endif
		}
	}

	if (!blit_hardware_rotation && (blit_flipx || blit_flipy || blit_swapxy))
	{
		switch (src_depth) {
		case 16:
			rotate_func = rotate_16_16;
			break;
		case 32:
			rotate_func = rotate_32_32;
			break;
		}

		if (effect == EFFECT_SCALE2X) {
			rotate_dbbuf0 = calloc(video_width*video_depth/8, sizeof(char));
			rotate_dbbuf1 = calloc(video_width*video_depth/8, sizeof(char));
			rotate_dbbuf2 = calloc(video_width*video_depth/8, sizeof(char));
		} else {
			rotate_dbbuf = calloc(video_width*video_depth/8, sizeof(char));
		}
	}
}


/* scale2x algorithm (Andrea Mazzoleni, http://advancemame.sourceforge.net):
 *
 * A 9-pixel rectangle is taken from the source bitmap:
 *
 *  a b c
 *  d e f
 *  g h i
 *
 * The central pixel e is expanded into four new pixels,
 *
 *  e0 e1
 *  e2 e3
 *
 * where
 *
 *  e0 = (d == b && b != f && d != h) ? d : e;
 *  e1 = (b == f && b != d && f != h) ? f : e;
 *  e2 = (d == h && d != b && h != f) ? d : e;
 *  e3 = (h == f && d != h && b != f) ? f : e;
 *
 */

void effect_scale2x_16_16
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src0 = (UINT16 *)src0;
	UINT16 *u16src1 = (UINT16 *)src1;
	UINT16 *u16src2 = (UINT16 *)src2;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
			*u16dst0 = u32lookup[u16src0[0]];
		else	*u16dst0 = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
			*(u16dst0+1) = u32lookup[u16src0[0]];
		else	*(u16dst0+1) = u32lookup[u16src1[0]];

		if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
			*u16dst1 = u32lookup[u16src2[0]];
		else	*u16dst1 = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
			*(u16dst1+1) = u32lookup[u16src2[0]];
		else	*(u16dst1+1) = u32lookup[u16src1[0]];

		++u16src0;
		++u16src1;
		++u16src2;
		u16dst0 += 2;
		u16dst1 += 2;
		--count;
	}
}

void effect_scale2x_16_16_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src0 = (UINT16 *)src0;
	UINT16 *u16src1 = (UINT16 *)src1;
	UINT16 *u16src2 = (UINT16 *)src2;

	while (count) {

		if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
			*u16dst0 = u16src0[0];
		else	*u16dst0 = u16src1[0];

		if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
			*(u16dst0+1) = u16src0[0];
		else	*(u16dst0+1) = u16src1[0];

		if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
			*u16dst1 = u16src2[0];
		else	*u16dst1 = u16src1[0];

		if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
			*(u16dst1+1) = u16src2[0];
		else	*(u16dst1+1) = u16src1[0];

		++u16src0;
		++u16src1;
		++u16src2;
		u16dst0 += 2;
		u16dst1 += 2;
		--count;
	}
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff
void effect_scale2x_16_YUY2
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	unsigned int *u32dst0 = (unsigned int *)dst0;
	unsigned int *u32dst1 = (unsigned int *)dst1;
	UINT16 *u16src0 = (UINT16 *)src0;
	UINT16 *u16src1 = (UINT16 *)src1;
	UINT16 *u16src2 = (UINT16 *)src2;
	UINT32 *u32lookup = (UINT32 *)lookup;
	INT32 y,y2,uv1,uv2;
	UINT32 p1,p2,p3,p4;
	while (count) {

		if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
			p1 = u32lookup[u16src0[0]];
		else	p1 = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
			p2 = u32lookup[u16src0[0]];
		else	p2 = u32lookup[u16src1[0]];

		if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
			p3 = u32lookup[u16src2[0]];
		else	p3 = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
			p4 = u32lookup[u16src2[0]];
		else	p4 = u32lookup[u16src1[0]];

		++u16src0;
		++u16src1;
		++u16src2;

		y=p1&0x000000ff;
		uv1=(p1&0xff00ff00)>>1;
		y2=p2&0x00ff0000;
		uv2=(p2&0xff00ff00)>>1;
		uv1=(uv1+uv2)&0xff00ff00;
		*u32dst0++=y|y2|uv1;

		y=p3&0x000000ff;
		uv1=(p3&0xff00ff00)>>1;
		y2=p4&0x00ff0000;
		uv2=(p4&0xff00ff00)>>1;
		uv1=(uv1+uv2)&0xff00ff00;
		*u32dst1++=y|y2|uv1;		y=p1>>24;

		--count;
	}
}

void effect_scale2x_32_YUY2_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count)
{
	unsigned char *u8dst0 = (unsigned char *)dst0;
	unsigned char *u8dst1 = (unsigned char *)dst1;
	UINT32 *u32src0 = (UINT32 *)src0;
	UINT32 *u32src1 = (UINT32 *)src1;
	UINT32 *u32src2 = (UINT32 *)src2;
	INT32 r,g,b,r2,g2,b2,y,y2,u,v;
	UINT32 p1,p2,p3,p4;
	while (count) {

		if (u32src1[-1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[1] != u32src0[0])
			p1 = u32src0[0];
		else	p1 = u32src1[0];

		if (u32src1[1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[-1] != u32src0[0])
			p2 = u32src0[0];
		else	p2 = u32src1[0];

		if (u32src1[-1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[1] != u32src2[0])
			p3 = u32src2[0];
		else	p3 = u32src1[0];

		if (u32src1[1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[-1] != u32src2[0])
			p4 = u32src2[0];
		else	p4 = u32src1[0];

		++u32src0;
		++u32src1;
		++u32src2;

    r=p1&RMASK;  r>>=16; \
    g=p1&GMASK;  g>>=8; \
    b=p1&BMASK;  b>>=0; \

    r2=p2&RMASK;  r2>>=16; \
    g2=p2&GMASK;  g2>>=8; \
    b2=p2&BMASK;  b2>>=0; \

    y = (( 9897*r + 19235*g + 3736*b ) >> 15);
    y2 = (( 9897*r2 + 19235*g2 + 3736*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2; \
    *u8dst0++=y;
    u = (( -5537*r - 10878*g + 16384*b ) >> 16) + 128; \
    *u8dst0++=u;
    v = (( 16384*r - 13730*g - 2664*b ) >> 16) + 128; \
    *u8dst0++=y2;
    *u8dst0++=v;

    r=p3&RMASK;  r>>=16; \
    g=p3&GMASK;  g>>=8; \
    b=p3&BMASK;  b>>=0; \

    r2=p4&RMASK;  r2>>=16; \
    g2=p4&GMASK;  g2>>=8; \
    b2=p4&BMASK;  b2>>=0; \

    y = (( 9897*r + 19235*g + 3736*b ) >> 15);
    y2 = (( 9897*r2 + 19235*g2 + 3736*b2 ) >> 15);
    r+=r2; g+=g2; b+=b2; \
    *u8dst1++=y;
    u = (( -5537*r - 10878*g + 16384*b ) >> 16) + 128; \
    *u8dst1++=u;
    v = (( 16384*r - 13730*g - 2664*b ) >> 16) + 128; \
    *u8dst1++=y2;
    *u8dst1++=v;

		--count;
	}
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/* FIXME: this probably doesn't work right for 24 bit */
void effect_scale2x_16_24
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src0 = (UINT16 *)src0;
	UINT16 *u16src1 = (UINT16 *)src1;
	UINT16 *u16src2 = (UINT16 *)src2;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
			*(u32dst0) = u32lookup[u16src0[0]];
		else	*(u32dst0) = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
			*(u32dst0+1) = u32lookup[u16src0[0]];
		else	*(u32dst0+1) = u32lookup[u16src1[0]];

		if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
			*(u32dst1) = u32lookup[u16src2[0]];
		else	*(u32dst1) = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
			*(u32dst1+1) = u32lookup[u16src2[0]];
		else	*(u32dst1+1) = u32lookup[u16src1[0]];

		++u16src0;
		++u16src1;
		++u16src2;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}

void effect_scale2x_16_32
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src0 = (UINT16 *)src0;
	UINT16 *u16src1 = (UINT16 *)src1;
	UINT16 *u16src2 = (UINT16 *)src2;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		if (u16src1[-1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[1] != u16src0[0])
			*(u32dst0) = u32lookup[u16src0[0]];
		else	*(u32dst0) = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src0[0] && u16src2[0] != u16src0[0] && u16src1[-1] != u16src0[0])
			*(u32dst0+1) = u32lookup[u16src0[0]];
		else	*(u32dst0+1) = u32lookup[u16src1[0]];

		if (u16src1[-1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[1] != u16src2[0])
			*(u32dst1) = u32lookup[u16src2[0]];
		else	*(u32dst1) = u32lookup[u16src1[0]];

		if (u16src1[1] == u16src2[0] && u16src0[0] != u16src2[0] && u16src1[-1] != u16src2[0])
			*(u32dst1+1) = u32lookup[u16src2[0]];
		else	*(u32dst1+1) = u32lookup[u16src1[0]];

		++u16src0;
		++u16src1;
		++u16src2;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}

void effect_scale2x_32_32_direct
		(void *dst0, void *dst1,
		const void *src0, const void *src1, const void *src2,
		unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32src0 = (UINT32 *)src0;
	UINT32 *u32src1 = (UINT32 *)src1;
	UINT32 *u32src2 = (UINT32 *)src2;

	while (count) {

		if (u32src1[-1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[1] != u32src0[0])
			*u32dst0 = u32src0[0];
		else	*u32dst0 = u32src1[0];

		if (u32src1[1] == u32src0[0] && u32src2[0] != u32src0[0] && u32src1[-1] != u32src0[0])
			*(u32dst0+1) = u32src0[0];
		else	*(u32dst0+1) = u32src1[0];

		if (u32src1[-1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[1] != u32src2[0])
			*u32dst1 = u32src2[0];
		else	*u32dst1 = u32src1[0];

		if (u32src1[1] == u32src2[0] && u32src0[0] != u32src2[0] && u32src1[-1] != u32src2[0])
			*(u32dst1+1) = u32src2[0];
		else	*(u32dst1+1) = u32src1[0];

		++u32src0;
		++u32src1;
		++u32src2;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}


/**********************************
 * scan2: light 2x2 scanlines
 **********************************/

void effect_scan2_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*u16dst0 = *(u16dst0+1) = u32lookup[*u16src];
			 
		*u16dst1 = *(u16dst1+1) = SHADE16_HALF( u32lookup[*u16src] ) + SHADE16_FOURTH( u32lookup[*u16src] );

		++u16src;
		u16dst0 += 2;
		u16dst1 += 2;
		--count;
	}
}

void effect_scan2_16_16_direct (void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src = (UINT16 *)src;

	while (count) {

		*u16dst0 = *(u16dst0+1) = *u16src;
			 
		*u16dst1 = *(u16dst1+1) = SHADE16_HALF( *u16src ) + SHADE16_FOURTH( *u16src );

		++u16src;
		u16dst0 += 2;
		u16dst1 += 2;
		--count;
	}
}

void effect_scan2_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*u32dst0 = *(u32dst0+1) = u32lookup[*u16src];
			 
		*u32dst1 = *(u32dst1+1) = SHADE32_HALF( u32lookup[*u16src] ) + SHADE32_FOURTH( u32lookup[*u16src] );

		++u16src;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}

void effect_scan2_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*u32dst0 = *(u32dst0+1) = u32lookup[*u16src];
			 
		*u32dst1 = *(u32dst1+1) = SHADE32_HALF( u32lookup[*u16src] ) + SHADE32_FOURTH( u32lookup[*u16src] );

		++u16src;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}

void effect_scan2_32_32_direct (void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32src = (UINT32 *)src;

	while (count) {

		*u32dst0 = *(u32dst0+1) = *u32src;
			 
		*u32dst1 = *(u32dst1+1) = SHADE32_HALF( *u32src ) +  SHADE32_FOURTH( *u32src );

		++u32src;
		u32dst0 += 2;
		u32dst1 += 2;
		--count;
	}
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff
void effect_scan2_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;
	UINT32 r,y,u,v;

	while (count) {
		r=u32lookup[*u16src];
		y=r&255;
		u=(r>>8)&255;
		v=(r>>24);
		*u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		*u32dst1 = ((y*3/4)&255)|((u&255)<<8)|(((y*3/4)&255)<<16)|((v&255)<<24);
		++u16src;
		u32dst0++;
		u32dst1++;
		--count;
	}
}

void effect_scan2_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32src = (UINT32 *)src;
	UINT32 r,g,b,y,u,v;

	while (count) {
		r=*u32src&RMASK;  r>>=16;
		g=*u32src&GMASK;  g>>=8;
		b=*u32src&BMASK;  b>>=0;
		y = (( 9897*r + 19235*g + 3736*b ) >> 15);
		u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
		v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;

		*u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		*u32dst1 = ((y*3/4)&255)|((u&255)<<8)|(((y*3/4)&255)<<16)|((v&255)<<24);
		++u32src;
		u32dst0++;
		u32dst1++;
		--count;
	}
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif

/**********************************
 * rgbstripe
 **********************************/

void effect_rgbstripe_16_16 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u16dst0+0) = *(u16dst1+0) = RMASK16_SEMI(u32lookup[*u16src]);
		*(u16dst0+1) = *(u16dst1+1) = GMASK16_SEMI(u32lookup[*u16src]);
		*(u16dst0+2) = *(u16dst1+2) = BMASK16_SEMI(u32lookup[*u16src]);

		++u16src;
		u16dst0 += 3;
		u16dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_16_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16src = (UINT16 *)src;

	while (count) {

		*(u16dst0+0) = *(u16dst1+0) = RMASK16_SEMI(*u16src);
		*(u16dst0+1) = *(u16dst1+1) = GMASK16_SEMI(*u16src);
		*(u16dst0+2) = *(u16dst1+2) = BMASK16_SEMI(*u16src);

		++u16src;
		u16dst0 += 3;
		u16dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_24 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+0) = *(u32dst1+0) = RMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst0+1) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst0+2) = *(u32dst1+2) = BMASK32_SEMI(u32lookup[*u16src]);

		++u16src;
		u32dst0 += 3;
		u32dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_16_32 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+0) = *(u32dst1+0) = RMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst0+1) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst0+2) = *(u32dst1+2) = BMASK32_SEMI(u32lookup[*u16src]);

		++u16src;
		u32dst0 += 3;
		u32dst1 += 3;
		--count;
	}
}

void effect_rgbstripe_32_32_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32src = (UINT32 *)src;

	while (count) {

		*(u32dst0+0) = *(u32dst1+0) = RMASK32_SEMI(*u32src);
		*(u32dst0+1) = *(u32dst1+1) = GMASK32_SEMI(*u32src);
		*(u32dst0+2) = *(u32dst1+2) = BMASK32_SEMI(*u32src);

		++u32src;
		u32dst0 += 3;
		u32dst1 += 3;
		--count;
	}
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff

void effect_rgbstripe_16_YUY2 (void *dst0, void *dst1, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;
	UINT32 r,g,b,y,u,v,y2,u2,v2,s;
	INT32 us,vs;
	UINT16 t, t2;

	s = 1;
	while (count) {
		if (s) {
			r = u32lookup[*u16src];
			y = r&255;
			u = (r>>8)&255;
			v = (r>>24);
			us = u - 128;
			vs = v - 128;
			r = ((512*y + 718*vs) >> 9);
			g = ((512*y - 176*us - 366*vs) >> 9);
			b = ((512*y + 907*us) >> 9);
			y = (( 9897*r + 19235*(g&0x7f) + 3736*(b&0x7f) ) >> 15);
			u = (( -5537*r - 10878*(g&0x7f) + 16384*(b&0x7f) ) >> 15) + 128;
			v = (( 16384*r - 13730*(g&0x7f) - 2664*(b&0x7f) ) >> 15) + 128;
			y2 = (( 9897*(r&0x7f) + 19235*g + 3736*(b&0x7f) ) >> 15);
			u2 = (( -5537*(r&0x7f) - 10878*g + 16384*(b&0x7f) ) >> 15) + 128;
			v2 = (( 16384*(r&0x7f) - 13730*g - 2664*(b&0x7f) ) >> 15) + 128;
			u = (((u&255)+(u2&255))>>1);
			v = (((v&255)+(v2&255))>>1);
			*u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			if (count != 1) {
				y = (( 9897*(r&0x7f) + 19235*(g&0x7f) + 3736*b ) >> 15);
				u = (( -5537*(r&0x7f) - 10878*(g&0x7f) + 16384*b ) >> 15) + 128;
				v = (( 16384*(r&0x7f) - 13730*(g&0x7f) - 2664*b ) >> 15) + 128;
				r = u32lookup[*(u16src+1)];
				y2 = r&255;
				u2 = (r>>8)&255;
				v2 = (r>>24);
				us = u2 - 128;
				vs = v2 - 128;
				r = ((512*y2 + 718*vs) >> 9);
				g = ((512*y2 - 176*us - 366*vs) >> 9);
				b = ((512*y2 + 907*us) >> 9);
				y2 = (( 9897*r + 19235*(g&0x7f) + 3736*(b&0x7f) ) >> 15);
				u2 = (( -5537*r - 10878*(g&0x7f) + 16384*(b&0x7f) ) >> 15) + 128;
				v2 = (( 16384*r - 13730*(g&0x7f) - 2664*(b&0x7f) ) >> 15) + 128;
				u = (((u&255)+(u2&255))>>1);
				v = (((v&255)+(v2&255))>>1);
				*(u32dst0+1) = *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			}
		} else {
			r = u32lookup[*u16src];
			y = r&255;
			u = (r>>8)&255;
			v = (r>>24);
			us = u - 128;
			vs = v - 128;
			r = ((512*y + 718*vs) >> 9);
			g = ((512*y - 176*us - 366*vs) >> 9);
			b = ((512*y + 907*us) >> 9);
			y = (( 9897*(r&0x7f) + 19235*g + 3736*(b&0x7f) ) >> 15);
			u = (( -5537*(r&0x7f) - 10878*g + 16384*(b&0x7f) ) >> 15) + 128;
			v = (( 16384*(r&0x7f) - 13730*g - 2664*(b&0x7f) ) >> 15) + 128;
			y2 = (( 9897*(r&0x7f) + 19235*(g&0x7f) + 3736*b ) >> 15);
			u2 = (( -5537*(r&0x7f) - 10878*(g&0x7f) + 16384*b ) >> 15) + 128;
			v2 = (( 16384*(r&0x7f) - 13730*(g&0x7f) - 2664*b ) >> 15) + 128;
			u = (((u&255)+(u2&255))>>1);
			v = (((v&255)+(v2&255))>>1);
			*u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
		}

		++u16src;
		if (s) {
			u32dst0 += 2;
			u32dst1 += 2;
		} else {
			u32dst0++;
			u32dst1++;
		}
		--count;
		s = !s;
	}
}

void effect_rgbstripe_32_YUY2_direct(void *dst0, void *dst1, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32src = (UINT32 *)src;
	UINT32 t,r,g,b,y,u,v,y2,s;

	s = 1;
	while (count) {
		if (s) {
			t = RMASK32_SEMI(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			t = GMASK32_SEMI(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
			v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
			*u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			if (count != 1) {
				t = BMASK32_SEMI(*u32src);
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
				v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
				t = BMASK32_SEMI(*(u32src+1));
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
				v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
				*(u32dst0+1) = *(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			}
		} else {
			t = GMASK32_SEMI(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			t = BMASK32_SEMI(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
			v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
			*u32dst0 = *u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
		}

		++u32src;
		if (s) {
			u32dst0 += 2;
			u32dst1 += 2;
		} else {
			u32dst0++;
			u32dst1++;
		}
		--count;
		s = !s;
	}
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/**********************************
 * rgbscan
 **********************************/

void effect_rgbscan_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16dst2 = (UINT16 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u16dst0+0) = *(u16dst0+1) = RMASK16_SEMI(u32lookup[*u16src]);
		*(u16dst1+0) = *(u16dst1+1) = GMASK16_SEMI(u32lookup[*u16src]);
		*(u16dst2+0) = *(u16dst2+1) = BMASK16_SEMI(u32lookup[*u16src]);

		++u16src;
		u16dst0 += 2;
		u16dst1 += 2;
		u16dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16dst2 = (UINT16 *)dst2;
	UINT16 *u16src = (UINT16 *)src;

	while (count) {

		*(u16dst0+0) = *(u16dst0+1) = RMASK16_SEMI(*u16src);
		*(u16dst1+0) = *(u16dst1+1) = GMASK16_SEMI(*u16src);
		*(u16dst2+0) = *(u16dst2+1) = BMASK16_SEMI(*u16src);

		++u16src;
		u16dst0 += 2;
		u16dst1 += 2;
		u16dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(u32lookup[*u16src]);

		++u16src;
		u32dst0 += 2;
		u32dst1 += 2;
		u32dst2 += 2;
		--count;
	}
}

void effect_rgbscan_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(u32lookup[*u16src]);
		*(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(u32lookup[*u16src]);

		++u16src;
		u32dst0 += 2;
		u32dst1 += 2;
		u32dst2 += 2;
		--count;
	}
}

void effect_rgbscan_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT32 *u32src = (UINT32 *)src;

	while (count) {

		*(u32dst0+0) = *(u32dst0+1) = RMASK32_SEMI(*u32src);
		*(u32dst1+0) = *(u32dst1+1) = GMASK32_SEMI(*u32src);
		*(u32dst2+0) = *(u32dst2+1) = BMASK32_SEMI(*u32src);

		++u32src;
		u32dst0 += 2;
		u32dst1 += 2;
		u32dst2 += 2;
		--count;
	}
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff

void effect_rgbscan_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;
	UINT32 r,g,b,y,u,v;
	INT32 us,vs;

	while (count) {
		r = u32lookup[*u16src];
		y = r&255;
		u = (r>>8)&255;
		v = (r>>24);
		us = u - 128;
		vs = v - 128;
		r = ((512*y + 718*vs) >> 9);
		g = ((512*y - 176*us - 366*vs) >> 9);
		b = ((512*y + 907*us) >> 9);
		y = (( 9897*r + 19235*(g&0x7f) + 3736*(b&0x7f) ) >> 15);
		u = (( -5537*r - 10878*(g&0x7f) + 16384*(b&0x7f) ) >> 15) + 128;
		v = (( 16384*r - 13730*(g&0x7f) - 2664*(b&0x7f) ) >> 15) + 128;
		*u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		y = (( 9897*(r&0x7f) + 19235*g + 3736*(b&0x7f) ) >> 15);
		u = (( -5537*(r&0x7f) - 10878*g + 16384*(b&0x7f) ) >> 15) + 128;
		v = (( 16384*(r&0x7f) - 13730*g - 2664*(b&0x7f) ) >> 15) + 128;
		*u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		y = (( 9897*(r&0x7f) + 19235*(g&0x7f) + 3736*b ) >> 15);
		u = (( -5537*(r&0x7f) - 10878*(g&0x7f) + 16384*b ) >> 15) + 128;
		v = (( 16384*(r&0x7f) - 13730*(g&0x7f) - 2664*b ) >> 15) + 128;
		*u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		++u16src;
		u32dst0++;
		u32dst1++;
		u32dst2++;
		--count;
	}
}

void effect_rgbscan_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT32 *u32src = (UINT32 *)src;
	UINT32 r,g,b,y,u,v;

	while (count) {
		r = RMASK32_SEMI(*u32src)&RMASK; r>>=16;
		g = RMASK32_SEMI(*u32src)&GMASK; g>>=8;
		b = RMASK32_SEMI(*u32src)&BMASK; b>>=0;
		y = (( 9897*r + 19235*g + 3736*b ) >> 15);
		u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
		v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
		*u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		r = GMASK32_SEMI(*u32src)&RMASK; r>>=16;
		g = GMASK32_SEMI(*u32src)&GMASK; g>>=8;
		b = GMASK32_SEMI(*u32src)&BMASK; b>>=0;
		y = (( 9897*r + 19235*g + 3736*b ) >> 15);
		u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
		v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
		*u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
		r = BMASK32_SEMI(*u32src)&RMASK; r>>=16;
		g = BMASK32_SEMI(*u32src)&GMASK; g>>=8;
		b = BMASK32_SEMI(*u32src)&BMASK; b>>=0;
		y = (( 9897*r + 19235*g + 3736*b ) >> 15);
		u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
		v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
		*u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);

		++u32src;
		u32dst0++;
		u32dst1++;
		u32dst2++;
		--count;
	}
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/**********************************
 * scan3
 **********************************/

/* All 3 lines are horizontally blurred a little
 * (the last pixel of each three in a line is averaged with the next pixel).
 * The first line is darkened by 25%,
 * the second line is full brightness, and
 * the third line is darkened by 50%.
 */

void effect_scan3_16_16 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16dst2 = (UINT16 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u16dst0+1) = *(u16dst0+0) =
			SHADE16_HALF(u32lookup[*u16src]) + SHADE16_FOURTH(u32lookup[*u16src]);
		*(u16dst0+2) =
			SHADE16_HALF( MEAN16( u32lookup[*u16src], u32lookup[*u16src+1] ) )
			+
			SHADE16_FOURTH( MEAN16( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		*(u16dst1+0) = *(u16dst1+1) = u32lookup[*u16src];
		*(u16dst1+2) = MEAN16( u32lookup[*u16src], u32lookup[*u16src+1] );

		*(u16dst2+0) = *(u16dst2+1) =
			SHADE16_HALF(u32lookup[*u16src]);
		*(u16dst2+2) =
			SHADE16_HALF( MEAN16( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		++u16src;
		u16dst0 += 3;
		u16dst1 += 3;
		u16dst2 += 3;
		--count;
	}
}

void effect_scan3_16_16_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT16 *u16dst0 = (UINT16 *)dst0;
	UINT16 *u16dst1 = (UINT16 *)dst1;
	UINT16 *u16dst2 = (UINT16 *)dst2;
	UINT16 *u16src = (UINT16 *)src;

	while (count) {

		*(u16dst0+1) = *(u16dst0+0) =
			SHADE16_HALF(*u16src) + SHADE16_FOURTH(*u16src);
		*(u16dst0+2) =
			SHADE16_HALF( MEAN16( *u16src, *u16src+1 ) )
			+
			SHADE16_FOURTH( MEAN16( *u16src, *u16src+1 ) );

		*(u16dst1+0) = *(u16dst1+1) = *u16src;
		*(u16dst1+2) = MEAN16( *u16src, *u16src+1 );

		*(u16dst2+0) = *(u16dst2+1) =
			SHADE16_HALF(*u16src);
		*(u16dst2+2) =
			SHADE16_HALF( MEAN16( *u16src, *u16src+1 ) );

		++u16src;
		u16dst0 += 3;
		u16dst1 += 3;
		u16dst2 += 3;
		--count;
	}
}

void effect_scan3_16_24 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+1) = *(u32dst0+0) =
			SHADE32_HALF(u32lookup[*u16src]) + SHADE32_FOURTH(u32lookup[*u16src]);
		*(u32dst0+2) =
			SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) )
			+
			SHADE32_FOURTH( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		*(u32dst1+0) = *(u32dst1+1) = u32lookup[*u16src];
		*(u32dst1+2) = MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] );

		*(u32dst2+0) = *(u32dst2+1) =
			SHADE32_HALF(u32lookup[*u16src]);
		*(u32dst2+2) =
			SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		++u16src;
		u32dst0 += 3;
		u32dst1 += 3;
		u32dst2 += 3;
		--count;
	}
}

void effect_scan3_16_32 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;

	while (count) {

		*(u32dst0+1) = *(u32dst0+0) =
			SHADE32_HALF(u32lookup[*u16src]) + SHADE32_FOURTH(u32lookup[*u16src]);
		*(u32dst0+2) =
			SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) )
			+
			SHADE32_FOURTH( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		*(u32dst1+0) = *(u32dst1+1) = u32lookup[*u16src];
		*(u32dst1+2) = MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] );

		*(u32dst2+0) = *(u32dst2+1) =
			SHADE32_HALF(u32lookup[*u16src]);
		*(u32dst2+2) =
			SHADE32_HALF( MEAN32( u32lookup[*u16src], u32lookup[*u16src+1] ) );

		++u16src;
		u32dst0 += 3;
		u32dst1 += 3;
		u32dst2 += 3;
		--count;
	}
}

void effect_scan3_32_32_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT32 *u32src = (UINT32 *)src;

	while (count) {

		*(u32dst0+1) = *(u32dst0+0) =
			SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
		*(u32dst0+2) =
			SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
			+
			SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );

		*(u32dst1+0) = *(u32dst1+1) = *u32src;
		*(u32dst1+2) = MEAN32( *u32src, *(u32src+1) );

		*(u32dst2+0) = *(u32dst2+1) =
			SHADE32_HALF(*u32src);
		*(u32dst2+2) =
			SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );

		++u32src;
		u32dst0 += 3;
		u32dst1 += 3;
		u32dst2 += 3;
		--count;
	}
}

#ifdef USE_HWSCALE
#define RMASK 0xff0000
#define GMASK 0xff00
#define BMASK 0xff

void effect_scan3_16_YUY2 (void *dst0, void *dst1, void *dst2, const void *src, unsigned count, const void *lookup)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT16 *u16src = (UINT16 *)src;
	UINT32 *u32lookup = (UINT32 *)lookup;
	UINT32 p1,p2,y1,uv1,uv2,y2,s;

	s = 1;
	while (count) {
		if (s) {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			*u32dst0 = ((y1*3/4)&255)|(((y1*3/4)&255)<<16)|uv1;
			if (count != 1) {
				p2 = u32lookup[*(u16src+1)];
				y2 = p2&255;
				y1 = ((y1>>1) + (y2>>1))&0x000000ff;
				uv2 = p2&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				*(u32dst0+1) = ((y1*3/4)&255)|(((y2*3/4)&255)<<16)|uv1;
			}
		} else {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			p2 = u32lookup[*(u16src+1)];
			y2 = p2&255;
			y2 = ((y1>>1) + (y2>>1))&0x000000ff;
			uv2 = p2&0xff00ff00;
			uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			*u32dst0 = ((y1*3/4)&255)|(((y2*3/4)&255)<<16)|uv1;
		}
		if (s) {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			*u32dst1 = (y1&255)|((y1&255)<<16)|uv1;
			if (count != 1) {
#if 0
				p2 = u32lookup[*(u16src+1)];
				y2 = p2&255;
				y1 = ((y1>>1) + (y2>>1))&0x000000ff;
				uv2 = p2&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				*(u32dst1+1) = (y1&255)|((y2&255)<<16)|uv1;
#endif
			}
		} else {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			p2 = u32lookup[*(u16src+1)];
			y2 = p2&255;
			y2 = ((y1>>1) + (y2>>1))&0x000000ff;
			uv2 = p2&0xff00ff00;
			uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			*u32dst1 = (y1&255)|((y2&255)<<16)|uv1;
		}
		if (s) {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			*u32dst2 = ((y1/2)&255)|(((y1/2)&255)<<16)|uv1;
			if (count != 1) {
#if 0
				p2 = u32lookup[*(u16src+1)];
				y2 = p2&255;
				y1 = ((y1>>1) + (y2>>1))&0x000000ff;
				uv2 = p2&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
				*(u32dst2+1) = ((y1/2)&255)|(((y2/2)&255)<<16)|uv1;
#endif
			}
		} else {
			p1 = u32lookup[*u16src];
			y1 = p1&255;
			uv1 = p1&0xff00ff00;
			p2 = u32lookup[*(u16src+1)];
			y2 = p2&255;
			y2 = ((y1>>1) + (y2>>1))&0x000000ff;
			uv2 = p2&0xff00ff00;
			uv2 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			uv1 = ((uv1>>1)+(uv2>>1))&0xff00ff00;
			*u32dst2 = ((y1/2)&255)|(((y2/2)&255)<<16)|uv1;
		}

		++u16src;
		if (s) {
			u32dst0 += 2;
			u32dst1 += 2;
			u32dst2 += 2;
		} else {
			u32dst0++;
			u32dst1++;
			u32dst2++;
		}
		--count;
		s = !s;
	}
}

void effect_scan3_32_YUY2_direct(void *dst0, void *dst1, void *dst2, const void *src, unsigned count)
{
	UINT32 *u32dst0 = (UINT32 *)dst0;
	UINT32 *u32dst1 = (UINT32 *)dst1;
	UINT32 *u32dst2 = (UINT32 *)dst2;
	UINT32 *u32src = (UINT32 *)src;
	UINT32 t,r,g,b,y,u,v,y2,s;

	s = 1;
	while (count) {
		if (s) {
			t = SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			*u32dst0 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
			if (count != 1) {
				t =
					SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
					+
					SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
				v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
				t = SHADE32_HALF(*(u32src+1)) + SHADE32_FOURTH(*(u32src+1));
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
				v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
				*(u32dst0+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			}
		} else {
			t = SHADE32_HALF(*u32src) + SHADE32_FOURTH(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			t =
				SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) )
				+
				SHADE32_FOURTH( MEAN32( *u32src, *(u32src+1) ) );
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
			v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
			*u32dst0 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
		}
		if (s) {
			r = *u32src&RMASK; r>>=16;
			g = *u32src&GMASK; g>>=8;
			b = *u32src&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			*u32dst1 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
			if (count != 1) {
				t = MEAN32( *u32src, *(u32src+1) );
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
				v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
				r = *(u32src+1)&RMASK; r>>=16;
				g = *(u32src+1)&GMASK; g>>=8;
				b = *(u32src+1)&BMASK; b>>=0;
				y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
				v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
				*(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			}
		} else {
			r = *u32src&RMASK; r>>=16;
			g = *u32src&GMASK; g>>=8;
			b = *u32src&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			t = MEAN32( *u32src, *(u32src+1) );
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
			v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
			*u32dst1 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
		}
		if (s) {
			t = SHADE32_HALF(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			*u32dst2 = (y&255)|((u&255)<<8)|((y&255)<<16)|((v&255)<<24);
			if (count != 1) {
				t = SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
				v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
				t = SHADE32_HALF(*(u32src+1));
				r = t&RMASK; r>>=16;
				g = t&GMASK; g>>=8;
				b = t&BMASK; b>>=0;
				y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
				u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
				v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
				*(u32dst1+1) = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
			}
		} else {
			t = SHADE32_HALF(*u32src);
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (( -5537*r - 10878*g + 16384*b ) >> 15) + 128;
			v = (( 16384*r - 13730*g - 2664*b ) >> 15) + 128;
			t = SHADE32_HALF( MEAN32( *u32src, *(u32src+1) ) );
			r = t&RMASK; r>>=16;
			g = t&GMASK; g>>=8;
			b = t&BMASK; b>>=0;
			y2 = (( 9897*r + 19235*g + 3736*b ) >> 15);
			u = (u + (( -5537*r - 10878*g + 16384*b ) >> 15) + 128)/2;
			v = (v + (( 16384*r - 13730*g - 2664*b ) >> 15) + 128)/2;
			*u32dst2 = (y&255)|((u&255)<<8)|((y2&255)<<16)|((v&255)<<24);
		}
		++u32src;
		if (s) {
			u32dst0 += 2;
			u32dst1 += 2;
			u32dst2 += 2;
		} else {
			u32dst0++;
			u32dst1++;
			u32dst2++;
		}
		--count;
		s = !s;
	}
}

#undef RMASK
#undef GMASK
#undef BMASK
#endif


/**********************************
 * rotate
 **********************************/

void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y)
{
	int x;
	UINT16 * u16dst = (UINT16 *)dst;

	if (blit_swapxy) {
		if (blit_flipx && blit_flipy)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
		else if (blit_flipx)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - x - 1])[y];
		else if (blit_flipy)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[x])[bitmap->width - y - 1];
		else
			for (x = visual.min_x; x <= visual.max_x; x++)
				u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[x])[y];
	} else if (blit_flipx && blit_flipy)
		for (x = visual.min_x; x <= visual.max_x; x++)
			u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if (blit_flipx)
	       for (x = visual.min_x; x <= visual.max_x; x++)
		       u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[y])[bitmap->width - x - 1];
       else if (blit_flipy)
	       for (x = visual.min_x; x <= visual.max_x; x++)
		       u16dst[x-visual.min_x] = ((UINT16 *)bitmap->line[bitmap->height - y -1])[x];
}

void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y)
{
	int x;
	UINT32 * u32dst = (UINT32 *)dst;

	if (blit_swapxy) {
		if (blit_flipx && blit_flipy)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
		else if (blit_flipx)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - x - 1])[y];
		else if (blit_flipy)
			for (x = visual.min_x; x <= visual.max_x; x++)
				u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[x])[bitmap->width - y - 1];
		else
			for (x = visual.min_x; x <= visual.max_x; x++)
				u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[x])[y];
	} else if (blit_flipx && blit_flipy)
		for (x = visual.min_x; x <= visual.max_x; x++)
			u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if (blit_flipx)
	       for (x = visual.min_x; x <= visual.max_x; x++)
		       u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[y])[bitmap->width - x - 1];
       else if (blit_flipy)
	       for (x = visual.min_x; x <= visual.max_x; x++)
		       u32dst[x-visual.min_x] = ((UINT32 *)bitmap->line[bitmap->height - y -1])[x];
}
