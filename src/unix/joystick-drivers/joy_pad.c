/*
 ************************** FM TOWNS PAD specifics routines *************
 */

#include <fcntl.h>
#include "xmame.h"
#include "devices.h"
#include "pad.h"

/*
 * Towns Pad control module for Xmame
 *
 * Author : Osamu KURATI
 * Version : 29 apr 1997 0.000
 */

/*
 * Joy Stick Code
 * Up : 1
 * Down       : 2
 * Left       : 4
 * Right      : 8
 * A  : 10
 * B  : 20
 *
 *
 * PAD bit number
 * up         : 0
 * down               : 1
 * left               : 2
 * right      : 3
 * A          : 4
 * B          : 5
 * RUN                : 6
 * SELECT     : 7
 */

#ifdef LIN_FM_TOWNS
static unsigned long lPadLastButton = 0;
static char *towns_pad_dev = NULL; /* name of FM-TOWNS device */
#endif

struct rc_option joy_pad_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#ifdef LIN_FM_TOWNS
   { "paddevname",	NULL,			rc_string,	&towns_pad_dev,
     "/dev/pad00",	0,			0,		NULL,
     "Name of pad device (defaults to /dev/pad00)" },
#endif
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef LIN_FM_TOWNS
void joy_pad_poll(void);

void joy_pad_init(void)
{
  int i;
  
  joy_poll_func  = joy_pad_poll;
  lPadLastButton = 0;
 
  if ((joy_data[0].fd = open(towns_pad_dev, O_NONBLOCK | O_RDONLY)) >= 0)
  {
    joy_data[0].num_buttons=4;
    joy_data[0].num_axis=2;
  }
}

static int Pad()
{
  struct pad_event ev;
  if (read(joy_data[0].fd, &ev, sizeof ev) == sizeof ev){
    lPadLastButton = ev.buttons;
  }
  return((int) lPadLastButton & 0xff);
}

/*
 * Linux FM-TOWNS game pad driver based joystick emulation routine
 */
void joy_pad_poll(void)
{
      int i;
      int res = Pad();

      /* get value of buttons */
      for (i=0; i<4; i++)
      {
         joy_data[0].buttons[i] = (res>>4) & (0x01<<i);
      }

      joy_data[0].axis[0].dirs[0] = res & 0x01;
      joy_data[0].axis[0].dirs[1] = res & 0x02;
      joy_data[0].axis[1].dirs[0] = res & 0x04;
      joy_data[0].axis[1].dirs[1] = res & 0x08;
}

/* TOWNS_PAD */
#endif
