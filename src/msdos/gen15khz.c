#include "mamalleg.h"
#include "driver.h"
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include <math.h>
#include "TwkUser.h"
#include "gen15khz.h"
#include "ati15khz.h"

/* from video.c */
void center_mode(Register *pReg);


int	wait_interlace;      /* config flag - indicates if we're waiting for odd/even updates */
static int display_interlaced=0;  /* interlaced display */

extern int video_sync;
extern int wait_vsync;
extern int use_triplebuf;
extern int unchained;

/* Generic SVGA 15.75KHz code and SVGA 15.75KHz driver selection */

/* To add a SVGA 15.75KHz driver for a given chipset you need to write (at least) */
/* 4 functions and put them into an entry in the array below. */
/* The 4 functions are as follows */

/* 1) 'Detect' -  checks for your chipset and returns 1 if found ,  0 if not */

/* 2) 'Width' - returns the logical width of the mode created */
/* This is to allow for drivers (such as the generic one) which generate a mode with a very wide */
/* scanline to avoid having to reprogram the card's clock */

/* 3) 'Setmode' - reprograms your chipset's clock and CRTC timing registers so that */
/* it produces a signal with a horizontal scanrate of 15.75KHz and a vertical refresh rate of 60Hz. */
/* (possibly producing an interlaced image if the chipset supports it) */
/* NOTE: 'vdouble' is passed into the function to indicate if MAME is doubling the */
/* height of the screen bitmap */
/* If it is - you can set your 15.75KHz mode to drop every other scanline */
/* and avoid having to interlace the display */
/* (see calc_mach64_height in ati15khz.c) */
/* Function should return 1 if it managed to set the 15.75KHz mode, 0 if it didn't */
/* It should also call 'setinterlaceflag(<n>)'; with a value of 1 or 0 to indicate if */
/* it setup an interlaced display or not. */
/* This is used to determine if the refresh needs to be delayed while both odd and even fields */
/* are drawn (if requested with -waitinterlace flag) */

/* 4) 'Reset' - restores any BIOS settings/tables you may have changed whilst setting up the 15.75KHz mode */

/* The Driver should : */
/* .support 640x480 at 60Hz */
/* .should use center_x and center_y as offsets for your horizontal and vertical retrace start/end values */
/* .should use tw640x480arc_h , tw640x480arc_v for h and v timing values */

/* Sources for information about programming SVGA chipsets- */
/* VGADOC4B.ZIP - from any SimTel site */
/* XFree86 source code */
/* the Allegro library */



/* our array of 15.75KHz drivers */
/* NOTE: Generic *MUST* be the last in the list as its */
/* detect function will always return 1 */
SVGA15KHZDRIVER drivers15KHz[]=
{
	{"ATI", detectati, widthati15KHz, setati15KHz, resetati15KHz},   /* ATI driver */
	{"Generic", genericsvga, widthgeneric15KHz, setgeneric15KHz, resetgeneric15KHz}   /* Generic driver (must be last in list) */
};

/* array of CRTC settings for Generic SVGA 15.75KHz modes */
/* Note - most of these values will be calculated at runtime, */
/* only the H/V totals and resync starts are really important */
/* 640x480 */
static Register scr640x480_15KHz[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0xc1},{ 0x3d4, 0x01, 0x9f},
	{ 0x3d4, 0x02, 0x9f},{ 0x3d4, 0x03, 0x91},{ 0x3d4, 0x04, 0x9f},
	{ 0x3d4, 0x05, 0x1f},{ 0x3d4, 0x06, 0x09},{ 0x3d4, 0x07, 0x11},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x40},{ 0x3d4, 0x10, 0xf2},
	{ 0x3d4, 0x11, 0x46},{ 0x3d4, 0x12, 0xef},{ 0x3d4, 0x13, 0x00},
	{ 0x3d4, 0x15, 0xef},{ 0x3d4, 0x16, 0x04}
};


/* seletct 15.75KHz driver */
int getSVGA15KHzdriver(SVGA15KHZDRIVER **driver15KHz)
{
	int n,nodrivers;
/* setup some defaults */
	*driver15KHz=NULL;
	display_interlaced = 0;
	nodrivers = sizeof(drivers15KHz) / sizeof(SVGA15KHZDRIVER);

/* then find a driver */
	for (n=0; n<nodrivers; n++)
	{
		if (drivers15KHz[n].detectsvgacard())
		{
			*driver15KHz = &drivers15KHz[n];
			return 1;
		}
	}
	return 0;
}

/* generic SVGA detect, always returns true and turns off SVGA triple buffering */
int genericsvga()
{
	use_triplebuf = 0;
	return 1;
}

/* generic SVGA logical width */
/* to make the most compatible mode possible - */
/* the generic driver's 640x480 mode is actually a 1280x240 mode that draws every other scanline */
/* this means the scanrate can be lowered to arcade monitor rates without altering the card's dot clock */
int widthgeneric15KHz(int width)
{
	return width << 1;
}

/* set 15.75KHz SVGA mode using just standard VGA registers + 1 VESA function */
/* may or may *NOT* work with a given chipset */
int setgeneric15KHz(int vdouble, int width, int height)
{
	int reglen,hcount;
	Register *scrSVGA_15KHz;
	__dpmi_regs	_dpmi_reg;
/* check the mode's supported */
	if (!sup_15Khz_res(width,height))
		return 0;
/* now check we support the character count */

	hcount = readCRTC(HZ_DISPLAY_END);

	switch(hcount)
	{
		case 79:	/* 640 */
			scrSVGA_15KHz = scr640x480_15KHz;
			reglen = (sizeof(scr640x480_15KHz) / sizeof(Register));
// set the tweak values */
  			scr640x480_15KHz[H_TOTAL_INDEX].value = tw640x480arc_h;
			scr640x480_15KHz[V_TOTAL_INDEX].value = tw640x480arc_v;
			break;
		default:
			logerror("15.75KHz: Unsupported %dx%d mode (%d char. clocks)\n", width, height, hcount);
			return 0;
	}
/* setup our adjusted memory offset */
/* first check if we can do it using VESA, as this gives us more flexibility than standard VGA */
/* Function 6 - Get/Set Scan Line Length, VESA 1.1 and up */
	_dpmi_reg.x.cx = 0;
	_dpmi_reg.x.ax = 0x4F06;
	_dpmi_reg.x.bx = 1;
	__dpmi_int(0x10, &_dpmi_reg);
/*did we get anything back ? */
	if (_dpmi_reg.x.cx)
/* we did, so just set the memory offset in the VGA array to whatever the mode claims to use */
/* and we'll set it using VESA after the outRegArray */
		scrSVGA_15KHz[MEM_OFFSET_INDEX].value = readCRTC (MEM_OFFSET);
	else
/* we didn't, so attempt to use the memory offset in the VGA array directly */
		scrSVGA_15KHz[MEM_OFFSET_INDEX].value = (readCRTC (MEM_OFFSET) * 2);

/* indicate we're not interlacing */
	setinterlaceflag (0);
/* center the display */
  	center_mode (scrSVGA_15KHz);
/* write out the array */
	outRegArray (scrSVGA_15KHz,reglen);

/* did the VESA call work ?, if it did use VESA to set our memory offset */
	if (_dpmi_reg.x.cx)
	{
		_dpmi_reg.x.ax = 0x4F06;
		_dpmi_reg.x.bx = 0;
		_dpmi_reg.x.cx <<= 1;
		__dpmi_int(0x10, &_dpmi_reg);
	}

	return 1;
}

/* generic cleanup - do nothing as we only changed values in the standard VGA register set */
void resetgeneric15KHz()
{
	return;
}

/* set our interlaced flag */
void setinterlaceflag(int interlaced)
{
	display_interlaced = interlaced;
	if (display_interlaced && wait_interlace)
	{
		/* make sure we're going to hit around 30FPS */
		video_sync = 0;
		wait_vsync = 0;
	}
}

/* function to delay for 2 screen updates */
/* allows complete odd/even field update of an interlaced display */
void interlace_sync()
{
	/* check it's been asked for, and that the display is actually interlaced */
	if (wait_interlace && display_interlaced)
	{
		/* make sure we get 2 vertical retraces */
		vsync();
		vsync();
	}
}


/* read a CRTC register at a given index */
int readCRTC(int nIndex)
{
	outportb (0x3d4, nIndex);
	return inportb (0x3d5);
}

/* Get vertical display height */
/* -it's stored over 3 registers */
int getVtEndDisplay()
{
	int   nVert;
/* get first 8 bits */
	nVert = readCRTC (VT_DISPLAY_END);
/* get 9th bit */
	nVert |= (readCRTC (CRTC_OVERFLOW) & 0x02) << 7;
/* and the 10th */
	nVert |= (readCRTC (CRTC_OVERFLOW) & 0x40) << 3;
	return nVert;
}


int sup_15Khz_res(int width,int height)
{
/* 640x480 */
	if (width == 640 && height == 480)
		return 1;
/* 800x600 */
/*
	if(width==800&&height==600)
		return 1;
*/
	logerror("15.75KHz: Unsupported %dx%d mode\n", width, height);
	return 0;
}


