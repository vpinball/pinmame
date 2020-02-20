#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "driver.h"
//#include "misc.h"
//#include "video.h"
#include "rc.h"
#include "fileio.h"


//============================================================
//	OPTIONS
//============================================================

// global input options
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input device options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

struct rc_option *ctrlr_input_opts = NULL;

struct rc_option ctrlr_input_opts2[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};


/******************************************************************************

Joystick & Mouse/Trackball

******************************************************************************/
#define MAX_JOY 4
static struct JoystickInfo joylist[MAX_JOY];
/*
return a list of all available joystick inputs (see input.h)
*/
const struct JoystickInfo *osd_get_joy_list(void)
{
	return joylist;
}

/*
tell whether the specified joystick direction/button is pressed or not.
joycode is the OS dependent code specified in the list returned by
osd_get_joy_list().
*/
int osd_is_joy_pressed(int joycode)
{
	return 0;
}

/* added for building joystick seq for analog inputs */
int osd_is_joystick_axis_code(int joycode)
{
	return 0;
}

/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration(void)
{
	return 0;
}

/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void)
{
}

/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void)
{
	return "";
}

/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void)
{
}

/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void)
{
}

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
}

void osd_trak_read(int player, int *deltax, int *deltay)
{
}

/* return values in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player, int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES])
{
}


/*
inptport.c defines some general purpose defaults for key and joystick bindings.
They may be further adjusted by the OS dependent code to better match the
available keyboard, e.g. one could map pause to the Pause key instead of P, or
snapshot to PrtScr instead of F12. Of course the user can further change the
settings to anything he/she likes.
This function is called on startup, before reading the configuration from disk.
Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_defaults(struct ipd *defaults)
{
}
