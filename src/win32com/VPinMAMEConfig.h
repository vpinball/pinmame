#ifndef VPINMAMECONFIG_H
#define VPINMAMECONFIG_H

void cli_frontend_init();

int set_option(const char *name, const char *arg, int priority);

BOOL  WriteRegistry(char* pszKey, char* pszName, DWORD dwValue);
DWORD ReadRegistry(char* pszKey, char* pszName, DWORD dwDefault);

#endif // VPINMAMECONFIG_H