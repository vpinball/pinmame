/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  options.c

  Stores global options and per-game options;

***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <math.h>
#include <driver.h>
#include "mame32.h"
#include "m32util.h"
#include "resource.h"

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static void  SaveOptions(void);
static void  LoadOptions(void);
static void  SaveGlobalOptions(BOOL bBackup);

static void  LoadRegGameOptions(HKEY hKey, options_type *o);
static DWORD GetRegOption(HKEY hKey, char *name);
static void  GetRegBoolOption(HKEY hKey, char *name, BOOL *value);
static char  *GetRegStringOption(HKEY hKey, char *name);

static BOOL  SaveRegGameOptions(HKEY hKey, options_type *o);
static void  PutRegOption(HKEY hKey, char *name, DWORD value);
static void  PutRegBoolOption(HKEY hKey, char *name, BOOL value);
static void  PutRegStringOption(HKEY hKey, char *name, char *option);

static void  ColumnEncodeString(void* data, char* str);
static void  ColumnDecodeString(const char* str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  SplitterEncodeString(void* data, char* str);
static void  SplitterDecodeString(const char* str, void* data);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(void* data, char* str);
static void  FontDecodeString(const char* str, void* data);

static void  SavePlayCount(int game_index);
static void  SaveFolderFlags(char *folderName, DWORD dwFlags);

static void  PutRegObj(HKEY hKey, REG_OPTIONS *regOpt);
static void  GetRegObj(HKEY hKey, REG_OPTIONS *regOpt);

/***************************************************************************
    Internal defines
 ***************************************************************************/

/* Used to create/retrieve Registry database */
/* #define KEY_BASE "Software\\Freeware\\TestMame32" */
#define KEY_BASE "Software\\Freeware\\" MAME32NAME
#define KEY_FMT (KEY_BASE "\\%s")
#define KEY_BACKUP (KEY_BASE "\\.Backup\\%s")

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static settings_type settings;

static options_type gOpts;  /* Used when saving/loading from Registry */
static options_type global; /* Global 'default' options */
static options_type *game;  /* Array of Game specific options */

/* Global UI options */
static REG_OPTIONS regSettings[] =
{
	{"DefaultGame",        RO_STRING,  settings.default_game,      0, 0},
	{"FolderID",           RO_INT,     &settings.folder_id,        0, 0},
	{"ShowScreenShot",     RO_BOOL,    &settings.show_screenshot,  0, 0},
	{"ShowFlyer",          RO_INT,     &settings.show_pict_type,   0, 0},
	{"ShowToolBar",        RO_BOOL,    &settings.show_toolbar,     0, 0},
	{"ShowStatusBar",      RO_BOOL,    &settings.show_statusbar,   0, 0},
	{"ShowFolderList",     RO_BOOL,    &settings.show_folderlist,  0, 0},
	{"GameCheck",          RO_BOOL,    &settings.game_check,       0, 0},
	{"VersionCheck",       RO_BOOL,    &settings.version_check,    0, 0},
	{"JoyGUI",             RO_BOOL,    &settings.use_joygui,       0, 0},
	{"Broadcast",          RO_BOOL,    &settings.broadcast,        0, 0},

	{"SortColumn",         RO_INT,     &settings.sort_column,      0, 0},
	{"SortReverse",        RO_BOOL,    &settings.sort_reverse,     0, 0},
	{"X",                  RO_INT,     &settings.area.x,           0, 0},
	{"Y",                  RO_INT,     &settings.area.y,           0, 0},
	{"Width",              RO_INT,     &settings.area.width,       0, 0},
	{"Height",             RO_INT,     &settings.area.height,      0, 0},

	{"Language",           RO_PSTRING, &settings.language,         0, 0},
	{"FlyerDir",           RO_PSTRING, &settings.flyerdir,         0, 0},
	{"CabinetDir",         RO_PSTRING, &settings.cabinetdir,       0, 0},
	{"MarqueeDir",         RO_PSTRING, &settings.marqueedir,       0, 0},

	{"rompath",            RO_PSTRING, &settings.romdirs,          0, 0},
	{"samplepath",         RO_PSTRING, &settings.sampledirs,       0, 0},
	{"cfg_directory",      RO_PSTRING, &settings.cfgdir,           0, 0},
	{"nvram_directory",    RO_PSTRING, &settings.nvramdir,         0, 0},
#ifdef PINMAME
	{"wave_directory",     RO_PSTRING, &settings.wavedir,          0, 0},
#endif /* PINMAME */
	{"memcard_directory",  RO_PSTRING, &settings.memcarddir,       0, 0},
	{"input_directory",    RO_PSTRING, &settings.inpdir,           0, 0},
	{"hiscore_directory",  RO_PSTRING, &settings.hidir,            0, 0},
	{"state_directory",    RO_PSTRING, &settings.statedir,         0, 0},
	{"artwork_directory",  RO_PSTRING, &settings.artdir,           0, 0},
	{"snapshot_directory", RO_PSTRING, &settings.imgdir,           0, 0},
	{"cheat_directory",    RO_PSTRING, &settings.cheatdir,         0, 0},
	{"cheat_file",         RO_PSTRING, &settings.cheatfile,        0, 0},
	{"history_file",       RO_PSTRING, &settings.history_filename, 0, 0},
	{"mameinfo_file",      RO_PSTRING, &settings.mameinfo_filename,0, 0},

	/* ListMode needs to be before ColumnWidths settings */
	{"ListMode",           RO_ENCODE,  &settings.view,             ListEncodeString,     ListDecodeString},
	{"Splitters",          RO_ENCODE,  settings.splitter,          SplitterEncodeString, SplitterDecodeString},
	{"ListFont",           RO_ENCODE,  &settings.list_font,        FontEncodeString,     FontDecodeString},
	{"ColumnWidths",       RO_ENCODE,  &settings.column_width,     ColumnEncodeString,   ColumnDecodeWidths},
	{"ColumnOrder",        RO_ENCODE,  &settings.column_order,     ColumnEncodeString,   ColumnDecodeString},
	{"ColumnShown",        RO_ENCODE,  &settings.column_shown,     ColumnEncodeString,   ColumnDecodeString},
};

/* Game Options */
static REG_OPTIONS regGameOpts[] =
{
	/* video */
	{ "autoframeskip",          RO_BOOL,    &gOpts.autoframeskip,     0, 0},
	{ "frameskip",              RO_INT,     &gOpts.frameskip,         0, 0},
	{ "waitvsync",              RO_BOOL,    &gOpts.wait_vsync,        0, 0},
	{ "triplebuffer",           RO_BOOL,    &gOpts.use_triplebuf,     0, 0},
	{ "window",                 RO_BOOL,    &gOpts.window_mode,       0, 0},
	{ "ddraw",                  RO_BOOL,    &gOpts.use_ddraw,         0, 0},
	{ "hwstretch",              RO_BOOL,    &gOpts.ddraw_stretch,     0, 0},
	{ "resolution",             RO_STRING,  &gOpts.resolution,        0, 0},
	{ "refresh",                RO_INT,     &gOpts.gfx_refresh,       0, 0},
	{ "scanlines",              RO_BOOL,    &gOpts.scanlines,         0, 0},
	{ "switchres",              RO_BOOL,    &gOpts.switchres,         0, 0},
	{ "switchbpp",              RO_BOOL,    &gOpts.switchbpp,         0, 0},
	{ "maximize",               RO_BOOL,    &gOpts.maximize,          0, 0},
	{ "keepaspect",             RO_BOOL,    &gOpts.keepaspect,        0, 0},
	{ "matchrefresh",           RO_BOOL,    &gOpts.matchrefresh,      0, 0},
	{ "syncrefresh",            RO_BOOL,    &gOpts.syncrefresh,       0, 0},
	{ "dirty",                  RO_BOOL,    &gOpts.use_dirty,         0, 0},
	{ "throttle",               RO_BOOL,    &gOpts.throttle,          0, 0},
	{ "full_screen_brightness", RO_DOUBLE,  &gOpts.gfx_brightness,    0, 0},
	{ "frames_to_run",          RO_INT,     &gOpts.frames_to_display, 0, 0},
	{ "effect",                 RO_STRING,  &gOpts.effect,            0, 0},
	{ "screen_aspect",          RO_STRING,  &gOpts.aspect,            0, 0},

	/* input */
	{ "hotrod",                 RO_BOOL,    &gOpts.hotrod,            0, 0},
	{ "hotrodse",               RO_BOOL,    &gOpts.hotrodse,          0, 0},
	{ "mouse",                  RO_BOOL,    &gOpts.use_mouse,         0, 0},
	{ "joystick",               RO_BOOL,    &gOpts.use_joystick,      0, 0},
	{ "steadykey",              RO_BOOL,    &gOpts.steadykey,         0, 0},

	/* core video */
	{ "bpp",                    RO_INT,     &gOpts.color_depth,       0, 0},
	{ "norotate",               RO_BOOL,    &gOpts.norotate,          0, 0},
	{ "ror",                    RO_BOOL,    &gOpts.ror,               0, 0},
	{ "rol",                    RO_BOOL,    &gOpts.rol,               0, 0},
	{ "flipx",                  RO_BOOL,    &gOpts.flipx,             0, 0},
	{ "flipy",                  RO_BOOL,    &gOpts.flipy,             0, 0},
	{ "debug_resolution",       RO_STRING,  &gOpts.debugres,          0, 0},
	{ "gamma",                  RO_DOUBLE,  &gOpts.gamma_correct,     0, 0},

	/* vector */
	{ "antialias",              RO_BOOL,    &gOpts.antialias,         0, 0},
	{ "translucency",           RO_BOOL,    &gOpts.translucency,      0, 0},
	{ "beam",                   RO_DOUBLE,  &gOpts.f_beam,            0, 0},
	{ "flicker",                RO_DOUBLE,  &gOpts.f_flicker,         0, 0},

	/* sound */
	{ "samplerate",             RO_INT,     &gOpts.samplerate,        0, 0},
	{ "use_samples",            RO_BOOL,    &gOpts.use_samples,       0, 0},
	{ "resamplefilter",         RO_BOOL,    &gOpts.use_filter,        0, 0},
	{ "sound",                  RO_BOOL,    &gOpts.enable_sound,      0, 0},
	{ "volume",                 RO_INT,     &gOpts.attenuation,       0, 0},

	/* misc */
	{ "artwork",                RO_BOOL,    &gOpts.use_artwork,       0, 0},
	{ "cheat",                  RO_BOOL,    &gOpts.cheat,             0, 0},
	{ "debug",                  RO_BOOL,    &gOpts.mame_debug,        0, 0},
/*	{ "playback",               RO_STRING,  &gOpts.playbackname,      0, 0},*/
/*	{ "record",                 RO_STRING,  &gOpts.recordname,        0, 0},*/
	{ "log",                    RO_BOOL,    &gOpts.errorlog,          0, 0},
#ifdef PINMAME
	{ "dmd_red",                RO_INT,     &gOpts.dmd_red,           0, 0},
	{ "dmd_green",              RO_INT,     &gOpts.dmd_green,         0, 0},
	{ "dmd_blue",               RO_INT,     &gOpts.dmd_blue,          0, 0},
	{ "dmd_perc0",              RO_INT,     &gOpts.dmd_perc0,         0, 0},
	{ "dmd_perc33",             RO_INT,     &gOpts.dmd_perc33,        0, 0},
	{ "dmd_perc66",             RO_INT,     &gOpts.dmd_perc66,        0, 0},
	{ "dmd_antialias",          RO_INT,     &gOpts.dmd_antialias,     0, 0},
	{ "dmd_only",               RO_BOOL,    &gOpts.dmd_only,          0, 0},
	{ "dmd_compact",            RO_BOOL,    &gOpts.dmd_only,          0, 0},
#endif /* PINMAME */
};

#define NUM_SETTINGS (sizeof(regSettings) / sizeof(regSettings[0]))
#define NUM_GAMEOPTS (sizeof(regGameOpts) / sizeof(regGameOpts[0]))

static int  num_games = 0;
static BOOL bResetGUI      = FALSE;
static BOOL bResetGameDefs = FALSE;

/* Default sizes based on 8pt font w/sort arrow in that column */
static int default_column_width[] = { 186, 68, 84, 84, 64, 88, 74,108, 60,144 };
static int default_column_shown[] = {   1,  0,  1,  1,  1,  1,  1,  1,  1,  1 };
/* Hidden columns need to go at the end of the order array */
static int default_column_order[] = {   0,  2,  3,  4,  5,  6,  7,  8,  9,  1 };

static char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details" };

static char oldInfoMsg[400] =
MAME32NAME " has detected outdated configuration data.\n\n\
The detected configuration data is from Version %s of " MAME32NAME ".\n\
The current version is %s. It is recommended that the\n\
configuration is set to the new defaults.\n\n\
Would you like to use the new configuration?";

#define DEFAULT_GAME "pacman"

/***************************************************************************
    External functions
 ***************************************************************************/

void OptionsInit(int total_games)
{
	int i;

	num_games = total_games;

	strcpy(settings.default_game, DEFAULT_GAME);
	settings.folder_id       = 0;
	settings.view            = VIEW_REPORT;
	settings.show_folderlist = TRUE;
	settings.show_toolbar    = TRUE;
	settings.show_statusbar  = TRUE;
	settings.show_screenshot = TRUE;
	settings.game_check      = TRUE;
	settings.version_check   = TRUE;
	settings.use_joygui      = FALSE;
	settings.broadcast       = FALSE;

	for (i = 0; i < COLUMN_MAX; i++)
	{
		settings.column_width[i] = default_column_width[i];
		settings.column_order[i] = default_column_order[i];
		settings.column_shown[i] = default_column_shown[i];
	}

	settings.sort_column = 0;
	settings.sort_reverse= FALSE;
	settings.area.x      = 0;
	settings.area.y      = 0;
	settings.area.width  = 640;
	settings.area.height = 400;
	settings.splitter[0] = 150;
	settings.splitter[1] = 300;

	settings.language          = strdup("english");
	settings.flyerdir          = strdup("flyers");
	settings.cabinetdir        = strdup("cabinets");
	settings.marqueedir        = strdup("marquees");

	settings.romdirs           = strdup("roms");
	settings.sampledirs        = strdup("samples");
	settings.cfgdir            = strdup("cfg");
	settings.nvramdir          = strdup("nvram");
#ifdef PINMAME
	settings.wavedir           = strdup("wave");
#endif /* PINMAME */
	settings.memcarddir        = strdup("memcard");
	settings.inpdir            = strdup("inp");
	settings.hidir             = strdup("hi");
	settings.statedir          = strdup("sta");
	settings.artdir            = strdup("artwork");
	settings.imgdir            = strdup("snap");
	settings.cheatdir          = strdup("cheat");
	settings.cheatfile         = strdup("cheat.dat");
	settings.history_filename  = strdup("history.dat");
	settings.mameinfo_filename = strdup("mameinfo.dat");

	settings.list_font.lfHeight         = -8;
	settings.list_font.lfWidth          = 0;
	settings.list_font.lfEscapement     = 0;
	settings.list_font.lfOrientation    = 0;
	settings.list_font.lfWeight         = FW_NORMAL;
	settings.list_font.lfItalic         = FALSE;
	settings.list_font.lfUnderline      = FALSE;
	settings.list_font.lfStrikeOut      = FALSE;
	settings.list_font.lfCharSet        = ANSI_CHARSET;
	settings.list_font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	settings.list_font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	settings.list_font.lfQuality        = DEFAULT_QUALITY;
	settings.list_font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	strcpy(settings.list_font.lfFaceName, "MS Sans Serif");

	settings.list_font_color = (COLORREF)-1;

	global.use_default = FALSE;

	global.play_count   = 0;
	global.has_roms     = UNKNOWN;
	global.has_samples  = UNKNOWN;
	global.is_favorite  = FALSE;

	/* video */
	global.autoframeskip     = TRUE;
	global.frameskip         = 0;
	global.wait_vsync        = FALSE;
	global.use_triplebuf     = FALSE;
	global.window_mode       = FALSE;
	global.use_ddraw         = TRUE;
	global.ddraw_stretch     = TRUE;
	strcpy(global.resolution, "auto");
	global.gfx_refresh       = 0;
	global.scanlines         = FALSE;
	global.switchres         = TRUE;
	global.switchbpp         = TRUE;
	global.maximize          = TRUE;
	global.keepaspect        = TRUE;
	global.matchrefresh      = FALSE;
	global.syncrefresh       = FALSE;
	global.use_dirty         = TRUE;
	global.throttle          = TRUE;
	global.gfx_brightness    = 1.0;
	global.frames_to_display = 0;
	strcpy(global.effect,    "none");
	strcpy(global.aspect,    "4:3");

	/* sound */

	/* input */
	global.hotrod            = FALSE;
	global.hotrodse          = FALSE;
	global.use_mouse         = FALSE;
	global.use_joystick      = FALSE;
	global.steadykey         = FALSE;

	/* Core video */
	global.color_depth       = 0;
	global.norotate          = FALSE;
	global.ror               = FALSE;
	global.rol               = FALSE;
	global.flipx             = FALSE;
	global.flipy             = FALSE;
	strcpy(global.debugres, "auto");
	global.gamma_correct     = 1.0;

	/* Core vector */
	global.antialias         = TRUE;
	global.translucency      = TRUE;
	global.f_beam            = 1.0;
	global.f_flicker         = 0.0;

	/* Sound */
	global.samplerate        = 44100;
	global.use_samples       = TRUE;
	global.use_filter        = TRUE;
	global.enable_sound      = TRUE;
	global.attenuation       = 0;

	/* misc */
	global.use_artwork       = TRUE;
	global.cheat             = FALSE;
	global.mame_debug        = FALSE;
	global.playbackname      = NULL;
	global.recordname        = NULL;
	global.errorlog          = FALSE;
#ifdef PINMAME
        global.dmd_red           = 225;
        global.dmd_green         = 224;
        global.dmd_blue          = 32;
        global.dmd_perc66        = 67;
        global.dmd_perc33        = 33;
        global.dmd_perc0         = 20;
        global.dmd_only          = FALSE;
        global.dmd_compact       = FALSE;
        global.dmd_antialias     = 50;
#endif /* PINMAME */
	/* This allocation should be checked */
	game = (options_type *)malloc(num_games * sizeof(options_type));

	for (i = 0; i < num_games; i++)
	{
		game[i] = global;
		game[i].use_default = TRUE;
	}

	SaveGlobalOptions(TRUE);
	LoadOptions();
}

void OptionsExit(void)
{
    SaveOptions();
    free(game);
    free(settings.language);
    free(settings.romdirs);
    free(settings.sampledirs);
    free(settings.cfgdir);
    free(settings.hidir);
    free(settings.inpdir);
    free(settings.imgdir);
    free(settings.statedir);
    free(settings.artdir);
    free(settings.memcarddir);
    free(settings.flyerdir);
    free(settings.cabinetdir);
    free(settings.marqueedir);
    free(settings.nvramdir);
#ifdef PINMAME
    free(settings.wavedir);
#endif /* PINMAME */
	free(settings.cheatdir);
	free(settings.cheatfile);
	free(settings.history_filename);
	free(settings.mameinfo_filename);
}

options_type * GetDefaultOptions(void)
{
	return &global;
}

options_type * GetGameOptions(int num_game)
{
	int play_count;
	int has_roms;
	int has_samples;
	BOOL is_favorite;

	assert(0 <= num_game && num_game < num_games);

	play_count	= game[num_game].play_count;
	has_roms	= game[num_game].has_roms;
	has_samples = game[num_game].has_samples;
	is_favorite = game[num_game].is_favorite;

	if (game[num_game].use_default)
	{
		game[num_game]				= global;
		game[num_game].use_default	= TRUE;
		game[num_game].play_count	= play_count;
		game[num_game].has_roms 	= has_roms;
		game[num_game].has_samples	= has_samples;
		game[num_game].is_favorite	= is_favorite;

	}
	return &game[num_game];
}

void ResetGUI(void)
{
	bResetGUI = TRUE;
}

void SetViewMode(int val)
{
	settings.view = val;
}

int GetViewMode(void)
{
	return settings.view;
}

void SetGameCheck(BOOL game_check)
{
	settings.game_check = game_check;
}

BOOL GetGameCheck(void)
{
	return settings.game_check;
}

void SetVersionCheck(BOOL version_check)
{
	settings.version_check = version_check;
}

BOOL GetVersionCheck(void)
{
	return settings.version_check;
}

void SetJoyGUI(BOOL use_joygui)
{
	settings.use_joygui = use_joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.use_joygui;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetSavedFolderID(UINT val)
{
	settings.folder_id = val;
}

UINT GetSavedFolderID(void)
{
	return settings.folder_id;
}

void SetShowScreenShot(BOOL val)
{
	settings.show_screenshot = val;
}

BOOL GetShowScreenShot(void)
{
	return settings.show_screenshot;
}

void SetShowFolderList(BOOL val)
{
	settings.show_folderlist = val;
}

BOOL GetShowFolderList(void)
{
	return settings.show_folderlist;
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetShowPictType(int val)
{
	settings.show_pict_type = val;
}

int GetShowPictType(void)
{
	return settings.show_pict_type;
}

void SetDefaultGame(const char *name)
{
	strcpy(settings.default_game,name);
}

const char *GetDefaultGame(void)
{
	return settings.default_game;
}

void SetWindowArea(AREA *area)
{
	memcpy(&settings.area, area, sizeof(AREA));
}

void GetWindowArea(AREA *area)
{
	memcpy(area, &settings.area, sizeof(AREA));
}

void SetListFont(LOGFONT *font)
{
	memcpy(&settings.list_font, font, sizeof(LOGFONT));
}

void GetListFont(LOGFONT *font)
{
	memcpy(font, &settings.list_font, sizeof(LOGFONT));
}

void SetListFontColor(COLORREF uColor)
{
	if (settings.list_font_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_font_color = (COLORREF)-1;
	else
		settings.list_font_color = uColor;
}

COLORREF GetListFontColor(void)
{
	if (settings.list_font_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_font_color;
}

void SetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_width[i] = width[i];
}

void GetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		width[i] = settings.column_width[i];
}

void SetSplitterPos(int splitterId, int pos)
{
	if (splitterId < SPLITTER_MAX)
		settings.splitter[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < SPLITTER_MAX)
		return settings.splitter[splitterId];

	return -1; /* Error */
};

void SetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_order[i] = order[i];
}

void GetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		order[i] = settings.column_order[i];
}

void SetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_shown[i] = shown[i];
}

void GetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		shown[i] = settings.column_shown[i];
}

void SetSortColumn(int column)
{
	settings.sort_reverse = (column < 0) ? TRUE : FALSE;
	settings.sort_column  = abs(column) - 1;
}

int GetSortColumn(void)
{
	int column = settings.sort_column + 1;

	if (settings.sort_reverse)
		column = -(column);

	return column;
}

const char* GetLanguage(void)
{
	return settings.language;
}

void SetLanguage(const char* lang)
{
	if (settings.language != NULL)
	{
		free(settings.language);
		settings.language = NULL;
	}

	if (lang != NULL)
		settings.language = strdup(lang);
}

const char* GetRomDirs(void)
{
	return settings.romdirs;
}

void SetRomDirs(const char* paths)
{
	if (settings.romdirs != NULL)
	{
		free(settings.romdirs);
		settings.romdirs = NULL;
	}

	if (paths != NULL)
		settings.romdirs = strdup(paths);
}

const char* GetSampleDirs(void)
{
	return settings.sampledirs;
}

void SetSampleDirs(const char* paths)
{
	if (settings.sampledirs != NULL)
	{
		free(settings.sampledirs);
		settings.sampledirs = NULL;
	}

	if (paths != NULL)
		settings.sampledirs = strdup(paths);
}

const char* GetCfgDir(void)
{
	return settings.cfgdir;
}

void SetCfgDir(const char* path)
{
	if (settings.cfgdir != NULL)
	{
		free(settings.cfgdir);
		settings.cfgdir = NULL;
	}

	if (path != NULL)
		settings.cfgdir = strdup(path);
}
#ifdef PINMAME
const char* GetWaveDir(void)
{
	return settings.wavedir;
}

void SetWaveDir(const char* path)
{
	if (settings.wavedir != NULL)
	{
		free(settings.wavedir);
		settings.wavedir = NULL;
	}

	if (path != NULL)
		settings.wavedir = strdup(path);
}
#endif /* PINMAME */
const char* GetHiDir(void)
{
	return settings.hidir;
}

void SetHiDir(const char* path)
{
	if (settings.hidir != NULL)
	{
		free(settings.hidir);
		settings.hidir = NULL;
	}

	if (path != NULL)
		settings.hidir = strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvramdir;
}

void SetNvramDir(const char* path)
{
	if (settings.nvramdir != NULL)
	{
		free(settings.nvramdir);
		settings.nvramdir = NULL;
	}

	if (path != NULL)
		settings.nvramdir = strdup(path);
}

const char* GetInpDir(void)
{
	return settings.inpdir;
}

void SetInpDir(const char* path)
{
	if (settings.inpdir != NULL)
	{
		free(settings.inpdir);
		settings.inpdir = NULL;
	}

	if (path != NULL)
		settings.inpdir = strdup(path);
}

const char* GetImgDir(void)
{
	return settings.imgdir;
}

void SetImgDir(const char* path)
{
	if (settings.imgdir != NULL)
	{
		free(settings.imgdir);
		settings.imgdir = NULL;
	}

	if (path != NULL)
		settings.imgdir = strdup(path);
}

const char* GetStateDir(void)
{
	return settings.statedir;
}

void SetStateDir(const char* path)
{
	if (settings.statedir != NULL)
	{
		free(settings.statedir);
		settings.statedir = NULL;
	}

	if (path != NULL)
		settings.statedir = strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artdir;
}

void SetArtDir(const char* path)
{
	if (settings.artdir != NULL)
	{
		free(settings.artdir);
		settings.artdir = NULL;
	}

	if (path != NULL)
		settings.artdir = strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcarddir;
}

void SetMemcardDir(const char* path)
{
	if (settings.memcarddir != NULL)
	{
		free(settings.memcarddir);
		settings.memcarddir = NULL;
	}

	if (path != NULL)
		settings.memcarddir = strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyerdir;
}

void SetFlyerDir(const char* path)
{
	if (settings.flyerdir != NULL)
	{
		free(settings.flyerdir);
		settings.flyerdir = NULL;
	}

	if (path != NULL)
		settings.flyerdir = strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinetdir;
}

void SetCabinetDir(const char* path)
{
	if (settings.cabinetdir != NULL)
	{
		free(settings.cabinetdir);
		settings.cabinetdir = NULL;
	}

	if (path != NULL)
		settings.cabinetdir = strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marqueedir;
}

void SetMarqueeDir(const char* path)
{
	if (settings.marqueedir != NULL)
	{
		free(settings.marqueedir);
		settings.marqueedir = NULL;
	}

	if (path != NULL)
		settings.marqueedir = strdup(path);
}

const char* GetCheatDir(void)
{
	return settings.cheatdir;
}

void SetCheatDir(const char* path)
{
	if (settings.cheatdir != NULL)
	{
		free(settings.cheatdir);
		settings.cheatdir = NULL;
	}

	if (path != NULL)
		settings.cheatdir = strdup(path);
}

const char* GetCheatFileName(void)
{
	return settings.cheatfile;
}

void SetCheatFileName(const char* path)
{
	if (settings.cheatfile != NULL)
	{
		free(settings.cheatfile);
		settings.cheatfile = NULL;
	}

	if (path != NULL)
		settings.cheatfile = strdup(path);
}

const char* GetHistoryFileName(void)
{
	return settings.history_filename;
}

void SetHistoryFileName(const char* path)
{
	if (settings.history_filename != NULL)
	{
		free(settings.history_filename);
		settings.history_filename = NULL;
	}

	if (path != NULL)
		settings.history_filename = strdup(path);
}


const char* GetMAMEInfoFileName(void)
{
	return settings.mameinfo_filename;
}

void SetMAMEInfoFileName(const char* path)
{
	if (settings.mameinfo_filename != NULL)
	{
		free(settings.mameinfo_filename);
		settings.mameinfo_filename = NULL;
	}

	if (path != NULL)
		settings.mameinfo_filename = strdup(path);
}

void ResetGameOptions(int num_game)
{
	int play_count;
	int has_roms;
	int has_samples;

	assert(0 <= num_game && num_game < num_games);

	play_count	= game[num_game].play_count;
	has_roms	= game[num_game].has_roms;
	has_samples = game[num_game].has_samples;

	game[num_game]				= global;
	game[num_game].use_default	= TRUE;
	game[num_game].play_count	= play_count;
	game[num_game].has_roms 	= has_roms;
	game[num_game].has_samples	= has_samples;
}

void ResetGameDefaults(void)
{
	bResetGameDefs = TRUE;
}

void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < num_games; i++)
		ResetGameOptions(i);
}

int  GetHasRoms(int num_game)
{
	assert(0 <= num_game && num_game < num_games);

	return game[num_game].has_roms;
}

void SetHasRoms(int num_game, int has_roms)
{
	assert(0 <= num_game && num_game < num_games);

	game[num_game].has_roms = has_roms;
}

int  GetHasSamples(int num_game)
{
	assert(0 <= num_game && num_game < num_games);

	return game[num_game].has_samples;
}

void SetHasSamples(int num_game, int has_samples)
{
	assert(0 <= num_game && num_game < num_games);

	game[num_game].has_samples = has_samples;
}

int  GetIsFavorite(int num_game)
{
	assert(0 <= num_game && num_game < num_games);

	return game[num_game].is_favorite;
}

void SetIsFavorite(int num_game, int is_favorite)
{
	assert(0 <= num_game && num_game < num_games);

	game[num_game].is_favorite = is_favorite;
}

void IncrementPlayCount(int num_game)
{
	assert(0 <= num_game && num_game < num_games);

	game[num_game].play_count++;

	SavePlayCount(num_game);
}

void SetFolderFlags(char *folderName, DWORD dwFlags)
{
	SaveFolderFlags(folderName, dwFlags);
}

int GetPlayCount(int num_game)
{
	assert(0 <= num_game && num_game < num_games);

	return game[num_game].play_count;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void ColumnEncodeString(void* data, char *str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < COLUMN_MAX; i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void ColumnDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < COLUMN_MAX; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

static void ColumnDecodeWidths(const char* str, void* data)
{
	if (settings.view == VIEW_REPORT)
		ColumnDecodeString(str, data);
}

static void SplitterEncodeString(void* data, char* str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < SPLITTER_MAX; i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void SplitterDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < SPLITTER_MAX; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

static void ListDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int i;

	*value = VIEW_REPORT;

	for (i = VIEW_LARGE_ICONS; i < VIEW_MAX; i++)
	{
		if (strcmp(str, view_modes[i]) == 0)
		{
			*value = i;
			return;
		}
	}
}

static void ListEncodeString(void* data, char *str)
{
	int* value = (int*)data;

	strcpy(str, view_modes[*value]);
}

/* Parse the given comma-delimited string into a LOGFONT structure */
static void FontDecodeString(const char* str, void* data)
{
	LOGFONT* f = (LOGFONT*)data;
	char*	 ptr;
	
	sscanf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
		   &f->lfHeight,
		   &f->lfWidth,
		   &f->lfEscapement,
		   &f->lfOrientation,
		   &f->lfWeight,
		   (int*)&f->lfItalic,
		   (int*)&f->lfUnderline,
		   (int*)&f->lfStrikeOut,
		   (int*)&f->lfCharSet,
		   (int*)&f->lfOutPrecision,
		   (int*)&f->lfClipPrecision,
		   (int*)&f->lfQuality,
		   (int*)&f->lfPitchAndFamily);
	ptr = strrchr(str, ',');
	if (ptr != NULL)
		strcpy(f->lfFaceName, ptr + 1);
}

/* Encode the given LOGFONT structure into a comma-delimited string */
static void FontEncodeString(void* data, char *str)
{
	LOGFONT* f = (LOGFONT*)data;

	sprintf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i,%s",
			f->lfHeight,
			f->lfWidth,
			f->lfEscapement,
			f->lfOrientation,
			f->lfWeight,
			f->lfItalic,
			f->lfUnderline,
			f->lfStrikeOut,
			f->lfCharSet,
			f->lfOutPrecision,
			f->lfClipPrecision,
			f->lfQuality,
			f->lfPitchAndFamily,
			f->lfFaceName);
}

/* Register access functions below */
static void LoadOptions(void)
{
	HKEY  hKey;
	DWORD value;
	LONG  result;
	char  keyString[80];
	int   i;
	char  *ptr;
	BOOL  bResetDefs = FALSE;
	BOOL  bVersionCheck = TRUE;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegOpenKeyEx(HKEY_CURRENT_USER,KEY_BASE,
						  0, KEY_QUERY_VALUE, &hKey);

	if (result == ERROR_SUCCESS)
	{
		BOOL bReset = FALSE;
		char tmp[80];

		strcpy(tmp, GetVersionString());

		GetRegBoolOption(hKey, "ResetGUI",			&bReset);
		GetRegBoolOption(hKey, "ResetGameDefaults", &bResetDefs);
		GetRegBoolOption(hKey, "VersionCheck",		&bVersionCheck);
		if (!bReset && bVersionCheck)
		{
			ptr = GetRegStringOption(hKey, "SaveVersion");
			if (ptr && strcmp(ptr, tmp) != 0)
			{
				char msg[400];
				sprintf(msg,oldInfoMsg, ptr, tmp);
				if (MessageBox(0, msg, "Version Mismatch", MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					bReset = TRUE;
					bResetDefs = TRUE;
				}
			}
		}

		if (bReset)
		{
			RegCloseKey(hKey);

			/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Backup */
			sprintf(keyString,KEY_FMT, ".Backup");
			if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString,
							 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
				return;
		}

		RegDeleteValue(hKey,"ListDetails");
		
		if ((value = GetRegOption(hKey, "FontColor")) != -1)
		{
			if (value == GetSysColor(COLOR_WINDOWTEXT))
				settings.list_font_color = (COLORREF)-1;
			else
				settings.list_font_color = value;
		}

		for (i = 0; i < NUM_SETTINGS; i++)
			GetRegObj(hKey, &regSettings[i]);

		RegCloseKey(hKey);
	}

	sprintf(keyString, (bResetDefs) ? KEY_BACKUP : KEY_FMT, ".Defaults");

	result = RegOpenKeyEx(HKEY_CURRENT_USER, keyString, 0, KEY_QUERY_VALUE, &hKey);

	if (result == ERROR_SUCCESS)
	{
		LoadRegGameOptions(hKey, &global);
		RegCloseKey(hKey);
	}

	for (i = 0 ; i < num_games; i++)
	{
		sprintf(keyString,KEY_FMT,drivers[i]->name);
		result = RegOpenKeyEx(HKEY_CURRENT_USER, keyString, 0, KEY_QUERY_VALUE, &hKey);
		if (result == ERROR_SUCCESS)
		{
			LoadRegGameOptions(hKey, &game[i]);
			RegCloseKey(hKey);
		}
	}
}

static DWORD GetRegOption(HKEY hKey, char *name)
{
	DWORD dwType;
	DWORD dwSize;
	DWORD value = -1;

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_DWORD)
		{
			dwSize = 4;
			RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE)&value, &dwSize);
		}
	}
	return value;
}

static void GetRegBoolOption(HKEY hKey, char *name, BOOL *value)
{
	char *ptr;

	if ((ptr = GetRegStringOption(hKey, name)) != NULL)
	{
		*value = (*ptr == '0') ? FALSE : TRUE;
	}
}

static char *GetRegStringOption(HKEY hKey, char *name)
{
	DWORD dwType;
	DWORD dwSize;
	static char str[300];

	memset(str, '\0', 300);

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_SZ)
		{
			dwSize = 299;
			RegQueryValueEx(hKey, name, 0, &dwType, (unsigned char *)str, &dwSize);
		}
	}
	else
	{
		return NULL;
	}

	return str;
}

static void SavePlayCount(int game_index)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];

	assert(0 <= game_index && game_index < num_games);

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	if (result == ERROR_SUCCESS)
	{
		sprintf(keyString,KEY_FMT,drivers[game_index]->name);
		result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
								0, "", REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

		if (result == ERROR_SUCCESS)
		{
			PutRegOption(hKey, "PlayCount", game[game_index].play_count);
			RegCloseKey(hSubkey);
		}
		RegCloseKey(hKey);
	}
}

DWORD GetFolderFlags(char *folderName)
{
	HKEY  hKey;
	long  value = 0;
	char  keyString[80];

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Folders */
	sprintf(keyString, KEY_FMT, ".Folders");
	if (RegOpenKeyEx(HKEY_CURRENT_USER, keyString, 0, KEY_QUERY_VALUE, &hKey)
		== ERROR_SUCCESS)
	{
		value = GetRegOption(hKey, folderName);
		RegCloseKey(hKey);
	}
	return (value < 0) ? 0 : (DWORD)value;
}

static void SaveFolderFlags(char *folderName, DWORD dwFlags)
{
	HKEY  hKey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Folders */
	sprintf(keyString, KEY_FMT, ".Folders");
	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		PutRegOption(hKey, folderName, dwFlags);

		if (dwFlags == 0) /* Delete this reg key */
			RegDeleteValue(hKey, folderName);

		RegCloseKey(hKey);
	}
}

void SaveOptions(void)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];
	int   i;
	BOOL  saved;

	SaveGlobalOptions(FALSE);

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	for (i = 0; i < num_games; i++)
	{
		sprintf(keyString, KEY_FMT, drivers[i]->name);
		result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
								0, "", REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

		if (result == ERROR_SUCCESS)
		{
			saved = SaveRegGameOptions(hSubkey, &game[i]);
			RegCloseKey(hSubkey);
			if (saved == FALSE)
				RegDeleteKey(hKey,drivers[i]->name);
		}
	}
	RegCloseKey(hKey);
}


void SaveGameOptions(int game_num)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];
	BOOL  saved;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	sprintf(keyString, KEY_FMT, drivers[game_num]->name);
	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		saved = SaveRegGameOptions(hSubkey, &game[game_num]);
		RegCloseKey(hSubkey);
		if (saved == FALSE)
			RegDeleteKey(hKey,drivers[game_num]->name);
	}
	RegCloseKey(hKey);
}

void SaveDefaultOptions(void)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];


	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	sprintf(keyString, KEY_FMT, ".Defaults");
	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		global.use_default = FALSE;
		SaveRegGameOptions(hSubkey, &global);
		RegCloseKey(hSubkey);
	}
	RegCloseKey(hKey);
}

static void PutRegOption(HKEY hKey, char *name, DWORD value)
{
	RegSetValueEx(hKey, name, 0, REG_DWORD, (void *)&value, sizeof(DWORD));
}

static void PutRegBoolOption(HKEY hKey, char *name, BOOL value)
{
	char str[2];

	str[0] = (value) ? '1' : '0';
	str[1] = '\0';

	RegSetValueEx(hKey, name, 0, REG_SZ, (void *)str, 2);
}

static void PutRegStringOption(HKEY hKey, char *name, char *option)
{
	RegSetValueEx(hKey, name, 0, REG_SZ, (void *)option, strlen(option) + 1);
}

static void SaveGlobalOptions(BOOL bBackup)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];

	if (bBackup)
		/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Backup */
		sprintf(keyString, KEY_FMT, ".Backup");
	else
		/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
		strcpy(keyString, KEY_BASE);

	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		int i;

		PutRegStringOption(hKey, "SaveVersion", GetVersionString());

		if (settings.list_font_color != (COLORREF)-1 )
			PutRegOption(hKey, "FontColor", settings.list_font_color);
		else
			RegDeleteValue(hKey, "FontColor");

		for (i = 0; i < NUM_SETTINGS; i++)
			PutRegObj(hKey, &regSettings[i]);
	}
	
	if (! bBackup)
	{
		/* Delete old reg key if it exists */
		RegDeleteKey(hKey,	 "Defaults");
		RegDeleteValue(hKey, "LargeScreenShot");
		RegDeleteValue(hKey, "LoadIcons");

		/* Save ResetGUI flag */
		PutRegBoolOption(hKey, "ResetGUI",			bResetGUI);
		PutRegBoolOption(hKey, "ResetGameDefaults", bResetGameDefs);
		/* Do normal save */
		sprintf(keyString, KEY_FMT,".Defaults");
	}
	else
	{
		/* Do backup save */
		sprintf(keyString, KEY_BACKUP, ".Defaults");
	}

	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, "", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		global.use_default = FALSE;
		SaveRegGameOptions(hSubkey, &global);
		RegCloseKey(hSubkey);
	}

	RegCloseKey(hKey);
}

static BOOL SaveRegGameOptions(HKEY hKey, options_type *o)
{
	int   i;

	PutRegOption(hKey,	   "PlayCount", o->play_count);
	PutRegOption(hKey,	   "ROMS",		o->has_roms);
	PutRegOption(hKey,	   "Samples",	o->has_samples);
	PutRegBoolOption(hKey, "Favorite",	o->is_favorite);

	if (o->use_default == TRUE)
	{
		for (i = 0; i < NUM_GAMEOPTS; i++)
		{
			RegDeleteValue(hKey, regGameOpts[i].m_cName);
		}
		return (o->has_roms != UNKNOWN);
	}

	/* copy passed in options to our struct */
	gOpts = *o;

	for (i = 0; i < NUM_GAMEOPTS; i++)
		PutRegObj(hKey, &regGameOpts[i]);

	return TRUE;
}

static void LoadRegGameOptions(HKEY hKey, options_type *o)
{
	int 	i;
	DWORD	value;
	DWORD	size;

	value = GetRegOption(hKey, "PlayCount");
	if (value != -1)
		o->play_count = value;

	value = GetRegOption(hKey, "ROMS");
	if (value != -1)
		o->has_roms = value;

	value = GetRegOption(hKey, "Samples");
	if (value != -1)
		o->has_samples = value;

	GetRegBoolOption(hKey, "Favorite", &o->is_favorite);

	/* look for window.  If it's not there, then use default options for this game */
	if (RegQueryValueEx(hKey, "window", 0, &value, NULL, &size) != ERROR_SUCCESS)
	   return;

	o->use_default = FALSE;

	/* copy passed in options to our struct */
	gOpts = *o;
	
	for (i = 0; i < NUM_GAMEOPTS; i++)
		GetRegObj(hKey, &regGameOpts[i]);

	/* copy options back out */
	*o = gOpts;
}

static void PutRegObj(HKEY hKey, REG_OPTIONS* regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	char*	cName = regOpt->m_cName;
	char	cTemp[80];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpt->m_vpData;
		sprintf(cTemp, "%03.02f", *pDouble);
		PutRegStringOption(hKey, cName, cTemp);
		break;

	case RO_STRING:
		pString = (char*)regOpt->m_vpData;
		if (pString)
			PutRegStringOption(hKey, cName, pString);
		break;

	case RO_PSTRING:
		pString = *(char**)regOpt->m_vpData;
		if (pString)
			PutRegStringOption(hKey, cName, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*)regOpt->m_vpData;
		PutRegBoolOption(hKey, cName, *pBool);
		break;

	case RO_INT:
		pInt = (int*)regOpt->m_vpData;
		PutRegOption(hKey, cName, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(regOpt->m_vpData, cTemp);
		PutRegStringOption(hKey, cName, cTemp);
		break;

	default:
		break;
	}
}

static void GetRegObj(HKEY hKey, REG_OPTIONS* regOpts)
{
	char*	cName = regOpts->m_cName;
	char*	pString;
	int*	pInt;
	double* pDouble;
	int 	value;
	
	switch (regOpts->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpts->m_vpData;
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			sscanf(pString, "%lf", pDouble);
		break;

	case RO_STRING:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			strcpy((char*)regOpts->m_vpData, pString);
		break;

	case RO_PSTRING:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
		{
			if (*(char**)regOpts->m_vpData != NULL)
				free(*(char**)regOpts->m_vpData);
			*(char**)regOpts->m_vpData = strdup(pString);
		}
		break;

	case RO_BOOL:
		GetRegBoolOption(hKey, cName, (BOOL*)regOpts->m_vpData);
		break;

	case RO_INT:
		pInt = (BOOL*)regOpts->m_vpData;
		value = GetRegOption(hKey, cName);
		if (value != -1)
			*pInt = value;
		break;

	case RO_ENCODE:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			regOpts->decode(pString, regOpts->m_vpData);
		break;

	default:
		break;
	}
	
}

char* GetVersionString(void)
{
	return build_version;
}

/* End of options.c */
