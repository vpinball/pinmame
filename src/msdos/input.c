#include "mamalleg.h"
#include "driver.h"

/* Main mouse activation flag */
int use_mouse;

/* Sub mouse activation flag */
/* assert(use_mouse == use_mouse_1 || use_mouse_2) */
static int use_mouse_1; /* Allegro mouse, mapped on player 1 */
static int use_mouse_2; /* OptiMAME mouse extension, mapped on player 2 */

/* Mouse button mask */
#define mouse_button_1 mouse_b /* Allegro mouse */
static int mouse_button_2; /* OptiMAME mouse extension */

int joystick;
int use_hotrod;
int steadykey;



static struct KeyboardInfo keylist[] =
{
	{ "A",			KEY_A,				KEYCODE_A },
	{ "B",			KEY_B,				KEYCODE_B },
	{ "C",			KEY_C,				KEYCODE_C },
	{ "D",			KEY_D,				KEYCODE_D },
	{ "E",			KEY_E,				KEYCODE_E },
	{ "F",			KEY_F,				KEYCODE_F },
	{ "G",			KEY_G,				KEYCODE_G },
	{ "H",			KEY_H,				KEYCODE_H },
	{ "I",			KEY_I,				KEYCODE_I },
	{ "J",			KEY_J,				KEYCODE_J },
	{ "K",			KEY_K,				KEYCODE_K },
	{ "L",			KEY_L,				KEYCODE_L },
	{ "M",			KEY_M,				KEYCODE_M },
	{ "N",			KEY_N,				KEYCODE_N },
	{ "O",			KEY_O,				KEYCODE_O },
	{ "P",			KEY_P,				KEYCODE_P },
	{ "Q",			KEY_Q,				KEYCODE_Q },
	{ "R",			KEY_R,				KEYCODE_R },
	{ "S",			KEY_S,				KEYCODE_S },
	{ "T",			KEY_T,				KEYCODE_T },
	{ "U",			KEY_U,				KEYCODE_U },
	{ "V",			KEY_V,				KEYCODE_V },
	{ "W",			KEY_W,				KEYCODE_W },
	{ "X",			KEY_X,				KEYCODE_X },
	{ "Y",			KEY_Y,				KEYCODE_Y },
	{ "Z",			KEY_Z,				KEYCODE_Z },
	{ "0",			KEY_0,				KEYCODE_0 },
	{ "1",			KEY_1,				KEYCODE_1 },
	{ "2",			KEY_2,				KEYCODE_2 },
	{ "3",			KEY_3,				KEYCODE_3 },
	{ "4",			KEY_4,				KEYCODE_4 },
	{ "5",			KEY_5,				KEYCODE_5 },
	{ "6",			KEY_6,				KEYCODE_6 },
	{ "7",			KEY_7,				KEYCODE_7 },
	{ "8",			KEY_8,				KEYCODE_8 },
	{ "9",			KEY_9,				KEYCODE_9 },
	{ "0 PAD",		KEY_0_PAD,			KEYCODE_0_PAD },
	{ "1 PAD",		KEY_1_PAD,			KEYCODE_1_PAD },
	{ "2 PAD",		KEY_2_PAD,			KEYCODE_2_PAD },
	{ "3 PAD",		KEY_3_PAD,			KEYCODE_3_PAD },
	{ "4 PAD",		KEY_4_PAD,			KEYCODE_4_PAD },
	{ "5 PAD",		KEY_5_PAD,			KEYCODE_5_PAD },
	{ "6 PAD",		KEY_6_PAD,			KEYCODE_6_PAD },
	{ "7 PAD",		KEY_7_PAD,			KEYCODE_7_PAD },
	{ "8 PAD",		KEY_8_PAD,			KEYCODE_8_PAD },
	{ "9 PAD",		KEY_9_PAD,			KEYCODE_9_PAD },
	{ "F1",			KEY_F1,				KEYCODE_F1 },
	{ "F2",			KEY_F2,				KEYCODE_F2 },
	{ "F3",			KEY_F3,				KEYCODE_F3 },
	{ "F4",			KEY_F4,				KEYCODE_F4 },
	{ "F5",			KEY_F5,				KEYCODE_F5 },
	{ "F6",			KEY_F6,				KEYCODE_F6 },
	{ "F7",			KEY_F7,				KEYCODE_F7 },
	{ "F8",			KEY_F8,				KEYCODE_F8 },
	{ "F9",			KEY_F9,				KEYCODE_F9 },
	{ "F10",		KEY_F10,			KEYCODE_F10 },
	{ "F11",		KEY_F11,			KEYCODE_F11 },
	{ "F12",		KEY_F12,			KEYCODE_F12 },
	{ "ESC",		KEY_ESC,			KEYCODE_ESC },
	{ "~",			KEY_TILDE,			KEYCODE_TILDE },
	{ "-",          KEY_MINUS,          KEYCODE_MINUS },
	{ "=",          KEY_EQUALS,         KEYCODE_EQUALS },
	{ "BKSPACE",	KEY_BACKSPACE,		KEYCODE_BACKSPACE },
	{ "TAB",		KEY_TAB,			KEYCODE_TAB },
	{ "[",          KEY_OPENBRACE,      KEYCODE_OPENBRACE },
	{ "]",          KEY_CLOSEBRACE,     KEYCODE_CLOSEBRACE },
	{ "ENTER",		KEY_ENTER,			KEYCODE_ENTER },
	{ ";",          KEY_COLON,          KEYCODE_COLON },
	{ ":",          KEY_QUOTE,          KEYCODE_QUOTE },
	{ "\\",         KEY_BACKSLASH,      KEYCODE_BACKSLASH },
	{ "<",          KEY_BACKSLASH2,     KEYCODE_BACKSLASH2 },
	{ ",",          KEY_COMMA,          KEYCODE_COMMA },
	{ ".",          KEY_STOP,           KEYCODE_STOP },
	{ "/",          KEY_SLASH,          KEYCODE_SLASH },
	{ "SPACE",		KEY_SPACE,			KEYCODE_SPACE },
	{ "INS",		KEY_INSERT,			KEYCODE_INSERT },
	{ "DEL",		KEY_DEL,			KEYCODE_DEL },
	{ "HOME",		KEY_HOME,			KEYCODE_HOME },
	{ "END",		KEY_END,			KEYCODE_END },
	{ "PGUP",		KEY_PGUP,			KEYCODE_PGUP },
	{ "PGDN",		KEY_PGDN,			KEYCODE_PGDN },
	{ "LEFT",		KEY_LEFT,			KEYCODE_LEFT },
	{ "RIGHT",		KEY_RIGHT,			KEYCODE_RIGHT },
	{ "UP",			KEY_UP,				KEYCODE_UP },
	{ "DOWN",		KEY_DOWN,			KEYCODE_DOWN },
	{ "/ PAD",      KEY_SLASH_PAD,      KEYCODE_SLASH_PAD },
	{ "* PAD",      KEY_ASTERISK,       KEYCODE_ASTERISK },
	{ "- PAD",      KEY_MINUS_PAD,      KEYCODE_MINUS_PAD },
	{ "+ PAD",      KEY_PLUS_PAD,       KEYCODE_PLUS_PAD },
	{ ". PAD",      KEY_DEL_PAD,        KEYCODE_DEL_PAD },
	{ "ENTER PAD",  KEY_ENTER_PAD,      KEYCODE_ENTER_PAD },
	{ "PRTSCR",     KEY_PRTSCR,         KEYCODE_PRTSCR },
	{ "PAUSE",      KEY_PAUSE,          KEYCODE_PAUSE },
	{ "LSHIFT",		KEY_LSHIFT,			KEYCODE_LSHIFT },
	{ "RSHIFT",		KEY_RSHIFT,			KEYCODE_RSHIFT },
	{ "LCTRL",		KEY_LCONTROL,		KEYCODE_LCONTROL },
	{ "RCTRL",		KEY_RCONTROL,		KEYCODE_RCONTROL },
	{ "ALT",		KEY_ALT,			KEYCODE_LALT },
	{ "ALTGR",		KEY_ALTGR,			KEYCODE_RALT },
	{ "LWIN",		KEY_LWIN,			KEYCODE_OTHER },
	{ "RWIN",		KEY_RWIN,			KEYCODE_OTHER },
	{ "MENU",		KEY_MENU,			KEYCODE_OTHER },
	{ "SCRLOCK",    KEY_SCRLOCK,        KEYCODE_SCRLOCK },
	{ "NUMLOCK",    KEY_NUMLOCK,        KEYCODE_NUMLOCK },
	{ "CAPSLOCK",   KEY_CAPSLOCK,       KEYCODE_CAPSLOCK },
	{ 0, 0, 0 }	/* end of table */
};


/* return a list of all available keys */
const struct KeyboardInfo *osd_get_key_list(void)
{
	return keylist;
}


/*
  since the keyboard controller is slow, it is not capable of reporting multiple
  key presses fast enough. We have to delay them in order not to lose special moves
  tied to simultaneous button presses.
 */

static int oldkey[KEY_MAX],currkey[KEY_MAX];

static void updatekeyboard(void)
{
	int i,changed;

	changed = 0;
	for (i = 0;i < KEY_MAX;i++)
	{
		if (key[i] != oldkey[i])
		{
			changed = 1;

			if (key[i] == 0 && currkey[i] == 0)
			/* keypress was missed, turn it on for one frame */
				currkey[i] = -1;
		}
	}

	/* if keyboard state is stable, copy it over */
	if (!changed)
	{
		for (i = 0;i < KEY_MAX;i++)
			currkey[i] = key[i];
	}

	for (i = 0;i < KEY_MAX;i++)
		oldkey[i] = key[i];
}


int osd_is_key_pressed(int keycode)
{
	if (keycode >= KEY_MAX) return 0;

	/* if update_video_and_audio() is not being called yet, copy key array */
	if (!Machine->scrbitmap)
	{
		int i;

		for (i = 0;i < KEY_MAX;i++)
			currkey[i] = key[i];
	}

	if (keycode == KEY_PAUSE)
	{
		static int pressed,counter;
		int res;

		res = currkey[KEY_PAUSE] ^ pressed;
		if (res)
		{
			if (counter > 0)
			{
				if (--counter == 0)
					pressed = currkey[KEY_PAUSE];
			}
			else counter = 10;
		}

		return res;
	}

    if(steadykey) return currkey[keycode];
	else return key[keycode];
}


int osd_readkey_unicode(int flush)
{
	if (flush) clear_keybuf();
	if (keypressed())
		return ureadkey(NULL);
	else
		return 0;
}


/*
  limits:
  - 7 joysticks
  - 15 sticks on each joystick
  - 63 buttons on each joystick

  - 256 total inputs
*/
#define JOYCODE(joy,stick,axis_or_button,dir) \
		((((dir)&0x03)<<14)|(((axis_or_button)&0x3f)<<8)|(((stick)&0x1f)<<3)|(((joy)&0x07)<<0))

#define GET_JOYCODE_JOY(code) (((code)>>0)&0x07)
#define GET_JOYCODE_STICK(code) (((code)>>3)&0x1f)
#define GET_JOYCODE_AXIS(code) (((code)>>8)&0x3f)
#define GET_JOYCODE_BUTTON(code) GET_JOYCODE_AXIS(code)
#define GET_JOYCODE_DIR(code) (((code)>>14)&0x03)

/* use otherwise unused joystick codes for the three mouse buttons */
#define MOUSECODE(mouse,button) JOYCODE(1,0,((mouse)-1)*3+(button),1)

#define MAX_JOY 256
#define MAX_JOY_NAME_LEN 40

static struct JoystickInfo joylist[MAX_JOY] =
{
	/* will be filled later */
	{ 0, 0, 0 }	/* end of table */
};

static char joynames[MAX_JOY][MAX_JOY_NAME_LEN+1];	/* will be used to store names for the above */


static int joyequiv[][2] =
{
	{ JOYCODE(1,1,1,1),	JOYCODE_1_LEFT },
	{ JOYCODE(1,1,1,2),	JOYCODE_1_RIGHT },
	{ JOYCODE(1,1,2,1),	JOYCODE_1_UP },
	{ JOYCODE(1,1,2,2),	JOYCODE_1_DOWN },
	{ JOYCODE(1,0,1,0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(1,0,2,0),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(1,0,3,0),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(1,0,4,0),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(1,0,5,0),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(1,0,6,0),	JOYCODE_1_BUTTON6 },
	{ JOYCODE(2,1,1,1),	JOYCODE_2_LEFT },
	{ JOYCODE(2,1,1,2),	JOYCODE_2_RIGHT },
	{ JOYCODE(2,1,2,1),	JOYCODE_2_UP },
	{ JOYCODE(2,1,2,2),	JOYCODE_2_DOWN },
	{ JOYCODE(2,0,1,0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(2,0,2,0),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(2,0,3,0),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(2,0,4,0),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(2,0,5,0),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(2,0,6,0),	JOYCODE_2_BUTTON6 },
	{ JOYCODE(3,1,1,1),	JOYCODE_3_LEFT },
	{ JOYCODE(3,1,1,2),	JOYCODE_3_RIGHT },
	{ JOYCODE(3,1,2,1),	JOYCODE_3_UP },
	{ JOYCODE(3,1,2,2),	JOYCODE_3_DOWN },
	{ JOYCODE(3,0,1,0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(3,0,2,0),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(3,0,3,0),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(3,0,4,0),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(3,0,5,0),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(3,0,6,0),	JOYCODE_3_BUTTON6 },
	{ JOYCODE(4,1,1,1),	JOYCODE_4_LEFT },
	{ JOYCODE(4,1,1,2),	JOYCODE_4_RIGHT },
	{ JOYCODE(4,1,2,1),	JOYCODE_4_UP },
	{ JOYCODE(4,1,2,2),	JOYCODE_4_DOWN },
	{ JOYCODE(4,0,1,0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(4,0,2,0),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(4,0,3,0),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(4,0,4,0),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(4,0,5,0),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(4,0,6,0),	JOYCODE_4_BUTTON6 },
	{ 0,0 }
};


static void init_joy_list(void)
{
	int tot,i,j,k;
	char buf[256];

	tot = 0;

	/* first of all, map mouse buttons */
	for (k = 0;k < 2;k++)
	{
		for (j = 0;j < 3;j++)
		{
			sprintf(buf,"MOUSE%d B%d",k+1,j+1);
			strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
			joynames[tot][MAX_JOY_NAME_LEN] = 0;
			joylist[tot].name = joynames[tot];
			joylist[tot].code = MOUSECODE(k+1,j+1);
			tot++;
		}
	}

	for (i = 0;i < num_joysticks;i++)
	{
		for (j = 0;j < joy[i].num_sticks;j++)
		{
			for (k = 0;k < joy[i].stick[j].num_axis;k++)
			{
				sprintf(buf,"J%d %s %s -",i+1,joy[i].stick[j].name,joy[i].stick[j].axis[k].name);
				strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
				joynames[tot][MAX_JOY_NAME_LEN] = 0;
				joylist[tot].name = joynames[tot];
				joylist[tot].code = JOYCODE(i+1,j+1,k+1,1);
				tot++;

				sprintf(buf,"J%d %s %s +",i+1,joy[i].stick[j].name,joy[i].stick[j].axis[k].name);
				strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
				joynames[tot][MAX_JOY_NAME_LEN] = 0;
				joylist[tot].name = joynames[tot];
				joylist[tot].code = JOYCODE(i+1,j+1,k+1,2);
				tot++;
			}
		}
		for (j = 0;j < joy[i].num_buttons;j++)
		{
			sprintf(buf,"J%d %s",i+1,joy[i].button[j].name);
			strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
			joynames[tot][MAX_JOY_NAME_LEN] = 0;
			joylist[tot].name = joynames[tot];
			joylist[tot].code = JOYCODE(i+1,0,j+1,0);
			tot++;
		}
	}

	/* terminate array */
	joylist[tot].name = 0;
	joylist[tot].code = 0;
	joylist[tot].standardcode = 0;

	/* fill in equivalences */
	for (i = 0;i < tot;i++)
	{
		joylist[i].standardcode = JOYCODE_OTHER;

		j = 0;
		while (joyequiv[j][0] != 0)
		{
			if (joyequiv[j][0] == joylist[i].code)
			{
				joylist[i].standardcode = joyequiv[j][1];
				break;
			}
			j++;
		}
	}
}


/* return a list of all available joys */
const struct JoystickInfo *osd_get_joy_list(void)
{
	return joylist;
}


int osd_is_joy_pressed(int joycode)
{
	int joy_num,stick;


	/* special case for mouse buttons */
	switch (joycode)
	{
		case MOUSECODE(1,1):
			return mouse_button_1 & 1; break;
		case MOUSECODE(1,2):
			return mouse_button_1 & 2; break;
		case MOUSECODE(1,3):
			return mouse_button_1 & 4; break;
                case MOUSECODE(2,1):
                        return mouse_button_2 & 1; break;
                case MOUSECODE(2,2):
                        return mouse_button_2 & 2; break;
                case MOUSECODE(2,3):
                        return mouse_button_2 & 4; break;
	}

	joy_num = GET_JOYCODE_JOY(joycode);

	/* do we have as many sticks? */
	if (joy_num == 0 || joy_num > num_joysticks)
		return 0;
	joy_num--;

	stick = GET_JOYCODE_STICK(joycode);

	if (stick == 0)
	{
		/* buttons */
		int button;

		button = GET_JOYCODE_BUTTON(joycode);
		if (button == 0 || button > joy[joy_num].num_buttons)
			return 0;
		button--;

		return joy[joy_num].button[button].b;
	}
	else
	{
		/* sticks */
		int axis,dir;

		if (stick > joy[joy_num].num_sticks)
			return 0;
		stick--;

		axis = GET_JOYCODE_AXIS(joycode);
		dir = GET_JOYCODE_DIR(joycode);

		if (axis == 0 || axis > joy[joy_num].stick[stick].num_axis)
			return 0;
		axis--;

		switch (dir)
		{
			case 1:
				return joy[joy_num].stick[stick].axis[axis].d1; break;
			case 2:
				return joy[joy_num].stick[stick].axis[axis].d2; break;
			default:
				return 0; break;
		}
	}

	return 0;
}


void poll_joysticks(void)
{
	updatekeyboard();

	if (joystick > JOY_TYPE_NONE)
		poll_joystick();
}


/* return a value in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y)
{
	*analog_x = *analog_y = 0;

	/* is there an analog joystick at all? */
	if (player+1 > num_joysticks || joystick == JOY_TYPE_NONE)
		return;

	*analog_x = joy[player].stick[0].axis[0].pos;
	*analog_y = joy[player].stick[0].axis[1].pos;
}


static int calibration_target;

int osd_joystick_needs_calibration (void)
{
	/* This could be improved, but unfortunately, this version of Allegro */
	/* lacks a flag which tells if a joystick is calibrationable, it only  */
	/* remembers whether calibration is yet to be done. */
	if (joystick == JOY_TYPE_NONE)
		return 0;
	else
		return 1;
}


void osd_joystick_start_calibration (void)
{
	/* reinitialises the joystick. */
	remove_joystick();
	install_joystick (joystick);
	calibration_target = 0;
}

const char *osd_joystick_calibrate_next (void)
{
	while (calibration_target < num_joysticks)
	{
		if (joy[calibration_target].flags & JOYFLAG_CALIBRATE)
			return (calibrate_joystick_name (calibration_target));
		else
			calibration_target++;
	}

	return 0;
}

void osd_joystick_calibrate (void)
{
	calibrate_joystick (calibration_target);
}

void osd_joystick_end_calibration (void)
{
	save_joystick_data(0);
}

#include <time.h>

void osd_trak_read(int player,int *deltax,int *deltay)
{
	if (player == 0 && use_mouse && use_mouse_1)
	{
		static int skip;
		static int mx,my;

		/* get_mouse_mickeys() doesn't work when called 60 times per second,
		   it often returns 0, so I have to call it every other frame and split
		   the result between two frames.
		  */
		if (skip)
		{
			*deltax = mx;
			*deltay = my;
		}
		else
		{
			get_mouse_mickeys(&mx,&my);
			*deltax = mx/2;
			*deltay = my/2;
			mx -= *deltax;
			my -= *deltay;
		}
		skip ^= 1;
	}
	else if (player == 1 && use_mouse && use_mouse_2)
	{
                static int skip2;
                static int mx2,my2;
                static __dpmi_regs r;

                /* Get mouse button state */
                r.x.ax = 103;
                __dpmi_int(0x33, &r);
                mouse_button_2 = r.x.bx;

		/* get_mouse_mickeys() doesn't work when called 60 times per second,
		   it often returns 0, so I have to call it every other frame and split
		   the result between two frames.
		  */
                if (skip2)
		{
                        *deltax = mx2;
                        *deltay = my2;
                }
                else  /* Poll secondary mouse */
		{
                        r.x.ax = 111;
                        __dpmi_int(0x33, &r);

                        mx2 = (signed short)r.x.cx>>1;
                        my2 = (signed short)r.x.dx>>1;

                        *deltax = mx2;
                        *deltay = my2;
		}
                skip2 ^= 1;
	}
	else
	{
		*deltax = 0;
		*deltay = 0;
	}
}


#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
extern int no_of_tiles;
extern struct GameDriver driver_neogeo;
#endif
#endif
#endif

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	if (use_hotrod)
	{
		while (defaults->type != IPT_END)
		{
			int j;
			for(j=0;j<SEQ_MAX;++j)
			{
				if (defaults->seq[j] == KEYCODE_UP) defaults->seq[j] = KEYCODE_8_PAD;
				if (defaults->seq[j] == KEYCODE_DOWN) defaults->seq[j] = KEYCODE_2_PAD;
				if (defaults->seq[j] == KEYCODE_LEFT) defaults->seq[j] = KEYCODE_4_PAD;
				if (defaults->seq[j] == KEYCODE_RIGHT) defaults->seq[j] = KEYCODE_6_PAD;
			}
			if (defaults->type == IPT_UI_SELECT) seq_set_1(&defaults->seq, KEYCODE_LCONTROL);
			if (defaults->type == IPT_START1) seq_set_1(&defaults->seq, KEYCODE_1);
			if (defaults->type == IPT_START2) seq_set_1(&defaults->seq, KEYCODE_2);
			if (defaults->type == IPT_COIN1)  seq_set_1(&defaults->seq, KEYCODE_3);
			if (defaults->type == IPT_COIN2)  seq_set_1(&defaults->seq, KEYCODE_4);
			if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_R);
			if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_F);
			if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_D);
			if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_G);
			if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_8_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_2_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_4_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_6_PAD);
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LCONTROL);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LALT);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_SPACE);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LSHIFT);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_Z);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_X);
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_A);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_S);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_Q);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_W);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_E);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_OPENBRACE);

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
			if (use_hotrod == 2 &&
					(Machine->gamedrv->clone_of == &driver_neogeo ||
					(Machine->gamedrv->clone_of && Machine->gamedrv->clone_of->clone_of == &driver_neogeo)))
			{
				if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_C);
				if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_LSHIFT);
				if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_Z);
				if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_X);
				if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER1)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_CLOSEBRACE);
				if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_W);
				if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_E);
				if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_OPENBRACE);
				if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
				if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER2)) seq_set_1(&defaults->seq,KEYCODE_NONE);
			}
#endif
#endif
#endif

			defaults++;
		}
	}
}




void msdos_init_input (void)
{
	int err;

	install_keyboard();
/* ALLEGRO BUG: the following hangs on many machines */
//	set_leds(0);    /* turn off all leds */

	if (joystick != JOY_TYPE_NONE)
	{
		/* Try to install Allegro's joystick handler */

		/* load calibration data (from mame.cfg) */
		/* if valid data was found, this also sets Allegro's joy_type */
		err=load_joystick_data(0);

		/* valid calibration? */
		if (err)
		{
			logerror("No calibration data found\n");
			if (install_joystick (joystick) != 0)
			{
				printf ("Joystick not found.\n");
				joystick = JOY_TYPE_NONE;
			}
		}
		else if (joystick != joy_type)
		{
			logerror("Calibration data is from different joystick\n");
			remove_joystick();
			if (install_joystick (joystick) != 0)
			{
				printf ("Joystick not found.\n");
				joystick = JOY_TYPE_NONE;
			}
		}

		if (joystick == JOY_TYPE_NONE)
			logerror("Joystick not found\n");
		else
			logerror("Installed %s %s\n",
					joystick_driver->name, joystick_driver->desc);
	}

	init_joy_list();

	if (use_mouse)
	{
                __dpmi_regs r;

		/* Initialize the Allegro mouse */
		use_mouse_1 = install_mouse() != -1;

		/* Initialize the custom MAME mouse driver extension */
                r.x.ax = 100;
                __dpmi_int(0x33, &r);
                use_mouse_2 = r.x.ax != 100 && r.x.ax != 0;

		if (!use_mouse_1 && !use_mouse_2)
			use_mouse = 0;
	}
	else
	{
		use_mouse_1 = 0;
		use_mouse_2 = 0;
	}
}


void msdos_shutdown_input(void)
{
	remove_keyboard();
}
