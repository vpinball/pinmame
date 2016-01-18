#ifndef INC_USBDMD
#define INC_USBDMD

#include "driver.h"

#define PINDMD2 // define for PinDMD2 support, undefine for PinDMD1 support, latter needs the separate ftd2xx.dll then!
//#define PINDMD3 // define for PinDMD3 support (needs PinDMD.dll then), undefine for PinDMD1 support, latter needs the separate ftd2xx.dll then!

#ifdef PINDMD3

#ifdef PINDMD_EXPORTS
#define PINDMD_API __declspec(dllexport) 
#else
#define PINDMD_API __declspec(dllimport) 
#endif

#define SETTING_DEBUG		      0x22
#define SETTING_BRIGHTNESS		  0x23
#define SETTING_4SHADES           0x24
#define SETTING_16SHADES          0x25
#define SETTING_RAINBOW_SPEED	  0x26

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;

typedef struct deviceInfo {
	UINT8 width;
	UINT8 height;
	char firmware[20];
} deviceInfo;

typedef struct dllInfo {
	char version[20];
} dllInfo;

typedef struct deviceSettings {
	UINT8 debugMode;
	UINT8 brightness;
	UINT8 rainbowSpeed;
	UINT8 shades4[4];
	UINT8 shades16[16];
} deviceSettings;

PINDMD_API int pindmdInit(tPMoptions colours);
PINDMD_API void pindmdDeInit(void);
PINDMD_API void renderDMDFrame(UINT64 gen, UINT8 width, UINT8 height, UINT8 *currbuffer, UINT8 doDumpFrame); // legacy pinMame
PINDMD_API void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, UINT8 total_disp, UINT8 *disp_lens);   // legacy pinMame
PINDMD_API void render16ShadeFrame(UINT8 *currbuffer);
PINDMD_API void renderRGB24Frame(rgb24 *currbuffer);
PINDMD_API void setSetting(UINT8 setting, UINT8 *params);
PINDMD_API void getSettings(deviceSettings settings);
PINDMD_API void getDeviceInfo(deviceInfo info);
PINDMD_API void getDllInfo(dllInfo info);
PINDMD_API void enableDebug(void);
PINDMD_API void disableDebug(void);
#define frameClock() (void)	// legacy pinMame

#else

extern void pindmdInit(tPMoptions colours);
extern void pindmdDeInit(void);
extern void sendLogo(void);
extern void sendColor(void);
extern void sendClearSettings(void);
extern void renderDMDFrame(UINT64 gen, UINT32 width, UINT32 height, UINT8 *currbuffer_in, UINT8 doDumpFrame);
extern void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, UINT8 total_disp, UINT8 *disp_lens);
extern void drawPixel(int x, int y, UINT8 colour);
extern UINT8 getPixel(int x, int y);
extern void dumpFrames(UINT8 *currbuffer, UINT16 buffersize, UINT8 doDumpFrame, UINT64 gen);
extern void frameClock(void);

UINT8 frame_buf[3072];

#endif

#endif
