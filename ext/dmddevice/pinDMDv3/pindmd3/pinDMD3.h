#ifndef INC_USBDMD
#define INC_USBDMD

#ifdef _WIN64
	#ifdef PINDMD_EXPORTS
	#define PINDMD_API __declspec(dllexport) 
	#else
	#define PINDMD_API __declspec(dllimport) 
	#endif
#else
	#define PINDMD_API
#endif



#define SETTING_DEBUG             0x22
#define SETTING_BRIGHTNESS        0x23
#define SETTING_4SHADES           0x24
#define SETTING_16SHADES          0x25
#define SETTING_RAINBOW_SPEED     0x26

/*typedef struct tPMoptions {
	int dmd_red, dmd_green, dmd_blue;
	int dmd_perc66, dmd_perc33, dmd_perc0;
	int dmd_only, dmd_compact, dmd_antialias;
	int dmd_colorize;
	int dmd_red66, dmd_green66, dmd_blue66;
	int dmd_red33, dmd_green33, dmd_blue33;
	int dmd_red0, dmd_green0, dmd_blue0;
} tPMoptions;

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;*/

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

#ifdef __cplusplus
extern "C"
{
#endif

PINDMD_API int  pindmdInit(tPMoptions colours);
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

#ifdef __cplusplus
}
#endif

#endif
