/* MAME Photon 2 Window Code
 *
 * Writen By: Travis Coady
 * Origional Code By: David Rempel
 *
 * web: http://www.classicgaming.com/phmame/
 * e-mail: smallfri@bigfoot.com
 *
 * Copyright (C) 2000-2001, The PhMAME Developement Team.
*/

#ifdef photon2
#define __PH_WINDOW_C_

/* TODO: - Remove some unwanted code...
		 - Add anything that needs adding...
		 - Make the remaing code better.
*/

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
#include "phkeyboard.h"
#include "devices.h"

static void ph_window_update_16_to_16bpp (struct mame_bitmap *bitmap);
static void ph_window_update_16_to_24bpp (struct mame_bitmap *bitmap);
static void ph_window_update_16_to_32bpp (struct mame_bitmap *bitmap);
static void ph_window_update_32_to_32bpp_direct (struct mame_bitmap *bitmap);
static void (*ph_window_update_display_func) (struct mame_bitmap *bitmap) = NULL;

/* hmm we need these to do the clean up correctly, or we could just 
   trust unix & X to clean up after us but lett's keep things clean */

static int private_cmap_allocated = 0;
static PdOffscreenContext_t *image = NULL;
static int orig_widthscale, orig_heightscale;
enum { PH_NORMAL };
static int image_width;
PhDim_t  view_size;
static int ph_window_update_method = PH_NORMAL;
static int startx = 0;
static int starty = 0;
static unsigned long black_pen;
static int use_xsync = 0;
static int root_window_id; /* root window id (for swallowing the mame window) */
static char *geometry = NULL;

static char *pseudo_color_allocated;
static unsigned long *pseudo_color_lookup;
static int pseudo_color_lookup_dirty;
static int pseudo_color_use_rw_palette;
static int pseudo_color_warn_low_on_colors;

static int pixels_per_line;

int phovr_fullscreen,phovr_filteron;
extern int update_mouse;

struct ph_func_struct {
   int  (*init)(void);
   int  (*create_display)(int depth);
   void (*close_display)(void);
   void (*update_display)(struct mame_bitmap *bitmap);
   int  (*alloc_palette)(int writable_colors);
   int  (*modify_pen)(int pen, unsigned char red, unsigned char green, unsigned char blue);
   int  (*_16bpp_capable)(void);
};

extern struct ph_func_struct ph_func[];
extern int current_mouse[MOUSE_AXIS] ;
struct rc_option ph_window_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Photon-window Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "cursor",		"nocursor",		rc_bool,       &show_cursor,
     "1",		0,			0,		NULL,
     "Show / don't show the cursor." },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

struct rc_option ph_ovr_opts[] = {

{ "Photon-overlay Related", NULL,	rc_seperator,	NULL,
  NULL,	0,	0,	NULL,
  NULL },
{ "fullscreen",	"window",	rc_bool,	&phovr_fullscreen,
  "1",		0,	0,	NULL,
  "Full Screen/Windowed"},
/*
{ "filteroff",	"filteron", rc_bool,	&phovr_filteron,
  "1",		0,	0,	NULL,
  "Filtering of scaled image on/off (if available)"},
*/

{NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL}
	
};
/*
 * Create a display screen, or window, large enough to accomodate a bitmap
 * of the given dimensions.
 */
int I_GetEvent(PtWidget_t *widget, void *data, PtCallbackInfo_t *cbinfo, int bitmap_depth )
{
	int mask=FALSE;
	int kdosomething=FALSE;
	PhKeyEvent_t *kevent;
	PhPointerEvent_t *pevent;	
	int                         keycode,code;
	int                         *pt;
	struct xmame_keyboard_event mame_key_event;
	char                        keyname[16+1];
	
   	mame_key_event.press = FALSE;
   
	switch (cbinfo->event->type)
	{
		case Ph_EV_KEY:
		{
			kevent = PhGetData (cbinfo->event);
			if (PkIsFirstDown (kevent->key_flags))
			{
              	mask=TRUE;
				mame_key_event.press=TRUE;
				kdosomething = TRUE;
			}
			else if (!PkIsKeyDown(kevent->key_flags))
				kdosomething = TRUE;

				
        	if (kdosomething)
			{
				if (kevent->key_flags & Pk_KF_Cap_Valid)
					keycode = kevent->key_cap;
				else 
					goto getevent_done;		

				if ((keycode & 0xF000) == 0xF000)
				{
					pt=extended_code_table;
					keycode &= 0x00FF;
				}
				else
				{
					pt=code_table;
				}
				
		 		mame_key_event.scancode = *(pt+keycode);
				if (PhKeyToMb (keyname, kevent) == -1)
				{
					keyname[0]=0;
				}
				mame_key_event.unicode = keyname[0];
				//phkey [ *(pt+keycode) ] = mask;
				xmame_keyboard_register_event(&mame_key_event); }		
			break;
		}
		case Ph_EV_BUT_PRESS:
			pevent = PhGetData( cbinfo->event );
			
			if (pevent->buttons & Ph_BUTTON_SELECT)
			{
				mouse_data[0].buttons[0] = TRUE;
			}
			if (pevent->buttons & Ph_BUTTON_MENU)
			{
				mouse_data[0].buttons[1] = TRUE;
			}	
			if (pevent->buttons & Ph_BUTTON_ADJUST)
			{
				mouse_data[0].buttons[2] = TRUE;
			}
		break;
		case Ph_EV_BUT_RELEASE:
			
			if( cbinfo->event->subtype != Ph_EV_RELEASE_REAL )
				break;

			pevent = PhGetData( cbinfo->event );
			
			if (pevent->buttons & Ph_BUTTON_SELECT)
			{
				mouse_data[0].buttons[0] = FALSE;
			}
			if (pevent->buttons & Ph_BUTTON_MENU)
			{
				mouse_data[0].buttons[1] = FALSE;
			}	
			if (pevent->buttons & Ph_BUTTON_ADJUST)
			{
				mouse_data[0].buttons[2] = FALSE;
			}
		break;

		case Ph_EV_PTR_MOTION_NOBUTTON:
		case Ph_EV_PTR_MOTION_BUTTON:
			if (ph_grab_mouse == FALSE)
			{
				pevent = PhGetData( cbinfo->event );
				update_mouse=TRUE;
				mouse_data[0].deltas[0] = pevent->pos.x-current_mouse[0];
				mouse_data[0].deltas[1] = pevent->pos.y-current_mouse[1];
        		current_mouse[0] = pevent->pos.x;
        		current_mouse[1] = pevent->pos.y;
			}
	 	break;
		case Ph_EV_EXPOSE:
			if (ph_video_mode==0)
			{
				ph_window_refresh_screen();
				PgFlush();
			}

		break;
		case Ph_EV_INFO:
		{
			switch (cbinfo->event->subtype)
			{
				case Ph_OFFSCREEN_INVALID :
				fprintf (stderr,"info: got offscreen invalid\n");
				if (image != NULL)
				{
					fprintf(stderr,"info: creating new image\n");
					PhDCRelease(image);
					image = PdCreateOffscreenContext(0, view_size.w, view_size.h, Pg_OSC_MEM_PAGE_ALIGN);
					if (image == NULL)
					{
						fprintf(stderr_file, "error: failed to create offscreen context\n");
						exit(1);
					}

					scaled_buffer_ptr = PdGetOffscreenContextPtr (image);
					if (!scaled_buffer_ptr)
					{
						fprintf (stderr_file, "error: failed get a pointer to offscreen context.\n");
						PhDCRelease (image);
						exit(1);
					}
	
					depth = 0;

					switch (image->format)
					{
						case Pg_IMAGE_PALETTE_BYTE   :
						// TODO :
						break;
						case Pg_IMAGE_DIRECT_565  :
						depth = 16;
						pixels_per_line = image->pitch >> 1;
						break;
						case Pg_IMAGE_DIRECT_555  :
						// TODO:
						break;
						case Pg_IMAGE_DIRECT_888  :
						depth = 24;
						pixels_per_line = image->pitch / 3;
						break;	
						case Pg_IMAGE_DIRECT_8888 :
						depth = 32;
						pixels_per_line = image->pitch >> 2;
						break;
					}
					ph_init_palette_info();
					ph_window_update_display_func=NULL;
					if (bitmap_depth == 16)
					{
						switch(depth)
						{
							case 16:
								ph_window_update_display_func = ph_window_update_16_to_16bpp;
							break;
							case 24:
								ph_window_update_display_func = ph_window_update_16_to_24bpp;
							break;
							case 32:
								ph_window_update_display_func = ph_window_update_16_to_32bpp;
							break;
						}
					}

					if (ph_window_update_display_func == NULL)
					{
						fprintf(stderr_file, "error: Unsupported\n");
						exit(1);
					}
				}
				break;
				
			}
		} 
		break;
	}

getevent_done:
	return (Pt_CONTINUE);
															
}

int ph_window_16bpp_capable(void)
{
   return 1;
}

int ph_window_create_display (int bitmap_depth)
{
	PtArg_t arg[9];
	PhRect_t rect;
    PhRegion_t region_info;
    
    // Only image_height??!!
    int image_height;
    int window_width, window_height;

	// Create the Photon Window

	view_size.w = widthscale * visual_width;
	view_size.h = heightscale * visual_height;
	
    image_width      = widthscale  * visual_width;
    image_height     = heightscale * visual_height;
    
    // TODO: Finish always ontop (Make phearbear happy)
	PtSetArg( &arg[0], Pt_ARG_FILL_COLOR, Pg_TRANSPARENT, 0 );
	PtSetArg( &arg[1], Pt_ARG_WINDOW_MANAGED_FLAGS, 0, Ph_WM_MAX | Ph_WM_RESIZE | Ph_WM_MENU | Ph_WM_CLOSE | Ph_WM_HIDE );
	PtSetArg( &arg[2], Pt_ARG_DIM, &view_size, 0 );
	PtSetArg( &arg[3], Pt_ARG_WINDOW_NOTIFY_FLAGS, Ph_WM_FOCUS, Ph_WM_FOCUS | Ph_WM_RESIZE | Ph_WM_CLOSE );
	PtSetArg( &arg[4], Pt_ARG_WINDOW_RENDER_FLAGS, Pt_FALSE, Ph_WM_RENDER_MENU | Ph_WM_RENDER_CLOSE | Ph_WM_RENDER_MAX | Ph_WM_RENDER_MIN | Ph_WM_RENDER_COLLAPSE | Ph_WM_RENDER_RESIZE );
	PtSetArg( &arg[5], Pt_ARG_WINDOW_TITLE, title, 0);
	//PtSetArg( &arg[6], Pt_ARG_WINDOW_STATE, 0, Ph_WM_STATE_ISFRONT );
	
	PtSetParentWidget(NULL);
	if((P_mainWindow = PtCreateWidget(PtWindow, NULL, 6, arg)) == NULL)
		fprintf(stderr,"error: could not create main photon window.\n");

	/* add raw callback handler */
	PtAddEventHandler( P_mainWindow,
		Ph_EV_BUT_PRESS |
		Ph_EV_BUT_RELEASE |
		Ph_EV_BOUNDARY |
		Ph_EV_EXPOSE |
		Ph_EV_PTR_MOTION |
		Ph_EV_KEY |
		Ph_EV_INFO,
		I_GetEvent,
		NULL );

	/* set draw buffer size */
	PgSetDrawBufferSize( 0xFF00 );

	PtRealizeWidget( P_mainWindow );

	if (show_cursor == FALSE)
	{
    	region_info.cursor_type = Ph_CURSOR_NONE;
    	region_info.rid = PtWidgetRid(P_mainWindow);
    	PhRegionChange (Ph_REGION_CURSOR, 0, &region_info, NULL, NULL); // turn off cursor
	}


	/* create and setup the image */
	switch (ph_window_update_method)
	{
		case PH_NORMAL:

//		image = PdCreateOffscreenContext(0, ((view_size.w+7) & ~7), view_size.h, Pg_OSC_MEM_PAGE_ALIGN);
		image = PdCreateOffscreenContext(0, view_size.w, view_size.h, Pg_OSC_MEM_PAGE_ALIGN);
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

		depth = 0;

		switch (image->format)
		{
			case Pg_IMAGE_PALETTE_BYTE   :
			// TODO :
			break;
			case Pg_IMAGE_DIRECT_565  :
				depth = 16;
				pixels_per_line = image->pitch >> 1;
			break;
			case Pg_IMAGE_DIRECT_555  :
			// TODO:
			break;
			case Pg_IMAGE_DIRECT_888  :
				depth = 24;
				pixels_per_line = image->pitch / 3;
			break;	
			case Pg_IMAGE_DIRECT_8888 :
				depth = 32;
				pixels_per_line = image->pitch >> 2;
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

	fprintf(stderr_file, "Actual bits per pixel = %d...\n", depth);
    if (bitmap_depth == 32)
   {
      if (depth == 32)
         ph_window_update_display_func = ph_window_update_32_to_32bpp_direct;
   }
	else if (bitmap_depth == 16)
	{
		switch(depth)
		{
			case 16:
				ph_window_update_display_func = ph_window_update_16_to_16bpp;
			break;
			case 24:
				ph_window_update_display_func = ph_window_update_16_to_24bpp;
			break;
			case 32:
				ph_window_update_display_func = ph_window_update_16_to_32bpp;
			break;
		}
	}

	if (ph_window_update_display_func == NULL)
	{
		fprintf(stderr_file, "error: unsupported\n");
		return OSD_NOT_OK;
	}

	fprintf(stderr_file, "Ok\n");

	return OSD_OK;
}

void ph_window_close_display (void)
{
   /* FIXME: free cursors */
   int i;

   widthscale  = orig_widthscale;
   heightscale = orig_heightscale;
  
   /* This is only allocated/done if we succeeded to get a window */
   if (P_mainWindow)
   {
     if (image)
      {
         PhDCRelease (image);
         scaled_buffer_ptr = NULL;
	 image=NULL;
      }
  
      PtUnrealizeWidget(P_mainWindow);
      P_mainWindow=NULL;
   }
}


int ph_window_alloc_palette (int writable_colors)
{
#if 0
   int i;
   
   if(!(pseudo_color_lookup = malloc(writable_colors * sizeof(unsigned long))))
   {
      fprintf(stderr_file, "error: malloc failed for pseudo color lookup table\n");
      return -1;
   }
   
   /* set the palette to black */
   for (i = 0; i < writable_colors; i++)
      pseudo_color_lookup[i] = black_pen;

   /* allocate color cells */
   if (XAllocColorCells (display, colormap, 0, 0, 0, pseudo_color_lookup,
      writable_colors))
   {
      pseudo_color_use_rw_palette = 1;
      fprintf (stderr_file, "info: using r/w palette entries to speed up, good\n");
      for (i = 0; i < writable_colors; i++)
         if (pseudo_color_lookup[i] != i) break;
   }
   else
   {
      if (!(pseudo_color_allocated = calloc(writable_colors, sizeof(char))))
      {
         fprintf(stderr_file, "error: malloc failed for pseudo color lookup table\n");
         free(pseudo_color_lookup);
         pseudo_color_lookup=NULL;
         return -1;
      }
   }
   
   display_palette_info.writable_colors = writable_colors;
#endif
   return 0;
}

int ph_window_modify_pen (int pen, unsigned char red, unsigned char green,
   unsigned char blue)
{
#if 0
   PgColor_t color;

   /* Translate 0-255 values of new color to X 0-65535 values. */
   color.flags = (DoRed | DoGreen | DoBlue);
   color.red = (int) red << 8;
   color.green = (int) green << 8;
   color.blue = (int) blue << 8;
   color.pixel = pseudo_color_lookup[pen];

   if (pseudo_color_use_rw_palette)
   {
      XStoreColor (display, colormap, &color);
   }
   else
   {
      /* free previously allocated color */
      if (pseudo_color_allocated[pen])
      {
         XFreeColors (display, colormap, &pseudo_color_lookup[pen], 1, 0);
         pseudo_color_allocated[pen] = FALSE;
      }

      /* allocate new color and assign it to pen index */
      if (XAllocColor (display, colormap, &color))
      {
         if (pseudo_color_lookup[pen] != color.pixel)
            pseudo_color_lookup_dirty = TRUE;
         pseudo_color_lookup[pen] = color.pixel;
         pseudo_color_allocated[pen] = TRUE;
      }
      else /* try again with the closest match */
      {
         int i;
         XColor colors[256];
         int my_red   = (int)red << 8;
         int my_green = (int)green << 8;
         int my_blue  = (int)blue << 8;
         int best_pixel = black_pen;
         float best_diff = FLT_MAX;
         
         for(i=0;i<256;i++)
            colors[i].pixel = i;
         
         XQueryColors(display, colormap, colors, 256);
         for(i=0;i<256;i++)
         {
            #define SQRT(x) ((float)(x)*(x))
            float diff = SQRT(my_red - colors[i].red) + 
               SQRT(my_green - colors[i].green) +
               SQRT(my_blue - colors[i].blue);
            if (diff < best_diff)
            {
               best_pixel = colors[i].pixel;
               best_diff  = diff;
            }
         }
         
         color = colors[best_pixel];
         
         if (XAllocColor (display, colormap, &color))
         {
            if (pseudo_color_lookup[pen] != color.pixel)
               pseudo_color_lookup_dirty = TRUE;
            pseudo_color_lookup[pen] = color.pixel;
            pseudo_color_allocated[pen] = TRUE;
         }
         else
         {
            if (pseudo_color_warn_low_on_colors)
            {
               pseudo_color_warn_low_on_colors = 0;
               fprintf (stderr_file,
                  "warning: Closest color match alloc failed\n"
                  "Couldn't allocate all colors, some parts of the emulation may be black\n"
                  "Try running mame with the -privatecmap option\n");
            }
            
            /* If color allocation failed, use black to ensure the
               pen is not left set to an invalid color */
            pseudo_color_lookup[pen] = black_pen;
            return -1;
         }
      }
   }
#endif
   return 0;
}

/* invoked by main tree code to update bitmap into screen */
void ph_window_update_display (struct mame_bitmap *bitmap)
{
   PhRegion_t region_info;	

//	fprintf(stderr,"Calling update display\n");

// TODO:  Not sure just yet what this is for...if it's only x related we can probably
//	  toss it.   
   (*ph_window_update_display_func) (bitmap);

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

   PgFlush();         /* flush buffer to server */
}

void ph_window_refresh_screen (void)
{
	PhArea_t sarea, darea;
	
//	fprintf(stderr,"refresh screen\n");
	switch (ph_window_update_method)
	{
		case PH_NORMAL:
			sarea.pos.x=sarea.pos.y=darea.pos.x=darea.pos.y=0;
			sarea.size.w=darea.size.w=image->dim.w;
			sarea.size.h=darea.size.h=image->dim.h;
			
			PgSetRegion(PtWidgetRid(P_mainWindow));
			PgSetClipping (0,NULL);
			PgContextBlitArea (image,&sarea,NULL,&darea);
		break;
	}
}

INLINE void ph_window_put_image (int x, int y, int width, int height)
{
	PhArea_t sarea, darea;


	switch (ph_window_update_method)
	{
		case PH_NORMAL:
			sarea.pos.x=x; sarea.pos.y=y;
			sarea.size.w=darea.size.w=width;
			sarea.size.h=darea.size.h=height;
			darea.pos.x=x+startx; darea.pos.y=y+starty;

			PgSetRegion(PtWidgetRid(P_mainWindow));
			PgSetClipping (0,NULL);
			PgContextBlitArea (image,&sarea,NULL,&darea);
		break;
	}
}

/* CHECK THIS! */
//#define DEST_WIDTH image->dim.w
#define DEST_WIDTH image_width
#define DEST scaled_buffer_ptr
#define SRC_PIXEL unsigned short
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) ph_window_put_image(X, Y, WIDTH, HEIGHT);

static void ph_window_update_16_to_16bpp (struct mame_bitmap *bitmap)
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

static void ph_window_update_16_to_24bpp (struct mame_bitmap *bitmap)
{
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
}

static void ph_window_update_16_to_32bpp (struct mame_bitmap *bitmap)
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

#undef DEST_PIXEL
#undef SRC_PIXEL

#endif /* ifdef photon2 */
