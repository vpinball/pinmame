#include "StdAfx.h"
#include "ControllerSplashWnd.h"

#include "resource.h"

#include <atlwin.h>
#include <time.h>

#define CLOSE_TIMER			1
#define SPLASH_WND_VISIBLE	5000 // ms

class CSplashWnd : public CWindowImpl<CSplashWnd> {
public:
	BEGIN_MSG_MAP(CSplashWnd)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
		MESSAGE_HANDLER(WM_TIMER, OnClick)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

private:
	char*		m_pszCredits;		// use defined credit text, displayed at the bottom of the picture
	UINT		m_uClosedTimer;		// if this timer runs out, the window will be closed
	HBITMAP		m_hBitmap;			// the bitmap we are displaying
	BITMAP		m_Bitmap;			// bitmap info to hBitmap
	HFONT		m_hFont;			// font for the credit line
	COLORREF	m_Color;			// color for the credit line
	int			m_iCreditStartY;	// start of the credit box (Y direction)
	bool		m_fEasterEggScreen; // true if the easter egg screen is displayed

	void DoPaint(HDC hPaintDC=0) {
		HDC hDC = hPaintDC;
		if ( !hPaintDC )
			hDC = GetDC();

		if ( m_hBitmap ) {
			HDC hMemDC = CreateCompatibleDC(hDC);
			HBITMAP hOldBitmap = (HBITMAP) SelectObject(hMemDC, m_hBitmap);
			BitBlt(hDC, 0, 0, m_Bitmap.bmWidth, m_Bitmap.bmHeight, hMemDC, 0, 0, SRCCOPY);
			SelectObject(hMemDC, hOldBitmap);
			DeleteDC(hMemDC);
		}

		if ( m_pszCredits && *m_pszCredits && !m_fEasterEggScreen ) {
			int OldTextColor = SetTextColor(hDC, m_Color);
			int OldBkMode    = SetBkMode(hDC, TRANSPARENT);
			HFONT hOldFont   = (HFONT) SelectObject(hDC, m_hFont);

			RECT Rect;
			GetClientRect(&Rect);
			if ( Rect.bottom > m_iCreditStartY )
				Rect.top = Rect.bottom - m_iCreditStartY;
			Rect.bottom = Rect.top + 45;
			Rect.left  += 5;
			Rect.right -= 5;

			DrawText(hDC, m_pszCredits, -1, &Rect, DT_EDITCONTROL|DT_NOPREFIX|DT_WORDBREAK|DT_CENTER);

			SelectObject(hDC, hOldFont);
			SetBkMode(hDC, OldBkMode);
			SetTextColor(hDC, OldTextColor);
		}

		if ( !hPaintDC )
			ReleaseDC(hDC);
	}

	LRESULT OnCreate(UINT, WPARAM, LPARAM lParam, BOOL&) {
		m_fEasterEggScreen = false;

		m_pszCredits = (char*) ((LPCREATESTRUCT) lParam)->lpCreateParams;

		srand( (unsigned)time(NULL));
		int iSplashScreenNo = int(rand()%4);

		// choose the right color and font style for the user setable text
		int iWeight = FW_NORMAL;
		m_iCreditStartY = 45;

		switch ( iSplashScreenNo ) {
			case 1: // Stein's image
				m_Color = RGB(0,255,0);
				iWeight = FW_BOLD;
				m_iCreditStartY = 48;
				break;
			case 2: // Forchia's image
				m_Color = RGB(0,0,0);
				iWeight = FW_BOLD;
				break;
			default: // The original one (0) and Steve's new one (3)
				m_Color = RGB(255,255,255);
				break;
		}
		m_hBitmap = LoadBitmap(_Module.m_hInst, MAKEINTRESOURCE(IDB_SPLASH)+iSplashScreenNo);
		if ( m_hBitmap ) {
			GetObject(m_hBitmap, sizeof m_Bitmap, &m_Bitmap);

			// resize the window so it fits the bitmap
			RECT Rect = {0, 0, m_Bitmap.bmWidth, m_Bitmap.bmHeight};
			SetWindowPos((HWND) 0, &Rect, SWP_NOMOVE|SWP_NOZORDER);
		}

		HDC hDC = GetDC();
		m_hFont = CreateFont(
			-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
			0, 0, 0, iWeight, 0, 0, 0, ANSI_CHARSET, 0, 0, 0,
			FF_DONTCARE, "Microsoft Sans Serife"
		);
		ReleaseDC(hDC);

		// center all
		CenterWindow();

		m_uClosedTimer = SetTimer(CLOSE_TIMER, SPLASH_WND_VISIBLE);
		return 1;
	}

	LRESULT OnClick(UINT, WPARAM, LPARAM, BOOL&) {
		DestroyWindow();
		return 1;
	}

	LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL&) {
		if ( wParam!='a' )
			DestroyWindow();
		else if ( !m_fEasterEggScreen ) {
			HBITMAP hBitmap = LoadBitmap(_Module.m_hInst, MAKEINTRESOURCE(IDB_EASTEREGG));
			if ( hBitmap ) {
				m_fEasterEggScreen = true;

				m_hBitmap = hBitmap;

				// get the bitmap
				GetObject(m_hBitmap, sizeof m_Bitmap, &m_Bitmap);

				// resize the window so it fits the bitmap
				RECT Rect = {0, 0, m_Bitmap.bmWidth, m_Bitmap.bmHeight};
				SetWindowPos((HWND) 0, &Rect, SWP_NOMOVE|SWP_NOZORDER);

				// center all
				CenterWindow();

				// repaint the window
				InvalidateRect(NULL, true);

				if ( m_uClosedTimer ) {
					KillTimer(m_uClosedTimer);
					m_uClosedTimer = 0;
				}
			}
		}

		return 1;
	}

	LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
		RECT Rect;
		if ( !GetUpdateRect(&Rect) )
			return 1;

		PAINTSTRUCT PaintStruct;
		BeginPaint(&PaintStruct);
		DoPaint(PaintStruct.hdc);
		EndPaint(&PaintStruct);
		return 1;
	};

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
		if ( m_uClosedTimer ) {
			KillTimer(m_uClosedTimer);
			m_uClosedTimer = 0;
		}
		DeleteObject(m_hFont);
		return 1;
	}
};

void CreateSplashWnd(void **ppData, char* pszCredits)
{
	if ( !ppData )
		return;

	// create and display dialog on the desktop
	CSplashWnd* pSplashWnd = new CSplashWnd;
	pSplashWnd->Create((HWND) 0, CWindow::rcDefault, NULL, WS_VISIBLE|WS_POPUP, NULL, 0U, pszCredits);
	*ppData = pSplashWnd;

	/* remove this line if you want to run the game at once */
	WaitForSplashWndToClose(ppData);
}

void DestroySplashWnd(void **ppData)
{
	if ( !ppData || !*ppData )
		return;

	// destroy dialog if it exists
	CSplashWnd* pSplashWnd = (CSplashWnd*) *ppData;
	if ( IsWindow(pSplashWnd->m_hWnd) )
		DestroyWindow(pSplashWnd->m_hWnd);
	delete pSplashWnd;

	*ppData = NULL;
}

void WaitForSplashWndToClose(void **ppData)
{
	if ( !ppData || !*ppData )
		return;

	// wait for the splash window until it closes
	CSplashWnd* pSplashWnd = (CSplashWnd*) *ppData;

	MSG msg;
	while ( pSplashWnd->IsWindow() ) {
		GetMessage(&msg,pSplashWnd->m_hWnd,0,0);

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
