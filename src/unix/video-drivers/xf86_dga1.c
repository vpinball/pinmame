/*
 *	XFree86 VidMode and DGA support by Jens Vaasjo <jvaasjo@iname.com>
 */
#ifdef USE_DGA
#define __XF86_DGA_C

#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#endif
#include "driver.h"
#include "xmame.h"
#include "x11.h"

#ifdef USE_DGA

static void xf86_dga_update_display_16_to_16bpp(struct mame_bitmap *bitmap);
static void xf86_dga_update_display_16_to_24bpp(struct mame_bitmap *bitmap);
static void xf86_dga_update_display_16_to_32bpp(struct mame_bitmap *bitmap);
static void xf86_dga_update_display_32_to_32bpp_direct(struct mame_bitmap *bitmap);

static struct
{
	int screen;
	unsigned char *addr;
	int grabbed_keybd;
	int grabbed_mouse;
	int old_grab_mouse;
	char *base_addr;
	int width;
	int bank_size;
	int ram_size;
	Colormap cmap;
	void (*xf86_dga_update_display_func)(struct mame_bitmap *bitmap);
	XF86VidModeModeInfo orig_mode;
	int vidmode_changed;
	int palette_dirty;
} xf86ctx = {-1,NULL,FALSE,FALSE,FALSE,NULL,-1,-1,-1,0,NULL,{0},FALSE,FALSE};
		
static unsigned char *doublebuffer_buffer = NULL;


int xf86_dga1_init(void)
{
	int i;
	char *s;
	
	mode_available[X11_DGA] = FALSE;
	xf86ctx.screen          = DefaultScreen(display);
	
	
	if(geteuid())
		fprintf(stderr,"DGA requires root rights\n");
	else if (!(s = getenv("DISPLAY")) || (s[0] != ':'))
		fprintf(stderr,"DGA only works on a local display\n");
	else if(!XF86DGAQueryVersion(display, &i, &i))
		fprintf(stderr,"XF86DGAQueryVersion failed\n");
	else if(!XF86DGAQueryExtension(display, &i, &i))
		fprintf(stderr,"XF86DGAQueryExtension failed\n");
	else if(!XF86DGAQueryDirectVideo(display, xf86ctx.screen, &i))
		fprintf(stderr,"XF86DGAQueryDirectVideo failed\n");
	else if(!(i & XF86DGADirectPresent))
		fprintf(stderr,"XF86DGADirectVideo support is not present\n");
	else if(!XF86DGAGetVideo(display,xf86ctx.screen,
		 &xf86ctx.base_addr,&xf86ctx.width,
		 &xf86ctx.bank_size,&xf86ctx.ram_size))
		fprintf(stderr,"XF86DGAGetVideo failed\n");
	else
		mode_available[X11_DGA] = TRUE; 
		
	if (!mode_available[X11_DGA])
		fprintf(stderr,"Use of DGA-modes is disabled\n");

	return OSD_OK;
}

int xf86_dga1_16bpp_capable(void)
{
   int screen_no = DefaultScreen (display);
   return (DefaultDepth(display, screen_no) >= 15);
}

static int xf86_dga_vidmode_check_exts(void)
{
	int major,minor,event_base,error_base;

	if(!XF86VidModeQueryVersion(display,&major,&minor))
	{
		fprintf(stderr_file,"XF86VidModeQueryVersion failed\n");
		return OSD_NOT_OK;
	}

	if(!XF86VidModeQueryExtension(display,&event_base,&error_base))
	{
		fprintf(stderr_file,"XF86VidModeQueryExtension failed\n");
		return OSD_NOT_OK;
	}

	return OSD_OK;
}

static XF86VidModeModeInfo *xf86_dga_vidmode_find_best_vidmode(int depth)
{
	XF86VidModeModeInfo **modes,*bestmode = NULL;
	int score, best_score = 0;
	int i,modecount = 0;

	if(!XF86VidModeGetAllModeLines(display,xf86ctx.screen,
						&modecount,&modes))
	{
		fprintf(stderr_file,"XF86VidModeGetAllModeLines failed\n");
		return NULL;
	}
	
	fprintf(stderr, "XF86DGA: info: found %d modes:\n", modecount);

	for(i=0;i<modecount;i++)
	{
		if (mode_disabled(modes[i]->hdisplay, modes[i]->vdisplay, depth))
			continue;
		fprintf(stderr, "XF86DGA: info: found mode: %dx%d\n",
		   modes[i]->hdisplay, modes[i]->vdisplay);
		/* ignore modes with a width which is not 64 bit aligned */
		if(modes[i]->hdisplay & 7) continue;
		
		score = mode_match(modes[i]->hdisplay, modes[i]->vdisplay);
		if(score > best_score)
		{
			best_score = score;
			bestmode   = modes[i];
		}
	}

	return bestmode;
}

static Bool xf86_dga_vidmode_getmodeinfo(XF86VidModeModeInfo *modeinfo)
{
	XF86VidModeModeLine modeline;
	int dotclock;
	Bool err;

	err = XF86VidModeGetModeLine(display,xf86ctx.screen,
					&dotclock,&modeline);

	modeinfo->dotclock = dotclock;
	modeinfo->hdisplay = modeline.hdisplay;
	modeinfo->hsyncstart = modeline.hsyncstart;
	modeinfo->hsyncend = modeline.hsyncend;
	modeinfo->htotal = modeline.htotal;
	modeinfo->vdisplay = modeline.vdisplay;
	modeinfo->vsyncstart = modeline.vsyncstart;
	modeinfo->vsyncend = modeline.vsyncend;
	modeinfo->vtotal = modeline.vtotal;
	modeinfo->flags = modeline.flags;
	modeinfo->privsize = modeline.privsize;
	modeinfo->private = modeline.private;

	return err;
}

static void xf86_dga_vidmode_restoremode(Display *disp)
{
	XF86VidModeSwitchToMode(disp, xf86ctx.screen, &xf86ctx.orig_mode);
	/* 'Mach64-hack': restores screen when screwed up */
	XF86VidModeSwitchMode(disp,xf86ctx.screen,-1);
	XF86VidModeSwitchMode(disp,xf86ctx.screen,1);
	/**************************************************/
	XSync(disp,False);
}

static int xf86_dga_vidmode_setup_mode_restore(void)
{
	Display *disp;
	int status;
	pid_t pid;

	if(!xf86_dga_vidmode_getmodeinfo(&xf86ctx.orig_mode))
	{
		fprintf(stderr_file,"XF86VidModeGetModeLine failed\n");
		return OSD_NOT_OK;
	}

	pid = fork();
	if(pid > 0)
	{
		waitpid(pid,&status,0);
		disp = XOpenDisplay(NULL);
		xf86_dga_vidmode_restoremode(disp);
		XCloseDisplay(disp);
		_exit(!WIFEXITED(status));
	}

	if (pid < 0)
	{
		perror("fork");
		return OSD_NOT_OK;
	}

	return OSD_OK;
}

int xf86_dga1_alloc_palette(int writable_colors)
{
	XColor color;
	int i;

	/* this is only relevant for 8bpp displays */
	if (depth != 8)
	   return 0;

	xf86ctx.cmap = XCreateColormap(display,window,xvisual,AllocAll);

	for(i=0;i<writable_colors;i++)
	{
		color.pixel = i;
		color.red   = 0;
		color.green = 0;
		color.blue  = 0;
		color.flags = DoRed | DoGreen | DoBlue;

		XStoreColor(display,xf86ctx.cmap,&color);
	}
	return 0;
}

static int xf86_dga_setup_graphics(XF86VidModeModeInfo *modeinfo, int bitmap_depth)
{
	int sizeof_pixel;
	
	if(xf86ctx.bank_size != (xf86ctx.ram_size * 1024))
	{
		fprintf(stderr_file,"banked graphics modes not supported\n");
		return OSD_NOT_OK;
	}

	if (bitmap_depth == 32)
	{
	    if (depth == 32) 
	    {
		xf86ctx.xf86_dga_update_display_func =
			xf86_dga_update_display_32_to_32bpp_direct;
	    }
	}
	else if (bitmap_depth == 16)
	{
	    switch(depth)
	    {
		case 16:
			xf86ctx.xf86_dga_update_display_func =
				xf86_dga_update_display_16_to_16bpp;
			break;
		case 24:
			xf86ctx.xf86_dga_update_display_func =
				xf86_dga_update_display_16_to_24bpp;
			break;
		case 32:
			xf86ctx.xf86_dga_update_display_func =
				xf86_dga_update_display_16_to_32bpp;
			break;
	    }
	}
	
	if (xf86ctx.xf86_dga_update_display_func == NULL)
	{
		fprintf(stderr_file, "unsupported depth %dbpp\n",depth);
		return OSD_NOT_OK;
	}
	
	fprintf(stderr_file, "XF86-DGA1 running at: %dbpp\n", depth);
	
	sizeof_pixel  = depth / 8;

	xf86ctx.addr  = (unsigned char*)xf86ctx.base_addr;
	xf86ctx.addr += (((modeinfo->hdisplay - visual_width*widthscale) / 2) & ~7)
						* sizeof_pixel;
	if (yarbsize)
	  xf86ctx.addr += ((modeinfo->vdisplay - yarbsize) / 2)
	    * xf86ctx.width * sizeof_pixel;
	else
	  xf86ctx.addr += ((modeinfo->vdisplay - visual_height*heightscale) / 2)
	    * xf86ctx.width * sizeof_pixel;

	return OSD_OK;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int xf86_dga1_create_display(int bitmap_depth)
{
	int i, count;
	XPixmapFormatValues *pixmaps;
	XF86VidModeModeInfo *bestmode;
	/* only have todo the fork's the first time we go DGA, otherwise people
	   who do a lott of dga <-> window switching will get a lott of
	   children */
	static int first_time  = 1;
	xf86_dga_fix_viewport  = 0;
	xf86_dga_first_click   = 1;
	xf86ctx.palette_dirty  = FALSE;
	xf86ctx.old_grab_mouse = x11_grab_mouse;
	x11_grab_mouse         = FALSE;
	
	xvisual = DefaultVisual(display,xf86ctx.screen);
	window  = RootWindow(display,xf86ctx.screen);
	/* dirty hack 24bpp can be either 24bpp packed or 32 bpp sparse */
	pixmaps = XListPixmapFormats(display, &count);
	if (!pixmaps)
	{
	   fprintf(stderr_file, "X11-Error: Couldn't list pixmap formats.\n"
	      "Probably out of memory.\n");
	   return OSD_NOT_OK;
	}
        for(i=0; i<count; i++)
        {
           if(pixmaps[i].depth==DefaultDepth(display,xf86ctx.screen))
           {
	      depth = pixmaps[i].bits_per_pixel;
	      break;
           }  
        }
        if(i==count)
        {
           fprintf(stderr_file, "Couldn't find a zpixmap with the defaultcolordepth\nThis should not happen!\n");
           return OSD_NOT_OK;
        }
        XFree(pixmaps);
        
	/* setup the palette_info struct now we have the depth */
	if (x11_init_palette_info() != OSD_OK)
	    return OSD_NOT_OK;
        
        if (widthscale != 1 || heightscale != 1 ||
	    yarbsize > visual_height)
        {
	   doublebuffer_buffer = malloc (visual_width * widthscale * depth / 8);
	   if (doublebuffer_buffer == NULL)
	   {
	      fprintf(stderr, "Error: Couldn't alloc enough memory\n");
	      return OSD_NOT_OK;
	   }
        }

	if(xf86_dga_vidmode_check_exts())
		return OSD_NOT_OK;

	bestmode = xf86_dga_vidmode_find_best_vidmode(bitmap_depth);
	if(!bestmode)
	{
		fprintf(stderr_file,"no suitable mode found\n");
		return OSD_NOT_OK;
	}

	if(xf86_dga_setup_graphics(bestmode, bitmap_depth))
		return OSD_NOT_OK;
	
	if (first_time)
	{
		if(xf86_dga_vidmode_setup_mode_restore())
			return OSD_NOT_OK;
	}

	fprintf(stderr_file,"VidMode Switching To Mode: %d x %d\n",
			bestmode->hdisplay,bestmode->vdisplay);

	if(!XF86VidModeSwitchToMode(display,xf86ctx.screen,bestmode))
	{
		fprintf(stderr_file,"XF86VidModeSwitchToMode failed\n");
		return OSD_NOT_OK;
	}
	xf86ctx.vidmode_changed = TRUE;

	if(XGrabKeyboard(display,window,True,
		GrabModeAsync,GrabModeAsync,CurrentTime))
	{
		fprintf(stderr_file,"XGrabKeyboard failed\n");
		return OSD_NOT_OK;
	}
	xf86ctx.grabbed_keybd = 1;

	if(use_mouse)
	{
		if(XGrabPointer(display,window,True,
			PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
			GrabModeAsync,GrabModeAsync,None,None,CurrentTime))
		{
			fprintf(stderr_file, "XGrabPointer failed, mouse disabled\n");
			use_mouse = 0;
		}
		else
			xf86ctx.grabbed_mouse = 1;
	}

	if(first_time)
	{
		if(XF86DGAForkApp(xf86ctx.screen))
		{
			perror("fork");
			return OSD_NOT_OK;
		}
		first_time = 0;
	}

	if(!XF86DGADirectVideo(display,xf86ctx.screen,
		XF86DGADirectGraphics|XF86DGADirectMouse|XF86DGADirectKeyb))
	{
		fprintf(stderr_file,"XF86DGADirectVideo failed\n");
		return OSD_NOT_OK;
	}

	if(!XF86DGASetViewPort(display,xf86ctx.screen,0,0))
	{
		fprintf(stderr_file,"XF86DGASetViewPort failed\n");
		return OSD_NOT_OK;
	}

	memset(xf86ctx.base_addr,0,xf86ctx.bank_size);

	effect_init2(bitmap_depth, depth, xf86ctx.width);
	
	return OSD_OK;
}


int xf86_dga1_modify_pen(int pen,
	unsigned char red,unsigned char green,unsigned char blue)
{
	XColor color;
	color.pixel = pen;
	color.red   = red   << 8;
	color.green = green << 8;
	color.blue  = blue  << 8;
	color.flags = DoRed | DoGreen | DoBlue;

	XStoreColor(display,xf86ctx.cmap,&color);
	xf86ctx.palette_dirty = TRUE;
	return 0;
}

#define DEST xf86ctx.addr
#define DEST_WIDTH xf86ctx.width
#define SRC_PIXEL unsigned short
/* Use double buffering where it speeds things up */
#define DOUBLEBUFFER

#define INDIRECT current_palette->lookup

static void xf86_dga_update_display_16_to_16bpp(struct mame_bitmap *bitmap)
{
#define DEST_PIXEL unsigned short
   if (current_palette->lookup)
   {
#include "blit.h"
   }
   else
   {
#undef  INDIRECT
#include "blit.h"
#define INDIRECT current_palette->lookup
   }
#undef DEST_PIXEL
}

#define DEST_PIXEL unsigned int

static void xf86_dga_update_display_16_to_24bpp(struct mame_bitmap *bitmap)
{
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
}

static void xf86_dga_update_display_16_to_32bpp(struct mame_bitmap *bitmap)
{
#include "blit.h"
}

#undef  INDIRECT
#undef  SRC_PIXEL
#define SRC_PIXEL unsigned int

static void xf86_dga_update_display_32_to_32bpp_direct(struct mame_bitmap *bitmap)
{
#include "blit.h"
}

#undef DEST_PIXEL

void xf86_dga1_update_display(struct mame_bitmap *bitmap)
{
	if(xf86_dga_fix_viewport)
	{
		XF86DGASetViewPort(display,xf86ctx.screen,0,0);
		xf86_dga_fix_viewport = 0;
	}
	
	(*xf86ctx.xf86_dga_update_display_func)(bitmap);
}

void xf86_dga1_close_display(void)
{
	if(doublebuffer_buffer)
	{
		free(doublebuffer_buffer);
		doublebuffer_buffer = NULL;
	}
	if(xf86ctx.cmap)
	{
		XFreeColormap(display,xf86ctx.cmap);
		xf86ctx.cmap = 0;
	}
	if(xf86ctx.grabbed_mouse)
	{
		XUngrabPointer(display,CurrentTime);
		xf86ctx.grabbed_mouse = FALSE;
	}
	if(xf86ctx.grabbed_keybd)
	{
		XUngrabKeyboard(display,CurrentTime);
		xf86ctx.grabbed_keybd = FALSE;
	}
	XF86DGADirectVideo(display,xf86ctx.screen, 0);
	if(xf86ctx.vidmode_changed)
	{
		xf86_dga_vidmode_restoremode(display);
		xf86ctx.vidmode_changed = FALSE;
	}
	x11_grab_mouse = xf86ctx.old_grab_mouse;
}

#endif /*def USE_DGA*/
