/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  status.c

  Deals with maintaining a status bar control on the bottom of a window

***************************************************************************/

#include "status.h"
#include "commctrl.h"
#include "mame32.h"
#include "m32util.h"
#include <resource.h>
#include <stdio.h>
#include <malloc.h>
#include "display.h"
#include "mame.h"
#include "driver.h"
#include "uclock.h"

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static int  leds_old;
static BOOL leds[3];
static HWND hStatus;
static BOOL bErasePauseText;

static HBRUSH hbrLight;
static HBRUSH hbrDark;
static uclock_t prev_update_time;

#define LED_AREA_WIDTH 52

/***************************************************************************
    External functions  
 ***************************************************************************/

void StatusCreate(void)
{
    RECT rect;
    INT  widths[2];
    
    leds_old = 0;

    leds[0] = FALSE;
    leds[1] = FALSE;
    leds[2] = FALSE;
    
    hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE,"",MAME32App.m_hWnd,IDW_STATUS);
    GetClientRect(MAME32App.m_hWnd,&rect);
    
    widths[0] = LED_AREA_WIDTH;
    widths[1] = -1;
    
    SendMessage(hStatus, SB_SETPARTS, 2, (LPARAM)widths);
    SendMessage(hStatus, SB_SETTEXT, 0 | SBT_OWNERDRAW, 0);
    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
    
    hbrLight = CreateSolidBrush(RGB(0xff, 0, 0));
    hbrDark  = CreateSolidBrush(RGB(0x3f, 0, 0));
    
    prev_update_time = 0;

    bErasePauseText = FALSE;
}

int GetStatusHeight(void)
{
    RECT rect;
    
    if (hStatus == NULL)
        return 0;
    
    GetWindowRect(hStatus,&rect);
    
    return rect.bottom - rect.top;
}

void StatusWindowSize(UINT state,int cx,int cy)
{
    if (hStatus == NULL)
        return;
    
    SendMessage(hStatus, WM_SIZE, (WPARAM)state, MAKELPARAM(cx, cy));
}

#define LIGHT_SIZE 10
#define LIGHT_SEPARATION 15

void StatusDrawItem(const DRAWITEMSTRUCT *lpdi)
{
    RECT rect;
    
    rect.top    = 6;
    rect.left   = 6;
    rect.bottom = rect.top  + LIGHT_SIZE;
    rect.right  = rect.left + LIGHT_SIZE;
    FillRect(lpdi->hDC, &rect, hbrDark);
    if (leds[0])
    {
        rect.top++;
        rect.left++;
        rect.bottom--;
        rect.right--;
        FillRect(lpdi->hDC, &rect, hbrLight);
        rect.top--;
        rect.left--;
        rect.bottom++;
        rect.right++;
    }
    
    rect.left += LIGHT_SEPARATION;
    rect.right = rect.left + LIGHT_SIZE;
    FillRect(lpdi->hDC, &rect, hbrDark);
    if (leds[1])
    {
        rect.top++;
        rect.left++;
        rect.bottom--;
        rect.right--;
        FillRect(lpdi->hDC, &rect, hbrLight);
        rect.top--;
        rect.left--;
        rect.bottom++;
        rect.right++;
    }
    
    rect.left += LIGHT_SEPARATION;
    rect.right = rect.left + LIGHT_SIZE;
    FillRect(lpdi->hDC, &rect, hbrDark);
    if (leds[2])
    {
        rect.top++;
        rect.left++;
        rect.bottom--;
        rect.right--;
        FillRect(lpdi->hDC, &rect, hbrLight);
    }
}

void StatusWrite(int leds_status)
{
    RECT rect;
    
    if (hStatus == NULL)
        return;
    
    if (leds_status == leds_old)
        return;

    leds_old = leds_status;
    
    leds[0] = leds_status & 0x1;
    leds[1] = leds_status & 0x2;
    leds[2] = leds_status & 0x4;

    rect.left   = 0;
    rect.right  = LED_AREA_WIDTH - 1;
    rect.top    = 0;
    rect.bottom = 20;

    InvalidateRect(hStatus, &rect, FALSE);
}

void StatusUpdate(void)
{
    if (hStatus == NULL)
        return;

    if (MAME32App.m_bMamePaused || MAME32App.m_bAutoPaused)
    {
        bErasePauseText = TRUE;
        SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"\t\tPaused");
        return;
    }

    if (bErasePauseText == TRUE)
    {
        bErasePauseText = FALSE;
        SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"\t\t");
    }
}

void StatusUpdateFPS(BOOL bShow, int nSpeed, int nFPS, int nMachineFPS, int nFrameskip, int nVecUPS)
{
    char buf[100];
    uclock_t now;
    
    if (hStatus == NULL)
        return;
    
    if (MAME32App.m_bMamePaused || MAME32App.m_bAutoPaused)
    {
        StatusUpdate();
        return;
    }

    if (bShow == FALSE)
    {
        SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
        return;
    }

    now = uclock();
    
    /* update twice a second or so */
    if ((now - prev_update_time) < (UCLOCKS_PER_SEC/2))
        return;
    
    sprintf(buf, "\t\tfskp%2d%4d%%%4d/%d fps", nFrameskip, nSpeed, nFPS, nMachineFPS);
    
    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)buf);
    
    prev_update_time = now;
}

void StatusSetString(char *str)
{
    char buf[100];
    
    if (hStatus == NULL)
        return;
    
    sprintf(buf, "\t\t%s", str);
    
    SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)buf);
}

void StatusDelete(void)
{
    DeleteObject(hbrLight);
    DeleteObject(hbrDark);
    DestroyWindow(hStatus);
}

