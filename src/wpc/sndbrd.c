#ifndef SNDBRD_RECURSIVE
#  define SNDBRD_RECURSIVE
#  include "driver.h"
#  include "core.h"
#  include "snd_cmd.h"
#  include "sndbrd.h"
#  define SNDBRDINTF(name) extern const struct sndbrdIntf name##Intf;
#  include "sndbrd.c"
#  undef SNDBRDINTF
#  define SNDBRDINTF(name) &name##Intf,
   static const struct sndbrdIntf noSound = {0};
   static const struct sndbrdIntf *allsndboards[] = { &noSound,
#  include "sndbrd.c"
   NULL};

static struct intfData {
  const struct sndbrdIntf *brdIntf;
  WRITE_HANDLER((*data_cb));
  WRITE_HANDLER((*ctrl_cb));
  int type;
  int manCmdBuf; // if board requires 2 sound commands, keep last value here.
} intf[2];

void sndbrd_init(int brdNo, int brdType, int cpuNo, UINT8 *romRegion,
                 WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb))) {
  const struct sndbrdIntf *b = allsndboards[brdType>>8];
  struct intfData *i = &intf[brdNo];
  struct sndbrdData brdData;
#if HAS_SAMPLES
  if ((brdType != SNDBRD_NONE) &&
      ((b->flags & SNDBRD_NOTSOUND) ||
       (Machine->drv->sound[0].sound_type && (Machine->drv->sound[0].sound_type != SOUND_SAMPLES))))
#else // HAS_SAMPLES
  if ((brdType != SNDBRD_NONE) &&
      ((b->flags & SNDBRD_NOTSOUND) || Machine->drv->sound[0].sound_type))
#endif // HAS_SAMPLES
  {
    brdData.boardNo = brdNo; brdData.subType = brdType & 0xff;
    brdData.cpuNo   = cpuNo; brdData.romRegion = romRegion;
    i->brdIntf = b;
    i->type    = brdType;
    i->data_cb = data_cb;
    i->ctrl_cb = ctrl_cb;
    i->manCmdBuf = -1;
    if (b && (coreGlobals.soundEn || b->flags & SNDBRD_NOTSOUND) && b->init)
      b->init(&brdData);
  }
}

int sndbrd_exists(int board) {
  return (intf[board].brdIntf &&
          ((intf[board].brdIntf->flags & SNDBRD_NOTSOUND) == 0));
}
const char* sndbrd_typestr(int board) {
  return intf[board].brdIntf ? intf[board].brdIntf->typestr : NULL;
}

void sndbrd_exit(int board) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->exit)
    b->exit(board);
  memset(&intf[board],0,sizeof(intf[0]));
}
void sndbrd_diag(int board, int button) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->diag)
    b->diag(button);
}

void sndbrd_data_w(int board, int data) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
    if(b && (b->flags & SNDBRD_NOTSOUND)==0)
		snd_cmd_log(board, data);
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->data_w) {
#if 0
    if((b->flags & SNDBRD_NOTSOUND)==0)
		snd_cmd_log(board, data);
#endif
    if (b->flags & SNDBRD_NODATASYNC)
      b->data_w(board, data);
    else
      sndbrd_sync_w(b->data_w, board, data);
  }
}
int sndbrd_data_r(int board) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->data_r)
    return b->data_r(board);
  return 0;
}
void sndbrd_ctrl_w(int board, int data) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->ctrl_w) {
    if (b->flags & SNDBRD_NOCTRLSYNC)
      b->ctrl_w(board, data);
    else
      sndbrd_sync_w(b->ctrl_w, board, data);
  }
}
int sndbrd_ctrl_r(int board) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->ctrl_r)
    return b->ctrl_r(board);
  return 0;
}
void sndbrd_ctrl_cb(int board, int data) {
  if (intf[board].ctrl_cb) {
    if (intf[board].brdIntf && (intf[board].brdIntf->flags & SNDBRD_NOCBSYNC))
      intf[board].ctrl_cb(board, data);
    else
      sndbrd_sync_w(intf[board].ctrl_cb, board, data);
  }
}
void sndbrd_data_cb(int board, int data) {
  if (intf[board].data_cb) {
    if (intf[board].brdIntf && (intf[board].brdIntf->flags & SNDBRD_NOCBSYNC))
      intf[board].data_cb(board, data);
    else
      sndbrd_sync_w(intf[board].data_cb, board, data);
  }
}
void sndbrd_manCmd(int board, int cmd) {
  const struct sndbrdIntf *b = intf[board].brdIntf;
  if (b && (coreGlobals.soundEn || (b->flags & SNDBRD_NOTSOUND)) && b->manCmd_w) {
    if ((b->flags & SNDBRD_DOUBLECMD) && (intf[board].manCmdBuf < 0))
      { intf[board].manCmdBuf = cmd; return; }
    b->manCmd_w(intf[board].manCmdBuf, cmd); intf[board].manCmdBuf = -1;
  }
}
void sndbrd_0_init(int brdType, int cpuNo, UINT8 *romRegion,
                   WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb))) {
  sndbrd_init(0, brdType, cpuNo, romRegion, data_cb, ctrl_cb);
}
void sndbrd_1_init(int brdType, int cpuNo, UINT8 *romRegion,
                   WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb))) {
  sndbrd_init(1, brdType, cpuNo, romRegion, data_cb, ctrl_cb);
}
void sndbrd_0_exit(void) { sndbrd_exit(0); }
void sndbrd_1_exit(void) { sndbrd_exit(1); }
void sndbrd_0_diag(int button) { sndbrd_diag(0,button); }
void sndbrd_1_diag(int button) { sndbrd_diag(1,button); }
WRITE_HANDLER(sndbrd_0_data_w) { sndbrd_data_w(0,data); }
WRITE_HANDLER(sndbrd_1_data_w) { sndbrd_data_w(1,data); }
WRITE_HANDLER(sndbrd_0_ctrl_w) { sndbrd_ctrl_w(0,data); }
WRITE_HANDLER(sndbrd_1_ctrl_w) { sndbrd_ctrl_w(1,data); }
 READ_HANDLER(sndbrd_0_data_r) { return sndbrd_data_r(0); }
 READ_HANDLER(sndbrd_1_data_r) { return sndbrd_data_r(1); }
 READ_HANDLER(sndbrd_0_ctrl_r) { return sndbrd_ctrl_r(0); }
 READ_HANDLER(sndbrd_1_ctrl_r) { return sndbrd_ctrl_r(1); }
int sndbrd_type(int offset) { return intf[offset].type; }
int sndbrd_0_type()         { return intf[0].type; }
int sndbrd_1_type()         { return intf[1].type; }

#define MAX_SYNCS 5
static struct {
  int used;
  WRITE_HANDLER((*handler));
  int offset,data;
} syncData[MAX_SYNCS];

static void sndbrd_doSync(int param) {
  syncData[param].used = FALSE;
  syncData[param].handler(syncData[param].offset,syncData[param].data);
}

void sndbrd_sync_w(WRITE_HANDLER((*handler)),int offset, int data) {
  int ii;

  if (!handler) return;
  for (ii = 0; ii < MAX_SYNCS; ii++)
    if (!syncData[ii].used) {
      syncData[ii].used = TRUE;
      syncData[ii].handler = handler;
      syncData[ii].offset = offset;
      syncData[ii].data = data;
      timer_set(TIME_NOW, ii, sndbrd_doSync);
      return;
    }
  DBGLOG(("Warning: out of sync timers"));
}
const struct sndbrdIntf NULLIntf = { 0 }; // remove when all boards below works.
#else /* SNDBRD_RECURSIVE */
/* Sound board drivers */
  SNDBRDINTF(s11cs)
  SNDBRDINTF(wpcs)
  SNDBRDINTF(dcs)
  SNDBRDINTF(by32)
  SNDBRDINTF(by51)
  SNDBRDINTF(s11js)
  SNDBRDINTF(by61)
  SNDBRDINTF(by45)
  SNDBRDINTF(byTCS)
  SNDBRDINTF(bySD)
  SNDBRDINTF(s67s)
  SNDBRDINTF(s11s)
  SNDBRDINTF(de2s)
  SNDBRDINTF(de1s)
  SNDBRDINTF(dedmd16)
  SNDBRDINTF(dedmd32)
  SNDBRDINTF(dedmd64)
  SNDBRDINTF(gts80s)
  SNDBRDINTF(gts80ss)
  SNDBRDINTF(gts80b)
  SNDBRDINTF(hankin)
  SNDBRDINTF(atari1s)
  SNDBRDINTF(atari2s)
  SNDBRDINTF(taito)
  SNDBRDINTF(zac1311)
  SNDBRDINTF(zac1125)
  SNDBRDINTF(zac1346)
  SNDBRDINTF(zac1370)
  SNDBRDINTF(techno)
  SNDBRDINTF(st100)
  SNDBRDINTF(st300)
  SNDBRDINTF(astro)
  SNDBRDINTF(gpSSU1)
  SNDBRDINTF(gpSSU2)
  SNDBRDINTF(gpSSU4)
  SNDBRDINTF(gpMSU1)
  SNDBRDINTF(gpMSU3)
  SNDBRDINTF(alvgs1)
  SNDBRDINTF(alvgs2)
  SNDBRDINTF(alvgdmd)
  SNDBRDINTF(capcoms)
  SNDBRDINTF(spinb)
  SNDBRDINTF(mrgame)
  SNDBRDINTF(de3s)
  SNDBRDINTF(rowamet)
  SNDBRDINTF(nuova)
  SNDBRDINTF(grand)
  SNDBRDINTF(jvh)
#endif /* SNDBRD_RECURSIVE */
