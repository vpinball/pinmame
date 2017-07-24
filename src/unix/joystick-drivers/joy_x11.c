/*
 * X-Mame x11 joystick code
 *
 */
#include "xmame.h"
#include "devices.h"

#ifdef X11_JOYSTICK
static char *x11joyname = NULL;
#endif

struct rc_option joy_x11_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
#ifdef X11_JOYSTICK
   { "x11joyname",	NULL,			rc_string,	&x11joyname,
     X11_JOYNAME,	0,			0,		NULL,
     "Name of X-based joystick device (if compiled in)" },
#endif
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#ifdef X11_JOYSTICK
#include "../video-drivers/x11.h"
#if !defined x11 && !defined xgl && !defined xfx
#error "x11 joystick support only works with an x11 display method, duh !"
#endif

/* standard X input extensions based joystick */
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>
/* event types returned by XSelecExtensionEvent() */
static XDevice *xdevice;
void joy_x11_poll(void);

/* FIXME shouldn't X support more then 1 joystick ? */

void process_x11_joy_event(XEvent *event) {
#ifdef USE_X11_JOYEVENTS
/* does not run yet, don't know why :-( */
    int i;
    /* remember that event types are not harcoded: we evaluated it in XOpenDevice() */
    /* hack: we'll suppose that:
	 first_axis is allways equals 0. 
	 device_id is joystic's id
       in a real program, should be checked... 
     */
    if ( (event->type==devicebuttonpress) || (event->type==devicebuttonrelease) ) {
	XDeviceButtonEvent *dbe=(XDeviceButtonEvent *) event;	
	/* evaluate button state */
	for (i=0; i<joy_data[0].num_buttons; i++)
	   joy_data[0].buttons[i] = dbe->device_state & (0x01 << i);
	for(i=0;i<joy_data[0].num_axis;i++)
		joy_data[0].axis[i].val = joy_data[0].axis[i].center +
		   dbe->axis_data[i];
    }
    if ( (event->type==devicemotionnotify) ) {
	XDeviceMotionEvent *dme=(XDeviceMotionEvent *) event;	
	/* evaluate button state */
	for (i=0; i<joy_data[0].num_buttons; i++)
	   joy_data[0].buttons[i] = dme->device_state & (0x01 << i);
	for(i=0;i<joy_data[0].num_axis;i++)
		joy_data[0].axis[i].val = joy_data[0].axis[i].center +
		   dme->axis_data[i];
    }
#endif
}

void joy_x11_init(void)
{
	int 		i,j,k;
	int 		result;
	XDeviceInfoPtr 	list,slist;
	XAnyClassPtr 	any;
	XButtonInfoPtr 	binfo;
	XValuatorInfoPtr vinfo;
	XInputClassInfo *classptr;
	XEventClass 	xeventlist[8];
	int 		xeventcount;
	
	joy_poll_func = joy_x11_poll;
	
	/* query server for input extensions */
	result =XQueryExtension(display,"XInputExtension",&i,&j,&k);
	if(!result) {
	    fprintf(stderr_file,"Your server doesn't support XInput Extensions\n");
	    fprintf(stderr_file,"X11 Joystick disabled\n");
	    joytype=JOY_NONE;
	    return;
	}
	/* now get input device list and locate desired device */
	list = XListInputDevices(display,&result);
	if (!list ) {
	    fprintf(stderr_file,"No extended input devices found !!\n");
	    fprintf(stderr_file,"X11 Joystick disabled\n");
	    joytype=JOY_NONE;
	    return;
	}
	slist=list;
	for(i=j=0;i<result;i++,list++) 
		if ( ! strcmp(x11joyname,list->name)  ) { j=1; break; }
	if (!j) {
	    fprintf(stderr_file,"Cannot locate device \"%s\" in available devices\n",x11joyname);
	    fprintf(stderr_file,"X11 Joystick disabled\n");
	    joytype=JOY_NONE;
	    XFreeDeviceList(slist);
	    return;
	}
	/* test for correct device ( search at least two buttons and two axis */
	any = (XAnyClassPtr)(list->inputclassinfo);
	result=0;
	for(j=0;j<list->num_classes;j++) {
	    switch(any->class) {
		case ButtonClass:
			binfo=(XButtonInfoPtr) any;
			if ((joy_data[0].num_buttons=binfo->num_buttons)>=2) result |= 0x01;
			fprintf(stderr_file,"%s: %d buttons\n",x11joyname,joy_data[0].num_buttons);
			if (joy_data[0].num_buttons > JOY_BUTTONS) joy_data[0].num_buttons = JOY_BUTTONS;
			break;
		case ValuatorClass:
			vinfo=(XValuatorInfoPtr) any;
			if ((joy_data[0].num_axis=vinfo->num_axes)>=2) result |= 0x02;
			fprintf(stderr_file,"%s: %d axes\n",x11joyname,joy_data[0].num_axis);
			if (joy_data[0].num_axis > JOY_AXIS) joy_data[0].num_axis = JOY_AXIS;
			for (i=0; i<joy_data[0].num_axis; i++)
			{
			   joy_data[0].axis[i].val = joy_data[0].axis[i].center =
			      (vinfo->axes[i].max_value - vinfo->axes[i].min_value) / 2;
			   joy_data[0].axis[i].min = vinfo->axes[i].min_value;
			   joy_data[0].axis[i].max = vinfo->axes[i].max_value;
			}
			break;
		case KeyClass: /* no sense to use a extended key device */
			fprintf(stderr_file,"%s: Ingnoring KeyClass info\n",x11joyname);
		default: break;  /* unnknown class: ignore */
	    }
	    any = (XAnyClassPtr) ((char *) any+any->length);
	}
	if (result != 0x03 ) {
	    fprintf(stderr_file,"Your selected X11 device \"%s\"doesn't match X-Mame/X-Mess requirements\n",x11joyname);
	    fprintf(stderr_file,"X11 Joystick disabled\n");
	    joytype=JOY_NONE;
	    XFreeDeviceList(slist);
	    return;
	}
	/* once located, try to open */	
	if ( ! (xdevice=XOpenDevice(display,list->id) ) ) {
	    fprintf(stderr_file,"XDeviceOpen error\n");
	    joytype=JOY_NONE;
	    XFreeDeviceList(slist);
	    return;
	} 
	/* buscamos los eventos asociados que necesitamos */
	/* realmente el bucle for y la sentencia switch no son necesarias, pues
	   en XInput.h se buscan automaticamente los elementos de cada dato, pero
	   lo pongo de ejemplo para si en el futuro se quieren chequear la existencia
           de una determinada clase antes de pedir eventos. Nosotros sabemos a 
	   priori que no deberia fallar....
	*/
	xeventcount=0;
	for (i=0,classptr=xdevice->classes;i<xdevice->num_classes;i++,classptr++ ) {
	    switch(classptr->input_class) {
		case KeyClass: break;
		case ButtonClass:
	    		DeviceButtonPress(xdevice,devicebuttonpress,xeventlist[xeventcount]);
			if (devicebuttonpress) xeventcount++;
	    		DeviceButtonRelease(xdevice,devicebuttonrelease,xeventlist[xeventcount]);
			if (devicebuttonrelease) xeventcount++;
			break;
		case ValuatorClass:
	    		DeviceMotionNotify(xdevice,devicemotionnotify,xeventlist[xeventcount]);
			if (devicemotionnotify) xeventcount++;
	    		DeviceButtonMotion(xdevice,devicebuttonmotion,xeventlist[xeventcount]);
			if (devicebuttonmotion) xeventcount++;
			break;
		case FocusClass: break;
		case ProximityClass: break;
		case OtherClass: break;
		default: break;
	    }
	}
#if 0
	/* 
	NOTE: don't know why but these two items don't work in my linux
	XInputExtension Joystick module. still working on it ...
	*/

	/* force relative motion report */
	XSetDeviceMode(display,xdevice,Relative);
	/* set starting point of joystick (to force joy to be centered) */
	for(i=0; i<joy_data[0].num_axis; i++)
	   XSetDeviceValuators(display,xdevice,&(joy_data[0].axis[i]),i,1);
	
#endif
#ifdef USE_X11_JOYEVENTS
	/* not sure why don't recognize all events type. still working... */
	XSelectExtensionEvent(display,window,xeventlist,xeventcount);
	fprintf(stderr_file,"X11PointerDevice: Using X-Window Events\n");
#else
	fprintf(stderr_file,"X11PointerDevice: Using Demand QueryState\n");
#endif
	fprintf(stderr_file,"Found and installed X11 pointer device \"%s\"\n",x11joyname);
	/* and finaly free requested device list */
	XFreeDeviceList(slist);
}

/* 
 * Routine to manage joystick via X-Windows Input Extensions
 * should work in any X-Server that supports them
 */
void joy_x11_poll(void)
{
#ifndef USE_X11_JOYEVENTS
	/* perform a roudtrip query to joy device to ask state */
	XDeviceState    *xstate;
	XInputClass     *any;
	XValuatorState  *vinfo;
	XButtonState    *binfo;
	int i,j;
	xstate = XQueryDeviceState(display,xdevice);
	any = (XInputClass *)(xstate->data);
	for(j=0;j<xstate->num_classes;j++) {
	    switch(any->class) {
		case ButtonClass:
			binfo=(XButtonState *) any;
			for (i=0; i<joy_data[0].num_buttons; i++)
			{
			   joy_data[0].buttons[i] = (int)binfo->buttons[0] & (0x01 << i);
			}
			break;
		case ValuatorClass:
			vinfo=(XValuatorState *) any;
			for (i=0; i<joy_data[0].num_axis; i++)
			   joy_data[0].axis[i].val =
			      joy_data[0].axis[i].center + vinfo->valuators[i];
			break;
		case KeyClass: /* no sense to use a extended key device */
		default: break;  /* unknown class: ignore */
	    }
	    any = (XInputClass *) ((char *) any+any->length);
	}
	XFreeDeviceState(xstate);
#endif 
	joy_evaluate_moves();
}

#endif /* ifdef X11_JOYSTICK */
