// Support for VPinMame plugins through dynamic loading of dmddevice/dmdscreen.dll
// See ext/dmddevice folder for example and main header

extern "C"
{
#include "gen.h"
#include "driver.h"
#include "core.h"
}

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

// Introduced in Windows 7 (not supported in Windows XP)
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#endif

// Introduced in Windows 7 (not supported in Windows XP)
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000
#endif

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;

typedef int(*DmdDev_Open_t)();
typedef bool(*DmdDev_Close_t)();
typedef void(*DmdDev_PM_GameSettings_t)(const char* GameName, UINT64 HardwareGeneration, const tPMoptions& Options);
typedef void(*DmdDev_Set_4_Colors_Palette_t)(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100);
typedef void(*DmdDev_Console_Data_t)(UINT8 data);
typedef int(*DmdDev_Console_Input_t)(UINT8* buf, int size);
typedef int(*DmdDev_Console_Input_Ptr_t)(DmdDev_Console_Input_t ptr);
typedef void(*DmdDev_Render_16_Shades_t)(UINT16 width, UINT16 height, UINT8* frame);
typedef void(*DmdDev_Render_4_Shades_t)(UINT16 width, UINT16 height, UINT8* frame);
typedef void(*DmdDev_Render_16_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8* frame, UINT32 noOfRawFrames, UINT8* rawbuffer);
typedef void(*DmdDev_Render_4_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8* frame, UINT32 noOfRawFrames, UINT8* rawbuffer);
typedef void(*DmdDev_Render_PM_Alphanumeric_Frame_t)(core_segOverallLayout_t layout, const UINT16* const seg_data, const UINT16* const seg_data2);
typedef void(*DmdDev_Render_PM_Alphanumeric_Dim_Frame_t)(core_segOverallLayout_t layout, const UINT16* const seg_data, const char* const seg_dim, const UINT16* const seg_data2);
typedef void(*DmdDev_Render_Lum_And_Raw_t)(UINT16 width, UINT16 height, UINT8* lumFrame, UINT8* rawFrame, UINT32 noOfRawFrames, UINT8* rawbuffer);

typedef struct {
	HMODULE hModule;
	// Setup
	DmdDev_Open_t Open;
	DmdDev_Close_t Close;
	DmdDev_PM_GameSettings_t PM_GameSettings;
	DmdDev_Set_4_Colors_Palette_t Set_4_Colors_Palette;
	// COnsole input/output for SAM hardware
	DmdDev_Console_Data_t Console_Data;
	DmdDev_Console_Input_Ptr_t Console_Input_Ptr;
	// DMD rendering
	DmdDev_Render_16_Shades_t Render_16_Shades;
	DmdDev_Render_4_Shades_t Render_4_Shades;
	DmdDev_Render_16_Shades_with_Raw_t Render_16_Shades_with_Raw;
	DmdDev_Render_4_Shades_with_Raw_t Render_4_Shades_with_Raw;
	DmdDev_Render_Lum_And_Raw_t Render_Lum_And_Raw;
	// ALphanum rendering
	DmdDev_Render_PM_Alphanumeric_Frame_t Render_PM_Alphanumeric_Frame;
	DmdDev_Render_PM_Alphanumeric_Dim_Frame_t Render_PM_Alphanumeric_Dim_Frame;
} dmddevice_t;

static bool        dmd_hasDMD = false; // Used to send a black frame for clean close
static UINT16      dmd_width = 0; // Only valid if dmd_hasDMD is set
static UINT16      dmd_height = 0; // Only valid if dmd_hasDMD is set
static dmddevice_t dmdDevices[2] = { {0} };

extern "C"
{
	#include "cpu/at91/at91.h"
	int at91_receive_serial(int usartno, data8_t* buf, int size);
}

extern "C" void dmddeviceFwdConsoleData(UINT8 data) {
	for (int i = 0; i < 2; i++)
		if (dmdDevices[i].Console_Data)
			dmdDevices[i].Console_Data(data);
}

extern "C" int dmddeviceRcvConsoleInput(UINT8* buf, int size)
{
	const int ret = at91_receive_serial(1, buf, size);
	return ret;
}

extern "C" int dmddeviceInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions* Options)
{
	dmd_hasDMD = false;
	memset(dmdDevices, 0, sizeof(dmdDevices));

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

	HMODULE hCurrentModule = NULL;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)dmddeviceInit, &hCurrentModule);

	bool dmddeviceFound = false;
	for (int i = 0; i < 2; i++)
	{
		// look for the DmdDevice(64).dll and DmdScreen(64).dll in the path of vpinmame.dll/libpinmame-X.X.dll
		char filename[MAX_PATH];
		GetModuleFileName(hCurrentModule, filename, MAX_PATH);
		strcpy(filename, filename);
		char* ptr = strrchr(filename, '\\');
#ifdef _WIN64
		strcpy(ptr + 1, i == 0 ? "DmdDevice64.dll" : "DmdScreen64.dll");
#else
		strcpy(ptr + 1, i == 0 ? "DmdDevice.dll" : "DmdScreen.dll");
#endif
		dmdDevices[i].hModule = LoadLibraryEx(filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (!dmdDevices[i].hModule) {
#ifdef _WIN64
			dmdDevices[i].hModule = LoadLibrary(i == 0 ? "DmdDevice64.dll" : "DmdScreen64.dll");
#else
			dmdDevices[i].hModule = LoadLibrary(i == 0 ? "DmdDevice.dll" : "DmdScreen.dll");
#endif
		}
		if (dmdDevices[i].hModule) {
			dmdDevices[i].Open = (DmdDev_Open_t)GetProcAddress(dmdDevices[i].hModule, "Open");
			dmdDevices[i].PM_GameSettings = (DmdDev_PM_GameSettings_t)GetProcAddress(dmdDevices[i].hModule, "PM_GameSettings");
			dmdDevices[i].Close = (DmdDev_Close_t)GetProcAddress(dmdDevices[i].hModule, "Close");
			dmdDevices[i].Render_4_Shades = (DmdDev_Render_4_Shades_t)GetProcAddress(dmdDevices[i].hModule, "Render_4_Shades");
			dmdDevices[i].Render_16_Shades = (DmdDev_Render_16_Shades_t)GetProcAddress(dmdDevices[i].hModule, "Render_16_Shades");
			dmdDevices[i].Render_4_Shades_with_Raw = (DmdDev_Render_4_Shades_with_Raw_t)GetProcAddress(dmdDevices[i].hModule, "Render_4_Shades_with_Raw");
			dmdDevices[i].Render_16_Shades_with_Raw = (DmdDev_Render_16_Shades_with_Raw_t)GetProcAddress(dmdDevices[i].hModule, "Render_16_Shades_with_Raw");
			dmdDevices[i].Render_Lum_And_Raw = (DmdDev_Render_Lum_And_Raw_t)GetProcAddress(dmdDevices[i].hModule, "Render_Lum_And_Raw");
			dmdDevices[i].Render_PM_Alphanumeric_Frame = (DmdDev_Render_PM_Alphanumeric_Frame_t)GetProcAddress(dmdDevices[i].hModule, "Render_PM_Alphanumeric_Frame");
			dmdDevices[i].Render_PM_Alphanumeric_Dim_Frame = (DmdDev_Render_PM_Alphanumeric_Dim_Frame_t)GetProcAddress(dmdDevices[i].hModule, "Render_PM_Alphanumeric_Dim_Frame");
			dmdDevices[i].Set_4_Colors_Palette = (DmdDev_Set_4_Colors_Palette_t)GetProcAddress(dmdDevices[i].hModule, "Set_4_Colors_Palette");
			dmdDevices[i].Console_Data = (DmdDev_Console_Data_t)GetProcAddress(dmdDevices[i].hModule, "Console_Data");
			dmdDevices[i].Console_Input_Ptr = (DmdDev_Console_Input_Ptr_t)GetProcAddress(dmdDevices[i].hModule, "Console_Input_Ptr");
			if (dmdDevices[i].Open && dmdDevices[i].Close && dmdDevices[i].PM_GameSettings && dmdDevices[i].Render_4_Shades && dmdDevices[i].Render_16_Shades && dmdDevices[i].Render_PM_Alphanumeric_Frame) {
				dmddeviceFound = true;
				dmdDevices[i].Open();
				if (dmdDevices[i].Console_Input_Ptr)
					dmdDevices[i].Console_Input_Ptr(dmddeviceRcvConsoleInput);
				if (dmdDevices[i].Set_4_Colors_Palette && Options->dmd_colorize)
					dmdDevices[i].Set_4_Colors_Palette(color0, color33, color66, color100);
				dmdDevices[i].PM_GameSettings(GameName, HardwareGeneration, *Options);
			}
		}
	}
	if (!dmddeviceFound) {
		MessageBox(NULL, "No external DMD driver found or DMD driver functions not found", "Visual PinMame Error", MB_ICONERROR);
		return 0;
	}
	return 1;
}

extern "C" void dmddeviceDeInit() {
	for (int i = 0; i < 2; i++)
	{
		if (dmd_hasDMD && dmdDevices[i].Render_4_Shades) {
			UINT8* tmpbuffer = (UINT8*)malloc(dmd_width * dmd_height);
			memset(tmpbuffer, 0x00, dmd_width * dmd_height);
			dmdDevices[i].Render_4_Shades(dmd_width, dmd_height, tmpbuffer); //clear screen
			free(tmpbuffer);
		}
		if (dmdDevices[i].Close)
			dmdDevices[i].Close();
		if (dmdDevices[i].hModule)
			FreeLibrary(dmdDevices[i].hModule);
	}
}

extern "C" void dmddeviceRenderDMDFrame(const int width, const int height, UINT8* dmdDotLum, UINT8* dmdDotRaw, UINT32 noOfRawFrames, UINT8* rawbuffer, const int isDMD2) {
	dmd_width = width; // store for DeInit
	dmd_height = height;
	dmd_hasDMD = true;
	for (int i = 0; i < 2; i++)
	{
		if ((isDMD2 & (1 << i)) == 0)
			continue;
		if (dmdDevices[i].Render_Lum_And_Raw)
		{
			// New implementation that sends both luminance information for rendering, and combined bitplanes as well as raw frames for frame identification for colorization & triggering events
			dmdDevices[i].Render_Lum_And_Raw(width, height, dmdDotLum, dmdDotRaw, noOfRawFrames, rawbuffer);
		}
		else
		{
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
				for (int i2 = 0; i2 < width * height; i2++)
					frame[i2] = dmdDotLum[i2] >> shift;
			}
			if (is16Shades) {
				if ((noOfRawFrames != 0) && dmdDevices[i].Render_16_Shades_with_Raw)
					dmdDevices[i].Render_16_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
				else if (dmdDevices[i].Render_16_Shades)
					dmdDevices[i].Render_16_Shades(width, height, frame);
			}
			else {
				if ((noOfRawFrames != 0) && dmdDevices[i].Render_4_Shades_with_Raw)
					dmdDevices[i].Render_4_Shades_with_Raw(width, height, frame, noOfRawFrames, rawbuffer);
				else if (dmdDevices[i].Render_4_Shades)
					dmdDevices[i].Render_4_Shades(width, height, frame);
			}
			if (core_gameData->gen & GEN_GTS3)
				free(frame);
		}
	}
}

extern "C" void dmddeviceRenderAlphanumericFrame(core_segOverallLayout_t layout, UINT16* seg_data, UINT16* seg_data2, char* seg_dim) {
	for (int i = 0; i < 2; i++)
	{
		if (dmdDevices[i].Render_PM_Alphanumeric_Dim_Frame)
			dmdDevices[i].Render_PM_Alphanumeric_Dim_Frame(layout, seg_data, seg_dim, seg_data2);
		else if (dmdDevices[i].Render_PM_Alphanumeric_Frame) // older interface without dimming
			dmdDevices[i].Render_PM_Alphanumeric_Frame(layout, seg_data, seg_data2);
	}
}
