UINT8 AlphaNumericFrameBuffer[2048] = {};

static const UINT8 segSizes[7][16] = {
	{5,5,5,5,5,5,2,2,5,5,5,2,5,5,5,1},
	{5,5,5,5,5,5,5,2,0,0,0,0,0,0,0,0},
	{5,5,5,5,5,5,5,2,5,5,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{5,2,2,5,2,2,5,2,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const UINT8 segs[7][17][5][2] = {
	/* alphanumeric display characters */
	{	{{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,0},{6,1},{6,2},{6,3},{6,4}},
		{{6,6},{6,7},{6,8},{6,9},{6,10}},
		{{1,10},{2,10},{3,10},{4,10},{5,10}},
		{{0,6},{0,7},{0,8},{0,9},{0,10}},
		{{0,0},{0,1},{0,2},{0,3},{0,4}},
		{{1,5},{2,5}},
		{{7,9},{7,10}},
		{{0,0},{1,1},{1,2},{2,3},{2,4}},
		{{3,0},{3,1},{3,2},{3,3},{3,4}},
		{{6,0},{5,1},{5,2},{4,3},{4,4}},
		{{4,5},{5,5}},
		{{4,6},{4,7},{5,8},{5,9},{6,10}},
		{{3,6},{3,7},{3,8},{3,9},{3,10}},
		{{0,10},{2,6},{2,7},{1,8},{1,9}},
		{{7,10}}
	}, 
	/* 8 segment LED characters */
	{ {{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,0},{6,1},{6,2},{6,3},{6,4}},
		{{6,6},{6,7},{6,8},{6,9},{6,10}},
		{{1,10},{2,10},{3,10},{4,10},{5,10}},
		{{0,6},{0,7},{0,8},{0,9},{0,10}},
		{{0,0},{0,1},{0,2},{0,3},{0,4}},
		{{1,5},{2,5},{3,5},{4,5},{5,5}},
		{{7,9},{7,10}}
	},
	/* 10 segment LED characters */
	{ {{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,0},{6,1},{6,2},{6,3},{6,4}},
		{{6,6},{6,7},{6,8},{6,9},{6,10}},
		{{1,10},{2,10},{3,10},{4,10},{5,10}},
		{{0,6},{0,7},{0,8},{0,9},{0,10}},
		{{0,0},{0,1},{0,2},{0,3},{0,4}},
		{{1,5},{2,5},{3,5},{4,5},{5,5}},
		{{7,9},{7,10}},
		{{3,0},{3,1},{3,2},{3,3},{3,4}},
		{{3,6},{3,7},{3,8},{3,9},{3,10}}
	},
	/* alphanumeric display characters (reversed comma with period) */
	{	{{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,0},{6,1},{6,2},{6,3},{6,4}},
		{{6,6},{6,7},{6,8},{6,9},{6,10}},
		{{1,10},{2,10},{3,10},{4,10},{5,10}},
		{{0,6},{0,7},{0,8},{0,9},{0,10}},
		{{0,0},{0,1},{0,2},{0,3},{0,4}},
		{{1,5},{2,5}},
		{{7,9},{7,10}},
		{{0,0},{1,1},{1,2},{2,3},{2,4}},
		{{3,0},{3,1},{3,2},{3,3},{3,4}},
		{{6,0},{5,1},{5,2},{4,3},{4,4}},
		{{4,5},{5,5}},
		{{4,6},{4,7},{5,8},{5,9},{6,10}},
		{{3,6},{3,7},{3,8},{3,9},{3,10}},
		{{0,10},{2,6},{2,7},{1,8},{1,9}},
		{{7,10}}
	}, 
	/* 8 segment LED characters with dots instead of commas */
	{ {{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,0},{6,1},{6,2},{6,3},{6,4}},
		{{6,6},{6,7},{6,8},{6,9},{6,10}},
		{{1,10},{2,10},{3,10},{4,10},{5,10}},
		{{0,6},{0,7},{0,8},{0,9},{0,10}},
		{{0,0},{0,1},{0,2},{0,3},{0,4}},
		{{1,5},{2,5},{3,5},{4,5},{5,5}},
		{{7,9},{7,10}}
	},
	/* 8 segment LED characters SMALL */
	{ {{1,0},{2,0},{3,0},{4,0},{5,0}},
		{{6,1},{6,2}},
		{{6,4},{6,5}},
		{{1,6},{2,6},{3,6},{4,6},{5,6}},
		{{0,4},{0,5}},
		{{0,1},{0,2}},
		{{1,3},{2,3},{3,3},{4,3},{5,3}},
		{{7,5},{7,6}}
	},
	/* 10 segment LED characters SMALL */
	{ {{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}},
		{{0,0},{0,0},{0,0},{0,0},{0,0}}
	}
	// add 10seg small
};


//*****************************************************
//* Name:			getPixel
//* Purpose:
//* In:
//* Out:
//*****************************************************
UINT8 getPixel(const int x, const int y)
{
   const int v = (y*16)+(x/8);
   const int z = 1<<(x%8);
   // just check high buff

   return ((AlphaNumericFrameBuffer[v+512]&z)!=0);
}

//*****************************************************
//* Name:			drawPixel
//* Purpose:
//* In:
//* Out:
//*****************************************************
void drawPixel(const int x, const int y, const UINT8 colour)
{
   const int v = (y*16)+(x/8);
   const int z = 1<<(x%8);
   // clear both low and high buffer pixel
   AlphaNumericFrameBuffer[v] |= z;
   AlphaNumericFrameBuffer[v+512] |= z;
   AlphaNumericFrameBuffer[v] ^= z;
   AlphaNumericFrameBuffer[v+512] ^= z;
#ifdef PINDMD1
   if(do16==1)
#endif
   {
	AlphaNumericFrameBuffer[v+1024] |= z;
	AlphaNumericFrameBuffer[v+1536] |= z;
	AlphaNumericFrameBuffer[v+1024] ^= z;
	AlphaNumericFrameBuffer[v+1536] ^= z;
   }
   // set low buffer pixel
   if (colour & 1)
      AlphaNumericFrameBuffer[v] |= z;
   //set high buffer pixel
   if (colour & 2)
      AlphaNumericFrameBuffer[v+512] ^= z;
#ifdef PINDMD1
   if(do16==1)
#endif
   if (colour!=0){
		AlphaNumericFrameBuffer[v+1024] |= z;
		AlphaNumericFrameBuffer[v+1536] ^= z;
   }
}

//*****************************************************
//* Name:			drawSegment
//* Purpose:
//* In:
//* Out:
//*****************************************************
void drawSegment(const int x, const int y, const UINT8 type, const UINT16 seg, const UINT8 colour)
{
	for (int i = 0; i<segSizes[type][seg]; i++)
		drawPixel(segs[type][seg][i][0] + x, segs[type][seg][i][1] + y, colour);
}

//*****************************************************
//* Name:			smoothDigitCorners
//* Purpose:
//* In:
//* Out:
//*****************************************************
void smoothDigitCorners(const int x, const int y) {
	// remove corner pixel (round font corners)
	if(getPixel(x,1+y) && getPixel(1+x,y))
		drawPixel(0+x,y,0);
	if(getPixel(x+6,1+y) && getPixel(5+x,y))
		drawPixel(6+x,y,0);
	if(getPixel(x,9+y) && getPixel(1+x,10+y))
		drawPixel(0+x,10+y,0);
	if(getPixel(x+6,9+y) && getPixel(5+x,10+y))
		drawPixel(6+x,10+y,0);
}

//*****************************************************
//* Name:			_2x16Alpha
//* Purpose:	2 X 16 AlphaNumeric Rows
//* In:
//* Out:
//*****************************************************
void _2x16Alpha(const UINT16 *const seg_data)
{
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment(i*8, 2, 0, j, 3);
			if ((seg_data[i+16] >> j) & 0x1) 
				drawSegment(i*8, 19, 0, j, 3);
			smoothDigitCorners(i*8,2);
			smoothDigitCorners(i*8,19);
		}
	}
}

//*****************************************************
//* Name:			_2x20Alpha
//* Purpose:	2 X 20 AlphaNumeric Rows
//* In:
//* Out:
//*****************************************************
void _2x20Alpha(const UINT16 *const seg_data)
{
	for (int i=0; i<16; i++) { //!! misses 4 chars in each row
		for (int j=0; j<16; j++) {
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment(i*8, 2, 0, j, 3);
			if ((seg_data[i+20] >> j) & 0x1) 
				drawSegment(i*8, 19, 0, j, 3);
			smoothDigitCorners(i*8,2);
			smoothDigitCorners(i*8,19);
		}
	}
}

//*****************************************************
//* Name:			_2x7Alpha_2x7Num
//* Purpose:	2 X 7 AlphaNumeric Rows
//*						2 X 7 Numeric
//* In:
//* Out:
//*****************************************************
void _2x7Alpha_2x7Num(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 alphanumeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 2, 0, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,2);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 19, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,19);
		}
	}
}

//*****************************************************
//* Name:			_2x7Alpha_2x7Num_4x1Num
//* Purpose:	2 X 7 AlphaNumeric Rows
//*						2 X 7 Numeric
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x7Alpha_2x7Num_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 alphanumeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 0, 0, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,0);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 21, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,21);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[28] >> j) & 0x1) 
			drawSegment(8, 12, 5, j, 3);
		if ((seg_data[29] >> j) & 0x1) 
			drawSegment(16, 12, 5, j, 3);
		if ((seg_data[30] >> j) & 0x1) 
			drawSegment(32, 12, 5, j, 3);
		if ((seg_data[31] >> j) & 0x1) 
			drawSegment(40, 12, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num
//* Purpose:	2 X 7 Numeric
//*						2 X 7 Numeric
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x6Num_2x6Num_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<12; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<6)?0:4))*8, 0, 1, j, 3);
			smoothDigitCorners((i+((i<6)?0:4))*8,0);
			// 2x7 numeric
			if ((seg_data[i+12] >> j) & 0x1) 
				drawSegment((i+((i<6)?0:4))*8, 12, 1, j, 3);
			smoothDigitCorners((i+((i<6)?0:4))*8,12);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[24] >> j) & 0x1) 
			drawSegment(8, 24, 5, j, 3);
		if ((seg_data[25] >> j) & 0x1) 
			drawSegment(16, 24, 5, j, 3);
		if ((seg_data[26] >> j) & 0x1) 
			drawSegment(32, 24, 5, j, 3);
		if ((seg_data[27] >> j) & 0x1) 
			drawSegment(40, 24, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num
//* Purpose:	2 X 6 Numeric10
//*						2 X 6 Numeric10
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x6Num10_2x6Num10_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<12; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<6)?0:4))*8, 0, 2, j, 3);
			smoothDigitCorners((i+((i<6)?0:4))*8,0);
			// 2x7 numeric
			if ((seg_data[i+12] >> j) & 0x1) 
				drawSegment((i+((i<6)?0:4))*8, 20, 2, j, 3);
			smoothDigitCorners((i+((i<6)?0:4))*8,20);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[24] >> j) & 0x1) 
			drawSegment(8, 12, 5, j, 3);
		if ((seg_data[25] >> j) & 0x1) 
			drawSegment(16, 12, 5, j, 3);
		if ((seg_data[26] >> j) & 0x1) 
			drawSegment(32, 12, 5, j, 3);
		if ((seg_data[27] >> j) & 0x1) 
			drawSegment(40, 12, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num
//* Purpose:	2 X 7 Numeric
//*						2 X 7 Numeric
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x7Num_2x7Num_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 0, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,0);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 12, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,12);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[28] >> j) & 0x1) 
			drawSegment(16, 24, 5, j, 3);
		if ((seg_data[29] >> j) & 0x1) 
			drawSegment(24, 24, 5, j, 3);
		if ((seg_data[30] >> j) & 0x1) 
			drawSegment(40, 24, 5, j, 3);
		if ((seg_data[31] >> j) & 0x1) 
			drawSegment(48, 24, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_8x1Num
//* Purpose:	2 X 7 Numeric
//*						2 X 7 Numeric
//*						10 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x7Num_2x7Num_10x1Num(const UINT16 *const seg_data, const UINT16 *const extra_seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 0, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,0);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 12, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,12);
		}
	}
	// 10x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[28] >> j) & 0x1) 
			drawSegment(16, 24, 5, j, 3);
		if ((seg_data[29] >> j) & 0x1) 
			drawSegment(24, 24, 5, j, 3);
		if ((seg_data[30] >> j) & 0x1) 
			drawSegment(40, 24, 5, j, 3);
		if ((seg_data[31] >> j) & 0x1) 
			drawSegment(48, 24, 5, j, 3);
		if ((extra_seg_data[0] >> j) & 0x1) 
			drawSegment(64, 24, 5, j, 3);
		if ((extra_seg_data[1] >> j) & 0x1) 
			drawSegment(72, 24, 5, j, 3);
		if ((extra_seg_data[2] >> j) & 0x1) 
			drawSegment(88, 24, 5, j, 3);
		if ((extra_seg_data[3] >> j) & 0x1) 
			drawSegment(96, 24, 5, j, 3);
		if ((extra_seg_data[4] >> j) & 0x1) 
			drawSegment(112, 24, 5, j, 3);
		if ((extra_seg_data[5] >> j) & 0x1) 
			drawSegment(120, 24, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num_gen7
//* Purpose:	2 X 7 Numeric
//*						4 X 1 Numeric
//*						2 X 7 Numeric
//*						
//* In:
//* Out:
//*****************************************************
void _2x7Num_2x7Num_4x1Num_gen7(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 21, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,21);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 1, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,1);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[28] >> j) & 0x1) 
			drawSegment(8, 13, 5, j, 3);
		if ((seg_data[29] >> j) & 0x1) 
			drawSegment(16, 13, 5, j, 3);
		if ((seg_data[30] >> j) & 0x1) 
			drawSegment(32, 13, 5, j, 3);
		if ((seg_data[31] >> j) & 0x1) 
			drawSegment(40, 13, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num10_2x7Num10_4x1Num
//* Purpose:	2 X 7 Numeric10
//*						2 X 7 Numeric10
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _2x7Num10_2x7Num10_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 0, 2, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,0);
			// 2x7 numeric
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 20, 2, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,20);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[28] >> j) & 0x1) 
			drawSegment(8, 12, 5, j, 3);
		if ((seg_data[29] >> j) & 0x1) 
			drawSegment(16, 12, 5, j, 3);
		if ((seg_data[30] >> j) & 0x1) 
			drawSegment(32, 12, 5, j, 3);
		if ((seg_data[31] >> j) & 0x1) 
			drawSegment(40, 12, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num
//* Purpose:	4 X 7 Numeric10
//* In:
//* Out:
//*****************************************************
void _4x7Num10(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric10
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 1, 2, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,1);
			// 2x7 numeric10
			if ((seg_data[i+14] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 13, 2, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,13);
		}
	}
}

//*****************************************************
//* Name:			_2x7Num_2x7Num_4x1Num
//* Purpose:	6 X 4 Numeric
//*						4 X 1 Numeric
//* In:
//* Out:
//*****************************************************
void _6x4Num_4x1Num(const UINT16 *const seg_data)
{
	for (int i=0; i<8; i++) {
		for (int j=0; j<16; j++) {
			// 2x4 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<4)?0:2))*8, 1, 5, j, 3);
			smoothDigitCorners((i+((i<4)?0:2))*8,1);
			// 2x4 numeric
			if ((seg_data[i+8] >> j) & 0x1) 
				drawSegment((i+((i<4)?0:2))*8, 9, 5, j, 3);
			smoothDigitCorners((i+((i<4)?0:2))*8,1);
			// 2x4 numeric
			if ((seg_data[i+16] >> j) & 0x1) 
				drawSegment((i+((i<4)?0:2))*8, 17, 5, j, 3);
			smoothDigitCorners((i+((i<4)?0:2))*8,1);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[24] >> j) & 0x1) 
			drawSegment(16, 25, 5, j, 3);
		if ((seg_data[25] >> j) & 0x1) 
			drawSegment(24, 25, 5, j, 3);
		if ((seg_data[26] >> j) & 0x1) 
			drawSegment(48, 25, 5, j, 3);
		if ((seg_data[27] >> j) & 0x1) 
			drawSegment(56, 25, 5, j, 3);
	}
}

//*****************************************************
//* Name:			_2x7Num_4x1Num_1x16Alpha
//* Purpose:	2 X 7 Numeric
//*						4 X 1 Numeric
//*						1 X 16 Alphanumeric
//* In:
//* Out:
//*****************************************************
void _2x7Num_4x1Num_1x16Alpha(const UINT16 *const seg_data)
{
	for (int i=0; i<14; i++) {
		for (int j=0; j<16; j++) {
			// 2x7 numeric
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i+((i<7)?0:2))*8, 0, 1, j, 3);
			smoothDigitCorners((i+((i<7)?0:2))*8,0);
		}
	}
	// 4x1 numeric small
	for (int j=0; j<16; j++) {
		if ((seg_data[14] >> j) & 0x1) 
			drawSegment(16, 12, 5, j, 3);
		if ((seg_data[15] >> j) & 0x1) 
			drawSegment(24, 12, 5, j, 3);
		if ((seg_data[16] >> j) & 0x1) 
			drawSegment(40, 12, 5, j, 3);
		if ((seg_data[17] >> j) & 0x1) 
			drawSegment(48, 12, 5, j, 3);
	}
	// 1x16 alphanumeric
	for (int i=0; i<12; i++) {
		for (int j=0; j<16; j++) {
			if ((seg_data[i+18] >> j) & 0x1) 
				drawSegment((i*8)+16, 21, 0, j, 3);
			smoothDigitCorners((i*8)+16,21);
		}
	}
}

//*****************************************************
//* Name:			_1x16Alpha_1x16Num_1x7Num
//* Purpose:	1 X 16 Alphanumeric
//*						1 X 16 Numeric
//*						1 X 8 Numeric 
//* In:
//* Out:
//*****************************************************
void _1x16Alpha_1x16Num_1x7Num(const UINT16 * const seg_data)
{
	// 1x16 alphanumeric
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			if ((seg_data[i] >> j) & 0x1) 
				drawSegment((i*8), 1, 0, j, 3);
			smoothDigitCorners((i*8),1);
		}
	}

	// 1x16 numeric
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			if ((seg_data[i+16] >> j) & 0x1) 
				drawSegment((i*8), 21, 1, j, 3);
			smoothDigitCorners((i*8),21);
		}
	}

	// 1x7 numeric small
	for (int i=0; i<7; i++) {
		for (int j=0; j<16; j++) {
			if ((seg_data[i+32] >> j) & 0x1) 
				drawSegment(i*8, 13, 5, j, 3);
		}
	}
}
