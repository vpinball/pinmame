#ifndef INC_SNDBRD
#define INC_SNDBRD

extern void sndbrd_sync_w(WRITE_HANDLER((*handler)),int offset, int data);
/*-- core interface --*/
extern void sndbrd_init(int brdNo, int brdType, int cpuNo, UINT8 *romRegion,
                       WRITE_HANDLER((*data_cb)),WRITE_HANDLER((*ctrl_cb)));
extern void sndbrd_exit(int board);
extern void sndbrd_diag(int board, int button);
extern WRITE_HANDLER(sndbrd_data_w);
extern READ_HANDLER(sndbrd_data_r);
extern WRITE_HANDLER(sndbrd_ctrl_w);
extern READ_HANDLER(sndbrd_ctrl_r);
extern WRITE_HANDLER(sndbrd_ctrl_cb);
extern WRITE_HANDLER(sndbrd_data_cb);
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

#define SNDBRD_NODATASYNC 0x01
#define SNDBRD_NOCTRLSYNC 0x02
#define SNDBRD_NOCBSYNC   0x04

extern WRITE_HANDLER(sndbrd_ctrl_cb);
extern WRITE_HANDLER(sndbrd_data_cb);

#define SNDBRD_TYPE(main,sub) (((main)<<8)|(sub))

#define SNDBRD_NONE  SNDBRD_TYPE( 0,0)
#define SNDBRD_S11CS SNDBRD_TYPE( 1,0)
#define SNDBRD_WPCS  SNDBRD_TYPE( 2,0)
#define SNDBRD_DCS   SNDBRD_TYPE( 3,0)
#define SNDBRD_DCS95 SNDBRD_TYPE( 3,1)
#define SNDBRD_BY32  SNDBRD_TYPE( 4,0)
#define SNDBRD_BY50  SNDBRD_BY32
#define SNDBRD_BY51  SNDBRD_TYPE( 5,0)
#define SNDBRD_BY56  SNDBRD_TYPE( 6,1)
#define SNDBRD_BY61  SNDBRD_TYPE( 7,0)
#define SNDBRD_BY61B SNDBRD_TYPE( 7,1)
#define SNDBRD_BY81  SNDBRD_TYPE( 7,2)
#define SNDBRD_BY45  SNDBRD_TYPE( 8,0)
#define SNDBRD_BYTCS SNDBRD_TYPE( 9,0)
#define SNDBRD_BYSD  SNDBRD_TYPE(10,0)
#define SNDBRD_S67S  SNDBRD_TYPE(11,0)
#define SNDBRD_S9S   SNDBRD_TYPE(12,0)
#define SNDBRD_S11S  SNDBRD_TYPE(12,1)
#define SNDBRD_COUNT 13

#endif /* INC_SNDBRD */
