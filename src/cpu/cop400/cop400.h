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
  COP400_SKL
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

#endif  /* _COP400_H */
