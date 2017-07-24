/*
 * file devices.c
 *
 * Routines for Pointers device processing
 *
 * Joystick and Mouse
 *
 * original idea from Chris Sharp <sharp@uk.ibm.com>
 *
 */

/*
 * updates by sdevaux <sebastien.devaux@laposte.net>
 */

#define __DEVICES_C_

#ifdef UGCICOIN
#include <ugci.h>
#endif

#include "xmame.h"
#include "devices.h"
#include "input.h"
#include "keyboard.h"
#include "driver.h"
#include "ui_text.h"

/* local variables */
static struct JoystickInfo joy_list[JOY_LIST_LEN + MOUSE_LIST_LEN + 1];
/* will be used to store names for the above */
static char joy_list_names[JOY_LIST_LEN + MOUSE_LIST_LEN][JOY_NAME_LEN];
static int analogstick = 0;
static int ugcicoin;
static const char *ctrlrtype;
static const char *ctrlrname;
static const char *trackball_ini;
static const char *paddle_ini;
static const char *dial_ini;
static const char *ad_stick_ini;
static const char *pedal_ini;
static const char *lightgun_ini;

/* this is used for the ipdef_custom_rc_func */
static struct ipd *ipddef_ptr = NULL;

static int num_osd_ik = 0;
static int size_osd_ik = 0;

void process_ctrlr_file(struct rc_struct *iptrc, const char *ctype, 
		const char *filename);

/* input relelated options */
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "joytype", "jt", rc_int, &joytype, "0", 0, 7, NULL, "Select type of joystick support to use:\n"
		"0 No joystick\n"
		"1 i386 style joystick driver (if compiled in)\n"
		"2 Fm Town Pad support (if compiled in)\n"
		"3 X11 input extension joystick (if compiled in)\n"
		"4 new i386 linux 1.x.x joystick driver(if compiled in)\n"
		"5 NetBSD USB joystick driver (if compiled in)\n"
		"6 PS2-Linux native pad (if compiled in)\n"
		"7 SDL joystick driver" },
	{ "analogstick", "as", rc_bool, &analogstick, "0", 0, 0, NULL, "Use Joystick as analog for analog controls" },
	{ NULL, NULL, rc_link, joy_i386_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, joy_pad_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, joy_x11_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, joy_usb_opts, NULL, 0, 0, NULL, NULL },
#ifdef PS2_JOYSTICK
	{ NULL, NULL, rc_link, joy_ps2_opts, NULL, 0, 0, NULL, NULL },
#endif
#ifdef USE_XINPUT_DEVICES
	{ NULL, NULL, rc_link, XInputDevices_opts, NULL, 0, 0, NULL, NULL },
#endif
	{ "mouse", "m", rc_bool, &use_mouse, "1", 0, 0, NULL, "Enable/disable mouse (if supported)" },
	{ "ugcicoin", NULL, rc_bool, &ugcicoin, "0", 0, 0, NULL, "Enable/disable UGCI(tm) Coin/Play support" },
	{ "usbpspad", "pspad", rc_bool, &is_usb_ps_gamepad, "0", 0, 0, NULL, "The Joystick(s) are USB PS Game Pads" },
	{ "rapidfire", "rapidf", rc_bool, &rapidfire_enable, "0", 0, 0, NULL, "Enable rapid-fire support for joysticks" },
	{ "ctrlr", NULL, rc_string, &ctrlrtype, 0, 0, 0, NULL, "Preconfigure for specified controller" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

struct rc_option *ctrlr_input_opts = NULL;

struct rc_option ctrlr_input_opts2[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "ctrlrname", NULL, rc_string, &ctrlrname, 0, 0, 0, NULL, "name of controller" },
	{ "trackball_ini", NULL, rc_string, &trackball_ini, 0, 0, 0, NULL, "ctrlr opts if game has TRACKBALL input" },
	{ "paddle_ini", NULL, rc_string, &paddle_ini, 0, 0, 0, NULL, "ctrlr opts if game has PADDLE input" },
	{ "dial_ini", NULL, rc_string, &dial_ini, 0, 0, 0, NULL, "ctrlr opts if game has DIAL input" },
	{ "ad_stick_ini", NULL, rc_string, &ad_stick_ini, 0, 0, 0, NULL, "ctrlr opts if game has AD STICK input" },
	{ "lightgun_ini", NULL, rc_string, &lightgun_ini, 0, 0, 0, NULL, "ctrlr opts if game has LIGHTGUN input" },
	{ "pedal_ini", NULL, rc_string, &pedal_ini, 0, 0, 0, NULL, "ctrlr opts if game has PEDAL input" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

static int joy_list_equiv[][2] =
{
	{ JOY_AXIS_CODE(0,0,0), JOYCODE_1_LEFT },
	{ JOY_AXIS_CODE(0,0,1), JOYCODE_1_RIGHT },
	{ JOY_AXIS_CODE(0,1,0), JOYCODE_1_UP },
	{ JOY_AXIS_CODE(0,1,1), JOYCODE_1_DOWN },
	{ JOY_BUTTON_CODE(0,0), JOYCODE_1_BUTTON1 },
	{ JOY_BUTTON_CODE(0,1), JOYCODE_1_BUTTON2 },
	{ JOY_BUTTON_CODE(0,2), JOYCODE_1_BUTTON3 },
	{ JOY_BUTTON_CODE(0,3), JOYCODE_1_BUTTON4 },
	{ JOY_BUTTON_CODE(0,4), JOYCODE_1_BUTTON5 },
	{ JOY_BUTTON_CODE(0,5), JOYCODE_1_BUTTON6 },
	{ JOY_BUTTON_CODE(0,6), JOYCODE_1_BUTTON7 },
	{ JOY_BUTTON_CODE(0,7), JOYCODE_1_BUTTON8 },
	{ JOY_BUTTON_CODE(0,8), JOYCODE_1_BUTTON9 },
	{ JOY_BUTTON_CODE(0,9), JOYCODE_1_BUTTON10 },
	{ JOY_AXIS_CODE(1,0,0), JOYCODE_2_LEFT },
	{ JOY_AXIS_CODE(1,0,1), JOYCODE_2_RIGHT },
	{ JOY_AXIS_CODE(1,1,0), JOYCODE_2_UP },
	{ JOY_AXIS_CODE(1,1,1), JOYCODE_2_DOWN },
	{ JOY_BUTTON_CODE(1,0), JOYCODE_2_BUTTON1 },
	{ JOY_BUTTON_CODE(1,1), JOYCODE_2_BUTTON2 },
	{ JOY_BUTTON_CODE(1,2), JOYCODE_2_BUTTON3 },
	{ JOY_BUTTON_CODE(1,3), JOYCODE_2_BUTTON4 },
	{ JOY_BUTTON_CODE(1,4), JOYCODE_2_BUTTON5 },
	{ JOY_BUTTON_CODE(1,5), JOYCODE_2_BUTTON6 },
	{ JOY_BUTTON_CODE(1,6), JOYCODE_2_BUTTON7 },
	{ JOY_BUTTON_CODE(1,7), JOYCODE_2_BUTTON8 },
	{ JOY_BUTTON_CODE(1,8), JOYCODE_2_BUTTON9 },
	{ JOY_BUTTON_CODE(1,9), JOYCODE_2_BUTTON10 },
	{ JOY_AXIS_CODE(2,0,0), JOYCODE_3_LEFT },
	{ JOY_AXIS_CODE(2,0,1), JOYCODE_3_RIGHT },
	{ JOY_AXIS_CODE(2,1,0), JOYCODE_3_UP },
	{ JOY_AXIS_CODE(2,1,1), JOYCODE_3_DOWN },
	{ JOY_BUTTON_CODE(2,0), JOYCODE_3_BUTTON1 },
	{ JOY_BUTTON_CODE(2,1), JOYCODE_3_BUTTON2 },
	{ JOY_BUTTON_CODE(2,2), JOYCODE_3_BUTTON3 },
	{ JOY_BUTTON_CODE(2,3), JOYCODE_3_BUTTON4 },
	{ JOY_BUTTON_CODE(2,4), JOYCODE_3_BUTTON5 },
	{ JOY_BUTTON_CODE(2,5), JOYCODE_3_BUTTON6 },
	{ JOY_BUTTON_CODE(2,6), JOYCODE_3_BUTTON7 },
	{ JOY_BUTTON_CODE(2,7), JOYCODE_3_BUTTON8 },
	{ JOY_BUTTON_CODE(2,8), JOYCODE_3_BUTTON9 },
	{ JOY_BUTTON_CODE(2,9), JOYCODE_3_BUTTON10 },
	{ JOY_AXIS_CODE(3,0,0), JOYCODE_4_LEFT },
	{ JOY_AXIS_CODE(3,0,1), JOYCODE_4_RIGHT },
	{ JOY_AXIS_CODE(3,1,0), JOYCODE_4_UP },
	{ JOY_AXIS_CODE(3,1,1), JOYCODE_4_DOWN },
	{ JOY_BUTTON_CODE(3,0), JOYCODE_4_BUTTON1 },
	{ JOY_BUTTON_CODE(3,1), JOYCODE_4_BUTTON2 },
	{ JOY_BUTTON_CODE(3,2), JOYCODE_4_BUTTON3 },
	{ JOY_BUTTON_CODE(3,3), JOYCODE_4_BUTTON4 },
	{ JOY_BUTTON_CODE(3,4), JOYCODE_4_BUTTON5 },
	{ JOY_BUTTON_CODE(3,5), JOYCODE_4_BUTTON6 },
	{ JOY_BUTTON_CODE(3,6), JOYCODE_4_BUTTON7 },
	{ JOY_BUTTON_CODE(3,7), JOYCODE_4_BUTTON8 },
	{ JOY_BUTTON_CODE(3,8), JOYCODE_4_BUTTON9 },
	{ JOY_BUTTON_CODE(3,9), JOYCODE_4_BUTTON10 },
	{ 0,0 }
};

#ifdef UGCICOIN

#define PLAY_KEYCODE_BASE     KEY_1
#define COIN_KEYCODE_BASE     KEY_3
#define MAX_PLAYERS           2
#define MIN_COIN_WAIT         3

static int coin_pressed[MAX_PLAYERS];

static void ugci_callback(int id, enum ugci_event_type type, int value)
{
	struct xmame_keyboard_event event;

	if (id >= MAX_PLAYERS)
		return;

	switch (type)
	{
		case UGCI_EVENT_PLAY:
			event.press = value;
			event.scancode = PLAY_KEYCODE_BASE + id;
			xmame_keyboard_register_event(&event);
			break;

		case UGCI_EVENT_COIN:
			if (coin_pressed[id])
				return;
			event.press = coin_pressed[id] = 1;
			event.scancode = COIN_KEYCODE_BASE + id;
			xmame_keyboard_register_event(&event);
			break;

		default:
			break;
	}
}
#endif

void load_rapidfire_settings(void)
{
	FILE *fp;
	char name[BUF_SIZE];

	if (!rapidfire_enable)
		return;

	snprintf(name, BUF_SIZE, "%s/.%s/cfg/%s.rpf", home_dir, NAME, Machine->gamedrv->name);
	fp = fopen(name, "rb");
	if (fp)
	{
		int i,j;

		for (i=0; i<4; i++)
		{
			fread(&rapidfire_data[i].ctrl_button, sizeof(rapidfire_data[0].ctrl_button), 1, fp);
			for (j=0; j<10; j++)
			{
				fread(&rapidfire_data[i].setting[j], sizeof(rapidfire_data[0].setting[0]), 1, fp);
			}
		}
		fclose(fp);
	}
}

void save_rapidfire_settings(void)
{
	FILE *fp;
	char name[BUF_SIZE];

	if (!rapidfire_enable)
		return;

	snprintf(name, BUF_SIZE, "%s/.%s/cfg/%s.rpf", home_dir, NAME, Machine->gamedrv->name);
	fp = fopen(name, "wb");
	if (fp)
	{
		int i,j;
		for (i=0; i<4; i++)
		{
			fwrite(&rapidfire_data[i].ctrl_button, sizeof(rapidfire_data[0].ctrl_button), 1, fp);
			for (j=0; j<10; j++)
			{
				fwrite(&rapidfire_data[i].setting[j], sizeof(rapidfire_data[0].setting[0]), 1, fp);
			}
		}
		fclose(fp);
	}
}

/* 2 init routines one for creating the display and one after that, since some
   (most) init stuff needs a display */

int osd_input_initpre(void)
{
	int i, j, k, joy_list_count = 0;

	joy_poll_func = NULL;

	memset(joy_data,   0, sizeof(joy_data));
	memset(mouse_data, 0, sizeof(mouse_data));

	if(rapidfire_enable)
	{
		memset(rapidfire_data, 0, sizeof(rapidfire_data));

		for(i=0; i<4; i++)
		{
			rapidfire_data[i].enable = 1;
			rapidfire_data[i].ctrl_button = -1;
			rapidfire_data[i].ctrl_prev_status = 0;
			for(j=0; j<10; j++)
			{
				rapidfire_data[i].setting[j] = 64;
				rapidfire_data[i].status[j] = 0;
			}
		}
		load_rapidfire_settings();
	}

	for(i=0; i<JOY; i++)
	{
		joy_data[i].fd = -1;
		for(j=0; j<JOY_AXIS; j++)
		{
			joy_data[i].axis[j].min = -10;
			joy_data[i].axis[j].max =  10;
			for(k=0; k<JOY_DIRS; k++)
			{
				snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
						"Joy %d axis %d %s", i+1, j+1, (k)? "pos":"neg");
				joy_list_count++;
			}
		}
		for(j=0; j<JOY_BUTTONS; j++)
		{
			snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
					"Joy %d button %d", i+1 ,j+1);
			joy_list_count++;
		}
	}

	for(i=0; i<MOUSE; i++)
	{
		for(j=0; j<MOUSE_BUTTONS; j++)
		{
			snprintf(joy_list_names[joy_list_count], JOY_NAME_LEN,
					"Mouse %d button %d", i+1, j+1);
			joy_list_count++;
		}
	}

	/* terminate array */
	joy_list[joy_list_count].name = 0;
	joy_list[joy_list_count].code = 0;
	joy_list[joy_list_count].standardcode = 0;

	/* fill in codes */
	for (i=0; i<joy_list_count; i++)
	{
		joy_list[i].code = i;
		joy_list[i].name = joy_list_names[i];
		joy_list[i].standardcode = JOYCODE_OTHER;

		for(j=0; joy_list_equiv[j][1]; j++)
		{
			if (joy_list_equiv[j][0] == joy_list[i].code)
			{
				joy_list[i].standardcode = joy_list_equiv[j][1];
				break;
			}
		}
	}

	if (use_mouse)
		fprintf (stderr_file, "Mouse/Trakball selected.\n");

#ifdef UGCICOIN
	if (ugcicoin) {
		if (ugci_init(ugci_callback, UGCI_EVENT_MASK_COIN | UGCI_EVENT_MASK_PLAY, 1) <= 0)
			ugcicoin = 0;
	}
#endif

#ifdef JOY_PS2
	/* Special mapping for PlayStation2 -- to be removed when 0.60 patch done */
	/* Add mappings for P1 SELECT, START, P2 SELECT, START */
	joy_list[JOY_BUTTON_CODE(0,6)] = JOYCODE_1_SELECT;
	joy_list[JOY_BUTTON_CODE(0,7)] = JOYCODE_1_START;
	joy_list[JOY_BUTTON_CODE(1,6)] = JOYCODE_2_SELECT;
	joy_list[JOY_BUTTON_CODE(1,7)] = JOYCODE_2_START;
	/* For now, L2 is equivalent of TAB, and R2 is equivalent of ESC */
	joy_list[JOY_BUTTON_CODE(0,8)] = KEYCODE_TAB;
	joy_list[JOY_BUTTON_CODE(0,9)] = KEYCODE_ESC;
	/* Remap L3 and R3 to BUTTON7 and BUTTON8 */
	joy_list[JOY_BUTTON_CODE(0,10)] = JOYCODE_1_BUTTON7;
	joy_list[JOY_BUTTON_CODE(0,11)] = JOYCODE_1_BUTTON8;
	joy_list[JOY_BUTTON_CODE(1,10)] = JOYCODE_2_BUTTON7;
	joy_list[JOY_BUTTON_CODE(1,11)] = JOYCODE_2_BUTTON8;
	/* Map the 4 directional buttons to the four axes. */
	joy_list[JOY_BUTTON_CODE(0,12)] = JOYCODE_1_LEFT;
	joy_list[JOY_BUTTON_CODE(0,13)] = JOYCODE_1_RIGHT;
	joy_list[JOY_BUTTON_CODE(0,14)] = JOYCODE_1_UP;
	joy_list[JOY_BUTTON_CODE(0,15)] = JOYCODE_1_DOWN;
	joy_list[JOY_BUTTON_CODE(1,12)] = JOYCODE_2_LEFT;
	joy_list[JOY_BUTTON_CODE(1,13)] = JOYCODE_2_RIGHT;
	joy_list[JOY_BUTTON_CODE(1,14)] = JOYCODE_2_UP;
	joy_list[JOY_BUTTON_CODE(1,15)] = JOYCODE_2_DOWN;
#endif

	return OSD_OK;
}

int osd_input_initpost(void)
{
	int i;

#ifdef USE_XINPUT_DEVICES
	XInputDevices_init();
#endif

	/* init the keyboard */
	if (xmame_keyboard_init())
		return OSD_NOT_OK;

	/* joysticks */
	switch (joytype)
	{
		case JOY_NONE:
			break;
#ifdef I386_JOYSTICK
		case JOY_I386NEW:
		case JOY_I386:
			joy_i386_init();
			break;
#endif
#ifdef LIN_FM_TOWNS
		case JOY_PAD:
			joy_pad_init ();
			break;
#endif
#ifdef X11_JOYSTICK
		case JOY_X11:
			joy_x11_init();
			break;
#endif
#ifdef USB_JOYSTICK
		case JOY_USB:
			joy_usb_init();
			break;
#endif
#ifdef PS2_JOYSTICK
		case JOY_PS2:
			joy_ps2_init();
			break;
#endif
#ifdef SDL
		case JOY_SDL:
			joy_SDL_init();
			break;
#endif
		default:
			fprintf (stderr_file, "OSD: Warning: unknown joytype: %d, or joytype not compiled in.\n"
					"   Disabling joystick support.\n", joytype);
			joytype = JOY_NONE;
	}

	if(joytype != JOY_NONE)
	{
		int found = FALSE;

		for (i=0; i<JOY; i++)
		{
			if(joy_data[i].num_axis || joy_data[i].num_buttons)
			{
				fprintf(stderr_file, "OSD: Info: Joystick %d, %d axis, %d buttons\n",
						i, joy_data[i].num_axis, joy_data[i].num_buttons);
				found = TRUE;
			}
		}

		if (!found)
		{
			fprintf(stderr_file, "OSD: Warning: No joysticks found disabling joystick support\n");
			joytype = JOY_NONE;
		}
	}

	return OSD_OK;
}

void osd_input_close(void)
{
	int i;

	xmame_keyboard_exit();

	switch(joytype)
	{
#ifdef PS2_JOYSTICK
		case JOY_PS2:
			joy_ps2_exit();
			break;
#endif
		default:
			break;
	}

	for(i=0;i<JOY;i++)
		if(joy_data[i].fd >= 0)
			close(joy_data[i].fd);

	if(rapidfire_enable)
		save_rapidfire_settings();
}

/* return a list of all available joys */
const struct JoystickInfo *osd_get_joy_list(void)
{
	return joy_list;
}

/* <jake> */
void osd_trak_read(int player,int *deltax,int *deltay)
{
#ifdef USE_XINPUT_DEVICES
	if (player < XINPUT_JOYSTICK_1)
	{
		XInputPollDevices(player,deltax,deltay);
	}
	else
	{
		*deltax = 0;
		*deltay = 0;
	}
#else
	if (player < MOUSE)
	{
		*deltax = mouse_data[player].deltas[0];
		*deltay = mouse_data[player].deltas[1];
	}
	else
	{
		*deltax = 0;
		*deltay = 0;
	}
#endif
}
/* </jake> */

int get_rapidfire_speed(int joy_num, int button_num)
{
	if (joy_num < 0 || 3 < joy_num)
		return 0;
	if (button_num < 0 || 9 < button_num)
		return 0;

	return rapidfire_data[joy_num].setting[button_num];
}

void set_rapidfire_speed(int joy_num, int button_num, int speed)
{
	if (joy_num < 0 || 3 < joy_num)
		return;
	if (button_num < 0 || 9 < button_num)
		return;

	rapidfire_data[joy_num].setting[button_num] = speed;
	rapidfire_data[joy_num].status[button_num] = 0;
}

int is_rapidfire_ctrl_button(int joy_num, int button_num)
{
	if (joy_num < 0 || 3 < joy_num)
		return 0;
	if (button_num < 0 || 9 < button_num)
		return 0;

	if (button_num == rapidfire_data[joy_num].ctrl_button)
		return 1;
	else
		return 0;
}

int no_rapidfire_ctrl_button(int joy_num)
{
	if (joy_num < 0 || 3 < joy_num)
		return 0;

	if (rapidfire_data[joy_num].ctrl_button < 0 ||
			rapidfire_data[joy_num].ctrl_button > 9)
		return 1;
	else
		return 0;
}

void set_rapidfire_ctrl_button(int joy_num, int button_num)
{
	if (joy_num < 0 || 3 < joy_num)
		return;
	if (button_num < 0 || 9 < button_num)
		return;

	rapidfire_data[joy_num].ctrl_button = button_num;
}

void unset_rapidfire_ctrl_button(int joy_num)
{
	if (joy_num < 0 || 3 < joy_num)
		return;

	rapidfire_data[joy_num].ctrl_button = -1;
}

int setrapidfire(struct mame_bitmap *bitmap, int selected)
{
	const char *menu_item[42] = {
		"Joy1Button 1",
		"Joy1Button 2",
		"Joy1Button 3",
		"Joy1Button 4",
		"Joy1Button 5",
		"Joy1Button 6",
		"Joy1Button 7",
		"Joy1Button 8",
		"Joy1Button 9",
		"Joy1Button10",
		"Joy2Button 1",
		"Joy2Button 2",
		"Joy2Button 3",
		"Joy2Button 4",
		"Joy2Button 5",
		"Joy2Button 6",
		"Joy2Button 7",
		"Joy2Button 8",
		"Joy2Button 9",
		"Joy2Button10",
		"Joy3Button 1",
		"Joy3Button 2",
		"Joy3Button 3",
		"Joy3Button 4",
		"Joy3Button 5",
		"Joy3Button 6",
		"Joy3Button 7",
		"Joy3Button 8",
		"Joy3Button 9",
		"Joy3Button10",
		"Joy4Button 1",
		"Joy4Button 2",
		"Joy4Button 3",
		"Joy4Button 4",
		"Joy4Button 5",
		"Joy4Button 6",
		"Joy4Button 7",
		"Joy4Button 8",
		"Joy4Button 9",
		"Joy4Button10",
		0,
		0              /* terminate array */
	};
	const char *menu_subitem[42];
	char sub[42][16];
	int sel;
	int arrowize;
	int items = 41;
	int joy;
	int button;
	int d0,flag;

	menu_subitem[40] = 0;
	menu_item[40] = ui_getstring (UI_returntomain);

	sel = selected - 1;

	for (joy = 0; joy < 4; joy += 1)
	{
		for (button = 0; button < 10; button += 1)
		{
			if (is_rapidfire_ctrl_button(joy, button))
			{
				sprintf(&sub[ (joy*10) + button ][0], "    SWITCH");
			}
			else
			{
				int speed = get_rapidfire_speed(joy, button);
				if (speed & 0x0100)
					sprintf(&sub[ (joy*10) + button ][0], "RAPID %3d", speed & 0xFF);
				else if(speed & 0x0200)
					sprintf(&sub[ (joy*10) + button ][0], "CHARGE %3d", speed & 0xFF);
				else
					sprintf(&sub[ (joy*10) + button ][0], "     OFF");
			}
			menu_subitem[ (joy*10) + button ] = &sub[ (joy*10) + button ][0];
		}
	}
	arrowize = 0;
	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN,8))
		sel = (sel + 1) % items;

	if (input_ui_pressed_repeat(IPT_UI_UP,8))
		sel = (sel + items - 1) % items;

	if (input_ui_pressed_repeat(IPT_UI_LEFT,8))
	{
		if (sel < items - 1)
		{
			joy    = sel / 10;
			button = sel % 10;
			d0 = get_rapidfire_speed(joy, button);
			flag = d0 & 0xff00;
			if((flag & 0x300)&&(!is_rapidfire_ctrl_button(joy, button)))
			{
				d0 &= 0xff;
				if              (d0 <= 32){
					d0 = 128;
				}else if(d0 <= 64){
					d0 = 32;
				}else if(d0 <= 96){
					d0 = 64;
				}else if(d0 <= 128){
					d0 = 96;
				}else{
					d0 = 128;
				}
				d0 |= flag;
				set_rapidfire_speed(joy, button, d0);
				/* tell updatescreen() to clean after us (in case the window changes size) */
				schedule_full_refresh();
			}
		}
	}

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,8))
	{
		if (sel < items - 1)
		{
			joy    = sel / 10;
			button = sel % 10;
			d0 = get_rapidfire_speed(joy, button);
			flag = d0 & 0xff00;
			if((flag & 0x300)&&(is_rapidfire_ctrl_button(joy, button)))
			{
				d0 &= 0xff;
				if (d0 <= 32)
					d0 = 64;
				else if (d0 <= 64)
					d0 = 96;
				else if (d0 <= 96)
					d0 = 128;
				else if (d0 <= 128)
					d0 = 32;
				else
					d0 = 128;

				d0 |= flag;
				set_rapidfire_speed(joy, button, d0);
				/* tell updatescreen() to clean after us (in case the window changes size) */
				schedule_full_refresh();
			}
		}
	}

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		if (sel == items - 1) sel = -1;         /* cancel */
		else if (sel < items - 1)
		{
			joy    = sel / 10;
			button = sel % 10;
			d0 = get_rapidfire_speed(joy, button);
			flag = d0 & 0xff00;
			d0 &= 0xff;
			if (is_rapidfire_ctrl_button(joy, button))
			{
				/* off */
				flag = 0;
				unset_rapidfire_ctrl_button(joy);
			}
			else if(flag & 0x100)
			{
				flag = 0x200; /* rapid */
			}
			else if(flag & 0x200)
			{
				/* switch */
				if(no_rapidfire_ctrl_button(joy))
				{
					set_rapidfire_ctrl_button(joy, button);
				}
				flag = 0;
			}
			else
			{
				flag = 0x100; /* charge */
			}
			d0 |= flag;
			set_rapidfire_speed(joy, button, d0);
			/* tell updatescreen() to clean after us (in case the window changes size) */
			schedule_full_refresh();
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;                                                       
	/* cancel */

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

void joystick_rapidfire(void)
{
	int joy_num;
	int button_num;
	int ctrl;

	if (setup_active())
		return;

	for (joy_num=0; joy_num<4 && joy_data[joy_num].fd>=0; joy_num++)
	{
		ctrl = rapidfire_data[joy_num].ctrl_button;
		if (ctrl >= 0 && ctrl < 10 && ctrl < joy_data[joy_num].num_buttons)
		{
			if (joy_data[joy_num].buttons[ctrl] &&
					rapidfire_data[joy_num].ctrl_prev_status == 0)
			{
				rapidfire_data[joy_num].enable = 1 - rapidfire_data[joy_num].enable;
			}
			rapidfire_data[joy_num].ctrl_prev_status = joy_data[joy_num].buttons[ctrl];
		}
		if (rapidfire_data[joy_num].enable)
		{
			for (button_num = 0;
					button_num < 10 && button_num < joy_data[joy_num].num_buttons;
					button_num++)
			{
				if (rapidfire_data[joy_num].setting[button_num] & 0xff &&
						rapidfire_data[joy_num].setting[button_num] & 0x300)
				{
					if (joy_data[joy_num].buttons[button_num] != 0)
					{
						joy_data[joy_num].buttons[button_num] &=
							(rapidfire_data[joy_num].status[button_num] >> 7) ? 0xFFFF : 0;
						rapidfire_data[joy_num].status[button_num] +=
							rapidfire_data[joy_num].setting[button_num];
						rapidfire_data[joy_num].status[button_num] &= 0xFF;
					}else{
						rapidfire_data[joy_num].status[button_num] = 0xFF;

						/* charge */
						if ((rapidfire_data[joy_num].setting[button_num] & 0x300) == 0x200)
							joy_data[joy_num].buttons[button_num] = 0xFFFF;
					}
				}
			}
		}
	}
}

static struct joydata_struct prev_joy_data[JOY];

void store_button_state(void)
{
	int i;

	for (i=0; i<JOY; i++)
		prev_joy_data[i] = joy_data[i];
}

void restore_button_state(void)
{
	int i, j;

	for (i=0; i<JOY; i++)
		for (j=0; j<JOY_BUTTONS; j++)
			joy_data[i].buttons[j] = prev_joy_data[i].buttons[j];
}

void osd_poll_joysticks(void)
{
	if (use_mouse)
		sysdep_mouse_poll ();
	if (joy_poll_func)
	{
		if (rapidfire_enable)
			restore_button_state();

		(*joy_poll_func) ();

		if (rapidfire_enable)
		{
			store_button_state();
			joystick_rapidfire();
		}
	}
#ifdef UGCICOIN
	if (ugcicoin)
	{
		int id;

		ugci_poll(0);

		/* Check coin-pressed. Simulate a release event */
		for (id = 0; id < MAX_PLAYERS; id++)
		{
			if (coin_pressed[id] && coin_pressed[id]++ > MIN_COIN_WAIT)
			{
				struct xmame_keyboard_event event;

				event.press = coin_pressed[id] = 0;
				event.scancode = COIN_KEYCODE_BASE + id;
				xmame_keyboard_register_event(&event);
			}
		}
	}
#endif
}

int osd_is_joy_pressed (int joycode)
{
	if (joycode >= (JOY_LIST_LEN+MOUSE_LIST_LEN))
		return FALSE;

	if (MOUSE_IS_BUTTON(joycode))
	{
		int mouse  = MOUSE_GET_MOUSE(joycode);
		int button = MOUSE_GET_BUTTON(joycode);
		return mouse_data[mouse].buttons[button];
	}
	else
	{
		int joy = JOY_GET_JOY(joycode);

		if (JOY_IS_AXIS(joycode))
		{
			int axis = JOY_GET_AXIS(joycode);
			int dir  = JOY_GET_DIR(joycode);
			return joy_data[joy].axis[axis].dirs[dir];
		}
		else
		{
			int button = JOY_GET_BUTTON(joycode);
			return joy_data[joy].buttons[button];
		}
	}
}

/*
 * given a new x an y joystick axis value convert it to a move definition
 */

void joy_evaluate_moves(void)
{
	int i, j, threshold;

	if( is_usb_ps_gamepad )
	{
		for (i=0; i<JOY; i++)
		{
			joy_data[i].axis[0].dirs[0] = joy_data[i].buttons[15] == 1 ? TRUE : FALSE;
			joy_data[i].axis[0].dirs[1] = joy_data[i].buttons[13] == 1 ? TRUE : FALSE;
			joy_data[i].axis[1].dirs[0] = joy_data[i].buttons[12] == 1 ? TRUE : FALSE;
			joy_data[i].axis[1].dirs[1] = joy_data[i].buttons[14] == 1 ? TRUE : FALSE;
		}
	} 
	else 
	{
		for (i=0; i<JOY; i++)
		{
			for (j=0; j<joy_data[i].num_axis; j++)
			{
				memset(joy_data[i].axis[j].dirs, FALSE, JOY_DIRS*sizeof(int));

				/* auto calibrate */
				/* sdevaux 04/2003 : update middle when autocalibrate */
				if (joy_data[i].axis[j].val > joy_data[i].axis[j].max)
				{
					joy_data[i].axis[j].max = joy_data[i].axis[j].val;
					joy_data[i].axis[j].center = (joy_data[i].axis[j].max + joy_data[i].axis[j].min)/2;
				}
				else if (joy_data[i].axis[j].val < joy_data[i].axis[j].min)
				{
					joy_data[i].axis[j].min = joy_data[i].axis[j].val;
					joy_data[i].axis[j].center = (joy_data[i].axis[j].max + joy_data[i].axis[j].min)/2;
				}

				threshold = (joy_data[i].axis[j].max - joy_data[i].axis[j].center) >> 1;

				if (joy_data[i].axis[j].val < (joy_data[i].axis[j].center - threshold))
					joy_data[i].axis[j].dirs[0] = TRUE;
				else if (joy_data[i].axis[j].val > (joy_data[i].axis[j].center + threshold))
					joy_data[i].axis[j].dirs[1] = TRUE;
			}
		}
	}
}

/* 
 * return a value in the range -128 .. 128 (yes, 128, not 127)
 * sdevaux 02/2003 : Updated from windows code.
 * sdevaux 04/2003 : fix y-axis not seen as analog (reported by Paul Rahme)
 */
void osd_analogjoy_read(int player, int analog_axis[],
		InputCode analogjoy_input[])
{
	int i, j;
	/* is player var enough to select joystick : what if joystick 2 is mapped to
	   player 1 ? */
	int max_axes=joy_data[player].num_axis;
	if (max_axes>MAX_ANALOG_AXES)
		max_axes=MAX_ANALOG_AXES;
	for (i=0; analogstick && i<max_axes ; i++)
	{
		struct axisdata_struct * axis;
		axis=&(joy_data[player].axis[i]);
		analog_axis[i] = (axis->val - axis->center) * 257 / (axis->max - axis->min);
		/* Does overflow can really happen, since axis->max and axis->min are updated
		   when val is outside [min;max] (cf void joy_evaluate_moves(void) ) ? */
		if (analog_axis[i] < -128)
			analog_axis[i] = -128;
		if (analog_axis[i] >  128)
			analog_axis[i] =  128;
		if (JOYTYPE( analogjoy_input[i] ) == JOYTYPE_AXIS_POS)
			analog_axis[i] = -analog_axis[i];
	}

	/* set remaining axes to MAX_ANALOG_AXES to 0 */
	for (j=i;j<MAX_ANALOG_AXES;j++)
		analog_axis[j]=0;
}

/*
 * sdevaux 02/2003 : Updated from windows code.
 */
int osd_is_joystick_axis_code(int joycode)
{
	switch (JOYTYPE(joycode))
	{
		case JOYTYPE_AXIS_POS:
		case JOYTYPE_AXIS_NEG:
			return 1;
		default:
			return 0;
	}
}

void osd_lightgun_read(int player, int *deltax, int *deltay)
{
	/* NEED TO FILL THIS IN */
}

int osd_joystick_needs_calibration(void)
{
	/* 
	 * xmame uses the kernel's joystick drivers calibration, or 
	 * autocalibration and thus never needs this
	 */
	return 0;
}

void osd_joystick_start_calibration(void)
{
}

const char *osd_joystick_calibrate_next(void)
{
	return NULL;
}

void osd_joystick_calibrate(void)
{
}

void osd_joystick_end_calibration(void)
{
}


extern struct rc_struct *rc;

void process_ctrlr_file(struct rc_struct *iptrc, const char *ctype, 
		const char *filename)
{
	mame_file *f;

	/* open the specified controller type/filename */
	f = mame_fopen (ctype, filename, FILETYPE_CTRLR, 0);

	if (f)
	{
		if (ctype)
			fprintf (stderr_file, "trying to parse ctrlr file %s/%s.ini\n", ctype, filename);
		else
			fprintf (stderr_file, "trying to parse ctrlr file %s.ini\n", filename);

		/* process this file */
		if (osd_rc_read(iptrc, f, filename, 1, 1))
		{
			if (ctype)
				fprintf (stderr_file, "problem parsing ctrlr file %s/%s.ini\n", ctype, filename);
			else
				fprintf (stderr_file, "problem parsing ctrlr file %s.ini\n", filename);
		}
	}

	/* close the file */
	if (f)
		mame_fclose (f);
}

void process_ctrlr_game(struct rc_struct *iptrc, const char *ctype, 
		const struct GameDriver *drv)
{
	/* recursive call to process parents first */
	if (drv->clone_of)
		process_ctrlr_game (iptrc, ctype, drv->clone_of);

	/* now process this game */
	if (drv->name && *(drv->name) != 0)
		process_ctrlr_file (iptrc, ctype, drv->name);
}

/* nice hack: load source_file.ini (omit if referenced later any) */
void process_ctrlr_system(struct rc_struct *iptrc, const char *ctype, 
		const struct GameDriver *drv)
{
	char buffer[128];
	const struct GameDriver *tmp_gd;

	sprintf(buffer, "%s", drv->source_file+12);
	buffer[strlen(buffer) - 2] = 0;

	tmp_gd = drv;
	while (tmp_gd != NULL)
	{
		if (strcmp(tmp_gd->name, buffer) == 0) break;
		tmp_gd = tmp_gd->clone_of;
	}

	/* not referenced later, so load it here */
	if (tmp_gd == NULL)
		/* now process this system */
		process_ctrlr_file (iptrc, ctype, buffer);
}

static int ipdef_custom_rc_func(struct rc_option *option, const char *arg, 
		int priority)
{
	struct ik *pinput_keywords = (struct ik *)option->dest;
	struct ipd *idef = ipddef_ptr;

	/* only process the default definitions if the input port definitions */
	/* pointer has been defined */
	if (idef)
	{
		/* if a keycode was re-assigned */
		if (pinput_keywords->type == IKT_STD)
		{
			InputSeq is;

			/* get the new keycode */
			seq_set_string (&is, arg);

			/* was a sequence was assigned to a keycode? - not valid! */
			if (is[1] != CODE_NONE)
			{
				fprintf(stderr_file, "error: can't map \"%s\" to \"%s\"\n",pinput_keywords->name,arg);
			}

			/* for all definitions */
			while (idef->type != IPT_END)
			{
				int j;

				/* reassign all matching keystrokes to the given argument */
				for (j = 0; j < SEQ_MAX; j++)
				{
					/* if the keystroke matches */
					if (idef->seq[j] == pinput_keywords->val)
					{
						/* re-assign */
						idef->seq[j] = is[0];
					}
				}
				/* move to the next definition */
				idef++;
			}
		}

		/* if an input definition was re-defined */
		else if (pinput_keywords->type == IKT_IPT ||
                 pinput_keywords->type == IKT_IPT_EXT)
		{
			/* loop through all definitions */
			while (idef->type != IPT_END)
			{
				/* if the definition matches */
				if (idef->type == pinput_keywords->val)
				{
                    if (pinput_keywords->type == IKT_IPT_EXT)
                        idef++;
					seq_set_string(&idef->seq, arg);
					/* and abort (there shouldn't be duplicate definitions) */
					break;
				}

				/* move to the next definition */
				idef++;
			}
		}
	}

	return 0;
}


/*============================================================ */
/*	osd_customize_inputport_defaults */
/*============================================================ */

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	static InputSeq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	UINT32 next_reserved = IPT_OSD_1;
	struct ipd *idef = defaults;
	int i;

	/* loop over all the defaults */
	while (idef->type != IPT_END)
	{
		/* if the type is OSD reserved */
		if (idef->type == IPT_OSD_RESERVED)
		{
			/* process the next reserved entry */
			switch (next_reserved)
			{
				/* OSD_1 is alt-enter for fullscreen */
				case IPT_OSD_1:
					idef->type = next_reserved;
					idef->name = "Toggle fullscreen";
					seq_set_2 (&idef->seq, KEYCODE_LALT, KEYCODE_ENTER);
				break;

				default:
				break;
			}
			next_reserved++;
		}

		/* disable the config menu if the ALT key is down */
		/* (allows ALT-TAB to switch between windows apps) */
		if (idef->type == IPT_UI_CONFIGURE)
		{
			seq_copy(&idef->seq, &no_alt_tab_seq);
		}

		/* find the next one */
		idef++;
	}

	/* create a structure for the input port options */
	if (!(ctrlr_input_opts = calloc (num_ik+num_osd_ik+1, sizeof(struct rc_option))))
	{
		fprintf(stderr, "error on ctrlr_input_opts creation\n");
		exit(1);
	}

	/* Populate the structure with the input_keywords. */
	/* For all, use the ipdef_custom_rc_func callback. */
	/* Also, reference the original ik structure. */
	for (i=0; i<num_ik+num_osd_ik; i++)
	{
		if (i < num_ik)
		{
	   		ctrlr_input_opts[i].name = input_keywords[i].name;
			ctrlr_input_opts[i].dest = (void *)&input_keywords[i];
		}
		else
		{
	   		ctrlr_input_opts[i].name = osd_input_keywords[i-num_ik].name;
			ctrlr_input_opts[i].dest = (void *)&osd_input_keywords[i-num_ik];
		}
		ctrlr_input_opts[i].shortname = NULL;
		ctrlr_input_opts[i].type = rc_use_function;
		ctrlr_input_opts[i].deflt = NULL;
		ctrlr_input_opts[i].min = 0.0;
		ctrlr_input_opts[i].max = 0.0;
		ctrlr_input_opts[i].func = ipdef_custom_rc_func;
		ctrlr_input_opts[i].help = NULL;
		ctrlr_input_opts[i].priority = 0;
	}

	/* add an end-of-opts indicator */
	ctrlr_input_opts[i].type = rc_end;

	if (rc_register(rc, ctrlr_input_opts))
	{
		fprintf (stderr, "error on registering ctrlr_input_opts\n");
		exit(1);
	}

	if (rc_register(rc, ctrlr_input_opts2))
	{
		fprintf (stderr, "error on registering ctrlr_input_opts2\n");
		exit(1);
	}

	/* set a static variable for the ipdef_custom_rc_func callback */
	ipddef_ptr = defaults;

	/* process the main platform-specific default file */
	process_ctrlr_file (rc, NULL, "default");

	/* if a custom controller has been selected */
	if (ctrlrtype && *ctrlrtype != 0 && (stricmp(ctrlrtype,"Standard") != 0))
	{
		const struct InputPortTiny* input = Machine->gamedrv->input_ports;
		int paddle = 0, dial = 0, trackball = 0, adstick = 0, pedal = 0, lightgun = 0;

		/* process the controller-specific default file */
		process_ctrlr_file (rc, ctrlrtype, "default");

		/* process the system-specific files for this controller */
		process_ctrlr_system (rc, ctrlrtype, Machine->gamedrv);

		/* process the game-specific files for this controller */
		process_ctrlr_game (rc, ctrlrtype, Machine->gamedrv);
	}
}
