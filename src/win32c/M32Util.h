/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __MAME32UTIL_H__
#define __MAME32UTIL_H__

#define RECT_WIDTH(r)   (r.right  - r.left)
#define RECT_HEIGHT(r)  (r.bottom - r.top)

extern BOOL bErrorMsgBox;

extern void __cdecl ErrorMsg(const char* fmt, ...);

extern void FontDecodeString(char *string, LOGFONT *f);
extern void FontEncodeString(LOGFONT *f, char *string);

extern UINT GetDepth(HWND hWnd);

extern BOOL OnNT(void);

/* Check for old version of comctl32.dll */
extern BOOL GetDllVersion(void);

extern char* MyStrStrI(const char* pFirst, const char* pSrch);

#endif
