/*************************************************
 * File Crypter (Non UI)						 *
 *												 *
 * by Steve Ellenoff (sellenoff@hotmail.com)	 *
 *												 *
 * 05/24/2002									 *
 *************************************************/

#include <stdio.h>
#include "cryptf.h"
#include "cryptfui.h"

extern BOOL bVerify;
extern CCrypto * pCrypto;

////////////////////////////////////////////////////////////////////////////////////////////////////
//GetFiles
//
//Reads the "files.txt" file and reads in all file names into the list box control
//Ignores empty lines and comments which start with ";"
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetFiles(HWND hWnd) {
	FILE *fp;
	HWND hList = GetDlgItem(hWnd,IDC_LIST1);
	UINT nFound = 0;
	char szTemp[MAX_PATH];
	char szTemp2[MAX_PATH];

	//Clear listbox contents
	SendMessage(hList,LB_RESETCONTENT,0,0);
	/*Update label*/
	wsprintf(szTemp,"0 Files to be processed (specified in 'Files.txt')");
	SetWindowText(GetDlgItem(hWnd,IDC_FILESLABEL),szTemp);

	if(!File("files.txt"))  {
		Display("Unable to locate required file list: files.txt","ERROR","Application - Serious Error!");
		return FALSE;
	}

	/*Read in files*/
	if((fp=fopen("files.txt","r"))==NULL) {
		Display("Unable to open files.txt for reading!","ERROR","Application - Serious Error!");
		return FALSE;
	}
	while(!feof(fp)){
		char szTemp1[2];
		if(fgets(szTemp,MAX_PATH,fp)) {
			if(!Empty(szTemp)) {
				Left(szTemp1,szTemp,1);
				if(strcmp(szTemp1,";")!=0 && strcmp(szTemp1,"\n")!=0) {
					nFound++;
					//Remove any line feeds
					Strtran(szTemp2,szTemp,"\n","",-1,-1,0);
					SendMessage(hList,LB_ADDSTRING,0,(LPARAM)szTemp2);
				}
			}
		}
	}
	fclose(fp);
	/*Update label*/
	wsprintf(szTemp,"%d File%s to be processed (specified in 'Files.txt')",nFound,(nFound==1)?"":"s");
	SetWindowText(GetDlgItem(hWnd,IDC_FILESLABEL),szTemp);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//ProcessAllFiles
//
//UI Based logic from Dialog Form
//Reads all values from the UI and process each file from the Listbox
////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessAllFiles(HWND hWnd, char cType)
{
	HWND hList = GetDlgItem(hWnd,IDC_LIST1);
	UINT nDone, nCount, i;
	char szEnc[MAX_PATH];
	char szDec[MAX_PATH];
	char szKey[MAX_PATH];
	char szExt[MAX_PATH];
	char szSource[MAX_PATH];
	char szDest[MAX_PATH];
	char szMessage[MAX_PATH];
	char szFileName[MAX_PATH];
	char szTempFile[MAX_PATH];
	nDone = nCount = i = 0;
	GetDlgItemText(hWnd,IDC_EDIT2,szKey,MAX_PATH);
	GetDlgItemText(hWnd,IDC_EDIT3,szExt,MAX_PATH);
	GetDlgItemText(hWnd,IDC_DIRTEXT,szEnc,MAX_PATH);
	GetDlgItemText(hWnd,IDC_DIRTEXT2,szDec,MAX_PATH);

	/* CHECK FOR EMPTY DATA FROM UI INPUT */
	if(Empty(szKey)) {
		Display("Please Enter the Key to use for Encryption/Decryption!","ERROR","Unable to Proceed!");
		SetFocus(GetDlgItem(hWnd,IDC_EDIT2));
		return;
	}
	if(Empty(szEnc)) {
		Display("Please specify the Encrypted Files Directory!","ERROR","Unable to Proceed!");
		SetFocus(GetDlgItem(hWnd,IDC_DIRTEXT));
		return;
	}
	if(Empty(szDec)) {
		Display("Please specify the Decrypted Files Directory!","ERROR","Unable to Proceed!");
		SetFocus(GetDlgItem(hWnd,IDC_DIRTEXT2));
		return;
	}
	//Note: We allow key to be empty! - But we ensure if it's not, we add a '.' to the ext.
	if(!Empty(szExt)) {
		char szTemp[2];
		Left(szTemp,szExt,1);
		if(strcmp(szTemp,".") != 0)
			PrependToString(szExt,".");
	}
	
	//Refresh the list again
	if(!GetFiles(hWnd))
		return;

	//Make sure directories end with a backslash
	AddBs(szEnc);
	AddBs(szDec);

	//Temp file will be created in the decrypted folder
	wsprintf(szTempFile,"%supdate.tmp",szDec);

	//For EACH FILE listed in Listbox
	nCount = SendMessage(hList,LB_GETCOUNT,0,0);
	for(i=0; i<nCount; i++)
	{
		char szTemp[MAX_PATH];
		SendMessage(hList,LB_GETTEXT, i, (LPARAM)szTemp);
		strcpy(szFileName,szTemp);
		strcpy(szSource,szFileName);
		strcpy(szDest,szSource);

		//Setup Source file string
		PrependToString(szSource,(cType=='D')?szEnc:szDec);
		strcat(szSource,(cType=='D')?szExt:"");
		//Setup Destination file string
		PrependToString(szDest,(cType=='E')?szEnc:szDec);
		strcat(szDest,(cType=='E')?szExt:"");
		
		//Process this file!
		nDone += ProcessFile(szSource, szDest, szKey, cType, szFileName, szTempFile);
	}
	wsprintf(szMessage,"%d File%s %s!",nDone,(nDone==1)?"":"s",(cType=='E')?"Encrypted":"Decrypted");
	Display(szMessage,"NOTICE","Notice!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ProcessFile
//
//NON-UI Based logic.. 
//Processes 1 file, based on fullpath of source and destination sent, as well as key, and type..
//FileName, and TempFile names are optional, but included for faster processing when called from ProcessAllFiles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ProcessFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, char cType, LPCSTR szFileName, LPCSTR szTempFile)
{
	UINT nDone = 0;
	char szMessage[MAX_PATH];
	char szFileName1[MAX_PATH];
	char szTempFile1[MAX_PATH];

	if(Empty(szSource)) Display("Source file not specified!","ERROR","Application - Serious Error");
	if(Empty(szDestination)) Display("Destination file not specified!","ERROR","Application - Serious Error");

	//If no filename, we must parse it...
	//	from Sourcefile when Encrypting, and Destination File when Decrypting!
	if(Empty(szFileName)) {
		if(cType=='E') {
			UINT nPos = Rat("\\",szSource,1);
			UINT nLen = Len(szSource) - nPos;
			if(nPos <= 0) {
				Display("Source file must include Path!","ERROR","Application - Serious Error");
				return 0;
			}
			Right(szFileName1,szSource,nLen);
		}
		else {
			UINT nPos = Rat("\\",szDestination,1);
			UINT nLen = Len(szDestination) - nPos;
			if(nPos <= 0) {
				Display("Destination file must include Path!","ERROR","Application - Serious Error");
				return 0;
			}
			Right(szFileName1,szDestination,nLen);
		}
	}
	else
		strcpy(szFileName1,szFileName);

	//If no tempfile we must parse the directory from the destination
	if(Empty(szTempFile)) {
		UINT nPos = Rat("\\",szDestination,1);
		if(nPos <= 0) {
			Display("Destination file must include Path!","ERROR","Application - Serious Error");
			return 0;
		}
		Left(szTempFile1,szDestination,nPos);
		strcat(szTempFile1,"update.tmp");
	}
	else
		strcpy(szTempFile1,szTempFile);

	//Verify source file exists!
	if(!File(szSource)) {
		wsprintf(szMessage,"Cannot find file: %s!",szSource);
		Display(szMessage,"ERROR","Warning!");
		return 0;
	}

	//ENCRYPTION:
	if(cType=='E') {
		if(EncryptFile(szSource, szDestination, szKey, szFileName1,szTempFile1))
			nDone++;
	}
	//DECRYPTION:
	else  {
		if(DecryptFile(szSource, szDestination, szKey, szFileName1,szTempFile1))
			nDone++;		
	}

	/*Delete Temp File*/
	DeleteAFile(szTempFile1);

	return nDone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//EncryptFile
//
//Logic to Encrypt a file, including verification before and after the encryption
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL EncryptFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, LPCSTR szFileName, LPCSTR szTempFile)
{
	BOOL bSuccess = FALSE;
	char szMessage[MAX_PATH];

	if(bVerify) {
		/*See if file is already encrypted!
		  Assumes the name of the file itself, is contained in the file (usually at top, but doesn't matter where)*/
		if(!FindInFile(szSource,szFileName)) {
			wsprintf(szMessage,"File: %s is already encrypted!",szSource);
			Display(szMessage,"ERROR","Skipping File!");
			return bSuccess;
		}
	}

	/*Attempt to Encrypt it!*/
	if(pCrypto->EncryptFile(szSource,szKey,szTempFile)) {

		if(bVerify) {
			/*Verify it worked!*/
			if(FindInFile(szTempFile,szFileName)) {
				wsprintf(szMessage,"File: %s did not encrypt properly! Please try again!",szSource);
				Display(szMessage,"ERROR","Warning!");
				return bSuccess;
			}
		}
		
		/*Copy the file from the temp file*/
		bSuccess = CopyAFile(szTempFile,szDestination);

		/*Message if failed*/
		if(!bSuccess) {
			wsprintf(szMessage,"File: %s did not encrypt successfully!",szSource);
			Display(szMessage,"ERROR","Warning!");
		}
	}
	//No message on failure of calling pCrypto-> method, because it will provide it's own message!
	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//DecryptFile
//
//Logic to Decrypt a file, including verification before and after the encryption
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DecryptFile(LPCSTR szSource, LPCSTR szDestination, LPCSTR szKey, LPCSTR szFileName, LPCSTR szTempFile)
{
	BOOL bSuccess = FALSE;
	char szMessage[MAX_PATH];

	if(bVerify) {
		/*See if file is already decrypted!
		  Assumes the name of the file itself, is contained in the file (usually at top, but doesn't matter where)*/
		if(FindInFile(szSource,szFileName)) {
				wsprintf(szMessage,"File: %s is already decrypted!",szSource);
				Display(szMessage,"ERROR","Skipping File!");
				return bSuccess;
		}
	}

	/*Attempt to Decrypt it!*/
	if(pCrypto->DecryptFile(szSource,szKey,szTempFile)) {

		if(bVerify) {
			/*Verify it worked!*/
			if(!FindInFile(szTempFile,szFileName)) {
				wsprintf(szMessage,"File: %s did not decrypt properly!\nCheck your key and try again!",szSource);
				Display(szMessage,"ERROR","Warning!");
				return bSuccess;
			}
		}

		/*Copy the file from the temp file*/
		bSuccess = CopyAFile(szTempFile,szDestination);

		if(!bSuccess) {
			wsprintf(szMessage,"File: %s did not decrypt successfully!",szSource);
			Display(szMessage,"ERROR","Warning!");
		}
	}
	//No message on failure of calling pCrypto-> method, because it will provide it's own message!
	return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CopyAFile
//
//Attempts to copy source to destination file. 
//If it fails because the destination file exists, and is read-only, attempts to reset this flag and try again!
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CopyAFile (LPCSTR szSource, LPCSTR szDestination)
{
	BOOL bSuccess = FALSE;
	/*Attempt to copy it!*/
	if(CopyFile(szSource,szDestination,FALSE))
		bSuccess = TRUE;
	else {
		/*COPY FAILED!!*/
		/*See if destination file is read-only*/
		if(GetFileAttributes(szDestination) & FILE_ATTRIBUTE_READONLY) {
			/*Set it to Normal, and try the copy again!*/
			if(SetFileAttributes(szDestination, FILE_ATTRIBUTE_NORMAL)) {
				if(CopyFile(szSource,szDestination,FALSE))
					bSuccess = TRUE;
				}
			}
		}
	return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RunFromCommandLine
//
//Parses the command line, and performs the command line action
//
//Command options in order are:
//-D (or -E) for Decrypt/Encrypt
//(Source File Name) 
//(Destination File Name)
//(Ini File Name)
//
//NOTE: Source & Destination CAN include full path, but if they don't. Path is determined from ini
//      If no path is included for INI file, it assumes applications current directory
//ex:
//cryptf -D Controller.cpp.crypt Controller.cpp cryptf.ini
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RunFromCommandLine(LPCSTR szCmdLine)
{
	char cType;
	char szType[MAX_PATH];
	char szSource[MAX_PATH];
	char szDestination[MAX_PATH];
	char szIniFile[MAX_PATH];
	char szMessage[MAX_PATH];
	char szKey[MAX_PATH];
	char szTemp[MAX_PATH];

	//Check if user wanted help with command line options
	Left(szTemp,szCmdLine,2);
	if(strcmp(szTemp,"/?")==0) {
		GetHelpMsg(szMessage);
		Display(szMessage,"NOTICE","Help with File Crypter!");
		return TRUE;
	}

	//Make sure user passed all 4 arguments!
	if(__argc < 5) {
		Display("Please specify all 4 parameters to cryptf.exe!","ERROR","Application - Serious Error!");
		return FALSE;
	}

	strcpy(szType,__argv[1]);
	strcpy(szTemp,__argv[2]);
	Strtran(szSource,szTemp,"\"","",-1,-1,0);		//Remove any Quotes(") from string
	strcpy(szTemp,__argv[3]);
	Strtran(szDestination,szTemp,"\"","",-1,-1,0);	//Remove any Quotes(") from string
	strcpy(szTemp,__argv[4]);
	Strtran(szIniFile,szTemp,"\"","",-1,-1,0);		//Remove any Quotes(") from string

	//Add a Path to Ini File if not specified!
	AddPathIfMissing(szIniFile);
	if(!File(szIniFile)) {
		wsprintf(szMessage,"The specified INI File: %s does not exist!",szIniFile);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE;
	}
	//Check for valid Type!
	if(strcmp("-D",szType)==0 || strcmp("-d",szType)==0)
		cType = 'D';
	else if(strcmp("-E",szType)==0 || strcmp("-e",szType)==0)
		cType = 'E';
	else {
		wsprintf(szMessage,"The specified action: %s is not valid!",szType);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE; 
	}
	//Read Key from INI file!
	ReadINI(szKey,"Defaults","CryptKey",szIniFile);
	if(Empty(szKey)) {
		wsprintf(szMessage,"The specified INI File: %s does not contain a valid Key!",szIniFile);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE;
	}
	//If Directories are not specified, grab it from the INI file and verify it!!
	if(!HasPath(szSource)) {
		char szTemp[MAX_PATH];
		char szWhich[MAX_PATH];
		
		if(cType=='D')
			strcpy(szWhich,"Encrypt_Directory");
		else
			strcpy(szWhich,"Decrypt_Directory");

		ReadINI(szTemp,"Defaults",szWhich,szIniFile);
		if(Empty(szTemp) || !IsDir(szTemp)) {
			wsprintf(szMessage,"The specified INI File: %s does not contain a valid entry for: %s!",szIniFile, szWhich);
			Display(szMessage,"ERROR","Application - Serious Error!");
			return FALSE;
		}
		//Add Directory to the filename!
		AddBs(szTemp);
		PrependToString(szSource,szTemp);
	}
	//If Directories are not specified, grab it from the INI file and verify it!!
	if(!HasPath(szDestination)) {
		char szTemp[MAX_PATH];
		char szWhich[MAX_PATH];
		
		if(cType=='E')
			strcpy(szWhich,"Encrypt_Directory");
		else
			strcpy(szWhich,"Decrypt_Directory");

		ReadINI(szTemp,"Defaults",szWhich,szIniFile);
		if(Empty(szTemp) || !IsDir(szTemp)) {
			wsprintf(szMessage,"The specified INI File: %s does not contain a valid entry for: %s!",szIniFile, szWhich);
			Display(szMessage,"ERROR","Application - Serious Error!");
			return FALSE;
		}
		//Add Directory to the filename!
		AddBs(szTemp);
		PrependToString(szDestination,szTemp);
	}
	
	//Ok we're ready to do it!
	return ProcessFile(szSource,szDestination,szKey,cType,"","");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//GetHelpMsg
//
//Returns a string containing the command line help message
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetHelpMsg(LPSTR szString)
{
	wsprintf(szString,"Usage: cryptf -Type SourceFile DestinationFile INIFile\n\nType: E=Encryption, D=Decryption\nSourceFile: Name of source file (if no path included it's read from INI File)\nDestinationFile: Name of destination file (if no path included it's read from INI File)\nINIFile: Name of INI file (if no path included uses current directory)");
	strcat(szString,"\n\nExample:\ncryptf -D test.cpp.crypt test.cpp crypt.ini");
	strcat(szString,"\ncryptf -E c:\\docs\\test.cpp.crypt c:\\output\\test.cpp c:\\cryptf\\crypt.ini");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//GetTheFileTimes
//
//Populates the last modified FILETIME structures of the INI File, and the Files.txt file
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetTheFileTimes(FILETIME *ftINI, FILETIME *ftFiles)
{
	char szTemp[MAX_PATH];
	strcpy(szTemp,"crypt.ini");
	AddPathIfMissing(szTemp);
	*ftINI = GetModifiedFileTime(szTemp,FALSE);
	strcpy(szTemp,"files.txt");
	AddPathIfMissing(szTemp);
	*ftFiles = GetModifiedFileTime(szTemp,FALSE);
}













////////OLD WAY - WHEN I HAD TO MANUALLY PARSE THE COMMAND LINE////////////

#if 0
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RunFromCommandLine
//
//Parses the command line, and performs the command line action
//
//Command options in order are:
//?D (or ?E) for Decrypt/Encrypt
//?(Source File Name) 
//?(Destination File Name)
//?(Ini File Name)
//
//NOTE: Source & Destination CAN include full path, but if they don't. Path is determined from ini
//      If no path is included for INI file, it assumes applications current directory
//
// ALL PARMS MUST begin with a " ?" (see example below)
//
//ex:
//cryptf -D -Controller.cpp.crypt -Controller.cpp -cryptf.ini
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RunFromCommandLine(LPCSTR szCmdLine)
{
	char cType;
	char szType[MAX_PATH];
	char szSource[MAX_PATH];
	char szDestination[MAX_PATH];
	char szIniFile[MAX_PATH];
	char szMessage[MAX_PATH];
	char szKey[MAX_PATH];
	char szTemp[MAX_PATH];
	UINT nPos,nHoldPos;

	//Check if user wanted help with command line options
	Left(szTemp,szCmdLine,2);
	if(strcmp(szTemp,"/?")==0) {
		GetHelpMsg(szMessage);
		Display(szMessage,"NOTICE","Help with File Crypter!");
		return TRUE;
	}

	//Make sure user passed all 4 arguments!
	if(Occurs(" ?",szCmdLine) < 3 || Occurs("?",szCmdLine) < 4) {
		Display("Please specify all 4 parameters to cryptf.exe!","ERROR","Application - Serious Error!");
		return FALSE;
	}

	nHoldPos = 1;
	//Get 1st Parm - Type!
	nPos = At(" ?",szCmdLine,1);
	Substr(szType,szCmdLine,nHoldPos,nPos-nHoldPos);
	nHoldPos = nPos+2;
	//Get 2nd Parm - Source File!
	nPos = At(" ?",szCmdLine,2);
	Substr(szTemp,szCmdLine,nHoldPos,nPos-nHoldPos);
	Strtran(szSource,szTemp,"\"","",-1,-1,0);	//Remove any Quotes(") from string
	nHoldPos = nPos+2;
	//Get 3rd Parm - Destination!
	nPos = At(" ?",szCmdLine,3);
	Substr(szTemp,szCmdLine,nHoldPos,nPos-nHoldPos);	
	Strtran(szDestination,szTemp,"\"","",-1,-1,0);	//Remove any Quotes(") from string
	nHoldPos = nPos+2;
	//Get 4th Parm - Ini File!
	Substr(szTemp,szCmdLine,nHoldPos,-1);
	Strtran(szIniFile,szTemp,"\"","",-1,-1,0);	//Remove any Quotes(") from string

	//Add a Path to Ini File if not specified!
	AddPathIfMissing(szIniFile);
	if(!File(szIniFile)) {
		wsprintf(szMessage,"The specified INI File: %s does not exist!",szIniFile);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE;
	}
	//Check for valid Type!
	if(strcmp("?D",szType)==0)
		cType = 'D';
	else if(strcmp("?E",szType)==0)
		cType = 'E';
	else {
		wsprintf(szMessage,"The specified action: %s is not valid!",szType);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE; 
	}
	//Read Key from INI file!
	ReadINI(szKey,"Defaults","CryptKey",szIniFile);
	if(Empty(szKey)) {
		wsprintf(szMessage,"The specified INI File: %s does not contain a valid Key!",szIniFile);
		Display(szMessage,"ERROR","Application - Serious Error!");
		return FALSE;
	}
	//If Directories are not specified, grab it from the INI file and verify it!!
	if(!HasPath(szSource)) {
		char szTemp[MAX_PATH];
		char szWhich[MAX_PATH];
		
		if(cType=='D')
			strcpy(szWhich,"Encrypt_Directory");
		else
			strcpy(szWhich,"Decrypt_Directory");

		ReadINI(szTemp,"Defaults",szWhich,szIniFile);
		if(Empty(szTemp) || !IsDir(szTemp)) {
			wsprintf(szMessage,"The specified INI File: %s does not contain a valid entry for: %s!",szIniFile, szWhich);
			Display(szMessage,"ERROR","Application - Serious Error!");
			return FALSE;
		}
		//Add Directory to the filename!
		AddBs(szTemp);
		PrependToString(szSource,szTemp);
	}
	//If Directories are not specified, grab it from the INI file and verify it!!
	if(!HasPath(szDestination)) {
		char szTemp[MAX_PATH];
		char szWhich[MAX_PATH];
		
		if(cType=='E')
			strcpy(szWhich,"Encrypt_Directory");
		else
			strcpy(szWhich,"Decrypt_Directory");

		ReadINI(szTemp,"Defaults",szWhich,szIniFile);
		if(Empty(szTemp) || !IsDir(szTemp)) {
			wsprintf(szMessage,"The specified INI File: %s does not contain a valid entry for: %s!",szIniFile, szWhich);
			Display(szMessage,"ERROR","Application - Serious Error!");
			return FALSE;
		}
		//Add Directory to the filename!
		AddBs(szTemp);
		PrependToString(szDestination,szTemp);
	}
	
	//Ok we're ready to do it!
	return ProcessFile(szSource,szDestination,szKey,cType,"","");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//GetHelpMsg
//
//Returns a string containing the command line help message
////////////////////////////////////////////////////////////////////////////////////////////////////
void GetHelpMsg(LPSTR szString)
{
	wsprintf(szString,"Usage: cryptf ?Type ?SourceFile ?DestinationFile ?INIFile\n\nType: E=Encryption, D=Decryption\nSourceFile: Name of source file (if no path included it's read from INI File)\nDestinationFile: Name of destination file (if no path included it's read from INI File)\nINIFile: Name of INI file (if no path included uses current directory)");
	strcat(szString,"\n\nExample:\ncryptf ?D ?test.cpp.crypt ?test.cpp ?crypt.ini");
	strcat(szString,"\ncryptf ?E ?c:\\docs\\test.cpp.crypt ?c:\\output\\test.cpp ?c:\\cryptf\\crypt.ini");
}
#endif