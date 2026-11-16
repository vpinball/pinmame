// license:BSD-3-Clause
#pragma once

/* 161200 changed manual_sound_commands to return TRUE if not in sound command mode */
/*        added snd_cmd_exit, snd_cmd_log, removed sound_mode variable */

/* Exported Functions */
int manual_sound_commands(struct mame_bitmap *bitmap);
void snd_cmd_init(void);
void snd_cmd_exit(void);
void snd_cmd_log(int boardNo, int cmd); // a sound command the driver logs itself (SND_CMD_DRIVER)

/* Where a logged sound command comes from. AltSound uses it to pick the command stream its
   pack was made for (new_sound_commands in altsound.ini, see alt_sound_handle) */
#define SND_CMD_DRIVER 0 /* logged by the driver itself: one byte (or value) per real sound command */
#define SND_CMD_DATA   1 /* a data write to the sound board (sndbrd_data_w), logged as in PinMAME 3.6 and before */
#define SND_CMD_LEGACY 2 /* a data write 3.6 and before logged, now replaced by the driver's own commands
                            (SNDBRD_DATALOG_LEGACY): AltSound only, for the packs made back then */
void snd_cmd_log_src(int boardNo, int cmd, int src);
int snd_get_cmd_log(int *last, int *buffer);

void reinit_pinSound(void);

/*Constants*/
#define DCS_COMMS	14	/* # of Sequential Commands to trigger a sound (DCS )*/
#define WPCS_COMMS	4	/* # of Sequential Commands to trigger a sound (Non-DCS )*/
#define REPEATKEY	2	/* # of keystrokes pressed before registering
							Lower # = keypresses register faster, higher = slower*/
#define MAXCOMMAND	256	/* Highest Sound Command # that can be triggered*/
#define SND_XROW	35	/* Where to start display of sound command text*/

/*Keys which affect sound mode & recording*/
/*-- REMEMBER: Change Help Text displayed on screen to match the keys here --*/
#define SMDCMD_MODETOGGLE	KEYCODE_F4
#define SMDCMD_RECORDTOGGLE	KEYCODE_F5
#define SMDCMD_DUMPTOGGLE	KEYCODE_F6
#define SMDCMD_NEXT		KEYCODE_RIGHT
#define SMDCMD_PREV		KEYCODE_LEFT
#define SMDCMD_UP		KEYCODE_UP
#define SMDCMD_DOWN		KEYCODE_DOWN
#define SMDCMD_ZERO		KEYCODE_DEL
#define SMDCMD_PLAY		KEYCODE_SPACE
#define SMDCMD_INSERT	KEYCODE_INSERT

#define MAX_CMD_LOG		16
