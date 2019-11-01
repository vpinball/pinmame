#include <string.h>
extern "C" {
  #include "driver.h"
  #include "audit.h"
}
#include <windows.h>
#include "Alias.h"

static const struct tAliasTable { const char* alias; const char* real; } aliasTable[] = {
  { "eshak_f1", "esha_pr4" },
  { "eshak_l1", "esha_la1" },
  { "eshak_l3", "esha_la3" },
  { "lwar", "lwar_a83" },
  { "ssvc", "ssvc_a26" },
  { "torpe", "torp_e21" },
  { "tmach", "tmac_a24" },
  { "play", "play_a24" },
  { "maverick", "mav_401" },
  { "mnfb", "mnfb_c27" },
  { "robo", "robo_a34" },
  { "poto", "poto_a32" },
  { "bttf", "bttf_a20" },
  { "simp", "simp_a27" },
  { "chkpnt", "ckpt_a17" },
  { "tmnt", "tmnt_104" },
  { "batmn", "btmn_101" },
  { "trek", "trek_201" },
  { "hook", "hook_408" },
  { "lw3", "lw3_208" },
  { "stwarde", "stwr_103" },
  { "rab", "rab_130" },
  { "jurpark", "jupk_513" },
  { "lah", "lah_112" },
  { "tftc", "tftc_303" },
  { "tommy", "tomy_400" },
  { "wwfrumb", "wwfr_103" },
  { "gnr", "gnr_300" },
  { "twister", "twst_405" },
  { "southpk", "sprk_103" },
  { "truckstp", "trucksp2" },
  { "harley", "harl_a10" },
  { "swtril", "swtril43" },
  { "jplstwld", "jplstw22" },
  { "bnzai_p1", "bnzai_pa" },
  { "gs_l4", "gs_lu4" },
  { "gs_l3", "gs_la3" },
  { "kpv106", "kpb105" },
  { "mousn_lx", "mousn_l4" },
  { NULL, NULL }
};

static char alias_from_file[50];

static const char* crcOfGamesNotSupported[] = {
	NULL
};

const char* checkGameAlias(const char* aRomName) {

	char *ptr;
	char AliasFilename[MAX_PATH];

#ifndef _WIN64
	const HINSTANCE hInst = GetModuleHandle("VPinMAME.dll");
#else
	const HINSTANCE hInst = GetModuleHandle("VPinMAME64.dll");
#endif
	GetModuleFileName(hInst, AliasFilename, MAX_PATH);
	ptr = strrchr(AliasFilename, '\\');
	if (ptr != NULL)
	{
		strcpy_s(ptr + 1, 13, "VPMAlias.txt");

		FILE *f = fopen(AliasFilename, "r");

		if (f != NULL)
		{
			char line[128];
			while (fgets(line, sizeof(line), f) != NULL)
			{
				char *token = strtok(line, ", ");
				if (_stricmp(token, aRomName) == 0)
				{
					strcpy_s(alias_from_file, sizeof(alias_from_file), strtok(NULL, " ,\n#;'"));
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

bool checkGameNotSupported(const struct GameDriver* aGameDriver) {
  for (const char** ii = crcOfGamesNotSupported; *ii; ++ii)
    if (RomInSet(aGameDriver, *ii)) return true;
  return false;
}
