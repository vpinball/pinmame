#include <windows.h>
//#include <time.h>
#include "pindmd.h"
#include "usbalphanumeric.h"
#include "driver.h"
#include "pinddrv.h"
#include "gen.h"

UINT8			dmdHeight;
UINT8			dmdWidth;
UINT8			frame_buf[3072];
UINT16			seg_data_old[50];
UINT16			extra_seg_data[50];
UINT8			hasExtraData;
FILE			*fDumpFrames;
UINT8			logobuf[32][128];
UINT8			dmdInUse;
UINT8			clkDiv = 0;
int				frameDelay = 0; // 30fps
UINT8			dump_4_map[4]	= {'0','A','K','V'};
UINT8			dump_16_map[16] = {'0','3','5','7','9','B','D','F','H','J','L','N','P','R','T','V'};

#define GEN_LOGO       U64(0xfffffffffff)
 
//*****************************************************
//* Name:			pindmdInit
//* Purpose:	initialize ftdi driver
//* In:
//* Out:
//*****************************************************
void pindmdInit(void)
{
	pinddrvInit();
	hasExtraData=0;
	dmdInUse=0;

	memset(seg_data_old,0,50*sizeof(UINT16));
	// clear dmd frame buf
	memset(frame_buf,0,sizeof(frame_buf));
	// send dmdlogo.txt to pinDMD
	//sendLogo();
}

//*****************************************************
//* Name:			pindmdDeInit
//* Purpose:
//* In:
//* Out:
//*****************************************************
void pindmdDeInit(void)
{
	sendLogo();
	Sleep(100);
	pinddrvDeInit();
}

//*****************************************************
//* Name:			sendLogo
//* Purpose:	initialize ftdi driver
//* In:
//* Out:
//*****************************************************                             
void sendLogo(void)
{
	FILE *fLogo;
	UINT8 i,j;

	// display dmd logo from text file if it exists
	fLogo = fopen("dmdlogo.txt","r");
	if(fLogo){
		for(i=0; i<32; i++){
			for(j=0; j<128; j++)
			{
				UINT8 fileChar = getc(fLogo);
				//Read next char after enter (beginning of next line)
				while(fileChar == 10)
					fileChar = getc(fLogo);
				if(do16 == 0)
				{
					logobuf[i][j] = fileChar - '0';
					if(logobuf[i][j] > 3)
						logobuf[i][j] = 0;
				}
				if(do16 == 1)
					switch(fileChar)
					{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							logobuf[i][j] = fileChar - '0';
							break;

						case 'a':
						case 'A':
							logobuf[i][j] = 10;
							break;
						
						case 'b':
						case 'B':
							logobuf[i][j] = 11;
							break;

						case 'c':
						case 'C':
							logobuf[i][j] = 12;
							break;

						case 'd':
						case 'D':
							logobuf[i][j] = 13;
							break;

						case 'e':
						case 'E':
							logobuf[i][j] = 14;
							break;

						case 'f':
						case 'F':
							logobuf[i][j] = 15;
							break;

						default:
							logobuf[i][j] = 0;
							break;
				}
			}
		}
		dmdInUse=0;
		fclose(fLogo);
	} else 
		memset(logobuf,0,4096);

	renderDMDFrame(GEN_LOGO,128,32,*logobuf,0);
}
	
//*****************************************************
//* Name:			renderDMDFrame
//* Purpose:
//* In:
//* Out:
//*****************************************************
void renderDMDFrame(UINT64 gen, UINT8 width, UINT8 height, UINT8 *currbuffer, UINT8 doDumpFrame)
{
	int byteIdx=4;
	int bd0,bd1,bd2,bd3;
	int pixel;
	int i,j,v;

	if(enabled==0)
		return;

	//if((gen == GEN_SAM) && (doOther == 0))
	//	return;

	// dont update dmd if segments have changed
	//if(memcmp(currbuffer, oldbuffer, 4096)==0)
	//	return;
	
	// clear dmd frame buf first time
	if(dmdInUse == 0)
		memset(frame_buf,0,sizeof(frame_buf));

	dmdInUse=1;

	frame_buf[0] = 0x81;	// frame sync bytes
	frame_buf[1] = 0xC3;
	frame_buf[2] = 0xE7;
	frame_buf[3] = 0x0;		// command byte

	// 128x16 = display centered vert
	// 128x32 = no change
	// 192x64 = display vert scaled 50%, display horz centered

	// dmd height
	for(j = 0; j < height; j+=((height==64)?2:1)) {
		// dmd width
		for(i = 0; i < width; i+=((width==192)?12:8)) {
			bd0 = 0;
			bd1 = 0;
			bd2 = 0;
			bd3 = 0;
			for (v = 7; v >= 0; v--) {
				UINT8 x = 0;
				if(width == 192)
					x = (v+1)/2;
				// pixel colour
				pixel = currbuffer[(j*((width==128)?128:192)) + (i+v+x)];
				// Some systems add 63 to pixel hue, lets delete 63 before rendering the frame for those systems
				if((gen == GEN_SAM) || (gen == GEN_GTS3) || (gen == GEN_ALVG)) //!! GEN_ALVG should be further separated into (LED vs DMD)
					pixel -= 63;
				// 16 color mode hue remapping for proper gradient
				if(do16 == 1)
				{
					// For 4 color games, map hues to show proper hue in 16 color mode
					if((gen != GEN_SAM) && (gen != GEN_GTS3) && (gen != GEN_ALVG) && (gen != GEN_LOGO)) //!! GEN_ALVG should be further separated into (LED vs DMD)
					{
						if(pixel==3)
							pixel=15;	
						if(pixel==2)
							pixel=4;
						if(pixel==1)
							pixel=1;
					}
					if((gen == GEN_GTS3) || (gen == GEN_ALVG)) //!! GEN_ALVG should be further separated into (LED vs DMD)
					{
						if(pixel==3)
							pixel=1;
						if(pixel==6)
							pixel=4;
						if(pixel==9)
							pixel=4;
						if(pixel==15)
							pixel=15;
					}
				}
				else if((gen == GEN_GTS3) || (gen == GEN_ALVG)) //!! GEN_ALVG should be further separated into (LED vs DMD)
				{
						if(pixel==3)
							pixel=1;
						if(pixel==6)
							pixel=2;
						if(pixel==9)
							pixel=2;
						if(pixel==15)
							pixel=3;
				}
				bd0 <<= 1;
				bd1 <<= 1;
				bd2 <<= 1;
				bd3 <<= 1;
				if(pixel & 1)
					bd0 |= 1;
				if(pixel & 2)
					bd1 |= 1;
				if(pixel & 4)
					bd2 |= 1;
				if(pixel & 8)
					bd3 |= 1;
			}
			frame_buf[byteIdx+((height==16)?128:0)] = bd0;
			frame_buf[byteIdx+512+((height==16)?128:0)] = bd1;
			frame_buf[byteIdx+1024+((height==16)?128:0)] = bd2;
			frame_buf[byteIdx+1536+((height==16)?128:0)] = bd3;
			byteIdx++;
		}
	}

	pinddrvSendFrame();
	dumpFrames(currbuffer, width*height, doDumpFrame, gen);
}

//*****************************************************
//* Name:			renderAlphanumericFrame
//* Purpose:	
//* In:
//* Out:
//*****************************************************
void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, UINT8 total_disp, UINT8 *disp_lens)
{
	if((enabled==0) || dmdInUse)
		return;
	// dont update dmd if segments have changed
	if(memcmp(seg_data,seg_data_old,50*sizeof(UINT16))==0)
		return;

	// Medusa fix
	if((gen == GEN_BY35) && (disp_lens[0] == 2))
	{
		memcpy(extra_seg_data,seg_data,50*sizeof(UINT16));
		hasExtraData = 1;
		return;
	}

	// clear dmd frame buf
	memset(frame_buf,0,sizeof(frame_buf));
	// create frame header
	frame_buf[0] = 0x81;	// frame sync bytes
	frame_buf[1] = 0xC3;
	frame_buf[2] = 0xE7;
	frame_buf[3] = 0x0;		// command byte
	
	// switch to current game tech
	switch(gen){
		// williams
			break;
		case GEN_S3:
		case GEN_S3C:
		case GEN_S4:
		case GEN_S6:
			_2x6Num_2x6Num_4x1Num(seg_data);
			break;
		case GEN_S7:
			//_2x7Num_4x1Num_1x16Alpha(seg_data);		hmm i did this for a reason??
			_2x7Num_2x7Num_4x1Num_gen7(seg_data);
			break;
		case GEN_S9:
			_2x7Num10_2x7Num10_4x1Num(seg_data);
			break;

		// williams
		case GEN_WPCALPHA_1:
		case GEN_WPCALPHA_2:
		case GEN_S11C:
		case GEN_S11B2:
			_2x16Alpha(seg_data);
			break;
		case GEN_S11:
			_6x4Num_4x1Num(seg_data);
			break;
		case GEN_S11X:
			switch(total_disp){
				case 2:
					_2x16Alpha(seg_data);
					break;
				case 3:
					_1x16Alpha_1x16Num_1x7Num(seg_data);
					break;
				case 4:
					_2x7Alpha_2x7Num(seg_data);
					break;
				case 8:
					_2x7Alpha_2x7Num_4x1Num(seg_data);
					break;
			}
			break;

		// dataeast
		case GEN_DE:
			switch(total_disp){
				case 2:
					_2x16Alpha(seg_data);
					break;
				case 4:
					_2x7Alpha_2x7Num(seg_data);
					break;
				case 8:
					_2x7Alpha_2x7Num_4x1Num(seg_data);
					break;
			}
			break;

		// gottlieb
		case GEN_GTS1:
		case GEN_GTS80:
			switch(disp_lens[0]){
				case 6:
					_2x6Num10_2x6Num10_4x1Num(seg_data);
					break;
				case 7:
					_2x7Num10_2x7Num10_4x1Num(seg_data);
					break;
			}
			break;
		case GEN_GTS80B:
			break;
		case GEN_GTS3:
			// 2 many digits :(
			_2x20Alpha(seg_data);
			break;

		// stern
		case GEN_STMPU100:
		case GEN_STMPU200:
			switch(disp_lens[0]){
				case 6:
					_2x6Num_2x6Num_4x1Num(seg_data);
					break;
				case 7:
					_2x7Num_2x7Num_4x1Num(seg_data);
					break;
			}
			break;
		// bally
		case GEN_BY17:
		case GEN_BY35:
			// check for   total:8 = 6x6num + 4x1num
			if(total_disp==8){

			}else{
				switch(disp_lens[0]){
					case 6:
						_2x6Num_2x6Num_4x1Num(seg_data);
						break;
					case 7:
						if(hasExtraData)
							_2x7Num_2x7Num_10x1Num(seg_data, extra_seg_data);
						else
							_2x7Num_2x7Num_4x1Num(seg_data);
						break;
				}
			}
			break;
		case GEN_BY6803:
		case GEN_BY6803A:
			_4x7Num10(seg_data);
			break;
		case GEN_BYPROTO:
			_2x6Num_2x6Num_4x1Num(seg_data);
			break;
		// astro
		case GEN_ASTRO:
			break;
		// hankin
		case GEN_HNK:
			break;
		case GEN_BOWLING:
			break;
		// zaccaria
		case GEN_ZAC1:
			break;
		case GEN_ZAC2:
			break;
	}

	pinddrvSendFrame();
	memcpy(seg_data_old,seg_data,50*sizeof(UINT16));
}

//*****************************************************
//* Name:			getPixel
//* Purpose:
//* In:
//* Out:
//*****************************************************
UINT8 getPixel(int x, int y)
{
   int v,z;
   v = (y*16)+(x/8);
   z = 1<<(x%8);
   // just check high buff

	return ((frame_buf[v+512+4]&z)!=0);
}

//*****************************************************
//* Name:			drawPixel
//* Purpose:
//* In:
//* Out:
//*****************************************************
void drawPixel(int x, int y, UINT8 colour)
{
   int v,z;
   v = (y*16)+(x/8);
   z = 1<<(x%8);
   // clear both low and high buffer pixel
   frame_buf[v+4] |= z;
   frame_buf[v+512+4] |= z;
   frame_buf[v+4] ^= z;
   frame_buf[v+512+4] ^= z;
	 if(do16==1){
		frame_buf[v+1024+4] |= z;
		frame_buf[v+1536+4] |= z;
		frame_buf[v+1024+4] ^= z;
		frame_buf[v+1536+4] ^= z;
	}
   // set low buffer pixel
   if(colour & 1)
      frame_buf[v+4] |= z;
   //set high buffer pixel
   if(colour & 2)
      frame_buf[v+512+4] ^= z;
	 // 16 colour mode
	 if(do16==1){
		if(colour!=0){
			frame_buf[v+1024+4] |= z;
			frame_buf[v+1536+4] ^= z;
		}
	}
}

//*****************************************************
//* Name:			dumpFrame
//* Purpose:
//* In:
//* Out:
//*****************************************************
void dumpFrames(UINT8 *currbuffer, UINT16 buffersize, UINT8 doDumpFrame, UINT64 gen)
{
	// Dump Frames disabled, close file if open, and return
	if(doDumpFrame == 0)
	{
		if(fDumpFrames)
		{
			fclose(fDumpFrames);
			fDumpFrames = NULL;
		}
		return;
	}

	// Dump Frames enabled
	// If file doesnt exist, create new file
	if(!fDumpFrames)
	{
		time_t now;
		char filename[35];
		filename[0] = '\0';
		now = time(NULL);

		if (now != -1)
		{
			strftime(filename, sizeof(filename), "%d%m%y_%H%M%S_dump.txt", gmtime(&now));
		}

		fDumpFrames = fopen(filename,"w");
	}

	// Add currbuffer to file
	if(fDumpFrames)
	{
		int x;
		for(x = 0; x < buffersize; x++)
		{
			if((gen == GEN_SAM) || (gen == GEN_GTS3) || (gen == GEN_ALVG)) //!! GEN_ALVG should be further separated into (LED vs DMD)
				fprintf(fDumpFrames,"%c",dump_16_map[currbuffer[x]-63]);
			else
				fprintf(fDumpFrames,"%c",dump_4_map[currbuffer[x]]);
		}
		fprintf(fDumpFrames,"\n%d\n",frameDelay);
		frameDelay=0;
	}
}

//*****************************************************
//* Name:			frameClock
//* Purpose:	clk from core running at 60fps
//* In:
//* Out:
//*****************************************************
void frameClock(void)
{
	clkDiv++;
	if(clkDiv>1){
		clkDiv=0;
		frameDelay++;
	}
}
