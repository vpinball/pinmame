/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef OPTIONS_H
#define OPTIONS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_GAMEDESC 256

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
	COLUMN_MAX
};

enum
{
	VIEW_LARGE_ICONS = 0,
	VIEW_SMALL_ICONS,
	VIEW_INLIST,
	VIEW_REPORT,
	VIEW_MAX
};

enum
{
	SPLITTER_LEFT = 0,
	SPLITTER_RIGHT,
	SPLITTER_MAX
};

/* per-game data we calculate */
enum
{
	UNKNOWN = 2
};

/* Reg helpers types */
enum RegTypes
{
	RO_BOOL = 0, /* BOOL value                                            */
	RO_INT,      /* int value                                             */
	RO_DOUBLE,   /* double value         -                                */
	RO_STRING,   /* string               - m_vpData is an array           */
	RO_PSTRING,  /* pointer to string    - m_vpData is an allocated array */
	RO_ENCODE    /* encode/decode string - calls decode/encode functions  */
};

/* List of artwork types to display in the screen shot area */
enum
{
	PICT_SCREENSHOT = 0,
	PICT_FLYER,
	PICT_CABINET,
	PICT_MARQUEE,
	MAX_PICT_TYPES
};

/* Default input */
enum 
{
	INPUT_LAYOUT_STD,
	INPUT_LAYOUT_HR,
	INPUT_LAYOUT_HRSE
};

/* Reg data list */
typedef struct
{
	char m_cName[40];                             /* reg key name     */
	int  m_iType;                                 /* reg key type     */
	void *m_vpData;                               /* reg key data     */
	void (*encode)(void *data, char *str);        /* encode function  */
	void (*decode)(const char *str, void *data);  /* decode function  */
} REG_OPTIONS;

typedef struct
{
	int x, y, width, height;
} AREA;

typedef struct
{
	BOOL   use_default; /* only for non-default options */

	int    play_count;
	int    has_roms;
	int    has_samples;
	BOOL   is_favorite;

	/* video */
	BOOL   autoframeskip;
	int    frameskip;
	BOOL   wait_vsync;
	BOOL   use_triplebuf;
	BOOL   window_mode;
	BOOL   use_ddraw;
	BOOL   ddraw_stretch;
	char   resolution[16];
	int    gfx_refresh;
	BOOL   scanlines;
	BOOL   switchres;
	BOOL   switchbpp;
	BOOL   maximize;
	BOOL   keepaspect;
	BOOL   matchrefresh;
	BOOL   syncrefresh;
	BOOL   use_dirty;
	BOOL   throttle;
	double gfx_brightness;
	int    frames_to_display;
	char   effect[16];
	char   aspect[16];

	/* sound */

	/* input */
	BOOL   hotrod;
	BOOL   hotrodse;
	BOOL   use_mouse;
	BOOL   use_joystick;
	BOOL   steadykey;

	/* Core video */
	int    color_depth; /* MAME bitmap depth/bpp */
	BOOL   norotate;
	BOOL   ror;
	BOOL   rol;
	BOOL   flipx;
	BOOL   flipy;
	char   debugres[16];
	double gamma_correct;

	/* Core vector */
	BOOL   antialias;
	BOOL   translucency;
	double f_beam;
	double f_flicker;

	/* Sound */
	int    samplerate;
	BOOL   use_samples;
	BOOL   use_filter;
	BOOL   enable_sound;
	int    attenuation;

	/* misc */
	BOOL   use_artwork;
	BOOL   cheat;
	BOOL   mame_debug;
	char*  playbackname; // ?
	char*  recordname; // ?
	BOOL   errorlog;

} options_type;

typedef struct
{
	INT      folder_id;
	BOOL     view;
	BOOL     show_folderlist;
	BOOL     show_toolbar;
	BOOL     show_statusbar;
	BOOL     show_screenshot;
	int      show_pict_type;
	BOOL     game_check;        /* Startup GameCheck */
	BOOL     version_check;     /* Version mismatch warings */
	BOOL     use_joygui;
	BOOL     broadcast;
	char     default_game[MAX_GAMEDESC];
	int      column_width[COLUMN_MAX];
	int      column_order[COLUMN_MAX];
	int      column_shown[COLUMN_MAX];
	int      sort_column;
	BOOL     sort_reverse;
	AREA     area;
	int      splitter[SPLITTER_MAX];
	LOGFONT  list_font;
	COLORREF list_font_color;

	char*    language;
	char*    flyerdir;
	char*    cabinetdir;
	char*    marqueedir;

	char*    romdirs;
	char*    sampledirs;
	char*    cfgdir;
	char*    nvramdir;
	char*    memcarddir;
	char*    inpdir;
	char*    hidir;
	char*    statedir;
	char*    artdir;
	char*    imgdir;
	char*    cheatdir;
	char*    cheatfile;
	char*    history_filename;
	char*    mameinfo_filename;

} settings_type; /* global settings for the UI only */

void OptionsInit(int total_games);
void OptionsExit(void);

options_type* GetDefaultOptions(void);
options_type* GetGameOptions(int num_game);

void ResetGUI(void);
void ResetGameDefaults(void);
void ResetAllGameOptions(void);

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

void SetShowPictType(int val);
int  GetShowPictType(void);

void SetDefaultGame(const char *name);
const char *GetDefaultGame(void);

void SetWindowArea(AREA *area);
void GetWindowArea(AREA *area);

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

DWORD GetFolderFlags(char *folderName);
void  SetFolderFlags(char *folderName, DWORD dwFlags);

void SetListFontColor(COLORREF uColor);
COLORREF GetListFontColor(void);

void SetSortColumn(int column);
int  GetSortColumn(void);

const char* GetLanguage(void);
void SetLanguage(const char* lang);

const char* GetRomDirs(void);
void SetRomDirs(const char* paths);

const char* GetSampleDirs(void);
void  SetSampleDirs(const char* paths);

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

const char* GetCheatDir(void);
void SetCheatFileDir(const char* path);

const char* GetCheatFileName(void);
void SetCheatFileName(const char* path);

const char* GetHistoryFileName(void);
void SetHistoryFileName(const char* path);

const char* GetMAMEInfoFileName(void);
void SetMAMEInfoFileName(const char* path);

void ResetGameOptions(int num_game);

int  GetHasRoms(int num_game);
void SetHasRoms(int num_game, int has_roms);

int  GetHasSamples(int num_game);
void SetHasSamples(int num_game, int has_samples);

int  GetIsFavorite(int num_game);
void SetIsFavorite(int num_game, BOOL is_favorite);

void IncrementPlayCount(int num_game);
int  GetPlayCount(int num_game);

char * GetVersionString(void);

void SaveGameOptions(int game_num);
void SaveDefaultOptions(void);

#endif
