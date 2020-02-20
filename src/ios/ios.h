//
//  ios.h
//  iPinMAME
//

#ifndef ios_h
#define ios_h

#include <osdepend.h>

extern void ipinmame_update_display();
extern void ipinmame_logger(const char* fmt, ...);
extern UINT8* ipinmame_init_video(int width, int height, int depth);
extern void ipinmame_logger(const char* fmt, ...);

#endif /* ios_h */
