#ifndef INC_SNDBRD
#define INC_SNDBRD

extern void sndbrd_sync_w(WRITE_HANDLER((*handler)),int offset, int data);
/*-- core interface --*/
extern void sndbrd_init(int brdNo, int brdType, int cpuNo, UINT8 *romRegion,
                       WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb)));
extern int sndbrd_exists(int board);
extern const char* sndbrd_typestr(int board);
extern void sndbrd_exit(int board);
extern void sndbrd_diag(int board, int button);
extern void sndbrd_data_w(int board, int data);
extern int sndbrd_data_r(int board);
extern void sndbrd_ctrl_w(int board, int data);
extern int sndbrd_ctrl_r(int board);
extern void sndbrd_ctrl_cb(int board, int data);
extern void sndbrd_data_cb(int board, int data);
void sndbrd_manCmd(int board, int cmd);
extern void sndbrd_0_init(int brdType, int cpuNo, UINT8 *romRegion,
                          WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb)));
extern void sndbrd_1_init(int brdType, int cpuNo, UINT8 *romRegion,
                          WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb)));
extern void sndbrd_0_exit(void);
extern void sndbrd_1_exit(void);
extern void sndbrd_0_diag(int button);
extern void sndbrd_1_diag(int button);
extern int sndbrd_type(int offset);
extern int sndbrd_0_type(void);
extern int sndbrd_1_type(void);
extern WRITE_HANDLER(sndbrd_0_data_w);
extern WRITE_HANDLER(sndbrd_1_data_w);
extern WRITE_HANDLER(sndbrd_0_ctrl_w);
extern WRITE_HANDLER(sndbrd_1_ctrl_w);
extern  READ_HANDLER(sndbrd_0_data_r);
extern  READ_HANDLER(sndbrd_1_data_r);
extern  READ_HANDLER(sndbrd_0_ctrl_r);
extern  READ_HANDLER(sndbrd_1_ctrl_r);

/*-- sound board interface --*/
struct sndbrdData {
  int boardNo, subType, cpuNo;
  UINT8 *romRegion;
};
struct sndbrdIntf {
  const char* typestr;
  void (*init)(struct sndbrdData *brdData);
  void (*exit)(int boardNo);
  void (*diag)(int buttons);
  WRITE_HANDLER((*manCmd_w));
  WRITE_HANDLER((*data_w));
   READ_HANDLER((*data_r));
  WRITE_HANDLER((*ctrl_w));
   READ_HANDLER((*ctrl_r));
  UINT32 flags;
};

#define SNDBRD_NODATASYNC 0x0001 // Don't use cpu sync'ed data writes
#define SNDBRD_NOCTRLSYNC 0x0002 // Don't use cpu sync'ed ctrl writes
#define SNDBRD_NOCBSYNC   0x0004 // Don't use cpu sync'ed callbacks
#define SNDBRD_DOUBLECMD  0x0010 // Requires 2 bytes for each manual sound command
#define SNDBRD_NOTSOUND   0x0100 // Board is available even if sound is disabled
#define SNDBRD_TYPE(main,sub) (((main)<<8)|(sub))

#define SNDBRD_NONE    SNDBRD_TYPE( 0,0)
#define SNDBRD_S11CS   SNDBRD_TYPE( 1,0)
#define SNDBRD_WPCS    SNDBRD_TYPE( 2,0)
#define SNDBRD_DCS     SNDBRD_TYPE( 3,0)
#define SNDBRD_DCSB    SNDBRD_TYPE( 3,1)
#define SNDBRD_DCS95   SNDBRD_TYPE( 3,2)
#define SNDBRD_BY32    SNDBRD_TYPE( 4,0)
#define SNDBRD_BY50    SNDBRD_BY32
#define SNDBRD_BY51    SNDBRD_TYPE( 5,0)
#define SNDBRD_BY56    SNDBRD_TYPE( 5,1)
#define SNDBRD_BY61    SNDBRD_TYPE( 7,0)
#define SNDBRD_BY61B   SNDBRD_TYPE( 7,1)
#define SNDBRD_BY81    SNDBRD_TYPE( 7,2)
#define SNDBRD_BY45    SNDBRD_TYPE( 8,0)
#define SNDBRD_BY45BP  SNDBRD_TYPE( 8,1)
#define SNDBRD_BYTCS   SNDBRD_TYPE( 9,0)
#define SNDBRD_BYSD    SNDBRD_TYPE(10,0)
#define SNDBRD_S67S    SNDBRD_TYPE(11,0)
#define SNDBRD_S7S_ND  SNDBRD_TYPE(11,1)
#define SNDBRD_S9S     SNDBRD_TYPE(12,0)
#define SNDBRD_S9PF    SNDBRD_TYPE(12,8)
#define SNDBRD_S11S    SNDBRD_TYPE(12,4)
#define SNDBRD_S11XS   SNDBRD_TYPE(12,1)
#define SNDBRD_S11BS   SNDBRD_TYPE(12,2)
#define SNDBRD_S11JS   SNDBRD_TYPE( 6,0)
#define SNDBRD_DE2S    SNDBRD_TYPE(13,0)
#define SNDBRD_DE1S    SNDBRD_TYPE(14,0)
#define SNDBRD_DEDMD16 SNDBRD_TYPE(15,0)
#define SNDBRD_DEDMD32 SNDBRD_TYPE(16,0)
#define SNDBRD_DEDMD64 SNDBRD_TYPE(17,0)
#define SNDBRD_GTS80S  SNDBRD_TYPE(18,0)
#define SNDBRD_GTS80SP SNDBRD_TYPE(18,1)
#define SNDBRD_GTS80SS SNDBRD_TYPE(19,0)
#define SNDBRD_GTS80SS_VOTRAX SNDBRD_TYPE(19,1)
#define SNDBRD_GTS80B  SNDBRD_TYPE(20,0)
#define SNDBRD_GTS3    SNDBRD_TYPE(20,1)
#define SNDBRD_HANKIN  SNDBRD_TYPE(21,0)
#define SNDBRD_ATARI1  SNDBRD_TYPE(22,0)
#define SNDBRD_ATARI2  SNDBRD_TYPE(23,0)
#define SNDBRD_TAITO_SINTETIZADOR   SNDBRD_TYPE(24,0)
#define SNDBRD_TAITO_SINTEVOX       SNDBRD_TYPE(24,1)
#define SNDBRD_TAITO_SINTETIZADORPP SNDBRD_TYPE(24,2)
#define SNDBRD_TAITO_SINTEVOXPP     SNDBRD_TYPE(24,3)
#define SNDBRD_ZAC1311 SNDBRD_TYPE(25,0)
#define SNDBRD_ZAC1125 SNDBRD_TYPE(26,0)
#define SNDBRD_ZAC1346 SNDBRD_TYPE(27,0)
#define SNDBRD_ZAC1370 SNDBRD_TYPE(28,0)
#define SNDBRD_ZAC13136 SNDBRD_TYPE(28,1)
#define SNDBRD_ZAC11178 SNDBRD_TYPE(28,2)
#define SNDBRD_ZAC11178_13181 SNDBRD_TYPE(28,3)
#define SNDBRD_ZAC13181x3 SNDBRD_TYPE(28,4)
#define SNDBRD_TECHNO  SNDBRD_TYPE(29,0)
#define SNDBRD_ST100   SNDBRD_TYPE(30,0)
#define SNDBRD_ST100B  SNDBRD_TYPE(30,1)
#define SNDBRD_ST300   SNDBRD_TYPE(31,0)
#define SNDBRD_ST300V  SNDBRD_TYPE(31,1)
#define SNDBRD_ASTRO   SNDBRD_TYPE(32,0)
#define SNDBRD_GPSSU1  SNDBRD_TYPE(33,0)
#define SNDBRD_GPSSU2  SNDBRD_TYPE(34,0)
#define SNDBRD_GPSSU3  SNDBRD_TYPE(34,1)
#define SNDBRD_GPSSU4  SNDBRD_TYPE(35,0)
#define SNDBRD_GPMSU1  SNDBRD_TYPE(36,0)
#define SNDBRD_GPMSU3  SNDBRD_TYPE(37,0)
#define SNDBRD_ALVGS1  SNDBRD_TYPE(38,0)
#define SNDBRD_ALVGS2  SNDBRD_TYPE(39,0)
#define SNDBRD_ALVGDMD SNDBRD_TYPE(40,0)
#define SNDBRD_CAPCOMS SNDBRD_TYPE(41,0)
#define SNDBRD_SPINB   SNDBRD_TYPE(42,0)
#define SNDBRD_MRGAME  SNDBRD_TYPE(43,0)
#define SNDBRD_DE3S    SNDBRD_TYPE(44,0)
#define SNDBRD_ROWAMET SNDBRD_TYPE(45,0)
#define SNDBRD_NUOVA   SNDBRD_TYPE(46,0)
#define SNDBRD_GRAND   SNDBRD_TYPE(47,0)
#define SNDBRD_JVH     SNDBRD_TYPE(48,0)
#endif /* INC_SNDBRD */
