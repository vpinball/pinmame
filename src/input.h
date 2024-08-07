#ifndef INPUT_H
#define INPUT_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

typedef unsigned InputCode;

struct KeyboardInfo
{
	char *name; /* OS dependant name; 0 terminates the list */
	unsigned code; /* OS dependant code */
	InputCode standardcode;	/* CODE_xxx equivalent from list below, or CODE_OTHER if n/a */
};

struct JoystickInfo
{
	char *name; /* OS dependant name; 0 terminates the list */
	unsigned code; /* OS dependant code */
	InputCode standardcode;	/* CODE_xxx equivalent from list below, or CODE_OTHER if n/a */
};

#if defined(PINMAME) && defined(PROC_SUPPORT)
struct PROCInfo
{
	char *name; /* OS dependant name; 0 terminates the list */
	unsigned code; /* OS dependant code */
	InputCode standardcode;	/* CODE_xxx equivalent from list below, or CODE_OTHER if n/a */
};
#endif /* PINMAME && PROC_SUPPORT */

enum
{
	/* key */
	KEYCODE_A, KEYCODE_B, KEYCODE_C, KEYCODE_D, KEYCODE_E, KEYCODE_F,
	KEYCODE_G, KEYCODE_H, KEYCODE_I, KEYCODE_J, KEYCODE_K, KEYCODE_L,
	KEYCODE_M, KEYCODE_N, KEYCODE_O, KEYCODE_P, KEYCODE_Q, KEYCODE_R,
	KEYCODE_S, KEYCODE_T, KEYCODE_U, KEYCODE_V, KEYCODE_W, KEYCODE_X,
	KEYCODE_Y, KEYCODE_Z, KEYCODE_0, KEYCODE_1, KEYCODE_2, KEYCODE_3,
	KEYCODE_4, KEYCODE_5, KEYCODE_6, KEYCODE_7, KEYCODE_8, KEYCODE_9,
	KEYCODE_0_PAD, KEYCODE_1_PAD, KEYCODE_2_PAD, KEYCODE_3_PAD, KEYCODE_4_PAD,
	KEYCODE_5_PAD, KEYCODE_6_PAD, KEYCODE_7_PAD, KEYCODE_8_PAD, KEYCODE_9_PAD,
	KEYCODE_F1, KEYCODE_F2, KEYCODE_F3, KEYCODE_F4, KEYCODE_F5,
	KEYCODE_F6, KEYCODE_F7, KEYCODE_F8, KEYCODE_F9, KEYCODE_F10,
	KEYCODE_F11, KEYCODE_F12,
	KEYCODE_ESC, KEYCODE_TILDE, KEYCODE_MINUS, KEYCODE_EQUALS, KEYCODE_BACKSPACE,
	KEYCODE_YEN,
	KEYCODE_TAB, KEYCODE_OPENBRACE, KEYCODE_CLOSEBRACE, KEYCODE_ENTER, KEYCODE_COLON,
	KEYCODE_QUOTE, KEYCODE_BACKSLASH, KEYCODE_BACKSLASH2, KEYCODE_COMMA, KEYCODE_STOP,
	KEYCODE_SLASH, KEYCODE_SPACE, KEYCODE_INSERT, KEYCODE_DEL,
	KEYCODE_HOME, KEYCODE_END, KEYCODE_PGUP, KEYCODE_PGDN, KEYCODE_LEFT,
	KEYCODE_RIGHT, KEYCODE_UP, KEYCODE_DOWN,
	KEYCODE_SLASH_PAD, KEYCODE_ASTERISK, KEYCODE_MINUS_PAD, KEYCODE_PLUS_PAD,
	KEYCODE_DEL_PAD, KEYCODE_ENTER_PAD, KEYCODE_PRTSCR, KEYCODE_PAUSE,
	KEYCODE_LSHIFT, KEYCODE_RSHIFT, KEYCODE_LCONTROL, KEYCODE_RCONTROL,
	KEYCODE_LALT, KEYCODE_RALT, KEYCODE_SCRLOCK, KEYCODE_NUMLOCK, KEYCODE_CAPSLOCK,
	KEYCODE_LWIN, KEYCODE_RWIN, KEYCODE_MENU,
#define __code_key_first KEYCODE_A
#define __code_key_last KEYCODE_MENU

	/* joy */
	JOYCODE_1_LEFT,JOYCODE_1_RIGHT,JOYCODE_1_UP,JOYCODE_1_DOWN,
	JOYCODE_1_BUTTON1,JOYCODE_1_BUTTON2,JOYCODE_1_BUTTON3,
	JOYCODE_1_BUTTON4,JOYCODE_1_BUTTON5,JOYCODE_1_BUTTON6,
	JOYCODE_1_BUTTON7,JOYCODE_1_BUTTON8,JOYCODE_1_BUTTON9,
	JOYCODE_1_BUTTON10, JOYCODE_1_START, JOYCODE_1_SELECT,
	JOYCODE_2_LEFT,JOYCODE_2_RIGHT,JOYCODE_2_UP,JOYCODE_2_DOWN,
	JOYCODE_2_BUTTON1,JOYCODE_2_BUTTON2,JOYCODE_2_BUTTON3,
	JOYCODE_2_BUTTON4,JOYCODE_2_BUTTON5,JOYCODE_2_BUTTON6,
	JOYCODE_2_BUTTON7,JOYCODE_2_BUTTON8,JOYCODE_2_BUTTON9,
	JOYCODE_2_BUTTON10, JOYCODE_2_START, JOYCODE_2_SELECT,
	JOYCODE_3_LEFT,JOYCODE_3_RIGHT,JOYCODE_3_UP,JOYCODE_3_DOWN,
	JOYCODE_3_BUTTON1,JOYCODE_3_BUTTON2,JOYCODE_3_BUTTON3,
	JOYCODE_3_BUTTON4,JOYCODE_3_BUTTON5,JOYCODE_3_BUTTON6,
	JOYCODE_3_BUTTON7,JOYCODE_3_BUTTON8,JOYCODE_3_BUTTON9,
	JOYCODE_3_BUTTON10, JOYCODE_3_START, JOYCODE_3_SELECT,
	JOYCODE_4_LEFT,JOYCODE_4_RIGHT,JOYCODE_4_UP,JOYCODE_4_DOWN,
	JOYCODE_4_BUTTON1,JOYCODE_4_BUTTON2,JOYCODE_4_BUTTON3,
	JOYCODE_4_BUTTON4,JOYCODE_4_BUTTON5,JOYCODE_4_BUTTON6,
	JOYCODE_4_BUTTON7,JOYCODE_4_BUTTON8,JOYCODE_4_BUTTON9,
	JOYCODE_4_BUTTON10, JOYCODE_4_START, JOYCODE_4_SELECT,
	JOYCODE_MOUSE_1_BUTTON1,JOYCODE_MOUSE_1_BUTTON2,JOYCODE_MOUSE_1_BUTTON3,
	JOYCODE_MOUSE_2_BUTTON1,JOYCODE_MOUSE_2_BUTTON2,JOYCODE_MOUSE_2_BUTTON3,
	JOYCODE_MOUSE_3_BUTTON1,JOYCODE_MOUSE_3_BUTTON2,JOYCODE_MOUSE_3_BUTTON3,
	JOYCODE_MOUSE_4_BUTTON1,JOYCODE_MOUSE_4_BUTTON2,JOYCODE_MOUSE_4_BUTTON3,
	JOYCODE_5_LEFT,JOYCODE_5_RIGHT,JOYCODE_5_UP,JOYCODE_5_DOWN,			/* JOYCODEs 5-8 placed here, */
	JOYCODE_5_BUTTON1,JOYCODE_5_BUTTON2,JOYCODE_5_BUTTON3,				/* after original joycode mouse button */
	JOYCODE_5_BUTTON4,JOYCODE_5_BUTTON5,JOYCODE_5_BUTTON6,				/* so old cfg files won't be broken */
	JOYCODE_5_BUTTON7,JOYCODE_5_BUTTON8,JOYCODE_5_BUTTON9,				/* as much by their addition */
	JOYCODE_5_BUTTON10, JOYCODE_5_START, JOYCODE_5_SELECT,
	JOYCODE_6_LEFT,JOYCODE_6_RIGHT,JOYCODE_6_UP,JOYCODE_6_DOWN,
	JOYCODE_6_BUTTON1,JOYCODE_6_BUTTON2,JOYCODE_6_BUTTON3,
	JOYCODE_6_BUTTON4,JOYCODE_6_BUTTON5,JOYCODE_6_BUTTON6,
	JOYCODE_6_BUTTON7,JOYCODE_6_BUTTON8,JOYCODE_6_BUTTON9,
	JOYCODE_6_BUTTON10, JOYCODE_6_START, JOYCODE_6_SELECT,
	JOYCODE_7_LEFT,JOYCODE_7_RIGHT,JOYCODE_7_UP,JOYCODE_7_DOWN,
	JOYCODE_7_BUTTON1,JOYCODE_7_BUTTON2,JOYCODE_7_BUTTON3,
	JOYCODE_7_BUTTON4,JOYCODE_7_BUTTON5,JOYCODE_7_BUTTON6,
	JOYCODE_7_BUTTON7,JOYCODE_7_BUTTON8,JOYCODE_7_BUTTON9,
	JOYCODE_7_BUTTON10, JOYCODE_7_START, JOYCODE_7_SELECT,
	JOYCODE_8_LEFT,JOYCODE_8_RIGHT,JOYCODE_8_UP,JOYCODE_8_DOWN,
	JOYCODE_8_BUTTON1,JOYCODE_8_BUTTON2,JOYCODE_8_BUTTON3,
	JOYCODE_8_BUTTON4,JOYCODE_8_BUTTON5,JOYCODE_8_BUTTON6,
	JOYCODE_8_BUTTON7,JOYCODE_8_BUTTON8,JOYCODE_8_BUTTON9,
	JOYCODE_8_BUTTON10, JOYCODE_8_START, JOYCODE_8_SELECT,
	JOYCODE_MOUSE_5_BUTTON1,JOYCODE_MOUSE_5_BUTTON2,JOYCODE_MOUSE_5_BUTTON3,
	JOYCODE_MOUSE_6_BUTTON1,JOYCODE_MOUSE_6_BUTTON2,JOYCODE_MOUSE_6_BUTTON3,
	JOYCODE_MOUSE_7_BUTTON1,JOYCODE_MOUSE_7_BUTTON2,JOYCODE_MOUSE_7_BUTTON3,
	JOYCODE_MOUSE_8_BUTTON1,JOYCODE_MOUSE_8_BUTTON2,JOYCODE_MOUSE_8_BUTTON3,
#define __code_joy_first JOYCODE_1_LEFT
#define __code_joy_last JOYCODE_MOUSE_8_BUTTON3

#if defined(PINMAME) && defined(PROC_SUPPORT)
	PROC_FLIPPER_L, PROC_FLIPPER_R, PROC_START, PROC_ESC_SEQ,
#define __code_proc_first PROC_FLIPPER_L
#define __code_proc_last PROC_ESC_SEQ
#endif /* PINMAME && PROC_SUPPORT */

	__code_max, /* Temination of standard code */

	/* special */
	CODE_NONE = 0x8000, /* no code, also marker of sequence end */
	CODE_OTHER, /* OS code not mapped to any other code */
	CODE_DEFAULT, /* special for input port definitions */
        CODE_PREVIOUS, /* special for input port definitions */
	CODE_NOT, /* operators for sequences */
	CODE_OR /* operators for sequences */
};

/* Wrapper for compatibility */
#define KEYCODE_OTHER CODE_OTHER
#define JOYCODE_OTHER CODE_OTHER
#define KEYCODE_NONE CODE_NONE
#define JOYCODE_NONE CODE_NONE
#define PROCCODE_OTHER CODE_OTHER
#define PROCCODE_NONE CODE_NONE

/***************************************************************************/
/* Single code functions */

int code_init(void);
void code_close(void);

InputCode keyoscode_to_code(unsigned oscode);
InputCode joyoscode_to_code(unsigned oscode);
#if defined(PINMAME) && defined(PROC_SUPPORT)
InputCode procoscode_to_code(unsigned oscode);
#endif /* PINMAME && PROC_SUPPORT */
InputCode savecode_to_code(unsigned savecode);
unsigned code_to_savecode(InputCode code);

const char *code_name(InputCode code);
int code_pressed(InputCode code);
int code_pressed_memory(InputCode code);
int code_pressed_memory_repeat(InputCode code, int speed);
InputCode code_read_async(void);
INT8 code_read_hex_async(void);

/* Wrappers for compatibility */
#define keyboard_name                   code_name
#define keyboard_pressed                code_pressed
#define keyboard_pressed_memory         code_pressed_memory
#define keyboard_pressed_memory_repeat  code_pressed_memory_repeat
#define keyboard_read_async             code_read_async

/***************************************************************************/
/* Sequence code funtions */

/* NOTE: If you modify this value you need also to modify the SEQ_DEF declarations */
#define SEQ_MAX 16

typedef InputCode InputSeq[SEQ_MAX];

INLINE InputCode seq_get_1(InputSeq* a) {
	return (*a)[0];
}

void seq_set_0(InputSeq* seq);
void seq_set_1(InputSeq* seq, InputCode code);
void seq_set_2(InputSeq* seq, InputCode code1, InputCode code2);
void seq_set_3(InputSeq* seq, InputCode code1, InputCode code2, InputCode code3);
void seq_set_4(InputSeq* seq, InputCode code1, InputCode code2, InputCode code3, InputCode code4);
void seq_set_5(InputSeq* seq, InputCode code1, InputCode code2, InputCode code3, InputCode code4, InputCode code5);
void seq_copy(InputSeq* seqdst, InputSeq* seqsrc);
int seq_cmp(InputSeq* seq1, InputSeq* seq2);
void seq_name(InputSeq* seq, char* buffer, size_t max);
int seq_pressed(InputSeq* seq);
void seq_read_async_start(void);
int seq_read_async(InputSeq* code, int first);

/* NOTE: It's very important that this sequence is EXACLY long SEQ_MAX */
#define SEQ_DEF_6(a,b,c,d,e,f) { a, b, c, d, e, f, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE }
#define SEQ_DEF_5(a,b,c,d,e) SEQ_DEF_6(a,b,c,d,e,CODE_NONE)
#define SEQ_DEF_4(a,b,c,d) SEQ_DEF_5(a,b,c,d,CODE_NONE)
#define SEQ_DEF_3(a,b,c) SEQ_DEF_4(a,b,c,CODE_NONE)
#define SEQ_DEF_2(a,b) SEQ_DEF_3(a,b,CODE_NONE)
#define SEQ_DEF_1(a) SEQ_DEF_2(a,CODE_NONE)
#define SEQ_DEF_0 SEQ_DEF_1(CODE_NONE)

/***************************************************************************/
/* input_ui */

int input_ui_pressed(int code);
int input_ui_pressed_repeat(int code, int speed);

/***************************************************************************/
/* analog joy code functions */

int is_joystick_axis_code(unsigned code);
int return_os_joycode(InputCode code);

#endif
