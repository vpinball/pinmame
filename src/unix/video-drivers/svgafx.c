/***************************************************************************

  Xmame 3Dfx console-mode driver

  Written based on Phillip Ezolt's svgalib driver by Mike Oliphant -

    oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

***************************************************************************/
#define __SVGAFX_C

#include <vga.h>
#include <glide.h>
#include "xmame.h"
#include "svgainput.h"

int  InitVScreen(void);
void CloseVScreen(void);
int  InitGlide(void);
int  SetResolution(struct rc_option *option, const char *arg, int priority);

extern struct rc_option fx_opts[];

struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL, 		NULL,			rc_link,	fx_opts,
     NULL,		0,			0,		NULL,
     NULL },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

int sysdep_init(void)
{
   fprintf(stderr,
      "info: using FXmame v0.5 driver for xmame, written by Mike Oliphant\n");
   
   if (InitGlide()!=OSD_OK)
      return OSD_NOT_OK;
   if (vga_init())
      return OSD_NOT_OK;
   if (svga_input_init())
      return OSD_NOT_OK;
   
   return OSD_OK;
}

void sysdep_close(void)
{
   svga_input_exit();
}

static void release_function(void)
{
   grSstControl(GR_CONTROL_DEACTIVATE);
}

static void acquire_function(void)
{
   grSstControl(GR_CONTROL_ACTIVATE);
}

/* This name doesn't really cover this function, since it also sets up mouse
   and keyboard. This is done over here, since on most display targets the
   mouse and keyboard can't be setup before the display has. */
int sysdep_create_display(int depth)
{
  if (InitVScreen() != OSD_OK)
     return OSD_NOT_OK;
     
  /* with newer svgalib's the console switch signals are only active if a
     graphics mode is set, so we set one which each card should support */
  vga_setmode(G320x200x16);
  
  /* init input */
  if(svga_input_open(release_function, acquire_function))
     return OSD_NOT_OK;
   
  return OSD_OK;
}


/* shut up the display */
void sysdep_display_close(void)
{
   /* close input */
   svga_input_close();
   
   /* close svgalib */
   vga_setmode(TEXT);
   
   /* close glide */
   CloseVScreen();
}
