/*************************************************
 * About Dialog for File Crypter                 *
 *												 *
 * by Steve Ellenoff (sellenoff@hotmail.com)	 *
 *												 *
 * 05/30/2002									 *
 *************************************************/
//Thanks to Tom Haukap for tips on changing font for hyperlink!

#include "about.h"

extern HINSTANCE g_hInstance;

// Mesage handler for about box.
LRESULT CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hLinkBrush = 0;
	static HFONT  hFont = 0;

	switch (message)
	{
		case WM_INITDIALOG: 
			{
			HDC hDC;
			UINT nHeight;

			//Create a font for later use with hyperlink
			hDC = GetDC(hDlg);
			nHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);	//This line comes from the Win32API help for specifying point size!
			hFont = CreateFont(nHeight, 0, 0, 0, FW_NORMAL, 0, TRUE, 0, ANSI_CHARSET, 0, 0, 0, FF_DONTCARE, "Tahoma");
			ReleaseDC(hDlg, hDC);

			/*Create new brush of color of the background for later use*/
			hLinkBrush = (HBRUSH) GetStockObject(HOLLOW_BRUSH);

			//Set Cursor for Static Email Link to our Hyperlink Cursor
			SetClassLong(GetDlgItem(hDlg,IDC_EMAILLINK),GCL_HCURSOR, (long)LoadCursor(NULL, IDC_HAND));
						
			CenterWindow(hDlg);
			return TRUE;
			break;
			}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
				break;
			}
			if (LOWORD(wParam) == IDC_EMAILLINK)
				ShellExecute(hDlg,"open","mailto:sellenoff@hotmail.com",NULL,NULL,SW_SHOW);
			break;

		case WM_CTLCOLORSTATIC: {
			int nID = (int)GetDlgCtrlID((HWND)lParam);
			if (nID == IDC_EMAILLINK) {
				/*Change the color of the text to blue*/
				SetTextColor((HDC) wParam, RGB(0,0,255));
				SetBkMode((HDC) wParam,TRANSPARENT);
				SelectObject((HDC) wParam, hFont);
				return (LRESULT) hLinkBrush;
				break;
				}
			}
			break;

		case WM_DESTROY:
			DeleteObject(hLinkBrush);
			DeleteObject(hFont);
			break;
	}
	return FALSE;
}