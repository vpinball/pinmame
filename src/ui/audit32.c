/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  audit32.c

  Audit dialog

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>

#include "resource.h"

#include <driver.h>
#include <audit.h>
#include <unzip.h>

#include "win32ui.h"
#include "m32util.h"
#include "audit32.h"
#include "Properties.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg);
static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void ProcessNextRom(void);
static void ProcessNextSample(void);
static void CLIB_DECL DetailsPrintf(const char *fmt, ...);
static const char * StatusString(int iStatus);

/***************************************************************************
    Internal variables
 ***************************************************************************/

#define SAMPLES_NOT_USED    3

HWND hAudit;
static int rom_index;
static int roms_correct;
static int roms_incorrect;
static int sample_index;
static int samples_correct;
static int samples_incorrect;

static BOOL bPaused = FALSE;
static BOOL bCancel = FALSE;

/***************************************************************************
    External functions  
 ***************************************************************************/

void AuditDialog(HWND hParent)
{
	rom_index         = 0;
	roms_correct      = 0;
	roms_incorrect    = 0;
	sample_index      = 0;
	samples_correct   = 0;
	samples_incorrect = 0;

	DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_AUDIT),hParent,AuditWindowProc);
}

void InitGameAudit(int gameIndex)
{
	rom_index = gameIndex;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg)
{
	char buffer[200];

	while (!bCancel)
	{
		if (!bPaused)
		{
			if (rom_index != -1)
			{
				sprintf(buffer, "Checking Game %s - %s",
					drivers[rom_index]->name, drivers[rom_index]->description);
				SetWindowText(hDlg, buffer);
				ProcessNextRom();
			}
			else
			{
				if (sample_index != -1)
				{
					sprintf(buffer, "Checking Game %s - %s",
						drivers[sample_index]->name, drivers[sample_index]->description);
					SetWindowText(hDlg, buffer);
					ProcessNextSample();
				}
				else
				{
					sprintf(buffer, "%s", "File Audit");
					SetWindowText(hDlg, buffer);
					EnableWindow(GetDlgItem(hDlg, IDPAUSE), FALSE);
					ExitThread(1);
				}
			}
		}
	}
	return 0;
}

static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread;
	static DWORD dwThreadID;
	DWORD dwExitCode;

	switch (Msg)
	{
	case WM_INITDIALOG:
		hAudit = hDlg;
		SendDlgItemMessage(hDlg, IDC_ROMS_PROGRESS,    PBM_SETRANGE, 0, MAKELPARAM(0, GetNumGames()));
		SendDlgItemMessage(hDlg, IDC_SAMPLES_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, GetNumGames()));
		bPaused = FALSE;
		bCancel = FALSE;
		rom_index = 0;

		hThread = CreateThread(NULL, 0, AuditThreadProc, hDlg, 0, &dwThreadID);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
            bPaused = FALSE;
			if (hThread)
			{
				bCancel = TRUE;
				if (GetExitCodeThread(hThread, &dwExitCode) && dwExitCode == STILL_ACTIVE)
				{
					PostMessage(hDlg, WM_COMMAND, wParam, lParam);
					return 1;
				}
				CloseHandle(hThread);
			}
			EndDialog(hDlg,0);
			break;

		case IDPAUSE:
			if (bPaused)
			{
				SendDlgItemMessage(hAudit, IDPAUSE, WM_SETTEXT, 0, (LPARAM)"Pause");
				bPaused = FALSE;
			}
			else
			{
				SendDlgItemMessage(hAudit, IDPAUSE, WM_SETTEXT, 0, (LPARAM)"Continue");
				bPaused = TRUE;
			}
			break;
		}
		return 1;
	}
	return 0;
}

#ifdef MESS
/* Checks to see if this system even has a ROM set */
static BOOL HasRomSet(int nDriver)
{
	const struct GameDriver *gamedrv = drivers[nDriver];
	const struct RomModule *region, *rom;

	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			return TRUE;
	return FALSE;
}
#endif

/* Callback for the Audit property sheet */
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		FlushFileCaches();
		hAudit = hDlg;
		Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(rom_index));
		SetTimer(hDlg, 0, 1, NULL);
		return 1;

	case WM_TIMER:
		KillTimer(hDlg, 0);
		{
			int iStatus;

			iStatus = VerifyRomSet(rom_index, (verify_printf_proc)DetailsPrintf);
#ifdef MESS
			SetHasRoms(rom_index, (iStatus == CORRECT || iStatus == BEST_AVAILABLE || !HasRomSet(rom_index)) ? 1 : 0);
#else
			SetHasRoms(rom_index, (iStatus == CORRECT || iStatus == BEST_AVAILABLE) ? 1 : 0);
#endif
			SetWindowText(GetDlgItem(hDlg, IDC_PROP_ROMS), StatusString(iStatus));

			iStatus = VerifySampleSet(rom_index, (verify_printf_proc)DetailsPrintf);
			SetHasSamples(rom_index,(iStatus == CORRECT || iStatus == BEST_AVAILABLE));
			if (DriverUsesSamples(rom_index))
				SetWindowText(GetDlgItem(hDlg, IDC_PROP_SAMPLES),StatusString(iStatus));
			else
				SetWindowText(GetDlgItem(hDlg, IDC_PROP_SAMPLES),"None required");
		}
		ShowWindow(hDlg, SW_SHOW);
		break;
	}
	return 0;
}

static void ProcessNextRom()
{
	int retval;
	char buffer[200];

	retval = VerifyRomSet(rom_index, (verify_printf_proc)DetailsPrintf);
	switch (retval)
	{
	case BEST_AVAILABLE: /* correct, incorrect or separate count? */
	case CORRECT:
		roms_correct++;
		sprintf(buffer, "%i", roms_correct);
		SendDlgItemMessage(hAudit, IDC_ROMS_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		sprintf(buffer, "%i", roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;

	case NOTFOUND:
		break;

	case INCORRECT:
		roms_incorrect++;
		sprintf(buffer, "%i", roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		sprintf(buffer, "%i", roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;
	}

#ifdef MESS
	SetHasRoms(rom_index, (retval == CORRECT || retval == BEST_AVAILABLE || !HasRomSet(rom_index)) ? 1 : 0);
#else
	SetHasRoms(rom_index, (retval == CORRECT || retval == BEST_AVAILABLE) ? 1 : 0);
#endif
	rom_index++;
	SendDlgItemMessage(hAudit, IDC_ROMS_PROGRESS, PBM_SETPOS, rom_index, 0);

	if (rom_index == GetNumGames())
	{
		sample_index = 0;
		rom_index = -1;
	}
}

static void ProcessNextSample()
{
	int  retval;
	char buffer[200];
	
	retval = VerifySampleSet(sample_index, (verify_printf_proc)DetailsPrintf);
	
	switch (retval)
	{
	case CORRECT:
		if (DriverUsesSamples(sample_index))
		{
			samples_correct++;
			sprintf(buffer, "%i", samples_correct);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
			sprintf(buffer, "%i", samples_correct + samples_incorrect);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
			break;
		}

	case NOTFOUND:
		break;
			
	case INCORRECT:
		samples_incorrect++;
		sprintf(buffer, "%i", samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		sprintf(buffer, "%i", samples_correct + samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		
		break;
	}
	
	SetHasSamples(sample_index, (retval == CORRECT || retval == BEST_AVAILABLE) ? 1 : 0);

	sample_index++;
	SendDlgItemMessage(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETPOS, sample_index, 0);
	
	if (sample_index == GetNumGames())
	{
		DetailsPrintf("Audit complete.\n");
		SendDlgItemMessage(hAudit, IDCANCEL, WM_SETTEXT, 0, (LPARAM)"Close");
		sample_index = -1;
	}
	
}

static void CLIB_DECL DetailsPrintf(const char *fmt, ...)
{
	HWND	hEdit;
	va_list marker;
	char	buffer[2000];
	char * s;
	
	hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
	
	va_start(marker, fmt);
	
	vsprintf(buffer, fmt, marker);
	
	va_end(marker);

	s = ConvertToWindowsNewlines(buffer);

	Edit_SetSel(hEdit, Edit_GetTextLength(hEdit), Edit_GetTextLength(hEdit));
	Edit_ReplaceSel(hEdit, s);
}

static const char * StatusString(int iStatus)
{
	static const char *ptr;

	ptr = "Unknown";

	switch (iStatus)
	{
	case CORRECT:
		ptr = "Passed";
		break;
		
	case BEST_AVAILABLE:
		ptr = "Best available";
		break;
		
	case CLONE_NOTFOUND:
	case NOTFOUND:
		ptr = "Not found";
		break;
		
	case INCORRECT:
		ptr = "Failed";
		break;
	}

	return ptr;
}
