/*
** File: tms9928a.c -- software implementation of the Texas Instruments
**                     TMS9918A and TMS9928A, used by the Coleco, MSX and
**                     TI99/4A.
**
** All undocumented features as described in the following file
** should be emulated.
**
** http://www.msxnet.org/tech/tms9918a.txt
**
** By Sean Young 1999 (sean@msxnet.org).
** Based on code by Mike Balfour. Features added:
** - read-ahead
** - single read/write address
** - AND mask for mode 2
** - multicolor mode
** - undocumented screen modes
** - illegal sprites (max 4 on one line)
** - vertical coordinate corrected -- was one to high (255 => 0, 0 => 1)
** - errors in interrupt emulation
** - back drop correctly emulated.
**
** 19 feb 2000, Sean:
** - now uses plot_pixel (..), so -ror works properly
** - fixed bug in tms.patternmask
**
** 3 nov 2000, Raphael Nabet:
** - fixed a nasty bug in _TMS9928A_sprites. A transparent sprite caused 
**   sprites at lower levels not to be displayed, which is wrong.
**
** 3 jan 2001, Sean Young:
** - A few minor cleanups
** - Changed TMS9928A_vram_[rw] and  TMS9928A_register_[rw] to READ_HANDLER 
**   and WRITE_HANDLER.
** - Got rid of the color table, unused. Also got rid of the old colors,
**   which where commented out anyways.
** 
**
** Todo:
** - The screen image is rendered in `one go'. Modifications during
**   screen build up are not shown.
** - Correctly emulate 4,8,16 kb VRAM if needed.
** - uses plot_pixel (...) in TMS_sprites (...), which is rended in
**   in a back buffer created with malloc (). Hmm..
** - Colours are incorrect. [fixed by R Nabet ?]
**
** MODIFIED: by Steve Ellenoff for multi-chip support (01/24/2002)
*/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "tms9928a.h"

/*
	New palette (R. Nabet).

	First 3 columns from TI datasheet (in volts).
	Next 3 columns based on formula :
		Y = .299*R + .587*G + .114*B (NTSC)
	(the coefficients are likely to be slightly different with PAL, but who cares ?)
	I assumed the "zero" for R-Y and B-Y was 0.47V.
	Last 3 coeffs are the 8-bit values.

	Color            Y  	R-Y 	B-Y 	R   	G   	B   	R	G	B
	0 Transparent
	1 Black         0.00	0.47	0.47	0.00	0.00	0.00	  0	  0	  0
	2 Medium green  0.53	0.07	0.20	0.13	0.79	0.26	 33	200	 66
	3 Light green   0.67	0.17	0.27	0.37	0.86	0.47	 94	220	120
	4 Dark blue     0.40	0.40	1.00	0.33	0.33	0.93	 84	 85	237
	5 Light blue    0.53	0.43	0.93	0.49	0.46	0.99	125	118	252
	6 Dark red      0.47	0.83	0.30	0.83	0.32	0.30	212	 82	 77
	7 Cyan          0.73	0.00	0.70	0.26	0.92	0.96	 66	235	245
	8 Medium red    0.53	0.93	0.27	0.99	0.33	0.33	252	 85	 84
	9 Light red     0.67	0.93	0.27	1.13(!)	0.47	0.47	255	121	120
	A Dark yellow   0.73	0.57	0.07	0.83	0.76	0.33	212	193	 84
	B Light yellow  0.80	0.57	0.17	0.90	0.81	0.50	230	206	128
	C Dark green    0.47	0.13	0.23	0.13	0.69	0.23	 33	176	 59
	D Magenta       0.53	0.73	0.67	0.79	0.36	0.73	201	 91	186
	E Gray          0.80	0.47	0.47	0.80	0.80	0.80	204	204	204
	F White         1.00	0.47	0.47	1.00	1.00	1.00	255	255	255
*/
static unsigned char TMS9928A_palette[16*3] =
{
	0, 0, 0,
	0, 0, 0,
	33, 200, 66,
	94, 220, 120,
	84, 85, 237,
	125, 118, 252,
	212, 82, 77,
	66, 235, 245,
	252, 85, 84,
	255, 121, 120,
	212, 193, 84,
	230, 206, 128,
	33, 176, 59,
	201, 91, 186,
	204, 204, 204,
	255, 255, 255
};

#if 0
unsigned short TMS9928A_colortable[] =
{
        0,1,
        0,2,
        0,3,
        0,4,
        0,5,
        0,6,
        0,7,
        0,8,
        0,9,
        0,10,
        0,11,
        0,12,
        0,13,
        0,14,
        0,15,
};
#endif

/*
** Defines for `dirty' optimization
*/

#define MAX_DIRTY_COLOUR        (256*3)
#define MAX_DIRTY_PATTERN       (256*3)
#define MAX_DIRTY_NAME          (40*24)

/*
** Forward declarations of internal functions.
*/
static void _TMS9928A_mode0 (int which,struct osd_bitmap*);
static void _TMS9928A_mode1 (int which,struct osd_bitmap*);
static void _TMS9928A_mode2 (int which,struct osd_bitmap*);
static void _TMS9928A_mode12 (int which,struct osd_bitmap*);
static void _TMS9928A_mode3 (int which,struct osd_bitmap*);
static void _TMS9928A_mode23 (int which,struct osd_bitmap*);
static void _TMS9928A_modebogus (int which,struct osd_bitmap*);
static void _TMS9928A_sprites (int which, struct osd_bitmap*);
static void _TMS9928A_change_register (int which, int reg, UINT8 data);
static void _TMS9928A_set_dirty (int which, char);

static void (*ModeHandlers[])(int which, struct osd_bitmap*) = {
        _TMS9928A_mode0, _TMS9928A_mode1, _TMS9928A_mode2,  _TMS9928A_mode12,
        _TMS9928A_mode3, _TMS9928A_modebogus, _TMS9928A_mode23,
        _TMS9928A_modebogus };

#define IMAGE_SIZE (256*192)        /* size of rendered image        */

#define TMS_SPRITES_ENABLED ((tms[which].Regs[1] & 0x50) == 0x40)
#define TMS_MODE ( (tms[which].model == TMS99x8A ? (tms[which].Regs[0] & 2) : 0) | \
	((tms[which].Regs[1] & 0x10)>>4) | ((tms[which].Regs[1] & 8)>>1))

typedef struct {
    /* TMS9928A internal settings */
    UINT8 ReadAhead,Regs[8],StatusReg,oldStatusReg,FirstByte,latch,INT;
    int Addr,BackColour,Change,mode;
    int colour,pattern,nametbl,spriteattribute,spritepattern;
    int colourmask,patternmask;
    void (*INTCallback)(int);
    /* memory */
    UINT8 *vMem, *dBackMem;
    struct osd_bitmap *tmpbmp;
    int vramsize, model;
    /* emulation settings */
    int LimitSprites; /* max 4 sprites on a row, like original TMS9918A */
    /* dirty tables */
    char anyDirtyColour, anyDirtyName, anyDirtyPattern;
    char *DirtyColour, *DirtyName, *DirtyPattern;
} TMS9928A;

static TMS9928A tms[MAX_VDP];

/*
** initialize the palette 
*/
void tms9928A_init_palette (unsigned char *palette, 
	unsigned short *colortable,const unsigned char *color_prom) {
    memcpy (palette, &TMS9928A_palette, sizeof(TMS9928A_palette));
}


/*
** The init, reset and shutdown functions
*/
void TMS9928A_reset (int which) {
	int  i;

	for (i=0;i<8;i++) tms[which].Regs[i] = 0;
	tms[which].StatusReg = 0;
	tms[which].nametbl = tms[which].pattern = tms[which].colour = 0;
	tms[which].spritepattern = tms[which].spriteattribute = 0;
	tms[which].colourmask = tms[which].patternmask = 0;
	tms[which].Addr = tms[which].ReadAhead = tms[which].INT = 0;
	tms[which].mode = tms[which].BackColour = 0;
	tms[which].Change = 1;
	tms[which].FirstByte = 0;
	tms[which].latch = 0;
	_TMS9928A_set_dirty (which,1);
}

int TMS9928A_start (int which, int model, unsigned int vram) {
	/* 4 or 16 kB vram please */
	if (! ((vram == 0x1000) || (vram == 0x4000) || (vram == 0x2000)) )
		return 1;

	tms[which].model = model;

	/* Video RAM */
	tms[which].vramsize = vram;
	tms[which].vMem = (UINT8*) malloc (vram);
	if (!tms[which].vMem) return (1);
	memset (tms[which].vMem, 0, vram);

	/* Sprite back buffer */
	tms[which].dBackMem = (UINT8*)malloc (IMAGE_SIZE);
	if (!tms[which].dBackMem) {
		free (tms[which].vMem);
		return 1;
	}

	/* dirty buffers */
	tms[which].DirtyName = (char*)malloc (MAX_DIRTY_NAME);
	if (!tms[which].DirtyName) {
		free (tms[which].vMem);
		free (tms[which].dBackMem);
		return 1;
	}

	tms[which].DirtyPattern = (char*)malloc (MAX_DIRTY_PATTERN);
	if (!tms[which].DirtyPattern) {
		free (tms[which].vMem);
		free (tms[which].DirtyName);
		free (tms[which].dBackMem);
		return 1;
	}

	tms[which].DirtyColour = (char*)malloc (MAX_DIRTY_COLOUR);
	if (!tms[which].DirtyColour) {
		free (tms[which].vMem);
		free (tms[which].DirtyName);
		free (tms[which].DirtyPattern);
		free (tms[which].dBackMem);
		return 1;
	}

	/* back bitmap */
	tms[which].tmpbmp = bitmap_alloc (256, 192);
	if (!tms[which].tmpbmp) {
		free (tms[which].vMem);
		free (tms[which].dBackMem);
		free (tms[which].DirtyName);
		free (tms[which].DirtyPattern);
		free (tms[which].DirtyColour);
		return 1;
	}

	TMS9928A_reset (which);
	tms[which].LimitSprites = 1;

	state_save_register_UINT8 ("tms9928a", which, "R0", &tms[which].Regs[0], 1);
	state_save_register_UINT8 ("tms9928a", which, "R1", &tms[which].Regs[1], 1);
	state_save_register_UINT8 ("tms9928a", which, "R2", &tms[which].Regs[2], 1);
	state_save_register_UINT8 ("tms9928a", which, "R3", &tms[which].Regs[3], 1);
	state_save_register_UINT8 ("tms9928a", which, "R4", &tms[which].Regs[4], 1);
	state_save_register_UINT8 ("tms9928a", which, "R5", &tms[which].Regs[5], 1);
	state_save_register_UINT8 ("tms9928a", which, "R6", &tms[which].Regs[6], 1);
	state_save_register_UINT8 ("tms9928a", which, "R7", &tms[which].Regs[7], 1);
	state_save_register_UINT8 ("tms9928a", which, "S", &tms[which].StatusReg, 1);
	state_save_register_UINT8 ("tms9928a", which, "read_ahead", &tms[which].ReadAhead, 1);
	state_save_register_UINT8 ("tms9928a", which, "first_byte", &tms[which].FirstByte, 1);
	state_save_register_UINT8 ("tms9928a", which, "latch", &tms[which].latch, 1);
	state_save_register_UINT16 ("tms9928a", which, "vram_latch", (UINT16*)&tms[which].Addr, 1);
	state_save_register_UINT8 ("tms9928a", which, "interrupt_line", &tms[which].INT, 1);
	state_save_register_UINT8 ("tms9928a", which, "VRAM", tms[which].vMem, vram);
	return 0;
}

void TMS9928A_post_load (int which) {
	int i;

	/* mark the screen as dirty */
	_TMS9928A_set_dirty (which, 1);

	/* all registers need to be re-written, so tables are recalculated */
	for (i=0;i<8;i++)
		_TMS9928A_change_register (which, i, tms[which].Regs[i]);

	/* make sure the back ground colour is reset */
	tms[which].BackColour = -1;

	/* make sure the interrupt request is set properly */
	if (tms[which].INTCallback) tms[which].INTCallback (tms[which].INT);
}

void TMS9928A_stop () {
	int which;
	/*For each chip*/
	for (which = 0; which < MAX_VDP; which++) {
		free (tms[which].vMem); tms[which].vMem = NULL;
		free (tms[which].dBackMem); tms[which].dBackMem = NULL;
		free (tms[which].DirtyColour); tms[which].DirtyColour = NULL;
		free (tms[which].DirtyName); tms[which].DirtyName = NULL;
		free (tms[which].DirtyPattern); tms[which].DirtyPattern = NULL;
		osd_free_bitmap (tms[which].tmpbmp); tms[which].tmpbmp = NULL;
	}
}

/*
** Set all dirty / clean
*/
static void _TMS9928A_set_dirty (int which, char dirty) {
    tms[which].anyDirtyColour = tms[which].anyDirtyName = tms[which].anyDirtyPattern = dirty;
    memset (tms[which].DirtyName, dirty, MAX_DIRTY_NAME);
    memset (tms[which].DirtyColour, dirty, MAX_DIRTY_COLOUR);
    memset (tms[which].DirtyPattern, dirty, MAX_DIRTY_PATTERN);
}

/*
** The I/O functions.
*/
int TMS9928A_vram_r(int which,int offset)
{
    UINT8 b;
    b = tms[which].ReadAhead;
    tms[which].ReadAhead = tms[which].vMem[tms[which].Addr];
    tms[which].Addr = (tms[which].Addr + 1) & (tms[which].vramsize - 1);
    tms[which].latch = 0;
    return b;
}

void TMS9928A_vram_w(int which,int offset,int data)
{
    int i;

    if (tms[which].vMem[tms[which].Addr] != data) {
        tms[which].vMem[tms[which].Addr] = data;
        tms[which].Change = 1;
        /* dirty optimization */
        if ( (tms[which].Addr >= tms[which].nametbl) &&
            (tms[which].Addr < (tms[which].nametbl + MAX_DIRTY_NAME) ) ) {
            tms[which].DirtyName[tms[which].Addr - tms[which].nametbl] = 1;
            tms[which].anyDirtyName = 1;
        }

        i = (tms[which].Addr - tms[which].colour) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_COLOUR) ) {
            tms[which].DirtyColour[i] = 1;
            tms[which].anyDirtyColour = 1;
        }

        i = (tms[which].Addr - tms[which].pattern) >> 3;
        if ( (i >= 0) && (i < MAX_DIRTY_PATTERN) ) {
            tms[which].DirtyPattern[i] = 1;
            tms[which].anyDirtyPattern = 1;
        }
    }
    tms[which].Addr = (tms[which].Addr + 1) & (tms[which].vramsize - 1);
    tms[which].ReadAhead = data;
    tms[which].latch = 0;
}


int TMS9928A_register_r(int which,int offset)
{
    UINT8 b;
    b = tms[which].StatusReg;
    tms[which].StatusReg = 0x1f;
    if (tms[which].INT) {
        tms[which].INT = 0;
        if (tms[which].INTCallback) tms[which].INTCallback (tms[which].INT);
    }
    tms[which].latch = 0;
    return b;
}

void TMS9928A_register_w(int which,int offset,int data)
{
	int reg;

    if (tms[which].latch) {
        if (data & 0x80) {
            /* register write */
			reg = data & 7;
			if (tms[which].FirstByte != tms[which].Regs[reg])
	            _TMS9928A_change_register (which, reg, tms[which].FirstByte);
        } else {
            /* set read/write address */
            tms[which].Addr = ((UINT16)data << 8 | tms[which].FirstByte) & (tms[which].vramsize - 1);
            if ( !(data & 0x40) ) {
				/* read ahead */
				TMS9928A_vram_r	(which, 0);
            }
        }
        tms[which].latch = 0;
    } else {
        tms[which].FirstByte = data;
		tms[which].latch = 1;
    }
}

static void _TMS9928A_change_register (int which, int reg, UINT8 val) {
    static const UINT8 Mask[8] =
        { 0x03, 0xfb, 0x0f, 0xff, 0x07, 0x7f, 0x07, 0xff };
    static const char *modes[] = {
        "Mode 0 (GRAPHIC 1)", "Mode 1 (TEXT 1)", "Mode 2 (GRAPHIC 2)",
        "Mode 1+2 (TEXT 1 variation)", "Mode 3 (MULTICOLOR)",
        "Mode 1+3 (BOGUS)", "Mode 2+3 (MULTICOLOR variation)",
        "Mode 1+2+3 (BOGUS)" };
    UINT8 b;
    int mode;

    val &= Mask[reg];
    tms[which].Regs[reg] = val;

    logerror("TMS9928A #%x: Reg %d = %02xh\n", which,  reg, (int)val);
    tms[which].Change = 1;
    switch (reg) {
    case 0:
        if (val & 2) {
            /* re-calculate masks and pattern generator & colour */
            if (val & 2) {
                tms[which].colour = ((tms[which].Regs[3] & 0x80) * 64) & (tms[which].vramsize - 1);
                tms[which].colourmask = (tms[which].Regs[3] & 0x7f) * 8 | 7;
                tms[which].pattern = ((tms[which].Regs[4] & 4) * 2048) & (tms[which].vramsize - 1);
                tms[which].patternmask = (tms[which].Regs[4] & 3) * 256 |
				    (tms[which].colourmask & 255);
            } else {
                tms[which].colour = (tms[which].Regs[3] * 64) & (tms[which].vramsize - 1);
                tms[which].pattern = (tms[which].Regs[4] * 2048) & (tms[which].vramsize - 1);
            }
            tms[which].mode = TMS_MODE;
            logerror("TMS9928A #%x: %s\n",which, modes[tms[which].mode]);
            _TMS9928A_set_dirty (which, 1);
        }
        break;
    case 1:
        /* check for changes in the INT line */
        b = (val & 0x20) && (tms[which].StatusReg & 0x80) ;
        if (b != tms[which].INT) {
            tms[which].INT = b;
            if (tms[which].INTCallback) tms[which].INTCallback (tms[which].INT);
        }
        mode = TMS_MODE;
        if (tms[which].mode != mode) {
            tms[which].mode = mode;
            _TMS9928A_set_dirty (which,1);
            logerror("TMS9928A #%x: %s\n", which,  modes[tms[which].mode]);
        }
        break;
    case 2:
        tms[which].nametbl = (val * 1024) & (tms[which].vramsize - 1);
        tms[which].anyDirtyName = 1;
        memset (tms[which].DirtyName, 1, MAX_DIRTY_NAME);
        break;
    case 3:
        if (tms[which].Regs[0] & 2) {
            tms[which].colour = ((val & 0x80) * 64) & (tms[which].vramsize - 1);
            tms[which].colourmask = (val & 0x7f) * 8 | 7;
         } else {
            tms[which].colour = (val * 64) & (tms[which].vramsize - 1);
        }
        tms[which].anyDirtyColour = 1;
        memset (tms[which].DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    case 4:
        if (tms[which].Regs[0] & 2) {
            tms[which].pattern = ((val & 4) * 2048) & (tms[which].vramsize - 1);
            tms[which].patternmask = (val & 3) * 256 | 255;
        } else {
            tms[which].pattern = (val * 2048) & (tms[which].vramsize - 1);
        }
        tms[which].anyDirtyPattern = 1;
        memset (tms[which].DirtyPattern, 1, MAX_DIRTY_PATTERN);
        break;
    case 5:
        tms[which].spriteattribute = (val * 128) & (tms[which].vramsize - 1);
        break;
    case 6:
        tms[which].spritepattern = (val * 2048) & (tms[which].vramsize - 1);
        break;
    case 7:
        /* The backdrop is updated at TMS9928A_refresh() */
        tms[which].anyDirtyColour = 1;
        memset (tms[which].DirtyColour, 1, MAX_DIRTY_COLOUR);
        break;
    }
}

/*
** Interface functions
*/

void TMS9928A_int_callback (int which, void (*callback)(int)) {
    tms[which].INTCallback = callback;
}

void TMS9928A_set_spriteslimit (int which, int limit) {
    tms[which].LimitSprites = limit;
}

/*
** Updates the screen (the dMem memory area).
*/
//STEVE: Need to rework palette for multi chip support? How does this work?
void TMS9928A_refresh (int which, struct osd_bitmap *bmp, int full_refresh) {
    int c;

    if (tms[which].Change) {
        c = tms[which].Regs[7] & 15; if (!c) c=1;
        if (tms[which].BackColour != c) {
            tms[which].BackColour = c;
            palette_change_color (0,
                TMS9928A_palette[c * 3], TMS9928A_palette[c * 3 + 1],
                TMS9928A_palette[c * 3 + 2]);
        }
    }

	if (palette_recalc() ) {
		_TMS9928A_set_dirty (which,1);
		tms[which].Change = 1;
	}

    if (tms[which].Change || full_refresh) {
        if (! (tms[which].Regs[1] & 0x40) ) {
            fillbitmap (bmp, Machine->pens[tms[which].BackColour],
                &Machine->visible_area);
        } else {
            if (tms[which].Change) ModeHandlers[tms[which].mode] (which, tms[which].tmpbmp);
            copybitmap (bmp, tms[which].tmpbmp, 0, 0, 0, 0,
                &Machine->visible_area, TRANSPARENCY_NONE, 0);
            if (TMS_SPRITES_ENABLED) {
                _TMS9928A_sprites (which, bmp);
            }
        }
    } else {
		tms[which].StatusReg = tms[which].oldStatusReg;
    }

    /* store Status register, so it can be restored at the next frame
       if there are no changes (sprite collision bit is lost) */
    tms[which].oldStatusReg = tms[which].StatusReg;
    tms[which].Change = 0;
	return;
}

int TMS9928A_interrupt (int which) {
    int b;

    /* when skipping frames, calculate sprite collision */
    if (osd_skip_this_frame() ) {
        if (tms[which].Change) {
            if (TMS_SPRITES_ENABLED) {
                _TMS9928A_sprites (which, NULL);
            }
        } else {
	    	tms[which].StatusReg = tms[which].oldStatusReg;
		}
    }

    tms[which].StatusReg |= 0x80;
    b = (tms[which].Regs[1] & 0x20) != 0;
    if (b != tms[which].INT) {
        tms[which].INT = b;
        if (tms[which].INTCallback) tms[which].INTCallback (tms[which].INT);
    }

    return b;
}

static void _TMS9928A_mode1 (int which, struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;
	struct rectangle rt;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    fg = Machine->pens[tms[which].Regs[7] / 16];
    bg = Machine->pens[tms[which].Regs[7] & 15];

    if (tms[which].anyDirtyColour) {
		/* colours at sides must be reset */
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 0; rt.max_x = 7;
		fillbitmap (bmp, bg, &rt);
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 248; rt.max_x = 255;
		fillbitmap (bmp, bg, &rt);
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = tms[which].vMem[tms[which].nametbl+name];
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[charcode]) &&
				!tms[which].anyDirtyColour)
                continue;
            patternptr = tms[which].vMem + tms[which].pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
		    		plot_pixel (bmp, 8+x*6+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (which,0);
}

static void _TMS9928A_mode12 (int which, struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode;
    UINT8 fg,bg,*patternptr;
	struct rectangle rt;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    fg = Machine->pens[tms[which].Regs[7] / 16];
    bg = Machine->pens[tms[which].Regs[7] & 15];

    if (tms[which].anyDirtyColour) {
		/* colours at sides must be reset */
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 0; rt.max_x = 7;
		fillbitmap (bmp, bg, &rt);
		rt.min_y = 0; rt.max_y = 191;
		rt.min_x = 248; rt.max_x = 255;
		fillbitmap (bmp, bg, &rt);
    }

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<40;x++) {
            charcode = (tms[which].vMem[tms[which].nametbl+name]+(y/8)*256)&tms[which].patternmask;
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[charcode]) &&
					!tms[which].anyDirtyColour)
                continue;
            patternptr = tms[which].vMem + tms[which].pattern + (charcode*8);
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                for (xx=0;xx<6;xx++) {
		    		plot_pixel (bmp, 8+x*6+xx, y*8+yy,
                        (pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (which, 0);
}

static void _TMS9928A_mode0 (int which, struct osd_bitmap *bmp) {
    int pattern,x,y,yy,xx,name,charcode,colour;
    UINT8 fg,bg,*patternptr;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms[which].vMem[tms[which].nametbl+name];
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[charcode] ||
                tms[which].DirtyColour[charcode/64]) )
                continue;
            patternptr = tms[which].vMem + tms[which].pattern + charcode*8;
            colour = tms[which].vMem[tms[which].colour+charcode/8];
            fg = Machine->pens[colour / 16];
            bg = Machine->pens[colour & 15];
            for (yy=0;yy<8;yy++) {
                pattern=*patternptr++;
                for (xx=0;xx<8;xx++) {
		    		plot_pixel (bmp, x*8+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (which,0);
}

static void _TMS9928A_mode2 (int which, struct osd_bitmap *bmp) {
    int colour,name,x,y,yy,pattern,xx,charcode;
    UINT8 fg,bg;
    UINT8 *colourptr,*patternptr;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms[which].vMem[tms[which].nametbl+name]+(y/8)*256;
            colour = (charcode&tms[which].colourmask);
            pattern = (charcode&tms[which].patternmask);
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[pattern] ||
                tms[which].DirtyColour[colour]) )
                continue;
            patternptr = tms[which].vMem+tms[which].pattern+colour*8;
            colourptr = tms[which].vMem+tms[which].colour+pattern*8;
            for (yy=0;yy<8;yy++) {
                pattern = *patternptr++;
                colour = *colourptr++;
                fg = Machine->pens[colour / 16];
                bg = Machine->pens[colour & 15];
                for (xx=0;xx<8;xx++) {
		    		plot_pixel (bmp, x*8+xx, y*8+yy,
						(pattern & 0x80) ? fg : bg);
                    pattern *= 2;
                }
            }
        }
    }
    _TMS9928A_set_dirty (which,0);
}

static void _TMS9928A_mode3 (int which, struct osd_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms[which].vMem[tms[which].nametbl+name];
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[charcode]) &&
					!tms[which].anyDirtyColour)
                continue;
            patternptr = tms[which].vMem+tms[which].pattern+charcode*8+(y&3)*2;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
		    plot_pixel (bmp, x*8+0, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+1, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+2, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+3, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+4, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+5, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+6, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+7, y*8+yy*4+yyy, bg);
                }
            }
        }
    }
    _TMS9928A_set_dirty (which,0);
}

static void _TMS9928A_mode23 (int which, struct osd_bitmap *bmp) {
    int x,y,yy,yyy,name,charcode;
    UINT8 fg,bg,*patternptr;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    name = 0;
    for (y=0;y<24;y++) {
        for (x=0;x<32;x++) {
            charcode = tms[which].vMem[tms[which].nametbl+name];
            if ( !(tms[which].DirtyName[name++] || tms[which].DirtyPattern[charcode]) &&
		!tms[which].anyDirtyColour)
                continue;
            patternptr = tms[which].vMem + tms[which].pattern +
                ((charcode+(y&3)*2+(y/8)*256)&tms[which].patternmask)*8;
            for (yy=0;yy<2;yy++) {
                fg = Machine->pens[(*patternptr / 16)];
                bg = Machine->pens[((*patternptr++) & 15)];
                for (yyy=0;yyy<4;yyy++) {
		    plot_pixel (bmp, x*8+0, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+1, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+2, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+3, y*8+yy*4+yyy, fg);
		    plot_pixel (bmp, x*8+4, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+5, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+6, y*8+yy*4+yyy, bg);
		    plot_pixel (bmp, x*8+7, y*8+yy*4+yyy, bg);
                }
            }
        }
    }
    _TMS9928A_set_dirty (which,0);
}

static void _TMS9928A_modebogus (int which, struct osd_bitmap *bmp) {
    UINT8 fg,bg;
    int x,y,n,xx;

    if ( !(tms[which].anyDirtyColour || tms[which].anyDirtyName || tms[which].anyDirtyPattern) )
         return;

    fg = Machine->pens[tms[which].Regs[7] / 16];
    bg = Machine->pens[tms[which].Regs[7] & 15];

    for (y=0;y<192;y++) {
        xx=0;
        n=8; while (n--) plot_pixel (bmp, xx++, y, bg);
        for (x=0;x<40;x++) {
            n=4; while (n--) plot_pixel (bmp, xx++, y, fg);
            n=2; while (n--) plot_pixel (bmp, xx++, y, bg);
        }
        n=8; while (n--) plot_pixel (bmp, xx++, y, bg);
    }

    _TMS9928A_set_dirty (which,0);
}

/*
** This function renders the sprites. Sprite collision is calculated in
** in a back buffer (tms.dBackMem), because sprite collision detection
** is rather complicated (transparent sprites also cause the sprite
** collision bit to be set, and ``illegal'' sprites do not count
** (they're not displayed)).
**
** This code should be optimized. One day.
*/
static void _TMS9928A_sprites (int which, struct osd_bitmap *bmp) {
    UINT8 *attributeptr,*patternptr,c;
    int p,x,y,size,i,j,large,yy,xx,limit[192],
        illegalsprite,illegalspriteline;
    UINT16 line,line2;

    attributeptr = tms[which].vMem + tms[which].spriteattribute;
    size = (tms[which].Regs[1] & 2) ? 16 : 8;
    large = (int)(tms[which].Regs[1] & 1);

    for (x=0;x<192;x++) limit[x] = 4;
    tms[which].StatusReg = 0x80;
    illegalspriteline = 255;
    illegalsprite = 0;

    memset (tms[which].dBackMem, 0, IMAGE_SIZE);
    for (p=0;p<32;p++) {
        y = *attributeptr++;
        if (y == 208) break;
        if (y > 208) {
            y=-(~y&255);
        } else {
            y++;
        }
        x = *attributeptr++;
        patternptr = tms[which].vMem + tms[which].spritepattern +
            ((size == 16) ? *attributeptr & 0xfc : *attributeptr) * 8;
        attributeptr++;
        c = (*attributeptr & 0x0f);
        if (*attributeptr & 0x80) x -= 32;
        attributeptr++;

        if (!large) {
            /* draw sprite (not enlarged) */
            for (yy=y;yy<(y+size);yy++) {
                if ( (yy < 0) || (yy > 191) ) continue;
                if (limit[yy] == 0) {
                    /* illegal sprite line */
                    if (yy < illegalspriteline) {
                        illegalspriteline = yy;
                        illegalsprite = p;
                    } else if (illegalspriteline == yy) {
                        if (illegalsprite > p) {
                            illegalsprite = p;
                        }
                    }
                    if (tms[which].LimitSprites) continue;
                } else limit[yy]--;
                line = 256*patternptr[yy-y] + patternptr[yy-y+16];
                for (xx=x;xx<(x+size);xx++) {
                    if (line & 0x8000) {
                        if ((xx >= 0) && (xx < 256)) {
                            if (tms[which].dBackMem[yy*256+xx]) {
                                tms[which].StatusReg |= 0x20;
                            } else {
                                tms[which].dBackMem[yy*256+xx] = 0x01;
                            }
                            if (c && ! (tms[which].dBackMem[yy*256+xx] & 0x02))
                            {
                            	tms[which].dBackMem[yy*256+xx] |= 0x02;
                            	if (bmp)
									plot_pixel (bmp, xx, yy, Machine->pens[c]);
							}
                        }
                    }
                    line *= 2;
                }
            }
        } else {
            /* draw enlarged sprite */
            for (i=0;i<size;i++) {
                yy=y+i*2;
                line2 = 256*patternptr[i] + patternptr[i+16];
                for (j=0;j<2;j++) {
                    if ( (yy >= 0) && (yy <= 191) ) {
                        if (limit[yy] == 0) {
                            /* illegal sprite line */
                            if (yy < illegalspriteline) {
                                illegalspriteline = yy;
                                 illegalsprite = p;
                            } else if (illegalspriteline == yy) {
                                if (illegalsprite > p) {
                                    illegalsprite = p;
                                }
                            }
                            if (tms[which].LimitSprites) continue;
                        } else limit[yy]--;
                        line = line2;
                        for (xx=x;xx<(x+size*2);xx+=2) {
                            if (line & 0x8000) {
                                if ((xx >=0) && (xx < 256)) {
                                    if (tms[which].dBackMem[yy*256+xx]) {
                                        tms[which].StatusReg |= 0x20;
                                    } else {
                                        tms[which].dBackMem[yy*256+xx] = 0x01;
                                    }
		                            if (c && ! (tms[which].dBackMem[yy*256+xx] & 0x02))
        		                    {
                		            	tms[which].dBackMem[yy*256+xx] |= 0x02;
                                        if (bmp)
                                        	plot_pixel (bmp, xx, yy, Machine->pens[c]);
                		            }
                                }
                                if (((xx+1) >=0) && ((xx+1) < 256)) {
                                    if (tms[which].dBackMem[yy*256+xx+1]) {
                                        tms[which].StatusReg |= 0x20;
                                    } else {
                                        tms[which].dBackMem[yy*256+xx+1] = 0x01;
                                    }
		                            if (c && ! (tms[which].dBackMem[yy*256+xx+1] & 0x02))
        		                    {
                		            	tms[which].dBackMem[yy*256+xx+1] |= 0x02;
                                        if (bmp)
                                        	plot_pixel (bmp, xx+1, yy, Machine->pens[c]);
									}
                                }
                            }
                            line *= 2;
                        }
                    }
                    yy++;
                }
            }
        }
    }
    if (illegalspriteline == 255) {
        tms[which].StatusReg |= (p > 31) ? 31 : p;
    } else {
        tms[which].StatusReg |= 0x40 + illegalsprite;
    }
}

/* I/O Routetines */
READ_HANDLER (TMS9928A_vram_0_r)		{ return TMS9928A_vram_r(0,offset); }
WRITE_HANDLER (TMS9928A_vram_0_w)		{ TMS9928A_vram_w(0,offset,data); }
READ_HANDLER (TMS9928A_register_0_r)	{ return TMS9928A_register_r(0,offset); }
WRITE_HANDLER (TMS9928A_register_0_w)	{ TMS9928A_register_w(0,offset,data); }
READ_HANDLER (TMS9928A_vram_1_r)		{ return TMS9928A_vram_r(1,offset); }
WRITE_HANDLER (TMS9928A_vram_1_w)		{ TMS9928A_vram_w(1,offset,data); }
READ_HANDLER (TMS9928A_register_1_r)	{ return TMS9928A_register_r(1,offset); }
WRITE_HANDLER (TMS9928A_register_1_w)	{ TMS9928A_register_w(1,offset,data); }