#ifndef VPINMAMECONFIG_H
#define VPINMAMECONFIG_H

void cli_frontend_init();
void cli_frontend_exit();

int set_option(const char *name, const char *arg, int priority);
void *get_option(const char *name);

BOOL  WriteRegistry(char* pszKey, char* pszName, DWORD dwValue);
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault);

void LoadGlobalSettings();
void DeleteGlobalSettings();

void LoadGameSettings(char* pszGameName);
void DeleteGameSettings(char *pszGameName);

char* GetInstallDir(char *pszInstallDir, int iSize);

BOOL PutSetting(char* pszGameName, char* pszName, VARIANT vValue);
BOOL GetSetting(char* pszGameName, char* pszName, VARIANT *pVal);

BOOL SettingAffectsRunningGame(const char* pszName);

#endif // VPINMAMECONFIG_H