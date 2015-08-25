/***************************************************************************

 Linux SVGALib adaptation by Phillip Ezolt pe28+@andrew.cmu.edu
  
***************************************************************************/
#define __SVGALIB_C

#include <math.h>
#include <vga.h>
#include <vgagl.h>
#include "xmame.h"
#include "svgainput.h"
#include "effect.h"

static int startx, starty;
static int scaled_visual_width, scaled_visual_height;
static int video_mode       = -1;
static int tweaked_mode     = -1;
static int update_function  = -1;
static int text_mode        = TRUE;
static unsigned char *video_mem = NULL;
static unsigned char *doublebuffer_buffer = NULL;
static int use_tweak = 0;
static int use_planar = 1;
static int use_linear = 0;
static int center_x = 0;
static int center_y = 0;
static vga_modeinfo video_modeinfo;
static void svgalib_update_linear(struct mame_bitmap *bitmap);
static void svgalib_update_planar(struct mame_bitmap *bitmap);
static void svgalib_update_gl(struct mame_bitmap *bitmap);
static void svgalib_update_gl_scaled(struct mame_bitmap *bitmap);
static void svgalib_update_linear_16bpp(struct mame_bitmap *bitmap);
static void svgalib_update_gl_16bpp(struct mame_bitmap *bitmap);
static void svgalib_update_gl_scaled_16bpp(struct mame_bitmap *bitmap);

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Svgalib Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "tweak",		NULL,			rc_bool,	&use_tweak,
     "0",		0,			0,		NULL,
     "Enable/disable svgalib tweaked video modes" },
   { "planar",		NULL,			rc_bool,	&use_planar,
     "1",		0,			0,		NULL,
     "Enable/disable use of planar (modeX) modes (slow)" },
   { "linear",		NULL,			rc_bool,	&use_linear,
     "0",		0,			0,		NULL,
     "Enable/disable use of linear framebuffer (fast)" },
   { "centerx",		NULL,			rc_int,		&center_x,
     "0",		0,			0,		NULL,
     "Adjust the horizontal center of tweaked vga modes" },
   { "centery",		NULL,			rc_int,		&center_y,
     "0",		0,			0,		NULL,
     "Adjust the vertical center of tweaked vga modes" },
   { NULL,		NULL,			rc_link,	mode_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};


typedef void (*update_func)(struct mame_bitmap *bitmap);

static update_func update_functions[8] = {
   svgalib_update_linear,
   svgalib_update_planar,
   svgalib_update_gl,
   svgalib_update_gl_scaled,
   svgalib_update_linear_16bpp,
   NULL,
   svgalib_update_gl_16bpp,
   svgalib_update_gl_scaled_16bpp
};

/* tweaked modes */
#ifdef __CPU_i386
#include "twkuser.c"
#include "twkmodes.h"

struct tweaked_mode_struct
{
   int width;
   int height;
   Register *registers;
   Register *registers_scanline;
   int planar;
   int horizontal_squashed;
};

/* all available videomodes */
static struct tweaked_mode_struct tweaked_modes[] = {
{  384, 256, scr384x256,    NULL,		 1, 0 },
{  384, 240, scr384x240,    NULL,		 1, 0 },
{  384, 224, scr384x224,    NULL,		 1, 0 },
{  336, 240, scr336x240,    NULL,		 1, 0 },
{  320, 240, scr320x240,    NULL,		 1, 0 },
{  320, 204, scr320x204,    NULL,                0, 0 },
{  288, 224, scr288x224,    NULL,                0, 0 },
{  256, 240, scr256x240,    NULL,                0, 0 },
{  256, 256, scr256x256,    NULL,                0, 0 },
{  256, 256, scr256x256ver, NULL,                0, 1 },
{  240, 256, scr240x256,    NULL,                0, 0 },
{  224, 288, scr224x288,    scr224x288scanlines, 0, 0 },
{  200, 320, scr200x320,    NULL,                0, 0 },
{    0,   0, NULL,          NULL,                0, 0 }
};

static void set_tweaked_mode(void)
{
	if (!text_mode && tweaked_mode >= 0)
	{
	   Register *r = tweaked_modes[tweaked_mode].registers;
	   center_mode(r);
	   if (use_scanlines)
	   {
	      if (tweaked_modes[tweaked_mode].registers_scanline)
	         r = tweaked_modes[tweaked_mode].registers_scanline;
	      else
	         r = make_scanline_mode(r, 21);
	   }
	   outRegArray(r, 21);
	}
}
#endif /* ifdef __CPU_i386 */

int sysdep_init(void)
{
   vga_init();
   
   if(svga_input_init())
      return OSD_NOT_OK;
   
   return OSD_OK;
}

void sysdep_close(void)
{
   svga_input_exit();

   /* close svgalib */
   vga_setmode(TEXT);
}

int sysdep_display_16bpp_capable(void)
{
   int i;
   vga_modeinfo *my_modeinfo;
   
   for (i=1; (my_modeinfo=vga_getmodeinfo(i)); i++)
      if( (my_modeinfo->colors == 32768) ||
          (my_modeinfo->colors == 65536) )
         return 1;
         
   return 0;
}

int sysdep_set_video_mode (void)
{
   int i;
   
   if (!text_mode) return OSD_NOT_OK;
   
   vga_setmode(video_mode);
   gl_setcontextvga(video_mode);
   text_mode = FALSE;
   set_tweaked_mode();

   if (video_modeinfo.flags & IS_MODEX)
      update_function=1;
   else
   {
#ifdef __CPU_i386
      /* do we have a linear framebuffer ? */
      i = video_modeinfo.width * video_modeinfo.height *
         video_modeinfo.bytesperpixel;
      if (i <= 65536 || 
          (video_modeinfo.flags & IS_LINEAR) ||
          (use_linear && (video_modeinfo.flags && CAPABLE_LINEAR) &&
           vga_setlinearaddressing() >=  i))
      {
         video_mem  = vga_getgraphmem();
         video_mem += startx * video_modeinfo.bytesperpixel;
         video_mem += starty * video_modeinfo.width *
            video_modeinfo.bytesperpixel;
         if ((widthscale > 1 || heightscale > 1 || yarbsize) &&
             doublebuffer_buffer == NULL)
         {
            doublebuffer_buffer = malloc(scaled_visual_width * 
               video_modeinfo.bytesperpixel);
            if (!doublebuffer_buffer)
            {
               fprintf(stderr_file, "Svgalib: Error: Couldn't allocate doublebuffer buffer\n");
               return OSD_NOT_OK;
            }
         }
         update_function=0;
         fprintf(stderr_file, "Svgalib: Info: Using a linear framebuffer to speed up\n");
      }
      else
#endif
      {
         if((widthscale == 1) && (heightscale == 1) && (yarbsize == 0))
            update_function=2;
         else
            update_function=3;
         /* we might need the doublebuffer_buffer for 1x1 in 16bpp, since it
            could be paletised */
         if( ((widthscale > 1) || (heightscale > 1) || (yarbsize) ||
	      (video_modeinfo.bytesperpixel == 2))
             && !doublebuffer_buffer)
         {
            doublebuffer_buffer = malloc(scaled_visual_width*scaled_visual_height*
               video_modeinfo.bytesperpixel);
            if (!doublebuffer_buffer)
            {
               fprintf(stderr_file, "Svgalib: Error: Couldn't allocate doublebuffer buffer\n");
               return OSD_NOT_OK;
            }
         }
      }
   }
      
   if (video_modeinfo.bytesperpixel == 2)
      update_function+=4;
   
   return OSD_OK;
}

void sysdep_set_text_mode (void)
{
   if (text_mode) return;
   vga_setmode(TEXT);
   text_mode=TRUE;
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
   int i;
   int score, best_score = 0;
   vga_modeinfo *my_modeinfo;
   
   video_mode       = -1;
   tweaked_mode     = -1;
   update_function  = -1;
   text_mode        = TRUE;
   video_mem        = NULL;
   doublebuffer_buffer = NULL;
   
   scaled_visual_width  = visual_width  * widthscale;
   scaled_visual_height = yarbsize ? yarbsize : visual_height * heightscale;
   
   for (i=1; (my_modeinfo=vga_getmodeinfo(i)); i++)
   {
      if(depth == 16)
      {
         if(my_modeinfo->colors != 32768 &&
            my_modeinfo->colors != 65536)
            continue;
      }
      else
      {
         if(my_modeinfo->colors != 256)
            continue;
         if((my_modeinfo->flags & IS_MODEX) &&
            (!use_planar || widthscale != 1 || heightscale != 1 || yarbsize))
            continue;
      }
      if (!vga_hasmode(i))
         continue;
      /* we only want modes which are a multiple of 8 width, due to alignment
         issues */
      if (my_modeinfo->width & 7)
         continue;
      if (mode_disabled(my_modeinfo->width, my_modeinfo->height, depth))
         continue;
      score = mode_match(my_modeinfo->width, my_modeinfo->height);
      if (score && score >= best_score)
      {
         best_score = score;
         video_mode = i;
         video_modeinfo = *my_modeinfo;
      }
      fprintf(stderr_file, "Svgalib: Info: Found videomode %dx%dx%d\n",
         my_modeinfo->width, my_modeinfo->height, my_modeinfo->colors);
   }
   
   if (use_tweak && depth == 8 && vga_hasmode(G320x200x256) &&
       vga_hasmode(G320x240x256))
   {
      float orig_display_aspect_ratio = display_aspect_ratio;
      for(i=0; tweaked_modes[i].width; i++)
      {
         if (mode_disabled(tweaked_modes[i].width, tweaked_modes[i].height, depth))
            continue;
         if((tweaked_modes[i].planar) &&
            (!use_planar || widthscale != 1 || heightscale != 1 || yarbsize))
            continue;
         if (tweaked_modes[i].horizontal_squashed)
            display_aspect_ratio = display_aspect_ratio * 9.0 / 16.0;
         score = mode_match(tweaked_modes[i].width, tweaked_modes[i].height);
         display_aspect_ratio = orig_display_aspect_ratio;
         if (score && score >= best_score)
         {
            best_score   = score;
            tweaked_mode = i;
            memset(&video_modeinfo, 0, sizeof(video_modeinfo));
            if (tweaked_modes[i].planar)
            {
               video_mode = G320x240x256;
               video_modeinfo.bytesperpixel = 0;
               video_modeinfo.maxpixels     = 262144;
               video_modeinfo.flags         = IS_MODEX;
            }
            else
            {
               video_mode = G320x200x256;
               video_modeinfo.bytesperpixel = 1;
               video_modeinfo.maxpixels     = 65536;
               video_modeinfo.flags         = IS_LINEAR;
            }
            video_modeinfo.colors = 256;
            video_modeinfo.width  = video_modeinfo.linewidth =
               tweaked_modes[i].width;
            video_modeinfo.height = tweaked_modes[i].height;
         }
         fprintf(stderr_file, "Svgalib: Info: Found videomode %dx%dx256\n",
            tweaked_modes[i].width, tweaked_modes[i].height);
      }
   }
   
   if (best_score == 0)
   {
      fprintf(stderr_file, "Svgalib: Couldn't find a suitable mode for a resolution of %d x %d\n",
         scaled_visual_width, scaled_visual_height);
      return OSD_NOT_OK;
   }
   
   fprintf(stderr_file, "Svgalib: Info: Choose videomode %dx%dx%d\n",
      video_modeinfo.width, video_modeinfo.height, video_modeinfo.colors);
   
   startx = ((video_modeinfo.width  - scaled_visual_width ) / 2) & ~7;
   starty =  (video_modeinfo.height - scaled_visual_height) / 2;
   
   if (sysdep_set_video_mode()!=OSD_OK) return OSD_NOT_OK;
   
   fprintf(stderr_file, "Using a mode with a resolution of %d x %d, starting at %d x %d\n",
      video_modeinfo.width, video_modeinfo.height, startx, starty);

   /* fill the display_palette_info struct */
   memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
   display_palette_info.depth = depth;
   if (depth == 8)
      display_palette_info.writable_colors = 256;
   else
      if (video_modeinfo.colors == 32768)
      {
         display_palette_info.red_mask   = 0x001F;
         display_palette_info.green_mask = 0x03E0;
         display_palette_info.blue_mask  = 0xEC00;
      }
      else
      {
         display_palette_info.red_mask   = 0xF800;
         display_palette_info.green_mask = 0x07E0;
         display_palette_info.blue_mask  = 0x001F;
      }


   effect_init2(depth, display_palette_info.depth, scaled_visual_width);

   /* init input */
#ifdef __CPU_i386
   if(svga_input_open(NULL, set_tweaked_mode))
#else
   if(svga_input_open(NULL, NULL))
#endif
      return OSD_NOT_OK;
   
   return OSD_OK;
}

/* shut up the display */
void sysdep_display_close(void)
{
   /* close input */
   svga_input_close();
   
   /* close svgalib */
   sysdep_set_text_mode();

   /* and don't forget to free our other resources */
   if (doublebuffer_buffer)
   {
      free(doublebuffer_buffer);
      doublebuffer_buffer = NULL;
   }
}

int sysdep_display_alloc_palette(int writable_colors)
{
   return 0;
}

int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green,
   unsigned char blue)
{
   gl_setpalettecolor(pen,(red>>2),(green>>2),(blue>>2));
   return 0;
}

/* Update the display. */
void sysdep_update_display(struct mame_bitmap *bitmap)
{
   update_functions[update_function](bitmap);
}


static void svgalib_update_linear(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned char
#define DEST video_mem
#define DEST_WIDTH video_modeinfo.linewidth
#define DOUBLEBUFFER
#include "blit.h"
#undef DEST
#undef DEST_WIDTH
#undef DOUBLEBUFFER
#undef SRC_PIXEL
#undef DEST_PIXEL
}

static void svgalib_update_planar(struct mame_bitmap *bitmap)
{
  /* use page flipping otherwise the screen tears in planar modes,
     unfortunatly this also means we can't use dirty in planar modes */
  static int page=0;
  if (page) page=0;
   else page=131072;
  vga_copytoplanar256(bitmap->line[visual.min_y] + visual.min_x,
     bitmap->line[1] - bitmap->line[0],
     (starty*video_modeinfo.width + startx + page)/4,
      video_modeinfo.width/4, visual_width, visual_height);
  vga_setdisplaystart(page);
}

static void svgalib_update_gl(struct mame_bitmap *bitmap)
{
   int bitmap_linewidth = (bitmap->line[1] - bitmap->line[0]) /
      video_modeinfo.bytesperpixel;
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
      gl_putboxpart( \
         startx + X, starty + Y, \
         WIDTH, HEIGHT, \
         bitmap_linewidth, bitmap->height, \
         bitmap->line[0], X + visual.min_x, Y + visual.min_y);
         /* Note: we calculate the real bitmap->width, as used in
            osd_create_bitmap, this fixes the scewing bug in tempest
            & others */
#include "blit.h"
#undef PUT_IMAGE
}

static void svgalib_update_gl_scaled(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned char
#define DEST_PIXEL unsigned char
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
      gl_putboxpart( \
         startx + X, starty + Y, \
         WIDTH, HEIGHT, \
         scaled_visual_width, scaled_visual_height, \
         doublebuffer_buffer, X, Y );
#include "blit.h"
#undef DEST
#undef DEST_WIDTH
#undef PUT_IMAGE
#undef SRC_PIXEL
#undef DEST_PIXEL
}

static void svgalib_update_linear_16bpp(struct mame_bitmap *bitmap)
{
   int linewidth = video_modeinfo.linewidth/2;
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST video_mem
#define DEST_WIDTH linewidth
#define DOUBLEBUFFER
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
   }
   else
   {
#include "blit.h"
   }
#undef DEST
#undef DEST_WIDTH
#undef DOUBLEBUFFER
#undef SRC_PIXEL
#undef DEST_PIXEL
}

static void svgalib_update_gl_16bpp(struct mame_bitmap *bitmap)
{
   if(current_palette->lookup)
   {
      /* since we need the lookups we need to go through an extra buffer,
         just like svgalib_update_gl_scaled_16bpp() does */
      svgalib_update_gl_scaled_16bpp(bitmap);
   }
   else
   {
      /* we can just call svgalib_update_gl which only uses gllib functions
         and thus doesn't care about 8 / 16 bpp */
      svgalib_update_gl(bitmap);
   }
}

static void svgalib_update_gl_scaled_16bpp(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
      gl_putboxpart( \
         startx + X, starty + Y, \
         WIDTH, HEIGHT, \
         scaled_visual_width, scaled_visual_height, \
         doublebuffer_buffer, X, Y );
   if(current_palette->lookup)
   {
#define INDIRECT current_palette->lookup
#include "blit.h"
#undef INDIRECT
   }
   else
   {
#include "blit.h"
   }
#undef DEST
#undef DEST_WIDTH
#undef PUT_IMAGE
#undef SRC_PIXEL
#undef DEST_PIXEL
}
