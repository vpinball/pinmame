#include "windows.h"
#include "stdafx.h"

extern UINT8 AlphaNumericFrameBuffer[];

extern void _2x16Alpha(UINT16 *seg_data);
extern void _2x20Alpha(UINT16 *seg_data);
extern void _2x7Alpha_2x7Num(UINT16 *seg_data);
extern void	_2x7Alpha_2x7Num_4x1Num(UINT16 *seg_data);
extern void _2x7Num_2x7Num_4x1Num(UINT16 *seg_data);
extern void _2x7Num_2x7Num_10x1Num(UINT16 *seg_data, UINT16 *extra_seg_data);
extern void _2x7Num_2x7Num_4x1Num_gen7(UINT16 *seg_data);
extern void	_2x7Num10_2x7Num10_4x1Num(UINT16 *seg_data);
extern void _2x6Num_2x6Num_4x1Num(UINT16 *seg_data);
extern void _2x6Num10_2x6Num10_4x1Num(UINT16 *seg_data);
extern void _4x7Num10(UINT16 *seg_data);
extern void _6x4Num_4x1Num(UINT16 *seg_data);
extern void _2x7Num_4x1Num_1x16Alpha(UINT16 *seg_data);
extern void _1x16Alpha_1x16Num_1x7Num(UINT16 *seg_data);
extern UINT8 getPixel(int x, int y);
extern void drawPixel(int x, int y, UINT8 colour);
extern void smoothDigitCorners(int x, int y);


