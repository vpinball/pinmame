#ifndef FOXPRO_H

#define FOXPRO_H

#include <windows.h>
#include <shlwapi.h>	//Needed by IsDir() - PathIsDirectory()
#include "utils.h"

void CurDir(LPSTR szResult);
BOOL IsDir(LPCSTR szPath);
BOOL File(LPCSTR szFile);
BOOL Empty(LPCSTR szStr);
void Left(LPSTR szReturnStr, LPCSTR szString, int nLen);
UINT Len(LPCSTR szStr);
UINT At(LPCSTR szSearch, LPCSTR szString, int nOccurrence);
UINT Rat(LPCSTR szSearch, LPCSTR szString, int nOccurrence);
void AddBs(LPSTR szString);
void Right(LPSTR szReturnStr, LPCSTR szString, int nLen);
UINT Display(LPCSTR szMessage, LPCSTR szType, LPCSTR szCaption);
void ReadINI(LPSTR szReturnString,LPCSTR szSection, LPCSTR szKey, LPCSTR szIniFile);
BOOL WriteINI(LPCSTR szSection, LPCSTR szKey, LPCSTR szValue, LPCSTR szIniFile);
BOOL DeleteAFile(LPCSTR szFileName);
void Substr(LPSTR szReturnStr, LPCSTR szString, int nStartPos, int nLen);
void AddPathIfMissing(LPSTR szString);
BOOL HasPath(LPCSTR szString);
void Strtran(LPSTR szReturnStr, LPCSTR szString, LPCSTR szMatch, LPCSTR szReplace, INT nStartOccurrence, INT nNumberOfOccurrences, UINT nFlags);
UINT Occurs(LPCSTR szMatch, LPCSTR szSearchString);

#endif