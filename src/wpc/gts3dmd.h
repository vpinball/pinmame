#ifndef INC_GTS3DMD
#define INC_GTS3DMD
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define GTS3DMD_FRAMES	24  //# of Frames to capture

VIDEO_UPDATE(gts3_dmd128x32a);
VIDEO_UPDATE(gts3_dmd128x32b);

#endif /* INC_GTS3DMD */
