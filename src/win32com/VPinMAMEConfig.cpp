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

  extern struct rc_option fileio_opts[];
  extern struct rc_option input_opts[];
  extern struct rc_option sound_opts[];
  extern struct rc_option video_opts[];
  extern struct rc_option pinmame_opts[];
  extern struct rc_option core_opts[];
  extern struct rc_struct *rc;
  extern FILE *config_get_logfile(void);
  // Global options
  int verbose	= 0;
  char *rompath_extra;
}

int fAllowWriteAccess = 1;

int dmd_border = 1;
int dmd_title  = 1;
int dmd_pos_x  = 0;
int dmd_pos_y  = 0;
int dmd_doublesize = 0;
int dmd_width = 0;
int dmd_height = 0;

int threadpriority = 1;
//int synclevel = 60;
int synclevel = 0;		//SJE: Default synclevel is 0 now.. 10/01/03

static FILE *logfile;

static struct rc_option vpinmame_opts[] = {
	// VPinMAME options
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "dmd_border", NULL, rc_bool, &dmd_border, "1", 0, 0, NULL, "DMD display border" },
	{ "dmd_title",  NULL, rc_bool, &dmd_title,  "1", 0, 0, NULL, "DMD display title" },
	{ "dmd_pos_x",  NULL, rc_int,  &dmd_pos_x,  "0", -10000, 10000, NULL, "DMD display position x" },
	{ "dmd_pos_y",  NULL, rc_int,  &dmd_pos_y,  "0", -10000, 10000, NULL, "DMD display position y" },
	{ "dmd_width",  NULL, rc_int,  &dmd_width,  "0", 0, 10000, NULL, "DMD display width" },
	{ "dmd_height", NULL, rc_int,  &dmd_height, "0", 0, 10000, NULL, "DMD display height" },
	{ "dmd_doublesize",  NULL, rc_bool,  &dmd_doublesize,  "0", 0, 0, NULL, "DMD display doublesize" },
	{ "threadpriority",  NULL, rc_int,  &threadpriority,  "1", 0, 2, NULL, "priority of the worker thread" },
	{ "synclevel",  NULL, rc_int,  &synclevel,  "0", -50, 60, NULL, "Sync. of frame rate for external programs (fps)" },	//SJE: Default synclevel is 0 now.. 10/01/03
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

/* struct definitions */
static struct rc_option opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, video_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, sound_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, input_opts, NULL, 0,	0, NULL, NULL },

	{ NULL, NULL, rc_link, core_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, pinmame_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, vpinmame_opts, NULL, 0,	0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

static char* GlobalSettings[] = {
	// fileio_opts
	"rompath",
	"samplepath",
	"cfg_directory",
	"nvram_directory",
	"memcard_directory",
	"input_directory",
	"hiscore_directory",
	"state_directory",
	"artwork_directory",
	"snapshot_directory",
	"diff_directory",
	"wave_directory",
	"cheat_file",
	"history_file",
	"mameinfo_file",

	// input_opts
	"hotrod",
	"hotrodse",
	"mouse",
	"joystick",
	"steadykey",

	// video_opts
	"screen",
	"window",

	NULL
};

static char* PathOrFileSettings[] = {
	// fileio_opts
	"rompath",
	"samplepath",
	"cfg_directory",
	"nvram_directory",
	"memcard_directory",
	"input_directory",
	"hiscore_directory",
	"state_directory",
	"artwork_directory",
	"snapshot_directory",
	"diff_directory",
	"wave_directory",
	"cheat_file",
	"history_file",
	"mameinfo_file",

	NULL
};

static char* IgnoredSettings[] = {
	"resolution",
	"debug_resolution",
	"maximize",
	"throttle",
	"sleep",
	"log",
	
	NULL
};

static char* RunningGameSettings[] = {
	// fileio_opts
	"dmd_pos_x",
	"dmd_pos_y",
	"dmd_doublesize",
	"dmd_border",
	"dmd_title",
	"dmd_width",
	"dmd_height",

	// video_opts
	"screen",
	"window",

	NULL
};

void vpm_frontend_init(void) {
  /* clear all core options */
  memset(&options,0,sizeof(options));
#if MAMEVER < 6100
  /* directly define these */
  options.use_emulated_ym3812 = 1;
#endif /* MAMEVER */
  /* create the rc object */
  if (!(rc = rc_create()))
    { fprintf (stderr, "error on rc creation\n"); exit(1); }
  if (rc_register(rc, opts))
    { fprintf (stderr, "error on registering opts\n"); exit(1); }
  /* need a decent default for debug width/height */
  if (options.debug_width == 0)  options.debug_width = 640;
  if (options.debug_height == 0) options.debug_height = 480;
#if MAMEVER >= 6100
  options.debug_depth = 8; // Debugger only works with 8 bits?
#endif /* MAMEVER */
  options.gui_host = 1;
//#ifdef MAME_DEBUG
//  options.mame_debug = 1;
//#endif /* MAME_DEBUG */
#if (defined(DEBUG) || defined(_DEBUG))
  set_option("log","1",0);
#endif /* DEBUG */
}

void vpm_frontend_exit(void) {
}

extern "C" {
extern UINT8		blit_flipx;
extern UINT8		blit_flipy;
extern UINT8		blit_swapxy;
}

void vpm_game_init(int game_index) {
	logfile = config_get_logfile();

    /* first start with the game's built in orientation */
	int orientation = drivers[game_index]->flags & ORIENTATION_MASK;
	options.ui_orientation = orientation;

	if (options.ui_orientation & ORIENTATION_SWAP_XY)
	{
		/* if only one of the components is inverted, switch them */
		if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
				(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
			options.ui_orientation ^= ROT180;
	}

	/* override if no rotation requested */
	// if (video_norotate)
	if ( (int) get_option("norotate") )
		orientation = options.ui_orientation = ROT0;

	/* rotate right */
//	if (video_ror)
	if ( (int) get_option("ror") )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* rotate left */
	// if (video_rol)
	if ( (int) get_option("rol") )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* auto-rotate right (e.g. for rotating lcds), based on original orientation */
	// if (video_autoror && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	if ( (int) get_option("autoror") && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* auto-rotate left (e.g. for rotating lcds), based on original orientation */
	// if (video_autorol && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	if ( (int) get_option("autorol") && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* flip X/Y */
	// if (video_flipx)
	if ( (int) get_option("flipx") )
		orientation ^= ORIENTATION_FLIP_X;
	// if (video_flipy)
	if ( (int) get_option("flipy") )
		orientation ^= ORIENTATION_FLIP_Y;

	blit_flipx = ((orientation & ORIENTATION_FLIP_X) != 0);
	blit_flipy = ((orientation & ORIENTATION_FLIP_Y) != 0);
	blit_swapxy = ((orientation & ORIENTATION_SWAP_XY) != 0);

	if( options.vector_width == 0 && options.vector_height == 0 )
	{
		options.vector_width = 640;
		options.vector_height = 480;
	}
	if( blit_swapxy )
	{
		int temp;
		temp = options.vector_width;
		options.vector_width = options.vector_height;
		options.vector_height = temp;
	}
}

void vpm_game_exit(int game_index) {
  /* close open files */
  if (options.language_file) /* this seems to never be opened in Win32 version */
    { mame_fclose(options.language_file); options.language_file = NULL; }
  if (logfile)
    { fclose(logfile); logfile = NULL; }
}

#if (!defined(PINMAME) || defined(MAME_DEBUG) || defined(_DEBUG)) // In PinMAME, log only in debug mode.
void CLIB_DECL logerror(const char *text,...) {
  va_list arg;
  /* standard vfprintf stuff here */
  va_start(arg, text);
  if (logfile) {
    char szBuffer[512];
    _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), text, arg);
    OutputDebugString(szBuffer);
    fprintf(logfile, szBuffer);
  }
  va_end(arg);
}
#endif /* PINMAME DEBUG */

int set_option(const char *name, const char *arg, int priority)
{
	return rc_set_option(rc, name, arg, priority);
}

void *get_option(const char *name)
{
	return *(char**) rc_get_option(rc, name)->dest;

//	return Value;
}

BOOL FindSettingInList(char* pList[], const char* pszName)
{
	if ( !pszName || !*pszName )
		return FALSE;

	while ( *pList ) {
		if ( _strcmpi(*pList, pszName)==0 )
			return TRUE;
		pList++;
	}

	return false;
}

BOOL IsGlobalSetting(const char* pszName)
{
	return FindSettingInList(GlobalSettings, pszName);
}

BOOL IsPathOrFile(const char* pszName)
{
	return FindSettingInList(PathOrFileSettings, pszName);
}

BOOL IgnoreSetting(const char* pszName)
{
	return FindSettingInList(IgnoredSettings, pszName);
}

BOOL SettingAffectsRunningGame(const char* pszName)
{
	return FindSettingInList(RunningGameSettings, pszName);
}

bool RegLoadOpts(HKEY hKey, rc_option *pOpt, char* pszDefault, char* pszValue)
{
	if ( !pszValue )
		return false;

	bool fNew = false;

	DWORD dwType;
	DWORD dwSize;
	char szValue[4096];
	DWORD dwValue;

	switch ( pOpt->type ) {
	case rc_string:
	case rc_float:
		dwType = REG_SZ;
		dwSize = sizeof szValue;
		if ( !hKey || (RegQueryValueEx(hKey, pOpt->name, 0, &dwType, (LPBYTE) &szValue, &dwSize)!=ERROR_SUCCESS) ) {
			if ( pszDefault )
				lstrcpy(szValue, pszDefault);
			else {
				if ( pOpt->deflt )
					lstrcpy(szValue, pOpt->deflt);
				else {
					if ( pOpt->type==rc_string )
						lstrcpy(szValue, "");
					else
						lstrcpy(szValue, "0.0");
				}
			}

			fNew = true;
		}
		break;

	case rc_int:
		dwType = REG_DWORD;
		dwSize = sizeof dwValue;
		if ( !hKey || (RegQueryValueEx(hKey, pOpt->name, 0, &dwType, (LPBYTE) &dwValue, &dwSize)!=ERROR_SUCCESS) ) {
			if ( pszDefault )
				lstrcpy(szValue, pszDefault);
			else {
				if ( pOpt->deflt )
					lstrcpy(szValue, pOpt->deflt);
				else
					lstrcpy(szValue, "0");
			}
			fNew = true;
		}
		else
			sprintf(szValue, "%i", dwValue);
		break;

	case rc_bool:
		dwType = REG_DWORD;
		dwSize = sizeof dwValue;
		if ( !hKey || (RegQueryValueEx(hKey, pOpt->name, 0, &dwType, (LPBYTE) &dwValue, &dwSize)!=ERROR_SUCCESS) ) {
			if ( pszDefault )
				lstrcpy(szValue, pszDefault);
			else {
				if ( pOpt->deflt )
					lstrcpy(szValue, pOpt->deflt);
				else
					lstrcpy(szValue, "0");
			}

			fNew = true;
		}
		else
			sprintf(szValue, "%i", dwValue?1:0);
		break;
	}

	lstrcpy(pszValue, szValue);
	return fNew;
}

bool RegSaveOpts(HKEY hKey, rc_option *pOpt, void* pValue)
{
	bool fFailed = true;
	char *pszValue;
	DWORD dwValue;
	char szTemp[256];

	if ( !pValue )
		return fFailed;

	switch ( pOpt->type ) {
	case rc_string:
		pszValue = *(char**) pValue;
		if ( pszValue )
			fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_SZ, (LPBYTE) pszValue, lstrlen(pszValue)+1)!=ERROR_SUCCESS);
		else {
			lstrcpy(szTemp, "");
			fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_SZ, (LPBYTE) &szTemp, lstrlen(szTemp)+1)!=ERROR_SUCCESS);
		}
		break;

	case rc_int:
		fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_DWORD, (LPBYTE) pValue, sizeof DWORD)!=ERROR_SUCCESS);
		break;

	case rc_bool:
		dwValue = *(int*) pValue?1:0;
		fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof dwValue)!=ERROR_SUCCESS);
		break;

	case rc_float:
		sprintf(szTemp, "%f", pValue);
		fFailed = (RegSetValueEx(hKey, pOpt->name, 0, REG_SZ, (LPBYTE) szTemp, lstrlen(szTemp)+1)!=ERROR_SUCCESS);
		break;
	}

	return fFailed;
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

	rc_option* opts[10];
	int sp = 0;

	opts[sp] = ::opts;
	while ( sp>=0 ) {
		switch ( opts[sp]->type ) {
			case rc_bool:
			case rc_string:
			case rc_int:
			case rc_float:
				if ( !IsGlobalSetting(opts[sp]->name) || IgnoreSetting(opts[sp]->name) )
					break;
				
				RegSaveOpts(hKey, opts[sp], opts[sp]->dest);
				break;

			case rc_end:
				sp--;
				continue;

			case rc_link:
				opts[sp+1] = (rc_option*) opts[sp]->dest;
				opts[sp]++;
				sp++;
				continue;
		}
		opts[sp]++;
	}

	RegCloseKey(hKey);
}

void LoadGlobalSettings()
{
	bool fNew = false;

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

	rc_option* opts[10];
	int sp = 0;

	char szValue[4096];

	opts[sp] = ::opts;
	while ( sp>=0 ) {
		switch ( opts[sp]->type ) {
			case rc_bool:
			case rc_string:
			case rc_int:
			case rc_float:
				if ( !IsGlobalSetting(opts[sp]->name) || IgnoreSetting(opts[sp]->name) )
					break;

				char szDefault[512];
				lstrcpy(szDefault, "");
				if ( IsPathOrFile(opts[sp]->name) )
					lstrcpy(szDefault, szInstallDir);
				
				lstrcat(szDefault, opts[sp]->deflt);

				fNew |= RegLoadOpts(hKey, opts[sp], szDefault, szValue);
				rc_set_option3(opts[sp], szValue, 0);
				break;

			case rc_end:
				sp--;
				continue;

			case rc_link:
				opts[sp+1] = (rc_option*) opts[sp]->dest;
				opts[sp]++;
				sp++;
				continue;
		}
		opts[sp]++;
	}

	if ( hKey )
		RegCloseKey(hKey);

	if ( fNew )
		SaveGlobalSettings();
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

void SaveGameSettings(char* pszGameName)
{
	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);
	lstrcat(szKey, "\\");
	if ( pszGameName && *pszGameName )
		lstrcat(szKey, pszGameName);
	else
		lstrcat(szKey, REG_DEFAULT);

	HKEY hKey;
	DWORD dwDisposition;
   	if ( RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS )
		return;

	rc_option* opts[10];
	int sp = 0;

	opts[sp] = ::opts;
	while ( sp>=0 ) {
		switch ( opts[sp]->type ) {
			case rc_bool:
			case rc_string:
			case rc_int:
			case rc_float:
				if ( IsGlobalSetting(opts[sp]->name) || IgnoreSetting(opts[sp]->name) )
					break;

				RegSaveOpts(hKey, opts[sp], opts[sp]->dest);
				break;

			case rc_end:
				sp--;
				continue;

			case rc_link:
				opts[sp+1] = (rc_option*) opts[sp]->dest;
				opts[sp]++;
				sp++;
				continue;
		}
		opts[sp]++;
	}

	RegCloseKey(hKey);
}

void LoadGameSettings(char* pszGameName)
{
	bool fNew = false;

	HKEY hDefaultKey = 0;

	char szDefaultKey[MAX_PATH];
	lstrcpy(szDefaultKey, REG_BASEKEY);
	lstrcat(szDefaultKey, "\\");
	lstrcat(szDefaultKey, REG_DEFAULT);

	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szDefaultKey, 0, KEY_QUERY_VALUE, &hDefaultKey)!=ERROR_SUCCESS )
		hDefaultKey = 0;

	HKEY hGameKey = 0;
	if ( pszGameName && *pszGameName ) {
		char szGameKey[MAX_PATH];
		lstrcpy(szGameKey, REG_BASEKEY);
		lstrcat(szGameKey, "\\");
		lstrcat(szGameKey, pszGameName);
		if ( RegOpenKeyEx(HKEY_CURRENT_USER, szGameKey, 0, KEY_QUERY_VALUE, &hGameKey)!=ERROR_SUCCESS )
			hGameKey = 0;
	}

	char szValue[4096];

	rc_option* opts[10];
	int sp = 0;

	opts[sp] = ::opts;
	while ( sp>=0 ) {
		switch ( opts[sp]->type ) {
			case rc_bool:
			case rc_string:
			case rc_int:
			case rc_float:
				if ( IsGlobalSetting(opts[sp]->name) || IgnoreSetting(opts[sp]->name) )
					break;

				char szDefault[4096];
				RegLoadOpts(hDefaultKey, opts[sp], NULL, szDefault);

				fNew |= RegLoadOpts(hGameKey, opts[sp], szDefault, szValue);
				rc_set_option3(opts[sp], szValue, 0);
				break;

			case rc_end:
				sp--;
				continue;

			case rc_link:
				opts[sp+1] = (rc_option*) opts[sp]->dest;
				opts[sp]++;
				sp++;
				continue;
		}
		opts[sp]++;
	}

	if ( hGameKey )
		RegCloseKey(hGameKey);

	if ( hDefaultKey )
		RegCloseKey(hDefaultKey);

	if ( fNew )
		SaveGameSettings(pszGameName);
}

void DeleteGameSettings(char *pszGameName)
{
	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);

	HKEY hKey;
   	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_WRITE, &hKey)!=ERROR_SUCCESS )
		return;

	if ( pszGameName && *pszGameName )
		RegDeleteKey(hKey, pszGameName);
	else
		RegDeleteKey(hKey, REG_GLOBALS);

	RegCloseKey(hKey);
}

BOOL GetSetting(char* pszGameName, char* pszName, VARIANT *pVal)
{
	if ( !pszName && !*pszName )
		return FALSE;

	if ( (pszGameName && IsGlobalSetting(pszName)) || IgnoreSetting(pszName) )
		return FALSE;

	struct rc_option *option;
	if(!(option = rc_get_option2(opts, pszName)))
		return FALSE;
	
	HKEY hKey = 0;
	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);
	lstrcat(szKey, "\\");

	if ( !pszGameName )
		lstrcat(szKey, REG_GLOBALS);
	else
		lstrcat(szKey, REG_DEFAULT);

	if ( RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS )
		hKey = 0;

	HKEY hGameKey = 0;
	if ( pszGameName && *pszGameName ) {
		char szGameKey[MAX_PATH];
		lstrcpy(szGameKey, REG_BASEKEY);
		lstrcat(szGameKey, "\\");
		lstrcat(szGameKey, pszGameName);
		if ( RegOpenKeyEx(HKEY_CURRENT_USER, szGameKey, 0, KEY_QUERY_VALUE, &hGameKey)!=ERROR_SUCCESS )
			hGameKey = 0;
	}

	char szValue[4096];

	char szHelp[4096];
	lstrcpy(szHelp, "");
	if ( IsPathOrFile(option->name) ) {
		GetInstallDir(szHelp, sizeof szHelp);
		lstrcat(szHelp, "\\");
	}
	lstrcat(szHelp, option->deflt);

	char szDefault[4096];
	RegLoadOpts(hKey, option, szHelp, szDefault);
	RegLoadOpts(hGameKey, option, szDefault, szValue);
	CComVariant vValue(szValue);

	switch ( option->type ) {
	case rc_int:
		VariantChangeType(&vValue, &vValue, 0, VT_I4);
		break;

	case rc_bool:
		VariantChangeType(&vValue, &vValue, 0, VT_BOOL);
		break;

	case rc_float:
		VariantChangeType(&vValue, &vValue, 0, VT_R4);
		break;
	}

	if ( hGameKey )
		RegCloseKey(hGameKey);

	if ( hKey )
		RegCloseKey(hKey);

	vValue.Detach(pVal);

	return TRUE;
}

BOOL PutSetting(char* pszGameName, char* pszName, VARIANT vValue)
{
	if ( !pszName && !*pszName )
		return FALSE;

	if ( (pszGameName && IsGlobalSetting(pszName)) || IgnoreSetting(pszName) )
		return FALSE;

	struct rc_option *option;
	if(!(option = rc_get_option2(opts, pszName)))
		return FALSE;

	char szKey[MAX_PATH];
	lstrcpy(szKey, REG_BASEKEY);
	lstrcat(szKey, "\\");
	if ( !pszGameName )
		lstrcat(szKey, REG_GLOBALS);
	else if ( *pszGameName )
		lstrcat(szKey, pszGameName);
	else
		lstrcat(szKey, REG_DEFAULT);

	HKEY hKey;
	DWORD dwDisposition;
   	if ( RegCreateKeyEx(HKEY_CURRENT_USER, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS )
		return FALSE;

	BOOL fSuccess = TRUE;

	switch ( option->type ) {
		case rc_bool:
			VariantChangeType(&vValue, &vValue, 0, VT_BOOL);
			int nValue;
			nValue = vValue.boolVal ? 1 : 0;
			RegSaveOpts(hKey, option, &nValue);
			break;

		case rc_string:
			VariantChangeType(&vValue, &vValue, 0, VT_BSTR);
			char szValue[4096];
			WideCharToMultiByte(CP_ACP, 0, vValue.bstrVal, -1, szValue, sizeof szValue, NULL, NULL);

			char* pszValue;
			pszValue = szValue;
			RegSaveOpts(hKey, option, &pszValue);
			break;
		
		case rc_int:
			VariantChangeType(&vValue, &vValue, 0, VT_I4);
			RegSaveOpts(hKey, option, &vValue.lVal);
			break;

		case rc_float:
			VariantChangeType(&vValue, &vValue, 0, VT_R4);
			RegSaveOpts(hKey, option, &vValue.fltVal);
			break;

		default:
			fSuccess = FALSE;
			break;
	}

	RegCloseKey(hKey);

	return fSuccess;
}


/* Registry function */

/* Writes a DWORD to the Registry! Opens the Registry Key Specified & Closes When Done*/
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

/* Reads a DWORD from the Registry! Opens the Registry Key Specified & Closes When Done*/
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
