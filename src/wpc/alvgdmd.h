#ifndef INC_ALVGDMD
#define INC_ALVGDMD

/*--------------------- DMD 128x32 -------------------*/
#define ALVGDMD_CPUNO     2
#define ALVGDMD_CPUREGION (REGION_CPU1 + ALVGDMD_CPUNO)
#define ALVGDMD_ROMREGION (REGION_GFX1 + ALVGDMD_CPUNO)

extern PINMAME_VIDEO_UPDATE(alvgdmd_update);

#endif /* INC_ALVGDMD */

