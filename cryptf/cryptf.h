#ifndef CRYPTF_H
#define CRYPTF_H

//Includes for both UI & Non UI
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

#include "resource.h"

#include "getdir.h"
#include "foxpro.h"
#include "utils.h"
#include "crypto.h"

//Return codes for command line functionality
#define FAILURE		1
#define SUCCESS		0

//Prototypes
BOOL GetFiles(HWND hWnd);
BOOL BrowseForDir(HWND hWnd, UINT ctrlid);
void ProcessAllFiles(HWND hWnd, char cType);
UINT ProcessFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, char cType, LPCSTR szFileName, LPCSTR szTempFile);
BOOL EncryptFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, LPCSTR szFileName1, LPCSTR szTempFile1);
BOOL DecryptFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, LPCSTR szFileName1, LPCSTR szTempFile1);
BOOL CopyAFile (LPCSTR szSource, LPCSTR szDestination);
BOOL RunFromCommandLine(LPCSTR szCmdLine);
void GetHelpMsg(LPSTR szString);
void GetTheFileTimes(FILETIME *ftINI, FILETIME *ftFiles);

#endif