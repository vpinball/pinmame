#include <string.h>
extern "C" {
  #include "driver.h"
  #include "audit.h"
}
#include "alias.h"

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
  { NULL, NULL }
};

static const char* crcOfGamesNotSupported[] = {
#ifndef TEST_NEW_STERN
	// Monopoly (1st sound rom, game roms 233, 251, 301, 303)
	CRC(f9bc55e8), CRC(f20a5ca6), CRC(0645cfae), CRC(24978872), CRC(4a66c9e4),

	// Playboy (1st sound rom, game roms 203, 300, 302, 303, 401, 500)
	CRC(f5502fec), CRC(50eb01b0), CRC(d7e5bada), CRC(206285ed), CRC(6a6f6aab), CRC(cb2e2824), CRC(e4d924ae),

	// Roller Coaster Tycoon (1st sound rom, game rom 400, 600, 701)
	CRC(18ba20ec), CRC(4691de23), CRC(2ada30e5), CRC(e1fe89f6),

	// The Simpsons Pinball Party (1st sound rom, game rom 204, 400)
	CRC(32efcdf6), CRC(5bc155f7), CRC(530b9782),

	// Terminator 3: Rise of the Machines (1st sound rom, game rom 200, 400)
	CRC(7f99e3af), CRC(b5cc4e24), CRC(872f9351),

	// Lord Of The Rings (1st sound rom en/sp, game roms 401, 700en, 700sp, 800en, 800sp)
	CRC(ba018c5c), CRC(980d970a), CRC(b69cdecc), CRC(208a4653), CRC(58d8611b), CRC(655e5b3c), CRC(38e83068),

	// Ripley's Believe It or Not! (1st sound rom, game rom 300)
	CRC(e5c7edea), CRC(8c4bf2a9),
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