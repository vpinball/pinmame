#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

extern UINT8 DMDFrames [GTS3DMD_FRAMES][0x200];
extern UINT8 DMDFrames2[GTS3DMD_FRAMES][0x200];
extern GTS3_DMDlocals GTS3_dmdlocals[2];

// Shaded frame computed from stored PWMed DMD frames
UINT16 accumulatedFrame[32][128];

#if defined(VPINMAME) || defined(LIBPINMAME)
extern UINT8  g_raw_gtswpc_dmd[GTS3DMD_FRAMES*0x200];
extern UINT32 g_raw_gtswpc_dmdframes;

static const unsigned char lookup[16] = {
0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };

INLINE UINT8 reverse(UINT8 n) {
  // Reverse the top and bottom nibble then swap them.
  return (lookup[n & 0x0f] << 4) | lookup[n >> 4];
}
#endif

// GTS3 hardware creates shades by quickly switching frames (PWM).
// It uses 24 frames at 376Hz to create the PWM pattern, so we need
// to store these 24 frames to avoid sampling artefact resulting
// in some residual flickering. Then we apply a low pass filter, 
// here a FIR based on a Kaiser window (other filter are commented
// out since I don't think that they give better results).
//
// Previous version had an optimized implementation, storing as little frames as
// possible for each GTS3 game (see commit 9b9ac9a5c2bfedac13ca382ff6669837882c129d).
// This was needed because each game had a different (somewhat wrong) VSync frequency.
// This lead to better performance since less frames were accumulated but had 2 side effects:
// - there were some occasional residual flickers,
// - DMD luminance were not fully coherent between GTS3 games (some 4 shades, some other 5 shades).

/*const int fir_weights[] = {1, 1, 1, 1, 1, 1, 1, 1,            // Moving average
                             1, 1, 1, 1, 1, 1, 1, 1,            // Overall sum = 24
                             1, 1, 1, 1, 1, 1, 1, 1 };          //*/
/*const int fir_weights[] = {30, 39, 47,  56,  64, 72, 80, 86,  // Octave: w = kaiser(24,2.5);
                             91, 96, 98, 100, 100, 98, 96, 91,
                             86, 80, 72,  64,  56, 47, 39, 30 };//*/
/*const int fir_weights[] = { 4,  9, 15,  24,  34, 46, 58, 70,  // Octave: w = kaiser(24,5.0);
                             81, 90, 96, 100, 100, 96, 90, 81,
                             70, 58, 46,  34,  26, 15,  9,  4 };//*/
const int fir_weights[] = {  1,  3,  6,  13,  21, 32, 46, 60,   // Octave: w = kaiser(24,7.0);
                            74, 86, 95, 100, 100, 95, 86, 74,   // Overall sum = 1074
                            60, 46, 32,  21,  13,  6,  3,  1 };
const int fir_sum = 1074;

int gts3_dmd128x32(int which, struct mame_bitmap* bitmap, const struct rectangle* cliprect, const struct core_dispLayout* layout)
{
  int ii,jj,kk,ll;
  const int frames = GTS3DMD_FRAMES;
  UINT8* dmdFrames = which == 0 ? &DMDFrames[0][0] : &DMDFrames2[0][0];

  /* int fir_sum = 0;
  for (ii = 0; ii < frames; ii++)
     fir_sum += fir_weights[ii]; //*/

  #if defined(VPINMAME) || defined(LIBPINMAME)
  int i = 0;
  g_raw_gtswpc_dmdframes = 5; // Only send the last 5 raw frames
  UINT8* rawData = &g_raw_gtswpc_dmd[0];
  for (ii = 0; ii < (int)g_raw_gtswpc_dmdframes; ii++) {
    UINT8* frameData = dmdFrames + ((GTS3_dmdlocals[0].nextDMDFrame + (GTS3DMD_FRAMES - 1) + (GTS3DMD_FRAMES - ii)) % GTS3DMD_FRAMES) * 0x200;
    for (jj = 0; jj < 32 * 16; jj++) {      // 32 lines of 16 columns of 8 pixels
      UINT8 data = *frameData++;
      *rawData = reverse(data);
      rawData++;
    }
  }
  #endif

  // Apply low pass filter over 24 frames
  memset(accumulatedFrame, 0, sizeof(accumulatedFrame));
  for (ii = 0; ii < frames; ii++) {
    UINT8* frameData = dmdFrames + ((GTS3_dmdlocals[0].nextDMDFrame + (GTS3DMD_FRAMES - 1) + (GTS3DMD_FRAMES - ii)) % GTS3DMD_FRAMES) * 0x200;
    for (jj = 1; jj <= 32; jj++) {          // 32 lines
      UINT16* line = &accumulatedFrame[jj - 1][0];
      for (kk = 0; kk < 16; kk++) {         // 16 columns/line
        UINT8 data = *frameData++;
        for (ll = 0; ll < 8; ll++) {        // 8 pixels/column
          (*line++) += (UINT16)(data>>7) * (UINT16) fir_weights[ii]; data <<= 1;
        }
      }
    }
  }

  // Scale down to 16 shades. This is somewhat wrong since the PWM pattern is made of 24 frames, so we should 
  // use 25 shades. But, while we need the 24 frames to avoid sampling artefacts, it does not seem to be really 
  // used and 16 shades looks good enough.
  for (ii = 1; ii <= 32; ii++)              // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT16 data = accumulatedFrame[ii-1][jj];
      coreGlobals.dotCol[ii][jj] = (15 * (unsigned int) data) / fir_sum;
    }

  video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}

PINMAME_VIDEO_UPDATE(gts3_dmd128x32a) { return gts3_dmd128x32(0, bitmap, cliprect, layout); }
PINMAME_VIDEO_UPDATE(gts3_dmd128x32b) { return gts3_dmd128x32(1, bitmap, cliprect, layout); }