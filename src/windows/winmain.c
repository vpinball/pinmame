//============================================================
//
//	win32.c - Win32 main program
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winnt.h>

// standard includes
#include <time.h>
#include <ctype.h>

// MAME headers
#include "driver.h"
#include "window.h"

// from config.c
int  cli_frontend_init (int argc, char **argv);
void cli_frontend_exit (void);

//============================================================
//	GLOBAL VARIABLES
//============================================================

int verbose;

// this line prevents globbing on the command line
int _CRT_glob = 0;


//============================================================
//	LOCAL VARIABLES
//============================================================

static char mapfile_name[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER pass_thru_filter;

static int original_leds;



//============================================================
//	PROTOTYPES
//============================================================

static LONG CALLBACK exception_filter(struct _EXCEPTION_POINTERS *info);
static const char *lookup_symbol(UINT32 address);



//============================================================
//	main
//============================================================

int main(int argc, char **argv)
{
	int game_index;
	char *ext;
	int res = 0;

	// set up exception handling
	strcpy(mapfile_name, argv[0]);
	ext = strchr(mapfile_name, '.');
	if (ext)
		strcpy(ext, ".map");
	else
		strcat(mapfile_name, ".map");
	pass_thru_filter = SetUnhandledExceptionFilter(exception_filter);

	// remember the initial LED states
	original_leds = osd_get_leds();

	// parse config and cmdline options
	game_index = cli_frontend_init (argc, argv);

	// have we decided on a game?
	if (game_index != -1)
		res = run_game(game_index);

	// restore the original LED state
	osd_set_leds(original_leds);
	exit(res);
}



//============================================================
//	osd_init
//============================================================

int osd_init(void)
{
	extern int win32_init_input(void);
	int result;

	result = win32_init_window();
	if (result == 0)
		result = win32_init_input();
	return result;
}



//============================================================
//	osd_exit
//============================================================

void osd_exit(void)
{
	extern void win32_shutdown_input(void);
	win32_shutdown_input();
	osd_set_leds(0);
}


//============================================================
//	exception_filter
//============================================================

static LONG CALLBACK exception_filter(struct _EXCEPTION_POINTERS *info)
{
	static const struct
	{
		DWORD code;
		const char *string;
	} exception_table[] =
	{
		{ EXCEPTION_ACCESS_VIOLATION,		"ACCESS VIOLATION" },
		{ EXCEPTION_DATATYPE_MISALIGNMENT,	"DATATYPE MISALIGNMENT" },
		{ EXCEPTION_BREAKPOINT, 			"BREAKPOINT" },
		{ EXCEPTION_SINGLE_STEP,			"SINGLE STEP" },
		{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED,	"ARRAY BOUNDS EXCEEDED" },
		{ EXCEPTION_FLT_DENORMAL_OPERAND,	"FLOAT DENORMAL OPERAND" },
		{ EXCEPTION_FLT_DIVIDE_BY_ZERO,		"FLOAT DIVIDE BY ZERO" },
		{ EXCEPTION_FLT_INEXACT_RESULT,		"FLOAT INEXACT RESULT" },
		{ EXCEPTION_FLT_INVALID_OPERATION,	"FLOAT INVALID OPERATION" },
		{ EXCEPTION_FLT_OVERFLOW,			"FLOAT OVERFLOW" },
		{ EXCEPTION_FLT_STACK_CHECK,		"FLOAT STACK CHECK" },
		{ EXCEPTION_FLT_UNDERFLOW,			"FLOAT UNDERFLOW" },
		{ EXCEPTION_INT_DIVIDE_BY_ZERO,		"INTEGER DIVIDE BY ZERO" },
		{ EXCEPTION_INT_OVERFLOW, 			"INTEGER OVERFLOW" },
		{ EXCEPTION_PRIV_INSTRUCTION, 		"PRIVILEGED INSTRUCTION" },
		{ EXCEPTION_IN_PAGE_ERROR, 			"IN PAGE ERROR" },
		{ EXCEPTION_ILLEGAL_INSTRUCTION, 	"ILLEGAL INSTRUCTION" },
		{ EXCEPTION_NONCONTINUABLE_EXCEPTION,"NONCONTINUABLE EXCEPTION" },
		{ EXCEPTION_STACK_OVERFLOW, 		"STACK OVERFLOW" },
		{ EXCEPTION_INVALID_DISPOSITION, 	"INVALID DISPOSITION" },
		{ EXCEPTION_GUARD_PAGE, 			"GUARD PAGE VIOLATION" },
		{ EXCEPTION_INVALID_HANDLE, 		"INVALID HANDLE" },
		{ 0,								"UNKNOWN EXCEPTION" }
	};
	static int already_hit = 0;
	int i;

	// if we're hitting this recursively, just exit
	if (already_hit)
		return EXCEPTION_EXECUTE_HANDLER;
	already_hit = 1;

	// find our man
	for (i = 0; exception_table[i].code != 0; i++)
		if (info->ExceptionRecord->ExceptionCode == exception_table[i].code)
			break;

	// print the exception type and address
	fprintf(stderr, "\n-----------------------------------------------------\n");
	fprintf(stderr, "Exception at EIP=%08X%s: %s\n", (UINT32)info->ExceptionRecord->ExceptionAddress,
			lookup_symbol((UINT32)info->ExceptionRecord->ExceptionAddress), exception_table[i].string);

	// for access violations, print more info
	if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		fprintf(stderr, "While attempting to %s memory at %08X\n",
				info->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
				(UINT32)info->ExceptionRecord->ExceptionInformation[1]);
/*
	UINT32 eip, ebp, esp;
	// attempt to print a call chain
	fprintf(stderr, "\nCall chain:\n");
	eip = (UINT32)info->ExceptionRecord->ExceptionAddress;
	ebp = info->ContextRecord->Ebp;
	esp = info->ContextRecord->Esp;
	while (1)
	{
		fprintf(stderr, "\t0x%08x\t%s\n", eip, lookup_symbol(eip));
fprintf(stderr, "esp = %08x  ebp = %08x\n", esp, ebp);
		if (esp - ebp >= 0x10000)
			break;

		ebp = *(UINT32 *)ebp;
		eip = *(UINT32 *)(ebp + 4);
	}
*/
	// exit
	return EXCEPTION_EXECUTE_HANDLER;
}



//============================================================
//	lookup_symbol
//============================================================

static const char *lookup_symbol(UINT32 address)
{
	static char buffer[1024];
	FILE *	map = fopen(mapfile_name, "r");
	char	symbol[1024], best_symbol[1024];
	UINT32	addr, best_addr = 0;
	char	line[1024];

	// if no file, return nothing
	if (map == NULL)
		return "";

	// reset the bests
	*best_symbol = 0;
	best_addr = 0;

	// parse the file, looking for map entries
	while (fgets(line, sizeof(line) - 1, map))
		if (!strncmp(line, "                0x", 18))
			if (sscanf(line, "                0x%08x %s", &addr, symbol) == 2)
				if (addr <= address && addr > best_addr)
				{
					best_addr = addr;
					strcpy(best_symbol, symbol);
				}

	// create the final result
	if (address - best_addr > 0x10000)
		return "";
	sprintf(buffer, " (%s+0x%04x)", best_symbol, address - best_addr);
	return buffer;
}
