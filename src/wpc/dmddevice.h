#pragma once

// Utility function to help rendering an alphanumeric display on a standard 128x32 DMD display

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;

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


layout_t layoutAlphanumericFrame(UINT64 gen, UINT16* seg_data, UINT16* seg_data_2, UINT8 total_disp, UINT8* disp_num_segs, const char* GameName);

// VPinMame function to send DMD/Alphanumeric information to an external dmddevice/dmdscreen.dll plugin
// Note that this part of the header is not used externally of VPinMame (move it to something like core_dmdevice.h/core_dmddevice.c ?)
#ifdef VPINMAME
int pindmdInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions* Options);
void renderDMDFrame(const int width, const int height, UINT8* dmdDotLum, UINT8* dmdDotRaw, UINT32 noOfRawFrames, UINT8* rawbuffer, const int isDMD2);
void renderAlphanumericFrame(UINT64 gen, UINT16* seg_data, char* seg_dim, UINT8 total_disp, UINT8* disp_num_segs, const char* GameName);
void FwdConsoleData(UINT8 data);
void pindmdDeInit(void);
#endif


#ifdef __cplusplus
}
#endif
