#include "stdafx.h"
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "windows.h"
#include "VPinMAMEConfig.h"
#include "ControllerRegKeys.h"

extern "C" {
#include "driver.h"
#include "rc.h"
#include "misc.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

extern struct rc_option fileio_opts[];
extern struct rc_option input_opts[];
extern struct rc_option sound_opts[];
extern struct rc_option video_opts[];

int verbose					= 0;

/* fix me - need to have the core call osd_set_gamma with this value */
/* instead of relying on the name of an osd variable */
extern float gamma_correct;

/* fix me - need to have the core call osd_set_mastervolume with this value */
/* instead of relying on the name of an osd variable */
extern int attenuation;

char *rompath_extra;
}

int fAllowWriteAccess = 1;

static int config_handle_arg(char *arg);

static FILE *logfile;
static int errorlog;
static int showconfig;
static int showusage;
static int readconfig;
static int createconfig;

static struct rc_struct *rc;

static char *debugres;
static char *playbackname;
static char *recordname;
static char *gamename;

static float f_beam;
static float f_flicker;

static int video_set_beam(struct rc_option *option, const char *arg, int priority)
{
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	if (options.beam > 0x00100000)
		options.beam = 0x00100000;
	option->priority = priority;
	return 0;
}

static int video_set_flicker(struct rc_option *option, const char *arg, int priority)
{
	options.vector_flicker = (int)(f_flicker * 2.55);
	if (options.vector_flicker < 0)
		options.vector_flicker = 0;
	if (options.vector_flicker > 255)
		options.vector_flicker = 255;
	option->priority = priority;
	return 0;
}

static int video_set_debugres(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		options.debug_width = options.debug_height = 0;
	}
	else if(sscanf(arg, "%dx%d", &options.debug_width, &options.debug_height) != 2)
	{
		options.debug_width = options.debug_height = 0;
		fprintf(stderr, "error: invalid value for debugres: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg, int priority)
{
	if ((options.color_depth != 0) &&
		(options.color_depth != 8) &&
		(options.color_depth != 15) &&
		(options.color_depth != 16) &&
		(options.color_depth != 32))
	{
		options.color_depth = 0;
		fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

static int init_errorlog(struct rc_option *option, const char *arg, int priority)
{
	/* provide errorlog from here on */
/*
	if (errorlog && !logfile)
	{
		logfile = fopen("error.log","wa");
		if (!logfile)
		{
			perror("unable to open log file\n");
			exit (1);
		}
	}
*/
	option->priority = priority;
	return 0;
}

static int enable_sound = 1;
static int sound_enable_sound(struct rc_option *option, const char *arg, int priority)
{
	if ( !enable_sound )
		options.samplerate = 0;

	return 0;
}

/* struct definitions */
static struct rc_option opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, video_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, sound_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, input_opts, NULL, 0,	0, NULL, NULL },

	/* options supported by the mame core */
	/* video */
	{ "Mame CORE video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "bpp", NULL, rc_int, &options.color_depth, "0",	0, 0, video_verify_bpp, "specify the colordepth the core should render in bits per pixel (bpp), one of: auto(0), 8, 16, 32" },
	{ "norotate", NULL, rc_bool , &options.norotate, "0", 0, 0, NULL, "do not apply rotation" },
	{ "ror", NULL, rc_bool, &options.ror, "0", 0, 0, NULL, "rotate screen clockwise" },
	{ "rol", NULL, rc_bool, &options.rol, "0", 0, 0, NULL, "rotate screen anti-clockwise" },
	{ "flipx", NULL, rc_bool, &options.flipx, "0", 0, 0, NULL, "flip screen upside-down" },
	{ "flipy", NULL, rc_bool, &options.flipy, "0", 0, 0, NULL, "flip screen left-right" },
	{ "debug_resolution", "dr", rc_string, &debugres, "auto", 0, 0, video_set_debugres, "set resolution for debugger window" },
	/* make it options.gamma_correction? */
	{ "gamma", NULL, rc_float, &gamma_correct , "1.0", 0.5, 2.0, NULL, "gamma correction"},

#ifdef PINMAME_EXT
	/* PinMAME options */
	{ "PinMAME options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "dmd_red",    NULL, rc_int, &pmoptions.dmd_red,   "225", 0, 255, NULL, "DMD color: Red" },
	{ "dmd_green",  NULL, rc_int, &pmoptions.dmd_green, "224", 0, 255, NULL, "DMD color: Green" },
	{ "dmd_blue",   NULL, rc_int, &pmoptions.dmd_blue,   "32", 0, 255, NULL, "DMD color: Blue" },
	{ "dmd_perc0",	NULL, rc_int, &pmoptions.dmd_perc0,  "20", 0, 100, NULL, "DMD off intensity [%]" },
	{ "dmd_perc33",	NULL, rc_int, &pmoptions.dmd_perc33,  "33", 0, 100, NULL, "DMD low intensity [%]" },
	{ "dmd_perc66", NULL, rc_int, &pmoptions.dmd_perc66,  "67", 0, 100, NULL, "DMD medium intensity [%]" },
	{ "dmd_only",	NULL, rc_bool,&pmoptions.dmd_only,    "0",  0, 0,   NULL, "Show only DMD" },
	{ "dmd_compact",NULL, rc_bool,&pmoptions.dmd_compact, "0",  0, 0,   NULL, "Show comact display" },
	{ "dmd_antialias", NULL, rc_int, &pmoptions.dmd_antialias,  "50", 0, 100, NULL, "DMD antialias intensity [%]" },
	{ "soundlimit", NULL, rc_bool, &pmoptions.soundlimit,  "1", 0, 0, NULL, "Limit Sound slowdown" },
#endif /* PINMAME_EXT */
	/* vector */
	{ "Mame CORE vector game options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "antialias", "aa", rc_bool, &options.antialias, "1", 0, 0, NULL, "draw antialiased vectors" },
	{ "translucency", "tl", rc_bool, &options.translucency, "1", 0, 0, NULL, "draw translucent vectors" },
	{ "beam", NULL, rc_float, &f_beam, "1.0", 1.0, 16.0, video_set_beam, "set beam width in vector games" },
	{ "flicker", NULL, rc_float, &f_flicker, "0.0", 0.0, 100.0, video_set_flicker, "set flickering in vector games" },

	/* sound */
	{ "Mame CORE sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "samplerate", "sr", rc_int, &options.samplerate, "44100", 5000, 50000, NULL, "set samplerate" },
	{ "samples", NULL, rc_bool, &options.use_samples, "1", 0, 0, NULL, "use samples" },
	{ "resamplefilter", NULL, rc_bool, &options.use_filter, "1", 0, 0, NULL, "resample if samplerate does not match" },
	{ "sound", NULL, rc_bool, &enable_sound, "1", 0, 0, sound_enable_sound, "enable/disable sound and sound CPUs" },
	{ "volume", "vol", rc_int, &attenuation, "0", -32, 0, NULL, "volume (range [-32,0])" },

	/* misc */
	{ "Mame CORE misc options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "artwork", "art", rc_bool, &options.use_artwork, "1", 0, 0, NULL, "use additional game artwork" },
	{ "cheat", "c", rc_bool, &options.cheat, "0", 0, 0, NULL, "enable/disable cheat subsystem" },
	{ "debug", "d", rc_bool, &options.mame_debug, "0", 0, 0, NULL, "enable/disable debugger (only if available)" },
	{ "playback", "pb", rc_string, &playbackname, NULL, 0, 0, NULL, "playback an input file" },
	{ "record", "rec", rc_string, &recordname, NULL, 0, 0, NULL, "record an input file" },
	{ "log", NULL, rc_bool, &errorlog, "0", 0, 0, init_errorlog, "generate error.log" },

	/* config options */
	{ "Configuration options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "createconfig", "cc", rc_set_int, &createconfig, NULL, 1, 0, NULL, "create the default configuration file" },
	{ "showconfig",	"sc", rc_set_int, &showconfig, NULL, 1, 0, NULL, "display running parameters in rc style" },
	{ "showusage", "su", rc_set_int, &showusage, NULL, 1, 0, NULL, "show this help" },
	{ "readconfig",	"rc", rc_bool, &readconfig, "1", 0, 0, NULL, "enable/disable loading of configfiles" },
	{ "verbose", "v", rc_bool, &verbose, "0", 0, 0, NULL, "display additional diagnostic information" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

/*
 * Penalty string compare, the result _should_ be a measure on
 * how "close" two strings ressemble each other.
 * The implementation is way too simple, but it sort of suits the
 * purpose.
 * This used to be called fuzzy matching, but there's no randomness
 * involved and it is in fact a penalty method.
 */

int penalty_compare (const char *s, const char *l)
{
	int gaps = 0;
	int match = 0;
	int last = 1;

	for (; *s && *l; l++)
	{
		if (*s == *l)
			match = 1;
		else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
			match = 1;
		else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
			match = 1;
		else
			match = 0;

		if (match)
			s++;

		if (match != last)
		{
			last = match;
			if (!match)
				gaps++;
		}
	}

	/* penalty if short string does not completely fit in */
	for (; *s; s++)
		gaps++;

	return gaps;
}

/*
 * We compare the game name given on the CLI against the long and
 * the short game names supported
 */
void show_approx_matches(void)
{
	struct { int penalty; int index; } topten[10];
	int i,j;
	int penalty; /* best fuzz factor so far */

	for (i = 0; i < 10; i++)
	{
		topten[i].penalty = 9999;
		topten[i].index = -1;
	}

	for (i = 0; (drivers[i] != 0); i++)
	{
		int tmp;

		penalty = penalty_compare (gamename, drivers[i]->description);
		tmp = penalty_compare (gamename, drivers[i]->name);
		if (tmp < penalty) penalty = tmp;

		/* eventually insert into table of approximate matches */
		for (j = 0; j < 10; j++)
		{
			if (penalty >= topten[j].penalty) break;
			if (j > 0)
			{
				topten[j-1].penalty = topten[j].penalty;
				topten[j-1].index = topten[j].index;
			}
			topten[j].index = i;
			topten[j].penalty = penalty;
		}
	}

	for (i = 9; i >= 0; i--)
	{
		if (topten[i].index != -1)
			fprintf (stderr, "%-10s%s\n", drivers[topten[i].index]->name, drivers[topten[i].index]->description);
	}
}

/*
 * gamedrv  = NULL --> parse named configfile
 * gamedrv != NULL --> parse gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int parse_config (const char* filename, const struct GameDriver *gamedrv)
{
	FILE *f;
	char buffer[128];
	int retval = 0;

	if (!readconfig) return 0;

	if (gamedrv)
	{
		if (gamedrv->clone_of && strlen(gamedrv->clone_of->name))
		{
			retval = parse_config (NULL, gamedrv->clone_of);
			if (retval)
				return retval;
		}
		sprintf(buffer, "%s.ini", gamedrv->name);
	}
	else
	{
		sprintf(buffer, "%s", filename);
	}

	if (verbose)
		fprintf(stderr, "trying to parse %s\n", buffer);

	f = fopen (buffer, "r");
	if (f)
	{
		if(rc_read(rc, f, buffer, 1, 1))
		{
			if (verbose)
				fprintf (stderr, "problem parsing %s\n", buffer);
			retval = 1;
		}
	}

	if (f)
		fclose (f);

	return retval;
}

void cli_frontend_init()
{
	/* clear all core options */
	memset(&options,0,sizeof(options));

	/* directly define these */
	options.use_emulated_ym3812 = 1;

	/* create the rc object */
	if (!(rc = rc_create()))
	{
		fprintf (stderr, "error on rc creation\n");
		exit(1);
	}

	if (rc_register(rc, opts))
	{
		fprintf (stderr, "error on registering opts\n");
		exit(1);
	}

	/* need a decent default for debug width/height */
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;

	options.gui_host = 1;
}

void cli_frontend_exit(void)
{
	/* close open files */

	if (options.playback) osd_fclose(options.playback);
	if (options.record)   osd_fclose(options.record);
	if (options.language_file) osd_fclose(options.language_file);
}

static int config_handle_arg(char *arg)
{
	static int got_gamename = 0;

	/* notice: for MESS game means system */
	if (got_gamename)
	{
		fprintf(stderr,"error: duplicate gamename: %s\n", arg);
		return -1;
	}

	rompath_extra = osd_dirname(arg);

	if (rompath_extra && !strlen(rompath_extra))
	{
		free (rompath_extra);
		rompath_extra = NULL;
	}

	gamename = arg;

	if (!gamename || !strlen(gamename))
	{
		fprintf(stderr,"error: no gamename given in %s\n", arg);
		return -1;
	}

	got_gamename = 1;
	return 0;
}


/*
 * logerror
 */

void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);
	if (errorlog) {
        char szBuffer[512];
        _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), text, arg);
        OutputDebugString(szBuffer);
	}
	va_end(arg);
}

int set_option(const char *name, const char *arg, int priority)
{
	return rc_set_option(rc, name, arg, priority);
}

void *get_option(const char *name)
{
	void *Value = *(char**) rc_get_option(rc, name)->dest;

	return Value;
}

void Load_fileio_opts()
{
	BOOL fNew = FALSE;

	rc_option *opts = fileio_opts;

	// initialize all
	char szInstallDir[MAX_PATH];
	GetInstallDir(szInstallDir, sizeof szInstallDir);
	lstrcat(szInstallDir, "\\");

	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);
	lstrcat(szKey, "\\");
	lstrcat(szKey, REG_GLOBALS);

	HKEY hKey = 0;
	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS )
		hKey = 0;

	char szPath[4096];
	DWORD dwType;
	DWORD dwSize;
	while ( opts->name ) {
		if ( opts->type==rc_string ) {
			dwType = REG_SZ;
 			dwSize = sizeof szPath; 

			if ( !hKey || (RegQueryValueEx(hKey, opts->name, 0, &dwType, (LPBYTE) &szPath, &dwSize)!=ERROR_SUCCESS) ) {
				lstrcpy(szPath, szInstallDir);
				lstrcat(szPath, opts->deflt);
				fNew = TRUE;
			}
			rc_set_option(rc, opts->name, szPath, 0);
		}
		opts++;
	}

	if ( hKey )
		RegCloseKey(hKey);

	if ( fNew )
		Save_fileio_opts();
}

void Save_fileio_opts()
{
	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);
	lstrcat(szKey, "\\");
	lstrcat(szKey, REG_GLOBALS);

	HKEY hKey;
	DWORD dwDisposition;
   	if ( RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS )
		return;

	rc_option *opts = fileio_opts;
	while ( opts->name ) {
		if ( opts->type==rc_string ) {
			char *szPath = *(char**) opts->dest;
			RegSetValueEx(hKey, opts->name, 0, REG_SZ, (LPBYTE) szPath, lstrlen(szPath)+1);
		}
		opts++;
	}
	RegCloseKey(hKey);
}

void Delete_fileio_opts()
{
	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);

	HKEY hKey;
   	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_WRITE, &hKey)!=ERROR_SUCCESS )
		return;

	RegDeleteKey(hKey, REG_GLOBALS);

	RegCloseKey(hKey);
}


/* Registry function */

/*Writes a DWORD to the Registry! Opens the Registry Key Specified & Closes When Done*/
BOOL WriteRegistry(char* pszKey, char* pszName, DWORD dwValue) {
	HKEY hKey;
	DWORD dwDisposition;

	if ( !pszKey || !*pszKey )
		return FALSE;

   	if ( RegCreateKeyEx(HKEY_CURRENT_USER, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS )
		return FALSE;

    if ( RegSetValueEx(hKey, pszName, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof dwValue)!=ERROR_SUCCESS ) {
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	
	return TRUE;
}

/*Reads a DWORD to the Registry! Opens the Registry Key Specified & Closes When Done*/
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault) {
	DWORD dwValue = dwDefault; 
	HKEY hKey;
	DWORD dwType;
	DWORD dwSize;

	if ( !pszKey || !*pszKey )
		return FALSE;

   	if ( RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS )
		return dwValue;

	dwType = REG_DWORD;
	dwSize = sizeof dwValue;
    if ( RegQueryValueEx(hKey, pszName, 0, &dwType, (LPBYTE) &dwValue, &dwSize)!=ERROR_SUCCESS ) 
		dwValue = dwDefault;

	RegCloseKey(hKey);
	return dwValue;
}

char* GetInstallDir(char *pszInstallDir, int iSize)
{
	if ( !pszInstallDir )
		return NULL;

	GetModuleFileName(_Module.m_hInst, pszInstallDir, iSize);

	char *lpHelp = pszInstallDir;
	char *lpSlash = NULL;
	while ( *lpHelp ) {
		if ( *lpHelp=='\\' )
			lpSlash = lpHelp;
		lpHelp++;
	}
	if ( lpSlash )
		*lpSlash = '\0';

	return pszInstallDir;
}
