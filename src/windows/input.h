//============================================================
//
//	input.c - Win32 implementation of MAME input routines
//
//============================================================

#ifndef __INPUT_H
#define __INPUT_H

//============================================================
//	MACROS
//============================================================

// Define the keyboard indicators.
// (Definitions borrowed from ntddkbd.h)
//

#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)


//============================================================
//	PARAMETERS
//============================================================

#define KEYBOARD_CAPS_LOCK_ON     4
#define KEYBOARD_NUM_LOCK_ON      2
#define KEYBOARD_SCROLL_LOCK_ON   1

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;		// Unit identifier.
    USHORT LedFlags;		// LED indicator state.

} KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;


//============================================================
//	PROTOTYPES
//============================================================

void start_led(void);
void stop_led(void);

#endif /* ifndef __INPUTD_H */
