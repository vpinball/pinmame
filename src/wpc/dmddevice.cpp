#include <windows.h>
#include "driver.h"
#include "gen.h"
#include "core.h"
#include "cpu/at91/at91.h"
#include "dmddevice.h"

#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
 #define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif

#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
 #define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

UINT16	seg_data2[50] = {};
UINT16	dmd_width = 128;
UINT16	dmd_height = 32;
bool    dmd_hasDMD = false;

HMODULE hModule;


typedef int (*Open_t)();
Open_t DmdDev_Open;

typedef bool (*Close_t)();
Close_t DmdDev_Close;

typedef void (*PM_GameSettings_t)(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options);
PM_GameSettings_t DmdDev_PM_GameSettings;

typedef void (*Set_4_Colors_Palette_t)(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100);
Set_4_Colors_Palette_t DmdDev_Set_4_Colors_Palette;

typedef void (*Console_Data_t)(UINT8 data);
Console_Data_t DmdDev_Console_Data;

typedef int(*Console_Input_t)(UINT8 *buf, int size);
typedef int(*Console_Input_Ptr_t)(Console_Input_t ptr);
Console_Input_Ptr_t DmdDev_Console_Input_Ptr;

typedef void (*Render_16_Shades_t)(UINT16 width, UINT16 height, UINT8 *currbuffer);
Render_16_Shades_t DmdDev_Render_16_Shades;

typedef void (*Render_4_Shades_t)(UINT16 width, UINT16 height, UINT8 *currbuffer);
Render_4_Shades_t DmdDev_Render_4_Shades;

typedef void (*render_PM_Alphanumeric_Frame_t)(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2);
render_PM_Alphanumeric_Frame_t DmdDev_render_PM_Alphanumeric_Frame;

void FwdConsoleData(UINT8 data){
	if (DmdDev_Console_Data)
		DmdDev_Console_Data(data);
}

extern "C" {
	int at91_receive_serial(int usartno, data8_t *buf, int size);
}

int RcvConsoleInput(UINT8 *buf, int size)
{
	int ret = at91_receive_serial(1, buf, size);
	return ret;
}

int pindmdInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions *Options) {
	
	// look for the DmdDevice(64).dll in the path of vpinmame.dll
	char filename[MAX_PATH];

#ifndef _WIN64
	const HINSTANCE hVpmDLL = GetModuleHandle("VPinMAME.dll");
#else
	const HINSTANCE hVpmDLL = GetModuleHandle("VPinMAME64.dll");
#endif

	GetModuleFileName(hVpmDLL,filename,MAX_PATH);
	char *ptr = strrchr(filename,'\\');
#ifdef _WIN64
	strcpy(ptr+1,"DmdDevice64.dll");
#else
	strcpy(ptr+1,"DmdDevice.dll");
#endif

	hModule = LoadLibraryEx(filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	
	if ( !hModule ) {
#ifdef _WIN64
		hModule = LoadLibrary("DmdDevice64.dll");
#else
		hModule = LoadLibrary("DmdDevice.dll");
#endif
	}
	
	if (!hModule) {
		MessageBox(NULL, "No DMD device driver found", filename, MB_ICONERROR);
		return 0;
	}

	DmdDev_Open = (Open_t) GetProcAddress(hModule, "Open");
	
	DmdDev_PM_GameSettings = (PM_GameSettings_t) GetProcAddress(hModule, "PM_GameSettings");

	DmdDev_Close = (Close_t) GetProcAddress(hModule, "Close");
	
	DmdDev_Render_4_Shades = (Render_4_Shades_t) GetProcAddress(hModule, "Render_4_Shades");

	DmdDev_Render_16_Shades = (Render_16_Shades_t) GetProcAddress(hModule, "Render_16_Shades");

	DmdDev_render_PM_Alphanumeric_Frame = (render_PM_Alphanumeric_Frame_t) GetProcAddress(hModule, "Render_PM_Alphanumeric_Frame");

	DmdDev_Set_4_Colors_Palette = (Set_4_Colors_Palette_t) GetProcAddress(hModule, "Set_4_Colors_Palette");

	DmdDev_Console_Data = (Console_Data_t) GetProcAddress(hModule, "Console_Data");

	DmdDev_Console_Input_Ptr = (Console_Input_Ptr_t)GetProcAddress(hModule, "Console_Input_Ptr");

	if ( !DmdDev_Open || !DmdDev_Close || !DmdDev_PM_GameSettings || !DmdDev_Render_4_Shades || !DmdDev_Render_16_Shades || !DmdDev_render_PM_Alphanumeric_Frame ) {
		MessageBox(NULL, "DMD device driver functions not found", filename, MB_ICONERROR);
		return 0;
	} else {

		DmdDev_Open();

		if (DmdDev_Console_Input_Ptr)
			DmdDev_Console_Input_Ptr(RcvConsoleInput);

		dmd_width = 128; // set default DMD size
		dmd_height = 32;
		dmd_hasDMD = false;

		rgb24 color0,color33,color66,color100;

		if (DmdDev_Set_4_Colors_Palette && Options->dmd_colorize) {
			color0.red = Options->dmd_red0;
			color0.green = Options->dmd_green0;
			color0.blue = Options->dmd_blue0;
			color33.red = Options->dmd_red33;
			color33.green = Options->dmd_green33;
			color33.blue = Options->dmd_blue33;
			color66.red = Options->dmd_red66;
			color66.green = Options->dmd_green66;
			color66.blue = Options->dmd_blue66;
			color100.red = Options->dmd_red;
			color100.green = Options->dmd_green;
			color100.blue = Options->dmd_blue;
			DmdDev_Set_4_Colors_Palette(color0,color33,color66,color100);
		}

		DmdDev_PM_GameSettings(GameName, HardwareGeneration, *Options);

		return 1;
	}
}

void pindmdDeInit() {

	UINT8 *tmpbuffer = (UINT8 *)malloc(dmd_width*dmd_height);
	memset(tmpbuffer, 0x00, dmd_width*dmd_height);

	if (DmdDev_Render_4_Shades) 
		DmdDev_Render_4_Shades(dmd_width, dmd_height, tmpbuffer); //clear screen

	if (DmdDev_Close)
		DmdDev_Close();

	free(tmpbuffer);
	FreeLibrary(hModule);
}

void renderDMDFrame(UINT64 gen, UINT16 width, UINT16 height, UINT8 *currbuffer, UINT8 doDumpFrame, const char* GameName) {

	dmd_width = width; // store for DeInit
	dmd_height = height;
	dmd_hasDMD = true;

	if ((gen == GEN_SAM) ||
		// extended handling also for some GTS3 games (SMB, SMBMW and CBW):
		(_strnicmp(GameName, "smb", 3) == 0) || (_strnicmp(GameName, "cueball", 7) == 0) ||
		(gen == GEN_ALVG_DMD2)) {
		if (DmdDev_Render_16_Shades)
			DmdDev_Render_16_Shades(width,height,currbuffer);
	} else {
		if (DmdDev_Render_4_Shades)
			DmdDev_Render_4_Shades(width,height,currbuffer);
	}
} 

void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, UINT8 total_disp, UINT8 *disp_lens){

	// Some GTS3 games like Teed Off update both empty alpha and real DMD.   If a DMD frame has been seen, 
	// block this from running.
	if (dmd_hasDMD)
		return;

	layout_t layout = None;
	bool hasExtraData = false;
	
	// Medusa fix
	if((gen == GEN_BY35) && (disp_lens[0] == 2))
	{
		memcpy(seg_data2,seg_data,50*sizeof(UINT16));
		hasExtraData = true;
		return;
	}

	// switch to current game tech
	switch(gen){
		// williams
		case GEN_S3:
		case GEN_S3C:
		case GEN_S4:
		case GEN_S6:
			layout = _2x6Num_2x6Num_4x1Num;
			break;
		case GEN_S7:
			//_2x7Num_4x1Num_1x16Alpha(seg_data);		hmm i did this for a reason??
			layout = _2x7Num_2x7Num_4x1Num_gen7;
			break;
		case GEN_S9:
			layout = _2x7Num10_2x7Num10_4x1Num;
			break;

		// williams
		case GEN_WPCALPHA_1:
		case GEN_WPCALPHA_2:
		case GEN_S11C:
		case GEN_S11B2:
			layout = _2x16Alpha;
			break;
		case GEN_S11:
			layout = _6x4Num_4x1Num;
			break;
		case GEN_S11X:
			switch(total_disp){
				case 2:
					layout = _2x16Alpha;
					break;
				case 3:
					layout = _1x16Alpha_1x16Num_1x7Num;
					break;
				case 4:
					layout = _2x7Alpha_2x7Num;
					break;
				case 8:
					layout = _2x7Alpha_2x7Num_4x1Num;
					break;
			}
			break;

		// dataeast
		case GEN_DE:
			switch(total_disp){
				case 2:
					layout = _2x16Alpha;
					break;
				case 4:
					layout = _2x7Alpha_2x7Num;
					break;
				case 8:
					layout = _2x7Alpha_2x7Num_4x1Num;
					break;
			}
			break;

		// gottlieb
		case GEN_GTS1:
		case GEN_GTS80:
			switch(disp_lens[0]){
				case 6:
					layout = _2x6Num10_2x6Num10_4x1Num;
					break;
				case 7:
					layout = _2x7Num10_2x7Num10_4x1Num;
					break;
			}
			break;
		case GEN_GTS80B:
			break;
		case GEN_GTS3:
			// 2 many digits :(
			layout = _2x20Alpha;
			break;

		// stern
		case GEN_STMPU100:
		case GEN_STMPU200:
			switch(disp_lens[0]){
				case 6:
					layout = _2x6Num_2x6Num_4x1Num;
					break;
				case 7:
					layout = _2x7Num_2x7Num_4x1Num;
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
						layout = _2x6Num_2x6Num_4x1Num;
						break;
					case 7:
						if(hasExtraData)
							layout = _2x7Num_2x7Num_10x1Num;
						else
							layout = _2x7Num_2x7Num_4x1Num;
						break;
				}
			}
			break;
		case GEN_BY6803:
		case GEN_BY6803A:
			layout = _4x7Num10;
			break;
		case GEN_BYPROTO:
			layout = _2x6Num_2x6Num_4x1Num;
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

	if (DmdDev_render_PM_Alphanumeric_Frame)
		DmdDev_render_PM_Alphanumeric_Frame (layout, seg_data, seg_data2);

}   // legacy pinMame
