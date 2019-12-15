#ifndef VPINMAMECONFIG_H
#define VPINMAMECONFIG_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

extern void vpm_frontend_init(void);
extern void vpm_frontend_exit(void);
#define cli_frontend_init vpm_frontend_init
#define cli_frontend_exit vpm_frontend_exit

void vpm_game_init(int game_index);
void vpm_game_exit(int game_index);

int set_option(const char* const name, const char * const arg, int priority);
void *get_option(const char* const name);

BOOL  WriteRegistry(const char* const pszKey, const char* const pszName, DWORD dwValue);
DWORD ReadRegistry(const char* const pszKey, const char* const pszName, DWORD dwDefault);

void LoadGlobalSettings();
void DeleteGlobalSettings();

void LoadGameSettings(const char* const pszGameName);
void DeleteGameSettings(const char* const pszGameName);

char* GetInstallDir(char * const pszInstallDir, int iSize);

BOOL PutSetting(const char* const pszGameName, const char* const pszName, VARIANT vValue);
BOOL GetSetting(const char* const pszGameName, const char* const pszName, VARIANT *pVal);

BOOL SettingAffectsRunningGame(const char* const pszName);

#endif // VPINMAMECONFIG_H