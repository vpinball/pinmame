#ifndef VPINMAMECONFIG_H
#define VPINMAMECONFIG_H

void cli_frontend_init();

int set_option(const char *name, const char *arg, int priority);
void *get_option(const char *name);

BOOL  WriteRegistry(char* pszKey, char* pszName, DWORD dwValue);
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault);

void Load_fileio_opts();
void Save_fileio_opts();
void Delete_fileio_opts();

char* GetInstallDir(char *pszInstallDir, int iSize);

#endif // VPINMAMECONFIG_H