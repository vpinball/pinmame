/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  win32ui.c

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string.h>
#include <sys/stat.h>
#if defined(INTELLIMOUSE)
#include <zmouse.h>         /* Intellimouse */
#endif
#include <wingdi.h>
#include <time.h>

#include <driver.h>
#include <osdepend.h>
#include <unzip.h>

#include "resource.h"
#include "resource.hm"
#include "mame32.h"
#include "M32Util.h"
#include "mzip.h"
#include "file.h"
#include "audit32.h"
#include "Directories.h"
#include "Screenshot.h"
#include "Properties.h"
#include "ColumnEdit.h"
#include "TreeView.h"
#include "Splitters.h"
#include "ChildOutputStream.h"
#include "help.h"

#include "DirectDraw.h"
#include "DirectInput.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */

#if defined(__GNUC__)

/* fix warning: cast does not match function type */
#undef TreeView_GetNextItem
#define TreeView_GetNextItem(w,i,c) (HTREEITEM)(LRESULT)SendMessage((w),TVM_GETNEXTITEM,c,(LPARAM)(HTREEITEM)(i))

#undef ListView_GetImageList
#define ListView_GetImageList(w,i) (HIMAGELIST)(LRESULT)SendMessage((w),LVM_GETIMAGELIST,(i),0)

#endif /* defined(__GNUC__) */

/* Uncomment this to use internal background bitmaps */
/* #define INTERNAL_BKBITMAP */


#define MM_PLAY_GAME (WM_APP + 15000)

#define JOYGUI_MS 100

/* Max size of a sub-menu */
#define DBU_MIN_WIDTH  292
#define DBU_MIN_HEIGHT 190

int MIN_WIDTH  = DBU_MIN_WIDTH;
int MIN_HEIGHT = DBU_MIN_HEIGHT;

typedef BOOL (WINAPI *common_file_dialog_proc)(LPOPENFILENAME lpofn);

/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL             Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow);
static void             Win32UI_exit(void);

static BOOL             PumpMessage(void);
static BOOL             PumpAndReturnMessage(MSG *pmsg);
static void             OnIdle(void);
static void             OnSize(HWND hwnd, UINT state, int width, int height);
static long WINAPI      MameWindowProc(HWND hwnd,UINT message,UINT wParam,LONG lParam);

static void             SetView(int menu_id,int listview_style);
static void             ResetListView(void);
static void             UpdateGameList(void);
static void             ReloadIcons(HWND hWnd);
static void             PollGUIJoystick(void);
static void             PressKey(HWND hwnd,UINT vk);
static BOOL             MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);

static void             UpdateStatusBar(void);
static BOOL             PickerHitTest(HWND hWnd);
static BOOL             MamePickerNotify(NMHDR *nm);
static BOOL             TreeViewNotify(NMHDR *nm);

static void             LoadListBitmap(void);
static void             PaintControl(HWND hWnd, BOOL useBitmap);

static void             InitPicker(void);
static int CALLBACK     ListCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem);

static void             DisableSelection(void);
static void             EnableSelection(int nGame);

static int              GetSelectedPick(void);
static int              GetSelectedPickItem(void);
static void             SetSelectedPick(int new_index);
static void             SetSelectedPickItem(int val);
static void             SetRandomPickItem(void);

static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK DirectXDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

static BOOL             SelectLanguageFile(HWND hWnd, TCHAR* filename);
static void             MamePlayRecordGame(void);
static void             MamePlayBackGame(void);
static BOOL             CommonFileDialog(common_file_dialog_proc cfd,char *filename, BOOL bZip);
static void             MamePlayGame(void);
static void             MamePlayGameWithOptions(int nGame);
static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void             ColumnSort(int column, BOOL bColumn);
static BOOL             GameCheck(void);

static void             ToggleScreenShot(void);
static void             AdjustMetrics(void);
static void             EnablePlayOptions(int nIndex, options_type *o);

/* for header */
static LRESULT CALLBACK HeaderWndProc(HWND, UINT, WPARAM, LPARAM);
static void             Header_SetSortInfo(HWND hWndList, int nCol, BOOL bAsc);
static void             Header_Initialize(HWND hWndList);
static BOOL             ListCtrlOnErase(HWND hWnd, HDC hDC);
static BOOL             ListCtrlOnPaint(HWND hWnd, UINT uMsg);

/* Icon routines */
static DWORD            GetShellLargeIconSize(void);
static BOOL             CreateIcons(HWND hWnd);
static int              WhichIcon(int nItem);
static void             AddIcon(int cmkindex);

/* Context Menu handlers */
static void             UpdateMenu(HWND hWnd, HMENU hMenu);
static BOOL             HandleContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HeaderOnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleTreeContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);

/* Re/initialize the ListView header columns */
static void             ResetColumnDisplay(BOOL firstime);

/* Custom Draw item */
static void             DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
static int              GetNumColumns(HWND hWnd);

static void             CopyToolTipText (LPTOOLTIPTEXT lpttt);

static void             ProgressBarShow(void);
static void             ProgressBarHide(void);
static void             ResizeProgressBar(void);
static void             ProgressBarStep(void);

static HWND             InitProgressBar(HWND hParent);
static HWND             InitToolbar(HWND hParent);
static HWND             InitStatusBar(HWND hParent);

static LRESULT          Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);

static void             UpdateHistory(int gameNum);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	BYTE    bWidth;               /* Width of the image */
	BYTE    bHeight;              /* Height of the image (times 2) */
	BYTE    bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	BYTE    bReserved;            /* Reserved */
	WORD    wPlanes;              /* Color Planes */
	WORD    wBitCount;            /* Bits per pixel */
	DWORD   dwBytesInRes;         /* how many bytes in this resource? */
	DWORD   dwImageOffset;        /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; /* Width, Height and bpp */
	LPBYTE          lpBits;                /* ptr to DIB bits */
	DWORD           dwNumBytes;            /* how many bytes? */
	LPBITMAPINFO    lpbi;                  /* ptr to header */
	LPBYTE          lpXOR;                 /* ptr to XOR image bits */
	LPBYTE          lpAND;                 /* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

/* Which edges of a control are anchored to the corresponding side of the parent window */
#define RA_LEFT     0x01
#define RA_RIGHT    0x02
#define RA_TOP      0x04
#define RA_BOTTOM   0x08
#define RA_ALL      0x0F

#define RA_END  0
#define RA_ID   1
#define RA_HWND 2

typedef struct
{
	int         type;       /* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
							   to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int     id;         /* Window control id */
		HWND    hwnd;       /* Window handle */
	} u;
	int         action;     /* What to do when control is resized */
	void        *subwindow; /* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT        rect;       /* Client rect of window; must be initialized before first resize */
	ResizeItem* items;      /* Array of subitems to be resized */
} Resize;

static void             ResizeWindow(HWND hParent, Resize *r);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

#define MAX_MENUS 3

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND   hMain  = NULL;
static HACCEL hAccel = NULL;

static HWND hPicker   = NULL;
static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;

static BOOL g_bAbortLoading = FALSE; /* doesn't work right */

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     /* Font for list view */

static game_data_type*  game_data = NULL;
static int              game_count = 0;

static int  last_sort = 0;

/* global data--know where to send messages */
static BOOL in_emulation;

/* idle work at startup */
static BOOL idle_work;

static int  game_index;
static int  progBarStep;

static BOOL bDoGameCheck = FALSE;

/* current menu check for listview */
static int current_view_id;

/* Tree control variables */
static BOOL bShowTree      = 1;
static BOOL bShowToolBar   = 1;
static BOOL bShowStatusBar = 1;
static BOOL bProgressShown = FALSE;
static BOOL bListReady     = FALSE;

/* use a joystick subsystem in the gui? */
static struct OSDJoystick* g_pJoyGUI = NULL;

/* Intellimouse available? (0 = no) */
#if defined(INTELLIMOUSE)
static UINT uiMsh_MsgMouseWheel;
#endif

/* sort columns in reverse order */
static BOOL    reverse_sort      = FALSE;
static UINT    lastColumnClick   = 0;
static UINT    m_uHeaderSortCol  = 0;
static BOOL    m_fHeaderSortAsc  = TRUE;
static WNDPROC g_lpHeaderWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

/* Tool and Status bar variables */
static HWND hStatusBar = 0;
static HWND hToolBar   = 0;

/* Column Order as Displayed */
static BOOL oldControl = FALSE;
static int  realColumn[COLUMN_MAX];

/* Used to recalculate the main window layout */
static int  bottomMargin;
static int  topMargin;
static int  have_history = FALSE;

static BOOL bScreenShotAvailable = FALSE;
static BOOL nPictType = PICT_SCREENSHOT;
static BOOL have_selection = FALSE;

/* Icon variables */
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static int          *icon_index = NULL;

/* Icon names we will load and use */
static char* icon_names[] =
{
	"noroms",
	"roms",
	"unknown",
	"warning"
};

#define NUM_ICONS (sizeof(icon_names) / sizeof(icon_names[0]))

static TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,    TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_SCREEN_SHOT,TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_ICON,       TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_SMALL_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{4, ID_VIEW_LIST_MENU,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{5, ID_VIEW_DETAIL,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 5},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{6, ID_HELP_ABOUT,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 6},
	{7, ID_HELP_CONTENTS,   TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7}
};

#define NUM_TOOLBUTTONS (sizeof(tbb) / sizeof(tbb[0]))

#define NUM_TOOLTIPS 7

static char szTbStrings[NUM_TOOLTIPS + 1][30] =
{
	"Toggle Folder List",
	"Toggle Screen Shot",
	"Large Icons",
	"Small Icons",
	"List",
	"Details",
	"About",
	"Help"
};

static int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_SCREEN_SHOT,
	ID_VIEW_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_HWND, { 0 },            RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSDEFPIC }, RA_RIGHT | RA_TOP,                 NULL },
	{ RA_END,  { 0 },            0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

/* our dialog/configured options */
static options_type playing_game_options;

/* last directory for common file dialogs */
static char last_directory[MAX_PATH];

/* system-wide window message sent out with an ATOM of the current game name
   each time it changes */
static UINT g_mame32_message = 0;
static BOOL g_bDoBroadcast   = FALSE;

#ifndef IsValidListControl
#define IsValidListControl(hwnd) ((hwnd) == hwndList)
#endif /* IsValidListControl */

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Background Image handles also accessed from TreeView.c */
HPALETTE         hPALbg   = 0;
HBITMAP          hBitmap  = 0;
MYBITMAPINFO     bmDesc;

/* List view Column text */
char* column_names[COLUMN_MAX] =
{
	"Game",
	"ROMs",
	"Samples",
	"Directory",
	"Type",
	"Trackball",
	"Played",
	"Manufacturer",
	"Year",
	"Clone Of"
};

/* a tiny compile is without Neogeo games */
#if (defined(NEOFREE) || defined(TINY_COMPILE)) && !defined(NEOMAME)
struct GameDriver driver_neogeo =
{
	__FILE__,
	0,
	"Neo-Geo Fake driver",
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NOT_A_DRIVER,
};
#else
extern struct GameDriver driver_neogeo;
#endif

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) \
    SendMessage(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) \
    SendMessage(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

/***************************************************************************
    External functions
 ***************************************************************************/

static char* g_pRecordName = NULL;
static char* g_pPlayBkName = NULL;

static void CreateCommandLine(int nGameIndex, char* pCmdLine)
{
	char pModule[_MAX_PATH];
	options_type* pOpts;

	GetModuleFileName(GetModuleHandle(NULL), pModule, _MAX_PATH);

	pOpts = GetGameOptions(nGameIndex);

	sprintf(pCmdLine, "%s %s", pModule, drivers[nGameIndex]->name);

	sprintf(&pCmdLine[strlen(pCmdLine)], " -rompath \"%s\"",            GetRomDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -samplepath \"%s\"",         GetSampleDirs());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cfg_directory \"%s\"",      GetCfgDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -nvram_directory \"%s\"",    GetNvramDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -memcard_directory \"%s\"",  GetMemcardDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -input_directory \"%s\"",    GetInpDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -hiscore_directory \"%s\"",  GetHiDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -state_directory \"%s\"",    GetStateDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -artwork_directory \"%s\"",  GetArtDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -snapshot_directory \"%s\"", GetImgDir());
/*	sprintf(&pCmdLine[strlen(pCmdLine)], " -cheat_directory %s",        GetCheatDir());*/
	sprintf(&pCmdLine[strlen(pCmdLine)], " -cheat_file \"%s\"",         GetCheatFileName());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -history_file \"%s\"",       GetHistoryFileName());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -mameinfo_file \"%s\"",      GetMAMEInfoFileName());

	/* video */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sautoframeskip",           pOpts->autoframeskip   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -frameskip %d",              pOpts->frameskip);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%swaitvsync",               pOpts->wait_vsync      ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%striplebuffer",            pOpts->use_triplebuf   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%swindow",                  pOpts->window_mode     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sddraw",                   pOpts->use_ddraw       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%shwstretch",               pOpts->ddraw_stretch   ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -resolution %s",             pOpts->resolution);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -refresh %d",                pOpts->gfx_refresh);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sscanlines",               pOpts->scanlines       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sswitchres",               pOpts->switchres       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sswitchbpp",               pOpts->switchbpp       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smaximize",                pOpts->maximize        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%skeepaspect",              pOpts->keepaspect      ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smatchrefresh",            pOpts->matchrefresh    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssyncrefresh",             pOpts->syncrefresh     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdirty",                   pOpts->use_dirty       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sthrottle",                pOpts->throttle        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -full_screen_brightness %f", pOpts->gfx_brightness);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -frames_to_run %d",          pOpts->frames_to_display);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -effect %s",                 pOpts->effect);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -screen_aspect %s",          pOpts->aspect);

	/* input */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%shotrod",                  pOpts->hotrod          ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%shotrodse",                pOpts->hotrodse        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%smouse",                   pOpts->use_mouse       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sjoystick",                pOpts->use_joystick    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssteadykey",               pOpts->steadykey    ? "" : "no");

	/* core video */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -bpp %d",                    pOpts->color_depth);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%snorotate",                pOpts->norotate        ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sror",                     pOpts->ror             ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%srol",                     pOpts->rol             ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipx",                   pOpts->flipx           ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sflipy",                   pOpts->flipy           ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -debug_resolution %s",       pOpts->debugres);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -gamma %f",                  pOpts->gamma_correct);

	/* vector */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%santialias",               pOpts->antialias       ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%stranslucency",            pOpts->translucency    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -beam %f",                   pOpts->f_beam);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -flicker %f",                pOpts->f_flicker);

	/* sound */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -samplerate %d",             pOpts->samplerate);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssamples",                 pOpts->use_samples     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sresamplefilter",          pOpts->use_filter      ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%ssound",                   pOpts->enable_sound    ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -volume %d",                 pOpts->attenuation);

	/* misc */
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sartwork",                 pOpts->use_artwork     ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%scheat",                   pOpts->cheat           ? "" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdebug",                   pOpts->mame_debug      ? "" : "no");
	if (g_pPlayBkName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -playback \"%s\"",       g_pPlayBkName);
	if (g_pRecordName != NULL)
		sprintf(&pCmdLine[strlen(pCmdLine)], " -record \"%s\"",         g_pRecordName);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%slog",                     pOpts->errorlog        ? "" : "no");
#ifdef PINMAME
	sprintf(&pCmdLine[strlen(pCmdLine)], " -wave_directory \"%s\"",     GetWaveDir());
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_red %d",                pOpts->dmd_red);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_green %d",              pOpts->dmd_green);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_blue %d",               pOpts->dmd_blue);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_perc0 %d",              pOpts->dmd_perc0);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_perc33 %d",             pOpts->dmd_perc33);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_perc66 %d",             pOpts->dmd_perc66);
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdmd_only",                pOpts->dmd_only?"" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -%sdmd_compact",             pOpts->dmd_compact?"" : "no");
	sprintf(&pCmdLine[strlen(pCmdLine)], " -dmd_antialias %d",          pOpts->dmd_antialias);
#endif /* PINMAME */
}

static BOOL WaitWithMessageLoop(HANDLE hEvent)
{
	DWORD dwRet;
	MSG   msg;

	while (1)
	{
		dwRet = MsgWaitForMultipleObjects(1, &hEvent, FALSE, INFINITE, QS_ALLINPUT);

		if (dwRet == WAIT_OBJECT_0)
			return TRUE;

		if (dwRet != WAIT_OBJECT_0 + 1)
			break;

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0)
				return TRUE;
		}
	}
	return FALSE;
}

static int RunMAME(int nGameIndex, HANDLE hErrorWrite)
{
	DWORD               dwExitCode = 0;
	STARTUPINFO         si;
	PROCESS_INFORMATION pi;
	char pCmdLine[2048];
	
	CreateCommandLine(nGameIndex, pCmdLine);

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	si.dwFlags    = STARTF_USESTDHANDLES;
	si.hStdOutput = hErrorWrite; /* combine stdout and stderr */
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdError  = hErrorWrite;

	if (!CreateProcess(NULL,
						pCmdLine,
						NULL,		  /* Process handle not inheritable. */
						NULL,		  /* Thread handle not inheritable. */
						TRUE,		  /* Handle inheritance.  */
						0,			  /* Creation flags. */
						NULL,		  /* Use parent's environment block.  */
						NULL,		  /* Use parent's starting directory.  */
						&si,		  /* STARTUPINFO */
						&pi))		  /* PROCESS_INFORMATION */
	{
		OutputDebugString("CreateProcess failed.");
		dwExitCode = GetLastError();
	}
	else
	{
		ShowWindow(hMain, SW_HIDE);

		CloseHandle(hErrorWrite);

		/* Wait until child process exits. */
		WaitWithMessageLoop(pi.hProcess);

		GetExitCodeProcess(pi.hProcess, &dwExitCode);

		ShowWindow(hMain, SW_SHOW);

		/* Close process and thread handles. */
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return dwExitCode;
}

int WINAPI WinMain(HINSTANCE    hInstance,
                   HINSTANCE    hPrevInstance,
                   LPSTR        lpCmdLine,
                   int          nCmdShow)
{
	MSG 	msg;

	options.gui_host = 1;

	if (__argc != 1)
	{
		/* Rename main because gcc will use it instead of WinMain even with -mwindows */
		extern int DECL_SPEC main_(int, char**);
		exit(main_(__argc, __argv));
	}

	if (!Win32UI_init(hInstance, lpCmdLine, nCmdShow))
		return 1;
	/*
		Simplified MFC Run() alg. See mfc/src/thrdcore.cpp.
	*/
	for (;;)
	{
		/* phase1: check to see if we can do idle work */
		while (idle_work && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			/* call OnIdle while idle_work */
			OnIdle();
		}

		/* phase2: pump messages while available */
		do
		{
			/* pump message, but quit on WM_QUIT */
			if (!PumpMessage())
			{
				Win32UI_exit();
				return msg.wParam;
			}

		}
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
	}

	return 0;
}


HWND GetMainWindow(void)
{
	return hMain;
}

int GetNumColumns(HWND hWnd)
{
	int  nColumn = 0;
	int  i;
	HWND hwndHeader;
	int  shown[COLUMN_MAX];

	GetColumnShown(shown);
	hwndHeader = GetDlgItem(hWnd, 0);

	if (oldControl || (nColumn = Header_GetItemCount(hwndHeader)) < 1)
	{
		nColumn = 0;
		for (i = 0; i < COLUMN_MAX ; i++ )
		{
			if (shown[i])
				nColumn++;
		}
	}
	return nColumn;
}

void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX];
	int nColumnMax;
	int i;

	nColumnMax = GetNumColumns(hwndList);

	/* Get the Column Order and save it */
	if (!oldControl)
	{
		ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++)
		{
			order[i] = realColumn[tmpOrder[i]];
		}
	}
}

BOOL GameUsesTrackball(int game)
{
	int port;

	/* new trackball support */
	if (drivers[game]->input_ports != 0)
	{
		port = 0;
		while (drivers[game]->input_ports[port].type != IPT_END)
		{
			int type = drivers[game]->input_ports[port].type & ~IPF_MASK;
			if (type == IPT_DIAL
			||	type == IPT_PADDLE
			||	type == IPT_TRACKBALL_X
			||	type == IPT_TRACKBALL_Y
			||	type == IPT_AD_STICK_X
			||	type == IPT_AD_STICK_Y)
			{
				return TRUE;
			}
			port++;
		}
		return FALSE;
	}
	return FALSE;
}

/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	ICONIMAGE           IconImage;
	LPICONDIRENTRY      lpIDE = NULL;
	UINT                nNumImages;
	UINT                nBufferIndex = 0;
	HICON               hIcon = NULL;

	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it 'reserved' ?	 (ie 0) */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Then that's the number of images in the ICO file */
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if ( nNumImages < 1 )
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
	{
		return NULL;
	}

	/* Assign icon directory entries from buffer */
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if ( lpIDE->dwImageOffset > nBufferSize )
	{
		return NULL;
	}

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
	{
		return NULL;
	}

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;

	hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000,
			(*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biWidth, (*(LPBITMAPINFOHEADER)(IconImage.lpBits)).biHeight/2, 0 );

	/* It failed, odds are good we're on NT so try the non-Ex way */
	if (hIcon == NULL)
	{
		/* We would break on NT if we try with a 16bpp image */
		if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		{
			hIcon = CreateIconFromResource(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000);
		}
	}
	return hIcon;
}

HICON LoadIconFromFile(char *iconname)
{
	HICON       hIcon = 0;
	struct stat file_stat;
	char        tmpStr[MAX_PATH];
	char        tmpIcoName[MAX_PATH];
	const char* sDirName = GetImgDir();
	PBYTE       bufferPtr;
	UINT        bufferLen;

	sprintf(tmpStr, "icons/%s.ico",iconname);
	if (stat(tmpStr, &file_stat) != 0
	|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
	{
		sprintf(tmpStr, "%s/%s.ico", sDirName, iconname);
		if (stat(tmpStr, &file_stat) != 0
		|| (hIcon = ExtractIcon(hInst, tmpStr, 0)) == 0)
		{
			sprintf(tmpStr, "icons/icons.zip");
			sprintf(tmpIcoName, "%s.ico", iconname);
			if (stat(tmpStr, &file_stat) == 0)
			{
				if (load_zipped_file(tmpStr, tmpIcoName, &bufferPtr, &bufferLen) == 0)
				{
					hIcon = FormatICOInMemoryToHICON(bufferPtr, bufferLen);
					free(bufferPtr);
				}
			}
		}
	}
	return hIcon;
}

/* Return the number of games currently displayed */
int GetNumGames()
{
	return game_count;
}

game_data_type * GetGameData()
{
	return game_data;
}

/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot(void)
{
	HWND hList = GetDlgItem(hPicker, IDC_LIST);
	HWND hParent = GetParent(hList);
	RECT rect;
	int  nWidth, show;
	int  nTreeWidth = 0;

	/* Size the List Control in the Picker */
	GetClientRect(hParent, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowScreenShot())
	{
		nWidth = nSplitterOffset[SPLITTER_MAX - 1];
		show   = SW_SHOW;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_SCREEN_SHOT, MF_CHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_SCREEN_SHOT, MF_CHECKED);
	}
	else
	{
		nWidth = rect.right;
		show   = SW_HIDE;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_SCREEN_SHOT, MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_SCREEN_SHOT, MF_UNCHECKED);
	}
	if (show == SW_HIDE)
	{
		ShowWindow(GetDlgItem(hParent, IDC_SSPICTURE), SW_HIDE);
		ShowWindow(GetDlgItem(hParent, IDC_SSFRAME),   SW_HIDE);
		ShowWindow(GetDlgItem(hParent, IDC_SSDEFPIC),  SW_HIDE);
	}

	ShowWindow(GetDlgItem(hParent, IDC_HISTORY),   SW_HIDE);

	/* Tree control */
	if (bShowTree)
	{
		nTreeWidth = nSplitterOffset[0];
		if (nTreeWidth >= nWidth)
		{
			nTreeWidth = nWidth - 10;
		}

		MoveWindow(GetDlgItem(hParent, IDC_TREE), 0, rect.top + 2, nTreeWidth - 2,
			(rect.bottom - rect.top) - 4 , TRUE);

		MoveWindow(GetDlgItem(hParent, IDC_SPLITTER), nTreeWidth, rect.top + 2,
			4, (rect.bottom - rect.top) - 4, TRUE);

		ShowWindow(GetDlgItem(hParent, IDC_TREE), SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hParent, IDC_TREE), SW_HIDE);
	}

	/* Splitter window #2 */
	MoveWindow(GetDlgItem(hParent, IDC_SPLITTER2), nSplitterOffset[1],
		rect.top + 2, 4, (rect.bottom - rect.top) - 2, FALSE);

	/* List control */
	MoveWindow(hList, 2 + nTreeWidth, rect.top + 2,
		(nWidth - nTreeWidth) - 2, (rect.bottom - rect.top) - 4 , TRUE);

	if (GetShowScreenShot())
		ShowWindow(GetDlgItem(hParent, IDC_SSFRAME), SW_SHOW);

	UpdateHistory(GetSelectedPickItem());

	if (GetShowScreenShot() && !nPictType && have_selection && have_history)
		ShowWindow(GetDlgItem(hParent, IDC_HISTORY), SW_SHOW);
}

void ResizePickerControls(HWND hWnd)
{
	RECT picRect;
	RECT frameRect;
	RECT rect, sRect;
	int  nListWidth, nScreenShotWidth, picX, picY, nTreeWidth;
	static BOOL firstTime = TRUE;
	int  doSSControls = TRUE;

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc 1/2 of the display for the screenshot and 1/4 for the treecontrol */
	if (firstTime)
	{
		RECT rWindow;

		nListWidth = rect.right / 2;
		nSplitterOffset[1] = nListWidth;
		nSplitterOffset[0] = nListWidth / 2;
		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		/*buttonMargin = (sRect.bottom + 4); */

		firstTime = FALSE;
	}
	else
	{
		doSSControls = GetShowScreenShot();
		nListWidth = nSplitterOffset[1];
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, TRUE);

	nScreenShotWidth = (rect.right - nListWidth) - 4;

	/* Tree Control */
	nTreeWidth = nSplitterOffset[0];
	MoveWindow(GetDlgItem(hWnd, IDC_TREE), 4, rect.top + 2,
		nTreeWidth - 4, (rect.bottom - rect.top) - 4, TRUE);

	/* Splitter window #1 */
	MoveWindow(GetDlgItem(hWnd, IDC_SPLITTER), nTreeWidth, rect.top + 2,
		4, (rect.bottom - rect.top) - 4, TRUE);

	/* List control */
	MoveWindow(GetDlgItem(hWnd, IDC_LIST), 4 + nTreeWidth, rect.top + 2,
		(nListWidth - nTreeWidth) - 4, (rect.bottom - rect.top) - 4, TRUE);

	/* Splitter window #2 */
	MoveWindow(GetDlgItem(hWnd, IDC_SPLITTER2), nListWidth, rect.top + 2,
		4, (rect.bottom - rect.top) - 2, doSSControls);

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2,
		nScreenShotWidth - 2, (rect.bottom - rect.top) - 4, doSSControls);

	/* The screen shot controls */
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);
	GetClientRect(GetDlgItem(hWnd, IDC_SSDEFPIC), &picRect);

	picX = (frameRect.right - (picRect.right - picRect.left)) / 2;
	picY = (frameRect.bottom - (picRect.bottom - picRect.top)) / 2;

	/* Mame logo */
	MoveWindow(GetDlgItem(hWnd, IDC_SSDEFPIC), nListWidth + picX + 4,
		rect.top + 24, picRect.right + 2, picRect.bottom + 2, doSSControls);

	/* Screen shot sunken frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSPICTURE), nListWidth + picX + 4,
		rect.top + picY , picRect.right, picRect.bottom, doSSControls);

	/* Text control - game history */
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + (nScreenShotWidth - 22);
	sRect.top = rect.top + 264;
	sRect.bottom = (rect.bottom - rect.top) - 278;

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY),
		sRect.left, sRect.top,
		sRect.right - sRect.left, sRect.bottom, doSSControls);
}

char *ModifyThe(const char *str)
{
	static int  bufno = 0;
	static char buffer[4][255];

	if (strncmp(str, "The ", 4) == 0)
	{
		char *s, *p;
		char temp[255];

		strcpy(temp, &str[4]);

		bufno = (bufno + 1) % 4;

		s = buffer[bufno];

		/* Check for version notes in parens */
		p = strchr(temp, '(');
		if (p)
		{
			p[-1] = '\0';
		}

		strcpy(s, temp);
		strcat(s, ", The");

		if (p)
		{
			strcat(s, " ");
			strcat(s, p);
		}

		return s;
	}
	return (char *)str;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static BOOL Win32UI_init(HINSTANCE hInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS	wndclass;
	RECT		rect;
	int 		i;

	srand((unsigned)time(NULL));

	game_count = 0;
	while (drivers[game_count] != 0)
		game_count++;

	game_data = (game_data_type *)malloc(game_count * sizeof(game_data_type));

	wndclass.style		   = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance	   = hInstance;
	wndclass.hIcon		   = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAME32_ICON));
	wndclass.hCursor	   = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = "MAME32";

	RegisterClass(&wndclass);

	InitCommonControls();

	/* Are we using an Old comctl32.dll? */
	oldControl = (GetDllVersion() == FALSE);
	if (oldControl)
	{
		char buf[] = MAME32NAME " has detected an old version of comctl32.dll\n\n"
					 "Game Properties, many configuration options and\n"
					 "features are not available without an updated DLL\n\n"
					 "Please install the common control update found at:\n\n"
					 "http://www.microsoft.com/msdownload/ieplatform/ie/comctrlx86.asp\n\n"
					 "Would you like to continue without using the new features?\n";

		if (IDNO == MessageBox(0, buf, MAME32NAME " Outdated comctl32.dll Warning", MB_YESNO | MB_ICONWARNING))
			return FALSE;
    }

	OptionsInit(game_count);

	for (i = 0; i < game_count; i++)
	{
		const struct GameDriver* pDriver;
		BOOL bNeoGeo = FALSE;

		pDriver = drivers[i]->clone_of;

		do
		{
			if (pDriver == &driver_neogeo)
				bNeoGeo = TRUE;
			pDriver = pDriver->clone_of;
		}
		while (pDriver && bNeoGeo == FALSE);

		game_data[i].neogeo = bNeoGeo;
	}

	g_mame32_message = RegisterWindowMessage("MAME32");
	g_bDoBroadcast = GetBroadcast();

	Help_Init();

	/* init files after OptionsInit to init paths */
	File_Init();
	strcpy(last_directory, GetInpDir());

	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);

	/* Load the pic for the default screenshot. */
	SendMessage(GetDlgItem(hMain, IDC_SSDEFPIC),
				STM_SETIMAGE,
				(WPARAM)IMAGE_BITMAP,
				(LPARAM)(void*)LoadImage(GetModuleHandle(NULL),
										 MAKEINTRESOURCE(IDB_ABOUT),
										 IMAGE_BITMAP, 0, 0, LR_SHARED));

	hPicker = hMain;

	/* Stash hInstance for later use */
	hInst = hInstance;

	hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	/* In order to handle 'Large Fonts' as the Windows
	 * default setting, we need to make the dialogs small
	 * enough to fit in our smallest window size with
	 * large fonts, then resize the picker, tab and button
	 * controls to fill the window, no matter which font
	 * is currently set.  This will still look like bad
	 * if the user uses a bigger default font than 125%
	 * (Large Fonts) on the Windows display setting tab.
	 *
	 * NOTE: This has to do with Windows default font size
	 * settings, NOT our picker font size.
	 */

	GetClientRect(hMain, &rect);

	hTreeView = GetDlgItem(hPicker, IDC_TREE);
	hwndList  = GetDlgItem(hPicker, IDC_LIST);

	InitSplitters();


	AddSplitter(GetDlgItem(hPicker, IDC_SPLITTER), hTreeView, hwndList,
				AdjustSplitter1Rect);
	AddSplitter(GetDlgItem(hPicker, IDC_SPLITTER2), hwndList,
				GetDlgItem(hPicker,IDC_SSFRAME),AdjustSplitter2Rect);

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hPicker);

	/* Reset the font */
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		hFont = CreateFontIndirect(&logfont);
		if (hFont != NULL)
		{
			if (hwndList != NULL)
			{
				HWND    hwndHeaderCtrl  = NULL;
				HFONT   hHeaderCtrlFont = NULL;

				hwndHeaderCtrl = GetDlgItem(hwndList, 0);
				if (hwndHeaderCtrl)
				{
					hHeaderCtrlFont = GetWindowFont( hwndHeaderCtrl);
				}

    			SetWindowFont(hwndList, hFont, FALSE);

				/* Reset header ctrl font back to original font */

				if (hHeaderCtrlFont)
				{
					SetWindowFont(hwndHeaderCtrl, hHeaderCtrlFont, TRUE);
				}
			}

			if (hTreeView != NULL)
			{
				SetWindowFont(hTreeView, hFont, FALSE);
			}

			SetWindowFont(GetDlgItem(hPicker, IDC_HISTORY), hFont, FALSE);
		}
	}

	nPictType = GetShowPictType();

	bDoGameCheck = GetGameCheck();
	idle_work    = TRUE;
	game_index   = 0;

	bShowTree	   = GetShowFolderList();
	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();

	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);

	if (oldControl)
	{
		EnableMenuItem(GetMenu(hMain), ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_GAME_PROPERTIES,	MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	/* Init DirectDraw */
	if (!DirectDraw_Initialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	LoadListBitmap();

	InitTree(hTreeView, game_count);

	/* Subclass the tree control for painting */
	Tree_Initialize(hTreeView);

	SetCurrentFolder(GetFolderByID(GetSavedFolderID()));

	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS:
		SetView(ID_VIEW_ICON,LVS_ICON);
		break;

	case VIEW_SMALL_ICONS:
		SetView(ID_VIEW_SMALL_ICON,LVS_SMALLICON);
		break;

	case VIEW_INLIST:
		SetView(ID_VIEW_LIST_MENU,LVS_LIST);
		break;

	case VIEW_REPORT:
	default:
		SetView(ID_VIEW_DETAIL,LVS_REPORT);
		break;
	}

	/* Initialize listview columns */
	InitPicker();
	SetFocus(hwndList);

	/* Init Intellimouse */
#if defined(INTELLIMOUSE)
	{
		UINT uiMsh_Msg3DSupport;
		UINT uiMsh_MsgScrollLines;
		BOOL f3DSupport;
		INT  iScrollLines;
		HWND hwndMsWheel;

		hwndMsWheel = HwndMSWheel(
			&uiMsh_MsgMouseWheel, &uiMsh_Msg3DSupport,
			&uiMsh_MsgScrollLines, &f3DSupport, &iScrollLines);
	}
#endif

	/* Init DirectInput */
	if (!DirectInputInitialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	AdjustMetrics();
	UpdateScreenShot();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TAB_KEYS));

	InitZip(hMain);

	if (GetJoyGUI() == TRUE)
	{
		g_pJoyGUI = &DIJoystick;
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, 0, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	ShowWindow(hMain, nCmdShow);

	return TRUE;
}

static void Win32UI_exit()
{
	ExitZip();

    if (g_bDoBroadcast == TRUE)
    {
        ATOM a = GlobalAddAtom("");
        SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
        GlobalDeleteAtom(a);
    }

	if (g_pJoyGUI != NULL)
		g_pJoyGUI->exit();

	DestroyAcceleratorTable(hAccel);

	if (game_data != NULL)
		free(game_data);
	if (icon_index != NULL)
		free(icon_index);

	DirectInputClose();
	DirectDraw_Close();

	SetSavedFolderID(GetCurrentFolderID());
	FreeFolders();
	SetViewMode(current_view_id - ID_VIEW_ICON);

    //DestroyTree(hTreeView);

	FreeScreenShot();

	OptionsExit();

	File_Exit();

	Help_Exit();
}

static long WINAPI MameWindowProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	MINMAXINFO	*mminfo;
	int 		i;

	switch (message)
	{
	case WM_INITDIALOG:
		/* Initialize info for resizing subitems */
		GetClientRect(hWnd, &main_resize.rect);
		return TRUE;

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETTINGCHANGE:
		AdjustMetrics();
		return 0;

	case WM_SIZE:
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(wParam));
		return TRUE;

	case WM_MENUSELECT:
		return Statusbar_MenuSelect(hWnd, wParam, lParam);

	case MM_PLAY_GAME:
		MamePlayGame();
		return TRUE;

	case WM_INITMENUPOPUP:
		UpdateMenu(GetDlgItem(hWnd, IDC_LIST), GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if (HandleTreeContextMenu(hWnd, wParam, lParam)
		||	HandleContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;

	case WM_COMMAND:
		return MameCommand(hWnd,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

	case WM_GETMINMAXINFO:
		/* Don't let the window get too small; it can break resizing */
		mminfo = (MINMAXINFO *) lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
		return 0;

	case WM_TIMER:
		PollGUIJoystick();
		return TRUE;

	case WM_CLOSE:
		{
			/* save current item */
			RECT rect;
			AREA area;
			int widths[COLUMN_MAX];
			int order[COLUMN_MAX];
			int shown[COLUMN_MAX];
			int tmpOrder[COLUMN_MAX];
			int nItem;
			int nColumnMax = 0;

			/* Restore the window before we attempt to save parameters,
			 * This fixed the lost window on startup problem.
			 */
			ShowWindow(hWnd, SW_RESTORE);

			GetColumnOrder(order);
			GetColumnShown(shown);
			GetColumnWidths(widths);

			nColumnMax = GetNumColumns(hwndList);

			if (oldControl)
			{
				for (i = 0; i < nColumnMax; i++)
				{
					widths[realColumn[i]] = ListView_GetColumnWidth(hwndList, i);
				}
			}
			else
			{
				/* Get the Column Order and save it */
				ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

				for (i = 0; i < nColumnMax; i++)
				{
					widths[realColumn[i]] = ListView_GetColumnWidth(hwndList, i);
					order[i] = realColumn[tmpOrder[i]];
				}
			}

			SetColumnWidths(widths);
			SetColumnOrder(order);

			for (i = 0; i < SPLITTER_MAX; i++)
				SetSplitterPos(i, nSplitterOffset[i]);

			GetWindowRect(hWnd, &rect);
			area.x		= rect.left;
			area.y		= rect.top;
			area.width	= rect.right  - rect.left;
			area.height = rect.bottom - rect.top;
			SetWindowArea(&area);

			/* Save the users current game options and default game */
			nItem = GetSelectedPickItem();
			SetDefaultGame(ModifyThe(drivers[nItem]->description));

			/* hide window to prevent orphan empty rectangles on the taskbar */
			/* ShowWindow(hWnd,SW_HIDE); */
            DestroyWindow( hWnd );

			/* PostQuitMessage(0); */
			break;
		}

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_MOUSEMOVE:
		OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_LBUTTONUP:
		OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_NOTIFY:
		/* Where is this message intended to go */
		{
			LPNMHDR lpNmHdr = (LPNMHDR)lParam;

			/* Fetch tooltip text */
			if (lpNmHdr->code == TTN_NEEDTEXT)
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				CopyToolTipText(lpttt);
			}
			if (lpNmHdr->hwndFrom == hwndList)
				return MamePickerNotify(lpNmHdr);
			if (lpNmHdr->hwndFrom == hTreeView)
				return TreeViewNotify(lpNmHdr);
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;
			switch (lpDis->CtlID)
			{
			case IDC_LIST:
				DrawItem((LPDRAWITEMSTRUCT)lParam);
				break;
			}
		}
		break;

	case WM_PAINT:
		if (GetShowScreenShot())
		{
			PAINTSTRUCT ps;
			RECT		rect;
			RECT		fRect;
			POINT		p = {0, 0};

			BeginPaint(hWnd, &ps);

			if (!have_history)
				ShowWindow(GetDlgItem(hWnd, IDC_HISTORY), SW_HIDE);

			ClientToScreen(hWnd, &p);
			GetWindowRect(GetDlgItem(hWnd, IDC_SSFRAME), &fRect);
			OffsetRect(&fRect, -p.x, -p.y);

			if (GetScreenShotRect(GetDlgItem(hWnd, IDC_SSFRAME), &rect, have_history && !nPictType))
			{
				DWORD dwStyle;
				DWORD dwStyleEx;

				dwStyle   = GetWindowLong(GetDlgItem(hWnd, IDC_SSPICTURE), GWL_STYLE);
				dwStyleEx = GetWindowLong(GetDlgItem(hWnd, IDC_SSPICTURE), GWL_EXSTYLE);

				AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);
				MoveWindow(GetDlgItem(hWnd, IDC_SSPICTURE),
						   fRect.left  + rect.left,
						   fRect.top   + rect.top,
						   rect.right  - rect.left,
						   rect.bottom - rect.top,
						   TRUE);
			}

			if (ScreenShotLoaded() && have_selection)
			{
				ShowWindow(GetDlgItem(hWnd, IDC_SSDEFPIC),	SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_SSPICTURE), SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_SSPICTURE), SW_SHOW);
			}
			else
			{
				ShowWindow(GetDlgItem(hWnd, IDC_SSDEFPIC),	SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_SSPICTURE), SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_SSDEFPIC),	SW_SHOW);
			}

			if (!nPictType && have_selection && have_history)
			{
				ShowWindow(GetDlgItem(hWnd, IDC_HISTORY), SW_HIDE);
				ShowWindow(GetDlgItem(hWnd, IDC_HISTORY), SW_SHOW);
			}

			/* Paint Screenshot Frame rect with the background image */
			PaintControl(GetDlgItem(hWnd, IDC_SSFRAME), TRUE);

			DrawScreenShot(GetDlgItem(hWnd, IDC_SSPICTURE));

			EndPaint(hWnd, &ps);
		}
		break;

	case WM_DESTROY:
        /* Free GDI resources */

		if (hBitmap)
		{
			DeleteObject(hBitmap);
			hBitmap = NULL;
		}

		if (hPALbg)
		{
			DeleteObject(hPALbg);
			hPALbg = NULL;
		}

		if (hFont)
		{
			DeleteObject(hFont);
			hFont = NULL;
		}

        PostQuitMessage(0);
		return 0;

#if defined(INTELLIMOUSE)
	case WM_MOUSEWHEEL:
		{
			/* WM_MOUSEWHEEL will be in Win98 */
			short zDelta = (short)HIWORD(wParam);

			if (zDelta < 0)
				SetSelectedPickItem(GetSelectedPick() + 1);
			else
				SetSelectedPickItem(GetSelectedPick() - 1);
			return 0;
		}
		break;
#endif

	default:

#if defined(INTELLIMOUSE)
		if (message == uiMsh_MsgMouseWheel)
		{
			int zDelta = (int)wParam; /* wheel rotation */

			if (zDelta < 0)
				SetSelectedPick(GetSelectedPick() + 1);
			else
				SetSelectedPick(GetSelectedPick() - 1);
			return 0;
		}
#endif
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOL PumpMessage()
{
	MSG msg;

	return PumpAndReturnMessage(&msg);
}

static BOOL PumpAndReturnMessage(MSG *pmsg)
{
	if (!(GetMessage(pmsg, NULL, 0, 0)))
		return FALSE;

	if (!Help_HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, (DWORD)pmsg))
	{
		if (IsWindow(hMain))
		{
			if (!TranslateAccelerator(hMain, hAccel, pmsg))
			{
				if (!IsDialogMessage(hMain, pmsg))
				{
					TranslateMessage(pmsg);
					DispatchMessage(pmsg);
				}
			}
		}
	}

	return TRUE;
}

static void EmptyQueue(void)
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!PumpMessage())
			ExitProcess(0);
	}
}

static void ColumnSort(int column, BOOL bColumn)
{
	LV_FINDINFO lvfi;
	int 		save_column = 0;
	int 		use_column = column;

	if (bColumn) /* Was a column enum used?? */
	{
		int i;

		for (i = 0; i < COLUMN_MAX; i++)
		{
			if (realColumn[i] == column)
			{
				use_column = i;
				break;
			}
		}
	}

	/* Sort 'em */
	save_column = realColumn[use_column] + 1;

	reverse_sort = (column == last_sort && !reverse_sort) ? TRUE : FALSE;
	ListView_SortItems(hwndList, ListCompareFunc, realColumn[use_column]);
	last_sort = use_column;

	if (reverse_sort)
		save_column = -(save_column);

	SetSortColumn(save_column);

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = GetSelectedPickItem();
	ListView_EnsureVisible(hwndList, ListView_FindItem(hwndList, -1, &lvfi), FALSE);
	Header_SetSortInfo(hwndList, use_column, !reverse_sort);
}

static BOOL IsGameRomless(int iGame)
{
	const struct RomModule *region, *rom;

	/* loop over regions, then over files */
	for (region = rom_first_region(drivers[iGame]); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			return FALSE;

	return TRUE;
}

static BOOL GameCheck(void)
{
	LV_FINDINFO lvfi;
	int 		i;
	BOOL		success;
	BOOL		changed = FALSE;

	if (game_index == 0)
		ProgressBarShow();

	if (game_count <= game_index)
	{
		bDoGameCheck = FALSE;
		ProgressBarHide();
		return FALSE;
	}

	if (GetHasRoms(game_index) == UNKNOWN)
	{
		if (IsGameRomless(game_index))
		{
			success = TRUE;
		}
		else
		{
			/* Only check for the ZIP file once */
			success = File_ExistZip(drivers[game_index]->name, OSD_FILETYPE_ROM);
		}

		if (!success)
			success = FindRomSet(game_index);

		SetHasRoms(game_index, success);
		changed = TRUE;
	}

	/* Check for SAMPLES */
	if (GetHasSamples(game_index) == UNKNOWN)
	{
		SetHasSamples(game_index, FindSampleSet(game_index));
		changed = TRUE;
	}

	lvfi.flags	= LVFI_PARAM;
	lvfi.lParam = game_index;

	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (changed && i != -1);
		ListView_RedrawItems(hwndList, i, i);
	if ((game_index % progBarStep) == 0)
		ProgressBarStep();
	game_index++;

	return changed;
}

static void OnIdle()
{
	LV_FINDINFO lvfi;
	int 		i;
	char*		pDescription;
	static int	bFirstTime = TRUE;
	static int	bResetList = TRUE;

	if (bFirstTime)
	{
		bResetList = FALSE;
		bFirstTime = FALSE;
	}
	if (bDoGameCheck)
	{
		bResetList |= GameCheck();
		return;
	}

	lvfi.flags = LVFI_STRING;
	lvfi.psz   = GetDefaultGame();
	i = ListView_FindItem(hwndList, -1, &lvfi);

	SetSelectedPick((i != -1) ? i : 0);
	i = GetSelectedPickItem();
	pDescription = ModifyThe(drivers[i]->description);
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)pDescription);
	if (bResetList || (GetViewMode() == VIEW_LARGE_ICONS))
	{
		InitGames(game_count);
		ResetListView();
	}
	idle_work = FALSE;
	UpdateStatusBar();
	bFirstTime = TRUE;
}

static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static BOOL firstTime = TRUE;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (!firstTime)
		OnSizeSplitter(hMain);
	else
		firstTime = FALSE;
	/* Update the splitters structures as appropriate */
	RecalcSplitters();
}

static void ResizeWindow(HWND hParent, Resize *r)
{
	int cmkindex = 0, dx, dy;
	HWND hControl;
	RECT parent_rect, rect;
	ResizeItem *ri;
	POINT p = {0, 0};

	if (hParent == NULL)
		return;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	dx = parent_rect.right	- r->rect.right;
	dy = parent_rect.bottom - r->rect.bottom;
	ClientToScreen(hParent, &p);

	while (r->items[cmkindex].type != RA_END)
	{
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;

		MoveWindow(hControl, rect.left, rect.top,
				   (rect.right - rect.left),
				   (rect.bottom - rect.top), TRUE);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow != NULL)
			ResizeWindow(hControl, ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}

static void ProgressBarShow()
{
	RECT rect;
	int  widths[2] = {150, -1};

	if (game_count < 100)
		progBarStep = 100 / game_count;
	else
		progBarStep = (game_count / 100);

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, game_count));
	SendMessage(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);

	StatusBar_GetItemRect(hStatusBar, 1, &rect);

	MoveWindow(hProgWnd, rect.left, rect.top,
			   rect.right - rect.left,
			   rect.bottom - rect.top, TRUE);

	bProgressShown = TRUE;
}

static void ProgressBarHide()
{
	RECT rect;
	int  widths[4];
	HDC  hDC;
	SIZE size;
	int  numParts = 4;

	if (hProgWnd == NULL)
	{
		return;
	}

    hDC = GetDC(hProgWnd);

	ShowWindow(hProgWnd, SW_HIDE);

	GetTextExtentPoint32(hDC, "MMX", 3 , &size);
	widths[3] = size.cx;
	GetTextExtentPoint32(hDC, "XXXX games", 10, &size);
	widths[2] = size.cx;
	GetTextExtentPoint32(hDC, "Imperfect Colors", 16, &size);
	widths[1] = size.cx;

	ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
	UpdateStatusBar();

	bProgressShown = FALSE;
}

static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top, TRUE);
	}
	else
	{
		ProgressBarHide();
	}
}

static void ProgressBarStep()
{
	char tmp[80];
	sprintf(tmp, "Game search %d%% complete",
			((game_index + 1) * 100) / game_count);
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)tmp);
	if (game_index == 0)
		ShowWindow(hProgWnd, SW_SHOW);
	SendMessage(hProgWnd, PBM_STEPIT, 0, 0);
}

static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return CreateWindowEx(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			"Progress Bar",
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right	- rect.left,
			rect.bottom - rect.top,
			hParent,
			NULL,
			hInst,
			NULL);
}

static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int   i;
	int   iButton = lpttt->hdr.idFrom;
	LPSTR pString;
	LPSTR pDest = lpttt->lpszText;

	/* Map command ID to string index */
	for (i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			break;
		}
	}

	/* Check for valid parameter */
	pString = (iButton > NUM_TOOLTIPS) ? "Invalid Button Index" : szTbStrings[iButton];

	lstrcpy(pDest, pString);
}

static HWND InitToolbar(HWND hParent)
{
	return CreateToolbarEx(hParent,
						   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						   CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						   1,
						   8,
						   hInst,
						   IDB_TOOLBAR,
						   tbb,
						   NUM_TOOLBUTTONS,
						   16,
						   16,
						   0,
						   0,
						   sizeof(TBBUTTON));
}

static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_UI_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
							  CCS_BOTTOM | SBARS_SIZEGRIP,
							  "Ready",
							  hParent,
							  2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags	= (UINT)HIWORD(wParam);
	HMENU hMainMenu = NULL;
	int   iMenu 	= 0;

	/* Handle non-system popup menu descriptions. */
	if (  (fuFlags & MF_POPUP)
	&&	(!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		/* Display helpful text in status bar */
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst,
				 hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst,
				 hStatusBar, &nZero);
	}

	return 0;
}

static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int 		 games_shown = 0;
	char		 game_text[20];
	int 		 i = -1;

	if (!lpFolder)
		return;

	do
	{
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (!GameFiltered(i, lpFolder->m_dwFlags))
				games_shown++;
		}
	}
	while (i != -1);

	/* Show number of games in the current 'View' in the status bar */
	sprintf(game_text, "%d games", games_shown);
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)2, (LPARAM)game_text);

	i = GetSelectedPickItem();

	if (games_shown == 0)
		DisableSelection();
	else
	{
		char* pStatus = GameInfoStatus(i);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1, (LPARAM)pStatus);
	}
}

static void UpdateHistory(int gameNum)
{
	int show = (have_history && have_selection) ? SW_SHOW : SW_HIDE;

	if (gameNum != -1)
	{
		char *histText = GameHistory(gameNum);

		have_history = (histText && histText[0]) ? TRUE : FALSE;
		Edit_SetText(GetDlgItem(hPicker, IDC_HISTORY), histText);
	}
	if (GetShowScreenShot())
	{
		show = (!nPictType && have_selection) ? show : SW_HIDE;
		ShowWindow(GetDlgItem(hPicker, IDC_HISTORY), show);
	}
}

static void DisableSelection()
{
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);
	BOOL			last_selection = have_selection;;

	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = "&Play";
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES,	   MF_GRAYED);

	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)"No Selection");
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1, (LPARAM)"");
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)3, (LPARAM)"");

	have_selection = FALSE;

	if (last_selection != have_selection)
		UpdateScreenShot();
}

static void EnableSelection(int nGame)
{
	char			buf[200];
	char*			pText;
	MENUITEMINFO	mmi;
	HMENU			hMenu = GetMenu(hMain);

	sprintf(buf, "&Play %s", ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));
	mmi.cbSize	   = sizeof(mmi);
	mmi.fMask	   = MIIM_TYPE;
	mmi.fType	   = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch 	   = strlen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	pText = ModifyThe(drivers[nGame]->description);
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)pText);
	/* Add this game's status to the status bar */
	pText = GameInfoStatus(nGame);
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1, (LPARAM)pText);
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)3, (LPARAM)"");

	/* If doing updating game status and the game name is NOT pacman.... */

	EnableMenuItem(hMenu, ID_FILE_PLAY, 		   MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,	   MF_ENABLED);

	if (!oldControl)
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	if (GetIsFavorite(nGame))
	{
		EnableMenuItem(hMenu, ID_CONTEXT_ADD_FAVORITE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_DEL_FAVORITE, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_CONTEXT_ADD_FAVORITE, MF_ENABLED);
		EnableMenuItem(hMenu, ID_CONTEXT_DEL_FAVORITE, MF_GRAYED);
	}

	if (bProgressShown && bListReady == TRUE)
	{
		SetDefaultGame(ModifyThe(drivers[nGame]->description));
	}
	have_selection = TRUE;

	UpdateScreenShot();
}

static void PaintControl(HWND hWnd, BOOL useBitmap)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;
	HDC 		hDC = GetDC(hWnd);

	/* So we don't paint over the controls border */
	GetClientRect(hWnd, &rcClient);
	InflateRect(&rcClient, -2, -2);

	if (hBitmap && useBitmap)
	{
		int 		i, j;
		HDC 		htempDC;
		HBITMAP 	oldBitmap;

		htempDC = CreateCompatibleDC(hDC);
		oldBitmap = SelectObject(htempDC, hBitmap);

		rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);

		hPAL = (!hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(htempDC, hPAL, FALSE);
			RealizePalette(htempDC);
		}

		for (i = rcClient.left; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = rcClient.top; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

		SelectObject(htempDC, oldBitmap);
		DeleteDC(htempDC);

		if (! bmDesc.bmColors)
		{
			DeleteObject(hPAL);
			hPAL = 0;
		}
	}
	else	/* Use standard control face color */
	{
		HBRUSH	hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		HBRUSH	oldBrush;

		oldBrush = SelectObject(hDC, hBrush);
		FillRect(hDC, &rcClient, hBrush);
		SelectObject(hDC, oldBrush);
		DeleteObject(hBrush);
	}
	ReleaseDC(hWnd, hDC);
}

static char* TriStateToText(int nState)
{
	if (nState == TRUE)
		return "Yes";

	if (nState == FALSE)
		return "No";

	return "?";
}

static LPCSTR GetCloneParent(int nItem)
{
	if (!(drivers[nItem]->clone_of->flags & NOT_A_DRIVER))
		return ModifyThe(drivers[nItem]->clone_of->description);
	return "";
}

static BOOL PickerHitTest(HWND hWnd)
{
	RECT			rect;
	POINTS			p;
	DWORD			res = GetMessagePos();
	LVHITTESTINFO	htInfo;

	memset(&htInfo, '\0', sizeof(LVHITTESTINFO));
	p = MAKEPOINTS(res);
	GetWindowRect(hWnd, &rect);
	htInfo.pt.x = p.x - rect.left;
	htInfo.pt.y = p.y - rect.top;
	ListView_HitTest(hWnd, &htInfo);

	return (! (htInfo.flags & LVHT_NOWHERE));
}

static BOOL MamePickerNotify(NMHDR *nm)
{
	NM_LISTVIEW *pnmv;
	static int nLastState = -1;
	static int nLastItem  = -1;

	switch (nm->code)
	{
	case NM_RCLICK:
	case NM_CLICK:
		/* don't allow selection of blank spaces in the listview */
		if (!PickerHitTest(hwndList))
		{
			/* we have no current game selected */
			if (nLastItem != -1);
				SetSelectedPickItem(nLastItem);
			return TRUE;
		}
		break;

	case NM_DBLCLK:
		/* Check here to make sure an item was selected */
		if (!PickerHitTest(hwndList))
		{
			if (nLastItem != -1);
				SetSelectedPickItem(nLastItem);
			return TRUE;
		}
		else
			MamePlayGame();
		return TRUE;

	case LVN_GETDISPINFO:
		{
			LV_DISPINFO* pDispInfo = (LV_DISPINFO*)nm;
			int nItem = pDispInfo->item.lParam;

			if (pDispInfo->item.mask & LVIF_IMAGE)
			{
			   pDispInfo->item.iImage = WhichIcon(nItem);
			}

			if (pDispInfo->item.mask & LVIF_STATE)
				pDispInfo->item.state = 0;

			if (pDispInfo->item.mask & LVIF_TEXT)
			{
				switch (realColumn[pDispInfo->item.iSubItem])
				{
				case COLUMN_GAMES:
					/* Driver description */
					pDispInfo->item.pszText = (char *)ModifyThe(drivers[nItem]->description);
					break;

				case COLUMN_ROMS:
					/* Has Roms */
					pDispInfo->item.pszText = TriStateToText(GetHasRoms(nItem));
					break;

				case COLUMN_SAMPLES:
					/* Samples */
					if (GameUsesSamples(nItem))
					{
						pDispInfo->item.pszText = TriStateToText(GetHasSamples(nItem));
					}
					else
					{
						pDispInfo->item.pszText = "";
					}
					break;

				case COLUMN_DIRECTORY:
					/* Driver name (directory) */
					pDispInfo->item.pszText = (char*)drivers[nItem]->name;
					break;

				case COLUMN_TYPE:
                {
                    struct InternalMachineDriver drv;
                    expand_machine_driver(drivers[nItem]->drv,&drv);

					/* Vector/Raster */
					if (drv.video_attributes & VIDEO_TYPE_VECTOR)
						pDispInfo->item.pszText = "Vector";
					else
						pDispInfo->item.pszText = "Raster";
					break;
                }
				case COLUMN_TRACKBALL:
					/* Trackball */
					if (GameUsesTrackball(nItem))
						pDispInfo->item.pszText = "Yes";
					else
						pDispInfo->item.pszText = "No";
					break;

				case COLUMN_PLAYED:
					/* times played */
					{
						static char buf[100];
						sprintf(buf, "%i", GetPlayCount(nItem));
						pDispInfo->item.pszText = buf;
					}
					break;

				case COLUMN_MANUFACTURER:
					/* Manufacturer */
					pDispInfo->item.pszText = (char *)drivers[nItem]->manufacturer;
					break;

				case COLUMN_YEAR:
					/* Year */
					pDispInfo->item.pszText = (char *)drivers[nItem]->year;
					break;

				case COLUMN_CLONE:
					pDispInfo->item.pszText = (char *)GetCloneParent(nItem);
					break;

				}
			}
		}
		return TRUE;

	case LVN_ITEMCHANGED :
		pnmv = (NM_LISTVIEW *)nm;

		if ( (pnmv->uOldState & LVIS_SELECTED)
		&&	!(pnmv->uNewState & LVIS_SELECTED))
		{
			if (pnmv->lParam != -1)
				nLastItem = pnmv->lParam;
			/* leaving item */
			/* printf("leaving %s\n",drivers[pnmv->lParam]->name); */
		}

		if (!(pnmv->uOldState & LVIS_SELECTED)
		&&	 (pnmv->uNewState & LVIS_SELECTED))
		{
			int  nState;

			/* We load the screen shot DIB here, instead
			 * of loading everytime we want to display it.
			 */
			nState = LoadScreenShot(pnmv->lParam, nPictType);
			bScreenShotAvailable = nState;
			if (nState == TRUE || nState != nLastState)
			{
				HWND hWnd;

				if (GetShowScreenShot()
				&&	(hWnd = GetDlgItem(hPicker, IDC_SSFRAME)))
				{
					RECT	rect;
					HWND	hParent;
					POINT	p = {0, 0};

					hParent = GetParent(hWnd);

					GetWindowRect(hWnd,&rect);
					ClientToScreen(hParent, &p);
					OffsetRect(&rect, -p.x, -p.y);
					InvalidateRect(hParent, &rect, FALSE);
					UpdateWindow(hParent);
					nLastState = nState;
				}
			}

			/* printf("entering %s\n",drivers[pnmv->lParam]->name); */
			if (g_bDoBroadcast == TRUE)
			{
				ATOM a = GlobalAddAtom(drivers[pnmv->lParam]->description);
				SendMessage(HWND_BROADCAST, g_mame32_message, a, a);
				GlobalDeleteAtom(a);
			}

			EnableSelection(pnmv->lParam);
		}
		return TRUE;

	case LVN_COLUMNCLICK :
		pnmv = (NM_LISTVIEW *)nm;
		ColumnSort(pnmv->iSubItem, FALSE);
		return TRUE;
	}
	return FALSE;
}

static BOOL TreeViewNotify(LPNMHDR nm)
{
	LPNMTREEVIEW lpNmtv;

	switch (nm->code)
	{
	case TVN_SELCHANGED:
		lpNmtv = (LPNMTREEVIEW)nm;
		{
			HTREEITEM hti = TreeView_GetSelection(hTreeView);
			TVITEM	  tvi;

			tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
			tvi.hItem = hti;

			if (TreeView_GetItem( hTreeView, &tvi))
			{
				SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
				if (bListReady)
					ResetListView();
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static BOOL HeaderOnContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	/* Right button was clicked on header */
	POINT	pt;
	RECT	rcCol;
	int 	cmkindex;
	int 	i;
	BOOL	found = FALSE;
	HWND	hwndHeader;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_LIST))
		return FALSE;

	hwndHeader = GetDlgItem(hwndList, 0);

	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	ScreenToClient(hwndHeader, &pt);

	/* Determine the column index */
	for (i = 0; Header_GetItemRect(hwndHeader, i, &rcCol); i++)
	{
		if (PtInRect(&rcCol, pt))
		{
			cmkindex = i;
			found = TRUE;
			break;
		}
	}

	/* Do the context menu */
	if (found)
	{
		hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
		hMenu = GetSubMenu(hMenuLoad, 0);
		lastColumnClick = i;
		TrackPopupMenu(hMenu,
					   TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					   LOWORD(lParam),
					   HIWORD(lParam),
					   0,
					   hWnd,
					   NULL);

		DestroyMenu(hMenuLoad);

		return TRUE;
	}
	return FALSE;
}

char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
	   for setting text of window controls that don't allow us to disable
	   the ampersand underlining.
	 */
	/* returns a static buffer--use before calling again */

	static char buf[200];
	char *ptr;

	ptr = buf;
	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;
			*ptr++ = *s++;
	}
	*ptr = 0;

	return buf;
}

static void PollGUIJoystick()
{
	if (in_emulation)
		return;

	if (g_pJoyGUI == NULL)
		return;

	g_pJoyGUI->poll_joysticks();

	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_AXIS, 2, JOYCODE_DIR_NEG)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_UP);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_AXIS, 2, JOYCODE_DIR_POS)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_DOWN);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_AXIS, 1, JOYCODE_DIR_NEG)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_LEFT);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_AXIS, 1, JOYCODE_DIR_POS)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_RIGHT);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_BTN, 1, JOYCODE_DIR_BTN)))
	{
		SetFocus(hwndList);
		MamePlayGame();
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_BTN, 2, JOYCODE_DIR_BTN)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_NEXT);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_BTN, 5, JOYCODE_DIR_BTN)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_PRIOR);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_BTN, 3, JOYCODE_DIR_BTN)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_END);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(1, JOYCODE_STICK_BTN, 6, JOYCODE_DIR_BTN)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_HOME);
	}

	if (g_pJoyGUI->is_joy_pressed(JOYCODE(2, JOYCODE_STICK_AXIS, 2, JOYCODE_DIR_NEG)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_PRIOR);
	}
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(2, JOYCODE_STICK_AXIS, 2, JOYCODE_DIR_POS)))
	{
		SetFocus(hwndList);
		PressKey(hwndList, VK_NEXT);
	}
}

static void PressKey(HWND hwnd, UINT vk)
{
	SendMessage(hwnd, WM_KEYDOWN, vk, 0);
	SendMessage(hwnd, WM_KEYUP,   vk, 0xc0000000);
}

static void SetView(int menu_id, int listview_style)
{
	/* first uncheck previous menu item */
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_ICON, ID_VIEW_DETAIL,
					   menu_id, MF_CHECKED);
	ToolBar_CheckButton(hToolBar, menu_id, MF_CHECKED);
	SetWindowLong(hwndList, GWL_STYLE,
				  (GetWindowLong(hwndList, GWL_STYLE) & ~LVS_TYPEMASK) | listview_style);

	current_view_id = menu_id;
	SetViewMode(menu_id - ID_VIEW_ICON);

	switch (menu_id)
	{
	case ID_VIEW_ICON:
		ResetListView();
		break;

	case ID_VIEW_SMALL_ICON:
		ListView_Arrange(hwndList, LVA_ALIGNTOP);
		break;
	}
}

static void ResetListView()
{
	RECT	rect;
	int 	i;
	int 	current_game;
	LV_ITEM lvi;
	int 	column;
	BOOL	no_selection = FALSE;
	LPTREEFOLDER lpFolder = GetCurrentFolder();

	if (!lpFolder)
		return;

	SetWindowRedraw(hwndList, FALSE);

	/* If the last folder was empty, no_selection is TRUE */
	if (have_selection == FALSE)
		no_selection = TRUE;

	current_game = GetSelectedPickItem();

	ListView_DeleteAllItems(hwndList);

	ListView_SetItemCount(hwndList, game_count);

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;

	i = -1;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem	 = i;
			lvi.iSubItem = 0;
			lvi.lParam	 = i;
			/* Do not set listview to LVS_SORTASCENDING or LVS_SORTDESCENDING */
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iImage	 = I_IMAGECALLBACK;
			ListView_InsertItem(hwndList, &lvi);
		}
	} while (i != -1);

	reverse_sort = FALSE;

	if ((column = GetSortColumn()) > 0)
	{
		last_sort = -1;
	}
	else
	{
		column = abs(column);
		last_sort = column - 1;
	}

	ColumnSort(column - 1, TRUE);

	/* If last folder was empty, select the first item in this folder */
	if (no_selection)
	{
		SetSelectedPick(0);
	}
	else
		SetSelectedPickItem(current_game);

	GetClientRect(hwndList, &rect);
	InvalidateRect(hwndList, &rect, TRUE);
	/* if (current_view_id == ID_VIEW_SMALL_ICON) */
		ListView_Arrange(hwndList, LVA_DEFAULT);/* LVA_ALIGNTOP); */

	UpdateStatusBar();

	SetWindowRedraw(hwndList, TRUE);
}

static void UpdateGameList()
{
	int i;

	for (i = 0; i < game_count; i++)
	{
		SetHasRoms(i, UNKNOWN);
		SetHasSamples(i, UNKNOWN);
	}

	idle_work	 = TRUE;
	bDoGameCheck = TRUE;
	game_index	 = 0;

	/* Let REFRESH also load new background if found */
	ReloadIcons(hwndList);
	LoadListBitmap();
	ResetListView();

	SetDefaultGame(ModifyThe(drivers[GetSelectedPickItem()]->description));
	/*SetSelectedPick(0);*/ /* To avoid flickering. */
}

static void PickFont(void)
{
	LOGFONT font;
	CHOOSEFONT cf;

	GetListFont(&font);
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.rgbColors   = GetListFontColor();
	cf.Flags	   = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		DeleteObject(hFont);
	hFont = CreateFontIndirect(&font);
	if (hFont != NULL)
	{
        HWND    hwndHeaderCtrl = NULL;
        HFONT   hHeaderCtrlFont = NULL;

		COLORREF textColor = cf.rgbColors;

		if (textColor == RGB(255,255,255))
        {
			textColor = RGB(240, 240, 240);
        }

        hwndHeaderCtrl = GetDlgItem( hwndList, 0 );
        if ( hwndHeaderCtrl )
        {
            hHeaderCtrlFont = GetWindowFont( hwndHeaderCtrl );
        }

		SetWindowFont(hwndList, hFont, TRUE);
		SetWindowFont(hTreeView, hFont, TRUE);

        /* Reset header ctrl font back to original font */

        if ( hHeaderCtrlFont )
        {
            SetWindowFont( hwndHeaderCtrl, hHeaderCtrlFont, TRUE );
        }

		ListView_SetTextColor(hwndList, textColor);
		TreeView_SetTextColor(hTreeView,textColor);
		SetListFontColor(cf.rgbColors);
		SetWindowFont(GetDlgItem(hPicker, IDC_HISTORY), hFont, FALSE);
		ResetListView();
	}
}

/* This centers a window on another window */
static BOOL CenterSubDialog(HWND hParent, HWND hWndCenter, BOOL in_client_coords)
{
	RECT rect, wRect;
	int x, y;

	if (in_client_coords)
	   GetClientRect(hParent, &rect);
	else
	   GetWindowRect(hParent, &rect);

	GetClientRect(hWndCenter, &wRect);

	x = (rect.left + rect.right) / 2 - (wRect.left + wRect.right) / 2;
	y = (rect.bottom + rect.top) / 2 - (wRect.bottom + wRect.top) / 2;

	/* map parent coordinates to child coordinates */
	return SetWindowPos(hWndCenter, NULL, x, y, -1, -1,
						SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static BOOL MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD:
		MamePlayRecordGame();
		return TRUE;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
		return TRUE;

	case ID_FILE_AUDIT:
		AuditDialog(hMain);
		InitGames(game_count);
		ResetListView();
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_EXIT:
		PostMessage(hMain, WM_CLOSE, 0, 0);
		return TRUE;

	case ID_VIEW_ICON:
		SetView(ID_VIEW_ICON, LVS_ICON);
		return TRUE;

	case ID_VIEW_DETAIL:
		SetView(ID_VIEW_DETAIL, LVS_REPORT);
		return TRUE;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON, LVS_SMALLICON);
		return TRUE;

	case ID_VIEW_LIST_MENU:
		SetView(ID_VIEW_LIST_MENU, LVS_LIST);
		return TRUE;

	/* Arrange Icons submenu */
	case ID_VIEW_BYGAME:
		ColumnSort(COLUMN_GAMES, TRUE);
		break;

	case ID_VIEW_BYDIRECTORY:
		ColumnSort(COLUMN_DIRECTORY, TRUE);
		break;

	case ID_VIEW_BYMANUFACTURER:
		ColumnSort(COLUMN_MANUFACTURER, TRUE);
		break;

	case ID_VIEW_BYTIMESPLAYED:
		ColumnSort(COLUMN_PLAYED, TRUE);
		break;

	case ID_VIEW_BYTYPE:
		ColumnSort(COLUMN_TYPE, TRUE);
		break;

	case ID_VIEW_BYYEAR:
		ColumnSort(COLUMN_YEAR, TRUE);
		break;

	case ID_VIEW_FOLDERS:
		bShowTree = !bShowTree;
		SetShowFolderList(bShowTree);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hPicker);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hPicker);
		UpdateScreenShot();
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (!oldControl)
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItem(), AUDIT_PAGE);
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
	   break;

	/* ListView Context Menu */
	case ID_CONTEXT_ADD_FAVORITE:
	case ID_CONTEXT_DEL_FAVORITE:
		SetIsFavorite(GetSelectedPickItem(), (id == ID_CONTEXT_ADD_FAVORITE) ? TRUE : FALSE);
		InitGames(game_count);
		if (GetFolderByID(FOLDER_FAVORITE) == GetCurrentFolder())
			ResetListView();
		break;

	/* Tree Context Menu */
	case ID_CONTEXT_FILTERS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == TRUE)
			ResetListView();
		SetFocus(hwndList);
		return TRUE;

	/* Header Context Menu */
	case ID_SORT_ASCENDING:
		last_sort = -1;
		ColumnSort(lastColumnClick, FALSE);
		break;

	case ID_SORT_DESCENDING:
		last_sort = lastColumnClick;
		ColumnSort(lastColumnClick, FALSE);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == TRUE)
			ResetColumnDisplay(FALSE);
		SetFocus(hwndList);
		return TRUE;

	/* View Menu */
	case ID_VIEW_LINEUPICONS:
		ResetListView();
		break;

	case ID_GAME_PROPERTIES:
		if (!oldControl)
		{
			InitPropertyPage(hInst, hwnd, GetSelectedPickItem());
			SaveGameOptions(GetSelectedPickItem());
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_VIEW_SCREEN_SHOT:
		ToggleScreenShot();
		if (current_view_id == ID_VIEW_ICON)
			ResetListView();
		break;

	case ID_UPDATE_GAMELIST:
		UpdateGameList();
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_DEFAULTS:
		/* Check the return value to see if changes were applied */
		if (!oldControl)
		{
			InitDefaultPropertyPage(hInst, hwnd);
			SaveDefaultOptions();
		}
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_DIR:
		{
			int  nResult;
			BOOL bUpdateRoms;
			BOOL bUpdateSamples;

			nResult = DialogBox(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_DIRECTORIES),
								hMain,
								DirectoriesDialogProc);

			bUpdateRoms    = ((nResult & DIRDLG_ROMS)	 == DIRDLG_ROMS)	? TRUE : FALSE;
			bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? TRUE : FALSE;

			/* update file code */
			File_UpdatePaths();

			/* update game list */
			if (bUpdateRoms == TRUE || bUpdateSamples == TRUE)
				UpdateGameList();

			SetFocus(hwndList);
		}
		return TRUE;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (DialogBox(GetModuleHandle(NULL),
					  MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == TRUE)
        {
            DestroyWindow( hwnd );
			PostQuitMessage(0);
        }
		return TRUE;

	case ID_OPTIONS_STARTUP:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_START_OPTIONS),
				  hMain, StartupDialogProc);
		return TRUE;

	case ID_OPTIONS_LANGUAGE:
		DialogBox(GetModuleHandle(NULL),
				  MAKEINTRESOURCE(IDD_LANGUAGE),
				  hMain,
				  LanguageDialogProc);
		return TRUE;

	case ID_HELP_CONTENTS:
//		Help_HtmlHelp(hMain, MAME32HELP, HH_DISPLAY_TOC, 0);
		Help_HtmlHelp(hMain, MAME32HELP "::/html/mame32%20overview.htm", HH_DISPLAY_TOPIC, 0);
		break;

	case ID_HELP_WHATS_NEW32:
//		Help_HtmlHelp(hMain, MAME32HELP, HH_HELP_CONTEXT, 100);
		Help_HtmlHelp(hMain, MAME32HELP "::/html/mame32%20what's%20new.htm", HH_DISPLAY_TOPIC, 0);
		break;

//	case ID_HELP_QUICKSTART:
//		Help_HtmlHelp(hMain, MAME32HELP, HH_HELP_CONTEXT, 101);
//		Help_HtmlHelp(hMain, MAME32HELP "::/html/mame32%20windows%20install-setup.htm", HH_DISPLAY_TOPIC, 0);
//		break;

	case ID_HELP_SUPPORT:
		DisplayTextFile(hMain, HELPTEXT_SUPPORT);
		break;

	case ID_HELP_RELEASE:
		DisplayTextFile(hMain, HELPTEXT_RELEASE);
		break;

	case ID_HELP_WHATS_NEW:
		DisplayTextFile(hMain, HELPTEXT_WHATS_NEW);
		break;

	case ID_HELP_ABOUT:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT),
				  hMain, AboutDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case IDC_PLAY_GAME:
		if (have_selection)
			MamePlayGame();
		break;

	case IDC_SSPICTURE:
	case IDC_SSDEFPIC:
		nPictType = (nPictType + 1) % MAX_PICT_TYPES;
		SetShowPictType(nPictType);
		bScreenShotAvailable = LoadScreenShot(GetSelectedPickItem(), nPictType);
		UpdateScreenShot();
		SendMessage(hPicker, WM_PAINT, 0, 0);
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;
	}

	SetFocus(hwndList);
	return FALSE;
}

static void LoadListBitmap()
{
	HGLOBAL hDIBbg;
	char*	pFileName = 0;

	if (hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = 0;
	}

	if (hPALbg)
	{
		DeleteObject(hPALbg);
		hPALbg = 0;
	}

	/* Pick images based on number of colors avaliable. */
	if (GetDepth(hwndList) <= 8)
	{
		pFileName = "bkgnd16";
		/*nResource = IDB_BKGROUND16;*/
	}
	else
	{
		pFileName = "bkground";
		/*nResource = IDB_BKGROUND;*/
	}

	if (LoadDIB(pFileName, &hDIBbg, &hPALbg, FALSE))
	{
		HDC hDC = GetDC(hwndList);
		hBitmap = DIBToDDB(hDC, hDIBbg, &bmDesc);
		GlobalFree(hDIBbg);
		ReleaseDC(hwndList, hDC);
	}
#ifdef INTERNAL_BKBITMAP
	else
		hBitmap = LoadBitmapAndPalette(hInst, MAKEINTRESOURCE(nResource), &hPALbg, &bmDesc);
#endif
}

/* Initialize the Picker and List controls */
static void InitPicker()
{
	int order[COLUMN_MAX];
	int shown[COLUMN_MAX];
	int i;

	Header_Initialize(hwndList);

	/* Disabled column customize with old Control */
	if (oldControl)
	{
		for (i = 0; i < COLUMN_MAX; i++)
		{
			order[i] = i;
			shown[i] = TRUE;
		}
		SetColumnOrder(order);
		SetColumnShown(shown);
	}

	/* Create IconsList for ListView Control */
	CreateIcons(hwndList);
	GetColumnOrder(realColumn);

	ResetColumnDisplay(TRUE);

	/* Allow selection to change the default saved game */
	bListReady = TRUE;
}

/* Re/initialize the ListControl Columns */
static void ResetColumnDisplay(BOOL firstime)
{
	LV_FINDINFO lvfi;
	LV_COLUMN   lvc;
	int         i;
	int         nColumn;
	int         widths[COLUMN_MAX];
	int         order[COLUMN_MAX];
	int         shown[COLUMN_MAX];

	GetColumnWidths(widths);
	GetColumnOrder(order);
	GetColumnShown(shown);

	if (!firstime)
	{
		nColumn = GetNumColumns(hwndList);

		/* The first time thru this won't happen, on purpose */
		for (i = 0; i < nColumn; i++)
		{
			widths[realColumn[i]] = ListView_GetColumnWidth(hwndList, i);
		}

		SetColumnWidths(widths);

		for (i = nColumn; i > 0; i--)
		{
			ListView_DeleteColumn(hwndList, i - 1);
		}
	}

	ListView_SetExtendedListViewStyle(hwndList,
		LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	lvc.fmt = LVCFMT_LEFT;

	nColumn = 0;
	for (i = 0; i < COLUMN_MAX; i++)
	{
		if (shown[order[i]])
		{
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;

			lvc.mask	 |= LVCF_TEXT;
			lvc.pszText   = column_names[order[i]];
			lvc.iSubItem  = nColumn;
			lvc.cx		  = widths[order[i]];
			ListView_InsertColumn(hwndList, nColumn, &lvc);
			realColumn[nColumn] = order[i];
			nColumn++;
		}
	}

	/* Fill this in so we can still sort on columns NOT shown */
	for (i = 0; i < COLUMN_MAX && nColumn < COLUMN_MAX; i++)
	{
		if (!shown[order[i]])
		{
			realColumn[nColumn] = order[i];
			nColumn++;
		}
	}

	if (GetListFontColor() == RGB(255, 255, 255))
		ListView_SetTextColor(hwndList, RGB(240, 240, 240));
	else
		ListView_SetTextColor(hwndList, GetListFontColor());

	ResetListView();

	lvfi.flags = LVFI_STRING;
	lvfi.psz = GetDefaultGame();
	i = ListView_FindItem(hwndList, -1, &lvfi);
	SetSelectedPick(i);
}

static void AddIcon(int cmkindex)
{
	HICON hIcon = 0;

	if (icon_index[cmkindex] != 0)
		return;

	if ((hIcon = LoadIconFromFile((char *)drivers[cmkindex]->name)) == 0)
	{
		if (drivers[cmkindex]->clone_of != 0)
		{
			hIcon = LoadIconFromFile((char *)drivers[cmkindex]->clone_of->name);
			if (!hIcon && drivers[cmkindex]->clone_of->clone_of)
				hIcon = LoadIconFromFile((char *)drivers[cmkindex]->clone_of->clone_of->name);
		}
	}

	if (hIcon)
	{
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		if (nIconPos != -1)
			icon_index[cmkindex] = nIconPos;
	}
	if (icon_index[cmkindex] == 0)
		icon_index[cmkindex] = 1;
}

static void ReloadIcons(HWND hWnd)
{
	hSmall = ListView_GetImageList(hWnd, LVSIL_SMALL);
	if (hSmall != NULL)
		ImageList_Destroy(hSmall);

	hLarge = ListView_GetImageList(hWnd, LVSIL_NORMAL);
	if (hLarge != NULL)
		ImageList_Destroy(hLarge);

	hSmall = 0;
	hLarge = 0;

	CreateIcons(hWnd);
}

static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize, dwLength = 512, dwType = REG_SZ;
	TCHAR  szBuffer[512];
	HKEY   hKey;

	/* Get the Key */
	RegOpenKey(HKEY_CURRENT_USER, "Control Panel\\desktop\\WindowMetrics", &hKey);
	/* Save the last size */
	RegQueryValueEx(hKey, "Shell Icon Size", NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	dwSize = atol(szBuffer);

	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}

/* create iconlist and Listview control */
static BOOL CreateIcons(HWND hWnd)
{
	HICON	hIcon;
	INT 	i;
	DWORD	dwLargeIconSize = GetShellLargeIconSize();

	if (!icon_index)
		icon_index = malloc(sizeof(int) * game_count);

	if (icon_index != NULL)
	{
		for (i = 0; i < game_count; i++)
			icon_index[i] = 0;
	}

	hSmall = ImageList_Create(ICONMAP_WIDTH, ICONMAP_HEIGHT,
		ILC_COLORDDB | ILC_MASK, NUM_ICONS, NUM_ICONS + game_count);

	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize,
		ILC_COLORDDB | ILC_MASK, NUM_ICONS, NUM_ICONS + game_count);

	for (i = IDI_WIN_NOROMS; i <= IDI_WIN_REDX; i++)
	{
		if ((hIcon = LoadIconFromFile(icon_names[i - IDI_WIN_NOROMS])) == 0)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(i));

		if ((ImageList_AddIcon(hSmall, hIcon) == -1)
		||	(ImageList_AddIcon(hLarge, hIcon) == -1))
			return FALSE;
	}

	/* Be sure that all the small icons were added. */
	if (ImageList_GetImageCount(hSmall) < NUM_ICONS)
		return FALSE;

	/* Be sure that all the large icons were added. */
	if (ImageList_GetImageCount(hLarge) < NUM_ICONS)
		return FALSE;

	/* Associate the image lists with the list view control. */
	ListView_SetImageList(hWnd, hSmall, LVSIL_SMALL);

	ListView_SetImageList(hWnd, hLarge, LVSIL_NORMAL);

	return TRUE;
}


/* Sort subroutine to sort the list control */
static int CALLBACK ListCompareFunc(LPARAM index1, LPARAM index2, int sort_subitem)
{
	int value;
	const char *name1 = NULL;
	const char *name2 = NULL;
	int   nTemp1, nTemp2;

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
		value = stricmp(ModifyThe(drivers[index1]->description),
						ModifyThe(drivers[index2]->description));
		break;

	case COLUMN_ROMS:
		nTemp1 = GetHasRoms(index1);
		nTemp2 = GetHasRoms(index2);
		if (nTemp1 == nTemp2)
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		if (nTemp1 == TRUE || nTemp2 == FALSE)
			value = -1;
		else
			value = 1;
		break;

	case COLUMN_SAMPLES:
		if ((nTemp1 = GetHasSamples(index1)) == TRUE)
			nTemp1++;

		if ((nTemp2 = GetHasSamples(index2)) == TRUE)
			nTemp2++;

		if (GameUsesSamples(index1) == FALSE)
			nTemp1 -= 1;

		if (GameUsesSamples(index2) == FALSE)
			nTemp2 -= 1;

		if (nTemp1 == nTemp2)
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		value = nTemp2 - nTemp1;
		break;

	case COLUMN_DIRECTORY:
		value = stricmp(drivers[index1]->name, drivers[index2]->name);
		break;

	case COLUMN_TYPE:
    {
        struct InternalMachineDriver drv1,drv2;
        expand_machine_driver(drivers[index1]->drv,&drv1);
        expand_machine_driver(drivers[index2]->drv,&drv2);

		if ((drv1.video_attributes & VIDEO_TYPE_VECTOR) ==
			(drv2.video_attributes & VIDEO_TYPE_VECTOR))
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		value = (drv1.video_attributes & VIDEO_TYPE_VECTOR) -
				(drv2.video_attributes & VIDEO_TYPE_VECTOR);
		break;
    }
	case COLUMN_TRACKBALL:
		if (GameUsesTrackball(index1) == GameUsesTrackball(index2))
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		value = GameUsesTrackball(index1) - GameUsesTrackball(index2);
		break;

	case COLUMN_PLAYED:
	   value = GetPlayCount(index1) - GetPlayCount(index2);
	   if (value == 0)
		  return ListCompareFunc(index1, index2, COLUMN_GAMES);

	   break;

	case COLUMN_MANUFACTURER:
		if (stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer) == 0)
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		value = stricmp(drivers[index1]->manufacturer, drivers[index2]->manufacturer);
		break;

	case COLUMN_YEAR:
		if (stricmp(drivers[index1]->year, drivers[index2]->year) == 0)
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		value = stricmp(drivers[index1]->year, drivers[index2]->year);
		break;

	case COLUMN_CLONE:
		name1 = GetCloneParent(index1);
		name2 = GetCloneParent(index2);

		if (*name1 == '\0')
			name1 = NULL;
		if (*name2 == '\0')
			name2 = NULL;

		if (name1 == name2)
			return ListCompareFunc(index1, index2, COLUMN_GAMES);

		if (name2 == NULL)
			value = -1;
		else if (name1 == NULL)
			value = 1;
		else
			value = stricmp(name1, name2);
		break;

	default :
		return ListCompareFunc(index1, index2, COLUMN_GAMES);
	}

	if (reverse_sort && value != 0)
		value = -(value);

	return value;
}

static int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}

static int GetSelectedPickItem()
{
	/*r eturns lParam (game index) of listview selected item */
	LV_ITEM lvi;

	lvi.iItem = GetSelectedPick();
	if (lvi.iItem == -1)
		return 0;

	lvi.iSubItem = 0;
	lvi.mask	 = LVIF_PARAM;
	ListView_GetItem(hwndList, &lvi);

	return lvi.lParam;
}

static void SetSelectedPick(int new_index)
{
	if (new_index < 0)
		new_index = 0;

	ListView_SetItemState(hwndList, new_index, LVIS_FOCUSED | LVIS_SELECTED,
						  LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hwndList, new_index, FALSE);
}

static void SetSelectedPickItem(int val)
{
	int i;
	LV_FINDINFO lvfi;

	if (val < 0)
		return;

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = val;
	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (i == -1)
	{
		POINT p = {0,0};
		lvfi.flags = LVFI_NEARESTXY;
		lvfi.pt = p;
		i = ListView_FindItem(hwndList, -1, &lvfi);
	}
	SetSelectedPick((i == -1) ? 0 : i);
}

static void SetRandomPickItem()
{
	int nListCount;

	nListCount = ListView_GetItemCount(hwndList);

	if (nListCount > 0)
	{
		SetSelectedPick(rand() % nListCount);
	}
}

static INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			HBITMAP hBmp;
			hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL),
									  MAKEINTRESOURCE(IDB_ABOUT),
									  IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_ABOUT), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
			Static_SetText(GetDlgItem(hDlg, IDC_VERSION), GetVersionString());
		}
		return 1;

	case WM_COMMAND:
		EndDialog(hDlg, 0);
		return 1;
	}
	return 0;
}


static INT_PTR CALLBACK DirectXDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hEdit;

	char *directx_help =
		MAME32NAME " requires DirectX version 3 or later, which is a set of operating\r\n"
		"system extensions by Microsoft for Windows 9x, NT and 2000.\r\n\r\n"
		"Visit Microsoft's DirectX web page at http://www.microsoft.com/directx\r\n"
		"download DirectX, install it, and then run " MAME32NAME " again.\r\n";

	switch (Msg)
	{
	case WM_INITDIALOG:
		hEdit = GetDlgItem(hDlg, IDC_DIRECTX_HELP);
		Edit_SetSel(hEdit, Edit_GetTextLength(hEdit), Edit_GetTextLength(hEdit));
		Edit_ReplaceSel(hEdit, directx_help);
		return 1;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDB_WEB_PAGE)
			ShellExecute(hMain, NULL, "http://www.microsoft.com/directx", NULL, NULL, SW_SHOWNORMAL);

		if (LOWORD(wParam) == IDCANCEL
		||	LOWORD(wParam) == IDB_WEB_PAGE)
			EndDialog(hDlg, 0);
		return 1;
	}
	return 0;
}

static BOOL CommonFileDialog(common_file_dialog_proc cfd, char *filename, BOOL bZip)
{
	BOOL success;

	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hMain;
	of.hInstance         = NULL;
	if (bZip == TRUE)
		of.lpstrFilter   = MAMENAME " input files (*.inp,*.zip)\0*.inp;*.zip\0All files (*.*)\0*.*\0";
	else
		of.lpstrFilter   = MAMENAME " input files (*.inp)\0*.inp;\0All files (*.*)\0*.*\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	of.lpstrInitialDir   = last_directory;
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	of.lpstrDefExt       = "inp";
	of.lpstrDefExt       = NULL;
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	success = cfd(&of);
	if (success)
	{
		/*GetDirectory(filename,last_directory,sizeof(last_directory));*/
	}

	return success;
}

static BOOL SelectLanguageFile(HWND hWnd, TCHAR* filename)
{
	OPENFILENAME of;

	of.lStructSize       = sizeof(of);
	of.hwndOwner         = hWnd;
	of.hInstance         = NULL;
	of.lpstrFilter       = MAMENAME " Language files (*.lng)\0*.lng\0";
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter    = 0;
	of.nFilterIndex      = 1;
	of.lpstrFile         = filename;
	of.nMaxFile          = MAX_PATH;
	of.lpstrFileTitle    = NULL;
	of.nMaxFileTitle     = 0;
	of.lpstrInitialDir   = NULL;
	of.lpstrTitle        = NULL;
	of.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	of.nFileOffset       = 0;
	of.nFileExtension    = 0;
	of.lpstrDefExt       = "lng";
	of.lCustData         = 0;
	of.lpfnHook          = NULL;
	of.lpTemplateName    = NULL;

	return GetOpenFileName(&of);
}

static INT_PTR CALLBACK LanguageDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR pLangFile[MAX_PATH];
	DWORD dwHelpIDs[] = { IDC_LANGUAGECHECK, HIDC_LANGUAGECHECK,
						  IDC_LANGUAGEEDIT,  HIDC_LANGUAGEEDIT,
						  0, 0};

	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			const char* pLang = GetLanguage();
			if (pLang == NULL || *pLang == '\0')
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), FALSE);
			}
			else
			{
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLang);
				Button_SetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), TRUE);
			}

			return TRUE;
		}

	case WM_HELP:
		Help_HtmlHelp(((LPHELPINFO)lParam)->hItemHandle, MAME32HELP, HH_TP_HELP_WM_HELP, (DWORD)dwHelpIDs);
		break;

	case WM_CONTEXTMENU:
		Help_HtmlHelp((HWND)wParam, MAME32HELP, HH_TP_HELP_CONTEXTMENU, (DWORD)dwHelpIDs);
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
		case IDOK:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			SetLanguage(pLangFile);

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;

		case IDC_LANGUAGECHECK:
			{
				BOOL bCheck = Button_GetCheck(GetDlgItem(hDlg, IDC_LANGUAGECHECK));
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEEDIT),   bCheck);
				EnableWindow(GetDlgItem(hDlg, IDC_LANGUAGEBROWSE), bCheck);
				if (bCheck == FALSE)
					Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), "");
				return TRUE;
			}

		case IDC_LANGUAGEBROWSE:
			Edit_GetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile, MAX_PATH);
			if (SelectLanguageFile(hDlg, pLangFile) == TRUE)
				Edit_SetText(GetDlgItem(hDlg, IDC_LANGUAGEEDIT), pLangFile);
			break;
		}
		break;
	}
	return 0;
}

static void MamePlayBackGame()
{
	int nGame;
	char filename[MAX_PATH];

	*filename = 0;

	nGame = GetSelectedPickItem();
	if (nGame != -1)
		strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetOpenFileName, filename, TRUE))
	{
		void* pPlayBack;
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(filename, NULL, NULL, fname, ext);

		/* BUGBUG: osd_fopen will use inpdir, not the path selected. */
		pPlayBack = osd_fopen(fname, 0, OSD_FILETYPE_INPUTLOG, 0);
		if (pPlayBack == NULL)
		{
			char buf[MAX_PATH + 64];
			sprintf(buf, "Could not open '%s' as a valid input file.", filename);
			MessageBox(NULL, buf, MAME32NAME, MB_OK | MB_ICONERROR);
			return;
		}

		/* check for game name embedded in .inp header */
		if (pPlayBack)
		{
			INP_HEADER inp_header;

			/* read playback header */
			osd_fread(pPlayBack, &inp_header, sizeof(INP_HEADER));

			if (!isalnum(inp_header.name[0])) /* If first byte is not alpha-numeric */
				osd_fseek(pPlayBack, 0, SEEK_SET); /* old .inp file - no header */
			else
			{
				int i;
				for (i = 0; drivers[i] != 0; i++) /* find game and play it */
				{
					if (strcmp(drivers[i]->name, inp_header.name) == 0)
					{
						nGame = i;
						break;
					}
				}
			}
		}

		osd_fclose(pPlayBack);

		g_pPlayBkName = fname;
		MamePlayGameWithOptions(nGame);
		g_pPlayBkName = NULL;
	}
}

static void MamePlayRecordGame()
{
	int  nGame;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = GetSelectedPickItem();
	strcpy(filename, drivers[nGame]->name);

	if (CommonFileDialog(GetSaveFileName, filename, FALSE))
	{
		void* pRecord;
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(filename, NULL, NULL, fname, ext);

		/* BUGBUG: osd_fopen will use inpdir, not the path selected. */
		pRecord = osd_fopen(fname, 0, OSD_FILETYPE_INPUTLOG, 1);
		if (pRecord == NULL)
		{
			char buf[MAX_PATH + 64];
			sprintf(buf, "Could not open '%s' as a valid input file.", filename);
			MessageBox(NULL, buf, MAME32NAME, MB_OK | MB_ICONERROR);
			return;
		}

		osd_fclose(pRecord);

		g_pRecordName = fname;
		MamePlayGameWithOptions(nGame);
		g_pRecordName = NULL;
	}
}

static void MamePlayGame()
{
	int nGame;

	nGame = GetSelectedPickItem();

	g_pPlayBkName = NULL;
	g_pRecordName = NULL;

	MamePlayGameWithOptions(nGame);
}

static void MamePlayGameWithOptions(int nGame)
{
	CChildOutputStream cErrorStream;

	ChildOutputStream_Init(&cErrorStream);

	memcpy(&playing_game_options, GetGameOptions(nGame), sizeof(options_type));

	/* Deal with options that can be disabled. */
	EnablePlayOptions(nGame, &playing_game_options);

	if (g_pJoyGUI != NULL)
		KillTimer(hMain, 0);

	g_bAbortLoading = FALSE;

	in_emulation = TRUE;

	ChildOutputStream_Create(&cErrorStream, 1024);

	if (RunMAME(nGame, cErrorStream.m_hWrite) == 0)
	{
	   IncrementPlayCount(nGame);
	   ListView_RedrawItems(hwndList, GetSelectedPickItem(), GetSelectedPickItem());
	}
	else
	{
		if (g_bAbortLoading == FALSE)
		{
			ChildOutputStream_WaitForThreadExit(&cErrorStream);

			if (cErrorStream.m_nBufferLen != 0 && cErrorStream.m_strBuffer != NULL)
			{
				cErrorStream.m_strBuffer[cErrorStream.m_nBufferLen - 1] = '\0';
				MessageBox(hMain, cErrorStream.m_strBuffer, MAME32NAME " Error", MB_ICONERROR);
			}

			ShowWindow(hMain, SW_SHOW);
		}
	}

	ChildOutputStream_WaitForThreadExit(&cErrorStream);
	ChildOutputStream_Dump(&cErrorStream, stderr);
	ChildOutputStream_Exit(&cErrorStream);

	in_emulation = FALSE;

	if (options.language_file)
		osd_fclose(options.language_file);

	/* resort if sorting on # of times played */
	if (abs(GetSortColumn()) - 1 == COLUMN_PLAYED)
	{
		int column = GetSortColumn();

		reverse_sort = FALSE;

		if (column > 0)
		{
			last_sort = -1;
		}
		else
		{
			column = abs(column);
			last_sort = column - 1;
		}

		ColumnSort(column - 1, TRUE);
	}

	UpdateStatusBar();

	ShowWindow(hMain, SW_SHOW);
	SetFocus(hwndList);

	if (g_pJoyGUI != NULL)
		SetTimer(hMain, 0, JOYGUI_MS, NULL);
}

/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot(void)
{
	BOOL showScreenShot = GetShowScreenShot();

	SetShowScreenShot((showScreenShot) ? FALSE : TRUE);
	UpdateScreenShot();

	/* Invalidate the List control */
	if (hBitmap != NULL && showScreenShot)
	{
		RECT rcClient;
		GetClientRect(hwndList, &rcClient);
		InvalidateRect(hwndList, &rcClient, TRUE);
	}
}

static void AdjustMetrics(void)
{
	HDC hDC;
	TEXTMETRIC tm;
	int xtraX, xtraY;
	AREA area;
	int  offX, offY;
	int  maxX, maxY;
	COLORREF textColor;

	/* WM_SETTINGCHANGE also */
	xtraX  = GetSystemMetrics(SM_CXFIXEDFRAME); /* Dialog frame width */
	xtraY  = GetSystemMetrics(SM_CYFIXEDFRAME); /* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE);	/* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION);	/* Caption Height */
	maxX   = GetSystemMetrics(SM_CXSCREEN); 	/* Screen Width */
	maxY   = GetSystemMetrics(SM_CYSCREEN); 	/* Screen Height */

	hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);

	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight 	  / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	ListView_SetBkColor(hwndList, GetSysColor(COLOR_WINDOW));

	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	ListView_SetTextColor(hwndList, textColor);
	TreeView_SetBkColor(hTreeView, GetSysColor(COLOR_WINDOW));
	TreeView_SetTextColor(hTreeView, textColor);
	GetWindowArea(&area);

	offX = area.x + area.width;
	offY = area.y + area.height;

	if (offX > maxX)
	{
		offX = maxX;
		area.x = (offX - area.width > 0) ? (offX - area.width) : 0;
	}
	if (offY > maxY)
	{
		offY = maxY;
		area.y = (offY - area.height > 0) ? (offY - area.height) : 0;
	}

	SetWindowArea(&area);
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW);
}


/* Adjust options - tune them to the currently selected game */
static void EnablePlayOptions(int nIndex, options_type *o)
{
	if (!DIJoystick.Available())
		o->use_joystick = FALSE;
}

static int WhichIcon(int nItem)
{
	int iconRoms;

	iconRoms = GetHasRoms(nItem);

	/* Show Red-X if the ROMs are present and flaged as NOT WORKING */
	if (iconRoms == 1 && drivers[nItem]->flags & GAME_BROKEN)
		iconRoms = 3;

	/* Use custom icon if found */
	if (iconRoms == 1 && icon_index != NULL)
	{
		if (icon_index[nItem] == 0)
			AddIcon(nItem);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}

static BOOL HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return FALSE;

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_TREE));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(GetDlgItem(hWnd, IDC_TREE), hMenu);

	TrackPopupMenu(hMenu,
				   TPM_LEFTALIGN | TPM_RIGHTBUTTON,
				   LOWORD(lParam),
				   HIWORD(lParam),
				   0,
				   hWnd,
				   NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static BOOL HandleContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_LIST))
		return FALSE;

	if (current_view_id == ID_VIEW_DETAIL
	&&	HeaderOnContextMenu(hWnd, wParam, lParam) == TRUE)
		return TRUE;

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(GetDlgItem(hWnd, IDC_LIST), hMenu);

	TrackPopupMenu(hMenu,
				   TPM_LEFTALIGN | TPM_RIGHTBUTTON,
				   LOWORD(lParam),
				   HIWORD(lParam),
				   0,
				   hWnd,
				   NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static void UpdateMenu(HWND hWnd, HMENU hMenu)
{
	char			buf[200];
	MENUITEMINFO	mItem;
	int 			nGame = GetSelectedPickItem();

	if (have_selection)
	{
		sprintf(buf, "&Play %s", ConvertAmpersandString(ModifyThe(drivers[nGame]->description)));

		mItem.cbSize	 = sizeof(mItem);
		mItem.fMask 	 = MIIM_TYPE;
		mItem.fType 	 = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch		 = strlen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mItem);

		if (GetIsFavorite(nGame))
			EnableMenuItem(hMenu, ID_CONTEXT_ADD_FAVORITE, MF_GRAYED);
		else
			EnableMenuItem(hMenu, ID_CONTEXT_DEL_FAVORITE, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, 			MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,		MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_ADD_FAVORITE,	MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_DEL_FAVORITE,	MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (oldControl)
	{
		EnableMenuItem(hMenu, ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,  MF_GRAYED);
		EnableMenuItem(hMenu, ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}
}

/* Add ... to Items in ListView if needed */
static LPCTSTR MakeShortString(HDC hDC, LPCTSTR lpszLong, int nColumnLen, int nOffset)
{
	static const CHAR szThreeDots[] = "...";
	static CHAR szShort[MAX_PATH];
	int nStringLen = lstrlen(lpszLong);
	int nAddLen;
	SIZE size;
	int i;

	GetTextExtentPoint32(hDC, lpszLong, nStringLen, &size);
	if (nStringLen == 0 || size.cx + nOffset <= nColumnLen)
		return lpszLong;

	lstrcpy(szShort, lpszLong);
	GetTextExtentPoint32(hDC, szThreeDots, sizeof(szThreeDots), &size);
	nAddLen = size.cx;

	for (i = nStringLen - 1; i > 0; i--)
	{
		szShort[i] = 0;
		GetTextExtentPoint32(hDC, szShort, i, &size);
		if (size.cx + nOffset + nAddLen <= nColumnLen)
			break;
	}

	lstrcat(szShort, szThreeDots);

	return szShort;
}

/* DrawItem for ListView */
static void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	HDC         hDC = lpDrawItemStruct->hDC;
	RECT        rcItem = lpDrawItemStruct->rcItem;
	UINT        uiFlags = ILD_TRANSPARENT;
	HIMAGELIST  hImageList;
	int         nItem = lpDrawItemStruct->itemID;
	COLORREF    clrTextSave = 0;
	COLORREF    clrBkSave = 0;
	COLORREF    clrImage = GetSysColor(COLOR_WINDOW);
	static CHAR szBuff[MAX_PATH];
	BOOL        bFocus = (GetFocus() == hwndList);
	LPCTSTR     pszText;
	UINT        nStateImageMask;
	BOOL        bSelected;
	LV_COLUMN   lvc;
	LV_ITEM     lvi;
	RECT        rcAllLabels;
	RECT        rcLabel;
	RECT        rcIcon;
	int         offset;
	SIZE        size;
	int         i, j;
	int         nColumn;
	int         nColumnMax = 0;
	int         order[COLUMN_MAX];

	nColumnMax = GetNumColumns(hwndList);

	if (oldControl)
	{
		GetColumnOrder(order);
	}
	else
	{
		/* Get the Column Order and save it */
		ListView_GetColumnOrderArray(hwndList, nColumnMax, order);

		/* Disallow moving column 0 */
		if (order[0] != 0)
		{
			for (i = 0; i < nColumnMax; i++)
			{
				if (order[i] == 0)
				{
					order[i] = order[0];
					order[0] = 0;
				}
			}
			ListView_SetColumnOrderArray(hwndList, nColumnMax, order);
		}
	}

	/* Labels are offset by a certain amount */
	/* This offset is related to the width of a space character */
	GetTextExtentPoint32(hDC, " ", 1 , &size);
	offset = size.cx * 2;

	lvi.mask	   = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
	lvi.iItem	   = nItem;
	lvi.iSubItem   = order[0];
	lvi.pszText    = szBuff;
	lvi.cchTextMax = sizeof(szBuff);
	lvi.stateMask  = 0xFFFF;	   /* get all state flags */
	ListView_GetItem(hwndList, &lvi);

	/* This makes NO sense, but doesn't work without it? */
	strcpy(szBuff, lvi.pszText);

	bSelected = ((lvi.state & LVIS_DROPHILITED) || ( (lvi.state & LVIS_SELECTED)
		&& ((bFocus) || (GetWindowLong(hwndList, GWL_STYLE) & LVS_SHOWSELALWAYS))));

	ListView_GetItemRect(hwndList, nItem, &rcAllLabels, LVIR_BOUNDS);

	ListView_GetItemRect(hwndList, nItem, &rcLabel, LVIR_LABEL);
	rcAllLabels.left = rcLabel.left;

	if (hBitmap != NULL)
	{
		RECT		rcClient;
		HRGN		rgnBitmap;
		RECT		rcTmpBmp = rcItem;
		RECT		rcFirstItem;
		HPALETTE	hPAL;
		HDC 		htempDC;
		HBITMAP 	oldBitmap;

		htempDC = CreateCompatibleDC(hDC);

		oldBitmap = SelectObject(htempDC, hBitmap);

		GetClientRect(hwndList, &rcClient);
		rcTmpBmp.right = rcClient.right;
		/* We also need to check whether it is the last item
		   The update region has to be extended to the bottom if it is */
		if (nItem == ListView_GetItemCount(hwndList) - 1)
			rcTmpBmp.bottom = rcClient.bottom;

		rgnBitmap = CreateRectRgnIndirect(&rcTmpBmp);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);

		hPAL = (! hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(htempDC, hPAL, FALSE);
			RealizePalette(htempDC);
		}

		ListView_GetItemRect(hwndList, 0, &rcFirstItem, LVIR_BOUNDS);

		for (i = rcFirstItem.left; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = rcFirstItem.top; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

		SelectObject(htempDC, oldBitmap);
		DeleteDC(htempDC);

		if (!bmDesc.bmColors)
		{
			DeleteObject(hPAL);
			hPAL = 0;
		}
	}

	SetTextColor(hDC, GetListFontColor());

	if (bSelected)
	{
		HBRUSH hBrush;
		HBRUSH hOldBrush;

		if (bFocus)
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			clrTextSave = SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
			clrBkSave	= SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			hBrush		= CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		}

		hOldBrush = SelectObject(hDC, hBrush);
		FillRect(hDC, &rcAllLabels, hBrush);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}
	else
	if (hBitmap == NULL)
	{
		HBRUSH hBrush;

		hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
		FillRect(hDC, &rcAllLabels, hBrush);
		DeleteObject(hBrush);
	}

	if (lvi.state & LVIS_CUT)
	{
		clrImage = GetSysColor(COLOR_WINDOW);
		uiFlags |= ILD_BLEND50;
	}
	else
	if (bSelected)
	{
		if (bFocus)
			clrImage = GetSysColor(COLOR_HIGHLIGHT);
		else
			clrImage = GetSysColor(COLOR_BTNFACE);

		uiFlags |= ILD_BLEND50;
	}

	nStateImageMask = lvi.state & LVIS_STATEIMAGEMASK;

	if (nStateImageMask)
	{
		int nImage = (nStateImageMask >> 12) - 1;
		hImageList = ListView_GetImageList(hwndList, LVSIL_STATE);
		if (hImageList)
			ImageList_Draw(hImageList, nImage, hDC, rcItem.left, rcItem.top, ILD_TRANSPARENT);
	}

	ListView_GetItemRect(hwndList, nItem, &rcIcon, LVIR_ICON);

	hImageList = ListView_GetImageList(hwndList, LVSIL_SMALL);
	if (hImageList)
	{
		UINT nOvlImageMask = lvi.state & LVIS_OVERLAYMASK;
		if (rcItem.left < rcItem.right - 1)
			ImageList_DrawEx(hImageList, lvi.iImage, hDC, rcIcon.left, rcIcon.top,
							 16, 16, GetSysColor(COLOR_WINDOW), clrImage, uiFlags | nOvlImageMask);
	}

	ListView_GetItemRect(hwndList, nItem, &rcItem, LVIR_LABEL);

	pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left,	2 * offset);

	rcLabel = rcItem;
	rcLabel.left  += offset;
	rcLabel.right -= offset;

	DrawText(hDC, pszText, -1, &rcLabel,
			 DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

	for (nColumn = 1; nColumn < nColumnMax; nColumn++)
	{
		int 	nRetLen;
		UINT	nJustify;
		LV_ITEM lvItem;

		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		ListView_GetColumn(hwndList, order[nColumn], &lvc);

		lvItem.mask 	  = LVIF_TEXT;
		lvItem.iItem	  = nItem;
		lvItem.iSubItem   = order[nColumn];
		lvItem.pszText	  = szBuff;
		lvItem.cchTextMax = sizeof(szBuff);

		if (ListView_GetItem(hwndList, &lvItem) == FALSE)
			continue;

		/* This shouldn't oughtta be, but it's needed!!! */
		strcpy(szBuff, lvItem.pszText);

		rcItem.left   = rcItem.right;
		rcItem.right += lvc.cx;

		nRetLen = strlen(szBuff);
		if (nRetLen == 0)
			continue;

		pszText = MakeShortString(hDC, szBuff, rcItem.right - rcItem.left, 2 * offset);

		nJustify = DT_LEFT;

		if (pszText == szBuff)
		{
			switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
			{
			case LVCFMT_RIGHT:
				nJustify = DT_RIGHT;
				break;

			case LVCFMT_CENTER:
				nJustify = DT_CENTER;
				break;

			default:
				break;
			}
		}

		rcLabel = rcItem;
		rcLabel.left  += offset;
		rcLabel.right -= offset;
		DrawText(hDC, pszText, -1, &rcLabel,
				 nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);
	}

	if (lvi.state & LVIS_FOCUSED && bFocus)
		DrawFocusRect(hDC, &rcAllLabels);

	if (bSelected)
	{
		SetTextColor(hDC, clrTextSave);
		SetBkColor(hDC, clrBkSave);
	}
}

/* Header code - Directional Arrows */
static LRESULT CALLBACK HeaderWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return ListCtrlOnErase(hwnd, (HDC)wParam);

	case WM_PAINT:
		if (current_view_id != ID_VIEW_DETAIL)
			if (ListCtrlOnPaint(hwnd, uMsg) == 0)
				return 0;
		break;

	case WM_DRAWITEM:
		{
			/* UINT idCtl = (UINT)wParam; */
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			RECT rcLabel;
			int nSavedDC;
			HRGN hrgn;
			int nOffset;
			SIZE size;
			TCHAR tszText[256];
			HD_ITEM hdi;
			UINT uFormat;
			HDC hdc;

			hdc = lpdis->hDC;

			CopyRect(&rcLabel, &(lpdis->rcItem));

			/* save the DC */
			nSavedDC = SaveDC(hdc);

			/* set clip region to column */
			hrgn = CreateRectRgnIndirect(&rcLabel);
			SelectObject(hdc, hrgn);
			DeleteObject(hrgn);

			/* draw the background */
			FillRect(hdc, &rcLabel, GetSysColorBrush(COLOR_3DFACE));

			/* offset the label */
			GetTextExtentPoint32(hdc, TEXT(" "), 1, &size);
			nOffset = size.cx * 2;

			/* get the column text and format */
			hdi.mask	   = HDI_TEXT | HDI_FORMAT;
			hdi.pszText    = tszText;
			hdi.cchTextMax = 255;
			Header_GetItem(GetDlgItem(hwnd, 0), lpdis->itemID, &hdi);

			/* determine format for drawing label */
			uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP |
					  DT_VCENTER | DT_END_ELLIPSIS;

			/* determine justification */
			if (hdi.fmt & HDF_CENTER)
				uFormat |= DT_CENTER;
			else
			if (hdi.fmt & HDF_RIGHT)
				uFormat |= DT_RIGHT;
			else
				uFormat |= DT_LEFT;

			/* adjust the rect if selected */
			if (lpdis->itemState & ODS_SELECTED)
			{
				rcLabel.left++;
				rcLabel.top += 2;
				rcLabel.right++;
			}

			/* adjust rect for sort arrow */
			if (lpdis->itemID == m_uHeaderSortCol)
			{
				rcLabel.right -= (3 * nOffset);
			}

			rcLabel.left  += nOffset;
			rcLabel.right -= nOffset;

			/* draw label */
			if (rcLabel.left < rcLabel.right)
				DrawText(hdc, tszText, -1, &rcLabel, uFormat);

			/* draw the arrow */
			if (lpdis->itemID == m_uHeaderSortCol)
			{
				RECT rcIcon;
				HPEN hpenLight, hpenShadow, hpenOld;

				CopyRect(&rcIcon, &(lpdis->rcItem));

				hpenLight  = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
				hpenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
				hpenOld    = SelectObject(hdc, hpenLight);

				if (m_fHeaderSortAsc)
				{
					/* draw triangle pointing up */
					MoveToEx(hdc, rcIcon.right - 2 * nOffset, nOffset - 1, NULL);
					LineTo(hdc, rcIcon.right - 3 * nOffset / 2,
						   rcIcon.bottom - nOffset);
					LineTo(hdc, rcIcon.right - 5 * nOffset / 2 - 2,
						   rcIcon.bottom - nOffset);
					MoveToEx(hdc, rcIcon.right - 5 * nOffset / 2 - 1,
							 rcIcon.bottom - nOffset, NULL);

					SelectObject(hdc, hpenShadow);
					LineTo(hdc, rcIcon.right - 2 * nOffset, nOffset - 2);
				}
				else
				{
					/* draw triangle pointing down */
					MoveToEx(hdc, rcIcon.right - 3 * nOffset / 2, nOffset - 1,
						NULL);
					LineTo(hdc, rcIcon.right - 2 * nOffset - 1,
						   rcIcon.bottom - nOffset);
					LineTo(hdc, rcIcon.right - 2 * nOffset - 1,
						   rcIcon.bottom - nOffset);
					MoveToEx(hdc, rcIcon.right - 2 * nOffset - 1,
							 rcIcon.bottom - nOffset, NULL);

					SelectObject(hdc, hpenShadow);
					LineTo(hdc, rcIcon.right - 5 * nOffset / 2 - 1,
						   nOffset - 1);
					LineTo(hdc, rcIcon.right - 3 * nOffset / 2,
						   nOffset - 1);
				}
				SelectObject(hdc, hpenOld);
				DeletePen(hpenLight);
				DeletePen(hpenShadow);
			}
			RestoreDC(hdc, nSavedDC);
			return 1;
		}

	case WM_NOTIFY:
		{
			HD_NOTIFY *pHDN = (HD_NOTIFY*)lParam;

			/*
			   This code is for using bitmap in the background
			   Invalidate the right side of the control when a column is resized			
			*/
			if (pHDN->hdr.code == HDN_ITEMCHANGINGW
			||	pHDN->hdr.code == HDN_ITEMCHANGINGA)
			{
				if (hBitmap != NULL)
				{
					RECT rcClient;
					DWORD dwPos;
					POINT pt;

					dwPos = GetMessagePos();
					pt.x = LOWORD(dwPos);
					pt.y = HIWORD(dwPos);

					GetClientRect(hwnd, &rcClient);
					ScreenToClient(hwnd, &pt);
					rcClient.left = pt.x;
					InvalidateRect(hwnd, &rcClient, FALSE);
				}
			}
		}
		break;
	}

	/* message not handled */
	return CallWindowProc(g_lpHeaderWndProc, hwnd, uMsg, wParam, lParam);
}

/* Subclass the Listview Header */
static void Header_Initialize(HWND hWndList)
{
	/*
		this will subclass the listview (where WM_DRAWITEM gets sent for
		the header control)
	*/
	g_lpHeaderWndProc = (WNDPROC)(LONG)GetWindowLong(hWndList, GWL_WNDPROC);
	SetWindowLong(hWndList, GWL_WNDPROC, (LONG)HeaderWndProc);
}


/* Set Sort information needed by Header */
static void Header_SetSortInfo(HWND hWndList, int nCol, BOOL bAsc)
{
	HWND hwndHeader;
	HD_ITEM hdi;

	m_uHeaderSortCol = nCol;
	m_fHeaderSortAsc = bAsc;

	hwndHeader = GetDlgItem(hWndList, 0);

	/* change this item to owner draw */
	hdi.mask = HDI_FORMAT;
	Header_GetItem(hwndHeader, nCol, &hdi);
	hdi.fmt |= HDF_OWNERDRAW;
	Header_SetItem(hwndHeader, nCol, &hdi);

	/* repaint the header */
	InvalidateRect(hwndHeader, NULL, TRUE);
}

static BOOL ListCtrlOnErase(HWND hWnd, HDC hDC)
{
	RECT		rcClient;
	HRGN		rgnBitmap;
	HPALETTE	hPAL;

	if (!IsValidListControl(hWnd))
		return 1;

	GetClientRect(hWnd, &rcClient);

	if (hBitmap)
	{
		int 		i, j;
		HDC 		htempDC;
		POINT		ptOrigin;
		POINT		pt = {0,0};
		HBITMAP 	hOldBitmap;

		htempDC = CreateCompatibleDC(hDC);
		hOldBitmap = SelectObject(htempDC, hBitmap);

		rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteObject(rgnBitmap);

		hPAL = (!hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(htempDC, hPAL, FALSE);
			RealizePalette(htempDC);
		}

		/* Get x and y offset */
		ClientToScreen(hWnd, &pt);
		GetDCOrgEx(hDC, &ptOrigin);
		ptOrigin.x -= pt.x;
		ptOrigin.y -= pt.y;
		ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
		ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

		for (i = ptOrigin.x; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = ptOrigin.y; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

		SelectObject(htempDC, hOldBitmap);
		DeleteDC(htempDC);

		if (!bmDesc.bmColors)
		{
			DeleteObject(hPAL);
			hPAL = 0;
		}
	}
	else	/* Use standard control face color */
	{
		HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
		HBRUSH hOldBrush;

		hOldBrush = SelectObject(hDC, hBrush);
		FillRect(hDC, &rcClient, hBrush);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}

	return TRUE;
}

static BOOL ListCtrlOnPaint(HWND hWnd, UINT uMsg)
{
	PAINTSTRUCT ps;
	HDC 		hDC;
	RECT		rcClip, rcClient;
	HDC 		memDC;
	HBITMAP 	bitmap;
	HBITMAP 	hOldBitmap;

	if (!IsValidListControl(hWnd))
		return 1;

	hDC = BeginPaint(hWnd, &ps);
	rcClient = ps.rcPaint;

	GetClipBox(hDC, &rcClip);
	GetClientRect(hWnd, &rcClient);

	/* Create a compatible memory DC */
	memDC = CreateCompatibleDC(hDC);

	/* Select a compatible bitmap into the memory DC */
	bitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
									rcClient.bottom - rcClient.top );
	hOldBitmap = SelectObject(memDC, bitmap);

	BitBlt(memDC, rcClip.left, rcClip.top,
		   rcClip.right - rcClip.left, rcClip.bottom - rcClip.top,
		   hDC, rcClip.left, rcClip.top, SRCCOPY);

	/* First let the control do its default drawing. */
	CallWindowProc(g_lpHeaderWndProc, hWnd, uMsg, (WPARAM)memDC, 0);

	/* Draw bitmap in the background if one has been set */
	if (hBitmap != NULL)
	{
		HPALETTE	hPAL;
		HDC maskDC;
		HBITMAP maskBitmap;
		HDC tempDC;
		HDC imageDC;
		HBITMAP bmpImage;
		HBITMAP hOldBmpImage;
		HBITMAP hOldMaskBitmap;
		HBITMAP hOldHBitmap;
		int i, j;
		POINT ptOrigin;
		POINT pt = {0,0};

		/* Now create a mask */
		maskDC = CreateCompatibleDC(hDC);

		/* Create monochrome bitmap for the mask */
		maskBitmap = CreateBitmap(rcClient.right  - rcClient.left,
								  rcClient.bottom - rcClient.top,
								  1, 1, NULL );

		hOldMaskBitmap = SelectObject(maskDC, maskBitmap);
		SetBkColor(memDC, GetSysColor(COLOR_WINDOW));

		/* Create the mask from the memory DC */
		BitBlt(maskDC, 0, 0, rcClient.right - rcClient.left,
			   rcClient.bottom - rcClient.top, memDC,
			   rcClient.left, rcClient.top, SRCCOPY);

		tempDC = CreateCompatibleDC(hDC);
		hOldHBitmap = SelectObject(tempDC, hBitmap);

		imageDC = CreateCompatibleDC(hDC);
		bmpImage = CreateCompatibleBitmap(hDC,
										  rcClient.right  - rcClient.left,
										  rcClient.bottom - rcClient.top);
		hOldBmpImage = SelectObject(imageDC, bmpImage);

		hPAL = (! hPALbg) ? CreateHalftonePalette(hDC) : hPALbg;

		if (GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
		{
			SelectPalette(hDC, hPAL, FALSE);
			RealizePalette(hDC);
			SelectPalette(imageDC, hPAL, FALSE);
		}

		/* Get x and y offset */
		ClientToScreen(hWnd, &pt);
		GetDCOrgEx(hDC, &ptOrigin);
		ptOrigin.x -= pt.x;
		ptOrigin.y -= pt.y;
		ptOrigin.x = -GetScrollPos(hWnd, SB_HORZ);
		ptOrigin.y = -GetScrollPos(hWnd, SB_VERT);

		/* Draw bitmap in tiled manner to imageDC */
		for (i = ptOrigin.x; i < rcClient.right; i += bmDesc.bmWidth)
			for (j = ptOrigin.y; j < rcClient.bottom; j += bmDesc.bmHeight)
				BitBlt(imageDC,  i, j, bmDesc.bmWidth, bmDesc.bmHeight,
					   tempDC, 0, 0, SRCCOPY);

		/*
		   Set the background in memDC to black. Using SRCPAINT with black
		   and any other color results in the other color, thus making black
		   the transparent color
		*/
		SetBkColor(memDC, RGB(0, 0, 0));
		SetTextColor(memDC, RGB(255, 255, 255));
		BitBlt(memDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		/* Set the foreground to black. See comment above. */
		SetBkColor(imageDC, RGB(255, 255, 255));
		SetTextColor(imageDC, RGB(0, 0, 0));
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   maskDC, rcClip.left, rcClip.top, SRCAND);

		/* Combine the foreground with the background */
		BitBlt(imageDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   memDC, rcClip.left, rcClip.top, SRCPAINT);

		/* Draw the final image to the screen */
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   imageDC, rcClip.left, rcClip.top, SRCCOPY);

		SelectObject(maskDC, hOldMaskBitmap);
		SelectObject(tempDC, hOldHBitmap);
		SelectObject(imageDC, hOldBmpImage);

		DeleteDC(maskDC);
		DeleteDC(imageDC);
		DeleteDC(tempDC);
		DeleteObject(bmpImage);
		DeleteObject(maskBitmap);
		if (!hPALbg)
		{
			DeleteObject(hPAL);
		}
	}
	else
	{
		BitBlt(hDC, rcClip.left, rcClip.top, rcClip.right - rcClip.left,
			   rcClip.bottom - rcClip.top,
			   memDC, rcClip.left, rcClip.top, SRCCOPY);
	}
	SelectObject(memDC, hOldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	EndPaint(hWnd, &ps);
	return 0;
}


/* replaces function in src/windows/fileio.c: */

int osd_display_loading_rom_message(const char* name, int current, int total)
{
	int retval;

	if (name != NULL)
		retval = UpdateLoadProgress(name, current - 1, total);
	else
		retval = UpdateLoadProgress("", total, total);
	
	return retval;
}

static INT_PTR CALLBACK LoadProgressDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
			char buf[256];

			sprintf(buf, "Loading %s", ModifyThe(Machine->gamedrv->description));
			SetWindowText(hDlg, buf);

			g_bAbortLoading = FALSE;
		}
		return 1;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return 1;

	case WM_COMMAND:
	   if (LOWORD(wParam) == IDCANCEL)
	   {
		   g_bAbortLoading = TRUE;
		   EndDialog(hDlg, IDCANCEL);
		   return 1;
	   }
	}
	return 0;
}

int UpdateLoadProgress(const char* name, int current, int total)
{
	static HWND hWndLoad = 0;
	MSG Msg;
	int nReturn = 0;

	if (hWndLoad == NULL)
	{
		hWndLoad = CreateDialog(GetModuleHandle(NULL),
								MAKEINTRESOURCE(IDD_LOAD_PROGRESS),
								hMain,
								LoadProgressDialogProc);
	}

	if (g_bAbortLoading == TRUE)
	{
		nReturn = 1;
		DestroyWindow(hWndLoad);
	}
	else
	{
		SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, total));
		SendDlgItemMessage(hWndLoad, IDC_LOAD_PROGRESS, PBM_SETPOS, current, 0);

		SetWindowText(GetDlgItem(hWndLoad, IDC_LOAD_ROMNAME), name);

		if (current == total)
			DestroyWindow(hWndLoad);
	}

	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (!IsDialogMessage(hWndLoad, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

	return nReturn;
}

/* End of source file */
