#include "StdAfx.h"
#include "VPinMAMESplashWnd.h"

#include "resource.h"

#include <atlwin.h>

#define CLOSE_TIMER			1
#define SPLASH_WND_VISIBLE	5000 // ms

class CSplashWnd : public CWindowImpl<CSplashWnd> {
public:
	BEGIN_MSG_MAP(CSplashWnd)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
		MESSAGE_HANDLER(WM_TIMER, OnClick)
		MESSAGE_HANDLER(WM_CHAR, OnClick)
		MESSAGE_HANDLER(WM_PAINT, OnPaint) 
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

private:
	char*	m_pszCredits;		// use defined credit text, displayed at the bottom of the picture
	UINT	m_uClosedTimer;		// if this timer runs out, the window will be closed
	HBITMAP	m_hBitmap;			// the bitmap we are displaying
	BITMAP	m_Bitmap;			// bitmap info to hBitmap
	HFONT	m_hFont;			// font for the credit line

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

		if ( m_pszCredits && *m_pszCredits ) {
			int OldTextColor = SetTextColor(hDC, RGB(255,255,255));
			int OldBkMode    = SetBkMode(hDC, TRANSPARENT);
			HFONT hOldFont   = (HFONT) SelectObject(hDC, m_hFont);

			RECT Rect;
			GetClientRect(&Rect);
			if ( Rect.bottom > 45 )
				Rect.top = Rect.bottom - 45;
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
		m_pszCredits = (char*) ((LPCREATESTRUCT) lParam)->lpCreateParams;

		m_hBitmap = LoadBitmap(_Module.m_hInst, MAKEINTRESOURCE(IDB_SPLASH));
		if ( m_hBitmap ) {
			GetObject(m_hBitmap, sizeof m_Bitmap, &m_Bitmap);

			// resize the window so it fits the bitmap
			RECT Rect = {0, 0, m_Bitmap.bmWidth, m_Bitmap.bmHeight};
			SetWindowPos((HWND) 0, &Rect, SWP_NOMOVE|SWP_NOZORDER);
		}

		HDC hDC = GetDC();
		m_hFont = CreateFont(
			-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
			0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 
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
		KillTimer(m_uClosedTimer);
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
}

void DestroySplashWnd(void **ppData)
{
	if ( !ppData )
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
	if ( !ppData )
		return;

	// wait for the splash window until it closes
	CSplashWnd* pSplashWnd = (CSplashWnd*) *ppData;

	MSG msg;
	do {
		GetMessage(&msg,0,0,0);

		TranslateMessage(&msg); 
		DispatchMessage(&msg);
	} while ( pSplashWnd->IsWindow() );
}