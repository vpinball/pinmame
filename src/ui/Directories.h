/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef DIRECTORIES_H
#define DIRECTORIES_H

/* Dialog return codes */
#define DIRDLG_ROMS         0x0010
#define DIRDLG_SAMPLES      0x0020
#define DIRDLG_CFG          0x0040
#define DIRDLG_HI           0x0080
#define DIRDLG_IMG          0x0100
#define DIRDLG_INP          0x0200
#ifdef PINMAME
#define DIRDLG_WAVE         0x0400
#endif /* PINMAME */
INT_PTR CALLBACK DirectoriesDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

#endif
