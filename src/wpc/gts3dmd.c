#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

extern GTS3_DMDlocals GTS3_dmdlocals[2];

// GTS3 hardware creates shades by quickly switching frames (PWM).
// It uses a pattern of 3, 6, 8 or 10 frames at 376Hz to create the
// PWM pattern. We store the last 24 frames and apply a FIR low pass
// filter computed with GNU Octave (script is given in core.c).
//
// Previous version had an optimized implementation, storing as little frames as
// possible for each GTS3 game (see commit 9b9ac9a5c2bfedac13ca382ff6669837882c129d).
// This was needed because each game had a different (somewhat wrong) VSync frequency.
// This lead to better performance since less frames were accumulated but had 2 side effects:
// - there were some occasional residual flickers,
// - DMD luminance was not fully coherent between GTS3 games.
int gts3_dmd128x32(int which, struct mame_bitmap* bitmap, const struct rectangle* cliprect, const struct core_dispLayout* layout)
{
  core_dmd_video_update(bitmap, cliprect, layout, &GTS3_dmdlocals[which].pwm_state);
  return 0;
}

PINMAME_VIDEO_UPDATE(gts3_dmd128x32a) { return gts3_dmd128x32(0, bitmap, cliprect, layout); }
PINMAME_VIDEO_UPDATE(gts3_dmd128x32b) { return gts3_dmd128x32(1, bitmap, cliprect, layout); }
