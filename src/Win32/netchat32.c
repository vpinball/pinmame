/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  netchat32.c

  NetChat dialog

***************************************************************************/

#ifdef MAME_NET

#include "driver.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>

#include "resource.h"

#include "win32ui.h"
#include "netchat32.h"
#include "inptport.h"
#include "network.h"
#include "TreeView.h"
#include "net32.h"      // for OSDNetwork structure
#include "options.h"    // for net_options structure
#include "Properties.h" // For GetGameInfo();
#include "MAME32.h"     // for MAME32App structure

/***************************************************************************
    function prototypes
 ***************************************************************************/
void                    AddChatText(HWND hWnd, char *buf);

static int              InitListView(HWND hWnd);
static int              GetSelectedItem(HWND hWnd, int game_index);
static LRESULT CALLBACK NetChatWindowProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam);
static BOOL             NetChatCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static BOOL             NetChatNotify(HWND hWnd, NMHDR *nm);

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND             hNetChat;
static HWND             hwndGameList;
static HWND             hMain;
static net_options *    netOpts;

/***************************************************************************
    External functions  
 ***************************************************************************/

BOOL NetChatDialog(HWND hParent)
{
	hMain   = hParent;
    netOpts = GetNetOptions();

	ShowWindow(hMain, SW_HIDE);

	return (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_NETCHAT),
                      hMain, NetChatWindowProc));
    //hNetChat = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_NETCHAT),
    //                        0, NetChatWindowProc);
    //return TRUE;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

/* Populate the game list for the chat window */
static int InitListView(HWND hWnd)
{
    RECT            rect;
    int             i, num_games;
    LV_ITEM         lvi;
    LV_COLUMN       lvc;
    LV_FINDINFO     lvfi;
    LPTREEFOLDER    lpFolder = GetFolderByID(FOLDER_AVAILABLE);
    char            buf[100];

    if (! lpFolder)
        return 0;

    GetClientRect(hWnd, &rect);

    ListView_SetExtendedListViewStyle(hWnd, LVS_EX_FULLROWSELECT);

    lvc.mask        = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem    = 0;
    lvc.cx          = rect.right - rect.left; 
    lvc.cx         -= GetSystemMetrics(SM_CXVSCROLL);

    ListView_InsertColumn(hWnd, 0, &lvc);

    SetWindowRedraw(hWnd,FALSE);

    ListView_DeleteAllItems(hWnd);

    ListView_SetItemCount(hWnd, GetNumGames());

    i = -1;

    num_games  = 0;
    do
    {
        /* Add the games that are in this folder */
        if ((i = FindGame(lpFolder, i + 1)) != -1)
        {
            if (GameFiltered(i, lpFolder->m_dwFlags) || GameUsesTrackball(i))
                continue;
         
            strcpy(buf, drivers[i]->description);

            lvi.mask        = LVIF_TEXT | LVIF_PARAM; 
            lvi.stateMask   = 0;
            lvi.iItem       = num_games++;
            lvi.iSubItem    = 0; 
            lvi.lParam      = i;
            lvi.iImage      = -1;
            lvi.pszText     = buf;
            ListView_InsertItem(hWnd,&lvi);
        }
    } while (i != -1);
    
    SetWindowRedraw(hWnd,TRUE);

    lvfi.flags  = LVFI_STRING;
    lvfi.psz    = GetDefaultGame();

    if ((i = ListView_FindItem(hWnd, -1, &lvfi)) == -1)
        i = 0;
   
    ListView_SetItemState(hWnd, i, LVIS_FOCUSED | LVIS_SELECTED,
                                   LVIS_FOCUSED | LVIS_SELECTED);
    ListView_EnsureVisible(hWnd, i, FALSE);

    return GetSelectedItem(hWnd, i);
}

/* Set the list selection to match the passed in string */
static int SetSelectedItem(HWND hWnd, char * game_title)
{
    LV_FINDINFO lvfi;
    int         i;

    lvfi.flags  = LVFI_STRING;
    lvfi.psz    = game_title;

    if ((i = ListView_FindItem(hWnd, -1, &lvfi)) == -1)
        i = 0;
   
    ListView_SetItemState(hWnd, i, LVIS_FOCUSED | LVIS_SELECTED,
                                   LVIS_FOCUSED | LVIS_SELECTED);
    ListView_EnsureVisible(hWnd, i, FALSE);
    return i;
}


/* returns lParam (game driver index) of listview selected item */
static int GetSelectedItem(HWND hWnd, int game_index)
{
    LV_ITEM lvi;

    lvi.iItem = game_index;
    if (lvi.iItem == -1)
        return 0;

    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    ListView_GetItem(hWnd, &lvi);

    return lvi.lParam;
}

/* Chat dialog message handler */
static LRESULT CALLBACK NetChatWindowProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
    HWND hCtrl;
    int  iGame;
    char buf[MAX_CHAT_LINE];
    int  player;

    switch (Msg)
    {
    case WM_INITDIALOG :
        hNetChat = hDlg; /* Store this for future use */
		/* Do initialization here
		 * Grey out 'start game' and 'kick player' boxes on client side
		 * Clear all list boxes and text boxes
		 * Fill list box with supported games
         */
        /* Common setup */
        ComboBox_ResetContent(GetDlgItem(hDlg, IDC_CHAT_PLAYER_LIST));
       
        hwndGameList = GetDlgItem(hDlg, IDC_CHAT_GAME_LIST);
        iGame = InitListView(hwndGameList);
        Static_SetText(GetDlgItem(hDlg, IDC_PROP_TITLE), GameInfoTitle(iGame));
        
        if (hCtrl = GetDlgItem(hDlg, IDC_CHAT_PLAYER_COMBO))
        {
            ComboBox_ResetContent(hCtrl);
            ComboBox_AddString(hCtrl, "Player 1");
            ComboBox_AddString(hCtrl, "Player 2");
            ComboBox_AddString(hCtrl, "Player 3");
            ComboBox_AddString(hCtrl, "Player 4");
            ComboBox_AddString(hCtrl, "Spectator");
            ComboBox_SetCurSel(hCtrl, 0);
        }

        if (netOpts->net_tcpip_mode == NET_ID_PLAYER_SERVER)
        {
            /* Server setup */
            if (hCtrl = GetDlgItem(hDlg, IDC_CHAT_PLAYER_LIST))
            {
                ListBox_ResetContent(hCtrl);
                ListBox_AddString(hCtrl, netOpts->net_player);
                ListBox_SetCurSel(hCtrl, 0);
            }
            SetWindowText(hDlg,MAME32NAME " Network Chat Server");
        }
        else
        {
            /* Client setup */
            Button_Enable(GetDlgItem(hDlg, IDC_CHAT_START_GAME), FALSE);
            Button_Enable(GetDlgItem(hDlg, IDC_CHAT_KICK), FALSE);
            SetWindowText(hDlg,MAME32NAME " Network Chat Client");
            ListBox_ResetContent(hCtrl);
            {
                char buf[300];
                int  buflen = strlen(netOpts->net_player);

                sprintf(buf, "JOIN%*.*s", buflen, buflen, netOpts->net_player);
                buflen = strlen(buf);
                if (MAME32App.m_pNetwork != NULL)
                    MAME32App.m_pNetwork->chat_send(buf,&buflen);
            }
        }
        Network_TCPIP_poll_sockets(hDlg, netOpts->net_tcpip_mode == NET_ID_PLAYER_SERVER);
        ShowWindow(hDlg, SW_SHOW);
        return 1;

    case WM_WINSOCK_SELECT:         /* Socket event */
        OnWinsockSelect(hDlg, wParam, lParam);
        return 1;

    case WM_WINSOCK_START_GAME:     /* Play game */
        ComboBox_GetText(GetDlgItem(hDlg, IDC_CHAT_PLAYER_COMBO), buf, 100);

		if (sscanf(buf, "Player %d", &player) == 0)
        	set_default_player_controls(NET_SPECTATOR);
		else
        	set_default_player_controls(player);
        EndDialog(hDlg,1);
        return 1;

    case WM_WINSOCK_SELECT_GAME:    /* Game selection changed */
        SetSelectedItem(GetDlgItem(hDlg, IDC_CHAT_GAME_LIST), (char *)lParam);
        return 1;

    case WM_WINSOCK_CHAT_JOIN:      /* someone joined chat */
        ListBox_AddString(GetDlgItem(hDlg, IDC_CHAT_PLAYER_LIST), (char *)lParam);
        return 1;

    case WM_WINSOCK_CHAT_EXIT:      /* recieved exit message */
        {
            char buf[100];
            int  nItem;
            HWND hCtrl = GetDlgItem(hDlg, IDC_CHAT_PLAYER_LIST);
            
            if ((nItem = ListBox_FindString(hCtrl, -1, (char *)lParam)) != -1)
                ListBox_DeleteString(hCtrl, nItem);
            sprintf(buf, "(Exit) %s", lParam);
            AddChatText(hDlg, buf);
        }
        return 1;

    case WM_WINSOCK_CHAT_TEXT:      /* recieved text message */
        AddChatText(hDlg, (char *)lParam);
        return 1;

    case WM_COMMAND :
		return NetChatCommand(hDlg,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

    case WM_NOTIFY:
        return NetChatNotify(hDlg, (LPNMHDR)lParam);

    case WM_CLOSE:
        Network_TCPIP_end_chat();
        EndDialog(hDlg,0);
	    ShowWindow(hMain, SW_SHOW);
        return TRUE;

	}
    return 0;
}

static BOOL NetChatCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	char buf[MAX_CHAT_LINE];
	char buf2[MAX_CHAT_LINE+2];
    int  buflen;

    switch (id)
    {
    case IDC_CHAT_START_GAME:   /* This button is only enabled on the server */
        if (codeNotify == BN_CLICKED)
        {
            /* force the other end to select the correct game */
            sprintf(buf, "GAME%s", GetDefaultGame());
            buflen = strlen(buf);
            if (MAME32App.m_pNetwork != NULL)
                MAME32App.m_pNetwork->chat_send(buf,&buflen);

            /* force the clients to play the game */
            strcpy(buf,"PLAYLet's go!");
            buflen = strlen(buf);
            if ( MAME32App.m_pNetwork != NULL )
                MAME32App.m_pNetwork->chat_send(buf,&buflen);
            Sleep(500);
            return TRUE;
		}
        
    case IDC_CHAT_CANCEL:
        if (codeNotify == BN_CLICKED)
		{
			// TODO:
			// tell the other peers we are leaving
			// close network connection gracefully
            sprintf(buf, "EXIT%s", netOpts->net_player);
            buflen = strlen(buf);
            if (MAME32App.m_pNetwork != NULL)
                MAME32App.m_pNetwork->chat_send(buf,&buflen);
            Network_TCPIP_end_chat();
			EndDialog(hwnd,0);
			ShowWindow(hMain, SW_SHOW);
			return TRUE;
		}
		
    case IDC_CHAT_KICK :
        if (codeNotify == BN_CLICKED)
		{
			// TODO:
			// (should only be server sending this message)
			// prompt them if they are sure they want to kick the other player
			// if yes, send message to target player
            // kill network connection to them
			return TRUE;
		}

	case IDC_CHAT_BTN :
        if (codeNotify == BN_CLICKED)
		{
			// TODO:
			// send text to server (who will bounce to everyone)
            Edit_GetText(GetDlgItem(hwnd,IDC_CHAT_TEXT),buf2,MAX_CHAT_LINE);
			sprintf(buf, "CHAT%s : %s\r\n", netOpts->net_player, buf2);
			Edit_SetText(GetDlgItem(hwnd, IDC_CHAT_TEXT), "");
            buflen = strlen(buf);
            if ( MAME32App.m_pNetwork != NULL )
		        MAME32App.m_pNetwork->chat_send(buf,&buflen);
            return TRUE;
		}
    }
    return FALSE;
}

/* Add chat message to the chat history window */
void AddChatText(HWND hWnd, char *buf)
{
    HWND hCtrl = GetDlgItem(hWnd, IDC_CHAT_LOG_TEXT);

    if (hCtrl)
    {
        Edit_SetSel(hCtrl, Edit_GetTextLength(hCtrl), Edit_GetTextLength(hCtrl));
	    Edit_ReplaceSel(hCtrl,buf);
    }
}

/* Handle game selection */
static BOOL NetChatNotify(HWND hWnd, NMHDR *nm)
{
    NM_LISTVIEW *pnmv;
    static int  nLastItem = -1;

    switch (nm->code)
    {
    case NM_RCLICK:
    case NM_CLICK:
    case NM_DBLCLK:
        /* Disable game selection on the client */
        if (netOpts->net_tcpip_mode != NET_ID_PLAYER_SERVER)
        {
            HWND hCtrl = GetDlgItem(hWnd, IDC_CHAT_GAME_LIST);
            if (nLastItem != -1)
                SetSelectedItem(hCtrl, (char *)drivers[nLastItem]->description);
            return TRUE;
        }
        break;
    case LVN_ITEMCHANGED :
        pnmv = (NM_LISTVIEW *)nm;

        if ((pnmv->uOldState & LVIS_SELECTED) 
            && !(pnmv->uNewState & LVIS_SELECTED))
        {
            /* leaving item */
            if (pnmv->lParam != -1)
            {
                nLastItem = pnmv->lParam;
            }
            /* printf("leaving %s\n",drivers[pnmv->lParam]->name); */
        }

        if (!(pnmv->uOldState & LVIS_SELECTED) 
            && (pnmv->uNewState & LVIS_SELECTED))
        {
            /* printf("entering %s\n",drivers[pnmv->lParam]->name); */
            Static_SetText(GetDlgItem(hWnd, IDC_PROP_TITLE),
                           GameInfoTitle(pnmv->lParam));
            SetDefaultGame(drivers[pnmv->lParam]->description);
            if (netOpts->net_tcpip_mode == NET_ID_PLAYER_SERVER)
            {
                char buf[300];
                int  buflen;

                sprintf(buf, "GAME%s",drivers[pnmv->lParam]->description);
                buflen = strlen(buf);
                if (MAME32App.m_pNetwork != NULL)
                    MAME32App.m_pNetwork->chat_send(buf,&buflen);
            }
        }
        return TRUE;
    }
    return FALSE;
}

#endif /* MAME_NET */
