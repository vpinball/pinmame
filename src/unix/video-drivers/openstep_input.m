/*
 * OpenStep input functions - placed here to separate them out from the
 * display functions. This code handles keyboard and mouse input using the
 * OpenStep event queue mechanism. Unknown events are passed onward to allow
 * minaturisation of the window.
 *
 * -bat. 14/3/2000
 */

#import <stdio.h> 
#import <stdlib.h>
#import <unistd.h> 
#import <AppKit/AppKit.h>
#import "xmame.h"
#import "osdepend.h"
#import "driver.h"
#import "keyboard.h"
#import "devices.h"

/*
 * Keyboard variables
 */

typedef struct {
	unsigned char scancode;
	unsigned short unicode;
} key_pair;
static key_pair ibm_keymap[256];	/* map ascii to ibm keycodes */
static NSDate *the_past = nil;

/*
 * External window variable
 */

extern NSWindow *theWindow;

/*
 * Keyboard init - all I really do here is set up the keymapping array
 * between ASCII values and MAME keycodes. Yes, it was all done by
 * hand, though there aren't actually that many of them. We also initialise
 * the "distant past" variable here as it belongs to the keyboard code.
 * This array was slightly over complexified by the introduction of unicode
 * support, but I *think* I have got it right !
 */

void
openstep_keyboard_init(void)
{
	int i;

	/* create the past */
	the_past = [NSDate distantPast];
	[the_past retain];

	/* zero certain arrays */
	for(i=0;i<256;i++)
		ibm_keymap[i]=(key_pair){0,0};

	/* and now we set up this big tedious array	*/
	ibm_keymap['0']=(key_pair){KEY_0,KEYCODE_0};
	ibm_keymap['1']=(key_pair){KEY_1,KEYCODE_1};
	ibm_keymap['2']=(key_pair){KEY_2,KEYCODE_2};
	ibm_keymap['3']=(key_pair){KEY_3,KEYCODE_3};
	ibm_keymap['4']=(key_pair){KEY_4,KEYCODE_4};
	ibm_keymap['5']=(key_pair){KEY_5,KEYCODE_5};
	ibm_keymap['6']=(key_pair){KEY_6,KEYCODE_6};
	ibm_keymap['7']=(key_pair){KEY_7,KEYCODE_7};
	ibm_keymap['8']=(key_pair){KEY_8,KEYCODE_8};
	ibm_keymap['9']=(key_pair){KEY_9,KEYCODE_9};

	ibm_keymap['-']=(key_pair){KEY_MINUS,KEYCODE_MINUS};
	ibm_keymap['_']=(key_pair){KEY_MINUS,KEYCODE_MINUS};
	ibm_keymap['+']=(key_pair){KEY_EQUALS,KEYCODE_EQUALS};
	ibm_keymap['=']=(key_pair){KEY_EQUALS,KEYCODE_EQUALS};
	ibm_keymap['\t']=(key_pair){KEY_TAB,KEYCODE_TAB};

	ibm_keymap['=']=(key_pair){KEY_EQUALS,KEYCODE_EQUALS};
	ibm_keymap['\t']=(key_pair){KEY_TAB,KEYCODE_TAB};
	ibm_keymap['\r']=(key_pair){KEY_ENTER,KEYCODE_ENTER};
	ibm_keymap['\n']=(key_pair){KEY_ENTER,KEYCODE_ENTER};

	ibm_keymap['q']=(key_pair){KEY_Q,KEYCODE_Q};
	ibm_keymap['w']=(key_pair){KEY_W,KEYCODE_W};
	ibm_keymap['e']=(key_pair){KEY_E,KEYCODE_E};
	ibm_keymap['r']=(key_pair){KEY_R,KEYCODE_R};
	ibm_keymap['t']=(key_pair){KEY_T,KEYCODE_T};
	ibm_keymap['y']=(key_pair){KEY_Y,KEYCODE_Y};
	ibm_keymap['u']=(key_pair){KEY_U,KEYCODE_U};
	ibm_keymap['i']=(key_pair){KEY_I,KEYCODE_I};
	ibm_keymap['o']=(key_pair){KEY_O,KEYCODE_O};
	ibm_keymap['p']=(key_pair){KEY_P,KEYCODE_P};
	ibm_keymap['[']=(key_pair){KEY_OPENBRACE,KEYCODE_OPENBRACE};
	ibm_keymap[']']=(key_pair){KEY_CLOSEBRACE,KEYCODE_CLOSEBRACE};

	ibm_keymap['a']=(key_pair){KEY_A,KEYCODE_A};
	ibm_keymap['s']=(key_pair){KEY_S,KEYCODE_S};
	ibm_keymap['d']=(key_pair){KEY_D,KEYCODE_D};
	ibm_keymap['f']=(key_pair){KEY_F,KEYCODE_F};
	ibm_keymap['g']=(key_pair){KEY_G,KEYCODE_G};
	ibm_keymap['h']=(key_pair){KEY_H,KEYCODE_H};
	ibm_keymap['j']=(key_pair){KEY_J,KEYCODE_J};
	ibm_keymap['k']=(key_pair){KEY_K,KEYCODE_K};
	ibm_keymap['l']=(key_pair){KEY_L,KEYCODE_L};
	ibm_keymap[';']=(key_pair){KEY_COLON,KEYCODE_COLON};
	ibm_keymap[':']=(key_pair){KEY_COLON,KEYCODE_COLON};
	ibm_keymap['\'']=(key_pair){KEY_QUOTE,KEYCODE_QUOTE};
	ibm_keymap['@']=(key_pair){KEY_QUOTE,KEYCODE_QUOTE};
	ibm_keymap['~']=(key_pair){KEY_TILDE,KEYCODE_TILDE};
	ibm_keymap['#']=(key_pair){KEY_TILDE,KEYCODE_TILDE};

	ibm_keymap['z']=(key_pair){KEY_Z,KEYCODE_Z};
	ibm_keymap['x']=(key_pair){KEY_X,KEYCODE_X};
	ibm_keymap['c']=(key_pair){KEY_C,KEYCODE_C};
	ibm_keymap['v']=(key_pair){KEY_V,KEYCODE_V};
	ibm_keymap['b']=(key_pair){KEY_B,KEYCODE_B};
	ibm_keymap['n']=(key_pair){KEY_N,KEYCODE_N};
	ibm_keymap['m']=(key_pair){KEY_M,KEYCODE_M};
	ibm_keymap[',']=(key_pair){KEY_COMMA,KEYCODE_COMMA};
	ibm_keymap['<']=(key_pair){KEY_COMMA,KEYCODE_COMMA};
	ibm_keymap['.']=(key_pair){KEY_STOP,KEYCODE_STOP};
	ibm_keymap['>']=(key_pair){KEY_STOP,KEYCODE_STOP};
	ibm_keymap['/']=(key_pair){KEY_SLASH,KEYCODE_SLASH};
	ibm_keymap['?']=(key_pair){KEY_SLASH,KEYCODE_SLASH};

	ibm_keymap['*']=(key_pair){KEY_ASTERISK,KEYCODE_ASTERISK};
	ibm_keymap[' ']=(key_pair){KEY_SPACE,KEYCODE_SPACE};

	ibm_keymap[8]=(key_pair){KEY_BACKSPACE,KEYCODE_BACKSPACE};
	ibm_keymap[27]=(key_pair){KEY_ESC,KEYCODE_ESC};
	ibm_keymap[96]=(key_pair){KEY_NUMLOCK,KEYCODE_NUMLOCK};
	ibm_keymap[127]=(key_pair){KEY_DEL,KEYCODE_DEL};
}


/*
 * Nothing needs doing to close the keybaord, but we release the variable
 * used to hold the distant past here as it is a keyboard variable.
 */

void
sysdep_keyboard_close(void)
{
	[the_past release];
	the_past = nil;
}

/*
 * Get the mouse location and use this to set the deltas relative to
 * the last time. We only use this to get the position of the mouse, it's
 * buttons come in as events in the normal way.
 */

void
sysdep_mouse_poll(void)
{
	static NSPoint last = {0.0, 0.0};
	NSPoint current;

	current = [theWindow mouseLocationOutsideOfEventStream];

	mouse_data[0].deltas[0] = current.x - last.x;
	mouse_data[0].deltas[1] = last.y - current.y; /* inverted */

	last = current;
}

/*
 * Here we are passed an NSEvent from the keyboard and expected to queue
 * the MAME key event associated with it. For ASCII characters there
 * is a simple lookup table, for Unicode characters we use a switch statement
 * with the constants defined in NSEvent.h. This function also deals with
 * using the command key to emulate the function keys.
 */

static inline void
queue_key_event(NSEvent *keyevent)
{
	struct xmame_keyboard_event event;
	unichar buf[2];	/* just in case theres more than 1 */
	NSString *string = [keyevent charactersIgnoringModifiers];
	[string getCharacters:buf range:NSMakeRange(0,1)];

	/* check to see if string is ASCII */
	if([string canBeConvertedToEncoding:NSASCIIStringEncoding]) {
		event.scancode = ibm_keymap[buf[0]].scancode;
		event.unicode = ibm_keymap[buf[0]].unicode;
	} else {
		 switch(buf[0]) {
			 case NSUpArrowFunctionKey:
				event.scancode = KEY_UP;
				event.unicode = KEYCODE_UP;
				break;
			 case NSDownArrowFunctionKey:
				event.scancode = KEY_DOWN;
				event.unicode = KEYCODE_DOWN;
				break;
			 case NSLeftArrowFunctionKey:
				event.scancode = KEY_LEFT;
				event.unicode = KEYCODE_LEFT;
				break;
			 case NSRightArrowFunctionKey:
				event.scancode = KEY_RIGHT;
				event.unicode = KEYCODE_RIGHT;
				break;
			 case NSF1FunctionKey:
				event.scancode = KEY_F1;
				event.unicode = KEYCODE_F1;
				break;
			 case NSF2FunctionKey:
				event.scancode = KEY_F2;
				event.unicode = KEYCODE_F2;
				break;
			 case NSF3FunctionKey:
				event.scancode = KEY_F3;
				event.unicode = KEYCODE_F3;
				break;
			 case NSF4FunctionKey:
				event.scancode = KEY_F4;
				event.unicode = KEYCODE_F4;
				break;
			 case NSF5FunctionKey:
				event.scancode = KEY_F5;
				event.unicode = KEYCODE_F5;
				break;
			 case NSF6FunctionKey:
				event.scancode = KEY_F6;
				event.unicode = KEYCODE_F6;
				break;
			 case NSF7FunctionKey:
				event.scancode = KEY_F7;
				event.unicode = KEYCODE_F7;
				break;
			 case NSF8FunctionKey:
				event.scancode = KEY_F8;
				event.unicode = KEYCODE_F8;
				break;
			 case NSF9FunctionKey:
				event.scancode = KEY_F9;
				event.unicode = KEYCODE_F9;
				break;
			 case NSF10FunctionKey:
				event.scancode = KEY_F10;
				event.unicode = KEYCODE_F10;
				break;
			 case NSF11FunctionKey:
				event.scancode = KEY_F11;
				event.unicode = KEYCODE_F11;
				break;
			 case NSF12FunctionKey:
				event.scancode = KEY_F12;
				event.unicode = KEYCODE_F12;
				break;
			 case NSInsertFunctionKey:
				event.scancode = KEY_INSERT;
				event.unicode = KEYCODE_INSERT;
				break;
			 case NSDeleteFunctionKey:
				event.scancode = KEY_DEL;
				event.unicode = KEYCODE_DEL;
				break;
			 case NSHomeFunctionKey:
				event.scancode = KEY_HOME;
				event.unicode = KEYCODE_HOME;
				break;
			 case NSEndFunctionKey:
				event.scancode = KEY_END;
				event.unicode = KEYCODE_END;
				break;
			 case NSPageUpFunctionKey:
				event.scancode = KEY_PGUP;
				event.unicode = KEYCODE_PGUP;
				break;
			 case NSPageDownFunctionKey:
				event.scancode = KEY_PGDN;
				event.unicode = KEYCODE_PGDN;
				break;
			 case NSPrintScreenFunctionKey:
				event.scancode = KEY_PRTSCR;
				event.unicode = KEYCODE_PRTSCR;
				break;
			 case NSScrollLockFunctionKey:
				event.scancode = KEY_SCRLOCK;
				event.unicode = KEYCODE_SCRLOCK;
				break;
			 case NSPauseFunctionKey:
				event.scancode = KEY_PAUSE;
				event.unicode = KEYCODE_PAUSE;
				break;
			 default:
				return;
		 }
	}

	/* deal with command key */
	if([keyevent modifierFlags] & NSCommandKeyMask)
		switch(event.scancode) {
			case KEY_1:
				event.scancode = KEY_F1;
				event.unicode = KEYCODE_F1;
				break;
			case KEY_2:
				event.scancode = KEY_F2;
				event.unicode = KEYCODE_F2;
				break;
			case KEY_3:
				event.scancode = KEY_F3;
				event.unicode = KEYCODE_F3;
				break;
			case KEY_4:
				event.scancode = KEY_F4;
				event.unicode = KEYCODE_F4;
				break;
			case KEY_5:
				event.scancode = KEY_F5;
				event.unicode = KEYCODE_F5;
				break;
			case KEY_6:
				event.scancode = KEY_F6;
				event.unicode = KEYCODE_F6;
				break;
			case KEY_7:
				event.scancode = KEY_F7;
				event.unicode = KEYCODE_F7;
				break;
			case KEY_8:
				event.scancode = KEY_F8;
				event.unicode = KEYCODE_F8;
				break;
			case KEY_9:
				event.scancode = KEY_F9;
				event.unicode = KEYCODE_F9;
				break;
			case KEY_0:
				event.scancode = KEY_F10;
				event.unicode = KEYCODE_F10;
				break;
			case KEY_MINUS:
				event.scancode = KEY_F11;
				event.unicode = KEYCODE_F11;
				break;
			case KEY_EQUALS:
				event.scancode = KEY_F12;
				event.unicode = KEYCODE_F12;
				break;
		}

	/* now queue it */
	switch([keyevent type]) {
		case NSKeyUp:
			event.press = FALSE;
			xmame_keyboard_register_event(&event);
			break;
		case NSKeyDown:
			event.press = TRUE;
			xmame_keyboard_register_event(&event);
			break;
		default:
			break;
	}
}

/*
 * This loop collect all events from the queue and deals with them by
 * queueing key up and down events to the xmame fifo. Anything we dont
 * use gets passed on to whoever might want it. We handle the mouse
 * button events in this loop too, passing them downwards to allow other
 * things such as minaturisation to happen. This loop is surrounded by an
 * autorelease pool as we do create some objects here.
 */

void
sysdep_update_keyboard(void)
{
	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	NSEvent *event=nil;

	for(;;) {
		/* get the next event */
		event= [NSApp nextEventMatchingMask:NSAnyEventMask
				untilDate:the_past inMode:NSDefaultRunLoopMode
				dequeue:YES];
		/* break out of the loop if there are no more events */
		if(event==nil)
			break;

		/* deal with the event */
		switch([event type]) {
			case NSKeyUp:
			case NSKeyDown:
				queue_key_event(event);
				break;
			case NSLeftMouseDown:
				mouse_data[0].buttons[0] = 1;
				[NSApp sendEvent:event];
				break;
			case NSLeftMouseUp:
				mouse_data[0].buttons[0] = 0;
				[NSApp sendEvent:event]; 
				break;
			case NSRightMouseDown:
				mouse_data[0].buttons[1] = 1;
				[NSApp sendEvent:event];
				break;
			case NSRightMouseUp:
				mouse_data[0].buttons[1] = 0;
				[NSApp sendEvent:event];
				break;
			default:
				[NSApp sendEvent:event];
				break;
		}
	}

	[pool release];
}

/*
 * We do not have access to the keyboard LED's
 */

void sysdep_set_leds(int leds)
{
}
