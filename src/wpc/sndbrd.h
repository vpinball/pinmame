#ifndef INC_SNDBRD
#define INC_SNDBRD

extern void sndbrd_sync_w(WRITE_HANDLER((*handler)),int offset, int data);
/*-- core interface --*/
extern void sndbrd_init(int brdNo, int brdType, int cpuNo, UINT8 *romRegion,
                       WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb)));
extern void sndbrd_exit(int board);
extern void sndbrd_diag(int board, int button);
extern void sndbrd_data_w(int board, int data);
extern int sndbrd_data_r(int board);
extern void sndbrd_ctrl_w(int board, int data);
extern int sndbrd_ctrl_r(int board);
extern void sndbrd_ctrl_cb(int board, int data);
extern void sndbrd_data_cb(int board, int data);
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
  void (*init)(struct sndbrdData *brdData);
  void (*exit)(int boardNo);
  void (*diag)(int buttons);
  WRITE_HANDLER((*data_w));
   READ_HANDLER((*data_r));
  WRITE_HANDLER((*ctrl_w));
   READ_HANDLER((*ctrl_r));
  UINT32 flags;
};

#define SNDBRD_NODATASYNC 0x0001
#define SNDBRD_NOCTRLSYNC 0x0002
#define SNDBRD_NOCBSYNC   0x0004
#define SNDBRD_NOTSOUND   0x0100

#define SNDBRD_TYPE(main,sub) (((main)<<8)|(sub))

#define SNDBRD_NONE    SNDBRD_TYPE( 0,0)
#define SNDBRD_S11CS   SNDBRD_TYPE( 1,0)
#define SNDBRD_WPCS    SNDBRD_TYPE( 2,0)
#define SNDBRD_DCS     SNDBRD_TYPE( 3,0)
#define SNDBRD_DCS95   SNDBRD_TYPE( 3,1)
#define SNDBRD_BY32    SNDBRD_TYPE( 4,0)
#define SNDBRD_BY50    SNDBRD_BY32
#define SNDBRD_BY51    SNDBRD_TYPE( 5,0)
#define SNDBRD_BY56    SNDBRD_TYPE( 5,1)
#define SNDBRD_BY61    SNDBRD_TYPE( 7,0)
#define SNDBRD_BY61B   SNDBRD_TYPE( 7,1)
#define SNDBRD_BY81    SNDBRD_TYPE( 7,2)
#define SNDBRD_BY45    SNDBRD_TYPE( 8,0)
#define SNDBRD_BYTCS   SNDBRD_TYPE( 9,0)
#define SNDBRD_BYSD    SNDBRD_TYPE(10,0)
#define SNDBRD_S67S    SNDBRD_TYPE(11,0)
#define SNDBRD_S9S     SNDBRD_TYPE(12,0)
#define SNDBRD_S11S    SNDBRD_TYPE(12,1)
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
#define SNDBRD_GTS80B  SNDBRD_TYPE(20,0)
#define SNDBRD_GTS3    SNDBRD_TYPE(20,1)
#define SNDBRD_HANKIN  SNDBRD_TYPE(21,0)
#define SNDBRD_ATARI1  SNDBRD_TYPE(22,0)
#define SNDBRD_ATARI2  SNDBRD_TYPE(23,0)
#define SNDBRD_TAITO_SINTEVOX     SNDBRD_TYPE(24,0)
#define SNDBRD_TAITO_SINTETIZADOR SNDBRD_TYPE(24,1)
#define SNDBRD_TAITO_SINTEVOXPP   SNDBRD_TYPE(24,2)
#define SNDBRD_ZAC     SNDBRD_TYPE(25,0)

#endif /* INC_SNDBRD */
