#ifndef CONTROLLEROPTIONS_H
#define CONTROLLEROPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagCONTROLLEROPTIONS {
	char	szROMName[32];
	int		nGameNo;
	char	szDescription[256];
	BOOL	fUseCheat;
	BOOL	fUseSound;
	BOOL	fUseSamples;
	BOOL	fCompactSize; // WPCMAME
	BOOL	fDoubleSize;  // WPCMAME
	int		nAntiAlias;   // WPCMAME
	int		nSampleRate;
	int		nDMDRED;
	int		nDMDGREEN;
	int		nDMDBLUE;
	int		nDMDPERC66;
	int		nDMDPERC33;
	int		nDMDPERC0;
	BOOL	fSpeedLimit;

	// these options are NOT saved to the registry
	BOOL	fHandleKeyboard;
	int		iHandleMechanics;
	BOOL	fShowTitle;
	BOOL	fShowDMDOnly;
	int		nBorderSizeX;
	int		nBorderSizeY;
	BOOL	fShowFrame;
	int		nWindowPosX;
	int     nWindowPosY;
	BOOL	fDisplayLocked;
	BOOL	fAntialias;

} CONTROLLEROPTIONS, *PCONTROLLEROPTIONS;

typedef struct tagCONTROLLERPATHES {
	char	szRomDirs[MAX_PATH];
	char	szCfgDir[MAX_PATH];
	char	szNVRamDir[MAX_PATH];
	char	szSampleDirs[MAX_PATH];
	char	szImgDir[MAX_PATH];
} CONTROLLERPATHES, *PCONTROLLERPATHES;

extern PCONTROLLEROPTIONS g_pControllerOptions;
extern PCONTROLLERPATHES  g_pControllerPathes;

#ifdef __cplusplus
}
#endif

BOOL GetOptions(PCONTROLLEROPTIONS pControllerOptions, char* pszROMName);
BOOL PutOptions(PCONTROLLEROPTIONS pControllerOptions, char* pszROMName);
void DelOptions(char* pszROMName);

void GetPathes(PCONTROLLERPATHES pControllerPathes);
void PutPathes(PCONTROLLERPATHES pControllerPathes);
void DelPathes();

int	  GetGameNumFromString(char *name);
char* GetInstallDir(char *pszInstallDir, int iSize);
char* GetGameRegistryKey(char *pszRegistryKey, char* pszGameName);
BOOL  GameUsedTheFirstTime(char* pszROMName);

BOOL GameWasNeverStarted(char* pszROMName);
void SetGameWasStarted(char* pszROMName);

#endif // CONTROLLEROPTIONS_H
