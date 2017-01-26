#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\..\dmddevice.h"
#include "..\pindmd3\pinDMD3.h"

#include "..\..\usbalphanumeric.h"

bool isOpen = false;
bool useRGB = false;
static UINT8 oldbuffer[16384] = {};
static UINT16 seg_data_old[50] = {};


DMDDEV int Open()
{
	useRGB = false;
	return 1;
}

DMDDEV bool Close()
{
	if (isOpen) {
		pindmdDeInit();
	}
	useRGB = false;
	isOpen = false;
	return true;
}

DMDDEV void PM_GameSettings(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options)
{
	if (!isOpen) {
		if (pindmdInit(Options))
			isOpen = true;
		else
			MessageBox(NULL, "pinDMD v3 not found", "Error", MB_ICONERROR);
	}
}

DMDDEV void Set_4_Colors_Palette(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100) 
{
	tPMoptions Options;
	Options.dmd_red = color100.red;
	Options.dmd_green = color100.green;
	Options.dmd_blue = color100.blue;
	Options.dmd_red66 = color66.red;
	Options.dmd_green66 = color66.green;
	Options.dmd_blue66 = color66.blue;
	Options.dmd_red33 = color33.red;
	Options.dmd_green33 = color33.green;
	Options.dmd_blue33 = color33.blue;
	Options.dmd_red0 = color0.red;
	Options.dmd_green0 = color0.green;
	Options.dmd_blue0 = color0.blue;
	Options.dmd_colorize = 1;
	if (!isOpen) {
		if (pindmdInit(Options))
			isOpen = true;
		else
			MessageBox(NULL, "pinDMD v3 not found", "Error", MB_ICONERROR);
	}
}

DMDDEV void Set_16_Colors_Palette(rgb24 *color)
{
	tPMoptions Options;
	Options.dmd_red = color[15].red;
	Options.dmd_green = color[15].green;
	Options.dmd_blue = color[15].blue;
	Options.dmd_colorize = 0;
	if (!isOpen) {
		if (pindmdInit(Options))
			isOpen = true;
		else
			MessageBox(NULL, "pinDMD v3 not found", "Error", MB_ICONERROR);
	}
}

DMDDEV void Render_4_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer)
{
	if (!memcmp(oldbuffer, currbuffer, width*height)) //check if same frame again
		return;

	memcpy(oldbuffer, currbuffer, width*height);

	if (isOpen) {
		renderDMDFrame(0x00000000080, (UINT8) width, (UINT8) height, currbuffer, 0x00); 
	}
}

DMDDEV void Render_16_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer) 
{
	if (!memcmp(oldbuffer, currbuffer, width*height)) //check if same frame again
		return;

	memcpy(oldbuffer, currbuffer, width*height);

	if (isOpen) {
		render16ShadeFrame(currbuffer);
	}
}


DMDDEV void Render_PM_Alphanumeric_Frame(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2) 
{
	if (!memcmp(seg_data, seg_data_old, 50 * sizeof(UINT16)))
		return;

	memcpy(seg_data_old, seg_data, 50 * sizeof(UINT16));

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
			tempbuffer[(i*8)]   = AlphaNumericFrameBuffer[i]    & 0x01 | AlphaNumericFrameBuffer[i+512]<<1 & 0x02 | AlphaNumericFrameBuffer[i+1024]<<2 & 0x04 | AlphaNumericFrameBuffer[i+1536]<<3 & 0x08;
			tempbuffer[(i*8)+1] = AlphaNumericFrameBuffer[i]>>1 & 0x01 | AlphaNumericFrameBuffer[i+512]    & 0x02 | AlphaNumericFrameBuffer[i+1024]<<1 & 0x04 | AlphaNumericFrameBuffer[i+1536]<<2 & 0x08;
			tempbuffer[(i*8)+2] = AlphaNumericFrameBuffer[i]>>2 & 0x01 | AlphaNumericFrameBuffer[i+512]>>1 & 0x02 | AlphaNumericFrameBuffer[i+1024]    & 0x04 | AlphaNumericFrameBuffer[i+1536]<<1 & 0x08;
			tempbuffer[(i*8)+3] = AlphaNumericFrameBuffer[i]>>3 & 0x01 | AlphaNumericFrameBuffer[i+512]>>2 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>1 & 0x04 | AlphaNumericFrameBuffer[i+1536]    & 0x08;
			tempbuffer[(i*8)+4] = AlphaNumericFrameBuffer[i]>>4 & 0x01 | AlphaNumericFrameBuffer[i+512]>>3 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>2 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>1 & 0x08;
			tempbuffer[(i*8)+5] = AlphaNumericFrameBuffer[i]>>5 & 0x01 | AlphaNumericFrameBuffer[i+512]>>4 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>3 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>2 & 0x08;
			tempbuffer[(i*8)+6] = AlphaNumericFrameBuffer[i]>>6 & 0x01 | AlphaNumericFrameBuffer[i+512]>>5 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>4 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>3 & 0x08;
			tempbuffer[(i*8)+7] = AlphaNumericFrameBuffer[i]>>7 & 0x01 | AlphaNumericFrameBuffer[i+512]>>6 & 0x02 | AlphaNumericFrameBuffer[i+1024]>>5 & 0x04 | AlphaNumericFrameBuffer[i+1536]>>4 & 0x08;
		}

		//if (!memcmp(oldbuffer, tempbuffer, 128*32)) //check if same frame again
		//	return;

		memcpy(oldbuffer, tempbuffer, 128*32);

		render16ShadeFrame(tempbuffer);
	}
}

DMDDEV void Render_RGB24(UINT16 width, UINT16 height, rgb24 *currbuffer)
{
	if (!memcmp(oldbuffer, currbuffer, width*height)) //check if same frame again
		return;

	memcpy(oldbuffer, currbuffer, width*height);

	if (!isOpen && !useRGB) {
		tPMoptions Options;
		Options.dmd_red = 255;
		Options.dmd_green = 0;
		Options.dmd_blue = 0;
		Options.dmd_colorize = 0;
		useRGB = true;
		if (pindmdInit(Options))
			isOpen = true;
		else
			MessageBox(NULL, "pinDMD v3 not found", "Error", MB_ICONERROR);
	}

	if (isOpen) {
		renderRGB24Frame(currbuffer);
	}
}
