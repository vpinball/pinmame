#include "mamalleg.h"
#include "driver.h"
#include <sys/farptr.h>
#include <go32.h>
#include "dirty.h"
/* for VGA triple buffer - we need the register structure */
#include "gen15khz.h"


/* from video.c (required for 15.75KHz Arcade Monitor Modes) */
extern int half_yres;
extern int unchained;

extern char *dirty_new;

extern int gfx_xoffset;
extern int gfx_yoffset;
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int gfx_width;
extern int gfx_height;
extern int skiplines;
extern int skipcolumns;
extern int use_triplebuf;
extern int triplebuf_pos,triplebuf_page_width;
extern int mmxlfb;

unsigned int doublepixel[256];
unsigned int quadpixel[256]; /* for quadring pixels */

UINT32 *palette_16bit_lookup;


/* current 'page' for unchained modes */
static int xpage = -1;
/*default page sizes for non-triple buffering */
int xpage_size = 0x8000;
int no_xpages = 2;

/* this function lifted from Allegro */
static int vesa_scroll_async(int x, int y)
{
   int ret;
	#define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)	/* in Allegro */
	extern __dpmi_regs _dpmi_reg;	/* in Allegro... I think */

//   vesa_xscroll = x;
//   vesa_yscroll = y;

#if 0
   int seg;
   long a;
	extern void (*_pm_vesa_scroller)(void);	/* in Allegro */
	extern int _mmio_segment;	/* in Allegro */
   if (_pm_vesa_scroller) {            /* use protected mode interface? */
      seg = _mmio_segment ? _mmio_segment : _my_ds();

      a = ((x * BYTES_PER_PIXEL(screen->vtable->color_depth)) +
	   (y * ((unsigned long)screen->line[1] - (unsigned long)screen->line[0]))) / 4;

      asm (
	 "  pushw %%es ; "
	 "  movw %w1, %%es ; "         /* set the IO segment */
	 "  call *%0 ; "               /* call the VESA function */
	 "  popw %%es "

      :                                /* no outputs */

      : "S" (_pm_vesa_scroller),       /* function pointer in esi */
	"a" (seg),                     /* IO segment in eax */
	"b" (0x00),                    /* mode in ebx */
	"c" (a & 0xFFFF),              /* low word of address in ecx */
	"d" (a >> 16)                  /* high word of address in edx */

//      : "memory", "%edi", "%cc"        /* clobbers edi and flags */
		: "memory", "%ebp", "%edi", "%cc" /* clobbers ebp, edi and flags (at least) */
      );

      ret = 0;
   }
   else
#endif
   {                              /* use a real mode interrupt call */
      _dpmi_reg.x.ax = 0x4F07;
      _dpmi_reg.x.bx = 0;
      _dpmi_reg.x.cx = x;
      _dpmi_reg.x.dx = y;

      __dpmi_int(0x10, &_dpmi_reg);
      ret = _dpmi_reg.h.ah;

//      _vsync_in();
   }

   return (ret ? -1 : 0);
}


void blitscreen_dirty1_vga(struct osd_bitmap *bitmap)
{
	int width4, x, y;
	unsigned long *lb, address;

	width4 = (bitmap->line[1] - bitmap->line[0]) / 4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(bitmap->line[skiplines] + skipcolumns);

	for (y = 0; y < gfx_display_lines; y += 16)
	{
		for (x = 0; x < gfx_display_columns; )
		{
			int w = 16;
			if (ISDIRTY(x,y))
			{
				unsigned long *lb0 = lb + x / 4;
				unsigned long address0 = address + x;
				int h;
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
                    w += 16;
				if (x + w > gfx_display_columns)
                    w = gfx_display_columns - x;
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++)
				{
					_dosmemputl(lb0, w/4, address0);
					lb0 += width4;
					address0 += gfx_width;
				}
			}
			x += w;
        }
		lb += 16 * width4;
		address += 16 * gfx_width;
	}
}

void blitscreen_dirty0_vga(struct osd_bitmap *bitmap)
{
	int width4,y,columns4;
	unsigned long *lb, address;

	width4 = (bitmap->line[1] - bitmap->line[0]) / 4;
	columns4 = gfx_display_columns/4;
	address = 0xa0000 + gfx_xoffset + gfx_yoffset * gfx_width;
	lb = (unsigned long *)(bitmap->line[skiplines] + skipcolumns);
	for (y = 0; y < gfx_display_lines; y++)
	{
		_dosmemputl(lb,columns4,address);
		lb+=width4;
		address+=gfx_width;
	}
}

/* for flipping between the unchained VGA pages */
INLINE void unchained_flip(void)
{
	int	flip_value1,flip_value2;
	int	temp;

/* memory address of non-visible page */
	temp = xpage_size * xpage;

	flip_value1 = ((temp & 0xff00) | 0x0c);
	flip_value2 = (((temp << 8) & 0xff00) | 0x0d);

/* flip the page var */
	xpage ++;
	if (xpage == no_xpages)
		xpage = 0;
/* need to change the offset address during the active display interval */
/* as the value is read during the vertical retrace */
	__asm__ __volatile__ (

	"movw   $0x3da,%%dx \n"
	"cli \n"
	".align 4                \n"
/* check for active display interval */
	"0:\n"
	"inb    %%dx,%%al \n"
	"testb  $1,%%al \n"
	"jz  	0b \n"
/* change the offset address */
	"movw   $0x3d4,%%dx \n"
	"movw   %%cx,%%ax \n"
	"outw   %%ax,%%dx \n"
	"movw   %%bx,%%ax \n"
	"outw   %%ax,%%dx \n"
	"sti \n"


/* outputs  (none)*/
	:"=c" (flip_value1),
	"=b" (flip_value2)
/* inputs -
 ecx = flip_value1 , ebx = flip_value2 */
	:"0" (flip_value1),
	"1" (flip_value2)
/* registers modified */
	:"ax", "dx", "cc", "memory"
	);
}



/* unchained dirty modes */
void blitscreen_dirty1_unchained_vga(struct osd_bitmap *bitmap)
{
	int x, y, i, outval, dirty_page, triple_page, write_triple;
	int plane, planeval, iloop, page;
	unsigned long *lb, address, triple_address;
	unsigned char *lbsave;
	unsigned long asave, triple_save;
	static int width4, word_blit, dirty_height;
	static int source_width, source_line_width, dest_width, dest_line_width;

	/* calculate our statics on the first call */
	if (xpage == -1)
	{
		width4 = ((bitmap->line[1] - bitmap->line[0]) >> 2);
		source_width = width4 << half_yres;
		dest_width = gfx_width >> 2;
		dest_line_width = (gfx_width << 2) >> half_yres;
		source_line_width = width4 << 4;
		xpage = 1;
		dirty_height = 16 >> half_yres;
		/* check if we have to do word rather than double word updates */
		word_blit = ((gfx_display_columns >> 2) & 3);
	}

	/* setup or selector */
	_farsetsel(screen->seg);

	dirty_page = xpage;
	/* non visible page, if we're triple buffering */
	triple_page = xpage + 1;
	if (triple_page == no_xpages)
		triple_page = 0;

	/* need to update all 'pages', but only update each page while it isn't visible */
	for (page = 0; page < 2; page ++)
	{
		planeval=0x0100;
		write_triple = (!page | (no_xpages == 3));

		/* go through each bit plane */
		for (plane = 0; plane < 4 ;plane ++)
		{
			address = 0xa0000 + (xpage_size * dirty_page)+(gfx_xoffset >> 2) + (((gfx_yoffset >> half_yres) * gfx_width) >> 2);
			triple_address = 0xa0000 + (xpage_size * triple_page)+(gfx_xoffset >> 2) + (((gfx_yoffset >> half_yres) * gfx_width) >> 2);
			lb = (unsigned long *)(bitmap->line[skiplines] + skipcolumns + plane);
			/*set the bit plane */
			outportw(0x3c4, planeval|0x02);
			for (y = 0; y < gfx_display_lines ; y += 16)
			{
				for (x = 0; x < gfx_display_columns; )
				{
					int w = 16;
					if (ISDIRTY(x,y))
					{
						unsigned long *lb0 = lb + (x >> 2);
						unsigned long address0 = address + (x >> 2);
						unsigned long address1 = triple_address + (x >> 2);
						int h;
						while (x + w < gfx_display_columns && ISDIRTY(x+w,y))
							w += 16;
						if (x + w > gfx_display_columns)
							w = gfx_display_columns - x;
						if(word_blit)
						{
							iloop = w >> 3;
							for (h = 0; h < dirty_height && (y + (h << half_yres)) < gfx_display_lines; h++)
							{
								asave = address0;
								triple_save = address1;
								lbsave = (unsigned char *)lb0;
								for(i = 0; i < iloop; i++)
								{
									outval = *lbsave | (lbsave[4] << 8);
									_farnspokew(asave, outval);
									/* write 2 pages on first pass if triple buffering */
									if (write_triple)
									{
										_farnspokew(triple_save, outval);
										triple_save += 2;
									}
									lbsave += 8;
									asave += 2;
								}
								if (w&4)
								{
									_farnspokeb(asave, *lbsave);
		   							if (write_triple)
										_farnspokeb(triple_save, *lbsave);
								}


								lb0 += source_width;
								address0 += dest_width;
								address1 += dest_width;
							}
						}
						else
						{
							iloop = w >> 4;
							for (h = 0; h < dirty_height && (y + (h << half_yres)) < gfx_display_lines; h++)
							{
								asave = address0;
								triple_save = address1;
								lbsave = (unsigned char *)lb0;
								for(i = 0; i < iloop; i++)
								{
									outval = *lbsave | (lbsave[4] << 8) | (lbsave[8] << 16) | (lbsave[12] << 24);
									_farnspokel(asave, outval);
									/* write 2 pages on first pass if triple buffering */
									if (page == 0 && no_xpages == 3)
									{
										_farnspokel(triple_save, outval);
										triple_save += 4;
									}
									lbsave += 16;
									asave += 4;
								}
								lb0 += source_width;
								address0 += dest_width;
								address1 += dest_width;
							}
						}
					}
					x += w;
				}
				lb += source_line_width;
				address += dest_line_width;
				triple_address += dest_line_width;
			}
			/* move onto the next bit plane */
			planeval <<= 1;
		}
		/* 'bank switch' our unchained output on the first loop */
		if(!page)
			unchained_flip();
		/* move onto next 'page' */
		dirty_page += (no_xpages - 1);
		if (dirty_page >= no_xpages)
			dirty_page -= no_xpages;
	}
}

void init_unchained_blit(void)
{
    xpage=-1;
}

/* Macros for non dirty unchained blits */
#define UNCHAIN_BLIT_START \
	int dummy1,dummy2,dummy3; \
	__asm__ __volatile__ ( \
/* save es and set it to our video selector */ \
	"pushw  %%es \n" \
	"movw   %%dx,%%es \n" \
	"movw   $0x102,%%ax \n" \
	"cld \n" \
	".align 4 \n" \
/* --bit plane loop-- */ \
	"0:\n" \
/* save everything */ \
	"pushl  %%ebx \n" \
	"pushl  %%edi \n" \
	"pushl  %%eax \n" \
	"pushl  %%ecx \n" \
/* set the bit plane */ \
	"movw   $0x3c4,%%dx \n" \
	"outw   %%ax,%%dx \n" \
/* edx now free, so use it for the memwidth */ \
	"movl   %4,%%edx \n" \
/* --height loop-- */ \
	"1:\n" \
/* save counter , source + dest address */ \
	"pushl 	%%ecx \n" \
	"pushl	%%ebx \n" \
	"pushl	%%edi \n" \
/* --width loop-- */ \
	"movl   %3,%%ecx \n" \
	"2:\n" \
/* get the 4 bytes */ \
/* bswap should be faster than shift, */ \
/* movl should be faster then movb */ \
	"movl   %%ds:12(%%ebx),%%eax \n" \
	"movb   %%ds:8(%%ebx),%%ah \n" \
	"bswap  %%eax \n" \
	"movb   %%ds:(%%ebx),%%al \n" \
	"movb   %%ds:4(%%ebx),%%ah \n" \
/* write the thing to video */ \
	"stosl \n" \
/* move onto next source address */ \
	"addl   $16,%%ebx \n" \
	"loop	2b \n"

#define UNCHAIN_BLIT_END \
/* --end of width loop-- */ \
/* get counter, source + dest address back */ \
	"popl	%%edi \n" \
	"popl	%%ebx \n" \
	"popl   %%ecx \n" \
/* move to the next line */ \
	"addl   %%edx,%%ebx \n" \
	"addl   %%esi,%%edi \n" \
	"loop   1b \n" \
/* --end of height loop-- */ \
/* get everything back */ \
	"popl   %%ecx \n" \
	"popl   %%eax \n"  \
	"popl   %%edi \n" \
	"popl   %%ebx \n" \
/* move onto next bit plane */ \
	"incl   %%ebx \n" \
	"shlb   $1,%%ah \n" \
/* check if we've done all 4 or not */ \
	"testb  $0x10,%%ah \n" \
	"jz		0b \n" \
/* --end of bit plane loop-- */ \
/* restore es */ \
	"popw   %%es \n" \
/* outputs (aka clobbered input) */ \
	:"=S" (dummy1), \
	"=b" (dummy2), \
	"=d" (dummy3) \
/* inputs */ \
/* %0=width, %1=memwidth, */ \
/* esi = scrwidth */ \
/* ebx = src, ecx = height */ \
/* edx = seg, edi = address */ \
	:"g" (width), \
	"g" (memwidth), \
	"c" (height), \
	"D" (address), \
	"0" (scrwidth), \
	"1" (src), \
	"2" (seg) \
/* registers modified */ \
	:"ax", "cc", "memory" \
	);



/*asm routine for unchained blit - writes 4 bytes at a time */
INLINE void unchain_dword_blit(unsigned long *src,short seg,unsigned long address,int width,int height,int memwidth,int scrwidth)
{
/* straight forward double word blit */
	UNCHAIN_BLIT_START
	UNCHAIN_BLIT_END
}

/*asm routine for unchained blit - writes 4 bytes at a time, then 2 'odd' bytes at the end of each row */
INLINE void unchain_word_blit(unsigned long *src,short seg,unsigned long address,int width,int height,int memwidth,int scrwidth)
{
	UNCHAIN_BLIT_START
/* get the extra 2 bytes at end of the row */
	"movl   %%ds:(%%ebx),%%eax \n"
	"movb   %%ds:4(%%ebx),%%ah \n"
/* write the thing to video */
	"stosw \n"
	UNCHAIN_BLIT_END
}

/*asm routine for unchained blit - writes 4 bytes at a time, then 1 'odd' byte at the end of each row */
INLINE void unchain_byte_blit(unsigned long *src,short seg,unsigned long address,int width,int height,int memwidth,int scrwidth)
{
	UNCHAIN_BLIT_START
/* get the extra byte at end of the row */
	"movb   %%ds:(%%ebx),%%al \n"
/* write the thing to video */
	"stosb \n"
	UNCHAIN_BLIT_END
}

/* unchained 'non-dirty' modes */
void blitscreen_dirty0_unchained_vga(struct osd_bitmap *bitmap)
{
	unsigned long *lb, address;
	static int width4,columns4,column_chained,memwidth,scrwidth,disp_height,blit_type;

   /* only calculate our statics the first time around */
	if(xpage==-1)
	{
		width4 = (bitmap->line[1] - bitmap->line[0]) >> 2;
		columns4 = gfx_display_columns >> 2;
		disp_height = gfx_display_lines >> half_yres;

		xpage = 1;
		memwidth = (bitmap->line[1] - bitmap->line[0]) << half_yres;
		scrwidth = gfx_width >> 2;

		/* check for 'not divisible by 8' */
		if((columns4 & 1))
			blit_type = 2;    /* byte blit */
		else
		{
			if((columns4 & 2))
				blit_type = 1; /* word blit */
			else
				blit_type = 0; /* double word blit */
		}

		column_chained = columns4 >> 2;
	}

	/* get the start of the screen bitmap */
	lb = (unsigned long *)(bitmap->line[skiplines] + skipcolumns);
	/* and the start address in video memory */
	address = 0xa0000 + (xpage_size * xpage)+(gfx_xoffset >> 2) + (((gfx_yoffset >> half_yres) * gfx_width) >> 2);
	/* call the appropriate blit routine */
	switch (blit_type)
	{
		case 0: /* double word blit */
			unchain_dword_blit (lb, screen->seg, address, column_chained, disp_height, memwidth, scrwidth);
			break;
		case 1: /* word blit */
			unchain_word_blit (lb, screen->seg, address, column_chained, disp_height, memwidth, scrwidth);
			break;
		case 2: /* byte blit */
			unchain_byte_blit (lb, screen->seg, address, column_chained, disp_height, memwidth, scrwidth);
			break;
	}
	/* 'bank switch' our unchained output */
	unchained_flip();
}



/* setup register array to be unchained */
void unchain_vga(Register *pReg)
{
/* setup registers for an unchained mode */
	pReg[UNDERLINE_LOC_INDEX].value = 0x00;
	pReg[MODE_CONTROL_INDEX].value = 0xe3;
	pReg[MEMORY_MODE_INDEX].value = 0x06;
/* flag the fact it's unchained */
	unchained = 1;
}

INLINE void copyline_1x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	short src_seg;

	src_seg = _my_ds();

	_movedatal(src_seg,(unsigned long)src,seg,address,width4);
}

INLINE void copyline_1x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	short src_seg;

	src_seg = _my_ds();

	_movedatal(src_seg,(unsigned long)src,seg,address,width2);
}

#if 1 /* use the C approach instead */
INLINE void copyline_2x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"bswap %%eax             \n"
	"xchgw %%ax,%%bx         \n"
	"roll $8, %%eax          \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"rorl $8, %%eax          \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	:
	"=c" (width4),
	"=d" (seg),
	"=S" (src),
	"=D" (address)
	:
	"0" (width4),
	"1" (seg),
	"2" (src),
	"3" (address):
	"ax", "bx", "cc", "memory");
}
#else
INLINE void copyline_2x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width; i > 0; i--)
	{
		_farnspokel (address, doublepixel[*src] | (doublepixel[*(src+1)] << 16));
		_farnspokel (address+4, doublepixel[*(src+2)] | (doublepixel[*(src+3)] << 16));
		address+=8;
		src+=4;
	}
}
#endif

INLINE void copyline_2x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"roll $16, %%eax         \n"
	"xchgw %%ax,%%bx         \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"stosl                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	:
	"=c" (width2),
	"=d" (seg),
	"=S" (src),
	"=D" (address)
	:
	"0" (width2),
	"1" (seg),
	"2" (src),
	"3" (address):
	"ax", "bx", "cc", "memory");
}

INLINE void copyline_3x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"roll $16, %%eax         \n"
	"xchgw %%ax,%%bx         \n"
	"stosl                   \n"
	"stosw                   \n"
	"movl %%ebx, %%eax       \n"
	"stosl                   \n"
	"stosw                   \n"
	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	:
	"=c" (width2),
	"=d" (seg),
	"=S" (src),
	"=D" (address)
	:
	"0" (width2),
	"1" (seg),
	"2" (src),
	"3" (address):
	"ax", "bx", "cc", "memory");
}

INLINE void copyline_4x_16bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	__asm__ __volatile__ (
	"pushw %%es              \n"
	"movw %%dx, %%es         \n"
	"cld                     \n"
	".align 4                \n"
	"0:                      \n"
	"lodsl                   \n"
	"movl %%eax, %%ebx       \n"
	"roll $16, %%eax         \n"
	"xchgw %%ax,%%bx         \n"
	"stosl                   \n"
	"stosl                   \n"
	"movl %%ebx, %%eax       \n"
	"stosl                   \n"
	"stosl 					 \n"


	"loop 0b                 \n"
	"popw %%ax               \n"
	"movw %%ax, %%es         \n"
	:
	"=c" (width2),
	"=d" (seg),
	"=S" (src),
	"=D" (address)
	:
	"0" (width2),
	"1" (seg),
	"2" (src),
	"3" (address):
	"ax", "bx", "cc", "memory");
}

INLINE void copyline_3x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width4; i > 0; i--)
	{
		_farnspokel (address, (quadpixel[*src] & 0x00ffffff) | (quadpixel[*(src+1)] & 0xff000000));
		_farnspokel (address+4, (quadpixel[*(src+1)] & 0x0000ffff) | (quadpixel[*(src+2)] & 0xffff0000));
		_farnspokel (address+8, (quadpixel[*(src+2)] & 0x000000ff) | (quadpixel[*(src+3)] & 0xffffff00));
		address+=3*4;
		src+=4;
	}
}

INLINE void copyline_4x_8bpp(unsigned char *src,short seg,unsigned long address,int width4)
{
	int i;

	/* set up selector */
	_farsetsel(seg);

	for (i = width4; i > 0; i--)
	{
		_farnspokel (address, quadpixel[*src]);
		_farnspokel (address+4, quadpixel[*(src+1)]);
		_farnspokel (address+8, quadpixel[*(src+2)]);
		_farnspokel (address+12, quadpixel[*(src+3)]);
		address+=16;
		src+=4;
	}
}




INLINE void copyline_1x_16bpp_palettized(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	unsigned short *s=(unsigned short *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		UINT32 d1,d2;

		d1 = palette_16bit_lookup[*(s++)];
		d2 = palette_16bit_lookup[*(s++)];
		_farnspokel(address,(d1 & 0x0000ffff) | (d2 & 0xffff0000));
		address+=4;
	}
}

INLINE void copyline_2x_16bpp_palettized(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	unsigned short *s=(unsigned short *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = 2*width2; i > 0; i--)
	{
		_farnspokel(address,palette_16bit_lookup[*(s++)]);
		address+=4;
	}
}

INLINE void copyline_3x_16bpp_palettized(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	unsigned short *s=(unsigned short *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		UINT32 d1,d2;

		d1 = palette_16bit_lookup[*(s++)];
		d2 = palette_16bit_lookup[*(s++)];
		_farnspokel(address,d1);
		_farnspokel(address+4,(d1 & 0x0000ffff) | (d2 & 0xffff0000));
		_farnspokel(address+8,d2);
		address+=3*4;
	}
}

INLINE void copyline_4x_16bpp_palettized(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	unsigned short *s=(unsigned short *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = 2*width2; i > 0; i--)
	{
		UINT32 d;

		d = palette_16bit_lookup[*(s++)];
		_farnspokel(address,d);
		_farnspokel(address+4,d);
		address+=8;
	}
}




INLINE void copyline_1x_32bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	UINT32 *s=(UINT32 *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		_farnspokel(address,*(s++));
		address+=4;
	}
}

INLINE void copyline_2x_32bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	UINT32 *s=(UINT32 *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		UINT32 d;

		d = *(s++);
		_farnspokel(address,d);
		_farnspokel(address+4,d);
		address+=8;
	}
}

INLINE void copyline_3x_32bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	UINT32 *s=(UINT32 *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		UINT32 d;

		d = *(s++);
		_farnspokel(address,d);
		_farnspokel(address+4,d);
		_farnspokel(address+8,d);
		address+=3*4;
	}
}

INLINE void copyline_4x_32bpp(unsigned char *src,short seg,unsigned long address,int width2)
{
	int i;
	UINT32 *s=(UINT32 *)src;

	/* set up selector */
	_farsetsel(seg);

	for (i = width2; i > 0; i--)
	{
		UINT32 d;

		d = *(s++);
		_farnspokel(address,d);
		_farnspokel(address+4,d);
		_farnspokel(address+8,d);
		_farnspokel(address+12,d);
		address+=16;
	}
}




#define DIRTY1(MX,MY,SL,BPP,PALETTIZED) \
	short dest_seg; \
	int x,y,vesa_line,line_offs,xoffs; \
	unsigned char *lb; \
	unsigned long address; \
	dest_seg = screen->seg; \
	vesa_line = gfx_yoffset; \
	line_offs = bitmap->line[1] - bitmap->line[0]; \
	xoffs = (BPP/8)*gfx_xoffset; \
	lb = bitmap->line[skiplines] + (BPP/8)*skipcolumns; \
	for (y = 0;y < gfx_display_lines;y += 16) \
	{ \
		for (x = 0;x < gfx_display_columns; /* */) \
		{ \
			int w = 16; \
			if (ISDIRTY(x,y)) \
            { \
				unsigned char *src = lb + (BPP/8)*x; \
                int vesa_line0 = vesa_line, h; \
				while (x + w < gfx_display_columns && ISDIRTY(x+w,y)) \
                    w += 16; \
				if (x + w > gfx_display_columns) \
					w = gfx_display_columns - x; \
				for (h = 0; h < 16 && y + h < gfx_display_lines; h++) \
                { \
					address = bmp_write_line(screen,vesa_line0) + xoffs + MX*(BPP/8)*x; \
					copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,w/(4/(BPP/8))); \
				    if ((MY > 2) || ((MY == 2) && (SL == 0))) { \
						address = bmp_write_line(screen,vesa_line0+1) + xoffs + MX*(BPP/8)*x; \
						copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,w/(4/(BPP/8))); \
					} \
					if ((MY > 3) || ((MY == 3) && (SL == 0))) { \
						address = bmp_write_line(screen,vesa_line0+2) + xoffs + MX*(BPP/8)*x; \
						copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,w/(4/(BPP/8))); \
					} \
					vesa_line0 += MY; \
					src += line_offs; \
				} \
			} \
			x += w; \
		} \
		vesa_line += MY*16; \
		lb += 16 * line_offs; \
	}

#define DIRTY0(MX,MY,SL,BPP,PALETTIZED) \
	short dest_seg; \
	int y,vesa_line,line_offs,xoffs,width; \
	unsigned char *src; \
	unsigned long address, address_offset; \
	dest_seg = screen->seg; \
	vesa_line = gfx_yoffset; \
	line_offs = (bitmap->line[1] - bitmap->line[0]); \
	xoffs = (BPP/8)*(gfx_xoffset + triplebuf_pos); \
	width = gfx_display_columns/(4/(BPP/8)); \
	src = bitmap->line[skiplines] + (BPP/8)*skipcolumns;	\
	if (mmxlfb) { \
		address = bmp_write_line(screen, vesa_line); \
		_farsetsel (screen->seg); \
		address_offset = bmp_write_line(screen, vesa_line+1) - address; \
		address += xoffs; \
		{ \
		extern void asmblit_##MX##x_##MY##y_##SL##sl_##BPP##bpp##PALETTIZED \
			(int, int, unsigned char *, int, unsigned long, unsigned long); \
		asmblit_##MX##x_##MY##y_##SL##sl_##BPP##bpp##PALETTIZED \
			(width, gfx_display_lines, src, line_offs, address, address_offset); \
		} \
	} \
	else { \
		for (y = 0;y < gfx_display_lines;y++) \
		{ \
			address = bmp_write_line(screen,vesa_line) + xoffs; \
			copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,width); \
		    if ((MY > 2) || ((MY == 2) && (SL == 0))) { \
				address = bmp_write_line(screen,vesa_line+1) + xoffs; \
				copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,width); \
			} \
			if ((MY > 3) || ((MY == 3) && (SL == 0))) { \
				address = bmp_write_line(screen,vesa_line+2) + xoffs; \
				copyline_##MX##x_##BPP##bpp##PALETTIZED(src,dest_seg,address,width); \
			} \
			vesa_line += MY; \
			src += line_offs; \
		} \
	} \
	if (use_triplebuf) \
	{ \
		vesa_scroll_async(triplebuf_pos,0); \
		triplebuf_pos = (triplebuf_pos + triplebuf_page_width) % (3*triplebuf_page_width); \
	}


void blitscreen_dirty1_vesa_1x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(1,1,0,8,)  }
void blitscreen_dirty1_vesa_1x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(1,2,0,8,)  }
void blitscreen_dirty1_vesa_1x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(1,2,1,8,)  }
void blitscreen_dirty1_vesa_2x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(2,1,0,8,)  }
void blitscreen_dirty1_vesa_3x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(3,1,0,8,)  }
void blitscreen_dirty1_vesa_2x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(2,2,0,8,)  }
void blitscreen_dirty1_vesa_2x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,2,1,8,)  }
void blitscreen_dirty1_vesa_2x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(2,3,0,8,)  }
void blitscreen_dirty1_vesa_2x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,3,1,8,)  }
void blitscreen_dirty1_vesa_3x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(3,2,0,8,)  }
void blitscreen_dirty1_vesa_3x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,2,1,8,)  }
void blitscreen_dirty1_vesa_3x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(3,3,0,8,)  }
void blitscreen_dirty1_vesa_3x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,3,1,8,)  }
void blitscreen_dirty1_vesa_4x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(4,2,0,8,)  }
void blitscreen_dirty1_vesa_4x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,2,1,8,)  }
void blitscreen_dirty1_vesa_4x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY1(4,3,0,8,)  }
void blitscreen_dirty1_vesa_4x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,3,1,8,)  }

void blitscreen_dirty1_vesa_1x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(1,1,0,16,) }
void blitscreen_dirty1_vesa_1x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(1,2,0,16,) }
void blitscreen_dirty1_vesa_1x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(1,2,1,16,) }
void blitscreen_dirty1_vesa_2x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,1,0,16,) }
void blitscreen_dirty1_vesa_2x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,2,0,16,) }
void blitscreen_dirty1_vesa_2x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(2,2,1,16,) }
void blitscreen_dirty1_vesa_2x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,3,0,16,) }
void blitscreen_dirty1_vesa_2x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(2,3,1,16,) }
void blitscreen_dirty1_vesa_3x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,1,0,16,) }
void blitscreen_dirty1_vesa_3x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,2,0,16,) }
void blitscreen_dirty1_vesa_3x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(3,2,1,16,) }
void blitscreen_dirty1_vesa_3x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,3,0,16,) }
void blitscreen_dirty1_vesa_3x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(3,3,1,16,) }
void blitscreen_dirty1_vesa_4x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,2,0,16,) }
void blitscreen_dirty1_vesa_4x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(4,2,1,16,) }
void blitscreen_dirty1_vesa_4x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,3,0,16,) }
void blitscreen_dirty1_vesa_4x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY1(4,3,1,16,) }

void blitscreen_dirty1_vesa_1x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(1,1,0,16,_palettized) }
void blitscreen_dirty1_vesa_1x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(1,2,0,16,_palettized) }
void blitscreen_dirty1_vesa_1x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(1,2,1,16,_palettized) }
void blitscreen_dirty1_vesa_2x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(2,1,0,16,_palettized) }
void blitscreen_dirty1_vesa_2x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(2,2,0,16,_palettized) }
void blitscreen_dirty1_vesa_2x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(2,2,1,16,_palettized) }
void blitscreen_dirty1_vesa_2x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(2,3,0,16,_palettized) }
void blitscreen_dirty1_vesa_2x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(2,3,1,16,_palettized) }
void blitscreen_dirty1_vesa_3x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(3,1,0,16,_palettized) }
void blitscreen_dirty1_vesa_3x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(3,2,0,16,_palettized) }
void blitscreen_dirty1_vesa_3x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(3,2,1,16,_palettized) }
void blitscreen_dirty1_vesa_3x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(3,3,0,16,_palettized) }
void blitscreen_dirty1_vesa_3x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(3,3,1,16,_palettized) }
void blitscreen_dirty1_vesa_4x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(4,2,0,16,_palettized) }
void blitscreen_dirty1_vesa_4x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(4,2,1,16,_palettized) }
void blitscreen_dirty1_vesa_4x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY1(4,3,0,16,_palettized) }
void blitscreen_dirty1_vesa_4x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY1(4,3,1,16,_palettized) }

void blitscreen_dirty1_vesa_1x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(1,1,0,32,) }
void blitscreen_dirty1_vesa_1x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(1,2,0,32,) }
void blitscreen_dirty1_vesa_1x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(1,2,1,32,) }
void blitscreen_dirty1_vesa_2x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,1,0,32,) }
void blitscreen_dirty1_vesa_2x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,2,0,32,) }
void blitscreen_dirty1_vesa_2x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(2,2,1,32,) }
void blitscreen_dirty1_vesa_2x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(2,3,0,32,) }
void blitscreen_dirty1_vesa_2x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(2,3,1,32,) }
void blitscreen_dirty1_vesa_3x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,1,0,32,) }
void blitscreen_dirty1_vesa_3x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,2,0,32,) }
void blitscreen_dirty1_vesa_3x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(3,2,1,32,) }
void blitscreen_dirty1_vesa_3x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(3,3,0,32,) }
void blitscreen_dirty1_vesa_3x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(3,3,1,32,) }
void blitscreen_dirty1_vesa_4x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,2,0,32,) }
void blitscreen_dirty1_vesa_4x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(4,2,1,32,) }
void blitscreen_dirty1_vesa_4x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY1(4,3,0,32,) }
void blitscreen_dirty1_vesa_4x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY1(4,3,1,32,) }

void blitscreen_dirty0_vesa_1x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(1,1,0,8,)  }
void blitscreen_dirty0_vesa_1x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(1,2,0,8,)  }
void blitscreen_dirty0_vesa_1x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(1,2,1,8,)  }
void blitscreen_dirty0_vesa_2x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(2,1,0,8,)  }
void blitscreen_dirty0_vesa_3x_1x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(3,1,0,8,)  }
void blitscreen_dirty0_vesa_2x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(2,2,0,8,)  }
void blitscreen_dirty0_vesa_2x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,2,1,8,)  }
void blitscreen_dirty0_vesa_2x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(2,3,0,8,)  }
void blitscreen_dirty0_vesa_2x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,3,1,8,)  }
void blitscreen_dirty0_vesa_3x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(3,2,0,8,)  }
void blitscreen_dirty0_vesa_3x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,2,1,8,)  }
void blitscreen_dirty0_vesa_3x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(3,3,0,8,)  }
void blitscreen_dirty0_vesa_3x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,3,1,8,)  }
void blitscreen_dirty0_vesa_4x_2x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(4,2,0,8,)  }
void blitscreen_dirty0_vesa_4x_2xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,2,1,8,)  }
void blitscreen_dirty0_vesa_4x_3x_8bpp(struct osd_bitmap *bitmap)   { DIRTY0(4,3,0,8,)  }
void blitscreen_dirty0_vesa_4x_3xs_8bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,3,1,8,)  }

void blitscreen_dirty0_vesa_1x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(1,1,0,16,) }
void blitscreen_dirty0_vesa_1x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(1,2,0,16,) }
void blitscreen_dirty0_vesa_1x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(1,2,1,16,) }
void blitscreen_dirty0_vesa_2x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,1,0,16,) }
void blitscreen_dirty0_vesa_2x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,2,0,16,) }
void blitscreen_dirty0_vesa_2x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(2,2,1,16,) }
void blitscreen_dirty0_vesa_2x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,3,0,16,)  }
void blitscreen_dirty0_vesa_2x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(2,3,1,16,)  }
void blitscreen_dirty0_vesa_3x_1x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,1,0,16,) }
void blitscreen_dirty0_vesa_3x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,2,0,16,) }
void blitscreen_dirty0_vesa_3x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(3,2,1,16,) }
void blitscreen_dirty0_vesa_3x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,3,0,16,) }
void blitscreen_dirty0_vesa_3x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(3,3,1,16,) }
void blitscreen_dirty0_vesa_4x_2x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,2,0,16,) }
void blitscreen_dirty0_vesa_4x_2xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(4,2,1,16,) }
void blitscreen_dirty0_vesa_4x_3x_16bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,3,0,16,) }
void blitscreen_dirty0_vesa_4x_3xs_16bpp(struct osd_bitmap *bitmap) { DIRTY0(4,3,1,16,) }

void blitscreen_dirty0_vesa_1x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(1,1,0,16,_palettized) }
void blitscreen_dirty0_vesa_1x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(1,2,0,16,_palettized) }
void blitscreen_dirty0_vesa_1x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(1,2,1,16,_palettized) }
void blitscreen_dirty0_vesa_2x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(2,1,0,16,_palettized) }
void blitscreen_dirty0_vesa_2x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(2,2,0,16,_palettized) }
void blitscreen_dirty0_vesa_2x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(2,2,1,16,_palettized) }
void blitscreen_dirty0_vesa_2x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(2,3,0,16,_palettized) }
void blitscreen_dirty0_vesa_2x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(2,3,1,16,_palettized) }
void blitscreen_dirty0_vesa_3x_1x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(3,1,0,16,_palettized) }
void blitscreen_dirty0_vesa_3x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(3,2,0,16,_palettized) }
void blitscreen_dirty0_vesa_3x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(3,2,1,16,_palettized) }
void blitscreen_dirty0_vesa_3x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(3,3,0,16,_palettized) }
void blitscreen_dirty0_vesa_3x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(3,3,1,16,_palettized) }
void blitscreen_dirty0_vesa_4x_2x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(4,2,0,16,_palettized) }
void blitscreen_dirty0_vesa_4x_2xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(4,2,1,16,_palettized) }
void blitscreen_dirty0_vesa_4x_3x_16bpp_palettized(struct osd_bitmap *bitmap)  { DIRTY0(4,3,0,16,_palettized) }
void blitscreen_dirty0_vesa_4x_3xs_16bpp_palettized(struct osd_bitmap *bitmap) { DIRTY0(4,3,1,16,_palettized) }

void blitscreen_dirty0_vesa_1x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(1,1,0,32,) }
void blitscreen_dirty0_vesa_1x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(1,2,0,32,) }
void blitscreen_dirty0_vesa_1x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(1,2,1,32,) }
void blitscreen_dirty0_vesa_2x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,1,0,32,) }
void blitscreen_dirty0_vesa_2x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,2,0,32,) }
void blitscreen_dirty0_vesa_2x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(2,2,1,32,) }
void blitscreen_dirty0_vesa_2x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(2,3,0,32,) }
void blitscreen_dirty0_vesa_2x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(2,3,1,32,) }
void blitscreen_dirty0_vesa_3x_1x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,1,0,32,) }
void blitscreen_dirty0_vesa_3x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,2,0,32,) }
void blitscreen_dirty0_vesa_3x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(3,2,1,32,) }
void blitscreen_dirty0_vesa_3x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(3,3,0,32,) }
void blitscreen_dirty0_vesa_3x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(3,3,1,32,) }
void blitscreen_dirty0_vesa_4x_2x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,2,0,32,) }
void blitscreen_dirty0_vesa_4x_2xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(4,2,1,32,) }
void blitscreen_dirty0_vesa_4x_3x_32bpp(struct osd_bitmap *bitmap)  { DIRTY0(4,3,0,32,) }
void blitscreen_dirty0_vesa_4x_3xs_32bpp(struct osd_bitmap *bitmap) { DIRTY0(4,3,1,32,) }
