// Checks PinmameSetPath() / PinmameGetPath(): overrides replace the folders derived from vpmPath, survive
// PinmameSetConfig(), and can be removed again. Needs no ROMs. Optionally pass a ROM folder and a ROM name
// (e.g. "pinmame_test_paths /path/to/roms t2_l8") to also check that a game is found through the override.
//
// Exits with 0 when all checks pass.

#include <cstdio>
#include <cstring>
#include "libpinmame.h"

static int _failures = 0;

static void ExpectPath(const char* const label, const PINMAME_FILE_TYPE fileType, const char* const expected)
{
	const char* const actual = PinmameGetPath(fileType);
	const bool ok = (actual == nullptr && expected == nullptr)
		|| (actual != nullptr && expected != nullptr && strcmp(actual, expected) == 0);
	printf("%s %s: %s\n", ok ? "PASS" : "FAIL", label, actual ? actual : "(null)");
	if (!ok) {
		printf("     expected: %s\n", expected ? expected : "(null)");
		_failures++;
	}
}

static void ExpectStatus(const char* const label, const PINMAME_STATUS actual, const PINMAME_STATUS expected)
{
	const bool ok = actual == expected;
	printf("%s %s: status %d\n", ok ? "PASS" : "FAIL", label, (int)actual);
	if (!ok) {
		printf("     expected: status %d\n", (int)expected);
		_failures++;
	}
}

static void PINMAMECALLBACK OnGame(PinmameGame* p_game, void* const p_userData)
{
	*(int*)p_userData = p_game->found;
}

static void ExpectFound(const char* const label, const char* const romName, const int expected)
{
	int found = -1;
	const PINMAME_STATUS status = PinmameGetGame(romName, &OnGame, &found);
	const bool ok = status == PINMAME_STATUS_OK && found == expected;
	printf("%s %s: status %d, found %d\n", ok ? "PASS" : "FAIL", label, (int)status, found);
	if (!ok)
		_failures++;
}

static void SetConfig(const char* const vpmPath)
{
	PinmameConfig config = {
		PINMAME_AUDIO_FORMAT_FLOAT,
		44100,
		"",
	};
	snprintf((char*)config.vpmPath, PINMAME_MAX_PATH, "%s", vpmPath);
	PinmameSetConfig(&config);
}

int main(int argc, char** argv)
{
	// set before the first config
	ExpectStatus("set nvram before config", PinmameSetPath(PINMAME_FILE_TYPE_NVRAM, "early-nvram"), PINMAME_STATUS_OK);
	ExpectPath("nvram before config", PINMAME_FILE_TYPE_NVRAM, "early-nvram");

	SetConfig("base1/");
	ExpectPath("roms from vpmPath", PINMAME_FILE_TYPE_ROMS, "base1/roms");
	ExpectPath("samples from vpmPath", PINMAME_FILE_TYPE_SAMPLES, "base1/samples");
	ExpectPath("cfg from vpmPath", PINMAME_FILE_TYPE_CONFIG, "base1/cfg");
	ExpectPath("hi from vpmPath", PINMAME_FILE_TYPE_HIGHSCORE, "base1/hi");
	ExpectPath("nvram set before config survives", PINMAME_FILE_TYPE_NVRAM, "early-nvram");

	// override, then re-apply a different config
	ExpectStatus("set roms", PinmameSetPath(PINMAME_FILE_TYPE_ROMS, "roms-a;roms-b"), PINMAME_STATUS_OK);
	ExpectPath("roms override", PINMAME_FILE_TYPE_ROMS, "roms-a;roms-b");
	SetConfig("base2/");
	ExpectPath("roms override survives config", PINMAME_FILE_TYPE_ROMS, "roms-a;roms-b");
	ExpectPath("samples follow new vpmPath", PINMAME_FILE_TYPE_SAMPLES, "base2/samples");

	// remove overrides
	ExpectStatus("clear roms with \"\"", PinmameSetPath(PINMAME_FILE_TYPE_ROMS, ""), PINMAME_STATUS_OK);
	ExpectPath("roms back to vpmPath", PINMAME_FILE_TYPE_ROMS, "base2/roms");
	ExpectStatus("clear nvram with NULL", PinmameSetPath(PINMAME_FILE_TYPE_NVRAM, nullptr), PINMAME_STATUS_OK);
	ExpectPath("nvram back to vpmPath", PINMAME_FILE_TYPE_NVRAM, "base2/nvram");

	// invalid file types
	ExpectStatus("set unknown type", PinmameSetPath((PINMAME_FILE_TYPE)99, "x"), PINMAME_STATUS_FILE_TYPE_INVALID);
	ExpectStatus("set negative type", PinmameSetPath((PINMAME_FILE_TYPE)-1, "x"), PINMAME_STATUS_FILE_TYPE_INVALID);
	ExpectPath("get unknown type", (PINMAME_FILE_TYPE)99, nullptr);

	// ROM lookup goes through the override
	if (argc >= 3) {
		const char* const romFolder = argv[1];
		const char* const romName = argv[2];
		ExpectFound("rom not in vpmPath", romName, 0);
		PinmameSetPath(PINMAME_FILE_TYPE_ROMS, romFolder);
		ExpectFound("rom found through override", romName, 1);
		SetConfig("base3/");
		ExpectFound("rom still found after config", romName, 1);
		PinmameSetPath(PINMAME_FILE_TYPE_ROMS, nullptr);
		ExpectFound("rom gone after clearing", romName, 0);
	}

	printf("%s (%d failure%s)\n", _failures ? "FAILED" : "OK", _failures, _failures == 1 ? "" : "s");
	return _failures ? 1 : 0;
}
