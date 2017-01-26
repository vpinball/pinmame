#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "..\libusb\include\lusb0_usb.h"

#include "..\..\dmddevice.h"
#include "..\..\usbalphanumeric.h"

//define vendor id and product id
#define VID 0x0314
#define PID 0xe457

//endpoints for communication
#define EP_IN 0x81
#define EP_OUT 0x01

bool isOpen = false;
static UINT8 oldbuffer[16384] = {};
static UINT16 seg_data_old[50] = {};

usb_dev_handle *DeviceHandle = NULL; 


/*void Send_Clear_Screen(void) //!! unused
{
	memset(OutputPacketBuffer,0x00, 2052);
	const UINT8 tmp[4] = {
		0x81, 0xC3, 0xE7, 0x00 //header
		}; 
	memcpy(OutputPacketBuffer, tmp, sizeof(tmp));
	usb_bulk_write(DeviceHandle, EP_OUT, (char*)OutputPacketBuffer, 2052, 1000);
	Sleep(50);
}*/

DMDDEV int Open()
{
		//init usb library
		usb_init();
		//find busses
		usb_find_busses();
		//find devices
		usb_find_devices();

		struct usb_bus *bus;
		struct usb_device *dev = NULL;
		for (bus = usb_get_busses(); bus; bus = bus->next) {
			for (dev = bus->devices; dev; dev = dev->next) {
				//if device vendor id and product id are match
				if (dev->descriptor.idVendor == VID && dev->descriptor.idProduct == PID)
				{
					//try to open our device
					DeviceHandle = usb_open(dev);
					if(DeviceHandle)
						break;
				}
			}
		}

		if(DeviceHandle == NULL)
		{
			MessageBox(NULL, "pinDMD v2 not found","Error", MB_ICONERROR);
			return 0;
		}

		if (usb_set_configuration(DeviceHandle, 1) < 0) {
			usb_close(DeviceHandle);
			return 0;
		}
		
	
		if (usb_claim_interface(DeviceHandle, 0) < 0) {
			usb_close(DeviceHandle);
			return 0;
		}

/*		char string[256] = {};
		int ret = usb_get_string_simple(DeviceHandle, dev->descriptor.iProduct, string, sizeof(string));

		if (ret > 0) {
			if (strcmp(string, "pinDMD V2") == 0) { 
*/
				OutputPacketBuffer = (unsigned char *)malloc(2052);
				isOpen = true;
/*			} else {
				MessageBox(NULL, L"pinDMD v2 not found",L"Error", MB_ICONERROR);
				usb_close(DeviceHandle);
				return 0;
			}
		}
*/
		return 1;
}


void Send_Logo(void)
{
		FILE *fLogo;
		char filename[MAX_PATH];
		UINT8 LogoBuffer[32][128] = {};

		// display dmd logo from text file if it exists

#ifndef _WIN64
		const HINSTANCE hInst = GetModuleHandle("VPinMAME.dll");
#else
		const HINSTANCE hInst = GetModuleHandle("VPinMAME64.dll");
#endif
		GetModuleFileName(hInst, filename, MAX_PATH);
		char *ptr = strrchr(filename, '\\');
		strcpy_s(ptr + 1, 12, "dmdlogo.txt");

		fopen_s(&fLogo, filename, "r");
		if(fLogo){
			for(int i=0; i<32; i++){
				for(int j=0; j<128; j++)
				{
					UINT8 fileChar = getc(fLogo);
					//Read next char after enter (beginning of next line)
					while(fileChar == 10)
						fileChar = getc(fLogo);
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


DMDDEV bool Close()
{
	if (isOpen) {
		Send_Logo();

		usb_release_interface( DeviceHandle, 0);
		usb_close(DeviceHandle);
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

		OutputPacketBuffer[0] = 0x81; // frame sync bytes
		OutputPacketBuffer[1] = 0xC3;
		OutputPacketBuffer[2] = 0xE7;
		OutputPacketBuffer[3] = 0x0;  // command byte

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

		usb_bulk_write(DeviceHandle, EP_OUT, (char*)OutputPacketBuffer, 2052, 1000);
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

		OutputPacketBuffer[0] = 0x81; // frame sync bytes
		OutputPacketBuffer[1] = 0xC3;
		OutputPacketBuffer[2] = 0xE7;
		OutputPacketBuffer[3] = 0x0;  // command byte

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

		usb_bulk_write(DeviceHandle, EP_OUT, (char*)OutputPacketBuffer, 2052, 1000);
	}
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

		usb_bulk_write(DeviceHandle, EP_OUT, (char*)OutputPacketBuffer, 2052, 1000);
	}
}
