#ifndef _OSD_DBG_H
#define _OSD_DBG_H

#ifdef MAME_DEBUG

#include <time.h>

#define ARGFMT

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef INVALID
#define INVALID 0xffffffff
#endif

enum {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
    DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE
};

#ifndef WIN_EMPTY
#define WIN_EMPTY   '°'
#endif
#ifndef CAPTION_L
#define CAPTION_L   '®'
#endif
#ifndef CAPTION_R
#define CAPTION_R   '¯'
#endif
#ifndef FRAME_TL
#define FRAME_TL    'Ú'
#endif
#ifndef FRAME_BL
#define FRAME_BL    'À'
#endif
#ifndef FRAME_TR
#define FRAME_TR    '¿'
#endif
#ifndef FRAME_BR
#define FRAME_BR    'Ù'
#endif
#ifndef FRAME_V
#define FRAME_V     '³'
#endif
#ifndef FRAME_H
#define FRAME_H     'Ä'
#endif

/***************************************************************************
 *
 * These functions have to be provided by the OS specific code
 *
 ***************************************************************************/

extern void osd_screen_update(void);
extern void osd_put_screen_char(int ch, int attr, int x, int y);
extern void osd_set_screen_curpos(int x, int y);

/***************************************************************************
 * set_screen_size should set any mode that is available
 * on the platform and the get_screen_size function should return the
 * resolution that is actually available.
 * The minimum required size is 80x25 characters, anything higher is ok.
 ***************************************************************************/

extern void osd_set_screen_size(unsigned width, unsigned height);
extern void osd_get_screen_size(unsigned *width, unsigned *height);

#endif  /* MAME_DEBUG */

#endif	/* _OSD_DBG_H */

