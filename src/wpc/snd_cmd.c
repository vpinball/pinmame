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
#include "sndbrd.h"
#include "snd_cmd.h"

#ifdef MAME_DEBUG
extern UINT8 debugger_focus;
#endif

#define MAX_CMD_LENGTH   6
#define MAX_NAME_LENGTH 30
#define MAX_LINE_LENGTH 100
#define SMDCMD_DIGITTOGGLE SMDCMD_ZERO

static int playCmd(int length, int *cmd);
static int checkName(const char *buf, const char *name);
static void readCmds(int boardNo, const char *head);
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
  int boards; // 0 = none, 1 = board0, 2 = board1, 3 = both
  int rollover; 
  int cmdIdx;
  int nextCmd[MAX_CMD_LENGTH+1];
} locals;

static void wave_init(void);
static void wave_exit(void);
static void wave_handle(void);
/*---------------------------------*/
/*-- init manual sound commands  --*/
/*---------------------------------*/
void snd_cmd_init(void) {
  int ii;
  memset(&locals, 0, sizeof(locals));
  locals.currCmd = &cmds;
  locals.boards = sndbrd_exists(0) + 2*sndbrd_exists(1);
  switch (locals.boards) {
    case 1: readCmds(-1, sndbrd_typestr(0)); break;
    case 2: readCmds(-1, sndbrd_typestr(1)); break;
    case 3: readCmds( 0, sndbrd_typestr(0));
            readCmds( 1, sndbrd_typestr(1)); break;
  }
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
void snd_cmd_log(int boardNo, int cmd) {
  if (locals.soundMode || (locals.boards == 0)) return; // Don't log from within sound commander
  if (locals.boards == 3) {
    locals.cmdLog[locals.firstLog++] = boardNo;
    locals.firstLog %= MAX_CMD_LOG;
  }
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
static void insert_char(char data) {
  locals.digits[locals.currDigit] = data;
  if (locals.rollover || (locals.currDigit < (MAX_CMD_LENGTH*2-1))) {
    if (++locals.currDigit > (MAX_CMD_LENGTH*2-1)) locals.currDigit = 0;
  }
}

int manual_sound_commands(struct mame_bitmap *bitmap) {
  int ii;

  /*-- we must have something to play with --*/
  if (!locals.boards) return TRUE;

#ifdef MAME_DEBUG
  /*-- Keypresses interferes with mame debugger (while it's active) --*/
  if(mame_debug && debugger_focus)	return TRUE;
#endif

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
      if ((Machine->drv->cpu[ii].cpu_type) &&
          (Machine->drv->cpu[ii].cpu_flags == 0))
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
      core_textOutf(SND_XROW, 85, BLACK, "INSERT      Turn Rollover %s",(locals.rollover)?"Off":"On ");
      if      ((keyboard_pressed_memory_repeat(KEYCODE_0, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_0_PAD, REPEATKEY)))
        insert_char(0x00);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_1, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_1_PAD, REPEATKEY)))
        insert_char(0x01);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_2, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_2_PAD, REPEATKEY)))
        insert_char(0x02);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_3, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_3_PAD, REPEATKEY)))
        insert_char(0x03);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_4, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_4_PAD, REPEATKEY)))
        insert_char(0x04);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_5, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_5_PAD, REPEATKEY)))
        insert_char(0x05);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_6, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_6_PAD, REPEATKEY)))
        insert_char(0x06);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_7, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_7_PAD, REPEATKEY)))
        insert_char(0x07);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_8, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_8_PAD, REPEATKEY)))
        insert_char(0x08);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_9, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_9_PAD, REPEATKEY)))
        insert_char(0x09);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_A, REPEATKEY))
        insert_char(0x0a);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_B, REPEATKEY))
        insert_char(0x0b);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_C, REPEATKEY))
        insert_char(0x0c);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_D, REPEATKEY))
        insert_char(0x0d);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_E, REPEATKEY))
        insert_char(0x0e);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_F, REPEATKEY))
        insert_char(0x0f);
      else if ((keyboard_pressed_memory_repeat(KEYCODE_MINUS, REPEATKEY)) ||
               (keyboard_pressed_memory_repeat(KEYCODE_MINUS_PAD, REPEATKEY)))
        insert_char(0x10);
      else if  (keyboard_pressed_memory_repeat(KEYCODE_HOME, REPEATKEY))
        locals.currDigit = 0;
      else if  (keyboard_pressed_memory_repeat(KEYCODE_END, REPEATKEY))
        locals.currDigit = MAX_CMD_LENGTH*2-1;
      else if ((keyboard_pressed_memory_repeat(SMDCMD_UP, REPEATKEY)) &&
               (locals.rollover || (locals.digits[locals.currDigit] < 0x10))) {
        if (++locals.digits[locals.currDigit] > 0x10) locals.digits[locals.currDigit] = 0;
      }
      else if ((keyboard_pressed_memory_repeat(SMDCMD_DOWN, REPEATKEY)) &&
               (locals.rollover || (locals.digits[locals.currDigit] > 0x00))) {
        if (--locals.digits[locals.currDigit] < 0x00) locals.digits[locals.currDigit] = 0x10;
      }
      else if ((keyboard_pressed_memory_repeat(SMDCMD_PREV, REPEATKEY)) &&
               (locals.rollover || (locals.currDigit > 0))) {
        if (--locals.currDigit < 0) locals.currDigit = (MAX_CMD_LENGTH*2-1);
      }
      else if ((keyboard_pressed_memory_repeat(SMDCMD_NEXT, REPEATKEY)) &&
               (locals.rollover || (locals.currDigit < (MAX_CMD_LENGTH*2-1)))) {
        if (++locals.currDigit > (MAX_CMD_LENGTH*2-1)) locals.currDigit = 0;
      }
      else if (keyboard_pressed_memory_repeat(SMDCMD_INSERT, REPEATKEY))
        locals.rollover = !locals.rollover;
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
        core_textOutf(SND_XROW + 13*ii/2, 95, (ii == locals.currDigit) ? WHITE : BLACK,
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
      core_textOutf(SND_XROW, 95, BLACK, "%-30s",locals.currCmd->name);
      for (ii = 0; ii < MAX_CMD_LENGTH; ii++)
        core_textOutf(SND_XROW + 13*ii, 105, BLACK,
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

static void readCmds(int boardNo, const char *head) {
  mame_file *f = mame_fopen(NULL, "sounds.dat", FILETYPE_HIGHSCORE_DB, 0);
  int getData = 0;

  if (f) {
    char buffer[MAX_LINE_LENGTH];
    while (mame_fgets(buffer, MAX_LINE_LENGTH, f)) {
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
              if ((getData == 2) && (boardNo >= 0)) {
                tmpCmd.cmd[tmpCmd.length/2-1] = boardNo; tmpCmd.length += 2;
              }
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
      else {
        if (head && checkName(buffer, head))
          getData = 2;
        else
          getData = (boardNo != 0) &&
                    (checkName(buffer, Machine->gamedrv->name) ||
                     (Machine->gamedrv->clone_of &&
                      !(Machine->gamedrv->clone_of->flags & NOT_A_DRIVER) &&
                      checkName(buffer, Machine->gamedrv->clone_of->name)));

      }
    } /* while */
    mame_fclose(f);
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
  /*-- send a new command --*/
  if (length >= 0) {
    if (locals.cmdIdx) return TRUE; /* Already playing */
    memcpy(locals.nextCmd, cmd, MAX_CMD_LENGTH*sizeof(locals.nextCmd[0]));
    locals.nextCmd[length] = -1;
    locals.cmdIdx = 1;
    return FALSE;
  }
  /* currently sending a command ? */
  if (locals.cmdIdx > 0) {
    locals.cmdIdx += 1;
    if ((locals.cmdIdx % 4) == 2) { /* only send cmd every 4th frame */
      if      (locals.nextCmd[locals.cmdIdx/4] == -1)
        locals.cmdIdx = 0;
      else if (locals.boards == 3) { // if we have 2 sound boards we need to send board no as well
        sndbrd_manCmd(locals.nextCmd[locals.cmdIdx/4], locals.nextCmd[locals.cmdIdx/4+1]); locals.cmdIdx += 4;
      }
      else
        sndbrd_manCmd(locals.boards - 1, locals.nextCmd[locals.cmdIdx/4]);
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
    if (mame_faccess(name,FILETYPE_WAVE)) {
      do { /* otherwise use "nameNNNN.wav" */
        sprintf(name,"%.4s%04d",Machine->gamedrv->name,++wavelocals.nextWaveFileNo);
      } while (mame_faccess(name, FILETYPE_WAVE));
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

  wavelocals.file = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_WAVE, 1);

  if (!wavelocals.file) return -1;
  /* write the core header for a WAVE file */
  wavelocals.offs = mame_fwrite(wavelocals.file, "RIFF", 4);
  /* filesize, updated when the file is closed */
  temp32 = 0;
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp32, 4);
  /* write the RIFF file type 'WAVE' */
  wavelocals.offs += mame_fwrite(wavelocals.file, "WAVE", 4);
  /* write a format tag */
  wavelocals.offs += mame_fwrite(wavelocals.file, "fmt ", 4);
  /* size of the following 'fmt ' fields */
  temp32 = intel32(16);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp32, 4);
  /* format: PCM */
  temp16 = intel16(1);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp16, 2);
  /* channels: one or two?  */
  temp16 = intel16(channels);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp16, 2);
  /* sample rate */
  temp32 = intel32(Machine->sample_rate);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp32, 4);
  /* byte rate */
  temp32 = intel32(channels * Machine->sample_rate * 2);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp32, 4);
  /* block align */
  temp16 = intel16(2*channels);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp16, 2);
  /* resolution */
  temp16 = intel16(16);
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp16, 2);
  /* 'data' tag */
  wavelocals.offs += mame_fwrite(wavelocals.file, "data", 4);
  /* data size */
  temp32 = 0;
  wavelocals.offs += mame_fwrite(wavelocals.file, &temp32, 4);
  if (wavelocals.offs < 44) return -1;
  return 1;
}

static void wave_close(void) {
  if (wavelocals.recording == 1) {
    UINT32 temp32;
    mame_fseek(wavelocals.file, 4, SEEK_SET);
    temp32 = intel32(wavelocals.offs);
    mame_fwrite(wavelocals.file, &temp32, 4);

    mame_fseek(wavelocals.file, 40, SEEK_SET);
    temp32 = intel32(wavelocals.offs-44);
    mame_fwrite(wavelocals.file, &temp32, 4);

    mame_fclose(wavelocals.file);
    wavelocals.file = NULL;
  }
}
/*--------------------*/
/* exported functions */
/*--------------------*/
/* called from mixer.c */
void pm_wave_record(INT16 *buffer, int samples) {
  if (wavelocals.recording == 1) {
    int written = mame_fwrite_lsbfirst(wavelocals.file, buffer, samples * 2 * CHANNELCOUNT);
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
