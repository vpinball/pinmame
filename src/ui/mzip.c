/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.
    
 ***************************************************************************/

/***************************************************************************

  mzip.c
  
    Supports zipping of .cfg and .hi files.
    
 ***************************************************************************/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "MAME32.h"
#include "mzip.h"
#include "resource.h"

#define ZP_DLL_NAME     "ZIP32.DLL\0"
#define ZP_DLL_VER_22   "\2\2\0\0"

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

void strucase(char *str);

/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct _ZpVer
{
	DWORD structlen;         /* length of the struct being passed */
	DWORD flag;              /* bit 0: is_beta   bit 1: uses_zlib */
	char  betalevel[10];     /* e.g., "g BETA" or "" */
	char  date[20];          /* e.g., "4 Sep 95" (beta) or "4 September 1995" */
	char  zlib_version[10];  /* e.g., "0.95" or NULL */
	char  zip[4];
	char  os2dll[4];
	char  windll[4];
} ZpVer;

#define ZPVER_LEN   sizeof(ZpVer)
#define PATH_MAX    128

typedef struct            /* zip options */
{
	BOOL fSuffix;               /* include suffixes (not implemented) */
	BOOL fEncrypt;              /* encrypt files */
	BOOL fSystem;               /* include system and hidden files */
	BOOL fVolume;               /* Include volume label */
	BOOL fExtra;                /* Include extra attributes */
	BOOL fNoDirEntries;         /* Do not add directory entries */
	BOOL fExcludeDate;          /* Exclude files earlier than specified date */
	BOOL fIncludeDate;          /* Include only files earlier than specified date */
	BOOL fVerbose;              /* Mention oddities in zip file structure */
	BOOL fQuiet;                /* Quiet operation */
	BOOL fCRLF_LF;              /* Translate CR/LF to LF */
	BOOL fLF_CRLF;              /* Translate LF to CR/LF */
	BOOL fJunkDir;              /* Junk directory names */
	BOOL fRecurse;              /* Recurse into subdirectories */
	BOOL fGrow;                 /* Allow appending to a zip file */
	BOOL fForce;                /* Make entries using DOS names (k for Katz) */
	BOOL fMove;                 /* Delete files added or updated in zip file */
	BOOL fDeleteEntries;        /* Delete files from zip file */
	BOOL fUpdate;               /* Update zip file--overwrite only if newer */
	BOOL fFreshen;              /* Freshen zip file--overwrite only */
	BOOL fJunkSFX;              /* Junk SFX prefix */
	BOOL fLatestTime;           /* Set zip file time to time of latest file in it */
	BOOL fComment;              /* Put comment in zip file */
	BOOL fOffsets;              /* Update archive offsets for SFX files */
	BOOL fPrivilege;            /* Use privileges (WIN32 only) */
	BOOL fEncryption;           /* TRUE if encryption supported, else FALSE.
								   this is a read only flag */
	int  fRepair;               /* Repair archive. 1 => -F, 2 => -FF */
	char fLevel;                /* Compression level (0 - 9) */
	char Date[9];               /* Date to include after */
	char szRootDir[PATH_MAX];   /* Directory to use as base for zipping */
} ZPOPT, *LPZPOPT;

typedef struct
{
	int   argc;                 /* Count of files to zip */
	LPSTR lpszZipFN;            /* name of archive to create/update */
	char  **FNV;                /* array of file names to zip up */
} ZCL, *LPZCL;

typedef void  (WINAPI *ZPVERSION)(ZpVer *);
typedef BOOL  (WINAPI *ZPSETOPTS)(ZPOPT Opts);
typedef int   (WINAPI *ZPARCHIVE)(ZCL C);

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HANDLE      hZipDll      = NULL;
static ZPARCHIVE   windll_zip   = NULL;
static BOOL        zipdll_found = FALSE;

/***************************************************************************
    External functions  
 ***************************************************************************/

void InitZip(HWND hwnd)
{
	ZpVer		version;
	ZPVERSION	windll_version;
	ZPSETOPTS	windll_setopts;

	zipdll_found = TRUE;
	hZipDll 	 = LoadLibrary(ZP_DLL_NAME);
	
	if ((UINT)hZipDll > (UINT)HINSTANCE_ERROR)
	{
		windll_zip	   = (ZPARCHIVE)GetProcAddress(hZipDll, "ZpArchive");
		windll_version = (ZPVERSION)GetProcAddress(hZipDll, "ZpVersion");
		windll_setopts = (ZPSETOPTS)GetProcAddress(hZipDll, "ZpSetOptions");

		if (windll_version != NULL)
		{
			windll_version(&version);

			/* do we have the correct DLL */
			if ((version.structlen != ZPVER_LEN) ||
				(strncmp(version.windll, ZP_DLL_VER_22, 4) != 0) ||
				(windll_setopts == NULL))
			{
				ExitZip();
			}
			else
			{
				/* Set ZIP options in the DLL */
				ZPOPT opts;

				memset(&opts, '\0', sizeof(ZPOPT));
				opts.fQuiet   = TRUE;
				opts.fJunkDir = TRUE;
				opts.fGrow	  = TRUE;
				opts.fForce   = TRUE;
				opts.fMove	  = TRUE;
				opts.fUpdate  = TRUE;
				opts.Date[0]  = '\0';
				/* Set directory to current directory */
				getcwd(opts.szRootDir, PATH_MAX);
				windll_setopts(opts);
			}
		}
		else
		{
			ExitZip();
		}
	}
	else
	{
		hZipDll = NULL;
		zipdll_found = FALSE;
	}
   
	/* Only show zip32.dll error if:
		1) The DLL is not loaded and
		2) ./cfg.zip, ./nv.zip or ./hi.zip are found
	*/
	if (zipdll_found == FALSE)
	{
		struct stat sbuf;

		if ((stat("cfg.zip", &sbuf) == 0 && (sbuf.st_mode & S_IFDIR) == 0)
		||	(stat("hi.zip",  &sbuf) == 0 && (sbuf.st_mode & S_IFDIR) == 0)
		||	(stat("nv.zip",  &sbuf) == 0 && (sbuf.st_mode & S_IFDIR) == 0))
		{
			char		buf[200];

			strcpy(buf, "Error loading zip32.dll\n\n");
			strcat(buf, "Zipped .cfg, .nv and .hi files\n");
			strcat(buf, "can not be used without it\n\n");
			strcat(buf, "Please reinstall the DLL from\n");
			strcat(buf, "the " MAME32NAME " distribution\n\n");
			strcat(buf, "Would you like to continue\n");
			strcat(buf, "without using the zip files?\n");
			if (IDNO == MessageBox(hwnd, buf, MAME32NAME " Zip32.dll Warning",
								   MB_YESNO | MB_ICONWARNING))
				PostQuitMessage(0);
		}
	}
}

void ExitZip(void)
{
	if (hZipDll != NULL)
		FreeLibrary(hZipDll);
	hZipDll = NULL;
	zipdll_found = FALSE;
}

/* Returns 0 for SUCCESS, non Zero is an error */
int ZipFile(char *zipfile, char *file)
{
	int 		nResult = 1;
	ZCL 		zcl;
	char		fname[20];
	char		*flist[1];
	struct stat sbuf;

	/* If the DLL, the .zip file, or file to archive doesn't exist, return 0 */
	if (hZipDll == NULL || windll_zip == NULL ||
		(stat(zipfile, &sbuf) != 0) ||
		(sbuf.st_mode & S_IFDIR) ||
		((sbuf.st_mode & S_IWRITE) == 0 && chmod(zipfile, S_IWRITE) == -1) ||
		(stat(file, &sbuf) != 0))
		return 0;

	strcpy(fname,file);
	strucase(fname);
	flist[0] = fname;

	zcl.argc = 1;
	zcl.lpszZipFN = zipfile;
	zcl.FNV = flist;

	/* windll_zip() will crash if the zipfile is read only or zero size. */
	__try
	{
		nResult = windll_zip(zcl);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return -1;
	}

	return nResult;
}


void strucase(char *str)
{
	char *ptr = str;

	while(*ptr != '\0')
	{
		if (*ptr >= 'a' && *ptr <= 'z')
			*ptr = toupper(*ptr);
		ptr++;
	}
}
