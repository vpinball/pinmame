/*********************************************************************

	cheat.c

	This is a massively rewritten version of the cheat engine. There
	are still quite a few features that are broken or not reimplemented.

	Busted (probably permanently) is the ability to use the UI_CONFIGURE
	key to pop the menu system on and off while retaining the current
	menu selection. The sheer number of submenus involved makes this
	a difficult task to pull off here.

	TODO:

	* key shortcuts to the search menus
		how should I implement this? add interface keys?

		if I use a modifier to enable the cheat keys, if people customize
		their key settings they will need to do a KEY and (not MODIFIER)
		config arrangment to bind to the unmodified key (that's not ideal)

		ideas:
			control + fkeys
			YUIOP
			use F5 to enable the cheat engine keys, then use YUIOP
	* check for compatibility with MESS
	* shift code over to flags-based cheat types
	* help text
	* show maximum number of characters available for cheat descriptions without overflow
	* better text editing
	* searching for values longer than one byte

  	04092001	Ian Patterson	cheat searching by value
								cheat searching by comparison (energy search)
								saving
								watch cheats
								support for 'Get x NOW!' cheats
								moved add/remove cheat buttons to A and D
									(I use a portable which doesn't have INS and DEL keys)
								bugfixes
									mostly in textedit
	04102001	Ian Patterson	fixed specifying watches from the results list
								adding cheats from the result list
								made the message boxes go away faster
								bugfixes
	04112001	Ian Patterson	time cheat searching (delta search)
								bit cheat searching
								deleting a result from the result list
	04122001	Ian Patterson	commenting cheats
								textedit fixes
								user-specified value cheats
								bugfixes
								updated cheat special documentation (below)
								to include 22-24 and 42-44
								disabling watches (individually and all at once)
								fixed restoring previous search data
	04132001	Ian Patterson	user-configurable keys
								ui_text support
								watching cheats from edit menu
	04142001	Ian Patterson	fixed compile errors
								changed 'lives (or some other value)' to 'lives
								(or another value)'
								added plus sign before delta search delta
								'help not yet available' message
								converted all hex display to upper case
								one shot cheats now say 'set' (and act like a button)
								selection bar skips over comment cheats in activation menu
								cheat name templates
								conflict checking
								bugfixes
	04152001	Ian Patterson	stupid bugfix
	04172001	Ian Patterson	saving MESS cheats? (untested)
	04202001	Ian Patterson	auto-pause on entry of menu (compile-time option)
								converting watches into cheats
								saving watches
	04282001	Ian Patterson	fixed small text corruption bug
	05042001	Steph			added cheat types 100 and 101 which patch ROM areas
	06012001	Steph			fixed cheat saving bug (incompatibility with multiple cheat files)
				Ian Patterson	fixed small bug with conflict checking and new ROM cheat types
	06022001	Ian Patterson	added 'speed ramping' for auto-repeat
	06032001	Ian Patterson	fixed integer size bug which prevented menus from having >128 items
								increased MAX_LOADEDCHEATS to 500
	06062001	Ian Patterson	added slow but sure search
								added options menu
								added user memory area selection
	06072001	Ian Patterson	finalized user memory area selection
								updated ui_text
	06082001	Ian Patterson	added true backspace support
	06092001	Ian Patterson	kludging around missing osd_readkey_unicode in MAMEW
								skips multiple consecutive sound CPUs
								address alt step change to 0x100
								address display length truncation based on CPU address bits
	06102001	Ian Patterson	increased MAX_LOADEDCHEATS to 3000 (ddsomj has about 1200 right now)
	06112001	Ian Patterson	fixed typing error
								ANSI compatibility *sigh*
	06122001	Ian Patterson	documented cheat types 100 and 101
								results list shows first and last search values
								added page up/down support to watchpoints and results list
								more comments
								compile fixes
								added "very slow" speed (all non constant memory)
								added W+S (not A+D) support to enable/disable cheat menu
	06132001	Ian Patterson	made base horizontal change speed faster
								added activation key support
								general bugfixing
	06162001	Ian Patterson	changed activation key input function to code_pressed to avoid eating keydowns

(Description from Pugsy's cheat.dat)

; all fields are separated by a colon (:)
;  -Name of the game (short name) [zip file or directory]
;  -No of the CPU usually 0, only different in multiple CPU games
;  -Address in Hexadecimal (where to poke)
;  -Data to put at this address in hexadecimal (what to poke)
;  -Special (see below) usually 000
;   -000 : the byte is poked every time and the cheat remains in active list.
;   -001 : the byte is poked once and the cheat is removed from active list.
;   -002 : the byte is poked every one second and the cheat remains in active
;          list.
;   -003 : the byte is poked every two seconds and the cheat remains in active
;          list.
;   -004 : the byte is poked every five seconds and the cheat remains in active
;          list.
;   -005 : the byte is poked one second after the original value has changed
;          and the cheat remains in active list.
;   -006 : the byte is poked two seconds after the original value has changed
;          and the cheat remains in active list.
;   -007 : the byte is poked five seconds after the original value has changed
;          and the cheat remains in active list.
;   -008 : the byte is poked unless the original value in being decremented by
;          1 each frame and the cheat remains in active list.
;   -009 : the byte is poked unless the original value in being decremented by
;          2 each frame and the cheat remains in active list.
;   -010 : the byte is poked unless the original value in being decremented by
;          3 each frame and the cheat remains in active list.
;   -011 : the byte is poked unless the original value in being decremented by
;          4 each frame and the cheat remains in active list.
;   -020 : the bits are set every time and the cheat remains in active list.
;   -021 : the bits are set once and the cheat is removed from active list.
;	-022 : the bits are set every second and the cheat remains in active list.
;	-023 : the bits are set every two seconds and the cheat remains in active
;          list.
;	-024 : the bits are set every five seconds and the cheat remains in active
;          list.
;   -040 : the bits are reset every time and the cheat remains in active list.
;   -041 : the bits are reset once and the cheat is removed from active list.
;	-042 : the bits are reset every second and the cheat remains in active
;          list.
;	-043 : the bits are reset every two seconds and the cheat remains in active
;          list.
;	-044 : the bits are reset every five seconds and the cheat remains in
;          active list.
;   -060 : the user selects a decimal value from 0 to byte
;          (display : 0 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -061 : the user selects a decimal value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once when it changes
;          and the cheat is removed from the active list.
;   -062 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -063 : the user selects a BCD value from 0 to byte
;          (display : 0 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -064 : the user selects a BCD value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once when it changes
;          and the cheat is removed from the active list.
;   -065 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once when it changes and
;          the cheat is removed from the active list.
;   -070 : the user selects a decimal value from 0 to byte
;          (display : 0 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -071 : the user selects a decimal value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once and the cheat is
;          removed from the active list.
;   -072 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -073 : the user selects a BCD value from 0 to byte
;          (display : 0 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;   -074 : the user selects a BCD value from 0 to byte
;          (display : 1 to byte+1) - the value is poked once and the cheat is
;          removed from the active list.
;   -075 : the user selects a decimal value from 1 to byte
;          (display : 1 to byte) - the value is poked once and the cheat is
;          removed from the active list.
;	-100 : constantly pokes the value to the selected CPU's ROM region
;	-101 : pokes the value to the selected CPU's ROM region and the cheat is
;	       removed from the active list
;   -500 to 575: These cheat types are identical to types 000 to 075 except
;                they are used in linked cheats (i.e. of 1/8 type). The first
;                cheat in the link list will be the normal type (eg type 000)
;                and the remaining cheats (eg 2/8...8/8) will be of this type
;                (eg type 500).
;   -998 : this is used as a watch cheat, ideal for showing answers in quiz
;          games .
;   -999 : this is used for comments only, cannot be enabled/selected by the
;          user.
;  -Name of the cheat
;  -Description for the cheat

*********************************************************************/

#include "driver.h"
#include "ui_text.h"

#include <ctype.h>

#ifndef MESS
#ifndef TINY_COMPILE
#ifndef CPSMAME
extern struct GameDriver driver_neogeo;
#endif
#endif
#endif

#define CHEAT_PAUSE			0

// check for MAMEW, install kludge if needed
#ifdef _WINDOWS_H
#define OSD_READKEY_KLUDGE	1
#else
#define OSD_READKEY_KLUDGE	0
#endif

/******************************************
 *
 * Cheats
 *
 */

#define MAX_LOADEDCHEATS		3000
#define CHEAT_FILENAME_MAXLEN	255
#define MAX_MEMORY_AREAS		256
#define MAX_SUBCHEATS			80

enum
{
	kCheatFlagActive =					1 << 0,
	kCheatFlagWatch =					1 << 1,
	kCheatFlagComment =					1 << 2,
	kCheatFlagDecPrompt =				1 << 3,
	kCheatFlagBCDPrompt =				1 << 4,
	kCheatFlagOneShot =					1 << 5,
	kCheatFlagActivationKeyPressed =	1 << 6,

	kCheatFlagPromptMask =	kCheatFlagDecPrompt |
							kCheatFlagBCDPrompt,
	kCheatFlagStatusMask =	kCheatFlagActive |
							kCheatFlagActivationKeyPressed,
	kCheatFlagModeMask =	kCheatFlagWatch |
							kCheatFlagComment |
							kCheatFlagPromptMask |
							kCheatFlagOneShot
};

enum
{
	kSubcheatFlagDone =			1 << 0,
	kSubcheatFlagTimed =		1 << 1,
	kSubcheatFlagBitModify =	1 << 2,
	kSubcheatFlagByteModify =	1 << 3,
	kSubcheatFlagCustomRegion =	1 << 4,

	kSubcheatFlagModifyMask =	kSubcheatFlagBitModify |
								kSubcheatFlagByteModify,
	kSubcheatFlagStatusMask =	kSubcheatFlagDone |
								kSubcheatFlagTimed,
	kSubcheatFlagModeMask =		kSubcheatFlagBitModify |
								kSubcheatFlagByteModify |
								kSubcheatFlagCustomRegion
};

struct subcheat_struct
{
	int		cpu;
	offs_t	address;
	data8_t	data;
#ifdef MESS
	data8_t	olddata;			/* old data for code patch when cheat is turned OFF */
#endif
	data8_t	backup;				/* The original value of the memory location, checked against the current */
	UINT32	code;
	UINT16	flags;
	data8_t	min;
	data8_t	max;
	UINT32	frames_til_trigger;	/* the number of frames until this cheat fires (does not change) */
	UINT32	frame_count;		/* decrementing frame counter to determine if cheat should fire */
};

struct cheat_struct
{
#ifdef MESS
	unsigned int			crc;		/* CRC of the game */
	char					patch;		/* 'C' : code patch - 'D' : data patch */
#endif
	char					* name;
	char					* comment;
	UINT8					flags;
	int						num_sub;	/* number of cheat cpu/address/data/code combos for this one cheat */
	struct subcheat_struct	* subcheat;	/* a variable-number of subcheats are attached to each "master" cheat */
	int						activate_key;
};

struct memory_struct
{
	int					enabled;
	char				name[40];
	mem_write_handler	handler;
};

enum
{
	kCheatSpecial_Poke =			0,
	kCheatSpecial_PokeRemove =		1,
	kCheatSpecial_Poke1 =			2,
	kCheatSpecial_Poke2 =			3,
	kCheatSpecial_Poke5 =			4,
	kCheatSpecial_Delay1 =			5,
	kCheatSpecial_Delay2 =			6,
	kCheatSpecial_Delay5 =			7,
	kCheatSpecial_Backup1 =			8,
	kCheatSpecial_Backup2 =			9,
	kCheatSpecial_Backup3 =			10,
	kCheatSpecial_Backup4 =			11,
	kCheatSpecial_SetBit =			20,
	kCheatSpecial_SetBitRemove =	21,
	kCheatSpecial_SetBit1 =			22,
	kCheatSpecial_SetBit2 =			23,
	kCheatSpecial_SetBit5 =			24,
	kCheatSpecial_ResetBit =		40,
	kCheatSpecial_ResetBitRemove =	41,
	kCheatSpecial_ResetBit1 =		42,
	kCheatSpecial_ResetBit2 =		43,
	kCheatSpecial_ResetBit5 =		44,
	kCheatSpecial_UserFirst =		60,
	kCheatSpecial_m0d0c =			60,		/* minimum value 0, display range 0 to byte, poke when changed */
	kCheatSpecial_m0d1c =			61,		/* minimum value 0, display range 1 to byte+1, poke when changed */
	kCheatSpecial_m1d1c =			62,		/* minimum value 1, display range 1 to byte, poke when changed */
	kCheatSpecial_m0d0bcdc =		63,		/* BCD, minimum value 0, display range 0 to byte, poke when changed */
	kCheatSpecial_m0d1bcdc =		64,		/* BCD, minimum value 0, display range 1 to byte+1, poke when changed */
	kCheatSpecial_m1d1bcdc =		65,		/* BCD, minimum value 1, display range 1 to byte, poke when changed */
	kCheatSpecial_m0d0 =			70,		/* minimum value 0, display range 0 to byte */
	kCheatSpecial_m0d1 =			71,		/* minimum value 0, display range 1 to byte+1 */
	kCheatSpecial_m1d1 =			72,		/* minimum value 1, display range 1 to byte */
	kCheatSpecial_m0d0bcd =			73,		/* BCD, minimum value 0, display range 0 to byte */
	kCheatSpecial_m0d1bcd =			74,		/* BCD, minimum value 0, display range 1 to byte+1 */
	kCheatSpecial_m1d1bcd =			75,		/* BCD, minimum value 1, display range 1 to byte */
	kCheatSpecial_UserLast =		75,
	/* Steph 2001.05.04 - added 'PokeROM' and 'PokeROMRemove', and changed 'Last' and 'LinkEnd' */
	kCheatSpecial_PokeROM =			100,
	kCheatSpecial_PokeROMRemove =	101,
	kCheatSpecial_Last =			199,
	kCheatSpecial_LinkStart =		500,	/* only used when loading the database */
	kCheatSpecial_LinkEnd =			699,	/* only used when loading the database */
	kCheatSpecial_Watch =			998,
	kCheatSpecial_Comment =			999,
	kCheatSpecial_Timed =			1000
};

/* Steph 2001.05.04 - added types 100 and 101 */
const int kSupportedCheatTypes[] =
{
	0,		1,		2,		3,		4,		5,		6,		7,		8,		9,
	10,		11,		20,		21,		40,		41,		60,		61,		62,		63,
	64,		65,		70,		71,		72,		73,		74,		75,		100,
	101,	998,	999,
	-1
};

char * cheatfile = "cheat.dat";

char database[CHEAT_FILENAME_MAXLEN + 1];

int he_did_cheat;

char * kCheatNameTemplates[] =
{
	"Infinite Lives",
	"Infinite Lives PL1",
	"Infinite Lives PL2",
	"Infinite Time",
	"Infinite Time PL1",
	"Infinite Time PL2",
	"Invincibility",
	"Invincibility PL1",
	"Invincibility PL2",
	"Infinite Energy",
	"Infinite Energy PL1",
	"Infinite Energy PL2",
	"Select next level",
	"Select current level",
	"Infinite Ammo",
	"Infinite Ammo PL1",
	"Infinite Ammo PL2",
	"Infinite Bombs",
	"Infinite Bombs PL1",
	"Infinite Bombs PL2",
	"Infinite Smart Bombs",
	"Infinite Smart Bombs PL1",
	"Infinite Smart Bombs PL2",
	"Select Score PL1",
	"Select Score PL2",
	"Drain all Energy Now! PL1",
	"Drain all Energy Now! PL2",
	"Watch me for good answer",
	"Infinite",
	"Always have",
	"Get",
	"Lose",
	"Finish this",
	"---> <ENTER> To Edit <---",
	"\0"
};

/******************************************
 *
 * Searches
 *
 */

/* Defines */
#define MAX_SEARCHES 500

enum
{
	kRestore_NoInit,
	kRestore_NoSave,
	kRestore_Done,
	kRestore_OK
};

enum
{
	kSpeed_Fast = 0,	// RAM + some banks
	kSpeed_Medium,		// RAM + BANKx
	kSpeed_Slow,		// all memory areas except ROM, NOP, and custom handlers w/o mapped memory
	kSpeed_VerySlow		// all memory areas except ROM and NOP
};

/* Local variables */
static int	searchCPU =		0;
static int	searchValue =	0;
static int	searchTime =	0;
static int	searchEnergy =	0;
static int	searchBit =		0;
static int	searchSlow =	0;
static int	searchSpeed =	kSpeed_Medium;
static int	restoreStatus =	kRestore_NoInit;

static struct ExtMemory	StartRam[MAX_EXT_MEMORY];
static struct ExtMemory	BackupRam[MAX_EXT_MEMORY];
static struct ExtMemory	FlagTable[MAX_EXT_MEMORY];

static struct ExtMemory	OldBackupRam[MAX_EXT_MEMORY];
static struct ExtMemory	OldFlagTable[MAX_EXT_MEMORY];

/******************************************
 *
 * Watchpoints
 *
 */

#define MAX_WATCHES 20

struct watch_struct
{
	int		cheat_num;	/* if this watchpoint is tied to a cheat, this is the index into the cheat array. -1 if none */
	UINT32	address;
	INT16	cpu;
	UINT8	num_bytes;	/* number of consecutive bytes to display */
	UINT8	label_type;	/* none, address, text */
	char	label[255];	/* optional text label */
	UINT16	x;			/* position of watchpoint on screen */
	UINT16	y;
};

static struct watch_struct	watches[MAX_WATCHES];
static int					is_watch_active;		/* true if at least one watchpoint is active */
static int					is_watch_visible;		/* we can toggle the visibility for all on or off */

/* in hiscore.c */
int computer_readmem_byte(int cpu, int addr);
void computer_writemem_byte(int cpu, int addr, int value);

/* Some macros to simplify the code */
#define READ_CHEAT				computer_readmem_byte(subcheat->cpu, subcheat->address)
#define WRITE_CHEAT				computer_writemem_byte(subcheat->cpu, subcheat->address, subcheat->data)
#define COMPARE_CHEAT			(computer_readmem_byte(subcheat->cpu, subcheat->address) != subcheat->data)
#define CPU_AUDIO_OFF(index)	((Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU) && (Machine->sample_rate == 0))

/* Steph */
#ifdef MESS
#define WRITE_OLD_CHEAT			computer_writemem_byte(subcheat->cpu, subcheat->address, subcheat->olddata)
#endif

/* Local prototypes */
static INT32	DisplayHelpFile(struct osd_bitmap * bitmap, INT32 selected);
static INT32	EditCheatMenu(struct osd_bitmap * bitmap, INT32 selected, UINT8 cheatnum);
static INT32	CommentMenu(struct osd_bitmap * bitmap, INT32 selected, int cheat_index);
static INT32	UserSelectValueMenu(struct osd_bitmap * bitmap, int selection, int cheat_num);
static int		FindFreeWatch(void);
static void		reset_table(struct ExtMemory * table);
static void		AddResultToListByIdx(int idx);
static void		AddCheatToList(offs_t address, UINT8 data, int cpu);
static void		SetupDefaultMemoryAreas(int cpu);

/* Local variables */
static int					ActiveCheatTotal;				/* number of cheats currently active */
static int					LoadedCheatTotal;				/* total number of cheats */
static struct cheat_struct	CheatTable[MAX_LOADEDCHEATS+1];

static int					CheatEnabled;
static int					cheatEngineWasActive;

static UINT8				memoryRegionEnabled[MAX_MEMORY_REGIONS];

const int					kVerticalKeyRepeatRate =		8;
const int					kHorizontalFastKeyRepeatRate =	5;
const int					kHorizontalSlowKeyRepeatRate =	8;

//	returns the number of nibbles required to represent the target CPU's
//	address range
static int CPUAddressWidth(int cpu)
{
	int bits = cpunum_address_bits(cpu);

	if(bits & 0x3)
		return (bits >> 2) + 1;
	else
		return bits >> 2;
}

//	returns if either shift key is pressed
static int ShiftKeyPressed(void)
{
	return (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT));
}

//	returns if either control key is pressed
static int ControlKeyPressed(void)
{
	return (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL));
}

//	returns if either alt/option key is pressed
static int AltKeyPressed(void)
{
	return (code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_RALT));
}

#if OSD_READKEY_KLUDGE

//	dirty hack until osd_readkey_unicode is supported in MAMEW
//	re-implementation of osd_readkey_unicode
static int ReadKeyAsync(int flush)
{
	int	code;

	if(flush)
	{
		while(code_read_async() != CODE_NONE) ;

		return 0;
	}

	while(1)
	{
		code = code_read_async();

		if(code == CODE_NONE)
		{
			return 0;
		}
		else if((code >= KEYCODE_A) && (code <= KEYCODE_Z))
		{
			if(ShiftKeyPressed())
			{
				return 'A' + (code - KEYCODE_A);
			}
			else
			{
				return 'a' + (code - KEYCODE_A);
			}
		}
		else if((code >= KEYCODE_0) && (code <= KEYCODE_9))
		{
			if(ShiftKeyPressed())
			{
				return ")!@#$%^&*()"[code - KEYCODE_0];
			}
			else
			{
				return '0' + (code - KEYCODE_0);
			}
		}
		else if((code >= KEYCODE_0_PAD) && (code <= KEYCODE_0_PAD))
		{
			return '0' + (code - KEYCODE_0);
		}
		else if(code == KEYCODE_TILDE)
		{
			if(ShiftKeyPressed())
			{
				return '~';
			}
			else
			{
				return '`';
			}
		}
		else if(code == KEYCODE_MINUS)
		{
			if(ShiftKeyPressed())
			{
				return '_';
			}
			else
			{
				return '-';
			}
		}
		else if(code == KEYCODE_EQUALS)
		{
			if(ShiftKeyPressed())
			{
				return '+';
			}
			else
			{
				return '=';
			}
		}
		else if(code == KEYCODE_BACKSPACE)
		{
			return 0x08;
		}
		else if(code == KEYCODE_OPENBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '{';
			}
			else
			{
				return '[';
			}
		}
		else if(code == KEYCODE_CLOSEBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '}';
			}
			else
			{
				return ']';
			}
		}
		else if(code == KEYCODE_COLON)
		{
			if(ShiftKeyPressed())
			{
				return ':';
			}
			else
			{
				return ';';
			}
		}
		else if(code == KEYCODE_QUOTE)
		{
			if(ShiftKeyPressed())
			{
				return '\"';
			}
			else
			{
				return '\'';
			}
		}
		else if(code == KEYCODE_BACKSLASH)
		{
			if(ShiftKeyPressed())
			{
				return '|';
			}
			else
			{
				return '\\';
			}
		}
		else if(code == KEYCODE_COMMA)
		{
			if(ShiftKeyPressed())
			{
				return '<';
			}
			else
			{
				return ',';
			}
		}
		else if(code == KEYCODE_STOP)
		{
			if(ShiftKeyPressed())
			{
				return '>';
			}
			else
			{
				return '.';
			}
		}
		else if(code == KEYCODE_SLASH)
		{
			if(ShiftKeyPressed())
			{
				return '?';
			}
			else
			{
				return '/';
			}
		}
		else if(code == KEYCODE_SLASH_PAD)
		{
			return '/';
		}
		else if(code == KEYCODE_ASTERISK)
		{
			return '*';
		}
		else if(code == KEYCODE_MINUS_PAD)
		{
			return '-';
		}
		else if(code == KEYCODE_PLUS_PAD)
		{
			return '+';
		}
		else if(code == KEYCODE_SPACE)
		{
			return ' ';
		}
	}
}

#define osd_readkey_unicode ReadKeyAsync

#endif

//	a version of input_ui_pressed_repeat which increases speed as the key is
//	held down
static int UIPressedRepeatThrottle(int code, int baseSpeed)
{
	static int	lastCode = -1;
	static int	lastSpeed = -1;
	static int	incrementTimer = 0;
	int			pressed = 0;

	const int	kDelayRampTimer = 10;

	if(seq_pressed(input_port_type_seq(code)))
	{
		if(lastCode != code)
		{
			lastCode = code;
			lastSpeed = baseSpeed;
			incrementTimer = kDelayRampTimer * lastSpeed;
		}
		else
		{
			incrementTimer--;

			if(incrementTimer <= 0)
			{
				incrementTimer = kDelayRampTimer * lastSpeed;

				lastSpeed /= 2;
				if(lastSpeed < 1)
					lastSpeed = 1;

				pressed = 1;
			}
		}

		//usrintf_showmessage_secs(1, "%d %d", lastSpeed, incrementTimer);
	}
	else
	{
		if(lastCode == code)
		{
			lastCode = -1;
		}
	}

	return input_ui_pressed_repeat(code, lastSpeed);
}

//	returns if a cheat type is valid
static int IsCheatTypeSupported(int type)
{
	const int	* traverse;

	for(traverse = kSupportedCheatTypes; *traverse != -1; traverse++)
		if(*traverse == type)
			return 1;

	return 0;
}

#ifdef MESS

//	returns if a device with a specified CRC exists
static int MatchCRC(unsigned int crc)
{
	int type, id;

	if(!crc)
		return 1;

	for(type = 0; type < IO_COUNT; type++)
		for(id = 0; id < device_count(type); id++)
			if(crc == device_crc(type,id))
				return 1;

	return 0;
}
#endif

//	returns if a byte is BCD
static int IsBCD(int value)
{
	return	((value & 0x0F) <= 0x09) &&
			((value & 0xF0) <= 0x90);
}

/***************************************************************************

  patch_rom		Steph			2001.05.04
				Tourniquet		2001.05.10
				Ian Patterson	2001.06.06
								2001.06.12

  This patches ROM area by using a direct write to memory_region
  instead of using the write handlers

***************************************************************************/
void patch_rom(struct subcheat_struct * subcheat)
{
	data32_t	* ROM32 =	(UINT32 *)memory_region(REGION_CPU1+subcheat->cpu);
	data16_t	* ROM16 =	(UINT16 *)memory_region(REGION_CPU1+subcheat->cpu);
	UINT8		* ROM8 =	memory_region(REGION_CPU1+subcheat->cpu);

	UINT32 adr_modulo4 = subcheat->address & 0x3;
	UINT32 adr_modulo2 = subcheat->address & 0x1;

	UINT32 cheat_data = subcheat->data;

	//logerror("Endianess: %d Data: %d\n", cputype_endianess(cpu_type), computer_readmem_byte(subcheat->cpu,subcheat->address));

	if(cpunum_endianess(subcheat->cpu) == CPU_IS_BE)
	{
		// Big Endian
		switch(cpunum_databus_width(subcheat->cpu))
		{
			case 32:
				switch(adr_modulo4)
				{
					case 0:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0x00FFFFFF) | ((cheat_data << 24) & 0xFF000000);	break;
					case 1:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFF00FFFF) | ((cheat_data << 16) & 0x00FF0000);	break;
					case 2:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFFFF00FF) | ((cheat_data <<  8) & 0x0000FF00);	break;
					case 3:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFFFFFF00) | ((cheat_data <<  0) & 0x000000FF);	break;
				}
				break;

			case 16:
				switch(adr_modulo2)
				{
					case 0:	ROM16[subcheat->address/2] = (ROM16[subcheat->address/2] & 0x00FF) | ((cheat_data << 8) & 0xFF00);			break;
					case 1:	ROM16[subcheat->address/2] = (ROM16[subcheat->address/2] & 0xFF00) | ((cheat_data << 0) & 0x00FF);			break;
				}
				break;

			case 8:
				ROM8[subcheat->address] = cheat_data & 0xFF;
				break;

			default:
				break;
		}
	}
	else
	{
		// Little Endian
		switch(cpunum_databus_width(subcheat->cpu))
		{
			case 32:
				switch(adr_modulo4)
				{
					case 0:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFFFFFF00) | ((cheat_data <<  0) & 0x000000FF);	break;
					case 1:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFFFF00FF) | ((cheat_data <<  8) & 0x0000FF00);	break;
					case 2:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0xFF00FFFF) | ((cheat_data << 16) & 0x00FF0000);	break;
					case 3:	ROM32[subcheat->address/4] = (ROM32[subcheat->address/4] & 0x00FFFFFF) | ((cheat_data << 24) & 0xFF000000);	break;
				}
				break;

			case 16:
				switch(adr_modulo2)
				{
					case 0:	ROM16[subcheat->address/2] = (ROM16[subcheat->address/2] & 0xFF00) | ((cheat_data << 0) & 0x00FF);			break;
					case 1:	ROM16[subcheat->address/2] = (ROM16[subcheat->address/2] & 0x00FF) | ((cheat_data << 8) & 0xFF00);			break;
				}
				break;

			case 8:
				ROM8[subcheat->address] = cheat_data & 0xFF;
				break;

			default:
				break;
		}
	}

	//cpu_set_reset_line(subcheat->cpu, PULSE_LINE);	// reset cpu
	//machine_reset();									// reset _entire_ system
}

/***************************************************************************

  cheat_set_code

  Given a cheat code, sets the various attribues of the cheat structure.
  This is to aid in making the cheat engine more flexible in the event that
  someday the codes are restructured or the case statement in DoCheat is
  simplified from its current form.

***************************************************************************/
void cheat_set_code(struct subcheat_struct * subcheat, int code, int cheat_num)
{
	/* clear all flags (except mode flags) */
	CheatTable[cheat_num].flags &= ~kCheatFlagModeMask;
	subcheat->flags &= ~kSubcheatFlagModeMask;

	switch (code)
	{
		case kCheatSpecial_Poke1:
		case kCheatSpecial_Delay1:
		case kCheatSpecial_SetBit1:
		case kCheatSpecial_ResetBit1:
			subcheat->frames_til_trigger = 1 * Machine->drv->frames_per_second;
			break;

		case kCheatSpecial_Poke2:
		case kCheatSpecial_Delay2:
		case kCheatSpecial_SetBit2:
		case kCheatSpecial_ResetBit2:
			subcheat->frames_til_trigger = 2 * Machine->drv->frames_per_second;
			break;

		case kCheatSpecial_Poke5:
		case kCheatSpecial_Delay5:
		case kCheatSpecial_SetBit5:
		case kCheatSpecial_ResetBit5:
			subcheat->frames_til_trigger = 5 * Machine->drv->frames_per_second;
			break;

		case kCheatSpecial_Comment:
			subcheat->frames_til_trigger =	0;
			subcheat->address =				0;
			subcheat->data =				0;
			CheatTable[cheat_num].flags |=	kCheatFlagComment;
			break;

		case kCheatSpecial_Watch:
			subcheat->frames_til_trigger =	0;
			subcheat->data =				0;
			CheatTable[cheat_num].flags |=	kCheatFlagWatch;
			break;

		default:
			subcheat->frames_til_trigger =	0;
			break;
	}

	if(	(code == kCheatSpecial_PokeRemove) ||
		(code == kCheatSpecial_SetBitRemove) ||
		(code == kCheatSpecial_ResetBitRemove) ||
		(code == kCheatSpecial_PokeROMRemove) ||		/* Steph 2001.05.04 */
		(	(code >= kCheatSpecial_UserFirst) &&
			(code <= kCheatSpecial_UserLast)))
	{
		CheatTable[cheat_num].flags |=	kCheatFlagOneShot;
	}

	/* Set the minimum value */
	if(	(code == kCheatSpecial_m1d1c) ||
		(code == kCheatSpecial_m1d1bcdc) ||
		(code == kCheatSpecial_m1d1) ||
		(code == kCheatSpecial_m1d1bcd))
	{
		subcheat->min = 1;
	}
	else
	{
		subcheat->min = 0;
	}

	/* Set the maximum value */
	if(	(code >= kCheatSpecial_UserFirst) &&
		(code <= kCheatSpecial_UserLast))
	{
		subcheat->max = subcheat->data;
		subcheat->data = 0;
	}
	else
	{
		subcheat->max = 0xff;
	}

	/* Set prompting flags */
	if(	(	(code >= kCheatSpecial_m0d0c) &&
			(code <= kCheatSpecial_m1d1c)) ||
		(	(code >= kCheatSpecial_m0d0) &&
			(code <= kCheatSpecial_m1d1)))
	{
		CheatTable[cheat_num].flags |= kCheatFlagDecPrompt;
	}
	else if(	(	(code >= kCheatSpecial_m0d0bcdc) &&
					(code <= kCheatSpecial_m1d1bcdc)) ||
				(	(code >= kCheatSpecial_m0d0bcd) &&
					(code <= kCheatSpecial_m1d1bcd)))
	{
		CheatTable[cheat_num].flags |= kCheatFlagBCDPrompt;
	}

	if(	(	(code >= kCheatSpecial_SetBit) &&
			(code <= kCheatSpecial_SetBit5)) ||
		(	(code >= kCheatSpecial_ResetBit) &&
			(code <= kCheatSpecial_ResetBit5)))
	{
		subcheat->flags |= kSubcheatFlagBitModify;
	}

	if(	(	(code >= kCheatSpecial_Poke) &&
			(code <= kCheatSpecial_Backup4)) ||
			(code == kCheatSpecial_PokeROM) ||		/* Steph 2001.05.04 */
		(	(code >= kCheatSpecial_m0d0c) &&
			(code <= kCheatSpecial_m1d1bcd)))
	{
		subcheat->flags |= kSubcheatFlagByteModify;
	}

	if(code == kCheatSpecial_PokeROM)
	{
		subcheat->flags |= kSubcheatFlagCustomRegion;
	}

	subcheat->code = code;
}

/***************************************************************************

  cheat_set_status

  Given an index into the cheat table array, make the selected cheat
  either active or inactive.

***************************************************************************/
void cheat_set_status(int cheat_num, int active)
{
	int i, j, k;

	if(active) /* enable the cheat */
	{
		/* check for conflict */
		for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
		{
			UINT32	address =	CheatTable[cheat_num].subcheat[i].address;
			UINT8	data =		CheatTable[cheat_num].subcheat[i].data;
			UINT32	flags =		CheatTable[cheat_num].subcheat[i].flags;
			UINT8	conflict =	0;

			for(j = 0; j < LoadedCheatTotal; j++)
			{
				if(	(j != cheat_num) &&
					(CheatTable[j].flags & kCheatFlagActive))
				{
					for(k = 0; k <= CheatTable[j].num_sub; k++)
					{
						if(	(CheatTable[j].subcheat[k].address == address) &&
							(!(CheatTable[j].subcheat[k].flags & kSubcheatFlagCustomRegion)))
						{
							if(flags & kSubcheatFlagBitModify)
							{
								if(	(CheatTable[j].subcheat[k].flags & kSubcheatFlagByteModify) &&
									data)
								{
									conflict = 1;
								}

								if(	(CheatTable[j].subcheat[k].flags & kSubcheatFlagBitModify) &&
									(CheatTable[j].subcheat[k].data & data))
								{
									conflict = 1;
								}
							}

							if(flags & kSubcheatFlagByteModify)
							{
								if(CheatTable[j].subcheat[k].flags & kSubcheatFlagByteModify)
								{
									conflict = 1;
								}

								if(	(CheatTable[j].subcheat[k].flags & kSubcheatFlagBitModify) &&
									CheatTable[j].subcheat[k].data)
								{
									conflict = 1;
								}
							}

							if(conflict)
							{
								if(CheatTable[j].name && CheatTable[j].name[0])
								{
									usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_conflict_found), CheatTable[j].name);
								}
								else
								{
									usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_conflict_found), ui_getstring(UI_none));
								}

								return;
							}
						}
					}
				}
			}
		}

		for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
		{
			/* Reset the active variables */
			CheatTable[cheat_num].subcheat[i].frame_count =	0;
			CheatTable[cheat_num].subcheat[i].backup =		0;
			CheatTable[cheat_num].subcheat[i].flags &=		~kSubcheatFlagStatusMask;

			/* add to the watch list (if needed) */
			if(CheatTable[cheat_num].flags & kCheatFlagWatch)
			{
				int	freeWatch;

				freeWatch = FindFreeWatch();

				if(freeWatch != -1)
				{
					watches[freeWatch].cheat_num =	cheat_num;
					watches[freeWatch].address =	CheatTable[cheat_num].subcheat[i].address;
					watches[freeWatch].cpu =		CheatTable[cheat_num].subcheat[i].cpu;
					watches[freeWatch].num_bytes =	1;
					watches[freeWatch].label_type =	0;
					watches[freeWatch].label[0] =	0;

					is_watch_active = 1;
				}
			}
		}

		/* only add if there's a cheat active already */
		if(!(CheatTable[cheat_num].flags & kCheatFlagActive))
		{
			CheatTable[cheat_num].flags |= kCheatFlagActive;
			ActiveCheatTotal++;
		}

		/* tell the MAME core that we're cheaters! */
		he_did_cheat = 1;
	}
	else /* disable the cheat (case 0, 2) */
	{
		for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
		{
			/* Reset the active variables */
			CheatTable[cheat_num].subcheat[i].frame_count = 0;
			CheatTable[cheat_num].subcheat[i].backup = 0;

#ifdef MESS
			/* Put the original code if it is a code patch */
			if (CheatTable[cheat_num].patch == 'C')
				WRITE_OLD_CHEAT;
#endif
		}

		/* only add if there's a cheat active already */
		if(CheatTable[cheat_num].flags & kCheatFlagActive)
		{
			CheatTable[cheat_num].flags &= ~kCheatFlagActive;

			ActiveCheatTotal--;
		}

		/* disable watches associated with cheats */
		for(i = 0; i < MAX_WATCHES; i++)
		{
			if(watches[i].cheat_num == cheat_num)
			{
				watches[i].cheat_num =	0;
				watches[i].address =	0;
				watches[i].cpu =		0;
				watches[i].num_bytes =	0;
				watches[i].label_type =	0;
				watches[i].label[0] =	0;
			}
		}
	}
}

//	adds a cheat to the list at the specified index
void cheat_insert_new(int cheat_num)
{
	/* if list is full, bail */
	if(LoadedCheatTotal >= MAX_LOADEDCHEATS)
		return;

	/* if the index is off the end of the list, fix it */
	if(cheat_num > LoadedCheatTotal)
		cheat_num = LoadedCheatTotal;

	/* clear space in the middle of the table if needed */
	if(cheat_num < LoadedCheatTotal)
		memmove(	&CheatTable[cheat_num+1],
					&CheatTable[cheat_num],
					sizeof(struct cheat_struct) * (LoadedCheatTotal - cheat_num));

	/* clear the new entry */
	memset(&CheatTable[cheat_num], 0, sizeof(struct cheat_struct));

	CheatTable[cheat_num].name = malloc(strlen(ui_getstring(UI_none)) + 1);
	strcpy(CheatTable[cheat_num].name, ui_getstring(UI_none));

	CheatTable[cheat_num].comment = calloc(1, 1);

	CheatTable[cheat_num].subcheat = calloc(1, sizeof(struct subcheat_struct));

	CheatTable[cheat_num].activate_key = -1;

	/*add one to the total */
	LoadedCheatTotal++;
}

//	removes the cheat at the specified index
void cheat_delete(int cheat_num)
{
	/* if the index is off the end, make it the last one */
	if(cheat_num >= LoadedCheatTotal)
		cheat_num = LoadedCheatTotal - 1;

	/* deallocate storage for the cheat */
	free(CheatTable[cheat_num].name);
	free(CheatTable[cheat_num].comment);
	free(CheatTable[cheat_num].subcheat);

	/* If it's active, decrease the count */
	if(CheatTable[cheat_num].flags & kCheatFlagActive)
		ActiveCheatTotal--;

	/* move all the elements after this one up one slot if there are more than 1 and it's not the last */
	if((LoadedCheatTotal > 1) && (cheat_num < LoadedCheatTotal - 1))
		memmove(	&CheatTable[cheat_num],
					&CheatTable[cheat_num+1],
					sizeof(struct cheat_struct) * (LoadedCheatTotal - (cheat_num + 1)));

	/* knock one off the total */
	LoadedCheatTotal--;
}

//	saves the specified cheat to the first database file in the list
void cheat_save(int cheat_num)
{
	void	* theFile;
	char	buf[4096];
	int		i;
	int		code;
	int		data;

	theFile = osd_fopen(NULL, database, OSD_FILETYPE_CHEAT, 1);

	if(!theFile)
		return;

	for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
	{
		code = CheatTable[cheat_num].subcheat[i].code;
		data = CheatTable[cheat_num].subcheat[i].data;

		if(	(code >= kCheatSpecial_UserFirst) &&
			(code <= kCheatSpecial_UserLast))
		{
			data = CheatTable[cheat_num].subcheat[i].max;
		}

		if(i > 0)
		{
			code += kCheatSpecial_LinkStart;
		}

		#ifdef MESS

		if(CheatTable[cheat_num].comment && CheatTable[cheat_num].comment[0])
		{
			sprintf(	buf,
						"%s:%08X:%c:%d:%.*X:%02X:%02X:%03d:%s:%s\n",
						Machine->gamedrv->name,
						CheatTable[cheat_num].crc,
						CheatTable[cheat_num].patch,
						CheatTable[cheat_num].subcheat[i].cpu,
						CPUAddressWidth(CheatTable[cheat_num].subcheat[i].cpu),
						CheatTable[cheat_num].subcheat[i].address,
						data,
						CheatTable[cheat_num].subcheat[i].olddata,
						code,
						CheatTable[cheat_num].name,
						CheatTable[cheat_num].comment);
		}
		else
		{
			sprintf(	buf,
						"%s:%08X:%c:%d:%.*X:%02X:%02X:%03d:%s\n",
						Machine->gamedrv->name,
						CheatTable[cheat_num].crc,
						CheatTable[cheat_num].patch,
						CheatTable[cheat_num].subcheat[i].cpu,
						CPUAddressWidth(CheatTable[cheat_num].subcheat[i].cpu),
						CheatTable[cheat_num].subcheat[i].address,
						data,
						CheatTable[cheat_num].subcheat[i].olddata,
						code,
						CheatTable[cheat_num].name);
		}

		#else

		if(CheatTable[cheat_num].comment && CheatTable[cheat_num].comment[0])
		{
			sprintf(	buf,
						"%s:%d:%.*X:%02X:%03d:%s:%s\n",
						Machine->gamedrv->name,
						CheatTable[cheat_num].subcheat[i].cpu,
						CPUAddressWidth(CheatTable[cheat_num].subcheat[i].cpu),
						CheatTable[cheat_num].subcheat[i].address,
						data,
						code,
						CheatTable[cheat_num].name,
						CheatTable[cheat_num].comment);
		}
		else
		{
			sprintf(	buf,
						"%s:%d:%.*X:%02X:%03d:%s\n",
						Machine->gamedrv->name,
						CheatTable[cheat_num].subcheat[i].cpu,
						CPUAddressWidth(CheatTable[cheat_num].subcheat[i].cpu),
						CheatTable[cheat_num].subcheat[i].address,
						data,
						code,
						CheatTable[cheat_num].name);
		}

		#endif

		osd_fwrite(theFile, buf, strlen(buf));
	}

	osd_fclose(theFile);
}

//	adds a new subcheat to the specified cheat at the specified index
void subcheat_insert_new(int cheat_num, int subcheat_num)
{
	/* don't exceed MAX_SUBCHEATS */
	if((CheatTable[cheat_num].num_sub + 1) >= MAX_SUBCHEATS)
		return;

	/* if the index is off the end of the list, fix it */
	if(subcheat_num > CheatTable[cheat_num].num_sub)
		subcheat_num = CheatTable[cheat_num].num_sub;

	/* grow the subcheat table allocation */
	CheatTable[cheat_num].subcheat = realloc(CheatTable[cheat_num].subcheat, sizeof(struct subcheat_struct) * (CheatTable[cheat_num].num_sub + 2));
	if(CheatTable[cheat_num].subcheat == NULL)
		return;

	/* insert space in the middle of the table if needed */
	if(subcheat_num <= CheatTable[cheat_num].num_sub)
		memmove(	&CheatTable[cheat_num].subcheat[subcheat_num+1],
					&CheatTable[cheat_num].subcheat[subcheat_num],
					sizeof(struct subcheat_struct) * (CheatTable[cheat_num].num_sub + 1 - subcheat_num));

	/* clear the new entry */
	memset(	&CheatTable[cheat_num].subcheat[subcheat_num],
			0,
			sizeof(struct subcheat_struct));

	/*add one to the total */
	CheatTable[cheat_num].num_sub++;
}

//	removes a specified subcheat from the specified cheat
void subcheat_delete(int cheat_num, int subcheat_num)
{
	if(CheatTable[cheat_num].num_sub < 1)
		return;

	/* if the index is off the end, make it the last one */
	if(subcheat_num > CheatTable[cheat_num].num_sub)
		subcheat_num = CheatTable[cheat_num].num_sub;

	/* remove the element in the middle if it's not the last */
	if(subcheat_num < CheatTable[cheat_num].num_sub)
		memmove(	&CheatTable[cheat_num].subcheat[subcheat_num],
					&CheatTable[cheat_num].subcheat[subcheat_num+1],
					sizeof(struct subcheat_struct) * (CheatTable[cheat_num].num_sub - subcheat_num));

	/* shrink the subcheat table allocation */
	CheatTable[cheat_num].subcheat = realloc(CheatTable[cheat_num].subcheat, sizeof(struct subcheat_struct) * (CheatTable[cheat_num].num_sub));

	/* knock one off the total */
	CheatTable[cheat_num].num_sub--;
}

//	loads cheats from a specified file, optionally replacing all current cheats
void LoadCheatFile(int merge, char * filename)
{
	void					* f;
	char					curline[2048];
	int						name_length;
	struct subcheat_struct	* subcheat;
	int						sub = 0;
	int						temp;

	f = osd_fopen (NULL, filename, OSD_FILETYPE_CHEAT, 0);

	if(!f)
		return;

	if(!merge)
	{
		ActiveCheatTotal = 0;
		LoadedCheatTotal = 0;
	}

	name_length = strlen(Machine->gamedrv->name);

	/* Load the cheats for that game */
	/* Ex.: pacman:0:4E14:06:000:1UP Unlimited lives:Coded on 1 byte */
	while((osd_fgets (curline, 2048, f) != NULL) && (LoadedCheatTotal < MAX_LOADEDCHEATS))
	{
		char			* ptr;
		int				temp_cpu;
#ifdef MESS
		unsigned int	temp_crc;
		char			temp_patch;
#endif
		offs_t			temp_address;
		data8_t			temp_data;
#ifdef MESS
		data8_t			temp_olddata;
#endif
		INT32			temp_code;

		/* Take a few easy-out cases to speed things up */
		if(curline[name_length] != ':')
			continue;

		if(strncmp(curline, Machine->gamedrv->name, name_length))
			continue;

		if(curline[0] == ';')
			continue;

#if 0
		if(sologame)
			if(	(strstr(str, "2UP") != NULL) || (strstr(str, "PL2") != NULL) ||
				(strstr(str, "3UP") != NULL) || (strstr(str, "PL3") != NULL) ||
				(strstr(str, "4UP") != NULL) || (strstr(str, "PL4") != NULL) ||
				(strstr(str, "2up") != NULL) || (strstr(str, "pl2") != NULL) ||
				(strstr(str, "3up") != NULL) || (strstr(str, "pl3") != NULL) ||
				(strstr(str, "4up") != NULL) || (strstr(str, "pl4") != NULL))
			continue;
#endif

		/* Extract the fields from the line */
		/* Skip the driver name */
		ptr = strtok(curline, ":");
		if(!ptr)
			continue;

#ifdef MESS
		/* CRC */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%x", &temp_crc);
		if(!MatchCRC(temp_crc))
			continue;

		/* Patch */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%c", &temp_patch);
		if(temp_patch != 'C')
			temp_patch = 'D';

#endif

		/* CPU number */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%d", &temp_cpu);

//		/* skip if it's a sound cpu and the audio is off */
//		if (CPU_AUDIO_OFF(temp_cpu))
//			continue;

		/* skip if this is a bogus CPU */
		if(temp_cpu >= cpu_gettotalcpu())
			continue;

		/* Address */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%X", &temp_address);
		temp_address &= cpunum_address_mask(temp_cpu);

		/* data byte */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr,"%x", &temp);
		temp_data = temp;

		temp_data &= 0xff;

#ifdef MESS
		/* old data byte */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%x", &temp_olddata);
		temp_olddata &= 0xff;
#endif

		/* special code */
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		sscanf(ptr, "%d", &temp_code);

		/* Is this a subcheat? */
		if(	(temp_code >= kCheatSpecial_LinkStart) &&
			(temp_code <= kCheatSpecial_LinkEnd))
		{
			sub++;

			/* don't overflow */
			if((sub + 1) > MAX_SUBCHEATS)
				continue;

			/* Adjust the special flag */
			temp_code -= kCheatSpecial_LinkStart;

			/* point to the last valid main cheat entry */
			LoadedCheatTotal--;
		}
		else
		{
			/* no, make this the first cheat in the series */
			sub = 0;
		}

		/* Allocate (or resize) storage for the subcheat array */
		CheatTable[LoadedCheatTotal].subcheat = realloc(CheatTable[LoadedCheatTotal].subcheat, sizeof(struct subcheat_struct) * (sub + 1));
		if(CheatTable[LoadedCheatTotal].subcheat == NULL)
			continue;

		/* Store the current number of subcheats embodied by this code */
		CheatTable[LoadedCheatTotal].num_sub = sub;

		subcheat = &CheatTable[LoadedCheatTotal].subcheat[sub];

		/* Reset the cheat */
		subcheat->frames_til_trigger =	0;
		subcheat->frame_count =			0;
		subcheat->backup =				0;
		subcheat->flags =				0;

		/* Copy the cheat data */
		subcheat->cpu =					temp_cpu;
		subcheat->address =				temp_address;
		subcheat->data =				temp_data;

#ifdef MESS
		subcheat->olddata =				temp_olddata;
#endif

		cheat_set_code(subcheat, temp_code, LoadedCheatTotal);

		/* don't bother with the names & comments for subcheats */
		if(sub)
			goto next;

#ifdef MESS
		/* CRC */
		CheatTable[LoadedCheatTotal].crc =		temp_crc;
		/* Patch */
		CheatTable[LoadedCheatTotal].patch =	temp_patch;
#endif

		/* Disable the cheat */
		CheatTable[LoadedCheatTotal].flags &= ~kCheatFlagStatusMask;

		CheatTable[LoadedCheatTotal].activate_key = -1;

		/* cheat name */
		CheatTable[LoadedCheatTotal].name = NULL;
		ptr = strtok(NULL, ":");
		if(!ptr)
			continue;

		/* Allocate storage and copy the name */
		CheatTable[LoadedCheatTotal].name = malloc(strlen(ptr) + 1);
		strcpy(CheatTable[LoadedCheatTotal].name, ptr);

		/* Strip line-ending if needed */
		if(strstr(CheatTable[LoadedCheatTotal].name, "\n") != NULL)
			CheatTable[LoadedCheatTotal].name[strlen(CheatTable[LoadedCheatTotal].name) - 1] = 0;

		/* read the "comment" field if there */
		ptr = strtok(NULL, ":");
		if(ptr)
		{
			/* Allocate storage and copy the comment */
			CheatTable[LoadedCheatTotal].comment = malloc(strlen(ptr) + 1);
			strcpy(CheatTable[LoadedCheatTotal].comment, ptr);

			/* Strip line-ending if needed */
			if(strstr(CheatTable[LoadedCheatTotal].comment, "\n"))
				CheatTable[LoadedCheatTotal].comment[strlen(CheatTable[LoadedCheatTotal].comment) - 1] = 0;
		}
		else
		{
			CheatTable[LoadedCheatTotal].comment = calloc(1, sizeof(char));
		}

		next:

		LoadedCheatTotal++;
	}

	osd_fclose(f);
}

//	loads cheats from all database files
void LoadCheatFiles(void)
{
	char	* ptr;
	char	str[CHEAT_FILENAME_MAXLEN + 1];
	char	filename[CHEAT_FILENAME_MAXLEN + 1];
	int		pos1;
	int		pos2;

	ActiveCheatTotal = 0;
	LoadedCheatTotal = 0;

	/* start off with the default cheat file, cheat.dat */
	strcpy(str, cheatfile);
	ptr = strtok(str, ";");

	/* append any additional cheat files */
	strcpy(database, ptr);
	strcpy(str, cheatfile);
	str[strlen(str) + 1] = 0;

	pos1 = 0;
	while(str[pos1])
	{
		pos2 = pos1;

		while((str[pos2]) && (str[pos2] != ';'))
			pos2++;

		if(pos1 != pos2)
		{
			memset(filename, '\0', sizeof(filename));
			strncpy(filename, &str[pos1], pos2 - pos1);

			LoadCheatFile(1, filename);

			pos1 = pos2 + 1;
		}
	}
}

//	inits the cheat engine
void InitCheat(void)
{
	int i;

	he_did_cheat = 0;
	CheatEnabled = 1;
	searchCPU = 0;

	/* set up the search tables */
	reset_table(StartRam);
	reset_table(BackupRam);
	reset_table(FlagTable);
	reset_table(OldBackupRam);
	reset_table(OldFlagTable);

	restoreStatus = kRestore_NoInit;

	/* Reset the watchpoints to their defaults */
	is_watch_active = 0;
	is_watch_visible = 1;

	for(i = 0; i < MAX_WATCHES; i++)
	{
		/* disable this watchpoint */
		watches[i].num_bytes =	0;

		watches[i].cheat_num =	-1;

		watches[i].cpu =		0;
		watches[i].label[0] =	0;
		watches[i].label_type =	0;
		watches[i].address =	0;

		/* set the screen position */
		watches[i].x =			0;
		watches[i].y =			i * Machine->uifontheight;
	}

	LoadCheatFiles();
	SetupDefaultMemoryAreas(searchCPU);
}

#ifdef macintosh
#pragma mark -
#endif

//	shows the "Enable/Disable a Cheat" menu
INT32 EnableDisableCheatMenu(struct osd_bitmap *bitmap, INT32 selected)
{
	INT32			sel;
	static INT32	submenu_choice;
	const char		* menu_item[MAX_LOADEDCHEATS + 4];
	const char		* menu_subitem[MAX_LOADEDCHEATS + 4];
	char			buf[MAX_LOADEDCHEATS][80];
	char			buf2[MAX_LOADEDCHEATS][10];
	INT32			i;
	INT32			total = 0;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if(submenu_choice)
	{
		switch(submenu_choice)
		{
			case 1:
				submenu_choice = CommentMenu(bitmap, submenu_choice, sel);
				break;

			case 2:
				submenu_choice = UserSelectValueMenu(bitmap, submenu_choice, sel);
				break;
		}

		if(submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	for(i = 0; i < LoadedCheatTotal; i++)
	{
		int string_num;

		if(CheatTable[i].comment && CheatTable[i].comment[0])
		{
			sprintf(buf[total], "%s (%s...)", CheatTable[i].name, ui_getstring (UI_moreinfo));
		}
		else
		{
			sprintf(buf[total], "%s", CheatTable[i].name);
		}

		menu_item[total] = buf[total];

		/* add submenu options for all cheats that are not comments */
		if(!(CheatTable[i].flags & kCheatFlagComment))
		{
			if(CheatTable[i].flags & kCheatFlagOneShot)
			{
				string_num = UI_set;
			}
			else
			{
				if(CheatTable[i].flags & kCheatFlagActive)
				{
					string_num = UI_on;
				}
				else
				{
					string_num = UI_off;
				}
			}

			sprintf(buf2[total], "%s", ui_getstring(string_num));
		}
		else
		{
			buf2[total][0] = 0;
		}

		menu_subitem[total] = buf2[total];

		total++;
	}

	menu_item[total] = ui_getstring(UI_returntoprior);
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = 0;	/* terminate array */

	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	while(	(sel < total - 1) &&
			CheatTable[sel].flags & kCheatFlagComment)
			sel++;

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;

		while(	(sel < total - 1) &&
				CheatTable[sel].flags & kCheatFlagComment)
			sel++;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
		{
			sel = total - 1;
		}
		else
		{
			while(	(sel != total - 1) &&
					CheatTable[sel].flags & kCheatFlagComment)
			{
				sel--;

				if(sel < 0)
					sel = total - 1;
			}
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_UP, kVerticalKeyRepeatRate))
	{
		sel -= Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel < 0)
		{
			sel = 0;

			while(	(sel < total - 1) &&
					CheatTable[sel].flags & kCheatFlagComment)
				sel++;
		}
		else
		{
			while(	(sel != total - 1) &&
					CheatTable[sel].flags & kCheatFlagComment)
			{
				sel--;

				if(sel < 0)
					sel = total - 1;
			}
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_DOWN, kVerticalKeyRepeatRate))
	{
		sel += Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel >= total)
		{
			sel = total - 1;
		}
		else
		{
			while(	(sel < total - 1) &&
					CheatTable[sel].flags & kCheatFlagComment)
				sel++;
		}
	}

	if((UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate)) || (UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate)))
	{
		if(	(sel < (total - 1)) &&
			!(CheatTable[sel].flags & kCheatFlagComment) &&
			!(CheatTable[sel].flags & kCheatFlagOneShot))
		{
			int active = CheatTable[sel].flags & kCheatFlagActive;

			active ^= 0x01;

			/* get the user's selected value if needed */
			if((CheatTable[sel].flags & kCheatFlagPromptMask) && active)
			{
				submenu_choice = 2;
				schedule_full_refresh();
			}
			else
			{
				cheat_set_status(sel, active);

				CheatEnabled = 1;
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (total - 1))
		{
			/* return to prior menu */
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			/* enable if it's a one-shot */
			if(CheatTable[sel].flags & kCheatFlagOneShot)
			{
				if(CheatTable[sel].flags & kCheatFlagPromptMask)
				{
					submenu_choice = 2;
					schedule_full_refresh();
				}
				else
				{
					cheat_set_status(sel, 1);

					CheatEnabled = 1;
				}
			}
			else
			{
				/* read comment if it's not */
				if(CheatTable[sel].comment && (CheatTable[sel].comment[0] != 0x00))
				{
					submenu_choice = 1;
					schedule_full_refresh();
				}
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
	{
		/* save the cheat at the current position (or the end) */
		if(sel < total - 1)
		{
			cheat_save(sel);
		}
	}

	if(input_ui_pressed(IPT_UI_WATCH_VALUE))
	{
		if(sel < total - 1)
		{
			int	freeWatch;

			freeWatch = FindFreeWatch();

			if(freeWatch != -1)
			{
				watches[freeWatch].cheat_num =	-1;
				watches[freeWatch].address =	CheatTable[sel].subcheat[0].address;
				watches[freeWatch].cpu =		CheatTable[sel].subcheat[0].cpu;
				watches[freeWatch].num_bytes =	1;
				watches[freeWatch].label_type =	0;
				watches[freeWatch].label[0] =	0;

				is_watch_active = 1;
			}
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	lets the user select a value for selection cheats
static INT32 UserSelectValueMenu(struct osd_bitmap * bitmap, int selection, int cheat_num)
{
	char					buf[2048];
	int						sel;
	struct cheat_struct		* cheat;
	struct subcheat_struct	* subcheat;
	static int				value = -1;
	int						delta = 0;

	sel =		selection - 1;

	cheat =		&CheatTable[cheat_num];
	subcheat =	&cheat->subcheat[0];

	if(value == -1)
		value = subcheat->data;

	sprintf(buf, "\t%s\n\t%.2X (%d)\n", ui_getstring(UI_search_select_value), value, value);

	/* menu system, use the normal menu keys */
	strcat(buf, "\t");
	strcat(buf, ui_getstring(UI_lefthilight));
	strcat(buf, " ");
	strcat(buf, ui_getstring(UI_OK));
	strcat(buf, " ");
	strcat(buf, ui_getstring(UI_righthilight));

	ui_displaymessagewindow(bitmap, buf);

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		delta = -1;
	}
	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		delta = 1;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(value != -1)
			subcheat->data = value;

		cheat_set_status(cheat_num, 1);

		CheatEnabled = 1;

		value = -1;
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CONFIGURE))
	{
		sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	if(delta)
	{
		int	count = 0;

		do
		{
			value += delta;

			if(value < subcheat->min)
			{
				value = subcheat->max;
			}
			if(value > subcheat->max)
			{
				value = subcheat->min;
			}

			if(delta > 0)
				delta = 1;
			else
				delta = -1;

			count++;
		}
		while(	(cheat->flags & kCheatFlagBCDPrompt) &&
				!IsBCD(value) &&
				(count < 1000));
	}

	return sel + 1;
}

//	shows a cheat's comment
static INT32 CommentMenu(struct osd_bitmap * bitmap, INT32 selected, int cheat_index)
{
	char	buf[2048];
	int		sel;

	sel = selected - 1;

	buf[0] = 0;

	if(CheatTable[cheat_index].comment[0])
	{
		sprintf(buf, "\t%s\n\t%s\n\n", ui_getstring (UI_moreinfoheader), CheatTable[cheat_index].name);

		strcat(buf, CheatTable[cheat_index].comment);
	}
	else
	{
		sel = -1;
		buf[0] = 0;
	}

	/* menu system, use the normal menu keys */
	strcat(buf, "\n\n\t");
	strcat(buf, ui_getstring (UI_lefthilight));
	strcat(buf, " ");
	strcat(buf, ui_getstring (UI_returntoprior));
	strcat(buf, " ");
	strcat(buf, ui_getstring (UI_righthilight));

	ui_displaymessagewindow(bitmap, buf);

	if(input_ui_pressed(IPT_UI_SELECT))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the "Add/Edit a Cheat" menu
INT32 AddEditCheatMenu(struct osd_bitmap * bitmap, INT32 selected)
{
	int				sel;
	static INT32	submenu_choice;
	const char		* menu_item[MAX_LOADEDCHEATS + 4];
	int				i;
	int				total;

	total = 0;
	sel = selected - 1;

	for(i = 0; i < LoadedCheatTotal; i++)
	{
		menu_item[total++] = CheatTable[i].name;
	}

	/* If a submenu has been selected, go there */
	if(submenu_choice)
	{
		submenu_choice = EditCheatMenu(bitmap, submenu_choice, sel);

		if(submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	menu_item[total++] =	ui_getstring (UI_returntoprior);
	//menu_item[total] =		NULL;	/* TODO: add help string */
	menu_item[total] =	0;		/* terminate array */

	ui_displaymenu(bitmap, menu_item, 0, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_ADD_CHEAT, 8))
	{
		/* add a new cheat at the current position (or the end) */
		if(sel < total - 1)
		{
			cheat_insert_new(sel);
		}
		else
		{
			cheat_insert_new(LoadedCheatTotal);
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DELETE_CHEAT, 4))
	{
		if(LoadedCheatTotal)
		{
			/* delete the selected cheat (or the last one) */
			if(sel < total - 1)
			{
				cheat_delete(sel);
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
	{
		/* save the cheat at the current position (or the end) */
		if(sel < total - 1)
		{
			cheat_save(sel);
		}
	}

	if(input_ui_pressed(IPT_UI_WATCH_VALUE))
	{
		if(sel < total - 1)
		{
			int	freeWatch;

			freeWatch = FindFreeWatch();

			if(freeWatch != -1)
			{
				watches[freeWatch].cheat_num =	-1;
				watches[freeWatch].address =	CheatTable[sel].subcheat[0].address;
				watches[freeWatch].cpu =		CheatTable[sel].subcheat[0].cpu;
				watches[freeWatch].num_bytes =	1;
				watches[freeWatch].label_type =	0;
				watches[freeWatch].label[0] =	0;

				is_watch_active = 1;
			}
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_UP, kVerticalKeyRepeatRate))
	{
		sel -= Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel < 0)
		{
			sel = 0;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_DOWN, kVerticalKeyRepeatRate))
	{
		sel += Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel >= total)
		{
			sel = total - 1;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (total - 1))
		{
			/* return to prior menu */
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the cheat editing menu
static INT32 EditCheatMenu(struct osd_bitmap * bitmap, INT32 selected, UINT8 cheat_num)
{
	enum
	{
		kMenu_Name = 0,
		kMenu_Description,
		kMenu_ActivationKey,
		kMenu_Subcheat_Base,

		kMenu_Offset_CPU = 0,
		kMenu_Offset_Address,
		kMenu_Offset_Value,
		kMenu_Offset_Code,
		kMenu_Offset_Max
	};

	char * kKeycodeTable[] =
	{
		"A",		"B",		"C",		"D",		"E",		"F",
		"G",		"H",		"I",		"J",		"K",		"L",
		"M",		"N",		"O",		"P",		"Q",		"R",
		"S",		"T",		"U",		"V",		"W",		"X",
		"Y",		"Z",		"0",		"1",		"2",		"3",
		"4",		"5",		"6",		"7",		"8",		"9",
		"[0]",		"[1]",		"[2]",		"[3]",		"[4]",
		"[5]",		"[6]",		"[7]",		"[8]",		"[9]",
		"F1",		"F2",		"F3",		"F4",		"F5",
		"F6",		"F7",		"F8",		"F9",		"F10",
		"F11",		"F12",
		"ESC",		"~",		"-",		"=",		"BACKSPACE",
		"TAB",		"[",		"]",		"ENTER",	":",
		"\'",		"\\",		"\\",		",",		".",
		"/",		"SPACE",	"INS",		"DEL",
		"HOME",		"END",		"PGUP",		"PGDN",		"LEFT",
		"RIGHT",	"UP",		"DOWN",
		"[/]",		"[*]",		"[-]",		"[+]",
		"[DEL]",	"[ENT]",	"PTSCR",	"PAUSE",
		"LSHIFT",	"RSHIFT",	"LCONTROL",	"RCONTROL",
		"LALT",		"RALT",		"SCRLLK",	"NUMLK",	"CAPSLK",
		"LWIN",		"RWIN",		"MENU"
	};

	#define kMaxMenuItems (kMenu_Subcheat_Base + (kMenu_Offset_Max * MAX_SUBCHEATS) + 2)
							// includes terminating zero

	int						sel;
	int						total;
	int						total2;
	struct subcheat_struct	* subcheat;
	static INT32			submenu_choice;
	static UINT8			textedit_active;
	const char				* menu_item[kMaxMenuItems];
	const char				* menu_subitem[kMaxMenuItems];
	char					setting[kMaxMenuItems][30];
	char					flag[kMaxMenuItems];
	int						arrowize;
	int						subcheat_num;
	int						i;
	static int				currentNameTemplate = 0;
	int						lastSubcheatItem;
	int						selectedSubcheatItem;
	int						selectedSubcheat;
	int						cancelKeyPressed = 0;
	int						activateKeyPressed = 0;

	sel =					selected - 1;
	total =					0;
	lastSubcheatItem =		((CheatTable[cheat_num].num_sub + 1) * kMenu_Offset_Max) + (kMenu_Subcheat_Base - 1);
	selectedSubcheatItem =	(sel - kMenu_Subcheat_Base) % kMenu_Offset_Max;
	selectedSubcheat =		(sel - kMenu_Subcheat_Base) / kMenu_Offset_Max;

	menu_item[total++] = ui_getstring(UI_cheatname);
	menu_item[total++] = ui_getstring(UI_cheatdescription);
	menu_item[total++] = ui_getstring(UI_cheatactivationkey);

	for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
	{
		menu_item[total++] = ui_getstring(UI_cpu);
		menu_item[total++] = ui_getstring(UI_address);

		if(CheatTable[cheat_num].flags & kCheatFlagPromptMask)
		{
			menu_item[total++] = ui_getstring(UI_max);
		}
		else
		{
			menu_item[total++] = ui_getstring(UI_value);
		}

		menu_item[total++] = ui_getstring(UI_code);
	}

	menu_item[total++] = ui_getstring(UI_returntoprior);
	menu_item[total] = 0;

	arrowize = 0;

	/* set up the submenu selections */
	total2 = 0;

	for(i = 0; i < kMaxMenuItems; i++)
		flag[i] = 0;

	/* if we're editing the label, make it inverse */
	if(textedit_active)
		flag[sel] = 1;

	/* name */
	if(CheatTable[cheat_num].name && CheatTable[cheat_num].name[0])
	{
		sprintf(setting[total2], "%s", CheatTable[cheat_num].name);
	}
	else
	{
		strcpy(setting[total2], ui_getstring(UI_none));
	}

	menu_subitem[total2] = setting[total2];
	total2++;

	/* comment */
	if(CheatTable[cheat_num].comment && CheatTable[cheat_num].comment[0])
	{
		strcpy(setting[total2], CheatTable[cheat_num].comment);
	}
	else
	{
		strcpy(setting[total2], ui_getstring(UI_none));
	}

	menu_subitem[total2] = setting[total2];
	total2++;

	if(	(CheatTable[cheat_num].activate_key >= __code_key_first) &&
		(CheatTable[cheat_num].activate_key <= __code_key_last))
	{
		menu_subitem[total2] = kKeycodeTable[CheatTable[cheat_num].activate_key - __code_key_first];
	}
	else
	{
		menu_subitem[total2] = ui_getstring(UI_none);
	}

	total2++;

	/* Subcheats */
	for(i = 0; i <= CheatTable[cheat_num].num_sub; i++)
	{
		subcheat = &CheatTable[cheat_num].subcheat[i];

		/* cpu number */
		sprintf(setting[total2], "%d", subcheat->cpu);

		menu_subitem[total2] = setting[total2];
		total2++;

		/* address */
		sprintf(setting[total2], "%.*X", CPUAddressWidth(subcheat->cpu), subcheat->address);

		menu_subitem[total2] = setting[total2];
		total2++;

		if(CheatTable[cheat_num].flags & kCheatFlagPromptMask)
		{
			/* max */
			sprintf(setting[total2], "%.2X (%d)", subcheat->max, subcheat->max);

			menu_subitem[total2] = setting[total2];
			total2++;
		}
		else
		{
			/* value */
			sprintf(setting[total2], "%.2X (%d)", subcheat->data, subcheat->data);

			menu_subitem[total2] = setting[total2];
			total2++;
		}

		/* code */
		sprintf(setting[total2], "%d", subcheat->code);

		menu_subitem[total2] = setting[total2];
		total2++;

		menu_subitem[total2] = NULL;
	}

	ui_displaymenu(bitmap, menu_item, menu_subitem, flag, sel, arrowize);

	if(!textedit_active && UIPressedRepeatThrottle(IPT_UI_ADD_CHEAT, 8))
	{
		if((sel >= kMenu_Subcheat_Base) && (sel <= lastSubcheatItem))
		{
			subcheat_num = selectedSubcheat;
		}
		else
		{
			subcheat_num = CheatTable[cheat_num].num_sub;
		}

		/* add a new subcheat at the current position (or the end) */
		subcheat_insert_new(cheat_num, subcheat_num);
	}

	if(!textedit_active && UIPressedRepeatThrottle(IPT_UI_DELETE_CHEAT, 8))
	{
		if((sel >= kMenu_Subcheat_Base) && (sel <= lastSubcheatItem))
		{
			subcheat_num = selectedSubcheat;
		}
		else
		{
			subcheat_num = CheatTable[cheat_num].num_sub;
		}

		if(CheatTable[cheat_num].num_sub > 0)
		{
			subcheat_delete(cheat_num, subcheat_num);
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel = (sel + 1) % total;

		textedit_active = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel = (sel + total - 1) % total;

		textedit_active = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		if(sel == kMenu_Name)
		{
			currentNameTemplate--;

			/* wrap to last name */
			if(currentNameTemplate < 0)
			{
				currentNameTemplate = 0;

				while(kCheatNameTemplates[currentNameTemplate + 1][0])
					currentNameTemplate++;
			}

			CheatTable[cheat_num].name = realloc(CheatTable[cheat_num].name, strlen(kCheatNameTemplates[currentNameTemplate]) + 1);
			strcpy(CheatTable[cheat_num].name, kCheatNameTemplates[currentNameTemplate]);
		}
		else if((sel >= kMenu_Subcheat_Base) && (sel <= lastSubcheatItem))
		{
			int	increment;
			int	tempCode;

			if(AltKeyPressed())
			{
				increment = 0x10;
			}
			else
			{
				increment = 1;
			}

			subcheat = &CheatTable[cheat_num].subcheat[selectedSubcheat];

			switch(selectedSubcheatItem)
			{
				case kMenu_Offset_CPU:
					subcheat->cpu--;

					/* skip audio CPUs when the sound is off */
					while((subcheat->cpu >= 0) && CPU_AUDIO_OFF(subcheat->cpu))
						subcheat->cpu--;

					if(subcheat->cpu < 0)
						subcheat->cpu = cpu_gettotalcpu() - 1;

					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;

				case kMenu_Offset_Address:
					if(increment == 0x10)
						increment = 0x100;

					textedit_active = 0;

					subcheat->address -= increment;

					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;

				case kMenu_Offset_Value:
					textedit_active = 0;

					if(CheatTable[cheat_num].flags & kCheatFlagPromptMask)
					{
						subcheat->max -= increment;

						subcheat->max &= 0xff;
					}
					else
					{
						subcheat->data -= increment;

						subcheat->data &= 0xff;
					}
					break;

				case kMenu_Offset_Code:
					textedit_active = 0;

					tempCode = subcheat->code;

					do
					{
						tempCode--;

						if(tempCode < 0)
							tempCode = kCheatSpecial_Comment;
					}
					while(!IsCheatTypeSupported(tempCode));

					cheat_set_code(subcheat, tempCode, cheat_num);
					break;
			}
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		if(sel == kMenu_Name)
		{
			currentNameTemplate++;

			/* wrap to first name */
			if(!kCheatNameTemplates[currentNameTemplate][0])
				currentNameTemplate = 0;

			CheatTable[cheat_num].name = realloc(CheatTable[cheat_num].name, strlen(kCheatNameTemplates[currentNameTemplate]) + 1);
			strcpy(CheatTable[cheat_num].name, kCheatNameTemplates[currentNameTemplate]);
		}
		else if((sel >= kMenu_Subcheat_Base) && (sel <= lastSubcheatItem))
		{
			int	increment;
			int	tempCode;

			if(AltKeyPressed())
				increment = 0x10;
			else
				increment = 1;

			subcheat = &CheatTable[cheat_num].subcheat[selectedSubcheat];

			switch(selectedSubcheatItem)
			{
				case kMenu_Offset_CPU:
					subcheat->cpu++;

					/* skip audio CPUs when the sound is off */
					while((subcheat->cpu < cpu_gettotalcpu()) && CPU_AUDIO_OFF(subcheat->cpu))
						subcheat->cpu++;

					if(subcheat->cpu >= cpu_gettotalcpu())
						subcheat->cpu = 0;

					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;

				case kMenu_Offset_Address:
					if(increment == 0x10)
						increment = 0x100;

					textedit_active = 0;

					subcheat->address += increment;

					subcheat->address &= cpunum_address_mask(subcheat->cpu);
					break;

				case kMenu_Offset_Value:
					textedit_active = 0;

					if(CheatTable[cheat_num].flags & kCheatFlagPromptMask)
					{
						subcheat->max += increment;

						subcheat->max &= 0xff;
					}
					else
					{
						subcheat->data += increment;

						subcheat->data &= 0xff;
					}
					break;

				case kMenu_Offset_Code:
					textedit_active = 0;

					tempCode = subcheat->code;

					do
					{
						tempCode++;

						if(tempCode > kCheatSpecial_Comment)
							tempCode = 0;
					}
					while(!IsCheatTypeSupported(tempCode));

					cheat_set_code(subcheat, tempCode, cheat_num);
					break;
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (lastSubcheatItem + 1))
		{
			/* return to main menu */
			submenu_choice = 0;
			sel = -1;
		}
		else if(	(selectedSubcheatItem == kMenu_Offset_Address) ||
					(sel == kMenu_Name) ||
					(sel == kMenu_Description) ||
					(sel == kMenu_ActivationKey))
		{
			/* wait for key up */
			while(input_ui_pressed(IPT_UI_SELECT)) ;

			/* flush the text buffer */
			osd_readkey_unicode (1);
			textedit_active ^= 1;
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}

		activateKeyPressed = 1;
	}

	/* After we've weeded out any control characters, look for text */
	if(textedit_active)
	{
		int code;

		if(sel == kMenu_ActivationKey)
		{
			if(input_ui_pressed(IPT_UI_CANCEL))
			{
				CheatTable[cheat_num].activate_key = -1;

				cancelKeyPressed = 1;

				textedit_active = 0;
			}
			else
			{
				if(!activateKeyPressed)
				{
					code = code_read_async();

					if((code != CODE_NONE) && (code != KEYCODE_ENTER))
					{
						CheatTable[cheat_num].activate_key = code;
					}
				}
			}
		}
		else if((sel == kMenu_Name) && CheatTable[cheat_num].name)
		{
			int length = strlen(CheatTable[cheat_num].name);

			code = osd_readkey_unicode(0) & 0xff; /* no 16-bit support */

			if(code == 0x08) /* backspace */
			{
				if(length > 0)
				{
					CheatTable[cheat_num].name[length - 1] = 0;
				}
			}
			else if(isprint(code))
			{
				/* append the character */
				CheatTable[cheat_num].name = realloc(CheatTable[cheat_num].name, length + 2);
				if (CheatTable[cheat_num].name)
				{
					CheatTable[cheat_num].name[length] = code;
					CheatTable[cheat_num].name[length+1] = 0;
				}
			}
		}
		else if((sel == kMenu_Description) && CheatTable[cheat_num].comment)
		{
			int length = strlen(CheatTable[cheat_num].comment);

			code = osd_readkey_unicode(0) & 0xff; /* no 16-bit support */

			if(code == 0x08) /* backspace */
			{
				if(length > 0)
				{
					CheatTable[cheat_num].comment[length - 1] = 0;
				}
			}
			else if(isprint(code))
			{
				/* append the character */
				CheatTable[cheat_num].comment = realloc(CheatTable[cheat_num].comment, length + 2);
				if(CheatTable[cheat_num].comment)
				{
					CheatTable[cheat_num].comment[length] = code;
					CheatTable[cheat_num].comment[length+1] = 0;
				}
			}
		}
		else if(selectedSubcheatItem == kMenu_Offset_Address)
		{
			INT8 hex_val;

			subcheat = &CheatTable[cheat_num].subcheat[selectedSubcheat];

			/* see if a hex digit was typed */
			hex_val = code_read_hex_async();
			if(hex_val != -1)
			{
				/* shift over one digit, add in the new value and clip */
				subcheat->address <<= 4;
				subcheat->address |= hex_val;
				subcheat->address &= cpunum_address_mask(subcheat->cpu);
			}
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		sel = -1;
	}

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		textedit_active = 0;

		/* flush the text buffer */
		osd_readkey_unicode (1);
		schedule_full_refresh();
	}

	return sel + 1;
}

#ifdef macintosh
#pragma mark -
#endif

/* make a copy of a source ram table to a dest. ram table */
static void copy_ram(struct ExtMemory * dest, struct ExtMemory * src)
{
	struct ExtMemory	* ext_dest,
						* ext_src;

	for(ext_src = src, ext_dest = dest; ext_src->data; ext_src++, ext_dest++)
	{
		memcpy(ext_dest->data, ext_src->data, ext_src->end - ext_src->start + 1);
	}
}

/* make a copy of each ram area from search CPU ram to the specified table */
static void backup_ram(struct ExtMemory * table, int cpu)
{
	struct ExtMemory	* ext;

	for(ext = table; ext->data; ext++)
	{
		int i;

		for(i=0; i <= ext->end - ext->start; i++)
			ext->data[i] = computer_readmem_byte(cpu, i+ext->start);
	}
}

/* set every byte in specified table to data */
static void memset_ram(struct ExtMemory * table, unsigned char data)
{
	struct ExtMemory	* ext;

	for(ext = table; ext->data; ext++)
		memset(ext->data, data, ext->end - ext->start + 1);
}

/* free all the memory and init the table */
static void reset_table(struct ExtMemory * table)
{
	struct ExtMemory	* ext;

	for(ext = table; ext->data; ext++)
		free(ext->data);

	memset(table, 0, sizeof(struct ExtMemory) * MAX_EXT_MEMORY);
}

/* create tables for storing copies of all MWA_RAM areas */
static int build_tables(int cpu)
{
	const struct Memory_WriteAddress	* mwa = Machine->drv->cpu[cpu].memory_write;
	int									region = REGION_CPU1 + cpu;
	struct ExtMemory					* ext_sr = StartRam;
	struct ExtMemory					* ext_br = BackupRam;
	struct ExtMemory					* ext_ft = FlagTable;
	struct ExtMemory					* ext_obr = OldBackupRam;
	struct ExtMemory					* ext_oft = OldFlagTable;
	static int							bail = 0;			/* set to 1 if this routine fails during startup */
	int									MemoryNeeded = 0;	/* Trap memory allocation errors */
	int									count = 0;

	/* free memory that was previously allocated if no error occured */
	/* it must also be there because mwa varies from one CPU to another */
	if(!bail)
	{
		reset_table(StartRam);
		reset_table(BackupRam);
		reset_table(FlagTable);

		reset_table(OldBackupRam);
		reset_table(OldFlagTable);
	}

	bail = 0;

	while(!IS_MEMPORT_END(mwa))
	{
		if(!IS_MEMPORT_MARKER(mwa))
		{
			int size = (mwa->end - mwa->start) + 1;

			if(!memoryRegionEnabled[count++])
			{
				mwa++;

				continue;
			}

			/* time to allocate */
			if(!bail)
			{
				ext_sr->data =	malloc (size);
				ext_br->data =	malloc (size);
				ext_ft->data =	malloc (size);

				ext_obr->data =	malloc (size);
				ext_oft->data =	malloc (size);

				if(ext_sr->data == NULL)
				{
					bail = 1;

					MemoryNeeded += size;
				}

				if(ext_br->data == NULL)
				{
					bail = 1;

					MemoryNeeded += size;
				}

				if(ext_ft->data == NULL)
				{
					bail = 1;

					MemoryNeeded += size;
				}

				if(ext_obr->data == NULL)
				{
					bail = 1;

					MemoryNeeded += size;
				}

				if(ext_oft->data == NULL)
				{
					bail = 1;

					MemoryNeeded += size;
				}

				if(!bail)
				{
					ext_sr->start =		ext_br->start =		ext_ft->start =		mwa->start;
					ext_sr->end =		ext_br->end =		ext_ft->end =		mwa->end;
					ext_sr->region =	ext_br->region =	ext_ft->region =	region;
					ext_sr++;
					ext_br++;
					ext_ft++;

					ext_obr->start =	ext_oft->start =	mwa->start;
					ext_obr->end =		ext_oft->end =		mwa->end;
					ext_obr->region =	ext_oft->region =	region;
					ext_obr++;
					ext_oft++;
				}
			}
			else
			{
				MemoryNeeded += 5 * size;
			}
		}

		mwa++;
	}

	/* free memory that was previously allocated if an error occured */
	if(bail)
	{
		reset_table(StartRam);
		reset_table(BackupRam);
		reset_table(FlagTable);

		reset_table(OldBackupRam);
		reset_table(OldFlagTable);
	}

	return bail;
}

/*****************
 * Start a cheat search
 * If the method 1 is selected, ask the user a number
 * In all cases, backup the ram.
 *
 * Ask the user to select one of the following:
 *	1 - Lives or other number (byte) (exact)        ask a start value, ask new value
 *	2 - Timers (byte) (+ or - X)                    nothing at start,  ask +-X
 *	3 - Energy (byte) (less, equal or greater)	    nothing at start,  ask less, equal or greater
 *	4 - Status (bit)  (true or false)               nothing at start,  ask same or opposite
 *	5 - Slow but sure (Same as start or different)  nothing at start,  ask same or different
 *
 * Another method is used in the Pro action Replay the Energy method
 *	you can tell that the value is now 25%/50%/75%/100% as the start
 *	the problem is that I probably cannot search for exactly 50%, so
 *	that do I do? search +/- 10% ?
 * If you think of other way to search for codes, let me know.
 */

INT32 StartSearch(struct osd_bitmap *bitmap, INT32 selected)
{
	enum
	{
		Menu_CPU = 0,
		Menu_Value,
		Menu_Time,
		Menu_Energy,
		Menu_Bit,
		Menu_Slow,
		Menu_Return,
		Menu_Total
	};

	const char		* menu_item[Menu_Total+1];
	const char		* menu_subitem[Menu_Total];
	char			setting[Menu_Total][30];
	INT32			sel;
	UINT8			total = 0;
	static INT32	submenu_choice;
	int				i;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		switch (sel)
		{
			case Menu_Return:
				submenu_choice = 0;
				sel = -1;
				break;
		}

		if (submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring (UI_cpu);
	menu_item[total++] = ui_getstring (UI_search_lives);
	menu_item[total++] = ui_getstring (UI_search_timers);
	menu_item[total++] = ui_getstring (UI_search_energy);
	menu_item[total++] = ui_getstring (UI_search_status);
	menu_item[total++] = ui_getstring (UI_search_slow);
	menu_item[total++] = ui_getstring (UI_returntoprior);
	menu_item[total] = 0;

	/* clear out the subitem menu */
	for (i = 0; i < Menu_Total; i ++)
		menu_subitem[i] = NULL;

	/* cpu number */
	sprintf (setting[Menu_CPU], "%d", searchCPU);
	menu_subitem[Menu_CPU] = setting[Menu_CPU];

	/* lives/byte value */
	sprintf(setting[Menu_Value], "%.2X (%d)", searchValue, searchValue);
	menu_subitem[Menu_Value] = setting[Menu_Value];

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
		sel = (sel + 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
		sel = (sel + total - 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
			{
				int old = searchCPU;

				searchCPU--;

				/* skip audio CPUs when the sound is off */
				while((searchCPU >= 0) && CPU_AUDIO_OFF(searchCPU))
					searchCPU--;

				if(searchCPU < 0)
					searchCPU = cpu_gettotalcpu() - 1;

				if(searchCPU != old)
					SetupDefaultMemoryAreas(searchCPU);
			}
			break;

			case Menu_Value:
				searchValue --;
				if(searchValue < 0)
					searchValue = 255;
				break;
		}
	}
	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
			{
				int old = searchCPU;

				searchCPU++;

				/* skip audio CPUs when the sound is off */
				while((searchCPU < cpu_gettotalcpu()) && CPU_AUDIO_OFF(searchCPU))
					searchCPU++;

				if(searchCPU >= cpu_gettotalcpu())
					searchCPU = 0;

				if(searchCPU != old)
					SetupDefaultMemoryAreas(searchCPU);
			}
			break;

			case Menu_Value:
				searchValue ++;
				if(searchValue > 255)
					searchValue = 0;
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == Menu_Return)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			if((sel >= Menu_Value) && (sel <= Menu_Slow))
			{
				/* set up the search tables */
				build_tables(searchCPU);

				/* backup RAM */
				backup_ram(StartRam, searchCPU);
				backup_ram(BackupRam, searchCPU);

				/* mark all RAM as good */
				memset_ram(FlagTable, 0xff);

				if(	(sel >= Menu_Time) &&
					(sel <= Menu_Slow))
				{
					usrintf_showmessage_secs(1, ui_getstring(UI_search_all_values_saved));
				}
				else if(sel == Menu_Value)
				{
					/* flag locations that match the starting value */
					struct ExtMemory	* ext;
					int					j;
					int					count = 0;

					for(ext = FlagTable; ext->data; ext++)
					{
						for(j = 0; j <= ext->end - ext->start; j++)
						{
							if(ext->data[j] != 0)
							{
								if(	(computer_readmem_byte(searchCPU, j+ext->start) != searchValue) &&
									(computer_readmem_byte(searchCPU, j+ext->start) != (searchValue - 1)))
									ext->data[j] = 0;
								else
									count++;
							}
						}
					}

					if(count == 1)
					{
						AddResultToListByIdx(0);

						usrintf_showmessage_secs(1, ui_getstring(UI_search_one_match_found_added));
					}
					else
					{
						usrintf_showmessage_secs(1, "%s: %d", ui_getstring(UI_search_matches_found), count);
					}
				}

				/* Copy the tables */
				copy_ram(OldBackupRam, BackupRam);
				copy_ram(OldFlagTable, FlagTable);

				restoreStatus = kRestore_NoSave;
			}
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the "Continue Search" menu
INT32 ContinueSearch(struct osd_bitmap *bitmap, INT32 selected)
{
	char * energyStrings[] =
	{
		"E",
		"L",
		"G",
		"LE",
		"GE",
		"NE",
		"FE"
	};

	char * bitStrings[] =
	{
		"E",
		"NE"
	};

	enum
	{
		Menu_CPU = 0,
		Menu_Value,
		Menu_Time,
		Menu_Energy,
		Menu_Bit,
		Menu_Slow,
		Menu_Return,
		Menu_Total
	};

	const char		* menu_item[Menu_Total+1];
	const char		* menu_subitem[Menu_Total];
	char			setting[Menu_Total][30];
	INT32			sel;
	UINT8			total = 0;
	static INT32	submenu_choice;
	int				i;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if (submenu_choice)
	{
		switch (sel)
		{
			case Menu_Return:
				submenu_choice = 0;
				sel = -1;
				break;
		}

		if (submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring(UI_cpu);
	menu_item[total++] = ui_getstring(UI_search_lives);
	menu_item[total++] = ui_getstring(UI_search_timers);
	menu_item[total++] = ui_getstring(UI_search_energy);
	menu_item[total++] = ui_getstring(UI_search_status);
	menu_item[total++] = ui_getstring(UI_search_slow);
	menu_item[total++] = ui_getstring(UI_returntoprior);
	menu_item[total] = 0;

	/* clear out the subitem menu */
	for(i = 0; i < Menu_Total; i++)
		menu_subitem[i] = NULL;

	/* cpu number */
	sprintf(setting[Menu_CPU], "%d", searchCPU);
	menu_subitem[Menu_CPU] = setting[Menu_CPU];

	/* lives/byte value */
	sprintf(setting[Menu_Value], "%.2X (%d)", searchValue, searchValue);
	menu_subitem[Menu_Value] = setting[Menu_Value];

	/* comparison value */
	strcpy(setting[Menu_Energy], energyStrings[searchEnergy]);
	menu_subitem[Menu_Energy] = setting[Menu_Energy];

	/* difference */
	sprintf(setting[Menu_Time], "%+d", searchTime);
	menu_subitem[Menu_Time] = setting[Menu_Time];

	/* comparison value */
	strcpy(setting[Menu_Bit], bitStrings[searchBit]);
	menu_subitem[Menu_Bit] = setting[Menu_Bit];

	strcpy(setting[Menu_Slow], bitStrings[searchSlow]);
	menu_subitem[Menu_Slow] = setting[Menu_Slow];

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
		sel = (sel + 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
		sel = (sel + total - 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
				break;
			case Menu_Value:
				searchValue--;
				if(searchValue < 0)
					searchValue = 255;
				break;
			case Menu_Energy:
				searchEnergy--;
				if(searchEnergy < 0)
					searchEnergy = 6;
				break;
			case Menu_Time:
				searchTime--;
				break;
			case Menu_Bit:
				if(searchBit > 0)
					searchBit = 0;
				break;
			case Menu_Slow:
				if(searchSlow > 0)
					searchSlow = 0;
				break;
		}
	}
	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
				break;
			case Menu_Value:
				searchValue++;
				if(searchValue > 255)
					searchValue = 0;
				break;
			case Menu_Energy:
				searchEnergy++;
				if(searchEnergy > 6)
					searchEnergy = 0;
				break;
			case Menu_Time:
				searchTime++;
				break;
			case Menu_Bit:
				if(searchBit <= 0)
					searchBit = 1;
				break;
			case Menu_Slow:
				if(searchSlow <= 0)
					searchSlow = 1;
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == Menu_Return)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			if((sel >= Menu_Value) && (sel <= Menu_Slow))
			{
				int count = 0;	/* Steph */

				copy_ram(OldBackupRam, BackupRam);
				copy_ram(OldFlagTable, FlagTable);

				if(sel == Menu_Value)
				{
					struct ExtMemory	* ext;
					int					j;

					for(ext = FlagTable; ext->data; ext++)
					{
						for(j = 0; j <= ext->end - ext->start; j++)
						{
							if(ext->data[j] != 0)
							{
								if(	(computer_readmem_byte(searchCPU, j+ext->start) != searchValue) &&
									(computer_readmem_byte(searchCPU, j+ext->start) != (searchValue - 1)))
									ext->data[j] = 0;
								else
									count++;
							}
						}
					}
				}
				else if(sel == Menu_Energy)
				{
					struct ExtMemory	* ext;
					struct ExtMemory	* save;
					int					j;

					switch(searchEnergy)
					{
						case 0:	// equals
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) != save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 1:	// less
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) >= save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 2:	// greater
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) <= save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 3:	// less or equals
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) > save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 4:	// greater or equals
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) < save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 5:	// not equals
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										if(computer_readmem_byte(searchCPU, j+ext->start) == save->data[j])
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;

						case 6:	// 'fuzzy' equals
							for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
							{
								for(j = 0; j <= ext->end - ext->start; j++)
								{
									if(ext->data[j] != 0)
									{
										INT32	data = computer_readmem_byte(searchCPU, j+ext->start);

										if(	(data != save->data[j]) && (data + 1 != save->data[j]))
											ext->data[j] = 0;
										else
											count++;
									}
								}
							}
							break;
					}
				}
				else if(sel == Menu_Time)
				{
					struct ExtMemory	* ext;
					struct ExtMemory	* save;
					int					j;

					for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
					{
						for(j = 0; j <= ext->end - ext->start; j++)
						{
							if(ext->data[j])
							{
								INT32	data = save->data[j];

								data += searchTime;

								if(computer_readmem_byte(searchCPU, j+ext->start) != data)
									ext->data[j] = 0;
								else
									count++;
							}
						}
					}
				}
				else if(sel == Menu_Bit)
				{
					struct ExtMemory	* ext;
					struct ExtMemory	* save;
					int					j;
					int					xorValue;

					xorValue = (searchBit == 0) ? 0xFF : 0x00;

					for(ext = FlagTable, save = BackupRam; ext->data && save->data; ext++, save++)
					{
						for(j = 0; j <= ext->end - ext->start; j++)
						{
							if(ext->data[j])
							{
								ext->data[j] &= (computer_readmem_byte(searchCPU, j+ext->start) ^ (save->data[j] ^ xorValue));

								if(ext->data[j])
									count++;
							}
						}
					}
				}
				else if(sel == Menu_Slow)
				{
					struct ExtMemory	* ext;
					struct ExtMemory	* save;
					int					j;

					if(searchSlow == 0)
					{
						// equals
						for(ext = FlagTable, save = StartRam; ext->data && save->data; ext++, save++)
						{
							for(j = 0; j <= ext->end - ext->start; j++)
							{
								if(ext->data[j] != 0)
								{
									if(computer_readmem_byte(searchCPU, j+ext->start) != save->data[j])
										ext->data[j] = 0;
									else
										count++;
								}
							}
						}
					}
					else
					{
						// not equals
						for(ext = FlagTable, save = StartRam; ext->data && save->data; ext++, save++)
						{
							for(j = 0; j <= ext->end - ext->start; j++)
							{
								if(ext->data[j] != 0)
								{
									if(computer_readmem_byte(searchCPU, j+ext->start) == save->data[j])
										ext->data[j] = 0;
									else
										count++;
								}
							}
						}
					}
				}

				/* Copy the tables */
				backup_ram(BackupRam, searchCPU);

				restoreStatus = kRestore_OK;

				if(count == 1)
				{
					AddResultToListByIdx(0);

					usrintf_showmessage_secs(1, ui_getstring(UI_search_one_match_found_added));
				}
				else
				{
					usrintf_showmessage_secs(1, "%s: %d", ui_getstring(UI_search_matches_found), count);
				}
			}
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the "View Last Results" menu
INT32 ViewSearchResults(struct osd_bitmap * bitmap, INT32 selected)
{
	int					sel;
	static INT32		submenu_choice;
	const char			* menu_item[MAX_SEARCHES + 2];
	offs_t				menu_addresses[MAX_SEARCHES];
	UINT8				menu_data[MAX_SEARCHES];
	char				buf[MAX_SEARCHES][20];
	int					i;
	int					total = 0;
	struct ExtMemory	* ext;
	struct ExtMemory	* ext_sr;
	struct ExtMemory	* ext_br;

	sel = selected - 1;

	/* Set up the menu */
	for(ext = FlagTable, ext_sr = StartRam, ext_br = BackupRam; ext->data && (total < MAX_SEARCHES); ext++, ext_sr++, ext_br++)
	{
		for(i = 0; i <= ext->end - ext->start; i++)
		{
			if(ext->data[i] != 0)
			{
				int		TrueAddr;
				int		TrueData;
				int		CurrentData;

				TrueAddr =		i+ext->start;
				TrueData =		ext_sr->data[i];
				CurrentData =	ext_br->data[i];

				sprintf(buf[total], "%.*X = %.2X %.2X", CPUAddressWidth(searchCPU), TrueAddr, TrueData, CurrentData);

				menu_addresses[total] =	TrueAddr;
				menu_data[total] =		TrueData;
				menu_item[total] =		buf[total];
				total++;

				if(total >= MAX_SEARCHES)
					break;
			}
		}
	}

	menu_item[total++] =	ui_getstring(UI_returntoprior);
	menu_item[total] =		0;

	ui_displaymenu(bitmap, menu_item, 0, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
		sel = (sel + 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
		sel = (sel + total - 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_PAN_UP, kVerticalKeyRepeatRate))
	{
		sel -= Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel < 0)
		{
			sel = 0;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_DOWN, kVerticalKeyRepeatRate))
	{
		sel += Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel >= total)
		{
			sel = total - 1;
		}
	}

	if(input_ui_pressed(IPT_UI_WATCH_VALUE))
	{
		/* watch the currently selected result */
		if(sel < total - 1)
		{
			int	freeWatch;

			freeWatch = FindFreeWatch();

			if(freeWatch != -1)
			{
				watches[freeWatch].cheat_num =	-1;
				watches[freeWatch].address =	menu_addresses[sel];
				watches[freeWatch].cpu =		searchCPU;
				watches[freeWatch].num_bytes =	1;
				watches[freeWatch].label_type =	0;
				watches[freeWatch].label[0] =	0;

				is_watch_active = 1;
			}
		}
	}

	if(input_ui_pressed(IPT_UI_ADD_CHEAT))
	{
		/* copy the currently selected result to the cheat list */
		if(sel < total - 1)
		{
			AddResultToListByIdx(sel);
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DELETE_CHEAT, 6))
	{
		/* remove the currently selected result */
		if(sel < total - 1)
		{
			for(ext = FlagTable; ext->data; ext++)
			{
				if(	(menu_addresses[sel] >= ext->start) &&
					(menu_addresses[sel] <= ext->end))
				{
					ext->data[menu_addresses[sel] - ext->start] = 0;

					break;
				}
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == total - 1)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the "Restore Previous Results" menu
void RestoreSearch(void)
{
	int restoreString = 0;	/* Steph */

	switch(restoreStatus)
	{
		case kRestore_NoInit:
			restoreString = UI_search_noinit;
			break;

		case kRestore_NoSave:
			restoreString = UI_search_nosave;
			break;

		case kRestore_Done:
			restoreString = UI_search_done;
			break;

		case kRestore_OK:
			restoreString = UI_search_OK;
			break;
	}

	usrintf_showmessage_secs(1, "%s", ui_getstring(restoreString));

	/* Now restore the tables if possible */
	if(restoreStatus == kRestore_OK)
	{
		copy_ram(BackupRam, OldBackupRam);
		copy_ram(FlagTable, OldFlagTable);

		/* flag it as restored so we don't do it again */
		restoreStatus = kRestore_Done;
	}
}

//	creates a new cheat from the results list
static void AddResultToListByIdx(int idx)
{
	int						count = 0;
	int						i;
	struct ExtMemory		* ext;
	struct ExtMemory		* ext_sr;

	/* traverse the results list */
	for(ext = FlagTable, ext_sr = StartRam; ext->data; ext++, ext_sr++)
	{
		for(i = 0; i <= ext->end - ext->start; i++)
		{
			if(ext->data[i])
			{
				if(count == idx)
				{
					AddCheatToList(ext->start + i, ext_sr->data[i], searchCPU);

					return;
				}

				count++;
			}
		}
	}
}

//	adds a cheat to the end of the cheat list
static void AddCheatToList(offs_t address, UINT8 data, int cpu)
{
	if(LoadedCheatTotal < MAX_LOADEDCHEATS)
	{
		char					newName[128];
		struct cheat_struct		* theCheat;
		struct subcheat_struct	* theSubCheat;

		cheat_insert_new(LoadedCheatTotal);

		theCheat =		&CheatTable[LoadedCheatTotal - 1];
		theSubCheat =	theCheat->subcheat;

		sprintf(newName, "%.*X (%d) = %.2X", CPUAddressWidth(cpu), address, cpu, data);

		theCheat->name = realloc(theCheat->name, strlen(newName) + 1);
		if(theCheat->name)
			strcpy(theCheat->name, newName);

		theSubCheat->cpu =		cpu;
		theSubCheat->address =	address;
		theSubCheat->data =		data;
		theSubCheat->code =		0;
	}
}

#ifdef macintosh
#pragma mark -
#endif

//	adds a watch to the cheat list
static void ConvertWatchToCheat(int idx)
{
	char					newName[128];
	struct cheat_struct		* theCheat;
	struct subcheat_struct	* theSubCheat;
	int						address;
	int						cpu;

	if(LoadedCheatTotal < MAX_LOADEDCHEATS)
	{
		address =	watches[idx].address;
		cpu =		watches[idx].cpu;

		cheat_insert_new(LoadedCheatTotal);

		theCheat =		&CheatTable[LoadedCheatTotal - 1];
		theSubCheat =	theCheat->subcheat;

		sprintf(newName, "%s: %.*X (%d)", ui_getstring(UI_watch), CPUAddressWidth(cpu), address, cpu);

		theCheat->name = realloc(theCheat->name, strlen(newName) + 1);
		if(theCheat->name)
			strcpy(theCheat->name, newName);

		theSubCheat->cpu =		cpu;
		theSubCheat->address =	address;
		theSubCheat->data =		0;
		theSubCheat->code =		kCheatSpecial_Watch;
	}
}

//	saves a watch (as a cheat) to the end of the cheat list
static void SaveWatch(int idx)
{
	void	* theFile;
	char	buf[4096];
	char	name[128];

	theFile = osd_fopen(NULL, cheatfile, OSD_FILETYPE_CHEAT, 1);

	if(!theFile)
		return;

	sprintf(name, "%s: %.*X (%d)", ui_getstring(UI_watch), CPUAddressWidth(watches[idx].cpu), watches[idx].address, watches[idx].cpu);

	#ifdef MESS

	sprintf(	buf,
				"%s:%08X:%c:%d:%.*X:%02X:%02X:%03d:%s\n",
				Machine->gamedrv->name,
				0,
				0,
				watches[idx].cpu,
				CPUAddressWidth(watches[idx].cpu),
				watches[idx].address,
				0,
				0,
				kCheatSpecial_Watch,
				name);

	#else

	sprintf(	buf,
				"%s:%d:%.*X:%02X:%03d:%s\n",
				Machine->gamedrv->name,
				watches[idx].cpu,
				CPUAddressWidth(watches[idx].cpu),
				watches[idx].address,
				0,
				kCheatSpecial_Watch,
				name);

	#endif

	osd_fwrite(theFile, buf, strlen(buf));

	osd_fclose(theFile);
}

//	returns the index of a free watch (-1 if none is found)
static int FindFreeWatch(void)
{
	int i;

	for(i = 0; i < MAX_WATCHES; i++)
		if(watches[i].num_bytes == 0)
			return i;

	/* indicate no free watch found */
	return -1;
}

//	displays all watches
static void DisplayWatches(struct osd_bitmap * bitmap)
{
	int		i, j;
	char	buf[256];

	if(!is_watch_active || !is_watch_visible)
		return;

	for(i = 0; i < MAX_WATCHES; i++)
	{
		/* Is this watchpoint active? */
		if(watches[i].num_bytes != 0)
		{
			char buf2[80];

			/* Display the first byte */
			sprintf(buf, "%.2X", computer_readmem_byte(watches[i].cpu, watches[i].address));

			/* If this is for more than one byte, display the rest */
			for(j = 1; j < watches[i].num_bytes; j++)
			{
				sprintf(buf2, " %02X", computer_readmem_byte(watches[i].cpu, watches[i].address + j));
				strcat(buf, buf2);
			}

			/* Handle any labels */
			switch(watches[i].label_type)
			{
				case 0:
				default:
					break;

				case 1:
					sprintf(buf2, " (%.*X)", CPUAddressWidth(watches[i].cpu), watches[i].address);
					strcat(buf, buf2);
					break;

				case 2:
					sprintf(buf2, " (%s)", watches[i].label);
					strcat(buf, buf2);
					break;
			}

			ui_text(bitmap, buf, watches[i].x, watches[i].y);
		}
	}
}

//	shows watch configuration menu
static INT32 ConfigureWatch(struct osd_bitmap * bitmap, INT32 selected, UINT8 watchnum)
{
	enum
	{
		Menu_CPU = 0,
		Menu_Address,
		Menu_WatchLength,
		Menu_WatchLabelType,
		Menu_WatchLabel,
		Menu_WatchX,
		Menu_WatchY,
		Menu_Return,

		Menu_Max
	};

	int	kLabelTextTable[] =
	{
		UI_none,
		UI_address,
		UI_text
	};

	int				sel;
	int				total = 0;
	static INT32	submenu_choice;
	static UINT8	textedit_active;
	const char		* menu_item[Menu_Max + 1];
	const char		* menu_subitem[Menu_Max + 1];
	char			setting[Menu_Max][30];
	char			flag[Menu_Max] = { 0 };
	int				arrowize = 0;
	int				i;

	sel = selected - 1;

	/* if we're editing the label, make it inverse */
	if(textedit_active)
		flag[sel] = 1;

	sprintf(setting[total], "%d", watches[watchnum].cpu);

	menu_item[total] =		ui_getstring(UI_cpu);
	menu_subitem[total] =	setting[total];
	total++;

	sprintf(setting[total], "%.*X", CPUAddressWidth(watches[watchnum].cpu), watches[watchnum].address);

	menu_item[total] =		ui_getstring(UI_address);
	menu_subitem[total] =	setting[total];
	total++;

	sprintf(setting[total], "%d", watches[watchnum].num_bytes);

	menu_item[total] =		ui_getstring(UI_watchlength);
	menu_subitem[total] =	setting[total];
	total++;

	menu_item[total] =		ui_getstring(UI_watchlabeltype);
	menu_subitem[total] =	ui_getstring(kLabelTextTable[watches[watchnum].label_type]);
	total++;

	menu_item[total] =		ui_getstring(UI_watchlabel);
	if(watches[watchnum].label[0])
		menu_subitem[total] =	watches[watchnum].label;
	else
		menu_subitem[total] =	ui_getstring(UI_none);
	total++;

	sprintf(setting[total], "%d", watches[watchnum].x);
	menu_item[total] =		ui_getstring(UI_watchx);
	menu_subitem[total] =	setting[total];
	total++;

	sprintf(setting[total], "%d", watches[watchnum].y);
	menu_item[total] =		ui_getstring(UI_watchy);
	menu_subitem[total] =	setting[total];
	total++;

	menu_item[total] =		ui_getstring(UI_returntoprior);
	menu_subitem[total] =	0;
	total++;

	menu_item[total] = 0;
	menu_subitem[total] = 0;

	ui_displaymenu(bitmap, menu_item, menu_subitem, flag, sel, arrowize);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		textedit_active = 0;

		sel = (sel + 1) % Menu_Max;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		textedit_active = 0;

		sel = (sel + total - 1) % Menu_Max;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
				watches[watchnum].cpu--;

				/* skip audio CPUs when the sound is off */
				while((watches[watchnum].cpu >= 0) && CPU_AUDIO_OFF(watches[watchnum].cpu))
					watches[watchnum].cpu--;

				if(watches[watchnum].cpu < 0)
					watches[watchnum].cpu = cpu_gettotalcpu() - 1;

				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;

			case Menu_Address:
				textedit_active = 0;

				watches[watchnum].address--;

				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;

			case Menu_WatchLength:
				if(watches[watchnum].num_bytes > 0)
					watches[watchnum].num_bytes--;
				else
					watches[watchnum].num_bytes = 16;
				break;

			case Menu_WatchLabelType:
				if(watches[watchnum].label_type > 0)
					watches[watchnum].label_type--;
				else
					watches[watchnum].label_type = 2;
				break;

			case Menu_WatchLabel:
				textedit_active = 0;
				break;

			case Menu_WatchX:
				if(watches[watchnum].x > 0)
					watches[watchnum].x--;
				else
					watches[watchnum].x = Machine->uiwidth - 1;
				break;

			case Menu_WatchY:
				if(watches[watchnum].y > 0)
					watches[watchnum].y--;
				else
					watches[watchnum].y = Machine->uiheight - 1;
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_CPU:
				watches[watchnum].cpu++;

				/* skip audio CPUs when the sound is off */
				while((watches[watchnum].cpu < cpu_gettotalcpu()) && CPU_AUDIO_OFF(watches[watchnum].cpu))
					watches[watchnum].cpu++;

				if(watches[watchnum].cpu >= cpu_gettotalcpu())
					watches[watchnum].cpu = 0;

				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;

			case Menu_Address:
				textedit_active = 0;

				watches[watchnum].address++;

				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
				break;

			case Menu_WatchLength:
				watches[watchnum].num_bytes++;

				if(watches[watchnum].num_bytes > 16)
					watches[watchnum].num_bytes = 0;
				break;

			case Menu_WatchLabelType:
				watches[watchnum].label_type++;

				if(watches[watchnum].label_type > 2)
					watches[watchnum].label_type = 0;
				break;

			case Menu_WatchLabel:
				textedit_active = 0;
				break;

			case Menu_WatchX:
				watches[watchnum].x++;

				if(watches[watchnum].x >= Machine->uiwidth)
					watches[watchnum].x = 0;
				break;

			case Menu_WatchY:
				watches[watchnum].y++;

				if(watches[watchnum].y >= Machine->uiheight)
					watches[watchnum].y = 0;
				break;
		}
	}

	/* see if any watchpoints are active and set the flag if so */
	is_watch_active = 0;
	for(i = 0; i < MAX_WATCHES; i++)
	{
		if(watches[i].num_bytes != 0)
		{
			is_watch_active = 1;
			break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == Menu_Return)
		{
			/* return to main menu */
			submenu_choice = 0;
			sel = -1;
		}
		else if((sel == Menu_Address) || (sel == Menu_WatchLabel))
		{
			/* wait for key up */
			while(input_ui_pressed(IPT_UI_SELECT)) ;

			/* flush the text buffer */
			osd_readkey_unicode(1);
			textedit_active ^= 1;
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		textedit_active = 0;

		/* flush the text buffer */
		osd_readkey_unicode(1);
		schedule_full_refresh();
	}

	/* After we've weeded out any control characters, look for text */
	if(textedit_active)
	{
		int code;

		/* is this the address field? */
		if(sel == Menu_Address)
		{
			INT8 hex_val;

			/* see if a hex digit was typed */
			hex_val = code_read_hex_async();
			if(hex_val != -1)
			{
				/* shift over one digit, add in the new value and clip */
				watches[watchnum].address <<= 4;
				watches[watchnum].address |= hex_val;
				watches[watchnum].address &= cpunum_address_mask(watches[watchnum].cpu);
			}
		}
		else if(sel == Menu_WatchLabel)
		{
			int length = strlen(watches[watchnum].label);

			if(length < 254)
			{
				code = osd_readkey_unicode(0) & 0xff; /* no 16-bit support */

				if(code == 0x08) /* backspace */
				{
					if(length > 0)
					{
						watches[watchnum].label[length - 1] = 0;
					}
				}
				else if(isprint(code))
				{
					/* append the character */
					watches[watchnum].label[length] =	code;
					watches[watchnum].label[length+1] =	0;
				}
			}
		}
	}

	return sel + 1;
}

//	shows the "Configure Watchpoint" menu
static INT32 ChooseWatch(struct osd_bitmap * bitmap, INT32 selected)
{
	int				sel;
	static INT32	submenu_choice;
	const char		* menu_item[MAX_WATCHES + 2];
	char			buf[MAX_WATCHES][80];
	const char		* watchpoint_str = ui_getstring(UI_watchpoint);
	const char		* disabled_str = ui_getstring(UI_disabled);
	int				i;
	int				total = 0;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if(submenu_choice)
	{
		submenu_choice = ConfigureWatch(bitmap, submenu_choice, sel);

		if(submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	for(i = 0; i < MAX_WATCHES; i++)
	{
		sprintf(buf[i], "%s %d: ", watchpoint_str, i);

		/* If the watchpoint is active (1 or more bytes long), show it */
		if(watches[i].num_bytes)
		{
			char buf2[80];

			sprintf(buf2, "%.*X", CPUAddressWidth(watches[i].cpu), watches[i].address);
			strcat(buf[i], buf2);
		}
		else
		{
			strcat(buf[i], disabled_str);
		}

		menu_item[total] = buf[i];
		total++;
	}

	menu_item[total] = ui_getstring (UI_returntoprior);
	total++;

	menu_item[total] = 0;	/* terminate array */

	ui_displaymenu(bitmap, menu_item, 0, 0, sel, 0);

	if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			for(i = 0; i < MAX_WATCHES; i++)
				watches[i].num_bytes = 0;
		}
		else
		{
			if((sel >= 0) && (sel < total))
				watches[sel].num_bytes = 0;
		}
	}

	if(input_ui_pressed(IPT_UI_ADD_CHEAT) && (LoadedCheatTotal < MAX_LOADEDCHEATS) && (sel >= 0) && (sel < total))
	{
		ConvertWatchToCheat(sel);
	}

	if(input_ui_pressed(IPT_UI_SAVE_CHEAT) && (sel >= 0) && (sel < total))
	{
		SaveWatch(sel);
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel = (sel + 1) % total;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel = (sel + total - 1) % total;
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_UP, kVerticalKeyRepeatRate))
	{
		sel -= Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel < 0)
		{
			sel = 0;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_PAN_DOWN, kVerticalKeyRepeatRate))
	{
		sel += Machine->uiheight / (3 * Machine->uifontheight / 2) - 1;

		if(sel >= total)
		{
			sel = total - 1;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == MAX_WATCHES)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}


#ifdef macintosh
#pragma mark -
#endif

//	shows the "General Help" menu
static INT32 DisplayHelpFile(struct osd_bitmap * bitmap, INT32 selected)
{
	char	buf[2048];
	int		sel;

	sel = selected - 1;

	strcpy(buf, ui_getstring(UI_no_help_available));

	/* menu system, use the normal menu keys */
	strcat(buf, "\n\n\t");
	strcat(buf, ui_getstring (UI_lefthilight));
	strcat(buf, " ");
	strcat(buf, ui_getstring (UI_returntoprior));
	strcat(buf, " ");
	strcat(buf, ui_getstring (UI_righthilight));

	ui_displaymessagewindow(bitmap, buf);

	if(input_ui_pressed(IPT_UI_SELECT))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

#ifdef macintosh
#pragma mark -
#endif

//	sets up which memory areas should be searched
static void SetupDefaultMemoryAreas(int cpu)
{
	const struct Memory_WriteAddress	* mwa = Machine->drv->cpu[searchCPU].memory_write;
	int									count = 0;

	memset(memoryRegionEnabled, 1, MAX_MEMORY_REGIONS);

	while(!IS_MEMPORT_END(mwa))
	{
		if(!IS_MEMPORT_MARKER(mwa))
		{
			mem_write_handler	handler = mwa->handler;
			int					enable = 1;

			switch(searchSpeed)
			{
				case kSpeed_Fast:
					enable = 0;

					// search RAM
					if(handler == MWA_RAM)
						enable = 1;

					#ifndef NEOFREE
					#ifndef TINY_COMPILE

					// for neogeo, search bank one
					if((Machine->gamedrv->clone_of == &driver_neogeo) && (cpu == 0) && (handler == MWA_BANK1))
						enable = 1;

					#endif
					#endif

					#if HAS_TMS34010

					// for exterminator, search bank one
					if(((Machine->drv->cpu[1].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010) && (cpu == 0) && (handler == MWA_BANK1))
						enable = 1;

					// for smashtv, search bank two
					if(((Machine->drv->cpu[0].cpu_type & ~CPU_FLAGS_MASK) == CPU_TMS34010) && (cpu == 0) && (handler == MWA_BANK2))
						enable = 1;

					#endif

					break;

				case kSpeed_Medium:
					// only search banks + RAM
					enable = (((((UINT32)handler) >= ((UINT32)MWA_BANK1)) && (((UINT32)handler) <= ((UINT32)MWA_BANK24))) || (((UINT32)handler) == ((UINT32)MWA_RAM)));
					break;

				case kSpeed_Slow:
					// ignore NOP sections
					if(handler == MWA_NOP)
						enable = 0;

					// ignore ROM sections
					if(handler == MWA_ROM)
						enable = 0;

					// ignore custom handlers which do not have memory mapped to them
					if(((UINT32)handler) > ((UINT32)(mem_write_handler)STATIC_COUNT))
						if(!mwa->base)
							enable = 0;
					break;

				case kSpeed_VerySlow:
					// ignore NOP sections
					if(handler == MWA_NOP)
						enable = 0;

					// ignore ROM sections
					if(handler == MWA_ROM)
						enable = 0;
					break;
			}

			memoryRegionEnabled[count] = enable;

			count++;
		}

		mwa++;
	}
}

//	shows the "Options:SelectMemoryAreas" menu
static INT32 SelectMemoryAreas(struct osd_bitmap * bitmap, INT32 selected)
{
	int									sel;
	static INT32						submenu_choice;
	char								buf[MAX_MEMORY_AREAS][80];
	const char							* menu_item[MAX_MEMORY_AREAS + 1];
	const char							* menu_subitem[MAX_MEMORY_AREAS + 1];
	int									total = 0;
	int									arrowize = 0;
	const struct Memory_WriteAddress	* mwa = Machine->drv->cpu[searchCPU].memory_write;

	sel = selected - 1;

	while(!IS_MEMPORT_END(mwa))
	{
		if(!IS_MEMPORT_MARKER(mwa))
		{
			mem_write_handler	handler = mwa->handler;
			char				desc[7];

			if((((UINT32)handler) >= ((UINT32)MWA_BANK1)) && (((UINT32)handler) <= ((UINT32)MWA_BANK24)))
			{
				sprintf(desc, "BANK%d", (((UINT32)handler) - ((UINT32)MWA_BANK1)) + 1);
			}
			else
			{
				switch((UINT32)handler)
				{
					case (UINT32)MWA_NOP:		strcpy(desc, "NOP");	break;
					case (UINT32)MWA_RAM:		strcpy(desc, "RAM");	break;
					case (UINT32)MWA_ROM:		strcpy(desc, "ROM");	break;
					case (UINT32)MWA_RAMROM:	strcpy(desc, "RAMROM");	break;
					default:					strcpy(desc, "CUSTOM");	break;
				}
			}

			sprintf(buf[total], "%.*X-%.*X %.6s", CPUAddressWidth(searchCPU), mwa->start, CPUAddressWidth(searchCPU), mwa->end, desc);

			menu_item[total] =		buf[total];
			menu_subitem[total] =	ui_getstring(memoryRegionEnabled[total] ? UI_on : UI_off);
			total++;
		}

		mwa++;
	}

	menu_item[total] =		ui_getstring(UI_returntoprior);
	menu_subitem[total] =	0;
	total++;

	menu_item[total] =		0;
	menu_subitem[total] =	0;

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, arrowize);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel = (sel + 1) % total;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel = (sel + total - 1) % total;
	}

	if(	UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate) ||
		UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if(sel < total - 1)
		{
			memoryRegionEnabled[sel] ^= 1;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == total - 1)
		{
			submenu_choice = 0;
			sel = -1;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

//	shows the "Options" menu
static INT32 SelectOptions(struct osd_bitmap * bitmap, INT32 selected)
{
	enum
	{
		Menu_SelectMemoryAreas = 0,
		Menu_SearchSpeed,
		Menu_Return,

		Menu_Max
	};

	int searchSpeedStrings[] =
	{
		UI_search_speed_fast,
		UI_search_speed_medium,
		UI_search_speed_slow,
		UI_search_speed_veryslow
	};

	int				sel;
	static INT32	submenu_choice;
	const char		* menu_item[Menu_Max + 1];
	const char		* menu_subitem[Menu_Max + 1];
	int				total = 0;
	int				arrowize = 0;

	sel = selected - 1;

	if(submenu_choice)
	{
		switch(sel)
		{
			case Menu_SelectMemoryAreas:
				submenu_choice = SelectMemoryAreas(bitmap, submenu_choice);
				break;
		}

		if(submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	menu_item[total] =		ui_getstring(UI_search_select_memory_areas);
	menu_subitem[total] =	0;
	total++;

	menu_item[total] =		ui_getstring(UI_search_speed);
	menu_subitem[total] =	ui_getstring(searchSpeedStrings[searchSpeed]);
	total++;

	menu_item[total] =		ui_getstring(UI_returntoprior);
	menu_subitem[total] =	0;
	total++;

	menu_item[total] =		0;
	menu_subitem[total] =	0;

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, arrowize);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel = (sel + 1) % total;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel = (sel + total - 1) % total;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_SearchSpeed:
				searchSpeed--;

				if((searchSpeed < kSpeed_Fast) || (searchSpeed > kSpeed_VerySlow))
					searchSpeed = kSpeed_VerySlow;

				SetupDefaultMemoryAreas(searchCPU);
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		switch(sel)
		{
			case Menu_SearchSpeed:
				searchSpeed++;

				if((searchSpeed < kSpeed_Fast) || (searchSpeed > kSpeed_VerySlow))
					searchSpeed = kSpeed_Fast;

				SetupDefaultMemoryAreas(searchCPU);
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == Menu_Return)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else if(sel == Menu_SelectMemoryAreas)
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

#ifdef macintosh
#pragma mark -
#endif

//	shows the main cheat menu
INT32 cheat_menu(struct osd_bitmap *bitmap, INT32 selected)
{
	enum
	{
		Menu_EnableDisable = 0,
		Menu_AddEdit,
		Menu_StartSearch,
		Menu_ContinueSearch,
		Menu_ViewResults,
		Menu_RestoreSearch,
		Menu_ChooseWatch,
		Menu_DisplayHelp,
		Menu_Options,
		Menu_Return,

		Menu_Max
	};

	const char		* menu_item[Menu_Max + 1];
	INT32			sel;
	UINT8			total = 0;
	static INT32	submenu_choice;

	#if CHEAT_PAUSE

	enum
	{
		kStatusRunning,
		kStatusSuspended,
		kStatusHeld
	};

	static INT8	needs_pause = 1;
	static INT8	cpu_disable_status[MAX_CPU];
	int			i;

	#endif

	cheatEngineWasActive = 1;

	sel = selected - 1;

	/* If a submenu has been selected, go there */
	if(submenu_choice)
	{
		switch(sel)
		{
			case Menu_EnableDisable:
				submenu_choice = EnableDisableCheatMenu(bitmap, submenu_choice);
				break;

			case Menu_AddEdit:
				submenu_choice = AddEditCheatMenu(bitmap, submenu_choice);
				break;

			case Menu_StartSearch:
				submenu_choice = StartSearch(bitmap, submenu_choice);
				break;

			case Menu_ContinueSearch:
				submenu_choice = ContinueSearch(bitmap, submenu_choice);
				break;

			case Menu_ViewResults:
				submenu_choice = ViewSearchResults(bitmap, submenu_choice);
				break;

			case Menu_ChooseWatch:
				submenu_choice = ChooseWatch(bitmap, submenu_choice);
				break;

			case Menu_DisplayHelp:
				submenu_choice = DisplayHelpFile(bitmap, submenu_choice);
				break;

			case Menu_Options:
				submenu_choice = SelectOptions(bitmap, submenu_choice);
				break;

			case Menu_Return:
				submenu_choice = 0;
				sel = -1;
				break;
		}

		if(submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	/* No submenu active, display the main cheat menu */
	menu_item[total++] = ui_getstring(UI_enablecheat);
	menu_item[total++] = ui_getstring(UI_addeditcheat);
	menu_item[total++] = ui_getstring(UI_startcheat);
	menu_item[total++] = ui_getstring(UI_continuesearch);
	menu_item[total++] = ui_getstring(UI_viewresults);
	menu_item[total++] = ui_getstring(UI_restoreresults);
	menu_item[total++] = ui_getstring(UI_memorywatch);
	menu_item[total++] = ui_getstring(UI_generalhelp);
	menu_item[total++] = ui_getstring(UI_options);
	menu_item[total++] = ui_getstring(UI_returntomain);
	menu_item[total] = 0;

	ui_displaymenu(bitmap, menu_item, 0, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
		sel = (sel + 1) % total;

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
		sel = (sel + total - 1) % total;

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == Menu_Return)
		{
			submenu_choice = 0;
			sel = -1;
		}
		else if(sel == Menu_RestoreSearch)
		{
			RestoreSearch();
		}
		else
		{
			submenu_choice = 1;
			schedule_full_refresh();
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	#if CHEAT_PAUSE

	if(needs_pause)
	{
		/* back up cpu status and halt execution */
		for(i = 0; i < cpu_gettotalcpu(); i++)
		{
			if(timer_iscpususpended(i, SUSPEND_REASON_DISABLE))
			{
				cpu_disable_status[i] = kStatusSuspended;
			}
			else if(timer_iscpuheld(i, SUSPEND_REASON_DISABLE))
			{
				cpu_disable_status[i] = kStatusHeld;
			}
			else
			{
				cpu_disable_status[i] = kStatusRunning;
			}

			timer_holdcpu(i, 1, SUSPEND_REASON_DISABLE);
		}

		watchdog_clear();

		needs_pause = 0;
	}
	else
	{
		if((sel == -1) || (sel == -2))
		{
			/* restore status */
			for(i = 0; i < cpu_gettotalcpu(); i++)
			{
				switch(cpu_disable_status[i])
				{
					case kStatusSuspended:
						timer_holdcpu(i, 0, SUSPEND_REASON_DISABLE);

						timer_suspendcpu(i, 1, SUSPEND_REASON_DISABLE);
						break;

					case kStatusHeld:
						break;

					case kStatusRunning:
						timer_holdcpu(i, 0, SUSPEND_REASON_DISABLE);
						break;
				}
			}

			needs_pause = 1;
		}
	}

	#endif

	return sel + 1;
}

//	shuts down the cheat engine
void StopCheat(void)
{
	int i;

	for(i = 0; i < LoadedCheatTotal; i++)
	{
		/* free storage for the strings */
		if(CheatTable[i].name)
		{
			free(CheatTable[i].name);
			CheatTable[i].name = NULL;
		}
		if(CheatTable[i].comment)
		{
			free(CheatTable[i].comment);
			CheatTable[i].comment = NULL;
		}
	}

	reset_table(StartRam);
	reset_table(BackupRam);
	reset_table(FlagTable);

	reset_table(OldBackupRam);
	reset_table(OldFlagTable);
}

//	performs all cheat actions, called once per frame
void DoCheat(struct osd_bitmap *bitmap)
{
	DisplayWatches(bitmap);

	if(CheatEnabled)
	{
		int i, j;
		int done;

		/* At least one cheat is active, handle them */
		for(i = 0; i < LoadedCheatTotal; i++)
		{
			if(	(CheatTable[i].activate_key != -1) &&
				(!(CheatTable[i].flags & kCheatFlagComment)) &&
				(!(CheatTable[i].flags & kCheatFlagPromptMask)) &&
				!cheatEngineWasActive)
			{
				/* use code_pressed to avoid issues */
				if(code_pressed(CheatTable[i].activate_key))
				{
					if(!(CheatTable[i].flags & kCheatFlagActivationKeyPressed))
					{
						if(CheatTable[i].flags & kCheatFlagOneShot)
						{
							cheat_set_status(i, 1);
						}
						else
						{
							if(CheatTable[i].flags & kCheatFlagActive)
								cheat_set_status(i, 0);
							else
								cheat_set_status(i, 1);
						}

						CheatTable[i].flags |= kCheatFlagActivationKeyPressed;
					}
				}
				else
				{
					CheatTable[i].flags ^= kCheatFlagActivationKeyPressed;
				}
			}

			/* skip if this isn't an active cheat */
			if(!(CheatTable[i].flags & kCheatFlagActive))
				continue;

			/* loop through all subcheats */
			for(j = 0; j <= CheatTable[i].num_sub; j++)
			{
				struct subcheat_struct	* subcheat = &CheatTable[i].subcheat[j];

				if(subcheat->flags & kSubcheatFlagDone)
					continue;

				/* most common case: 0 */
				if(subcheat->code == kCheatSpecial_Poke)
				{
					WRITE_CHEAT;
				}
				/* Check special function if cheat counter is ready */
				else if(subcheat->frame_count == 0)
				{
					switch(subcheat->code)
					{
						case 1:
							WRITE_CHEAT;
							subcheat->flags |= kSubcheatFlagDone;
							break;

						case 2:
						case 3:
						case 4:
							WRITE_CHEAT;
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 5,6,7 check if the value has changed, if yes, start a timer. */
						/* When the timer ends, change the location */
						case 5:
						case 6:
						case 7:
							if(subcheat->flags & kSubcheatFlagTimed)
							{
								WRITE_CHEAT;
								subcheat->flags &= ~kSubcheatFlagTimed;
							}
							else if(COMPARE_CHEAT)
							{
								subcheat->frame_count = subcheat->frames_til_trigger;
								subcheat->flags |= kSubcheatFlagTimed;
							}
							break;

						/* 8,9,10,11 do not change the location if the value change by X every frames
						  This is to try to not change the value of an energy bar
				 		  when a bonus is awarded to it at the end of a level
				 		  See Kung Fu Master */
						case 8:
						case 9:
						case 10:
						case 11:
							if(subcheat->flags & kSubcheatFlagTimed)
							{
								/* Check the value to see if it has increased over the original value by 1 or more */
								if(READ_CHEAT != subcheat->backup - (kCheatSpecial_Backup1 - subcheat->code + 1))
									WRITE_CHEAT;

								subcheat->flags &= ~kSubcheatFlagTimed;
							}
							else
							{
								subcheat->backup = READ_CHEAT;
								subcheat->frame_count = 1;
								subcheat->flags |= kSubcheatFlagTimed;
							}
							break;

						/* 20-24: set bits */
						case 20:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							break;

						case 21:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							subcheat->flags |= kSubcheatFlagDone;
							break;

						case 22:
						case 23:
						case 24:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT | subcheat->data);
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 40-44: reset bits */
						case 40:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							break;

						case 41:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							subcheat->flags |= kSubcheatFlagDone;
							break;

						case 42:
						case 43:
						case 44:
							computer_writemem_byte(subcheat->cpu, subcheat->address, READ_CHEAT & ~subcheat->data);
							subcheat->frame_count = subcheat->frames_til_trigger;
							break;

						/* 60-65: user select, poke when changes */
						case 60:
						case 61:
						case 62:
						case 63:
						case 64:
						case 65:
							if(subcheat->flags & kSubcheatFlagTimed)
							{
								if(READ_CHEAT != subcheat->backup)
								{
									WRITE_CHEAT;
									subcheat->flags |= kSubcheatFlagDone;
								}
							}
							else
							{
								subcheat->backup = READ_CHEAT;
								subcheat->frame_count = 1;
								subcheat->flags |= kSubcheatFlagTimed;
							}
							break;

						/* 70-75: user select, poke once */
						case 70:
						case 71:
						case 72:
						case 73:
						case 74:
						case 75:
							WRITE_CHEAT;
							subcheat->flags |= kSubcheatFlagDone;
							break;

						/* Steph 2001.05.04 - The following types patch ROM areas */
						case 100:
							patch_rom(subcheat);
							break;

						case 101:
							patch_rom(subcheat);
							subcheat->flags |= kSubcheatFlagDone;
							break;

					}
				}
				else
				{
					subcheat->frame_count--;
				}
			}

			done = 1;

			for(j = 0; j <= CheatTable[i].num_sub; j++)
			{
				if(!(CheatTable[i].subcheat[j].flags & kSubcheatFlagDone))
				{
					done = 0;

					break;
				}
			}

			/* disable cheat if 100% done */
			if(done)
			{
				cheat_set_status(i, 0);
			}
		} /* end for */
	}

	/* IPT_UI_TOGGLE_CHEAT Enable/Disable the active cheats on the fly. Required for some cheats. */
	if(input_ui_pressed(IPT_UI_TOGGLE_CHEAT))
	{
		/* Hold down shift to toggle the watchpoints */
		if(ShiftKeyPressed())
		{
			is_watch_visible ^= 1;
			usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_watchpoints), (is_watch_visible ? ui_getstring (UI_on) : ui_getstring (UI_off)));
		}
		else if(ActiveCheatTotal)
		{
			CheatEnabled ^= 1;
			usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_cheats), (CheatEnabled ? ui_getstring (UI_on) : ui_getstring (UI_off)));
		}
	}

	cheatEngineWasActive = 0;
}
