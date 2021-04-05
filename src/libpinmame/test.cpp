#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <thread>
#include "libpinmame.h"

#if defined(_WIN32) || defined(_WIN64)
#define CLEAR_SCREEN "cls"
#define VPM_PATH "C:\\.pinmame\\"
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define CLEAR_SCREEN "clear"
#define VPM_PATH "~/.pinmame/"
#endif

typedef unsigned short UINT16;
static void* _p_displayBuffer = nullptr;

void DumpDmd(unsigned char* p_displayBuffer, PinmameDisplayLayout* p_displayLayout) {
	for (int y = 0; y < p_displayLayout->height; y++) {
		for (int x = 0; x < p_displayLayout->width; x++) {
			switch (p_displayBuffer[y * p_displayLayout->width + x]) {
				case 0x00:
					printf(" ");
					break;
				case 0x14:
					printf("░");
					break;
				case 0x21:
					printf("▒");
					break;
				case 0x43:
					printf("▓");
					break;
				case 0x64:
					printf("▓");
					break;
			}
		}

		printf("\n");
	}
}

void DumpAlphanumeric(UINT16* p_displayBuffer, PinmameDisplayLayout* p_displayLayout) {
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

	for (int index = 0; index < p_displayLayout->length; index++) {
		const UINT16 value = *(p_displayBuffer++);

		char segments[8][10] = {
			{ " AAAAA   " },
			{ "FI J KB  " },
			{ "F IJK B  " },
			{ " GG LL   " },
			{ "E ONM C  " },
			{ "EO N MC P" }, 
			{ " DDDDD  H" },
			{ "       H " },
		};

		for (int row = 0; row < 8; row++) {
			for (int column = 0; column < 9; column++) {
				for (UINT16 bit = 0; bit < 16; bit++) {
					if (segments[row][column] == ('A' + bit)) {
						segments[row][column] = (value & (1 << bit)) ? '*' : ' ';
					}
					break;
				}
			}

			strcat(output[row], segments[row]);
			strcat(output[row], " "); 
		}
	}

	for (int row = 0; row < 8; row++) {
		printf("%s\n", output[row]);
	}
}

void CALLBACK Game(PinmameGame* game) {
	printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s\n",
		game->name, game->description, game->manufacturer, game->year);
}

void CALLBACK DisplayLayout(int index, PinmameDisplayLayout* p_displayLayout) {
	printf("DisplayLayout(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, length=%d\n", 
		index,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->length);
}

void CALLBACK Display(int index, PinmameDisplayLayout* p_displayLayout) {
	printf("Display(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, length=%d\n",
		index,
		p_displayLayout->type,
		p_displayLayout->top,
		p_displayLayout->left,
		p_displayLayout->width,
		p_displayLayout->height,
		p_displayLayout->length);

	if (p_displayLayout->type == DMD) {
		DumpDmd((unsigned char*)_p_displayBuffer, p_displayLayout);
	}
	else {
		DumpAlphanumeric((UINT16*)_p_displayBuffer, p_displayLayout);
	}
}

void CALLBACK OnStateChange(int state) {
	if (state) {
		PinmameGetDisplayLayouts(&DisplayLayout);

		_p_displayBuffer = malloc(256 * 64);
	}
	else {
		if (_p_displayBuffer) {
			free(_p_displayBuffer);
		}

		exit(1);
	}
}

void CALLBACK OnSolenoid(int solenoid, int isActive) {
	printf("OnSolenoid: solenoid=%d, isActive=%d\n", solenoid, isActive);
}

int main(int, char**) {
	system(CLEAR_SCREEN);

	PinmameGetGames(&Game);

	PinmameConfig config = {
		48000,
		VPM_PATH,
		&OnStateChange,
		&OnSolenoid
	};

	PinmameSetConfig(&config);

	//PinmameRun("mm_109c");
	//PinmameRun("fh_906h");
	//PinmameRun("hh7");
	//PinmameRun("rescu911");

	if (PinmameRun("mm_109c") != OK) {
		exit(1);
	}

	while (1) {
		if (_p_displayBuffer) {
			PinmameGetDisplays(_p_displayBuffer, &Display);
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	return 0;
}
