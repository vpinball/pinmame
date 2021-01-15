#include "PinMAMEdll.h"

#include <stdio.h>
#include <conio.h>
#include <algorithm>

void DisplayDMD();
unsigned char* rawDMD = nullptr;

int main()
{
	SetVPMPath("C:/PinMAME/");
	SetSampleRate(48000);

	StartThreadedGame("taf_l7");

	//Sleep(1000);

	char c;
	while ((c =_getch()) != 'q')
	{
		if (c == 'd' || NeedsDMDUpdate())
			DisplayDMD();

		if (c == 's')
			StartThreadedGame("taf_l7", true);

		if (c == 'k')
		{
			StopThreadedGame(true);
			delete [] rawDMD;
			rawDMD = nullptr;
		}

		if (c == 'l')
		//if(IsGameReady())
		{
			int* cl = new int[GetMaxLamps() * 2];
			int nb = GetChangedLamps(cl);
			delete [] cl;

			cl = new int[GetMaxSolenoids() * 2];
			nb = GetChangedSolenoids(cl);
			delete [] cl;

			cl = new int[GetMaxGIStrings() * 2];
			nb = GetChangedGIs(cl);
			delete [] cl;
		}

		if (c == 'a')
		{
			float* ca = new float[2 * 10000];
			GetPendingAudioSamples(ca,2,10000);
			delete [] ca;
		}

		if (c == 'r')
			ResetGame();
	}
	
	StopThreadedGame(true);

	return 0;
}

void DisplayDMD()
{
	int w = GetRawDMDWidth();
	int h = GetRawDMDHeight();
	//printf("NeedsDMDUpdate(): %d\n", NeedsDMDUpdate());

	if (w < 0 || h < 0)
		return;

	if (rawDMD == nullptr)
		rawDMD = new unsigned char[w*h];

	printf("osd_update_video_and_audio: %dx%d\n", w, h);
	int copied = GetRawDMDPixels(rawDMD);
	printf("copied %d\n", copied);
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < std::min(100,w); i++)
			printf("%s", (rawDMD[j * w + i] > 20 ? "X" : " "));
		printf("\n");
	}
	//printf("\n");
}
