#ifdef __cplusplus
extern "C"
{
#endif

#include "gen.h"
#include "driver.h"
#include "core.h"
#include "dmddevice.h"

#ifdef __cplusplus
}
#endif

static UINT16  seg_data2[CORE_SEGCOUNT] = {};

layout_t layoutAlphanumericFrame(UINT64 gen, UINT16* seg_data, UINT16* seg_data_2, UINT8 total_disp, UINT8 *disp_num_segs, const char* GameName) {

	// Medusa fix
	if((strncasecmp(GameName, "medusa", 6) == 0) && (disp_num_segs[0] == 2))
	{
		memcpy(seg_data_2,seg_data, sizeof(seg_data2));
		return __Invalid;
	}

	layout_t layout = __None;

	// switch to current game tech
	switch(gen){
		// williams
		case GEN_S3:
		case GEN_S3C:
		case GEN_S4:
		case GEN_S6:
			layout = __2x6Num_2x6Num_4x1Num;
			if ((strncasecmp(GameName, "algar", 5) == 0) || // Sys6A with 7 digit displays
				(strncasecmp(GameName, "alpok", 5) == 0) ||
				(strncasecmp(GameName, "frpwr_b6", 8) == 0) ||
				(strncasecmp(GameName, "frpwr_c6", 8) == 0))
				layout = __2x7Num_2x7Num_4x1Num_gen7;
			break;
		case GEN_S7:
			//__2x7Num_4x1Num_1x16Alpha;		//!! hmm i did this for a reason??
			layout = __2x7Num_2x7Num_4x1Num_gen7;
			break;
		case GEN_S9:
			layout = __2x7Num10_2x7Num10_4x1Num;
			break;

		case GEN_WPCALPHA_1:
		case GEN_WPCALPHA_2:
		case GEN_S11C:
		case GEN_S11B2:
			if (strncasecmp(GameName, "rvrbt", 5) == 0) // Additional displays for Riverboat Gambler
				layout = __1x16Alpha_1x16Num_1x7Num_1x4Num;
			else if (strncasecmp(GameName, "polic", 5) == 0) // Police force has an additional display (jackpot)
				layout = __1x7Num_1x16Alpha_1x16Num;
			else
				layout = __2x16Alpha;
			break;
		case GEN_S11:
			layout = __6x4Num_4x1Num;
			break;
		case GEN_S11X: // incl. GEN_S11A, GEN_S11B
			switch(total_disp){
				case 2:
					layout = __2x16Alpha;
					break;
				case 3:
					layout = __1x16Alpha_1x16Num_1x7Num;
					break;
				case 4:
					layout = __2x7Alpha_2x7Num;
					break;
				case 8:
					layout = __2x7Alpha_2x7Num_4x1Num;
					break;
			}
			if (strncasecmp(GameName, "polic", 5) == 0)
				layout = __1x7Num_1x16Alpha_1x16Num;
			break;

		// dataeast
		case GEN_DE:
			switch(total_disp){
				case 2:
					layout = __2x16Alpha;
					break;
				case 4:
					layout = __2x7Alpha_2x7Num;
					break;
				case 8:
					layout = __2x7Alpha_2x7Num_4x1Num;
					break;
			}
			break;

		// gottlieb
		case GEN_GTS1:
		case GEN_GTS80:
			switch(disp_num_segs[0]){
				case 6:
					layout = __2x6Num10_2x6Num10_4x1Num;
					break;
				case 7:
					layout = __2x7Num10_2x7Num10_4x1Num;
					break;
			}
			break;
		case GEN_GTS80B:
		case GEN_GTS3:
			layout = __2x20Alpha;
			break;

		// stern
		case GEN_STMPU100:
		case GEN_STMPU200:
			switch(disp_num_segs[0]){
				case 6:
					layout = __2x6Num_2x6Num_4x1Num;
					break;
				case 7:
					layout = __2x7Num_2x7Num_4x1Num;
					break;
			}
			break;

		// bally
		case GEN_BY17:
		case GEN_BY35:
			// check for   total:8 = 6x6num + 4x1num
			if(total_disp==8){
				//!! ??
			}else{
				switch(disp_num_segs[0]){
					case 6:
						layout = __2x6Num_2x6Num_4x1Num;
						break;
					case 7:
						if(strncasecmp(GameName, "medusa", 6) == 0)
							layout = __2x7Num_2x7Num_10x1Num;
						else
							layout = __2x7Num_2x7Num_4x1Num;
						break;
				}
			}
			break;
		case GEN_BY6803:
		case GEN_BY6803A:
			layout = __4x7Num10;
			break;
		case GEN_BYPROTO:
			layout = __2x6Num_2x6Num_4x1Num;
			break;

		// hankin
		case GEN_HNK:
			layout = __2x20Alpha;
			break;

		//!! unsupported so far:
		// astro
		case GEN_ASTRO:
			break;
		case GEN_BOWLING:
			break;
		// zaccaria
		case GEN_ZAC1:
			break;
		case GEN_ZAC2:
			break;
	}

	return layout;
}





#ifdef VPINMAME

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#if _MSC_VER >= 1800
 // Windows XP _WIN32_WINNT_WINXP
 #define _WIN32_WINNT 0x0501
#elif _MSC_VER < 1600
 #define _WIN32_WINNT 0x0400
#else
 #define _WIN32_WINNT 0x0403
#endif
#define WINVER _WIN32_WINNT
#endif
#include <windows.h>

#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
 #define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif

#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
 #define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

static UINT16  dmd_width = 0; // Only valid if dmd_hasDMD is set
static UINT16  dmd_height = 0; // Only valid if dmd_hasDMD is set
static bool    dmd_hasDMD = false;

static HMODULE DmdDev_hModule;
static HMODULE DmdScr_hModule;

typedef int(*DmdDev_Open_t)();
typedef bool(*DmdDev_Close_t)();
typedef void(*DmdDev_PM_GameSettings_t)(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options);
typedef void(*DmdDev_Set_4_Colors_Palette_t)(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100);
typedef void(*DmdDev_Console_Data_t)(UINT8 data);
typedef int(*DmdDev_Console_Input_t)(UINT8 *buf, int size);
typedef int(*DmdDev_Console_Input_Ptr_t)(DmdDev_Console_Input_t ptr);
typedef void(*DmdDev_Render_16_Shades_t)(UINT16 width, UINT16 height, UINT8 *frame);
typedef void(*DmdDev_Render_4_Shades_t)(UINT16 width, UINT16 height, UINT8 *frame);
typedef void(*DmdDev_Render_16_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8 *frame, UINT32 noOfRawFrames, UINT8 *rawbuffer);
typedef void(*DmdDev_Render_4_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8 *frame, UINT32 noOfRawFrames, UINT8 *rawbuffer);
typedef void(*DmdDev_Render_PM_Alphanumeric_Frame_t)(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2);
typedef void(*DmdDev_Render_PM_Alphanumeric_Dim_Frame_t)(layout_t layout, const UINT16 *const seg_data, const char *const seg_dim, const UINT16 *const seg_data2);
typedef void(*DmdDev_Render_Lum_And_Raw_t)(UINT16 width, UINT16 height, UINT8* lumFrame, UINT8* rawFrame, UINT32 noOfRawFrames, UINT8* rawbuffer);


static DmdDev_Open_t DmdDev_Open;
static DmdDev_Close_t DmdDev_Close;
static DmdDev_PM_GameSettings_t DmdDev_PM_GameSettings;
static DmdDev_Set_4_Colors_Palette_t DmdDev_Set_4_Colors_Palette;
static DmdDev_Console_Data_t DmdDev_Console_Data;
static DmdDev_Console_Input_Ptr_t DmdDev_Console_Input_Ptr;
static DmdDev_Render_16_Shades_t DmdDev_Render_16_Shades;
static DmdDev_Render_4_Shades_t DmdDev_Render_4_Shades;
static DmdDev_Render_16_Shades_with_Raw_t DmdDev_Render_16_Shades_with_Raw;
static DmdDev_Render_4_Shades_with_Raw_t DmdDev_Render_4_Shades_with_Raw;
static DmdDev_Render_PM_Alphanumeric_Frame_t DmdDev_Render_PM_Alphanumeric_Frame;
static DmdDev_Render_PM_Alphanumeric_Dim_Frame_t DmdDev_Render_PM_Alphanumeric_Dim_Frame;
static DmdDev_Render_Lum_And_Raw_t DmdDev_Render_Lum_And_Raw;

static DmdDev_Open_t DmdScr_Open;
static DmdDev_Close_t DmdScr_Close;
static DmdDev_PM_GameSettings_t DmdScr_PM_GameSettings;
static DmdDev_Set_4_Colors_Palette_t DmdScr_Set_4_Colors_Palette;
static DmdDev_Console_Data_t DmdScr_Console_Data;
static DmdDev_Console_Input_Ptr_t DmdScr_Console_Input_Ptr;
static DmdDev_Render_16_Shades_t DmdScr_Render_16_Shades;
static DmdDev_Render_4_Shades_t DmdScr_Render_4_Shades;
static DmdDev_Render_16_Shades_with_Raw_t DmdScr_Render_16_Shades_with_Raw;
static DmdDev_Render_4_Shades_with_Raw_t DmdScr_Render_4_Shades_with_Raw;
static DmdDev_Render_PM_Alphanumeric_Frame_t DmdScr_Render_PM_Alphanumeric_Frame;
static DmdDev_Render_PM_Alphanumeric_Dim_Frame_t DmdScr_Render_PM_Alphanumeric_Dim_Frame;
static DmdDev_Render_Lum_And_Raw_t DmdScr_Render_Lum_And_Raw;


extern "C"
{
#include "cpu/at91/at91.h"
	int at91_receive_serial(int usartno, data8_t* buf, int size);
}

void FwdConsoleData(UINT8 data) {
	if (DmdDev_Console_Data)
		DmdDev_Console_Data(data);

	if (DmdScr_Console_Data)
		DmdScr_Console_Data(data);
}

int RcvConsoleInput(UINT8 *buf, int size)
{
	const int ret = at91_receive_serial(1, buf, size);
	return ret;
}

static HMODULE GetCurrentModule()
{
	HMODULE hModule = NULL;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,	(LPCTSTR)GetCurrentModule, &hModule);
	return hModule;
}

int pindmdInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions *Options) {

	// look for the DmdDevice(64).dll and DmdScreen(64).dll in the path of vpinmame.dll/libpinmame-X.X.dll
	char DmdDev_filename[MAX_PATH];
	char DmdScr_filename[MAX_PATH];
	bool DmdScr = false;
	bool DmdDev = false;

	GetModuleFileName(GetCurrentModule(),DmdDev_filename,MAX_PATH);
	strcpy(DmdScr_filename,DmdDev_filename);
	char *ptr  = strrchr(DmdDev_filename,'\\');
	char *ptr2 = strrchr(DmdScr_filename,'\\');

#ifdef _WIN64
	strcpy(ptr+1,"DmdDevice64.dll");
#else
	strcpy(ptr+1,"DmdDevice.dll");
#endif

#ifdef _WIN64
	strcpy(ptr2+1,"DmdScreen64.dll");
#else
	strcpy(ptr2+1,"DmdScreen.dll");
#endif

	DmdDev_hModule = LoadLibraryEx(DmdDev_filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	if ( !DmdDev_hModule ) {
#ifdef _WIN64
		DmdDev_hModule = LoadLibrary("DmdDevice64.dll");
#else
		DmdDev_hModule = LoadLibrary("DmdDevice.dll");
#endif
	}

	DmdScr_hModule = LoadLibraryEx(DmdScr_filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	if (!DmdScr_hModule) {
#ifdef _WIN64
		DmdScr_hModule = LoadLibrary("DmdScreen64.dll");
#else
		DmdScr_hModule = LoadLibrary("DmdScreen.dll");
#endif
	}

	if (DmdDev_hModule) {
		DmdDev_Open = (DmdDev_Open_t)GetProcAddress(DmdDev_hModule, "Open");
		DmdDev_PM_GameSettings = (DmdDev_PM_GameSettings_t)GetProcAddress(DmdDev_hModule, "PM_GameSettings");
		DmdDev_Close = (DmdDev_Close_t)GetProcAddress(DmdDev_hModule, "Close");
		DmdDev_Render_4_Shades = (DmdDev_Render_4_Shades_t)GetProcAddress(DmdDev_hModule, "Render_4_Shades");
		DmdDev_Render_16_Shades = (DmdDev_Render_16_Shades_t)GetProcAddress(DmdDev_hModule, "Render_16_Shades");
		DmdDev_Render_4_Shades_with_Raw = (DmdDev_Render_4_Shades_with_Raw_t)GetProcAddress(DmdDev_hModule, "Render_4_Shades_with_Raw");
		DmdDev_Render_16_Shades_with_Raw = (DmdDev_Render_16_Shades_with_Raw_t)GetProcAddress(DmdDev_hModule, "Render_16_Shades_with_Raw");
		DmdDev_Render_Lum_And_Raw = (DmdDev_Render_Lum_And_Raw_t)GetProcAddress(DmdDev_hModule, "Render_Lum_And_Raw");
		DmdDev_Render_PM_Alphanumeric_Frame = (DmdDev_Render_PM_Alphanumeric_Frame_t)GetProcAddress(DmdDev_hModule, "Render_PM_Alphanumeric_Frame");
		DmdDev_Render_PM_Alphanumeric_Dim_Frame = (DmdDev_Render_PM_Alphanumeric_Dim_Frame_t)GetProcAddress(DmdDev_hModule, "Render_PM_Alphanumeric_Dim_Frame");
		DmdDev_Set_4_Colors_Palette = (DmdDev_Set_4_Colors_Palette_t)GetProcAddress(DmdDev_hModule, "Set_4_Colors_Palette");
		DmdDev_Console_Data = (DmdDev_Console_Data_t)GetProcAddress(DmdDev_hModule, "Console_Data");
		DmdDev_Console_Input_Ptr = (DmdDev_Console_Input_Ptr_t)GetProcAddress(DmdDev_hModule, "Console_Input_Ptr");

		if (!DmdDev_Open || !DmdDev_Close || !DmdDev_PM_GameSettings || !DmdDev_Render_4_Shades || !DmdDev_Render_16_Shades || !DmdDev_Render_PM_Alphanumeric_Frame) {
			DmdDev = false;
		}
		else {
			DmdDev = true;
		}
	}

	if (DmdScr_hModule) {
		DmdScr_Open = (DmdDev_Open_t)GetProcAddress(DmdScr_hModule, "Open");
		DmdScr_PM_GameSettings = (DmdDev_PM_GameSettings_t)GetProcAddress(DmdScr_hModule, "PM_GameSettings");
		DmdScr_Close = (DmdDev_Close_t)GetProcAddress(DmdScr_hModule, "Close");
		DmdScr_Render_4_Shades = (DmdDev_Render_4_Shades_t)GetProcAddress(DmdScr_hModule, "Render_4_Shades");
		DmdScr_Render_16_Shades = (DmdDev_Render_16_Shades_t)GetProcAddress(DmdScr_hModule, "Render_16_Shades");
		DmdScr_Render_4_Shades_with_Raw = (DmdDev_Render_4_Shades_with_Raw_t)GetProcAddress(DmdScr_hModule, "Render_4_Shades_with_Raw");
		DmdScr_Render_16_Shades_with_Raw = (DmdDev_Render_16_Shades_with_Raw_t)GetProcAddress(DmdScr_hModule, "Render_16_Shades_with_Raw");
		DmdScr_Render_Lum_And_Raw = (DmdDev_Render_Lum_And_Raw_t)GetProcAddress(DmdScr_hModule, "Render_Lum_And_Raw");
		DmdScr_Render_PM_Alphanumeric_Frame = (DmdDev_Render_PM_Alphanumeric_Frame_t)GetProcAddress(DmdScr_hModule, "Render_PM_Alphanumeric_Frame");
		DmdScr_Render_PM_Alphanumeric_Dim_Frame = (DmdDev_Render_PM_Alphanumeric_Dim_Frame_t)GetProcAddress(DmdScr_hModule, "Render_PM_Alphanumeric_Dim_Frame");
		DmdScr_Set_4_Colors_Palette = (DmdDev_Set_4_Colors_Palette_t)GetProcAddress(DmdScr_hModule, "Set_4_Colors_Palette");
		DmdScr_Console_Data = (DmdDev_Console_Data_t)GetProcAddress(DmdScr_hModule, "Console_Data");
		DmdScr_Console_Input_Ptr = (DmdDev_Console_Input_Ptr_t)GetProcAddress(DmdScr_hModule, "Console_Input_Ptr");

		if (!DmdScr_Open || !DmdScr_Close || !DmdScr_PM_GameSettings || !DmdScr_Render_4_Shades || !DmdScr_Render_16_Shades || !DmdScr_Render_PM_Alphanumeric_Frame) {
			DmdScr = false;
		}
		else {
			DmdScr = true;
		}
	}

	dmd_hasDMD = false;
	memset(seg_data2, 0, sizeof(seg_data2));

	rgb24 color0, color33, color66, color100;

	if (Options->dmd_colorize) {
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
	}

	if (DmdDev) {
		DmdDev_Open();

		if (DmdDev_Console_Input_Ptr)
			DmdDev_Console_Input_Ptr(RcvConsoleInput);

		if (DmdDev_Set_4_Colors_Palette && Options->dmd_colorize) {
			DmdDev_Set_4_Colors_Palette(color0,color33,color66,color100);
		}
		DmdDev_PM_GameSettings(GameName, HardwareGeneration, *Options);
	}

	if (DmdScr) {
		DmdScr_Open();

		if (DmdScr_Console_Input_Ptr)
			DmdScr_Console_Input_Ptr(RcvConsoleInput);

		if (DmdScr_Set_4_Colors_Palette && Options->dmd_colorize) {
			DmdScr_Set_4_Colors_Palette(color0, color33, color66, color100);
		}
		DmdScr_PM_GameSettings(GameName, HardwareGeneration, *Options);
	}

	if (!DmdScr && !DmdDev){
		MessageBox(NULL, "No external DMD driver found or DMD driver functions not found", "Visual PinMame Error", MB_ICONERROR);
		return 0;
	}
	else
		return 1;

}

void pindmdDeInit() {
	if (dmd_hasDMD) {
		UINT8* tmpbuffer = (UINT8*)malloc(dmd_width * dmd_height);
		memset(tmpbuffer, 0x00, dmd_width * dmd_height);
		if (DmdDev_Render_4_Shades)
			DmdDev_Render_4_Shades(dmd_width, dmd_height, tmpbuffer); //clear screen
		if (DmdScr_Render_4_Shades)
			DmdScr_Render_4_Shades(dmd_width, dmd_height, tmpbuffer); //clear screen
		free(tmpbuffer);
	}

	if (DmdDev_Close) {
		DmdDev_Close();
		FreeLibrary(DmdDev_hModule);
	}
	if (DmdScr_Close){
		DmdScr_Close();
		FreeLibrary(DmdScr_hModule);
	}
}

void renderDMDFrame(const int width, const int height, UINT8* dmdDotLum, UINT8* dmdDotRaw, UINT32 noOfRawFrames, UINT8 *rawbuffer, const int isDMD2) {
	dmd_width = width; // store for DeInit
	dmd_height = height;
	dmd_hasDMD = true;

	// New implementation that sends both luminance information for rendering, and combined bitplanes as well as raw frames for frame identification for colorization & triggering events
	if (((isDMD2 & 1) != 0) && DmdDev_Render_Lum_And_Raw)
		DmdDev_Render_Lum_And_Raw(width, height, dmdDotLum, dmdDotRaw, noOfRawFrames, rawbuffer);
	if (((isDMD2 & 2) != 0) && DmdScr_Render_Lum_And_Raw)
		DmdScr_Render_Lum_And_Raw(width, height, dmdDotLum, dmdDotRaw, noOfRawFrames, rawbuffer);

	// Legacy implementation uses a single stream for both frame identification and rendering.
	// To ensure some backward compatibility with existing coloriation/rendering implementation:
	// - for GTS3: send luminance information adapted to legacy 2/4 bit format (breaks frame identification, but would break frame rendering otherwise)
	// - for others: send identification frames
	// This is somewhat hacky but needed until all external dmddevice.dll are updated to the new implementation
	UINT8* frame = dmdDotRaw;
	// 16 shades based on hardware generation and extended to some GTS3 games using long PWM pattern (SMB, SMBMW and CBW)
	const int is16Shades = (core_gameData->gen & (GEN_SAM | GEN_SPA | GEN_ALVG_DMD2)) || (strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) || (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0);
	if (core_gameData->gen & GEN_GTS3) {
		const int shift = is16Shades ? 4 : 6;
		frame = (UINT8*)malloc(width * height);
		for (int i = 0; i < width * height; i++)
			frame[i] = dmdDotLum[i] >> shift;
	}
	if (is16Shades) {
		if (((isDMD2 & 1) != 0) && (noOfRawFrames != 0) && DmdDev_Render_16_Shades_with_Raw)
			DmdDev_Render_16_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
		else if (((isDMD2 & 1) != 0) && DmdDev_Render_16_Shades)
			DmdDev_Render_16_Shades(width, height, frame);
		if (((isDMD2 & 2) != 0) && (noOfRawFrames != 0) && DmdScr_Render_16_Shades_with_Raw)
			DmdScr_Render_16_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
		else if (((isDMD2 & 2) != 0) && DmdScr_Render_16_Shades)
			DmdScr_Render_16_Shades(width, height, frame);
	}
	else {
		if (((isDMD2 & 1) != 0) && (noOfRawFrames != 0) && DmdDev_Render_4_Shades_with_Raw)
			DmdDev_Render_4_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
		else if (((isDMD2 & 1) != 0) && DmdDev_Render_4_Shades)
			DmdDev_Render_4_Shades(width, height, frame);
		if (((isDMD2 & 2) != 0) && (noOfRawFrames != 0) && DmdScr_Render_4_Shades_with_Raw)
			DmdScr_Render_4_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
		else if (((isDMD2 & 2) != 0) && DmdScr_Render_4_Shades)
			DmdScr_Render_4_Shades(width, height, frame);
	}
	if (core_gameData->gen & GEN_GTS3) {
		free(frame);
	}
}

void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, char *seg_dim, UINT8 total_disp, UINT8 *disp_num_segs, const char* GameName) {

	// Some GTS3 games like Teed Off update both empty alpha and real DMD.   If a DMD frame has been seen, 
	// block this from running.
	if (dmd_hasDMD)
		return;

	const layout_t layout = layoutAlphanumericFrame(gen, seg_data, seg_data2, total_disp, disp_num_segs, GameName);

	if (layout == __Invalid)
		return;

	if (DmdDev_Render_PM_Alphanumeric_Dim_Frame)
		DmdDev_Render_PM_Alphanumeric_Dim_Frame(layout, seg_data, seg_dim, seg_data2);
	else if (DmdDev_Render_PM_Alphanumeric_Frame) // older interface without dimming
		DmdDev_Render_PM_Alphanumeric_Frame(layout, seg_data, seg_data2);

	if (DmdScr_Render_PM_Alphanumeric_Dim_Frame)
		DmdScr_Render_PM_Alphanumeric_Dim_Frame(layout, seg_data, seg_dim, seg_data2);
	else if (DmdScr_Render_PM_Alphanumeric_Frame) // older interface without dimming
		DmdScr_Render_PM_Alphanumeric_Frame(layout, seg_data, seg_data2);
}

#endif
