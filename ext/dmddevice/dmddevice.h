// license:BSD-3-Clause

#ifdef DMDDEVICE_DLL_EXPORTS
	#define DMDDEV __declspec(dllexport)
#else
	#define DMDDEV __declspec(dllimport)
#endif

typedef struct tPMoptions {
	int dmd_red, dmd_green, dmd_blue;
	int dmd_perc66, dmd_perc33, dmd_perc0;
	int dmd_only, dmd_compact, dmd_antialias;
	int dmd_colorize;
	int dmd_red66, dmd_green66, dmd_blue66;
	int dmd_red33, dmd_green33, dmd_blue33;
	int dmd_red0, dmd_green0, dmd_blue0;
} tPMoptions;

typedef struct rgb24 {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} rgb24;

#define numberOfColorsPerPalette 16

typedef struct palStruct {
	unsigned char index;
	unsigned char rgbData[numberOfColorsPerPalette * 3];
	struct palStruct* next;
} Palette;

UINT8 *OutputPacketBuffer;

typedef int(*Console_Input_t)(UINT8 *buf, int size);
Console_Input_t Console_Input = NULL;

typedef enum {
	__None,
	__2x16Alpha,
	__2x20Alpha,
	__2x7Alpha_2x7Num,
	__2x7Alpha_2x7Num_4x1Num,
	__2x7Num_2x7Num_4x1Num,
	__2x7Num_2x7Num_10x1Num,
	__2x7Num_2x7Num_4x1Num_gen7,
	__2x7Num10_2x7Num10_4x1Num,
	__2x6Num_2x6Num_4x1Num,
	__2x6Num10_2x6Num10_4x1Num,
	__4x7Num10,
	__6x4Num_4x1Num,
	__2x7Num_4x1Num_1x16Alpha,
	__1x16Alpha_1x16Num_1x7Num,
	__1x7Num_1x16Alpha_1x16Num,
	__1x16Alpha_1x16Num_1x7Num_1x4Num,
	__Invalid
} layout_t;

#ifdef __cplusplus
extern "C"
{
#endif

// ----- External device setup
DMDDEV int Open();
DMDDEV bool Close();
DMDDEV void Set_4_Colors_Palette(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100);
DMDDEV void Set_16_Colors_Palette(rgb24 *color);
DMDDEV void PM_GameSettings(const char* GameName, UINT64 HardwareGeneration, const tPMoptions &Options);

// ----- Main DMD display data
// Render_Lum_And_Raw was added in later revision of this interface and superseeds the deprecated Render_xx_Shades and Render_xx_Shades_with_Raw.
// The host will check if the newer API is implemented. If so, it will only call the new API.
// Render_xx_Shades_with_Raw were variants introduced for better frame identification. They are deprecated in favor of Render_Lum_And_Raw
DMDDEV void Render_4_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer);
DMDDEV void Render_16_Shades(UINT16 width, UINT16 height, UINT8 *currbuffer);
//!! DMDDEV void Render_4_Shades_with_Raw(UINT16 width, UINT16 height, UINT8 *currbuffer, UINT32 noOfRawFrames, UINT8 *rawbuffer);
//!! DMDDEV void Render_16_Shades_with_Raw(UINT16 width, UINT16 height, UINT8 *currbuffer, UINT32 noOfRawFrames, UINT8 *rawbuffer);
DMDDEV void Render_RGB24(UINT16 width, UINT16 height, rgb24 *currbuffer);
//!! DMDDEV void Render_Lum_And_Raw(UINT16 width, UINT16 height, UINT8 *lumBuffer, UINT8 *identifyBuffer, UINT32 noOfRawFrames, UINT8 *rawFrames);

// ----- Alphanumeric segment data
// Render_PM_Alphanumeric_Dim_Frame was added in later revision of this interface and superseeds the deprecated Render_PM_Alphanumeric_Frame.
// The host will check if the newer API is implemented. If so, it will only call the new API.
DMDDEV void Render_PM_Alphanumeric_Frame(layout_t layout, const UINT16 *const seg_data, const UINT16 *const seg_data2);
//!! DMDDEV void Render_PM_Alphanumeric_Dim_Frame(layout_t layout, const UINT16 *const seg_data, const char *const seg_dim, const UINT16 *const seg_data2);

// ----- Console input.output
// This is implemented for SAM hardware. Input pointer was added in latest revision of this interface
DMDDEV void Console_Data(UINT8 data);
//!! DMDDEV void Console_Input_Ptr(Console_Input_t ptr);

#ifdef __cplusplus
}
#endif
