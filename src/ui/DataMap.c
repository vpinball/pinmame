
/* DataMap.c - Data Exchange routines by Mike Haaland <mhaaland@hypertech.com> */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "resource.h"
#include "options.h"
#include "DataMap.h"

static DATA_MAP dataMap[100];
static int      numCtrls = 0;

static BOOL validate(HWND hCtrl, DATA_MAP *lpDmap);

void DataMapAdd(DWORD dwCtrlId, DataMapType dmType, CtrlType cType, void *var, int min, int max, void (*func)(HWND hWnd))
{
	DATA_MAP	*lpDmap = 0;
	int 		i;
	int 		ctrlNum = -1;

	for (i = 0; i < numCtrls; i++)
	{
		if (dwCtrlId == dataMap[i].dwCtrlId)
		{
			ctrlNum = i;
			break;
		}
	}
	lpDmap = &dataMap[(ctrlNum < 0) ? numCtrls : ctrlNum];

	lpDmap->dwCtrlId = dwCtrlId;
	lpDmap->dmType   = dmType;
	lpDmap->cType    = cType;
	lpDmap->func     = func;

	switch (dmType)
	{
	case DM_BOOL:
		lpDmap->bValue = *(BOOL*)var;
		lpDmap->bVar   = (BOOL*)var;
		if (ctrlNum == -1)
			numCtrls++;
		break;

	case DM_INT:
		lpDmap->nValue = *(int*)var;
		lpDmap->nMin   = min;
		lpDmap->nMax   = max;
		lpDmap->nVar   = (int*)var;
		if (ctrlNum == -1)
			numCtrls++;
		break;

	case DM_NONE:
		break;
	}
}

void PopulateControls(HWND hWnd)
{
	int       i;
	HWND      hCtrl;
	DATA_MAP* lpDmap;

	for (i = 0; i < numCtrls; i++)
	{
		lpDmap = &dataMap[i];
		hCtrl = GetDlgItem(hWnd, lpDmap->dwCtrlId);
		if (hCtrl == 0)
			continue;
		
		switch (lpDmap->cType)
		{
		case CT_BUTTON:
			Button_SetCheck(hCtrl, lpDmap->bValue);
			break;

		case CT_COMBOBOX:
			ComboBox_SetCurSel(hCtrl, lpDmap->nValue);
			break;

		case CT_SLIDER:
			SendMessage(hCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)lpDmap->nValue);
			break;

		case CT_NONE:
			break;
		}
	}
}

void ReadControls(HWND hWnd)
{
	int       i;
	HWND      hCtrl;
	DATA_MAP* lpDmap;

	for (i = 0; i < numCtrls; i++)
	{
		lpDmap = &dataMap[i];

		hCtrl = GetDlgItem(hWnd, lpDmap->dwCtrlId);
		if (hCtrl == 0)
			continue;

		switch (lpDmap->cType)
		{
		case CT_BUTTON:
			lpDmap->bValue = Button_GetCheck(hCtrl);
			validate(hCtrl, lpDmap);
			break;

		case CT_COMBOBOX:
			lpDmap->nValue = ComboBox_GetCurSel(hCtrl);
			validate(hCtrl, lpDmap);
			break;

		case CT_SLIDER:
			lpDmap->nValue = SendMessage(hCtrl, TBM_GETPOS, 0, 0);
			validate(hCtrl, lpDmap);
			break;

		case CT_NONE:
			break;
		}
	}
}

/* Returns TRUE if the control is in the list
 * and the new value is different than before
 */
BOOL ReadControl(HWND hWnd, DWORD dwCtrlId)
{
	int 	  i;
	HWND	  hCtrl;
	DATA_MAP* lpDmap = 0;

	for (i = 0; i < numCtrls; i++)
	{
		if (dwCtrlId == dataMap[i].dwCtrlId)
		{
			lpDmap = &dataMap[i];
			break;
		}
	}
	if (lpDmap && (hCtrl = GetDlgItem(hWnd, lpDmap->dwCtrlId)) != 0)
	{
		switch (lpDmap->cType)
		{
		case CT_BUTTON:
			lpDmap->bValue = Button_GetCheck(hCtrl);
			return validate(hCtrl, lpDmap);

		case CT_COMBOBOX:
			lpDmap->nValue = ComboBox_GetCurSel(hCtrl);
			return validate(hCtrl, lpDmap);

		case CT_SLIDER:
			lpDmap->nValue = SendMessage(hCtrl, TBM_GETPOS, 0, 0);
			return validate(hCtrl, lpDmap);

		case CT_NONE:
			break;
		}
	}
	return FALSE;
}

static BOOL validate(HWND hCtrl, DATA_MAP *lpDmap)
{
	switch (lpDmap->dmType)
	{
	case DM_BOOL:
		*lpDmap->bVar = lpDmap->bValue;
		if (lpDmap->func)
			lpDmap->func(hCtrl);
		return TRUE;

	case DM_INT:
		if (lpDmap->nMin != lpDmap->nMax) // Validate iff a range has been set
		{
			if (lpDmap->nValue < lpDmap->nMin && lpDmap->nValue > lpDmap->nMax)
			{
				lpDmap->nValue = *lpDmap->nVar;
				switch (lpDmap->cType)
				{
				case CT_COMBOBOX:
					ComboBox_SetCurSel(hCtrl, lpDmap->nValue);
					break;

				case CT_SLIDER:
					SendMessage(hCtrl, TBM_GETPOS, (WPARAM)TRUE, (LPARAM)lpDmap->nValue);
					break;

				case CT_BUTTON: /* ? */
				case CT_NONE:
					break;
				}
				return FALSE;
			}
		}

		*lpDmap->nVar = lpDmap->nValue;
		
		if (lpDmap->func)
			lpDmap->func(hCtrl);
		return TRUE;

	case DM_NONE:
		break;
	}
	return FALSE;
}

void InitDataMap(void)
{
	memset(&dataMap, '\0', sizeof(DATA_MAP) * 100);
	numCtrls = 0;
}

/* End of source file */
