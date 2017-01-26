#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\..\dmddevice.h"

bool do16 = false;

#include "..\..\usbalphanumeric.h"

#include "..\ftdi\ftd2xx.h"

FT_STATUS ftStatus;
FT_HANDLE ftHandle;

bool isOpen = false;
static UINT8 oldbuffer[16384] = {};
static UINT16 seg_data_old[50] = {};

//bool doOther;
bool slowUSB = false;


void Send_Clear_Screen(void)
{
	memset(OutputPacketBuffer,0x00, 2052);
	const UINT8 tmp[4] = {
		0x81, 0xC3, 0xE7, 0x00 //header
		}; 
	memcpy(OutputPacketBuffer, tmp, sizeof(tmp));
	DWORD bytes;
	// send dmd frame buffer to pindmd board
	ftStatus = FT_Write(ftHandle, &OutputPacketBuffer, do16 ? (DWORD)2052:(DWORD)1028, &bytes);
	Sleep(50);
}

DMDDEV int Open()
{
	// create the device information list
	DWORD numDevs;
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	isOpen = false;

	// ftdi devices found
	int deviceId = -1;
	if (numDevs > 0) {
		// allocate storage for list based on numDevs
		FT_DEVICE_LIST_INFO_NODE *devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
		// info request successful
		if (ftStatus == FT_OK) {
			for (DWORD i = 0; i < numDevs; i++) {
				// search for pindmd board serial number
				if((strcmp(devInfo[i].SerialNumber,"DMD1000")==0) || (strcmp(devInfo[i].SerialNumber,"DMD1001")==0) || (strcmp(devInfo[i].SerialNumber,"DMD2000A")==0)){
					// assign device id (incase other ftdi devices are connected)
					deviceId = i;
					isOpen = true;
				}
				slowUSB = (strcmp(devInfo[i].SerialNumber,"DMD1001")==0);
			}
		}
	}

	if(numDevs == 0 || deviceId == -1)
	{
		MessageBox(NULL, "pinDMD v1 not found","Error", MB_ICONERROR);
		return 0;
	}

	// get handle on device
	ftStatus = FT_Open(deviceId, &ftHandle);
	if(ftStatus != FT_OK){
		// FT_Open failed return;
		isOpen=false;
		return 0;
	}
	
	// check pinDMD firmware to see if its 16 colour (bit4=true)
	FT_SetBitMode(ftHandle, 0x0, 0x20);
	UINT8 BitMode;
	ftStatus = FT_GetBitMode(ftHandle, &BitMode);
	if (ftStatus == FT_OK) {
		// BitMode contains current value
		do16 = ((BitMode&0x08)==0x08);
		//doOther = ((BitMode&0x04)==0x04);
	}

	// set Asynchronous Bit Bang Mode
	FT_SetBitMode(ftHandle, 0xff, 0x1);
	// set Baud
	FT_SetBaudRate(ftHandle, slowUSB ? 11000:12000);  // Actually 10400 * 16

	return 1;
}


DMDDEV bool Close()
{
	if (isOpen) {
		Send_Clear_Screen();

		// have to reset bitbangmode or the ftdi chip will flood the serial with '[00]'
		FT_SetBitMode(ftHandle, 0x00, 0x0);
		FT_Close(ftHandle);

		free(OutputPacketBuffer);
	}

	isOpen = false;
	return true;
}


DMDDEV void PM_GameSettings(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options)
{
}


DMDDEV void Render_4_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer) 
{
	if (!memcmp(oldbuffer, currbuffer, width*height)) //check if same frame again
		return;

	memcpy(oldbuffer, currbuffer, width*height);

	if (isOpen) {
		int byteIdx=4;
		UINT8 tempbuffer[128*32]; // for rescale

		OutputPacketBuffer[0] = 0x81;	// frame sync bytes
		OutputPacketBuffer[1] = 0xC3;
		OutputPacketBuffer[2] = 0xE7;
		OutputPacketBuffer[3] = 0x0;	// command byte

		// 128x16 = display centered vert
		// 128x32 = no change
		// 192x64 = rescaled
		// 256x64 = rescaled

		if(width == 192 && height == 64)
		{
			UINT32 o = 0;
			for(int j = 0; j < 32; ++j)
				for(int i = 0; i < 128; ++i,++o)
				{
					const UINT32 offs = j*(2*192)+i*3/2;
					if((i&1) == 1) // filter only each 2nd pixel, could do better than this
						tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+192] + (int)currbuffer[offs+1] + (int)currbuffer[offs+193])/4);
					else
						tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+192])/2);
				}
		}
		else if(width == 256 && height == 64)
		{
			UINT32 o = 0;
			for(int j = 0; j < 32; ++j)
				for(int i = 0; i < 128; ++i,++o)
				{
					const UINT32 offs = j*(2*256)+i*2;
					tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+256] + (int)currbuffer[offs+1] + (int)currbuffer[offs+257])/4);
				}
		} else
			memcpy(tempbuffer,currbuffer,width*height);
	

		// dmd height
		for(int j = 0; j < ((height==16)?16:32); ++j)
		{
			// dmd width
			for(int i = 0; i < 128; i+=8)
			{
				int bd0,bd1,bd2,bd3;
				bd0 = 0;
				bd1 = 0;
				bd2 = 0;
				bd3 = 0;
				for (int v = 7; v >= 0; v--)
				{
					// pixel colour
					int pixel = tempbuffer[j*128 + i+v];

					bd0 <<= 1;
					bd1 <<= 1;
					bd2 <<= 1;
					bd3 <<= 1;
					if(do16)
					{
						if(pixel==3)
							pixel=15;	
						else if(pixel==2)
							pixel=4;
					}
					if(pixel & 1)
						bd0 |= 1;
					if(pixel & 2)
						bd1 |= 1;
					if(pixel & 4)
						bd2 |= 1;
					if(pixel & 8)
						bd3 |= 1;
				}

				OutputPacketBuffer[byteIdx     +((height==16)?128:0)] = bd0;
				OutputPacketBuffer[byteIdx+ 512+((height==16)?128:0)] = bd1;
				OutputPacketBuffer[byteIdx+1024+((height==16)?128:0)] = bd2;
				OutputPacketBuffer[byteIdx+1536+((height==16)?128:0)] = bd3;
				byteIdx++;
			}
		}

		DWORD bytes;
		// send dmd frame buffer to pindmd board
		ftStatus = FT_Write(ftHandle, &OutputPacketBuffer, do16 ? (DWORD)2052:(DWORD)1028, &bytes);
	}
}

DMDDEV void Render_16_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer) 
{
	if (!memcmp(oldbuffer, currbuffer, width*height)) //check if same frame again
		return;

	memcpy(oldbuffer, currbuffer, width*height);

	if (isOpen) {
		int byteIdx=4;
		UINT8 tempbuffer[128*32]; // for rescale

		OutputPacketBuffer[0] = 0x81;	// frame sync bytes
		OutputPacketBuffer[1] = 0xC3;
		OutputPacketBuffer[2] = 0xE7;
		OutputPacketBuffer[3] = 0x0;		// command byte

		// 128x16 = display centered vert
		// 128x32 = no change
		// 192x64 = rescaled
		// 256x64 = rescaled

		if(width == 192 && height == 64)
		{
			UINT32 o = 0;
			for(int j = 0; j < 32; ++j)
				for(int i = 0; i < 128; ++i,++o)
				{
					const UINT32 offs = j*(2*192)+i*3/2;
					if((i&1) == 1) // filter only each 2nd pixel, could do better than this
						tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+192] + (int)currbuffer[offs+1] + (int)currbuffer[offs+193])/4);
					else
						tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+192])/2);
				}
		}
		else if(width == 256 && height == 64)
		{
			UINT32 o = 0;
			for(int j = 0; j < 32; ++j)
				for(int i = 0; i < 128; ++i,++o)
				{
					const UINT32 offs = j*(2*256)+i*2;
					tempbuffer[o] = (UINT8)(((int)currbuffer[offs] + (int)currbuffer[offs+256] + (int)currbuffer[offs+1] + (int)currbuffer[offs+257])/4);
				}
		} else
			memcpy(tempbuffer,currbuffer,width*height);


		// dmd height
		for(int j = 0; j < ((height==16)?16:32); ++j)
		{
			// dmd width
			for(int i = 0; i < 128; i+=8)
			{
				int bd0,bd1,bd2,bd3;
				bd0 = 0;
				bd1 = 0;
				bd2 = 0;
				bd3 = 0;
				for (int v = 7; v >= 0; v--)
				{
					// pixel colour
					int pixel = tempbuffer[j*128 + i+v];

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

				OutputPacketBuffer[byteIdx     +((height==16)?128:0)] = bd0;
				OutputPacketBuffer[byteIdx+ 512+((height==16)?128:0)] = bd1;
				OutputPacketBuffer[byteIdx+1024+((height==16)?128:0)] = bd2;
				OutputPacketBuffer[byteIdx+1536+((height==16)?128:0)] = bd3;
				byteIdx++;
			}
		}

		DWORD bytes;
		// send dmd frame buffer to pindmd board
		ftStatus = FT_Write(ftHandle, &OutputPacketBuffer, do16 ? (DWORD)2052:(DWORD)1028, &bytes);	}
}


DMDDEV void Render_PM_Alphanumeric_Frame(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2) 
{
	if (!memcmp(seg_data, seg_data_old, 50 * sizeof(UINT16)))
		return;

	memcpy(seg_data_old, seg_data, 50 * sizeof(UINT16));

	if (isOpen) {	
		memset(AlphaNumericFrameBuffer,0x00,2048);

		OutputPacketBuffer[0] = 0x81;	// frame sync bytes
		OutputPacketBuffer[1] = 0xC3;
		OutputPacketBuffer[2] = 0xE7;
		OutputPacketBuffer[3] = 0x0;

		switch (layout) {
			case __2x16Alpha :
				_2x16Alpha(seg_data);
				break;
			case __2x20Alpha :
				_2x20Alpha(seg_data);
				break;
			case __2x7Alpha_2x7Num :
				_2x7Alpha_2x7Num(seg_data);
				break;
			case __2x7Alpha_2x7Num_4x1Num :
				_2x7Alpha_2x7Num_4x1Num(seg_data);
				break;
			case __2x7Num_2x7Num_4x1Num :
				_2x7Num_2x7Num_4x1Num(seg_data);
				break;
			case __2x7Num_2x7Num_10x1Num :
				_2x7Num_2x7Num_10x1Num(seg_data,seg_data2);
				break;
			case __2x7Num_2x7Num_4x1Num_gen7 :
				_2x7Num_2x7Num_4x1Num_gen7(seg_data);
				break;
			case __2x7Num10_2x7Num10_4x1Num :
				_2x7Num10_2x7Num10_4x1Num(seg_data);
				break;
			case __2x6Num_2x6Num_4x1Num :
				_2x6Num_2x6Num_4x1Num(seg_data);
				break;
			case __2x6Num10_2x6Num10_4x1Num :
				_2x6Num10_2x6Num10_4x1Num(seg_data);
				break;
			case __4x7Num10 :
				_4x7Num10(seg_data);
				break;
			case __6x4Num_4x1Num :
				_6x4Num_4x1Num(seg_data);
				break;
			case __2x7Num_4x1Num_1x16Alpha :
				_2x7Num_4x1Num_1x16Alpha(seg_data);
				break;
			case __1x16Alpha_1x16Num_1x7Num :
				_1x16Alpha_1x16Num_1x7Num(seg_data);
				break;
			default:
				break;
		}

		//if (!memcmp(oldbuffer, AlphaNumericFrameBuffer, 2048)) //check if same frame again
		//	return;

		memcpy(oldbuffer, AlphaNumericFrameBuffer, 2048);

		memcpy(OutputPacketBuffer+4,AlphaNumericFrameBuffer,2048);
		DWORD bytes;
		// send dmd frame buffer to pindmd board
		ftStatus = FT_Write(ftHandle, &OutputPacketBuffer, do16 ? (DWORD)2052:(DWORD)1028, &bytes);	
	}
}


#if 0 // unused
void Send_Logo()
{
		FILE *fLogo;

		UINT8 LogoBuffer[32][128] = {};

		// display dmd logo from text file if it exists
		fopen_s(&fLogo, "dmdlogo.txt","r");
		if(fLogo){
			for(int i=0; i<32; i++){
				for(int j=0; j<128; j++)
				{
					UINT8 fileChar = getc(fLogo);
					//Read next char after enter (beginning of next line)
					while(fileChar == 10)
						fileChar = getc(fLogo);

					if(!do16)
					{
						LogoBuffer[i][j] = fileChar - '0';
						if(LogoBuffer[i][j] > 3)
							LogoBuffer[i][j] = 0;
					}
					else
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
								LogoBuffer[i][j] = fileChar - '0';
								break;

							case 'a':
							case 'A':
								LogoBuffer[i][j] = 10;
								break;
						
							case 'b':
							case 'B':
								LogoBuffer[i][j] = 11;
								break;

							case 'c':
							case 'C':
								LogoBuffer[i][j] = 12;
								break;

							case 'd':
							case 'D':
								LogoBuffer[i][j] = 13;
								break;

							case 'e':
							case 'E':
								LogoBuffer[i][j] = 14;
								break;

							case 'f':
							case 'F':
								LogoBuffer[i][j] = 15;
								break;

							default:
								LogoBuffer[i][j] = 0;
								break;
						}
				}
			}
			fclose(fLogo);
		}

		Render_16_Shades(128,32,*LogoBuffer);
}
#endif
