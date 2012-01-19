#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

//#define DEBUGSWAP

extern UINT8 DMDFrames[GTS3DMD_FRAMES][0x200];
extern UINT8 DMDFrames2[GTS3DMD_FRAMES][0x200];
#ifdef DEBUGSWAP
extern int crtc6845_start_addr;
#endif

static int level[16] = {0, 3, 3, 3, 6, 6, 6, 9, 9, 9, 12, 12, 12, 15, 15, 15};

//DMD #2 Display routine for Strikes N Spares - code is IDENTICAL to the gts3_dmd128x32
PINMAME_VIDEO_UPDATE(gts3_dmd128x32a) {
  tDMDDot dotCol;
  UINT8 *frameData = &DMDFrames2[0][0];
  int ii,jj,kk,ll;

  memset(dotCol,0,sizeof(tDMDDot));
  for (ii = 0; ii < GTS3DMD_FRAMES; ii++) {
    for (jj = 1; jj <= 32; jj++) {           // 32 lines
      UINT8 *line = &dotCol[jj][0];
      for (kk = 0; kk < 16; kk++) {      // 16 columns/line
        UINT8 data = *frameData++;
        for (ll = 0; ll < 8; ll++)          // 8 pixels/column
          { (*line++) += (data>>7); data <<= 1; }
      }
    }
  }
  for (ii = 1; ii <= 32; ii++)               // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT8 data = dotCol[ii][jj];
      dotCol[ii][jj] = 63 + level[data];
  }

  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

PINMAME_VIDEO_UPDATE(gts3_dmd128x32) {
  tDMDDot dotCol;
  UINT8 *frameData = &DMDFrames[0][0];
  int ii,jj,kk,ll;
#ifdef DEBUGSWAP
  char temp[250];
  sprintf(temp,"location=%04x   %04x",0x1000+(crtc6845_start_addr>>2), crtc6845_start_addr);
  core_textOutf(50,50,1,temp);
#endif

  /* Drawing is not optimised so just clear everything */
  // !!! if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);
  memset(dotCol,0,sizeof(tDMDDot));
  for (ii = 0; ii < GTS3DMD_FRAMES; ii++) {
    for (jj = 1; jj <= 32; jj++) {           // 32 lines
      UINT8 *line = &dotCol[jj][0];
      for (kk = 0; kk < 16; kk++) {      // 16 columns/line
        UINT8 data = *frameData++;
        for (ll = 0; ll < 8; ll++)          // 8 pixels/column
          { (*line++) += (data>>7); data <<= 1; }
      }
    }
  }
  for (ii = 1; ii <= 32; ii++)               // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT8 data = dotCol[ii][jj];
      dotCol[ii][jj] = 63 + level[data];
  }

  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
