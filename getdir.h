#ifndef GETDIR_H

#define GETDIR_H

#include <shlobj.h>

/*Static Vars*/
static char szCap[MAX_PATH];		//Used to change caption of Browse Dialog

/* Prototypes*/
BOOL BrowseForDirectory(HWND hWnd, LPCSTR lpszDefault, char* pResult, LPCSTR szText, UINT nFlag, BOOL bRoot);
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
LPITEMIDLIST GetPIDLFromPath(LPCSTR lpszDefault);

BOOL GetDir(LPSTR szSelDir, LPCSTR szInitDir, LPCSTR szText, LPCSTR szCaption, UINT nFlags, BOOL bRootOnly);

#endif