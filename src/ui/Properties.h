/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef PROPERTIES_H
#define PROPERTIES_H

/* Get title string to display in the top of the property page,
 * Called also in Audit32.c
 */
char*   GameInfoTitle(UINT nIndex);

/* Called in win32ui.c to create the property page */
void    InitPropertyPage(HINSTANCE hInst, HWND hwnd, int game_num);

#define PROPERTIES_PAGE 0
#define AUDIT_PAGE      1   

void    InitPropertyPageToPage(HINSTANCE hInst, HWND hwnd, int game_num, int start_page);
void    InitDefaultPropertyPage(HINSTANCE hInst, HWND hWnd);

/* Get Help ID array for WM_HELP and WM_CONTEXTMENU */
DWORD   GetHelpIDs(void);

/* Check for ROMs and Samples called in OnIdle in win32ui.c */
BOOL    FindRomSet(int game);
BOOL    FindSampleSet(int game);

/* Check the driver to see if it uses samples */
BOOL    GameUsesSamples(int game);

/* Get Game status text string */
char*   GameInfoStatus(UINT game);

/* Get Game history information */
char*   GameHistory(int game_index);

#endif
