/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  Properties.c

    Properties Popup and Misc UI support routines.

    Created 8/29/98 by Mike Haaland (mhaaland@hypertech.com)

***************************************************************************/

#define MULTI_MONITOR

#include "driver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include <commdlg.h>
#include <strings.h>
#include <sys/stat.h>
#include <ddraw.h>

#include "info.h"
#include "audit32.h"
#include "options.h"
#include "file.h"
#include "resource.h"
#include "Joystick.h"       /* For joystick avalibility. */
#include "DIJoystick.h"     /* For DIJoystick avalibility. */
#include "DIKeyboard.h"     /* For DIKeyboard avalibility. */
#include "DDrawDisplay.h"   /* For display modes. */
#include "m32util.h"
#include "directdraw.h"
#include "properties.h"

#include "Mame32.h"
#include "DataMap.h"
#include "resource.hm"

/*
   For compilers that don't support nameless unions
*/
#if 0
#ifdef NONAMELESSUNION
/* PROPSHEETHEADER*/
#define nStartPage  DUMMYUNIONNAME2.nStartPage
/* PROPSHEETPAGE */
#define pszTemplate DUMMYUNIONNAME.pszTemplate
#define ppsp        DUMMYUNIONNAME3.ppsp
/* PROPSHEETHEADER */
#define pszIcon     DUMMYUNIONNAME.pszIcon
#endif
#endif
/***************************************************************
 * Imported function prototypes
 ***************************************************************/

extern BOOL GameUsesTrackball(int game);
extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);

/**************************************************************
 * Local function prototypes
 **************************************************************/

static INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK GameDisplayOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

static void SetStereoEnabled(HWND hWnd, int index);
static void SetYM3812Enabled(HWND hWnd, int index);
static void SetSamplesEnabled(HWND hWnd, int index);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void FlickerSelectionChange(HWND hwnd);
static void BeamSelectionChange(HWND hwnd);
static void GammaSelectionChange(HWND hwnd);
static void BrightnessSelectionChange(HWND hwnd);
static void DepthSelectionChange(HWND hWnd, HWND hWndCtrl);
static void DisplayChange(HWND hWnd);
static void VolumeSelectionChange(HWND hwnd);
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth);
static void InitializeDisplayModeUI(HWND hwnd);
static void InitializeSoundUI(HWND hwnd);
static void InitializeDisplayTypeUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void InitializeDepthUI(HWND hwnd);
static void InitializeDefaultInputUI(HWND hWnd);
static void PropToOptions(HWND hWnd, options_type *o);
static void OptionsToProp(HWND hWnd, options_type *o);
static void SetPropEnabledControls(HWND hWnd);

static void BuildDataMap(void);
static void ResetDataMap(void);

static void HistoryFixBuf(char *buf);

/**************************************************************
 * Local private variables
 **************************************************************/

static options_type  origGameOpts;
static options_type* lpGameOpts = NULL;

static int  iGame        = 0;
static BOOL bUseDefaults = FALSE;
static BOOL bReset       = FALSE;
static BOOL bFullScreen  = 0;
static int  nSampleRate  = 0;
static int  nVolume      = 0;
static BOOL bDouble      = 0;
static int  nScreenSize  = 0;
static int  nDepth       = 0;
static int  nGamma       = 0;
static int  nBeam        = 0;

/* Game history variables */
#define MAX_HISTORY_LEN     (8 * 1024)

static char   historyBuf[MAX_HISTORY_LEN];
static char   tempHistoryBuf[MAX_HISTORY_LEN];

/* Property sheets */
static DWORD dwDlgId[] = {
    IDD_PROP_GAME,
    IDD_PROP_AUDIT,
    IDD_PROP_DISPLAY,
    IDD_PROP_SOUND,
    IDD_PROP_INPUT,
    IDD_PROP_ADVANCED,
    IDD_PROP_MISC,
    IDD_PROP_VECTOR
};

#define NUM_PROPSHEETS (sizeof(dwDlgId) / sizeof(dwDlgId[0]))

/* Help IDs */
static DWORD dwHelpIDs [] = {
    IDC_AI_JOYSTICK,        HIDC_AI_JOYSTICK,
    IDC_AI_MOUSE,           HIDC_AI_MOUSE,
    IDC_ANTIALIAS,          HIDC_ANTIALIAS,
    IDC_ARTWORK,            HIDC_ARTWORK,
    IDC_AUTOFRAMESKIP,      HIDC_AUTOFRAMESKIP,
    IDC_AUTO_PAUSE,         HIDC_AUTO_PAUSE,
    IDC_BEAM,               HIDC_BEAM,
    IDC_BRIGHTNESS,         HIDC_BRIGHTNESS,
    IDC_CHEAT,              HIDC_CHEAT,
    IDC_DEFAULT_INPUT,      HIDC_DEFAULT_INPUT,
    IDC_DEPTH,              HIDC_DEPTH,
    IDC_DIJOYSTICK,         HIDC_DIJOYSTICK,
    IDC_DIKEYBOARD,         HIDC_DIKEYBOARD,
    IDC_DIRTY,              HIDC_DIRTY,
    IDC_DISABLE_MMX,        HIDC_DISABLE_MMX,
    IDC_DOUBLE,             HIDC_DOUBLE,
    IDC_FLICKER,            HIDC_FLICKER,
    IDC_FLIPX,              HIDC_FLIPX,
    IDC_FLIPY,              HIDC_FLIPY,
    IDC_FRAMESKIP,          HIDC_FRAMESKIP,
    IDC_FULLSCREEN,         HIDC_FULLSCREEN,
    IDC_DISPLAYS,           HIDC_DISPLAYS,
    IDC_GAMMA,              HIDC_GAMMA,
    IDC_JOYSTICK,           HIDC_JOYSTICK,
    IDC_LOG,                HIDC_LOG,
    IDC_PROP_RESET,         HIDC_PROP_RESET,
    IDC_ROTATE,             HIDC_ROTATE,
    IDC_SAMPLERATE,         HIDC_SAMPLERATE,
    IDC_SAMPLES,            HIDC_SAMPLES,
    IDC_SCANLINES,          HIDC_SCANLINES,
    IDC_SIZES,              HIDC_SIZES,
    IDC_SKIP_COLUMNS,       HIDC_SKIP_COLUMNS,
    IDC_SKIP_LINES,         HIDC_SKIP_LINES,
    IDC_SOUNDTYPE,          HIDC_SOUNDTYPE,
    IDC_STEREO,             HIDC_STEREO,
    IDC_VOLUME,             HIDC_VOLUME,
    IDC_TRANSLUCENCY,       HIDC_TRANSLUCENCY,
    IDC_USEBLIT,            HIDC_USEBLIT,
    IDC_USE_DEFAULT,        HIDC_USE_DEFAULT,
    IDC_USE_FM_YM3812,      HIDC_USE_FM_YM3812,
    IDC_USE_FILTER,         HIDC_USE_FILTER,
    IDC_VECTOR_DOUBLE,      HIDC_VECTOR_DOUBLE,
    IDC_VSCANLINES,         HIDC_VSCANLINES,
    IDC_WINDOW_DDRAW,       HIDC_WINDOW_DDRAW,
    IDC_HISTORY,            HIDC_HISTORY,
/*    IDC_FILTER_AVAILABLE,   HIDC_FILTER_AVAILABLE,*/
    IDC_FILTER_CLONES,      HIDC_FILTER_CLONES,
#ifndef NEOFREE
    IDC_FILTER_NEOGEO,      HIDC_FILTER_NEOGEO,
#endif
    IDC_FILTER_NONWORKING,  HIDC_FILTER_NONWORKING,
    IDC_FILTER_ORIGINALS,   HIDC_FILTER_ORIGINALS,
    IDC_FILTER_RASTER,      HIDC_FILTER_RASTER,
    IDC_FILTER_UNAVAILABLE, HIDC_FILTER_UNAVAILABLE,
    IDC_FILTER_VECTOR,      HIDC_FILTER_VECTOR,
    IDC_FILTER_WORKING,     HIDC_FILTER_WORKING,
    IDC_FILTER_EDIT,        HIDC_FILTER_EDIT,
    IDC_RESET_DEFAULT,      HIDC_RESET_DEFAULT,
    IDC_RESET_FILTERS,      HIDC_RESET_FILTERS,
    IDC_RESET_GAMES,        HIDC_RESET_GAMES,
    IDC_RESET_UI,           HIDC_RESET_UI,
    IDC_START_GAME_CHECK,   HIDC_START_GAME_CHECK,
    IDC_START_VERSION_WARN, HIDC_START_VERSION_WARN,
    IDC_START_MMX_CHECK,    HIDC_START_MMX_CHECK,
    IDC_TRIPLE_BUFFER,      HIDC_TRIPLE_BUFFER,
    0,                      0
};


/***************************************************************
 * Public functions
 ***************************************************************/

DWORD GetHelpIDs(void)
{
    return (DWORD) (LPSTR) dwHelpIDs;
}

/* Checks of all ROMs are available for 'game' and returns result
 * Returns TRUE if all ROMs found, 0 if any ROMs are missing.
 */
BOOL FindRomSet(int game)
{
    const struct RomModule  *region, *rom;
    const struct GameDriver *gamedrv;
    const char              *name;
    int                     err;
    unsigned int            length, icrc;

    gamedrv = drivers[game];

    if (!osd_faccess(gamedrv->name, OSD_FILETYPE_ROM))
    {
        /* if the game is a clone, try loading the ROM from the main version */
        if (gamedrv->clone_of == 0
        ||  (gamedrv->clone_of->flags & NOT_A_DRIVER)
        ||  !osd_faccess(gamedrv->clone_of->name, OSD_FILETYPE_ROM))
            return FALSE;
    }

    /* loop over regions, then over files */
    for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
    {
        for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
        {
            extern struct GameDriver driver_0;
            const struct GameDriver *drv;

            name = ROM_GETNAME(rom);
            icrc = ROM_GETCRC(rom);
            length = 0;

            /* obtain CRC-32 and length of ROM file */
            drv = gamedrv;
            do
            {
                err = osd_fchecksum(drv->name, name, &length, &icrc);
                drv = drv->clone_of;
            } while (err && drv && drv != &driver_0);

            if (err)
                return FALSE;
        }
    }

    return TRUE;
}

/* Checks if the game uses external samples at all
 * Returns TRUE if this driver expects samples
 */
BOOL GameUsesSamples(int game)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)

    int i;
    static const struct GameDriver *gamedrv;

    gamedrv = drivers[game];

    for (i = 0; gamedrv->drv->sound[i].sound_type && i < MAX_SOUND; i++)
    {
        const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
        if (gamedrv->drv->sound[i].sound_type == SOUND_SAMPLES)
            samplenames = ((struct Samplesinterface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

#if (HAS_VLM5030 == 1)
        if (gamedrv->drv->sound[i].sound_type == SOUND_VLM5030)
            samplenames = ((struct VLM5030interface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

        if (samplenames != 0 && samplenames[0] != 0)
            return TRUE;
    }

#endif

    return FALSE;
}

/* Checks for all samples in a sample set.
 * Returns TRUE if all samples are found, FALSE if any are missing.
 */
BOOL FindSampleSet(int game)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)

    static const struct GameDriver *gamedrv;
    const char* sharedname;
    BOOL bStatus;
    int  skipfirst;
    int  count = 0;
    int  j, i;

    if (GameUsesSamples(game) == FALSE)
        return TRUE;

    gamedrv = drivers[game];

    for (i = 0; gamedrv->drv->sound[i].sound_type && i < MAX_SOUND; i++)
    {
        const char **samplenames = NULL;

#if (HAS_SAMPLES == 1)
        if (gamedrv->drv->sound[i].sound_type == SOUND_SAMPLES)
            samplenames = ((struct Samplesinterface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

#if (HAS_VLM5030 == 1)
        if (gamedrv->drv->sound[i].sound_type == SOUND_VLM5030)
            samplenames = ((struct VLM5030interface *)gamedrv->drv->sound[i].sound_interface)->samplenames;
#endif

        if (samplenames != 0 && samplenames[0] != 0)
        {
            BOOL have_samples = FALSE;
            BOOL have_shared  = FALSE;

            if (samplenames[0][0]=='*')
            {
                sharedname = samplenames[0]+1;
                skipfirst = 1;
            }
            else
            {
                sharedname = NULL;
                skipfirst = 0;
            }

            /* do we have samples for this game? */
            have_samples = osd_faccess(gamedrv->name, OSD_FILETYPE_SAMPLE);

            /* try shared samples */
            if (skipfirst)
                have_shared = osd_faccess(sharedname, OSD_FILETYPE_SAMPLE);

            /* if still not found, we're done */
            if (!have_samples && !have_shared)
                return FALSE;

            for (j = skipfirst; samplenames[j] != 0; j++)
            {
                bStatus = FALSE;

                /* skip empty definitions */
                if (strlen(samplenames[j]) == 0)
                    continue;

                if (have_samples)
                    bStatus = File_Status(gamedrv->name, samplenames[j], OSD_FILETYPE_SAMPLE);

                if (!bStatus && have_shared)
                {
                    bStatus = File_Status(sharedname, samplenames[j], OSD_FILETYPE_SAMPLE);
                    if (!bStatus)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }

#endif

    return TRUE;
}

void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd)
{
    PROPSHEETHEADER pshead;
    PROPSHEETPAGE   pspage[NUM_PROPSHEETS];
    int             i;
    int             maxPropSheets;

    iGame = -1;
    maxPropSheets = NUM_PROPSHEETS - 2;

    /* Get default options to populate property sheets */
    lpGameOpts = GetDefaultOptions();
    bUseDefaults = FALSE;
    /* Stash the result for comparing later */
    memcpy(&origGameOpts, lpGameOpts, sizeof(options_type));
    bReset = FALSE;
    BuildDataMap();

    ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));
    ZeroMemory(pspage, sizeof(PROPSHEETPAGE) * maxPropSheets);

    /* Fill in the property sheet header */
    pshead.hwndParent     = hWnd;
    pshead.dwSize         = sizeof(PROPSHEETHEADER);
    pshead.dwFlags        = PSH_PROPSHEETPAGE | PSH_USEICONID |
                            PSH_PROPTITLE;
    pshead.hInstance      = hInst;
    pshead.pszCaption     = "Default Game";
    pshead.nPages         = maxPropSheets;
    pshead.nStartPage     = 0;
    pshead.pszIcon        = MAKEINTRESOURCE(IDI_MAME32_ICON);
    pshead.ppsp           = pspage;

    /* Fill out the property page templates */
    for (i = 0; i < maxPropSheets; i++)
    {
        pspage[i].dwSize      = sizeof(PROPSHEETPAGE);
        pspage[i].dwFlags     = 0;
        pspage[i].hInstance   = hInst;
        pspage[i].pszTemplate = MAKEINTRESOURCE(dwDlgId[i + 2]);
        pspage[i].pfnCallback = NULL;
        pspage[i].lParam      = 0;
        pspage[i].pfnDlgProc  = GameOptionsProc;
    }
    pspage[2 - 2].pfnDlgProc = GameDisplayOptionsProc;

    /* Create the Property sheet and display it */
    if (PropertySheet(&pshead) == -1)
    {
        char temp[100];
        DWORD dwError = GetLastError();
        sprintf(temp, "Propery Sheet Error %d %X", dwError, dwError);
        MessageBox(0, temp, "Error", IDOK);
    }
}


void InitPropertyPage(HINSTANCE hInst, HWND hWnd, int game_num)
{
    InitPropertyPageToPage(hInst, hWnd, game_num, PROPERTIES_PAGE);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, int game_num,int start_page)
{
    PROPSHEETHEADER pshead;
    PROPSHEETPAGE   pspage[NUM_PROPSHEETS];
    int             i;
    int             maxPropSheets;

    InitGameAudit(game_num);
    iGame = game_num;
    maxPropSheets = (drivers[iGame]->drv->video_attributes & VIDEO_TYPE_VECTOR) ? NUM_PROPSHEETS : NUM_PROPSHEETS - 1;

    /* Get Game options to populate property sheets */
    lpGameOpts = GetGameOptions(game_num);
    bUseDefaults = lpGameOpts->use_default;
    /* Stash the result for comparing later */
    memcpy(&origGameOpts, lpGameOpts, sizeof(options_type));
    bReset = FALSE;
    BuildDataMap();

    ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));
    ZeroMemory(pspage, sizeof(PROPSHEETPAGE) * maxPropSheets);

    /* Fill in the property sheet header */
    pshead.hwndParent     = hWnd;
    pshead.dwSize         = sizeof(PROPSHEETHEADER);
    pshead.dwFlags        = PSH_PROPSHEETPAGE | PSH_USEICONID |
                            PSH_PROPTITLE;
    pshead.hInstance      = hInst;
    pshead.pszCaption     = ModifyThe(drivers[iGame]->description);
    pshead.nPages         = maxPropSheets;
    pshead.nStartPage     = start_page;
    pshead.pszIcon        = MAKEINTRESOURCE(IDI_MAME32_ICON);
    pshead.ppsp           = pspage;

    /* Fill out the property page templates */
    for (i = 0; i < maxPropSheets; i++)
    {
        pspage[i].dwSize      = sizeof(PROPSHEETPAGE);
        pspage[i].dwFlags     = 0;
        pspage[i].hInstance   = hInst;
        pspage[i].pszTemplate = MAKEINTRESOURCE(dwDlgId[i]);
        pspage[i].pfnCallback = NULL;
        pspage[i].lParam      = 0;
    }

    pspage[0].pfnDlgProc = GamePropertiesDialogProc;
    pspage[1].pfnDlgProc = GameAuditDialogProc;

    pspage[2].pfnDlgProc = GameDisplayOptionsProc;
    pspage[3].pfnDlgProc = GameOptionsProc;
    pspage[4].pfnDlgProc = GameOptionsProc;
    pspage[5].pfnDlgProc = GameOptionsProc;
    pspage[6].pfnDlgProc = GameOptionsProc;

    /* If this is a vector game, add the vector prop sheet */
    if (maxPropSheets == NUM_PROPSHEETS)
    {
        pspage[NUM_PROPSHEETS - 1].pfnDlgProc = GameOptionsProc;
    }

    /* Create the Property sheet and display it */
    if (PropertySheet(&pshead) == -1)
    {
        char temp[100];
        DWORD dwError = GetLastError();
        sprintf(temp, "Propery Sheet Error %d %X", dwError, dwError);
        MessageBox(0, temp, "Error", IDOK);
    }
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Build CPU info string */
char *GameInfoCPU(UINT nIndex)
{
    int i;
    static char buf[1024] = "";

    ZeroMemory(buf, sizeof(buf));
    i = 0;
    while (i < MAX_CPU && drivers[nIndex]->drv->cpu[i].cpu_type)
    {
        if (drivers[nIndex]->drv->cpu[i].cpu_clock >= 1000000)
            sprintf(&buf[strlen(buf)], "%s %d.%06d MHz",
                    cputype_name(drivers[nIndex]->drv->cpu[i].cpu_type),
                    drivers[nIndex]->drv->cpu[i].cpu_clock / 1000000,
                    drivers[nIndex]->drv->cpu[i].cpu_clock % 1000000);
        else
            sprintf(&buf[strlen(buf)], "%s %d.%03d kHz",
                    cputype_name(drivers[nIndex]->drv->cpu[i].cpu_type),
                    drivers[nIndex]->drv->cpu[i].cpu_clock / 1000,
                    drivers[nIndex]->drv->cpu[i].cpu_clock % 1000);

        if (drivers[nIndex]->drv->cpu[i].cpu_type & CPU_AUDIO_CPU)
            strcat(buf, " (sound)");

        strcat(buf, "\n");

        i++;
    }

    return buf;
}

/* Build Sound system info string */
char *GameInfoSound(UINT nIndex)
{
    int i;
    static char buf[1024] = "";

    ZeroMemory(buf, sizeof(buf));
    i = 0;
    while (i < MAX_SOUND && drivers[nIndex]->drv->sound[i].sound_type)
    {
        if (1 < sound_num(&drivers[nIndex]->drv->sound[i]))
            sprintf(&buf[strlen(buf)], "%d x ", sound_num(&drivers[nIndex]->drv->sound[i]));

        sprintf(&buf[strlen(buf)], "%s", sound_name(&drivers[nIndex]->drv->sound[i]));

        if (sound_clock(&drivers[nIndex]->drv->sound[i]))
        {
            if (sound_clock(&drivers[nIndex]->drv->sound[i]) >= 1000000)
                sprintf(&buf[strlen(buf)], " %d.%06d MHz",
                        sound_clock(&drivers[nIndex]->drv->sound[i]) / 1000000,
                        sound_clock(&drivers[nIndex]->drv->sound[i]) % 1000000);
            else
                sprintf(&buf[strlen(buf)], " %d.%03d kHz",
                        sound_clock(&drivers[nIndex]->drv->sound[i]) / 1000,
                        sound_clock(&drivers[nIndex]->drv->sound[i]) % 1000);
        }

        strcat(buf,"\n");

        i++;
    }
    return buf;
}

/* Build Display info string */
char *GameInfoScreen(UINT nIndex)
{
    static char buf[1024];

    if (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR)
        strcpy(buf, "Vector Game");
    else
    {
        if (drivers[nIndex]->flags & ORIENTATION_SWAP_XY)
            sprintf(buf,"%d x %d (vert) %5.2f Hz",
                    drivers[nIndex]->drv->default_visible_area.max_y - drivers[nIndex]->drv->default_visible_area.min_y + 1,
                    drivers[nIndex]->drv->default_visible_area.max_x - drivers[nIndex]->drv->default_visible_area.min_x + 1,
                    drivers[nIndex]->drv->frames_per_second);
        else
            sprintf(buf,"%d x %d (horz) %5.2f Hz",
                    drivers[nIndex]->drv->default_visible_area.max_x - drivers[nIndex]->drv->default_visible_area.min_x + 1,
                    drivers[nIndex]->drv->default_visible_area.max_y - drivers[nIndex]->drv->default_visible_area.min_y + 1,
                    drivers[nIndex]->drv->frames_per_second);
    }
    return buf;
}

/* Build color information string */
char *GameInfoColors(UINT nIndex)
{
    static char buf[1024];

    ZeroMemory(buf, sizeof(buf));
    if (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR)
        strcpy(buf, "Vector Game");
    else
    {
        sprintf(buf, "%d colors ", drivers[nIndex]->drv->total_colors);
        if (drivers[nIndex]->drv->video_attributes & GAME_REQUIRES_16BIT)
            strcat(buf, "(16-bit required)");
        else
        if (drivers[nIndex]->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
            strcat(buf, "(dynamic)");
        else
            strcat(buf, "(static)");
    }
    return buf;
}

/* Build game status string */
char *GameInfoStatus(UINT nIndex)
{
    switch (GetHasRoms(nIndex))
    {
    case 0:
        return "ROMs missing";

    case 1:
        if (drivers[nIndex]->flags & GAME_BROKEN)
            return "Not working";
        if (drivers[nIndex]->flags & GAME_WRONG_COLORS)
            return "Colors are wrong";
        if (drivers[nIndex]->flags & GAME_IMPERFECT_COLORS)
            return "Imperfect Colors";
        else
            return "Working";

    default:
    case 2:
        return "Unknown";
    }
}

/* Build game manufacturer string */
char *GameInfoManufactured(UINT nIndex)
{
    static char buf[1024];

    sprintf(buf, "%s %s", drivers[nIndex]->year, drivers[nIndex]->manufacturer);
    return buf;
}

/* Build Game title string */
char *GameInfoTitle(UINT nIndex)
{
    static char buf[1024];

    if (nIndex == -1)
        strcpy(buf, "Global game options\nDefault options used by all games");
    else
        sprintf(buf, "%s\n\"%s\"", ModifyThe(drivers[nIndex]->description), drivers[nIndex]->name);
    return buf;
}

/* Build game clone infromation string */
char *GameInfoCloneOf(UINT nIndex)
{
    static char buf[1024];

    buf[0] = '\0';

    if (drivers[nIndex]->clone_of != 0
    &&  !(drivers[nIndex]->clone_of->flags & NOT_A_DRIVER))
    {
        sprintf(buf, "%s - \"%s\"",
                ModifyThe(drivers[nIndex]->clone_of->description),
                drivers[nIndex]->clone_of->name);
    }

    return buf;
}

/* Handle the information property page */
static INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
#if USE_SINGLELINE_TABCONTROL
        {
            HWND hWnd = PropSheet_GetTabControl(GetParent(hDlg));
            DWORD tabStyle = (GetWindowLong(hWnd,GWL_STYLE) & ~TCS_MULTILINE);
            SetWindowLong(hWnd,GWL_STYLE,tabStyle | TCS_SINGLELINE);
        }
#endif

        Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE),         GameInfoTitle(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_MANUFACTURED),  GameInfoManufactured(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_STATUS),        GameInfoStatus(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_CPU),           GameInfoCPU(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOUND),         GameInfoSound(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_SCREEN),        GameInfoScreen(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_COLORS),        GameInfoColors(iGame));
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_CLONEOF),       GameInfoCloneOf(iGame));
        if (drivers[iGame]->clone_of != 0
        && !(drivers[iGame]->clone_of->flags & NOT_A_DRIVER))
        {
            ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_SHOW);
        }
        else
        {
            ShowWindow(GetDlgItem(hDlg, IDC_PROP_CLONEOF_TEXT), SW_HIDE);
        }

        ShowWindow(hDlg, SW_SHOW);
        return 1;
    }
    return 0;
}

BOOL ReadSkipCtrl(HWND hWnd, UINT nCtrlID, int *value)
{
    HWND hCtrl;
    char buf[100];
    int  nValue = *value;

    hCtrl = GetDlgItem(hWnd, nCtrlID);
    if (hCtrl)
    {
        /* Skip lines control */
        Edit_GetText(hCtrl, buf, 100);
        if (sscanf(buf, "%d", value) != 1)
            *value = 0;
    }
    return (nValue == *value) ? FALSE : TRUE;
}

/* Handle all options property pages */
static INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
        /* Fill in the Game info at the top of the sheet */
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(iGame));
        InitializeOptions(hDlg);
        InitializeMisc(hDlg);

        PopulateControls(hDlg);
        OptionsToProp(hDlg, lpGameOpts);
        SetPropEnabledControls(hDlg);
        if (iGame == -1)
            ShowWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), SW_HIDE);
        else
            EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);

        EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), bReset);
        ShowWindow(hDlg, SW_SHOW);

        return 1;

    case WM_HSCROLL:
        /* slider changed */
        HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
        bUseDefaults = FALSE;
        bReset = TRUE;
        EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), TRUE);
        PropSheet_Changed(GetParent(hDlg), hDlg);
        break;

    case WM_COMMAND:
        {
            /* Below, 'changed' is used to signify the 'Apply'
             * button should be enabled.
             */
            WORD wID         = GET_WM_COMMAND_ID(wParam, lParam);
            HWND hWndCtrl    = GET_WM_COMMAND_HWND(wParam, lParam);
            WORD wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
            BOOL changed     = FALSE;

            switch (wID)
            {
            case IDC_FULLSCREEN:
                changed = ReadControl(hDlg, wID);
                break;

            case IDC_SOUNDTYPE:
                if (wNotifyCode == CBN_SELCHANGE)
                    changed = ReadControl(hDlg, wID);
                break;

            case IDC_DEPTH:
                if (wNotifyCode == LBN_SELCHANGE)
                {
                    DepthSelectionChange(hDlg, hWndCtrl);
                    changed = TRUE;
                }
                break;

            case IDC_SIZES:
            case IDC_FRAMESKIP:
            case IDC_DEFAULT_INPUT:
            case IDC_ROTATE:
            case IDC_SAMPLERATE:
                if (wNotifyCode == CBN_SELCHANGE)
                    changed = TRUE;
                break;

            case IDC_SKIP_LINES:
                if (wNotifyCode == EN_UPDATE)
                    changed = ReadSkipCtrl(hDlg, wID, &lpGameOpts->skip_lines);
                break;
            case IDC_SKIP_COLUMNS:
                if (wNotifyCode == EN_UPDATE)
                    changed = ReadSkipCtrl(hDlg, wID, &lpGameOpts->skip_columns);
                break;

            case IDC_TRIPLE_BUFFER:
                changed = ReadControl(hDlg, wID);
                break;

            case IDC_PROP_RESET:
                if (wNotifyCode != BN_CLICKED)
                    break;

                memcpy(lpGameOpts, &origGameOpts, sizeof(options_type));
                BuildDataMap();
                PopulateControls(hDlg);
                OptionsToProp(hDlg, lpGameOpts);
                SetPropEnabledControls(hDlg);
                bReset = FALSE;
                PropSheet_UnChanged(GetParent(hDlg), hDlg);
                bUseDefaults = lpGameOpts->use_default;
                EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);
                break;

            case IDC_USE_DEFAULT:
                if (iGame != -1)
                {
                    lpGameOpts->use_default = TRUE;
                    lpGameOpts = GetGameOptions(iGame);
                    bUseDefaults = lpGameOpts->use_default;
                    BuildDataMap();
                    PopulateControls(hDlg);
                    OptionsToProp(hDlg, lpGameOpts);
                    SetPropEnabledControls(hDlg);
                    if (origGameOpts.use_default != bUseDefaults)
                    {
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        bReset = TRUE;
                    }
                    else
                    {
                        PropSheet_UnChanged(GetParent(hDlg), hDlg);
                        bReset = FALSE;
                    }
                    EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);
                }

                break;

            case IDC_DISPLAYS:
                if (wNotifyCode == LBN_SELCHANGE)
                {
                    DisplayChange(hDlg);
                    changed = TRUE;
                }
                break;

            default:
                if (wNotifyCode == BN_CLICKED)
                {
                    switch (wID)
                    {
                    case IDC_SCANLINES:
                        if (Button_GetCheck(GetDlgItem(hDlg, IDC_SCANLINES)))
                        {
                            Button_SetCheck(GetDlgItem(hDlg, IDC_VSCANLINES), FALSE);
                            Button_SetCheck(GetDlgItem(hDlg, IDC_USEBLIT),    FALSE);
                        }
                        break;
                    case IDC_VSCANLINES:
                        if (Button_GetCheck(GetDlgItem(hDlg, IDC_VSCANLINES)))
                        {
                            Button_SetCheck(GetDlgItem(hDlg, IDC_SCANLINES), FALSE);
                            Button_SetCheck(GetDlgItem(hDlg, IDC_USEBLIT),   FALSE);
                        }
                        break;
                    case IDC_USEBLIT:
                        if (Button_GetCheck(GetDlgItem(hDlg, IDC_USEBLIT)))
                        {
                            Button_SetCheck(GetDlgItem(hDlg, IDC_VSCANLINES),FALSE);
                            Button_SetCheck(GetDlgItem(hDlg, IDC_SCANLINES), FALSE);
                        }
                        break;
                    }
                    changed = TRUE;
                }
            }

            /* Enable the apply button */
            if (changed == TRUE)
            {
                lpGameOpts->use_default = bUseDefaults = FALSE;
                PropSheet_Changed(GetParent(hDlg), hDlg);
                bReset = TRUE;
                EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);
            }
            SetPropEnabledControls(hDlg);
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            /* Initialize the controls. */
            PopulateControls(hDlg);
            OptionsToProp(hDlg, lpGameOpts);
            SetPropEnabledControls(hDlg);
            EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);
            break;

        case PSN_APPLY:
            /* Save and apply the options here */
            PropToOptions(hDlg, lpGameOpts);
            ReadControls(hDlg);
            lpGameOpts->use_default = bUseDefaults;
            if (iGame == -1)
                lpGameOpts = GetDefaultOptions();
            else
                lpGameOpts = GetGameOptions(iGame);

            memcpy(&origGameOpts, lpGameOpts, sizeof(options_type));
            BuildDataMap();
            PopulateControls(hDlg);
            OptionsToProp(hDlg, lpGameOpts);
            SetPropEnabledControls(hDlg);
            EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (bUseDefaults) ? FALSE : TRUE);
            bReset = FALSE;
            PropSheet_UnChanged(GetParent(hDlg), hDlg);
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;

        case PSN_KILLACTIVE:
            /* Save Changes to the options here. */
            ReadControls(hDlg);
            ResetDataMap();
            lpGameOpts->use_default = bUseDefaults;
            PropToOptions(hDlg, lpGameOpts);
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;

        case PSN_RESET:
            /* Reset to the original values. Disregard changes */
            memcpy(lpGameOpts, &origGameOpts, sizeof(options_type));
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;

        case PSN_HELP:
            /* User wants help for this property page */
            break;
        }
        break;

    case WM_HELP:
        /* User clicked the ? from the upper right on a control */
        WinHelp(((LPHELPINFO) lParam)->hItemHandle, "mame32.hlp", HELP_WM_HELP, GetHelpIDs());
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, "mame32.hlp", HELP_CONTEXTMENU, GetHelpIDs());
        break;

    }
    EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), bReset);

    return 0;
}

static INT_PTR CALLBACK GameDisplayOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
        {
            HBITMAP hBitmap;
            hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),
                                         MAKEINTRESOURCE(IDB_PROP_DISPLAY),
                                         IMAGE_BITMAP, 0, 0,
                                         LR_SHARED | LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
            SendMessage(GetDlgItem(hDlg, IDC_PROP_DISPLAY), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        }
    }

    return GameOptionsProc(hDlg, Msg, wParam, lParam);
}

/* Read controls that are not handled in the DataMap */
void PropToOptions(HWND hWnd, options_type *o)
{
    char buf[100];
    HWND hCtrl;

    o->use_default = bUseDefaults;

    /* Game Display Options box */
    hCtrl = GetDlgItem(hWnd, IDC_SIZES);
    if (hCtrl)
    {
        int index;

        /* Screen size control */
        index = ComboBox_GetCurSel(hCtrl);
        o->display_best_fit = (index > 0) ? FALSE : TRUE;

        if (! o->display_best_fit)
        {
            DWORD   w, h;

            ComboBox_GetText(hCtrl, buf, 100);
            if (sscanf(buf, "%d x %d", &w, &h) == 2)
            {
                o->width  = w;
                o->height = h;
            }
            else
            {
                o->display_best_fit = TRUE;
            }
        }
    }

    hCtrl = GetDlgItem(hWnd, IDC_SKIP_LINES);
    if (hCtrl)
    {
        /* Skip lines control */
        Edit_GetText(hCtrl, buf, 100);
        if (sscanf(buf, "%d", &o->skip_lines) != 1)
            o->skip_lines = 0;
    }

    hCtrl = GetDlgItem(hWnd, IDC_SKIP_COLUMNS);
    if (hCtrl)
    {
        /* Skip columns control */
        Edit_GetText(hCtrl, buf, 100);
        if (sscanf(buf, "%d", &o->skip_columns) != 1)
            o->skip_columns = 0;
    }
}

/* Populate controls that are not handled in the DataMap */
void OptionsToProp(HWND hWnd, options_type *o)
{
    HWND hCtrl;
    char buf[100];

    hCtrl = GetDlgItem(hWnd, IDC_SIZES);
    if (hCtrl)
    {
        /* Screen size drop down list */
        if (o->display_best_fit)
        {
            ComboBox_SetCurSel(hCtrl, 0);
        }
        else
        {
            /* Select the mode in the list. */
            int nSelection = 0;
            int nCount = 0;

            /* Get the number of items in the control */
            nCount = ComboBox_GetCount(hCtrl);

            while (0 < nCount--)
            {
                int     nWidth, nHeight;

                /* Get the screen size */
                ComboBox_GetLBText(hCtrl, nCount, buf);

                if (sscanf(buf, "%d x %d", &nWidth, &nHeight) == 2)
                {
                    /* If we match, set nSelection to the right value */
                    if (o->width  == nWidth &&  o->height == nHeight)
                    {
                        nSelection = nCount;
                        break;
                    }
                }
            }
            ComboBox_SetCurSel(hCtrl, nSelection);
        }
    }
    /* Gamma */
    hCtrl = GetDlgItem(hWnd, IDC_GAMMADISP);
    if (hCtrl)
    {
        /* Set the static display to the new value */
        sprintf(buf, "%03.02f", o->gamma);
        Static_SetText(hCtrl, buf);
    }

    hCtrl = GetDlgItem(hWnd, IDC_BRIGHTNESSDISP);
    if (hCtrl)
    {
        /* Set the static display to the new value */
        sprintf(buf, "%3d%%", o->brightness);
        Static_SetText(hCtrl, buf);
    }

    hCtrl = GetDlgItem(hWnd, IDC_FLICKERDISP);
    if (hCtrl)
    {
        /* Now set the static flicker display the new value to match */
        sprintf(buf, "%3d", o->flicker);
        Static_SetText(hCtrl, buf);
    }

    hCtrl = GetDlgItem(hWnd, IDC_BEAMDISP);
    if (hCtrl)
    {
        /* Set the static display to the new value */
        sprintf(buf, "%03.02f", o->beam);
        Static_SetText(hCtrl, buf);
    }

    hCtrl = GetDlgItem(hWnd, IDC_VOLUMEDISP);
    if (hCtrl)
    {
        /* Set the static display to the new value */
        sprintf(buf, "%ddB", - o->volume);
        Static_SetText(hCtrl, buf);
    }

    /* Send message to skip lines control */
    SendDlgItemMessage(hWnd, IDC_SKIP_LINES_SPIN, UDM_SETPOS, 0,
                       (LPARAM)MAKELONG(o->skip_lines, 0));

    /* Send message to skip columns control */
    SendDlgItemMessage(hWnd, IDC_SKIP_COLUMNS_SPIN, UDM_SETPOS, 0,
                       (LPARAM)MAKELONG(o->skip_columns, 0));

    /* update list of modes for this display by simulating a display change */
    DisplayChange(hWnd);

}

/* Adjust controls - tune them to the currently selected game */
void SetPropEnabledControls(HWND hWnd)
{
    HWND hCtrl;
    int  nIndex;
    int  screen, sound;

    nIndex = iGame;

    screen = !bFullScreen;

    /* Double Vector, Direct draw and Skiplines */
    if (0 == screen)
    {
        Button_Enable(GetDlgItem(hWnd, IDC_VECTOR_DOUBLE), (nIndex == -1) ? TRUE : FALSE);
        Button_Enable(GetDlgItem(hWnd, IDC_WINDOW_DDRAW),  (nIndex == -1) ? TRUE : FALSE);

        ComboBox_Enable(GetDlgItem(hWnd, IDC_SIZES),      TRUE);
        Edit_Enable( GetDlgItem(hWnd, IDC_SKIP_LINES),    TRUE);
        Edit_Enable( GetDlgItem(hWnd, IDC_SKIP_COLUMNS),  TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_SIZETEXT),      TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_SKIPLINESTEXT), TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_SKIPCOLSTEXT),  TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_TRIPLE_BUFFER), TRUE);
    }
    else
    {
        ComboBox_Enable(GetDlgItem(hWnd, IDC_SIZES),      FALSE);
        Edit_Enable( GetDlgItem(hWnd, IDC_SKIP_LINES),    FALSE);
        Edit_Enable( GetDlgItem(hWnd, IDC_SKIP_COLUMNS),  FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_SIZETEXT),      FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_SKIPLINESTEXT), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_SKIPCOLSTEXT),  FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_TRIPLE_BUFFER), FALSE);

        if (nIndex == -1 || drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR)
            Button_Enable(GetDlgItem(hWnd, IDC_VECTOR_DOUBLE), TRUE);
        else
            Button_Enable(GetDlgItem(hWnd, IDC_VECTOR_DOUBLE), FALSE);

        Button_Enable(GetDlgItem(hWnd, IDC_WINDOW_DDRAW), TRUE);
    }

    /* DirectDraw in window isn't working, so disable option. */
    Button_Enable(GetDlgItem(hWnd, IDC_WINDOW_DDRAW), FALSE);


    ComboBox_Enable(GetDlgItem(hWnd, IDC_DISPLAYS), (screen == FALSE) &&
                    (DirectDraw_GetNumDisplays() > 1));

    if (nIndex == -1 || Joystick.Available() || GameUsesTrackball(nIndex))
        EnableWindow(GetDlgItem(hWnd, IDC_INPUTDEVTEXT), TRUE);
    else
        EnableWindow(GetDlgItem(hWnd, IDC_INPUTDEVTEXT), FALSE);

    hCtrl = GetDlgItem(hWnd, IDC_JOYSTICK);
    if (hCtrl)
        Button_Enable(hCtrl, Joystick.Available());

    hCtrl = GetDlgItem(hWnd, IDC_DIJOYSTICK);
    if (hCtrl)
        Button_Enable(hCtrl, DIJoystick.Available());

    hCtrl = GetDlgItem(hWnd, IDC_DIKEYBOARD);
    if (hCtrl)
        Button_Enable(hCtrl, DIKeyboard_Available());

    /* Trackball / Mouse options */
    if (nIndex == -1 || GameUsesTrackball(nIndex))
    {
        Button_Enable(GetDlgItem(hWnd, IDC_AI_MOUSE),     TRUE);
        Button_Enable(GetDlgItem(hWnd, IDC_AI_JOYSTICK),  TRUE);
        EnableWindow(GetDlgItem(hWnd, IDC_TRACKSPINTEXT), TRUE);
    }
    else
    {
        Button_Enable(GetDlgItem(hWnd, IDC_AI_MOUSE),     FALSE);
        Button_Enable(GetDlgItem(hWnd, IDC_AI_JOYSTICK),  FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_TRACKSPINTEXT), FALSE);
    }

    hCtrl = GetDlgItem(hWnd, IDC_AI_MOUSE);
    if (hCtrl)
    {
        if (Button_GetCheck(hCtrl))
            Button_SetCheck(GetDlgItem(hWnd, IDC_AI_JOYSTICK), FALSE);
        else
            Button_SetCheck(GetDlgItem(hWnd, IDC_AI_JOYSTICK), TRUE);
    }

    /* Sound options */
    hCtrl = GetDlgItem(hWnd, IDC_SOUNDTYPE);
    if (hCtrl)
    {
        sound = ComboBox_GetCurSel(hCtrl);
        ComboBox_Enable(GetDlgItem(hWnd, IDC_SAMPLERATE), (sound != 0));

        EnableWindow(GetDlgItem(hWnd, IDC_VOLUME),     (sound != 0));
        EnableWindow(GetDlgItem(hWnd, IDC_BITTEXT),    (sound != 0));
        EnableWindow(GetDlgItem(hWnd, IDC_RATETEXT),   (sound != 0));
        EnableWindow(GetDlgItem(hWnd, IDC_VOLUMEDISP), (sound != 0));
    }
    /* Double size */
    if (Button_GetCheck(GetDlgItem(hWnd, IDC_DOUBLE)))
    {
        Button_Enable(GetDlgItem(hWnd, IDC_SCANLINES),  TRUE);
        Button_Enable(GetDlgItem(hWnd, IDC_VSCANLINES), TRUE);
    }
    else
    {
        Button_Enable(GetDlgItem(hWnd, IDC_SCANLINES),  FALSE);
        Button_Enable(GetDlgItem(hWnd, IDC_VSCANLINES), FALSE);
    }

    /* If in FULL_SCREEN mode */
    if ((0 == screen)
    &&   Button_GetCheck(GetDlgItem(hWnd, IDC_DOUBLE))
    &&  !Button_GetCheck(GetDlgItem(hWnd, IDC_SCANLINES))
    &&  !Button_GetCheck(GetDlgItem(hWnd, IDC_VSCANLINES)))
    {
        Button_Enable(GetDlgItem(hWnd, IDC_USEBLIT), TRUE);
    }
    else
    {
        Button_Enable(GetDlgItem(hWnd, IDC_USEBLIT), FALSE);
    }

    /* Dirty rectangles */
    if (nIndex == -1
    ||  (drivers[nIndex]->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
    ||  (drivers[nIndex]->drv->video_attributes & VIDEO_TYPE_VECTOR))
    {
        Button_Enable(GetDlgItem(hWnd, IDC_DIRTY), TRUE);
    }
    else
    {
        Button_Enable(GetDlgItem(hWnd, IDC_DIRTY), FALSE);
        Button_SetCheck(GetDlgItem(hWnd, IDC_DIRTY), FALSE);
    }

    /* Turn off MMX if it's not detected */
    if (MAME32App.m_bMMXDetected)
    {
        Button_Enable(GetDlgItem(hWnd, IDC_DISABLE_MMX), TRUE);
    }
    else
    {
        Button_SetCheck(GetDlgItem(hWnd, IDC_DISABLE_MMX), TRUE);
        Button_Enable(GetDlgItem(hWnd, IDC_DISABLE_MMX), FALSE);
    }

    ComboBox_Enable(GetDlgItem(hWnd, IDC_DISPLAYTYPE), TRUE);

    if (Button_GetCheck(GetDlgItem(hWnd, IDC_AUTOFRAMESKIP)))
        EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), FALSE);
    else
        EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), TRUE);

    SetStereoEnabled(hWnd, nIndex);
    SetYM3812Enabled(hWnd, nIndex);
    SetSamplesEnabled(hWnd, nIndex);
}

/**************************************************************
 * Control Helper functions for data exchange
 **************************************************************/
void AssignSampleRate(HWND hWnd)
{
    switch (nSampleRate)
    {
        case 0:  lpGameOpts->sample_rate = 11025; break;
        case 1:  lpGameOpts->sample_rate = 22050; break;
        case 2:  lpGameOpts->sample_rate = 44100; break;
        default: lpGameOpts->sample_rate = 22050; break;
    }
}

void AssignVolume(HWND hWnd)
{
    lpGameOpts->volume = 32 - nVolume;
}

void AssignFullScreen(HWND hWnd)
{
    lpGameOpts->is_window = (bFullScreen) ? FALSE : TRUE;
}

void AssignScale(HWND hWnd)
{
    lpGameOpts->scale = (bDouble) ? 2 : 1;
}

void AssignDepth(HWND hWnd)
{
    lpGameOpts->depth = 8 * (nDepth);
    UpdateDisplayModeUI(hWnd, lpGameOpts->depth);
}

void AssignGamma(HWND hWnd)
{
    /* Gamma control */
    lpGameOpts->gamma = nGamma / 20.0 + 0.5;
}

void AssignBeam(HWND hWnd)
{
    lpGameOpts->beam = nBeam / 20.0 + 1.0;
}

/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
void ResetDataMap(void)
{
    nSampleRate = (lpGameOpts->sample_rate / 22050) % 3;
    nVolume     = 32 - lpGameOpts->volume;
    bDouble     = (lpGameOpts->scale == 2) ? TRUE : FALSE;
    nDepth      = lpGameOpts->depth / 8;
    bFullScreen = (lpGameOpts->is_window) ? FALSE : TRUE;
    nGamma      = (int)((lpGameOpts->gamma - 0.5) * 20.0);
    nBeam       = (int)((lpGameOpts->beam  - 1.0) * 20.0);
}

/* Build the control mapping by adding all needed information to the DataMap */
void BuildDataMap(void)
{
    InitDataMap();

    ResetDataMap();

    DataMapAdd(IDC_FULLSCREEN,    DM_BOOL, CT_BUTTON,   &bFullScreen,  0, 0, AssignFullScreen);
    DataMapAdd(IDC_SAMPLERATE,    DM_INT,  CT_COMBOBOX, &nSampleRate,  0, 0, AssignSampleRate);
    DataMapAdd(IDC_VOLUME,        DM_INT,  CT_SLIDER,   &nVolume,      0, 0, AssignVolume);
    DataMapAdd(IDC_DOUBLE,        DM_BOOL, CT_BUTTON,   &bDouble,      0, 0, AssignScale);
    DataMapAdd(IDC_DEPTH,         DM_INT,  CT_COMBOBOX, &nDepth,       0, 0, AssignDepth);
    DataMapAdd(IDC_GAMMA,         DM_INT,  CT_SLIDER,   &nGamma,       0, 0, AssignGamma);
    DataMapAdd(IDC_BEAM,          DM_INT,  CT_SLIDER,   &nBeam,        0, 0, AssignBeam);

    DataMapAdd(IDC_DISPLAYS,      DM_INT,  CT_COMBOBOX, &lpGameOpts->display_monitor, 0, 0, 0);
    DataMapAdd(IDC_VECTOR_DOUBLE, DM_BOOL, CT_BUTTON,   &lpGameOpts->double_vector,   0, 0, 0);
    DataMapAdd(IDC_WINDOW_DDRAW,  DM_BOOL, CT_BUTTON,   &lpGameOpts->window_ddraw,    0, 0, 0);
    DataMapAdd(IDC_DIRTY,         DM_BOOL, CT_BUTTON,   &lpGameOpts->use_dirty,       0, 0, 0);
    DataMapAdd(IDC_SCANLINES,     DM_BOOL, CT_BUTTON,   &lpGameOpts->hscan_lines,     0, 0, 0);
    DataMapAdd(IDC_VSCANLINES,    DM_BOOL, CT_BUTTON,   &lpGameOpts->vscan_lines,     0, 0, 0);
    DataMapAdd(IDC_USEBLIT,       DM_BOOL, CT_BUTTON,   &lpGameOpts->use_blit,        0, 0, 0);
    DataMapAdd(IDC_DISABLE_MMX,   DM_BOOL, CT_BUTTON,   &lpGameOpts->disable_mmx,     0, 0, 0);
    DataMapAdd(IDC_FLIPX,         DM_BOOL, CT_BUTTON,   &lpGameOpts->flipx,           0, 0, 0);
    DataMapAdd(IDC_FLIPY,         DM_BOOL, CT_BUTTON,   &lpGameOpts->flipy,           0, 0, 0);
    DataMapAdd(IDC_ANTIALIAS,     DM_BOOL, CT_BUTTON,   &lpGameOpts->antialias,       0, 0, 0);
    DataMapAdd(IDC_TRANSLUCENCY,  DM_BOOL, CT_BUTTON,   &lpGameOpts->translucency,    0, 0, 0);
    DataMapAdd(IDC_AI_MOUSE,      DM_BOOL, CT_BUTTON,   &lpGameOpts->use_ai_mouse,    0, 0, 0);
    DataMapAdd(IDC_DIKEYBOARD,    DM_BOOL, CT_BUTTON,   &lpGameOpts->di_keyboard,     0, 0, 0);
    DataMapAdd(IDC_DIJOYSTICK,    DM_BOOL, CT_BUTTON,   &lpGameOpts->di_joystick,     0, 0, 0);
    DataMapAdd(IDC_JOYSTICK,      DM_BOOL, CT_BUTTON,   &lpGameOpts->use_joystick,    0, 0, 0);
    DataMapAdd(IDC_DEFAULT_INPUT, DM_INT,  CT_COMBOBOX, &lpGameOpts->default_input,   0, 0, 0);
    DataMapAdd(IDC_USE_FM_YM3812, DM_BOOL, CT_BUTTON,   &lpGameOpts->fm_ym3812,       0, 0, 0);
    DataMapAdd(IDC_USE_FILTER,    DM_BOOL, CT_BUTTON,   &lpGameOpts->use_filter,      0, 0, 0);
    DataMapAdd(IDC_STEREO,        DM_BOOL, CT_BUTTON,   &lpGameOpts->stereo,          0, 0, 0);
    DataMapAdd(IDC_CHEAT,         DM_BOOL, CT_BUTTON,   &lpGameOpts->cheat,           0, 0, 0);
    DataMapAdd(IDC_AUTO_PAUSE,    DM_BOOL, CT_BUTTON,   &lpGameOpts->auto_pause,      0, 0, 0);
    DataMapAdd(IDC_LOG,           DM_BOOL, CT_BUTTON,   &lpGameOpts->error_log,       0, 0, 0);
    DataMapAdd(IDC_ARTWORK,       DM_BOOL, CT_BUTTON,   &lpGameOpts->use_artwork,     0, 0, 0);
    DataMapAdd(IDC_SAMPLES ,      DM_BOOL, CT_BUTTON,   &lpGameOpts->use_samples,     0, 0, 0);
    DataMapAdd(IDC_SOUNDTYPE,     DM_INT,  CT_COMBOBOX, &lpGameOpts->sound,           0, 0, 0);
    DataMapAdd(IDC_AUTOFRAMESKIP, DM_BOOL, CT_BUTTON,   &lpGameOpts->auto_frame_skip, 0, 0, 0);
    DataMapAdd(IDC_FRAMESKIP,     DM_INT,  CT_COMBOBOX, &lpGameOpts->frame_skip,      0, 0, 0);
    DataMapAdd(IDC_ROTATE,        DM_INT,  CT_COMBOBOX, &lpGameOpts->rotate,          0, 0, 0);
    DataMapAdd(IDC_BRIGHTNESS,    DM_INT,  CT_SLIDER,   &lpGameOpts->brightness,      0, 0, 0);
    DataMapAdd(IDC_FLICKER,       DM_INT,  CT_SLIDER,   &lpGameOpts->flicker,         0, 0, 0);
    DataMapAdd(IDC_TRIPLE_BUFFER, DM_BOOL, CT_BUTTON,   &lpGameOpts->triple_buffer,   0, 0, 0);
}

static void SetStereoEnabled(HWND hWnd, int index)
{
    BOOL enabled = FALSE;
    HWND hCtrl;

    hCtrl = GetDlgItem(hWnd, IDC_STEREO);
    if (hCtrl)
    {
        if (index == -1 || drivers[index]->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
            enabled = TRUE;

        EnableWindow(hCtrl, enabled);
        if (!enabled)
            Button_SetCheck(hCtrl, FALSE);
    }
}

static void SetYM3812Enabled(HWND hWnd, int index)
{
    int i;
    BOOL enabled;
    HWND hCtrl;

    hCtrl = GetDlgItem(hWnd, IDC_USE_FM_YM3812);
    if (hCtrl)
    {
        enabled = FALSE;
        for (i = 0; i < MAX_SOUND; i++)
        {
            if (index == -1
#if HAS_YM3812
            ||  drivers[index]->drv->sound[i].sound_type == SOUND_YM3812
#endif
#if HAS_YM3526
            ||  drivers[index]->drv->sound[i].sound_type == SOUND_YM3526
#endif
#if HAS_YM2413
            ||  drivers[index]->drv->sound[i].sound_type == SOUND_YM2413
#endif
            )
                enabled = TRUE;
        }

        EnableWindow(hCtrl, enabled);
        if (!enabled)
            Button_SetCheck(hCtrl, FALSE);
    }
}

static void SetSamplesEnabled(HWND hWnd, int index)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)
    int i;
    BOOL enabled = FALSE;
    HWND hCtrl;

    hCtrl = GetDlgItem(hWnd, IDC_SAMPLES);
    if (hCtrl)
    {
        for (i = 0; i < MAX_SOUND; i++)
        {
            if (index == -1
#if (HAS_SAMPLES)
                ||  drivers[index]->drv->sound[i].sound_type == SOUND_SAMPLES
#endif
#if (HAS_VLM5030)
                ||  drivers[index]->drv->sound[i].sound_type == SOUND_VLM5030
#endif
               )
                enabled = TRUE;
        }

        EnableWindow(hCtrl, enabled);
        if (!enabled)
            Button_SetCheck(hCtrl, FALSE);
    }
#endif
}

/* Moved here cause it's called in a few places */
static void InitializeOptions(HWND hDlg)
{
    InitializeDepthUI(hDlg);
    InitializeDisplayTypeUI(hDlg);
    InitializeDisplayModeUI(hDlg);
    InitializeSoundUI(hDlg);
    InitializeSkippingUI(hDlg);
    InitializeRotateUI(hDlg);
    InitializeDefaultInputUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
    Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK),   Joystick.Available());
    Button_Enable(GetDlgItem(hDlg, IDC_DIJOYSTICK), DIJoystick.Available());
    Button_Enable(GetDlgItem(hDlg, IDC_DIKEYBOARD), DIKeyboard_Available());

    Edit_LimitText(GetDlgItem(hDlg, IDC_SKIP_LINES), 4);
    SendDlgItemMessage(hDlg, IDC_SKIP_LINES_SPIN, UDM_SETRANGE, 0,
                       (LPARAM)MAKELONG(9999, 0));
    SendDlgItemMessage(hDlg, IDC_SKIP_LINES_SPIN, UDM_SETPOS, 0,
                       (LPARAM)MAKELONG(0, 0));
    SendDlgItemMessage(hDlg, IDC_SKIP_LINES_SPIN, UDM_SETBUDDY,
                       (WPARAM)GetDlgItem(hDlg, IDC_SKIP_LINES), 0);

    Edit_LimitText(GetDlgItem(hDlg, IDC_SKIP_COLUMNS), 4);
    SendDlgItemMessage(hDlg, IDC_SKIP_COLUMNS_SPIN, UDM_SETRANGE, 0,
                       (LPARAM)MAKELONG(9999, 0));
    SendDlgItemMessage(hDlg, IDC_SKIP_COLUMNS_SPIN, UDM_SETPOS, 0,
                       (LPARAM)MAKELONG(0, 0));
    SendDlgItemMessage(hDlg, IDC_SKIP_COLUMNS_SPIN, UDM_SETBUDDY,
                       (WPARAM)GetDlgItem(hDlg, IDC_SKIP_COLUMNS), 0);

    SendMessage(GetDlgItem(hDlg, IDC_GAMMA), TBM_SETRANGE,
                (WPARAM)FALSE,
                (LPARAM)MAKELONG(0, 30)); /* 0.50 - 2.00 in .05 increments */

    SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETRANGE,
                (WPARAM)FALSE,
                (LPARAM)MAKELONG(0, 100));

    SendMessage(GetDlgItem(hDlg, IDC_FLICKER), TBM_SETRANGE,
                (WPARAM)FALSE,
                (LPARAM)MAKELONG(0, 100));

    SendMessage(GetDlgItem(hDlg, IDC_BEAM), TBM_SETRANGE,
                (WPARAM)FALSE,
                (LPARAM)MAKELONG(0, 300)); /* 1.00 - 16.00 in .05 increments */

    SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETRANGE,
                (WPARAM)FALSE,
                (LPARAM)MAKELONG(0, 32)); /* -32 - 0 */

}

static void OptOnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
    {
        FlickerSelectionChange(hwnd);
    }
    else
    if (hwndCtl == GetDlgItem(hwnd, IDC_GAMMA))
    {
        GammaSelectionChange(hwnd);
    }
    else
    if (hwndCtl == GetDlgItem(hwnd, IDC_BRIGHTNESS))
    {
        BrightnessSelectionChange(hwnd);
    }
    else
    if (hwndCtl == GetDlgItem(hwnd, IDC_BEAM))
    {
        BeamSelectionChange(hwnd);
    }
    else
    if (hwndCtl == GetDlgItem(hwnd, IDC_VOLUME))
    {
        VolumeSelectionChange(hwnd);
    }
}

/* Handle changes to the Flicker slider */
static void FlickerSelectionChange(HWND hwnd)
{
    char buf[100];
    int  nValue;

    /* Get the current value of the control */
    nValue = SendMessage(GetDlgItem(hwnd, IDC_FLICKER), TBM_GETPOS, 0, 0);

    /* Now set the static flicker display the new value to match */
    sprintf(buf, "%3d", nValue);
    Static_SetText(GetDlgItem(hwnd, IDC_FLICKERDISP), buf);
}

/* Handle changes to the Beam slider */
static void BeamSelectionChange(HWND hwnd)
{
    char buf[100];
    UINT nValue;
    double dBeam;

    /* Get the current value of the control */
    nValue = SendMessage(GetDlgItem(hwnd, IDC_BEAM), TBM_GETPOS, 0, 0);

    dBeam = nValue / 20.0 + 1.0;

    /* Set the static display to the new value */
    sprintf(buf, "%03.02f", dBeam);
    Static_SetText(GetDlgItem(hwnd, IDC_BEAMDISP), buf);
}


/* Handle changes to the Gamma slider */
static void GammaSelectionChange(HWND hwnd)
{
    char buf[100];
    UINT nValue;
    double dGamma;

    /* Get the current value of the control */
    nValue = SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);

    dGamma = nValue / 20.0 + 0.5;

    /* Set the static display to the new value */
    sprintf(buf, "%03.02f", dGamma);
    Static_SetText(GetDlgItem(hwnd, IDC_GAMMADISP), buf);
}

/* Handle changes to the Brightness slider */
static void BrightnessSelectionChange(HWND hwnd)
{
    char buf[100];
    int  nValue;

    /* Get the current value of the control */
    nValue = SendMessage(GetDlgItem(hwnd, IDC_BRIGHTNESS), TBM_GETPOS, 0, 0);

    /* Set the static display to the new value */
    sprintf(buf, "%3d%%", nValue);
    Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTNESSDISP), buf);
}

/* Handle changes to the Color Depth drop down */
static void DepthSelectionChange(HWND hWnd, HWND hWndCtrl)
{
    int     nCurSelection;

    nCurSelection = ComboBox_GetCurSel(hWndCtrl);
    if (nCurSelection != CB_ERR)
    {
        int     nDepth;

        nDepth = ComboBox_GetCurSel(hWndCtrl);
        nDepth *= 8;
        UpdateDisplayModeUI(hWnd, nDepth);
    }
}

/* Handle changes to the current display to use */
static void DisplayChange(HWND hWnd)
{
    int     nCurSelection;

    nCurSelection = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_DISPLAYS));
    if (nCurSelection != CB_ERR)
    {
       /* reset modes with the newly selected display */
       DirectDraw_CreateByIndex(nCurSelection);
       UpdateDisplayModeUI(hWnd, lpGameOpts->depth);

    }
}

/* Handle changes to the Volume slider */
static void VolumeSelectionChange(HWND hwnd)
{
    char buf[100];
    int  nValue;

    /* Get the current value of the control */
    nValue = SendMessage(GetDlgItem(hwnd, IDC_VOLUME), TBM_GETPOS, 0, 0);

    /* Set the static display to the new value */
    sprintf(buf, "%ddB", nValue - 32);
    Static_SetText(GetDlgItem(hwnd, IDC_VOLUMEDISP), buf);
}

/* Adjust possible choices in the Screen Size drop down */
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth)
{
    int                     i;
    char                    buf[100];
    struct tDisplayModes*   pDisplayModes;
    int                     nPick;
    int                     nCount = 0;
    int                     nSelection = 0;
    DWORD                   w = 0, h = 0;
    HWND                    hCtrl = GetDlgItem(hwnd, IDC_SIZES);

    if (!hCtrl)
        return;

    /* Find out what is currently selected if anything. */
    nPick = ComboBox_GetCurSel(hCtrl);
    if (nPick != 0 && nPick != CB_ERR)
    {
        ComboBox_GetText(GetDlgItem(hwnd, IDC_SIZES), buf, 100);
        if (sscanf(buf, "%d x %d", &w, &h) != 2)
        {
            w = 0;
            h = 0;
        }
    }

    /* Auto mode. Find out what depth is ok for this game. */
    if (dwDepth == 0)
    {
        if (iGame == -1)
        {
            dwDepth = 8; /* punt */
        }
        else
        {
            if (drivers[iGame]->flags & GAME_REQUIRES_16BIT)
                dwDepth = 16;
            else
                dwDepth = 8;
        }
    }

    /* Remove all items in the list. */
    ComboBox_ResetContent(hCtrl);

    ComboBox_AddString(hCtrl, "Auto size");

    pDisplayModes = DirectDraw_GetDisplayModes();

    for (i = 0; i < pDisplayModes->m_nNumModes; i++)
    {
        if (pDisplayModes->m_Modes[i].m_dwBPP == dwDepth)
        {
            nCount++;
            sprintf(buf, "%i x %i", pDisplayModes->m_Modes[i].m_dwWidth,
                                    pDisplayModes->m_Modes[i].m_dwHeight);
            ComboBox_AddString(hCtrl, buf);

            if (w == pDisplayModes->m_Modes[i].m_dwWidth
            &&  h == pDisplayModes->m_Modes[i].m_dwHeight)
                nSelection = nCount;
        }
    }

    ComboBox_SetCurSel(hCtrl, nSelection);
}

/* Initialize the Display options to auto mode */
static void InitializeDisplayModeUI(HWND hwnd)
{
    UpdateDisplayModeUI(hwnd, 0);
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
    int     sound;
    HWND    hCtrl;

    sound = SOUND_NONE;

    hCtrl = GetDlgItem(hwnd, IDC_SOUNDTYPE);
    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "No Sound");
#if defined(MIDAS)
        ComboBox_AddString(hCtrl, "MIDAS");
#endif
        ComboBox_AddString(hCtrl, "DirectSound");

        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SOUNDTYPE), sound);
    }

    hCtrl = GetDlgItem(hwnd, IDC_SAMPLERATE);
    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "11025");
        ComboBox_AddString(hCtrl, "22050");
        ComboBox_AddString(hCtrl, "44100");
        ComboBox_SetCurSel(hCtrl, 1);
    }
}

#ifdef MULTI_MONITOR

/* Populate the Display type drop down */
static void InitializeDisplayTypeUI(HWND hwnd)
{
    int i;

    HWND hWndDisplays = GetDlgItem(hwnd, IDC_DISPLAYS);

    if (hWndDisplays == 0)
        return;

    ComboBox_ResetContent(hWndDisplays);
    for (i = 0; i < DirectDraw_GetNumDisplays(); i++)
    {
        ComboBox_AddString(hWndDisplays, DirectDraw_GetDisplayName(i));
    }
}

#else

/* Populate the Display type drop down */
static void InitializeDisplayTypeUI(HWND hwnd)
{
    hWndDisplays = GetDlgItem(hwnd, IDC_DISPLAYS);

    if (hWndDisplays == 0)
        return;

    ComboBox_ResetContent(hWndDisplays);
    ComboBox_AddString(hWndDisplays, "Primary Display");
    ComboBox_SetCurSel(hWndDisplays, 0);
    nNumDisplays++;
}

#endif

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
    HWND hCtrl = GetDlgItem(hwnd, IDC_FRAMESKIP);

    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "Draw every frame");
        ComboBox_AddString(hCtrl, "Skip 1 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 2 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 3 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 4 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 5 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 6 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 7 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 8 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 9 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 10 of every 12 frames");
        ComboBox_AddString(hCtrl, "Skip 11 of every 12 frames");
    }
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
    HWND hCtrl = GetDlgItem(hwnd, IDC_ROTATE);

    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "None");   /* 0 */
        ComboBox_AddString(hCtrl, "Right");  /* 1 */
        ComboBox_AddString(hCtrl, "Left");   /* 2 */
    }
}

/* Populate the Color depth drop down */
static void InitializeDepthUI(HWND hwnd)
{
    HWND hCtrl = GetDlgItem(hwnd, IDC_DEPTH);

    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "Auto");
        ComboBox_AddString(hCtrl, "256 Colors");
        ComboBox_AddString(hCtrl, "High Color (16 bit)");
    }
}

/* Populate the Default Input drop down */
static void InitializeDefaultInputUI(HWND hwnd)
{
    HWND hCtrl = GetDlgItem(hwnd, IDC_DEFAULT_INPUT);

    if (hCtrl)
    {
        ComboBox_AddString(hCtrl, "Standard");
        ComboBox_AddString(hCtrl, "HotRod");
        ComboBox_AddString(hCtrl, "HotRod SE");
    }
}

/**************************************************************************
    Game History functions
 **************************************************************************/

/* Load indexes from history.dat if found */
char * GameHistory(int game_index)
{
    historyBuf[0] = '\0';

    if (load_driver_history(drivers[game_index], historyBuf, sizeof(historyBuf)) == 0)
        HistoryFixBuf(historyBuf);

    return historyBuf;
}

static void HistoryFixBuf(char *buf)
{
    char *s  = tempHistoryBuf;
    char *p  = buf;
    int  len = 0;

    if (strlen(buf) < 3)
    {
        *buf = '\0';
        return;
    }

    while (*p && len < MAX_HISTORY_LEN - 1)
    {
        if (*p == '\n')
        {
            *s++ = '\r';
            len++;
        }

        *s++ = *p++;
        len++;
    }

    *s++ = '\0';
    strcpy(buf, tempHistoryBuf);
}

/* End of source file */
