#ifndef INC_GTS3DMD
#define INC_GTS3DMD

#define GTS3DMD_FRAMES	18  //# of Frames to capture
#define GTS3DMD_100 	(GTS3DMD_FRAMES * 7 / 8)  //100% intensity minimum level
#define GTS3DMD_66 		(GTS3DMD_FRAMES / 3)  //66% intensity minimum level
#define GTS3DMD_33 		2  //33% intensity minimum level

// void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap,int full_refresh);

VIDEO_UPDATE(gts3_dmd128x32);
VIDEO_UPDATE(gts3_dmd128x32a);

#endif /* INC_GTS3DMD */

