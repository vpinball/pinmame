/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  Registry.c

  registry dependent stuff

 ***************************************************************************/
#include "Windows.h"
#include "Registry.h"

/*Writes a DWORD to the Registry! Opens the Registry Key Specified & Closes When Done*/
BOOL WriteRegistry(char* pszKey, char* pszName, DWORD dwValue) {
	HKEY hKey;
	DWORD dwDisposition;

	if ( !pszKey || !*pszKey )
		return FALSE;

   	if ( RegCreateKeyEx(HKEY_CURRENT_USER, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition)!=ERROR_SUCCESS )
		return FALSE;

    if ( RegSetValueEx(hKey, pszName, 0, REG_DWORD, (LPBYTE) &dwValue, sizeof dwValue)!=ERROR_SUCCESS ) {
		RegCloseKey(hKey);
		return FALSE;
	}
	RegCloseKey(hKey);
	
	return TRUE;
}

/*Reads a DWORD to the Registry! Opens the Registry Key Specified & Closes When Done*/
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault) {
	DWORD dwValue = dwDefault; 
	HKEY hKey;
	DWORD dwType;
	DWORD dwSize;

	if ( !pszKey || !*pszKey )
		return FALSE;

   	if ( RegOpenKeyEx(HKEY_CURRENT_USER, pszKey, 0, KEY_QUERY_VALUE, &hKey)!=ERROR_SUCCESS )
		return dwValue;

	dwType = REG_DWORD;
	dwSize = sizeof dwValue;
    if ( RegQueryValueEx(hKey, pszName, 0, &dwType, (LPBYTE) &dwValue, &dwSize)!=ERROR_SUCCESS ) 
		dwValue = dwDefault;

	RegCloseKey(hKey);
	return dwValue;
}
