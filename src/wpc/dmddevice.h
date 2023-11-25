#pragma once

#ifdef WIN32

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

int pindmdInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions *Options);
void pindmdDeInit(void);
void renderDMDFrame(UINT64 gen, UINT16 width, UINT16 height, UINT8 *currbuffer, UINT8 doDumpFrame, const char* GameName, UINT32 noOfRawFrames, UINT8 *rawbuffer);
void render2ndDMDFrame(UINT64 gen, UINT16 width, UINT16 height, UINT8 *currbuffer, UINT8 doDumpFrame, const char* GameName, UINT32 noOfRawFrames, UINT8 *rawbuffer);
layout_t layoutAlphanumericFrame(UINT64 gen, UINT16* seg_data, UINT16* seg_data_2, UINT8 total_disp, UINT8* disp_num_segs, const char* GameName);
void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, char *seg_dim, UINT8 total_disp, UINT8 *disp_num_segs, const char* GameName);
void FwdConsoleData(UINT8 data);

#ifdef __cplusplus
}
#endif

#endif
