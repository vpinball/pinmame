//============================================================
//
//	input.c - Win32 implementation of MAME input routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>

// undef WINNT for dinput.h to prevent duplicate definition
#undef WINNT
#include <dinput.h>

// MAME headers
#include "driver.h"
#include "window.h"
#include "rc.h"
#include "ticker.h"



//============================================================
//	IMPORTS
//============================================================

extern int verbose;



//============================================================
//	PARAMETERS
//============================================================

#define MAX_KEYBOARDS		1
#define MAX_MICE			4
#define MAX_JOYSTICKS		4

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

UINT8						trying_to_quit;



//============================================================
//	LOCAL VARIABLES
//============================================================

// DirectInput variables
static LPDIRECTINPUT		dinput;
static int					dinput_version;

// global states
static int					input_paused;
static TICKER				last_poll;

// HotRod override options
static int					hotrod;
static int					hotrodse;
static int					use_mouse;
static int					use_joystick;

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

// joystick states
static int					joystick_count;
static LPDIRECTINPUTDEVICE	joystick_device[MAX_JOYSTICKS];
static LPDIRECTINPUTDEVICE2	joystick_device2[MAX_JOYSTICKS];
static DIDEVCAPS			joystick_caps[MAX_JOYSTICKS];
static DIJOYSTATE			joystick_state[MAX_JOYSTICKS];
static DIPROPRANGE			joystick_range[MAX_JOYSTICKS][MAX_AXES];



//============================================================
//	OPTIONS
//============================================================

// global input options
struct rc_option input_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Input device options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "hotrod", NULL, rc_bool, &hotrod, "0", 0, 0, NULL, "preconfigure for hotrod" },
	{ "hotrodse", NULL, rc_bool, &hotrodse, "0", 0, 0, NULL, "preconfigure for hotrod se" },
	{ "mouse", NULL, rc_bool, &use_mouse, "0", 0, 0, NULL, "enable mouse input" },
	{ "joystick", "joy", rc_bool, &use_joystick, "0", 0, 0, NULL, "enable joystick input" },
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
	{ JOYCODE(3, JOYTYPE_BUTTON, 5),	JOYCODE_4_BUTTON6 }
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
	result = IDirectInputDevice_SetCooperativeLevel(keyboard_device[keyboard_count], video_window,
					DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	keyboard_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_get_caps:
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
	result = IDirectInputDevice_SetCooperativeLevel(mouse_device[mouse_count], video_window,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	mouse_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
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
	result = IDirectInputDevice_SetCooperativeLevel(joystick_device[joystick_count], video_window,
					DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (result != DI_OK)
		goto cant_set_coop_level;

	// increment the count
	joystick_count++;
	return DIENUM_CONTINUE;

cant_set_coop_level:
cant_set_format:
cant_set_axis_mode:
cant_get_caps:
	IDirectInputDevice_Release(joystick_device[joystick_count]);
cant_create_device:
out_of_joysticks:
	return DIENUM_CONTINUE;
}



//============================================================
//	win32_init_input
//============================================================

int win32_init_input(void)
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

	// initialize keyboard devices
	keyboard_count = 0;
	result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_KEYBOARD, enum_keyboard_callback, 0, DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
		goto cant_init_keyboard;

	// initialize mouse devices
	mouse_count = 0;
	if (use_mouse)
	{
		result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_MOUSE, enum_mouse_callback, 0, DIEDFL_ATTACHEDONLY);
		if (result != DI_OK)
			goto cant_init_mouse;
	}

	// initialize joystick devices
	joystick_count = 0;
	if (use_joystick)
	{
		result = IDirectInput_EnumDevices(dinput, DIDEVTYPE_JOYSTICK, enum_joystick_callback, 0, DIEDFL_ATTACHEDONLY);
		if (result != DI_OK)
			goto cant_init_joystick;
	}

	// init the keyboard list
	init_keylist();

	// init the joystick list
	init_joylist();

	// print the results
	if (verbose)
		fprintf(stderr, "Keyboards=%d  Mice=%d  Joysticks=%d\n", keyboard_count, mouse_count, joystick_count);
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
//	win32_shutdown_input
//============================================================

void win32_shutdown_input(void)
{
	int i;

	// release all our keyboards
	for (i = 0; i < keyboard_count; i++)
		IDirectInputDevice_Release(keyboard_device[i]);

	// release all our joysticks
	for (i = 0; i < joystick_count; i++)
		IDirectInputDevice_Release(joystick_device[i]);

	// release all our mice
	for (i = 0; i < mouse_count; i++)
		IDirectInputDevice_Release(mouse_device[i]);

	// release DirectInput
	if (dinput)
		IDirectInput_Release(dinput);
	dinput = NULL;
}



//============================================================
//	win32_pause_input
//============================================================

void win32_pause_input(int paused)
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
		if (mouse_active)
			for (i = 0; i < mouse_count; i++)
				IDirectInputDevice_Acquire(mouse_device[i]);
	}

	// set the paused state
	input_paused = paused;
	update_cursor_state();
}



//============================================================
//	win32_poll_input
//============================================================

void win32_poll_input(void)
{
	HWND focus = GetFocus();
	HRESULT result = 1;
	int i, j;

	// remember when this happened
	last_poll = ticker();

	// periodically process events, in case they're not coming through
	process_events_periodic();

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
	if (debug_window != NULL && IsWindowVisible(debug_window))
		return;

	// poll all joysticks
	for (i = 0; i < joystick_count; i++)
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
	if (mouse_active)
		for (i = 0; i < mouse_count; i++)
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

int is_mouse_captured(void)
{
	return (!input_paused && mouse_active && mouse_count > 0);
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
	if (ticker() > last_poll + TICKS_PER_SEC/4)
		win32_poll_input();

	// special case: if we're trying to quit, fake up/down/up/down
	if (dik == DIK_ESCAPE && trying_to_quit)
	{
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	// if the video window isn't visible, we have to get our events from the console
	if (!video_window || !IsWindowVisible(video_window))
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
	return currkey[dik];
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
		joylist[*joycount].standardcode = JOYCODE_OTHER;
		if (entry < ELEMENTS(joy_trans_table))
			joylist[*joycount].standardcode = joy_trans_table[entry][1];
		*joycount += 1;
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

			// watch for movement 1/4 of the way along either axis
			if (joytype == JOYTYPE_AXIS_POS)
				return (val > middle + (top - middle) / 4);
			else
				return (val < middle - (middle - bottom) / 4);
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

void osd_analogjoy_read(int player, int *analog_x, int *analog_y)
{
	LONG top, bottom, middle;

	// if the mouse isn't yet active, make it so
	if (!mouse_active)
	{
		mouse_active = 1;
		win32_pause_input(0);
	}

	// if out of range, skip it
	if (player >= joystick_count)
	{
		*analog_x = *analog_y = 0;
		return;
	}

	// return the X axis value
	top = joystick_range[player][0].lMax;
	bottom = joystick_range[player][0].lMin;
	middle = (top + bottom) / 2;
	*analog_x = (joystick_state[player].lX - middle) * 257 / (top - bottom);
	if (*analog_x < -128) *analog_x = -128;
	if (*analog_x >  128) *analog_x =  128;

	// return the Y axis value
	top = joystick_range[player][1].lMax;
	bottom = joystick_range[player][1].lMin;
	middle = (top + bottom) / 2;
	*analog_y = (joystick_state[player].lY - middle) * 257 / (top - bottom);
	if (*analog_y < -128) *analog_y = -128;
	if (*analog_y >  128) *analog_y =  128;
}



//============================================================
//	osd_trak_read
//============================================================

void osd_trak_read(int player, int *deltax, int *deltay)
{
	// if the mouse isn't yet active, make it so
	if (!mouse_active)
	{
		mouse_active = 1;
		win32_pause_input(0);
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

void osd_customize_inputport_defaults(struct ipd *defaults)
{
	static InputSeq no_alt_tab_seq = SEQ_DEF_5(KEYCODE_TAB, CODE_NOT, KEYCODE_LALT, CODE_NOT, KEYCODE_RALT);

	// loop over all the defaults
	while (defaults->type != IPT_END)
	{
		// in all cases, disable the config menu if the ALT key is down
		if (defaults->type == IPT_UI_CONFIGURE)
			seq_copy(&defaults->seq, &no_alt_tab_seq);

		// if we're mapping the hotrod, handle that
		if (hotrod || hotrodse)
		{
			int j;

			// map up/down/left/right to the numpad
			for (j = 0; j < SEQ_MAX; j++)
			{
				if (defaults->seq[j] == KEYCODE_UP) defaults->seq[j] = KEYCODE_8_PAD;
				if (defaults->seq[j] == KEYCODE_DOWN) defaults->seq[j] = KEYCODE_2_PAD;
				if (defaults->seq[j] == KEYCODE_LEFT) defaults->seq[j] = KEYCODE_4_PAD;
				if (defaults->seq[j] == KEYCODE_RIGHT) defaults->seq[j] = KEYCODE_6_PAD;
			}

			// UI select is button 1
			if (defaults->type == IPT_UI_SELECT) seq_set_1(&defaults->seq, KEYCODE_LCONTROL);

			// map to the old start/coinage
			if (defaults->type == IPT_START1) seq_set_1(&defaults->seq, KEYCODE_1);
			if (defaults->type == IPT_START2) seq_set_1(&defaults->seq, KEYCODE_2);
			if (defaults->type == IPT_COIN1)  seq_set_1(&defaults->seq, KEYCODE_3);
			if (defaults->type == IPT_COIN2)  seq_set_1(&defaults->seq, KEYCODE_4);

			// map left/right joysticks to the player1/2 joysticks
			if (defaults->type == (IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_R);
			if (defaults->type == (IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_F);
			if (defaults->type == (IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_D);
			if (defaults->type == (IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_G);
			if (defaults->type == (IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_8_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_2_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_4_PAD);
			if (defaults->type == (IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_6_PAD);

			// make sure the buttons are mapped like the hotrod expects
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_LALT, CODE_OR, JOYCODE_1_BUTTON2);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_SPACE, CODE_OR, JOYCODE_1_BUTTON3);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_LSHIFT, CODE_OR, JOYCODE_1_BUTTON4);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_Z, CODE_OR, JOYCODE_1_BUTTON5);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_X, CODE_OR, JOYCODE_1_BUTTON6);
			if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON1);
			if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_S, CODE_OR, JOYCODE_2_BUTTON2);
			if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_Q, CODE_OR, JOYCODE_2_BUTTON3);
			if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_W, CODE_OR, JOYCODE_2_BUTTON4);
			if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_E, CODE_OR, JOYCODE_2_BUTTON5);
			if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_OPENBRACE, CODE_OR, JOYCODE_2_BUTTON6);

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
			{
				extern struct GameDriver driver_neogeo;

				// if hotrodse is specified, and this is a neogeo game, work some more magic I don't understand
				if (hotrodse &&
						(Machine->gamedrv->clone_of == &driver_neogeo ||
						(Machine->gamedrv->clone_of && Machine->gamedrv->clone_of->clone_of == &driver_neogeo)))
				{
					if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_C, CODE_OR, JOYCODE_1_BUTTON1);
					if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_LSHIFT, CODE_OR, JOYCODE_1_BUTTON2);
					if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_Z, CODE_OR, JOYCODE_1_BUTTON3);
					if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER1)) seq_set_3(&defaults->seq, KEYCODE_X, CODE_OR, JOYCODE_1_BUTTON4);
					if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER1)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON1 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_CLOSEBRACE, CODE_OR, JOYCODE_2_BUTTON1);
					if (defaults->type == (IPT_BUTTON2 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_W, CODE_OR, JOYCODE_2_BUTTON2);
					if (defaults->type == (IPT_BUTTON3 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_E, CODE_OR, JOYCODE_2_BUTTON3);
					if (defaults->type == (IPT_BUTTON4 | IPF_PLAYER2)) seq_set_3(&defaults->seq, KEYCODE_OPENBRACE, CODE_OR, JOYCODE_2_BUTTON4);
					if (defaults->type == (IPT_BUTTON5 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON6 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON7 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
					if (defaults->type == (IPT_BUTTON8 | IPF_PLAYER2)) seq_set_1(&defaults->seq, KEYCODE_NONE);
				}
			}
#endif
#endif
#endif
		}

		// find the next one
		defaults++;
	}
}
