/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum
{
	COLUMN_GAMES = 0,
	COLUMN_ROMS,
	COLUMN_SAMPLES,
	COLUMN_DIRECTORY,
	COLUMN_TYPE,
	COLUMN_TRACKBALL,
	COLUMN_PLAYED,
	COLUMN_MANUFACTURER,
	COLUMN_YEAR,
	COLUMN_CLONE,
	COLUMN_SRCDRIVERS,
	COLUMN_MAX
};

enum
{
	VIEW_LARGE_ICONS = 0,
	VIEW_SMALL_ICONS,
	VIEW_INLIST,
	VIEW_REPORT,
	VIEW_GROUPED,
	VIEW_MAX
};

/* per-game data we calculate */
enum
{
	UNKNOWN = 2
};

// config helpers types
enum
{
	RO_BOOL = 0, // BOOL value
	RO_INT,      // int value
	RO_DOUBLE,   // double value
	RO_COLOR,    // COLORREF value
	RO_STRING,   // pointer to string    - m_vpData is an allocated buffer
	RO_ENCODE    // encode/decode string - calls decode/encode functions
};

/* List of artwork types to display in the screen shot area */
enum
{
	PICT_SCREENSHOT = 0,
	PICT_FLYER,
	PICT_CABINET,
	PICT_MARQUEE,
	PICT_TITLES,
	MAX_PICT_TYPES
};

/* Default input */
enum
{
	INPUT_LAYOUT_STD,
	INPUT_LAYOUT_HR,
	INPUT_LAYOUT_HRSE
};

// d3d effect types
enum
{
	// these must match array of strings d3d_effects_long_name in options.c
	D3D_EFFECT_NONE = 0,
	D3D_EFFECT_AUTO = 1,

	MAX_D3D_EFFECTS = 17,
};

// d3d filter types
enum
{
	MAX_D3D_FILTERS = 5,
};

// used to be "registry option", now is just for a game/global option
typedef struct
{
	char ini_name[40]; // ini name
	int  m_iType;                                 // key type
	void *m_vpData;                               // key data
	void (*encode)(void *data, char *str);        // encode function
	void (*decode)(const char *str, void *data);  // decode function
} REG_OPTION;

typedef struct
{
	int x, y, width, height;
} AREA;

typedef struct
{
	/* video */
	BOOL   autoframeskip;
	int    frameskip;
	BOOL   wait_vsync;
	BOOL   use_triplebuf;
	BOOL   window_mode;
	BOOL   use_ddraw;
	BOOL   ddraw_stretch;
	char *resolution;
	int    gfx_refresh;
	BOOL   scanlines;
	BOOL   switchres;
	BOOL   switchbpp;
	BOOL   maximize;
	BOOL   keepaspect;
	BOOL   matchrefresh;
	BOOL   syncrefresh;
	BOOL   throttle;
	double gfx_brightness;
	int    frames_to_display;
	char   *effect;
	char   *aspect;
	BOOL clean_stretch;
	int zoom;

	// d3d
	BOOL use_d3d;
	int d3d_filter;
	BOOL d3d_texture_management;
	int d3d_effect;
	BOOL d3d_prescale;
	BOOL d3d_rotate_effects;
	BOOL d3d_scanlines_enable;
	int d3d_scanlines;
	BOOL d3d_feedback_enable;
	int d3d_feedback;
	BOOL d3d_saturation_enable;
	int d3d_saturation;

	/* sound */

	/* input */
	BOOL   use_mouse;
	BOOL   use_joystick;
	double f_a2d;
	BOOL   steadykey;
	BOOL   lightgun;
	char *ctrlr;

	/* Core video */
	double f_bright_correct; /* "1.0", 0.5, 2.0 */
	double f_pause_bright; /* "0.65", 0.5, 2.0 */
	BOOL   norotate;
	BOOL   ror;
	BOOL   rol;
	BOOL   auto_ror;
	BOOL   auto_rol;
	BOOL   flipx;
	BOOL   flipy;
	char *debugres;
	double f_gamma_correct;

	/* Core vector */
	BOOL   antialias;
	BOOL   translucency;
	double f_beam;
	double f_flicker;
	double f_intensity;

	/* Sound */
	int    samplerate;
	BOOL   use_samples;
	BOOL   use_filter;
	BOOL   enable_sound;
	int    attenuation;
	int audio_latency;

	/* Misc artwork options */
	BOOL   use_artwork;
	BOOL   backdrops;
	BOOL   overlays;
	BOOL   bezels;
	BOOL   artwork_crop;
	int    artres;

	/* misc */
	BOOL   cheat;
	BOOL   mame_debug;
	BOOL   errorlog;
	BOOL   sleep;
	BOOL   old_timing;
	BOOL   leds;
	int bios;
#ifdef PINMAME
        int dmd_red,    dmd_green,   dmd_blue;
        int dmd_perc66, dmd_perc33,  dmd_perc0;
        int dmd_only,   dmd_compact, dmd_antialias;
#endif /* PINMAME */

} options_type;

// per-game data we store, not to pass to mame, but for our own use.
typedef struct
{
    int play_count;
    int has_roms;
    int has_samples;

	BOOL options_loaded; // whether or not we've loaded the game options yet
	BOOL use_default; // whether or not we should just use default options

} game_variables_type;

// show_tab_flags
// these must match up with the tab_texts strings in options.c
enum
{
	SHOW_TAB_SNAPSHOT = 0x01,
	SHOW_TAB_FLYER = 0x02,
	SHOW_TAB_CABINET = 0x04,
	SHOW_TAB_MARQUEE = 0x08,
	SHOW_TAB_TITLE = 0x10,
	NUM_SHOW_TABS = 5
};

typedef struct
{
    INT      folder_id;
    BOOL     view;
    BOOL     show_folderlist;
    BOOL     show_toolbar;
    BOOL     show_statusbar;
    BOOL     show_screenshot;
    BOOL     show_tabctrl;
	int show_tab_flags;
    int      show_pict_type;
    BOOL     game_check;        /* Startup GameCheck */
    BOOL     version_check;     /* Version mismatch warings */
    BOOL     use_joygui;
    BOOL     broadcast;
    BOOL     random_bg;
    char     *default_game;
    int      column_width[COLUMN_MAX];
    int      column_order[COLUMN_MAX];
    int      column_shown[COLUMN_MAX];
    int      sort_column;
    BOOL     sort_reverse;
    AREA     area;
    UINT     windowstate;
    int      splitter[4];		/* NPW 5-Feb-2003 - I don't like hard coding this, but I don't have a choice */
    LOGFONT  list_font;
    COLORREF list_font_color;
    COLORREF list_clone_color;
    BOOL skip_disclaimer;
    BOOL skip_gameinfo;
    BOOL high_priority;

    char*    language;
    char*    flyerdir;
    char*    cabinetdir;
    char*    marqueedir;
    char*    titlesdir;

    char*    romdirs;
    char*    sampledirs;
    char*    inidir;
    char*    cfgdir;
    char*    nvramdir;
    char*    memcarddir;
    char*    inpdir;
    char*    hidir;
    char*    statedir;
    char*    artdir;
    char*    imgdir;
    char*    diffdir;
    char*	 iconsdir;
    char*    bgdir;
#ifdef PINMAME
        char*    wavedir;
#endif /* PINMAME */
    char*    cheat_filename;
    char*    history_filename;
    char*    mameinfo_filename;
    char*    ctrlrdir;
    char*    folderdir;

} settings_type; /* global settings for the UI only */

BOOL OptionsInit(void);
void OptionsExit(void);

void FreeGameOptions(options_type *o);
void CopyGameOptions(options_type *source,options_type *dest);
options_type * GetDefaultOptions(void);
options_type * GetGameOptions(int driver_index);
BOOL GetGameUsesDefaults(int driver_index);
void SetGameUsesDefaults(int driver_index,BOOL use_defaults);
void LoadGameOptions(int driver_index);

void SaveOptions(void);

void ResetGUI(void);
void ResetGameDefaults(void);
void ResetAllGameOptions(void);

const char * GetTabName(int tab_index);

const char * GetD3DEffectLongName(int d3d_effect);
const char * GetD3DEffectShortName(int d3d_effect);

const char * GetD3DFilterLongName(int d3d_filter);

void SetViewMode(int val);
int  GetViewMode(void);

void SetGameCheck(BOOL game_check);
BOOL GetGameCheck(void);

void SetVersionCheck(BOOL version_check);
BOOL GetVersionCheck(void);

void SetJoyGUI(BOOL use_joygui);
BOOL GetJoyGUI(void);

void SetBroadcast(BOOL broadcast);
BOOL GetBroadcast(void);

void SetSkipDisclaimer(BOOL skip_disclaimer);
BOOL GetSkipDisclaimer(void);

void SetSkipGameInfo(BOOL show_gameinfo);
BOOL GetSkipGameInfo(void);

void SetHighPriority(BOOL high_priority);
BOOL GetHighPriority(void);

void SetRandomBackground(BOOL random_bg);
BOOL GetRandomBackground(void);

void SetSavedFolderID(UINT val);
UINT GetSavedFolderID(void);

void SetShowScreenShot(BOOL val);
BOOL GetShowScreenShot(void);

void SetShowFolderList(BOOL val);
BOOL GetShowFolderList(void);

void SetShowStatusBar(BOOL val);
BOOL GetShowStatusBar(void);

void SetShowToolBar(BOOL val);
BOOL GetShowToolBar(void);

void SetShowTabCtrl(BOOL val);
BOOL GetShowTabCtrl(void);

void SetShowPictType(int val);
int  GetShowPictType(void);

void SetDefaultGame(const char *name);
const char *GetDefaultGame(void);

void SetWindowArea(AREA *area);
void GetWindowArea(AREA *area);

void SetWindowState(UINT state);
UINT GetWindowState(void);

void SetColumnWidths(int widths[]);
void GetColumnWidths(int widths[]);

void SetColumnOrder(int order[]);
void GetColumnOrder(int order[]);

void SetColumnShown(int shown[]);
void GetColumnShown(int shown[]);

void SetSplitterPos(int splitterId, int pos);
int  GetSplitterPos(int splitterId);

void SetListFont(LOGFONT *font);
void GetListFont(LOGFONT *font);

DWORD GetFolderFlags(int folder_index);

void SetListFontColor(COLORREF uColor);
COLORREF GetListFontColor(void);

void SetListCloneColor(COLORREF uColor);
COLORREF GetListCloneColor(void);

int GetShowTabFlags(void);
void SetShowTabFlags(int new_flags);

void SetSortColumn(int column);
int  GetSortColumn(void);

void SetSortReverse(BOOL reverse);
BOOL GetSortReverse(void);

const char* GetLanguage(void);
void SetLanguage(const char* lang);

const char* GetRomDirs(void);
void SetRomDirs(const char* paths);

const char* GetSampleDirs(void);
void  SetSampleDirs(const char* paths);

const char * GetIniDir(void);
void SetIniDir(const char *path);

const char* GetCfgDir(void);
void SetCfgDir(const char* path);

const char* GetHiDir(void);
void SetHiDir(const char* path);

const char* GetNvramDir(void);
void SetNvramDir(const char* path);

const char* GetInpDir(void);
void SetInpDir(const char* path);

const char* GetImgDir(void);
void SetImgDir(const char* path);

const char* GetStateDir(void);
void SetStateDir(const char* path);

const char* GetArtDir(void);
void SetArtDir(const char* path);

const char* GetMemcardDir(void);
void SetMemcardDir(const char* path);

const char* GetFlyerDir(void);
void SetFlyerDir(const char* path);

const char* GetCabinetDir(void);
void SetCabinetDir(const char* path);

const char* GetMarqueeDir(void);
void SetMarqueeDir(const char* path);

const char* GetTitlesDir(void);
void SetTitlesDir(const char* path);

const char* GetDiffDir(void);
void SetDiffDir(const char* path);

const char* GetIconsDir(void);
void SetIconsDir(const char* path);

const char *GetBgDir(void);
void SetBgDir(const char *path);

#ifdef PINMAME
const char* GetWaveDir(void);
void SetWaveDir(const char* path);
#endif /* PINMAME */

const char* GetCtrlrDir(void);
void SetCtrlrDir(const char* path);

const char* GetFolderDir(void);
void SetFolderDir(const char* path);

const char* GetCheatFileName(void);
void SetCheatFileName(const char* path);

const char* GetHistoryFileName(void);
void SetHistoryFileName(const char* path);

const char* GetMAMEInfoFileName(void);
void SetMAMEInfoFileName(const char* path);

void ResetGameOptions(int driver_index);

int GetHasRoms(int driver_index);
void SetHasRoms(int driver_index, int has_roms);

int GetHasSamples(int driver_index);
void SetHasSamples(int driver_index, int has_samples);

void IncrementPlayCount(int driver_index);
int GetPlayCount(int driver_index);

char * GetVersionString(void);

void SaveGameOptions(int driver_index);
void SaveDefaultOptions(void);

#endif
