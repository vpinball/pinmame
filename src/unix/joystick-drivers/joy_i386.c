#include "xmame.h"
#include "devices.h"

static char *joy_dev = NULL; /* name of joystick device prefix */

struct rc_option joy_i386_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#if defined(__ARCH_netbsd) || defined(__ARCH_freebsd) || defined(__ARCH_openbsd)
   { "joydevname",	"jdev",			rc_string,	&joy_dev,
     "/dev/joy",	0,			0,		NULL,
     "Joystick device prefix (defaults to /dev/joy)" },
#elif defined __ARCH_linux
   { "joydevname",	"jdev",			rc_string,	&joy_dev,
     "/dev/js",		0,			0,		NULL,
     "Joystick device prefix (defaults to /dev/js)" },
#endif  /* arch */
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef I386_JOYSTICK

#include <sys/ioctl.h>

/* specific joystick for PC clones */
#if defined(__ARCH_netbsd) || defined(__ARCH_freebsd) || defined(__ARCH_openbsd)

#include <machine/joystick.h>
typedef struct joystick joy_struct;

#elif defined __ARCH_linux

#include <linux/joystick.h>
typedef struct JS_DATA_TYPE joy_struct;

#ifdef JS_VERSION
#define I386NEW_JOYSTICK 1
#endif

#else
#error "i386 style joystick only supported under linux, openbsd, netbsd & freebsd. "
   "patches to support other arch's are welcome ;)"
#endif

/* #define JDEBUG */

void joy_i386_poll(void);
void joy_i386new_poll(void);
static joy_struct my_joy_data;

void joy_i386_init(void)
{
   int i, j;
   char devname[20];
#ifdef I386NEW_JOYSTICK
   int version;
#endif

   int first_dev = 0;
   int last_dev = JOY - 1;
   
   /* 
    * If the device name ends with an in-range digit, then don't 
    * loop through all possible values below.  Just extract the 
    * device number and use it.
    */
   int pos = strlen(joy_dev) - 1;
   if (pos >= 0 && isdigit(joy_dev[pos]))
   {
      int devnum = joy_dev[pos] - '0';
      if (devnum < JOY)
      {
         first_dev = last_dev = devnum;
         joy_dev[pos] = 0;
      }
   }

   fprintf (stderr_file, "I386 joystick interface initialization...\n");
   for (i = first_dev; i <= last_dev; i++)
   {
      sprintf (devname, "%s%d", joy_dev, i);
      if ((joy_data[i].fd = open (devname, O_RDONLY)) >= 0)
      {
         if (read(joy_data[i].fd, &my_joy_data, sizeof(joy_struct)) != sizeof(joy_struct))
         {
            close(joy_data[i].fd);
            joy_data[i].fd = -1;
            continue;
         }
         switch(joytype)
         {
            case JOY_I386NEW:
#ifdef I386NEW_JOYSTICK
               /* new joystick driver 1.x.x API 
                  check the running version of driver, if 1.x.x is
                  not detected fall back to 0.8 API */

               if (ioctl (joy_data[i].fd, JSIOCGVERSION, &version)==0)
               {
                  char name[60];
                  ioctl (joy_data[i].fd, JSIOCGAXES, &joy_data[i].num_axis);
                  ioctl (joy_data[i].fd, JSIOCGBUTTONS, &joy_data[i].num_buttons);
                  ioctl (joy_data[i].fd, JSIOCGNAME (sizeof (name)), name);
                  if (joy_data[i].num_buttons > JOY_BUTTONS)
                     joy_data[i].num_buttons = JOY_BUTTONS;
                  if (joy_data[i].num_axis > JOY_AXIS)
                     joy_data[i].num_axis = JOY_AXIS;
                  fprintf (stderr_file, "Joystick: %s is %s\n", devname, name);
                  fprintf (stderr_file, "Joystick: Built in driver version: %d.%d.%d\n", JS_VERSION >> 16, (JS_VERSION >> 8) & 0xff, JS_VERSION & 0xff);
                  fprintf (stderr_file, "Joystick: Kernel driver version  : %d.%d.%d\n", version >> 16, (version >> 8) & 0xff, version & 0xff);
                  for (j=0; j<joy_data[i].num_axis; j++)
                  {
                     joy_data[i].axis[j].min = -32768;
                     joy_data[i].axis[j].max =  32768;
                  }
                  joy_poll_func = joy_i386new_poll;
                  break;
               }
               /* else we're running on a kernel with 0.8 driver */
               fprintf (stderr_file, "Joystick: %s unknown type\n", devname);
               fprintf (stderr_file, "Joystick: Built in driver version: %d.%d.%d\n", JS_VERSION >> 16, (JS_VERSION >> 8) & 0xff, JS_VERSION & 0xff);
               fprintf (stderr_file, "Joystick: Kernel driver version  : 0.8 ??\n");
               fprintf (stderr_file, "Joystick: Please update your Joystick driver !\n");
               fprintf (stderr_file, "Joystick: Using old interface method\n");
#else
               fprintf (stderr_file, "New joystick driver (1.x.x) support not compiled in.\n");
               fprintf (stderr_file, "Falling back to 0.8 joystick driver api\n");
#endif            
               joytype = JOY_I386;
            case JOY_I386:
               joy_data[i].num_axis = 2;
#if defined(__ARCH_netbsd) || defined(__ARCH_freebsd) || defined(__ARCH_openbsd)
               joy_data[i].num_buttons = 2;
#else
               joy_data[i].num_buttons = JOY_BUTTONS;
#endif
               joy_data[i].axis[0].center = my_joy_data.x;
               joy_data[i].axis[1].center = my_joy_data.y;
               joy_data[i].axis[0].min    = my_joy_data.x - 10;
               joy_data[i].axis[1].min    = my_joy_data.y - 10;
               joy_data[i].axis[0].max    = my_joy_data.x + 10;
               joy_data[i].axis[1].max    = my_joy_data.y + 10;
               
               joy_poll_func = joy_i386_poll;
               break;
         }
         fcntl (joy_data[i].fd, F_SETFL, O_NONBLOCK);
      }
   }
}

#ifdef I386NEW_JOYSTICK
/* 
 * Routine to manage PC clones joystick via new Linux driver 1.2.xxx
 */
void joy_i386new_poll (void)
{
   struct js_event js;
   int i;

   for (i=0; i<JOY; i++)
   {
      if (joy_data[i].fd < 0)
         continue;
      while ((read (joy_data[i].fd, &js, sizeof (struct js_event))) == sizeof (struct js_event))
      {
         switch (js.type & ~JS_EVENT_INIT)
         {
            case JS_EVENT_BUTTON:
               if (js.number < JOY_BUTTONS)
                  joy_data[i].buttons[js.number] = js.value;
#ifdef JDEBUG
               fprintf (stderr, "Button=%d,value=%d\n", js.number, js.value);
#endif
               break;

            case JS_EVENT_AXIS:
               if (js.number < JOY_AXIS)
                  joy_data[i].axis[js.number].val = js.value;
#ifdef JDEBUG
               fprintf (stderr, "Axis=%d,value=%d\n", js.number, js.value);
#endif
               break;
         }
      }
   }
      
   /* evaluate joystick movements */
   joy_evaluate_moves ();
}
#endif

/* 
 * Routine to manage PC clones joystick via standard driver 
 */
void joy_i386_poll (void)
{
   int i, j;

   for (i=0; i<JOY; i++)
   {
      if (joy_data[i].fd < 0)
         continue;
      if (read (joy_data[i].fd, &my_joy_data, sizeof (joy_struct)) != sizeof (joy_struct))
         continue;
      
      /* get value of buttons */
#if defined(__ARCH_netbsd) || defined(__ARCH_freebsd) || defined(__ARCH_openbsd)
      joy_data[i].buttons[0] = my_joy_data.b1;
      joy_data[i].buttons[1] = my_joy_data.b2;
#else
      for (j = 0; j < JOY_BUTTONS; j++)
         joy_data[i].buttons[j] = my_joy_data.buttons & (0x01 << j);
#endif
      joy_data[i].axis[0].val = my_joy_data.x;
      joy_data[i].axis[1].val = my_joy_data.y;
   }

   /* evaluate joystick movements */
   joy_evaluate_moves ();
}

#endif
