#ifndef INC_GTS3DMD
#define INC_GTS3DMD

#define GTS3DMD_FRAMES	42  //# of Frames to capture
#define GTS3DMD_100 	(GTS3DMD_FRAMES * 2 / 3)  //100% intensity minimum level
#define GTS3DMD_66 		(GTS3DMD_FRAMES / 4)  //66% intensity minimum level
#define GTS3DMD_33 		2  //33% intensity minimum level

// void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap,int full_refresh);

VIDEO_UPDATE(gts3_dmd128x32);

#endif /* INC_GTS3DMD */

