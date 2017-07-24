/* 
 vid_lisy.c part of LISY80
 bontango October 2016
 */

#include "xmame.h"
struct rc_option display_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "LISY Related", NULL,                   rc_seperator,   NULL,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_link,        mode_opts,
     NULL,              0,                      0,              NULL,
     NULL },
   { NULL,              NULL,                   rc_end,         NULL,
     NULL,              0,                      0,              NULL,
     NULL }
};

int sysdep_init(void)
{
   return OSD_OK;
}

void sysdep_close(void)
{
}

int sysdep_display_16bpp_capable(void)
{
  //return 1 or 0 ??
  return 1;

}

// Parse keyboard events
void sysdep_update_keyboard (void)
{
}

void sysdep_mouse_poll (void)
{
}

// This name doesn't really cover this function, since it also sets up mouse
//   and keyboard. This is done over here, since on most display targets the
//   mouse and keyboard can't be setup before the display has.
int sysdep_create_display(int depth)
{
 return OSD_OK;
}
