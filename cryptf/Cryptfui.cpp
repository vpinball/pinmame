/*************************************************
 * File Crypter	UI								 *
 *												 *
 * by Steve Ellenoff (sellenoff@hotmail.com)	 *
 *												 *
 * 05/24/2002									 *
 *************************************************/

#include "cryptf.h"
#include "cryptfui.h"
#include "about.h"

/*Public vars*/
HINSTANCE	g_hInstance = 0;		//Global variable of this app's instance handle
CCrypto * pCrypto;					//Pointer to the Crypto Class
FILETIME ftINI, ftFiles;			//Used to track the last modified date & time of the INI File, and the Files.txt
BOOL bVerify = FALSE;				//Verify before & after encryption/decryption

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormDestroy
//Code to run as Form is being destroyed
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FormDestroy(HWND hWnd) {
	char szTemp[MAX_PATH];

	/*Save INI Values*/
	GetDlgItemText(hWnd,IDC_EDIT2,szTemp,MAX_PATH);
	WriteINI("Defaults","CryptKey",szTemp,"crypt.ini");
	GetDlgItemText(hWnd,IDC_EDIT3,szTemp,MAX_PATH);
	WriteINI("Defaults","Encrypt_Extension",szTemp,"crypt.ini");
	GetDlgItemText(hWnd,IDC_DIRTEXT,szTemp,MAX_PATH);
	WriteINI("Defaults","Encrypt_Directory",szTemp,"crypt.ini");
	GetDlgItemText(hWnd,IDC_DIRTEXT2,szTemp,MAX_PATH);
	WriteINI("Defaults","Decrypt_Directory",szTemp,"crypt.ini");
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormInit
//Code to run as Form is being created
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FormInit(HWND hWnd) {
	char szTemp[MAX_PATH];
	static HICON m_hIcon = 0;			//Anyone know why this has to be static?
	memset(&ftINI,0,sizeof(FILETIME));
	memset(&ftFiles,0,sizeof(FILETIME));

	CenterWindow(hWnd);

	/*Give Dialog an Icon To Left of Caption*/
	m_hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) m_hIcon);

	/*Create tooltips*/
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDC_EDIT2),"Enter the Key (Case Sensitive) used to Encrypt/Decrypt the files",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDC_EDIT3),"Enter the file extension added to files that are encrypted",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDC_DIRTEXT),"Enter the directory where any files that are/or will be encrypted.",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDC_DIRTEXT2),"Enter the directory where any files that are/or will be decrypted.",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDDIR),"Click to select a folder",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDDIR2),"Click to select a folder",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDC_LIST1),"Lists all files that will be Encrypted/Decrypted",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDENCRYPT),"Click to Encrypt all files ready to be processed",0);
	CreateBalloonTip(g_hInstance,GetDlgItem(hWnd,IDDECRYPT),"Click to Decrypt all files ready to be processed",0);

	/*Default values for the directories = the current directory*/
	CurDir(szTemp);
	SetDlgItemText(hWnd,IDC_DIRTEXT,szTemp);
	SetDlgItemText(hWnd,IDC_DIRTEXT2,szTemp);

	/*Store Files Last Modified Times and Call Refresh Data to populate the screen*/
	GetTheFileTimes(&ftINI, &ftFiles);
	RefreshData(hWnd);
	
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormCommand
//Code which handles all WM_COMMAND messages from form
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FormCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int nID;
	nID = LOWORD(wParam);
	switch (HIWORD(wParam))
	{
		//LOST FOCUS Events
		case EN_KILLFOCUS:
			FormLostFocus(hWnd, nID);
	}
	//Buttons were pressed
	FormButtonsPressed(hWnd, nID);
	//Menu Selected
	FormMenuSelected(hWnd, nID);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormLostFocus
//Code to run as child window objects or main form Lose Focus
////////////////////////////////////////////////////////////////////////////////////////////////////
void FormLostFocus(HWND hWnd, UINT nID)
{
	switch (nID) {
		//Enforce a valid directory to be specified!
		case IDC_DIRTEXT:
		case IDC_DIRTEXT2:
		{
			char szTemp[MAX_PATH];
			GetDlgItemText(hWnd,nID,szTemp,MAX_PATH);
			//Only if current textbox is not empty!
			if(!Empty(szTemp)){
				if(!IsDir(szTemp)) {
					MessageBox(hWnd,"Please enter a valid directory!","Unable to Proceed!",MB_ICONERROR);
					SetFocus(GetDlgItem(hWnd,nID));
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormButtonsPressed
//Code to handle form's buttons being pressed
////////////////////////////////////////////////////////////////////////////////////////////////////
void FormButtonsPressed(HWND hWnd, UINT nID)
{
	//Buttons Pressed
	switch (nID) {
		case IDCANCEL:
			EndDialog(hWnd, IDCANCEL);
			break;
		case IDENCRYPT:
			ProcessAllFiles(hWnd,'E');
			break;
		case IDDECRYPT:
			ProcessAllFiles(hWnd,'D');
			break;
		case IDDIR:
			BrowseForDir(hWnd,IDC_DIRTEXT);
			break;
		case IDDIR2:
			BrowseForDir(hWnd,IDC_DIRTEXT2);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormMenuSelected
//Code to handle a form's menu selections
////////////////////////////////////////////////////////////////////////////////////////////////////
void FormMenuSelected(HWND hWnd, UINT nID)
{
	char szTemp[500];

	switch (nID) {
		case ID_SHOWCOMMAND:
			GetHelpMsg(szTemp);
			Display(szTemp,"NOTICE","Help with File Crypter!");
			break;
		case ID_README:
			strcpy(szTemp,"readme.txt");
			AddPathIfMissing(szTemp);
			DisplayTextFile(hWnd,szTemp);
			break;
		case ID_MOD_INI:
			strcpy(szTemp,"crypt.ini");
			AddPathIfMissing(szTemp);
			DisplayTextFile(hWnd,szTemp);
			break;
		case ID_MOD_FILES:
			strcpy(szTemp,"files.txt");
			AddPathIfMissing(szTemp);
			DisplayTextFile(hWnd,szTemp);
			break;
		case ID_ABOUT:
			DialogBox(g_hInstance, (LPCTSTR)IDD_ABOUT, hWnd, (DLGPROC)AboutProc);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormActivate
//Code to handle a form when it becomes activated!
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FormActivate(HWND hWnd) {
	/*Check if either of the 2 files have been modified, and if so, call refreshdata!*/
	FILETIME ftINI_New, ftFiles_New;
	memset(&ftINI_New,0,sizeof(FILETIME));
	memset(&ftFiles_New,0,sizeof(FILETIME));
	GetTheFileTimes(&ftINI_New, &ftFiles_New);
	if(CompareFileTime(&ftINI,&ftINI_New) !=0 ||
	   CompareFileTime(&ftFiles,&ftFiles_New) !=0) {
		/*Update with the latest modified times (NOTE: MUST come before call to RefreshData() or endless loop occurs!)*/
		ftINI = ftINI_New;
		ftFiles = ftFiles_New;
		/*Refresh the data from the files*/
		RefreshData(hWnd);
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//FormDeActivate
//Code to handle a form when it becomes deactivated!
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FormDeActivate(HWND hWnd) {
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//RefreshData
//Loads all data in from the INI File and the Files List, and populates the controls
////////////////////////////////////////////////////////////////////////////////////////////////////
void RefreshData(HWND hWnd)
{
	char szTemp[MAX_PATH];

	/*Get list of files to process and populate in list box*/
	GetFiles(hWnd);

	/*Read in default values from INI File*/
	ReadINI(szTemp,"Defaults","CryptKey","crypt.ini");
	SetDlgItemText(hWnd,IDC_EDIT2,szTemp);
	ReadINI(szTemp,"Defaults","Encrypt_Extension","crypt.ini");
	SetDlgItemText(hWnd,IDC_EDIT3,szTemp);
	ReadINI(szTemp,"Defaults","Encrypt_Directory","crypt.ini");
	/*Ensure directory is valid - Skip if empty*/
	if(!Empty(szTemp)) {
		if(!IsDir(szTemp)) {
			char szMessage[MAX_PATH];
			wsprintf(szMessage,"Encrypted File Directory specified in configuration is invalid!\nValue=%s\nUsing default directory instead!",szTemp);
			Display(szMessage,"WARNING","Warning!");
			}
		else
			SetDlgItemText(hWnd,IDC_DIRTEXT,szTemp);
	}

	ReadINI(szTemp,"Defaults","Decrypt_Directory","crypt.ini");
	/*Ensure directory is valid - Skip if empty*/
	if(!Empty(szTemp)) {
		if(!IsDir(szTemp)) {
			char szMessage[MAX_PATH];
			wsprintf(szMessage,"Decrypted File Directory specified in configuration is invalid!\nValue=%s\nUsing default directory instead!",szTemp);
			Display(szMessage,"WARNING","Warning!");
		}
		else
			SetDlgItemText(hWnd,IDC_DIRTEXT2,szTemp);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//BrowseForDir:
//Grab current specified directory, and browse for a new directory, defaulting to the one specified!
//And then specify the new selection in the text box
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BrowseForDir(HWND hWnd, UINT ctrlid) {
		char szDir[MAX_PATH];
		char buf[MAX_PATH];
		GetDlgItemText(hWnd,ctrlid,buf,MAX_PATH);
		if ( GetDir(szDir,buf,"","",BIF_RETURNONLYFSDIRS,FALSE) )
			SetDlgItemText(hWnd, ctrlid, szDir);
		return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//MainDlgProc
//Callback for the Dialog
////////////////////////////////////////////////////////////////////////////////////////////////////
int PASCAL MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg ) {
		case WM_DESTROY:
			return FormDestroy(hWnd);
		case WM_INITDIALOG:
			return FormInit(hWnd);
		case WM_COMMAND:
			return FormCommand(hWnd, wParam, lParam);
		case WM_ACTIVATE:
			if(wParam==WA_INACTIVE)
				return FormDeActivate(hWnd);
			else
				return FormActivate(hWnd);
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//WinMain
//Code that runs when the program begins. Creates the Dialog Form
////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	/*Create Crypto Class*/
	pCrypto = new CCrypto();
	if(!pCrypto) {
		Display("Unable to create crypto class!","ERROR","Application - Serious Error!");
		return FALSE;
	}

	/*If Command Line options specified, process them and do not load the UI*/
	if(!Empty(lpCmdLine))
		return (int)RunFromCommandLine(lpCmdLine);
	/*No command line, so run the UI*/
	else {
		g_hInstance = hInstance;
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, MainDlgProc);
	}

	/*Remove class from memory*/
	delete pCrypto;

	return 1;
}
