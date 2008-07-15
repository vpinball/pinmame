/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

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
// #define RED_TEST

#define WIN32_LEAN_AND_MEAN
#define NONAMELESSUNION 1
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#undef NONAMELESSUNION
#include <ddraw.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <driver.h>
#include <info.h>
#include "audit.h"
#include "audit32.h"
#include "bitmask.h"
#include "options.h"
#include "file.h"
#include "resource.h"
#include "DIJoystick.h"     /* For DIJoystick avalibility. */
#include "m32util.h"
#include "directdraw.h"
#include "properties.h"

#include "screenshot.h"
#include "Mame32.h"
#include "DataMap.h"
#include "help.h"
#include "resource.hm"

#ifdef MESS
/* done like this until I figure out a better idea */
#include "ui/resourcems.h"

void MessOptionsToProp(int nGame, HWND hWnd, options_type *o);
BOOL MessPropertiesCommand(int nGame, HWND hWnd, WORD wNotifyCode, WORD wID, BOOL *changed);
void MessPropToOptions(int nGame, HWND hWnd, options_type *o);
#endif

// missing win32 api defines
#ifndef TBCD_TICS
#define TBCD_TICS 1
#endif
#ifndef TBCD_THUMB
#define TBCD_THUMB 2
#endif
#ifndef TBCD_CHANNEL
#define TBCD_CHANNEL 3
#endif

/***************************************************************
 * Imported function prototypes
 ***************************************************************/

/**************************************************************
 * Local function prototypes
 **************************************************************/

static void SetStereoEnabled(HWND hWnd, int nIndex);
static void SetYM3812Enabled(HWND hWnd, int nIndex);
static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled);
static void InitializeOptions(HWND hDlg);
static void InitializeMisc(HWND hDlg);
static void OptOnHScroll(HWND hWnd, HWND hwndCtl, UINT code, int pos);
static void BeamSelectionChange(HWND hwnd);
static void FlickerSelectionChange(HWND hwnd);
static void GammaSelectionChange(HWND hwnd);
static void BrightCorrectSelectionChange(HWND hwnd);
static void PauseBrightSelectionChange(HWND hwnd);
static void BrightnessSelectionChange(HWND hwnd);
static void IntensitySelectionChange(HWND hwnd);
static void A2DSelectionChange(HWND hwnd);
static void ResDepthSelectionChange(HWND hWnd, HWND hWndCtrl);
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl);
static void VolumeSelectionChange(HWND hwnd);
static void AudioLatencySelectionChange(HWND hwnd);
static void D3DScanlinesSelectionChange(HWND hwnd);
static void D3DFeedbackSelectionChange(HWND hwnd);
static void ZoomSelectionChange(HWND hwnd);
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth, DWORD dwRefresh);
static void InitializeDisplayModeUI(HWND hwnd);
static void InitializeSoundUI(HWND hwnd);
static void InitializeSkippingUI(HWND hwnd);
static void InitializeRotateUI(HWND hwnd);
static void InitializeResDepthUI(HWND hwnd);
static void InitializeRefreshUI(HWND hwnd);
static void InitializeDefaultInputUI(HWND hWnd);
static void InitializeEffectUI(HWND hWnd);
static void InitializeArtresUI(HWND hWnd);
static void InitializeD3DFilterUI(HWND hwnd);
static void InitializeD3DEffectUI(HWND hwnd);
static void InitializeD3DPrescaleUI(HWND hwnd);
static void InitializeBIOSUI(HWND hwnd);
static void InitializeCleanStretchUI(HWND hwnd);
static void PropToOptions(HWND hWnd, options_type *o);
static void OptionsToProp(HWND hWnd, options_type *o);
static void SetPropEnabledControls(HWND hWnd);
#ifdef PINMAME
static void DMDREDSelectionChange(HWND hwnd);
static void DMDGREENSelectionChange(HWND hwnd);
static void DMDBLUESelectionChange(HWND hwnd);
static void DMDPERC0SelectionChange(HWND hwnd);
static void DMDPERC33SelectionChange(HWND hwnd);
static void DMDPERC66SelectionChange(HWND hwnd);
static void DMDANTIALIASSelectionChange(HWND hwnd);
#endif /* PINMAME */

static void BuildDataMap(void);
static void ResetDataMap(void);

static BOOL IsControlDefaultValue(HWND hDlg,HWND hwnd_ctrl);

/**************************************************************
 * Local private variables
 **************************************************************/

static options_type  origGameOpts;
static BOOL orig_uses_defaults;
static options_type* pGameOpts = NULL;

static int  g_nGame            = 0;
static BOOL g_bInternalSet     = FALSE;
static BOOL g_bUseDefaults     = FALSE;
static BOOL g_bReset           = FALSE;
static int  g_nSampleRateIndex = 0;
static int  g_nVolumeIndex     = 0;
static int  g_nGammaIndex      = 0;
static int  g_nBrightCorrectIndex = 0;
static int  g_nPauseBrightIndex = 0;
static int  g_nBeamIndex       = 0;
static int  g_nFlickerIndex    = 0;
static int  g_nIntensityIndex  = 0;
static int  g_nRotateIndex     = 0;
static int  g_nInputIndex      = 0;
static int  g_nBrightnessIndex = 0;
static int  g_nEffectIndex     = 0;
static int  g_nA2DIndex		   = 0;
#ifdef PINMAME
static int  g_nDMDRedIndex     = 0;
static int  g_nDMDGreenIndex   = 0;
static int  g_nDMDBlueIndex    = 0;
static int  g_nDMDPerc0Index   = 0;
static int  g_nDMDPerc33Index  = 0;
static int  g_nDMDPerc66Index  = 0;
static int  g_nDMDAntialiasIndex= 0;
#endif /* PINMAME */

static HICON g_hIcon = NULL;

/* Property sheets */

#define HIGHLIGHT_COLOR RGB(0,196,0)
HBRUSH highlight_brush = NULL;
HBRUSH background_brush = NULL;

BOOL PropSheetFilter_Vector(const struct InternalMachineDriver *drv, const struct GameDriver *gamedrv)
{
	return (drv->video_attributes & VIDEO_TYPE_VECTOR) != 0;
}

/* Help IDs */
static DWORD dwHelpIDs[] =
{
	
	IDC_A2D,				HIDC_A2D,
	IDC_ANTIALIAS,          HIDC_ANTIALIAS,
	IDC_ARTRES,				HIDC_ARTRES,
	IDC_ARTWORK,            HIDC_ARTWORK,
	IDC_ARTWORK_CROP,		HIDC_ARTWORK_CROP,
	IDC_ASPECTRATIOD,       HIDC_ASPECTRATIOD,
	IDC_ASPECTRATION,       HIDC_ASPECTRATION,
	IDC_AUTOFRAMESKIP,      HIDC_AUTOFRAMESKIP,
	IDC_BACKDROPS,			HIDC_BACKDROPS,
	IDC_BEAM,               HIDC_BEAM,
	IDC_BEZELS,				HIDC_BEZELS,
	IDC_BRIGHTNESS,         HIDC_BRIGHTNESS,
	IDC_BRIGHTCORRECT,      HIDC_BRIGHTCORRECT,
	IDC_BROADCAST,			HIDC_BROADCAST,
	IDC_RANDOM_BG,          HIDC_RANDOM_BG,
	IDC_CHEAT,              HIDC_CHEAT,
	IDC_DDRAW,              HIDC_DDRAW,
	IDC_DEFAULT_INPUT,      HIDC_DEFAULT_INPUT,
	IDC_EFFECT,             HIDC_EFFECT,
	IDC_FILTER_CLONES,      HIDC_FILTER_CLONES,
	IDC_FILTER_EDIT,        HIDC_FILTER_EDIT,
	IDC_FILTER_NONWORKING,  HIDC_FILTER_NONWORKING,
	IDC_FILTER_ORIGINALS,   HIDC_FILTER_ORIGINALS,
	IDC_FILTER_RASTER,      HIDC_FILTER_RASTER,
	IDC_FILTER_UNAVAILABLE, HIDC_FILTER_UNAVAILABLE,
	IDC_FILTER_VECTOR,      HIDC_FILTER_VECTOR,
	IDC_FILTER_WORKING,     HIDC_FILTER_WORKING,
	IDC_FLICKER,            HIDC_FLICKER,
	IDC_FLIPX,              HIDC_FLIPX,
	IDC_FLIPY,              HIDC_FLIPY,
	IDC_FRAMESKIP,          HIDC_FRAMESKIP,
	IDC_GAMMA,              HIDC_GAMMA,
	IDC_HISTORY,            HIDC_HISTORY,
	IDC_HWSTRETCH,          HIDC_HWSTRETCH,
	IDC_INTENSITY,          HIDC_INTENSITY,
	IDC_JOYSTICK,           HIDC_JOYSTICK,
	IDC_KEEPASPECT,         HIDC_KEEPASPECT,
	IDC_LANGUAGECHECK,      HIDC_LANGUAGECHECK,
	IDC_LANGUAGEEDIT,       HIDC_LANGUAGEEDIT,
	IDC_LEDS,				HIDC_LEDS,
	IDC_LOG,                HIDC_LOG,
	IDC_SLEEP,				HIDC_SLEEP,
	IDC_MATCHREFRESH,       HIDC_MATCHREFRESH,
	IDC_MAXIMIZE,           HIDC_MAXIMIZE,
	IDC_OVERLAYS,			HIDC_OVERLAYS,
	IDC_PROP_RESET,         HIDC_PROP_RESET,
	IDC_REFRESH,            HIDC_REFRESH,
	IDC_RESDEPTH,           HIDC_RESDEPTH,
	IDC_RESET_DEFAULT,      HIDC_RESET_DEFAULT,
	IDC_RESET_FILTERS,      HIDC_RESET_FILTERS,
	IDC_RESET_GAMES,        HIDC_RESET_GAMES,
	IDC_RESET_UI,           HIDC_RESET_UI,
	IDC_ROTATE,             HIDC_ROTATE,
	IDC_SAMPLERATE,         HIDC_SAMPLERATE,
	IDC_SAMPLES,            HIDC_SAMPLES,
	IDC_SCANLINES,          HIDC_SCANLINES,
	IDC_SIZES,              HIDC_SIZES,
	IDC_START_GAME_CHECK,   HIDC_START_GAME_CHECK,
	IDC_SWITCHBPP,          HIDC_SWITCHBPP,
	IDC_SWITCHRES,          HIDC_SWITCHRES,
	IDC_SYNCREFRESH,        HIDC_SYNCREFRESH,
	IDC_THROTTLE,           HIDC_THROTTLE,
	IDC_TRANSLUCENCY,       HIDC_TRANSLUCENCY,
	IDC_TRIPLE_BUFFER,      HIDC_TRIPLE_BUFFER,
	IDC_USE_DEFAULT,        HIDC_USE_DEFAULT,
	IDC_USE_FILTER,         HIDC_USE_FILTER,
	IDC_USE_MOUSE,          HIDC_USE_MOUSE,
	IDC_USE_SOUND,          HIDC_USE_SOUND,
	IDC_VOLUME,             HIDC_VOLUME,
	IDC_WAITVSYNC,          HIDC_WAITVSYNC,
	IDC_WINDOWED,           HIDC_WINDOWED,
	IDC_PAUSEBRIGHT,        HIDC_PAUSEBRIGHT,
	IDC_LIGHTGUN,           HIDC_LIGHTGUN,
	IDC_STEADYKEY,          HIDC_STEADYKEY,
	IDC_OLD_TIMING,         HIDC_OLD_TIMING,
	IDC_JOY_GUI,            HIDC_JOY_GUI,
	IDC_RANDOM_BG,          HIDC_RANDOM_BG,
	IDC_SKIP_DISCLAIMER,    HIDC_SKIP_DISCLAIMER,
	IDC_SKIP_GAME_INFO,     HIDC_SKIP_GAME_INFO,
	IDC_HIGH_PRIORITY,      HIDC_HIGH_PRIORITY,
	IDC_D3D,                HIDC_D3D,
	IDC_D3D_FILTER,         HIDC_D3D_FILTER,
	IDC_D3D_EFFECT,         HIDC_D3D_EFFECT,
	IDC_D3D_TEXTURE_MANAGEMENT, HIDC_D3D_TEXTURE_MANAGEMENT,
	IDC_AUDIO_LATENCY,      HIDC_AUDIO_LATENCY,
	IDC_D3D_PRESCALE,       HIDC_D3D_PRESCALE,
	IDC_D3D_SCANLINES_ENABLE, HIDC_D3D_SCANLINES,
	IDC_D3D_SCANLINES,      HIDC_D3D_SCANLINES,
	IDC_D3D_FEEDBACK_ENABLE, HIDC_D3D_FEEDBACK,
	IDC_D3D_FEEDBACK,       HIDC_D3D_FEEDBACK,
	IDC_ZOOM,               HIDC_ZOOM,
	IDC_BIOS,               HIDC_BIOS,
	IDC_CLEAN_STRETCH,      HIDC_CLEAN_STRETCH,
	IDC_D3D_ROTATE_EFFECTS, HIDC_D3D_ROTATE_EFFECTS,
	IDC_CYCLE_SCREENSHOT,   HIDC_CYCLE_SCREENSHOT,
	IDC_STRETCH_SCREENSHOT_LARGER, HIDC_STRETCH_SCREENSHOT_LARGER,
	0,                      0
};

static struct ComboBoxEffect
{
	const char*	m_pText;
	const char* m_pData;
} g_ComboBoxEffect[] =
{
	{ "None",                           "none"    },
	{ "25% scanlines",                  "scan25"  },
	{ "50% scanlines",                  "scan50"  },
	{ "75% scanlines",                  "scan75"  },
	{ "75% vertical scanlines",         "scan75v" },
	{ "RGB triad of 16 pixels",         "rgb16"   },
	{ "RGB triad of 6 pixels",          "rgb6"    },
	{ "RGB triad of 4 pixels",          "rgb4"    },
	{ "RGB triad of 4 vertical pixels", "rgb4v"   },
	{ "RGB triad of 3 pixels",          "rgb3"    },
	{ "RGB tiny",                       "rgbtiny" },
	{ "RGB sharp",                      "sharp"   }
};

#define NUMEFFECTS (sizeof(g_ComboBoxEffect) / sizeof(g_ComboBoxEffect[0]))

/***************************************************************
 * Public functions
 ***************************************************************/

DWORD GetHelpIDs(void)
{
	return (DWORD) (LPSTR) dwHelpIDs;
}

static void CLIB_DECL DetailsPrintf(const char *fmt, ...)
{
	// throw it away
}

static PROPSHEETPAGE *CreatePropSheetPages(HINSTANCE hInst, BOOL bOnlyDefault,
	const struct GameDriver *gamedrv, UINT *pnMaxPropSheets)
{
	PROPSHEETPAGE *pspages;
	int maxPropSheets;
	int possiblePropSheets;
	int i;
    struct InternalMachineDriver drv;

	if (gamedrv)
	    expand_machine_driver(gamedrv->drv, &drv);

	for (i = 0; g_propSheets[i].pfnDlgProc; i++)
		;

	possiblePropSheets = i + 1;

	pspages = malloc(sizeof(PROPSHEETPAGE) * possiblePropSheets);
	if (!pspages)
		return NULL;
	memset(pspages, 0, sizeof(PROPSHEETPAGE) * possiblePropSheets);

	maxPropSheets = 0;
	for (i = 0; g_propSheets[i].pfnDlgProc; i++)
	{
		if (!bOnlyDefault || g_propSheets[i].bOnDefaultPage)
		{
			if (!gamedrv || !g_propSheets[i].pfnFilterProc || g_propSheets[i].pfnFilterProc(&drv, gamedrv))
			{
				pspages[maxPropSheets].dwSize                     = sizeof(PROPSHEETPAGE);
				pspages[maxPropSheets].dwFlags                    = 0;
				pspages[maxPropSheets].hInstance                  = hInst;
				pspages[maxPropSheets].DUMMYUNIONNAME.pszTemplate = MAKEINTRESOURCE(g_propSheets[i].dwDlgID);
				pspages[maxPropSheets].pfnCallback                = NULL;
				pspages[maxPropSheets].lParam                     = 0;
				pspages[maxPropSheets].pfnDlgProc                 = g_propSheets[i].pfnDlgProc;
				maxPropSheets++;
			}
		}
	}

	if (pnMaxPropSheets)
		*pnMaxPropSheets = maxPropSheets;

	return pspages;
}

void InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   *pspage;

	g_nGame = -1;

	/* Get default options to populate property sheets */
	pGameOpts = GetDefaultOptions();
	g_bUseDefaults = FALSE;
	/* Stash the result for comparing later */
	CopyGameOptions(pGameOpts,&origGameOpts);
	orig_uses_defaults = FALSE;
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(pshead));

	pspage = CreatePropSheetPages(hInst, TRUE, NULL, &pshead.nPages);
	if (!pspage)
		return;

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = "Default Game";
	pshead.DUMMYUNIONNAME2.nStartPage = 0;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
	}

	free(pspage);
}

void InitPropertyPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon)
{
	InitPropertyPageToPage(hInst, hWnd, game_num, hIcon, PROPERTIES_PAGE);
}

void InitPropertyPageToPage(HINSTANCE hInst, HWND hWnd, int game_num, HICON hIcon, int start_page)
{
	PROPSHEETHEADER pshead;
	PROPSHEETPAGE   *pspage;

	if (highlight_brush == NULL)
		highlight_brush = CreateSolidBrush(HIGHLIGHT_COLOR);

	if (background_brush == NULL)
		background_brush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

	g_hIcon = CopyIcon(hIcon);
	InitGameAudit(game_num);
	g_nGame = game_num;

	/* Get Game options to populate property sheets */
	pGameOpts = GetGameOptions(game_num);
	g_bUseDefaults = GetGameUsesDefaults(game_num);
	/* Stash the result for comparing later */
	CopyGameOptions(pGameOpts,&origGameOpts);
	orig_uses_defaults = g_bUseDefaults;
	g_bReset = FALSE;
	BuildDataMap();

	ZeroMemory(&pshead, sizeof(PROPSHEETHEADER));

	pspage = CreatePropSheetPages(hInst, FALSE, drivers[game_num], &pshead.nPages);
	if (!pspage)
		return;

	/* Fill in the property sheet header */
	pshead.hwndParent                 = hWnd;
	pshead.dwSize                     = sizeof(PROPSHEETHEADER);
	pshead.dwFlags                    = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_PROPTITLE;
	pshead.hInstance                  = hInst;
	pshead.pszCaption                 = ModifyThe(drivers[g_nGame]->description);
	pshead.DUMMYUNIONNAME2.nStartPage = start_page;
	pshead.DUMMYUNIONNAME.pszIcon     = MAKEINTRESOURCE(IDI_MAME32_ICON);
	pshead.DUMMYUNIONNAME3.ppsp       = pspage;

	/* Create the Property sheet and display it */
	if (PropertySheet(&pshead) == -1)
	{
		char temp[100];
		DWORD dwError = GetLastError();
		sprintf(temp, "Propery Sheet Error %d %X", (int)dwError, (int)dwError);
		MessageBox(0, temp, "Error", IDOK);
	}

	free(pspage);
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/* Build CPU info string */
static char *GameInfoCPU(UINT nIndex)
{
	int i;
	static char buf[1024] = "";
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_CPU && drv.cpu[i].cpu_type)
	{
		if (drv.cpu[i].cpu_clock >= 1000000)
			sprintf(&buf[strlen(buf)], "%s %d.%06d MHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000000,
					drv.cpu[i].cpu_clock % 1000000);
		else
			sprintf(&buf[strlen(buf)], "%s %d.%03d kHz",
					cputype_name(drv.cpu[i].cpu_type),
					drv.cpu[i].cpu_clock / 1000,
					drv.cpu[i].cpu_clock % 1000);

		if (drv.cpu[i].cpu_flags & CPU_AUDIO_CPU)
			strcat(buf, " (sound)");

		strcat(buf, "\n");

		i++;
    }

	return buf;
}

/* Build Sound system info string */
static char *GameInfoSound(UINT nIndex)
{
	int i;
	static char buf[1024] = "";
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	i = 0;
	while (i < MAX_SOUND && drv.sound[i].sound_type)
	{
		if (1 < sound_num(&drv.sound[i]))
			sprintf(&buf[strlen(buf)], "%d x ", sound_num(&drv.sound[i]));

		sprintf(&buf[strlen(buf)], "%s", sound_name(&drv.sound[i]));

		if (sound_clock(&drv.sound[i]))
		{
			if (sound_clock(&drv.sound[i]) >= 1000000)
				sprintf(&buf[strlen(buf)], " %d.%06d MHz",
						sound_clock(&drv.sound[i]) / 1000000,
						sound_clock(&drv.sound[i]) % 1000000);
			else
				sprintf(&buf[strlen(buf)], " %d.%03d kHz",
						sound_clock(&drv.sound[i]) / 1000,
						sound_clock(&drv.sound[i]) % 1000);
		}

		strcat(buf,"\n");

		i++;
	}
	return buf;
}

/* Build Display info string */
static char *GameInfoScreen(UINT nIndex)
{
	static char buf[1024];
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		if (drivers[nIndex]->flags & ORIENTATION_SWAP_XY)
			sprintf(buf,"%d x %d (vert) %5.2f Hz",
					drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1,
					drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
					drv.frames_per_second);
		else
			sprintf(buf,"%d x %d (horz) %5.2f Hz",
					drv.default_visible_area.max_x - drv.default_visible_area.min_x + 1,
					drv.default_visible_area.max_y - drv.default_visible_area.min_y + 1,
					drv.frames_per_second);
	}
	return buf;
}

/* Build color information string */
static char *GameInfoColors(UINT nIndex)
{
	static char buf[1024];
    struct InternalMachineDriver drv;
    expand_machine_driver(drivers[nIndex]->drv,&drv);

	ZeroMemory(buf, sizeof(buf));
	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		strcpy(buf, "Vector Game");
	else
	{
		sprintf(buf, "%d colors ", drv.total_colors);
	}

	return buf;
}

/* Build game status string */
const char *GameInfoStatus(int driver_index)
{
	int audit_result = GetRomAuditResults(driver_index);

	if (IsAuditResultKnown(audit_result) == FALSE)
	{
		return "Unknown";
	}

	if (IsAuditResultYes(audit_result))
	{
		if (DriverIsBroken(driver_index))
			return "Not working";
		if (drivers[driver_index]->flags & GAME_WRONG_COLORS)
			return "Colors are totally wrong";
		if (drivers[driver_index]->flags & GAME_IMPERFECT_COLORS)
			return "Imperfect Colors";
		else
			return "Working";

	}

	// audit result is no

#ifdef MESS
		return "BIOS missing";
#else
		return "ROMs missing";
#endif
}

/* Build game manufacturer string */
static char *GameInfoManufactured(UINT nIndex)
{
	static char buffer[1024];

	_snprintf(buffer,sizeof(buffer),"%s %s",drivers[nIndex]->year,drivers[nIndex]->manufacturer);
	return buffer;
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
static char *GameInfoCloneOf(UINT nIndex)
{
	static char buf[1024];

	buf[0] = '\0';

	if (DriverIsClone(nIndex))
	{
		sprintf(buf, "%s - \"%s\"",
				ConvertAmpersandString(ModifyThe(drivers[nIndex]->clone_of->description)),
				drivers[nIndex]->clone_of->name);
	}

	return buf;
}

static const char * GameInfoSource(UINT nIndex)
{
	return GetDriverFilename(nIndex);
}

/* Handle the information property page */
INT_PTR CALLBACK GamePropertiesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		if (g_hIcon)
			SendMessage(GetDlgItem(hDlg, IDC_GAME_ICON), STM_SETICON, (WPARAM) g_hIcon, 0);
#if defined(USE_SINGLELINE_TABCONTROL)
		{
			HWND hWnd = PropSheet_GetTabControl(GetParent(hDlg));
			DWORD tabStyle = (GetWindowLong(hWnd,GWL_STYLE) & ~TCS_MULTILINE);
			SetWindowLong(hWnd,GWL_STYLE,tabStyle | TCS_SINGLELINE);
		}
#endif

		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE),         GameInfoTitle(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_MANUFACTURED),  GameInfoManufactured(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_STATUS),        GameInfoStatus(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CPU),           GameInfoCPU(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOUND),         GameInfoSound(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SCREEN),        GameInfoScreen(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_COLORS),        GameInfoColors(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_CLONEOF),       GameInfoCloneOf(g_nGame));
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_SOURCE),        GameInfoSource(g_nGame));

		if (DriverIsClone(g_nGame))
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

static BOOL ReadSkipCtrl(HWND hWnd, UINT nCtrlID, int *value)
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
INT_PTR CALLBACK GameOptionsProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		/* Fill in the Game info at the top of the sheet */
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(g_nGame));
		InitializeOptions(hDlg);
		InitializeMisc(hDlg);

		PopulateControls(hDlg);
		OptionsToProp(hDlg, pGameOpts);
		SetPropEnabledControls(hDlg);
		if (g_nGame == -1)
			ShowWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), SW_HIDE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);

		EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);
		ShowWindow(hDlg, SW_SHOW);

		return 1;

	case WM_HSCROLL:
		/* slider changed */
		HANDLE_WM_HSCROLL(hDlg, wParam, lParam, OptOnHScroll);
		g_bUseDefaults = FALSE;
		g_bReset = TRUE;
		EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), TRUE);
		PropSheet_Changed(GetParent(hDlg), hDlg);

		// make sure everything's copied over, to determine what's changed
		PropToOptions(hDlg,pGameOpts);
		ReadControls(hDlg);
		// redraw it, it might be a new color now
		InvalidateRect((HWND)lParam,NULL,TRUE);

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
			case IDC_SIZES:
			case IDC_FRAMESKIP:
			case IDC_EFFECT:
			case IDC_DEFAULT_INPUT:
			case IDC_ROTATE:
			case IDC_SAMPLERATE:
			case IDC_ARTRES:
			case IDC_CLEAN_STRETCH :
				if (wNotifyCode == CBN_SELCHANGE)
				{
					changed = TRUE;
				}
				break;

			case IDC_WINDOWED:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_RESDEPTH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					ResDepthSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_REFRESH:
				if (wNotifyCode == LBN_SELCHANGE)
				{
					RefreshSelectionChange(hDlg, hWndCtrl);
					changed = TRUE;
				}
				break;

			case IDC_ASPECTRATION:
			case IDC_ASPECTRATIOD:
				if (wNotifyCode == EN_CHANGE)
				{
					if (g_bInternalSet == FALSE)
						changed = TRUE;
				}
				break;

			case IDC_TRIPLE_BUFFER:
				changed = ReadControl(hDlg, wID);
				break;

			case IDC_D3D_FILTER :
			case IDC_D3D_EFFECT :
			case IDC_D3D_PRESCALE :
				if (wNotifyCode == CBN_SELCHANGE)
					changed = TRUE;
				break;

			case IDC_BIOS :
				if (wNotifyCode == CBN_SELCHANGE)
					changed = TRUE;
				break;

			case IDC_PROP_RESET:
				if (wNotifyCode != BN_CLICKED)
					break;

				FreeGameOptions(pGameOpts);
				CopyGameOptions(&origGameOpts,pGameOpts);
				if (g_nGame != -1)
					SetGameUsesDefaults(g_nGame,orig_uses_defaults);

				BuildDataMap();
				PopulateControls(hDlg);
				OptionsToProp(hDlg, pGameOpts);
				SetPropEnabledControls(hDlg);
				g_bReset = FALSE;
				PropSheet_UnChanged(GetParent(hDlg), hDlg);
				g_bUseDefaults = orig_uses_defaults;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				break;

			case IDC_USE_DEFAULT:
				if (g_nGame != -1)
				{
					SetGameUsesDefaults(g_nGame,TRUE);

					pGameOpts = GetGameOptions(g_nGame);
					g_bUseDefaults = GetGameUsesDefaults(g_nGame);
					BuildDataMap();
					PopulateControls(hDlg);
					OptionsToProp(hDlg, pGameOpts);
					SetPropEnabledControls(hDlg);
					if (orig_uses_defaults != g_bUseDefaults)
					{
						PropSheet_Changed(GetParent(hDlg), hDlg);
						g_bReset = TRUE;
					}
					else
					{
						PropSheet_UnChanged(GetParent(hDlg), hDlg);
						g_bReset = FALSE;
					}
					EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
				}

				break;

			default:
#ifdef MESS
				if (MessPropertiesCommand(g_nGame, hDlg, wNotifyCode, wID, &changed))
					break;
#endif

				if (wNotifyCode == BN_CLICKED)
				{
					switch (wID)
					{
					case IDC_SCANLINES:
						if (Button_GetCheck(GetDlgItem(hDlg, IDC_SCANLINES)))
						{
							Button_SetCheck(GetDlgItem(hDlg, IDC_VSCANLINES), FALSE);
						}
						break;

					case IDC_VSCANLINES:
						if (Button_GetCheck(GetDlgItem(hDlg, IDC_VSCANLINES)))
						{
							Button_SetCheck(GetDlgItem(hDlg, IDC_SCANLINES), FALSE);
						}
						break;
					}
					changed = TRUE;
				}
			}

			if (changed == TRUE)
			{
				// enable the apply button
				if (g_nGame != -1)
					SetGameUsesDefaults(g_nGame,FALSE);
				g_bUseDefaults = FALSE;
				PropSheet_Changed(GetParent(hDlg), hDlg);
				g_bReset = TRUE;
				EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			}
			SetPropEnabledControls(hDlg);

			// make sure everything's copied over, to determine what's changed
			PropToOptions(hDlg, pGameOpts);
			ReadControls(hDlg);

			// redraw it, it might be a new color now
			if (GetDlgItem(hDlg,wID))
				InvalidateRect(GetDlgItem(hDlg,wID),NULL,FALSE);

		}
		break;

	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->code)
		{
		case PSN_SETACTIVE:
			/* Initialize the controls. */
			PopulateControls(hDlg);
			OptionsToProp(hDlg, pGameOpts);
			SetPropEnabledControls(hDlg);
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			break;

		case PSN_APPLY:
			/* Save and apply the options here */
			PropToOptions(hDlg, pGameOpts);
			ReadControls(hDlg);
			if (g_nGame == -1)
				pGameOpts = GetDefaultOptions();
			else
			{
				SetGameUsesDefaults(g_nGame,g_bUseDefaults);
				orig_uses_defaults = g_bUseDefaults;
				pGameOpts = GetGameOptions(g_nGame);
			}

			FreeGameOptions(&origGameOpts);
			CopyGameOptions(pGameOpts,&origGameOpts);

			BuildDataMap();
			PopulateControls(hDlg);
			OptionsToProp(hDlg, pGameOpts);
			SetPropEnabledControls(hDlg);
			EnableWindow(GetDlgItem(hDlg, IDC_USE_DEFAULT), (g_bUseDefaults) ? FALSE : TRUE);
			g_bReset = FALSE;
			PropSheet_UnChanged(GetParent(hDlg), hDlg);
			SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
			return PSNRET_NOERROR;

		case PSN_KILLACTIVE:
			/* Save Changes to the options here. */
			PropToOptions(hDlg, pGameOpts);
			ReadControls(hDlg);
			ResetDataMap();
			if (g_nGame != -1)
				SetGameUsesDefaults(g_nGame,g_bUseDefaults);
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			return 1;

		case PSN_RESET:
			// Reset to the original values. Disregard changes
			FreeGameOptions(pGameOpts);
			CopyGameOptions(&origGameOpts,pGameOpts);
			if (g_nGame != -1)
				SetGameUsesDefaults(g_nGame,orig_uses_defaults);
			SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
			break;

		case PSN_HELP:
			// User wants help for this property page
			break;
		}
		break;

#ifdef RED_TEST
	case WM_CTLCOLORSTATIC :
	case WM_CTLCOLOREDIT :
		if (IsControlDefaultValue(hDlg,(HWND)lParam) == FALSE)
		{
			SetTextColor((HDC)wParam,HIGHLIGHT_COLOR);
			return (INT_PTR)background_brush;
		}
		break;
#endif

	case WM_HELP:
		/* User clicked the ? from the upper right on a control */
		HelpFunction(((LPHELPINFO)lParam)->hItemHandle, MAME32CONTEXTHELP, HH_TP_HELP_WM_HELP, GetHelpIDs());
		break;

	case WM_CONTEXTMENU:
		HelpFunction((HWND)wParam, MAME32CONTEXTHELP, HH_TP_HELP_CONTEXTMENU, GetHelpIDs());
		break;

	}
	EnableWindow(GetDlgItem(hDlg, IDC_PROP_RESET), g_bReset);

	return 0;
}

/* Read controls that are not handled in the DataMap */
static void PropToOptions(HWND hWnd, options_type *o)
{
	HWND hCtrl;
	HWND hCtrl2;
	int  nIndex;

	if (g_nGame != -1)
		SetGameUsesDefaults(g_nGame,g_bUseDefaults);

	/* resolution size */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		char buffer[200];

		/* Screen size control */
		nIndex = ComboBox_GetCurSel(hCtrl);
		
		if (nIndex == 0)
			sprintf(buffer, "%dx%d", 0, 0); // auto
		else
		{
			int w, h;

			ComboBox_GetText(hCtrl, buffer, sizeof(buffer)-1);
			if (sscanf(buffer, "%d x %d", &w, &h) == 2)
			{
				sprintf(buffer, "%dx%d", w, h);
			}
			else
			{
				sprintf(buffer, "%dx%d", 0, 0); // auto
			}
		}

		/* resolution depth */
		hCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
		if (hCtrl)
		{
			int nResDepth = 0;

			nIndex = ComboBox_GetCurSel(hCtrl);
			if (nIndex != CB_ERR)
				nResDepth = ComboBox_GetItemData(hCtrl, nIndex);

			switch (nResDepth)
			{
			default:
			case 0:  strcat(buffer, "x0"); break;
			case 16: strcat(buffer, "x16"); break;
			case 24: strcat(buffer, "x24"); break;
			case 32: strcat(buffer, "x32"); break;
			}
		}
		if (strcmp(buffer,"0x0x0") == 0)
			sprintf(buffer,"auto");
		FreeIfAllocated(&o->resolution);
		o->resolution = _strdup(buffer);
	}

	/* refresh */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		nIndex = ComboBox_GetCurSel(hCtrl);
		
		pGameOpts->gfx_refresh = ComboBox_GetItemData(hCtrl, nIndex);
	}

	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		int n = 0;
		int d = 0;
		char buffer[200];

		Edit_GetText(hCtrl,buffer,sizeof(buffer));
		sscanf(buffer,"%d",&n);

		Edit_GetText(hCtrl2,buffer,sizeof(buffer));
		sscanf(buffer,"%d",&d);

		if (n == 0 || d == 0)
		{
			n = 4;
			d = 3;
		}

		_snprintf(buffer,sizeof(buffer),"%d:%d",n,d);
		FreeIfAllocated(&o->aspect);
		o->aspect = _strdup(buffer);
	}
#ifdef MESS
	MessPropToOptions(g_nGame, hWnd, o);
#endif
}

/* Populate controls that are not handled in the DataMap */
static void OptionsToProp(HWND hWnd, options_type* o)
{
	HWND hCtrl;
	HWND hCtrl2;
	char buf[100];
	int  h = 0;
	int  w = 0;
	int  n = 0;
	int  d = 0;

	g_bInternalSet = TRUE;

	/* video */

	/* get desired resolution */
	if (!_stricmp(o->resolution, "auto"))
	{
		w = h = 0;
	}
	else
	if (sscanf(o->resolution, "%dx%dx%d", &w, &h, &d) < 2)
	{
		w = h = d = 0;
	}

	/* Setup sizes list based on depth. */
	UpdateDisplayModeUI(hWnd, d, o->gfx_refresh);

	/* Screen size drop down list */
	hCtrl = GetDlgItem(hWnd, IDC_SIZES);
	if (hCtrl)
	{
		if (w == 0 && h == 0)
		{
			/* default to auto */
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
				int nWidth, nHeight;

				/* Get the screen size */
				ComboBox_GetLBText(hCtrl, nCount, buf);

				if (sscanf(buf, "%d x %d", &nWidth, &nHeight) == 2)
				{
					/* If we match, set nSelection to the right value */
					if (w == nWidth
					&&  h == nHeight)
					{
						nSelection = nCount;
						break;
					}
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	/* Screen depth drop down list */
	hCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
	if (hCtrl)
	{
		if (d == 0)
		{
			/* default to auto */
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
				int nDepth;

				/* Get the screen depth */
				nDepth = ComboBox_GetItemData(hCtrl, nCount);

				/* If we match, set nSelection to the right value */
				if (d == nDepth)
				{
					nSelection = nCount;
					break;
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	/* Screen refresh list */
	hCtrl = GetDlgItem(hWnd, IDC_REFRESH);
	if (hCtrl)
	{
		if (o->gfx_refresh == 0)
		{
			/* default to auto */
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
				int nRefresh;

				/* Get the screen depth */
				nRefresh = ComboBox_GetItemData(hCtrl, nCount);

				/* If we match, set nSelection to the right value */
				if (o->gfx_refresh == nRefresh)
				{
					nSelection = nCount;
					break;
				}
			}
			ComboBox_SetCurSel(hCtrl, nSelection);
		}
	}

	
	hCtrl = GetDlgItem(hWnd, IDC_BRIGHTNESSDISP);
	if (hCtrl)
	{
		_snprintf(buf,sizeof(buf), "%03.2f", o->gfx_brightness);
		Static_SetText(hCtrl, buf);
	}
	
	/* aspect ratio */
	hCtrl  = GetDlgItem(hWnd, IDC_ASPECTRATION);
	hCtrl2 = GetDlgItem(hWnd, IDC_ASPECTRATIOD);
	if (hCtrl && hCtrl2)
	{
		n = 0;
		d = 0;

		if (sscanf(o->aspect, "%d:%d", &n, &d) == 2 && n != 0 && d != 0)
		{
			sprintf(buf, "%d", n);
			Edit_SetText(hCtrl, buf);
			sprintf(buf, "%d", d);
			Edit_SetText(hCtrl2, buf);
		}
		else
		{
			Edit_SetText(hCtrl,  "4");
			Edit_SetText(hCtrl2, "3");
		}
	}

	ZoomSelectionChange(hWnd);

	/* core video */
	hCtrl = GetDlgItem(hWnd, IDC_GAMMADISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_gamma_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_BRIGHTCORRECTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_bright_correct);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_PAUSEBRIGHTDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_pause_bright);
		Static_SetText(hCtrl, buf);
	}

	/* Input */
	hCtrl = GetDlgItem(hWnd, IDC_A2DDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_a2d);
		Static_SetText(hCtrl, buf);
	}

	/* vector */
	hCtrl = GetDlgItem(hWnd, IDC_BEAMDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_beam);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_FLICKERDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_flicker);
		Static_SetText(hCtrl, buf);
	}

	hCtrl = GetDlgItem(hWnd, IDC_INTENSITYDISP);
	if (hCtrl)
	{
		sprintf(buf, "%03.2f", o->f_intensity);
		Static_SetText(hCtrl, buf);
	}

	/* sound */
	hCtrl = GetDlgItem(hWnd, IDC_VOLUMEDISP);
	if (hCtrl)
	{
		sprintf(buf, "%ddB", o->attenuation);
		Static_SetText(hCtrl, buf);
	}
	AudioLatencySelectionChange(hWnd);

	// d3d
	D3DScanlinesSelectionChange(hWnd);
	D3DFeedbackSelectionChange(hWnd);

	g_bInternalSet = FALSE;

	g_nInputIndex = 0;
	hCtrl = GetDlgItem(hWnd, IDC_DEFAULT_INPUT);
	if (hCtrl)
	{
		int nCount;

		/* Get the number of items in the control */
		nCount = ComboBox_GetCount(hCtrl);

		while (0 < nCount--)
		{
			ComboBox_GetLBText(hCtrl, nCount, buf);

			if (_stricmp (buf,o->ctrlr) == 0)
			{
				g_nInputIndex = nCount;
			}
		}

		ComboBox_SetCurSel(hCtrl, g_nInputIndex);
	}

#ifdef MESS
	MessOptionsToProp(g_nGame, hWnd, o);
#endif
#ifdef PINMAME
	hCtrl = GetDlgItem(hWnd, IDC_DMD_RED_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d", o->dmd_red);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_GREEN_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d", o->dmd_green);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_BLUE_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d", o->dmd_blue);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_PERC0_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d%%", o->dmd_perc0);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_PERC33_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d%%", o->dmd_perc33);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_PERC66_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d%%", o->dmd_perc66);
		Static_SetText(hCtrl, buf);
	}
	hCtrl = GetDlgItem(hWnd, IDC_DMD_ANTIALIAS_DISP);
	if (hCtrl)
	{
		sprintf(buf, "%d%%", o->dmd_antialias);
		Static_SetText(hCtrl, buf);
	}
#endif /* PINMAME */
}

/* Adjust controls - tune them to the currently selected game */
static void SetPropEnabledControls(HWND hWnd)
{
	HWND hCtrl;
	int  nIndex;
	int  sound;
	BOOL ddraw = FALSE;
	BOOL d3d = FALSE;
	BOOL useart = FALSE;
	int joystick_attached = 9;
	int in_window = 0;

	nIndex = g_nGame;

	hCtrl = GetDlgItem(hWnd, IDC_DDRAW);
	ddraw = Button_GetCheck(hCtrl);

	in_window = pGameOpts->window_mode;

	EnableWindow(GetDlgItem(hWnd, IDC_RESDEPTH), !in_window);
	EnableWindow(GetDlgItem(hWnd, IDC_RESDEPTHTEXT), !in_window);

	EnableWindow(GetDlgItem(hWnd, IDC_WAITVSYNC),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_TRIPLE_BUFFER),   ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_HWSTRETCH),       ddraw && DirectDraw_HasHWStretch());
	EnableWindow(GetDlgItem(hWnd, IDC_SWITCHRES),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_SWITCHBPP),       ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_MATCHREFRESH),    ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_SYNCREFRESH),     ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESH),         !in_window && ddraw && DirectDraw_HasRefresh());
	EnableWindow(GetDlgItem(hWnd, IDC_REFRESHTEXT),     !in_window && ddraw && DirectDraw_HasRefresh());
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESS),      ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESSTEXT),  ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_BRIGHTNESSDISP),  ddraw);
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOTEXT), ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATION),    ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));
	EnableWindow(GetDlgItem(hWnd, IDC_ASPECTRATIOD),    ddraw && DirectDraw_HasHWStretch() && Button_GetCheck(GetDlgItem(hWnd, IDC_HWSTRETCH)));

	// d3d
	hCtrl = GetDlgItem(hWnd,IDC_D3D);
	d3d = Button_GetCheck(hCtrl);

	EnableWindow(GetDlgItem(hWnd,IDC_D3D_FILTER),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_TEXTURE_MANAGEMENT),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_EFFECT),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_PRESCALE),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_ROTATE_EFFECTS),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_SCANLINES_ENABLE),d3d);
	EnableWindow(GetDlgItem(hWnd,IDC_D3D_FEEDBACK_ENABLE),d3d);

	hCtrl = GetDlgItem(hWnd,IDC_D3D_SCANLINES_ENABLE);
	if (hCtrl)
	{
		BOOL enable = d3d && Button_GetCheck(hCtrl);
		EnableWindow(GetDlgItem(hWnd,IDC_D3D_SCANLINES),enable);
		EnableWindow(GetDlgItem(hWnd,IDC_D3D_SCANLINES_DISP),enable);
	}

	hCtrl = GetDlgItem(hWnd,IDC_D3D_FEEDBACK_ENABLE);
	if (hCtrl)
	{
		BOOL enable = d3d && Button_GetCheck(hCtrl);
		EnableWindow(GetDlgItem(hWnd,IDC_D3D_FEEDBACK),enable);
		EnableWindow(GetDlgItem(hWnd,IDC_D3D_FEEDBACK_DISP),enable);
	}

	/* Artwork options */
	hCtrl = GetDlgItem(hWnd, IDC_ARTWORK);

	useart = Button_GetCheck(hCtrl);

	EnableWindow(GetDlgItem(hWnd, IDC_ARTWORK_CROP),	useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BACKDROPS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_BEZELS),			useart);
	EnableWindow(GetDlgItem(hWnd, IDC_OVERLAYS),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTRES),			useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTRESTEXT),		useart);
	EnableWindow(GetDlgItem(hWnd, IDC_ARTMISCTEXT),		useart);

	/* Joystick options */
	joystick_attached = DIJoystick.Available();

	Button_Enable(GetDlgItem(hWnd,IDC_JOYSTICK),		joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2DTEXT),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2DDISP),			joystick_attached);
	EnableWindow(GetDlgItem(hWnd, IDC_A2D),				joystick_attached);

	/* Trackball / Mouse options */
	if (nIndex == -1 || DriverUsesTrackball(nIndex) || DriverUsesLightGun(nIndex))
		Button_Enable(GetDlgItem(hWnd,IDC_USE_MOUSE),TRUE);
	else
		Button_Enable(GetDlgItem(hWnd,IDC_USE_MOUSE),FALSE);

	if (nIndex == -1 || DriverUsesLightGun(nIndex))
		Button_Enable(GetDlgItem(hWnd,IDC_LIGHTGUN),TRUE);
	else
		Button_Enable(GetDlgItem(hWnd,IDC_LIGHTGUN),FALSE);


	/* Sound options */
	hCtrl = GetDlgItem(hWnd, IDC_USE_SOUND);
	if (hCtrl)
	{
		sound = Button_GetCheck(hCtrl);
		ComboBox_Enable(GetDlgItem(hWnd, IDC_SAMPLERATE), (sound != 0));

		EnableWindow(GetDlgItem(hWnd,IDC_VOLUME),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_RATETEXT),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_USE_FILTER),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_VOLUMEDISP),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_VOLUMETEXT),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY_DISP),sound);
		EnableWindow(GetDlgItem(hWnd,IDC_AUDIO_LATENCY_TEXT),sound);
		SetSamplesEnabled(hWnd, nIndex, sound);
		SetStereoEnabled(hWnd, nIndex);
		SetYM3812Enabled(hWnd, nIndex);
	}

	if (Button_GetCheck(GetDlgItem(hWnd, IDC_AUTOFRAMESKIP)))
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), FALSE);
	else
		EnableWindow(GetDlgItem(hWnd, IDC_FRAMESKIP), TRUE);


	// misc
	if (nIndex == -1 || DriverHasOptionalBIOS(nIndex))
		EnableWindow(GetDlgItem(hWnd,IDC_BIOS),TRUE);
	else
		EnableWindow(GetDlgItem(hWnd,IDC_BIOS),FALSE);

}

/**************************************************************
 * Control Helper functions for data exchange
 **************************************************************/

static void AssignSampleRate(HWND hWnd)
{
	switch (g_nSampleRateIndex)
	{
		case 0:  pGameOpts->samplerate = 11025; break;
		case 1:  pGameOpts->samplerate = 22050; break;
		case 2:  pGameOpts->samplerate = 44100; break;
		default: pGameOpts->samplerate = 44100; break;
	}
}

static void AssignVolume(HWND hWnd)
{
	pGameOpts->attenuation = g_nVolumeIndex - 32;
}

static void AssignBrightCorrect(HWND hWnd)
{
	/* "1.0", 0.5, 2.0 */
	pGameOpts->f_bright_correct = g_nBrightCorrectIndex / 20.0 + 0.5;
	
}

static void AssignPauseBright(HWND hWnd)
{
	/* "0.65", 0.5, 2.0 */
	pGameOpts->f_pause_bright = g_nPauseBrightIndex / 20.0 + 0.5;
	
}

static void AssignGamma(HWND hWnd)
{
	pGameOpts->f_gamma_correct = g_nGammaIndex / 20.0 + 0.5;
}

static void AssignBrightness(HWND hWnd)
{
	pGameOpts->gfx_brightness = g_nBrightnessIndex / 20.0 + 0.1;
}

static void AssignBeam(HWND hWnd)
{
	pGameOpts->f_beam = g_nBeamIndex / 20.0 + 1.0;
}

static void AssignFlicker(HWND hWnd)
{
	pGameOpts->f_flicker = g_nFlickerIndex;
}

static void AssignIntensity(HWND hWnd)
{
	pGameOpts->f_intensity = g_nIntensityIndex / 20.0 + 0.5;
}

static void AssignA2D(HWND hWnd)
{
	pGameOpts->f_a2d = g_nA2DIndex / 20.0;
}

static void AssignRotate(HWND hWnd)
{
	pGameOpts->ror = 0;
	pGameOpts->rol = 0;
	pGameOpts->norotate = 0;
	pGameOpts->auto_ror = 0;
	pGameOpts->auto_rol = 0;

	switch (g_nRotateIndex)
	{
	case 1 : pGameOpts->ror = 1; break;
	case 2 : pGameOpts->rol = 1; break;
	case 3 : pGameOpts->norotate = 1; break;
	case 4 : pGameOpts->auto_ror = 1; break;
	case 5 : pGameOpts->auto_rol = 1; break;
	default : break;
	}
}

static void AssignInput(HWND hWnd)
{
	int new_length;

	FreeIfAllocated(&pGameOpts->ctrlr);

	new_length = ComboBox_GetLBTextLen(hWnd,g_nInputIndex);
	if (new_length == CB_ERR)
	{
		dprintf("error getting text len");
		return;
	}
	pGameOpts->ctrlr = (char *)malloc(new_length + 1);
	ComboBox_GetLBText(hWnd, g_nInputIndex, pGameOpts->ctrlr);
}

static void AssignEffect(HWND hWnd)
{
	const char* ptr = (const char*)ComboBox_GetItemData(hWnd, g_nEffectIndex);

	FreeIfAllocated(&pGameOpts->effect);
	if (ptr != NULL)
		pGameOpts->effect = _strdup(ptr);
}

#ifdef PINMAME
static void AssignDMDRed(HWND hWnd)
{
	pGameOpts->dmd_red = g_nDMDRedIndex;
}
static void AssignDMDGreen(HWND hWnd)
{
	pGameOpts->dmd_green = g_nDMDGreenIndex;
}
static void AssignDMDBlue(HWND hWnd)
{
	pGameOpts->dmd_blue = g_nDMDBlueIndex;
}
static void AssignDMDPerc0(HWND hWnd)
{
	pGameOpts->dmd_perc0 = g_nDMDPerc0Index;
}
static void AssignDMDPerc33(HWND hWnd)
{
	pGameOpts->dmd_perc33 = g_nDMDPerc33Index;
}
static void AssignDMDPerc66(HWND hWnd)
{
	pGameOpts->dmd_perc66 = g_nDMDPerc66Index;
}
static void AssignDMDAntialias(HWND hWnd)
{
	pGameOpts->dmd_antialias = g_nDMDAntialiasIndex;
}
#endif /* PINMAME */

/************************************************************
 * DataMap initializers
 ************************************************************/

/* Initialize local helper variables */
static void ResetDataMap(void)
{
	int i;

	// add the 0.001 to make sure it truncates properly to the integer
	// (we don't want 35.99999999 to be cut down to 35 because of floating point error)
	g_nGammaIndex			= (int)((pGameOpts->f_gamma_correct  - 0.5) * 20.0 + 0.001);
	g_nBrightnessIndex		= (int)((pGameOpts->gfx_brightness   - 0.1) * 20.0 + 0.001);
	g_nBrightCorrectIndex	= (int)((pGameOpts->f_bright_correct - 0.5) * 20.0 + 0.001);
	g_nPauseBrightIndex   	= (int)((pGameOpts->f_pause_bright   - 0.5) * 20.0 + 0.001);
	g_nBeamIndex			= (int)((pGameOpts->f_beam           - 1.0) * 20.0 + 0.001);
	g_nFlickerIndex			= (int)(pGameOpts->f_flicker);
	g_nIntensityIndex		= (int)((pGameOpts->f_intensity      - 0.5) * 20.0 + 0.001);
	g_nA2DIndex				= (int)(pGameOpts->f_a2d                    * 20.0 + 0.001);

	// if no controller type was specified or it was standard
	if (pGameOpts->ctrlr == NULL || _stricmp(pGameOpts->ctrlr,"Standard") == 0)
	{
		FreeIfAllocated(&pGameOpts->ctrlr);
		pGameOpts->ctrlr = _strdup("Standard");
	}

	g_nRotateIndex = 0;
	if (pGameOpts->ror == TRUE && pGameOpts->rol == FALSE)
		g_nRotateIndex = 1;
	if (pGameOpts->ror == FALSE && pGameOpts->rol == TRUE)
		g_nRotateIndex = 2;
	if (pGameOpts->norotate)
		g_nRotateIndex = 3;
	if (pGameOpts->auto_ror)
		g_nRotateIndex = 4;
	if (pGameOpts->auto_rol)
		g_nRotateIndex = 5;

	g_nVolumeIndex = pGameOpts->attenuation + 32;
	switch (pGameOpts->samplerate)
	{
		case 11025:  g_nSampleRateIndex = 0; break;
		case 22050:  g_nSampleRateIndex = 1; break;
		default:
		case 44100:  g_nSampleRateIndex = 2; break;
	}

	g_nEffectIndex = 0;
	for (i = 0; i < NUMEFFECTS; i++)
	{
		if (!_stricmp(pGameOpts->effect, g_ComboBoxEffect[i].m_pData))
			g_nEffectIndex = i;
	}
#ifdef PINMAME
        g_nDMDRedIndex     = pGameOpts->dmd_red;
        g_nDMDGreenIndex   = pGameOpts->dmd_green;
        g_nDMDBlueIndex    = pGameOpts->dmd_blue;
        g_nDMDPerc0Index   = pGameOpts->dmd_perc0;
        g_nDMDPerc33Index  = pGameOpts->dmd_perc33;
        g_nDMDPerc66Index  = pGameOpts->dmd_perc66;
        g_nDMDAntialiasIndex= pGameOpts->dmd_antialias;
#endif /* PINMAME */

}

/* Build the control mapping by adding all needed information to the DataMap */
static void BuildDataMap(void)
{
	InitDataMap();


	ResetDataMap();

	/* video */
	DataMapAdd(IDC_AUTOFRAMESKIP, DM_BOOL, CT_BUTTON,   &pGameOpts->autoframeskip, DM_BOOL, &pGameOpts->autoframeskip, 0, 0, 0);
	DataMapAdd(IDC_FRAMESKIP,     DM_INT,  CT_COMBOBOX, &pGameOpts->frameskip,     DM_INT, &pGameOpts->frameskip,     0, 0, 0);
	DataMapAdd(IDC_WAITVSYNC,     DM_BOOL, CT_BUTTON,   &pGameOpts->wait_vsync,    DM_BOOL, &pGameOpts->wait_vsync,    0, 0, 0);
	DataMapAdd(IDC_TRIPLE_BUFFER, DM_BOOL, CT_BUTTON,   &pGameOpts->use_triplebuf, DM_BOOL, &pGameOpts->use_triplebuf, 0, 0, 0);
	DataMapAdd(IDC_WINDOWED,      DM_BOOL, CT_BUTTON,   &pGameOpts->window_mode,   DM_BOOL, &pGameOpts->window_mode,   0, 0, 0);
	DataMapAdd(IDC_DDRAW,         DM_BOOL, CT_BUTTON,   &pGameOpts->use_ddraw,     DM_BOOL, &pGameOpts->use_ddraw,     0, 0, 0);
	DataMapAdd(IDC_HWSTRETCH,     DM_BOOL, CT_BUTTON,   &pGameOpts->ddraw_stretch, DM_BOOL, &pGameOpts->ddraw_stretch, 0, 0, 0);
	DataMapAdd(IDC_REFRESH,       DM_NONE, CT_NONE, &pGameOpts->gfx_refresh,   DM_INT, &pGameOpts->gfx_refresh, 0, 0, 0);
	DataMapAdd(IDC_SCANLINES,     DM_BOOL, CT_BUTTON,   &pGameOpts->scanlines,     DM_BOOL, &pGameOpts->scanlines,     0, 0, 0);
	DataMapAdd(IDC_SWITCHRES,     DM_BOOL, CT_BUTTON,   &pGameOpts->switchres,     DM_BOOL, &pGameOpts->switchres,     0, 0, 0);
	DataMapAdd(IDC_SWITCHBPP,     DM_BOOL, CT_BUTTON,   &pGameOpts->switchbpp,     DM_BOOL, &pGameOpts->switchbpp,     0, 0, 0);
	DataMapAdd(IDC_MAXIMIZE,      DM_BOOL, CT_BUTTON,   &pGameOpts->maximize,      DM_BOOL, &pGameOpts->maximize,      0, 0, 0);
	DataMapAdd(IDC_KEEPASPECT,    DM_BOOL, CT_BUTTON,   &pGameOpts->keepaspect,    DM_BOOL, &pGameOpts->keepaspect,    0, 0, 0);
	DataMapAdd(IDC_MATCHREFRESH,  DM_BOOL, CT_BUTTON,   &pGameOpts->matchrefresh,  DM_BOOL, &pGameOpts->matchrefresh,  0, 0, 0);
	DataMapAdd(IDC_SYNCREFRESH,   DM_BOOL, CT_BUTTON,   &pGameOpts->syncrefresh,   DM_BOOL, &pGameOpts->syncrefresh,   0, 0, 0);
	DataMapAdd(IDC_THROTTLE,      DM_BOOL, CT_BUTTON,   &pGameOpts->throttle,      DM_BOOL, &pGameOpts->throttle,      0, 0, 0);
	DataMapAdd(IDC_BRIGHTNESS,    DM_INT,  CT_SLIDER,   &g_nBrightnessIndex,	   DM_DOUBLE, &pGameOpts->gfx_brightness, 0, 0, AssignBrightness);
	DataMapAdd(IDC_BRIGHTNESSDISP,DM_NONE, CT_NONE,   NULL,  DM_DOUBLE, &pGameOpts->gfx_brightness, 0, 0, 0);
	/* pGameOpts->frames_to_display */
	DataMapAdd(IDC_EFFECT,        DM_INT,  CT_COMBOBOX, &g_nEffectIndex,           DM_STRING, &pGameOpts->effect,		   0, 0, AssignEffect);
	DataMapAdd(IDC_ASPECTRATIOD,  DM_NONE, CT_NONE, &pGameOpts->aspect,    DM_STRING, &pGameOpts->aspect, 0, 0, 0);
	DataMapAdd(IDC_ASPECTRATION,  DM_NONE, CT_NONE, &pGameOpts->aspect,    DM_STRING, &pGameOpts->aspect, 0, 0, 0);
	DataMapAdd(IDC_SIZES,         DM_NONE, CT_NONE, &pGameOpts->resolution,    DM_STRING, &pGameOpts->resolution, 0, 0, 0);
	DataMapAdd(IDC_RESDEPTH,      DM_NONE, CT_NONE, &pGameOpts->resolution,    DM_STRING, &pGameOpts->resolution, 0, 0, 0);
	DataMapAdd(IDC_CLEAN_STRETCH, DM_INT,  CT_COMBOBOX, &pGameOpts->clean_stretch, DM_INT, &pGameOpts->clean_stretch, 0, 0, 0);
	DataMapAdd(IDC_ZOOM,          DM_INT,  CT_SLIDER,   &pGameOpts->zoom,          DM_INT, &pGameOpts->zoom,      0, 0, 0);

	// direct3d
	DataMapAdd(IDC_D3D,           DM_BOOL, CT_BUTTON,   &pGameOpts->use_d3d,       DM_BOOL, &pGameOpts->use_d3d,       0, 0, 0);
	DataMapAdd(IDC_D3D_FILTER,    DM_INT,  CT_COMBOBOX, &pGameOpts->d3d_filter,    DM_INT, &pGameOpts->d3d_filter, 0, 0, 0);
	DataMapAdd(IDC_D3D_TEXTURE_MANAGEMENT,DM_BOOL,CT_BUTTON,&pGameOpts->d3d_texture_management,DM_BOOL,&pGameOpts->d3d_texture_management, 0, 0, 0);
	DataMapAdd(IDC_D3D_EFFECT,    DM_INT,  CT_COMBOBOX, &pGameOpts->d3d_effect,    DM_INT, &pGameOpts->d3d_effect, 0, 0, 0);
	DataMapAdd(IDC_D3D_PRESCALE,  DM_INT,  CT_COMBOBOX, &pGameOpts->d3d_prescale,  DM_INT, &pGameOpts->d3d_prescale,  0, 0, 0);
	DataMapAdd(IDC_D3D_ROTATE_EFFECTS,DM_BOOL,CT_BUTTON,&pGameOpts->d3d_rotate_effects,DM_BOOL,&pGameOpts->d3d_rotate_effects, 0, 0, 0);
	DataMapAdd(IDC_D3D_SCANLINES_ENABLE,DM_BOOL, CT_BUTTON, &pGameOpts->d3d_scanlines_enable, DM_BOOL, &pGameOpts->d3d_scanlines_enable, 0, 0, 0);
	DataMapAdd(IDC_D3D_SCANLINES, DM_INT,  CT_SLIDER,   &pGameOpts->d3d_scanlines, DM_INT, &pGameOpts->d3d_scanlines, 0, 0, 0);
	DataMapAdd(IDC_D3D_FEEDBACK_ENABLE,DM_BOOL, CT_BUTTON, &pGameOpts->d3d_feedback_enable, DM_BOOL, &pGameOpts->d3d_feedback_enable, 0, 0, 0);
	DataMapAdd(IDC_D3D_FEEDBACK,  DM_INT,  CT_SLIDER,   &pGameOpts->d3d_feedback,  DM_INT, &pGameOpts->d3d_feedback, 0, 0, 0);

	/* input */
	DataMapAdd(IDC_DEFAULT_INPUT, DM_INT,  CT_COMBOBOX, &g_nInputIndex,            DM_STRING, &pGameOpts->ctrlr, 0, 0, AssignInput);
	DataMapAdd(IDC_USE_MOUSE,     DM_BOOL, CT_BUTTON,   &pGameOpts->use_mouse,     DM_BOOL, &pGameOpts->use_mouse,     0, 0, 0);
	DataMapAdd(IDC_JOYSTICK,      DM_BOOL, CT_BUTTON,   &pGameOpts->use_joystick,  DM_BOOL, &pGameOpts->use_joystick,  0, 0, 0);
	DataMapAdd(IDC_A2D,           DM_INT,  CT_SLIDER,   &g_nA2DIndex,              DM_DOUBLE, &pGameOpts->f_a2d, 0, 0, AssignA2D);
	DataMapAdd(IDC_A2DDISP,       DM_NONE, CT_NONE,     NULL,  DM_DOUBLE, &pGameOpts->f_a2d, 0, 0, 0);
	DataMapAdd(IDC_STEADYKEY,     DM_BOOL, CT_BUTTON,   &pGameOpts->steadykey,     DM_BOOL, &pGameOpts->steadykey,     0, 0, 0);
	DataMapAdd(IDC_LIGHTGUN,      DM_BOOL, CT_BUTTON,   &pGameOpts->lightgun,      DM_BOOL, &pGameOpts->lightgun,      0, 0, 0);

	/* core video */
	DataMapAdd(IDC_BRIGHTCORRECT, DM_INT,  CT_SLIDER,   &g_nBrightCorrectIndex,    DM_DOUBLE, &pGameOpts->f_bright_correct, 0, 0, AssignBrightCorrect);
	DataMapAdd(IDC_BRIGHTCORRECTDISP,DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_bright_correct, 0, 0, 0);
	DataMapAdd(IDC_PAUSEBRIGHT,   DM_INT,  CT_SLIDER,   &g_nPauseBrightIndex,      DM_DOUBLE, &pGameOpts->f_pause_bright,      0, 0, AssignPauseBright);
	DataMapAdd(IDC_PAUSEBRIGHTDISP,DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_pause_bright, 0, 0, 0);
	DataMapAdd(IDC_ROTATE,        DM_INT,  CT_COMBOBOX, &g_nRotateIndex,           DM_INT, &pGameOpts->ror, 0, 0, AssignRotate);
	DataMapAdd(IDC_FLIPX,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipx,         DM_BOOL, &pGameOpts->flipx,         0, 0, 0);
	DataMapAdd(IDC_FLIPY,         DM_BOOL, CT_BUTTON,   &pGameOpts->flipy,         DM_BOOL, &pGameOpts->flipy,         0, 0, 0);
	/* debugres */
	DataMapAdd(IDC_GAMMA,         DM_INT,  CT_SLIDER,   &g_nGammaIndex,            DM_DOUBLE, &pGameOpts->f_gamma_correct, 0, 0, AssignGamma);
	DataMapAdd(IDC_GAMMADISP,     DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_gamma_correct, 0, 0, 0);

	/* vector */
	DataMapAdd(IDC_ANTIALIAS,     DM_BOOL, CT_BUTTON,   &pGameOpts->antialias,     DM_BOOL, &pGameOpts->antialias,     0, 0, 0);
	DataMapAdd(IDC_TRANSLUCENCY,  DM_BOOL, CT_BUTTON,   &pGameOpts->translucency,  DM_BOOL, &pGameOpts->translucency,  0, 0, 0);
	DataMapAdd(IDC_BEAM,          DM_INT,  CT_SLIDER,   &g_nBeamIndex,             DM_DOUBLE, &pGameOpts->f_beam, 0, 0, AssignBeam);
	DataMapAdd(IDC_BEAMDISP,      DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_beam, 0, 0, 0);
	DataMapAdd(IDC_FLICKER,       DM_INT,  CT_SLIDER,   &g_nFlickerIndex,          DM_DOUBLE, &pGameOpts->f_flicker, 0, 0, AssignFlicker);
	DataMapAdd(IDC_FLICKERDISP,   DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_flicker, 0, 0, 0);
	DataMapAdd(IDC_INTENSITY,     DM_INT,  CT_SLIDER,   &g_nIntensityIndex,        DM_DOUBLE, &pGameOpts->f_intensity, 0, 0, AssignIntensity);
	DataMapAdd(IDC_INTENSITYDISP, DM_NONE, CT_NONE,  NULL,  DM_DOUBLE, &pGameOpts->f_intensity, 0, 0, 0);

	/* sound */
	DataMapAdd(IDC_SAMPLERATE,    DM_INT,  CT_COMBOBOX, &g_nSampleRateIndex,       DM_INT, &pGameOpts->samplerate, 0, 0, AssignSampleRate);
	DataMapAdd(IDC_SAMPLES,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_samples,   DM_BOOL, &pGameOpts->use_samples,   0, 0, 0);
	DataMapAdd(IDC_USE_FILTER,    DM_BOOL, CT_BUTTON,   &pGameOpts->use_filter,    DM_BOOL, &pGameOpts->use_filter,    0, 0, 0);
	DataMapAdd(IDC_USE_SOUND,     DM_BOOL, CT_BUTTON,   &pGameOpts->enable_sound,  DM_BOOL, &pGameOpts->enable_sound,  0, 0, 0);
	DataMapAdd(IDC_VOLUME,        DM_INT,  CT_SLIDER,   &g_nVolumeIndex,           DM_INT, &pGameOpts->attenuation, 0, 0, AssignVolume);
	DataMapAdd(IDC_VOLUMEDISP,    DM_NONE, CT_NONE,  NULL,  DM_INT, &pGameOpts->attenuation, 0, 0, 0);
	DataMapAdd(IDC_AUDIO_LATENCY, DM_INT,  CT_SLIDER,   &pGameOpts->audio_latency, DM_INT, &pGameOpts->audio_latency, 0, 0, 0);

	/* misc artwork options */
	DataMapAdd(IDC_ARTWORK,       DM_BOOL, CT_BUTTON,   &pGameOpts->use_artwork,   DM_BOOL, &pGameOpts->use_artwork,   0, 0, 0);
	DataMapAdd(IDC_BACKDROPS,     DM_BOOL, CT_BUTTON,   &pGameOpts->backdrops,     DM_BOOL, &pGameOpts->backdrops,     0, 0, 0);
	DataMapAdd(IDC_OVERLAYS,      DM_BOOL, CT_BUTTON,   &pGameOpts->overlays,      DM_BOOL, &pGameOpts->overlays,      0, 0, 0);
	DataMapAdd(IDC_BEZELS,        DM_BOOL, CT_BUTTON,   &pGameOpts->bezels,        DM_BOOL, &pGameOpts->bezels,        0, 0, 0);
	DataMapAdd(IDC_ARTRES,        DM_INT,  CT_COMBOBOX, &pGameOpts->artres,        DM_INT, &pGameOpts->artres,        0, 0, 0);
	DataMapAdd(IDC_ARTWORK_CROP,  DM_BOOL, CT_BUTTON,   &pGameOpts->artwork_crop,  DM_BOOL, &pGameOpts->artwork_crop,  0, 0, 0);

	/* misc */
	DataMapAdd(IDC_CHEAT,         DM_BOOL, CT_BUTTON,   &pGameOpts->cheat,         DM_BOOL, &pGameOpts->cheat,         0, 0, 0);
/*	DataMapAdd(IDC_DEBUG,         DM_BOOL, CT_BUTTON,   &pGameOpts->mame_debug,    DM_BOOL, &pGameOpts->mame_debug,    0, 0, 0);*/
	DataMapAdd(IDC_LOG,           DM_BOOL, CT_BUTTON,   &pGameOpts->errorlog,      DM_BOOL, &pGameOpts->errorlog,      0, 0, 0);
	DataMapAdd(IDC_SLEEP,         DM_BOOL, CT_BUTTON,   &pGameOpts->sleep,         DM_BOOL, &pGameOpts->sleep,         0, 0, 0);
	DataMapAdd(IDC_OLD_TIMING,    DM_BOOL, CT_BUTTON,   &pGameOpts->old_timing,    DM_BOOL, &pGameOpts->old_timing,    0, 0, 0);
	DataMapAdd(IDC_LEDS,          DM_BOOL, CT_BUTTON,   &pGameOpts->leds,          DM_BOOL, &pGameOpts->leds,          0, 0, 0);
	DataMapAdd(IDC_BIOS,          DM_INT,  CT_COMBOBOX, &pGameOpts->bios,          DM_INT, &pGameOpts->bios,        0, 0, 0);
#ifdef MESS
	DataMapAdd(IDC_USE_NEW_UI,    DM_BOOL, CT_BUTTON,   &pGameOpts->use_new_ui,    DM_BOOL, &pGameOpts->use_new_ui, 0, 0, 0);
#endif
#ifdef PINMAME
	DataMapAdd(IDC_DMD_RED,       DM_INT,  CT_SLIDER,   &pGameOpts->dmd_red,       DM_INT,  &pGameOpts->dmd_red,       0, 0, 0);
	DataMapAdd(IDC_DMD_GREEN,     DM_INT,  CT_SLIDER,   &pGameOpts->dmd_green,     DM_INT,  &pGameOpts->dmd_green,     0, 0, 0);
	DataMapAdd(IDC_DMD_BLUE,      DM_INT,  CT_SLIDER,   &pGameOpts->dmd_blue,      DM_INT,  &pGameOpts->dmd_blue,      0, 0, 0);
	DataMapAdd(IDC_DMD_PERC0,     DM_INT,  CT_SLIDER,   &pGameOpts->dmd_perc0,     DM_INT,  &pGameOpts->dmd_perc0,     0, 0, 0);
	DataMapAdd(IDC_DMD_PERC33,    DM_INT,  CT_SLIDER,   &pGameOpts->dmd_perc33,    DM_INT,  &pGameOpts->dmd_perc33,    0, 0, 0);
	DataMapAdd(IDC_DMD_PERC66,    DM_INT,  CT_SLIDER,   &pGameOpts->dmd_perc66,    DM_INT,  &pGameOpts->dmd_perc66,    0, 0, 0);
	DataMapAdd(IDC_DMD_ANTIALIAS, DM_INT,  CT_SLIDER,   &pGameOpts->dmd_antialias, DM_INT,  &pGameOpts->dmd_antialias, 0, 0, 0);
	DataMapAdd(IDC_DMD_ONLY,      DM_BOOL, CT_BUTTON,   &pGameOpts->dmd_only,      DM_BOOL, &pGameOpts->dmd_only,      0, 0, 0);
	DataMapAdd(IDC_DMD_COMPACT,   DM_BOOL, CT_BUTTON,   &pGameOpts->dmd_compact,   DM_BOOL, &pGameOpts->dmd_compact,   0, 0, 0);
#endif /* PINMAME */

}

BOOL IsControlDefaultValue(HWND hDlg,HWND hwnd_ctrl)
{
	int control_id = GetControlID(hDlg,hwnd_ctrl);

	options_type *default_options = GetDefaultOptions();

	// certain controls we need to handle specially
	switch (control_id)
	{
	case IDC_ASPECTRATION :
	{
		int n1=0, n2=0;

		sscanf(pGameOpts->aspect,"%i",&n1);
		sscanf(default_options->aspect,"%i",&n2);

		return n1 == n2;
	}
	case IDC_ASPECTRATIOD :
	{
		int temp, d1=0, d2=0;

		sscanf(pGameOpts->aspect,"%i:%i",&temp,&d1);
		sscanf(default_options->aspect,"%i:%i",&temp,&d2);

		return d1 == d2;
	}
	case IDC_SIZES :
	{
		int x1=0,y1=0,x2=0,y2=0;

		if (strcmp(pGameOpts->resolution,"auto") == 0 &&
			strcmp(default_options->resolution,"auto") == 0)
			return TRUE;
		
		sscanf(pGameOpts->resolution,"%d x %d",&x1,&y1);
		sscanf(default_options->resolution,"%d x %d",&x2,&y2);

		return x1 == y1 && x2 == y2;		
	}
	case IDC_RESDEPTH :
	{
		int temp,d1=0,d2=0;

		if (strcmp(pGameOpts->resolution,"auto") == 0 &&
			strcmp(default_options->resolution,"auto") == 0)
			return TRUE;
		
		sscanf(pGameOpts->resolution,"%d x %d x %d",&temp,&temp,&d1);
		sscanf(default_options->resolution,"%d x %d x %d",&temp,&temp,&d2);

		return d1 == d2;
	}
	case IDC_ROTATE :
	{
		ReadControl(hDlg,control_id);
	
		return pGameOpts->ror == default_options->ror &&
			pGameOpts->rol == default_options->rol;

	}
	}
	// most options we can compare using data in the data map
	if (IsControlDifferent(hDlg,hwnd_ctrl,pGameOpts,default_options))
		return FALSE;

	return TRUE;
}

static void SetStereoEnabled(HWND hWnd, int nIndex)
{
	BOOL enabled = FALSE;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	hCtrl = GetDlgItem(hWnd, IDC_STEREO);
	if (hCtrl)
	{
		if (nIndex == -1 || drv.sound_attributes & SOUND_SUPPORTS_STEREO)
			enabled = TRUE;

		EnableWindow(hCtrl, enabled);
	}
}

static void SetYM3812Enabled(HWND hWnd, int nIndex)
{
	int i;
	BOOL enabled;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	hCtrl = GetDlgItem(hWnd, IDC_USE_FM_YM3812);
	if (hCtrl)
	{
		enabled = FALSE;
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex == -1
#if HAS_YM3812
			||  drv.sound[i].sound_type == SOUND_YM3812
#endif
#if HAS_YM3526
			||  drv.sound[i].sound_type == SOUND_YM3526
#endif
#if HAS_YM2413
			||  drv.sound[i].sound_type == SOUND_YM2413
#endif
			)
				enabled = TRUE;
		}

		EnableWindow(hCtrl, enabled);
	}
}

static void SetSamplesEnabled(HWND hWnd, int nIndex, BOOL bSoundEnabled)
{
#if (HAS_SAMPLES == 1) || (HAS_VLM5030 == 1)
	int i;
	BOOL enabled = FALSE;
	HWND hCtrl;
    struct InternalMachineDriver drv;

	hCtrl = GetDlgItem(hWnd, IDC_SAMPLES);

	if (-1 != nIndex)
		expand_machine_driver(drivers[nIndex]->drv,&drv);

	if (hCtrl)
	{
		for (i = 0; i < MAX_SOUND; i++)
		{
			if (nIndex == -1
			||  drv.sound[i].sound_type == SOUND_SAMPLES
#if HAS_VLM5030
			||  drv.sound[i].sound_type == SOUND_VLM5030
#endif
			)
				enabled = TRUE;
		}
		enabled = enabled && bSoundEnabled;
		EnableWindow(hCtrl, enabled);
	}
#endif
}

/* Moved here cause it's called in a few places */
static void InitializeOptions(HWND hDlg)
{
	InitializeResDepthUI(hDlg);
	InitializeRefreshUI(hDlg);
	InitializeDisplayModeUI(hDlg);
	InitializeSoundUI(hDlg);
	InitializeSkippingUI(hDlg);
	InitializeRotateUI(hDlg);
	InitializeDefaultInputUI(hDlg);
	InitializeEffectUI(hDlg);
	InitializeArtresUI(hDlg);
	InitializeD3DFilterUI(hDlg);
	InitializeD3DEffectUI(hDlg);
	InitializeD3DPrescaleUI(hDlg);
	InitializeBIOSUI(hDlg);
	InitializeCleanStretchUI(hDlg);
}

/* Moved here because it is called in several places */
static void InitializeMisc(HWND hDlg)
{
	Button_Enable(GetDlgItem(hDlg, IDC_JOYSTICK), DIJoystick.Available());

	SendMessage(GetDlgItem(hDlg, IDC_GAMMA), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BRIGHTCORRECT), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_PAUSEBRIGHT), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 30)); /* [0.50, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BRIGHTNESS), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 38)); /* [0.10, 2.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_INTENSITY), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 50)); /* [0.50, 3.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_A2D), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 20)); /* [0.00, 1.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_FLICKER), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */

	SendMessage(GetDlgItem(hDlg, IDC_BEAM), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 300)); /* [1.00, 16.00] in .05 increments */

	SendMessage(GetDlgItem(hDlg, IDC_VOLUME), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 32)); /* [-32, 0] */
	SendMessage(GetDlgItem(hDlg, IDC_AUDIO_LATENCY), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(1, 4)); // [1, 4]
	SendMessage(GetDlgItem(hDlg, IDC_D3D_SCANLINES), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); // [0, 100]
	SendMessage(GetDlgItem(hDlg, IDC_D3D_FEEDBACK), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); // [0, 100]
	SendMessage(GetDlgItem(hDlg, IDC_ZOOM), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(1, 8)); // [1, 8]
#ifdef PINMAME
	SendMessage(GetDlgItem(hDlg, IDC_DMD_RED), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 255)); /* [0.0, 255.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_GREEN), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 255)); /* [0.0, 255.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_BLUE), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 255)); /* [0.0, 255.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_PERC0), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_PERC33), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_PERC66), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */
	SendMessage(GetDlgItem(hDlg, IDC_DMD_ANTIALIAS), TBM_SETRANGE,
				(WPARAM)FALSE,
				(LPARAM)MAKELONG(0, 100)); /* [0.0, 100.0] in 1.0 increments */
#endif /* PINMAME */
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
	if (hwndCtl == GetDlgItem(hwnd, IDC_BRIGHTCORRECT))
	{
		BrightCorrectSelectionChange(hwnd);
	}
	if (hwndCtl == GetDlgItem(hwnd, IDC_PAUSEBRIGHT))
	{
		PauseBrightSelectionChange(hwnd);
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
	if (hwndCtl == GetDlgItem(hwnd, IDC_FLICKER))
	{
		FlickerSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_VOLUME))
	{
		VolumeSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_INTENSITY))
	{
		IntensitySelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_A2D))
	{
		A2DSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_AUDIO_LATENCY))
	{
		AudioLatencySelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_D3D_SCANLINES))
	{
		D3DScanlinesSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_D3D_FEEDBACK))
	{
		D3DFeedbackSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_ZOOM))
	{
		ZoomSelectionChange(hwnd);
	}
#ifdef PINMAME
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_RED))
	{
		DMDREDSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_GREEN))
	{
		DMDGREENSelectionChange(hwnd);
	}
	else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_BLUE))
	{
		DMDBLUESelectionChange(hwnd);
	}
        else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_PERC0))
	{
		DMDPERC0SelectionChange(hwnd);
	}
        else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_PERC33))
	{
		DMDPERC33SelectionChange(hwnd);
	}
        else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_PERC66))
	{
		DMDPERC66SelectionChange(hwnd);
	}
        else
	if (hwndCtl == GetDlgItem(hwnd, IDC_DMD_ANTIALIAS))
	{
		DMDANTIALIASSelectionChange(hwnd);
	}
#endif /* PINMAME */
}

/* Handle changes to the Beam slider */
static void BeamSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dBeam;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BEAM), TBM_GETPOS, 0, 0);

	dBeam = nValue / 20.0 + 1.0;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf), "%03.2f", dBeam);
	Static_SetText(GetDlgItem(hwnd, IDC_BEAMDISP), buf);
}

/* Handle changes to the Flicker slider */
static void FlickerSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dFlicker;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_FLICKER), TBM_GETPOS, 0, 0);

	dFlicker = nValue;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf), "%03.2f", dFlicker);
	Static_SetText(GetDlgItem(hwnd, IDC_FLICKERDISP), buf);
}

/* Handle changes to the Gamma slider */
static void GammaSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dGamma;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_GAMMA), TBM_GETPOS, 0, 0);

	dGamma = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf), "%03.2f", dGamma);
	Static_SetText(GetDlgItem(hwnd, IDC_GAMMADISP), buf);
}

/* Handle changes to the Brightness Correction slider */
static void BrightCorrectSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BRIGHTCORRECT), TBM_GETPOS, 0, 0);

	dValue = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	_snprintf(buf, sizeof(buf), "%03.2f", dValue);
	Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTCORRECTDISP), buf);
}

/* Handle changes to the Pause Brightness slider */
static void PauseBrightSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_PAUSEBRIGHT), TBM_GETPOS, 0, 0);

	dValue = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	_snprintf(buf, sizeof(buf), "%03.2f", dValue);
	Static_SetText(GetDlgItem(hwnd, IDC_PAUSEBRIGHTDISP), buf);
}

/* Handle changes to the Brightness slider */
static void BrightnessSelectionChange(HWND hwnd)
{
	char   buf[100];
	int    nValue;
	double dBrightness;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_BRIGHTNESS), TBM_GETPOS, 0, 0);

	dBrightness = nValue / 20.0 + 0.1;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf),"%03.2f", dBrightness);
	Static_SetText(GetDlgItem(hwnd, IDC_BRIGHTNESSDISP), buf);
}

/* Handle changes to the Intensity slider */
static void IntensitySelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dIntensity;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_INTENSITY), TBM_GETPOS, 0, 0);

	dIntensity = nValue / 20.0 + 0.5;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf), "%03.2f", dIntensity);
	Static_SetText(GetDlgItem(hwnd, IDC_INTENSITYDISP), buf);
}

/* Handle changes to the A2D slider */
static void A2DSelectionChange(HWND hwnd)
{
	char   buf[100];
	UINT   nValue;
	double dA2D;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_A2D), TBM_GETPOS, 0, 0);

	dA2D = nValue / 20.0;

	/* Set the static display to the new value */
	_snprintf(buf,sizeof(buf), "%03.2f", dA2D);
	Static_SetText(GetDlgItem(hwnd, IDC_A2DDISP), buf);
}

/* Handle changes to the Color Depth drop down */
static void ResDepthSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		HWND hRefreshCtrl;
		int nResDepth = 0;
		int nRefresh  = 0;

		nResDepth = ComboBox_GetItemData(hWndCtrl, nCurSelection);

		hRefreshCtrl = GetDlgItem(hWnd, IDC_REFRESH);
		if (hRefreshCtrl)
		{
			nCurSelection = ComboBox_GetCurSel(hRefreshCtrl);
			if (nCurSelection != CB_ERR)
				nRefresh = ComboBox_GetItemData(hRefreshCtrl, nCurSelection);
		}

		UpdateDisplayModeUI(hWnd, nResDepth, nRefresh);
	}
}

/* Handle changes to the Refresh drop down */
static void RefreshSelectionChange(HWND hWnd, HWND hWndCtrl)
{
	int nCurSelection;

	nCurSelection = ComboBox_GetCurSel(hWndCtrl);
	if (nCurSelection != CB_ERR)
	{
		HWND hResDepthCtrl;
		int nResDepth = 0;
		int nRefresh  = 0;

		nRefresh = ComboBox_GetItemData(hWndCtrl, nCurSelection);

		hResDepthCtrl = GetDlgItem(hWnd, IDC_RESDEPTH);
		if (hResDepthCtrl)
		{
			nCurSelection = ComboBox_GetCurSel(hResDepthCtrl);
			if (nCurSelection != CB_ERR)
				nResDepth = ComboBox_GetItemData(hResDepthCtrl, nCurSelection);
		}

		UpdateDisplayModeUI(hWnd, nResDepth, nRefresh);
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
	_snprintf(buf,sizeof(buf), "%ddB", nValue - 32);
	Static_SetText(GetDlgItem(hwnd, IDC_VOLUMEDISP), buf);
}

static void AudioLatencySelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendMessage(GetDlgItem(hwnd,IDC_AUDIO_LATENCY), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	_snprintf(buffer,sizeof(buffer),"%i/5",value);
	Static_SetText(GetDlgItem(hwnd,IDC_AUDIO_LATENCY_DISP),buffer);

}

static void D3DScanlinesSelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendMessage(GetDlgItem(hwnd,IDC_D3D_SCANLINES), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	_snprintf(buffer,sizeof(buffer),"%i",value);
	Static_SetText(GetDlgItem(hwnd,IDC_D3D_SCANLINES_DISP),buffer);

}

static void D3DFeedbackSelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendMessage(GetDlgItem(hwnd,IDC_D3D_FEEDBACK), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	_snprintf(buffer,sizeof(buffer),"%i",value);
	Static_SetText(GetDlgItem(hwnd,IDC_D3D_FEEDBACK_DISP),buffer);

}

static void ZoomSelectionChange(HWND hwnd)
{
	char buffer[100];
	int value;

	// Get the current value of the control
	value = SendMessage(GetDlgItem(hwnd,IDC_ZOOM), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	_snprintf(buffer,sizeof(buffer),"%i",value);
	Static_SetText(GetDlgItem(hwnd,IDC_ZOOMDIST),buffer);

}
#ifdef PINMAME
/* Handle changes to the Color slider */
static void DMDREDSelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_RED), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_RED_DISP), buf);
}
static void DMDGREENSelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_GREEN), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_GREEN_DISP), buf);
}
static void DMDBLUESelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_BLUE), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_BLUE_DISP), buf);
}
static void DMDPERC0SelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_PERC0), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d%%", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_PERC0_DISP), buf);
}
static void DMDPERC33SelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_PERC33), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d%%", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_PERC33_DISP), buf);
}
static void DMDPERC66SelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_PERC66), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d%%", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_PERC66_DISP), buf);
}
static void DMDANTIALIASSelectionChange(HWND hwnd)
{
	char buf[100];
	int  nValue;

	/* Get the current value of the control */
	nValue = SendMessage(GetDlgItem(hwnd, IDC_DMD_ANTIALIAS), TBM_GETPOS, 0, 0);

	/* Set the static display to the new value */
	sprintf(buf, "%d%%", nValue);
	Static_SetText(GetDlgItem(hwnd, IDC_DMD_ANTIALIAS_DISP), buf);
}
#endif /* PINMAME */


/* Adjust possible choices in the Screen Size drop down */
static void UpdateDisplayModeUI(HWND hwnd, DWORD dwDepth, DWORD dwRefresh)
{
	int                   i;
	char                  buf[100];
	struct tDisplayModes* pDisplayModes;
	int                   nPick;
	int                   nCount = 0;
	int                   nSelection = 0;
	DWORD                 w = 0, h = 0;
	HWND                  hCtrl = GetDlgItem(hwnd, IDC_SIZES);

	if (!hCtrl)
		return;

	/* Find out what is currently selected if anything. */
	nPick = ComboBox_GetCurSel(hCtrl);
	if (nPick != 0 && nPick != CB_ERR)
	{
		ComboBox_GetText(GetDlgItem(hwnd, IDC_SIZES), buf, 100);
		if (sscanf(buf, "%lu x %lu", &w, &h) != 2)
		{
			w = 0;
			h = 0;
		}
	}

	/* Remove all items in the list. */
	ComboBox_ResetContent(hCtrl);

	ComboBox_AddString(hCtrl, "Auto");

	pDisplayModes = DirectDraw_GetDisplayModes();

	for (i = 0; i < pDisplayModes->m_nNumModes; i++)
	{
		if ((pDisplayModes->m_Modes[i].m_dwBPP     == dwDepth   || dwDepth   == 0)
		&&  (pDisplayModes->m_Modes[i].m_dwRefresh == dwRefresh || dwRefresh == 0))
		{
			sprintf(buf, "%li x %li", pDisplayModes->m_Modes[i].m_dwWidth,
									  pDisplayModes->m_Modes[i].m_dwHeight);

			if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
			{
				ComboBox_AddString(hCtrl, buf);
				nCount++;

				if (w == pDisplayModes->m_Modes[i].m_dwWidth
				&&  h == pDisplayModes->m_Modes[i].m_dwHeight)
					nSelection = nCount;
			}
		}
	}

	ComboBox_SetCurSel(hCtrl, nSelection);
}

/* Initialize the Display options to auto mode */
static void InitializeDisplayModeUI(HWND hwnd)
{
	UpdateDisplayModeUI(hwnd, 0, 0);
}

/* Initialize the sound options */
static void InitializeSoundUI(HWND hwnd)
{
	HWND    hCtrl;

	hCtrl = GetDlgItem(hwnd, IDC_SAMPLERATE);
	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "11025");
		ComboBox_AddString(hCtrl, "22050");
		ComboBox_AddString(hCtrl, "44100");
		ComboBox_SetCurSel(hCtrl, 1);
	}
}

/* Populate the Frame Skipping drop down */
static void InitializeSkippingUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_FRAMESKIP);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Draw every frame");
		ComboBox_AddString(hCtrl, "Skip 1 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 2 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 3 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 4 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 5 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 6 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 7 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 8 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 9 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 10 of 12 frames");
		ComboBox_AddString(hCtrl, "Skip 11 of 12 frames");
	}
}

/* Populate the Rotate drop down */
static void InitializeRotateUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ROTATE);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Default");             // 0
		ComboBox_AddString(hCtrl, "Clockwise");           // 1
		ComboBox_AddString(hCtrl, "Anti-clockwise");      // 2
		ComboBox_AddString(hCtrl, "None");                // 3
		ComboBox_AddString(hCtrl, "Auto clockwise");      // 4
		ComboBox_AddString(hCtrl, "Auto anti-clockwise"); // 5
	}
}

/* Populate the resolution depth drop down */
static void InitializeResDepthUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_RESDEPTH);

	if (hCtrl)
	{
		struct tDisplayModes* pDisplayModes;
		int nCount = 0;
		int i;

		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);

		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_SetItemData(hCtrl, nCount++, 0);

		pDisplayModes = DirectDraw_GetDisplayModes();

		for (i = 0; i < pDisplayModes->m_nNumModes; i++)
		{
			if (pDisplayModes->m_Modes[i].m_dwBPP == 16
			||  pDisplayModes->m_Modes[i].m_dwBPP == 24
			||  pDisplayModes->m_Modes[i].m_dwBPP == 32)
			{
				char buf[16];

				sprintf(buf, "%li bit", pDisplayModes->m_Modes[i].m_dwBPP);

				if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(hCtrl, nCount, buf);
					ComboBox_SetItemData(hCtrl, nCount++, pDisplayModes->m_Modes[i].m_dwBPP);
				}
			}
		}
	}
}

/* Populate the refresh drop down */
static void InitializeRefreshUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_REFRESH);

	if (hCtrl)
	{
		struct tDisplayModes* pDisplayModes;
		int nCount = 0;
		int i;

		/* Remove all items in the list. */
		ComboBox_ResetContent(hCtrl);

		ComboBox_AddString(hCtrl, "Auto");
		ComboBox_SetItemData(hCtrl, nCount++, 0);

		pDisplayModes = DirectDraw_GetDisplayModes();

		for (i = 0; i < pDisplayModes->m_nNumModes; i++)
		{
			if (pDisplayModes->m_Modes[i].m_dwRefresh != 0)
			{
				char buf[16];

				sprintf(buf, "%li Hz", pDisplayModes->m_Modes[i].m_dwRefresh);

				if (ComboBox_FindString(hCtrl, 0, buf) == CB_ERR)
				{
					ComboBox_InsertString(hCtrl, nCount, buf);
					ComboBox_SetItemData(hCtrl, nCount++, pDisplayModes->m_Modes[i].m_dwRefresh);
				}
			}
		}
	}
}

/* Populate the Default Input drop down */
static void InitializeDefaultInputUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_DEFAULT_INPUT);

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	char *ext;
	int isZipFile;
	char root[256];
	char path[256];

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Standard");

		sprintf (path, "%s\\*.*", GetCtrlrDir());

		hFind = FindFirstFile(path, &FindFileData);

	    if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (strcmp (FindFileData.cFileName,".") != 0 &&
					strcmp (FindFileData.cFileName,"..") != 0)
				{
					// copy the filename
					strcpy (root,FindFileData.cFileName);

					// assume it's not a zip file
					isZipFile = 0;

					// find the extension
					ext = strrchr (root,'.');
					if (ext)
					{
						// check if it's a zip file
						if (strcmp (ext, ".zip") == 0)
						{
							isZipFile = 1;
						}

						// and strip off the extension
						*ext = 0;
					}

					if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || isZipFile)
					{
						ComboBox_AddString(hCtrl, root);
					}
				}
			}
			while (FindNextFile (hFind, &FindFileData) != 0);
			
			FindClose (hFind);
		}
	}
}

static void InitializeEffectUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_EFFECT);

	if (hCtrl)
	{
		int i;
		for (i = 0; i < NUMEFFECTS; i++)
		{
			ComboBox_InsertString(hCtrl, i, g_ComboBoxEffect[i].m_pText);
			ComboBox_SetItemData( hCtrl, i, g_ComboBoxEffect[i].m_pData);
		}
	}
}

/* Populate the Art Resolution drop down */
static void InitializeArtresUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd, IDC_ARTRES);

	if (hCtrl)
	{
		ComboBox_AddString(hCtrl, "Auto");		/* 0 */
		ComboBox_AddString(hCtrl, "Standard");  /* 1 */
		ComboBox_AddString(hCtrl, "High");		/* 2 */
	}
}

static void InitializeD3DFilterUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_D3D_FILTER);

	if (hCtrl)
	{
		int i;

		for (i=0;i<MAX_D3D_FILTERS;i++)
			ComboBox_AddString(hCtrl,GetD3DFilterLongName(i));
	}
}

static void InitializeD3DEffectUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_D3D_EFFECT);

	if (hCtrl)
	{
		int i;

		for (i=0;i<MAX_D3D_EFFECTS;i++)
			ComboBox_AddString(hCtrl,GetD3DEffectLongName(i));
	}
}

static void InitializeD3DPrescaleUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_D3D_PRESCALE);

	if (hCtrl)
	{
		int i;

		for (i=0;i<MAX_D3D_PRESCALE;i++)
			ComboBox_AddString(hCtrl,GetD3DPrescaleLongName(i));
	}
}

static void InitializeBIOSUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_BIOS);

	if (hCtrl)
	{
		const struct GameDriver *gamedrv = drivers[g_nGame];
		const struct SystemBios *thisbios;
		
		if (g_nGame == -1)
		{
			ComboBox_AddString(hCtrl,"0");
			ComboBox_AddString(hCtrl,"1");
			ComboBox_AddString(hCtrl,"2");
			ComboBox_AddString(hCtrl,"3");
			ComboBox_AddString(hCtrl,"4");
			ComboBox_AddString(hCtrl,"5");
			ComboBox_AddString(hCtrl,"6");

			return;
		}
		if (DriverHasOptionalBIOS(g_nGame) == FALSE)
		{
			ComboBox_AddString(hCtrl,"None");
			return;
		}

		thisbios = gamedrv->bios;

		while (!BIOSENTRY_ISEND(thisbios))
		{
			ComboBox_AddString(hCtrl,thisbios->_description);
			thisbios++;
		}

	}
}

static void InitializeCleanStretchUI(HWND hwnd)
{
	HWND hCtrl = GetDlgItem(hwnd,IDC_CLEAN_STRETCH);

	if (hCtrl)
	{
		int i;

		for (i=0;i<MAX_CLEAN_STRETCH;i++)
			ComboBox_AddString(hCtrl,GetCleanStretchLongName(i));
	}
}

/* End of source file */
