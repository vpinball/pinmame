#ifndef INC_GTS3DMD
#define INC_GTS3DMD

#define GTS3DMD_FRAMES 	24  //# of Frames to capture
#define AVAIL_INTENSITY 3	//Max # of Intensity levels supported by PinMAME
#define FRAMEDIV (GTS3DMD_FRAMES/AVAIL_INTENSITY)-1)	//Break Frames into pieces

void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap,int full_refresh);

#endif /* INC_GTS3DMD */

