//============================================================
//
//	video.h - Win32 implementation of MAME video routines
//
//============================================================

#ifndef __WIN_VIDEO__
#define __WIN_VIDEO__


//============================================================
//	PARAMETERS
//============================================================

// maximum video size
#define MAX_VIDEO_WIDTH			1600
#define MAX_VIDEO_HEIGHT		1200

// dirty sizes (16x16 grid); keep X size an even power of 2 for speed
#define DIRTY_H					256
#define DIRTY_V					(MAX_VIDEO_HEIGHT / 16)



//============================================================
//	TYPE DEFINITIONS
//============================================================

// the actual dirty grid
typedef UINT8 DIRTYGRID[DIRTY_V * DIRTY_H];



//============================================================
//	MACROS
//============================================================

#define IS_DIRTY(x,y) 			(dirty_grid[(y) / 16 * DIRTY_H + (x) / 16])
#define MARK_DIRTY(x,y)			dirty_grid[(y) / 16 * DIRTY_H + (x) / 16] = 1



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
extern int			frameskip;
extern int			autoframeskip;

// dirty grid setting
extern int			use_dirty;
extern DIRTYGRID	dirty_grid;

// gamma and brightness adjustments
extern float		gamma_correct;

// speed throttling
extern int			throttle;
extern int			game_speed_percent;

// palette lookups
extern UINT32 *		palette_16bit_lookup;
extern UINT32 *		palette_32bit_lookup;

// debugger palette
extern const UINT8 *dbg_palette;



//============================================================
//	PROTOTYPES
//============================================================

void win_mark_palette_dirty(void);


#endif
