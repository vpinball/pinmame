#ifndef INC_PINDDRV
#define INC_PINDDRV

extern void pinddrvInit(void);
extern void pinddrvDeInit(void);
extern void pinddrvSendFrame(void);

UINT8			enabled;		// pindmd enabled? (ie device found)
UINT8			doOther;		// sam mode?
UINT8			do16;			// 16 shades?

#endif
