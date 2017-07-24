#ifndef __KEYBOARD_H
#define __KEYBOARD_H

struct xmame_keyboard_event {
   unsigned char press;
   unsigned char scancode;
   unsigned short unicode;
};

int xmame_keyboard_init(void);
void xmame_keyboard_exit(void);
void xmame_keyboard_register_event(struct xmame_keyboard_event *event);
void xmame_keyboard_clear(void);

/* Defines for the standard pc-scancodes which xmame uses as keysyms.
   The names of the defines have been kept the same as used in
   src/msdos/input.c for easy cut and paste */

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

#endif
