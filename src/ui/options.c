/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

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

#include "screenshot.h"
#include "bitmask.h"
#include "mame32.h"
#include "m32util.h"
#include "resource.h"
#include "treeview.h"
#include "file.h"
#include "splitters.h"
#include "dijoystick.h"
#include "audit.h"
#include "options.h"

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static void LoadFolderFilter(int folder_index,int filters);

static REG_OPTION * GetOption(REG_OPTION *option_array,int num_options,const char *key);
static void LoadOption(REG_OPTION *option,const char *value_str);
static BOOL LoadGameVariableOrFolderFilter(char *key,const char *value);
static void ParseKeyValueStrings(char *buffer,char **key,char **value);
static void LoadOptionsAndSettings(void);
static BOOL LoadOptions(const char *filename,options_type *o,BOOL load_global_game_options);

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value);
static void WriteIntOptionToFile(FILE *fptr,const char *key,int value);
static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value);
static void WriteColorOptionToFile(FILE *fptr,const char *key,COLORREF value);

static BOOL IsOptionEqual(int option_index,options_type *o1,options_type *o2);
static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt);

static void  ColumnEncodeString(void* data, char* str);
static void  ColumnDecodeString(const char* str, void* data);

static void  JoyInfoEncodeString(void *data, char* str);
static void  JoyInfoDecodeString(const char *str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  SplitterEncodeString(void* data, char* str);
static void  SplitterDecodeString(const char* str, void* data);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(void* data, char* str);
static void  FontDecodeString(const char* str, void* data);

static void D3DEffectEncodeString(void *data,char *str);
static void D3DEffectDecodeString(const char *str,void *data);

static void D3DPrescaleEncodeString(void *data,char *str);
static void D3DPrescaleDecodeString(const char *str,void *data);

static void CleanStretchEncodeString(void *data,char *str);
static void CleanStretchDecodeString(const char *str,void *data);

static void CurrentTabEncodeString(void *data,char *str);
static void CurrentTabDecodeString(const char *str,void *data);

static void FolderFlagsEncodeString(void *data,char *str);
static void FolderFlagsDecodeString(const char *str,void *data);

static void TabFlagsEncodeString(void *data,char *str);
static void TabFlagsDecodeString(const char *str,void *data);

/***************************************************************************
    Internal defines
 ***************************************************************************/

#define UI_INI_FILENAME MAME32NAME "ui.ini"
#define DEFAULT_OPTIONS_INI_FILENAME MAME32NAME ".ini"

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	int folder_index;
	int filters;
} folder_filter_type;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static settings_type settings;

static options_type gOpts;  // Used in conjunction with regGameOpts

static options_type global; // Global 'default' options
static options_type *game_options;  // Array of Game specific options
static game_variables_type *game_variables;  // Array of game specific extra data

// UI options in mame32ui.ini
static REG_OPTION regSettings[] =
{
	{"default_game",       RO_STRING,  &settings.default_game,      0, 0},
	{"default_folder_id",  RO_INT,     &settings.folder_id,        0, 0},
	{"show_image_section", RO_BOOL,    &settings.show_screenshot,  0, 0},
	{"current_tab",          RO_ENCODE,&settings.current_tab,
	 CurrentTabEncodeString, CurrentTabDecodeString },
	{"show_tool_bar",      RO_BOOL,    &settings.show_toolbar,     0, 0},
	{"show_status_bar",    RO_BOOL,    &settings.show_statusbar,   0, 0},
	{"show_folder_section",RO_BOOL,    &settings.show_folderlist,  0, 0},
	{"hide_folders",       RO_ENCODE,  &settings.show_folder_flags,
	 FolderFlagsEncodeString, FolderFlagsDecodeString },

	{"show_tabs",          RO_BOOL,    &settings.show_tabctrl,     0, 0},
	{"hide_tabs",          RO_ENCODE,  &settings.show_tab_flags,
	 TabFlagsEncodeString, TabFlagsDecodeString },

	{"check_game",         RO_BOOL,    &settings.game_check,       0, 0},
	{"joystick_in_interface",RO_BOOL,&settings.use_joygui,     0, 0},
	{"broadcast_game_name",RO_BOOL,    &settings.broadcast,        0, 0},
	{"random_background",  RO_BOOL,    &settings.random_bg,        0, 0},

	{"sort_column",        RO_INT,     &settings.sort_column,      0, 0},
	{"sort_reversed",      RO_BOOL,    &settings.sort_reverse,     0, 0},
	{"window_x",           RO_INT,     &settings.area.x,           0, 0},
	{"window_y",           RO_INT,     &settings.area.y,           0, 0},
	{"window_width",       RO_INT,     &settings.area.width,       0, 0},
	{"window_height",      RO_INT,     &settings.area.height,      0, 0},
	{"window_state",       RO_INT,     &settings.windowstate,      0, 0},

	{"text_color",         RO_COLOR,   &settings.list_font_color,  0, 0},
	{"clone_color",        RO_COLOR,   &settings.list_clone_color,  0, 0},
	/* ListMode needs to be before ColumnWidths settings */
	{"list_mode",          RO_ENCODE,  &settings.view,             ListEncodeString,     ListDecodeString},
	{"splitters",          RO_ENCODE,  settings.splitter,          SplitterEncodeString, SplitterDecodeString},
	{"list_font",          RO_ENCODE,  &settings.list_font,        FontEncodeString,     FontDecodeString},
	{"column_widths",      RO_ENCODE,  &settings.column_width,     ColumnEncodeString,   ColumnDecodeWidths},
	{"column_order",       RO_ENCODE,  &settings.column_order,     ColumnEncodeString,   ColumnDecodeString},
	{"column_shown",       RO_ENCODE,  &settings.column_shown,     ColumnEncodeString,   ColumnDecodeString},
	{"ui_joy_up",          RO_ENCODE,  &settings.ui_joy_up,        JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_down",        RO_ENCODE,  &settings.ui_joy_down,      JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_left",        RO_ENCODE,  &settings.ui_joy_left,      JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_right",       RO_ENCODE,  &settings.ui_joy_right,     JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_start",       RO_ENCODE,  &settings.ui_joy_start,     JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_pgup",        RO_ENCODE,  &settings.ui_joy_pgup,      JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_pgdwn",       RO_ENCODE,  &settings.ui_joy_pgdwn,     JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_home",        RO_ENCODE,  &settings.ui_joy_home,      JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_end",         RO_ENCODE,  &settings.ui_joy_end,       JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_ss_change",   RO_ENCODE,  &settings.ui_joy_ss_change, JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_history_up",  RO_ENCODE,  &settings.ui_joy_history_up, JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_history_down",RO_ENCODE,  &settings.ui_joy_history_down, JoyInfoEncodeString,  JoyInfoDecodeString},
	{"ui_joy_exec",        RO_ENCODE,  &settings.ui_joy_exec,      JoyInfoEncodeString,  JoyInfoDecodeString},
	{"exec_command",       RO_STRING,  &settings.exec_command,  0, 0},
	{"exec_wait",          RO_INT,     &settings.exec_wait,  0, 0},
	{"hide_mouse",         RO_BOOL,    &settings.hide_mouse,  0, 0},
	{"full_screen",        RO_BOOL,    &settings.full_screen,  0, 0},
	{"cycle_screenshot",   RO_INT,     &settings.cycle_screenshot,  0, 0},
	{"stretch_screenshot_larger", RO_BOOL, &settings.stretch_screenshot_larger,  0, 0},

	{"language",           RO_STRING,  &settings.language,         0, 0},
	{"flyer_directory",    RO_STRING,  &settings.flyerdir,         0, 0},
	{"cabinet_directory",  RO_STRING,  &settings.cabinetdir,       0, 0},
	{"marquee_directory",  RO_STRING,  &settings.marqueedir,       0, 0},
	{"title_directory",    RO_STRING,  &settings.titlesdir,        0, 0},
	{"cpanel_directory",   RO_STRING,  &settings.cpaneldir,        0, 0},
	{"background_directory",RO_STRING, &settings.bgdir,            0, 0},
	{"folder_directory",   RO_STRING,  &settings.folderdir,        0, 0},
	{"icons_directory",    RO_STRING,  &settings.iconsdir,         0, 0},

};
#define NUM_SETTINGS (sizeof(regSettings) / sizeof(regSettings[0]))

// options in mame32.ini or (gamename).ini
static REG_OPTION regGameOpts[] =
{
	// video
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
	{ "throttle",               RO_BOOL,    &gOpts.throttle,          0, 0},
	{ "full_screen_brightness", RO_DOUBLE,  &gOpts.gfx_brightness,    0, 0},
	{ "frames_to_run",          RO_INT,     &gOpts.frames_to_display, 0, 0},
	{ "effect",                 RO_STRING,  &gOpts.effect,            0, 0},
	{ "screen_aspect",          RO_STRING,  &gOpts.aspect,            0, 0},
	{ "cleanstretch",           RO_ENCODE,  &gOpts.clean_stretch,
	  CleanStretchEncodeString, CleanStretchDecodeString },
	{ "zoom",                   RO_INT,     &gOpts.zoom,              0, 0},

	// d3d
	{ "d3d",                    RO_BOOL,    &gOpts.use_d3d,           0, 0},
	{ "d3dtexmanage",           RO_BOOL,    &gOpts.d3d_texture_management,0, 0},
	{ "d3dfilter",              RO_INT,     &gOpts.d3d_filter,        0, 0},
	{ "d3deffect",              RO_ENCODE,  &gOpts.d3d_effect,
	  D3DEffectEncodeString, D3DEffectDecodeString },
	{ "d3dprescale",            RO_ENCODE,  &gOpts.d3d_prescale,
	  D3DPrescaleEncodeString, D3DPrescaleDecodeString },
	{ "d3deffectrotate",        RO_BOOL,    &gOpts.d3d_rotate_effects,0, 0},
	{ "#*d3dscan_enable",        RO_BOOL,    &gOpts.d3d_scanlines_enable,0,0},
	{ "#*d3dscan",               RO_INT,     &gOpts.d3d_scanlines,     0, 0},
	{ "#*d3dfeedback_enable",    RO_BOOL,    &gOpts.d3d_feedback_enable,0,0},
	{ "#*d3dfeedback",           RO_INT,     &gOpts.d3d_feedback,      0, 0},

	{ "mouse",                  RO_BOOL,    &gOpts.use_mouse,         0, 0},
	{ "joystick",               RO_BOOL,    &gOpts.use_joystick,      0, 0},
	{ "a2d",                    RO_DOUBLE,  &gOpts.f_a2d,             0, 0},
	{ "steadykey",              RO_BOOL,    &gOpts.steadykey,         0, 0},
	{ "lightgun",               RO_BOOL,    &gOpts.lightgun,          0, 0},
	{ "ctrlr",                  RO_STRING,  &gOpts.ctrlr,             0, 0},

	// core video
	{ "brightness",             RO_DOUBLE,  &gOpts.f_bright_correct,  0, 0}, 
	{ "pause_brightness",       RO_DOUBLE,  &gOpts.f_pause_bright    ,0, 0}, 
	{ "norotate",               RO_BOOL,    &gOpts.norotate,          0, 0},
	{ "ror",                    RO_BOOL,    &gOpts.ror,               0, 0},
	{ "rol",                    RO_BOOL,    &gOpts.rol,               0, 0},
	{ "autoror",                RO_BOOL,    &gOpts.auto_ror,          0, 0},
	{ "autorol",                RO_BOOL,    &gOpts.auto_rol,          0, 0},
	{ "flipx",                  RO_BOOL,    &gOpts.flipx,             0, 0},
	{ "flipy",                  RO_BOOL,    &gOpts.flipy,             0, 0},
	{ "debug_resolution",       RO_STRING,  &gOpts.debugres,          0, 0}, 
	{ "gamma",                  RO_DOUBLE,  &gOpts.f_gamma_correct,   0, 0},

	// vector
	{ "antialias",              RO_BOOL,    &gOpts.antialias,         0, 0},
	{ "translucency",           RO_BOOL,    &gOpts.translucency,      0, 0},
	{ "beam",                   RO_DOUBLE,  &gOpts.f_beam,            0, 0},
	{ "flicker",                RO_DOUBLE,  &gOpts.f_flicker,         0, 0},
	{ "intensity",              RO_DOUBLE,  &gOpts.f_intensity,       0, 0},

	// sound
	{ "samplerate",             RO_INT,     &gOpts.samplerate,        0, 0},
	{ "samples",                RO_BOOL,    &gOpts.use_samples,       0, 0},
	{ "resamplefilter",         RO_BOOL,    &gOpts.use_filter,        0, 0},
	{ "sound",                  RO_BOOL,    &gOpts.enable_sound,      0, 0},
	{ "volume",                 RO_INT,     &gOpts.attenuation,       0, 0},
	{ "audio_latency",          RO_INT,     &gOpts.audio_latency,     0, 0},

	// misc artwork options
	{ "artwork",                RO_BOOL,    &gOpts.use_artwork,       0, 0},
	{ "backdrop",               RO_BOOL,    &gOpts.backdrops,         0, 0},
	{ "overlay",                RO_BOOL,    &gOpts.overlays,          0, 0},
	{ "bezel",                  RO_BOOL,    &gOpts.bezels,            0, 0},
	{ "artwork_crop",           RO_BOOL,    &gOpts.artwork_crop,      0, 0},
	{ "artres",                 RO_INT,     &gOpts.artres,            0, 0},

	// misc
	{ "cheat",                  RO_BOOL,    &gOpts.cheat,             0, 0},
	{ "debug",                  RO_BOOL,    &gOpts.mame_debug,        0, 0},
	{ "log",                    RO_BOOL,    &gOpts.errorlog,          0, 0},
	{ "sleep",                  RO_BOOL,    &gOpts.sleep,             0, 0},
	{ "rdtsc",                  RO_BOOL,    &gOpts.old_timing,        0, 0},
	{ "leds",                   RO_BOOL,    &gOpts.leds,              0, 0},
	{ "bios",                   RO_INT,     &gOpts.bios,              0, 0},
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
#define NUM_GAME_OPTIONS (sizeof(regGameOpts) / sizeof(regGameOpts[0]))

// options in mame32.ini that we'll never override with with game-specific options
static REG_OPTION global_game_options[] =
{
	{"skip_disclaimer",    RO_BOOL,    &settings.skip_disclaimer,  0, 0},
	{"skip_gameinfo",      RO_BOOL,    &settings.skip_gameinfo,    0, 0},
	{"high_priority",      RO_BOOL,    &settings.high_priority,    0, 0},

	{"rompath",            RO_STRING,  &settings.romdirs,          0, 0},
	{"samplepath",         RO_STRING,  &settings.sampledirs,       0, 0},
	{"inipath",            RO_STRING,  &settings.inidir,           0, 0},
	{"cfg_directory",      RO_STRING,  &settings.cfgdir,           0, 0},
	{"nvram_directory",    RO_STRING,  &settings.nvramdir,         0, 0},
	{"memcard_directory",  RO_STRING,  &settings.memcarddir,       0, 0},
	{"input_directory",    RO_STRING,  &settings.inpdir,           0, 0},
	{"hiscore_directory",  RO_STRING,  &settings.hidir,            0, 0},
	{"state_directory",    RO_STRING,  &settings.statedir,         0, 0},
	{"artwork_directory",  RO_STRING,  &settings.artdir,           0, 0},
	{"snapshot_directory", RO_STRING,  &settings.imgdir,           0, 0},
	{"diff_directory",     RO_STRING,  &settings.diffdir,          0, 0},
	{"cheat_file",         RO_STRING,  &settings.cheat_filename,   0, 0},
	{"history_file",       RO_STRING,  &settings.history_filename, 0, 0},
	{"mameinfo_file",      RO_STRING,  &settings.mameinfo_filename,0, 0},
	{"ctrlr_directory",    RO_STRING,  &settings.ctrlrdir,         0, 0},
#ifdef PINMAME
	{"wave_directory",     RO_STRING,  &settings.wavedir,          0, 0},
#endif /* PINMAME */

};
#define NUM_GLOBAL_GAME_OPTIONS (sizeof(global_game_options) / sizeof(global_game_options[0]))

typedef struct
{
	const char *name;
	int m_iType;
	size_t m_iOffset;
	BOOL (*m_pfnQualifier)(int driver_index);
	const void *m_vpDefault;
} GAMEVARIABLE_OPTION;

static GAMEVARIABLE_OPTION gamevariable_options[] =
{
	{ "play_count",		RO_INT,		offsetof(game_variables_type, play_count),				NULL,				(const void *) 0},
	{ "play_time",		RO_INT, 	offsetof(game_variables_type, play_time),				NULL,				(const void *) 0},
	{ "rom_audit",		RO_INT,		offsetof(game_variables_type, rom_audit_results),		NULL,				(const void *) UNKNOWN },
	{ "samples_audit",	RO_INT,		offsetof(game_variables_type, samples_audit_results),	DriverUsesSamples,	(const void *) UNKNOWN },
#ifdef MESS
	{ "extra_software",	RO_STRING,	offsetof(game_variables_type, extra_software_paths),	NULL,				(const void *) "" }
#endif
};

// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)
const char* image_tabs_long_name[MAX_TAB_TYPES] =
{
	"Snapshot ",
	"Flyer ",
	"Cabinet ",
	"Marquee",
	"Title",
	"Control Panel",
	"History ",
};

const char* image_tabs_short_name[MAX_TAB_TYPES] =
{
	"snapshot",
	"flyer",
	"cabinet",
	"marquee",
	"title",
	"cpanel",
	"history",
};

// must match D3D_EFFECT_... in options.h, and count must match MAX_D3D_EFFECTS
const char * d3d_effects_long_name[MAX_D3D_EFFECTS] =
{
	"None",
	"Auto",
	"Aperture grille",
	"Dot medium mask",
	"Dot medium bright",
	"RGB minimum mask",
	"RGB medium mask",
	"RGB maximum bright",
	"RGB micro",
	"RGB tiny",
	"RGB 3 pixel triad",
	"RGB 4 pixel triad",
	"RGB 6 pixel triad",
	"RGB 16 pixel triad",
	"Scanlines 25%",
	"Scanlines 50%",
	"Scanlines 75%",
};

const char * d3d_effects_short_name[MAX_D3D_EFFECTS] =
{
	"none",
	"auto",
	"aperturegrille",
	"dotmedmask",
	"dotmedbright",
	"rgbminmask",
	"rgbmedmask",
	"rgbmaxbright",
	"rgbmicro",
	"rgbtiny",
	"rgb3",
	"rgb4",
	"rgb6",
	"rgb16",
	"scan25",
	"scan50",
	"scan75",
};

// must match D3D_PRESCALE_... in options.h, and count must match MAX_D3D_PRESCALE
const char * d3d_prescale_long_name[MAX_D3D_PRESCALE] =
{
	"None",
	"Auto",
	"Full",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
};

const char * d3d_prescale_short_name[MAX_D3D_PRESCALE] =
{
	"none",
	"auto",
	"full",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
};

const char * d3d_filter_long_name[MAX_D3D_FILTERS] =
{
	"None",
	"Bilinear",
	"Cubic (flat kernel)",
	"Cubic (gaussian kernel)",
	"Anisotropic",
};

const char * clean_stretch_long_name[MAX_CLEAN_STRETCH] =
{
	"None",
	"Auto",
	"Full",
	"Horizontal",
	"Vertical",
};
	
const char * clean_stretch_short_name[MAX_CLEAN_STRETCH] =
{
	"none",
	"auto",
	"full",
	"horizontal",
	"vertical",
};
	

static int  num_games = 0;
static BOOL save_gui_settings = TRUE;
static BOOL save_default_options = TRUE;

// Default sizes based on 8pt font w/sort arrow in that column
static int default_column_width[] = { 185, 68, 84, 84, 64, 88, 74,108, 60,144, 84, 60 };
static int default_column_shown[] = {   1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0, 0 };

// Hidden columns need to go at the end of the order array
static int default_column_order[] = {   0,  2,  3,  4,  5,  6,  7,  8,  9,  1, 10, 11 };

static const char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details", "Grouped" };

folder_filter_type *folder_filters;
int size_folder_filters;
int num_folder_filters;

/***************************************************************************
    External functions  
 ***************************************************************************/

BOOL OptionsInit()
{
	int i;

	extern const char g_szDefaultGame[];

	num_games = GetNumGames();

	settings.default_game    = _strdup(g_szDefaultGame);
	settings.folder_id       = 0;
	settings.view            = VIEW_GROUPED;
	settings.show_folderlist = TRUE;
	settings.show_folder_flags = NewBits(MAX_FOLDERS);
	SetAllBits(settings.show_folder_flags,TRUE);

	settings.show_toolbar    = TRUE;
	settings.show_statusbar  = TRUE;
	settings.show_screenshot = TRUE;
	settings.show_tabctrl    = TRUE;
	settings.show_tab_flags = (1 << TAB_SCREENSHOT) | (1 << TAB_FLYER)
		| (1 << TAB_CABINET) | (1 << TAB_MARQUEE) | (1 << TAB_TITLE) | (1 << TAB_CONTROL_PANEL);
	settings.game_check      = TRUE;
	settings.use_joygui      = FALSE;
	settings.broadcast       = FALSE;
	settings.random_bg       = FALSE;

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
	settings.windowstate = 1;
	settings.splitter[0] = 152;
	settings.splitter[1] = 362;

	settings.ui_joy_up[0] = 1;
	settings.ui_joy_up[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_up[2] = 2;
	settings.ui_joy_up[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_down[0] = 1;
	settings.ui_joy_down[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_down[2] = 2;
	settings.ui_joy_down[3] = JOYCODE_DIR_POS;

	settings.ui_joy_left[0] = 1;
	settings.ui_joy_left[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_left[2] = 1;
	settings.ui_joy_left[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_right[0] = 1;
	settings.ui_joy_right[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_right[2] = 1;
	settings.ui_joy_right[3] = JOYCODE_DIR_POS;

	settings.ui_joy_start[0] = 1;
	settings.ui_joy_start[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_start[2] = 1;
	settings.ui_joy_start[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_pgup[0] = 2;
	settings.ui_joy_pgup[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_pgup[2] = 2;
	settings.ui_joy_pgup[3] = JOYCODE_DIR_NEG;

	settings.ui_joy_pgdwn[0] = 2;
	settings.ui_joy_pgdwn[1] = JOYCODE_STICK_AXIS;
	settings.ui_joy_pgdwn[2] = 2;
	settings.ui_joy_pgdwn[3] = JOYCODE_DIR_POS;

	settings.ui_joy_home[0] = 0;
	settings.ui_joy_home[1] = 0;
	settings.ui_joy_home[2] = 0;
	settings.ui_joy_home[3] = 0;

	settings.ui_joy_end[0] = 0;
	settings.ui_joy_end[1] = 0;
	settings.ui_joy_end[2] = 0;
	settings.ui_joy_end[3] = 0;

	settings.ui_joy_history_up[0] = 2;
	settings.ui_joy_history_up[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_history_up[2] = 4;
	settings.ui_joy_history_up[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_history_down[0] = 2;
	settings.ui_joy_history_down[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_history_down[2] = 1;
	settings.ui_joy_history_down[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_ss_change[0] = 2;
	settings.ui_joy_ss_change[1] = JOYCODE_STICK_BTN;
	settings.ui_joy_ss_change[2] = 3;
	settings.ui_joy_ss_change[3] = JOYCODE_DIR_BTN;

	settings.ui_joy_exec[0] = 0;
	settings.ui_joy_exec[1] = 0;
	settings.ui_joy_exec[2] = 0;
	settings.ui_joy_exec[3] = 0;

	settings.exec_command             = _strdup("");
	settings.exec_wait                = 0;
	settings.hide_mouse               = FALSE;
	settings.full_screen              = FALSE;
	settings.cycle_screenshot = 0;
	settings.stretch_screenshot_larger = TRUE;

	settings.language          = _strdup("english");
	settings.flyerdir          = _strdup("flyers");
	settings.cabinetdir        = _strdup("cabinets");
	settings.marqueedir        = _strdup("marquees");
	settings.titlesdir         = _strdup("titles");
	settings.cpaneldir         = _strdup("cpanel");

	settings.romdirs           = _strdup("roms");
	settings.sampledirs        = _strdup("samples");
	settings.inidir 		   = _strdup("ini");
	settings.cfgdir            = _strdup("cfg");
	settings.nvramdir          = _strdup("nvram");
	settings.memcarddir        = _strdup("memcard");
	settings.inpdir            = _strdup("inp");
	settings.hidir             = _strdup("hi");
	settings.statedir          = _strdup("sta");
	settings.artdir            = _strdup("artwork");
	settings.imgdir            = _strdup("snap");
	settings.diffdir           = _strdup("diff");
	settings.iconsdir          = _strdup("icons");
	settings.bgdir             = _strdup("bkground");
#ifdef PINMAME
	settings.wavedir           = _strdup("wave");
#endif /* PINMAME */
	settings.cheat_filename    = _strdup("cheat.dat");
	settings.history_filename  = _strdup("history.dat");
	settings.mameinfo_filename = _strdup("mameinfo.dat");
	settings.ctrlrdir          = _strdup("ctrlr");
	settings.folderdir         = _strdup("folders");

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
	settings.list_clone_color = (COLORREF)-1;
	
	settings.skip_disclaimer = FALSE;
	settings.skip_gameinfo = FALSE;
	settings.high_priority = FALSE;

	/* video */
	global.autoframeskip     = TRUE;
	global.frameskip         = 0;
	global.wait_vsync        = FALSE;
	global.use_triplebuf     = FALSE;
	global.window_mode       = FALSE;
	global.use_ddraw         = TRUE;
	global.ddraw_stretch     = TRUE;
	global.resolution        = _strdup("auto");
	global.gfx_refresh       = 0;
	global.scanlines         = FALSE;
	global.switchres         = TRUE;
	global.switchbpp         = TRUE;
	global.maximize          = TRUE;
	global.keepaspect        = TRUE;
	global.matchrefresh      = FALSE;
	global.syncrefresh       = FALSE;
	global.throttle          = TRUE;
	global.gfx_brightness    = 1.0;
	global.frames_to_display = 0;
	global.effect            = _strdup("none");
	global.aspect            = _strdup("4:3");
	global.clean_stretch     = CLEAN_STRETCH_AUTO;
	global.zoom              = 2;

	// d3d
	global.use_d3d = FALSE;
	global.d3d_filter = 1;
	global.d3d_texture_management = TRUE;
	global.d3d_effect = D3D_EFFECT_AUTO;
	global.d3d_prescale = D3D_PRESCALE_AUTO;
	global.d3d_rotate_effects = TRUE;
	global.d3d_scanlines_enable = FALSE;
	global.d3d_scanlines = 50;
	global.d3d_feedback_enable = FALSE;
	global.d3d_feedback = 50;

	/* input */
	global.use_mouse         = FALSE;
	global.use_joystick      = FALSE;
	global.f_a2d             = 0.3;
	global.steadykey         = FALSE;
	global.lightgun          = FALSE;
	global.ctrlr             = _strdup("Standard");

	/* Core video */
	global.f_bright_correct  = 1.0;
	global.f_pause_bright    = 0.65;
	global.norotate          = FALSE;
	global.ror               = FALSE;
	global.rol               = FALSE;
	global.auto_ror          = FALSE;
	global.auto_rol          = FALSE;
	global.flipx             = FALSE;
	global.flipy             = FALSE;
	global.debugres          = _strdup("auto");
	global.f_gamma_correct   = 1.0;

	/* Core vector */
	global.antialias         = TRUE;
	global.translucency      = TRUE;
	global.f_beam            = 1.0;
	global.f_flicker         = 0.0;
	global.f_intensity		 = 1.5;

	/* Sound */
	global.samplerate        = 44100;
	global.use_samples       = TRUE;
	global.use_filter        = TRUE;
	global.enable_sound      = TRUE;
	global.attenuation       = 0;
	global.audio_latency     = 1;

	/* misc artwork options */
	global.use_artwork       = TRUE;
	global.backdrops         = TRUE;
	global.overlays          = TRUE;
	global.bezels            = TRUE;
	global.artwork_crop      = FALSE;
	global.artres            = 0; /* auto */

	/* misc */
	global.cheat             = FALSE;
	global.mame_debug        = FALSE;
	global.errorlog          = FALSE;
	global.sleep             = FALSE;
	global.old_timing        = TRUE;
	global.leds				 = FALSE;
	global.bios              = 0;
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

	// game_options[x] is valid iff game_variables[i].options_loaded == true
	game_options = (options_type *)malloc(num_games * sizeof(options_type));
	game_variables = (game_variables_type *)malloc(num_games * sizeof(game_variables_type));

	if (!game_options || !game_variables)
		return FALSE;

	memset(game_options, 0, num_games * sizeof(options_type));
	memset(game_variables, 0, num_games * sizeof(game_variables_type));

	for (i = 0; i < num_games; i++)
	{
		game_variables[i].play_count = 0;
		game_variables[i].play_time = 0;
		game_variables[i].rom_audit_results = UNKNOWN;
		game_variables[i].samples_audit_results = UNKNOWN;
		
		game_variables[i].options_loaded = FALSE;
		game_variables[i].use_default = TRUE;
	}

	size_folder_filters = 1;
	num_folder_filters = 0;
	folder_filters = (folder_filter_type *)malloc(size_folder_filters*sizeof(folder_filter_type));
	if (!folder_filters)
		return FALSE;

	LoadOptionsAndSettings();

	// have our mame core (file code) know about our rom path
	// this leaks a little, but the win32 file core writes to this string
	set_pathlist(FILETYPE_ROM,_strdup(settings.romdirs));
	set_pathlist(FILETYPE_SAMPLE,_strdup(settings.sampledirs));
#ifdef MESS
	set_pathlist(FILETYPE_CRC,_strdup(settings.crcdir));
#endif
	return TRUE;

}

void OptionsExit(void)
{
	int i;

	for (i=0;i<num_games;i++)
	{
		FreeGameOptions(&game_options[i]);
	}

    free(game_options);

	FreeGameOptions(&global);

    FreeIfAllocated(&settings.default_game);
    FreeIfAllocated(&settings.language);
    FreeIfAllocated(&settings.romdirs);
    FreeIfAllocated(&settings.sampledirs);
    FreeIfAllocated(&settings.inidir);
    FreeIfAllocated(&settings.cfgdir);
    FreeIfAllocated(&settings.hidir);
    FreeIfAllocated(&settings.inpdir);
    FreeIfAllocated(&settings.imgdir);
    FreeIfAllocated(&settings.statedir);
    FreeIfAllocated(&settings.artdir);
    FreeIfAllocated(&settings.memcarddir);
    FreeIfAllocated(&settings.flyerdir);
    FreeIfAllocated(&settings.cabinetdir);
    FreeIfAllocated(&settings.marqueedir);
    FreeIfAllocated(&settings.titlesdir);
    FreeIfAllocated(&settings.cpaneldir);
    FreeIfAllocated(&settings.nvramdir);
    FreeIfAllocated(&settings.diffdir);
    FreeIfAllocated(&settings.iconsdir);
    FreeIfAllocated(&settings.bgdir);
#ifdef PINMAME
    FreeIfAllocated(&settings.wavedir);
#endif /* PINMAME */
	FreeIfAllocated(&settings.cheat_filename);
	FreeIfAllocated(&settings.history_filename);
	FreeIfAllocated(&settings.mameinfo_filename);
    FreeIfAllocated(&settings.ctrlrdir);
	FreeIfAllocated(&settings.folderdir);

	DeleteBits(settings.show_folder_flags);
	settings.show_folder_flags = NULL;

}

// frees the sub-data (strings)
void FreeGameOptions(options_type *o)
{
	int i;

	for (i=0;i<NUM_GAME_OPTIONS;i++)
	{
		if (regGameOpts[i].m_iType == RO_STRING)
		{
			char **string_to_free = 
				(char **)((char *)o + ((char *)regGameOpts[i].m_vpData - (char *)&gOpts));
			if (*string_to_free  != NULL)
			{
				free(*string_to_free);
				*string_to_free = NULL;
			}
		}
	}
}

// performs a "deep" copy--strings in source are allocated and copied in dest
void CopyGameOptions(options_type *source,options_type *dest)
{
	int i;

	*dest = *source;

	// now there's a bunch of strings in dest that need to be reallocated
	// to be a separate copy
	for (i=0;i<NUM_GAME_OPTIONS;i++)
	{
		if (regGameOpts[i].m_iType == RO_STRING)
		{
			char **string_to_copy = 
				(char **)((char *)dest + ((char *)regGameOpts[i].m_vpData - (char *)&gOpts));
			if (*string_to_copy != NULL)
			{
				*string_to_copy = _strdup(*string_to_copy);
			}
		}
	}
}

options_type * GetDefaultOptions(void)
{
	return &global;
}

options_type * GetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	if (game_variables[driver_index].use_default)
	{
		CopyGameOptions(&global,&game_options[driver_index]);
	}

	if (game_variables[driver_index].options_loaded == FALSE)
	{
		LoadGameOptions(driver_index);
		game_variables[driver_index].options_loaded = TRUE;
	}

	return &game_options[driver_index];
}

BOOL GetGameUsesDefaults(int driver_index)
{
	if (driver_index < 0)
	{
		dprintf("got getgameusesdefaults with driver index %i",driver_index);
		return TRUE;
	}
	return game_variables[driver_index].use_default;
}

void SetGameUsesDefaults(int driver_index,BOOL use_defaults)
{
	if (driver_index < 0)
	{
		dprintf("got setgameusesdefaults with driver index %i",driver_index);
		return;
	}
	game_variables[driver_index].use_default = use_defaults;
}

void LoadFolderFilter(int folder_index,int filters)
{
	//dprintf("loaded folder filter %i %i",folder_index,filters);

	if (num_folder_filters == size_folder_filters)
	{
		size_folder_filters *= 2;
		folder_filters = (folder_filter_type *)realloc(
			folder_filters,size_folder_filters * sizeof(folder_filter_type));
	}
	folder_filters[num_folder_filters].folder_index = folder_index;
	folder_filters[num_folder_filters].filters = filters;

	num_folder_filters++;
}

void ResetGUI(void)
{
	save_gui_settings = FALSE;
}

const char * GetImageTabLongName(int tab_index)
{
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	return image_tabs_short_name[tab_index];
}

const char * GetD3DEffectLongName(int d3d_effect)
{
	return d3d_effects_long_name[d3d_effect];
}

const char * GetD3DEffectShortName(int d3d_effect)
{
	return d3d_effects_short_name[d3d_effect];
}

const char * GetD3DPrescaleLongName(int d3d_prescale)
{
	return d3d_prescale_long_name[d3d_prescale];
}

const char * GetD3DPrescaleShortName(int d3d_prescale)
{
	return d3d_prescale_short_name[d3d_prescale];
}

const char * GetD3DFilterLongName(int d3d_filter)
{
	return d3d_filter_long_name[d3d_filter];
}

const char * GetCleanStretchLongName(int clean_stretch)
{
	return clean_stretch_long_name[clean_stretch];
}

const char * GetCleanStretchShortName(int clean_stretch)
{
	return clean_stretch_short_name[clean_stretch];
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

void SetJoyGUI(BOOL use_joygui)
{
	settings.use_joygui = use_joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.use_joygui;
}

void SetCycleScreenshot(int cycle_screenshot)
{
	settings.cycle_screenshot = cycle_screenshot;
}

int GetCycleScreenshot(void)
{
	return settings.cycle_screenshot;
}

void SetStretchScreenShotLarger(BOOL stretch)
{
	settings.stretch_screenshot_larger = stretch;
}

BOOL GetStretchScreenShotLarger(void)
{
	return settings.stretch_screenshot_larger;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetSkipDisclaimer(BOOL skip_disclaimer)
{
	settings.skip_disclaimer = skip_disclaimer;
}

BOOL GetSkipDisclaimer(void)
{
	return settings.skip_disclaimer;
}

void SetSkipGameInfo(BOOL skip_gameinfo)
{
	settings.skip_gameinfo = skip_gameinfo;
}

BOOL GetSkipGameInfo(void)
{
	return settings.skip_gameinfo;
}

void SetHighPriority(BOOL high_priority)
{
	settings.high_priority = high_priority;
}

BOOL GetHighPriority(void)
{
	return settings.high_priority;
}

void SetRandomBackground(BOOL random_bg)
{
	settings.random_bg = random_bg;
}

BOOL GetRandomBackground(void)
{
	return settings.random_bg;
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

BOOL GetShowFolder(int folder)
{
	return TestBit(settings.show_folder_flags,folder);
}

void SetShowFolder(int folder,BOOL show)
{
	if (show)
		SetBit(settings.show_folder_flags,folder);
	else
		ClearBit(settings.show_folder_flags,folder);
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowTabCtrl (BOOL val)
{
	settings.show_tabctrl = val;
}

BOOL GetShowTabCtrl (void)
{
	return settings.show_tabctrl;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetCurrentTab(int val)
{
	settings.current_tab = val;
}

int GetCurrentTab(void)
{
	return settings.current_tab;
}

void SetDefaultGame(const char *name)
{
	FreeIfAllocated(&settings.default_game);

	if (name != NULL)
		settings.default_game = _strdup(name);
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

void SetWindowState(UINT state)
{
	settings.windowstate = state;
}

UINT GetWindowState(void)
{
	return settings.windowstate;
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

void SetListCloneColor(COLORREF uColor)
{
	if (settings.list_clone_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_clone_color = (COLORREF)-1;
	else
		settings.list_clone_color = uColor;
}

COLORREF GetListCloneColor(void)
{
	if (settings.list_clone_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_clone_color;

}

int GetShowTab(int tab)
{
	return (settings.show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab,BOOL show)
{
	if (show)
		settings.show_tab_flags |= 1 << tab;
	else
		settings.show_tab_flags &= ~(1 << tab);
}

// don't delete the last one
BOOL AllowedToSetShowTab(int tab,BOOL show)
{
	int show_tab_flags = settings.show_tab_flags;

	if (show == TRUE)
		return TRUE;

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
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
	if (splitterId < GetSplitterCount())
		settings.splitter[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < GetSplitterCount())
		return settings.splitter[splitterId];

	return -1; /* Error */
}

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
	settings.sort_column = column;
}

int GetSortColumn(void)
{
	return settings.sort_column;
}

void SetSortReverse(BOOL reverse)
{
	settings.sort_reverse = reverse;
}

BOOL GetSortReverse(void)
{
	return settings.sort_reverse;
}

const char* GetLanguage(void)
{
	return settings.language;
}

void SetLanguage(const char* lang)
{
	FreeIfAllocated(&settings.language);

	if (lang != NULL)
		settings.language = _strdup(lang);
}

const char* GetRomDirs(void)
{
	return settings.romdirs;
}

void SetRomDirs(const char* paths)
{
	FreeIfAllocated(&settings.romdirs);

	if (paths != NULL)
	{
		settings.romdirs = _strdup(paths);

		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_ROM,_strdup(settings.romdirs));
	}
}

const char* GetSampleDirs(void)
{
	return settings.sampledirs;
}

void SetSampleDirs(const char* paths)
{
	FreeIfAllocated(&settings.sampledirs);

	if (paths != NULL)
	{
		settings.sampledirs = _strdup(paths);
		
		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_SAMPLE,_strdup(settings.sampledirs));
	}

}

const char * GetIniDir(void)
{
	return settings.inidir;
}

void SetIniDir(const char *path)
{
	FreeIfAllocated(&settings.inidir);

	if (path != NULL)
		settings.inidir = _strdup(path);
}

const char* GetCtrlrDir(void)
{
	return settings.ctrlrdir;
}

void SetCtrlrDir(const char* path)
{
	FreeIfAllocated(&settings.ctrlrdir);

	if (path != NULL)
		settings.ctrlrdir = _strdup(path);
}

const char* GetCfgDir(void)
{
	return settings.cfgdir;
}

void SetCfgDir(const char* path)
{
	FreeIfAllocated(&settings.cfgdir);

	if (path != NULL)
		settings.cfgdir = _strdup(path);
}

const char* GetHiDir(void)
{
	return settings.hidir;
}

void SetHiDir(const char* path)
{
	FreeIfAllocated(&settings.hidir);

	if (path != NULL)
		settings.hidir = _strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvramdir;
}

void SetNvramDir(const char* path)
{
	FreeIfAllocated(&settings.nvramdir);

	if (path != NULL)
		settings.nvramdir = _strdup(path);
}

const char* GetInpDir(void)
{
	return settings.inpdir;
}

void SetInpDir(const char* path)
{
	FreeIfAllocated(&settings.inpdir);

	if (path != NULL)
		settings.inpdir = _strdup(path);
}

const char* GetImgDir(void)
{
	return settings.imgdir;
}

void SetImgDir(const char* path)
{
	FreeIfAllocated(&settings.imgdir);

	if (path != NULL)
		settings.imgdir = _strdup(path);
}

const char* GetStateDir(void)
{
	return settings.statedir;
}

void SetStateDir(const char* path)
{
	FreeIfAllocated(&settings.statedir);

	if (path != NULL)
		settings.statedir = _strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artdir;
}

void SetArtDir(const char* path)
{
	FreeIfAllocated(&settings.artdir);

	if (path != NULL)
		settings.artdir = _strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcarddir;
}

void SetMemcardDir(const char* path)
{
	FreeIfAllocated(&settings.memcarddir);

	if (path != NULL)
		settings.memcarddir = _strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyerdir;
}

void SetFlyerDir(const char* path)
{
	FreeIfAllocated(&settings.flyerdir);

	if (path != NULL)
		settings.flyerdir = _strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinetdir;
}

void SetCabinetDir(const char* path)
{
	FreeIfAllocated(&settings.cabinetdir);

	if (path != NULL)
		settings.cabinetdir = _strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marqueedir;
}

void SetMarqueeDir(const char* path)
{
	FreeIfAllocated(&settings.marqueedir);

	if (path != NULL)
		settings.marqueedir = _strdup(path);
}

const char* GetTitlesDir(void)
{
	return settings.titlesdir;
}

void SetTitlesDir(const char* path)
{
	FreeIfAllocated(&settings.titlesdir);

	if (path != NULL)
		settings.titlesdir = _strdup(path);
}

const char * GetControlPanelDir(void)
{
	return settings.cpaneldir;
}

void SetControlPanelDir(const char *path)
{
	FreeIfAllocated(&settings.cpaneldir);
	if (path != NULL)
		settings.cpaneldir = _strdup(path);
}

const char * GetDiffDir(void)
{
	return settings.diffdir;
}

void SetDiffDir(const char* path)
{
	FreeIfAllocated(&settings.diffdir);

	if (path != NULL)
		settings.diffdir = _strdup(path);
}

const char* GetIconsDir(void)
{
	return settings.iconsdir;
}

void SetIconsDir(const char* path)
{
	FreeIfAllocated(&settings.iconsdir);

	if (path != NULL)
		settings.iconsdir = _strdup(path);
}

const char* GetBgDir (void)
{
	return settings.bgdir;
}

void SetBgDir (const char* path)
{
	FreeIfAllocated(&settings.bgdir);

	if (path != NULL)
		settings.bgdir = _strdup(path);
}
#ifdef PINMAME
const char* GetWaveDir(void)
{
	return settings.wavedir;
}

void SetWaveDir(const char* path)
{
	FreeIfAllocated(&settings.wavedir);

	if (path != NULL)
		settings.wavedir = _strdup(path);
}
#endif /* PINMAME */

const char* GetFolderDir(void)
{
	return settings.folderdir;
}

void SetFolderDir(const char* path)
{
	FreeIfAllocated(&settings.folderdir);

	if (path != NULL)
		settings.folderdir = _strdup(path);
}

const char* GetCheatFileName(void)
{
	return settings.cheat_filename;
}

void SetCheatFileName(const char* path)
{
	FreeIfAllocated(&settings.cheat_filename);

	if (path != NULL)
		settings.cheat_filename = _strdup(path);
}

const char* GetHistoryFileName(void)
{
	return settings.history_filename;
}

void SetHistoryFileName(const char* path)
{
	FreeIfAllocated(&settings.history_filename);

	if (path != NULL)
		settings.history_filename = _strdup(path);
}


const char* GetMAMEInfoFileName(void)
{
	return settings.mameinfo_filename;
}

void SetMAMEInfoFileName(const char* path)
{
	FreeIfAllocated(&settings.mameinfo_filename);

	if (path != NULL)
		settings.mameinfo_filename = _strdup(path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	// make sure it's all loaded up.
	GetGameOptions(driver_index);

	if (game_variables[driver_index].use_default == FALSE)
	{
		FreeGameOptions(&game_options[driver_index]);
		game_variables[driver_index].use_default = TRUE;
		
		// this will delete the custom file
		SaveGameOptions(driver_index);
	}
}

void ResetGameDefaults(void)
{
	save_default_options = FALSE;
}

void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < num_games; i++)
	{
		ResetGameOptions(i);
	}
}

int GetRomAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].rom_audit_results;
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].rom_audit_results = audit_results;
}

int  GetSampleAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].samples_audit_results;
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].samples_audit_results = audit_results;
}

void IncrementPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].play_count++;

	// maybe should do this
	//SavePlayCount(driver_index);
}

int GetPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_count;
}

int GetPlayTime(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_time;
}

void IncrementPlayTime(int driver_index,int playtime)
{
	assert(0 <= driver_index && driver_index < num_games);
	game_variables[driver_index].play_time += playtime;
}

void GetTextPlayTime(int driver_index,char *buf)
{
	int hour, minute, second;
	int temp = game_variables[driver_index].play_time;

	assert(0 <= driver_index && driver_index < num_games);

	hour = temp / 3600;
	temp = temp - 3600*hour;
	minute = temp / 60; //Calc Minutes
	second = temp - 60*minute;

	if (hour == 0)
		sprintf(buf, "%d:%02d", minute, second );
	else
		sprintf(buf, "%d:%02d:%02d", hour, minute, second );
}

int GetUIJoyUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
	
	return settings.ui_joy_up[joycodeIndex];
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_up[joycodeIndex] = val;
}

int GetUIJoyDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_down[joycodeIndex];
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_down[joycodeIndex] = val;
}

int GetUIJoyLeft(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_left[joycodeIndex];
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_left[joycodeIndex] = val;
}

int GetUIJoyRight(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_right[joycodeIndex];
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_right[joycodeIndex] = val;
}

int GetUIJoyStart(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_start[joycodeIndex];
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_start[joycodeIndex] = val;
}

int GetUIJoyPageUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgup[joycodeIndex];
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgup[joycodeIndex] = val;
}

int GetUIJoyPageDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgdwn[joycodeIndex];
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgdwn[joycodeIndex] = val;
}

int GetUIJoyHome(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_home[joycodeIndex];
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_home[joycodeIndex] = val;
}

int GetUIJoyEnd(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_end[joycodeIndex];
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_end[joycodeIndex] = val;
}

int GetUIJoySSChange(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_ss_change[joycodeIndex];
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_ss_change[joycodeIndex] = val;
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_up[joycodeIndex];
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
  
	settings.ui_joy_history_up[joycodeIndex] = val;
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_down[joycodeIndex];
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_history_down[joycodeIndex] = val;
}

void SetUIJoyExec(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_exec[joycodeIndex] = val;
}

int GetUIJoyExec(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_exec[joycodeIndex];
}

char * GetExecCommand(void)
{
	return settings.exec_command;
}

void SetExecCommand(char *cmd)
{
	settings.exec_command = cmd;
}

int GetExecWait(void)
{
	return settings.exec_wait;
}

void SetExecWait(int wait)
{
	settings.exec_wait = wait;
}
 
BOOL GetHideMouseOnStartup(void)
{
	return settings.hide_mouse;
}

void SetHideMouseOnStartup(BOOL hide)
{
	settings.hide_mouse = hide;
}

BOOL GetRunFullScreen(void)
{
	return settings.full_screen;
}

void SetRunFullScreen(BOOL fullScreen)
{
	settings.full_screen = fullScreen;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void ColumnEncodeStringWithCount(void* data, char *str, int count)
{
	int* value = (int*)data;
	int  i;
	char buffer[100];

	_snprintf(buffer,sizeof(buffer),"%d",value[0]);
	
	strcpy(str,buffer);

    for (i = 1; i < count; i++)
	{
		_snprintf(buffer,sizeof(buffer),",%d",value[i]);
		strcat(str,buffer);
	}
}

static void ColumnDecodeStringWithCount(const char* str, void* data, int count)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
    for (i = 0; p && i < count; i++)
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

static void ColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, COLUMN_MAX);
}

static void ColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, COLUMN_MAX);
}

static void JoyInfoEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, 4);
}

static void JoyInfoDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, 4);
}

static void ColumnDecodeWidths(const char* str, void* data)
{
	ColumnDecodeString(str, data);
}

static void SplitterEncodeString(void* data, char* str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < GetSplitterCount(); i++)
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
	
	for (i = 0; p && i < GetSplitterCount(); i++)
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

	*value = VIEW_GROUPED;

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

static void D3DEffectEncodeString(void *data,char *str)
{
	int d3d_effect = *(int *)data;

	strcpy(str,GetD3DEffectShortName(d3d_effect));
}

static void D3DEffectDecodeString(const char *str,void *data)
{
	int i;

	*(int *)data = D3D_EFFECT_NONE;

	for (i=0;i<MAX_D3D_EFFECTS;i++)
	{
		if (_stricmp(GetD3DEffectShortName(i),str) == 0)
		{
			*(int *)data = i;
			return;
		}
	}
	dprintf("invalid d3d effect string %s",str);
}

static void D3DPrescaleEncodeString(void *data,char *str)
{
	int d3d_prescale = *(int *)data;

	strcpy(str,GetD3DPrescaleShortName(d3d_prescale));
}

static void D3DPrescaleDecodeString(const char *str,void *data)
{
	int i;

	*(int *)data = D3D_PRESCALE_NONE;

	for (i=0;i<MAX_D3D_PRESCALE;i++)
	{
		if (_stricmp(GetD3DPrescaleShortName(i),str) == 0)
		{
			*(int *)data = i;
			return;
		}
	}
	dprintf("invalid d3d prescale string %s",str);
}

static void CleanStretchEncodeString(void *data,char *str)
{
	int clean_stretch = *(int *)data;

	strcpy(str,GetCleanStretchShortName(clean_stretch));
}

static void CleanStretchDecodeString(const char *str,void *data)
{
	int i;

	*(int *)data = CLEAN_STRETCH_NONE;

	for (i=0;i<MAX_CLEAN_STRETCH;i++)
	{
		if (_stricmp(GetCleanStretchShortName(i),str) == 0)
		{
			*(int *)data = i;
			return;
		}
	}
	dprintf("invalid clean stretch string %s",str);
}

static void CurrentTabEncodeString(void *data,char *str)
{
	int tab_index = *(int *)data;

	strcpy(str,GetImageTabShortName(tab_index));
}

static void CurrentTabDecodeString(const char *str,void *data)
{
	int i;

	*(int *)data = TAB_SCREENSHOT;

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if (_stricmp(GetImageTabShortName(i),str) == 0)
		{
			*(int *)data = i;
			return;
		}
	}
	dprintf("invalid tab index string %s",str);
}

static void FolderFlagsEncodeString(void *data,char *str)
{
	int i;
	int num_saved = 0;
	extern FOLDERDATA g_folderData[];

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (TestBit(*(LPBITS *)data,i) == FALSE)
		{
			int j;

			if (num_saved != 0)
				strcat(str,", ");

			for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
			{
				if (g_folderData[j].m_nFolderId == i)
				{
					strcat(str,g_folderData[j].short_name);
					num_saved++;
					break;
				}
			}
		}
	}
}

static void FolderFlagsDecodeString(const char *str,void *data)
{
	char s[2000];
	extern FOLDERDATA g_folderData[];
	char *token;

	_snprintf(s,sizeof(s),"%s",str);

	SetAllBits(*(LPBITS *)data,TRUE);

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;g_folderData[j].m_lpTitle != NULL;j++)
		{
			if (strcmp(g_folderData[j].short_name,token) == 0)
			{
				//dprintf("found folder to hide %i",g_folderData[j].m_nFolderId);
				ClearBit(*(LPBITS *)data,g_folderData[j].m_nFolderId);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}
}

static void TabFlagsEncodeString(void *data,char *str)
{
	int i;
	int num_saved = 0;

	strcpy(str,"");

	// we save the ones that are NOT displayed, so we can add new ones
	// and upgraders will see them
	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		if ((*(int *)data & (1 << i)) == 0)
		{
			if (num_saved != 0)
				strcat(str,", ");

			strcat(str,GetImageTabShortName(i));
			num_saved++;
		}
	}
}

static void TabFlagsDecodeString(const char *str,void *data)
{
	char s[2000];
	char *token;

	_snprintf(s,sizeof(s),"%s",str);

	// simple way to set all tab bits "on"
	*(int *)data = (1 << MAX_TAB_TYPES) - 1;

	token = strtok(s,", \t");
	while (token != NULL)
	{
		int j;

		for (j=0;j<MAX_TAB_TYPES;j++)
		{
			if (strcmp(GetImageTabShortName(j),token) == 0)
			{
				// turn off this bit
				*(int *)data &= ~(1 << j);
				break;
			}
		}
		token = strtok(NULL,", \t");
	}

	if (*(int *)data == 0)
	{
		// not allowed to hide all tabs, because then why even show the area?
		*(int *)data = (1 << TAB_SCREENSHOT);
	}
}

static REG_OPTION * GetOption(REG_OPTION *option_array,int num_options,const char *key)
{
	int i;

	for (i=0;i<num_options;i++)
	{
		if (option_array[i].ini_name[0] != '\0')
		{
			if (strcmp(option_array[i].ini_name,key) == 0)
				return &option_array[i];
		}
	}
	return NULL;
}

static void LoadOption(REG_OPTION *option,const char *value_str)
{
	//dprintf("trying to load %s type %i [%s]",option->ini_name,option->m_iType,value_str);

	switch (option->m_iType)
	{
	case RO_DOUBLE :
		*((double *)option->m_vpData) = atof(value_str);
		break;

	case RO_COLOR :
	{
		unsigned int r,g,b;
		if (sscanf(value_str,"%u,%u,%u",&r,&g,&b) == 3)
			*((COLORREF *)option->m_vpData) = RGB(r,g,b);
		break;
	}

	case RO_STRING:
		if (*(char**)option->m_vpData != NULL)
			free(*(char**)option->m_vpData);
		*(char **)option->m_vpData = _strdup(value_str);
		break;

	case RO_BOOL:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)option->m_vpData) = (value_int != 0);
		break;
	}

	case RO_INT:
	{
		int value_int;
		if (sscanf(value_str,"%i",&value_int) == 1)
			*((int *)option->m_vpData) = value_int;
		break;
	}

	case RO_ENCODE:
		option->decode(value_str,option->m_vpData);
		break;

	default:
		break;
	}
	
}

static BOOL LoadGameVariableOrFolderFilter(char *key,const char *value)
{
	REG_OPTION fake_option;
	int i;
	int driver_index;
	const char *suffix;

	for (i = 0; i < sizeof(gamevariable_options) / sizeof(gamevariable_options[0]); i++)
	{
		_snprintf(fake_option.ini_name, sizeof(fake_option.ini_name), "drivername_%s", gamevariable_options[i].name);
		suffix = strchr(fake_option.ini_name, '_');

		if (StringIsSuffixedBy(key, suffix))
		{
			key[strlen(key) - strlen(suffix)] = '\0';
			driver_index = GetGameNameIndex(key);
			if (driver_index < 0)
			{
				dprintf("error loading game variable for invalid game %s",key);
				return TRUE;
			}

			fake_option.m_iType = gamevariable_options[i].m_iType;
			fake_option.m_vpData = (void *) (((UINT8 *) &game_variables[driver_index]) + gamevariable_options[i].m_iOffset);
			LoadOption(&fake_option,value);
			return TRUE;
		}
	}

	suffix = "_filters";
	if (StringIsSuffixedBy(key, suffix))
	{
		int folder_index;
		int filters;

		key[strlen(key) - strlen(suffix)] = '\0';
		if (sscanf(key,"%i",&folder_index) != 1)
		{
			dprintf("error loading game variable for invalid game %s",key);
			return TRUE;
		}
		if (folder_index < 0)
			return TRUE;

		if (sscanf(value,"%i",&filters) != 1)
			return TRUE;

		LoadFolderFilter(folder_index,filters);
		return TRUE;
	}

	return FALSE;
}

// out of a string, parse two substrings (non-blanks, or quoted).
// we modify buffer, and return key and value to point to the substrings, or NULL
static void ParseKeyValueStrings(char *buffer,char **key,char **value)
{
	char *ptr;
	BOOL quoted;

	*key = NULL;
	*value = NULL;

	ptr = buffer;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	*key = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of key
			if (*ptr == 0)
				return;

			*ptr = '\0';
			ptr++;
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	*value = ptr;
	quoted = FALSE;
	while (1)
	{
		if (*ptr == 0 || (!quoted && (*ptr == ' ' || *ptr == '\t')) || (quoted && *ptr == '\"'))
		{
			// found end of value;
			*ptr = '\0';
			break;
		}

		if (*ptr == '\"')
			quoted = TRUE;
		ptr++;
	}

	if (**key == '\"')
		(*key)++;

	if (**value == '\"')
		(*value)++;

	if (**key == '\0')
	{
		*key = NULL;
		*value = NULL;
	}
}

/* Register access functions below */
static void LoadOptionsAndSettings(void)
{
	char buffer[512];
	FILE *fptr;

	fptr = fopen(UI_INI_FILENAME,"rt");
	if (fptr != NULL)
	{
		while (fgets(buffer,sizeof(buffer),fptr) != NULL)
		{
			char *key,*value_str;
			REG_OPTION *option;

			if (buffer[0] == '\0' || buffer[0] == '#')
				continue;

			// we're guaranteed that strlen(buffer) >= 1 now
			buffer[strlen(buffer)-1] = '\0';

			ParseKeyValueStrings(buffer,&key,&value_str);
			if (key == NULL || value_str == NULL)
			{
				//dprintf("invalid line [%s]",buffer);
				continue;
			}

			option = GetOption(regSettings,NUM_SETTINGS,key);
			if (option == NULL)
			{
				// search for game_have_rom/have_sample/play_count/play_time thing
				if (LoadGameVariableOrFolderFilter(key,value_str) == FALSE)
				{
					dprintf("found unknown option %s",key);
				}
			}
			else
			{
				LoadOption(option,value_str);
			}

		}

		fclose(fptr);
	}

	_snprintf(buffer,sizeof(buffer),"%s\\%s",GetIniDir(),DEFAULT_OPTIONS_INI_FILENAME);
	gOpts = global;
	if (LoadOptions(buffer,&global,TRUE))
	{
		global = gOpts;
		ZeroMemory(&gOpts,sizeof(gOpts));
	}

}

void LoadGameOptions(int driver_index)
{
	char buffer[512];

	_snprintf(buffer,sizeof(buffer),"%s\\%s.ini",GetIniDir(),drivers[driver_index]->name);
	
	CopyGameOptions(&global,&gOpts);
	if (LoadOptions(buffer,&game_options[driver_index],FALSE))
	{
		// successfully loaded
		game_options[driver_index] = gOpts;
		game_variables[driver_index].use_default = FALSE;
	}
}

static BOOL LoadOptions(const char *filename,options_type *o,BOOL load_global_game_options)
{
	FILE *fptr;
	char buffer[512];

	fptr = fopen(filename,"rt");
	if (fptr == NULL)
		return FALSE;

	while (fgets(buffer,sizeof(buffer),fptr) != NULL)
	{
		char *key,*value_str;
		REG_OPTION *option;

		if (buffer[0] == '\0')
			continue;

		// we're guaranteed that strlen(buffer) >= 1 now
		buffer[strlen(buffer)-1] = '\0';
		
		// # starts a comment, but #* is a special MAME32 code
		// saying it's an option for us, but NOT for the main
		// MAME
		if (buffer[0] == '#')
		{
			if (buffer[1] != '*')
				continue;
		}

		ParseKeyValueStrings(buffer,&key,&value_str);
		if (key == NULL || value_str == NULL)
		{
			dprintf("invalid line [%s]",buffer);
			continue;
		}
		option = GetOption(regGameOpts,NUM_GAME_OPTIONS,key);
		if (option == NULL)
		{
			if (load_global_game_options)
				option = GetOption(global_game_options,NUM_GLOBAL_GAME_OPTIONS,key);
			
			if (option == NULL)
			{
				dprintf("load game options found unknown option %s",key);
				continue;
			}
		}

		//dprintf("loading option <%s> <%s>",option,value_str);
		LoadOption(option,value_str);
	}

	fclose(fptr);
	return TRUE;
}

DWORD GetFolderFlags(int folder_index)
{
	int i;

	for (i=0;i<num_folder_filters;i++)
		if (folder_filters[i].folder_index == folder_index)
		{
			//dprintf("found folder filter %i %i",folder_index,folder_filters[i].filters);
			return folder_filters[i].filters;
		}
	return 0;
}

void SaveOptions(void)
{
	int i;
	FILE *fptr;

	fptr = fopen(UI_INI_FILENAME,"wt");
	if (fptr != NULL)
	{
		fprintf(fptr,"### " UI_INI_FILENAME " ###\n\n");
		fprintf(fptr,"### interface ###\n\n");
		
		if (save_gui_settings)
		{
			for (i=0;i<NUM_SETTINGS;i++)
			{
				if ((regSettings[i].ini_name[0] != '\0') && !regSettings[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr,&regSettings[i]);
			}
		}
		
		fprintf(fptr,"\n");
		fprintf(fptr,"### folder filters ###\n\n");
		
		for (i=0;i<GetNumFolders();i++)
		{
			LPTREEFOLDER lpFolder = GetFolder(i);

			if ((lpFolder->m_dwFlags & F_MASK) != 0)
			{
				fprintf(fptr,"# %s\n",lpFolder->m_lpTitle);
				fprintf(fptr,"%i_filters %i\n",i,(int)(lpFolder->m_dwFlags & F_MASK));
			}
		}

		fprintf(fptr,"\n");
		fprintf(fptr,"### game variables ###\n\n");
		for (i=0;i<num_games;i++)
		{
			int j;
			int nValue;
			const char *pValue;
			void *pv;
			int driver_index = GetIndexFromSortedIndex(i); 

			// need to improve this to not save too many
			for (j = 0; j < sizeof(gamevariable_options) / sizeof(gamevariable_options[0]); j++)
			{
				if (!gamevariable_options[j].m_pfnQualifier || gamevariable_options[j].m_pfnQualifier(driver_index))
				{
					pv = ((UINT8 *) &game_variables[driver_index]) + gamevariable_options[j].m_iOffset;

					switch(gamevariable_options[j].m_iType) {
					case RO_INT:
					case RO_BOOL:
						nValue = *((int *) pv);
						if (nValue != (int) gamevariable_options[j].m_vpDefault)
						{
							fprintf(fptr, "%s_%s %i\n",
								drivers[driver_index]->name,
								gamevariable_options[j].name,
								nValue);
						}
						break;

					case RO_STRING:
						pValue = *((const char **) pv);
						if (!pValue)
							pValue = "";

						if (strcmp(pValue, (const char *) gamevariable_options[j].m_vpDefault))
						{
							fprintf(fptr, "%s_%s \"%s\"\n",
								drivers[driver_index]->name,
								gamevariable_options[j].name,
								pValue);
						}
						break;

					default:
						assert(0);
						break;
					}
				}
			}
		}
		fclose(fptr);
	}
}


void SaveGameOptions(int driver_index)
{
	int i;
	FILE *fptr;
	char buffer[512];
	BOOL options_different = FALSE;

	if (game_variables[driver_index].use_default == FALSE)
	{
		for (i=0;i<NUM_GAME_OPTIONS;i++)
		{
			if (IsOptionEqual(i,&game_options[driver_index],&global) == FALSE)
			{
				options_different = TRUE;
			}

		}
	}

	_snprintf(buffer,sizeof(buffer),"%s\\%s.ini",GetIniDir(),drivers[driver_index]->name);
	if (options_different)
	{
		fptr = fopen(buffer,"wt");
		if (fptr != NULL)
		{
			fprintf(fptr,"### ");
			fprintf(fptr,"%s",drivers[driver_index]->name);
			fprintf(fptr," ###\n\n");

			for (i=0;i<NUM_GAME_OPTIONS;i++)
			{
				if (IsOptionEqual(i,&game_options[driver_index],&global) == FALSE)
				{
					gOpts = game_options[driver_index];
					WriteOptionToFile(fptr,&regGameOpts[i]);
				}
			}

			fclose(fptr);
		}
	}
	else
	{
		if (DeleteFile(buffer) == 0)
		{
			dprintf("error deleting %s; error %d\n",buffer, GetLastError());
		}
	}
}

void SaveDefaultOptions(void)
{
	int i;
	FILE *fptr;
	char buffer[512];

	_snprintf(buffer,sizeof(buffer),"%s\\%s",GetIniDir(),DEFAULT_OPTIONS_INI_FILENAME);

	fptr = fopen(buffer,"wt");
	if (fptr != NULL)
	{
		fprintf(fptr,"### " DEFAULT_OPTIONS_INI_FILENAME " ###\n\n");
		
		if (save_gui_settings)
		{
			fprintf(fptr,"### global-only options ###\n\n");
		
			for (i=0;i<NUM_GLOBAL_GAME_OPTIONS;i++)
			{
				if (!global_game_options[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr,&global_game_options[i]);
			}
		}

		if (save_default_options)
		{
			fprintf(fptr,"\n### default game options ###\n\n");
			gOpts = global;
			for (i=0;i<NUM_GAME_OPTIONS;i++)
			{
				if (!regGameOpts[i].m_bOnlyOnGame)
					WriteOptionToFile(fptr,&regGameOpts[i]);
			}
		}

		fclose(fptr);
	}
}

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value)
{
	if (value[0] && !strchr(value,' '))
		fprintf(fptr,"%s %s\n",key,value);
	else
		fprintf(fptr,"%s \"%s\"\n",key,value);
}

static void WriteIntOptionToFile(FILE *fptr,const char *key,int value)
{
	fprintf(fptr,"%s %i\n",key,value);
}

static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value)
{
	fprintf(fptr,"%s %i\n",key,value ? 1 : 0);
}

static void WriteColorOptionToFile(FILE *fptr,const char *key,COLORREF value)
{
	fprintf(fptr,"%s %i,%i,%i\n",key,(int)(value & 0xff),(int)((value >> 8) & 0xff),
			(int)((value >> 16) & 0xff));
}

static BOOL IsOptionEqual(int option_index,options_type *o1,options_type *o2)
{
	switch (regGameOpts[option_index].m_iType)
	{
	case RO_DOUBLE:
	{
		double a,b;
		gOpts = *o1;
		a = *(double *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(double *)regGameOpts[option_index].m_vpData;
		return fabs(a-b) < 0.000001;
	}
	case RO_COLOR :
	{
		COLORREF a,b;
		gOpts = *o1;
		a = *(COLORREF *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(COLORREF *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_STRING:
	{
		char *a,*b;
		gOpts = *o1;
		a = *(char **)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(char **)regGameOpts[option_index].m_vpData;
		return strcmp(a,b) == 0;
	}
	case RO_BOOL:
	{
		BOOL a,b;
		gOpts = *o1;
		a = *(BOOL *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(BOOL *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_INT:
	{
		int a,b;
		gOpts = *o1;
		a = *(int *)regGameOpts[option_index].m_vpData;
		gOpts = *o2;
		b = *(int *)regGameOpts[option_index].m_vpData;
		return a == b;
	}
	case RO_ENCODE:
	{
		char a[1000],b[1000];
		gOpts = *o1;
		regGameOpts[option_index].encode(regGameOpts[option_index].m_vpData,a);
		gOpts = *o2;
		regGameOpts[option_index].encode(regGameOpts[option_index].m_vpData,b);
		return strcmp(a,b) == 0;
	}

	default:
		break;
	}

	return TRUE;
}


static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	const char *key = regOpt->ini_name;
	char	cTemp[1000];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpt->m_vpData;
        _gcvt( *pDouble, 10, cTemp );
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	case RO_COLOR :
	{
		COLORREF color = *(COLORREF *)regOpt->m_vpData;
		if (color != (COLORREF)-1)
			WriteColorOptionToFile(fptr,key,color);
		break;
	}

	case RO_STRING:
		pString = *((char **)regOpt->m_vpData);
		if (pString)
		    WriteStringOptionToFile(fptr, key, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*)regOpt->m_vpData;
		WriteBoolOptionToFile(fptr, key, *pBool);
		break;

	case RO_INT:
		pInt = (int*)regOpt->m_vpData;
		WriteIntOptionToFile(fptr, key, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(regOpt->m_vpData, cTemp);
		WriteStringOptionToFile(fptr, key, cTemp);
		break;

	default:
		break;
	}

}

char * GetVersionString(void)
{
	return build_version;
}

/* End of options.c */
