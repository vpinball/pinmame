#include "xmame.h"
#include "sound.h"

int sysdep_audio_init(void)
{
   play_sound = FALSE;
   return OSD_OK;
}

void sysdep_audio_close(void)
{
}

int sysdep_audio_play(unsigned char *buf, int bufsize)
{
   return 0;
}

long sysdep_audio_get_freespace(void)
{
   return 0;
}
