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
	COLUMN_PLAYTIME,
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

// can't be the same as the VerifyRomSet() results, listed in audit.h
enum
{
	UNKNOWN	= -1
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

/* Default input */
enum
{
	INPUT_LAYOUT_STD,
	INPUT_LAYOUT_HR,
	INPUT_LAYOUT_HRSE
};

// clean stretch types
enum
{
	// these must match array of strings clean_stretch_long_name in options.c
	CLEAN_STRETCH_NONE = 0,
	CLEAN_STRETCH_AUTO = 1,

	MAX_CLEAN_STRETCH = 5,
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

// d3d prescale types
enum
{
	D3D_PRESCALE_NONE = 0,
	D3D_PRESCALE_AUTO = 1,
	MAX_D3D_PRESCALE = 10,
};

// used to be "registry option", now is just for a game/global option
typedef struct
{
	char ini_name[40]; // ini name
	int  m_iType;                                 // key type
	void *m_vpData;                               // key data
	void (*encode)(void *data, char *str);        // encode function
	void (*decode)(const char *str, void *data);  // decode function
	BOOL m_bOnlyOnGame;                           // use this option only on games
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
	int clean_stretch;
	int zoom;

	// d3d
	BOOL use_d3d;
	int d3d_filter;
	BOOL d3d_texture_management;
	int d3d_effect;
	int d3d_prescale;
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
	int play_time;
    int rom_audit_results;
    int samples_audit_results;

	BOOL options_loaded; // whether or not we've loaded the game options yet
	BOOL use_default; // whether or not we should just use default options

} game_variables_type;

// List of artwork types to display in the screen shot area
enum
{
	// these must match array of strings image_tabs_long_name in options.c
	TAB_SCREENSHOT = 0,
	TAB_FLYER,
	TAB_CABINET,
	TAB_MARQUEE,
	TAB_TITLE,
	TAB_CONTROL_PANEL,
	TAB_HISTORY,

	MAX_TAB_TYPES
};

typedef struct
{
    INT      folder_id;
    BOOL     view;
    BOOL     show_folderlist;
	LPBITS show_folder_flags;
    BOOL     show_toolbar;
    BOOL     show_statusbar;
    BOOL     show_screenshot;
    BOOL     show_tabctrl;
	int show_tab_flags;
    int current_tab;
    BOOL     game_check;        /* Startup GameCheck */
    BOOL     use_joygui;
    BOOL     broadcast;
    BOOL     random_bg;
    int      cycle_screenshot;
	BOOL stretch_screenshot_larger;

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

    // Joystick control of ui
	// array of 4 is joystick index, stick or button, etc.
    int      ui_joy_up[4];
    int      ui_joy_down[4];
    int      ui_joy_left[4];
    int      ui_joy_right[4];
    int      ui_joy_start[4];
    int      ui_joy_pgup[4];
    int      ui_joy_pgdwn[4];
    int      ui_joy_home[4];
    int      ui_joy_end[4];
    int      ui_joy_ss_change[4];
    int      ui_joy_history_up[4];
    int      ui_joy_history_down[4];
    int      ui_joy_exec[4];

    char*    exec_command;  // Command line to execute on ui_joy_exec   
    int      exec_wait;     // How long to wait before executing
    BOOL     hide_mouse;    // Should mouse cursor be hidden on startup?
    BOOL     full_screen;   // Should we fake fullscreen?

    char*    language;
    char*    flyerdir;
    char*    cabinetdir;
    char*    marqueedir;
    char*    titlesdir;
    char *cpaneldir;

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

const char * GetImageTabLongName(int tab_index);
const char * GetImageTabShortName(int tab_index);

const char * GetD3DEffectLongName(int d3d_effect);
const char * GetD3DEffectShortName(int d3d_effect);

const char * GetD3DPrescaleLongName(int d3d_prescale);
const char * GetD3DPrescaleShortName(int d3d_prescale);

const char * GetD3DFilterLongName(int d3d_filter);

const char * GetCleanStretchLongName(int clean_stretch);
const char * GetCleanStretchShortName(int clean_stretch);

void SetViewMode(int val);
int  GetViewMode(void);

void SetGameCheck(BOOL game_check);
BOOL GetGameCheck(void);

void SetVersionCheck(BOOL version_check);
BOOL GetVersionCheck(void);

void SetJoyGUI(BOOL use_joygui);
BOOL GetJoyGUI(void);

void SetCycleScreenshot(int cycle_screenshot);
int GetCycleScreenshot(void);

void SetStretchScreenShotLarger(BOOL stretch);
BOOL GetStretchScreenShotLarger(void);

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

BOOL GetShowFolder(int folder);
void SetShowFolder(int folder,BOOL show);


void SetShowStatusBar(BOOL val);
BOOL GetShowStatusBar(void);

void SetShowToolBar(BOOL val);
BOOL GetShowToolBar(void);

void SetShowTabCtrl(BOOL val);
BOOL GetShowTabCtrl(void);

void SetCurrentTab(int val);
int  GetCurrentTab(void);

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

int GetShowTab(int tab);
void SetShowTab(int tab,BOOL show);
BOOL AllowedToSetShowTab(int tab,BOOL show);

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

const char * GetControlPanelDir(void);
void SetControlPanelDir(const char *path);

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

int GetRomAuditResults(int driver_index);
void SetRomAuditResults(int driver_index, int audit_results);

int GetSampleAuditResults(int driver_index);
void SetSampleAuditResults(int driver_index, int audit_results);

void IncrementPlayCount(int driver_index);
int GetPlayCount(int driver_index);

void IncrementPlayTime(int driver_index,int playtime);
int GetPlayTime(int driver_index);
void GetTextPlayTime(int driver_index,char *buf);

char * GetVersionString(void);

void SaveGameOptions(int driver_index);
void SaveDefaultOptions(void);
int GetUIJoyUp(int joycodeIndex);
void SetUIJoyUp(int joycodeIndex, int val);

int GetUIJoyDown(int joycodeIndex);
void SetUIJoyDown(int joycodeIndex, int val);

int GetUIJoyLeft(int joycodeIndex);
void SetUIJoyLeft(int joycodeIndex, int val);

int GetUIJoyRight(int joycodeIndex);
void SetUIJoyRight(int joycodeIndex, int val);

int GetUIJoyStart(int joycodeIndex);
void SetUIJoyStart(int joycodeIndex, int val);

int GetUIJoyPageUp(int joycodeIndex);
void SetUIJoyPageUp(int joycodeIndex, int val);

int GetUIJoyPageDown(int joycodeIndex);
void SetUIJoyPageDown(int joycodeIndex, int val);

int GetUIJoyHome(int joycodeIndex);
void SetUIJoyHome(int joycodeIndex, int val);

int GetUIJoyEnd(int joycodeIndex);
void SetUIJoyEnd(int joycodeIndex, int val);

int GetUIJoySSChange(int joycodeIndex);
void SetUIJoySSChange(int joycodeIndex, int val);

int GetUIJoyHistoryUp(int joycodeIndex);
void SetUIJoyHistoryUp(int joycodeIndex, int val);

int GetUIJoyHistoryDown(int joycodeIndex);
void SetUIJoyHistoryDown(int joycodeIndex, int val);

int GetUIJoyExec(int joycodeIndex);
void SetUIJoyExec(int joycodeIndex, int val);

char* GetExecCommand(void);
void SetExecCommand(char* cmd);

int GetExecWait(void);
void SetExecWait(int wait);

BOOL GetHideMouseOnStartup(void);
void SetHideMouseOnStartup(BOOL hide);

BOOL GetRunFullScreen(void);
void SetRunFullScreen(BOOL fullScreen);

#endif
