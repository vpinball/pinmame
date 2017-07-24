/* MAME Photon 2 Overlay Code
 *
 * Writen By: Travis Coady
 * Origional Code By: David Rempel
 *
 * web: http://www.classicgaming.com/phmame/
 * e-mail: smallfri@bigfoot.com
 *
 * Copyright (C) 2000-2001, The PhMAME Developement Team.
*/

/* TRAVIS'S NOTE: This needs an overhaul. */

/*
 * Include files.
 */

/* for FLT_MAX */
#include <float.h>
#include <Ph.h>
#include <Pt.h>
#include "xmame.h"
#include "photon2.h"
#include "driver.h"
//#include "phkeyboard.h"
static void ph_ovr_update_16_to_16bpp (struct mame_bitmap *bitmap);
static void ph_ovr_update_16_to_24bpp (struct mame_bitmap *bitmap);
static void ph_ovr_update_16_to_32bpp (struct mame_bitmap *bitmap);
static void ph_window_update_32_to_32bpp_direct (struct mame_bitmap *bitmap);
static void (*ph_ovr_update_display_func) (struct mame_bitmap *bitmap) = NULL;

/* hmm we need these to do the clean up correctly, or we could just 
   trust unix & X to clean up after us but lett's keep things clean */

static int private_cmap_allocated = 0;
static PdOffscreenContext_t *image = NULL;
static int orig_widthscale, orig_heightscale;
enum { PH_NORMAL };
PhDim_t  view_size;
static int ph_ovr_update_method = PH_NORMAL;
static int startx = 0;
static int starty = 0;
static unsigned long black_pen;
static int use_xsync = 0;
static int root_window_id; /* root window id (for swallowing the mame window) */
static char *geometry = NULL;
static float scrnaspect,vscrnaspect;
static float vscrntlx;
static float vscrntly;
static float vscrnwidth;
static float vscrnheight;
static float vw,vh;
static char *pseudo_color_allocated;
static unsigned long *pseudo_color_lookup;
static int pseudo_color_lookup_dirty;
static int pseudo_color_use_rw_palette;
static int pseudo_color_warn_low_on_colors;

static PgVideoChannel_t *channel;
static PgScalerCaps_t   caps;
static PgScalerProps_t  props;

static unsigned char   *ybuf0, *ybuf1;
static unsigned char   *ubuf0, *ubuf1;
static unsigned char   *vbuf0, *vbuf1;

static int swidth ;
static int sheight;
static int dwidth ;
static int dheight;

static int overlay_on=0;

extern int I_GetEvent(PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo );

extern int phovr_filteron;

int ph_ovr_16bpp_capable(void)
{
	return 1;
}

/* I've commented out the fprintf's in this function.
   It'll keep the compiler quiet. I'm not exacally
   sure why there even here (debuggin?), but the Raw
   Drawing & Animation Chapter in the docs gives me a
   good idea <evil grin at dave>. */
void grab_ptrs(PgVideoChannel_t *channel)
{
	/* Buffers have moved; re-obtain the pointers */
	ybuf0 = PdGetOffscreenContextPtr(channel->yplane1);
	ybuf1 = PdGetOffscreenContextPtr(channel->yplane2);
	ubuf0 = PdGetOffscreenContextPtr(channel->uplane1);
	ubuf1 = PdGetOffscreenContextPtr(channel->uplane2);
	vbuf0 = PdGetOffscreenContextPtr(channel->vplane1);
	vbuf1 = PdGetOffscreenContextPtr(channel->vplane2);

	if (channel->yplane1)
		//fprintf(stderr, "Photon 2 Overlay: ybuf0: %x, stride %d\n", ybuf0, channel->yplane1->pitch);
		printf("info: photon2 overlay: bleh.\n");
	if (channel->uplane1)
		//fprintf(stderr, "Photon 2 Overlay: ubuf0: %x, stride %d\n", ubuf0, channel->uplane1->pitch);
		printf("info: photon2 overlay: bleh.\n");
	if (channel->vplane1)
		//fprintf(stderr, "Photon 2 Overlay: vbuf0: %x, stride %d\n", vbuf0, channel->vplane1->pitch);
		printf("info: photon2 overlay: bleh.\n");
	if (channel->yplane2)
		//fprintf(stderr, "Photon 2 Overlay: ybuf1: %x, stride %d\n", ybuf1, channel->yplane2->pitch);
		printf("info: photon2 overlay: bleh.\n");
	if (channel->uplane2)
		//fprintf(stderr, "Photon 2 Overlay: ubuf1: %x, stride %d\n", ubuf1, channel->uplane2->pitch);
		printf("info: photon2 overlay: bleh.\n");
	if (channel->vplane2)
		//fprintf(stderr, "Photon 2 Overlay: vbuf1: %x, stride %d\n", vbuf1, channel->vplane2->pitch);
		printf("info: photon2 overlay: bleh.\n");
}

unsigned char *setup_overlay(int src_width, int src_height, int dst_width, int dst_height, int dst_x, int dst_y )
{
	void    *p;
	int i = 0;
	int bCont;

	swidth = src_width;
	sheight = src_height;
	dwidth = dst_width;
	dheight = dst_height;

        if ((channel = PgCreateVideoChannel(Pg_VIDEO_CHANNEL_SCALER,0)) == NULL) 
	{
		fprintf(stderr, "error: could not create channel!\n");
		return 0;
	}


	caps.size = sizeof (caps);
	bCont = TRUE;

	/* XXX could support 888 and 555 as well */
	while ((bCont) &&(PgGetScalerCapabilities(channel, i++, &caps) == 0))
        {
		if(caps.format == Pg_VIDEO_FORMAT_RGB8888)
                {
			printf("info: overlay supports RGB8888\n");
			props.format = Pg_VIDEO_FORMAT_RGB8888;
                        bCont = FALSE;
                }
                else if(caps.format  == Pg_VIDEO_FORMAT_RGB565)
                {
                        printf("info: overlay supports RGB565\n");
                        props.format = Pg_VIDEO_FORMAT_RGB565;
                        bCont = FALSE;
                }
                if(!bCont)
                   break;

                caps.size = sizeof (caps);
        }


        props.size = sizeof (props);
        props.src_dim.w = swidth;
        props.src_dim.h = sheight;

        props.viewport.ul.x = dst_x;
        props.viewport.ul.y = dst_y;
	props.viewport.lr.x = dst_x+(dwidth-1);
        props.viewport.lr.y = dst_y+(dheight-1);
        props.flags = Pg_SCALER_PROP_SCALER_ENABLE | ((phovr_filteron) ? Pg_SCALER_PROP_DISABLE_FILTERING : 0);

        if (PgConfigScalerChannel(channel, &props) == -1) 
	{
		fprintf(stderr, "error: could not configure channel!\n");
                return 0;
        }

        grab_ptrs(channel);

        overlay_on = 1;

	return ybuf0;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int ph_ovr_create_display (int bitmap_depth)
{
	PtArg_t arg[8];
	PhRect_t rect;
	PhRegion_t region_info;	
	
	// Create the Photon Window
	view_size.w = widthscale * visual_width;
	view_size.h = heightscale * visual_height;

	PhWindowQueryVisible( Ph_QUERY_GRAPHICS, 0, 1, &rect );
	
	PtSetArg( &arg[0], Pt_ARG_FILL_COLOR, Pg_BLACK, 0 );
	PtSetArg( &arg[1], Pt_ARG_WINDOW_MANAGED_FLAGS, 0, Ph_WM_MAX | Ph_WM_RESIZE | Ph_WM_MENU );
	PtSetArg( &arg[2], Pt_ARG_DIM, &rect, 0 );
	PtSetArg( &arg[3], Pt_ARG_WINDOW_NOTIFY_FLAGS, Ph_WM_FOCUS, Ph_WM_FOCUS | Ph_WM_RESIZE );
	PtSetArg( &arg[4], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, ~0 );
	PtSetArg( &arg[5], Pt_ARG_WINDOW_TITLE, title, 0 );
	PtSetArg( &arg[6], Pt_ARG_WINDOW_STATE, Pt_TRUE, Ph_WM_STATE_ISALTKEY | Ph_WM_STATE_ISFRONT | Ph_WM_STATE_ISMAX | Ph_WM_STATE_ISFOCUS );
	PtSetArg(&arg[7], Pt_ARG_WINDOW_MANAGED_FLAGS,Pt_TRUE, Ph_WM_FFRONT |Ph_WM_TOFRONT |Ph_WM_CONSWITCH );	
	PtSetParentWidget(NULL);
	if((P_mainWindow = PtCreateWidget(PtWindow, NULL, 8, arg)) == NULL)
		fprintf(stderr,"error: could not create main photon window!\n");

	/* add raw callback handler */
	PtAddEventHandler( P_mainWindow,      Ph_EV_BUT_PRESS |
					      Ph_EV_BUT_RELEASE |
					      Ph_EV_BOUNDARY |
					      Ph_EV_EXPOSE |
					      Ph_EV_PTR_MOTION |
					      Ph_EV_KEY, I_GetEvent, NULL );

	/* set draw buffer size */
	PgSetDrawBufferSize( 0xFF00 );
	
	region_info.cursor_type = Ph_CURSOR_NONE;
        region_info.rid = PtWidgetRid(P_mainWindow);
	
	/* add background handler */
	PhWindowQueryVisible( Ph_QUERY_GRAPHICS, 0, 1, &rect );
	
	scrnaspect=(float)view_size.w/(float)view_size.h;

	vw=rect.lr.x - rect.ul.x + 1;
	vh=rect.lr.y - rect.ul.y + 1;
	
	vscrnaspect=(float)(vw)/(float)(vh);
   
	if(scrnaspect<vscrnaspect) 
	{
		vscrnheight=(float)vh;
		vscrnwidth=vscrnheight*scrnaspect;
		vscrntlx=((float)vw-vscrnwidth)/2.0;
		vscrntly=0.0;
	}
	else 
	{
		vscrnwidth=(float)vw;
		vscrnheight=vscrnwidth/scrnaspect;
		vscrntlx=0.0;
		vscrntly=((float)vh-vscrnheight)/2.0;
	}
	
	scaled_buffer_ptr=setup_overlay(view_size.w, view_size.h, 
					     vscrnwidth, 
					     vscrnheight, vscrntlx, vscrntly);
	
	if (scaled_buffer_ptr == NULL)
		return OSD_NOT_OK;
	
	PtRealizeWidget( P_mainWindow );
	PtFlush();
	memset(&region_info,0,sizeof(region_info));
	region_info.cursor_type = Ph_CURSOR_NONE;
        region_info.rid = PtWidgetRid(P_mainWindow);
	PhRegionChange (Ph_REGION_CURSOR, 0, &region_info, NULL, NULL); // turn off cursor
 
	/* create and setup the image */
	switch (ph_ovr_update_method)
		
	{
		case PH_NORMAL:

#if 0
		image = PdCreateOffscreenContext(0, ((view_size.w+7) & ~7), view_size.h, Pg_OSC_MEM_PAGE_ALIGN);
	 	if (image == NULL)
	 	{
			fprintf(stderr_file, "error: failed to create offscreen context\n");
			return OSD_NOT_OK;
		}

		scaled_buffer_ptr = PdGetOffscreenContextPtr (image);
		if (!scaled_buffer_ptr)
		{
			fprintf (stderr_file, "error: failed get a pointer to offscreen context.\n");
			PhDCRelease (image);
			return OSD_NOT_OK;
		}
#endif
		depth = 0;

		switch (props.format)
		{
			case Pg_VIDEO_FORMAT_RGB565  :
				depth = 16;
			break;
			case Pg_VIDEO_FORMAT_RGB555  :
			// TODO:
			break;
			case Pg_VIDEO_FORMAT_RGB8888 :
				depth = 32;
			break;
		}
		break;
	
		default:
			fprintf (stderr_file, "error: unknown photon update method, this shouldn't happen\n");
		return OSD_NOT_OK;
	}

	/* setup the palette_info struct now we have the depth */
	if (ph_init_palette_info() != OSD_OK)
	return OSD_NOT_OK;

	fprintf(stderr_file, "info: actual bits per pixel = %d...\n", depth);
	if (bitmap_depth == 16)
	{
		switch(depth)
		{
			case 16:
				ph_ovr_update_display_func = ph_ovr_update_16_to_16bpp;
			break;
			case 24:
				ph_ovr_update_display_func = ph_ovr_update_16_to_24bpp;
			break;
			case 32:
				ph_ovr_update_display_func = ph_ovr_update_16_to_32bpp;
			break;
		}
	}

	if (ph_ovr_update_display_func == NULL)
	{
		fprintf(stderr_file, "error: unsupported\n");
		return OSD_NOT_OK;
	}

	fprintf(stderr_file, "Ok\n");

	return OSD_OK;
}

/*
 * Shut down the display, also called by the core to clean up if any error
 * happens when creating the display.
 */
/* This is insane, utterly, utterly, utterly insane. */
int I_OverlayOff(void)
{

	//printf("I_OverlayOff\n");

        if ( !overlay_on )
                return 1;
	props.size = sizeof (props);
        props.src_dim.w = swidth;
        props.src_dim.h = sheight;

        props.viewport.ul.x = 0;
	props.viewport.ul.y = 0;
        props.viewport.lr.x = swidth;
        props.viewport.lr.y = sheight;
        props.flags &= ~Pg_SCALER_PROP_SCALER_ENABLE;
        switch(PgConfigScalerChannel(channel, &props)){
	        case -1:
	           fprintf(stderr, "error: configure channel failed!\n");
	           exit(1);
                break;
		case 1:
			grab_ptrs(channel);
		break;
		case 0:
		default:
                break;
        }

        overlay_on = 0;

}

void ph_ovr_close_display (void)
{
	/* This is only allocated/done if we succeeded to get a window */
	if (P_mainWindow)
	{
		I_OverlayOff();
		scaled_buffer_ptr = NULL;
	}
 
	PtUnrealizeWidget(P_mainWindow);
	P_mainWindow=NULL;
}

/*
 * Set the screen colors using the given palette.
 *
 */
int ph_ovr_alloc_palette (int writable_colors)
{
	return 0;
}
 
int ph_ovr_modify_pen (int pen, unsigned char red, unsigned char green, unsigned char blue)
{
	return 0;
}

//* invoked by main tree code to update bitmap into screen */
void ph_ovr_update_display (struct mame_bitmap *bitmap)
{
   PhRegion_t region_info;	
	
   (*ph_ovr_update_display_func) (bitmap);

   if (use_mouse &&
       keyboard_pressed (KEYCODE_LALT) &&
       keyboard_pressed_memory (KEYCODE_PGDN))
   {
      if (ph_grab_mouse)
      {
         region_info.cursor_type = 0;
	 region_info.rid = PtWidgetRid(P_mainWindow);
	 ph_grab_mouse = FALSE;
      }
      else
      {
	 region_info.cursor_type = Ph_CURSOR_NONE;
	 region_info.rid = PtWidgetRid(P_mainWindow);      
	 ph_grab_mouse = TRUE;
      }

      PhRegionChange (Ph_REGION_CURSOR, 0, &region_info, NULL, NULL);
   }

//   PgFlush();         /* flush buffer to server */
}

void ph_ovr_refresh_screen (void)
{
}

INLINE void ph_ovr_put_image (int x, int y, int width, int height)
{
}

#define DEST_WIDTH swidth
#define DEST scaled_buffer_ptr
#define SRC_PIXEL unsigned short
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) ph_ovr_put_image(X, Y, WIDTH, HEIGHT);

static void ph_ovr_update_16_to_16bpp (struct mame_bitmap *bitmap)
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

static void ph_ovr_update_16_to_24bpp (struct mame_bitmap *bitmap)
{
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
}

static void ph_ovr_update_16_to_32bpp (struct mame_bitmap *bitmap)
{
#include "blit.h"
}

#undef  INDIRECT
#undef  SRC_PIXEL
#define SRC_PIXEL unsigned int

static void ph_window_update_32_to_32bpp_direct(struct mame_bitmap *bitmap)
{
#include "blit.h"
}

#undef  DEST_PIXEL
#undef SRC_PIXEL
