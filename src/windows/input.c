#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif

//============================================================
//
//	input.c - Win32 implementation of MAME input routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <winioctl.h>

// undef WINNT for dinput.h to prevent duplicate definition
#undef WINNT
#include <dinput.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "rc.h"
#include "input.h"



//============================================================
//	IMPORTS
//============================================================

extern int verbose;
extern int win_physical_width;
extern int win_physical_height;
extern int win_window_mode;


//============================================================
//	PARAMETERS
//============================================================

#define MAX_KEYBOARDS		1
#define MAX_MICE			8
#define MAX_JOYSTICKS		8

#define MAX_KEYS			256

#define MAX_JOY				256
#define MAX_AXES			8
#define MAX_BUTTONS			32
#define MAX_POV				4



//============================================================
//	MACROS
//============================================================

#define STRUCTSIZE(x)		((dinput_version == 0x0300) ? sizeof(x##_DX3) : sizeof(x))

#define ELEMENTS(x)			(sizeof(x) / sizeof((x)[0]))



//============================================================
//	GLOBAL VARIABLES
//============================================================

UINT8						win_trying_to_quit;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectInput variables
static LPDIRECTINPUT		dinput;
static int					dinput_version;

// global states
static int					input_paused;
static cycles_t				last_poll;

// Controller override options
//static int					hotrod;
//static int					hotrodse;
static float				a2d_deadzone;
static int					use_mouse;
static int					use_joystick;
static int					use_lightgun;
static int					use_keyboard_leds;
static int					steadykey;
static const char*			ctrlrtype;
static const char*			ctrlrname;
static const char*			trackball_ini;
static const char*			paddle_ini;
static const char*			dial_ini;
static const char*			ad_stick_ini;
static const char*			pedal_ini;
static const char*			lightgun_ini;

// this is used for the ipdef_custom_rc_func
static struct ipd 			*ipddef_ptr = NULL;

static int					num_osd_ik = 0;
static int					size_osd_ik = 0;

// keyboard states
static int					keyboard_count;
static LPDIRECTINPUTDEVICE	keyboard_device[MAX_KEYBOARDS];
static LPDIRECTINPUTDEVICE2	keyboard_device2[MAX_KEYBOARDS];
static DIDEVCAPS			keyboard_caps[MAX_KEYBOARDS];
static BYTE					keyboard_state[MAX_KEYBOARDS][MAX_KEYS];

// additional key data
static INT8					oldkey[MAX_KEYS];
static INT8					currkey[MAX_KEYS];

// mouse states
static int					mouse_active;
static int					mouse_count;
static LPDIRECTINPUTDEVICE	mouse_device[MAX_MICE];
static LPDIRECTINPUTDEVICE2	mouse_device2[MAX_MICE];
static DIDEVCAPS			mouse_caps[MAX_MICE];
static DIMOUSESTATE			mouse_state[MAX_MICE];
static int					lightgun_count;

// joystick states
static int					joystick_count;
static LPDIRECTINPUTDEVICE	joystick_device[MAX_JOYSTICKS];
static LPDIRECTINPUTDEVICE2	joystick_device2[MAX_JOYSTICKS];
static DIDEVCAPS			joystick_caps[MAX_JOYSTICKS];
static DIJOYSTATE			joystick_state[MAX_JOYSTICKS];
static DIPROPRANGE			joystick_range[MAX_JOYSTICKS][MAX_AXES];

// led states
static int original_leds;
static HANDLE hKbdDev;
static OSVERSIONINFO osinfo = { sizeof(OSVERSIONINFO) };


//============================================================
//	OPTIONS
//============================================================

// global input options
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input device options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
//	{ "hotrod", NULL, rc_bool, &hotrod, "0", 0, 0, NULL, "preconfigure for hotrod" },
//	{ "hotrodse", NULL, rc_bool, &hotrodse, "0", 0, 0, NULL, "preconfigure for hotrod se" },
	{ "mouse", NULL, rc_bool, &use_mouse, "0", 0, 0, NULL, "enable mouse input" },
	{ "joystick", "joy", rc_bool, &use_joystick, "0", 0, 0, NULL, "enable joystick input" },
	{ "lightgun", "gun", rc_bool, &use_lightgun, "0", 0, 0, NULL, "enable lightgun input" },
	{ "steadykey", "steady", rc_bool, &steadykey, "0", 0, 0, NULL, "enable steadykey support" },
	{ "keyboard_leds", "leds", rc_bool, &use_keyboard_leds, "1", 0, 0, NULL, "enable keyboard LED emulation" },
	{ "a2d_deadzone", "a2d", rc_float, &a2d_deadzone, "0.3", 0.0, 1.0, NULL, "minimal analog value for digital input" },
	{ "ctrlr", NULL, rc_string, &ctrlrtype, 0, 0, 0, NULL, "preconfigure for specified controller" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
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


//============================================================
//	PROTOTYPES
//============================================================

static void updatekeyboard(void);
static void init_keylist(void);
static void init_joylist(void);



//============================================================
//	KEYBOARD LIST
//============================================================

// this will be filled in dynamically
static struct KeyboardInfo keylist[MAX_KEYS];

// macros for building/mapping keycodes
#define KEYCODE(dik, vk, ascii)		((dik) | ((vk) << 8) | ((ascii) << 16))
#define DICODE(keycode)				((keycode) & 0xff)
#define VKCODE(keycode)				(((keycode) >> 8) & 0xff)
#define ASCIICODE(keycode)			(((keycode) >> 16) & 0xff)

// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3

// master translation table
static int key_trans_table[][4] =
{
	// MAME key				dinput key			virtual key		ascii
	{ KEYCODE_ESC, 			DIK_ESCAPE,			VK_ESCAPE,	 	27 },
	{ KEYCODE_1, 			DIK_1,				'1',			'1' },
	{ KEYCODE_2, 			DIK_2,				'2',			'2' },
	{ KEYCODE_3, 			DIK_3,				'3',			'3' },
	{ KEYCODE_4, 			DIK_4,				'4',			'4' },
	{ KEYCODE_5, 			DIK_5,				'5',			'5' },
	{ KEYCODE_6, 			DIK_6,				'6',			'6' },
	{ KEYCODE_7, 			DIK_7,				'7',			'7' },
	{ KEYCODE_8, 			DIK_8,				'8',			'8' },
	{ KEYCODE_9, 			DIK_9,				'9',			'9' },
	{ KEYCODE_0, 			DIK_0,				'0',			'0' },
	{ KEYCODE_MINUS, 		DIK_MINUS, 			0xbd,			'-' },
	{ KEYCODE_EQUALS, 		DIK_EQUALS,		 	0xbb,			'=' },
	{ KEYCODE_BACKSPACE,	DIK_BACK, 			VK_BACK, 		8 },
	{ KEYCODE_TAB, 			DIK_TAB, 			VK_TAB, 		9 },
	{ KEYCODE_Q, 			DIK_Q,				'Q',			'Q' },
	{ KEYCODE_W, 			DIK_W,				'W',			'W' },
	{ KEYCODE_E, 			DIK_E,				'E',			'E' },
	{ KEYCODE_R, 			DIK_R,				'R',			'R' },
	{ KEYCODE_T, 			DIK_T,				'T',			'T' },
	{ KEYCODE_Y, 			DIK_Y,				'Y',			'Y' },
	{ KEYCODE_U, 			DIK_U,				'U',			'U' },
	{ KEYCODE_I, 			DIK_I,				'I',			'I' },
	{ KEYCODE_O, 			DIK_O,				'O',			'O' },
	{ KEYCODE_P, 			DIK_P,				'P',			'P' },
	{ KEYCODE_OPENBRACE,	DIK_LBRACKET, 		0xdb,			'[' },
	{ KEYCODE_CLOSEBRACE,	DIK_RBRACKET, 		0xdd,			']' },
	{ KEYCODE_ENTER, 		DIK_RETURN, 		VK_RETURN, 		13 },
	{ KEYCODE_LCONTROL, 	DIK_LCONTROL, 		VK_CONTROL, 	0 },
	{ KEYCODE_A, 			DIK_A,				'A',			'A' },
	{ KEYCODE_S, 			DIK_S,				'S',			'S' },
	{ KEYCODE_D, 			DIK_D,				'D',			'D' },
	{ KEYCODE_F, 			DIK_F,				'F',			'F' },
	{ KEYCODE_G, 			DIK_G,				'G',			'G' },
	{ KEYCODE_H, 			DIK_H,				'H',			'H' },
	{ KEYCODE_J, 			DIK_J,				'J',			'J' },
	{ KEYCODE_K, 			DIK_K,				'K',			'K' },
	{ KEYCODE_L, 			DIK_L,				'L',			'L' },
	{ KEYCODE_COLON, 		DIK_SEMICOLON,		0xba,			';' },
	{ KEYCODE_QUOTE, 		DIK_APOSTROPHE,		0xde,			'\'' },
	{ KEYCODE_TILDE, 		DIK_GRAVE, 			0xc0,			'`' },
	{ KEYCODE_LSHIFT, 		DIK_LSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_BACKSLASH,	DIK_BACKSLASH, 		0xdc,			'\\' },
	{ KEYCODE_Z, 			DIK_Z,				'Z',			'Z' },
	{ KEYCODE_X, 			DIK_X,				'X',			'X' },
	{ KEYCODE_C, 			DIK_C,				'C',			'C' },
	{ KEYCODE_V, 			DIK_V,				'V',			'V' },
	{ KEYCODE_B, 			DIK_B,				'B',			'B' },
	{ KEYCODE_N, 			DIK_N,				'N',			'N' },
	{ KEYCODE_M, 			DIK_M,				'M',			'M' },
	{ KEYCODE_COMMA, 		DIK_COMMA,			0xbc,			',' },
	{ KEYCODE_STOP, 		DIK_PERIOD, 		0xbe,			'.' },
	{ KEYCODE_SLASH, 		DIK_SLASH, 			0xbf,			'/' },
	{ KEYCODE_RSHIFT, 		DIK_RSHIFT, 		VK_SHIFT, 		0 },
	{ KEYCODE_ASTERISK, 	DIK_MULTIPLY, 		VK_MULTIPLY,	'*' },
	{ KEYCODE_LALT, 		DIK_LMENU, 			VK_MENU, 		0 },
	{ KEYCODE_SPACE, 		DIK_SPACE, 			VK_SPACE,		' ' },
	{ KEYCODE_CAPSLOCK, 	DIK_CAPITAL, 		VK_CAPITAL, 	0 },
	{ KEYCODE_F1, 			DIK_F1,				VK_F1, 			0 },
	{ KEYCODE_F2, 			DIK_F2,				VK_F2, 			0 },
	{ KEYCODE_F3, 			DIK_F3,				VK_F3, 			0 },
	{ KEYCODE_F4, 			DIK_F4,				VK_F4, 			0 },
	{ KEYCODE_F5, 			DIK_F5,				VK_F5, 			0 },
	{ KEYCODE_F6, 			DIK_F6,				VK_F6, 			0 },
	{ KEYCODE_F7, 			DIK_F7,				VK_F7, 			0 },
	{ KEYCODE_F8, 			DIK_F8,				VK_F8, 			0 },
	{ KEYCODE_F9, 			DIK_F9,				VK_F9, 			0 },
	{ KEYCODE_F10, 			DIK_F10,			VK_F10, 		0 },
	{ KEYCODE_NUMLOCK, 		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ KEYCODE_SCRLOCK, 		DIK_SCROLL,			VK_SCROLL, 		0 },
	{ KEYCODE_7_PAD, 		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ KEYCODE_8_PAD, 		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ KEYCODE_9_PAD, 		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ KEYCODE_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT, 	0 },
	{ KEYCODE_4_PAD, 		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ KEYCODE_5_PAD, 		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ KEYCODE_6_PAD, 		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ KEYCODE_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ KEYCODE_1_PAD, 		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ KEYCODE_2_PAD, 		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ KEYCODE_3_PAD, 		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ KEYCODE_0_PAD, 		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ KEYCODE_DEL_PAD, 		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ KEYCODE_F11, 			DIK_F11,			VK_F11, 		0 },
	{ KEYCODE_F12, 			DIK_F12,			VK_F12, 		0 },
	{ KEYCODE_OTHER, 		DIK_F13,			VK_F13, 		0 },
	{ KEYCODE_OTHER, 		DIK_F14,			VK_F14, 		0 },
	{ KEYCODE_OTHER, 		DIK_F15,			VK_F15, 		0 },
	{ KEYCODE_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN, 		0 },
	{ KEYCODE_RCONTROL, 	DIK_RCONTROL,		VK_CONTROL, 	0 },
	{ KEYCODE_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE, 		0 },
	{ KEYCODE_PRTSCR, 		DIK_SYSRQ, 			0, 				0 },
	{ KEYCODE_RALT, 		DIK_RMENU,			VK_MENU, 		0 },
	{ KEYCODE_HOME, 		DIK_HOME,			VK_HOME, 		0 },
	{ KEYCODE_UP, 			DIK_UP,				VK_UP, 			0 },
	{ KEYCODE_PGUP, 		DIK_PRIOR,			VK_PRIOR, 		0 },
	{ KEYCODE_LEFT, 		DIK_LEFT,			VK_LEFT, 		0 },
	{ KEYCODE_RIGHT, 		DIK_RIGHT,			VK_RIGHT, 		0 },
	{ KEYCODE_END, 			DIK_END,			VK_END, 		0 },
	{ KEYCODE_DOWN, 		DIK_DOWN,			VK_DOWN, 		0 },
	{ KEYCODE_PGDN, 		DIK_NEXT,			VK_NEXT, 		0 },
	{ KEYCODE_INSERT, 		DIK_INSERT,			VK_INSERT, 		0 },
	{ KEYCODE_DEL, 			DIK_DELETE,			VK_DELETE, 		0 },
	{ KEYCODE_LWIN, 		DIK_LWIN,			VK_LWIN, 		0 },
	{ KEYCODE_RWIN, 		DIK_RWIN,			VK_RWIN, 		0 },
	{ KEYCODE_MENU, 		DIK_APPS,			VK_APPS, 		0 }
};



//============================================================
//	JOYSTICK LIST
//============================================================

// this will be filled in dynamically
static struct JoystickInfo joylist[MAX_JOY];

// macros for building/mapping keycodes
#define JOYCODE(joy, type, index)	((index) | ((type) << 8) | ((joy) << 12))
#define JOYINDEX(joycode)			((joycode) & 0xff)
#define JOYTYPE(joycode)			(((joycode) >> 8) & 0xf)
#define JOYNUM(joycode)				(((joycode) >> 12) & 0xf)

// joystick types
#define JOYTYPE_AXIS_NEG			0
#define JOYTYPE_AXIS_POS			1
#define JOYTYPE_POV_UP				2
#define JOYTYPE_POV_DOWN			3
#define JOYTYPE_POV_LEFT			4
#define JOYTYPE_POV_RIGHT			5
#define JOYTYPE_BUTTON				6
#define JOYTYPE_MOUSEBUTTON			7

// master translation table
static int joy_trans_table[][2] =
{
	// internal code					MAME code
	{ JOYCODE(0, JOYTYPE_AXIS_NEG, 0),	JOYCODE_1_LEFT },
	{ JOYCODE(0, JOYTYPE_AXIS_POS, 0),	JOYCODE_1_RIGHT },
	{ JOYCODE(0, JOYTYPE_AXIS_NEG, 1),	JOYCODE_1_UP },
	{ JOYCODE(0, JOYTYPE_AXIS_POS, 1),	JOYCODE_1_DOWN },
	{ JOYCODE(0, JOYTYPE_BUTTON, 0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 1),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 2),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 3),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 4),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(0, JOYTYPE_BUTTON, 5),	JOYCODE_1_BUTTON6 },

	{ JOYCODE(1, JOYTYPE_AXIS_NEG, 0),	JOYCODE_2_LEFT },
	{ JOYCODE(1, JOYTYPE_AXIS_POS, 0),	JOYCODE_2_RIGHT },
	{ JOYCODE(1, JOYTYPE_AXIS_NEG, 1),	JOYCODE_2_UP },
	{ JOYCODE(1, JOYTYPE_AXIS_POS, 1),	JOYCODE_2_DOWN },
	{ JOYCODE(1, JOYTYPE_BUTTON, 0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 1),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 2),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 3),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 4),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(1, JOYTYPE_BUTTON, 5),	JOYCODE_2_BUTTON6 },

	{ JOYCODE(2, JOYTYPE_AXIS_NEG, 0),	JOYCODE_3_LEFT },
	{ JOYCODE(2, JOYTYPE_AXIS_POS, 0),	JOYCODE_3_RIGHT },
	{ JOYCODE(2, JOYTYPE_AXIS_NEG, 1),	JOYCODE_3_UP },
	{ JOYCODE(2, JOYTYPE_AXIS_POS, 1),	JOYCODE_3_DOWN },
	{ JOYCODE(2, JOYTYPE_BUTTON, 0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 1),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 2),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 3),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 4),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(2, JOYTYPE_BUTTON, 5),	JOYCODE_3_BUTTON6 },

	{ JOYCODE(3, JOYTYPE_AXIS_NEG, 0),	JOYCODE_4_LEFT },
	{ JOYCODE(3, JOYTYPE_AXIS_POS, 0),	JOYCODE_4_RIGHT },
	{ JOYCODE(3, JOYTYPE_AXIS_NEG, 1),	JOYCODE_4_UP },
	{ JOYCODE(3, JOYTYPE_AXIS_POS, 1),	JOYCODE_4_DOWN },
	{ JOYCODE(3, JOYTYPE_BUTTON, 0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 1),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 2),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 3),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 4),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(3, JOYTYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 },

	{ JOYCODE(4, JOYTYPE_AXIS_NEG, 0),	JOYCODE_5_LEFT },
	{ JOYCODE(4, JOYTYPE_AXIS_POS, 0),	JOYCODE_5_RIGHT },
	{ JOYCODE(4, JOYTYPE_AXIS_NEG, 1),	JOYCODE_5_UP },
	{ JOYCODE(4, JOYTYPE_AXIS_POS, 1),	JOYCODE_5_DOWN },
	{ JOYCODE(4, JOYTYPE_BUTTON, 0),	JOYCODE_5_BUTTON1 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 1),	JOYCODE_5_BUTTON2 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 2),	JOYCODE_5_BUTTON3 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 3),	JOYCODE_5_BUTTON4 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 4),	JOYCODE_5_BUTTON5 },
	{ JOYCODE(4, JOYTYPE_BUTTON, 5),	JOYCODE_5_BUTTON6 },

	{ JOYCODE(5, JOYTYPE_AXIS_NEG, 0),	JOYCODE_6_LEFT },
	{ JOYCODE(5, JOYTYPE_AXIS_POS, 0),	JOYCODE_6_RIGHT },
	{ JOYCODE(5, JOYTYPE_AXIS_NEG, 1),	JOYCODE_6_UP },
	{ JOYCODE(5, JOYTYPE_AXIS_POS, 1),	JOYCODE_6_DOWN },
	{ JOYCODE(5, JOYTYPE_BUTTON, 0),	JOYCODE_6_BUTTON1 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 1),	JOYCODE_6_BUTTON2 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 2),	JOYCODE_6_BUTTON3 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 3),	JOYCODE_6_BUTTON4 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 4),	JOYCODE_6_BUTTON5 },
	{ JOYCODE(5, JOYTYPE_BUTTON, 5),	JOYCODE_6_BUTTON6 },

	{ JOYCODE(6, JOYTYPE_AXIS_NEG, 0),	JOYCODE_7_LEFT },
	{ JOYCODE(6, JOYTYPE_AXIS_POS, 0),	JOYCODE_7_RIGHT },
	{ JOYCODE(6, JOYTYPE_AXIS_NEG, 1),	JOYCODE_7_UP },
	{ JOYCODE(6, JOYTYPE_AXIS_POS, 1),	JOYCODE_7_DOWN },
	{ JOYCODE(6, JOYTYPE_BUTTON, 0),	JOYCODE_7_BUTTON1 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 1),	JOYCODE_7_BUTTON2 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 2),	JOYCODE_7_BUTTON3 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 3),	JOYCODE_7_BUTTON4 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 4),	JOYCODE_7_BUTTON5 },
	{ JOYCODE(6, JOYTYPE_BUTTON, 5),	JOYCODE_7_BUTTON6 },

	{ JOYCODE(7, JOYTYPE_AXIS_NEG, 0),	JOYCODE_8_LEFT },
	{ JOYCODE(7, JOYTYPE_AXIS_POS, 0),	JOYCODE_8_RIGHT },
	{ JOYCODE(7, JOYTYPE_AXIS_NEG, 1),	JOYCODE_8_UP },
	{ JOYCODE(7, JOYTYPE_AXIS_POS, 1),	JOYCODE_8_DOWN },
	{ JOYCODE(7, JOYTYPE_BUTTON, 0),	JOYCODE_8_BUTTON1 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 1),	JOYCODE_8_BUTTON2 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 2),	JOYCODE_8_BUTTON3 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 3),	JOYCODE_8_BUTTON4 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 4),	JOYCODE_8_BUTTON5 },
	{ JOYCODE(7, JOYTYPE_BUTTON, 5),	JOYCODE_8_BUTTON6 },

	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 0), 	JOYCODE_MOUSE_1_BUTTON1 },
	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 1), 	JOYCODE_MOUSE_1_BUTTON2 },
	{ JOYCODE(0, JOYTYPE_MOUSEBUTTON, 2), 	JOYCODE_MOUSE_1_BUTTON3 },
};



//============================================================
//	enum_keyboard_callback
//============================================================

static BOOL CALLBACK enum_keyboard_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	HRESULT result;

	// if we're not out of mice, log this one
	if (keyboard_count >= MAX_KEYBOARDS)
		goto out_of_keyboards;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &keyboard_device[keyboard_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(keyboard_device[keyboard_count], &IID_IDirectInputDevice2, (void **)&keyboard_device2[keyboard_count]);
	if (result != DI_OK)
		keyboard_device2[keyboard_count] = NULL;

	// get the caps
	keyboard_caps[keyboard_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(keyboard_device[keyboard_count], &keyboard_caps[keyboard_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(keyboard_device[keyboard_count], &c_dfDIKeyboard);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	result = IDirectInputDevice_SetCooperativeLevel(keyboard_device[keyboard_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	keyboard_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_get_caps:
	if (keyboard_device2[keyboard_count])
		IDirectInputDevice_Release(keyboard_device2[keyboard_count]);
	IDirectInputDevice_Release(keyboard_device[keyboard_count]);
cant_create_device:
out_of_keyboards:
	return DIENUM_CONTINUE;
}



//============================================================
//	enum_mouse_callback
//============================================================

static BOOL CALLBACK enum_mouse_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result;

	// if we're not out of mice, log this one
	if (mouse_count >= MAX_MICE)
		goto out_of_mice;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &mouse_device[mouse_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(mouse_device[mouse_count], &IID_IDirectInputDevice2, (void **)&mouse_device2[mouse_count]);
	if (result != DI_OK)
		mouse_device2[mouse_count] = NULL;

	// get the caps
	mouse_caps[mouse_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(mouse_device[mouse_count], &mouse_caps[mouse_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set relative mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_REL;
	result = IDirectInputDevice_SetProperty(mouse_device[mouse_count], DIPROP_AXISMODE, &value.diph);
	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(mouse_device[mouse_count], &c_dfDIMouse);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
	if (use_lightgun)
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	else
		result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], win_video_window,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);

	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	if (use_lightgun)
		lightgun_count++;
	mouse_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (mouse_device2[mouse_count])
		IDirectInputDevice_Release(mouse_device2[mouse_count]);
	IDirectInputDevice_Release(mouse_device[mouse_count]);
cant_create_device:
out_of_mice:
	return DIENUM_CONTINUE;
}



//============================================================
//	enum_joystick_callback
//============================================================

static BOOL CALLBACK enum_joystick_callback(LPCDIDEVICEINSTANCE instance, LPVOID ref)
{
	DIPROPDWORD value;
	HRESULT result = DI_OK;
	DWORD flags;

	// if we're not out of mice, log this one
	if (joystick_count >= MAX_JOYSTICKS)
		goto out_of_joysticks;

	// attempt to create a device
	result = IDirectInput_CreateDevice(dinput, &instance->guidInstance, &joystick_device[joystick_count], NULL);
	if (result != DI_OK)
		goto cant_create_device;

	// try to get a version 2 device for it
	result = IDirectInputDevice_QueryInterface(joystick_device[joystick_count], &IID_IDirectInputDevice2, (void **)&joystick_device2[joystick_count]);
	if (result != DI_OK)
		joystick_device2[joystick_count] = NULL;

	// get the caps
	joystick_caps[joystick_count].dwSize = STRUCTSIZE(DIDEVCAPS);
	result = IDirectInputDevice_GetCapabilities(joystick_device[joystick_count], &joystick_caps[joystick_count]);
	if (result != DI_OK)
		goto cant_get_caps;

	// set absolute mode
	value.diph.dwSize = sizeof(DIPROPDWORD);
	value.diph.dwHeaderSize = sizeof(value.diph);
	value.diph.dwObj = 0;
	value.diph.dwHow = DIPH_DEVICE;
	value.dwData = DIPROPAXISMODE_ABS;
	result = IDirectInputDevice_SetProperty(joystick_device[joystick_count], DIPROP_AXISMODE, &value.diph);
 	if (result != DI_OK)
		goto cant_set_axis_mode;

	// attempt to set the data format
	result = IDirectInputDevice_SetDataFormat(joystick_device[joystick_count], &c_dfDIJoystick);
	if (result != DI_OK)
		goto cant_set_format;

	// set the cooperative level
#if HAS_WINDOW_MENU
	flags = DISCL_BACKGROUND | DISCL_EXCLUSIVE;
#else
	flags = DISCL_FOREGROUND | DISCL_EXCLUSIVE;
#endif
	result = IDirectInputDevice_SetCooperativeLevel(joystick_device[joystick_count], win_video_window,
					flags);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	joystick_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	if (joystick_device2[joystick_count])
		IDirectInputDevice_Release(joystick_device2[joystick_count]);
	IDirectInputDevice_Release(joystick_device[joystick_count]);
cant_create_device:
out_of_joysticks:
	return DIENUM_CONTINUE;
}



//============================================================
//	win_init_input
//============================================================

int win_init_input(void)
{
	HRESULT result;

	// first attempt to initialize DirectInput
	dinput_version = DIRECTINPUT_VERSION;
	result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
	if (result != DI_OK)
	{
		dinput_version = 0x0300;
		result = DirectInputCreate(GetModuleHandle(NULL), dinput_version, &dinput, NULL);
		if (result != DI_OK)
			goto cant_create_dinput;
	}
	if (verbose)
		fprintf(stderr, "Using DirectInput %d\n", dinput_version >> 8);
#ifdef VPINMAME
	// initialize this global variable (for VPinMAME)
	win_trying_to_quit = 0;
#endif /* VPINMAME */
	// initialize keyboard devices
	keyboard_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_KEYBOARD, enum_keyboard_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_keyboard;

	// initialize mouse devices
	lightgun_count = 0;
	mouse_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_MOUSE, enum_mouse_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_mouse;

	// initialize joystick devices
	joystick_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_JOYSTICK, enum_joystick_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_joystick;

	// init the keyboard list
	init_keylist();

	// init the joystick list
	init_joylist();

	// print the results
	if (verbose)
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d Lightguns=%d\n", keyboard_count, mouse_count, joystick_count, lightgun_count);
	return 0;

cant_init_joystick:
cant_init_mouse:
cant_init_keyboard:
	IDirectInput_Release(dinput);
cant_create_dinput:
	dinput = NULL;
	return 1;
}



//============================================================
//	win_shutdown_input
//============================================================

void win_shutdown_input(void)
{
	int i;


	// release all our keyboards
	for (i = 0; i < keyboard_count; i++) {
		IDirectInputDevice_Release(keyboard_device[i]);
		if (keyboard_device2[i])
			IDirectInputDevice_Release(keyboard_device2[i]);
		keyboard_device2[i]=0;
	}

	// release all our joysticks
	for (i = 0; i < joystick_count; i++) {
		IDirectInputDevice_Release(joystick_device[i]);
		if (joystick_device2[i])
			IDirectInputDevice_Release(joystick_device2[i]);
		joystick_device2[i]=0;
	}

	// release all our mice
	for (i = 0; i < mouse_count; i++) {
		IDirectInputDevice_Release(mouse_device[i]);
		if (mouse_device2[i])
			IDirectInputDevice_Release(mouse_device2[i]);
		mouse_device2[i]=0;
	}

	// release DirectInput
	if (dinput)
		IDirectInput_Release(dinput);
	dinput = NULL;
}



//============================================================
//	win_pause_input
//============================================================

void win_pause_input(int paused)
{
	int i;

	// if paused, unacquire all devices
	if (paused)
	{
		// unacquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Unacquire(keyboard_device[i]);

		// unacquire all our mice
		for (i = 0; i < mouse_count; i++)
			IDirectInputDevice_Unacquire(mouse_device[i]);
	}

	// otherwise, reacquire all devices
	else
	{
		// acquire all keyboards
		for (i = 0; i < keyboard_count; i++)
			IDirectInputDevice_Acquire(keyboard_device[i]);

		// acquire all our mice if active
		if (mouse_active && !win_has_menu())
			for (i = 0; i < mouse_count && (use_mouse||use_lightgun); i++)
				IDirectInputDevice_Acquire(mouse_device[i]);
	}

	// set the paused state
	input_paused = paused;
	win_update_cursor_state();
}



//============================================================
//	win_poll_input
//============================================================

void win_poll_input(void)
{
	HWND focus = GetFocus();
	HRESULT result = 1;
	int i, j;

	// remember when this happened
	last_poll = osd_cycles();

	// periodically process events, in case they're not coming through
	win_process_events_periodic();

	// if we don't have focus, turn off all keys
	if (!focus)
	{
		memset(&keyboard_state[0][0], 0, sizeof(keyboard_state[i]));
		updatekeyboard();
		return;
	}

	// poll all keyboards
	for (i = 0; i < keyboard_count; i++)
	{
		// first poll the device
		if (keyboard_device2[i])
			IDirectInputDevice2_Poll(keyboard_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);

		// handle lost inputs here
		if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
		{
			result = IDirectInputDevice_Acquire(keyboard_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(keyboard_device[i], sizeof(keyboard_state[i]), &keyboard_state[i][0]);
		}

		// convert to 0 or 1
		if (result == DI_OK)
			for (j = 0; j < sizeof(keyboard_state[i]); j++)
				keyboard_state[i][j] >>= 7;
	}

	// if we couldn't poll the keyboard that way, poll it via GetAsyncKeyState
	if (result != DI_OK)
		for (i = 0; keylist[i].code; i++)
		{
			int dik = DICODE(keylist[i].code);
			int vk = VKCODE(keylist[i].code);

			// if we have a non-zero VK, query it
			if (vk)
				keyboard_state[0][dik] = (GetAsyncKeyState(vk) >> 15) & 1;
		}

	// update the lagged keyboard
	updatekeyboard();

	// if the debugger is up and visible, don't bother with the rest
	if (win_debug_window != NULL && IsWindowVisible(win_debug_window))
		return;

	// poll all joysticks
	for (i = 0; i < joystick_count && use_joystick; i++)
	{
		// first poll the device
		if (joystick_device2[i])
			IDirectInputDevice2_Poll(joystick_device2[i]);

		// get the state
		result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);

		// handle lost inputs here
		if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED)
		{
			result = IDirectInputDevice_Acquire(joystick_device[i]);
			if (result == DI_OK)
				result = IDirectInputDevice_GetDeviceState(joystick_device[i], sizeof(joystick_state[i]), &joystick_state[i]);
		}
	}

	// poll all our mice if active
	if (mouse_active && !win_has_menu())
		for (i = 0; i < mouse_count && (use_mouse||use_lightgun); i++)
		{
			// first poll the device
			if (mouse_device2[i])
				IDirectInputDevice2_Poll(mouse_device2[i]);

			// get the state
			result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);

			// handle lost inputs here
			if ((result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) && !input_paused)
			{
				result = IDirectInputDevice_Acquire(mouse_device[i]);
				if (result == DI_OK)
					result = IDirectInputDevice_GetDeviceState(mouse_device[i], sizeof(mouse_state[i]), &mouse_state[i]);
			}
		}
}



//============================================================
//	is_mouse_captured
//============================================================

int win_is_mouse_captured(void)
{
	return (!input_paused && mouse_active && mouse_count > 0 && use_mouse && !win_has_menu());
}



//============================================================
//	osd_get_key_list
//============================================================

const struct KeyboardInfo *osd_get_key_list(void)
{
	return keylist;
}



//============================================================
//	updatekeyboard
//============================================================

// since the keyboard controller is slow, it is not capable of reporting multiple
// key presses fast enough. We have to delay them in order not to lose special moves
// tied to simultaneous button presses.

static void updatekeyboard(void)
{
	int i, changed = 0;

	// see if any keys have changed state
	for (i = 0; i < MAX_KEYS; i++)
		if (keyboard_state[0][i] != oldkey[i])
		{
			changed = 1;

			// keypress was missed, turn it on for one frame
			if (keyboard_state[0][i] == 0 && currkey[i] == 0)
				currkey[i] = -1;
		}

	// if keyboard state is stable, copy it over
	if (!changed)
		memcpy(currkey, &keyboard_state[0][0], sizeof(currkey));

	// remember the previous state
	memcpy(oldkey, &keyboard_state[0][0], sizeof(oldkey));
}



//============================================================
//	osd_is_key_pressed
//============================================================

int osd_is_key_pressed(int keycode)
{
	int dik = DICODE(keycode);

	// make sure we've polled recently
	if (osd_cycles() > last_poll + osd_cycles_per_second()/4)
		win_poll_input();

	// special case: if we're trying to quit, fake up/down/up/down
	if (dik == DIK_ESCAPE && win_trying_to_quit)
	{
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	// if the video window isn't visible, we have to get our events from the console
	if (!win_video_window || !IsWindowVisible(win_video_window))
	{
		// warning: this code relies on the assumption that when you're polling for
		// keyboard events before the system is initialized, they are all of the
		// "press any key" to continue variety
		int result = _kbhit();
		if (result)
			_getch();
		return result;
	}

	// otherwise, just return the current keystate
	if (steadykey)
		return currkey[dik];
	else
		return keyboard_state[0][dik];
}



//============================================================
//	osd_readkey_unicode
//============================================================

int osd_readkey_unicode(int flush)
{
#if 0
	if (flush) clear_keybuf();
	if (keypressed())
		return ureadkey(NULL);
	else
		return 0;
#endif
	return 0;
}



//============================================================
//	init_joylist
//============================================================

static void init_keylist(void)
{
	int keycount = 0, key;
	struct ik *temp;

	// iterate over all possible keys
	for (key = 0; key < MAX_KEYS; key++)
	{
		DIDEVICEOBJECTINSTANCE instance = { 0 };
		HRESULT result;

		// attempt to get the object info
		instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
		result = IDirectInputDevice_GetObjectInfo(keyboard_device[0], &instance, key, DIPH_BYOFFSET);
		if (result == DI_OK)
		{
			// if it worked, assume we have a valid key

			// copy the name
			char *namecopy = malloc(strlen(instance.tszName) + 1);
			if (namecopy)
			{
				unsigned code, standardcode;
				int entry;

				// find the table entry, if there is one
				for (entry = 0; entry < ELEMENTS(key_trans_table); entry++)
					if (key_trans_table[entry][DI_KEY] == key)
						break;

				// compute the code, which encodes DirectInput, virtual, and ASCII codes
				code = KEYCODE(key, 0, 0);
				standardcode = KEYCODE_OTHER;
				if (entry < ELEMENTS(key_trans_table))
				{
					code = KEYCODE(key, key_trans_table[entry][VIRTUAL_KEY], key_trans_table[entry][ASCII_KEY]);
					standardcode = key_trans_table[entry][MAME_KEY];
				}

				// fill in the key description
				keylist[keycount].name = strcpy(namecopy, instance.tszName);
				keylist[keycount].code = code;
				keylist[keycount].standardcode = standardcode;
				keycount++;

				// make sure we have enough room for the new entry and the terminator (2 more)
				if ((num_osd_ik + 2) > size_osd_ik)
				{
					// attempt to allocate 16 more
					temp = realloc (osd_input_keywords, (size_osd_ik + 16)*sizeof (struct ik));

					// if the realloc was successful
					if (temp)
					{
						// point to the new buffer and increase the size indicator
						osd_input_keywords =  temp;
						size_osd_ik += 16;
					}
				}

				// if we have enough room for the new entry and the terminator
				if ((num_osd_ik + 2) <= size_osd_ik)
				{
					const char *src;
					char *dst;

					osd_input_keywords[num_osd_ik].name = malloc (strlen(instance.tszName) + 4 + 1);

					src = instance.tszName;
					dst = (char *)osd_input_keywords[num_osd_ik].name;

					strcpy (dst, "Key_");
					dst += strlen(dst);

					// copy name converting all spaces to underscores
					while (*src != 0)
					{
						if (*src == ' ')
							*dst++ = '_';
						else
							*dst++ = *src;
						src++;
					}
					*dst = 0;

					osd_input_keywords[num_osd_ik].type = IKT_OSD_KEY;
					osd_input_keywords[num_osd_ik].val = code;

					num_osd_ik++;

					// indicate end of list
					osd_input_keywords[num_osd_ik].name = 0;
				}
			}
		}
	}

	// terminate the list
	memset(&keylist[keycount], 0, sizeof(keylist[keycount]));
}



//============================================================
//	add_joylist_entry
//============================================================

static void add_joylist_entry(const char *name, int code, int *joycount)
{
	int standardcode = JOYCODE_OTHER;
 	struct ik *temp;

	// copy the name
	char *namecopy = malloc(strlen(name) + 1);
	if (namecopy)
	{
		int entry;

		// find the table entry, if there is one
		for (entry = 0; entry < ELEMENTS(joy_trans_table); entry++)
			if (joy_trans_table[entry][0] == code)
				break;

		// fill in the joy description
		joylist[*joycount].name = strcpy(namecopy, name);
		joylist[*joycount].code = code;
		if (entry < ELEMENTS(joy_trans_table))
			standardcode = joy_trans_table[entry][1];
		joylist[*joycount].standardcode = standardcode;
		*joycount += 1;

		// make sure we have enough room for the new entry and the terminator (2 more)
		if ((num_osd_ik + 2) > size_osd_ik)
		{
			// attempt to allocate 16 more
			temp = realloc (osd_input_keywords, (size_osd_ik + 16)*sizeof (struct ik));

			// if the realloc was successful
			if (temp)
			{
				// point to the new buffer and increase the size indicator
				osd_input_keywords =  temp;
				size_osd_ik += 16;
			}
		}

		// if we have enough room for the new entry and the terminator
		if ((num_osd_ik + 2) <= size_osd_ik)
		{
			const char *src;
			char *dst;

			osd_input_keywords[num_osd_ik].name = malloc (strlen(name) + 1);

			src = name;
			dst = (char *)osd_input_keywords[num_osd_ik].name;

			// copy name converting all spaces to underscores
			while (*src != 0)
			{
				if (*src == ' ')
					*dst++ = '_';
				else
					*dst++ = *src;
				src++;
			}
			*dst = 0;

			osd_input_keywords[num_osd_ik].type = IKT_OSD_JOY;
			osd_input_keywords[num_osd_ik].val = code;

			num_osd_ik++;

			// indicate end of list
			osd_input_keywords[num_osd_ik].name = 0;
		}
	}
}



//============================================================
//	init_joylist
//============================================================

static void init_joylist(void)
{
	int mouse, stick, axis, button, pov;
	char tempname[MAX_PATH];
	int joycount = 0;

	// first of all, map mouse buttons
	for (mouse = 0; mouse < mouse_count; mouse++)
		for (button = 0; button < 4; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(mouse_device[mouse], &instance, offsetof(DIMOUSESTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add mouse number to the name
				if (mouse_count > 1)
					sprintf(tempname, "Mouse %d %s", mouse + 1, instance.tszName);
				else
					sprintf(tempname, "Mouse %s", instance.tszName);
				add_joylist_entry(tempname, JOYCODE(mouse, JOYTYPE_MOUSEBUTTON, button), &joycount);
			}
		}

	// now map joysticks
	for (stick = 0; stick < joystick_count; stick++)
	{
		// loop over all axes
		for (axis = 0; axis < MAX_AXES; axis++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add negative value
				sprintf(tempname, "J%d %s -", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_AXIS_NEG, axis), &joycount);

				// add positive value
				sprintf(tempname, "J%d %s +", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_AXIS_POS, axis), &joycount);

				// get the axis range while we're here
				joystick_range[stick][axis].diph.dwSize = sizeof(DIPROPRANGE);
				joystick_range[stick][axis].diph.dwHeaderSize = sizeof(joystick_range[stick][axis].diph);
				joystick_range[stick][axis].diph.dwObj = offsetof(DIJOYSTATE, lX) + axis * sizeof(LONG);
				joystick_range[stick][axis].diph.dwHow = DIPH_BYOFFSET;
				result = IDirectInputDevice_GetProperty(joystick_device[stick], DIPROP_RANGE, &joystick_range[stick][axis].diph);
			}
		}

		// loop over all buttons
		for (button = 0; button < MAX_BUTTONS; button++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgbButtons[button]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// make the name for this item
				sprintf(tempname, "J%d %s", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_BUTTON, button), &joycount);
			}
		}

		// check POV hats
		for (pov = 0; pov < MAX_POV; pov++)
		{
			DIDEVICEOBJECTINSTANCE instance = { 0 };
			HRESULT result;

			// attempt to get the object info
			instance.dwSize = STRUCTSIZE(DIDEVICEOBJECTINSTANCE);
			result = IDirectInputDevice_GetObjectInfo(joystick_device[stick], &instance, offsetof(DIJOYSTATE, rgdwPOV[pov]), DIPH_BYOFFSET);
			if (result == DI_OK)
			{
				// add up direction
				sprintf(tempname, "J%d %s U", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_UP, pov), &joycount);

				// add down direction
				sprintf(tempname, "J%d %s D", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_DOWN, pov), &joycount);

				// add left direction
				sprintf(tempname, "J%d %s L", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_LEFT, pov), &joycount);

				// add right direction
				sprintf(tempname, "J%d %s R", stick + 1, instance.tszName);
				add_joylist_entry(tempname, JOYCODE(stick, JOYTYPE_POV_RIGHT, pov), &joycount);
			}
		}
	}

	// terminate array
	memset(&joylist[joycount], 0, sizeof(joylist[joycount]));
}



//============================================================
//	osd_get_joy_list
//============================================================

const struct JoystickInfo *osd_get_joy_list(void)
{
	return joylist;
}



//============================================================
//	osd_is_joy_pressed
//============================================================

int osd_is_joy_pressed(int joycode)
{
	int joyindex = JOYINDEX(joycode);
	int joytype = JOYTYPE(joycode);
	int joynum = JOYNUM(joycode);
	DWORD pov;

	// switch off the type
	switch (joytype)
	{
		case JOYTYPE_MOUSEBUTTON:
			/* ActLabs lightgun - remap button 2 (shot off-screen) as button 1 */
			if (use_lightgun && joynum==0) {
				if (joyindex==0 && (mouse_state[0].rgbButtons[1]&0x80))
					return 1;
				if (joyindex==1 && (mouse_state[0].rgbButtons[1]&0x80))
					return 0;
			}
			return mouse_state[joynum].rgbButtons[joyindex] >> 7;

		case JOYTYPE_BUTTON:
			return joystick_state[joynum].rgbButtons[joyindex] >> 7;

		case JOYTYPE_AXIS_POS:
		case JOYTYPE_AXIS_NEG:
		{
			LONG val = ((LONG *)&joystick_state[joynum].lX)[joyindex];
			LONG top = joystick_range[joynum][joyindex].lMax;
			LONG bottom = joystick_range[joynum][joyindex].lMin;
			LONG middle = (top + bottom) / 2;

			// watch for movement greater "a2d_deadzone" along either axis
			// FIXME in the two-axis joystick case, we need to find out
			// the angle. Anything else is unprecise.
			if (joytype == JOYTYPE_AXIS_POS)
				return (val > middle + ((top - middle) * a2d_deadzone));
			else
				return (val < middle - ((middle - bottom) * a2d_deadzone));
		}

		// anywhere from 0-45 (315) deg to 0+45 (45) deg
		case JOYTYPE_POV_UP:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 31500 || pov <= 4500));

		// anywhere from 90-45 (45) deg to 90+45 (135) deg
		case JOYTYPE_POV_RIGHT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 4500 && pov <= 13500));

		// anywhere from 180-45 (135) deg to 180+45 (225) deg
		case JOYTYPE_POV_DOWN:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 13500 && pov <= 22500));

		// anywhere from 270-45 (225) deg to 270+45 (315) deg
		case JOYTYPE_POV_LEFT:
			pov = joystick_state[joynum].rgdwPOV[joyindex];
			return ((pov & 0xffff) != 0xffff && (pov >= 22500 && pov <= 31500));

	}

	// keep the compiler happy
	return 0;
}



//============================================================
//	osd_analogjoy_read
//============================================================

void osd_analogjoy_read(int player, int analog_axis[], InputCode analogjoy_input[])
{
	LONG top, bottom, middle;
	int i;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && use_mouse && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	for (i=0; i<MAX_ANALOG_AXES; i++)
	{
		int joyindex, joytype, joynum;

		analog_axis[i] = 0;

		if (analogjoy_input[i] == CODE_NONE || !use_joystick)
			continue;

		joyindex = JOYINDEX( analogjoy_input[i] );
		joytype = JOYTYPE( analogjoy_input[i] );
		joynum = JOYNUM( analogjoy_input[i] );

		top = joystick_range[joynum][joyindex].lMax;
		bottom = joystick_range[joynum][joyindex].lMin;
		middle = (top + bottom) / 2;
		analog_axis[i] = (((LONG *)&joystick_state[joynum].lX)[joyindex] - middle) * 257 / (top - bottom);
		if (analog_axis[i] < -128) analog_axis[i] = -128;
		if (analog_axis[i] >  128) analog_axis[i] =  128;
		if (joytype == JOYTYPE_AXIS_POS)
			analog_axis[i] = -analog_axis[i];
	}
}


int osd_is_joystick_axis_code(int joycode)
{
	switch (JOYTYPE( joycode ))
	{
		case JOYTYPE_AXIS_POS:
		case JOYTYPE_AXIS_NEG:
			return 1;
		default:
			return 0;
	}
	return 0;
}


//============================================================
//	osd_lightgun_read
//============================================================

void osd_lightgun_read(int player,int *deltax,int *deltay)
{
	POINT point;

	// if the mouse isn't yet active, make it so
	if (!mouse_active && (use_mouse||use_lightgun) && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// if out of range, skip it
	if (!use_lightgun || !win_physical_width || !win_physical_height || player >= lightgun_count)
	{
		*deltax = *deltay = 0;
		return;
	}

	// Warning message to users - design wise this probably isn't the best function to put this in...
	if (win_window_mode)
		usrintf_showmessage("Lightgun not supported in windowed mode");

	// Hack - if button 2 is pressed on lightgun, then return 0,0 (off-screen) to simulate reload
	if (mouse_state[0].rgbButtons[1]&0x80) {
		*deltax = -128;
		*deltay = -128;
		return;
	}

	// I would much prefer to use DirectInput to read the gun values but there seem to be
	// some problems...  DirectInput (8.0 tested) on Win98 returns garbage for both buffered
	// and immediate, absolute and relative axis modes.  Win2k (DX 8.1) returns good data
	// for buffered absolute reads, but WinXP (8.1) returns garbage on all modes.  DX9 betas
	// seem to exhibit the same behaviour.  I have no idea of the cause of this, the only
	// consistent way to read the location seems to be the Windows system call GetCursorPos
	// which requires the application have non-exclusive access to the mouse device
	//
	GetCursorPos(&point);

	// Map absolute pixel values into -128 -> 128 range
	*deltax = (point.x * 256 + win_physical_width/2) / (win_physical_width-1) - 128;
	*deltay = (point.y * 256 + win_physical_height/2) / (win_physical_height-1) - 128;

	if (*deltax < -128) *deltax = -128;
	if (*deltax > 128) *deltax = 128;
	if (*deltay < -128) *deltay = -128;
	if (*deltay > 128) *deltay = 128;
}

//============================================================
//	osd_trak_read
//============================================================

void osd_trak_read(int player, int *deltax, int *deltay)
{
	// if the mouse isn't yet active, make it so
	if (!mouse_active && use_mouse && !win_has_menu())
	{
		mouse_active = 1;
		win_pause_input(0);
	}

	// return the latest mouse info
	*deltax = mouse_state[player].lX;
	*deltay = mouse_state[player].lY;
}



//============================================================
//	osd_joystick_needs_calibration
//============================================================

int osd_joystick_needs_calibration(void)
{
	return 0;
}



//============================================================
//	osd_joystick_start_calibration
//============================================================

void osd_joystick_start_calibration(void)
{
}



//============================================================
//	osd_joystick_calibrate_next
//============================================================

const char *osd_joystick_calibrate_next(void)
{
	return 0;
}



//============================================================
//	osd_joystick_calibrate
//============================================================

void osd_joystick_calibrate(void)
{
}



//============================================================
//	osd_joystick_end_calibration
//============================================================

void osd_joystick_end_calibration(void)
{
}



//============================================================
//	osd_customize_inputport_defaults
//============================================================

extern struct rc_struct *rc;

void process_ctrlr_file(struct rc_struct *iptrc, const char *ctype, const char *filename)
{
	mame_file *f;

	// open the specified controller type/filename
	f = mame_fopen (ctype, filename, FILETYPE_CTRLR, 0);

	if (f)
	{
		if (verbose)
		{
			if (ctype)
				fprintf (stderr, "trying to parse ctrlr file %s/%s.ini\n", ctype, filename);
			else
				fprintf (stderr, "trying to parse ctrlr file %s.ini\n", filename);
		}

		// process this file
		if(osd_rc_read(iptrc, f, filename, 1, 1))
		{
			if (verbose)
			{
				if (ctype)
					fprintf (stderr, "problem parsing ctrlr file %s/%s.ini\n", ctype, filename);
				else
					fprintf (stderr, "problem parsing ctrlr file %s.ini\n", filename);
			}
		}
	}

	// close the file
	if (f)
		mame_fclose (f);
}

void process_ctrlr_game(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
{
	// recursive call to process parents first
	if (drv->clone_of)
		process_ctrlr_game (iptrc, ctype, drv->clone_of);

	// now process this game
	if (drv->name && *(drv->name) != 0)
		process_ctrlr_file (iptrc, ctype, drv->name);
}

// nice hack: load source_file.ini (omit if referenced later any)
void process_ctrlr_system(struct rc_struct *iptrc, const char *ctype, const struct GameDriver *drv)
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

	// not referenced later, so load it here
	if (tmp_gd == NULL)
		// now process this system
		process_ctrlr_file (iptrc, ctype, buffer);
}

static int ipdef_custom_rc_func(struct rc_option *option, const char *arg, int priority)
{
	struct ik *pinput_keywords = (struct ik *)option->dest;
	struct ipd *idef = ipddef_ptr;

	// only process the default definitions if the input port definitions
	// pointer has been defined
	if (idef)
	{
		// if a keycode was re-assigned
		if (pinput_keywords->type == IKT_STD)
		{
			InputSeq is;

			// get the new keycode
			seq_set_string (&is, arg);

			// was a sequence was assigned to a keycode? - not valid!
			if (is[1] != CODE_NONE)
			{
				fprintf(stderr, "error: can't map \"%s\" to \"%s\"\n",pinput_keywords->name,arg);
			}

			// for all definitions
			while (idef->type != IPT_END)
			{
				int j;

				// reassign all matching keystrokes to the given argument
				for (j = 0; j < SEQ_MAX; j++)
				{
					// if the keystroke matches
					if (idef->seq[j] == pinput_keywords->val)
					{
						// re-assign
						idef->seq[j] = is[0];
					}
				}
				// move to the next definition
				idef++;
			}
		}

		// if an input definition was re-defined
		else if (pinput_keywords->type == IKT_IPT ||
                 pinput_keywords->type == IKT_IPT_EXT)
		{
			// loop through all definitions
			while (idef->type != IPT_END)
			{
				// if the definition matches
				if (idef->type == pinput_keywords->val)
				{
                    if (pinput_keywords->type == IKT_IPT_EXT)
                        idef++;
					seq_set_string(&idef->seq, arg);
					// and abort (there shouldn't be duplicate definitions)
					break;
				}

				// move to the next definition
				idef++;
			}
		}
	}

	return 0;
}


void osd_customize_inputport_defaults(struct ipd *defaults)
{
	static InputSeq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);
	UINT32 next_reserved = IPT_OSD_1;
	struct ipd *idef = defaults;
	int i;

	// loop over all the defaults
	while (idef->type != IPT_END)
	{
		// if the type is OSD reserved
		if (idef->type == IPT_OSD_RESERVED)
		{
			// process the next reserved entry
			switch (next_reserved)
			{
				// OSD_1 is alt-enter for fullscreen
				case IPT_OSD_1:
					idef->type = next_reserved;
					idef->name = "Toggle fullscreen";
					seq_set_2 (&idef->seq, KEYCODE_LALT, KEYCODE_ENTER);
				break;

#ifdef MESS
				case IPT_OSD_2:
					if (options.disable_normal_ui)
					{
						idef->type = next_reserved;
						idef->name = "Toggle menubar";
						seq_set_1 (&idef->seq, KEYCODE_SCRLOCK);
					}
				break;
#endif /* MESS */

				default:
				break;
			}
			next_reserved++;
		}

		// disable the config menu if the ALT key is down
		// (allows ALT-TAB to switch between windows apps)
		if (idef->type == IPT_UI_CONFIGURE)
		{
			seq_copy(&idef->seq, &no_alt_tab_seq);
		}

#ifdef MESS
		if (idef->type == IPT_UI_THROTTLE)
		{
			static InputSeq empty_seq = SEQ_DEF_0;
			seq_copy(&idef->seq, &empty_seq);
		}
#endif /* MESS */

		// find the next one
		idef++;
	}

#if 0
	// if a controller type hasn't been specified
	if (ctrlrtype == NULL || *ctrlrtype == 0 || (_stricmp(ctrlrtype,"Standard") == 0))
	{
		// default to the legacy controller types if selected
		if (hotrod)
			ctrlrtype = "hotrod";

		if (hotrodse)
			ctrlrtype = "hotrodse";
	}
#endif

	// create a structure for the input port options
	if (!(ctrlr_input_opts = calloc (num_ik+num_osd_ik+1, sizeof(struct rc_option))))
	{
		fprintf(stderr, "error on ctrlr_input_opts creation\n");
		exit(1);
	}

	// Populate the structure with the input_keywords.
	// For all, use the ipdef_custom_rc_func callback.
	// Also, reference the original ik structure.
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

	// add an end-of-opts indicator
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

	// set a static variable for the ipdef_custom_rc_func callback
	ipddef_ptr = defaults;

	// process the main platform-specific default file
	process_ctrlr_file (rc, NULL, "windows");

	// if a custom controller has been selected
	if (ctrlrtype && *ctrlrtype != 0 && (_stricmp(ctrlrtype,"Standard") != 0))
	{
		const struct InputPortTiny* input = Machine->gamedrv->input_ports;
		int paddle = 0, dial = 0, trackball = 0, adstick = 0, pedal = 0, lightgun = 0;

		// process the controller-specific default file
		process_ctrlr_file (rc, ctrlrtype, "default");

		// process the system-specific files for this controller
		process_ctrlr_system (rc, ctrlrtype, Machine->gamedrv);

		// process the game-specific files for this controller
		process_ctrlr_game (rc, ctrlrtype, Machine->gamedrv);


		while ((input->type & ~IPF_MASK) != IPT_END)
		{
			switch (input->type & ~IPF_MASK)
			{
				case IPT_PADDLE:
				case IPT_PADDLE_V:
					if (!paddle)
					{
						if ((paddle_ini != NULL) && (*paddle_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, paddle_ini);
						paddle = 1;
					}
					break;

				case IPT_DIAL:
				case IPT_DIAL_V:
					if (!dial)
					{
						if ((dial_ini != NULL) && (*dial_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, dial_ini);
						dial = 1;
					}
					break;

				case IPT_TRACKBALL_X:
				case IPT_TRACKBALL_Y:
					if (!trackball)
					{
						if ((trackball_ini != NULL) && (*trackball_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, trackball_ini);
						trackball = 1;
					}
					break;

				case IPT_AD_STICK_X:
				case IPT_AD_STICK_Y:
					if (!adstick)
					{
						if ((ad_stick_ini != NULL) && (*ad_stick_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, ad_stick_ini);
						adstick = 1;
					}
					break;

				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					if (!lightgun)
					{
						if ((lightgun_ini != NULL) && (*lightgun_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, lightgun_ini);
						lightgun = 1;
					}
					break;

				case IPT_PEDAL:
					if (!pedal)
					{
						if ((pedal_ini != NULL) && (*pedal_ini != 0))
							process_ctrlr_file (rc, ctrlrtype, pedal_ini);
						pedal = 1;
					}
					break;

			}
			++input;
		}
	}

	// print the results
	if (verbose)
	{
		if (ctrlrname)
			fprintf (stderr,"\"%s\" controller support enabled\n",ctrlrname);

		fprintf(stderr, "Mouse support %sabled\n",use_mouse ? "en" : "dis");
		fprintf(stderr, "Joystick support %sabled\n",use_joystick ? "en" : "dis");
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d\n",
			keyboard_count,
			use_mouse ? mouse_count : 0,
			use_joystick ? joystick_count : 0);
	}
}



//============================================================
//	osd_get_leds
//============================================================

int osd_get_leds(void)
{
	int result = 0;

	if (!use_keyboard_leds)
		return 0;

	// if we're on Win9x, use GetKeyboardState
	if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// set the numlock bit
		result |= (key_states[VK_NUMLOCK] & 1);
		result |= (key_states[VK_CAPITAL] & 1) << 1;
		result |= (key_states[VK_SCROLL] & 1) << 2;
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;	  // Output buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer

		// Address first keyboard
		OutputBuffer.UnitId = 0;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_QUERY_INDICATORS,
						NULL, 0,
						&OutputBuffer, DataLength,
						&ReturnedLength, NULL);

		// Demangle lights to match 95/98
		if (OutputBuffer.LedFlags & KEYBOARD_NUM_LOCK_ON) result |= 0x1;
		if (OutputBuffer.LedFlags & KEYBOARD_CAPS_LOCK_ON) result |= 0x2;
		if (OutputBuffer.LedFlags & KEYBOARD_SCROLL_LOCK_ON) result |= 0x4;
	}

	return result;
}



//============================================================
//	osd_set_leds
//============================================================

void osd_set_leds(int state)
{
	if (!use_keyboard_leds)
		return;

	// if we're on Win9x, use SetKeyboardState
	if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		// thanks to Lee Taylor for the original version of this code
		BYTE key_states[256];

		// get the current state
		GetKeyboardState(&key_states[0]);

		// mask states and set new states
		key_states[VK_NUMLOCK] = (key_states[VK_NUMLOCK] & ~1) | ((state >> 0) & 1);
		key_states[VK_CAPITAL] = (key_states[VK_NUMLOCK] & ~1) | ((state >> 1) & 1);
		key_states[VK_SCROLL] = (key_states[VK_NUMLOCK] & ~1) | ((state >> 2) & 1);

		SetKeyboardState(&key_states[0]);
	}
	else // WinNT/2K/XP, use DeviceIoControl
	{
		KEYBOARD_INDICATOR_PARAMETERS InputBuffer;	  // Input buffer for DeviceIoControl
		ULONG				DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
		ULONG				ReturnedLength; // Number of bytes returned in output buffer
		UINT				LedFlags=0;

		// Demangle lights to match 95/98
		if (state & 0x1) LedFlags |= KEYBOARD_NUM_LOCK_ON;
		if (state & 0x2) LedFlags |= KEYBOARD_CAPS_LOCK_ON;
		if (state & 0x4) LedFlags |= KEYBOARD_SCROLL_LOCK_ON;

		// Address first keyboard
		InputBuffer.UnitId = 0;
		InputBuffer.LedFlags = LedFlags;

		DeviceIoControl(hKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
						&InputBuffer, DataLength,
						NULL, 0,
						&ReturnedLength, NULL);
	}

	return;
}



//============================================================
//	start_led
//============================================================

void start_led(void)
{
	if (!use_keyboard_leds)
		return;

	// retrive windows version
	GetVersionEx(&osinfo);

	// nt/2k/xp
	if (!(osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
	{
		int error_number;

		if (!DefineDosDevice (DDD_RAW_TARGET_PATH, "Kbd",
					"\\Device\\KeyboardClass0"))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}

		hKbdDev = CreateFile("\\\\.\\Kbd", GENERIC_WRITE, 0,
					NULL,	OPEN_EXISTING,	0,	NULL);

		if (hKbdDev == INVALID_HANDLE_VALUE)
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to open the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	// remember the initial LED states
	original_leds = osd_get_leds();

	return;
}



//============================================================
//	stop_led
//============================================================

void stop_led(void)
{
	int error_number = 0;

	if (!use_keyboard_leds)
		return;

	// restore the initial LED states
	osd_set_leds(original_leds);

	// nt/2k/xp
	if (!(osinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
	{
		if (!DefineDosDevice (DDD_REMOVE_DEFINITION, "Kbd", NULL))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}

		if (!CloseHandle(hKbdDev))
		{
			error_number = GetLastError();
			fprintf(stderr, "Unable to close the keyboard device. (error %d)\n", error_number);
			return;
		}
	}

	return;
}
