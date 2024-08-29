#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

extern UINT8 DMDFrames [GTS3DMD_FRAMES][0x200];
extern UINT8 DMDFrames2[GTS3DMD_FRAMES][0x200];
extern GTS3_DMDlocals GTS3_dmdlocals[2];

// Shaded frame computed from stored PWMed DMD frames
UINT16 GTS3_accumulatedFrame[32][128];

#if defined(VPINMAME) || defined(LIBPINMAME)
extern UINT8  g_raw_gtswpc_dmd[GTS3DMD_FRAMES*0x200];
extern UINT32 g_raw_gtswpc_dmdframes;
#endif

// GTS3 hardware creates shades by quickly switching frames (PWM).
// It uses pattern of 3, 6, 8 or 10 frames at 376Hz to create the 
// PWM pattern. We store the last 24 frames and apply a FIR low pass 
// filter computed with GNU Octave (script is given below).
//
// Previous version had an optimized implementation, storing as little frames as
// possible for each GTS3 game (see commit 9b9ac9a5c2bfedac13ca382ff6669837882c129d).
// This was needed because each game had a different (somewhat wrong) VSync frequency.
// This lead to better performance since less frames were accumulated but had 2 side effects:
// - there were some occasional residual flickers,
// - DMD luminance were not fully coherent between GTS3 games (some 4 shades, some other 5 shades).

/* The following script allows to compute and display the filter applied to some PWM pattern in GNU Octave:
pkg load signal
fc = 15; % Cut-off frequency (Hz)
fs = 376; % Sampling rate (Hz)
data=[repmat([0;0;1],100,1),repmat([0;0;0;1;1;1],50,1),repmat([0;0;0;0;1;1;1;1;1;1],30,1),repmat([0;0;0;0;0;0;0;0;0;1],30,1)];
b = fir1(23, fc/(fs/2));
filtered = filter(b,1,data);
clf
subplot ( columns ( filtered ), 1, 1)
plot(filtered(:,1),";1/3 - 3;")
subplot ( columns ( filtered ), 1, 2 )
plot(filtered(:,2),";1/2 - 6;")
subplot ( columns ( filtered ), 1, 3 )
plot(filtered(:,3),";6/10 - 10;")
subplot ( columns ( filtered ), 1, 4 )
plot(filtered(:,4),";1/10 - 10;")
bp = 10000 * b; % scaled filter used for PinMame integer math
*/

int gts3_dmd128x32(int which, struct mame_bitmap* bitmap, const struct rectangle* cliprect, const struct core_dispLayout* layout)
{
  static const UINT16 fir_weights[] = {  8,  19,  44,  91, 168, 274, 405, 552,   // Octave: b = fir1(23, fc/(fs/2)); with fc = 15; and fs = 376;
                                       699, 830, 928, 981, 981, 928, 830, 699,
                                       552, 405, 274, 168,  91,  44,  19,   8 };
  const UINT16 fir_sum = 9998;

  int ii,jj,kk,ll;
  UINT8* dmdFrames = which == 0 ? &DMDFrames[0][0] : &DMDFrames2[0][0];

  #if defined(VPINMAME) || defined(LIBPINMAME)
  int i = 0;
  g_raw_gtswpc_dmdframes = 5; // Only send the last 5 raw frames
  UINT8* rawData = &g_raw_gtswpc_dmd[0];
  for (ii = 0; ii < (int)g_raw_gtswpc_dmdframes; ii++) {
    UINT8* frameData = dmdFrames + ((GTS3_dmdlocals[0].nextDMDFrame + (GTS3DMD_FRAMES - 1) + (GTS3DMD_FRAMES - ii)) % GTS3DMD_FRAMES) * 0x200;
    for (jj = 0; jj < 32 * 16; jj++) {      // 32 lines of 16 columns of 8 pixels
      UINT8 data = *frameData++;
      *rawData = core_revbyte(data);
      rawData++;
    }
  }
  #endif

  // Apply low pass filter over 24 frames
  memset(GTS3_accumulatedFrame, 0, sizeof(GTS3_accumulatedFrame));
  for (ii = 0; ii < sizeof(fir_weights)/sizeof(fir_weights[0]); ii++) {
    const UINT16 frame_weight = fir_weights[ii];
    UINT8* frameData = dmdFrames + ((GTS3_dmdlocals[0].nextDMDFrame + (GTS3DMD_FRAMES - 1) + (GTS3DMD_FRAMES - ii)) % GTS3DMD_FRAMES) * 0x200;
    for (jj = 1; jj <= 32; jj++) {          // 32 lines
      UINT16* line = &GTS3_accumulatedFrame[jj - 1][0];
      for (kk = 0; kk < 16; kk++) {         // 16 columns/line
        UINT8 data = *frameData++;
        for (ll = 0; ll < 8; ll++) {        // 8 pixels/column
          (*line++) += frame_weight * (UINT16)(data>>7);
          data <<= 1;
        }
      }
    }
  }

  // Scale down to 16 shades (note that precision matters and is needed to avoid flickering)
  for (ii = 1; ii <= 32; ii++)              // 32 lines
    for (jj = 0; jj < 128; jj++) {          // 128 pixels/line
      UINT16 data = GTS3_accumulatedFrame[ii-1][jj];
      coreGlobals.dotCol[ii][jj] = ((UINT8)((255 * (unsigned int) data) / fir_sum)) >> 4;
    }

  video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}

PINMAME_VIDEO_UPDATE(gts3_dmd128x32a) { return gts3_dmd128x32(0, bitmap, cliprect, layout); }
PINMAME_VIDEO_UPDATE(gts3_dmd128x32b) { return gts3_dmd128x32(1, bitmap, cliprect, layout); }