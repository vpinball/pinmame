#include "PinMAMEdll.h"

#include <stdio.h>
#include <conio.h>

void DisplayDMD();

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
			StopThreadedGame(true);

		if (c == 'l')
		//if(IsGameReady())
		{
			int* cl = new int[GetMaxLamps() * 2];
			int nb = GetChangedLamps(cl);
		}

		if (c == 'r')
			ResetGame();
	}
	
	StopThreadedGame(true);
}

unsigned char* rawDmd = nullptr;

void DisplayDMD()
{
	int w = GetRawDMDWidth();
	int h = GetRawDMDHeight();
	//printf("g_needs_DMD_update:%d\n", g_needs_DMD_update);

	if (w < 0 || h < 0)
		return;

	if (rawDmd == nullptr)
		rawDmd = new unsigned char[w*h];

	printf("osd_update_video_and_audio: %dx%d \n", w, h);
	int copied = GetRawDMDPixels(rawDmd);
	printf("copied %d\n", copied);
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < 100;i++)//g_raw_dmdx; i++)
			printf("%s", (rawDmd[j * w + i] > 20 ? "X" : " "));
		printf("\n");
	}
	//printf("\n");
}
