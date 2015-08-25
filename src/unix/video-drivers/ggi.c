/***************************************************************************

 Linux libGGI driver by Gabriele Boccone - clayton@dist.unige.it

  Something is recycled (and/or tweaked) from svgalib.c. This is only
  a "Quick and Dirty Hack"(TM) to make things interesting.

  Please if you test GGI-mame send me a mail, saying: "It works on my system"
  or "It does not work on my system", and what kind of computer you tested
  GGI-mame on. If you also want to send me sugar, coffee, chocolate, etc,
  feel free to send it by e-mail.

  Adapted for xmame-0.31 by Christian Groessler - cpg@aladdin.de

  * tested with GGI 2.0 Beta2 *
***************************************************************************/
#ifdef ggi
#define __GGI_C

#include <ggi/ggi.h>
#include <signal.h>
#include <math.h>

/*#define KEY_DEBUG*/
/*#define GGI_DEBUG*/
/*#define CATCH_SIGNALS*/

#include "xmame.h"
#include "driver.h"
#include "devices.h"
#include "keyboard.h"
#include "effect.h"

static int video_width,video_height;
static int scaled_visual_width,scaled_visual_height;
static ggi_visual_t vis = NULL;
static int screen_startx,screen_starty;
static int lastmouse[MOUSE_AXIS]={0,0,0,0,0,0,0,0};
static unsigned char *video_mem;
static unsigned char *doublebuffer_buffer = NULL; /* also used for scaling */
static ggi_mode mode;
static int use_linear = 0;
static int force_x,force_y;

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "GGI Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "linear",		NULL,			rc_bool,	&use_linear,
     "0",		0,			0,		NULL,
     "Enable/disable use of linear framebuffer (fast)" },
   { "xres",            NULL,                   rc_int,         &force_x,
     "0",               0,                      0,              NULL,
     "Force the X resolution" },
   { "yres",            NULL,                   rc_int,         &force_y,
     "0",               0,                      0,              NULL,
     "Force the Y resolution" },
   { NULL,		NULL,			rc_link,	mode_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

struct mode_list {
    int width;
    int height;
};

/* table of modes to try */
static struct mode_list ggimodes[] = {
    {  320,  200 },
    {  320,  240 },
    {  360,  240 },
    {  360,  400 },
    {  360,  480 },
    {  320,  400 },
    {  320,  480 },
    {  400,  300 },
    {  600,  400 },
    {  640,  200 },
    {  640,  400 },
    {  640,  480 },
    {  800,  600 },
    { 1024,  768 },
    { 1280, 1024 },
    { 2048, 1536 }
};
#define ML_ANZ (sizeof(ggimodes) / sizeof(struct mode_list))   /* # of entries in table */

static int first_call=TRUE;
/* do we need this? It makes debugging crashes sorta hard without a core file */
#ifdef CATCH_SIGNALS
static void (*oldsigsegvh)(int) = NULL;
static void (*oldsigbush)(int) = NULL;
static void (*oldsigquith)(int) = NULL;
#endif

static void ggi_cleanup(void);
static void (*update_function)(struct mame_bitmap *bitmap);
static void ggi_update_16_to_16bpp(struct mame_bitmap *bitmap);
static void ggi_update_16_to_16bpp_scaled(struct mame_bitmap *bitmap);
static void ggi_update_16_to_24bpp(struct mame_bitmap *bitmap);
static void ggi_update_16_to_32bpp(struct mame_bitmap *bitmap);
static void ggi_update_linear_16_to_16bpp(struct mame_bitmap *bitmap);
static void ggi_update_linear_16_to_24bpp(struct mame_bitmap *bitmap);
static void ggi_update_linear_16_to_32bpp(struct mame_bitmap *bitmap);


/* do we need this? It makes debugging crashes sorta hard without a core file */
#ifdef CATCH_SIGNALS
/* emergency signal handler: try to restore things */

static void myhandler(int signum)
{
    char *signam;
    char tmpbuf[32];
    void (*orgh)(int) = NULL;
    switch(signum) {
        case SIGSEGV:
            signam="SIGSEGV";
            orgh=oldsigsegvh;
            break;
        case SIGBUS:
            signam="SIGBUS";
            orgh=oldsigbush;
            break;
        case SIGQUIT:
            signam="SIGQUIT";
            orgh=oldsigquith;
            break;
        default:
            sprintf(tmpbuf,"unknown(%d)",signum);
            signam=tmpbuf;
    }
    fprintf(stderr_file,"%s: aborting...\n",signam);
    if (first_call) {
        first_call=FALSE;
        ggi_cleanup(); /* try again once */
    }
    if (orgh) orgh(signum);
    exit(255);
}
#endif

int sysdep_init(void)
{
#ifdef GGI_DEBUG
   if (stderr_file)
      fprintf(stderr_file,"sysdep_init called\n");
#endif

   if (ggiInit())
   {
      fprintf(stderr, "Unable to initialize GGI subsystem!\n"); /* sounds good, doesn't it? */
      return OSD_NOT_OK;
   }

   return OSD_OK;
}

void sysdep_close(void)
{
#ifdef GGI_DEBUG
    fprintf(stderr_file,"sysdep_close called\n");
#endif
    ggiExit();
}


int sysdep_display_16bpp_capable(void)
{
   /* ehm ? */
   return 1;
}

/*
 * check whether a given mode exists
 * try 8bit and >8bit color depths as needed
 * 15-Oct-1999, chris
 */
static int ggi_check_mode(ggi_visual_t vis, int w, int h, int depth,
   ggi_graphtype *type)
{
    ggi_mode mode;

#ifdef GGI_DEBUG
    fprintf(stderr_file,"ggi_check_mode called (%dx%d)\n",w,h);
#endif
    memset(&mode,0xff,sizeof(mode));
    /* try 8bit color depth */
    if ((depth == 8) &&
        (! ggiCheckSimpleMode(vis, w, h, GGI_AUTO, GT_8BIT, &mode)))
    {
       *type = GT_8BIT;
       return(TRUE);
    }
    /* try 16bit color depth */
    if (! ggiCheckSimpleMode(vis, w, h, GGI_AUTO, GT_16BIT, &mode))
    {
       *type = GT_16BIT;
       return(TRUE);
    }
    /* try 15bit color depth */
    if (! ggiCheckSimpleMode(vis, w, h, GGI_AUTO, GT_15BIT, &mode))
    {
       *type = GT_15BIT;
       return(TRUE);
    }
    /* try 24bit color depth */
    if (! ggiCheckSimpleMode(vis, w, h, GGI_AUTO, GT_24BIT, &mode))
    {
       *type = GT_24BIT;
       return(TRUE);
    }
    /* try 32bit color depth */
    if (! ggiCheckSimpleMode(vis, w, h, GGI_AUTO, GT_32BIT, &mode))
    {
       *type = GT_32BIT;
       return(TRUE);
    }
    return(FALSE);
}

/*
 * determine video mode to set:
 * must have >= colors as game (bitmap->depth == 16)
 * must be >= resolution than game (visual_width/visual_height)
 * 03-Nov-1999, chris
 * 14-Mar-2000, chris, force_x + force_y
 */
static int set_video_mode(int depth)
{
    int i, best_score = 0;
    ggi_graphtype type, best_type;
    const ggi_directbuffer *direct_buf;
    typedef void(*updater_t)(struct mame_bitmap *bitmap);
    updater_t updaters[] = {
        /* linear updaters */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        ggi_update_linear_16_to_16bpp,
        ggi_update_linear_16_to_24bpp,
        ggi_update_linear_16_to_32bpp,
        /* non-linear updaters */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        ggi_update_16_to_16bpp,
        ggi_update_16_to_24bpp,
        ggi_update_16_to_32bpp,
        /* scaled non-linear updaters */
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        ggi_update_16_to_16bpp_scaled,
        ggi_update_16_to_24bpp,
        ggi_update_16_to_32bpp
    };
    int updater = 0;

#ifdef GGI_DEBUG
    fprintf(stderr_file,"set_video_mode called\n");
#endif
    scaled_visual_width = visual_width  * widthscale;
    scaled_visual_height = visual_height * heightscale;

    if (force_x == 0)
        video_width = scaled_visual_width;
    else
        video_width = force_x;

    if (force_y == 0)
        video_height = scaled_visual_height;
    else
        video_height = force_y;

    if (video_height < scaled_visual_height || video_width < scaled_visual_width) {
        fprintf(stderr_file,"Forced resolution %dx%d < needed resolution %dx%d -- aborting...\n",
                video_width,video_height,scaled_visual_width,scaled_visual_height);
        return(FALSE);
    }

    if (force_x || force_y)
        fprintf(stderr_file,"Command line override: setting mode %dx%d\n",video_width,video_height);

    /* some GGI stuff */
    vis = ggiOpen(NULL);
    ggiSetFlags(vis, GGIFLAG_ASYNC);
    ggiSetEventMask(vis, emKey | emPointer);

    /* first try exact game resolution... */
    if (! ggi_check_mode(vis, video_width, video_height, depth, &best_type) &&
        (!force_x && !force_y))
    {
        int w, h, score;
        /* now try from my [just hacked] list of modes */
        /* (grrr -- isn't there a way to get all supported modes from GGI?) */
        for (i=0; i<ML_ANZ; i++)
        {
            w = ggimodes[i].width;
            h = ggimodes[i].height;
            if (ggi_check_mode(vis, w, h, depth, &type)) {
                score = mode_match(w,h);
                if (score && score >= best_score) {
                    best_score = score;
                    best_type  = type;
                    video_width = w; video_height = h;
                }
            }
        }
        if (! best_score) {
            fprintf(stderr_file, "GGI: Couldn't find a suitable mode for a resolution of %dx%d\n"
                    "Trying to get any mode....\n",
                    scaled_visual_width,scaled_visual_height);
            /* trying to get any mode from GGI */
            if (ggiSetSimpleMode(vis,GGI_AUTO,GGI_AUTO,GGI_AUTO,GT_AUTO) != 0) {
                fprintf(stderr_file, "GGI: Couldn't find a suitable mode for a resolution of %dx%d\n",
                        scaled_visual_width,scaled_visual_height);
                return(FALSE);
            }
            goto mode_set;
        }
    }

    if (ggiCheckSimpleMode(vis, video_width, video_height, GGI_AUTO,
                           best_type, &mode) != 0)
       return(FALSE);

    if (ggiSetMode(vis, &mode) != 0)
       return(FALSE);

 mode_set:

    ggiGetMode(vis, &mode); /* Maybe we did not get what we asked for */
    if ((mode.visible.x < scaled_visual_width)||
	(mode.visible.y < scaled_visual_height)) {
	fprintf(stderr_file,
		"Fatal: cannot get big enough mode %dx%d\n",
		scaled_visual_width,scaled_visual_height);
        return(FALSE);
    }
    if ((mode.visible.x != scaled_visual_width)||
	(mode.visible.y != scaled_visual_height)) {
	fprintf(stderr_file,
		"Notice: cannot get ideal mode %dx%d, setting to %dx%d\n",
		scaled_visual_width,scaled_visual_height,mode.visible.x,mode.visible.y);
    }
    video_width   = mode.visible.x;
    video_height  = mode.visible.y;
    screen_startx = ((video_width - scaled_visual_width) / 2) & ~7;
    screen_starty = (video_height - scaled_visual_height) / 2;
    
    /* choose the correct updater for this graphtype */
    updater += (GT_SIZE(mode.graphtype) / 8) - 1;
    
    /* can we do linear ? */
    if (use_linear && (direct_buf = ggiDBGetBuffer(vis,0)) &&
        (direct_buf->type & GGI_DB_SIMPLE_PLB) )
    {
        if ((widthscale > 1 || heightscale > 2))
        {
           doublebuffer_buffer = malloc(scaled_visual_width * 
              GT_SIZE(mode.graphtype) / 8);
           if (!doublebuffer_buffer)
           {
              fprintf(stderr_file, "GGI: Error: Couldn't allocate doublebuffer buffer\n");
              return FALSE;
           }
        }
        video_mem = direct_buf->write;
        video_mem += screen_startx * GT_SIZE(mode.graphtype) / 8;
        video_mem += screen_starty * video_width *
           GT_SIZE(mode.graphtype) / 8;
#ifdef GGI_DEBUG
        fprintf(stderr_file,
           "ggi.c: set_video_mode: using %d bit linear update\n",
           GT_SIZE(mode.graphtype));
#endif
    }
    else
    {
        if((widthscale == 1) && (heightscale == 1))
           updater += 8;
        else
           updater += 16;
        /* we need the doublebuffer_buffer in the following scenarios:
           -scale != 1x1
           -16bpp modes, since it could be paletised
           -if the depths don't match */
        if( (widthscale > 1) || (heightscale > 1) || (depth == 16) ||
            (depth != GT_SIZE(mode.graphtype)) )
        {
           doublebuffer_buffer = malloc(scaled_visual_width*scaled_visual_height*
              GT_SIZE(mode.graphtype) / 8);
           if (!doublebuffer_buffer)
           {
              fprintf(stderr_file, "GGI: Error: Couldn't allocate doublebuffer buffer\n");
              return FALSE;
           }
        }
    }
    
    if (depth == 16)
       updater+=4;
       
    update_function = updaters[updater];
    
    /* fill the display_palette_info */
    memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
    display_palette_info.depth = GT_SIZE(mode.graphtype);
    switch (GT_SIZE(mode.graphtype))
    {
       case 8:
          display_palette_info.writable_colors = 256;
          break;
       case 15:
          display_palette_info.red_mask   = 0x001F;
          display_palette_info.green_mask = 0x03E0;
          display_palette_info.blue_mask  = 0xEC00;
          break;
       case 16:
          display_palette_info.red_mask   = 0xF800;
          display_palette_info.green_mask = 0x07E0;
          display_palette_info.blue_mask  = 0x001F;
          break;
       case 24:
       case 32:
          display_palette_info.red_mask   = 0xFF0000;
          display_palette_info.green_mask = 0x00FF00;
          display_palette_info.blue_mask  = 0x0000FF;
          break;
    }
    
    return TRUE;
}


/*
 * parts from svgalib.c version
 */
int sysdep_create_display(int depth)
{
#ifdef GGI_DEBUG
    fprintf(stderr_file,"sysdep_create_display called\n");
#endif
/* do we need this? It makes debugging crashes sorta hard without a core file */
#ifdef CATCH_SIGNALS
    oldsigsegvh=signal(SIGSEGV,myhandler);
    oldsigbush=signal(SIGBUS,myhandler);
    oldsigquith=signal(SIGQUIT,myhandler);
    if (oldsigsegvh == SIG_ERR || oldsigbush == SIG_ERR || oldsigquith == SIG_ERR) {
	fprintf (stderr_file, "Cannot install signal handler. Exiting\n");
	return OSD_NOT_OK;
    }
#endif
    if (! set_video_mode(depth)) {
        fprintf(stderr_file,"cannot find a mode to use :-(\n");
        return OSD_NOT_OK;
    }

    fprintf(stderr_file,"GGI: using mode %dx%d\n",video_width,video_height);
#ifdef GGI_DEBUG
    fprintf(stderr_file,"16bit game: %s\n",(bitmap->depth == 16) ? "yes" : "no");
#endif

   effect_init2(depth, display_palette_info.depth, video_width);

    return OSD_OK;
}


/*
 * close down the display
 */
void sysdep_display_close(void)
{
#ifdef GGI_DEBUG
    fprintf(stderr_file,"sysdep_display_close called\n");
#endif
/* do we need this? It makes debugging crashes sorta hard without a core file */
#ifdef CATCH_SIGNALS
    if (oldsigsegvh) signal(SIGSEGV,oldsigsegvh);
    if (oldsigbush) signal(SIGBUS,oldsigbush);
    if (oldsigquith) signal(SIGBUS,oldsigquith);
#endif
    ggi_cleanup();
#ifdef GGI_DEBUG
    fprintf(stderr_file,"sysdep_display_close finished\n");
#endif
}


static void ggi_cleanup(void)
{
#ifdef GGI_DEBUG
    fprintf(stderr_file,"ggi_cleanup called\n");
#endif
    if (vis) {
      ggiClose(vis);
      vis=NULL;
    }
    if (doublebuffer_buffer) free(doublebuffer_buffer);
}


int sysdep_display_alloc_palette(int writable_colors)
{
#ifdef GGI_DEBUG
    fprintf(stderr_file,"sysdep_alloc_palette called\n");
#endif

    return 0;
}


int sysdep_display_set_pen(int pen,unsigned char red, unsigned char green,
   unsigned char blue)
{
    ggi_color gpen;

#if 0 && defined(GGI_DEBUG)
    fprintf(stderr_file,"sysdep_modify_pen called\n");
#endif
    if (!vis) return -1; /* visual not initialized yet -- do nothing */

    gpen.r = red << 8;
    gpen.g = green << 8;
    gpen.b = blue << 8;
    ggiSetPalette(vis, pen, 1, &gpen);
    
    return 0;
}


/*
 * low-level update routines
 * for different color depths
 * (04-Nov-99, they are working, but could
 *  be improved: move calculations of
 *  e.g. "X+screen_startx" out of the loops...)
 */

static void ggi_update_16_to_16bpp(struct mame_bitmap *bitmap)
{
   if (current_palette->lookup)
   {
      /* since we need todo the lookups we need to go through an extra buffer,
         just like ggi_update_16_to_16bpp_scaled() does */
      ggi_update_16_to_16bpp_scaled(bitmap);
   }
   else
   {
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
    { \
        int _i; \
        for (_i=0; _i<HEIGHT; _i++) { \
            ggiPutHLine(vis,screen_startx+(X),screen_starty+(Y)+_i, \
                        WIDTH,bitmap->line[(Y)+visual.min_y+_i]+(visual.min_x+(X))*2); \
        } \
    }
#include "blit.h"
#undef PUT_IMAGE
   }
}

/*---------*/

void ggi_update_16_to_16bpp_scaled(struct mame_bitmap *bitmap)
{
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned short
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
    { \
        int _i; \
        for (_i=0; _i<HEIGHT; _i++) { \
            ggiPutHLine(vis,screen_startx+(X),screen_starty+(Y)+_i, \
                        WIDTH,DEST + (X) * sizeof(DEST_PIXEL) + DEST_WIDTH * sizeof(DEST_PIXEL) * (_i + (Y)) ); \
        } \
    }
    
    if (current_palette->lookup)
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
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef PUT_IMAGE
}

/*---------*/

static void ggi_update_16_to_24bpp(struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PACK_BITS
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
    do { \
        int _i; \
        for (_i=0; _i<HEIGHT; _i++) { \
            ggiPutHLine(vis,screen_startx+(X),screen_starty+(Y)+_i, \
                        WIDTH,DEST + (X) * 3 + DEST_WIDTH * 3 * (_i + (Y)) ); \
        } \
    } while(0);
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef PUT_IMAGE
#undef PACK_BITS
#undef INDIRECT
}

/*---------*/

static void ggi_update_16_to_32bpp(struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define DEST doublebuffer_buffer
#define DEST_WIDTH scaled_visual_width
#define PUT_IMAGE(X, Y, WIDTH, HEIGHT) \
    do { \
        int _i; \
        for (_i=0; _i<HEIGHT; _i++) { \
            ggiPutHLine(vis,screen_startx+(X),screen_starty+(Y)+_i, \
                        WIDTH,DEST + (X) * sizeof(DEST_PIXEL) + DEST_WIDTH * sizeof(DEST_PIXEL) * (_i + (Y)) ); \
        } \
    } while(0);
#include "blit.h"
#undef DEST_WIDTH
#undef DEST
#undef DEST_PIXEL
#undef SRC_PIXEL
#undef PUT_IMAGE
#undef INDIRECT
}

/*---------*/

static void ggi_update_linear_16_to_16bpp(struct mame_bitmap *bitmap)
{
#define DEST_PIXEL unsigned short
#define SRC_PIXEL unsigned short
#define DEST video_mem
#define DEST_WIDTH mode.virt.x
#define DOUBLEBUFFER
    if (current_palette->lookup)
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

/*---------*/

static void ggi_update_linear_16_to_24bpp(struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define DEST_PIXEL unsigned int
#define SRC_PIXEL unsigned short
#define PACK_BITS
#define DEST video_mem
#define DEST_WIDTH mode.virt.x
#define DOUBLEBUFFER
#include "blit.h"
#undef DEST
#undef DEST_WIDTH
#undef DOUBLEBUFFER
#undef PACK_BITS
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef INDIRECT
}

/*---------*/

static void ggi_update_linear_16_to_32bpp(struct mame_bitmap *bitmap)
{
#define INDIRECT current_palette->lookup
#define DEST_PIXEL unsigned int
#define SRC_PIXEL unsigned short
#define DEST video_mem
#define DEST_WIDTH mode.virt.x
#define DOUBLEBUFFER
#include "blit.h"
#undef PACK_BITS
#undef DEST_WIDTH
#undef DOUBLEBUFFER
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef INDIRECT
}


/*
 * Update the display.
 */
void sysdep_update_display(struct mame_bitmap *bitmap) {

    update_function(bitmap);
    ggiFlush(vis);
}

int ggi_key(ggi_event *ev)
{
    unsigned int keycode=KEY_NONE;
    int label = ev->key.label;

#ifdef KEY_DEBUG
    fprintf(stderr_file,
        "Keyevent detected: sym = 0x%02x, code = 0x%02x, label = 0x%02x\n",
        ev->key.sym, ev->key.button, label);
#endif

    switch (label >> 8)
    {
       case GII_KT_LATIN1:
          switch (label) { /* for now, the simple way */
              case GIIUC_BackSpace:  keycode = KEY_BACKSPACE;  break;
              case GIIUC_Tab:        keycode = KEY_TAB;        break;
              case GIIUC_Linefeed:   keycode = KEY_ENTER;      break;
              case GIIUC_Return:     keycode = KEY_ENTER;      break;
              case GIIUC_Escape:     keycode = KEY_ESC;        break;
              case GIIUC_Delete:     keycode = KEY_DEL;        break;
              case GIIUC_Space:      keycode = KEY_SPACE;      break;
              case GIIUC_Exclam:     keycode = KEY_1;          break;
              case GIIUC_QuoteDbl:   keycode = KEY_QUOTE;      break;
              case GIIUC_Hash:       keycode = KEY_3;          break;
              case GIIUC_Dollar:     keycode = KEY_4;          break;
              case GIIUC_Percent:    keycode = KEY_5;          break;
              case GIIUC_Ampersand:  keycode = KEY_7;          break;
              case GIIUC_Apostrophe: keycode = KEY_QUOTE;      break;
              case GIIUC_ParenLeft:  keycode = KEY_9;          break;
              case GIIUC_ParenRight: keycode = KEY_0;          break;
              case GIIUC_Asterisk:   keycode = KEY_ASTERISK;   break;
              case GIIUC_Plus:       keycode = KEY_EQUALS;     break;
              case GIIUC_Comma:      keycode = KEY_COMMA;      break;
              case GIIUC_Minus:      keycode = KEY_MINUS;      break;
              case GIIUC_Period:     keycode = KEY_STOP;       break;
              case GIIUC_Slash:      keycode = KEY_SLASH;      break;
              case GIIUC_0:          keycode = KEY_0;          break;
              case GIIUC_1:          keycode = KEY_1;          break;
              case GIIUC_2:          keycode = KEY_2;          break;
              case GIIUC_3:          keycode = KEY_3;          break;
              case GIIUC_4:          keycode = KEY_4;          break;
              case GIIUC_5:          keycode = KEY_5;          break;
              case GIIUC_6:          keycode = KEY_6;          break;
              case GIIUC_7:          keycode = KEY_7;          break;
              case GIIUC_8:          keycode = KEY_8;          break;
              case GIIUC_9:          keycode = KEY_9;          break;
              case GIIUC_Colon:      keycode = KEY_COLON;      break;
              case GIIUC_Semicolon:  keycode = KEY_COLON;      break;
              case GIIUC_Less:       keycode = KEY_COMMA;      break;
              case GIIUC_Equal:      keycode = KEY_EQUALS;     break;
              case GIIUC_Greater:    keycode = KEY_STOP;       break;
              case GIIUC_Question:   keycode = KEY_SLASH;      break;
              case GIIUC_At:         keycode = KEY_2;          break;
              case GIIUC_A:          keycode = KEY_A;          break;
              case GIIUC_B:          keycode = KEY_B;          break;
              case GIIUC_C:          keycode = KEY_C;          break;
              case GIIUC_D:          keycode = KEY_D;          break;
              case GIIUC_E:          keycode = KEY_E;          break;
              case GIIUC_F:          keycode = KEY_F;          break;
              case GIIUC_G:          keycode = KEY_G;          break;
              case GIIUC_H:          keycode = KEY_H;          break;
              case GIIUC_I:          keycode = KEY_I;          break;
              case GIIUC_J:          keycode = KEY_J;          break;
              case GIIUC_K:          keycode = KEY_K;          break;
              case GIIUC_L:          keycode = KEY_L;          break;
              case GIIUC_M:          keycode = KEY_M;          break;
              case GIIUC_N:          keycode = KEY_N;          break;
              case GIIUC_O:          keycode = KEY_O;          break;
              case GIIUC_P:          keycode = KEY_P;          break;
              case GIIUC_Q:          keycode = KEY_Q;          break;
              case GIIUC_R:          keycode = KEY_R;          break;
              case GIIUC_S:          keycode = KEY_S;          break;
              case GIIUC_T:          keycode = KEY_T;          break;
              case GIIUC_U:          keycode = KEY_U;          break;
              case GIIUC_V:          keycode = KEY_V;          break;
              case GIIUC_W:          keycode = KEY_W;          break;
              case GIIUC_X:          keycode = KEY_X;          break;
              case GIIUC_Y:          keycode = KEY_Y;          break;
              case GIIUC_Z:          keycode = KEY_Z;          break;
              case GIIUC_BracketLeft:  keycode = KEY_OPENBRACE;  break;
              case GIIUC_BackSlash:    keycode = KEY_BACKSLASH;  break;
              case GIIUC_BracketRight: keycode = KEY_CLOSEBRACE; break;
              case GIIUC_Circumflex:   keycode = KEY_6;          break;
              case GIIUC_Underscore:   keycode = KEY_MINUS;      break;
              case GIIUC_Grave:        keycode = KEY_TILDE;      break;
              case GIIUC_a:          keycode = KEY_A;          break;
              case GIIUC_b:          keycode = KEY_B;          break;
              case GIIUC_c:          keycode = KEY_C;          break;
              case GIIUC_d:          keycode = KEY_D;          break;
              case GIIUC_e:          keycode = KEY_E;          break;
              case GIIUC_f:          keycode = KEY_F;          break;
              case GIIUC_g:          keycode = KEY_G;          break;
              case GIIUC_h:          keycode = KEY_H;          break;
              case GIIUC_i:          keycode = KEY_I;          break;
              case GIIUC_j:          keycode = KEY_J;          break;
              case GIIUC_k:          keycode = KEY_K;          break;
              case GIIUC_l:          keycode = KEY_L;          break;
              case GIIUC_m:          keycode = KEY_M;          break;
              case GIIUC_n:          keycode = KEY_N;          break;
              case GIIUC_o:          keycode = KEY_O;          break;
              case GIIUC_p:          keycode = KEY_P;          break;
              case GIIUC_q:          keycode = KEY_Q;          break;
              case GIIUC_r:          keycode = KEY_R;          break;
              case GIIUC_s:          keycode = KEY_S;          break;
              case GIIUC_t:          keycode = KEY_T;          break;
              case GIIUC_u:          keycode = KEY_U;          break;
              case GIIUC_v:          keycode = KEY_V;          break;
              case GIIUC_w:          keycode = KEY_W;          break;
              case GIIUC_x:          keycode = KEY_X;          break;
              case GIIUC_y:          keycode = KEY_Y;          break;
              case GIIUC_z:          keycode = KEY_Z;          break;
              case GIIUC_BraceLeft:  keycode = KEY_OPENBRACE;  break;
              case GIIUC_Pipe:       keycode = KEY_BACKSLASH;  break;
              case GIIUC_BraceRight: keycode = KEY_CLOSEBRACE; break;
              case GIIUC_Tilde:      keycode = KEY_TILDE;      break;
          }
          break;
       case GII_KT_SPEC:
          switch (label) { /* for now, the simple way */
              case GIIK_Break:       keycode = KEY_PAUSE;      break;
              case GIIK_ScrollForw:  keycode = KEY_PGUP;       break;
              case GIIK_ScrollBack:  keycode = KEY_PGDN;       break;
              case GIIK_Menu:        keycode = KEY_MENU;       break;
              case GIIK_Cancel:      keycode = KEY_ESC;        break;
              case GIIK_PrintScreen: keycode = KEY_PRTSCR;     break;
              case GIIK_Execute:     keycode = KEY_ENTER;      break;
              case GIIK_Begin:       keycode = KEY_HOME;       break;
              case GIIK_Clear:       keycode = KEY_DEL;        break;
              case GIIK_Insert:      keycode = KEY_INSERT;     break;
              case GIIK_Select:      keycode = KEY_ENTER_PAD;  break;
              case GIIK_Pause:       keycode = KEY_PAUSE;      break;
              case GIIK_SysRq:       keycode = KEY_PRTSCR;     break;
              case GIIK_ModeSwitch:  keycode = KEY_ALTGR;      break;
              case GIIK_Up:          keycode = KEY_UP;         break;
              case GIIK_Down:        keycode = KEY_DOWN;       break;
              case GIIK_Left:        keycode = KEY_LEFT;       break;
              case GIIK_Right:       keycode = KEY_RIGHT;      break;
              case GIIK_PageUp:      keycode = KEY_PGUP;       break;
              case GIIK_PageDown:    keycode = KEY_PGDN;       break;
              case GIIK_Home:        keycode = KEY_HOME;       break;
              case GIIK_End:         keycode = KEY_END;        break;
          }
          break;
       case GII_KT_FN:
          switch (label) { /* for now, the simple way */
              case GIIK_F1:      keycode = KEY_F1;     break;
              case GIIK_F2:      keycode = KEY_F2;     break;
              case GIIK_F3:      keycode = KEY_F3;     break;
              case GIIK_F4:      keycode = KEY_F4;     break;
              case GIIK_F5:      keycode = KEY_F5;     break;
              case GIIK_F6:      keycode = KEY_F6;     break;
              case GIIK_F7:      keycode = KEY_F7;     break;
              case GIIK_F8:      keycode = KEY_F8;     break;
              case GIIK_F9:      keycode = KEY_F9;     break;
              case GIIK_F10:     keycode = KEY_F10;    break;
              case GIIK_F11:     keycode = KEY_F11;    break;
              case GIIK_F12:     keycode = KEY_F12;    break;
          }
          break;
       case GII_KT_PAD:
          switch (label) { /* for now, the simple way */
              case GIIK_P0:          keycode = KEY_0_PAD;      break;
              case GIIK_P1:          keycode = KEY_1_PAD;      break;
              case GIIK_P2:          keycode = KEY_2_PAD;      break;
              case GIIK_P3:          keycode = KEY_3_PAD;      break;
              case GIIK_P4:          keycode = KEY_4_PAD;      break;
              case GIIK_P5:          keycode = KEY_5_PAD;      break;
              case GIIK_P6:          keycode = KEY_6_PAD;      break;
              case GIIK_P7:          keycode = KEY_7_PAD;      break;
              case GIIK_P8:          keycode = KEY_8_PAD;      break;
              case GIIK_P9:          keycode = KEY_9_PAD;      break;
              case GIIK_PA:          keycode = KEY_A;          break;
              case GIIK_PB:          keycode = KEY_B;          break;
              case GIIK_PC:          keycode = KEY_C;          break;
              case GIIK_PD:          keycode = KEY_D;          break;
              case GIIK_PE:          keycode = KEY_E;          break;
              case GIIK_PF:          keycode = KEY_F;          break;
              case GIIK_PPlus:       keycode = KEY_PLUS_PAD;   break;
              case GIIK_PMinus:      keycode = KEY_MINUS_PAD;  break;
              case GIIK_PSlash:      keycode = KEY_SLASH_PAD;  break;
              case GIIK_PAsterisk:   keycode = KEY_ASTERISK;   break;
              case GIIK_PEqual:       keycode = KEY_ENTER_PAD;  break;
              case GIIK_PSeparator:  keycode = KEY_DEL_PAD;    break;
              case GIIK_PDecimal:    keycode = KEY_DEL_PAD;    break;
              case GIIK_PParenLeft:  keycode = KEY_9_PAD;      break;
              case GIIK_PParenRight: keycode = KEY_0_PAD;      break;
              case GIIK_PSpace:      keycode = KEY_SPACE;      break;
              case GIIK_PEnter:      keycode = KEY_ENTER_PAD;  break;
              case GIIK_PTab:        keycode = KEY_TAB;        break;
              case GIIK_PBegin:      keycode = KEY_HOME;       break;
              case GIIK_PF1:         keycode = KEY_F1;         break;
              case GIIK_PF2:         keycode = KEY_F2;         break;
              case GIIK_PF3:         keycode = KEY_F3;         break;
              case GIIK_PF4:         keycode = KEY_F4;         break;
              case GIIK_PF5:         keycode = KEY_F5;         break;
              case GIIK_PF6:         keycode = KEY_F6;         break;
              case GIIK_PF7:         keycode = KEY_F7;         break;
              case GIIK_PF8:         keycode = KEY_F8;         break;
              case GIIK_PF9:         keycode = KEY_F9;         break;
          }
          break;
       case GII_KT_MOD:
          switch (label) { /* for now, the simple way */
              case GIIK_ShiftL:      keycode = KEY_LSHIFT;     break;
              case GIIK_ShiftR:      keycode = KEY_RSHIFT;     break;
              case GIIK_CtrlL:       keycode = KEY_LCONTROL;   break;
              case GIIK_CtrlR:       keycode = KEY_RCONTROL;   break;
              case GIIK_AltL:        keycode = KEY_ALT;        break;
              case GIIK_AltR:        keycode = KEY_ALTGR;      break;
              case GIIK_MetaL:       keycode = KEY_LWIN;       break;
              case GIIK_MetaR:       keycode = KEY_RWIN;       break;
              case GIIK_ShiftLock:   keycode = KEY_CAPSLOCK;   break;
              case GIIK_CapsLock:    keycode = KEY_CAPSLOCK;   break;
              case GIIK_NumLock:     keycode = KEY_NUMLOCK;    break;
              case GIIK_ScrollLock:  keycode = KEY_SCRLOCK;    break;
          }
          break;
       case GII_KT_DEAD:
          switch (label) { /* for now, the simple way */
          }
          break;
    }
#ifdef KEY_DEBUG
    fprintf(stderr_file,"returning keycode = %d\n",keycode);
#endif
    return(keycode);
}

void sysdep_update_keyboard(void)
{
    ggi_event_mask em = emAll; /*emKeyPress | emKeyRelease;*/
    ggi_event ev;
    struct timeval to = { 0 , 0 };
    struct xmame_keyboard_event event;

    if (vis) {
        while(ggiEventPoll(vis,em,&to)) {
            event.press = 0;
            
            ggiEventRead(vis,&ev,em);

            switch(ev.any.type) {
              case evKeyPress:
                  event.press = 1;
              case evKeyRelease:
                  event.scancode = ggi_key(&ev);
                  event.unicode = ev.key.sym;
                  xmame_keyboard_register_event(&event);
                  break;
            }

            to.tv_sec=to.tv_usec=0;
        }
    }
    return;
}


/*
 * mouse not really tested
 */
void sysdep_mouse_poll(void)
{
    ggi_event_mask em = emPtrButtonPress | emPtrButtonRelease | emPtrMove;
    ggi_event ev;
    struct timeval to = { 0 , 0 };
    int bi;

    if (vis) {
        while(ggiEventPoll(vis,em,&to)) {
            ggiEventRead(vis,&ev,em);
            bi = 0;

            switch(ev.any.type) {

              case evPtrButtonPress:
                  bi = 1;
              case evPtrButtonRelease:
                  if (ev.pbutton.button < MOUSE_BUTTONS)
                     mouse_data[0].buttons[ev.pbutton.button] = bi;
                  break;
              case evPtrAbsolute:
                  mouse_data[0].deltas[0] = lastmouse[0] - ev.pmove.x;
                  mouse_data[0].deltas[1] = lastmouse[1] - ev.pmove.y;
                  lastmouse[0] = ev.pmove.x;
                  lastmouse[1] = ev.pmove.y;
                  break;
              case evPtrRelative:
                  mouse_data[0].deltas[0] = ev.pmove.x;
                  mouse_data[0].deltas[1] = ev.pmove.y;
                  lastmouse[0] += ev.pmove.x;
                  lastmouse[1] += ev.pmove.y;
                  break;
            }
            to.tv_sec=to.tv_usec=0;
        }
    }
}

void sysdep_set_leds(int leds)
{
}
#endif /* ifdef ggi */
