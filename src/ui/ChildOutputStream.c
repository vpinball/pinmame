/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "ChildOutputStream.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI ChildOutputReadThreadProc(LPVOID lpParameter);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

/***************************************************************************
    External functions  
 ***************************************************************************/

void ChildOutputStream_Init(struct t_ChildOutputStream* pThis)
{
	memset(pThis, 0, sizeof(struct t_ChildOutputStream));
}

void ChildOutputStream_Exit(struct t_ChildOutputStream* pThis)
{
	ChildOutputStream_WaitForThreadExit(pThis);

	if (pThis->m_strBuffer)
	{
		HeapFree(GetProcessHeap(), 0, pThis->m_strBuffer);
		pThis->m_strBuffer = NULL;
	}
	pThis->m_nBufferLen = 0;

	if (pThis->m_hRead)
	{
		CloseHandle(pThis->m_hRead);
		pThis->m_hRead = NULL;
	}
}

void ChildOutputStream_WaitForThreadExit(struct t_ChildOutputStream* pThis)
{
	BOOL bResult;

	if (pThis->m_hReadThread)
	{
		WaitForSingleObject(pThis->m_hReadThread, INFINITE); /* TODO: wait for finite time. */
		bResult = CloseHandle(pThis->m_hReadThread);
		pThis->m_hReadThread = NULL;
	}
}

BOOL ChildOutputStream_Create(struct t_ChildOutputStream* pThis, DWORD dwMaxBufLen)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead;
	HANDLE hReadDuplicate;
	HANDLE hWrite;
	DWORD  dwThreadID;
	BOOL   bResult;
	
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle       = TRUE;
	
	bResult = CreatePipe(&hRead, &hWrite, &sa, 0);
	if (!bResult)
		return bResult;

	/* Duplicate only to make the read handle uninheritable. */
	bResult = DuplicateHandle(GetCurrentProcess(),
							  hRead,
							  GetCurrentProcess(),
							  &hReadDuplicate,
							  0,
							  FALSE,
							  DUPLICATE_SAME_ACCESS);
	if (!bResult)
		return bResult;
	
	bResult = CloseHandle(hRead);
	if (!bResult)
		return bResult;

	pThis->m_hRead  = hReadDuplicate;
	pThis->m_hWrite = hWrite;

	pThis->m_nMaxBufferLen = dwMaxBufLen;

	dwThreadID = 0;
	pThis->m_hReadThread = CreateThread(NULL, 0, ChildOutputReadThreadProc, (VOID*)pThis, 0, &dwThreadID);
	if (pThis->m_hReadThread == NULL)
		return FALSE;

	return TRUE;
}

void ChildOutputStream_Dump(struct t_ChildOutputStream* pThis, FILE* pFile)
{
	if (pThis->m_nBufferLen != 0
	&&  pThis->m_strBuffer  != NULL)
	{
/*		pThis->m_strBuffer[pThis->m_nBufferLen - 1] = '\0'; */
		fwrite(pThis->m_strBuffer, 1, pThis->m_nBufferLen, pFile);
		fflush(pFile);
	}
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static DWORD WINAPI ChildOutputReadThreadProc(LPVOID lpParameter)
{
	struct t_ChildOutputStream* pThis = (struct t_ChildOutputStream*)lpParameter;
	CHAR pBuffer[10];
	
	while (TRUE)
	{
		BOOL  bResult;
		DWORD nBytesRead = 0;

		bResult = ReadFile(pThis->m_hRead, pBuffer, sizeof(pBuffer), &nBytesRead, NULL);
		if (!bResult || !nBytesRead)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				/* child closed pipe */
				break;
			}
			else
			{
				/* Read Error. */
				OutputDebugString("ReadFile() error.\n");
				break;
			}
		}
		
		if (0 < nBytesRead && nBytesRead <= sizeof(pBuffer))
		{
			if (pThis->m_strBuffer == NULL)
			{
				pThis->m_strBuffer = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pThis->m_nMaxBufferLen);
				if (pThis->m_strBuffer == NULL)
				{
					/* Memory allocation error. */
					pThis->m_nBufferLen = 0;
					OutputDebugString("HeapAlloc() error.\n");
					return 0;
				}
			}

			if ((pThis->m_nBufferLen + nBytesRead) < pThis->m_nMaxBufferLen)
			{
				CopyMemory(&pThis->m_strBuffer[pThis->m_nBufferLen], pBuffer, nBytesRead);
				pThis->m_nBufferLen += nBytesRead;
			}
			else
			if (pThis->m_nMaxBufferLen < nBytesRead)
			{
				CopyMemory(pThis->m_strBuffer,
						   &pBuffer[nBytesRead - pThis->m_nMaxBufferLen],
						   pThis->m_nMaxBufferLen);
				pThis->m_nBufferLen = pThis->m_nMaxBufferLen;
			}
			else
			if (pThis->m_nMaxBufferLen < (pThis->m_nBufferLen + nBytesRead))
			{
				DWORD dwOverflow;

				dwOverflow = (pThis->m_nBufferLen + nBytesRead) - pThis->m_nMaxBufferLen;

				MoveMemory(pThis->m_strBuffer,
						   &pThis->m_strBuffer[dwOverflow],
						   pThis->m_nBufferLen - dwOverflow);

				CopyMemory(&pThis->m_strBuffer[pThis->m_nBufferLen - dwOverflow],
						   pBuffer,
						   nBytesRead);

				pThis->m_nBufferLen = pThis->m_nMaxBufferLen;
			}


		}
	}

	return 0;
}

