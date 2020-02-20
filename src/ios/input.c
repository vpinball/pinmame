#include "driver.h"
#include "window.h"
#include "input.h"

int ipinmame_get_keycode_status(int keycode);

static struct KeyboardInfo keylist[] = {
	{ "ESC",		KEY_ESC,	       KEYCODE_ESC },
    { "F1",         KEY_F1,            KEYCODE_F1 },
    { "F2",         KEY_F2,            KEYCODE_F2 },
    { "F3",         KEY_F3,            KEYCODE_F3 },
    { "F4",         KEY_F4,            KEYCODE_F4 },
    { "F5",         KEY_F5,            KEYCODE_F5 },
    { "F6",         KEY_F6,            KEYCODE_F6 },
    { "F7",         KEY_F7,            KEYCODE_F7 },
    { "F8",         KEY_F8,            KEYCODE_F8 },
    { "F9",         KEY_F9,            KEYCODE_F9 },
    { "F10",        KEY_F10,           KEYCODE_F10 },
    { "F11",        KEY_F11,           KEYCODE_F11 },
    { "F12",        KEY_F12,           KEYCODE_F12 },
 // { "EJECT",      KEY_EJECT,         KEYCODE_EJECT },

 // { "F13",        KEY_F13,           KEYCODE_F13 },
 // { "F14",        KEY_F14,           KEYCODE_F14 },
 // { "F15",        KEY_F15,           KEYCODE_F15 },
    
 // { "F16",        KEY_F16,           KEYCODE_F16 },
 // { "F17",        KEY_F17,           KEYCODE_F17 },
 // { "F18",        KEY_F18,           KEYCODE_F18 },
 // { "F19",        KEY_F19,           KEYCODE_F19 },

    { "~",          KEY_TILDE,         KEYCODE_TILDE },
    { "1",          KEY_1,             KEYCODE_1 },
    { "2",          KEY_2,             KEYCODE_2 },
    { "3",          KEY_3,             KEYCODE_3 },
    { "4",          KEY_4,             KEYCODE_4 },
    { "5",          KEY_5,             KEYCODE_5 },
    { "6",          KEY_6,             KEYCODE_6 },
    { "7",          KEY_7,             KEYCODE_7 },
    { "8",          KEY_8,             KEYCODE_8 },
    { "9",          KEY_9,             KEYCODE_9 },
    { "0",          KEY_0,             KEYCODE_0 },
    { "-",          KEY_MINUS,         KEYCODE_MINUS },
    { "=",          KEY_EQUALS,        KEYCODE_EQUALS },
    { "DELETE",     KEY_BACKSPACE,     KEYCODE_BACKSPACE },
    
 // { "FN",         KEY_FN,            KEYCODE_FN },
   
    { "HOME",       KEY_HOME,          KEYCODE_HOME },
    { "PGUP",       KEY_PGUP,          KEYCODE_PGUP },
   
    { "DEL",        KEY_DEL_PAD,       KEYCODE_DEL_PAD },
 // { "=",          KEY_EQUALS_PAD,    KEYCODE_EQUALS_PAD },
    { "/",          KEY_SLASH_PAD,     KEYCODE_SLASH_PAD },
 // { "*",          KEY_ASTERISK_PAD,  KEYCODE_ASTERISK_PAD },
    
    { "TAB",		KEY_TAB,		   KEYCODE_TAB },
    { "Q",			KEY_Q,			   KEYCODE_Q },
    { "W",			KEY_W,			   KEYCODE_W },
    { "E",			KEY_E,			   KEYCODE_E },
    { "R",			KEY_R,			   KEYCODE_R },
    { "T",			KEY_T,			   KEYCODE_T },
    { "Y",			KEY_Y,		   	   KEYCODE_Y },
    { "U",			KEY_U,			   KEYCODE_U },
    { "I",			KEY_I,			   KEYCODE_I },
    { "O",			KEY_O,			   KEYCODE_O },
    { "P",			KEY_P,			   KEYCODE_P },
    { "[",          KEY_OPENBRACE,     KEYCODE_OPENBRACE },
    { "]",          KEY_CLOSEBRACE,    KEYCODE_CLOSEBRACE },
    { "\\",         KEY_BACKSLASH,     KEYCODE_BACKSLASH },
    
    { "DEL",        KEY_DEL,           KEYCODE_DEL },
    { "END",        KEY_END,           KEYCODE_END },
    { "PGDN",       KEY_PGDN,          KEYCODE_PGDN },
    
    { "7",          KEY_7_PAD,         KEYCODE_7_PAD },
    { "8",          KEY_8_PAD,         KEYCODE_8_PAD },
    { "9",          KEY_9_PAD,         KEYCODE_9_PAD },
    { "-",          KEY_MINUS_PAD,     KEYCODE_MINUS_PAD },
   
    { "CAPS",       KEY_CAPSLOCK,      KEYCODE_CAPSLOCK },
    { "A",			KEY_A,			   KEYCODE_A },
    { "S",			KEY_S,			   KEYCODE_S },
    { "D",			KEY_D,			   KEYCODE_D },
    { "F",			KEY_F,			   KEYCODE_F },
 	{ "G",			KEY_G,			   KEYCODE_G },
    { "H",			KEY_H,			   KEYCODE_H },
    { "J",			KEY_J,			   KEYCODE_J },
    { "K",			KEY_K,			   KEYCODE_K },
    { "L",			KEY_L,		  	   KEYCODE_L },
    { ";",			KEY_COLON,	       KEYCODE_COLON },
    { "'",          KEY_QUOTE,         KEYCODE_QUOTE },
    { "ENTER" ,     KEY_ENTER,         KEYCODE_ENTER },
      
    { "4",          KEY_4_PAD   ,      KEYCODE_4_PAD },
    { "5",          KEY_5_PAD,         KEYCODE_5_PAD },
    { "6",          KEY_6_PAD,         KEYCODE_6_PAD },
    { "+",          KEY_PLUS_PAD,      KEYCODE_PLUS_PAD },
    
    { "SHIFT",		KEY_LSHIFT,		   KEYCODE_LSHIFT },
    { "Z",			KEY_Z,			   KEYCODE_Z },
    { "X",			KEY_X,			   KEYCODE_X },
 	{ "C",			KEY_C,			   KEYCODE_C },
    { "V",			KEY_V,			   KEYCODE_V },
    { "B",			KEY_B,			   KEYCODE_B },
    { "N",			KEY_N,			   KEYCODE_N },
    { "M",			KEY_M,			   KEYCODE_M },
    { ",",			KEY_COMMA,	       KEYCODE_COMMA },
    { ".",			KEY_STOP,	       KEYCODE_STOP },
    { "/",			KEY_SLASH,	       KEYCODE_SLASH },
    { "SHIFT",	    KEY_RSHIFT,		   KEYCODE_RSHIFT },

    { "UP",         KEY_UP,            KEYCODE_UP },

    { "1",          KEY_1_PAD,         KEYCODE_1_PAD },
    { "2",          KEY_2_PAD,         KEYCODE_2_PAD },
    { "3",          KEY_3_PAD,         KEYCODE_3_PAD },
    { "ENTER",      KEY_ENTER_PAD,     KEYCODE_ENTER_PAD },

    { "CONTROL",    KEY_LCONTROL,      KEYCODE_LCONTROL },
    { "ALT",		KEY_LALT,		   KEYCODE_LALT },
    { "COMMAND",	KEY_LWIN,		   KEYCODE_LWIN },
    { "SPACE",      KEY_SPACE,         KEYCODE_SPACE },
    { "COMMAND",	KEY_RWIN,		   KEYCODE_RWIN },
    { "ALT",		KEY_RALT,		   KEYCODE_RALT },
    { "CONTROL",	KEY_RCONTROL,      KEYCODE_RCONTROL },

    { "LEFT",       KEY_LEFT,          KEYCODE_LEFT },
    { "DOWN",       KEY_DOWN,          KEYCODE_DOWN },
    { "RIGHT",      KEY_RIGHT,         KEYCODE_RIGHT },
    
    { "0",          KEY_0_PAD,         KEYCODE_0_PAD },
 // { ".",          KEY_STOP_PAD,      KEYCODE_STOP_PAD },
    
 	{ 0, 0, 0 }	
};

static struct JoystickInfo joylist[] = {
    { 0, 0, 0 }	
};

UINT8						win_trying_to_quit;

/**
 * osd_get_key_list
 */

const struct KeyboardInfo* osd_get_key_list(void) {
	return keylist;
}

/**
 * osd_get_joy_list
 */

const struct JoystickInfo* osd_get_joy_list(void) {
	return joylist;
}

/**
 * osd_customize_inputport_defaults
 */

void osd_customize_inputport_defaults(struct ipd *defaults) {
}

/**
 * osd_analogjoy_read
 */

void osd_analogjoy_read(int player, int analog_axis[MAX_ANALOG_AXES], InputCode analogjoy_input[MAX_ANALOG_AXES]) {
}

/**
 * osd_trak_read
 */

void osd_trak_read(int player, int *deltax, int *deltay) {
}

/**
 * osd_lightgun_read
 */

void osd_lightgun_read(int player, int *deltax, int *deltay) {
}

/**
 * osd_is_key_pressed
 */

int osd_is_key_pressed(int keycode) {
	return ipinmame_get_keycode_status(keycode);
}

/**
 * osd_is_joy_pressed
 */

int osd_is_joy_pressed(int joycode) {
    return 0;
}

/**
 * osd_is_joystick_axis_code
 */

int osd_is_joystick_axis_code(int joycode) {
	return 0;
}

/**
 * osd_joystick_needs_calibration
 */

int osd_joystick_needs_calibration(void) {
    return 0;
}

/**
 * osd_joystick_start_calibration
 */

void osd_joystick_start_calibration(void) {
}

/**
 * osd_joystick_calibrate
 */

void osd_joystick_calibrate(void) {
}

/**
 * osd_joystick_calibrate_next
 */

const char* osd_joystick_calibrate_next(void) {
	return NULL;
}

/**
 * osd_joystick_end_calibration
 */

void osd_joystick_end_calibration(void) {
}
