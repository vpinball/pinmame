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
#include "core.h"
#include "wmssnd.h"
#include "snd_cmd.h"
#include "wpc.h"

#define MAX_CMD_LENGTH   6
#define MAX_NAME_LENGTH 30
#define MAX_LINE_LENGTH 100
#define SMDCMD_DIGITTOGGLE SMDCMD_ZERO

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

static void wave_init(void);
static void wave_exit(void);
static void wave_handle(void);
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
  wave_init();
}

/*----------------------------------------*/
/*-- clean up manual sound command data --*/
/*----------------------------------------*/
void snd_cmd_exit(void) {
  clrCmds();
  wave_exit();
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
int manual_sound_commands(struct mame_bitmap *bitmap) {
  int ii;
  /*-- we must have something to play with --*/
  if (!locals.soundCmd) return TRUE;

  /*-- handle recording --*/
  if (keyboard_pressed_memory_repeat(SMDCMD_RECORDTOGGLE,REPEATKEY))
    wave_handle();

  /* Toggle command mode */
  if (keyboard_pressed_memory_repeat(SMDCMD_MODETOGGLE, REPEATKEY) &&
      !playCmd(-1, NULL)) {
    locals.soundMode = !locals.soundMode;
    /* clear screen */
    fillbitmap(bitmap,Machine->uifont->colortable[0],NULL);
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
/* Local Functions */
static int wave_open(char *filename);
static void wave_close(void);
static struct {
  void  *file;
  UINT32 offs;
  int recording;
  int spinner;
  int nextWaveFileNo;
} wavelocals;

static void wave_init(void) {
  memset(&wavelocals, 0, sizeof(wavelocals));
}
static void wave_exit(void) {
  if (wavelocals.recording == 1) {
    wave_close(); wavelocals.recording = 0;
  }
}

/*------------------------------------
  Handle Wave Recording
  ----------------------------------------*/
static void wave_handle(void) {
  if (wavelocals.recording == 1)
    { wave_close(); wavelocals.recording = 0; } /*finished recording*/
  else {
    char name[120];
    /* avoid overwriting existing files */
    /* first of all try with "gamename.wav" */
    sprintf(name,"%.8s", Machine->gamedrv->name);
    if (osd_faccess(name,OSD_FILETYPE_WAVEFILE)) {
      do { /* otherwise use "nameNNNN.wav" */
        sprintf(name,"%.4s%04d",Machine->gamedrv->name,++wavelocals.nextWaveFileNo);
      } while (osd_faccess(name, OSD_FILETYPE_WAVEFILE));
    }
    wavelocals.recording = wave_open(name);
  }
}

#ifdef LSB_FIRST
#define intel32(x) (x)
#define intel16(x) (x)
#else
#define intel32(x) ((((x)<<24) | (((UINT32)(x))>>24) | (((x) & 0x0000ff00)<<8) | (((x) & 0x00ff0000)>>8)))
#define intel16(x) (((x)<<8) | ((x)>>8))
#endif
#define CHANNELCOUNT ((Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO) ? 2 : 1)
static int wave_open(char *filename) {
  const int channels = CHANNELCOUNT;
  UINT16 temp16;
  UINT32 temp32;

  wavelocals.file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_WAVEFILE, 1);

  if (!wavelocals.file) return -1;
  /* write the core header for a WAVE file */
  wavelocals.offs = osd_fwrite(wavelocals.file, "RIFF", 4);
  /* filesize, updated when the file is closed */
  temp32 = 0;
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp32, 4);
  /* write the RIFF file type 'WAVE' */
  wavelocals.offs += osd_fwrite(wavelocals.file, "WAVE", 4);
  /* write a format tag */
  wavelocals.offs += osd_fwrite(wavelocals.file, "fmt ", 4);
  /* size of the following 'fmt ' fields */
  temp32 = intel32(16);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp32, 4);
  /* format: PCM */
  temp16 = intel16(1);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp16, 2);
  /* channels: one or two?  */
  temp16 = intel16(channels);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp16, 2);
  /* sample rate */
  temp32 = intel32(Machine->sample_rate);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp32, 4);
  /* byte rate */
  temp32 = intel32(channels * Machine->sample_rate * 2);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp32, 4);
  /* block align */
  temp16 = intel16(2*channels);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp16, 2);
  /* resolution */
  temp16 = intel16(16);
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp16, 2);
  /* 'data' tag */
  wavelocals.offs += osd_fwrite(wavelocals.file, "data", 4);
  /* data size */
  temp32 = 0;
  wavelocals.offs += osd_fwrite(wavelocals.file, &temp32, 4);
  if (wavelocals.offs < 44) return -1;
  return 1;
}

static void wave_close(void) {
  if (wavelocals.recording == 1) {
    UINT32 temp32;
    osd_fseek(wavelocals.file, 4, SEEK_SET);
    temp32 = intel32(wavelocals.offs);
    osd_fwrite(wavelocals.file, &temp32, 4);

    osd_fseek(wavelocals.file, 40, SEEK_SET);
    temp32 = intel32(wavelocals.offs-44);
    osd_fwrite(wavelocals.file, &temp32, 4);

    osd_fclose(wavelocals.file);
    wavelocals.file = NULL;
  }
}
/*--------------------*/
/* exported functions */
/*--------------------*/
/* called from mixer.c */
void pm_wave_record(INT16 *buffer, int samples) {
  if (wavelocals.recording == 1) {
    int written = osd_fwrite_lsbfirst(wavelocals.file, buffer, samples * 2 * CHANNELCOUNT);
    wavelocals.offs += written;
    if (written < samples * 2) {
      wave_close(); wavelocals.recording = -1;
    }
  }
}

/* called from video update */
void pm_wave_disp(struct mame_bitmap *abitmap) {
  char buf[25];
  if (wavelocals.recording == 0) {
    if (wavelocals.spinner == 0) return;
    strcpy(buf,"                  ");
    wavelocals.spinner = 0;
  }
  else {
    wavelocals.spinner = (wavelocals.spinner + 1);
    if (wavelocals.recording == -1) {
      if (wavelocals.spinner == 180)
        wavelocals.recording = 0;
      else {
        strcpy(buf, "Wave record error!");
        wavelocals.spinner = 0;
      }
    }
    else {
      const char spinner[4] = {'|','/','-','\\'};
      sprintf(buf,"Recording %c", spinner[(wavelocals.spinner/10) & 3]);
    }
  }
  ui_text(abitmap, buf, 0, 0);
}
