#ifndef __DEVICES_H_
#define __DEVICES_H_

#ifdef __DEVICES_C_
#define EXTERN
#else
#define EXTERN extern
#endif

#define JOY 6
#define JOY_BUTTONS 16
#define JOY_AXIS 8
#define JOY_DIRS 2
#define JOY_LIST_AXIS_ENTRIES (JOY_AXIS * JOY_DIRS)
#define JOY_LIST_TOTAL_ENTRIES (JOY_LIST_AXIS_ENTRIES + JOY_BUTTONS)
#define JOY_LIST_LEN (JOY * JOY_LIST_TOTAL_ENTRIES)
#define JOY_NAME_LEN 20

#ifndef USE_XINPUT_DEVICES
  /* only one mouse for now */
  #define MOUSE 1
#else
  /* now we have 4 */
  #define MOUSE 4
#endif
#define MOUSE_BUTTONS 8
#define MOUSE_AXIS 8

/* now axis entries in the mouse_list, these are get through another way,
   like the analog joy-values */
#define MOUSE_LIST_TOTAL_ENTRIES MOUSE_BUTTONS
#define MOUSE_LIST_LEN (MOUSE * MOUSE_LIST_TOTAL_ENTRIES)


#define JOY_BUTTON_CODE(joy, button) \
 (joy * JOY_LIST_TOTAL_ENTRIES + JOY_LIST_AXIS_ENTRIES + button)

#define MOUSE_BUTTON_CODE(mouse, button) \
 (JOY_LIST_LEN + mouse * MOUSE_LIST_TOTAL_ENTRIES + button)
  
#define JOY_AXIS_CODE(joy, axis, dir) \
 (joy * JOY_LIST_TOTAL_ENTRIES + JOY_DIRS * axis + dir)

/* mouse doesn't support axis this way */
 
#define JOY_GET_JOY(code) \
 (code / JOY_LIST_TOTAL_ENTRIES)

#define MOUSE_GET_MOUSE(code) \
 ((code - JOY_LIST_LEN) / MOUSE_LIST_TOTAL_ENTRIES)
 
#define JOY_IS_AXIS(code) \
 ((code < JOY_LIST_LEN) && \
  ((code % JOY_LIST_TOTAL_ENTRIES) <  JOY_LIST_AXIS_ENTRIES))


  
/* mouse doesn't support axis */

#define JOY_IS_BUTTON(code) \
 ((code < JOY_LIST_LEN) && \
  (((code % JOY_LIST_TOTAL_ENTRIES) >= JOY_LIST_AXIS_ENTRIES))

#define MOUSE_IS_BUTTON(code) \
 (code >= JOY_LIST_LEN) 

#define JOY_GET_AXIS(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) / JOY_DIRS)
 
/* mouse doesn't support axis this way */
 
#define JOY_GET_DIR(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) % JOY_DIRS)
 
/* mouse doesn't support axis this way */

#define JOY_GET_BUTTON(code) \
 ((code % JOY_LIST_TOTAL_ENTRIES) -  JOY_LIST_AXIS_ENTRIES)

#define MOUSE_GET_BUTTON(code) \
 ((code - JOY_LIST_LEN) % MOUSE_LIST_TOTAL_ENTRIES)

enum { JOY_NONE, JOY_I386, JOY_PAD, JOY_X11, JOY_I386NEW, JOY_USB, JOY_PS2, JOY_SDL };

/*** variables ***/

struct axisdata_struct
{
   /* current value */
   int val;
   /* calibration data */
   int min;
   int center;
   int max;
   /* boolean values */
   int dirs[JOY_DIRS];
};

struct joydata_struct
{
   int fd;
   int num_axis;
   int num_buttons;
   struct axisdata_struct axis[JOY_AXIS];
   int buttons[JOY_BUTTONS];
};

struct mousedata_struct
{
   int buttons[MOUSE_BUTTONS];
   int deltas[MOUSE_AXIS];
};

struct rapidfire_struct
{
   int setting[10];
   int status[10];
   int enable;
   int ctrl_button;
   int ctrl_prev_status;
};

EXTERN struct joydata_struct joy_data[JOY];
EXTERN struct mousedata_struct mouse_data[MOUSE];
EXTERN struct rapidfire_struct rapidfire_data[4];
EXTERN void (*joy_poll_func) (void);
EXTERN int joytype;
EXTERN int is_usb_ps_gamepad;
EXTERN int rapidfire_enable;

extern struct rc_option joy_i386_opts[];
extern struct rc_option joy_pad_opts[];
extern struct rc_option joy_x11_opts[];
extern struct rc_option joy_usb_opts[];
extern struct rc_option joy_ps2_opts[];

#ifdef USE_XINPUT_DEVICES
#include "joystick-drivers/XInputDevices.h"
#endif

/*** prototypes ***/
void joy_evaluate_moves(void);
void joy_i386_init(void);
void joy_pad_init(void);
void joy_x11_init(void);
void joy_usb_init(void);
void joy_ps2_init(void);
void joy_ps2_exit(void);
void joy_SDL_init(void);
#undef EXTERN

/*
 * sdevaux 02/2003 : JOY macros copied from windows mame source code
 *          but don't know if InputCode data type is strictely equivalent
 */ 
/* macros for building/mapping keycodes */
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12))
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define JOYTYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

/* joystick types */
#define JOYTYPE_AXIS_NEG			0
#define JOYTYPE_AXIS_POS			1
#define JOYTYPE_POV_UP				2
#define JOYTYPE_POV_DOWN			3
#define JOYTYPE_POV_LEFT			4
#define JOYTYPE_POV_RIGHT			5
#define JOYTYPE_BUTTON				6
#define JOYTYPE_MOUSEBUTTON			7

#endif
