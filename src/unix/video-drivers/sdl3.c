/***************************************************************************

 SDL3 display, input and joystick driver for xmame/PinMAME.

 Successor of the SDL 1.2 driver (sdl.c) written by Tadeusz Szczyrba,
 Ricardo Calixto Quesada, Patrice Mandin and Dan Scholnik.

 The emulated bitmap is blitted (using the generic blit.h scaler/effect
 code) into a 32bpp XRGB8888 buffer that is uploaded to a streaming texture
 and drawn by an SDL_Renderer. The renderer takes care of scaling to the
 window size, fullscreen letterboxing and high DPI displays, so we no
 longer care about physical display modes like the SDL 1.2 driver did.

***************************************************************************/
#define __SDL3_C

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>
#include "xmame.h"
#include "devices.h"
#include "keyboard.h"
#include "driver.h"
#include "effect.h"

/* A display is a window with a renderer and a streaming texture of the size
   of the (scaled) emulated bitmap. The unix video layer only ever draws to
   one display at a time, but switches between the game and the debugger one
   when the MAME debugger (MAME_DEBUG builds, -debug) takes or loses focus.
   We keep both windows around, like the Windows builds, so the game and the
   debugger are visible side by side; the inactive one just keeps showing
   its last frame. */
struct sdl3_display {
   SDL_Window *window;
   SDL_Renderer *renderer;
   SDL_Texture *texture;
   unsigned int *framebuffer;
   int width;
   int height;
};

enum { DISPLAY_GAME = 0, DISPLAY_DEBUGGER, DISPLAY_COUNT };
static struct sdl3_display displays[DISPLAY_COUNT];
static struct sdl3_display *current_display = NULL;

static void sdl3_destroy_display(struct sdl3_display *display);
static void sdl3_select_display(struct sdl3_display *display);

/* the current display, for blit.h and the event handling */
static SDL_Window *window = NULL;
static unsigned int *framebuffer = NULL;
static int vid_width = 0;
static int vid_height = 0;
static const int vid_depth = 32; /* we always render to XRGB8888 */

/* rc options */
static int start_fullscreen = 0;
static int window_scale = 0;
static int use_vsync = 0;
static int linear_filter = 0;

/* joystick instance id -> joy_data index mapping */
static SDL_Joystick *joysticks[JOY];
static int joystick_driver_active = 0;

/* set to exit the emulation gracefully, see keyboard.c */
extern UINT8 trying_to_quit;

typedef void (*update_func_t)(struct mame_bitmap *bitmap);
static update_func_t update_function = NULL;

static int sdl3_mapkey(struct rc_option *option, const char *arg, int priority);

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "SDL3 Related",  NULL,    rc_seperator,  NULL,
      NULL,           0,       0,             NULL,
      NULL },
   { "fullscreen",    NULL,    rc_bool,       &start_fullscreen,
      "0",            0,       0,             NULL,
      "Start fullscreen" },
   { "windowscale",   "ws",    rc_int,        &window_scale,
      "0",            0,       32,            NULL,
      "Scale the window by this integer factor (rendered by the GPU, unlike -scale).\n"
      "0 (default) picks the largest factor that keeps the window within half of the desktop" },
   { "vsync",         NULL,    rc_bool,       &use_vsync,
      "0",            0,       0,             NULL,
      "Synchronize screen updates with the display refresh" },
   { "linear",        NULL,    rc_bool,       &linear_filter,
      "0",            0,       0,             NULL,
      "Use linear (smooth) instead of nearest neighbour filtering when scaling" },
   { "sdlmapkey",     "sdlmk", rc_use_function, NULL,
      NULL,           0,       0,             sdl3_mapkey,
      "Map an SDL scancode to a MAME keycode, as 0x<sdl scancode>,0x<mame keycode>" },
   { NULL,            NULL,    rc_end,        NULL,
      NULL,           0,       0,             NULL,
      NULL }
};

/*
 * SDL scancode (physical key position) -> xmame keycode (pc scancode).
 * Physical positions are what MAME expects, this makes the mapping
 * independent of the keyboard layout.
 */
static unsigned char scancode_to_key[SDL_SCANCODE_COUNT] = {
   [SDL_SCANCODE_A] = KEY_A,
   [SDL_SCANCODE_B] = KEY_B,
   [SDL_SCANCODE_C] = KEY_C,
   [SDL_SCANCODE_D] = KEY_D,
   [SDL_SCANCODE_E] = KEY_E,
   [SDL_SCANCODE_F] = KEY_F,
   [SDL_SCANCODE_G] = KEY_G,
   [SDL_SCANCODE_H] = KEY_H,
   [SDL_SCANCODE_I] = KEY_I,
   [SDL_SCANCODE_J] = KEY_J,
   [SDL_SCANCODE_K] = KEY_K,
   [SDL_SCANCODE_L] = KEY_L,
   [SDL_SCANCODE_M] = KEY_M,
   [SDL_SCANCODE_N] = KEY_N,
   [SDL_SCANCODE_O] = KEY_O,
   [SDL_SCANCODE_P] = KEY_P,
   [SDL_SCANCODE_Q] = KEY_Q,
   [SDL_SCANCODE_R] = KEY_R,
   [SDL_SCANCODE_S] = KEY_S,
   [SDL_SCANCODE_T] = KEY_T,
   [SDL_SCANCODE_U] = KEY_U,
   [SDL_SCANCODE_V] = KEY_V,
   [SDL_SCANCODE_W] = KEY_W,
   [SDL_SCANCODE_X] = KEY_X,
   [SDL_SCANCODE_Y] = KEY_Y,
   [SDL_SCANCODE_Z] = KEY_Z,

   [SDL_SCANCODE_1] = KEY_1,
   [SDL_SCANCODE_2] = KEY_2,
   [SDL_SCANCODE_3] = KEY_3,
   [SDL_SCANCODE_4] = KEY_4,
   [SDL_SCANCODE_5] = KEY_5,
   [SDL_SCANCODE_6] = KEY_6,
   [SDL_SCANCODE_7] = KEY_7,
   [SDL_SCANCODE_8] = KEY_8,
   [SDL_SCANCODE_9] = KEY_9,
   [SDL_SCANCODE_0] = KEY_0,

   [SDL_SCANCODE_RETURN] = KEY_ENTER,
   [SDL_SCANCODE_ESCAPE] = KEY_ESC,
   [SDL_SCANCODE_BACKSPACE] = KEY_BACKSPACE,
   [SDL_SCANCODE_TAB] = KEY_TAB,
   [SDL_SCANCODE_SPACE] = KEY_SPACE,

   [SDL_SCANCODE_MINUS] = KEY_MINUS,
   [SDL_SCANCODE_EQUALS] = KEY_EQUALS,
   [SDL_SCANCODE_LEFTBRACKET] = KEY_OPENBRACE,
   [SDL_SCANCODE_RIGHTBRACKET] = KEY_CLOSEBRACE,
   [SDL_SCANCODE_BACKSLASH] = KEY_BACKSLASH,
   [SDL_SCANCODE_NONUSHASH] = KEY_BACKSLASH2,
   [SDL_SCANCODE_SEMICOLON] = KEY_COLON,
   [SDL_SCANCODE_APOSTROPHE] = KEY_QUOTE,
   [SDL_SCANCODE_GRAVE] = KEY_TILDE,
   [SDL_SCANCODE_COMMA] = KEY_COMMA,
   [SDL_SCANCODE_PERIOD] = KEY_STOP,
   [SDL_SCANCODE_SLASH] = KEY_SLASH,
   [SDL_SCANCODE_CAPSLOCK] = KEY_CAPSLOCK,

   [SDL_SCANCODE_F1] = KEY_F1,
   [SDL_SCANCODE_F2] = KEY_F2,
   [SDL_SCANCODE_F3] = KEY_F3,
   [SDL_SCANCODE_F4] = KEY_F4,
   [SDL_SCANCODE_F5] = KEY_F5,
   [SDL_SCANCODE_F6] = KEY_F6,
   [SDL_SCANCODE_F7] = KEY_F7,
   [SDL_SCANCODE_F8] = KEY_F8,
   [SDL_SCANCODE_F9] = KEY_F9,
   [SDL_SCANCODE_F10] = KEY_F10,
   [SDL_SCANCODE_F11] = KEY_F11,
   [SDL_SCANCODE_F12] = KEY_F12,

   [SDL_SCANCODE_PRINTSCREEN] = KEY_PRTSCR,
   [SDL_SCANCODE_SCROLLLOCK] = KEY_SCRLOCK,
   [SDL_SCANCODE_PAUSE] = KEY_PAUSE,
   [SDL_SCANCODE_INSERT] = KEY_INSERT,
   [SDL_SCANCODE_HOME] = KEY_HOME,
   [SDL_SCANCODE_PAGEUP] = KEY_PGUP,
   [SDL_SCANCODE_DELETE] = KEY_DEL,
   [SDL_SCANCODE_END] = KEY_END,
   [SDL_SCANCODE_PAGEDOWN] = KEY_PGDN,
   [SDL_SCANCODE_RIGHT] = KEY_RIGHT,
   [SDL_SCANCODE_LEFT] = KEY_LEFT,
   [SDL_SCANCODE_DOWN] = KEY_DOWN,
   [SDL_SCANCODE_UP] = KEY_UP,

   [SDL_SCANCODE_NUMLOCKCLEAR] = KEY_NUMLOCK,
   [SDL_SCANCODE_KP_DIVIDE] = KEY_SLASH_PAD,
   [SDL_SCANCODE_KP_MULTIPLY] = KEY_ASTERISK,
   [SDL_SCANCODE_KP_MINUS] = KEY_MINUS_PAD,
   [SDL_SCANCODE_KP_PLUS] = KEY_PLUS_PAD,
   [SDL_SCANCODE_KP_ENTER] = KEY_ENTER_PAD,
   [SDL_SCANCODE_KP_1] = KEY_1_PAD,
   [SDL_SCANCODE_KP_2] = KEY_2_PAD,
   [SDL_SCANCODE_KP_3] = KEY_3_PAD,
   [SDL_SCANCODE_KP_4] = KEY_4_PAD,
   [SDL_SCANCODE_KP_5] = KEY_5_PAD,
   [SDL_SCANCODE_KP_6] = KEY_6_PAD,
   [SDL_SCANCODE_KP_7] = KEY_7_PAD,
   [SDL_SCANCODE_KP_8] = KEY_8_PAD,
   [SDL_SCANCODE_KP_9] = KEY_9_PAD,
   [SDL_SCANCODE_KP_0] = KEY_0_PAD,
   [SDL_SCANCODE_KP_PERIOD] = KEY_DEL_PAD,
   [SDL_SCANCODE_NONUSBACKSLASH] = KEY_BACKSLASH2,
   [SDL_SCANCODE_APPLICATION] = KEY_MENU,

   [SDL_SCANCODE_LCTRL] = KEY_LCONTROL,
   [SDL_SCANCODE_LSHIFT] = KEY_LSHIFT,
   [SDL_SCANCODE_LALT] = KEY_ALT,
   [SDL_SCANCODE_LGUI] = KEY_LWIN,
   [SDL_SCANCODE_RCTRL] = KEY_RCONTROL,
   [SDL_SCANCODE_RSHIFT] = KEY_RSHIFT,
   [SDL_SCANCODE_RALT] = KEY_ALTGR,
   [SDL_SCANCODE_RGUI] = KEY_RWIN,
};

/*
 * keyboard remapping routine, invoked from the rc/command line handling
 */
static int sdl3_mapkey(struct rc_option *option, const char *arg, int priority)
{
   unsigned int from, to;

   if (sscanf(arg, "0x%x,0x%x", &from, &to) == 2)
   {
      if (from < SDL_SCANCODE_COUNT && to < KEY_MAX)
      {
         scancode_to_key[from] = to;
         return OSD_OK;
      }
      /* stderr_file isn't defined yet when we're called. */
      fprintf(stderr, "Invalid keymapping %s. Ignoring...\n", arg);
   }
   return OSD_NOT_OK;
}

int sysdep_init(void)
{
   /* SDL_Init is called by sysdep_init as well as, for the audio subsystem,
      by the dsp plugin. SDL reference counts subsystems, so this is fine. */
   if (!SDL_Init(SDL_INIT_VIDEO))
   {
      fprintf(stderr, "SDL3: Error: %s\n", SDL_GetError());
      return OSD_NOT_OK;
   }
   fprintf(stderr, "SDL3: Info: SDL %d.%d.%d initialized, video driver: %s\n",
      SDL_VERSIONNUM_MAJOR(SDL_GetVersion()),
      SDL_VERSIONNUM_MINOR(SDL_GetVersion()),
      SDL_VERSIONNUM_MICRO(SDL_GetVersion()),
      SDL_GetCurrentVideoDriver());
   return OSD_OK;
}

void sysdep_close(void)
{
   int i;

   sdl3_select_display(NULL);
   for (i = 0; i < DISPLAY_COUNT; i++)
      sdl3_destroy_display(&displays[i]);
   SDL_Quit();
}

/* We always render to a 32bpp texture, so we can handle any bitmap depth */
int sysdep_display_16bpp_capable(void)
{
   return 1;
}

static void sdl3_destroy_display(struct sdl3_display *display)
{
   if (display->texture)
      SDL_DestroyTexture(display->texture);
   if (display->renderer)
      SDL_DestroyRenderer(display->renderer);
   if (display->window)
      SDL_DestroyWindow(display->window);
   if (display->framebuffer)
      free(display->framebuffer);
   memset(display, 0, sizeof(*display));
}

static void sdl3_select_display(struct sdl3_display *display)
{
   current_display = display;
   window = display ? display->window : NULL;
   framebuffer = display ? display->framebuffer : NULL;
   vid_width = display ? display->width : 0;
   vid_height = display ? display->height : 0;
}

/* Update routines, these use the generic blit.h code */
static void sdl3_update_16_to_32bpp(struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define DEST framebuffer
#define DEST_WIDTH vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef INDIRECT
}

static void sdl3_update_rgb_direct_32bpp(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL unsigned int
#define DEST_PIXEL unsigned int
#define DEST framebuffer
#define DEST_WIDTH vid_width
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
}

/* Pick a window scale factor so that the (often tiny, e.g. 128x32 for a DMD)
   emulated display fills a reasonable part of the desktop. The scale is in
   pixels, not in (possibly fractionally scaled) desktop points, so the
   emulated pixels map to an integer number of screen pixels. */
static int sdl3_auto_window_scale(SDL_Window *window, int width, int height)
{
   const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
   const float density = SDL_GetWindowPixelDensity(window);
   SDL_Rect bounds;
   int scale = 1;

   if (display && SDL_GetDisplayUsableBounds(display, &bounds))
   {
      const int max_w = (int)(bounds.w * density) / 2;
      const int max_h = (int)(bounds.h * density) / 2;
      while ((width * (scale + 1) <= max_w) && (height * (scale + 1) <= max_h))
         scale++;
   }
   return scale;
}

/* (re)create the texture and framebuffer for a display size and size the window accordingly */
static int sdl3_resize_display(struct sdl3_display *display, int width, int height)
{
   float density;
   int scale, pixel_w, pixel_h;

   if (display->texture)
      SDL_DestroyTexture(display->texture);
   if (display->framebuffer)
      free(display->framebuffer);

   display->width = width;
   display->height = height;

   /* The window size is in desktop points, and the points to pixels factor
      (e.g. 1.75 for 175% desktop scaling on wayland, 2 on a retina mac) is
      only known once the window exists, so size it here to get an integer
      number of pixels per emulated pixel */
   SDL_SyncWindow(display->window);
   density = SDL_GetWindowPixelDensity(display->window);
   if (density <= 0.0f)
      density = 1.0f;
   scale = window_scale ? window_scale : sdl3_auto_window_scale(display->window, width, height);
   SDL_SetWindowSize(display->window,
      (int)SDL_lroundf(width * scale / density), (int)SDL_lroundf(height * scale / density));
   SDL_SyncWindow(display->window);
   SDL_GetWindowSizeInPixels(display->window, &pixel_w, &pixel_h);
   fprintf(stderr, "SDL3: Info: %dx%d display, window scale factor %d, %dx%d pixels\n",
      width, height, scale, pixel_w, pixel_h);

   /* Keep the aspect ratio when the window gets resized or goes fullscreen */
   SDL_SetRenderLogicalPresentation(display->renderer, width, height,
      SDL_LOGICAL_PRESENTATION_LETTERBOX);

   display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_XRGB8888,
      SDL_TEXTUREACCESS_STREAMING, width, height);
   if (!display->texture)
   {
      fprintf(stderr, "SDL3: Error: Creating texture failed: %s\n", SDL_GetError());
      return OSD_NOT_OK;
   }
   /* pixel art mode keeps pixels crisp also at non integer scale factors (fractional desktop scaling) */
   SDL_SetTextureScaleMode(display->texture, linear_filter ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_PIXELART);

   display->framebuffer = calloc((size_t)width * height, sizeof(unsigned int));
   if (!display->framebuffer)
   {
      fprintf(stderr, "SDL3: Error: Allocating the framebuffer failed\n");
      return OSD_NOT_OK;
   }

   /* Present a first (black) frame right away: on Wayland a window without
      a presented frame is not shown at all, and the game window can be
      switched away from before it ever got a frame (e.g. when the debugger
      takes focus at startup) */
   SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
   SDL_RenderClear(display->renderer);
   SDL_RenderPresent(display->renderer);

   return OSD_OK;
}

/* create the window and renderer for a display */
static int sdl3_open_display(struct sdl3_display *display, const char *window_title)
{
   SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

   if (start_fullscreen && display == &displays[DISPLAY_GAME])
      window_flags |= SDL_WINDOW_FULLSCREEN;

   display->window = SDL_CreateWindow(window_title, 64, 64, window_flags);
   if (!display->window)
   {
      fprintf(stderr, "SDL3: Error: Creating window failed: %s\n", SDL_GetError());
      return OSD_NOT_OK;
   }

   display->renderer = SDL_CreateRenderer(display->window, NULL);
   if (!display->renderer)
   {
      fprintf(stderr, "SDL3: Error: Creating renderer failed: %s\n", SDL_GetError());
      return OSD_NOT_OK;
   }
   fprintf(stderr, "SDL3: Info: Using the %s renderer\n", SDL_GetRendererName(display->renderer));

   if (!SDL_SetRenderVSync(display->renderer, use_vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED))
      fprintf(stderr, "SDL3: Warning: Setting vsync failed: %s\n", SDL_GetError());

   return OSD_OK;
}

int sysdep_create_display(int depth)
{
   struct sdl3_display *display = &displays[DISPLAY_GAME];
   int width = visual_width * widthscale;
   int height = visual_height * heightscale;
   char window_title[sizeof(title) + 16];

   if (yarbsize)
      height = yarbsize;

   switch (depth)
   {
      case 16:
         update_function = sdl3_update_16_to_32bpp;
         break;
      case 32:
         update_function = sdl3_update_rgb_direct_32bpp;
         break;
      default:
         fprintf(stderr, "SDL3: Error: Unsupported bitmap depth %d\n", depth);
         return OSD_NOT_OK;
   }

   snprintf(window_title, sizeof(window_title), "%s", title);
#ifdef MAME_DEBUG
   /* the debugger display is recognized by its size (-debug_resolution) */
   if (options.mame_debug && width == options.debug_width && height == options.debug_height)
   {
      display = &displays[DISPLAY_DEBUGGER];
      snprintf(window_title, sizeof(window_title), "%s - debugger", title);
   }
#endif

   /* the window is created once and reused (and resized when needed) when
      the display is switched away from and back */
   if (!display->window)
   {
      if (sdl3_open_display(display, window_title) != OSD_OK)
      {
         sdl3_destroy_display(display);
         return OSD_NOT_OK;
      }
      if (sdl3_resize_display(display, width, height) != OSD_OK)
      {
         sdl3_destroy_display(display);
         return OSD_NOT_OK;
      }

      /* put the debugger next to the game window (where the window manager allows it) */
      if (display == &displays[DISPLAY_DEBUGGER] && displays[DISPLAY_GAME].window)
      {
         int x, y, w, h;
         if (SDL_GetWindowPosition(displays[DISPLAY_GAME].window, &x, &y)
            && SDL_GetWindowSize(displays[DISPLAY_GAME].window, &w, &h))
            SDL_SetWindowPosition(display->window, x + w + 16, y);
      }
   }
   else
   {
      if ((display->width != width || display->height != height)
         && sdl3_resize_display(display, width, height) != OSD_OK)
      {
         sdl3_destroy_display(display);
         return OSD_NOT_OK;
      }
      SDL_RaiseWindow(display->window);
   }

   sdl3_select_display(display);

   /* fill the display_palette_info struct, XRGB8888 */
   memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
   display_palette_info.depth = vid_depth;
   display_palette_info.red_mask   = 0x00FF0000;
   display_palette_info.green_mask = 0x0000FF00;
   display_palette_info.blue_mask  = 0x000000FF;

   if (display == &displays[DISPLAY_GAME])
      SDL_HideCursor();
   else
      SDL_ShowCursor();

   effect_init2(depth, vid_depth, vid_width);

   return OSD_OK;
}

void sysdep_update_display(struct mame_bitmap *bitmap)
{
   struct sdl3_display *display = current_display;

   if (!display)
      return;

   (*update_function)(bitmap);

   SDL_UpdateTexture(display->texture, NULL, display->framebuffer, display->width * sizeof(unsigned int));
   SDL_RenderClear(display->renderer);
   SDL_RenderTexture(display->renderer, display->texture, NULL, NULL);
   SDL_RenderPresent(display->renderer);
}

/* Called when the display is switched (game <-> debugger, or a size change,
   which is handled in sysdep_create_display) and at exit. The windows are
   kept, sysdep_close destroys them. */
void sysdep_display_close(void)
{
   SDL_ShowCursor();
   sdl3_select_display(NULL);
}

/* We only support truecolor, so nothing to do for the palette calls */
int sysdep_display_alloc_palette(int totalcolors)
{
   return 0;
}

int sysdep_display_set_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
   return 0;
}

void sysdep_mouse_poll(void)
{
   float x, y;
   int i;
   SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&x, &y);

   mouse_data[0].deltas[0] = (int)x;
   mouse_data[0].deltas[1] = (int)y;
   for (i = 0; i < MOUSE_BUTTONS; i++)
      mouse_data[0].buttons[i] = (buttons & SDL_BUTTON_MASK(i + 1)) ? 1 : 0;
}

void sysdep_set_leds(int leds)
{
}

/*
 * Joystick support, selected with -joytype 7. Joysticks are opened here and
 * the events are handled in sysdep_update_keyboard.
 */
static int sdl3_joystick_index(SDL_JoystickID id)
{
   int i;
   for (i = 0; i < JOY; i++)
      if (joysticks[i] && SDL_GetJoystickID(joysticks[i]) == id)
         return i;
   return -1;
}

static void sdl3_joystick_add(SDL_JoystickID id)
{
   int i, j;

   if (sdl3_joystick_index(id) >= 0)
      return;

   for (i = 0; i < JOY; i++)
      if (!joysticks[i])
         break;
   if (i == JOY)
   {
      fprintf(stderr, "SDL3: Warning: More than %d joysticks, ignoring joystick %u\n", JOY, id);
      return;
   }

   joysticks[i] = SDL_OpenJoystick(id);
   if (!joysticks[i])
   {
      fprintf(stderr, "SDL3: Warning: Opening joystick %u failed: %s\n", id, SDL_GetError());
      return;
   }

   joy_data[i].fd = i;
   joy_data[i].num_buttons = SDL_GetNumJoystickButtons(joysticks[i]);
   joy_data[i].num_axis = SDL_GetNumJoystickAxes(joysticks[i]);
   if (joy_data[i].num_buttons > JOY_BUTTONS)
      joy_data[i].num_buttons = JOY_BUTTONS;
   if (joy_data[i].num_axis > JOY_AXIS)
      joy_data[i].num_axis = JOY_AXIS;
   for (j = 0; j < joy_data[i].num_axis; j++)
   {
      joy_data[i].axis[j].min = -32768;
      joy_data[i].axis[j].center = 0;
      joy_data[i].axis[j].max = 32767;
   }

   fprintf(stderr, "SDL3: Info: Joystick %d: %s, %d axes, %d buttons\n", i,
      SDL_GetJoystickName(joysticks[i]), joy_data[i].num_axis, joy_data[i].num_buttons);
}

static void sdl3_joystick_remove(SDL_JoystickID id)
{
   int i = sdl3_joystick_index(id);
   if (i < 0)
      return;

   fprintf(stderr, "SDL3: Info: Joystick %d removed\n", i);
   SDL_CloseJoystick(joysticks[i]);
   joysticks[i] = NULL;
   memset(&joy_data[i], 0, sizeof(joy_data[i]));
   joy_data[i].fd = -1;
}

static void sdl3_joystick_poll(void)
{
   /* the axis/button values are updated by the event loop in
      sysdep_update_keyboard, here we only derive the digital directions */
   joy_evaluate_moves();
}

void joy_SDL_init(void)
{
   int i, count = 0;
   SDL_JoystickID *ids;

   if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
   {
      fprintf(stderr, "SDL3: Warning: Joystick init failed: %s\n", SDL_GetError());
      return;
   }
   joystick_driver_active = 1;

   for (i = 0; i < JOY; i++)
      joy_data[i].fd = -1;

   ids = SDL_GetJoysticks(&count);
   if (ids)
   {
      for (i = 0; i < count; i++)
         sdl3_joystick_add(ids[i]);
      SDL_free(ids);
   }
   fprintf(stderr, "SDL3: Info: %d joystick(s) found\n", count);

   joy_poll_func = sdl3_joystick_poll;
}

static unsigned short sdl3_key_unicode(const SDL_KeyboardEvent *key)
{
   /* osd_readkey_unicode() is used for text entry in the ui, approximate the
      typed character from the keycode and the modifiers */
   SDL_Keycode code = SDL_GetKeyFromScancode(key->scancode, key->mod, false);
   if (code >= 0x20 && code < 0x7f)
      return (unsigned short)code;
   if (code == SDLK_RETURN || code == SDLK_KP_ENTER)
      return '\r';
   if (code == SDLK_BACKSPACE || code == SDLK_TAB || code == SDLK_ESCAPE)
      return (unsigned short)code;
   return 0;
}

void sysdep_update_keyboard(void)
{
   struct xmame_keyboard_event kevent;
   SDL_Event event;

   if (!displays[DISPLAY_GAME].window && !displays[DISPLAY_DEBUGGER].window)
      return;

   while (SDL_PollEvent(&event))
   {
      switch (event.type)
      {
         case SDL_EVENT_KEY_DOWN:
         case SDL_EVENT_KEY_UP:
            if (event.key.repeat)
               break;

            /* ALT-Enter: toggle fullscreen */
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_RETURN
               && (event.key.mod & SDL_KMOD_ALT))
            {
               SDL_Window *key_window = SDL_GetWindowFromID(event.key.windowID);
               if (key_window)
                  SDL_SetWindowFullscreen(key_window, !(SDL_GetWindowFlags(key_window) & SDL_WINDOW_FULLSCREEN));
               break;
            }

            if (event.key.scancode >= SDL_SCANCODE_COUNT)
               break;
            kevent.press = (event.type == SDL_EVENT_KEY_DOWN);
            kevent.scancode = scancode_to_key[event.key.scancode];
            kevent.unicode = kevent.press ? sdl3_key_unicode(&event.key) : 0;
            if (kevent.scancode == KEY_NONE)
            {
               fprintf(stderr, "SDL3: Warning: Unmapped key %s (scancode 0x%x)\n",
                  SDL_GetScancodeName(event.key.scancode), event.key.scancode);
               break;
            }
            xmame_keyboard_register_event(&kevent);
            break;

         case SDL_EVENT_QUIT:
         case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            /* exit cleanly (saving nvram etc), keyboard.c fakes the escape
               presses for this */
            trying_to_quit = 1;
            break;

         case SDL_EVENT_JOYSTICK_ADDED:
            if (joystick_driver_active)
               sdl3_joystick_add(event.jdevice.which);
            break;

         case SDL_EVENT_JOYSTICK_REMOVED:
            if (joystick_driver_active)
               sdl3_joystick_remove(event.jdevice.which);
            break;

         case SDL_EVENT_JOYSTICK_AXIS_MOTION:
         {
            int joy = sdl3_joystick_index(event.jaxis.which);
            if (joy >= 0 && event.jaxis.axis < JOY_AXIS)
               joy_data[joy].axis[event.jaxis.axis].val = event.jaxis.value;
            break;
         }

         case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
         case SDL_EVENT_JOYSTICK_BUTTON_UP:
         {
            int joy = sdl3_joystick_index(event.jbutton.which);
            if (joy >= 0 && event.jbutton.button < JOY_BUTTONS)
               joy_data[joy].buttons[event.jbutton.button] = event.jbutton.down;
            break;
         }

         default:
            break;
      }
   }
}
