/***************************************************************************
 Sound Command Mode & Wave Recording Functionality (SJE 301000)
 ---------------------------------------------------------------------------

 Sound Command Mode allows user to trigger any valid sound commands to the
 Pinball Sound Hardware. This allows user to play any sound contained in the
 Sound Roms.

 Wave Recording allows user to save the current sound output to a file.

 HEAVILY MODIFIED by WPCMAME - 12/00!
 280101 Added '-' as digit to allow shorter commands
 ****************************************************************************/

#include <ctype.h>
#include "driver.h"
#include "wmssnd.h"
#include "snd_cmd.h"
#include "wpc.h"

#define MAX_CMD_LENGTH   6
#define MAX_NAME_LENGTH 30
#define MAX_LINE_LENGTH 100
#define SMDCMD_DIGITTOGGLE SMDCMD_ZERO

//Don't love this idea. Let's get a group vote! SJE

#if 0
/* Use memcard directory for wavefiles */
#define OSD_FILETYPE_WAVEFILE OSD_FILETYPE_MEMCARD
#endif

static int playCmd(int length, int *cmd);
static int checkName(const char *buf, const char *name);
static void readCmds(char *head);
static void clrCmds(void);

static struct cmds {
  struct cmds *prev, *next;
  int    length;
  int    cmd[MAX_CMD_LENGTH];
  char   name[MAX_NAME_LENGTH];
} cmds = {NULL, NULL, 2, {0,0}, "Stop playing"};

static struct {
  struct cmds *currCmd;
  int sndCmd;
  int soundMode;
  int digitMode;
  int currDigit;
  int digits[MAX_CMD_LENGTH*2];
  int cmdLog[MAX_CMD_LOG];
  int firstLog;
  mem_write_handler soundCmd;
} locals;

#ifdef PINMAME_EXT
static void handle_recording(void);
static void wave_close(void);
#else
#define handle_recording()
#define wave_close()
#endif /* PINMAME_EXT */

/*---------------------------------*/
/*-- init manual sound commands  --*/
/*---------------------------------*/
void snd_cmd_init(mem_write_handler soundCmd, char *head) {
  int ii;
  memset(&locals, 0, sizeof(locals));
  locals.currCmd = &cmds;
  locals.soundCmd = soundCmd;
  if (soundCmd) readCmds(head);
  for (ii = 0; ii < MAX_CMD_LENGTH*2; ii++) locals.digits[ii] = 0x10;
}

/*----------------------------------------*/
/*-- clean up manual sound command data --*/
/*----------------------------------------*/
void snd_cmd_exit(void) {
  clrCmds();
  wave_close();
}

/*----------------
/ log handling
/-----------------*/
void snd_cmd_log(int cmd) {
  if (locals.soundMode) return; // Don't log from within sound commander
  locals.cmdLog[locals.firstLog++] = cmd;
  locals.firstLog %= MAX_CMD_LOG;
}

int snd_get_cmd_log(int *last, int *buffer) {
  int count = 0;
  while (*last != locals.firstLog) {
    buffer[count++] = locals.cmdLog[(*last)++];
    *last %= MAX_CMD_LOG;
  }
  return count;
}

/*-----------------------------------*/
/*-- handle manual command display --*/
/*-----------------------------------*/
int manual_sound_commands(struct mame_bitmap *bitmap, int *fullRefresh) {
  int ii;
  /*-- we must have something to play with --*/
  if (!locals.soundCmd) return TRUE;

  handle_recording();

  /* Toggle command mode */
  if (keyboard_pressed_memory_repeat(SMDCMD_MODETOGGLE, REPEATKEY) &&
      !playCmd(-1, NULL)) {
    locals.soundMode = !locals.soundMode;
    /* clear screen */
    fillbitmap(bitmap,Machine->uifont->colortable[0],NULL);
    *fullRefresh = TRUE;
    /* start/stop all non-sound CPU(s) */
    for (ii = 0; ii < MAX_CPU; ii++)
      if ((Machine->drv->cpu[ii].cpu_type & ~CPU_FLAGS_MASK) &&
          (Machine->drv->cpu[ii].cpu_type & CPU_AUDIO_CPU) == 0)
        cpu_set_halt_line(ii, locals.soundMode);
  }

  if (locals.soundMode) {
    if (keyboard_pressed_memory_repeat(SMDCMD_DIGITTOGGLE, REPEATKEY)) {
      locals.digitMode = !locals.digitMode;
      /* clear screen */
      fillbitmap(bitmap,Machine->uifont->colortable[0],NULL);
    }
    /*-- some general help --*/
    core_textOutf(SND_XROW, 30, BLACK,   "* * * SOUND COMMAND MODE * * *");
    core_textOutf(SND_XROW, 45, BLACK,   "SPACE       Play sound");
    core_textOutf(SND_XROW, 55, BLACK,   "DEL         Manual / Command mode");

    if (locals.digitMode) {
      /*-- specific help --*/
      core_textOutf(SND_XROW, 65, BLACK, "LEFT/RIGHT  Move Left/Right");
      core_textOutf(SND_XROW, 75, BLACK, "UP/DOWN     Digit +1/-1");
      if      ((keyboard_pressed_memory_repeat(SMDCMD_UP, REPEATKEY)) &&
               (locals.digits[locals.currDigit] < 0x10))
        locals.digits[locals.currDigit] += 1;
      else if ((keyboard_pressed_memory_repeat(SMDCMD_DOWN, REPEATKEY)) &&
               (locals.digits[locals.currDigit] > 0x00))
        locals.digits[locals.currDigit] -= 1;
      else if ((keyboard_pressed_memory_repeat(SMDCMD_PREV, REPEATKEY)) &&
               (locals.currDigit > 0))
        locals.currDigit -= 1;
      else if ((keyboard_pressed_memory_repeat(SMDCMD_NEXT, REPEATKEY)) &&
               (locals.currDigit < (MAX_CMD_LENGTH*2-1)))
        locals.currDigit += 1;
      else if (keyboard_pressed_memory_repeat(SMDCMD_PLAY, REPEATKEY)) {
        int command[MAX_CMD_LENGTH];
        int count = 0;

        for (ii = 0; ii < MAX_CMD_LENGTH; ii++) {
          if ((locals.digits[ii*2] == 0x10) || (locals.digits[ii*2+1] == 0x10)) {
            locals.digits[ii*2] = locals.digits[ii*2+1] = 0x10;
            continue;
          }
          command[count++] = locals.digits[ii*2]*16+locals.digits[ii*2+1];
        }
        playCmd(count, command);
      }

      for (ii = 0; ii < MAX_CMD_LENGTH*2; ii++)
        core_textOutf(SND_XROW + 13*ii/2, 90, (ii == locals.currDigit) ? WHITE : BLACK,
                     (locals.digits[ii] > 0xf) ? "-" : "%x",locals.digits[ii]);
    }
    else { /* command mode */
      /*-- specific help --*/
      core_textOutf(SND_XROW, 65, BLACK, "UP/DOWN     Next/Prev command");
      if      ((keyboard_pressed_memory_repeat(SMDCMD_DOWN, REPEATKEY)) && locals.currCmd->prev)
        { locals.currCmd = locals.currCmd->prev; }
      else if ((keyboard_pressed_memory_repeat(SMDCMD_UP, REPEATKEY)) && locals.currCmd->next)
        { locals.currCmd = locals.currCmd->next; }
      else if (keyboard_pressed_memory_repeat(SMDCMD_PLAY, REPEATKEY))
        playCmd(locals.currCmd->length, locals.currCmd->cmd);
      core_textOutf(SND_XROW, 90, BLACK, "%-30s",locals.currCmd->name);
      for (ii = 0; ii < MAX_CMD_LENGTH; ii++)
        core_textOutf(SND_XROW + 13*ii, 100, BLACK,
                     (ii < locals.currCmd->length) ? "%02x" : "  ",
                     locals.currCmd->cmd[ii]);
    }
    core_textOutf(SND_XROW, 130, BLACK, "Last commands");
    for (ii = 0; ii < MAX_CMD_LOG; ii++)
      core_textOutf(SND_XROW + 13*ii, 140, BLACK, "%02x",
                   locals.cmdLog[(locals.firstLog+ii) % MAX_CMD_LOG]);
    playCmd(-1, NULL);
    return FALSE;
  }
  return TRUE; /* not in sound command mode */
}

static int checkName(const char *buf, const char *name) {
  while (*name)
    if ((*buf == '\0') || (*(buf++) != *(name++)))
      return FALSE;
  return TRUE;
}

static void readCmds(char *head) {
  void *f = osd_fopen(NULL, "sounds.dat", OSD_FILETYPE_HIGHSCORE_DB, 0);
  int getData = FALSE;

  if (f) {
    char buffer[MAX_LINE_LENGTH];
    while (osd_fgets(buffer, MAX_LINE_LENGTH, f)) {
      /* logerror("line=%s",buffer); */
      if (buffer[0] == ':') {
        if (getData) {
          struct cmds tmpCmd;
          char *tmp = &buffer[1];
          int   cmd = 0;

          /*-- fetch commands --*/
          tmpCmd.length = 0;
          for (;;) {
            if (isdigit(*tmp))
              cmd = cmd*16 + (*tmp) - '0';
            else if ((*tmp >= 'a') && (*tmp <= 'f'))
              cmd = cmd*16 + (*tmp) - 'a' + 10;
            else
              break;
            if (tmpCmd.length++ & 0x01) {
              tmpCmd.cmd[tmpCmd.length/2-1] = cmd;
              /* logerror("cmd=%x\n",cmd); */
              cmd = 0;
            }
            tmp += 1;
          }
          tmpCmd.length /= 2;
          strncpy(tmpCmd.name, &tmp[1], MAX_NAME_LENGTH);

          /*-- make sure name is less than MAX_NAME_LENGTH --*/
          cmd = strlen(&tmp[1]) - 1;
          if (cmd >= MAX_NAME_LENGTH)
            cmd = MAX_NAME_LENGTH - 1;
          tmpCmd.name[cmd] = '\0';

          /*-- check if same command already exists --*/
          {
            struct cmds *p = &cmds;

            while (p->next) {
              p = p->next;
              if ((p->length != tmpCmd.length) ||
                  (memcmp(p->cmd, tmpCmd.cmd, tmpCmd.length*sizeof(tmpCmd.cmd[0]))))
                continue;
              DBGLOG(("duplicate found old=%s, new=%s\n",p->name,tmpCmd.name));
              strcpy(p->name, tmpCmd.name);
              goto found;
            }
            p->next = malloc(sizeof(struct cmds));
            memcpy(p->next, &tmpCmd, sizeof(tmpCmd));
            p->next->prev = p;
            p = p->next;
            p->next = NULL;
found:      ;
          }
        }
      }
      else if ((buffer[0] == ';') || (buffer[0] == '#') ||
               (buffer[0] == '\n') || (buffer[0] == ' '))
        continue;
      else
        getData = (checkName(buffer, head) ||
                   checkName(buffer, Machine->gamedrv->name) ||
                   (Machine->gamedrv->clone_of &&
                    !(Machine->gamedrv->clone_of->flags & NOT_A_DRIVER) &&
                    checkName(buffer, Machine->gamedrv->clone_of->name)));
    } /* while */
    osd_fclose(f);
  } /* if */
}

static void clrCmds(void) {
  struct cmds *p1 = cmds.next;
  if (p1) {
    while (p1->next) {
      p1 = p1->next;
      free(p1->prev);
    }
    cmds.next = NULL;
  }
}

static int playCmd(int length, int *cmd) {
  static int cmdIdx = 0;
  static int nextCmd[MAX_CMD_LENGTH+1];

  /*-- send a new command --*/
  if (length >= 0) {
    if (cmdIdx) return TRUE; /* Already playing */
    memcpy(nextCmd, cmd, MAX_CMD_LENGTH*sizeof(nextCmd[0]));
    nextCmd[length] = -1;
    cmdIdx = 1;
    return FALSE;
  }
  /* logerror("cmdIdx = %d\n",cmdIdx); */
  /* currently sending a command ? */
  if (cmdIdx > 0) {
    cmdIdx += 1;
    if ((cmdIdx % 4) == 2) { /* only send cmd every 4th frame */
      int comm = nextCmd[cmdIdx/4];
      if      (comm == -1)
        cmdIdx = 0;
      else
        locals.soundCmd(0,comm);
    }
  }
  return FALSE;
}


/*---------------------------------------------------------------------------*/
/* This is just copied from SJE's file */
#ifdef PINMAME_EXT


/* Local Functions */
static int  wave_open(char *filename);

static void *wave_file = NULL;
static unsigned wave_offs;

//NOT STATIC so that video.c can access it.
int recording=0; /*Flag signifying if we're currently recording a wav file!*/

#define is_stereo ((Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO) != 0)
/*------------------------------------
  Handle Wave Recording
  ----------------------------------------*/
static void handle_recording(void) {
  /*If Wave Toggle Key pressed, toggle Recording of Wave File*/
  if(keyboard_pressed_memory_repeat(SMDCMD_RECORDTOGGLE,REPEATKEY)) {
    if (wave_file)
      { wave_close(); recording = 2; } /*finished recording*/
    else {
      static int nextWaveFileNo = 0; /* static makes it faster second time */
      char name[120];
      /* avoid overwriting existing files */
      /* first of all try with "gamename.wav" */
      sprintf(name,"%.8s.wav", Machine->gamedrv->name);
      if (osd_faccess(name,OSD_FILETYPE_WAVEFILE)) {
        do {
          /* otherwise use "nameNNNN.wav" */
          sprintf(name,"%.4s%04d.wav",Machine->gamedrv->name,++nextWaveFileNo);
        } while (osd_faccess(name, OSD_FILETYPE_WAVEFILE));
      }
      logerror("Using %s for wave filename!\n", name);
      /*Able to start recording?*/
      if (wave_open(name) > 0) recording = 1; /*recording succeeded*/
      else                    recording =-1; /*failed to record..*/
    }
  }
}

#ifdef	LSB_FIRST
#define intel(x)    (x)
#else
#define intel(x)	((x)<<24)|((x)&0xff00)<<8)|((x)&0xff0000)>>8)|((x)>>24)
#endif

static void wave_close(void) {
  UINT32 filesize, wavesize, temp32;

  if (wave_file) {
    filesize = wave_offs;
    osd_fseek(wave_file, 4, SEEK_SET);
    temp32 = intel(filesize);
    osd_fwrite(wave_file, &temp32, 4);

    wavesize = wave_offs - 44;
    osd_fseek(wave_file, 40, SEEK_SET);
    temp32 = intel(wavesize);
    osd_fwrite(wave_file, &temp32, 4);

    osd_fclose(wave_file);
    wave_file = NULL;
  }
}

static int wave_open(char *filename) {
  UINT32 filesize, temp32;
  UINT16 temp16;

  wave_file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_WAVEFILE, 1);

  if (!wave_file) {
    logerror("Unable to create new wave file %s\n",filename);
    return -1;
  }

    filesize =
		4 + 	/* 'RIFF' */
		4 + 	/* size of entire file */
		4 + 	/* 'WAVE' */
		4 + 	/* size of WAVE tag */
		16 +	/* WAVE tag (including 'fmt ' tag) */
		4 + 	/* 'data' */
		4;		/* size of data */

    /* write the core header for a WAVE file */
    wave_offs += osd_fwrite(wave_file, "RIFF", 4);
    /* filesize (40 bytes so far, this is updated when the file is closed) */
    temp32 = intel(filesize);
    wave_offs += osd_fwrite(wave_file, &temp32, 4);
    /* write the RIFF file type 'WAVE' */
    wave_offs += osd_fwrite(wave_file, "WAVE", 4);
    /* write a format tag */
    wave_offs += osd_fwrite(wave_file, "fmt ", 4);
    /* size of the following 'fmt ' fields */
    temp32 = intel(16);
    wave_offs += osd_fwrite(wave_file, &temp32, 4);
    /* format: PCM */
    temp16 = 1;
    wave_offs += osd_fwrite_lsbfirst(wave_file, &temp16, 2);
    /* channels: one or two?  */
    temp16 = is_stereo ? 2 : 1;
    wave_offs += osd_fwrite_lsbfirst(wave_file, &temp16, 2);
    /* sample rate */
    temp32 = intel(Machine->sample_rate);
    wave_offs += osd_fwrite(wave_file, &temp32, 4);
    /* byte rate */
    temp32 = intel(Machine->sample_rate * 16 * (is_stereo ? 2 : 1) / 8);
    wave_offs += osd_fwrite(wave_file, &temp32, 4);
    /* block align */
    temp16 = 2;
    wave_offs += osd_fwrite_lsbfirst(wave_file, &temp16, 2);
    /* resolution */
    temp16 = 16;
    wave_offs += osd_fwrite_lsbfirst(wave_file, &temp16, 2);
    /* 'data' tag */
    wave_offs += osd_fwrite(wave_file, "data", 4);
    /* data size */
    temp32 = 0;
    wave_offs += osd_fwrite(wave_file, &temp32, 4);
    if (wave_offs < 44) {
      logerror("WAVE write error at offs %d\n", wave_offs); return -1;
    }
    return 1;
}

void pm_wave_record(INT16 *buffer, int samples) {
  if (wave_file) {
    int written = osd_fwrite_lsbfirst(wave_file, buffer, samples * 2);
    wave_offs += written;
    if (written < samples * 2) {
      /* oops! the disk is full, better stop now */
      logerror("WAVE write error at offs %d\n", wave_offs);
      wave_close();
    }
  }
}

#endif /* PINMAME_EXT */

