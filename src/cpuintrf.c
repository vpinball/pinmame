/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulators with the other parts of
  the emulation.

***************************************************************************/

#include <signal.h>
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"
#include "hiscore.h"

#if (HAS_Z80)
#include "cpu/z80/z80.h"
#endif
#if (HAS_8080 || HAS_8085A)
#include "cpu/i8085/i8085.h"
#endif
#if (HAS_M6502 || HAS_M65C02 || HAS_M65SC02 || HAS_M6510 || HAS_M6510T || HAS_M7501 || HAS_M8502 || HAS_N2A03)
#include "cpu/m6502/m6502.h"
#endif
#if (HAS_M4510)
#include "cpu/m6502/m4510.h"
#endif
#if (HAS_M65CE02)
#include "cpu/m6502/m65ce02.h"
#endif
#if (HAS_M6509)
#include "cpu/m6502/m6509.h"
#endif
#if (HAS_H6280)
#include "cpu/h6280/h6280.h"
#endif
#if (HAS_I86)
#include "cpu/i86/i86intf.h"
#endif
#if (HAS_I88)
#include "cpu/i86/i88intf.h"
#endif
#if (HAS_I186)
#include "cpu/i86/i186intf.h"
#endif
#if (HAS_I188)
#include "cpu/i86/i188intf.h"
#endif
#if (HAS_I286)
#include "cpu/i86/i286intf.h"
#endif
#if (HAS_V20 || HAS_V30 || HAS_V33)
#include "cpu/nec/necintrf.h"
#endif
#if (HAS_I8035 || HAS_I8039 || HAS_I8048 || HAS_N7751)
#include "cpu/i8039/i8039.h"
#endif
#if (HAS_I8X41)
#include "cpu/i8x41/i8x41.h"
#endif
#if (HAS_M6800 || HAS_M6801 || HAS_M6802 || HAS_M6803 || HAS_M6808 || HAS_HD63701)
#include "cpu/m6800/m6800.h"
#endif
#if (HAS_M6805 || HAS_M68705 || HAS_HD63705)
#include "cpu/m6805/m6805.h"
#endif
#if (HAS_M6809)
#include "cpu/m6809/m6809.h"
#endif
#if (HAS_HD6309)
#include "cpu/hd6309/hd6309.h"
#endif
#if (HAS_KONAMI)
#include "cpu/konami/konami.h"
#endif
#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
#include "cpu/m68000/m68000.h"
#endif
#if (HAS_T11)
#include "cpu/t11/t11.h"
#endif
#if (HAS_S2650)
#include "cpu/s2650/s2650.h"
#endif
#if (HAS_TMS34010 || HAS_TMS34020)
#include "cpu/tms34010/tms34010.h"
#endif
#if (HAS_TMS9900 || HAS_TMS9940 || HAS_TMS9980 || HAS_TMS9985 || HAS_TMS9989 || HAS_TMS9995 || HAS_TMS99105A || HAS_TMS99110A)
#include "cpu/tms9900/tms9900.h"
#endif
#if (HAS_Z8000)
#include "cpu/z8000/z8000.h"
#endif
#if (HAS_TMS320C10)
#include "cpu/tms32010/tms32010.h"
#endif
#if (HAS_CCPU)
#include "cpu/ccpu/ccpu.h"
#endif
#if (HAS_ADSP2100 || HAS_ADSP2105)
#include "cpu/adsp2100/adsp2100.h"
#endif
#if (HAS_PSXCPU)
#include "cpu/mips/mips.h"
#endif
#if (HAS_ASAP)
#include "cpu/asap/asap.h"
#endif
#if (HAS_UPD7810)
#include "cpu/upd7810/upd7810.h"
#endif


#ifdef MESS

#if (HAS_APEXC)
#include "mess/cpu/apexc/apexc.h"
#endif
#if (HAS_ARM)
#include "mess/cpu/arm/arm.h"
#endif
#if (HAS_CDP1802)
#include "mess/cpu/cdp1802/cdp1802.h"
#endif
#if (HAS_CP1600)
#include "mess/cpu/cp1600/cp1600.h"
#endif
#if (HAS_F8)
#include "mess/cpu/f8/f8.h"
#endif
#if (HAS_G65816)
#include "mess/cpu/g65816/g65816.h"
#endif
#if (HAS_LH5801)
#include "mess/cpu/lh5801/lh5801.h"
#endif
#if (HAS_PDP1)
#include "mess/cpu/pdp1/pdp1.h"
#endif
#if (HAS_SATURN)
#include "mess/cpu/saturn/saturn.h"
#endif
#if (HAS_SC61860)
#include "mess/cpu/sc61860/sc61860.h"
#endif
#if (HAS_SH2)
#include "mess/cpu/sh2/sh2.h"
#endif
#if (HAS_SPC700)
#include "mess/cpu/spc700/spc700.h"
#endif
#if (HAS_Z80GB)
#include "mess/cpu/z80gb/z80gb.h"
#endif
#if (HAS_Z80_MSX)
#include "cpu/z80/z80_msx.h"
#endif

#endif


/* these are triggers sent to the timer system for various interrupt events */
#define TRIGGER_TIMESLICE		-1000
#define TRIGGER_INT 			-2000
#define TRIGGER_YIELDTIME		-3000
#define TRIGGER_SUSPENDTIME 	-4000

#define VERBOSE 0

#define SAVE_STATE_TEST 0
#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define CPUINFO_SIZE	(5*sizeof(int)+4*sizeof(void*)+2*sizeof(double))
/* How do I calculate the next power of two from CPUINFO_SIZE using a macro? */
#ifdef __LP64__
#define CPUINFO_ALIGN	(128-CPUINFO_SIZE)
#else
#define CPUINFO_ALIGN	(64-CPUINFO_SIZE)
#endif

struct cpuinfo
{
	struct cpu_interface *intf; 	/* pointer to the interface functions */
	int iloops; 					/* number of interrupts remaining this frame */
	int totalcycles;				/* total CPU cycles executed */
	int vblankint_countdown;		/* number of vblank callbacks left until we interrupt */
	int vblankint_multiplier;		/* number of vblank callbacks per interrupt */
	void *vblankint_timer;			/* reference to elapsed time counter */
	double vblankint_period;		/* timing period of the VBLANK interrupt */
	void *timedint_timer;			/* reference to this CPU's timer */
	double timedint_period; 		/* timing period of the timed interrupt */
	void *context;					/* dynamically allocated context buffer */
	int save_context;				/* need to context switch this CPU? yes or no */
	UINT8 filler[CPUINFO_ALIGN];	/* make the array aligned to next power of 2 */
};

static struct cpuinfo cpu[MAX_CPU];

static int activecpu,totalcpu;
static int cycles_running;	/* number of cycles that the CPU emulation was requested to run */
					/* (needed by cpu_getfcount) */
static int have_to_reset;

static UINT8 interrupt_enable[MAX_CPU];
static INT32 interrupt_vector[MAX_CPU];

static UINT8 irq_line_state[MAX_CPU * MAX_IRQ_LINES];
static INT32 irq_line_vector[MAX_CPU * MAX_IRQ_LINES];

static INT32 watchdog_counter;

static void *vblank_timer;
static int vblank_countdown;
static int vblank_multiplier;
static double vblank_period;

static void *refresh_timer;
static double refresh_period;
static double refresh_period_inv;

static void *timeslice_timer;
static double timeslice_period;

static double scanline_period;
static double scanline_period_inv;

static int usres; /* removed from cpu_run and made global */
static int vblank;
static int current_frame;

static int loadsave_schedule;
static char loadsave_schedule_id;

static void cpu_generate_interrupt(int cpunum, int (*func)(void), int num);
static void cpu_vblankintcallback(int param);
static void cpu_timedintcallback(int param);
static void cpu_internal_interrupt(int cpunum, int type);
static void cpu_manualnmicallback(int param);
static void cpu_manualirqcallback(int param);
static void cpu_internalintcallback(int param);
static void cpu_manualintcallback(int param);
static void cpu_clearintcallback(int param);
static void cpu_resetcallback(int param);
static void cpu_haltcallback(int param);
static void cpu_timeslicecallback(int param);
static void cpu_vblankreset(void);
static void cpu_vblankcallback(int param);
static void cpu_updatecallback(int param);
static double cpu_computerate(int value);
static void cpu_inittimers(void);


/* default irq callback handlers */
static int cpu_0_irq_callback(int irqline);
static int cpu_1_irq_callback(int irqline);
static int cpu_2_irq_callback(int irqline);
static int cpu_3_irq_callback(int irqline);
static int cpu_4_irq_callback(int irqline);
static int cpu_5_irq_callback(int irqline);
static int cpu_6_irq_callback(int irqline);
static int cpu_7_irq_callback(int irqline);

/* and a list of them for indexed access */
static int (*cpu_irq_callbacks[MAX_CPU])(int) = {
	cpu_0_irq_callback, cpu_1_irq_callback, cpu_2_irq_callback, cpu_3_irq_callback,
	cpu_4_irq_callback, cpu_5_irq_callback, cpu_6_irq_callback, cpu_7_irq_callback
};

/* and a list of driver interception hooks */
static int (*drv_irq_callbacks[MAX_CPU])(int) = { NULL, };

/* Default window layout for the debugger */
UINT8 default_win_layout[] = {
	 0, 0,80, 5,	/* register window (top rows) */
	 0, 5,24,17,	/* disassembler window (left, middle columns) */
	25, 5,55, 8,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1 	/* command line window (bottom row) */
};

/* Dummy interfaces for non-CPUs */
static void Dummy_init(void);
static void Dummy_reset(void *param);
static void Dummy_exit(void);
static int Dummy_execute(int cycles);
static void Dummy_burn(int cycles);
static unsigned Dummy_get_context(void *regs);
static void Dummy_set_context(void *regs);
static unsigned Dummy_get_pc(void);
static void Dummy_set_pc(unsigned val);
static unsigned Dummy_get_sp(void);
static void Dummy_set_sp(unsigned val);
static unsigned Dummy_get_reg(int regnum);
static void Dummy_set_reg(int regnum, unsigned val);
static void Dummy_set_nmi_line(int state);
static void Dummy_set_irq_line(int irqline, int state);
static void Dummy_set_irq_callback(int (*callback)(int irqline));
static int Dummy_ICount;
static const char *Dummy_info(void *context, int regnum);
static unsigned Dummy_dasm(char *buffer, unsigned pc);

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define INIT(index) 					((*cpu[index].intf->init)())
#define RESET(index)					((*cpu[index].intf->reset)(Machine->drv->cpu[index].reset_param))
#define EXECUTE(index,cycles)			((*cpu[index].intf->execute)(cycles))
#define GETCONTEXT(index,context)		((*cpu[index].intf->get_context)(context))
#define SETCONTEXT(index,context)		((*cpu[index].intf->set_context)(context))
#define GETCYCLETBL(index,which)		((*cpu[index].intf->get_cycle_table)(which))
#define SETCYCLETBL(index,which,cnts)	((*cpu[index].intf->set_cycle_table)(which,cnts))
#define GETPC(index)					((*cpu[index].intf->get_pc)())
#define SETPC(index,val)				((*cpu[index].intf->set_pc)(val))
#define GETSP(index)					((*cpu[index].intf->get_sp)())
#define SETSP(index,val)				((*cpu[index].intf->set_sp)(val))
#define GETREG(index,regnum)			((*cpu[index].intf->get_reg)(regnum))
#define SETREG(index,regnum,value)		((*cpu[index].intf->set_reg)(regnum,value))
#define SETNMILINE(index,state) 		((*cpu[index].intf->set_nmi_line)(state))
#define SETIRQLINE(index,line,state)	((*cpu[index].intf->set_irq_line)(line,state))
#define SETIRQCALLBACK(index,callback)	((*cpu[index].intf->set_irq_callback)(callback))
#define INTERNAL_INTERRUPT(index,type)	if( cpu[index].intf->internal_interrupt ) ((*cpu[index].intf->internal_interrupt)(type))
#define CPUINFO(index,context,regnum)	((*cpu[index].intf->cpu_info)(context,regnum))
#define CPUDASM(index,buffer,pc)		((*cpu[index].intf->cpu_dasm)(buffer,pc))
#define ICOUNT(index)					(*cpu[index].intf->icount)
#define INT_TYPE_NONE(index)			(cpu[index].intf->no_int)
#define INT_TYPE_IRQ(index) 			(cpu[index].intf->irq_int)
#define INT_TYPE_NMI(index) 			(cpu[index].intf->nmi_int)
#define READMEM(index,offset)			((*cpu[index].intf->memory_read)(offset))
#define WRITEMEM(index,offset,data) 	((*cpu[index].intf->memory_write)(offset,data))
#define SET_OP_BASE(index,pc)			((*cpu[index].intf->set_op_base)(pc))

#define CPU_TYPE(index) 				(Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK)
#define CPU_AUDIO(index)				(Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU)

#define IFC_INFO(cpu,context,regnum)	((cpuintf[cpu].cpu_info)(context,regnum))

/* most CPUs use this macro */
#define CPU0(cpu,name,nirq,dirq,oc,i0,i1,i2,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		NULL, name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_ICount, oc, i0, i1, i2, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* CPUs which have the _burn function */
#define CPU1(cpu,name,nirq,dirq,oc,i0,i1,i2,datawidth,mem,shift,bits,endian,align,maxinst)	 \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, 				   \
		name##_burn,															   \
		name##_get_context, name##_set_context, 								   \
		name##_get_cycle_table, name##_set_cycle_table, 						   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		NULL, name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_ICount, oc, i0, i1, i2, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* CPUs which have the _internal_interrupt function */
#define CPU2(cpu,name,nirq,dirq,oc,i0,i1,i2,datawidth,mem,shift,bits,endian,align,maxinst)	 \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, 				   \
		NULL,																	   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		name##_internal_interrupt, name##_info, name##_dasm,					   \
		nirq, dirq, &name##_ICount, oc, i0, i1, i2, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}																			   \

/* like CPU0, but CPU has Harvard-architecture like program/data memory */
#define CPU3(cpu,name,nirq,dirq,oc,i0,i1,i2,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		NULL, name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_icount, oc, i0, i1, i2, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, NULL, NULL,						   \
		cpu##_PGM_OFFSET, cpu_setopbase##mem,									   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}

/* like CPU0, but CPU has internal memory (or I/O ports, timers or similiar) */
#define CPU4(cpu,name,nirq,dirq,oc,i0,i1,i2,datawidth,mem,shift,bits,endian,align,maxinst) \
	{																			   \
		CPU_##cpu,																   \
		name##_init, name##_reset, name##_exit, name##_execute, NULL,			   \
		name##_get_context, name##_set_context, NULL, NULL, 					   \
		name##_get_pc, name##_set_pc,											   \
		name##_get_sp, name##_set_sp, name##_get_reg, name##_set_reg,			   \
		name##_set_nmi_line, name##_set_irq_line, name##_set_irq_callback,		   \
		NULL, name##_info, name##_dasm, 										   \
		nirq, dirq, &name##_icount, oc, i0, i1, i2, 							   \
		datawidth,																   \
		(mem_read_handler)cpu_readmem##mem, (mem_write_handler)cpu_writemem##mem, name##_internal_r, name##_internal_w, \
		0, cpu_setopbase##mem,													   \
		shift, bits, CPU_IS_##endian, align, maxinst							   \
	}



/* warning the ordering must match the one of the enum in driver.h! */
struct cpu_interface cpuintf[] =
{
	CPU0(DUMMY,    Dummy,	 1,  0,1.00,0,				   -1,			   -1,			   8, 16,	  0,16,LE,1, 1	),
#if (HAS_Z80)
	CPU1(Z80,	   z80, 	 1,255,1.00,Z80_IGNORE_INT,    Z80_IRQ_INT,    Z80_NMI_INT,    8, 16,	  0,16,LE,1, 4	),
#endif
#if (HAS_8080)
	CPU0(8080,	   i8080,	 4,255,1.00,I8080_NONE, 	   I8080_INTR,	   I8080_TRAP,	   8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_8085A)
	CPU0(8085A,    i8085,	 4,255,1.00,I8085_NONE, 	   I8085_INTR,	   I8085_TRAP,	   8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6502)
	CPU0(M6502,    m6502,	 1,  0,1.00,M6502_INT_NONE,    M6502_INT_IRQ,  M6502_INT_NMI,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65C02)
	CPU0(M65C02,   m65c02,	 1,  0,1.00,M65C02_INT_NONE,   M65C02_INT_IRQ, M65C02_INT_NMI, 8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65SC02)
	CPU0(M65SC02,  m65sc02,  1,  0,1.00,M65SC02_INT_NONE,  M65SC02_INT_IRQ,M65SC02_INT_NMI,8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M65CE02)
	CPU0(M65CE02,  m65ce02,  1,  0,1.00,M65CE02_INT_NONE,  M65CE02_INT_IRQ,M65CE02_INT_NMI,8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6509)
	CPU0(M6509,    m6509,	 1,  0,1.00,M6509_INT_NONE,    M6509_INT_IRQ,  M6509_INT_NMI,  8, 20,	  0,20,LE,1, 3	),
#endif
#if (HAS_M6510)
	CPU0(M6510,    m6510,	 1,  0,1.00,M6510_INT_NONE,    M6510_INT_IRQ,  M6510_INT_NMI,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M6510T)
	CPU0(M6510T,   m6510t,	 1,  0,1.00,M6510T_INT_NONE,   M6510T_INT_IRQ, M6510T_INT_NMI, 8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M7501)
	CPU0(M7501,    m7501,	 1,  0,1.00,M7501_INT_NONE,    M7501_INT_IRQ,  M7501_INT_NMI,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M8502)
	CPU0(M8502,    m8502,	 1,  0,1.00,M8502_INT_NONE,    M8502_INT_IRQ,  M8502_INT_NMI,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_N2A03)
	CPU0(N2A03,    n2a03,	 1,  0,1.00,N2A03_INT_NONE,    N2A03_INT_IRQ,  N2A03_INT_NMI,  8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_M4510)
	CPU0(M4510,    m4510,	 1,  0,1.00,M4510_INT_NONE,    M4510_INT_IRQ,  M4510_INT_NMI,  8, 20,	  0,20,LE,1, 3	),
#endif
#if (HAS_H6280)
	CPU0(H6280,    h6280,	 3,  0,1.00,H6280_INT_NONE,    -1,			   H6280_INT_NMI,  8, 21,	  0,21,LE,1, 3	),
#endif
#if (HAS_I86)
	CPU0(I86,	   i86, 	 1,  0,1.00,I86_INT_NONE,	   -1000,		   I86_NMI_INT,    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I88)
	CPU0(I88,	   i88, 	 1,  0,1.00,I88_INT_NONE,	   -1000,		   I88_NMI_INT,    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I186)
	CPU0(I186,	   i186,	 1,  0,1.00,I186_INT_NONE,	   -1000,		   I186_NMI_INT,   8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I188)
	CPU0(I188,	   i188,	 1,  0,1.00,I188_INT_NONE,	   -1000,		   I188_NMI_INT,   8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I286)
	CPU0(I286,	   i286,	 1,  0,1.00,I286_INT_NONE,	   -1000,		   I286_NMI_INT,   8, 24,	  0,24,LE,1, 5	),
#endif
#if (HAS_V20)
	CPU0(V20,	   v20, 	 1,  0,1.00,NEC_INT_NONE,	   -1000,		   NEC_NMI_INT,    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_V30)
	CPU0(V30,	   v30, 	 1,  0,1.00,NEC_INT_NONE,	   -1000,		   NEC_NMI_INT,    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_V33)
	CPU0(V33,	   v33, 	 1,  0,1.00,NEC_INT_NONE,	   -1000,		   NEC_NMI_INT,    8, 20,	  0,20,LE,1, 5	),
#endif
#if (HAS_I8035)
	CPU0(I8035,    i8035,	 1,  0,1.00,I8035_IGNORE_INT,  I8035_EXT_INT,  -1,			   8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8039)
	CPU0(I8039,    i8039,	 1,  0,1.00,I8039_IGNORE_INT,  I8039_EXT_INT,  -1,			   8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8048)
	CPU0(I8048,    i8048,	 1,  0,1.00,I8048_IGNORE_INT,  I8048_EXT_INT,  -1,			   8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_N7751)
	CPU0(N7751,    n7751,	 1,  0,1.00,N7751_IGNORE_INT,  N7751_EXT_INT,  -1,			   8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_I8X41)
	CPU0(I8X41,    i8x41,	 1,  0,1.00,I8X41_INT_NONE,    I8X41_INT_IBF,  -1,			   8, 16,	  0,16,LE,1, 2	),
#endif
#if (HAS_M6800)
	CPU0(M6800,    m6800,	 1,  0,1.00,M6800_INT_NONE,    M6800_INT_IRQ,  M6800_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6801)
	CPU0(M6801,    m6801,	 1,  0,1.00,M6801_INT_NONE,    M6801_INT_IRQ,  M6801_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6802)
	CPU0(M6802,    m6802,	 1,  0,1.00,M6802_INT_NONE,    M6802_INT_IRQ,  M6802_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6803)
	CPU0(M6803,    m6803,	 1,  0,1.00,M6803_INT_NONE,    M6803_INT_IRQ,  M6803_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6808)
	CPU0(M6808,    m6808,	 1,  0,1.00,M6808_INT_NONE,    M6808_INT_IRQ,  M6808_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_HD63701)
	CPU0(HD63701,  hd63701,  1,  0,1.00,HD63701_INT_NONE,  HD63701_INT_IRQ,HD63701_INT_NMI,8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_NSC8105)
	CPU0(NSC8105,  nsc8105,  1,  0,1.00,NSC8105_INT_NONE,  NSC8105_INT_IRQ,NSC8105_INT_NMI,8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6805)
	CPU0(M6805,    m6805,	 1,  0,1.00,M6805_INT_NONE,    M6805_INT_IRQ,  -1,			   8, 16,	  0,11,BE,1, 3	),
#endif
#if (HAS_M68705)
	CPU0(M68705,   m68705,	 1,  0,1.00,M68705_INT_NONE,   M68705_INT_IRQ, -1,			   8, 16,	  0,11,BE,1, 3	),
#endif
#if (HAS_HD63705)
	CPU0(HD63705,  hd63705,  8,  0,1.00,HD63705_INT_NONE,  HD63705_INT_IRQ,-1,			   8, 16,	  0,16,BE,1, 3	),
#endif
#if (HAS_HD6309)
	CPU0(HD6309,   hd6309,	 2,  0,1.00,HD6309_INT_NONE,   HD6309_INT_IRQ, HD6309_INT_NMI, 8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M6809)
	CPU0(M6809,    m6809,	 2,  0,1.00,M6809_INT_NONE,    M6809_INT_IRQ,  M6809_INT_NMI,  8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_KONAMI)
	CPU0(KONAMI,   konami,	 2,  0,1.00,KONAMI_INT_NONE,   KONAMI_INT_IRQ, KONAMI_INT_NMI, 8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_M68000)
	CPU0(M68000,   m68000,	 8, -1,1.00,MC68000_INT_NONE,  -1,			   -1,			   16,24bew,  0,24,BE,2,10	),
#endif
#if (HAS_M68010)
	CPU0(M68010,   m68010,	 8, -1,1.00,MC68010_INT_NONE,  -1,			   -1,			   16,24bew,  0,24,BE,2,10	),
#endif
#if (HAS_M68EC020)
	CPU0(M68EC020, m68ec020, 8, -1,1.00,MC68EC020_INT_NONE,-1,			   -1,			   32,24bedw, 0,24,BE,4,10	),
#endif
#if (HAS_M68020)
	CPU0(M68020,   m68020,	 8, -1,1.00,MC68020_INT_NONE,	-1, 		   -1,			   32,32bedw, 0,32,BE,4,10	),
#endif
#if (HAS_T11)
	CPU0(T11,	   t11, 	 4,  0,1.00,T11_INT_NONE,	   -1,			   -1,			   16,16lew,  0,16,LE,2, 6	),
#endif
#if (HAS_S2650)
	CPU0(S2650,    s2650,	 2,  0,1.00,S2650_INT_NONE,    -1,			   -1,			   8, 16,	  0,15,LE,1, 3	),
#endif
#if (HAS_TMS34010)
	CPU2(TMS34010, tms34010, 2,  0,1.00,TMS34010_INT_NONE, TMS34010_INT1,  -1,			   16,29lew,  3,29,LE,2,10	),
#endif
#if (HAS_TMS34020)
	CPU2(TMS34020, tms34020, 2,  0,1.00,TMS34020_INT_NONE, TMS34020_INT1,  -1,			   16,29lew,  3,29,LE,2,10	),
#endif
#if (HAS_TMS9900)
	CPU0(TMS9900,  tms9900,  1,  0,1.00,TMS9900_NONE,	   -1,			   -1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS9940)
	CPU0(TMS9940,  tms9940,  1,  0,1.00,TMS9940_NONE,	   -1,			   -1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS9980)
	CPU0(TMS9980,  tms9980a, 1,  0,1.00,TMS9980A_NONE,	   -1,			   -1,			   8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9985)
	CPU0(TMS9985,  tms9985,  1,  0,1.00,TMS9985_NONE,	   -1,			   -1,			   8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9989)
	CPU0(TMS9989,  tms9989,  1,  0,1.00,TMS9989_NONE,	   -1,			   -1,			   8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS9995)
	CPU0(TMS9995,  tms9995,  1,  0,1.00,TMS9995_NONE,	   -1,			   -1,			   8, 16,	  0,16,BE,1, 6	),
#endif
#if (HAS_TMS99105A)
	CPU0(TMS99105A,tms99105a,1,  0,1.00,TMS99105A_NONE,    -1,			   -1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS99110A)
	CPU0(TMS99110A,tms99110a,1,  0,1.00,TMS99110A_NONE,    -1,			   -1,			   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_Z8000)
	CPU0(Z8000,    z8000,	 2,  0,1.00,Z8000_INT_NONE,    Z8000_NVI,	   Z8000_NMI,	   16,16bew,  0,16,BE,2, 6	),
#endif
#if (HAS_TMS320C10)
	CPU3(TMS320C10,tms320c10,2,  0,1.00,TMS320C10_INT_NONE,-1,			   -1,			   16,16bew, -1,16,BE,2, 4	),
#endif
#if (HAS_CCPU)
	CPU3(CCPU,	   ccpu,	 2,  0,1.00,0,				   -1,			   -1,			   16,16bew,  0,15,BE,2, 3	),
#endif
#if (HAS_ADSP2100)
	CPU3(ADSP2100, adsp2100, 4,  0,1.00,ADSP2100_INT_NONE, -1,			   -1,			   16,17lew, -1,14,LE,2, 4	),
#endif
#if (HAS_ADSP2105)
	CPU3(ADSP2105, adsp2105, 4,  0,1.00,ADSP2105_INT_NONE, -1,			   -1,			   16,17lew, -1,14,LE,2, 4	),
#endif
#if (HAS_PSXCPU)
	CPU0(PSXCPU,   mips,	 8, -1,1.00,MIPS_INT_NONE,	   MIPS_INT_NONE,  MIPS_INT_NONE,  16,32lew,  0,32,LE,4, 4	),
#endif
#if (HAS_ASAP)
	#define asap_ICount asap_icount
	CPU0(ASAP,	   asap,	 1,  0,1.00,ASAP_INT_NONE,	   -1,			   -1,			   32,32ledw, 0,32,LE,4, 12 ),
#endif
#if (HAS_UPD7810)
#define upd7810_ICount upd7810_icount
	CPU0(UPD7810,  upd7810,  2,  0,1.00,UPD7810_INT_NONE,  UPD7810_INTF1,  UPD7810_INTNMI, 8, 16,	  0,16,LE,1, 4	),
#endif

#ifdef MESS
#if (HAS_APEXC)
	CPU0(APEXC,    apexc,	 0,  0,1.00,0,				   -1,			   -1,			   32,18bedw, 0,18,LE,1, 1	),
#endif
#if (HAS_ARM)
	CPU0(ARM,	   arm, 	 2,  0,1.00,ARM_INT_NONE,	   ARM_FIRQ,	   ARM_IRQ, 	   32,26ledw, 0,26,LE,4, 4	),
#endif
#if (HAS_CDP1802)
#define cdp1802_ICount cdp1802_icount
	CPU0(CDP1802,  cdp1802,  1,  0,1.00,CDP1802_INT_NONE,  CDP1802_IRQ,    -1,			   8, 16,	  0,16,BE,1, 3	),
#endif
#if (HAS_CP1600)
#define cp1600_ICount cp1600_icount
	CPU0(CP1600,   cp1600,	 0,  0,1.00,CP1600_INT_NONE,   -1,			   -1,			   8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_F8)
#define f8_ICount f8_icount
	CPU4(F8,	   f8,		 1,  0,1.00,F8_INT_NONE,	   F8_INT_INTR,    -1,			   8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_G65816)
	CPU0(G65816,  g65816,	 1,  0,1.00,G65816_INT_NONE,   G65816_INT_IRQ, G65816_INT_NMI, 8, 24,	  0,24,BE,1, 3	),
#endif
#if (HAS_LH5801)
#define lh5801_ICount lh5801_icount
	CPU0(LH5801,   lh5801,	 1,  0,1.00,LH5801_INT_NONE,   LH5801_IRQ,	   -1,			   8, 17,	  0,17,BE,1, 5	),
#endif
#if (HAS_PDP1)
	CPU0(PDP1,	   pdp1,	 0,  0,1.00,0,				   -1,			   -1,			   8, 16,	  0,18,LE,1, 3	),
#endif
#if (HAS_SATURN)
#define saturn_ICount saturn_icount
	CPU0(SATURN,   saturn,	 1,  0,1.00,SATURN_INT_NONE,   SATURN_INT_IRQ, SATURN_INT_NMI,	8,20,	  0,20,LE,1, 21 ),
#endif
#if (HAS_SC61860)
	#define sc61860_ICount sc61860_icount
	CPU0(SC61860,  sc61860,  1,  0,1.00,-1, 			   -1,			   -1,			   8, 16,	  0,16,BE,1, 4	),
#endif
#if (HAS_SH2)
	CPU4(SH2,	   sh2, 	16,  0,1.00,SH2_INT_NONE ,				 0, 			-1,   32,32bedw,   0,32,BE,2, 2  ),
#endif
#if (HAS_SPC700)
	CPU0(SPC700,   spc700,	 0,  0,1.00,0,				   -1,			   -1,			   8, 16,	  0,16,LE,1, 3	),
#endif
#if (HAS_Z80GB)
	CPU0(Z80GB,    z80gb,	 5,255,1.00,Z80GB_IGNORE_INT,  0,			   1,			   8, 16,	  0,16,LE,1, 4	),
#endif
#if (HAS_Z80_MSX)
	CPU1(Z80_MSX,  z80_msx,	 1,255,1.00,Z80_IGNORE_INT,    Z80_IRQ_INT,    Z80_NMI_INT,    8, 16,	  0,16,LE,1, 4	),
#endif
#endif
};

void cpu_init(void)
{
	int i;

	/* Verify the order of entries in the cpuintf[] array */
	for( i = 0; i < CPU_COUNT; i++ )
	{
		if( cpuintf[i].cpu_num != i )
		{
			printf("CPU #%d [%s] wrong ID %d: check enum CPU_... in src/cpuintrf.h!\n", i, cputype_name(i), cpuintf[i].cpu_num);
			exit(1);
		}
	}

	/* count how many CPUs we have to emulate */
	totalcpu = 0;

	while (totalcpu < MAX_CPU)
	{
		if( CPU_TYPE(totalcpu) == CPU_DUMMY ) break;
		totalcpu++;
	}

	/* zap the CPU data structure */
	memset(cpu, 0, sizeof(cpu));

	/* Set up the interface functions */
	for (i = 0; i < MAX_CPU; i++)
	{
		cpu[i].intf = &cpuintf[CPU_TYPE(i)];
	}

	for (i = 0; i < totalcpu; i++)
	{
		int j, size;

		activecpu = i;

		/* allocate a context buffer for the CPU */
		size = GETCONTEXT(i,NULL);
		if( size == 0 )
		{
			/* That can't really be true */
logerror("CPU #%d claims to need no context buffer!\n", i);
			raise( SIGABRT );
		}

		cpu[i].context = malloc( size );
		if( cpu[i].context == NULL )
		{
			/* That's really bad :( */
logerror("CPU #%d failed to allocate context buffer (%d bytes)!\n", i, size);
			raise( SIGABRT );
		}

		/* Zap the context buffer */
		memset(cpu[i].context, 0, size );

		/* Save if there is another CPU of the same type */
		cpu[i].save_context = 0;

		for (j = 0; j < totalcpu; j++)
			if ( i != j && !strcmp(cpunum_core_file(i),cpunum_core_file(j)) )
				cpu[i].save_context = 1;

		#ifdef MAME_DEBUG

		/* or if we're running with the debugger */
		{
			cpu[i].save_context |= mame_debug;
		}
		#endif

		for( j = 0; j < MAX_IRQ_LINES; j++ )
		{
			irq_line_state[i * MAX_IRQ_LINES + j] = CLEAR_LINE;
			irq_line_vector[i * MAX_IRQ_LINES + j] = cpuintf[CPU_TYPE(i)].default_vector;
		}

		state_save_set_current_tag(i+1);
		INIT(i);
		if (cpu[i].save_context) GETCONTEXT(i, cpu[i].context);
	}

	state_save_set_current_tag(0);

	state_save_register_UINT8("cpu", 0, "irq enable",     interrupt_enable,  totalcpu);
	state_save_register_INT32("cpu", 0, "irq vector",     interrupt_vector,  totalcpu);
	state_save_register_UINT8("cpu", 0, "irqline state",  irq_line_state,    totalcpu*MAX_IRQ_LINES);
	state_save_register_INT32("cpu", 0, "irqline vector", irq_line_vector,   totalcpu*MAX_IRQ_LINES);
	state_save_register_INT32("cpu", 0, "watchdog count", &watchdog_counter, 1);

	/* reset the timer system */
	timer_init();
	timeslice_timer = refresh_timer = vblank_timer = NULL;
}

void cpu_run(void)
{
	int i;

#ifdef	MAME_DEBUG
	/* Initialize the debugger */
	if( mame_debug )
		mame_debug_init();
#endif


reset:
	/* read hi scores information from hiscore.dat */
	hs_open(Machine->gamedrv->name);
	hs_init();

	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers();
	watchdog_counter = -1;

	/* reset sound chips */
	sound_reset();

	/* enable all CPUs (except for audio CPUs if the sound is off) */
	for (i = 0; i < totalcpu; i++)
	{
		if (!CPU_AUDIO(i) || Machine->sample_rate != 0)
		{
			timer_suspendcpu(i, 0, SUSPEND_ANY_REASON);
		}
		else
		{
			timer_suspendcpu(i, 1, SUSPEND_REASON_DISABLE);
		}
	}

	have_to_reset = 0;
	vblank = 0;

logerror("Machine reset\n");

	/* start with interrupts enabled, so the generic routine will work even if */
	/* the machine doesn't have an interrupt enable port */
	for (i = 0;i < MAX_CPU;i++)
	{
		interrupt_enable[i] = 1;
		interrupt_vector[i] = 0xff;
		/* Reset any driver hooks into the IRQ acknowledge callbacks */
		drv_irq_callbacks[i] = NULL;
	}

	/* do this AFTER the above so init_machine() can use cpu_halt() to hold the */
	/* execution of some CPUs, or disable interrupts */
	if (Machine->drv->init_machine) (*Machine->drv->init_machine)();

	/* reset each CPU */
	for (i = 0; i < totalcpu; i++)
#ifdef PINMAME
	if (!CPU_AUDIO(i) || Machine->sample_rate != 0)
#endif /* PINMAME */
	{
		/* swap memory contexts and reset */
		memory_set_context(i);
		if (cpu[i].save_context) SETCONTEXT(i, cpu[i].context);
		activecpu = i;
		RESET(i);

		/* Set the irq callback for the cpu */
		SETIRQCALLBACK(i,cpu_irq_callbacks[i]);

		/* save the CPU context if necessary */
		if (cpu[i].save_context) GETCONTEXT (i, cpu[i].context);

		/* reset the total number of cycles */
		cpu[i].totalcycles = 0;
	}

	/* reset the globals */
	cpu_vblankreset();
	current_frame = 0;
	state_save_dump_registry();

	/* loop until the user quits */
	usres = 0;
	while (usres == 0)
	{
		int cpunum;

		/* was machine_reset() called? */
		if (have_to_reset)
		{
#if defined(MESS) || defined(PINMAME_EXIT)
			if (Machine->drv->stop_machine) (*Machine->drv->stop_machine)();
#endif
			goto reset;
		}
		profiler_mark(PROFILER_EXTRA);

		if (loadsave_schedule != LOADSAVE_NONE)
		{
			if (loadsave_schedule == LOADSAVE_SAVE)
			{
				void *file;
				char name[2];
				name[0] = loadsave_schedule_id;
				name[1] = 0;
				file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_STATE, 1);

				state_save_save_begin(file);
				state_save_set_current_tag(0);
				state_save_save_continue();

				for( cpunum = 0; cpunum < totalcpu; cpunum++ )
				{
					activecpu = cpunum;
					memory_set_context(activecpu);
					if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
					/* make sure any bank switching is reset */
					SET_OP_BASE(activecpu, cpu_get_pc_byte());
					state_save_set_current_tag(activecpu + 1);
					state_save_save_continue();
				}

				state_save_save_finish();
				osd_fclose(file);
			}
			else
			{
				void *file;
				char name[2];
				name[0] = loadsave_schedule_id;
				name[1] = 0;
				file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_STATE, 0);

				if(file)
				{
					if (!state_save_load_begin(file))
					{
						state_save_set_current_tag(0);
						state_save_load_continue();

						for( cpunum = 0; cpunum < totalcpu; cpunum++ )
						{
							activecpu = cpunum;
							memory_set_context(activecpu);
							if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
							/* make sure any bank switching is reset */
							SET_OP_BASE(activecpu, cpu_get_pc_byte());
							state_save_set_current_tag(activecpu + 1);
							state_save_load_continue();
							/* update the contexts */
							if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
						}

						state_save_load_finish();
					}
					osd_fclose(file);
				}
			}

			loadsave_schedule = LOADSAVE_NONE;
		}
		/* ask the timer system to schedule */
		if (timer_schedule_cpu(&cpunum, &cycles_running))
		{
			int ran;

			/* switch memory and CPU contexts */
			activecpu = cpunum;
			memory_set_context(activecpu);
			if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

			/* make sure any bank switching is reset */
			SET_OP_BASE(activecpu, cpu_get_pc_byte());

			/* run for the requested number of cycles */
			profiler_mark(PROFILER_CPU1 + cpunum);
			ran = EXECUTE(activecpu, cycles_running);
			profiler_mark(PROFILER_END);

			/* update based on how many cycles we really ran */
			cpu[activecpu].totalcycles += ran;

			/* update the contexts */
			if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
			activecpu = -1;

			/* update the timer with how long we actually ran */
			timer_update_cpu(cpunum, ran);

		}

		profiler_mark(PROFILER_END);
	}

	/* write hi scores to disk - No scores saving if cheat */
	hs_close();

#if defined(MESS) || defined(PINMAME_EXIT)
	if (Machine->drv->stop_machine) (*Machine->drv->stop_machine)();
#endif

#ifdef	MAME_DEBUG
	/* Shut down the debugger */
	if( mame_debug )
		mame_debug_exit();
#endif

	/* shut down the CPU cores */
	for (i = 0; i < totalcpu; i++)
	{
		/* if the CPU core defines an exit function, call it now */
		if( cpu[i].intf->exit )
			(*cpu[i].intf->exit)();

		/* free the context buffer for that CPU */
		if( cpu[i].context )
		{
			free( cpu[i].context );
			cpu[i].context = NULL;
		}
	}
	totalcpu = 0;
}

void cpu_loadsave_schedule(int type, char id)
{
	loadsave_schedule = type;
	loadsave_schedule_id = id;
}

void cpu_loadsave_reset(void)
{
	loadsave_schedule = LOADSAVE_NONE;
}


/***************************************************************************

  Use this function to initialize, and later maintain, the watchdog. For
  convenience, when the machine is reset, the watchdog is disabled. If you
  call this function, the watchdog is initialized, and from that point
  onwards, if you don't call it at least once every 2 seconds, the machine
  will be reset.

  The 3 seconds delay is targeted at qzshowby, which otherwise would reset
  at the start of a game.

***************************************************************************/

static void watchdog_reset(void)
{
	if (watchdog_counter == -1) logerror("watchdog armed\n");
	watchdog_counter = 3*Machine->drv->frames_per_second;
}

WRITE_HANDLER( watchdog_reset_w )
{
	watchdog_reset();
}

READ_HANDLER( watchdog_reset_r )
{
	watchdog_reset();
	return 0xff;
}

WRITE16_HANDLER( watchdog_reset16_w )
{
	watchdog_reset();
}

READ16_HANDLER( watchdog_reset16_r )
{
	watchdog_reset();
	return 0xffff;
}


/***************************************************************************

  This function resets the machine (the reset will not take place
  immediately, it will be performed at the end of the active CPU's time
  slice)

***************************************************************************/
void machine_reset(void)
{
	/* write hi scores to disk - No scores saving if cheat */
	hs_close();

	have_to_reset = 1;
}



/***************************************************************************

  Use this function to reset a specified CPU immediately

***************************************************************************/
void cpu_set_reset_line(int cpunum,int state)
{
	timer_set(TIME_NOW, (cpunum & 7) | (state << 3), cpu_resetcallback);
}


/***************************************************************************

  Use this function to control the HALT line on a CPU

***************************************************************************/
void cpu_set_halt_line(int cpunum,int state)
{
	timer_set(TIME_NOW, (cpunum & 7) | (state << 3), cpu_haltcallback);
}


/***************************************************************************

  Use this function to install a callback for IRQ acknowledge

***************************************************************************/
void cpu_set_irq_callback(int cpunum, int (*callback)(int))
{
	drv_irq_callbacks[cpunum] = callback;
}


/***************************************************************************

  This function returns CPUNUM current status  (running or halted)

***************************************************************************/
int cpu_getstatus(int cpunum)
{
	if (cpunum >= MAX_CPU) return 0;

	return !timer_iscpususpended(cpunum,
			SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE);
}



int cpu_getactivecpu(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpunum;
}

void cpu_setactivecpu(int cpunum)
{
	activecpu = cpunum;
}

int cpu_gettotalcpu(void)
{
	return totalcpu;
}



offs_t cpu_get_pc(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETPC(cpunum);
}

offs_t cpu_get_pc_byte(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int shift = cpuintf[CPU_TYPE(cpunum)].address_shift;
	offs_t base = cpuintf[CPU_TYPE(cpunum)].pgm_memory_base;
	offs_t pc = GETPC(cpunum);
	return base + ((shift < 0) ? (pc << -shift) : (pc >> shift));
}

void cpu_set_pc(offs_t val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETPC(cpunum,val);
}

offs_t cpu_get_sp(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETSP(cpunum);
}

void cpu_set_sp(offs_t val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETSP(cpunum,val);
}

/* these are available externally, for the timer system */
int cycles_currently_ran(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cycles_running - ICOUNT(cpunum);
}

int cycles_left_to_run(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return ICOUNT(cpunum);
}

void cpu_set_op_base(unsigned val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SET_OP_BASE(cpunum,val);
}


/***************************************************************************

  Returns the number of CPU cycles since the last reset of the CPU

  IMPORTANT: this value wraps around in a relatively short time.
  For example, for a 6MHz CPU, it will wrap around in
  2^32/6000000 = 716 seconds = 12 minutes.
  Make sure you don't do comparisons between values returned by this
  function, but only use the difference (which will be correct regardless
  of wraparound).

***************************************************************************/
int cpu_gettotalcycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpu[cpunum].totalcycles + cycles_currently_ran();
}



/***************************************************************************

  Returns the number of CPU cycles before the next interrupt handler call

***************************************************************************/
int cpu_geticount(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int result = TIME_TO_CYCLES(cpunum, cpu[cpunum].vblankint_period - timer_timeelapsed(cpu[cpunum].vblankint_timer));
	return (result < 0) ? 0 : result;
}



/***************************************************************************

  Returns the number of CPU cycles before the end of the current video frame

***************************************************************************/
int cpu_getfcount(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int result = TIME_TO_CYCLES(cpunum, refresh_period - timer_timeelapsed(refresh_timer));
	return (result < 0) ? 0 : result;
}



/***************************************************************************

  Returns the number of CPU cycles in one video frame

***************************************************************************/
int cpu_getfperiod(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES(cpunum, refresh_period);
}



/***************************************************************************

  Scales a given value by the ratio of fcount / fperiod

***************************************************************************/
int cpu_scalebyfcount(int value)
{
	int result = (int)((double)value * timer_timeelapsed(refresh_timer) * refresh_period_inv);
	if (value >= 0) return (result < value) ? result : value;
	else return (result > value) ? result : value;
}



/***************************************************************************

  Returns the current scanline, or the time until a specific scanline

  Note: cpu_getscanline() counts from 0, 0 being the first visible line. You
  might have to adjust this value to match the hardware, since in many cases
  the first visible line is >0.

***************************************************************************/
int cpu_getscanline(void)
{
	return (int)(timer_timeelapsed(refresh_timer) * scanline_period_inv);
}


double cpu_getscanlinetime(int scanline)
{
	double ret;
	double scantime = timer_starttime(refresh_timer) + (double)scanline * scanline_period;
	double abstime = timer_get_time();
	if (abstime >= scantime) scantime += TIME_IN_HZ(Machine->drv->frames_per_second);
	ret = scantime - abstime;
	if (ret < TIME_IN_NSEC(1))
	{
		ret = TIME_IN_HZ(Machine->drv->frames_per_second);
	}

	return ret;
}


double cpu_getscanlineperiod(void)
{
	return scanline_period;
}


/***************************************************************************

  Returns the number of cycles in a scanline

 ***************************************************************************/
int cpu_getscanlinecycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES(cpunum, scanline_period);
}


/***************************************************************************

  Returns the number of cycles since the beginning of this frame

 ***************************************************************************/
int cpu_getcurrentcycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES(cpunum, timer_timeelapsed(refresh_timer));
}


/***************************************************************************

  Returns the current horizontal beam position in pixels

 ***************************************************************************/
int cpu_gethorzbeampos(void)
{
	double elapsed_time = timer_timeelapsed(refresh_timer);
	int scanline = (int)(elapsed_time * scanline_period_inv);
	double time_since_scanline = elapsed_time -
						 (double)scanline * scanline_period;
	return (int)(time_since_scanline * scanline_period_inv *
						 (double)Machine->drv->screen_width);
}


/***************************************************************************

  Returns the number of times the interrupt handler will be called before
  the end of the current video frame. This can be useful to interrupt
  handlers to synchronize their operation. If you call this from outside
  an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
  that the interrupt handler will be called once.

***************************************************************************/
int cpu_getiloops(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpu[cpunum].iloops;
}



/***************************************************************************

  Interrupt handling

***************************************************************************/

/***************************************************************************

  These functions are called when a cpu calls the callback sent to it's
  set_irq_callback function. It clears the irq line if the current state
  is HOLD_LINE and returns the interrupt vector for that line.

***************************************************************************/
#define MAKE_IRQ_CALLBACK(num)												\
static int cpu_##num##_irq_callback(int irqline)							\
{																			\
	int vector = irq_line_vector[num * MAX_IRQ_LINES + irqline];			\
	if( irq_line_state[num * MAX_IRQ_LINES + irqline] == HOLD_LINE )		\
	{																		\
		SETIRQLINE(num, irqline, CLEAR_LINE);								\
		irq_line_state[num * MAX_IRQ_LINES + irqline] = CLEAR_LINE; 		\
	}																		\
	LOG(("cpu_##num##_irq_callback(%d) $%04x\n", irqline, vector));         \
	if( drv_irq_callbacks[num] )											\
		return (*drv_irq_callbacks[num])(irqline);							\
	return vector;															\
}

MAKE_IRQ_CALLBACK(0)
MAKE_IRQ_CALLBACK(1)
MAKE_IRQ_CALLBACK(2)
MAKE_IRQ_CALLBACK(3)
MAKE_IRQ_CALLBACK(4)
MAKE_IRQ_CALLBACK(5)
MAKE_IRQ_CALLBACK(6)
MAKE_IRQ_CALLBACK(7)

/***************************************************************************

  This function is used to generate internal interrupts (TMS34010)

***************************************************************************/
void cpu_generate_internal_interrupt(int cpunum, int type)
{
	timer_set(TIME_NOW, (cpunum & 7) | (type << 3), cpu_internalintcallback);
}


/***************************************************************************

  Use this functions to set the vector for a irq line of a CPU

***************************************************************************/
void cpu_irq_line_vector_w(int cpunum, int irqline, int vector)
{
	cpunum &= (MAX_CPU - 1);
	irqline &= (MAX_IRQ_LINES - 1);
	if( irqline < cpu[cpunum].intf->num_irqs )
	{
		LOG(("cpu_irq_line_vector_w(%d,%d,$%04x)\n",cpunum,irqline,vector));
		irq_line_vector[cpunum * MAX_IRQ_LINES + irqline] = vector;
		return;
	}
	LOG(("cpu_irq_line_vector_w CPU#%d irqline %d > max irq lines\n", cpunum, irqline));
}

/***************************************************************************

  Use these functions to set the vector (data) for a irq line (offset)
  of CPU #0 to #3

***************************************************************************/
WRITE_HANDLER( cpu_0_irq_line_vector_w ) { cpu_irq_line_vector_w(0, offset, data); }
WRITE_HANDLER( cpu_1_irq_line_vector_w ) { cpu_irq_line_vector_w(1, offset, data); }
WRITE_HANDLER( cpu_2_irq_line_vector_w ) { cpu_irq_line_vector_w(2, offset, data); }
WRITE_HANDLER( cpu_3_irq_line_vector_w ) { cpu_irq_line_vector_w(3, offset, data); }
WRITE_HANDLER( cpu_4_irq_line_vector_w ) { cpu_irq_line_vector_w(4, offset, data); }
WRITE_HANDLER( cpu_5_irq_line_vector_w ) { cpu_irq_line_vector_w(5, offset, data); }
WRITE_HANDLER( cpu_6_irq_line_vector_w ) { cpu_irq_line_vector_w(6, offset, data); }
WRITE_HANDLER( cpu_7_irq_line_vector_w ) { cpu_irq_line_vector_w(7, offset, data); }

/***************************************************************************

  Use this function to set the state the NMI line of a CPU

***************************************************************************/
void cpu_set_nmi_line(int cpunum, int state)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	LOG(("cpu_set_nmi_line(%d,%d)\n",cpunum,state));
	timer_set(TIME_NOW, (cpunum & 7) | (state << 3), cpu_manualnmicallback);
}

/***************************************************************************

  Use this function to set the state of an IRQ line of a CPU
  The meaning of irqline varies between the different CPU types

***************************************************************************/
void cpu_set_irq_line(int cpunum, int irqline, int state)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	LOG(("cpu_set_irq_line(%d,%d,%d)\n",cpunum,irqline,state));
	timer_set(TIME_NOW, (irqline & 7) | ((cpunum & 7) << 3) | (state << 6), cpu_manualirqcallback);
}

/***************************************************************************

  Use this function to cause an interrupt immediately (don't have to wait
  until the next call to the interrupt handler)

***************************************************************************/
void cpu_cause_interrupt(int cpunum,int type)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	timer_set(TIME_NOW, (cpunum & 7) | (type << 3), cpu_manualintcallback);
}



void cpu_clear_pending_interrupts(int cpunum)
{
	timer_set(TIME_NOW, cpunum, cpu_clearintcallback);
}



void cpu_interrupt_enable(int cpunum,int enabled)
{
	interrupt_enable[cpunum] = enabled;

	/* make sure there are no queued interrupts */
	if (enabled == 0) cpu_clear_pending_interrupts(cpunum);
}

WRITE_HANDLER( interrupt_enable_w )
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	cpu_interrupt_enable(cpunum,data);
}



WRITE_HANDLER( interrupt_vector_w )
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_vector[cpunum] != data)
	{
		LOG(("CPU#%d interrupt_vector_w $%02x\n", cpunum, data));
		interrupt_vector[cpunum] = data;

		/* make sure there are no queued interrupts */
		cpu_clear_pending_interrupts(cpunum);
	}
}



int interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int val;

	if (interrupt_enable[cpunum] == 0)
		return INT_TYPE_NONE(cpunum);

	val = INT_TYPE_IRQ(cpunum);
	if (val == -1000)
		val = interrupt_vector[cpunum];

	return val;
}



int nmi_interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;

	if (interrupt_enable[cpunum] == 0)
		return INT_TYPE_NONE(cpunum);

	return INT_TYPE_NMI(cpunum);
}



#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
int m68_level1_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_1;
}
int m68_level2_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_2;
}
int m68_level3_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_3;
}
int m68_level4_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_4;
}
int m68_level5_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_5;
}
int m68_level6_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_6;
}
int m68_level7_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
	return MC68000_IRQ_7;
}
#endif


int ignore_interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return INT_TYPE_NONE(cpunum);
}



/***************************************************************************

  CPU timing and synchronization functions.

***************************************************************************/

/* generate a trigger */
void cpu_trigger(int trigger)
{
	timer_trigger(trigger);
}

/* generate a trigger after a specific period of time */
void cpu_triggertime(double duration, int trigger)
{
	timer_set(duration, trigger, cpu_trigger);
}



/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger(int trigger)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	timer_suspendcpu_trigger(cpunum, trigger);
}

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	cpu_spinuntil_trigger(TRIGGER_INT + cpunum);
}

/* burn CPU cycles until our timeslice is up */
void cpu_spin(void)
{
	cpu_spinuntil_trigger(TRIGGER_TIMESLICE);
}

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time(double duration)
{
	static int timetrig = 0;

	cpu_spinuntil_trigger(TRIGGER_SUSPENDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_SUSPENDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



/* yield our timeslice for a specific period of time */
void cpu_yielduntil_trigger(int trigger)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	timer_holdcpu_trigger(cpunum, trigger);
}

/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	cpu_yielduntil_trigger(TRIGGER_INT + cpunum);
}

/* yield our current timeslice */
void cpu_yield(void)
{
	cpu_yielduntil_trigger(TRIGGER_TIMESLICE);
}

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time(double duration)
{
	static int timetrig = 0;

	cpu_yielduntil_trigger(TRIGGER_YIELDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_YIELDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



int cpu_getvblank(void)
{
	return vblank;
}


int cpu_getcurrentframe(void)
{
	return current_frame;
}


/***************************************************************************

  Internal CPU event processors.

***************************************************************************/

static void cpu_manualnmicallback(int param)
{
	int cpunum, state, oldactive;
	cpunum = param & 7;
	state = param >> 3;

	/* swap to the CPU's context */
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	LOG(("cpu_manualnmicallback %d,%d\n",cpunum,state));

	switch (state)
	{
		case PULSE_LINE:
			SETNMILINE(cpunum,ASSERT_LINE);
			SETNMILINE(cpunum,CLEAR_LINE);
			break;
		case HOLD_LINE:
		case ASSERT_LINE:
			SETNMILINE(cpunum,ASSERT_LINE);
			break;
		case CLEAR_LINE:
			SETNMILINE(cpunum,CLEAR_LINE);
			break;
		default:
			logerror("cpu_manualnmicallback cpu #%d unknown state %d\n", cpunum, state);
	}
	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (state != CLEAR_LINE)
		timer_trigger(TRIGGER_INT + cpunum);
}

static void cpu_manualirqcallback(int param)
{
	int cpunum, irqline, state, oldactive;

	irqline = param & 7;
	cpunum = (param >> 3) & 7;
	state = param >> 6;

	/* swap to the CPU's context */
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	LOG(("cpu_manualirqcallback %d,%d,%d\n",cpunum,irqline,state));

	irq_line_state[cpunum * MAX_IRQ_LINES + irqline] = state;
	switch (state)
	{
		case PULSE_LINE:
			SETIRQLINE(cpunum,irqline,ASSERT_LINE);
			SETIRQLINE(cpunum,irqline,CLEAR_LINE);
			break;
		case HOLD_LINE:
		case ASSERT_LINE:
			SETIRQLINE(cpunum,irqline,ASSERT_LINE);
			break;
		case CLEAR_LINE:
			SETIRQLINE(cpunum,irqline,CLEAR_LINE);
			break;
		default:
			logerror("cpu_manualirqcallback cpu #%d, line %d, unknown state %d\n", cpunum, irqline, state);
	}

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (state != CLEAR_LINE)
		timer_trigger(TRIGGER_INT + cpunum);
}

static void cpu_internal_interrupt(int cpunum, int type)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	INTERNAL_INTERRUPT(cpunum, type);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	timer_trigger(TRIGGER_INT + cpunum);
}

static void cpu_internalintcallback(int param)
{
	int type = param >> 3;
	int cpunum = param & 7;

	LOG(("CPU#%d internal interrupt type $%04x\n", cpunum, type));
	/* generate the interrupt */
	cpu_internal_interrupt(cpunum, type);
}

static void cpu_generate_interrupt(int cpunum, int (*func)(void), int num)
{
	int oldactive = activecpu;

	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	/* cause the interrupt, calling the function if it exists */
	if (func) num = (*func)();

	/* wrapper for the new interrupt system */
	if (num != INT_TYPE_NONE(cpunum))
	{
		LOG(("CPU#%d interrupt type $%04x: ", cpunum, num));
		/* is it the NMI type interrupt of that CPU? */
		if (num == INT_TYPE_NMI(cpunum))
		{

			LOG(("NMI\n"));
			cpu_manualnmicallback(cpunum | (PULSE_LINE << 3) );

		}
		else
		{
			int irq_line;

			switch (CPU_TYPE(cpunum))
			{
#if (HAS_Z80)
			case CPU_Z80:				irq_line = 0; LOG(("Z80 IRQ\n")); break;
#endif
#if (HAS_8080)
			case CPU_8080:
				switch (num)
				{
				case I8080_INTR:		irq_line = 0; LOG(("I8080 INTR\n")); break;
				default:				irq_line = 0; LOG(("I8080 unknown\n"));
				}
				break;
#endif
#if (HAS_8085A)
			case CPU_8085A:
				switch (num)
				{
				case I8085_INTR:		irq_line = 0; LOG(("I8085 INTR\n")); break;
				case I8085_RST55:		irq_line = 1; LOG(("I8085 RST55\n")); break;
				case I8085_RST65:		irq_line = 2; LOG(("I8085 RST65\n")); break;
				case I8085_RST75:		irq_line = 3; LOG(("I8085 RST75\n")); break;
				default:				irq_line = 0; LOG(("I8085 unknown\n"));
				}
				break;
#endif
#if (HAS_M6502)
			case CPU_M6502: 			irq_line = 0; LOG(("M6502 IRQ\n")); break;
#endif
#if (HAS_M65C02)
			case CPU_M65C02:			irq_line = 0; LOG(("M65C02 IRQ\n")); break;
#endif
#if (HAS_M65SC02)
			case CPU_M65SC02:			irq_line = 0; LOG(("M65SC02 IRQ\n")); break;
#endif
#if (HAS_M65CE02)
			case CPU_M65CE02:			irq_line = 0; LOG(("M65CE02 IRQ\n")); break;
#endif
#if (HAS_M6509)
			case CPU_M6509: 			irq_line = 0; LOG(("M6509 IRQ\n")); break;
#endif
#if (HAS_M6510)
			case CPU_M6510: 			irq_line = 0; LOG(("M6510 IRQ\n")); break;
#endif
#if (HAS_M6510T)
			case CPU_M6510T:			irq_line = 0; LOG(("M6510T IRQ\n")); break;
#endif
#if (HAS_M7501)
			case CPU_M7501: 			irq_line = 0; LOG(("M7501 IRQ\n")); break;
#endif
#if (HAS_M8502)
			case CPU_M8502: 			irq_line = 0; LOG(("M8502 IRQ\n")); break;
#endif
#if (HAS_N2A03)
			case CPU_N2A03: 			irq_line = 0; LOG(("N2A03 IRQ\n")); break;
#endif
#if (HAS_M4510)
			case CPU_M4510: 			irq_line = 0; LOG(("M4510 IRQ\n")); break;
#endif
#if (HAS_H6280)
			case CPU_H6280:
				switch (num)
				{
				case H6280_INT_IRQ1:	irq_line = 0; LOG(("H6280 INT 1\n")); break;
				case H6280_INT_IRQ2:	irq_line = 1; LOG(("H6280 INT 2\n")); break;
				case H6280_INT_TIMER:	irq_line = 2; LOG(("H6280 TIMER INT\n")); break;
				default:				irq_line = 0; LOG(("H6280 unknown\n"));
				}
				break;
#endif
#if (HAS_I86)
			case CPU_I86:				irq_line = 0; LOG(("I86 IRQ\n")); break;
#endif
#if (HAS_I88)
			case CPU_I88:				irq_line = 0; LOG(("I88 IRQ\n")); break;
#endif
#if (HAS_I186)
			case CPU_I186:				irq_line = 0; LOG(("I186 IRQ\n")); break;
#endif
#if (HAS_I188)
			case CPU_I188:				irq_line = 0; LOG(("I188 IRQ\n")); break;
#endif
#if (HAS_I286)
			case CPU_I286:				irq_line = 0; LOG(("I286 IRQ\n")); break;
#endif
#if (HAS_V20)
			case CPU_V20:				irq_line = 0; LOG(("V20 IRQ\n")); break;
#endif
#if (HAS_V30)
			case CPU_V30:				irq_line = 0; LOG(("V30 IRQ\n")); break;
#endif
#if (HAS_V33)
			case CPU_V33:				irq_line = 0; LOG(("V33 IRQ\n")); break;
#endif
#if (HAS_I8035)
			case CPU_I8035: 			irq_line = 0; LOG(("I8035 IRQ\n")); break;
#endif
#if (HAS_I8039)
			case CPU_I8039: 			irq_line = 0; LOG(("I8039 IRQ\n")); break;
#endif
#if (HAS_I8048)
			case CPU_I8048: 			irq_line = 0; LOG(("I8048 IRQ\n")); break;
#endif
#if (HAS_N7751)
			case CPU_N7751: 			irq_line = 0; LOG(("N7751 IRQ\n")); break;
#endif
#if (HAS_M6800)
			case CPU_M6800: 			irq_line = 0; LOG(("M6800 IRQ\n")); break;
#endif
#if (HAS_M6801)
			case CPU_M6801: 			irq_line = 0; LOG(("M6801 IRQ\n")); break;
#endif
#if (HAS_M6802)
			case CPU_M6802: 			irq_line = 0; LOG(("M6802 IRQ\n")); break;
#endif
#if (HAS_M6803)
			case CPU_M6803: 			irq_line = 0; LOG(("M6803 IRQ\n")); break;
#endif
#if (HAS_M6808)
			case CPU_M6808: 			irq_line = 0; LOG(("M6808 IRQ\n")); break;
#endif
#if (HAS_HD63701)
			case CPU_HD63701:			irq_line = 0; LOG(("HD63701 IRQ\n")); break;
#endif
#if (HAS_M6805)
			case CPU_M6805: 			irq_line = 0; LOG(("M6805 IRQ\n")); break;
#endif
#if (HAS_M68705)
			case CPU_M68705:			irq_line = 0; LOG(("M68705 IRQ\n")); break;
#endif
#if (HAS_HD63705)
			case CPU_HD63705:			irq_line = 0; LOG(("HD68705 IRQ\n")); break;
#endif
#if (HAS_HD6309)
			case CPU_HD6309:
				switch (num)
				{
				case HD6309_INT_IRQ:	irq_line = 0; LOG(("M6309 IRQ\n")); break;
				case HD6309_INT_FIRQ:	irq_line = 1; LOG(("M6309 FIRQ\n")); break;
				default:				irq_line = 0; LOG(("M6309 unknown\n"));
				}
				break;
#endif
#if (HAS_M6809)
			case CPU_M6809:
				switch (num)
				{
				case M6809_INT_IRQ: 	irq_line = 0; LOG(("M6809 IRQ\n")); break;
				case M6809_INT_FIRQ:	irq_line = 1; LOG(("M6809 FIRQ\n")); break;
				default:				irq_line = 0; LOG(("M6809 unknown\n"));
				}
				break;
#endif
#if (HAS_KONAMI)
				case CPU_KONAMI:
				switch (num)
				{
				case KONAMI_INT_IRQ:	irq_line = 0; LOG(("KONAMI IRQ\n")); break;
				case KONAMI_INT_FIRQ:	irq_line = 1; LOG(("KONAMI FIRQ\n")); break;
				default:				irq_line = 0; LOG(("KONAMI unknown\n"));
				}
				break;
#endif
#if (HAS_M68000)
			case CPU_M68000:
				switch (num)
				{
				case MC68000_IRQ_1: 	irq_line = 1; LOG(("M68K IRQ1\n")); break;
				case MC68000_IRQ_2: 	irq_line = 2; LOG(("M68K IRQ2\n")); break;
				case MC68000_IRQ_3: 	irq_line = 3; LOG(("M68K IRQ3\n")); break;
				case MC68000_IRQ_4: 	irq_line = 4; LOG(("M68K IRQ4\n")); break;
				case MC68000_IRQ_5: 	irq_line = 5; LOG(("M68K IRQ5\n")); break;
				case MC68000_IRQ_6: 	irq_line = 6; LOG(("M68K IRQ6\n")); break;
				case MC68000_IRQ_7: 	irq_line = 7; LOG(("M68K IRQ7\n")); break;
				default:				irq_line = 0; LOG(("M68K unknown\n"));
				}
				/* until now only auto vector interrupts supported */
				num = MC68000_INT_ACK_AUTOVECTOR;
				break;
#endif
#if (HAS_M68010)
			case CPU_M68010:
				switch (num)
				{
				case MC68010_IRQ_1: 	irq_line = 1; LOG(("M68010 IRQ1\n")); break;
				case MC68010_IRQ_2: 	irq_line = 2; LOG(("M68010 IRQ2\n")); break;
				case MC68010_IRQ_3: 	irq_line = 3; LOG(("M68010 IRQ3\n")); break;
				case MC68010_IRQ_4: 	irq_line = 4; LOG(("M68010 IRQ4\n")); break;
				case MC68010_IRQ_5: 	irq_line = 5; LOG(("M68010 IRQ5\n")); break;
				case MC68010_IRQ_6: 	irq_line = 6; LOG(("M68010 IRQ6\n")); break;
				case MC68010_IRQ_7: 	irq_line = 7; LOG(("M68010 IRQ7\n")); break;
				default:				irq_line = 0; LOG(("M68010 unknown\n"));
				}
				/* until now only auto vector interrupts supported */
				num = MC68000_INT_ACK_AUTOVECTOR;
				break;
#endif
#if (HAS_M68020)
			case CPU_M68020:
				switch (num)
				{
				case MC68020_IRQ_1: 	irq_line = 1; LOG(("M68020 IRQ1\n")); break;
				case MC68020_IRQ_2: 	irq_line = 2; LOG(("M68020 IRQ2\n")); break;
				case MC68020_IRQ_3: 	irq_line = 3; LOG(("M68020 IRQ3\n")); break;
				case MC68020_IRQ_4: 	irq_line = 4; LOG(("M68020 IRQ4\n")); break;
				case MC68020_IRQ_5: 	irq_line = 5; LOG(("M68020 IRQ5\n")); break;
				case MC68020_IRQ_6: 	irq_line = 6; LOG(("M68020 IRQ6\n")); break;
				case MC68020_IRQ_7: 	irq_line = 7; LOG(("M68020 IRQ7\n")); break;
				default:				irq_line = 0; LOG(("M68020 unknown\n"));
				}
				/* until now only auto vector interrupts supported */
				num = MC68000_INT_ACK_AUTOVECTOR;
				break;
#endif
#if (HAS_M68EC020)
			case CPU_M68EC020:
				switch (num)
				{
				case MC68EC020_IRQ_1:	irq_line = 1; LOG(("M68EC020 IRQ1\n")); break;
				case MC68EC020_IRQ_2:	irq_line = 2; LOG(("M68EC020 IRQ2\n")); break;
				case MC68EC020_IRQ_3:	irq_line = 3; LOG(("M68EC020 IRQ3\n")); break;
				case MC68EC020_IRQ_4:	irq_line = 4; LOG(("M68EC020 IRQ4\n")); break;
				case MC68EC020_IRQ_5:	irq_line = 5; LOG(("M68EC020 IRQ5\n")); break;
				case MC68EC020_IRQ_6:	irq_line = 6; LOG(("M68EC020 IRQ6\n")); break;
				case MC68EC020_IRQ_7:	irq_line = 7; LOG(("M68EC020 IRQ7\n")); break;
				default:				irq_line = 0; LOG(("M68EC020 unknown\n"));
				}
				/* until now only auto vector interrupts supported */
				num = MC68000_INT_ACK_AUTOVECTOR;
				break;
#endif
#if (HAS_T11)
			case CPU_T11:
				switch (num)
				{
				case T11_IRQ0:			irq_line = 0; LOG(("T11 IRQ0\n")); break;
				case T11_IRQ1:			irq_line = 1; LOG(("T11 IRQ1\n")); break;
				case T11_IRQ2:			irq_line = 2; LOG(("T11 IRQ2\n")); break;
				case T11_IRQ3:			irq_line = 3; LOG(("T11 IRQ3\n")); break;
				default:				irq_line = 0; LOG(("T11 unknown\n"));
				}
				break;
#endif
#if (HAS_S2650)
			case CPU_S2650: 			irq_line = 0; LOG(("S2650 IRQ\n")); break;
#endif
#if (HAS_TMS34010)
			case CPU_TMS34010:
				switch (num)
				{
				case TMS34010_INT1: 	irq_line = 0; LOG(("TMS34010 INT1\n")); break;
				case TMS34010_INT2: 	irq_line = 1; LOG(("TMS34010 INT2\n")); break;
				default:				irq_line = 0; LOG(("TMS34010 unknown\n"));
				}
				break;
#endif
/*#if (HAS_TMS9900)
			case CPU_TMS9900:	irq_line = 0; LOG(("TMS9900 IRQ\n")); break;
#endif*/
#if (HAS_TMS9900) || (HAS_TMS9940) || (HAS_TMS9980) || (HAS_TMS9985) \
	|| (HAS_TMS9989) || (HAS_TMS9995) || (HAS_TMS99105A) || (HAS_TMS99110A)
	#if (HAS_TMS9900)
			case CPU_TMS9900:
	#endif
	#if (HAS_TMS9940)
			case CPU_TMS9940:
	#endif
	#if (HAS_TMS9980)
			case CPU_TMS9980:
	#endif
	#if (HAS_TMS9985)
			case CPU_TMS9985:
	#endif
	#if (HAS_TMS9989)
			case CPU_TMS9989:
	#endif
	#if (HAS_TMS9995)
			case CPU_TMS9995:
	#endif
	#if (HAS_TMS99105A)
			case CPU_TMS99105A:
	#endif
	#if (HAS_TMS99110A)
			case CPU_TMS99110A:
	#endif
				LOG(("Please use the new interrupt scheme for your new developments !\n"));
				irq_line = 0;
				break;
#endif
#if (HAS_Z8000)
			case CPU_Z8000:
				switch (num)
				{
				case Z8000_NVI: 		irq_line = 0; LOG(("Z8000 NVI\n")); break;
				case Z8000_VI:			irq_line = 1; LOG(("Z8000 VI\n")); break;
				default:				irq_line = 0; LOG(("Z8000 unknown\n"));
				}
				break;
#endif
#if (HAS_TMS320C10)
			case CPU_TMS320C10:
				switch (num)
				{
				case TMS320C10_ACTIVE_INT:	irq_line = 0; LOG(("TMS32010 INT\n")); break;
				case TMS320C10_ACTIVE_BIO:	irq_line = 1; LOG(("TMS32010 BIO\n")); break;
				default:					irq_line = 0; LOG(("TMS32010 unknown\n"));
				}
				break;
#endif
#if (HAS_ADSP2100)
			case CPU_ADSP2100:
				switch (num)
				{
				case ADSP2100_IRQ0: 		irq_line = 0; LOG(("ADSP2100 IRQ0\n")); break;
				case ADSP2100_IRQ1: 		irq_line = 1; LOG(("ADSP2100 IRQ1\n")); break;
				case ADSP2100_IRQ2: 		irq_line = 2; LOG(("ADSP2100 IRQ1\n")); break;
				case ADSP2100_IRQ3: 		irq_line = 3; LOG(("ADSP2100 IRQ1\n")); break;
				default:					irq_line = 0; LOG(("ADSP2100 unknown\n"));
				}
				break;
#endif
#if (HAS_PSXCPU)
			case CPU_PSXCPU:
				switch (num)
				{
				case MIPS_IRQ0: 		irq_line = 0; LOG(("MIPS IRQ0\n")); break;
				case MIPS_IRQ1: 		irq_line = 1; LOG(("MIPS IRQ1\n")); break;
				case MIPS_IRQ2: 		irq_line = 2; LOG(("MIPS IRQ2\n")); break;
				case MIPS_IRQ3: 		irq_line = 3; LOG(("MIPS IRQ3\n")); break;
				case MIPS_IRQ4: 		irq_line = 4; LOG(("MIPS IRQ4\n")); break;
				case MIPS_IRQ5: 		irq_line = 5; LOG(("MIPS IRQ5\n")); break;
				default:				irq_line = 0; LOG(("MIPS unknown\n"));
				}
				break;
#endif
			default:
				irq_line = 0;
				/* else it should be an IRQ type; assume line 0 and store vector */
				LOG(("unknown IRQ\n"));
			}
			cpu_irq_line_vector_w(cpunum, irq_line, num);
			cpu_manualirqcallback(irq_line | (cpunum << 3) | (HOLD_LINE << 6) );
		}
	}

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);

	/* trigger already generated by cpu_manualirqcallback or cpu_manualnmicallback */
}

static void cpu_clear_interrupts(int cpunum)
{
	int oldactive = activecpu;
	int i;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	/* clear NMI line */
	SETNMILINE(activecpu,CLEAR_LINE);

	/* clear all IRQ lines */
	for (i = 0; i < cpu[activecpu].intf->num_irqs; i++)
		SETIRQLINE(activecpu,i,CLEAR_LINE);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);
}


static void cpu_reset_cpu(int cpunum)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	/* reset the CPU */
	RESET(cpunum);

	/* Set the irq callback for the cpu */
	SETIRQCALLBACK(cpunum,cpu_irq_callbacks[cpunum]);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memory_set_context(activecpu);
}

/***************************************************************************

  Interrupt callback. This is called once per CPU interrupt by either the
  VBLANK handler or by the CPU's own timer directly, depending on whether
  or not the CPU's interrupts are synced to VBLANK.

***************************************************************************/
static void cpu_vblankintcallback(int param)
{
	if (Machine->drv->cpu[param].vblank_interrupt)
		cpu_generate_interrupt(param, Machine->drv->cpu[param].vblank_interrupt, 0);

	/* update the counters */
	cpu[param].iloops--;
}


static void cpu_timedintcallback(int param)
{
	/* bail if there is no routine */
	if (!Machine->drv->cpu[param].timed_interrupt)
		return;

	/* generate the interrupt */
	cpu_generate_interrupt(param, Machine->drv->cpu[param].timed_interrupt, 0);
}


static void cpu_manualintcallback(int param)
{
	int intnum = param >> 3;
	int cpunum = param & 7;

	/* generate the interrupt */
	cpu_generate_interrupt(cpunum, 0, intnum);
}


static void cpu_clearintcallback(int param)
{
	/* clear the interrupts */
	cpu_clear_interrupts(param);
}


static void cpu_resetcallback(int param)
{
	int state = param >> 3;
	int cpunum = param & 7;

	/* reset the CPU */
	if (state == PULSE_LINE)
		cpu_reset_cpu(cpunum);
	else if (state == ASSERT_LINE)
	{
/* ASG - do we need this?		cpu_reset_cpu(cpunum);*/
		timer_suspendcpu(cpunum, 1, SUSPEND_REASON_RESET);	/* halt cpu */
	}
	else if (state == CLEAR_LINE)
	{
		if (timer_iscpususpended(cpunum, SUSPEND_REASON_RESET))
			cpu_reset_cpu(cpunum);
		timer_suspendcpu(cpunum, 0, SUSPEND_REASON_RESET);	/* restart cpu */
	}
}


static void cpu_haltcallback(int param)
{
	int state = param >> 3;
	int cpunum = param & 7;

	/* reset the CPU */
	if (state == ASSERT_LINE)
		timer_suspendcpu(cpunum, 1, SUSPEND_REASON_HALT);	/* halt cpu */
	else if (state == CLEAR_LINE)
		timer_suspendcpu(cpunum, 0, SUSPEND_REASON_HALT);	/* restart cpu */
}



/***************************************************************************

  VBLANK reset. Called at the start of emulation and once per VBLANK in
  order to update the input ports and reset the interrupt counter.

***************************************************************************/
static void cpu_vblankreset(void)
{
	int i;

	/* read hi scores from disk */
	hs_update();

	/* read keyboard & update the status of the input ports */
	update_input_ports();

	/* reset the cycle counters */
	for (i = 0; i < totalcpu; i++)
	{
		if (!timer_iscpususpended(i, SUSPEND_ANY_REASON))
			cpu[i].iloops = Machine->drv->cpu[i].vblank_interrupts_per_frame - 1;
		else
			cpu[i].iloops = -1;
	}
}


/***************************************************************************

  VBLANK callback. This is called 'vblank_multipler' times per frame to
  service VBLANK-synced interrupts and to begin the screen update process.

***************************************************************************/
static void cpu_firstvblankcallback(int param)
{
	/* now that we're synced up, pulse from here on out */
	vblank_timer = timer_pulse(vblank_period, param, cpu_vblankcallback);

	/* but we need to call the standard routine as well */
	cpu_vblankcallback(param);
}

/* note that calling this with param == -1 means count everything, but call no subroutines */
static void cpu_vblankcallback(int param)
{
	int i;

	/* loop over CPUs */
	for (i = 0; i < totalcpu; i++)
	{
		/* if the interrupt multiplier is valid */
		if (cpu[i].vblankint_multiplier != -1)
		{
			/* decrement; if we hit zero, generate the interrupt and reset the countdown */
			if (!--cpu[i].vblankint_countdown)
			{
				if (param != -1)
					cpu_vblankintcallback(i);
				cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier;
				timer_reset(cpu[i].vblankint_timer, TIME_NEVER);
			}
		}

		/* else reset the VBLANK timer if this is going to be a real VBLANK */
		else if (vblank_countdown == 1)
			timer_reset(cpu[i].vblankint_timer, TIME_NEVER);
	}

	/* is it a real VBLANK? */
	if (!--vblank_countdown)
	{

		/* do we update the screen now? */
		if (!(Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK))
			usres = updatescreen();

		/* Set the timer to update the screen */
		timer_set(TIME_IN_USEC(Machine->drv->vblank_duration), 0, cpu_updatecallback);
		vblank = 1;

		/* reset the globals */
		cpu_vblankreset();

		/* reset the counter */
		vblank_countdown = vblank_multiplier;
	}
}


/***************************************************************************

  Video update callback. This is called a game-dependent amount of time
  after the VBLANK in order to trigger a video update.

***************************************************************************/
static void cpu_updatecallback(int param)
{
	/* update the screen if we didn't before */
	if (Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK)
		usres = updatescreen();
	vblank = 0;

	/* update IPT_VBLANK input ports */
	inputport_vblank_end();

	/* check the watchdog */
	if (watchdog_counter > 0)
	{
		if (--watchdog_counter == 0)
		{
logerror("reset caused by the watchdog\n");
			machine_reset();
		}
	}

	current_frame++;

	/* reset the refresh timer */
	timer_reset(refresh_timer, TIME_NEVER);
}


/***************************************************************************

  Converts an integral timing rate into a period. Rates can be specified
  as follows:

		rate > 0	   -> 'rate' cycles per frame
		rate == 0	   -> 0
		rate >= -10000 -> 'rate' cycles per second
		rate < -10000  -> 'rate' nanoseconds

***************************************************************************/
static double cpu_computerate(int value)
{
	/* values equal to zero are zero */
	if (value <= 0)
		return 0.0;

	/* values above between 0 and 50000 are in Hz */
	if (value < 50000)
		return TIME_IN_HZ(value);

	/* values greater than 50000 are in nanoseconds */
	else
		return TIME_IN_NSEC(value);
}


static void cpu_timeslicecallback(int param)
{
	timer_trigger(TRIGGER_TIMESLICE);
}


/***************************************************************************

  Initializes all the timers used by the CPU system.

***************************************************************************/
static void cpu_inittimers(void)
{
	double first_time;
	int i, max, ipf;

	/* remove old timers */
	if (timeslice_timer)
		timer_remove(timeslice_timer);
	if (refresh_timer)
		timer_remove(refresh_timer);
	if (vblank_timer)
		timer_remove(vblank_timer);

	/* allocate a dummy timer at the minimum frequency to break things up */
	ipf = Machine->drv->cpu_slices_per_frame;
	if (ipf <= 0)
		ipf = 1;
	timeslice_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
	timeslice_timer = timer_pulse(timeslice_period, 0, cpu_timeslicecallback);

	/* allocate an infinite timer to track elapsed time since the last refresh */
	refresh_period = TIME_IN_HZ(Machine->drv->frames_per_second);
	refresh_period_inv = 1.0 / refresh_period;
	refresh_timer = timer_set(TIME_NEVER, 0, NULL);

	/* while we're at it, compute the scanline times */
	if (Machine->drv->vblank_duration)
		scanline_period = (refresh_period - TIME_IN_USEC(Machine->drv->vblank_duration)) /
				(double)(Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
	else
		scanline_period = refresh_period / (double)Machine->drv->screen_height;
	scanline_period_inv = 1.0 / scanline_period;

	/*
	 *		The following code finds all the CPUs that are interrupting in sync with the VBLANK
	 *		and sets up the VBLANK timer to run at the minimum number of cycles per frame in
	 *		order to service all the synced interrupts
	 */

	/* find the CPU with the maximum interrupts per frame */
	max = 1;
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
		if (ipf > max)
			max = ipf;
	}

	/* now find the LCD with the rest of the CPUs (brute force - these numbers aren't huge) */
	vblank_multiplier = max;
	while (1)
	{
		for (i = 0; i < totalcpu; i++)
		{
			ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
			if (ipf > 0 && (vblank_multiplier % ipf) != 0)
				break;
		}
		if (i == totalcpu)
			break;
		vblank_multiplier += max;
	}

	/* initialize the countdown timers and intervals */
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
		if (ipf > 0)
			cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier = vblank_multiplier / ipf;
		else
			cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier = -1;
	}

	/* allocate a vblank timer at the frame rate * the LCD number of interrupts per frame */
	vblank_period = TIME_IN_HZ(Machine->drv->frames_per_second * vblank_multiplier);
	vblank_timer = timer_pulse(vblank_period, 0, cpu_vblankcallback);
	vblank_countdown = vblank_multiplier;

	/*
	 *		The following code creates individual timers for each CPU whose interrupts are not
	 *		synced to the VBLANK, and computes the typical number of cycles per interrupt
	 */

	/* start the CPU interrupt timers */
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;

		/* remove old timers */
		if (cpu[i].vblankint_timer)
			timer_remove(cpu[i].vblankint_timer);
		if (cpu[i].timedint_timer)
			timer_remove(cpu[i].timedint_timer);

		/* compute the average number of cycles per interrupt */
		if (ipf <= 0)
			ipf = 1;
		cpu[i].vblankint_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
		cpu[i].vblankint_timer = timer_set(TIME_NEVER, 0, NULL);

		/* see if we need to allocate a CPU timer */
		ipf = Machine->drv->cpu[i].timed_interrupts_per_second;
		if (ipf)
		{
			cpu[i].timedint_period = cpu_computerate(ipf);
			cpu[i].timedint_timer = timer_pulse(cpu[i].timedint_period, i, cpu_timedintcallback);
		}
	}

	/* note that since we start the first frame on the refresh, we can't pulse starting
	   immediately; instead, we back up one VBLANK period, and inch forward until we hit
	   positive time. That time will be the time of the first VBLANK timer callback */
	timer_remove(vblank_timer);

	first_time = -TIME_IN_USEC(Machine->drv->vblank_duration) + vblank_period;
	while (first_time < 0)
	{
		cpu_vblankcallback(-1);
		first_time += vblank_period;
	}
	vblank_timer = timer_set(first_time, 0, cpu_firstvblankcallback);
}


/* AJP 981016 */
int cpu_is_saving_context(int _activecpu)
{
	return (cpu[_activecpu].save_context);
}


/* JB 971019 */
void* cpu_getcontext(int _activecpu)
{
	return cpu[_activecpu].context;
}


/***************************************************************************
  Retrieve or set the entire context of the active CPU
***************************************************************************/

unsigned cpu_get_context(void *context)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETCONTEXT(cpunum,context);
}

void cpu_set_context(void *context)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETCONTEXT(cpunum,context);
}

/***************************************************************************
  Retrieve or set a cycle counts lookup table for the active CPU
***************************************************************************/

void *cpu_get_cycle_table(int which)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETCYCLETBL(cpunum,which);
}

void cpu_set_cycle_tbl(int which, void *new_table)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETCYCLETBL(cpunum,which,new_table);
}

/***************************************************************************
  Retrieve or set the value of a specific register of the active CPU
***************************************************************************/

unsigned cpu_get_reg(int regnum)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETREG(cpunum,regnum);
}

void cpu_set_reg(int regnum, unsigned val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETREG(cpunum,regnum,val);
}

/***************************************************************************

  Get various CPU information

***************************************************************************/

/***************************************************************************
  Returns the number of address bits for the active CPU
***************************************************************************/
unsigned cpu_address_bits(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].address_bits;
}

/***************************************************************************
  Returns the address bit mask for the active CPU
***************************************************************************/
unsigned cpu_address_mask(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return 0xffffffffUL >> (32 - cpuintf[CPU_TYPE(cpunum)].address_bits);
}

/***************************************************************************
  Returns the address shift factor for the active CPU
***************************************************************************/
int cpu_address_shift(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].address_shift;
}

/***************************************************************************
  Returns the endianess for the active CPU
***************************************************************************/
unsigned cpu_endianess(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].endianess;
}

/***************************************************************************
  Returns the code align unit for the active CPU (1 byte, 2 word, ...)
***************************************************************************/
unsigned cpu_align_unit(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].align_unit;
}

/***************************************************************************
  Returns the max. instruction length for the active CPU
***************************************************************************/
unsigned cpu_max_inst_len(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].max_inst_len;
}

/***************************************************************************
  Returns the name for the active CPU
***************************************************************************/
const char *cpu_name(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Returns the family name for the active CPU
***************************************************************************/
const char *cpu_core_family(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FAMILY);
	return "";
}

/***************************************************************************
  Returns the version number for the active CPU
***************************************************************************/
const char *cpu_core_version(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_VERSION);
	return "";
}

/***************************************************************************
  Returns the core filename for the active CPU
***************************************************************************/
const char *cpu_core_file(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FILE);
	return "";
}

/***************************************************************************
  Returns the credits for the active CPU
***************************************************************************/
const char *cpu_core_credits(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_CREDITS);
	return "";
}

/***************************************************************************
  Returns the register layout for the active CPU (debugger)
***************************************************************************/
const char *cpu_reg_layout(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_REG_LAYOUT);
	return "";
}

/***************************************************************************
  Returns the window layout for the active CPU (debugger)
***************************************************************************/
const char *cpu_win_layout(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_WIN_LAYOUT);
	return "";
}

/***************************************************************************
  Returns a dissassembled instruction for the active CPU
***************************************************************************/
unsigned cpu_dasm(char *buffer, unsigned pc)
{
	if( activecpu >= 0 )
		return CPUDASM(activecpu,buffer,pc);
	return 0;
}

/***************************************************************************
  Returns a flags (state, condition codes) string for the active CPU
***************************************************************************/
const char *cpu_flags(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FLAGS);
	return "";
}

/***************************************************************************
  Returns a specific register string for the currently active CPU
***************************************************************************/
const char *cpu_dump_reg(int regnum)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_REG+regnum);
	return "";
}

/***************************************************************************
  Returns a state dump for the currently active CPU
***************************************************************************/
const char *cpu_dump_state(void)
{
	static char buffer[1024+1];
	unsigned addr_width = (cpu_address_bits() + 3) / 4;
	char *dst = buffer;
	const char *src;
	const INT8 *regs;
	int width;

	dst += sprintf(dst, "CPU #%d [%s]\n", activecpu, cputype_name(CPU_TYPE(activecpu)));
	width = 0;
	regs = (INT8 *)cpu_reg_layout();
	while( *regs )
	{
		if( *regs == -1 )
		{
			dst += sprintf(dst, "\n");
			width = 0;
		}
		else
		{
			src = cpu_dump_reg( *regs );
			if( *src )
			{
				if( width + strlen(src) + 1 >= 80 )
				{
					dst += sprintf(dst, "\n");
					width = 0;
				}
				dst += sprintf(dst, "%s ", src);
				width += strlen(src) + 1;
			}
		}
		regs++;
	}
	dst += sprintf(dst, "\n%0*X: ", addr_width, cpu_get_pc());
	cpu_dasm( dst, cpu_get_pc() );
	strcat(dst, "\n\n");

	return buffer;
}

/***************************************************************************
  Returns the number of address bits for a specific CPU type
***************************************************************************/
unsigned cputype_address_bits(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].address_bits;
	return 0;
}

/***************************************************************************
  Returns the address bit mask for a specific CPU type
***************************************************************************/
unsigned cputype_address_mask(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return 0xffffffffUL >> (32 - cpuintf[cpu_type].address_bits);
	return 0;
}

/***************************************************************************
  Returns the address shift factor for a specific CPU type
***************************************************************************/
int cputype_address_shift(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].address_shift;
	return 0;
}

/***************************************************************************
  Returns the endianess for a specific CPU type
***************************************************************************/
unsigned cputype_endianess(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].endianess;
	return 0;
}

/***************************************************************************
  Returns the data bus width for a specific CPU type
***************************************************************************/
unsigned cputype_databus_width(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].databus_width;
	return 0;
}

/***************************************************************************
  Returns the code align unit for a speciific CPU type (1 byte, 2 word, ...)
***************************************************************************/
unsigned cputype_align_unit(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].align_unit;
	return 0;
}

/***************************************************************************
  Returns the max. instruction length for a specific CPU type
***************************************************************************/
unsigned cputype_max_inst_len(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].max_inst_len;
	return 0;
}

/***************************************************************************
  Returns the name for a specific CPU type
***************************************************************************/
const char *cputype_name(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Returns the family name for a specific CPU type
***************************************************************************/
const char *cputype_core_family(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FAMILY);
	return "";
}

/***************************************************************************
  Returns the version number for a specific CPU type
***************************************************************************/
const char *cputype_core_version(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_VERSION);
	return "";
}

/***************************************************************************
  Returns the core filename for a specific CPU type
***************************************************************************/
const char *cputype_core_file(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FILE);
	return "";
}

/***************************************************************************
  Returns the credits for a specific CPU type
***************************************************************************/
const char *cputype_core_credits(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_CREDITS);
	return "";
}

/***************************************************************************
  Returns the register layout for a specific CPU type (debugger)
***************************************************************************/
const char *cputype_reg_layout(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_REG_LAYOUT);
	return "";
}

/***************************************************************************
  Returns the window layout for a specific CPU type (debugger)
***************************************************************************/
const char *cputype_win_layout(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_WIN_LAYOUT);

	/* just in case... */
	return (const char *)default_win_layout;
}

/***************************************************************************
  Returns the number of address bits for a specific CPU number
***************************************************************************/
unsigned cpunum_address_bits(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_address_bits(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the address bit mask for a specific CPU number
***************************************************************************/
unsigned cpunum_address_mask(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_address_mask(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the endianess for a specific CPU number
***************************************************************************/
unsigned cpunum_endianess(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_endianess(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the data bus width for a specific CPU number
***************************************************************************/
unsigned cpunum_databus_width(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_databus_width(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the code align unit for the active CPU (1 byte, 2 word, ...)
***************************************************************************/
unsigned cpunum_align_unit(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_align_unit(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the max. instruction length for a specific CPU number
***************************************************************************/
unsigned cpunum_max_inst_len(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_max_inst_len(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the name for a specific CPU number
***************************************************************************/
const char *cpunum_name(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_name(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns the family name for a specific CPU number
***************************************************************************/
const char *cpunum_core_family(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_core_family(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns the core version for a specific CPU number
***************************************************************************/
const char *cpunum_core_version(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_core_version(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns the core filename for a specific CPU number
***************************************************************************/
const char *cpunum_core_file(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_core_file(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns the credits for a specific CPU number
***************************************************************************/
const char *cpunum_core_credits(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_core_credits(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns (debugger) register layout for a specific CPU number
***************************************************************************/
const char *cpunum_reg_layout(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_reg_layout(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns (debugger) window layout for a specific CPU number
***************************************************************************/
const char *cpunum_win_layout(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_win_layout(CPU_TYPE(cpunum));
	return (const char *)default_win_layout;
}

/***************************************************************************
  Return a register value for a specific CPU number of the running machine
***************************************************************************/
unsigned cpunum_get_reg(int cpunum, int regnum)
{
	int oldactive;
	unsigned val = 0;

	if( cpunum == activecpu )
		return cpu_get_reg( regnum );

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	val = GETREG(activecpu,regnum);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}

	return val;
}

/***************************************************************************
  Set a register value for a specific CPU number of the running machine
***************************************************************************/
void cpunum_set_reg(int cpunum, int regnum, unsigned val)
{
	int oldactive;

	if( cpunum == activecpu )
	{
		cpu_set_reg( regnum, val );
		return;
	}

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	SETREG(activecpu,regnum,val);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}
}

/***************************************************************************
  Return a dissassembled instruction for a specific CPU
***************************************************************************/
unsigned cpunum_dasm(int cpunum,char *buffer,unsigned pc)
{
	unsigned result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_dasm(buffer,pc);

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	result = CPUDASM(activecpu,buffer,pc);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}

	return result;
}

/***************************************************************************
  Return a flags (state, condition codes) string for a specific CPU
***************************************************************************/
const char *cpunum_flags(int cpunum)
{
	const char *result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_flags();

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	result = CPUINFO(activecpu,NULL,CPU_INFO_FLAGS);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}

	return result;
}

/***************************************************************************
  Return a specific register string for a specific CPU
***************************************************************************/
const char *cpunum_dump_reg(int cpunum, int regnum)
{
	const char *result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_dump_reg(regnum);

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	result = CPUINFO(activecpu,NULL,CPU_INFO_REG+regnum);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}

	return result;
}

/***************************************************************************
  Return a state dump for a specific CPU
***************************************************************************/
const char *cpunum_dump_state(int cpunum)
{
	static char buffer[1024+1];
	int oldactive;

	/* swap to the CPU's context */
	if (activecpu >= 0)
		if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	oldactive = activecpu;
	activecpu = cpunum;
	memory_set_context(activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);

	strcpy( buffer, cpu_dump_state() );

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0)
	{
		memory_set_context(activecpu);
		if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}

	return buffer;
}

/***************************************************************************
  Dump all CPU's state to stdout
***************************************************************************/
void cpu_dump_states(void)
{
	int i;

	for( i = 0; i < totalcpu; i++ )
	{
		puts( cpunum_dump_state(i) );
	}
	fflush(stdout);
}

/***************************************************************************

  Dummy interfaces for non-CPUs

***************************************************************************/
static void Dummy_init(void) { }
static void Dummy_reset(void *param) { }
static void Dummy_exit(void) { }
static int Dummy_execute(int cycles) { return cycles; }
static void Dummy_burn(int cycles) { }
static unsigned Dummy_get_context(void *regs) { return 0; }
static void Dummy_set_context(void *regs) { }
static unsigned Dummy_get_pc(void) { return 0; }
static void Dummy_set_pc(unsigned val) { }
static unsigned Dummy_get_sp(void) { return 0; }
static void Dummy_set_sp(unsigned val) { }
static unsigned Dummy_get_reg(int regnum) { return 0; }
static void Dummy_set_reg(int regnum, unsigned val) { }
static void Dummy_set_nmi_line(int state) { }
static void Dummy_set_irq_line(int irqline, int state) { }
static void Dummy_set_irq_callback(int (*callback)(int irqline)) { }

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
static const char *Dummy_info(void *context, int regnum)
{
	if( !context && regnum )
		return "";

	switch (regnum)
	{
		case CPU_INFO_NAME: return "Dummy";
		case CPU_INFO_FAMILY: return "no CPU";
		case CPU_INFO_VERSION: return "0.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "The MAME team.";
	}
	return "";
}

static unsigned Dummy_dasm(char *buffer, unsigned pc)
{
	strcpy(buffer, "???");
	return 1;
}

#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
void cpu_set_m68k_reset(int cpunum, void (*resetfn)(void))
{
	void m68k_set_reset_instr_callback(void  (*callback)(void));

	if ( 1
#if (HAS_M68000)
		&& CPU_TYPE(cpunum) != CPU_M68000
#endif
#if (HAS_M68010)
		&& CPU_TYPE(cpunum) != CPU_M68010
#endif
#if (HAS_M68020)
		&& CPU_TYPE(cpunum) != CPU_M68020
#endif
#if (HAS_M68EC020)
		&& CPU_TYPE(cpunum) != CPU_M68EC020
#endif
		)
	{
		logerror("Trying to set m68k reset vector on non-68k cpu\n");
		exit(1);
	}

	if (cpunum != activecpu)
	{
		if(activecpu != -1)
			if (cpu[activecpu].save_context) GETCONTEXT(activecpu, cpu[activecpu].context);
		memory_set_context(cpunum);
		if (cpu[cpunum].save_context) SETCONTEXT(cpunum, cpu[cpunum].context);
	}

	m68k_set_reset_instr_callback(resetfn);

	if(cpunum != activecpu)
	{
		memory_set_context(cpunum);
		if (cpu[cpunum].save_context) GETCONTEXT(cpunum, cpu[cpunum].context);
		if(activecpu != -1)
			if (cpu[activecpu].save_context) SETCONTEXT(activecpu, cpu[activecpu].context);
	}
}
#endif
