/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef TREEVIEW_H
#define TREEVIEW_H

#include "Bitmask.h"
#include "Win32ui.h"

/* TreeView structures */
enum FolderIds
{
	FOLDER_NONE = 0,
	FOLDER_ALLGAMES,
	FOLDER_AVAILABLE,
	FOLDER_UNAVAILABLE,
	FOLDER_NEOGEO,
	FOLDER_MANUFACTURER,
	FOLDER_YEAR,
	FOLDER_WORKING,
	FOLDER_NONWORKING,
	FOLDER_CUSTOM,
	FOLDER_PLAYED,
	FOLDER_FAVORITE,
	FOLDER_ORIGINAL,
	FOLDER_CLONES,
	FOLDER_RASTER,
	FOLDER_VECTOR,
	FOLDER_TRACKBALL,
	FOLDER_STEREO,
	FOLDER_END
};

typedef enum
{
	IS_ROOT = 1,
	IS_FOLDER
} FOLDERTYPE;

typedef enum
{
	F_NEOGEO        = 0x00000001,
	F_CLONES        = 0x00000002,
	F_NONWORKING    = 0x00000004,
	F_UNAVAILABLE   = 0x00000008,
	F_VECTOR        = 0x00000010,
	F_RASTER        = 0x00000020,
	F_ORIGINALS     = 0x00000040,
	F_WORKING       = 0x00000080,
	F_AVAILABLE     = 0x00000100,
	F_NUM_FILTERS   = 9,
	F_MASK          = 0x00000FFF,
	F_CUSTOM        = 0x01000000
} FOLDERFLAG;

typedef struct
{
	LPSTR       m_lpTitle;          /* String contains the folder name */
	FOLDERTYPE  m_nFolderType;      /* Contains enum FolderTypes */
	UINT        m_nFolderId;        /* Index / Folder ID number */
	UINT        m_nParent;          /* Parent folder ID */
	UINT        m_nIconId;          /* Icon to use with this folder */
	DWORD       m_dwFlags;          /* Misc flags */
	LPBITS      m_lpGameBits;       /* Game bits, represent game indices */
} TREEFOLDER, *LPTREEFOLDER;

extern BOOL InitFolders(UINT nGames);
extern void FreeFolders(void);

extern void SetCurrentFolder(LPTREEFOLDER lpFolder);
extern UINT GetCurrentFolderID(void);

extern LPTREEFOLDER GetCurrentFolder(void);
extern LPTREEFOLDER GetFolder(UINT nFolder);
extern LPTREEFOLDER GetFolderByID(UINT nID);

extern void AddGame(LPTREEFOLDER lpFolder, UINT nGame);
extern void RemoveGame(LPTREEFOLDER lpFolder, UINT nGame);
extern int  FindGame(LPTREEFOLDER lpFolder, int nGame);

extern void InitTree(HWND hWnd, UINT nGames);
extern void InitGames(UINT nGames);

extern void Tree_Initialize(HWND hWnd);
extern BOOL GameFiltered(int nGame, DWORD dwFlags);

INT_PTR CALLBACK ResetDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FilterDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK StartupDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

extern void SetTreeIconSize(HWND hWnd, BOOL bLarge);
extern BOOL GetTreeIconSize(void);

#endif /* TREEVIEW_H */
