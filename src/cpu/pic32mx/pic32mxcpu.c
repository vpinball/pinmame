// license:BSD-3-Clause

#include "driver.h"
#include "cpuintrf.h"
#include "pic32mxcpu.h"
#include <stdio.h>
#include <string.h>

int pic32cpu_ICount;

static pic32mx soc;
static pic32mx_board board;
static int have_board;

static void load_flash(void)
{
	pic32mx_init(&soc, have_board ? &board : NULL, memory_region(REGION_CPU1), memory_region_length(REGION_CPU1));
}

void pic32cpu_set_board(const pic32mx_board *b)
{
	board = *b;
	have_board = 1;
	soc.board = board;
}

pic32mx *pic32cpu_soc(void) { return &soc; }

void pic32cpu_init(void) { load_flash(); }
void pic32cpu_reset(void *param) { (void)param; load_flash(); }
void pic32cpu_exit(void) { have_board = 0; }

int pic32cpu_execute(int cycles)
{
	pic32cpu_ICount = cycles;
	if (!soc.flash) return cycles;
	soc.icount = &pic32cpu_ICount;
	pic32mx_run(&soc, cycles);
	return cycles - pic32cpu_ICount;
}

unsigned pic32cpu_get_context(void *dst) { (void)dst; return sizeof(void *); }
void pic32cpu_set_context(void *src) { (void)src; }

unsigned pic32cpu_get_reg(int regnum)
{
	mips32_settle(&soc.cpu);
	switch (regnum) {
	case REG_PC: case PIC32CPU_PC: return soc.cpu.pc;
	case REG_PREVIOUSPC: return soc.cpu.cur_pc;
	case REG_SP: return mips32_regs(&soc.cpu)[29];
	case PIC32CPU_HI: return soc.cpu.hi;
	case PIC32CPU_LO: return soc.cpu.lo;
	case PIC32CPU_STATUS: return soc.cpu.status;
	case PIC32CPU_CAUSE: return soc.cpu.cause;
	case PIC32CPU_EPC: return soc.cpu.epc;
	}
	if (regnum >= PIC32CPU_R0 && regnum <= PIC32CPU_R31) return mips32_regs(&soc.cpu)[regnum - PIC32CPU_R0];
	return 0;
}

void pic32cpu_set_reg(int regnum, unsigned val)
{
	mips32_settle(&soc.cpu);
	switch (regnum) {
	case REG_PC: case PIC32CPU_PC: soc.cpu.pc = val; soc.cpu.npc = val + 4; soc.cpu.delay = 0; return;
	case REG_SP: mips32_regs(&soc.cpu)[29] = val; return;
	case PIC32CPU_HI: soc.cpu.hi = val; return;
	case PIC32CPU_LO: soc.cpu.lo = val; return;
	}
	if (regnum > PIC32CPU_R0 && regnum <= PIC32CPU_R31) mips32_regs(&soc.cpu)[regnum - PIC32CPU_R0] = val;
}

void pic32cpu_set_irq_line(int irqline, int state) { (void)irqline; (void)state; }
void pic32cpu_set_irq_callback(int (*callback)(int irqline)) { (void)callback; }

const char *pic32cpu_info(void *context, int regnum)
{
	static char buf[32];
	(void)context;
	if (regnum < CPU_INFO_FLAGS) mips32_settle(&soc.cpu);
	switch (regnum) {
	case CPU_INFO_NAME: return "PIC32MX";
	case CPU_INFO_FAMILY: return "MIPS32 M4K";
	case CPU_INFO_VERSION: return "1.0";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright Gerwout van der Veen";
	case CPU_INFO_REG + PIC32CPU_PC: sprintf(buf, "PC:%08X", soc.cpu.pc); return buf;
	}
	if (regnum >= CPU_INFO_REG + PIC32CPU_R0 && regnum <= CPU_INFO_REG + PIC32CPU_R31) {
		int r = regnum - CPU_INFO_REG - PIC32CPU_R0;
		sprintf(buf, "R%d:%08X", r, mips32_regs(&soc.cpu)[r]);
		return buf;
	}
	return "";
}

unsigned pic32cpu_dasm(char *buffer, unsigned pc)
{
	uint32_t pa;
	int err = 0;
	uint32_t op;
	if (!mips32_translate(&soc.cpu, pc, &pa)) { sprintf(buffer, "???"); return 4; }
	op = soc.cpu.bus.read(soc.cpu.bus.ctx, pa, 4, 1, &err);
	if (err) { sprintf(buffer, "???"); return 4; }
	return mips32_dasm(buffer, pc, op);
}
