#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

//#define DEBUGSWAP

extern UINT8 DMDFrames [GTS3DMD_FRAMES_5C][0x200];
extern UINT8 DMDFrames2[GTS3DMD_FRAMES_5C][0x200];
extern GTS3_DMDlocals GTS3_dmdlocals[2];

#ifdef DEBUGSWAP
extern int crtc6845_start_addr;
#endif

static unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };


//static int level4_a[9]  = { 0, 1, 5, 5, 5, 5, 5, 5, 15 }; // mapping for 4 color roms, mode a
//static int level4_b[16] = { 0, 1, 1, 1, 5, 5, 5, 10, 10, 10, 15, 15, 15, 15, 15, 15 }; // mapping for 4 color roms, mode b
//static int level4_a[16] = { 0, 3, 3, 3, 6, 6, 6, 8, 8, 9, 10, 11, 12, 13, 14, 15 };
static int level4_a[7] = { 0, 1, 2, 2, 2, 2, 3 }; // 4 colors
static int level4_a2[7] = { 0, 1, 1, 2, 2, 2, 3 }; // 4 colors
static int level4_b[9] = { 0, 1, 2, 2, 2, 2, 2, 2, 3 }; // 4 colors
static int level5[13] = { 0, 3, 3, 7, 7, 7, 11, 11, 11, 11, 11, 11, 15 }; // 5 colors
//static int level5[19] = { 0, 3, 3, 4, 5, 5, 5, 7, 8, 9, 11, 11, 11, 12, 13, 14, 15, 15, 15 };
//static int level[25]  = { 0, 0, 1, 1, 1, 5, 5, 5, 5, 5, 5, 5, 10, 10, 10, 10, 10, 15, 15, 15, 15, 15, 15, 15, 15 }; // temporary mapping for both 4 and 5 color roms // deprecated

#ifdef VPINMAME
extern UINT8  g_raw_gtswpc_dmd[GTS3DMD_FRAMES_5C*0x200];
extern UINT32 g_raw_gtswpc_dmdframes;
#endif

INLINE UINT8 reverse(UINT8 n) {
  // Reverse the top and bottom nibble then swap them.
  return (lookup[n & 0x0f] << 4) | lookup[n >> 4];
}

//DMD #2 Display routine for Strikes N Spares - code is IDENTICAL to the gts3_dmd128x32
PINMAME_VIDEO_UPDATE(gts3_dmd128x32a) {
  tDMDDot dotCol;
  UINT8 *frameData = &DMDFrames2[0][0];
  int ii,jj,kk,ll;
  int frames = GTS3_dmdlocals[0].color_mode == 0 ? GTS3DMD_FRAMES_4C_a : (GTS3_dmdlocals[0].color_mode == 1 ? GTS3DMD_FRAMES_4C_b : GTS3DMD_FRAMES_5C);
  int *level = GTS3_dmdlocals[0].color_mode == 0 ? level4_a : (GTS3_dmdlocals[0].color_mode == 1 ? level4_b : level5);
#ifdef VPINMAME
  int i = 0;
#endif

  memset(dotCol,0,sizeof(tDMDDot));
  for (ii = 0; ii < frames; ii++) {
    for (jj = 1; jj <= 32; jj++) {           // 32 lines
      UINT8 *line = &dotCol[jj][0];
      for (kk = 0; kk < 16; kk++) {      // 16 columns/line
        UINT8 data = *frameData++;
#ifdef VPINMAME
        g_raw_gtswpc_dmd[i] = reverse(data);
        i++;
#endif
        for (ll = 0; ll < 8; ll++)          // 8 pixels/column
          { (*line++) += (data>>7); data <<= 1; }
      }
    }
  }

  // detect special case for some otherwise flickering frames
  if (frames == GTS3DMD_FRAMES_4C_a) {
	  for (ii = 1; ii <= 32; ii++)               // 32 lines
		  for (jj = 0; jj < 128; jj++) {		// 128 pixels/line
			  if (dotCol[ii][jj] == 4){
				  level = level4_a2;
				  break;
			  }
		  }
  }

  for (ii = 1; ii <= 32; ii++)               // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT8 data = dotCol[ii][jj];
      dotCol[ii][jj] = level[data];
  }

  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

PINMAME_VIDEO_UPDATE(gts3_dmd128x32) {
  tDMDDot dotCol;
  UINT8 *frameData = &DMDFrames[0][0];
  int ii,jj,kk,ll;
  int frames = GTS3_dmdlocals[0].color_mode == 0 ? GTS3DMD_FRAMES_4C_a : (GTS3_dmdlocals[0].color_mode == 1 ? GTS3DMD_FRAMES_4C_b : GTS3DMD_FRAMES_5C);
  int *level = GTS3_dmdlocals[0].color_mode == 0 ? level4_a : (GTS3_dmdlocals[0].color_mode == 1 ? level4_b : level5);

#ifdef VPINMAME
  int i = 0;
  g_raw_gtswpc_dmdframes = frames;
#endif

#ifdef DEBUGSWAP
  char temp[250];
  sprintf(temp,"location=%04x   %04x",0x1000+(crtc6845_start_addr>>2), crtc6845_start_addr);
  core_textOutf(50,50,1,temp);
#endif

  /* Drawing is not optimised so just clear everything */
  // !!! if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);
  memset(dotCol,0,sizeof(tDMDDot));
  for (ii = 0; ii < frames; ii++) {
    for (jj = 1; jj <= 32; jj++) {          // 32 lines
      UINT8 *line = &dotCol[jj][0];
      for (kk = 0; kk < 16; kk++) {         // 16 columns/line
        UINT8 data = *frameData++;
#ifdef VPINMAME
        g_raw_gtswpc_dmd[i] = reverse(data);
        i++;
#endif
        for (ll = 0; ll < 8; ll++)          // 8 pixels/column
          { (*line++) += (data>>7); data <<= 1; }
      }
    }
  }


  // detect special case for some otherwise flickering frames
  if (frames == GTS3DMD_FRAMES_4C_a) {
	  for (ii = 1; ii <= 32; ii++)               // 32 lines
		  for (jj = 0; jj < 128; jj++) {         // 128 pixels/line
			  if (dotCol[ii][jj] == 4) {
				  level = level4_a2;
				  break;
			  }
		  }
  }

  for (ii = 1; ii <= 32; ii++)              // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT8 data = dotCol[ii][jj];
      dotCol[ii][jj] = level[data];
  }

  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
