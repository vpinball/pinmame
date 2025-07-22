/***************************************************************************
                                          
 This is the SDL XMAME display driver.
 FIrst incarnation by Tadeusz Szczyrba <trevor@pik-nel.pl>,
 based on the Linux SVGALib adaptation by Phillip Ezolt.

 updated and patched by Ricardo Calixto Quesada (riq@core-sdi.com)

 patched by Patrice Mandin (pmandin@caramail.com)
  modified support for fullscreen modes using SDL and XFree 4
  added toggle fullscreen/windowed mode (Alt + Return)
  added title for the window
  hide mouse cursor in fullscreen mode
  added command line switch to start fullscreen or windowed
  modified the search for the best screen size (SDL modes are sorted by
    Y size)

 patched by Dan Scholnik (scholnik@ieee.org)
  added support for 32bpp XFree86 modes
  new update routines: 8->32bpp & 16->32bpp

 TODO: Test the HERMES code.
       Test the 16bpp->24bpp update routine
       Test the 16bpp->32bpp update routine
       Improve performance.
       Test mouse buttons (which games use them?)

***************************************************************************/
/* #define PARANOIC */
#define __SDL_C

/* #define DIRECT_HERMES */

// SDL defines this to __inline__ which no longer works with gcc 5+ ?
// TODO find the correct way to handle this, similar issue with ALSA
#ifndef SDL_FORCE_INLINE
#if ( (defined(__GNUC__) && (__GNUC__ >= 5)))
#define SDL_FORCE_INLINE __attribute__((always_inline)) static inline
#endif
#endif /* take the definition from SDL */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL3/SDL.h>
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"
#include "driver.h"
#include "SDL-keytable.h"
#ifdef DIRECT_HERMES 
#include <Hermes/Hermes.h>
#endif /* DIRECT_HERMES */
#include "effect.h"

static int Vid_width;
static int Vid_height;
static int Vid_depth = 8;
static SDL_Window *Window;
static SDL_Surface* Surface;
static SDL_Surface* Offscreen_surface;
static int hardware=1;
static int mode_number=-1;
static int start_fullscreen=0;
SDL_Color *Colors=NULL;
static bool cursor_state; /* previous mouse cursor state */

#ifdef DIRECT_HERMES
HermesHandle   H_PaletteHandle;
HermesHandle H_ConverterHandle;
int32_t* H_Palette;
static int H_Palette_modified = 0;
#endif

typedef void (*update_func_t)(struct mame_bitmap *bitmap);

update_func_t update_function;

static int sdl_mapkey(struct rc_option *option, const char *arg, int priority);

static int list_sdl_modes(struct rc_option *option, const char *arg, int priority);

struct rc_option display_opts[] = {
    /* name, shortname, type, dest, deflt, min, max, func, help */
   { "SDL Related",  NULL,    rc_seperator,  NULL,
       NULL,         0,       0,             NULL,
       NULL },
   { "fullscreen",   NULL,    rc_bool,       &start_fullscreen,
      "0",           0,       0,             NULL,
      "Start fullscreen" },
   { "listmodes",    NULL,    rc_use_function_no_arg,       NULL,
      NULL,           0,       0,             list_sdl_modes,
      "List all posible fullscreen modes" },
   { "modenumber",   NULL,    rc_int,        &mode_number,
      "-1",          0,       0,             NULL,
      "Try to use the fullscreen mode numbered 'n' (see the output of -listmodes)" },
   { "sdlmapkey",	"sdlmk",	rc_use_function,	NULL,
     NULL,		0,			0,		sdl_mapkey,
     "Set a specific key mapping, see xmamerc.dist" },
   { NULL,           NULL,    rc_end,        NULL,
      NULL,          0,       0,             NULL,
      NULL }
};

void sdl_update_16_to_16bpp(struct mame_bitmap *bitmap);
void sdl_update_16_to_24bpp(struct mame_bitmap *bitmap);
void sdl_update_16_to_32bpp(struct mame_bitmap *bitmap);
void sdl_update_rgb_direct_32bpp(struct mame_bitmap *bitmap);

int sysdep_init(void)
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
      fprintf (stderr, "SDL: Init error: %s\n",SDL_GetError());
      return OSD_NOT_OK;
   }
   SDL_SetJoystickEventsEnabled(true);
   const bool enabled = SDL_JoystickEventsEnabled();
   if (!enabled) {
      fprintf (stderr, "SDL: Error enabling joystick events: %s\n", SDL_GetError());
      SDL_Quit();
      exit (OSD_NOT_OK);
   }
   // list all joysticks
   int num_joysticks = 0;
   SDL_JoystickID *joysticks =  SDL_GetJoysticks(&num_joysticks);
   fprintf (stderr, "SDL: Info: Found %d joysticks\n", num_joysticks);
   for (int i = 0; i < num_joysticks; i++) {
      // get the id
      SDL_JoystickID id = joysticks[i];
      SDL_Joystick *joystick = SDL_OpenJoystick(id);
      if (joystick == NULL) {
         fprintf (stderr, "SDL: Error opening joystick %d: %s\n", i, SDL_GetError());
         SDL_Quit();
         exit (OSD_NOT_OK);
      }
      fprintf (stderr, "SDL: Info: Joystick %d: %s\n", i, SDL_GetJoystickName(joystick));
      //SDL_JoystickClose(joystick);
   }
   SDL_free(joysticks);

#ifdef DIRECT_HERMES
   Hermes_Init(0);
#endif /* DIRECT_HERMES */
   fprintf (stderr, "SDL: Info: SDL initialized\n");
   atexit (SDL_Quit);
   return OSD_OK;
}

void sysdep_close(void)
{
   SDL_Quit();
}

int sysdep_create_display(int depth)
{
   // print all video drivers and mark the one in use
   int num_drivers = SDL_GetNumVideoDrivers();
   const char *driver_name = SDL_GetCurrentVideoDriver();
   fprintf(stderr, "SDL: Info: Found %d video drivers\n", num_drivers);
   for (int i = 0; i < num_drivers; i++) {
      const char *driver = SDL_GetVideoDriver(i);
      fprintf(stderr, "SDL: Info: Video driver %d: %s", i, driver);
      if (strcmp(driver, driver_name) == 0) {
         fprintf(stderr, " (in use)");
      }
      fprintf(stderr, "\n");
   }

   fprintf(stderr, "SDL: Info: Create display with depth %d for %ix%i\n", depth, visual_width*widthscale, visual_height*heightscale);
   int vid_modes_i;
#ifdef DIRECT_HERMES 
   HermesFormat* H_src_format;
   HermesFormat* H_dst_format;
#endif /* DIRECT_HERMES */
   int vid_mode_flag; /* Flag to set the video mode */

   const SDL_DisplayID display = SDL_GetPrimaryDisplay();
   const SDL_DisplayMode *current_mode = SDL_GetCurrentDisplayMode(display);
   if (current_mode == NULL) {
      fprintf(stderr, "SDL: Error getting current display mode: %s\n", SDL_GetError());
      SDL_Quit();
      exit (OSD_NOT_OK);
   }

   const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(current_mode->format);

#ifdef SDL_DEBUG
   fprintf (stderr,"SDL: create_display(%d): \n",depth);
   fprintf (stderr,
      // "SDL: Info: HW blits %d\n"
      // "SDL: Info: SW blits %d\n"
      // "SDL: Info: Vid mem %d\n"
      "SDL: Info: Best supported depth %d\n",
      // video_info->blit_hw,
      // video_info->blit_sw,
      // video_info->video_mem,
      format->bits_per_pixel);
#endif

   // get SDL_PixelFormat from mode.format

   Vid_depth = format->bits_per_pixel;

   // TODO how do we know if hardware acceleration is available?
   // hardware = video_info->hw_available;

   int modes_count = 0;
   SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &modes_count);
   /* Best video mode found */
   int best_vid_mode = -1;
   int best_width = -1;
   int best_height = -1;
   if (modes_count == 0 || modes == NULL)
   {
#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Error listing display modes: %s\n",SDL_GetError());
#endif
      Vid_height = visual_height*heightscale;
      Vid_width = visual_width*widthscale;
   }else
   {

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: visual w:%d visual h:%d\n", visual_width, visual_height);
#endif


      // last mode is NULL,
      for (int mode_index = 0; mode_index < modes_count; mode_index++)
      {
         const SDL_DisplayMode mode = *modes[mode_index];
         SDL_Log(" %i bpp\t%i x %i (%.1fx) @ %fHz",
             SDL_BITSPERPIXEL(mode.format), mode.w, mode.h, mode.pixel_density, mode.refresh_rate);

#ifdef SDL_DEBUG
         fprintf (stderr, "SDL: Info: Found mode %d x %d\n", mode.w, mode.h);
#endif /* SDL_DEBUG */

         /* If width and height too small, skip to next mode */
         if ((mode.w < visual_width*widthscale) || (mode.h < visual_height*heightscale)) {
            continue;
         }

         /* If width or height smaller than current best, keep it */
         if ((mode.w < best_width) || (mode.h < best_height)) {
            best_vid_mode = mode_index;
            best_width = mode.w;
            best_height = mode.h;
         }
      }

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Best mode found : %d x %d\n",
         best_width,
         best_height);
#endif /* SDL_DEBUG */

      vid_modes_i = best_vid_mode;

      /* mode_number is a command line option */
      if( mode_number != -1) {
         if( mode_number >vid_modes_i)
            fprintf(stderr, "SDL: The mode number is invalid... ignoring\n");
         else
            vid_modes_i = mode_number;
      }
      if( vid_modes_i<0 ) {
         fprintf(stderr, "SDL: None of the modes match :-(\n");
         Vid_height = visual_height*heightscale;
         Vid_width = visual_width*widthscale;
      } else {
         Vid_width = best_width;
         Vid_height = best_height;
      }
   }
   SDL_free(modes);

   if( depth == 16 )
   {
      switch( Vid_depth ) {
      case 32:
         update_function = &sdl_update_16_to_32bpp;
         break;
      case 24:
         update_function = &sdl_update_16_to_24bpp;
         break;
      case 16:
         update_function = &sdl_update_16_to_16bpp;
         break;
      default:
         fprintf (stderr, "SDL: Unsupported Vid_depth=%d in depth=%d\n", Vid_depth,depth);
         SDL_Quit();
         exit (OSD_NOT_OK);
         break;
      }
   }
   else if (depth == 32)
   {
      if (Vid_depth == 32)
      {
         update_function = &sdl_update_rgb_direct_32bpp; 
      }
      else
      {
         fprintf (stderr, "SDL: Unsupported Vid_depth=%d in depth=%d\n",
            Vid_depth, depth);
         SDL_Quit();
         exit (OSD_NOT_OK);
      }
   }
   else
   {
      fprintf (stderr, "SDL: Unsupported depth=%d\n", depth);
      SDL_Quit();
      exit (OSD_NOT_OK);
   }


   /* Set video mode according to flags */
   vid_mode_flag = 0;
   if (start_fullscreen) {
      vid_mode_flag |= SDL_WINDOW_FULLSCREEN;
   }

   // Enable hidpi/scaling support
   // On Linux this requires SDL_VIDEODRIVER=wayland
   vid_mode_flag |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

   if(! (Window = SDL_CreateWindow(title,Vid_width, Vid_height, vid_mode_flag))) {
      fprintf (stderr, "SDL: Error: Setting video mode failed\n");
      SDL_Quit();
      exit (OSD_NOT_OK);
   } else {
      fprintf (stderr, "SDL: Info: Video mode set as %d x %d, depth %d\n", Vid_width, Vid_height, Vid_depth);
   }

   // If this is a hdpi window we scale the window to the correct size as we want physical pixels,
   // not content scale. We can only do this after window creation as there is no way to know
   // the density before that.
   // This will only affect MacOS and iOS
   // see https://wiki.libsdl.org/SDL3/README/highdpi
   const float density = SDL_GetWindowPixelDensity(Window);
   SDL_SetWindowSize(Window, Vid_width / density, Vid_height / density);

   Surface = SDL_GetWindowSurface(Window);

#ifndef DIRECT_HERMES
   Offscreen_surface = SDL_CreateSurface(Vid_width, Vid_height,
              SDL_GetPixelFormatForMasks(Vid_depth, 0, 0, 0, 0));
   if(Offscreen_surface==NULL) {
      SDL_Quit();
      exit (OSD_NOT_OK);
   }
#else /* DIRECT_HERMES */
   /* No offscreen surface when using hermes directly */
   H_ConverterHandle = Hermes_ConverterInstance(0);
   H_src_format = Hermes_FormatNew (8,0,0,0,0,HERMES_INDEXED);
   /* TODO: More general destination choosing - uptil
       now only 16 bit */
   H_dst_format = Hermes_FormatNew (16,Surface->format->Rmask,Surface->format->Gmask,Surface->format->Bmask,0,0);
   /*  H_dst_format = Hermes_FormatNew (16,5,5,5,0,0); */
   if ( ! (Hermes_ConverterRequest(H_ConverterHandle,H_src_format , H_dst_format)) ) {
      fprintf (stderr_file, "Hermes: Info: Converter request failed\n");
      exit (OSD_NOT_OK);
   }
#endif /* DIRECT_HERMES */


   /* Creating event mask */
   SDL_SetEventEnabled(SDL_EVENT_KEY_UP, true);
   SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, true);

   // TODO no longer exists
   //SDL_EnableUNICODE(1);

    /* fill the display_palette_info struct */
    memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
    display_palette_info.depth = Vid_depth;
    if (Vid_depth == 8)
         display_palette_info.writable_colors = 256;
    else if (Vid_depth == 16) {
      display_palette_info.red_mask = 0xF800;
      display_palette_info.green_mask = 0x07E0;
      display_palette_info.blue_mask   = 0x001F;
    }
    else {
      display_palette_info.red_mask   = 0x00FF0000;
      display_palette_info.green_mask = 0x0000FF00;
      display_palette_info.blue_mask  = 0x000000FF;
    };

   /* Hide mouse cursor and save its previous status */
   cursor_state = SDL_GetCursor();
   SDL_HideCursor();

   effect_init2(depth, Vid_depth, Vid_width);

   return OSD_OK;
}

/*
 *  keyboard remapping routine
 *  invoiced in startup code
 *  returns 0-> success 1-> invalid from or to
 */
static int sdl_mapkey(struct rc_option *option, const char *arg, int priority)
{
   unsigned int from, to;
   /* ultrix sscanf() requires explicit leading of 0x for hex numbers */
   if (sscanf(arg, "0x%x,0x%x", &from, &to) == 2)
   {
      /* perform tests */
      /* fprintf(stderr,"trying to map %x to %x\n", from, to); */
      if (from < SDL_SCANCODE_COUNT && to <= 127)
      {
         klookup[from] = to;
	 return OSD_OK;
      }
      /* stderr_file isn't defined yet when we're called. */
      fprintf(stderr,"Invalid keymapping %s. Ignoring...\n", arg);
   }
   return OSD_NOT_OK;
}

/* Update routines */
void sdl_update_16_to_16bpp (struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned short
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width
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
#undef SRC_PIXEL
#undef DEST_PIXEL
}

void sdl_update_16_to_24bpp (struct mame_bitmap *bitmap)
{
#define SRC_PIXEL  unsigned short
#define DEST_PIXEL unsigned int
#define PACK_BITS
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width
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
#undef DEST_WIDTH
#undef DEST
#undef PACK_BITS
#undef DEST_PIXEL
#undef SRC_PIXEL
}

void sdl_update_16_to_32bpp (struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
}

void sdl_update_rgb_direct_32bpp(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#define DEST Offscreen_surface->pixels
#define DEST_WIDTH Vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
}

#ifndef DIRECT_HERMES
void sysdep_update_display(struct mame_bitmap *bitmap)
{
   SDL_Rect srect = { 0,0,0,0 };
   SDL_Rect drect = { 0,0,0,0 };
   srect.w = Vid_width;
   srect.h = Vid_height;

   /* Center the display */
   drect.x = (Vid_width - visual_width*widthscale ) >> 1;
   drect.y = (Vid_height - visual_height*heightscale ) >> 1;

   drect.w = Vid_width;
   drect.h = Vid_height;

   (*update_function)(bitmap);

   
   if(SDL_BlitSurface (Offscreen_surface, &srect, Surface, &drect)<0) 
      fprintf (stderr,"SDL: Warn: Unsuccessful blitting\n");

   // FIXME how do we do this?
   // if(hardware==0)
   //    SDL_UpdateRects(Surface,1, &drect);

   // switch buffer
   SDL_UpdateWindowSurface(Window);
}
#else /* DIRECT_HERMES */
void sysdep_update_display(struct mame_bitmap *bitmap)
{
   int i,j,x,y,w,h;
   int locked =0 ;
   static int first_run = 1;
   int line_amount;

#ifdef SDL_DEBUG
   static int update_count = 0;
   static char* bak_bitmap;
   int corrected = 0;
   int debug = 0;
#endif /* SDL_DEBUG */

   if (H_Palette_modified) {
      Hermes_PaletteInvalidateCache(H_PaletteHandle);
      Hermes_ConverterPalette(H_ConverterHandle,H_PaletteHandle,0);
      H_Palette_modified = 0;
   }
   
#ifdef PANANOIC 
      memset(Offscreen_surface->pixels,'\0' ,Vid_height * Vid_width);
#endif 

   switch   (use_dirty) {
      long line_min;
      long line_max;
      long col_min;
      long col_amount;

      
#ifdef SDL_DEBUG
      int my_off;
#endif       
   case 0:
      /* Not using dirty */
      if (SDL_MUSTLOCK(Surface))
         SDL_LockSurface(Surface);
      
      Hermes_ConverterCopy (H_ConverterHandle, 
               bitmap->line[0] ,
               0, 0 , 
               Vid_width,Vid_height, bitmap->line[1] - bitmap->line[0],
               Surface->pixels, 
               0,0,
               Vid_width, Vid_height, Vid_width <<1 );
      
      SDL_UnlockSurface(Surface);
      SDL_UpdateRect(Surface,0,0,Vid_width,Vid_height);
      break;

   case 1:
      /* Algorithm:
          search through dirty & find max maximal polygon, which 
          we can get to clipping (don't know if 8x8 is enought)
      */
      /*osd_dirty_merge();*/
      
   case 2:
      h = (bitmap->height+7) >> 3; /* Divide by 8, up rounding */
      w = (bitmap->width +7) >> 3; /* Divide by 8, up rounding */
      
#ifdef PARANOIC
      /* Rechecking dirty correctness ...*/
      if ( (! first_run) && debug) {
         for (y=0;y<h;y++ ) {
            for (i=0;i<8;i++) {
               int line_off = ((y<<3) + i);
               for (x=0;x<w;x++) {
                  for (j=0;j<8;j++) {
                     int col_off = ((x<<3) + j);
                     if ( *(bak_bitmap + (line_off * (bitmap->line[1]- bitmap->line[0])) + col_off ) != *(*(bitmap->line + line_off) + col_off)) {
                        if (! dirty_blocks[y][x] ) {
                           printf ("Warn!!! Block should be dirty %d, %d, %d - correcting \n",y,x,i);
                           dirty_blocks[y][x] = 1;
                           dirty_lines[y]=1;
                           corrected = 1;
                        }
                     } 
                  }
               }
            }
         }
      } else {
         bak_bitmap = (void*)malloc(w<<3 * h<<3);
      }
      
      if (! corrected) {
         printf ("dirty ok\n");
      }
      
      first_run = 0;
      for (i = 0;i< bitmap->height;i++)
         memcpy(bak_bitmap + (bitmap->line[1] - bitmap->line[0])*i,*(bitmap->line+i),w<<3);
      
#endif /* PARANOIC */

      /* #define dirty_lines old_dirty_lines     */
      /* #define dirty_blocks old_dirty_blocks */
      
      for (y=0;y<h;y++) {
         if (dirty_lines[y]) {
            line_min = y<<3;
            line_max = line_min + 8;
            /* old_dirty_lines[y]=1; */
            for (x=0;x<w;x++) {
               if (dirty_blocks[y][x]) {
                  col_min = x<<3;
                  col_amount = 0;
                  do { 
                     col_amount++; 
                     dirty_blocks[y][x] = 0;
                     x++; 
                  } while (dirty_blocks[y][x]); 

                  dirty_blocks[y][x] = 0;
                  col_amount <<= 3;

                  line_amount = line_max - line_min;
                  /* Trying to use direct hermes library for fast blitting */
                  if (SDL_MUSTLOCK(Surface))
                     SDL_LockSurface(Surface);
               
                  Hermes_ConverterCopy (H_ConverterHandle, 
                     bitmap->line[0] ,
                     col_min, line_min , 
                     col_amount,line_amount, bitmap->line[1] - bitmap->line[0],
                     Surface->pixels, 
                     col_min, line_min, 
                     col_amount ,line_amount, Vid_width <<1 );
               
                  SDL_UnlockSurface(Surface);
                  SDL_UpdateRect(Surface,col_min,line_min,col_amount,line_amount);
               }
            }
            dirty_lines[y] = 0;
         }
      }
      
      /* Vector game .... */
      break;
      return ;
   }
   
   /* TODO - It's the real evil - better to use would be original 
       hermes routines */

#ifdef SDL_DEBUG
   update_count++;
#endif
}
#endif /* DIRECT_HERMES */

/* shut up the display */
void sysdep_display_close(void)
{
   fprintf(stderr, "SDL: Info: Shutting down display\n");
   SDL_DestroySurface(Offscreen_surface);

   /* Restore cursor state */
   if (cursor_state)
      SDL_ShowCursor();
   else
      SDL_HideCursor();

   // close the window
   SDL_DestroyWindow(Window);
}

/*
 * In 8 bpp we should alloc pallete - some ancient people  
 * are still using 8bpp displays
 */
int sysdep_display_alloc_palette(int totalcolors)
{
   int ncolors = totalcolors;

   fprintf (stderr, "SDL: sysdep_display_alloc_palette(%d);\n",totalcolors);
   if (Vid_depth != 8)
      return 0;

#ifndef DIRECT_HERMES
   Colors = (SDL_Color*) malloc (totalcolors * sizeof(SDL_Color));
   if( !Colors )
      return 1;
   for (int i = 0;i<totalcolors;i++) {
      (Colors + i)->r = 0xFF;
      (Colors + i)->g = 0x00;
      (Colors + i)->b = 0x00;
   }
   SDL_Palette *palette = SDL_CreateSurfacePalette(Offscreen_surface);
   SDL_SetPaletteColors (palette,Colors,0,totalcolors-1);
#else /* DIRECT_HERMES */
   H_PaletteHandle = Hermes_PaletteInstance();
   if ( !(H_Palette = Hermes_PaletteGet(H_PaletteHandle)) ) {
      fprintf (stderr_file, "Hermes: Info: PaletteHandle invalid");
      exit(OSD_NOT_OK);
   }
#endif /* DIRECT_HERMES */

   fprintf (stderr, "SDL: Info: Palette with %d colors allocated\n", totalcolors);
   return 0;
}

int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
   static int warned = 0;
#ifdef SDL_DEBUG
   fprintf(stderr,"sysdep_display_set_pen(%d,%d,%d,%d)\n",pen,red,green,blue);
#endif

#ifndef DIRECT_HERMES
   if( Colors ) {
      (Colors + pen)->r = red;
      (Colors + pen)->g = green;
      (Colors + pen)->b = blue;
      SDL_Palette *palette = SDL_CreateSurfacePalette(Offscreen_surface);
      if ( (! SDL_SetPaletteColors(palette, Colors + pen, pen,1)) && (! warned)) {
         printf ("Color allocation failed, or > 8 bit display\n");
         warned = 0;
      }
   }
#else /* DIRECT_HERMES */
   *(H_Palette + pen) = (red<<16) | ((green) <<8) | (blue );
   H_Palette_modified = 1; 
#endif 

#ifdef SDL_DEBUG
   fprintf(stderr, "STD: Debug: Pen %d modification: r %d, g %d, b, %d\n", pen, red,green,blue);
#endif /* SDL_DEBUG */
   return 0;
}

void sysdep_mouse_poll (void)
{
   float x,y;

   Uint8 buttons = SDL_GetRelativeMouseState(&x, &y);
   mouse_data[0].deltas[0] = (int)x;
   mouse_data[0].deltas[1] = (int)y;
   for(int i = 0;i<MOUSE_BUTTONS;i++) {
      mouse_data[0].buttons[i] = buttons & (0x01 << i);
   }
}

/* Keyboard procs */
/* Lighting keyboard leds */
void sysdep_set_leds(int leds) 
{
}

int sdl_keycode_to_key(const SDL_Keycode key_code)
{
   // since we don't have hashmaps in c
   if(key_code < 512 ){
      return klookup[key_code];
   } else{
      switch (key_code)
      {
      case SDLK_F1:
         return KEY_F1;
      case SDLK_F2:
         return KEY_F2;
      case SDLK_F3:
         return KEY_F3;
      case SDLK_F4:
         return KEY_F4;
      case SDLK_F5:
         return KEY_F5;
      case SDLK_F6:
         return KEY_F6;
      case SDLK_F7:
         return KEY_F7;
      case SDLK_F8:
         return KEY_F8;
      case SDLK_F9:
         return KEY_F9;
      case SDLK_F10:
         return KEY_F10;
      case SDLK_F11:
         return KEY_F11;
      case SDLK_F12:
         return KEY_F12;
      case SDLK_BACKSPACE:
         return KEY_BACKSPACE;
      case SDLK_TAB:
         return KEY_TAB;
      case SDLK_UP:
         return KEY_UP;
      case SDLK_DOWN:
         return KEY_DOWN;
      case SDLK_LEFT:
         return KEY_LEFT;
      case SDLK_RIGHT:
         return KEY_RIGHT;
      case SDLK_LCTRL:
         return KEY_LCONTROL;
      case SDLK_RCTRL:
         return KEY_RCONTROL;
      case SDLK_LSHIFT:
         return KEY_LSHIFT;
      case SDLK_RSHIFT:
         return KEY_RSHIFT;
      case SDLK_LALT:
         return KEY_ALT;
      case SDLK_RALT:
         return KEY_ALTGR;
      case SDLK_CAPSLOCK:
         return KEY_CAPSLOCK;
      case SDLK_INSERT:
         return KEY_INSERT;
      case SDLK_DELETE:
         return KEY_DEL;
      case SDLK_HOME:
         return KEY_HOME;
      case SDLK_END:
         return KEY_END;
      case SDLK_PAGEUP:
         return KEY_PGUP;
      case SDLK_PAGEDOWN:
         return KEY_PGDN;
      case SDLK_MENU:
         return KEY_MENU;
      case SDLK_KP_ENTER:
         return KEYCODE_ENTER_PAD;
      case SDLK_KP_PLUS:
         return KEY_PLUS_PAD;
      case SDLK_KP_MINUS:
         return KEY_MINUS_PAD;
      // case SDLK_KP_MULTIPLY:
      //    return KEY_ASTERISK;
      case SDLK_KP_DIVIDE:
         return KEY_SLASH_PAD;
      // case SDLK_KP_PERIOD:
      //    return KEY_PERIOD_PAD;
      case SDLK_KP_0:
         return KEY_0_PAD;
      case SDLK_KP_1:
         return KEY_1_PAD;
      case SDLK_KP_2:
         return KEY_2_PAD;
      case SDLK_KP_3:
         return KEY_3_PAD;
      case SDLK_KP_4:
         return KEY_4_PAD;
      case SDLK_KP_5:
         return KEY_5_PAD;
      case SDLK_KP_6:
         return KEY_6_PAD;
      case SDLK_KP_7:
         return KEY_7_PAD;
      case SDLK_KP_8:
         return KEY_8_PAD;
      case SDLK_KP_9:
         return KEY_9_PAD;
      default:
         return KEY_NONE;
      }
   }
}

void sysdep_update_keyboard()
{
   struct xmame_keyboard_event kevent;
   SDL_Event event;

   if (Surface) {
      while(SDL_PollEvent(&event)) {
         kevent.press = 0;

         switch (event.type)
         {
         case SDL_EVENT_KEY_DOWN:
            kevent.press = 1;

         /* ALT-Enter: toggle fullscreen */
            if (event.key.key == SDLK_RETURN)
            {
               if (event.key.mod & SDL_KMOD_ALT)
               {
                  SDL_Window* window = SDL_GetWindowFromID(event.key.windowID);
                  const bool is_fullscreen = SDL_GetWindowFullscreenMode(window) != NULL;
                  SDL_SetWindowFullscreen(window, !is_fullscreen);
               }
            }

         case SDL_EVENT_KEY_UP:
            kevent.unicode = 0;
            kevent.scancode = sdl_keycode_to_key(event.key.key);
            xmame_keyboard_register_event(&kevent);
            if (kevent.scancode == KEY_NONE)
               fprintf(stderr, "SDL unknown symbol 0x%x scancode: %d, sym: %d\n", event.key.key,
                       event.key.scancode, event.key.key);

#ifdef SDL_DEBUG
            fprintf(stderr, "Key %s %ssed\n",
                    SDL_GetKeyName(event.key.key),
                    kevent.press ? "pres" : "relea");
#endif
            break;
         case SDL_EVENT_TEXT_INPUT:
#ifdef SDL_DEBUG
            fprintf(stderr, "SDL: Text input: %s\n", event.text.text);
#endif
            kevent.unicode = event.text.text[0];
            kevent.scancode = KEY_NONE;
            xmame_keyboard_register_event(&kevent);
            break;
         case SDL_EVENT_QUIT:
            /* Shoult leave this to application */
            exit(OSD_OK);
            break;

         case SDL_EVENT_JOYSTICK_AXIS_MOTION:
            if (event.jaxis.which < JOY_AXIS)
               joy_data[event.jaxis.which].axis[event.jaxis.axis].val = event.jaxis.value;
#ifdef SDL_DEBUG
            fprintf(stderr, "Axis=%d,value=%d\n", event.jaxis.axis, event.jaxis.value);
#endif
            break;
         case SDL_EVENT_JOYSTICK_BUTTON_DOWN:

         case SDL_EVENT_JOYSTICK_BUTTON_UP:
            if (event.jbutton.which < JOY_BUTTONS)
               joy_data[event.jbutton.which].buttons[event.jbutton.button] = event.jbutton.down;
#ifdef SDL_DEBUG
            fprintf(stderr, "Button=%d,value=%d\n", event.jbutton.button, event.jbutton.down);
#endif
            break;


         default:
#ifdef SDL_DEBUG
            fprintf(stderr, "SDL: Debug: Other event %d\n", event.type);
#endif /* SDL_DEBUG */
            break;
         }
    joy_evaluate_moves ();
      }
   }
}

/* added functions */
int sysdep_display_16bpp_capable(void)
{
   const SDL_DisplayID display = SDL_GetPrimaryDisplay();
   const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
   if (mode == NULL)
   {
      fprintf(stderr, "SDL: Error getting current display mode: %s\n", SDL_GetError());
      return false;
   }
   const int bpp = SDL_BITSPERPIXEL(mode->format);
   return ( bpp >=16);
}

int list_sdl_modes(struct rc_option *option, const char *arg, int priority)
{
    // TODO we might want to go over all displays
   const int display_index = 0;
   int modes_count = 0;
   SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display_index, &modes_count);
   if (modes == NULL || modes_count == 0)
   {
      printf("This option only works in a full-screen mode (eg: linux's framebuffer)\n");
      return - 1;
   }

   // print modes available
   printf("Modes available:\n");

   for (int mode_index = 0; mode_index <= modes_count; mode_index++)
   {
      const SDL_DisplayMode mode = *modes[mode_index];
      printf("\t%d) Mode %d x %d @ %fHz\n",
          mode_index,
          mode.w,
          mode.h,
          mode.refresh_rate
          );
   }

   return -1;
}
