#include <string.h>
extern "C" {
  #include "driver.h"
  #include "audit.h"
}
#include "alias.h"

static const struct tAliasTable { const char* alias; const char* real; } aliasTable[] = {
  { "eshak_f1", "esha_pr4" },
  { "eshak_l1", "esha_la1" },
  { NULL, NULL }
};

static const char* crcOfGamesNotSupported[] = {
#ifndef TEST_NEW_STERN
	// Austin Powers (1st sound rom, game roms 201, 300, 301, 302)
	CRC(c1e33fee), CRC(a06b2b03), CRC(a06b2b03), CRC(a4ddcdca), CRC(2920b59b),

	// Monopoly (1st sound rom, game roms 233, 251, 301, 303)
	CRC(f9bc55e8), CRC(f20a5ca6), CRC(0645cfae), CRC(24978872), CRC(4a66c9e4),

	// Playboy (1st sound rom, game roms 203, 300, 302, 303, 401, 500)
	CRC(f5502fec), CRC(50eb01b0), CRC(d7e5bada), CRC(206285ed), CRC(6a6f6aab), CRC(cb2e2824), CRC(e4d924ae),

	// Roller Coaster Tycoon (1st sound rom, game rom 400, 600, 701)
	CRC(18ba20ec), CRC(4691de23), CRC(2ada30e5), CRC(e1fe89f6),

	// The Simpsons Pinball Party (1st sound rom, game rom 204)
	CRC(32efcdf6), CRC(5bc155f7),

	// Terminator 3: Rise of the Machines (1st sound rom, game rom 200)
	CRC(7f99e3af), CRC(b5cc4e24),
#endif // TEST_NEW_STERN
	// end of the list
	NULL
};

const char* checkGameAlias(const char* aRomName) {
  for (const tAliasTable* ii = aliasTable; ii->alias; ++ii)
    if (stricmp(aRomName, ii->alias) == 0) return ii->real;
  return aRomName;
}

bool checkGameNotSupported(const struct GameDriver* aGameDriver) {
  for (const char** ii = crcOfGamesNotSupported; *ii; ++ii)
    if (RomInSet(aGameDriver, *ii)) return true;
  return false;
}