/**************************************************
 * C++ Equivalent of the Foxpro Function GETDIR() *
 *												  *
 * by Steve Ellenoff (sellenoff@hotmail.com)	  *
 * 05/26/2002									  *
 **************************************************

Displays the Select Directory dialog box from which you can choose a directory.

GETDIR(cSelectedDir [cDirectory [, cText [, cCaption [, nFlags [, lRootOnly]]]]])

Returns: FALSE if no directory was selected, ie, cancel button was clicked.

Parameters:
cSelectedDir - Buffer to hold selected directory
cDirectory   - (Optional) Specifies the directory that is initially displayed in the dialog box.
cText        - (Optional) Specifies the text for the directory list in the dialog box. 
cCaption     - (Optional) Specifies the caption to display in the dialog title bar. The Windows default is "Browse for Folder". 
nFlags       - (Optional) Specify the options for the dialog box. 

	(from win32api)
	BIF_BROWSEFORCOMPUTER	Only returns computers. If the user selects anything other than a computer, the OK button is grayed.
	BIF_BROWSEFORPRINTER	Only returns printers. If the user selects anything other than a printer, the OK button is grayed.
	BIF_DONTGOBELOWDOMAIN	Does not include network folders below the domain level in the tree view control.
	BIF_RETURNFSANCESTORS	Only returns file system ancestors. If the user selects anything other than a file system ancestor, the OK button is grayed.
	BIF_RETURNONLYFSDIRS	Only returns file system directories. If the user selects folders that are not part of the file system, the OK button is grayed.
	BIF_STATUSTEXT			Includes a status area in the dialog box. The callback function can set the status text by sending messages to the dialog box.

lRootOnly    - (Optional) Specifies that only cDirectory and its subfolders display. This parameter prevents navigation above the root folder.

NOTES: 
  All optional parameters should send EMPTY STRINGS, and FALSE for BOOL parameters
  BIF_STATUSTEXT not supported at this time. The flag works, but there's no mechanism to set the text yet.
  NO SUPPORT FOR New Shell32.dll Dialog style from flags - BIF_USENEWUI AND BIF_NEWDIALOGSTYLE
*/

#include "getdir.h"

//////////////////////////////////////////////////////////////////////
// GetDir - See description above
//////////////////////////////////////////////////////////////////////
BOOL GetDir(LPSTR szSelDir, LPCSTR szInitDir, LPCSTR szText, LPCSTR szCaption, UINT nFlags, BOOL bRootOnly)
{
	/*copy string into our static string*/
	wsprintf(szCap,"%s",szCaption);
	return BrowseForDirectory(GetActiveWindow(), szInitDir, szSelDir, szText, nFlags, bRootOnly);
}

//////////////////////////////////////////////////////////////////////
// GetPIDLFromPath - Returns a PIDL structure from a File Path String
//////////////////////////////////////////////////////////////////////
LPITEMIDLIST GetPIDLFromPath(LPCSTR lpszDefault) 
{
		IShellFolder* pDesktop;
		LPITEMIDLIST pidlRoot = NULL;

		/* gets the ITEMIDLIST for the desktop folder. */
		if (SHGetDesktopFolder(&pDesktop) == NOERROR) {
			  WCHAR wszPath[_MAX_PATH];
			  ULONG cchEaten;
			  ULONG dwAttr;

			  /* converts the root path into unicode. */
			  MultiByteToWideChar(CP_OEMCP, 0,
				  lpszDefault, -1, wszPath, _MAX_PATH);

			  /* translates the root path into ITEMIDLIST. */
			  if (pDesktop->ParseDisplayName(
					  NULL, NULL, wszPath,
					  &cchEaten, &pidlRoot, &dwAttr) != NOERROR)
				  pidlRoot = NULL;
			  pDesktop->Release();
		}
		return pidlRoot;
}

////////////////////////////////////////////////////////////////////////////////
// GetPIDLFromPath - Browse Call Back Function for setting Browse Dialog Options
////////////////////////////////////////////////////////////////////////////////
int CALLBACK BrowseCallbackProc(HWND hwnd,
        UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg) {
        case BFFM_INITIALIZED:
			/* change the window caption if specified */
			if(strlen(szCap)>0)
				SetWindowText(hwnd,szCap);
            /* change the selected folder. */
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
            break;
        case BFFM_SELCHANGED:
            break;
        default:
            break;
    }
    return(0);
}

///////////////////////////////////////////////////////////////////////////////////////
// BrowseForDirectory - Creates and displays a Browse for Folder dialog with options set
////////////////////////////////////////////////////////////////////////////////////////
BOOL BrowseForDirectory(HWND hWnd, LPCSTR lpszDefault, char* pResult, LPCSTR szText, UINT nFlag, BOOL bRoot) 
{
	BOOL        bResult = FALSE;
	LPMALLOC	piMalloc;
	BROWSEINFO  Info;
	ITEMIDLIST* pItemIDList = NULL;
	LPITEMIDLIST pidlRoot = NULL;
	char        buf[MAX_PATH];

	if (!SUCCEEDED(SHGetMalloc(&piMalloc)))
		return FALSE;

	/*Set the Default Directory to be the ROOT as well if selected*/
	if(bRoot)
		pidlRoot = GetPIDLFromPath(lpszDefault);

	Info.hwndOwner      = hWnd;
	Info.pidlRoot       = pidlRoot;
	Info.pszDisplayName = (LPSTR)buf;
	/*set text of dialog if specified*/
	if(strlen(szText)>0)
		Info.lpszTitle  = szText;
	else
		Info.lpszTitle      = (LPCSTR)"Directory name:";
	Info.ulFlags        = nFlag;
	Info.lpfn           = (BFFCALLBACK)BrowseCallbackProc;
	Info.lParam         = (LPARAM)lpszDefault;

	pItemIDList = SHBrowseForFolder(&Info);

	if (pItemIDList != NULL)
	{
		if (SHGetPathFromIDList(pItemIDList, buf) == TRUE)
		{
			strncpy(pResult, buf, MAX_PATH);
			bResult = TRUE;
		}
		piMalloc->Free(pItemIDList);
	}
	else
	{
		bResult = FALSE;
	}
	piMalloc->Release();
	return bResult;
}

