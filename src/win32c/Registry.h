/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef REGISTRY_H
#define REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL  WriteRegistry(char* pszKey, char* pszName, DWORD dwValue);
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault);

#ifdef __cplusplus
}
#endif

#endif // REGISTRY_H