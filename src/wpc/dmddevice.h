#pragma once

#ifdef WIN32

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;

typedef enum {
	None,
	_2x16Alpha, 
	_2x20Alpha, 
	_2x7Alpha_2x7Num, 
	_2x7Alpha_2x7Num_4x1Num, 
	_2x7Num_2x7Num_4x1Num, 
	_2x7Num_2x7Num_10x1Num, 
	_2x7Num_2x7Num_4x1Num_gen7, 
	_2x7Num10_2x7Num10_4x1Num,
	_2x6Num_2x6Num_4x1Num,
	_2x6Num10_2x6Num10_4x1Num,
	_4x7Num10,
	_6x4Num_4x1Num,
	_2x7Num_4x1Num_1x16Alpha,
	_1x16Alpha_1x16Num_1x7Num
} layout_t;


#ifdef __cplusplus
extern "C"
{
#endif

int pindmdInit(const char* GameName, UINT64 HardwareGeneration, const tPMoptions *Options);
void pindmdDeInit(void);
void renderDMDFrame(UINT64 gen, UINT16 width, UINT16 height, UINT8 *currbuffer, UINT8 doDumpFrame, const char* GameName, UINT32 noOfRawFrames, UINT8 *rawbuffer);
void renderAlphanumericFrame(UINT64 gen, UINT16 *seg_data, char *seg_dim, UINT8 total_disp, UINT8 *disp_num_segs);
void FwdConsoleData(UINT8 data);

#ifdef __cplusplus
}
#endif

#endif
