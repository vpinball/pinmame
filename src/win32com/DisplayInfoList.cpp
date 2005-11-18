#include "StdAfx.h"
#include ".\displayinfolist.h"
#include <ddraw.h>

static BOOL WINAPI DDEnumCallbackEx(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR  hm)
{
    // Context is a pointer to a display list
    CDisplayInfoList* displayList = (CDisplayInfoList*)lpContext;

	// Add the info to the list
	displayList->AddDisplay(lpGUID, lpDriverDescription, lpDriverName);

	// Continue enumeration (to find other cards)
    return TRUE;
}

static BOOL WINAPI DDEnumCallback(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
    return DDEnumCallbackEx(lpGUID, lpDriverDescription, lpDriverName, lpContext, NULL);
}

/************************************************
 * Constructors
 ***********************************************/
CDisplayInfoList::CDisplayInfoList(void)
{
	// Enumerate the list
	Enumerate();	
}

/************************************************
 * Destructors
 ***********************************************/
CDisplayInfoList::~CDisplayInfoList(void)
{
	// Cleanup displays
	//for(size_t i=0; i < Count(); i++)
	//{
	//	Item(i)->Cleanup();
	//}

	// Release displays
	mDisplays.clear();
}

/************************************************
 * Methods
 ***********************************************/
void CDisplayInfoList::AddDisplay(GUID FAR *lpGuid, LPSTR lpDriverDesc, LPSTR lpDriverName)
{
	CDisplayInfo di = CDisplayInfo(lpGuid, lpDriverDesc, lpDriverName);
    mDisplays.push_back( di );
}

BOOL CDisplayInfoList::Enumerate()
{
	// Get to DirectDraw
	HINSTANCE hDDraw = LoadLibrary("ddraw.dll");;

	// If ddraw.dll doesn't exist in the search path,
	// then DirectX probably isn't installed, so fail.
	if (!hDDraw) return FALSE;

	// Note that you must know which version of the
	// function to retrieve (see the following text).
	// For this example, we use the ANSI version.
	LPDIRECTDRAWENUMERATEEX lpDDEnumEx = (LPDIRECTDRAWENUMERATEEX) GetProcAddress(hDDraw,"DirectDrawEnumerateExA");

	// Enumeration results placeholder
	HRESULT result = DD_OK;

	// If the function is there, call it to enumerate all display
	// devices attached to the desktop, and any non-display DirectDraw
	// devices.
	if (lpDDEnumEx) 
	{
		result = lpDDEnumEx(DDEnumCallbackEx, this, DDENUM_ATTACHEDSECONDARYDEVICES | DDENUM_DETACHEDSECONDARYDEVICES);
	}
	else
	{
		/*
		* We must be running on an old version of DirectDraw.
		* Therefore MultiMon isn't supported. Fall back on
		* DirectDrawEnumerate to enumerate standard devices on a
		* single-monitor system.
		*/
		result = DirectDrawEnumerate(DDEnumCallback,this);
	}

	// If the library was loaded by calling LoadLibrary(),
	// then you must use FreeLibrary() to let go of it.
	FreeLibrary(hDDraw);

	// Make sure enumeration was a success
	if (result != DD_OK)
	{
		// Place to hold the message
		char buff[MAX_PATH];

		// Copy message into buffer
		_snprintf(buff, MAX_PATH, "Error enumerating displays: %08x\n", (UINT32)result);

		// Show error
		MessageBox(NULL, (LPCTSTR)buff, NULL, MB_OK);

		// Indicate failure
		return FALSE;
	}

	// Indicate Success
	return TRUE;
}

/************************************************
 * Properties
 ***********************************************/
size_t CDisplayInfoList::Count(void) const
{
    return mDisplays.size();
}

CDisplayInfo* CDisplayInfoList::Item(size_t index)
{
    return &mDisplays.at( index );
}