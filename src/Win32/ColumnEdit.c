/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  ColumnEdit.c

  Column Edit dialog

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>

#include "resource.h"
#include "win32ui.h"

// Returns TRUE if successful
int DoExchangeItem(HWND hFrom, HWND hTo, int nMinItem)
{
    LV_ITEM lvi;
    char    buf[80];
    int     nFrom, nTo;

    nFrom = ListView_GetItemCount(hFrom);
    nTo   = ListView_GetItemCount(hTo);

    lvi.iItem = ListView_GetNextItem(hFrom, -1, LVIS_SELECTED | LVIS_FOCUSED);
    if (lvi.iItem < nMinItem)
    {
        if (lvi.iItem != -1) // Can't remove the first column
            MessageBox(0,"Cannot Move Selected Item", "Move Item", IDOK);
        SetFocus(hFrom);
        return FALSE;
    }
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.pszText = buf;
    lvi.cchTextMax = sizeof(buf);
    if (ListView_GetItem(hFrom, &lvi))
    {
        // Add this item to the Show and delete it from Available
        ListView_DeleteItem(hFrom, lvi.iItem);
        lvi.iItem = ListView_GetItemCount(hTo);
        ListView_InsertItem(hTo,&lvi);
        ListView_SetItemState(hTo, lvi.iItem,
            LVIS_FOCUSED | LVIS_SELECTED,
            LVIS_FOCUSED | LVIS_SELECTED);
        SetFocus(hTo);
        return lvi.iItem;
    }
    return FALSE;
}

void DoMoveItem( HWND hWnd, BOOL bDown)
{
    LV_ITEM lvi;
    char    buf[80];
    int     nMaxpos;
    
    lvi.iItem = ListView_GetNextItem(hWnd, -1, LVIS_SELECTED | LVIS_FOCUSED);
    nMaxpos = ListView_GetItemCount(hWnd);
    if (lvi.iItem == -1 || 
        (lvi.iItem < 2 && bDown == FALSE) || // Disallow moving First column
        (lvi.iItem == 0 && bDown == TRUE) || // ""
        (lvi.iItem == nMaxpos - 1 && bDown == TRUE))
    {
        SetFocus(hWnd);
        return;
    }
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM | LVIF_TEXT;
    lvi.pszText = buf;
    lvi.cchTextMax = sizeof(buf);
    if (ListView_GetItem(hWnd, &lvi))
    {
        // Add this item to the Show and delete it from Available
        ListView_DeleteItem(hWnd, lvi.iItem);
        lvi.iItem += (bDown) ? 1 : -1;
        ListView_InsertItem(hWnd,&lvi);
        ListView_SetItemState(hWnd, lvi.iItem,
            LVIS_FOCUSED | LVIS_SELECTED,
            LVIS_FOCUSED | LVIS_SELECTED);
        if (lvi.iItem == nMaxpos - 1)
            EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEDOWN), FALSE);
        else
            EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEDOWN), TRUE);

        if (lvi.iItem < 2)
            EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEUP),   FALSE);
        else
            EnableWindow(GetDlgItem(GetParent(hWnd), IDC_BUTTONMOVEUP),   TRUE);

        SetFocus(hWnd);
    }
}

INT_PTR CALLBACK ColumnDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hShown;
    static HWND hAvailable;
    static int  shown[COLUMN_MAX];
    static int  order[COLUMN_MAX];
    static BOOL showMsg = FALSE;
    int         nShown;
    int         nAvail;
    int         i, nCount = 0;
    LV_ITEM     lvi;

    switch (Msg)
    {
    case WM_INITDIALOG:
        hShown = GetDlgItem(hDlg, IDC_LISTSHOWCOLUMNS);
        hAvailable  = GetDlgItem(hDlg, IDC_LISTAVAILABLECOLUMNS);
        GetColumnOrder(order);
        GetColumnShown(shown);

        showMsg = TRUE;
        nShown = 0;
        nAvail = 0;
                        
        lvi.mask = LVIF_TEXT | LVIF_PARAM; 
        lvi.stateMask = 0;                 
        lvi.iSubItem  = 0; 
        lvi.iImage    = -1;

        /* Get the Column Order and save it */
        GetRealColumnOrder(order);

#if 0
        {
            char tmp[80];
            
            sprintf(tmp,"ColumnOrder: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                order[0], order[1], order[2], order[3], order[4],
                order[5], order[6], order[7], order[8], order[9]);
            MessageBox(0,tmp,"Column Order", IDOK);
        }
#endif
        for (i = 0 ; i < COLUMN_MAX; i++)
        {        
            lvi.pszText = column_names[order[i]];
            lvi.lParam  = order[i];

            if (shown[order[i]])
            {
                lvi.iItem = nShown;
                ListView_InsertItem(hShown,&lvi);
                nShown++;
            }
            else
            {
                lvi.iItem = nAvail;
                ListView_InsertItem(hAvailable,&lvi);
                nAvail++;
            }
        }
        return TRUE;

    case WM_NOTIFY:
        {
            NMHDR *nm = (NMHDR *)lParam;
            NM_LISTVIEW *pnmv;
            int         nPos;
            
            switch (nm->code)
            {
            case LVN_KEYDOWN:
                {
                    LV_KEYDOWN *pnkd = (LV_KEYDOWN *)nm;
                }
                break;
                
            case NM_DBLCLK :
                // Do Data Exchange here, which ListView was double clicked?
                switch (nm->idFrom)
                {
                case IDC_LISTAVAILABLECOLUMNS:
                    // Move selected Item from Available to Shown column
                    nPos = DoExchangeItem(hAvailable, hShown, 0);
                    if (nPos)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),      FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                    }
                    break;
                case IDC_LISTSHOWCOLUMNS:
                    // Move selected Item from Show to Available column
                    if (DoExchangeItem(hShown, hAvailable, 1))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD)  ,    TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                    }
                    break;
                }
                return TRUE;
                
            case LVN_ITEMCHANGED :
                // Don't handle this message for now
                pnmv = (NM_LISTVIEW *)nm;
                if (//!(pnmv->uOldState & LVIS_SELECTED) &&
                    (pnmv->uNewState  & LVIS_SELECTED))
                {
                    if (pnmv->iItem == 0 && pnmv->hdr.idFrom == IDC_LISTSHOWCOLUMNS)
                    {
                        // Don't allow selecting the first item
                        ListView_SetItemState(hShown, pnmv->iItem,
                            0, LVIS_FOCUSED | LVIS_SELECTED);
                        if (showMsg)
                        {
                            MessageBox(0,"Selecting this Item is not permitted", "Select Item", IDOK);
                            showMsg = FALSE;
                        }
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                        SetFocus(GetDlgItem(hDlg,IDOK));
                        return TRUE;
                    }
                    else
                        showMsg = TRUE;
                }
                break;
            case NM_CLICK:
                switch (nm->idFrom)
                {
                case IDC_LISTAVAILABLECOLUMNS:
                    if (ListView_GetItemCount(nm->hwndFrom) != 0)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),      TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
                    }
                    break;
                case IDC_LISTSHOWCOLUMNS:
                    if (ListView_GetItemCount(nm->hwndFrom) != 0)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),      FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
                    }
                    break;
                }
                return TRUE;
            }
        }
        return FALSE;

    case WM_COMMAND :
       {
            WORD wID         = GET_WM_COMMAND_ID(wParam, lParam);
            HWND hWndCtrl    = GET_WM_COMMAND_HWND(wParam, lParam);
            WORD wNotifyCode = GET_WM_COMMAND_CMD(wParam, lParam);
            int  nPos = 0;

            switch (wID)
            {
                case IDC_BUTTONADD:
                    // Move selected Item in Available to Shown
                    nPos = DoExchangeItem(hAvailable, hShown, 0);
                    if (nPos)
                    {
                        EnableWindow(hWndCtrl,FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONREMOVE),   TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                    }
                    break;

                case IDC_BUTTONREMOVE:
                    // Move selected Item in Show to Available
                    if (DoExchangeItem( hShown, hAvailable, 1))
                    {
                        EnableWindow(hWndCtrl,FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADD),      TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEUP),   FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BUTTONMOVEDOWN), FALSE);
                    }
                    break;

                case IDC_BUTTONMOVEDOWN:
                    // Move selected item in the Show window up 1 item
                    DoMoveItem(hShown, TRUE);
                    break;

                case IDC_BUTTONMOVEUP:
                    // Move selected item in the Show window down 1 item
                    DoMoveItem(hShown, FALSE);
                    break;

                case IDOK :
                    // Save users choices
                    nShown = ListView_GetItemCount(hShown);
                    nAvail = ListView_GetItemCount(hAvailable);
                    nCount = 0;
                    for (i = 0; i < nShown ; i++)
                    {
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_PARAM;
                        lvi.pszText = 0;
                        lvi.iItem = i;
                        ListView_GetItem(hShown, &lvi);
                        order[nCount++] = lvi.lParam;
                        shown[lvi.lParam] = TRUE;
                    }
                    for (i = 0; i < nAvail ; i++)
                    {
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_PARAM;
                        lvi.pszText = 0;
                        lvi.iItem = i;
                        ListView_GetItem(hAvailable, &lvi);
                        order[nCount++] = lvi.lParam;
                        shown[lvi.lParam] = FALSE;
                    }
#if 0
                    {
                        char tmp[80];
                        sprintf(tmp,"Shown (%d) - Hidden (%d)",nShown,nAvail);
                        MessageBox(0,tmp,"List Counts",IDOK);
                        sprintf(tmp,"ColumnOrder: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                            order[0], order[1], order[2], order[3], order[4],
                            order[5], order[6], order[7], order[8], order[9]);
                        MessageBox(0,tmp,"Column Order", IDOK);
                        sprintf(tmp,"ColumnShown: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                            shown[0], shown[1], shown[2], shown[3], shown[4],
                            shown[5], shown[6], shown[7], shown[8], shown[9]);
                        MessageBox(0,tmp,"Column Shown", IDOK);
                    }
#endif
                    SetColumnOrder(order);
                    SetColumnShown(shown);
                    EndDialog(hDlg, 1);
                    return TRUE;

                case IDCANCEL :
                    EndDialog(hDlg, 0);
                    return TRUE;
            }
        }
        break;
    }
    return 0;
}



