/**************************************************************************
 *               National Semiconductor COP400 Emulator                   *
 *                                                                        *
 *                   Copyright (C) 2006 MAME Team                         *
 **************************************************************************/

#ifndef _COP400_H
#define _COP400_H

#ifndef INLINE
#define INLINE static inline
#endif

#define COP400_CLOCK_DIVIDER	4

#define COP400_PORT_L	0x100
#define COP400_PORT_G	0x101
#define COP400_PORT_D	0x102
#define	COP400_PORT_IN	0x103
#define	COP400_PORT_SK	0x104
#define	COP400_PORT_SIO	0x105

enum {
  COP400_PC=1,
  COP400_A,
  COP400_B,
  COP400_C,
  COP400_G,
  COP400_EN,
  COP400_Q,
  COP400_SA,
  COP400_SB,
  COP400_SC,
  COP400_SIO,
  COP400_SKL,
  COP400_T
};

extern void cop420_init(void);
extern void cop420_reset(void *param);
extern void cop420_exit(void);
extern int cop420_execute(int cycles);
extern unsigned cop420_get_context(void *dst);
extern void cop420_set_context(void *src);
extern unsigned cop420_get_reg(int regnum);
extern void cop420_set_reg(int regnum, unsigned val);
extern const char *cop420_info(void *context, int regnum);
extern void cop420_set_irq_line(int irqline, int state);
extern void cop420_set_irq_callback(int (*callback)(int irqline));
extern WRITE_HANDLER(cop420_internal_w);
extern READ_HANDLER(cop420_internal_r);
extern unsigned cop420_dasm(char *buffer, unsigned pc);

extern int cop420_ICount;
#define cop420_icount cop420_ICount

#ifdef MAME_DEBUG
int 	DasmCOP420(char *dst, unsigned pc);
#endif

#define UINT1	UINT8
#define UINT4	UINT8
#define UINT6	UINT8
#define UINT10	UINT16

typedef struct
{
	UINT10 	R_PC;
	UINT10	R_PREVPC;
	UINT4	R_A;
	UINT6	R_B;
	UINT1	R_C;
	UINT4	R_EN;
	UINT4	R_G;
	UINT8	R_Q;
	UINT10	R_SA, R_SB, R_SC;
	UINT4	R_SIO;
	UINT1	R_SKL;
	UINT8   R_skip, R_skipLBI;
	UINT1	timerlatch;
	UINT10	counter;
	UINT4	R_RAM[64];
	UINT8	G_mask;
	UINT8	D_mask;
	UINT4	IL;
	UINT4 in[4];
	int		last_skip;
} COP420_Regs;

#endif  /* _COP400_H */
