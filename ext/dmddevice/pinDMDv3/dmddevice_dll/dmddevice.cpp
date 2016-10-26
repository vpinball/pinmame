#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\..\dmddevice.h"
#include "..\pindmd3\pinDMD3.h"

#include "..\..\usbalphanumeric.h"

bool isOpen = false;
//UINT64 gen = 0;


void Send_Clear_Screen(void) //!! unused
{
	memset(OutputPacketBuffer,0x00, 2048);
	render16ShadeFrame(OutputPacketBuffer);
	Sleep(50);
}

DMDDEV int Open()
{
	return 1;
}

DMDDEV bool Close()
{
	if (isOpen) {
		pindmdDeInit();
		free(OutputPacketBuffer);
	}

	isOpen = false;
	return true;
}

DMDDEV void PM_GameSettings(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options)
{
	//gen = HardwareGeneration;
	if (pindmdInit(Options)) {
		isOpen = true;
		OutputPacketBuffer = (UINT8 *)malloc(2048);
	}
}

DMDDEV void Set_4_Colors_Palette(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100) 
{
}


DMDDEV void Render_4_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer)
{
	if (isOpen) {
		//int byteIdx=0;
		UINT8 tempbuffer[128*32]; // for rescale

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

					switch (tempbuffer[o]){
					case 0:
						tempbuffer[o] = 0;
						break;
					case 1:
						tempbuffer[o] = 1;
						break;
					case 2:
						tempbuffer[o] = 7;
						break;
					case 3:
						tempbuffer[o] = 15;
						break;
					}
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

					switch (tempbuffer[o]){
					case 0:
						tempbuffer[o] = 0;
						break;
					case 1:
						tempbuffer[o] = 1;
						break;
					case 2:
						tempbuffer[o] = 7;
						break;
					case 3:
						tempbuffer[o] = 15;
						break;
					}
				}
		} else
			for (int i = 0; i < width*height; i++){
				switch (currbuffer[i]){
				case 0:
					tempbuffer[i] = 0;
					break;
				case 1:
					tempbuffer[i] = 1;
					break;
				case 2:
					tempbuffer[i] = 7;
					break;
				case 3:
					tempbuffer[i] = 15;
					break;
				}
			}
	
		// dmd height
/*		for(int j = 0; j < ((height==16)?16:32); ++j)
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

					if(pixel==3)
						pixel=15;	
					else if(pixel==2)
						pixel=4;

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
*/
		render16ShadeFrame(tempbuffer);
	}
}

DMDDEV void Render_16_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer) 
{
	if (isOpen) {
		//int byteIdx=0;
		UINT8 tempbuffer[128*32]; // for rescale

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
		/*for(int j = 0; j < ((height==16)?16:32); ++j)
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
*/
		render16ShadeFrame(tempbuffer);
	}
}


DMDDEV void Render_PM_Alphanumeric_Frame(layout_t layout, UINT16 *seg_data, UINT16 *seg_data2) 
{
	if (isOpen) {	
		memset(AlphaNumericFrameBuffer,0x00,2048);
	
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

		UINT8 tempbuffer[128*32]; 
		for (int i = 0; i < 512; ++i) {
			tempbuffer[(i*8)] = AlphaNumericFrameBuffer[i] & 0x01 | AlphaNumericFrameBuffer[i+512]<<1 & 0x02 | AlphaNumericFrameBuffer[i+1024]<<2 & 0x04 | AlphaNumericFrameBuffer[i+1536]<<3 & 0x08;
			tempbuffer[(i*8)+1] = AlphaNumericFrameBuffer[i]>>1 & 0x01 | AlphaNumericFrameBuffer[i+512] & 0x02 | AlphaNumericFrameBuffer[i+1024]<<1 & 0x04 | AlphaNumericFrameBuffer[i+1536]<<2 & 0x08;
			tempbuffer[(i*8)+2] = AlphaNumericFrameBuffer[i]>>2 & 0x01 | AlphaNumericFrameBuffer[i+512]>>1 & 0x02 | AlphaNumericFrameBuffer[i+1024] & 0x04 | AlphaNumericFrameBuffer[i+1536]<<1 & 0x08;
			tempbuffer[(i*8)+3] = AlphaNumericFrameBuffer[i]>>3 & 0x01 | AlphaNumericFrameBuffer[i+512]>>2 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>1 & 0x04 | AlphaNumericFrameBuffer[i+1536] & 0x08;
			tempbuffer[(i*8)+4] = AlphaNumericFrameBuffer[i]>>4 & 0x01 | AlphaNumericFrameBuffer[i+512]>>3 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>2 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>1 & 0x08;
			tempbuffer[(i*8)+5] = AlphaNumericFrameBuffer[i]>>5 & 0x01 | AlphaNumericFrameBuffer[i+512]>>4 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>3 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>2 & 0x08;
			tempbuffer[(i*8)+6] = AlphaNumericFrameBuffer[i]>>6 & 0x01 | AlphaNumericFrameBuffer[i+512]>>5 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>4 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>3 & 0x08;
			tempbuffer[(i*8)+7] = AlphaNumericFrameBuffer[i]>>7 & 0x01 | AlphaNumericFrameBuffer[i+512]>>6 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>5 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>4 & 0x08;
		}

		render16ShadeFrame(tempbuffer);
	}
}
