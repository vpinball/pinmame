// license:BSD-3-Clause

/* PinMAME Spike 1 subsystem - the contract with PinMAME's half of the build

   The subsystem includes no PinMAME headers: those belong to the other half, built with different
   settings (see cmake/spike1.cmake). This header is the one crossing between them - owned by the
   subsystem, included by src/wpc/spike1.c - so it is plain C. src/spike1/spike1_pinmame.cpp
   implements the functions. Numbers are the factory manual's, from the title's own tables: Switch,
   Driver and Light Reference numbers, with the CPU board's switches C1-C16 as 101-116 and an
   optional topper's own switches, which it numbers from 1 again, as 121 and up */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* The emulated CPU's clock: PinMAME's MDRV_CPU_ADD and the subsystem's time base both use it */
#define SPIKE1_CPU_HZ 400000000

#define SPIKE1_DMD_WIDTH  128
#define SPIKE1_DMD_HEIGHT 32
/* The LCD insert, as the player sees it (Ghostbusters' Ecto goggles) */
#define SPIKE1_INSERT_WIDTH  160
#define SPIKE1_INSERT_HEIGHT 128
/* The CPU board's own LCD (WWE's playfield screen) */
#define SPIKE1_LCD_WIDTH  320
#define SPIKE1_LCD_HEIGHT 240

/* One of the title's files, by its name in the game folder; the caller keeps the bytes alive
   until spike1_pinmame_stop() */
typedef struct spike1_file
{
	const char *name;
	const unsigned char *data;
	unsigned size;
} spike1_file;

/* The address (and size) of one of the game program's tables or functions the machine reads, for
   a program shipped without its symbol table (Heavy Metal); a list ends with a NULL name */
typedef struct spike1_symbol
{
	const char *name;
	unsigned address, size;
} spike1_symbol;

/* Starts the machine from the title's files. game is the game folder's name (the program runs as
   /games/<game>/game); symbols is NULL, or the list for a program without a symbol table; nvram
   is the block spike1_pinmame_nvram() gave the last time, or NULL for a new machine. 0 on failure,
   with the reason in error */
int spike1_pinmame_start(const char *game, const spike1_file *files, unsigned count, const spike1_symbol *symbols,
                         const unsigned char *nvram, unsigned nvram_size, char *error, unsigned error_size);
void spike1_pinmame_stop(void);
/* 1 while the game program runs; when it has stopped, why, in reason */
int spike1_pinmame_running(char *reason, unsigned reason_size);

/* Runs the machine for `cycles` CPU cycles and returns the cycles used: all of them, also while
   every guest thread waits, so emulated time keeps PinMAME's pace */
int spike1_pinmame_run(int cycles);

/* Everything the machine keeps across power cycles, as one block: the size it needs, copied into
   dst when it fits in capacity */
unsigned spike1_pinmame_nvram(unsigned char *dst, unsigned capacity);
/* Switches the machine off as the mains would. A game keeps settings, audits and the like in
   memory and commits them to its NVRAM files when it sees the power fail, so a host calls this
   before taking the block: it runs the machine until the game has committed, for at most max_ms
   of emulated time. 1 when the game committed */
int spike1_pinmame_power_down(unsigned max_ms);

/* Switches: their numbers and names, the state they rest in, and a change */
int spike1_pinmame_switch(unsigned index, int *number, int *closed_at_rest, const char **name); /* 0 past the last */
int spike1_pinmame_find_switch(const char *name); /* the number of the switch so named, -1 if none */
void spike1_pinmame_set_switch(int number, int closed);

/* Coils: their factory-manual Driver Reference numbers and names, and a driver's PWM duty (0-255)
   as a host shows it. Take it once a frame: a pulse that came and went since the last call shows
   at its power, and the 1 ms pulses of the game's power-on driver check show nothing */
int spike1_pinmame_coil(unsigned index, int *number, const char **name); /* 0 past the last */
unsigned spike1_pinmame_coil_output(int number);

/* LED channels (an RGB LED is three): each one's factory-manual Light Reference number, its class
   bits (1 lamp, 2 GI string, 4 flasher, 8 motor drive, 0x10 and 0x20 cabinet) and name, and its
   level now (0-255) */
#define SPIKE1_LED_MOTOR 0x08
unsigned spike1_pinmame_led_count(void);
int spike1_pinmame_led(unsigned index, int *number, int *kind, const char **name); /* 0 past the last */
unsigned spike1_pinmame_led_level(unsigned index);

/* A motor the boards run on their own (Ghostbusters' Slimer), in board and motor order, then their
   steppers (Whoa Nellie's reels, Game of Thrones' dragons), then the limit motors below: its
   position now - a motor's in the game's units, a stepper's step within its turn - and whether it
   moves */
int spike1_pinmame_motor(unsigned index, int *position, int *moving); /* 0 when there is none */
/* A stepper whose home switch is wired to another board (Game of Thrones' dragons): the stepper by
   its board's node address and its index there, the switch by its number. The switch is then
   closed while the stepper stands at step 0. Call after spike1_pinmame_start; 0 if no such switch */
int spike1_pinmame_stepper_home(unsigned node, unsigned stepper, int switch_number);
/* A motor the game runs between two limit switches through two LED channels (KISS's Starchild):
   the forward channel's lamp number drives it toward the away switch, the backward one's toward
   home, and it takes travel_ms between them. It stands at home at power-on, and comes after the
   steppers in spike1_pinmame_motor(), as 0 (home) to 100 (away). Call after spike1_pinmame_start;
   0 if the title has no such channel or switch */
int spike1_pinmame_limit_motor(int forward_lamp, int backward_lamp, int home_switch, int away_switch, unsigned travel_ms);

/* The DMD: 128x32 dots of 0-15 into dots; returns the count of frames the game has sent */
unsigned spike1_pinmame_dmd(unsigned char *dots);
/* The LCD insert, when the title has one: 1 and its picture as RGB565 (160x128) and backlight
   (0-255) while it shows a frame, 0 otherwise */
int spike1_pinmame_insert_present(void);
int spike1_pinmame_insert(unsigned short *rgb565, unsigned *backlight);
/* The CPU board's LCD, when the title draws on it: 1 and its picture as RGB565 (320x240) once the
   game has shown a frame, 0 otherwise */
int spike1_pinmame_lcd(unsigned short *rgb565);
/* The 16-byte key the title's LCD videos are encrypted with, which the board's factory key store
   would give the game (WWE's clips): call after spike1_pinmame_start; 0 if the title has no
   encrypted video */
int spike1_pinmame_video_key(const unsigned char *key);

/* Sound: 16-bit stereo at spike1_pinmame_audio_rate(); takes up to frames frames, interleaved
   left/right, and returns how many it took */
unsigned spike1_pinmame_audio_rate(void);
unsigned spike1_pinmame_audio(short *dst, unsigned frames);

#ifdef __cplusplus
}
#endif
