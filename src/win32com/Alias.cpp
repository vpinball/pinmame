#include <string.h>
extern "C" {
  #include "driver.h"
  #include "audit.h"
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#if _MSC_VER >= 1800
 // Windows 2000 _WIN32_WINNT_WIN2K
 #define _WIN32_WINNT 0x0500
#elif _MSC_VER < 1600
 #define _WIN32_WINNT 0x0400
#else
 #define _WIN32_WINNT 0x0403
#endif
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>
#include "Alias.h"

// do not remove, the following array is being filled by vpinmame.yml via VPMAlias.txt
static const struct tAliasTable { const char* alias; const char* real; } aliasTable[] = {
/*VPMALIAS*/
	  { NULL, NULL }
};

static char alias_from_file[50];

const char* checkGameAlias(const char* aRomName) {
	char AliasFilename[MAX_PATH];

#ifndef _WIN64
	const HINSTANCE hInst = GetModuleHandle("VPinMAME.dll");
#else
	const HINSTANCE hInst = GetModuleHandle("VPinMAME64.dll");
#endif
	GetModuleFileName(hInst, AliasFilename, MAX_PATH);
	char* ptr = strrchr(AliasFilename, '\\');
	if (ptr != NULL)
	{
		strcpy_s(ptr + 1, 13, "VPMAlias.txt");

		FILE *f = fopen(AliasFilename, "r");

		if (f != NULL)
		{
			char line[128];
			while (fgets(line, sizeof(line), f) != NULL)
			{
				// Skip lines that start with "#"
				if (line[0] == '#') {
					continue;
				}
				char *token = strtok(line, ", ");
				
				if (_stricmp(token, aRomName) == 0)
				{
					strcpy_s(alias_from_file, sizeof(alias_from_file), strtok(NULL, " ,\n#;'"));
					fclose(f);
					return alias_from_file;
				}
			}
			fclose(f);
		}
	}
	for (const tAliasTable* ii = aliasTable; ii->alias; ++ii)
		if (_stricmp(aRomName, ii->alias) == 0) return ii->real;
	return aRomName;
}
