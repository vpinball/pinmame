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

int verbose	= 0;

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

	if ( errorlog )
	{
		if ( !logfile ) {
			logfile = fopen("error.log","wa");
			if (!logfile)
				perror("unable to open log file\n");
		}
	}
	else {
		if ( logfile ) {
			fclose(logfile);
			logfile = 0;
		}
	}

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

	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

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

	logfile = 0;
}

void cli_frontend_exit(void)
{
	/* close open files */

	if (options.playback) osd_fclose(options.playback);
	options.playback = NULL;

	if (options.record)   osd_fclose(options.record);
	options.record = NULL;

	if (options.language_file) osd_fclose(options.language_file);
	options.language_file = NULL;

	if ( logfile ) fclose(logfile);
	logfile = 0;

	errorlog = 0;
}


/*
 * logerror
 */

void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);
	if ( errorlog ) {
        char szBuffer[512];
        _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), text, arg);
        OutputDebugString(szBuffer);
		if ( logfile )
			fprintf(logfile, szBuffer);
	}
	va_end(arg);
}

int set_option(const char *name, const char *arg, int priority)
{
	return rc_set_option(rc, name, arg, priority);
}

void *get_option(const char *name)
{
	return *(char**) rc_get_option(rc, name)->dest;

//	return Value;
}

bool RegLoadOpts(HKEY hKey, rc_option *pOpt, char* szDefault)
{
	bool fNew = false;

	DWORD dwType;
	DWORD dwSize;
	char szHelp[4096];

	switch ( pOpt->type ) {
	case rc_string:
		dwType = REG_SZ;
		dwSize = sizeof szHelp;
		if ( !hKey || (RegQueryValueEx(hKey, pOpt->name, 0, &dwType, (LPBYTE) &szHelp, &dwSize)!=ERROR_SUCCESS) ) {
			if ( szDefault )
				lstrcpy(szHelp, szDefault);
			else
				lstrcpy(szHelp, pOpt->deflt);

			fNew = true;
		}
		break;
	}

	rc_set_option(rc, pOpt->name, szHelp, 0);
	return fNew;
}

bool regSaveOpts(HKEY hKey, rc_option *pOpt)
{
	bool fFailed = true;
	char *szHelp;

	switch ( pOpt->type ) {
	case rc_string:
		szHelp = *(char**) opts->dest;
		fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_SZ, (LPBYTE) szHelp, lstrlen(szHelp)+1)!=ERROR_SUCCESS);
		break;
	}

	return fFailed;
}

void LoadGlobalSettings()
{
	bool fNew = false;

	rc_option *opts = fileio_opts;

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

	char szDefault[4096];
	while ( opts->name ) {
		if ( opts->type==rc_string ) {
			lstrcpy(szDefault, szInstallDir);
			lstrcat(szDefault, opts->deflt);
			fNew |= RegLoadOpts(hKey, opts, szDefault);
		}
		opts++;
	}

	if ( hKey )
		RegCloseKey(hKey);

	if ( fNew )
		SaveGlobalSettings();
}

void SaveGlobalSettings()
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
		if ( opts->type==rc_string )
			regSaveOpts(hKey, opts);
		opts++;
	}

	RegCloseKey(hKey);
}

void DeleteGlobalSettings()
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
