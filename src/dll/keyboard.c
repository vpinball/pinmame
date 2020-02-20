#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "driver.h"
//#include "misc.h"
//#include "video.h"
#include "fileio.h"

#define MAX_KEYS 128

#define KEY_NONE		0

#define KEY_ESC			1

#define KEY_1			2
#define KEY_2			3
#define KEY_3			4
#define KEY_4			5
#define KEY_5			6
#define KEY_6			7
#define KEY_7			8
#define KEY_8			9
#define KEY_9			10
#define KEY_0			11

#define KEY_MINUS		12
#define KEY_EQUALS		13

#define KEY_BACKSPACE		14
#define KEY_TAB			15

#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_OPENBRACE		26
#define KEY_CLOSEBRACE		27

#define KEY_ENTER		28

#define KEY_LCONTROL		29

#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_COLON		39
#define KEY_QUOTE		40
#define KEY_TILDE		41

#define KEY_LSHIFT		42
#define KEY_BACKSLASH		43

#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M			50
#define KEY_COMMA		51
#define KEY_STOP		52
#define KEY_SLASH		53

#define KEY_RSHIFT		54
#define KEY_ASTERISK		55

#define KEY_ALT			56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58

#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68

#define KEY_NUMLOCK		69
#define KEY_SCRLOCK		70

#define KEY_7_PAD		71
#define KEY_8_PAD		72
#define KEY_9_PAD		73
#define KEY_MINUS_PAD		74
#define KEY_4_PAD		75
#define KEY_5_PAD		76
#define KEY_6_PAD		77
#define KEY_PLUS_PAD		78
#define KEY_1_PAD		79
#define KEY_2_PAD		80
#define KEY_3_PAD		81
#define KEY_0_PAD		82
#define KEY_DEL_PAD		83

#define KEY_BACKSLASH2		86

#define KEY_F11			87
#define KEY_F12			88

#define KEY_ENTER_PAD		96
#define KEY_RCONTROL		97
#define KEY_SLASH_PAD		98
#define KEY_PRTSCR		99
#define KEY_ALTGR		100
#define KEY_PAUSE		101	/* Beware: is 119     */
#define KEY_PAUSE_ALT		119	/* on some keyboards! */

#define KEY_HOME		102
#define KEY_UP			103
#define KEY_PGUP		104
#define KEY_LEFT		105
#define KEY_RIGHT		106
#define KEY_END			107
#define KEY_DOWN		108
#define KEY_PGDN		109
#define KEY_INSERT		110
#define KEY_DEL			111

#define KEY_LWIN		125
#define KEY_RWIN		126
#define KEY_MENU		127

#define KEY_MAX			128

/******************************************************************************

Keyboard

******************************************************************************/

volatile int trying_to_quit=0;

/*
return a list of all available keys (see input.h)
*/
// this will be filled in dynamically
static struct KeyboardInfo keylist[] =
{
	{ "A",		KEY_A,		KEYCODE_A },
	{ "B",		KEY_B,		KEYCODE_B },
	{ "C",		KEY_C,		KEYCODE_C },
	{ "D",		KEY_D,		KEYCODE_D },
	{ "E",		KEY_E,		KEYCODE_E },
	{ "F",		KEY_F,		KEYCODE_F },
	{ "G",		KEY_G,		KEYCODE_G },
	{ "H",		KEY_H,		KEYCODE_H },
	{ "I",		KEY_I,		KEYCODE_I },
	{ "J",		KEY_J,		KEYCODE_J },
	{ "K",		KEY_K,		KEYCODE_K },
	{ "L",		KEY_L,		KEYCODE_L },
	{ "M",		KEY_M,		KEYCODE_M },
	{ "N",		KEY_N,		KEYCODE_N },
	{ "O",		KEY_O,		KEYCODE_O },
	{ "P",		KEY_P,		KEYCODE_P },
	{ "Q",		KEY_Q,		KEYCODE_Q },
	{ "R",		KEY_R,		KEYCODE_R },
	{ "S",		KEY_S,		KEYCODE_S },
	{ "T",		KEY_T,		KEYCODE_T },
	{ "U",		KEY_U,		KEYCODE_U },
	{ "V",		KEY_V,		KEYCODE_V },
	{ "W",		KEY_W,		KEYCODE_W },
	{ "X",		KEY_X,		KEYCODE_X },
	{ "Y",		KEY_Y,		KEYCODE_Y },
	{ "Z",		KEY_Z,		KEYCODE_Z },
	{ "0",		KEY_0,		KEYCODE_0 },
	{ "1",		KEY_1,		KEYCODE_1 },
	{ "2",		KEY_2,		KEYCODE_2 },
	{ "3",		KEY_3,		KEYCODE_3 },
	{ "4",		KEY_4,		KEYCODE_4 },
	{ "5",		KEY_5,		KEYCODE_5 },
	{ "6",		KEY_6,		KEYCODE_6 },
	{ "7",		KEY_7,		KEYCODE_7 },
	{ "8",		KEY_8,		KEYCODE_8 },
	{ "9",		KEY_9,		KEYCODE_9 },
	{ "0 PAD",	KEY_0_PAD,	KEYCODE_0_PAD },
	{ "1 PAD",	KEY_1_PAD,	KEYCODE_1_PAD },
	{ "2 PAD",	KEY_2_PAD,	KEYCODE_2_PAD },
	{ "3 PAD",	KEY_3_PAD,	KEYCODE_3_PAD },
	{ "4 PAD",	KEY_4_PAD,	KEYCODE_4_PAD },
	{ "5 PAD",	KEY_5_PAD,	KEYCODE_5_PAD },
	{ "6 PAD",	KEY_6_PAD,	KEYCODE_6_PAD },
	{ "7 PAD",	KEY_7_PAD,	KEYCODE_7_PAD },
	{ "8 PAD",	KEY_8_PAD,	KEYCODE_8_PAD },
	{ "9 PAD",	KEY_9_PAD,	KEYCODE_9_PAD },
	{ "F1",		KEY_F1,		KEYCODE_F1 },
	{ "F2",		KEY_F2,		KEYCODE_F2 },
	{ "F3",		KEY_F3,		KEYCODE_F3 },
	{ "F4",		KEY_F4,		KEYCODE_F4 },
	{ "F5",		KEY_F5,		KEYCODE_F5 },
	{ "F6",		KEY_F6,		KEYCODE_F6 },
	{ "F7",		KEY_F7,		KEYCODE_F7 },
	{ "F8",		KEY_F8,		KEYCODE_F8 },
	{ "F9",		KEY_F9,		KEYCODE_F9 },
	{ "F10",	KEY_F10,	KEYCODE_F10 },
	{ "F11",	KEY_F11,	KEYCODE_F11 },
	{ "F12",	KEY_F12,	KEYCODE_F12 },
	{ "ESC",	KEY_ESC,	KEYCODE_ESC },
	{ "~",		KEY_TILDE,	KEYCODE_TILDE },
	{ "-",		KEY_MINUS,	KEYCODE_MINUS },
	{ "=",		KEY_EQUALS,	KEYCODE_EQUALS },
	{ "BKSPACE",	KEY_BACKSPACE,	KEYCODE_BACKSPACE },
	{ "TAB",	KEY_TAB,	KEYCODE_TAB },
	{ "[",		KEY_OPENBRACE,	KEYCODE_OPENBRACE },
	{ "]",		KEY_CLOSEBRACE,	KEYCODE_CLOSEBRACE },
	{ "ENTER",	KEY_ENTER,	KEYCODE_ENTER },
	{ ";",		KEY_COLON,	KEYCODE_COLON },
	{ ":",		KEY_QUOTE,	KEYCODE_QUOTE },
	{ "\\",		KEY_BACKSLASH,	KEYCODE_BACKSLASH },
	{ "<",		KEY_BACKSLASH2,	KEYCODE_BACKSLASH2 },
	{ ",",		KEY_COMMA,	KEYCODE_COMMA },
	{ ".",		KEY_STOP,	KEYCODE_STOP },
	{ "/",		KEY_SLASH,	KEYCODE_SLASH },
	{ "SPACE",	KEY_SPACE,	KEYCODE_SPACE },
	{ "INS",	KEY_INSERT,	KEYCODE_INSERT },
	{ "DEL",	KEY_DEL,	KEYCODE_DEL },
	{ "HOME",	KEY_HOME,	KEYCODE_HOME },
	{ "END",	KEY_END,	KEYCODE_END },
	{ "PGUP",	KEY_PGUP,	KEYCODE_PGUP },
	{ "PGDN",	KEY_PGDN,	KEYCODE_PGDN },
	{ "LEFT",	KEY_LEFT,	KEYCODE_LEFT },
	{ "RIGHT",	KEY_RIGHT,	KEYCODE_RIGHT },
	{ "UP",		KEY_UP,		KEYCODE_UP },
	{ "DOWN",	KEY_DOWN,	KEYCODE_DOWN },
	{ "/ PAD",	KEY_SLASH_PAD,	KEYCODE_SLASH_PAD },
	{ "* PAD",	KEY_ASTERISK,	KEYCODE_ASTERISK },
	{ "- PAD",	KEY_MINUS_PAD,	KEYCODE_MINUS_PAD },
	{ "+ PAD",	KEY_PLUS_PAD,	KEYCODE_PLUS_PAD },
	{ ". PAD",	KEY_DEL_PAD,	KEYCODE_DEL_PAD },
	{ "ENTER PAD",	KEY_ENTER_PAD,	KEYCODE_ENTER_PAD },
	{ "PRTSCR",	KEY_PRTSCR,	KEYCODE_PRTSCR },
	{ "PAUSE",	KEY_PAUSE,	KEYCODE_PAUSE },
	{ "PAUSE",	KEY_PAUSE_ALT,	KEYCODE_PAUSE },
	{ "LSHIFT",	KEY_LSHIFT,	KEYCODE_LSHIFT },
	{ "RSHIFT",	KEY_RSHIFT,	KEYCODE_RSHIFT },
	{ "LCTRL",	KEY_LCONTROL,	KEYCODE_LCONTROL },
	{ "RCTRL",	KEY_RCONTROL,	KEYCODE_RCONTROL },
	{ "ALT",	KEY_ALT,	KEYCODE_LALT },
	{ "ALTGR",	KEY_ALTGR,	KEYCODE_RALT },
	{ "LWIN",	KEY_LWIN,	KEYCODE_OTHER },
	{ "RWIN",	KEY_RWIN,	KEYCODE_OTHER },
	{ "MENU",	KEY_MENU,	KEYCODE_OTHER },
	{ "SCRLOCK",	KEY_SCRLOCK,	KEYCODE_SCRLOCK },
	{ "NUMLOCK",	KEY_NUMLOCK,	KEYCODE_NUMLOCK },
	{ "CAPSLOCK",	KEY_CAPSLOCK,	KEYCODE_CAPSLOCK },
	{ 0, 0, 0 }	/* end of table */
};

const struct KeyboardInfo *osd_get_key_list(void)
{
	return keylist;
}

/*
tell whether the specified key is pressed or not. keycode is the OS dependent
code specified in the list returned by osd_get_key_list().
*/
int osd_is_key_pressed(int keycode)
{
	if (keycode >= KEY_MAX)
		return 0;

	/* special case: if we're trying to quit, fake up/down/up/down */
	if (keycode == KEY_ESC && trying_to_quit)
	{
		static int dummy_state = 1;
		return dummy_state ^= 1;
	}

	return 0;
}

/*
Return the Unicode value of the most recently pressed key. This
function is used only by text-entry routines in the user interface and should
not be used by drivers. The value returned is in the range of the first 256
bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

Set flush to 1 to clear the buffer before entering text. This will avoid
having prior UI and game keys leak into the text entry.
*/
int osd_readkey_unicode(int flush)
{
	return 0;
}
