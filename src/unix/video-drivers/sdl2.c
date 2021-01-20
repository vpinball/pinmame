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

#undef SDL_DEBUG
/* #define DIRECT_HERMES */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <SDL.h>
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
static SDL_Surface* Surface;
static SDL_Surface* Offscreen_surface;
static int hardware=1;
static int mode_number=-1;
static int start_fullscreen=0;
SDL_Color *Colors=NULL;
static int cursor_state; /* previous mouse cursor state */

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
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf (stderr, "SDL: Error: %s\n",SDL_GetError());
      return OSD_NOT_OK;
   } 
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
   SDL_Rect** vid_modes;
   const SDL_VideoInfo* video_info;
   int vid_modes_i;
#ifdef DIRECT_HERMES 
   HermesFormat* H_src_format;
   HermesFormat* H_dst_format;
#endif /* DIRECT_HERMES */
   int vid_mode_flag; /* Flag to set the video mode */

   video_info = SDL_GetVideoInfo();

#ifdef SDL_DEBUG
   fprintf (stderr,"SDL: create_display(%d): \n",depth);
   fprintf (stderr,"SDL: Info: HW blits %d\n"
      "SDL: Info: SW blits %d\n"
      "SDL: Info: Vid mem %d\n"
      "SDL: Info: Best supported depth %d\n",
      video_info->blit_hw,
      video_info->blit_sw,
      video_info->video_mem,
      video_info->vfmt->BitsPerPixel);
#endif

   Vid_depth = video_info->vfmt->BitsPerPixel;

   vid_modes = SDL_ListModes(NULL,SDL_FULLSCREEN);
   vid_modes_i = 0;

   hardware = video_info->hw_available;

   if ( (! vid_modes) || ((long)vid_modes == -1)) {
#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Possible all video modes available\n");
#endif
      Vid_height = visual_height*heightscale;
      Vid_width = visual_width*widthscale;
   } else {
      int best_vid_mode; /* Best video mode found */
      int best_width,best_height;
      int i;

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: visual w:%d visual h:%d\n", visual_width, visual_height);
#endif
      best_vid_mode = 0;
      best_width = vid_modes[best_vid_mode]->w;
      best_height = vid_modes[best_vid_mode]->h;
      for (i=0;vid_modes[i];++i)
      {
         int cur_width, cur_height;

         cur_width = vid_modes[i]->w;
         cur_height = vid_modes[i]->h;

#ifdef SDL_DEBUG
         fprintf (stderr, "SDL: Info: Found mode %d x %d\n", cur_width, cur_height);
#endif /* SDL_DEBUG */

         /* If width and height too small, skip to next mode */
         if ((cur_width < visual_width*widthscale) || (cur_height < visual_height*heightscale)) {
            continue;
         }

         /* If width or height smaller than current best, keep it */
         if ((cur_width < best_width) || (cur_height < best_height)) {
            best_vid_mode = i;
            best_width = cur_width;
            best_height = cur_height;
         }
      }

#ifdef SDL_DEBUG
      fprintf (stderr, "SDL: Info: Best mode found : %d x %d\n",
         vid_modes[best_vid_mode]->w,
         vid_modes[best_vid_mode]->h);
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
         if(*(vid_modes+vid_modes_i)==NULL) 
            vid_modes_i--;

         Vid_width = (*(vid_modes + vid_modes_i))->w;
         Vid_height = (*(vid_modes + vid_modes_i))->h;
      }
   }

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
   vid_mode_flag = SDL_HWSURFACE;
   if (start_fullscreen) {
      vid_mode_flag |= SDL_FULLSCREEN;
   }

   if(! (Surface = SDL_SetVideoMode(Vid_width, Vid_height,Vid_depth, vid_mode_flag))) {
      fprintf (stderr, "SDL: Error: Setting video mode failed\n");
      SDL_Quit();
      exit (OSD_NOT_OK);
   } else {
      fprintf (stderr, "SDL: Info: Video mode set as %d x %d, depth %d\n", Vid_width, Vid_height, Vid_depth);
   }

#ifndef DIRECT_HERMES
   Offscreen_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,Vid_width,Vid_height,Vid_depth,0,0,0,0); 
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
   SDL_EventState(SDL_KEYUP, SDL_ENABLE);
   SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
   SDL_EnableUNICODE(1);
   
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
   cursor_state = SDL_ShowCursor(0);

   /* Set window title */
   SDL_WM_SetCaption(title, NULL);

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
      if (from >= SDLK_FIRST && from < SDLK_LAST && to >= 0 && to <= 127)
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

   if(hardware==0)
      SDL_UpdateRects(Surface,1, &drect);
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
   SDL_FreeSurface(Offscreen_surface);

   /* Restore cursor state */
   SDL_ShowCursor(cursor_state);
}

/*
 * In 8 bpp we should alloc pallete - some ancient people  
 * are still using 8bpp displays
 */
int sysdep_display_alloc_palette(int totalcolors)
{
   int ncolors;
   int i;
   ncolors = totalcolors;

   fprintf (stderr, "SDL: sysdep_display_alloc_palette(%d);\n",totalcolors);
   if (Vid_depth != 8)
      return 0;

#ifndef DIRECT_HERMES
   Colors = (SDL_Color*) malloc (totalcolors * sizeof(SDL_Color));
   if( !Colors )
      return 1;
   for (i=0;i<totalcolors;i++) {
      (Colors + i)->r = 0xFF;
      (Colors + i)->g = 0x00;
      (Colors + i)->b = 0x00;
   }
   SDL_SetColors (Offscreen_surface,Colors,0,totalcolors-1);
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
      if ( (! SDL_SetColors(Offscreen_surface, Colors + pen, pen,1)) && (! warned)) {
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
   int i;
   int x,y;
   Uint8 buttons;

   buttons = SDL_GetRelativeMouseState( &x, &y);
   mouse_data[0].deltas[0] = x;
   mouse_data[0].deltas[1] = y;
   for(i=0;i<MOUSE_BUTTONS;i++) {
      mouse_data[0].buttons[i] = buttons & (0x01 << i);
   }
}

/* Keyboard procs */
/* Lighting keyboard leds */
void sysdep_set_leds(int leds) 
{
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
            case SDL_KEYDOWN:
               kevent.press = 1;

               /* ALT-Enter: toggle fullscreen */
               if ( event.key.keysym.sym == SDLK_RETURN )
               {
                  if(event.key.keysym.mod & KMOD_ALT)
                     SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
               }

            case SDL_KEYUP:
               kevent.scancode = klookup[event.key.keysym.sym];
               kevent.unicode = event.key.keysym.unicode;
               xmame_keyboard_register_event(&kevent);
               if(!kevent.scancode)
                  fprintf (stderr, "Unknown symbol 0x%x\n",
                     event.key.keysym.sym);
#ifdef SDL_DEBUG
               fprintf (stderr, "Key %s %ssed\n",
                  SDL_GetKeyName(event.key.keysym.sym),
                  kevent.press? "pres":"relea");
#endif
               break;
            case SDL_QUIT:
               /* Shoult leave this to application */
               exit(OSD_OK);
               break;

    	    case SDL_JOYAXISMOTION:   
               if (event.jaxis.which < JOY_AXIS)
                  joy_data[event.jaxis.which].axis[event.jaxis.axis].val = event.jaxis.value;
#ifdef SDL_DEBUG
               fprintf (stderr,"Axis=%d,value=%d\n",event.jaxis.axis ,event.jaxis.value);
#endif
		break;
	    case SDL_JOYBUTTONDOWN:

	    case SDL_JOYBUTTONUP:
               if (event.jbutton.which < JOY_BUTTONS)
                  joy_data[event.jbutton.which].buttons[event.jbutton.button] = event.jbutton.state;
#ifdef SDL_DEBUG
               fprintf (stderr, "Button=%d,value=%d\n",event.jbutton.button ,event.jbutton.state);
#endif
		break;


            default:
#ifdef SDL_DEBUG
               fprintf(stderr, "SDL: Debug: Other event\n");
#endif /* SDL_DEBUG */
               break;
         }
    joy_evaluate_moves ();
      }
   }
}

/* added funcions */
int sysdep_display_16bpp_capable(void)
{
   const SDL_VideoInfo* video_info;
   video_info = SDL_GetVideoInfo();
   return ( video_info->vfmt->BitsPerPixel >=16);
}

int list_sdl_modes(struct rc_option *option, const char *arg, int priority)
{
   SDL_Rect** vid_modes;
   int vid_modes_i;

   vid_modes = SDL_ListModes(NULL,SDL_FULLSCREEN);
   vid_modes_i = 0;

   if ( (! vid_modes) || ((long)vid_modes == -1)) {
      printf("This option only works in a full-screen mode (eg: linux's framebuffer)\n");
      return - 1;
   }

   printf("Modes available:\n");

   while( *(vid_modes+vid_modes_i) ) {
      printf("\t%d) Mode %d x %d\n",
         vid_modes_i,
         (*(vid_modes+vid_modes_i))->w,
         (*(vid_modes+vid_modes_i))->h
         );
   
      vid_modes_i++;
   }

   return -1;
}
