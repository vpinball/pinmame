#ifndef VPINMAMECONFIG_H
#define VPINMAMECONFIG_H

void cli_frontend_init();
void cli_frontend_exit();

int set_option(const char *name, const char *arg, int priority);
void *get_option(const char *name);

BOOL  WriteRegistry(char* pszKey, char* pszName, DWORD dwValue);
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault);

void LoadGlobalSettings();
void SaveGlobalSettings();
void DeleteGlobalSettings();

void LoadGameSettings(char *szName);
void SaveGameSettings(char *szName);
void DeleteGameSettings(char *szName);

char* GetInstallDir(char *pszInstallDir, int iSize);

#endif // VPINMAMECONFIG_H