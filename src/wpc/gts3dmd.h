#ifndef INC_GTS3DMD
#define INC_GTS3DMD
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

//#define GTS3DMD_FRAMES_4C_a	8   //# of Frames to capture for 4 color roms before WCS and after Shaq Attaq
//#define GTS3DMD_FRAMES_4C_b	15  //# of Frames to capture for 4 color roms WCS - Shaq Attaq
#define GTS3DMD_FRAMES_4C_a	6  //# of Frames to capture for 4 color roms Street Fighter II - Shaq Attaq
#define GTS3DMD_FRAMES_4C_b	8  //# of Frames to capture for 4 color roms after Shaq Attaq
#define GTS3DMD_FRAMES_5C	12  //# of Frames to capture for 5 color roms SMB, SMBMW and Cue Ball Wizard
//#define GTS3DMD_FRAMES	24  //# of Frames to capture for either 4 and 5 color roms // deprecated

VIDEO_UPDATE(gts3_dmd128x32);
VIDEO_UPDATE(gts3_dmd128x32a);

#endif /* INC_GTS3DMD */
