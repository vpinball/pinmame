/***************************************************************************

	VPinMAME - Visual Pinball Multiple Arcade Machine Emulator

	This file is based on the code of Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  OSDepend.c

  OS dependent stuff (display handling, keyboard scan...)

 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ControllerOptions.h"

#include "OSDepend.h"
#include "DirectSound.h"
#include "Keyboard.h"
#include "Display.h"
#include "UClock.h"

#include "File.h"
#include "Mame.h"
#include "common.h"

extern HANDLE		g_hGameRunning;

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static FILE* g_pErrorLog;
BOOL g_bOsDebug;

/***************************************************************************
    External OSD function definitions  
 ***************************************************************************/

int osd_init()
{
    uclock_init();

	Display.init(g_pControllerOptions);
	DirectSound.init(g_pControllerOptions);
	Keyboard.init(g_pControllerOptions);

	return 0;
}

void osd_exit(void)
{
	DirectSound.exit();
    uclock_exit();
}

/***************************************************************************
    Display
 ***************************************************************************/

struct osd_bitmap* osd_alloc_bitmap(int width, int height, int depth)
{
	return Display.alloc_bitmap(width, height, depth);
}

void osd_free_bitmap(struct osd_bitmap* pBitmap)
{
//    MAME32App.m_pDisplay->free_bitmap(pBitmap);
}

int osd_create_display(int width, int height, int depth, int fps, int attributes, int orientation)
{
    return Display.create_display(width, height, depth, fps, attributes, orientation);
}

void osd_close_display(void)
{
	Display.close_display();
}

void osd_set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	Display.set_visible_area(min_x, max_x, min_y, max_y);
}

#if MAMEVER >= 3709
int osd_allocate_colors(unsigned int totalcolors,
                        const UINT8* palette,
                        UINT32*      pens,
                        int          modifiable,
                        const UINT8* debug_palette,
                        UINT32*      debug_pens)
#else
int osd_allocate_colors(unsigned int totalcolors,
                        const UINT8* palette,
                        UINT16*      pens,
                        int          modifiable,
                        const UINT8* debug_palette,
                        UINT16*      debug_pens)
#endif
{
	if ( Display.allocate_colors )
		return Display.allocate_colors(totalcolors, palette, pens, modifiable, debug_palette, debug_pens);
	else
		return 0;
}

void osd_modify_pen(int pen, unsigned char red, unsigned char green, unsigned char blue)
{
	Display.modify_pen(pen, red, green, blue);
}

void osd_get_pen(int pen, unsigned char* red, unsigned char* green, unsigned char* blue)
{
	Display.get_pen(pen, red, green, blue);
}

void osd_update_video_and_audio(struct osd_bitmap *game_bitmap,
                                struct osd_bitmap *debug_bitmap,
                                int leds_status)
{
    DirectSound.update_audio();
	Display.update_display(game_bitmap, debug_bitmap);

	while ( MsgWaitForMultipleObjects(1, &g_hGameRunning, FALSE, INFINITE, QS_ALLINPUT)!=WAIT_OBJECT_0 ) {
		MSG Msg;
		while ( PeekMessage(&Msg, 0, 0, 0, PM_REMOVE) ) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
}

void osd_mark_dirty(int x1, int y1, int x2, int y2)
{
	Display.mark_dirty(x1, y1, x2, y2);
}

int osd_skip_this_frame()
{
	return 0;
}

void osd_set_gamma(float _gamma)
{
}

float osd_get_gamma(void)
{
	return 1;
}

void osd_set_brightness(int brightness)
{
}

int osd_get_brightness(void)
{
	return 1;
}

void osd_save_snapshot(struct osd_bitmap *bitmap)
{
	save_screen_snapshot(bitmap);
}

void osd_debugger_focus(int debugger_has_focus)
{
}


/***************************************************************************
    Sound
 ***************************************************************************/

int osd_start_audio_stream(int stereo)
{
	if (Machine->sample_rate==0)
		return 0;
	return DirectSound.start_audio_stream(stereo);
}

int osd_update_audio_stream(INT16* buffer)
{
	if (Machine->sample_rate==0)
		return 0;
	return DirectSound.update_audio_stream(buffer);
}

void osd_stop_audio_stream(void)
{
	if (Machine->sample_rate==0)
		return;
	DirectSound.stop_audio_stream();
}

void osd_set_mastervolume(int attenuation)
{
	if (Machine->sample_rate==0)
		return;

    if (attenuation > 0)
        attenuation = 0;
    if (attenuation < -32)
        attenuation = -32;

    DirectSound.set_mastervolume(attenuation);
}

int osd_get_mastervolume()
{
	if (Machine->sample_rate==0)
		return 10;
    return DirectSound.get_mastervolume();
}

void osd_sound_enable(int enable)
{
	if (Machine->sample_rate==0)
		return;
	DirectSound.sound_enable(enable);
}

void osd_opl_control(int chip, int reg)
{
}

void osd_opl_write(int chip, int data)
{
}

/***************************************************************************
    Keyboard
 ***************************************************************************/

/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo *osd_get_key_list(void)
{
	return Keyboard.get_key_list();
}

/*
  inptport.c defines some general purpose defaults for key bindings. They may be
  further adjusted by the OS dependant code to better match the available keyboard,
  e.g. one could map pause to the Pause key instead of P, or snapshot to PrtScr
  instead of F12. Of course the user can further change the settings to anything
  he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys you want.
*/
void osd_customize_inputport_defaults(struct ipd *defaults)
{
	Keyboard.customize_inputport_defaults(defaults);
}

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_customize_inputport_defaults().
*/
int osd_is_key_pressed(int keycode)
{
	return Keyboard.is_key_pressed(keycode);
}

/*
  Return the Unicode value of the most recently pressed key. This
  function is used only by text-entry routines in the user interface and should
  not be used by drivers. The value returned is in the range of the first 256
  bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

  Set flush to 1 to clear the buffer before entering text. This will avoid
  having prior UI and game keys leak into the text entry.
*/

int osd_readkey_unicode(int flush)
{
	return Keyboard.readkey_unicode(flush);
}

void osd_pause(int paused)
{
}

/***************************************************************************
    Joystick
 ***************************************************************************/

static struct JoystickInfo joyinfo = {"", 0, 0};

const struct JoystickInfo *osd_get_joy_list(void)
{
	return &joyinfo;
}

int osd_is_joy_pressed(int joycode)
{
	return 0;
}

/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration(void)
{
    return 0;
}

/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void)
{
    assert(FALSE);
}

/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
#if MAMEVER >= 3709
const
#endif
char *osd_joystick_calibrate_next(void)
{
    assert(FALSE);
    return " ";
}

/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void)
{
    assert(FALSE);
}

/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void)
{
    assert(FALSE);
}

void osd_analogjoy_read(int player, int *analog_x, int *analog_y)
{
}

/***************************************************************************
    Trakball
 ***************************************************************************/

void osd_trak_read(int player, int *deltax, int *deltay)
{
}

/***************************************************************************
    Files
 ***************************************************************************/

int osd_faccess(const char *filename, int filetype)
{
    return File.faccess(filename, filetype);
}

void *osd_fopen(const char *gamename, const char *filename, int filetype, int write)
{
    return File.fopen(gamename,filename,filetype,write);
}

int osd_fread(void *file, void *buffer, int length)
{
    return File.fread(file, buffer, length);
}

int osd_fwrite(void *file, const void *buffer, int length)
{
    return File.fwrite(file, buffer, length);
}

int osd_fread_swap(void *file, void *buffer, int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;


	res = osd_fread(file,buffer,length);

	buf = buffer;
	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	return res;
}

int osd_fwrite_swap(void *file,const void *buffer,int length)
{
	int i;
	unsigned char *buf;
	unsigned char temp;
	int res;


	buf = (unsigned char *)buffer;
	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	res = osd_fwrite(file,buffer,length);

	for (i = 0;i < length;i+=2)
	{
		temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}

	return res;
}

int osd_fread_scatter(void *file, void *buffer, int length, int increment)
{
	return File.fread_scatter(file, buffer, length, increment);
}

int osd_fseek(void *file, int offset, int whence)
{
    return File.fseek(file, offset, whence);
}

void osd_fclose(void *file)
{
    File.fclose(file);
}

int osd_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum)
{
    return File.fchecksum(gamename, filename, length, sum);
}

int osd_fsize(void *file)
{
    return File.fsize(file);
}

unsigned int osd_fcrc(void *file)
{
    return File.fcrc(file);
}

int osd_fgetc(void *file)
{
    return File.fgetc(file);
}

int osd_ungetc(int c, void *file)
{
    return File.ungetc(c, file);
}

char *osd_fgets(char *s, int n, void *file)
{
    return File.fgets(s, n, file);
}

int osd_feof(void *file)
{
    return File.eof(file);
}

int osd_ftell(void *file)
{
    return File.ftell(file);
}

int osd_display_loading_rom_message(const char* name, int current, int total)
{
    int retval;
    
    if (name != NULL)
        retval = 0; // UpdateLoadProgress(name, current - 1, total);
    else
        retval = 0; //UpdateLoadProgress("", total, total);
    
    return retval;
}

void CLIB_DECL logerror(const char *text, ...)
{
    va_list arg;
    va_start(arg,text);

    if (g_pErrorLog)
        vfprintf(g_pErrorLog, text, arg);

    if (g_bOsDebug) {
        char szBuffer[512];
        _vsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), text, arg);
        OutputDebugString(szBuffer);
    }

    va_end(arg);
}

#ifdef MAME_NET

/***************************************************************************
    Network
 ***************************************************************************/

int osd_net_init(void)
{
    if (!MAME32App.m_pNetwork)
    {
        MAME32App.m_pNetwork = &Network;
    }
	return MAME32App.m_pNetwork->init();
}

int osd_net_game_init(void)
{
    if (!MAME32App.m_pNetwork)
    {
        MAME32App.m_pNetwork = &Network;
    }
    return MAME32App.m_pNetwork->game_init();
}

int osd_net_game_exit(void)
{
    if (!MAME32App.m_pNetwork)
	    return 0;
    else
        return MAME32App.m_pNetwork->game_exit();
}

int osd_net_exit(void)
{
    if (!MAME32App.m_pNetwork)
	    return 0;
    else
        return MAME32App.m_pNetwork->exit();
}

int osd_net_send(int port, unsigned char buf[], int *size)
{
	return MAME32App.m_pNetwork->send( port, buf, size );
}

int osd_net_recv(int port, unsigned char buf[], int *size)
{
	return MAME32App.m_pNetwork->recv( port, buf, size );
}

int osd_net_sync(void)
{
    if (MAME32App.m_pNetwork == NULL)
        return -1;
    else
	    return MAME32App.m_pNetwork->sync();
}

int osd_net_input_sync(void)
{
	return 0;
}

int osd_net_add_player(void)
{
	if ( MAME32App.m_pNetwork == NULL )
		return -1;
	else
		return MAME32App.m_pNetwork->add_player();
}

int osd_net_remove_player( int player )
{
	if ( MAME32App.m_pNetwork == NULL )
		return -1;
	else
		return MAME32App.m_pNetwork->remove_player( player );
}

#endif /* MAME_NET */
