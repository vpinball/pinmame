#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\..\dmddevice.h"
#include "..\..\usbalphanumeric.h"

HWND hWnd;
HDC hdc;
COLORREF *arr;

COLORREF colors[16] = {};
bool isOpen = false;

DMDDEV int Open()
{
	hWnd = CreateWindowEx(0, "#32770", "DMD Device", WS_VISIBLE, 100, 100, 512+5, 128+30, 0, 0, 0, 0);
	hdc = GetDC(hWnd);
	arr = (COLORREF*)calloc(512 * 128, sizeof(COLORREF));
	isOpen = true;
	return 1;
}

DMDDEV bool Close()
{
	if (isOpen) {
		DeleteDC(hdc); 
		DestroyWindow(hWnd);
		free(arr);
	}

	isOpen = false;
	return true;
}

DMDDEV void PM_GameSettings(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options)
{
	if (Options.dmd_colorize == 0) {
		rgb24  Col[16] = {};
		double R, G, B = 0.;
		if (Options.dmd_red > 0)
			R = Options.dmd_red / 255.;
		if (Options.dmd_green > 0)
			G = Options.dmd_green / 255.;
		if (Options.dmd_blue > 0)
			B = Options.dmd_blue / 255.;
		for (int i = 0; i < 16; i++) {
			int r = (int)(R * (i * 17));
			int g = (int)(G * (i * 17));
			int b = (int)(B * (i * 17));
			if (r > 255)
				r = 255;
			if (g > 255)
				g = 255;
			if (b > 255)
				b = 255;
			Col[i].red = (UINT8)r;
			Col[i].green = (UINT8)g;
			Col[i].blue = (UINT8)b;
		}
		Set_16_Colors_Palette(Col);
	}
}

DMDDEV void Set_4_Colors_Palette(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100) 
{
	colors[0] = ((int)color0.red << 16) | ((int)color0.green << 8) | ((int)color0.blue /*<< 0*/);
	colors[1] = ((int)color33.red << 16) | ((int)color33.green << 8) | ((int)color33.blue /*<< 0*/);
	colors[4] = ((int)color66.red << 16) | ((int)color66.green << 8) | ((int)color66.blue /*<< 0*/);
	colors[15] = ((int)color100.red << 16) | ((int)color100.green << 8) | ((int)color100.blue /*<< 0*/);
}

DMDDEV void Set_16_Colors_Palette(rgb24 *color)
{
	for (int i = 0; i < 16;i++)
		colors[i] = ((int)color[i].red << 16) | ((int)color[i].green << 8) | ((int)color[i].blue /*<< 0*/);
}

DMDDEV void Render_4_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer)
{
	if (isOpen) {
		for (int i = 0; i < width*height; i++){
			switch (currbuffer[i]) {
			case 0:
				arr[i] = colors[0];
				break;
			case 1:
				arr[i] = colors[1];
				break;
			case 2:
				arr[i] = colors[4];
				break;
			case 3:
				arr[i] = colors[15];
				break;
			}
		}
		HBITMAP map = CreateBitmap(width, height, 1, 32, (void*)arr);
		HDC src = CreateCompatibleDC(hdc);
		HGDIOBJ oldbmp = SelectObject(src, map);
		StretchBlt(hdc, 0, 0, 512, 128, src, 0, 0, width, height,SRCCOPY);
		
		// clean up the GDI objects
		SelectObject(src, oldbmp);
		DeleteObject(map);
		DeleteDC(src);
	}
}

DMDDEV void Render_16_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer) 
{
	if (isOpen) {
		for (int i = 0; i < width*height; i++)
			arr[i] = colors[currbuffer[i]];
		HBITMAP map = CreateBitmap(width, height, 1, 32, (void*)arr);
		HDC src = CreateCompatibleDC(hdc);
		HGDIOBJ oldbmp = SelectObject(src, map);
		StretchBlt(hdc, 0, 0, 512, 128, src, 0, 0, width, height, SRCCOPY);

		// clean up the GDI objects
		SelectObject(src, oldbmp);
		DeleteObject(map);
		DeleteDC(src);
	}
}


DMDDEV void Render_PM_Alphanumeric_Frame(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2) 
{
	if (isOpen) {	
		memset(AlphaNumericFrameBuffer,0x00,2048);
	
		switch (layout) {
			case __2x16Alpha :
				_2x16Alpha(seg_data);
				break;
			case __2x20Alpha :
				_2x20Alpha(seg_data); //!! misses 4 chars in each row
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

		Render_16_Shades(128, 32, tempbuffer);
	}
}

DMDDEV void Render_RGB24(UINT16 width, UINT16 height, rgb24 *currbuffer)
{
	if (isOpen) {
		for (int i = 0; i < width*height; i++){
			arr[i] = ((int)currbuffer[i].red << 16) | ((int)currbuffer[i].green << 8) | ((int)currbuffer[i].blue /*<< 0*/);
		}
		HBITMAP map = CreateBitmap(width, height, 1, 32, (void*)arr);
		HDC src = CreateCompatibleDC(hdc);
		HGDIOBJ oldbmp = SelectObject(src, map);
		StretchBlt(hdc, 0, 0, 512, 128, src, 0, 0, width, height, SRCCOPY);

		// Clean up GDI objects
		SelectObject(src, oldbmp);
		DeleteObject(map);
		DeleteDC(src);
	}
}


DMDDEV void Console_Data(UINT8 data)
{
	/*
	FILE *file;
	fopen_s(&file, "console.txt", "ab");
	if (file) {
		fputc(data, file);
		fclose(file);
	}
	*/
}
