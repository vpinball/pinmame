/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
	Network Portions Copyright (C) 1998 Peter Eberhardy and Michael Adcock
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  net32.h

 ***************************************************************************/
#ifdef MAME_NET
#ifndef __NET32_H__
#define __NET32_H__

#include "osdepend.h"
#include "network.h"

extern struct OSDNetwork Network;

struct OSDNetwork 
{
	int  (*init)(void);
    int  (*game_init)(void);
	int  (*send)(int player, unsigned char buf[], int *size);
	int  (*recv)(int player, unsigned char buf[], int *size);
	int  (*sync)(void);
	int  (*input_sync)(void);
    int  (*game_exit)(void);
	int  (*exit)(void);
	int  (*add_player)(void);
	int  (*remove_player)(int player);
    int  (*chat_send)(unsigned char buf[], int *size);
	int  (*chat_recv)(int player, unsigned char buf[], int *size);
};

/* Enable windows socket messages */
void Network_TCPIP_poll_sockets(HWND hDlg, BOOL server);
void OnWinsockSelect(HWND hWnd, WPARAM wParam, LPARAM lParam);
void Network_TCPIP_end_chat(void);

#define WM_WINSOCK_SELECT       (WM_USER + 10)
#define WM_WINSOCK_START_GAME   (WM_USER + 11)
#define WM_WINSOCK_SELECT_GAME  (WM_USER + 12)
#define WM_WINSOCK_CHAT_JOIN    (WM_USER + 13)
#define WM_WINSOCK_CHAT_TEXT    (WM_USER + 14)
#define WM_WINSOCK_CHAT_EXIT    (WM_USER + 15)

#endif /* __NET32_H__ */
#endif /* MAME_NET */
