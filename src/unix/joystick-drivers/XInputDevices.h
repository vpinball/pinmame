#ifndef __XINPUT_DEVICES_H__
#define __XINPUT_DEVICES_H__

#include <X11/extensions/XInput.h>

enum { XMAME_NULLDEVICE, XMAME_TRACKBALL, XMAME_JOYSTICK };
enum { XINPUT_MOUSE_0, XINPUT_MOUSE_1, XINPUT_MOUSE_2, XINPUT_MOUSE_3,
       XINPUT_JOYSTICK_0, XINPUT_JOYSTICK_1, XINPUT_JOYSTICK_2, XINPUT_JOYSTICK_3,
       XINPUT_MAX_NUM_DEVICES };

#define XINPUT_MAX_NUM_AXIS 2

/* struct which keeps all info for a XInput-devices */
typedef struct {
  char *deviceName;
  XDeviceInfo *info;
  int mameDevice;
  int previousValue[JOY_AXIS];
  int neverMoved;
} XInputDeviceData;

/* prototypes */
void XInputDevices_init(void);
int XInputProcessEvent(XEvent *);
/* <jake> */
#ifdef USE_XINPUT_DEVICES
void XInputPollDevices(int, int *, int *);
#endif
/* </jake> */

extern struct rc_option XInputDevices_opts[];

#endif
