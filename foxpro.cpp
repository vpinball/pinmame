/**************************************************
 * C++ Equivalent of various Foxpro Functions     *
 * (Some functions here are not native to Foxpro) *
 *												  *
 * by Steve Ellenoff (sellenoff@hotmail.com)	  *
 * 05/26/2002									  *
 **************************************************/

#include "foxpro.h"

//////////////////////////////////////////////////////////////////////
// CurDir: Return the current application's directory
//////////////////////////////////////////////////////////////////////
void CurDir(LPSTR szResult)
{
	char szTemp[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,szTemp);
	strcpy(szResult,szTemp);
}

//////////////////////////////////////////////////////////////////////
// IsDir: Returns TRUE if the specified path is valid
//////////////////////////////////////////////////////////////////////
/*NOTE: Must link with shlwapi.lib file*/
BOOL IsDir(LPCSTR szPath)
{
	return PathIsDirectory(szPath);
}

//////////////////////////////////////////////////////////////////////
// File: Returns TRUE if the specified file is valid
//////////////////////////////////////////////////////////////////////
BOOL File(LPCSTR szFile)
{
	char szFile1[MAX_PATH];
	if(Empty(szFile))	return FALSE;

	strcpy(szFile1,szFile);

	//If no directory specified, use current
	AddPathIfMissing(szFile1);
	
	if(GetFileAttributes(szFile1)==0xFFFFFFFF)
		return FALSE;
	else
		//NOTE: Since GetFileAttributes WILL SUCCEED for a valid Directory AS WELL AS FILE, we must make sure it's not a directory!
		return (BOOL)!IsDir(szFile1);
}

//////////////////////////////////////////////////////////////////////
// Empty: Return TRUE if specified string is empty
//////////////////////////////////////////////////////////////////////
BOOL Empty(LPCSTR szStr){
	return Len(szStr)<=0;
}

//////////////////////////////////////////////////////////////////////
// Len: Return the length of a string
//////////////////////////////////////////////////////////////////////
UINT Len(LPCSTR szStr) {
	return (UINT)strlen(szStr);
}

//////////////////////////////////////////////////////////////////////
// Left: Return specified # of characters from the Left of a string
//////////////////////////////////////////////////////////////////////
void Left(LPSTR szReturnStr, LPCSTR szStr, int nLen)
{
	Substr(szReturnStr,szStr,1,nLen);

//Code used before I created substr()
#if 0
	UINT nStrLen = Len(szStr);
	UINT nNum;
	const char *p=szStr;
	char szTemp[2];
	wsprintf(szReturnStr,"");
	if(nLen <= 0) return;
	//Ensure length does not exceed total size
	if(nLen>nStrLen)
		nLen = nStrLen;
	for(nNum = 0; nNum < nLen; nNum++) {
		wsprintf(szTemp,"%c",*p++);
		strcat(szReturnStr,szTemp);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// Right: Return specified # of characters from the Right(End) of a string
//////////////////////////////////////////////////////////////////////////
void Right(LPSTR szReturnStr, LPCSTR szStr, int nLen)
{
	UINT nStrLen = Len(szStr);
	int nStart = nStrLen-nLen+1;
	if(nStart < 0) nStart = 0;
	Substr(szReturnStr,szStr,nStart,nLen);

//Code used before I created substr()
#if 0
	UINT nNum = 0;
	const char *p=szStr;
	char szTemp[2];
	wsprintf(szReturnStr,"");
	if(nLen <= 0) return;
	//Ensure length does not exceed total size
	if(nLen>nStrLen)
		nLen = nStrLen;
	//Get to starting character
	while(nNum++ < nStrLen-nLen)
		*p++;
	//Copy rest
	while(*p) {
		wsprintf(szTemp,"%c",*p++);
		strcat(szReturnStr,szTemp);
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// At: Returns position of search string found in string.
//
//	   Setting nOccurrence allows user to choose if there is more than 1
//	   Occurrence
//////////////////////////////////////////////////////////////////////////
UINT At(LPCSTR szSearch, LPCSTR szString, int nOccurrence)
{
	UINT nStringLen = Len(szString);
	UINT nSearchLen = Len(szSearch);
	UINT i;
	UINT nCharMatch = 0;
	UINT nResult = 0;
	UINT nOccurrenceFound = 0;
	const char *p = szString;
	const char *q = szSearch;
	
	//Check for valid input
	if(nOccurrence <= 0) nOccurrence = 1;
	if(nStringLen <= 0) return 0;
	if(nSearchLen <= 0) return 0;

	for(i = 0; i < nStringLen; i++, *p++) {
		//Do characters match?
		if(*p==*q++) {
			//Hold position of 1st matching character
			if(nCharMatch == 0)
				nResult = i+1;
			nCharMatch++;
			//Did we find a match on the entire string?
			if(nCharMatch == nSearchLen) {
				nOccurrenceFound++;
				//Does it match the # occurence we want?
				if((UINT)nOccurrence == nOccurrenceFound)
					break;
				else {
				q = szSearch;
				nCharMatch = nResult = 0;
				}
			}
		}
		else {
			q = szSearch;
			nCharMatch = nResult = 0;
		}
	}
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// Rat: Returns position of search string found in string, starting from
//		the end of the string.. ie first occurrence found from right to left.
//
//	   Setting nOccurrence allows user to choose if there is more than 1
//	   Occurrence
//////////////////////////////////////////////////////////////////////////
UINT Rat(LPCSTR szSearch, LPCSTR szString, int nOccurrence)
{
	UINT nStringLen = Len(szString);
	UINT nSearchLen = Len(szSearch);
	UINT i;
	UINT nCharMatch = 0;
	UINT nResult = 0;
	UINT nOccurrenceFound = 0;
	const char *p = szString;
	const char *q = szSearch;
	
	//Check for valid input
	if(nOccurrence <= 0) nOccurrence = 1;
	if(nStringLen <= 0) return 0;
	if(nSearchLen <= 0) return 0;

	//Start p at the end of the string
	while(*p) *p++;
	*p--;	//Start at last character

	for(i = nStringLen; i > 0; i--, *p--) {
		//Do characters match?
		if(*p==*q++) {
			//Hold position of 1st matching character
			if(nCharMatch == 0)
				nResult = i;
			nCharMatch++;
			//Did we find a match on the entire string?
			if(nCharMatch == nSearchLen) {
				nOccurrenceFound++;
				//Does it match the # occurence we want?
				if((UINT)nOccurrence == nOccurrenceFound)
					break;
				else {
				q = szSearch;
				nCharMatch = nResult = 0;
				}
			}
		}
		else {
			q = szSearch;
			nCharMatch = nResult = 0;
		}
	}
	return nResult;
}

//////////////////////////////////////////////////////////////////////////
// AddBs: Adds backslash to string, but only if it doesn't exist already
//////////////////////////////////////////////////////////////////////////
void AddBs(LPSTR szString)
{
	char szTemp[MAX_PATH];
	Right(szTemp,szString,1);
	if(strcmp(szTemp,"\\")==0)
		return;
	else
		strcat(szString,"\\");
}

//////////////////////////////////////////////////////////////////////////
// Display: Common MessageBox functions
// NOTE: Not a native Foxpro command, but created by me for easier programming
//////////////////////////////////////////////////////////////////////////
UINT Display(LPCSTR szMessage, LPCSTR szType, LPCSTR szCaption)
{
	UINT nType=0;
	if(strcmp(szType,"NOTICE")==0)
		nType = 64;
	if(strcmp(szType,"NOTE")==0)
		nType = 64;
	if(strcmp(szType,"WARNING")==0 || strcmp(szType,"OK")==0)
		nType = 48;
	if(strcmp(szType,"STOP")==0 || strcmp(szType,"ERROR")==0)
		nType = 16;
	if(strcmp(szType,"YESNO")==0)
		nType = 36;		//YES is defaul selection
	if(strcmp(szType,"NOYES")==0)
		nType = 36+256;	//NO is defaul selection
	return MessageBox(NULL,szMessage,szCaption,nType);
}

//////////////////////////////////////////////////////////////////////////
// ReadINI: Read a value in from an INI file
// NOTE: Not a native Foxpro command, but created by me for easier programming
//////////////////////////////////////////////////////////////////////////
void ReadINI(LPSTR szReturnString,LPCSTR szSection, LPCSTR szKey, LPCSTR szIniFile) {
	char szIniFile1[MAX_PATH];
	wsprintf(szReturnString,"");
	//Error checking
	if(Empty(szSection)) return;
	if(Empty(szKey)) return;
	if(Empty(szIniFile)) return;

	strcpy(szIniFile1,szIniFile);
	//If no directory specified, use current
	AddPathIfMissing(szIniFile1);
	//Do it
	GetPrivateProfileString(szSection,szKey,"",szReturnString,MAX_PATH,szIniFile1);
}


//////////////////////////////////////////////////////////////////////////
// WriteINI: Write a value to an INI file
// NOTE: Not a native Foxpro command, but created by me for easier programming
//////////////////////////////////////////////////////////////////////////
BOOL WriteINI(LPCSTR szSection, LPCSTR szKey, LPCSTR szValue, LPCSTR szIniFile) {
	char szIniFile1[MAX_PATH];
	//Error checking
	if(Empty(szSection)) return FALSE;
	if(Empty(szKey)) return FALSE;
	if(Empty(szIniFile)) return FALSE;

	strcpy(szIniFile1,szIniFile);
	//If no directory specified, use current
	AddPathIfMissing(szIniFile1);
	//Do It
	return WritePrivateProfileString(szSection,szKey,szValue,szIniFile1);
}

////////////////////////////////////////////////////////////////////////////////
// DeleteAFile: Attempts to Delete a file (with retry) and verifies if it worked
// NOTE: Not a native Foxpro command, but vastly better than Foxpro's ERASE command
////////////////////////////////////////////////////////////////////////////////
BOOL DeleteAFile(LPCSTR szFileName) {
	if(File(szFileName)) {
		int iTries = 0;
		int iMaxTry = 30;
		while(iTries < iMaxTry) {
			if(!DeleteFile(szFileName)) {
				iTries++;
				Sleep(1000);
			}
			else
				break;
		}
	}
	return !File(szFileName);
}

////////////////////////////////////////////////////////////////////////////////
// Substr: Returns a portion of the original string, from the start position for 
//		   the specified # of characters. If nLen < 0, returns the rest
//		   of the string
////////////////////////////////////////////////////////////////////////////////
void Substr(LPSTR szReturnStr, LPCSTR szString, int nStartPos, int nLen)
{
	UINT nStrLen = Len(szString);
	UINT nNum, nDone;
	const char *p=szString;
	char szTemp[2];
	wsprintf(szReturnStr,"");
	//Startpos must be positive
	if(nStartPos < 0) nStartPos = 0;
	//If length specified as negative, returns the entire string
	if(nLen < 0)	nLen = nStrLen-nStartPos+1;
	//Ensure length does not exceed total size
	if(nLen>(int)nStrLen)
		nLen = nStrLen;
	nDone = 0;
	for(nNum = 1; nNum <= nStrLen; nNum++) {
		if(nNum >= (UINT)nStartPos) {
			if( (++nDone) > (UINT)nLen )
				break;
			wsprintf(szTemp,"%c",*p);
			strcat(szReturnStr,szTemp);
		}
		*p++;
	}
}

////////////////////////////////////////////////////////////////////////////////
// AddPathIfMissing: If string has no directory, add current directory to it!
// NOTE: Not a native Foxpro command, but created by me for easier programming
////////////////////////////////////////////////////////////////////////////////
void AddPathIfMissing(LPSTR szString)
{
	if(!HasPath(szString)) {
		char szTemp[MAX_PATH];
		CurDir(szTemp);
		AddBs(szTemp);
		PrependToString(szString,szTemp);
	}
}

////////////////////////////////////////////////////////////////////////////////
// HasPath: Returns True if a string has a directory specified (or at least 1 '\' character)
// NOTE: Not a native Foxpro command, but created by me for easier programming
////////////////////////////////////////////////////////////////////////////////
BOOL HasPath(LPCSTR szString)
{
	//Assumes a directory has at least 1 '\' character
	return At("\\",szString,1) > 0;
}

////////////////////////////////////////////////////////////////////////////////
// Strtran: Searches a string for occurrences of a second string
//          and then replaces each occurrence with a third string
//
// Optionally you can specify which occurrence to start replacing, and how many after it
// NOTE: nFlags Not supported at this time
////////////////////////////////////////////////////////////////////////////////
void Strtran(LPSTR szReturnStr, LPCSTR szString, LPCSTR szMatch, LPCSTR szReplace, INT nStartOccurrence, INT nNumberOfOccurrences, UINT nFlags)
{
	BOOL bRemoveChars = FALSE;
	UINT nStrLen = Len(szString);
	UINT nLenMatch = Len(szMatch);
	UINT nDone, nPrevPos, nPos;
	const char *p=szString;
	char szTempStr[MAX_PATH];
	wsprintf(szReturnStr,"");
	nDone = 0;

	//Check for empty match string!
	if(Empty(szMatch)) {
		strcpy(szReturnStr, szString);
		return;
	}

	//Empty Replace string means, we'll REMOVE characters
	if(Empty(szReplace))
		bRemoveChars = TRUE;

	//If not specified nStartOccurrence = 1
	if(nStartOccurrence <= 0) nStartOccurrence = 1;
	//If # of Occurrences not specified, make it equal length of original string
	if(nNumberOfOccurrences <= 0) nNumberOfOccurrences = nStrLen;

	//Find first match
	nPrevPos = 1;
	nPos = At(szMatch,szString,nStartOccurrence++);
	while(nPos > 0) {
		//Add all of previous string!
		Substr(szTempStr,szString,nPrevPos,nPos-nPrevPos);
		strcat(szReturnStr,szTempStr);
		//Add replacement string (if specified)
		if(!bRemoveChars)
			strcat(szReturnStr,szReplace);
		nDone++;
		nPrevPos = nPos+nLenMatch;
		//Any more occurrences to process?
		if(nDone == (UINT)nNumberOfOccurrences)
			break;
		//Get next match
		nPos = At(szMatch,szString,nStartOccurrence++);
	}
	//Fill in remainder of string
	Substr(szTempStr,szString,nPrevPos,-1);
	strcat(szReturnStr,szTempStr);
}

////////////////////////////////////////////////////////////////////////////////
// Occurs: Returns the number of times a string occurs within another string
////////////////////////////////////////////////////////////////////////////////
UINT Occurs(LPCSTR szMatch, LPCSTR szSearchString)
{
	UINT nFound = 0;
	UINT nPos;
	do {
		nPos = At(szMatch,szSearchString,nFound+1);
		if(nPos > 0) nFound++;
	}
	while(nPos > 0);
	return nFound;
}