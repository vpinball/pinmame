#ifndef SNDBRD_RECURSIVE
#  define SNDBRD_RECURSIVE
#  include "driver.h"
#  include "core.h"
#  include "sndbrd.h"
#  define SNDBRDINTF(name) extern const struct sndbrdIntf name##Intf;
#  include "sndbrd.c"
#  undef SNDBRDINTF
#  define SNDBRDINTF(name) &name##Intf,
   static const struct sndbrdIntf noSound = {0};
   static const struct sndbrdIntf *allsndboards[] = { &noSound,
#  include "sndbrd.c"
   NULL};

static struct {
  const struct sndbrdIntf *brdIntf;
  WRITE_HANDLER((*data_cb));
  WRITE_HANDLER((*ctrl_cb));
  int type;
} intf[2];

void sndbrd_init(int brdNo, int brdType, int cpuNo, UINT8 *romRegion,
                 WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb))) {
  if (coreGlobals.soundEn) {
    struct sndbrdData brdData;
    brdData.boardNo = brdNo; brdData.subType = brdType & 0xff;
    brdData.cpuNo   = cpuNo; brdData.romRegion = romRegion;
    intf[brdNo].brdIntf = allsndboards[brdType>>8];
    intf[brdNo].type    = brdType;
    intf[brdNo].data_cb = data_cb;
    intf[brdNo].ctrl_cb = ctrl_cb;
    if (intf[brdNo].brdIntf->init) intf[brdNo].brdIntf->init(&brdData);
  }
}

void sndbrd_exit(int board) {
  if (coreGlobals.soundEn && intf[board].brdIntf && intf[board].brdIntf->exit)
    intf[board].brdIntf->exit(board);
  memset(&intf[board],0,sizeof(intf[0]));
}
void sndbrd_diag(int board, int button) {
  if (coreGlobals.soundEn && intf[board].brdIntf && intf[board].brdIntf->diag)
    intf[board].brdIntf->diag(button);
}

WRITE_HANDLER(sndbrd_data_w) {
  if (coreGlobals.soundEn && intf[offset].brdIntf && intf[offset].brdIntf->data_w) {
    if (intf[offset].brdIntf->flags & SNDBRD_NODATASYNC)
      intf[offset].brdIntf->data_w(offset, data);
    else
      sndbrd_sync_w(intf[offset].brdIntf->data_w, offset, data);
  }
}
READ_HANDLER(sndbrd_data_r) {
  return (coreGlobals.soundEn && intf[offset].brdIntf && intf[offset].brdIntf->data_r) ? 
         intf[offset].brdIntf->data_r(offset) : 0;
}
WRITE_HANDLER(sndbrd_ctrl_w) {
  if (coreGlobals.soundEn && intf[offset].brdIntf && intf[offset].brdIntf->ctrl_w) {
    if (intf[offset].brdIntf->flags & SNDBRD_NOCTRLSYNC)
      intf[offset].brdIntf->ctrl_w(offset, data);
    else
      sndbrd_sync_w(intf[offset].brdIntf->ctrl_w, offset, data);
  }
}
READ_HANDLER(sndbrd_ctrl_r) {
  return (coreGlobals.soundEn && intf[offset].brdIntf && intf[offset].brdIntf->ctrl_r) ?
         intf[offset].brdIntf->ctrl_r(offset) : 0;
}
WRITE_HANDLER(sndbrd_ctrl_cb) {
  if (intf[offset].ctrl_cb) {
    if (intf[offset].brdIntf && (intf[offset].brdIntf->flags & SNDBRD_NOCBSYNC))
      intf[offset].ctrl_cb(offset, data);
    else
      sndbrd_sync_w(intf[offset].ctrl_cb, offset, data);
  }
}
WRITE_HANDLER(sndbrd_data_cb) {
  if (intf[offset].data_cb) {
    if (intf[offset].brdIntf && (intf[offset].brdIntf->flags & SNDBRD_NOCBSYNC))
      intf[offset].data_cb(offset, data);
    else
      sndbrd_sync_w(intf[offset].data_cb, offset, data);
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
#else /* SNDBRD_RECURSIVE */
/* Sound board drivers */
  SNDBRDINTF(s11cs)
  SNDBRDINTF(wpcs)
  SNDBRDINTF(dcs)
  SNDBRDINTF(by32)
  SNDBRDINTF(by51)
  SNDBRDINTF(by56)
  SNDBRDINTF(by61)
  SNDBRDINTF(by45)
  SNDBRDINTF(byTCS)
  SNDBRDINTF(bySD)
  SNDBRDINTF(s67s)
  SNDBRDINTF(s11s)
#endif /* SNDBRD_RECURSIVE */


