#include "driver.h"
#include "core.h"
#include "wpc.h"
#include "wpcdmd.h"
/*----------------------------------*/
/* Williams WPC 128x32 DMD Handling */
/*----------------------------------*/
int dmd_start(void) {
  unsigned char *RAM = memory_region(WPC_MEMREG_DMD);
  int ii;

  for (ii = 0; ii < DMD_FRAMES; ii++)
    coreGlobals_dmd.DMDFrames[ii] = RAM;
  coreGlobals_dmd.nextDMDFrame = 0;
  return 0;
}

void dmd_refresh(struct osd_bitmap *bitmap, int fullRefresh) {
  tDMDDot dotCol;
  int ii,jj,kk;

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  /* Create a temporary buffer with all pixels */
  for (kk = 0, ii = 1; ii < 33; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 16; jj++) {
      /* Intensity depends on how many times the pixel */
      /* been on in the last 3 frames                  */
      unsigned int intens1 = ((coreGlobals_dmd.DMDFrames[0][kk] & 0x55) +
                              (coreGlobals_dmd.DMDFrames[1][kk] & 0x55) +
                              (coreGlobals_dmd.DMDFrames[2][kk] & 0x55));
      unsigned int intens2 = ((coreGlobals_dmd.DMDFrames[0][kk] & 0xaa) +
                              (coreGlobals_dmd.DMDFrames[1][kk] & 0xaa) +
                              (coreGlobals_dmd.DMDFrames[2][kk] & 0xaa));

      *line++ = (intens1)    & 0x03;
      *line++ = (intens2>>1) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2>>3) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>5) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>7) & 0x03;
      kk +=1;
    }
    *line = 0; /* to simplify antialiasing */
  }
  dmd_draw(bitmap->line, dotCol, core_gameData->lcdLayout ? core_gameData->lcdLayout : &wpc_dispDMD[0]);

  drawStatus(bitmap,fullRefresh);
}

