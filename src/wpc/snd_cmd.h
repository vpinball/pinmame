#ifndef INC_SNDCMD
#define INC_SNDCMD

/* 161200 changed manual_sound_commands to return TRUE if not in sound command mode */
/*        added snd_cmd_exit, snd_cmd_log, removed sound_mode variable */

/* Exported Functions */
int manual_sound_commands(struct mame_bitmap *bitmap);
void snd_cmd_init(void);
void snd_cmd_exit(void);
void snd_cmd_log(int boardNo, int cmd);
int snd_get_cmd_log(int *last, int *buffer);

/*Constants*/
#define DCS_COMMS	14	/* # of Sequential Commands to trigger a sound (DCS )*/
#define WPCS_COMMS	4	/* # of Sequential Commands to trigger a sound (Non-DCS )*/
#define REPEATKEY	2	/* # of keystrokes pressed before registering
				   Lower # = keypresses register faster, higher = slower*/
#define MAXCOMMAND 	256	/* Highest Sound Command # that can be triggered*/
#define SND_XROW	35  	/* Where to start display of sound command text*/

/*Keys which affect sound mode & recording*/
/*-- REMEMBER: Change Help Text displayed on screen to match the keys here --*/
#define SMDCMD_MODETOGGLE	KEYCODE_F4
#define SMDCMD_RECORDTOGGLE	KEYCODE_F5
#define SMDCMD_NEXT		KEYCODE_RIGHT
#define SMDCMD_PREV		KEYCODE_LEFT
#define SMDCMD_UP		KEYCODE_UP
#define SMDCMD_DOWN		KEYCODE_DOWN
#define SMDCMD_ZERO		KEYCODE_DEL
#define SMDCMD_PLAY		KEYCODE_SPACE
#define SMDCMD_INSERT	KEYCODE_INSERT

#define MAX_CMD_LOG     16


#endif	/*INC_SNDCMD*/
