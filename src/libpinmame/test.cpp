#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <thread>
#include "libpinmame.h"

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define CLEAR_SCREEN "clear"
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;

void DumpVideo(int index, UINT8* p_displayData, PinmameDisplayLayout* p_displayLayout)
{
}

void DumpDmd(int index, UINT8* p_displayData, PinmameDisplayLayout* p_displayLayout)
{
	for (int y = 0; y < p_displayLayout->height; y++) {
		for (int x = 0; x < p_displayLayout->width; x++) {
			UINT8 value = p_displayData[y * p_displayLayout->width + x];
			
			if (p_displayLayout->depth == 2) {
				switch(value) {
					case 0:
						printf("░");
						break;
					case 1:
						printf("▒");
						break;
					case 2:
						printf("▓");
						break;
					case 3:
						printf("▓");
						break;
				}
			}
			else {
				switch(value) {
					case 0:
					case 1:
					case 2:
					case 3:
						printf("░");
						break;
					case 4:
					case 5:
					case 6:
					case 7:
						printf("▒");
						break;
					case 8:
					case 9:
					case 10:
					case 11:
						printf("▓");
						break;
					case 12:
					case 13:
					case 14:
					case 15:
						printf("▓");
						break;
				}
			}
		}

		printf("\n");
	}
}

void DumpAlphanumeric(int index, UINT16* p_displayData, PinmameDisplayLayout* p_displayLayout)
{
	char output[8][512] = {
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' },
		{ '\0' }
	};

	for (int pos = 0; pos < p_displayLayout->length; pos++) {
		const UINT16 value = *(p_displayData++);

		char segments_16c[8][10] = {
			{ " AAAAA   " },
			{ "FI J KB  " },
			{ "F IJK B  " },
			{ " GG LL   " },
			{ "E ONM C  " },
			{ "EO N MC P" }, 
			{ " DDDDD  H" },
			{ "       H " },
		};

		char segments_16s[8][10] = {
			{ " AA BB   " },
			{ "HI J KC  " },
			{ "H IJK C  " },
			{ " PP LL   " },
			{ "G ONM D  " },
			{ "GO N MD  " },
			{ " FF EE   " },
			{ "         " },
		};

		char (*segments)[10] = (p_displayLayout->type == PINMAME_DISPLAY_TYPE_SEG16S) ? segments_16s : segments_16c;

		for (int row = 0; row < 8; row++) {
			for (int column = 0; column < 9; column++) {
				for (UINT16 bit = 0; bit < 16; bit++) {
					if (segments[row][column] == ('A' + bit)) {
						segments[row][column] = (value & (1 << bit)) ? '*' : ' ';
						break;
					}
				}
			}

			strcat(output[row], segments[row]);
			strcat(output[row], " "); 
		}
	}

	for (int row = 0; row < 8; row++)
		printf("%s\n", output[row]);
}

void PINMAMECALLBACK Game(PinmameGame* game, void* const p_userData)
{
	printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s, flags=%lu, found=%d\n",
		game->name, game->description, game->manufacturer, game->year, (unsigned long)game->flags, game->found);
}

void PINMAMECALLBACK OnStateUpdated(int state, void* const p_userData)
{
	printf("OnStateUpdated(): state=%d\n", state);

	if (!state)
		exit(1);

	PinmameMechConfig mechConfig;
	memset(&mechConfig, 0, sizeof(mechConfig));

	mechConfig.sol1 = 11;
	mechConfig.length = 240;
	mechConfig.steps = 240;
	mechConfig.type = PINMAME_MECH_FLAGS_NONLINEAR | PINMAME_MECH_FLAGS_REVERSE | PINMAME_MECH_FLAGS_ONESOL;
	mechConfig.sw[0].swNo = 32;
	mechConfig.sw[0].startPos = 0;
	mechConfig.sw[0].endPos = 5;

	PinmameSetMech(1, &mechConfig);
}

void PINMAMECALLBACK OnDisplayAvailable(int index, int displayCount, PinmameDisplayLayout* p_displayLayout, void* const p_userData)
{
	printf("OnDisplayAvailable(): index=%d, displayCount=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		displayCount,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);
}

void PINMAMECALLBACK OnDisplayUpdated(int index, void* p_displayData, PinmameDisplayLayout* p_displayLayout, void* const p_userData)
{
	printf("OnDisplayUpdated(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
		index,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->depth,
		p_displayLayout->length);

	if (p_displayData) {
		if (p_displayLayout->type == PINMAME_DISPLAY_TYPE_VIDEO)
			DumpVideo(index, (UINT8*)p_displayData, p_displayLayout);
		else if ((p_displayLayout->type & PINMAME_DISPLAY_TYPE_DMD) == PINMAME_DISPLAY_TYPE_DMD)
			DumpDmd(index, (UINT8*)p_displayData, p_displayLayout);
		else
			DumpAlphanumeric(index, (UINT16*)p_displayData, p_displayLayout);
	}
}

int PINMAMECALLBACK OnAudioAvailable(PinmameAudioInfo* p_audioInfo, void* const p_userData)
{
	printf("OnAudioAvailable(): format=%d, channels=%d, sampleRate=%.2f, framesPerSecond=%.2f, samplesPerFrame=%d, bufferSize=%d\n",
		p_audioInfo->format,
		p_audioInfo->channels,
		p_audioInfo->sampleRate,
		p_audioInfo->framesPerSecond,
		p_audioInfo->samplesPerFrame,
		p_audioInfo->bufferSize);
	return p_audioInfo->samplesPerFrame;
}

int PINMAMECALLBACK OnAudioUpdated(void* p_buffer, int samples, void* const p_userData)
{
	return samples;
}

void PINMAMECALLBACK OnSolenoidUpdated(PinmameSolenoidState* p_solenoidState, void* const p_userData)
{
	printf("OnSolenoidUpdated: solenoid=%d, state=%d\n", p_solenoidState->solNo,  p_solenoidState->state);
}

void PINMAMECALLBACK OnMechAvailable(int mechNo, PinmameMechInfo* p_mechInfo, void* const p_userData)
{
	printf("OnMechAvailable: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void PINMAMECALLBACK OnMechUpdated(int mechNo, PinmameMechInfo* p_mechInfo, void* const p_userData)
{
	printf("OnMechUpdated: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
		mechNo,
		p_mechInfo->type,
		p_mechInfo->length,
		p_mechInfo->steps,
		p_mechInfo->pos,
		p_mechInfo->speed);
}

void PINMAMECALLBACK OnConsoleDataUpdated(void* p_data, int size, void* const p_userData)
{
	printf("OnConsoleDataUpdated: size=%d\n", size);
}

int PINMAMECALLBACK IsKeyPressed(PINMAME_KEYCODE keycode, void* const p_userData)
{
	return 0;
}

void PINMAMECALLBACK OnLogMessage(PINMAME_LOG_LEVEL logLevel, const char* format, va_list args, void* const p_userData)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, args);

	if (logLevel == PINMAME_LOG_LEVEL_INFO)
		printf("INFO: %s\n", buffer);
	else if (logLevel == PINMAME_LOG_LEVEL_ERROR)
		printf("ERROR: %s\n", buffer);
}

void PINMAMECALLBACK OnSoundCommand(int boardNo, int cmd, void* const p_userData)
{
	printf("OnSoundCommand: boardNo=%d, cmd=%d\n", boardNo, cmd);
}

int main(int, char**)
{
	system(CLEAR_SCREEN);

	PinmameConfig config = {
		PINMAME_AUDIO_FORMAT_FLOAT,
		44100,
		"",
		&OnStateUpdated,
		&OnDisplayAvailable,
		&OnDisplayUpdated,
		&OnAudioAvailable,
		&OnAudioUpdated,
		&OnMechAvailable,
		&OnMechUpdated,
		&OnSolenoidUpdated,
		&OnConsoleDataUpdated,
		&IsKeyPressed,
		&OnLogMessage,
		&OnSoundCommand,
	};

	#if defined(_WIN32) || defined(_WIN64)
		snprintf((char*)config.vpmPath, PINMAME_MAX_PATH, "%s%s\\pinmame\\", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
	#else
		snprintf((char*)config.vpmPath, PINMAME_MAX_PATH, "%s/.pinmame/", getenv("HOME"));
	#endif

	PinmameSetConfig(&config);

	PinmameSetCheat(0);
	PinmameSetHandleKeyboard(0);
	PinmameSetHandleMechanics(0);

	PinmameSetDmdMode(PINMAME_DMD_MODE_RAW);

	PinmameGetGames(&Game, NULL);
	PinmameGetGame("fourx4", &Game, NULL);

	//PinmameRun("mm_109c");
	//PinmameRun("fh_906h");
	//PinmameRun("hh7");
	//PinmameRun("rescu911");
	//PinmameRun("tf_180h");
	//PinmameRun("flashgdn");
	//PinmameRun("fourx4");
	//PinmameRun("ripleys");
	//PinmameRun("fh_l9");
	//PinmameRun("acd_170hc");
	//PinmameRun("snspares");
	//PinmameRun("gnr_300");
	//PinmameRun("babypac");
	//PinmameRun("t2_l8");
	//PinmameRun("galaxy");
	//PinmameRun("medusa");

	if (PinmameRun("motrshow") == PINMAME_STATUS_OK) {
		while (true)
			std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	return 0;
}
