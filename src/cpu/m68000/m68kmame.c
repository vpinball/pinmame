#include <stdio.h>
#include <stdlib.h>
#include "m68k.h"
#include "m68000.h"
#include "state.h"
#include "driver.h"	//needed for cpu_boost

/* global access */

struct m68k_memory_interface m68k_memory_intf;

#ifndef A68K0

/****************************************************************************
 * 24-bit address, 16-bit data memory interface
 ****************************************************************************/

static data32_t readlong_a24_d16(offs_t address)
{
	data32_t result = cpu_readmem24bew_word(address) << 16;
	return result | cpu_readmem24bew_word(address + 2);
}

static void writelong_a24_d16(offs_t address, data32_t data)
{
	cpu_writemem24bew_word(address, data >> 16);
	cpu_writemem24bew_word(address + 2, data);
}

static void changepc_a24_d16(offs_t pc)
{
	change_pc24bew(pc);
}

/* interface for 24-bit address bus, 16-bit data bus (68000, 68010) */
static const struct m68k_memory_interface interface_a24_d16 =
{
	0,
	cpu_readmem24bew,
	cpu_readmem24bew_word,
	readlong_a24_d16,
	cpu_writemem24bew,
	cpu_writemem24bew_word,
	writelong_a24_d16,
	changepc_a24_d16
};

#endif // A68K0

/****************************************************************************
 * 24-bit address, 32-bit data memory interface
 ****************************************************************************/

#ifndef A68K2

/* potentially misaligned 16-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data16_t readword_a24_d32(offs_t address)
{
	data16_t result;

	if (!(address & 1))
		return cpu_readmem24bedw_word(address);
	result = cpu_readmem24bedw(address) << 8;
	return result | cpu_readmem24bedw(address + 1);
}

/* potentially misaligned 16-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writeword_a24_d32(offs_t address, data16_t data)
{
	if (!(address & 1))
	{
		cpu_writemem24bedw_word(address, data);
		return;
	}
	cpu_writemem24bedw(address, data >> 8);
	cpu_writemem24bedw(address + 1, data);
}

/* potentially misaligned 32-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data32_t readlong_a24_d32(offs_t address)
{
	data32_t result;

	if (!(address & 3))
		return cpu_readmem24bedw_dword(address);
	else if (!(address & 1))
	{
		result = cpu_readmem24bedw_word(address) << 16;
		return result | cpu_readmem24bedw_word(address + 2);
	}
	result = cpu_readmem24bedw(address) << 24;
	result |= cpu_readmem24bedw_word(address + 1) << 8;
	return result | cpu_readmem24bedw(address + 3);
}

/* potentially misaligned 32-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writelong_a24_d32(offs_t address, data32_t data)
{
	if (!(address & 3))
	{
		cpu_writemem24bedw_dword(address, data);
		return;
	}
	else if (!(address & 1))
	{
		cpu_writemem24bedw_word(address, data >> 16);
		cpu_writemem24bedw_word(address + 2, data);
		return;
	}
	cpu_writemem24bedw(address, data >> 24);
	cpu_writemem24bedw_word(address + 1, data >> 8);
	cpu_writemem24bedw(address + 3, data);
}

static void changepc_a24_d32(offs_t pc)
{
	change_pc24bedw(pc);
}

/* interface for 24-bit address bus, 32-bit data bus (68EC020) */
static const struct m68k_memory_interface interface_a24_d32 =
{
	WORD_XOR_BE(0),
	cpu_readmem24bedw,
	readword_a24_d32,
	readlong_a24_d32,
	cpu_writemem24bedw,
	writeword_a24_d32,
	writelong_a24_d32,
	changepc_a24_d32
};


/****************************************************************************
 * 32-bit address, 32-bit data memory interface
 ****************************************************************************/

/* potentially misaligned 16-bit reads with a 32-bit data bus (and 32-bit address bus) */
static data16_t readword_a32_d32(offs_t address)
{
	data16_t result;

	if (!(address & 1))
		return cpu_readmem32bedw_word(address);
	result = cpu_readmem32bedw(address) << 8;
	return result | cpu_readmem32bedw(address + 1);
}

/* potentially misaligned 16-bit writes with a 32-bit data bus (and 32-bit address bus) */
static void writeword_a32_d32(offs_t address, data16_t data)
{
	if (!(address & 1))
	{
		cpu_writemem32bedw_word(address, data);
		return;
	}
	cpu_writemem32bedw(address, data >> 8);
	cpu_writemem32bedw(address + 1, data);
}

/* potentially misaligned 32-bit reads with a 32-bit data bus (and 32-bit address bus) */
static data32_t readlong_a32_d32(offs_t address)
{
	data32_t result;

	if (!(address & 3))
		return cpu_readmem32bedw_dword(address);
	else if (!(address & 1))
	{
		result = cpu_readmem32bedw_word(address) << 16;
		return result | cpu_readmem32bedw_word(address + 2);
	}
	result = cpu_readmem32bedw(address) << 24;
	result |= cpu_readmem32bedw_word(address + 1) << 8;
	return result | cpu_readmem32bedw(address + 3);
}

/* potentially misaligned 32-bit writes with a 32-bit data bus (and 32-bit address bus) */
static void writelong_a32_d32(offs_t address, data32_t data)
{
	if (!(address & 3))
	{
		cpu_writemem32bedw_dword(address, data);
		return;
	}
	else if (!(address & 1))
	{
		cpu_writemem32bedw_word(address,     data >> 16);
		cpu_writemem32bedw_word(address + 2, data);
		return;
	}
	cpu_writemem32bedw(address, data >> 24);
	cpu_writemem32bedw_word(address + 1, data >> 8);
	cpu_writemem32bedw(address + 3, data);
}

static void changepc_a32_d32(offs_t pc)
{
	change_pc32bedw(pc);
}

/* interface for 24-bit address bus, 32-bit data bus (68020) */
static const struct m68k_memory_interface interface_a32_d32 =
{
	WORD_XOR_BE(0),
	cpu_readmem32bedw,
	readword_a32_d32,
	readlong_a32_d32,
	cpu_writemem32bedw,
	writeword_a32_d32,
	writelong_a32_d32,
	changepc_a32_d32
};

/* global access */
struct m68k_memory_interface m68k_memory_intf;

#endif // A68K2

/****************************************************************************
 * 68000 section
 ****************************************************************************/

#ifndef A68K0

static UINT8 m68000_reg_layout[] = {
	M68K_PC, M68K_ISP, -1,
	M68K_SR, M68K_USP, -1,
	M68K_D0, M68K_A0, -1,
	M68K_D1, M68K_A1, -1,
	M68K_D2, M68K_A2, -1,
	M68K_D3, M68K_A3, -1,
	M68K_D4, M68K_A4, -1,
	M68K_D5, M68K_A5, -1,
	M68K_D6, M68K_A6, -1,
	M68K_D7, M68K_A7, 0
};

static UINT8 m68000_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

void m68000_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_memory_intf = interface_a24_d16;
	m68k_state_register("m68000");
}

void m68000_reset(void* param)
{
	m68k_pulse_reset();
}

void m68000_exit(void)
{
	/* nothing to do */
}

int m68000_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68000_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68000_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bew)
		m68k_memory_intf = interface_a24_d16;
	m68k_set_context(src);
}

unsigned m68000_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68000_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68000_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}


const char *m68000_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? '?':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? '?':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68000_win_layout;
	}
	return buffer[which];
}

unsigned m68000_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble( buffer, pc, M68K_CPU_TYPE_68000 );
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}


/****************************************************************************
 * M68010 section
 ****************************************************************************/
#if HAS_M68010

static UINT8 m68010_reg_layout[] = {
	M68K_PC,  M68K_ISP, -1,
	M68K_SR,  M68K_USP, -1,
	M68K_SFC, M68K_VBR, -1,
	M68K_DFC, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68010_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68010_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68010);
	m68k_memory_intf = interface_a24_d16;
	m68k_state_register("m68010");
}

void m68010_reset(void* param)
{
	m68k_pulse_reset();
}

void m68010_exit(void)
{
	/* nothing to do */
}

int m68010_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68010_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68010_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bew)
		m68k_memory_intf = interface_a24_d16;
	m68k_set_context(src);
}

unsigned m68010_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68010_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68010_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68010_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}


const char *m68010_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? '?':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? '?':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68010";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68010_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68010_win_layout;
	}
	return buffer[which];
}

unsigned m68010_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68010);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}

#endif /* HAS_M68010 */

#endif // A68K0

/****************************************************************************
 * M680EC20 section
 ****************************************************************************/

#ifndef A68K2

#if HAS_M68EC020

static UINT8 m68ec020_reg_layout[] = {
	M68K_PC,  M68K_MSP, -1,
	M68K_SR,  M68K_ISP, -1,
	M68K_SFC, M68K_USP, -1,
	M68K_DFC, M68K_VBR, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68ec020_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68ec020_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68EC020);
	m68k_memory_intf = interface_a24_d32;
	m68k_state_register("m68ec020");
}

void m68ec020_reset(void* param)
{
	m68k_pulse_reset();
}

void m68ec020_exit(void)
{
	/* nothing to do */
}

int m68ec020_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68ec020_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68ec020_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bedw)
		m68k_memory_intf = interface_a24_d32;
	m68k_set_context(src);
}

unsigned m68ec020_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_MSP: return m68k_get_reg(NULL, M68K_REG_MSP); /* 68020+ */
		case M68K_CACR:  return m68k_get_reg(NULL, M68K_REG_CACR); /* 68020+ */
		case M68K_CAAR:  return m68k_get_reg(NULL, M68K_REG_CAAR); /* 68020+ */
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68ec020_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_MSP:  m68k_set_reg(M68K_REG_MSP, val); break; /* 68020+ */
		case M68K_CACR: m68k_set_reg(M68K_REG_CACR, val); break; /* 68020+ */
		case M68K_CAAR: m68k_set_reg(M68K_REG_CAAR, val); break; /* 68020+ */
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68ec020_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68ec020_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}

const char *m68ec020_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_MSP:  sprintf(buffer[which], "MSP:%08X", m68k_get_reg(context, M68K_REG_MSP)); break;
		case CPU_INFO_REG+M68K_CACR: sprintf(buffer[which], "CCR:%08X", m68k_get_reg(context, M68K_REG_CACR)); break;
		case CPU_INFO_REG+M68K_CAAR: sprintf(buffer[which], "CAR:%08X", m68k_get_reg(context, M68K_REG_CAAR)); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? 't':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? 'M':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68EC020";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68ec020_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68ec020_win_layout;
	}
	return buffer[which];
}

unsigned m68ec020_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68EC020);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}
#endif /* HAS_M68EC020 */

/****************************************************************************
 * M68020 section
 ****************************************************************************/
#if HAS_M68020

static UINT8 m68020_reg_layout[] = {
	M68K_PC,  M68K_MSP, -1,
	M68K_SR,  M68K_ISP, -1,
	M68K_SFC, M68K_USP, -1,
	M68K_DFC, M68K_VBR, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68020_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68020_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68020);
	m68k_memory_intf = interface_a32_d32;
	m68k_state_register("m68020");
}

void m68020_reset(void* param)
{
	m68k_pulse_reset();
}

void m68020_exit(void)
{
	/* nothing to do */
}

int m68020_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68020_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68020_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem32bedw)
		m68k_memory_intf = interface_a32_d32;
	m68k_set_context(src);
}

unsigned m68020_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_MSP: return m68k_get_reg(NULL, M68K_REG_MSP); /* 68020+ */
		case M68K_CACR:  return m68k_get_reg(NULL, M68K_REG_CACR); /* 68020+ */
		case M68K_CAAR:  return m68k_get_reg(NULL, M68K_REG_CAAR); /* 68020+ */
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68020_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_MSP:  m68k_set_reg(M68K_REG_MSP, val); break; /* 68020+ */
		case M68K_CACR: m68k_set_reg(M68K_REG_CACR, val); break; /* 68020+ */
		case M68K_CAAR: m68k_set_reg(M68K_REG_CAAR, val); break; /* 68020+ */
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68020_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68020_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}

const char *m68020_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_MSP:  sprintf(buffer[which], "MSP:%08X", m68k_get_reg(context, M68K_REG_MSP)); break;
		case CPU_INFO_REG+M68K_CACR: sprintf(buffer[which], "CCR:%08X", m68k_get_reg(context, M68K_REG_CACR)); break;
		case CPU_INFO_REG+M68K_CAAR: sprintf(buffer[which], "CAR:%08X", m68k_get_reg(context, M68K_REG_CAAR)); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? 't':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? 'M':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68020";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68020_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68020_win_layout;
	}
	return buffer[which];
}

unsigned m68020_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68020);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}
#endif /* HAS_M68020 */


#if defined(PINMAME) && (HAS_M68306)
/****************************************************************************
 * 68306 section
 ****************************************************************************/
//NOTE: FOR SPEED REASONS FOR CAPCOM EMULTION - WE DO NOT PROCESS CS/DRAM SIGNALS
// - INSTEAD WE HARDCODE THE MEMORY MAPPINGS IN THE DRIVER (SJE -11/14/03)

//Disable the 68306 Receiver/Transmitter functions for testing individual sections
#define DISABLE_68306_RX		0
#define DISABLE_68306_TX		0
#define DISABLE_68306_TIMER		0

//This line causes Transmitted data to be sent immediately (not accurate, but useful for testing)
#define M68306_TX_SEND_IMMEDIATE	0

//#define VERBOSE
//#define DEBUGGING

#ifdef VERBOSE
//#define LOG(x)	logerror x
#define LOG(x)	printf x
#else
#define LOG(x)
#endif

#ifdef DEBUGGING
#define PRINTF(x) printf x
#else
#define PRINTF(x)
#endif

// Could not find any other way to get the interrupts to work than inlcuding this
#include "m68kcpu.h"
#undef REG_PC

enum {
  irCS0H, irCS0L, irCS1H, irCS1L, irCS2H, irCS2L, irCS3H, irCS3L,
  irCS4H, irCS4L, irCS5H, irCS5L, irCS6H, irCS6L, irCS7H, irCS7L,
  irDRAM0H, irDRAM0L, irDRAM1H, irDRAM1L,
  irPDATA=0x18, irPDIR, irPPIN, irISR=0x1c, irICR, irBUSERR, irSYSTEM
};

static void m68306_intreg_w(offs_t address, data16_t data, int word);
static data16_t m68306_intreg_r(offs_t address, int word);
static void m68306irq(int irqline, int state);
static UINT16 m68306intreg[0x20];
static UINT16 m68306holdirq; // HOLD_LINE handling
static int  (*m68306intack)(int int_line); /* Interrupt Acknowledge */

static struct { offs_t addr, mask; UINT16 rw; } m68306cs[10];

/*Duart stuff*/

//Registers for read only will be even numbered, write only will be odd numbered
enum {  dirDUMR1A,dirDUMR2A,		//0,1 (exception, these are r&w, but there's 2 so it works out)
		dirDUSRA,dirDUCSRA,			//2,3
		dirDUCRA=5,					//4,5 (nothing for 4)
		dirDURBA,dirDUTBA,			//6,7
		dirDUIPCR,dirDUACR,			//8,9
		dirDUISR,dirDUIMR,			//10,11
		dirCNT_MSB,					//12,13 (13 is same as 12)
		dirCNT_LSB=14,				//14,15 (15 is same as 14)
		dirDUMR1B=16,dirDUMR2B,		//16,17 (exception, these are r&w, but there's 2 so it works out)
		dirDUSRB,dirDUCSRB,			//18,19
		dirDUCRB=21,				//20,21 (nothing for 20)
		dirDURBB,dirDUTBB,			//22,23
		dirDUIVR,					//24,25 (25 is same as 24)
		dirDUIP=26,dirDUOPCR,		//26,27
		dirDUOPS,dirDUOPRS			//28,29 (these are probably not needed)
};
static UINT16 m68306duartreg[0x20];							//Duart registers
static int m68306_duart_int=0;								//Is Interrupt triggered by DUART?
static void trigger_duart_int(int which);					//Cause a DUART Int to occur
static void duart_command_register_w(int which,int data);	//Handle writes to command register
static void m68306_duart_set_dusr(int which,int data);		//Set the DUSR register
static void m68306_duart_set_duisr(int data);				//Set the DUISR register
static void m68306_duart_set_duimr(int data);				//Set the DUIMR register
static void m68306_duart_check_int(void);					//Check for a DUART interrupt
static void duart_start_timer(void);						//Duart Start Timer
static void duart_timer_callback (int param);				//Duart Timer Callback
static void m68306_rx_cause_int(int which,int data);		//Generate a Receiver Interrupt
static void m68306_rx_cause_int_channel_a(int data);		//Callback (internal) to fire an Receiver Interrupt
static void m68306_rx_cause_int_channel_b(int data);		//Callback (internal) to fire an Receiver Interrupt
static void m68306_load_transmitter(int which,int data);	//Load Transmitter with data
static void m68306_tx_send_byte(int param);					//Callback (internal) to Transmit a byte

/*-- uart --*/
typedef struct {
	int tx_enable;			//Is channel enabled for tx?
	int tx_sending;			//Is channel sending data?
	int tx_new_data;		//Has new data arrived
	int tx_hold_reg;		//Transmit Holding Register
	int tx_shift_reg;		//Transmit Shift Out Register
	int rx_enable;			//Is channel enabled for rx?
} uart;

/*-- duart --*/
static struct {
	void *timer;			//Pointer to MAME timer
	int timer_output;		//Timer's output bit
	uart channel[2];		//Channels A & B
	int mode_pointer[2];	//Mode Pointer for Channels A & B
} m68306_duart;

//temporary hacks to get direct access to capcoms.c funcs
extern void set_cts_line_to_8752(int data);
extern int get_rts_line_from_8752(void);
extern void send_data_to_8752(int data);

//-------------------------------------------
// Memory read/write functions
//   Check for internal registers
//-------------------------------------------

//WRITE 8BIT
static void m68306_write8(offs_t address, data8_t data) {
  if (address >= 0xfffff000)
		m68306_intreg_w(address, data, 0);
  else
		cpu_writemem32bew(address,data);
}
//WRITE 16BIT
static void m68306_write16(offs_t address, data16_t data) {
	if (address >= 0xfffff000)
		m68306_intreg_w(address, data, 1);
	else
		cpu_writemem32bew_word(address,data);
}
//WRITE 32BIT
static void m68306_write32(offs_t address, data32_t data) {
	if (address >= 0xfffff000) {
		m68306_intreg_w(address,   data>>16, 1);
		m68306_intreg_w(address+2, data,     1);
	}
	else {
		cpu_writemem32bew_word(address, data >> 16);
		cpu_writemem32bew_word(address + 2, data);
	}
}
//READ 8BIT
static data8_t m68306_read8(offs_t address) {
	if (address >= 0xfffff000)
		return (data8_t)m68306_intreg_r(address, 0);
	else
		return cpu_readmem32bew(address);
}
//READ 16BIT
static data16_t m68306_read16(offs_t address) {
	if (address >= 0xfffff000)
		return m68306_intreg_r(address, 1);
	else
		return cpu_readmem32bew_word(address);
}
//READ 32BIT
static data32_t m68306_read32(offs_t address) {
	data32_t result;
	if (address >= 0xfffff000) {
		result = m68306_intreg_r(address, 1)<<16;
		return result | m68306_intreg_r(address+2, 1);
	}
	else {
		result = cpu_readmem32bew_word(address)<<16;
		return result | cpu_readmem32bew_word(address + 2);
	}
}
//-------------------------------------------------------------------
// PC has changed
//-------------------------------------------------------------------
static void m68306_changepc(offs_t pc) {
	change_pc32bew(pc);
}

static const struct m68k_memory_interface interface_m68306 = {
	/*WORD_XOR_BE(0)*/0,
	m68306_read8,
	m68306_read16,
	m68306_read32,
	m68306_write8,
	m68306_write16,
	m68306_write32,
	m68306_changepc
};

//--------------------------------
//  DUART Internal register read
//--------------------------------
static data16_t m68306_duart_reg_r(offs_t address, int word) {
	//odd word checks already handled!

	data16_t data =	m68306duartreg[address-0xf7e1];		//Grab data from registers

	switch(address) {
		//F7E1 - MODE A REGISTERS (DUMR1A & DUMR2A)
		case 0xf7e1:
			//Mode pointer determines whether we return dumr1 or dumr2
			data = m68306duartreg[dirDUMR1A+m68306_duart.mode_pointer[0]];
			if(m68306_duart.mode_pointer[0])
				LOG(("%8x:MODE A REGISTER - DUMR2A Read = %x\n",activecpu_get_pc(),data));
			else
				LOG(("%8x:MODE A REGISTER - DUMR1A Read = %x\n",activecpu_get_pc(),data));
			//After reading, update the mode pointer (note, it can only go to 1)
			m68306_duart.mode_pointer[0]=1;
			break;
		//F7E3 - STATUS REGISTER A (DUSRA)
		case 0xf7e3:
			LOG(("%8x:STATUS REGISTER A (DUSRA) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7E5 - N/A
		case 0xf7e5:
			LOG(("%8x:ILLEGAL DUART REGISTER Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7E7 - RECEIVE BUFFER A(DURBA)
		case 0xf7e7: {
			LOG(("%8x:RECEIVE BUFFER A(DURBA) Read = %x\n",activecpu_get_pc(),data));
			//Clear RxRDY status (bit 0)
			m68306_duart_set_dusr(0,m68306duartreg[dirDUSRA]&(~1));
			break;
		}

		//F7E9 - INPUT PORT CHANGE REG (DUIPCR)
		case 0xf7e9:
			LOG(("%8x:INPUT PORT CHANGE REG (DUIPCR) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7EB - INTERRUPT STATUS(DUISR)
		case 0xf7eb:
			LOG(("%8x:INTERRUPT STATUS(DUISR) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7ED - COUNTERMODE: MSB OF COUNTER
		case 0xf7ed:
			LOG(("%8x:COUNTERMODE: MSB OF COUNTER Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7EF - COUNTERMODE: LSB OF COUNTER
		case 0xf7ef:
			LOG(("%8x:COUNTERMODE: LSB OF COUNTER Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7F1 - MODE B (DUMR1B,DUMR2B)
		case 0xf7f1:
			//Mode pointer determines whether we return dumr1 or dumr2
			data = m68306duartreg[dirDUMR1B+m68306_duart.mode_pointer[1]];
			if(m68306_duart.mode_pointer[1])
				LOG(("%8x:MODE B REGISTER - DUMR2B Read = %x\n",activecpu_get_pc(),data));
			else
				LOG(("%8x:MODE B REGISTER - DUMR1B Read = %x\n",activecpu_get_pc(),data));
			//After reading, update the mode pointer (note, it can only go to 1)
			m68306_duart.mode_pointer[1]=1;
			break;
		//F7F3 - STATUS REGISTER B (DUSRB)
		case 0xf7f3:
			LOG(("%8x:STATUS REGISTER B (DUSRB) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7F5 - N/A
		case 0xf7f5:
			LOG(("%8x:ILLEGAL DUART REGISTER Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7F7 - RECEIVE BUFFER B(DURBB)
		case 0xf7f7: {
			LOG(("%8x:RECEIVE BUFFER B(DURBB) Read = %x\n",activecpu_get_pc(),data));
			PRINTF(("%8x:RECEIVE BUFFER B(DURBB) Read = %x\n",activecpu_get_pc(),data));
			//Clear RxRDY status (bit 0)
			m68306_duart_set_dusr(1,m68306duartreg[dirDUSRB]&(~0x01));
			//Set CTS to 0, so we can receive more data.. - this is a hack to ensure no more data is received before being read!
			set_cts_line_to_8752(0);
			break;
		}
		//F7F9 - INTERRUPT VECTOR(DUIPVR)
		case 0xf7f9:
			LOG(("%8x:INTERRUPT VECTOR(DUIPVR) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7FB - INPUT PORT REGISTER(DUIP)
		case 0xf7fb:
			LOG(("%8x:INPUT PORT REGISTER(DUIP) Read = %x\n",activecpu_get_pc(),data));
			break;
		//F7FD - START COUNTER COMMAND
		case 0xf7fd:
			LOG(("%8x:START COUNTER COMMAND Read = %x\n",activecpu_get_pc(),data));
#if !DISABLE_68306_TIMER
			//TIMER MODE?
			if(m68306duartreg[dirDUACR] & 0x40) {
				//Look for 1->0 transition in the output to trigger an interrupt!
				if(m68306_duart.timer_output) {
					//Set CTR/TMR RDY bit (3) in DUISR
					m68306_duart_set_duisr(m68306duartreg[dirDUISR] | 0x08);
				}
				//Clear OP3 output
				m68306_duart.timer_output = 0;
				//Restart timer!
				duart_start_timer();
			}
			//COUNTER MODE?
			else {
				//Start count down from preloaded value
			}
#endif
			break;
		//F7FF - STOP COUNTER COMMAND
		case 0xf7ff:
			LOG(("%8x:STOP COUNTER COMMAND Read = %x\n",activecpu_get_pc(),data));
#if !DISABLE_68306_TIMER
			//Clear CTR/TMR RDY bit (3) in DUISR - BOTH TIMER & COUNTER MODE
			m68306duartreg[dirDUISR] &= (~0x08);

			//COUNTER MODE? - NOT IMPLEMENTED
			if((m68306duartreg[dirDUACR] & 0x40) == 0) {
				//Clear OP3 output
				m68306_duart.timer_output = 0;
				//Stop the counter..
			}
#endif
			break;

		default:
			LOG(("%8x:M68306UART_r[%04x]= %x\n",activecpu_get_pc(),address,data));
	}
	return (word || (address & 1)) ? data : data>>8;
}

//--------------------------------
//  DUART Internal register write
//--------------------------------
//NOTE: These are really UINT8 values, not UINT16, ie, they are addressable as bytes only, not words!
static void m68306_duart_reg_w(offs_t address, data16_t data, int word) {
	//odd word checks already handled!
	switch(address) {
		//F7E1 - MODE A REGISTERS (DUMR1A & DUMR2A)
		case 0xf7e1:
			//Mode pointer determines whether we return dumr1 or dumr2
			m68306duartreg[dirDUMR1A+m68306_duart.mode_pointer[0]] = data;
			if(m68306_duart.mode_pointer[0])
				LOG(("%8x:MODE A REGISTER - DUMR2A Write = %x\n",activecpu_get_pc(),data));
			else
				LOG(("%8x:MODE A REGISTER - DUMR1A Write = %x\n",activecpu_get_pc(),data));
			//After writing, update the mode pointer (note, it can only go to 1)
			m68306_duart.mode_pointer[0]=1;
			return;	//We're done

		//F7E3 - CLOCK SELECT (DUCSRA)
		case 0xf7e3:
			LOG(("%8x:CLOCK SELECT (DUCSRA) Write = %x\n",activecpu_get_pc(),data));
			m68306duartreg[dirDUCSRA] = data;
			return;

		//F7E5 - COMMAND REGISTER A (DUCRA)
		case 0xf7e5:
			duart_command_register_w(0,data);
			return;

		//F7E7 - TRANSMIT BUFFER A(DUTBA)
		case 0xf7e7:
#if !DISABLE_68306_TX
			if(m68306_duart.channel[0].tx_enable) {
				LOG(("%8x:TRANSMIT BUFFER A(DUTBA) Write = %x\n",activecpu_get_pc(),data));
				//Update buffer
				m68306duartreg[dirDUTBA] = data;
				//Set up to send it!
				m68306_load_transmitter(0,data);
			}
			else
				LOG(("%8x: TC DISABLED! - TRANSMIT BUFFER A(DUTBA) Write = %x\n",activecpu_get_pc(),data));
#endif
			return;

		//F7E9 - AUX CONTROL REGISTER(DUACR)
		case 0xf7e9:
			LOG(("%8x:AUX CONTROL REGISTER(DUACR) Write = %x\n",activecpu_get_pc(),data));
			m68306duartreg[dirDUACR] = data;
			return;

		//F7EB - INTERRUPT MASK (DUIMR)
		case 0xf7eb:
			LOG(("%8x:INTERRUPT MASK (DUIMR) Write = %x\n",activecpu_get_pc(),data));
			m68306_duart_set_duimr(data);
			return;

		//F7ED - COUNTER/TIMER MSB
		case 0xf7ed:
			LOG(("%8x:COUNTER/TIMER MSB Write = %x\n",activecpu_get_pc(),data));
			break;
		//F7EF - COUNTER/TIMER LSB
		case 0xf7ef:
			LOG(("%8x:COUNTER/TIMER LSB Write = %x\n",activecpu_get_pc(),data));
			break;

		//F7F1 - MODE B (DUMR1B,DUMR2B)
		case 0xf7f1:
			//Mode pointer determines whether we return dumr1 or dumr2
			m68306duartreg[dirDUMR1B+m68306_duart.mode_pointer[1]] = data;
			if(m68306_duart.mode_pointer[1])
				LOG(("%8x:MODE B REGISTER - DUMR2B Write = %x\n",activecpu_get_pc(),data));
			else
				LOG(("%8x:MODE B REGISTER - DUMR1B Write = %x\n",activecpu_get_pc(),data));
			//After writing, update the mode pointer (note, it can only go to 1)
			m68306_duart.mode_pointer[1]=1;
			return;	//We're done

		//F7F3 - CLOCK SELECT (DUCSRB)
		case 0xf7f3:
			LOG(("%8x:CLOCK SELECT (DUCSRB) Write = %x\n",activecpu_get_pc(),data));
			m68306duartreg[dirDUCSRB] = data;
			return;

		//F7F5 - COMMAND REGISTER B (DUCRB)
		case 0xf7f5:
			duart_command_register_w(1,data);
			return;

		//F7F7 - TRANSMIT BUFFER B(DUTBB)
		case 0xf7f7:
#if !DISABLE_68306_TX
			if(m68306_duart.channel[1].tx_enable) {
				LOG(("%8x:TRANSMIT BUFFER B(DUTBB) Write = %x\n",activecpu_get_pc(),data));
				//Update buffer
				m68306duartreg[dirDUTBB] = data;
				//Set up to send it!
				m68306_load_transmitter(1,data);
			}
			else
				LOG(("%8x: TC DISABLED! - TRANSMIT BUFFER B(DUTBB) Write = %x\n",activecpu_get_pc(),data));
#endif
			return;

		//F7F9 - INTERRUPT VECTOR(DUIPVR)
		case 0xf7f9:
			LOG(("%8x:INTERRUPT VECTOR(DUIPVR) Write = %x\n",activecpu_get_pc(),data));
			break;
		//F7FB - OUTPUT PORT CONFIGURATION (DUOPCR)
		case 0xf7fb:
			LOG(("%8x:OUTPUT PORT CONFIGURATION (DUOPCR) Write = %x\n",activecpu_get_pc(),data));
			break;
		//F7FD - OUTPUT PORT(DUOP) BIT SET
		case 0xf7fd:
			LOG(("%8x:OUTPUT PORT(DUOP) BIT SET Write = %x\n",activecpu_get_pc(),data));
			break;
		//F7FF - OUTPUT PORT(DUOP) BIT RESET
		case 0xf7ff:
			LOG(("%8x:OUTPUT PORT(DUOP) BIT RESET Write = %x\n",activecpu_get_pc(),data));
			break;

		default:
			LOG(("%8x:M68306UART_w[%04x]= %x\n",activecpu_get_pc(),address,data));
	}

	//Store value
	m68306duartreg[address-0xf7e1] = data;
}


//TX Byte sent
static void m68306_tx_byte_sent(int which)
{
	int check_cts = (m68306duartreg[dirDUMR2A+(which*0x10)]&0x10)>>4;
	int get_rts = 0;

	//Are we clear to send the byte? ..check CTS if configured to check it first and if it's 1, try again!
	if(check_cts) {

		//Channel B - rts line comes from 8752
		if(which)
			get_rts = get_rts_line_from_8752();
		else
		//Channel A - rts line comes from ?? (printer?) - unsupported
			get_rts = 0;

		if(get_rts) {
			PRINTF(("cts line hi, so we'll wait!\n"));
			cpu_boost_interleave(TIME_IN_HZ(10), TIME_IN_USEC(1500));
			timer_set(TIME_IN_HZ(2),which, m68306_tx_byte_sent);
			return;
		}
	}

	//Clear flags
	m68306_duart.channel[which].tx_sending = 0;

	//Send the byte
	//Channel B - goes to 8752
	if(which)
		send_data_to_8752(m68306_duart.channel[which].tx_shift_reg);
	else
	//Channel A - goes to ?? (printer?) - unsupported
		LOG(("data sent from channel a = %x!\n",m68306_duart.channel[which].tx_shift_reg));

	//Any more data waiting to go?
	if(!m68306_duart.channel[which].tx_new_data) {
		//Set txEMP bit 3 in DUSR
		m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] | 0x08);
	}
}

//Send a byte of data out the tx line
static void m68306_tx_send_byte(int which)
{
	//Are we already sending data(more accurately, have we not yet sent it?)
	//If so, call again in a bit..
	if(m68306_duart.channel[which].tx_sending) {
		PRINTF(("sending already, so we'll wait!\n"));
		cpu_boost_interleave(TIME_IN_HZ(10), TIME_IN_USEC(1500));
		timer_set(TIME_IN_HZ(2),which, m68306_tx_send_byte);
		return;
	}
	//Flag that we're sending..
	m68306_duart.channel[which].tx_sending = 1;
	//Transfer hold register to shift register
	m68306_duart.channel[which].tx_shift_reg = m68306_duart.channel[which].tx_hold_reg;
	//Clear new data flag
	m68306_duart.channel[which].tx_new_data = 0;
	//Set txRDY bit 2 in DUSR
	m68306_duart_set_dusr(which, m68306duartreg[dirDUSRA+(which*0x10)] | 0x04);
	//Setup to shift the data (no idea what the value should be used for the time)
	timer_set(TIME_IN_CYCLES(100,0),which, m68306_tx_byte_sent);
	//m68306_tx_byte_sent(which);
}

//Load Transmitter with data
static void m68306_load_transmitter(int which, int data)
{
#if M68306_TX_SEND_IMMEDIATE
	if(which) send_data_to_8752(data);
#else
	//Load holding register with data
	m68306_duart.channel[which].tx_hold_reg = data;
	//Clear txEMP,txRDY bits (bits 3 & 2) in DUSR
	m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] & (~0x0c));
	//Try & Send Data
	m68306_duart.channel[which].tx_new_data = 1;
	m68306_tx_send_byte(which);
#endif
}

void send_data_to_68306(int data)
{
#if !DISABLE_68306_RX
	timer_set(TIME_NOW, data , m68306_rx_cause_int_channel_b);
	//timer_set(TIME_IN_CYCLES(10000,0), data , m68306_rx_cause_int_channel_b);
#endif
}

//Callback to generate an Receiver Interrupt
static void m68306_rx_cause_int_channel_a(int data) { m68306_rx_cause_int(0,data); }
static void m68306_rx_cause_int_channel_b(int data) { m68306_rx_cause_int(1,data); }

//Generate an Receiver Interrupt
static void m68306_rx_cause_int(int which,int data)
{
	int push = 0;
	if(m68306_duart.channel[which].rx_enable) {

		//Set CTS to 1, so we can't receive any more data..
		if(which==1)
			set_cts_line_to_8752(1);

		//Force CPU 0 to be active - otherwise this doesn't work!
		if(cpu_getactivecpu() != 0) {
			cpuintrf_push_context(0);
			push = 1;
		}
		PRINTF(("data to 68306 = %x\n",data));
		//write data to receive buffer
		m68306duartreg[dirDURBB]=data;
		//Set RxRDY status (bit 0)
		m68306_duart_set_dusr(1,m68306duartreg[dirDUSRB] | 0x01);
		if(push)
			cpuintrf_pop_context();
	}
	else
		LOG(("SEND_DATA_TO_68306 - RC DISABLED!\n"));
}

//Duart Start Timer
static void duart_start_timer(void)
{
	double time;
	double clock_src;
	double preload;

	//Reset Timer
	timer_enable(m68306_duart.timer, 0);

	//For now, support only external clock src (but really we should check the clock source bits in auxillary register
	clock_src = TIME_IN_HZ(3686400);	// Clock src is fixed @ 3.6864MHz
	//Get preload  value
	preload = (m68306duartreg[dirCNT_MSB] << 8) | m68306duartreg[dirCNT_LSB];
	if(!preload) return;
	//Restart the timer (period is clock source * (2 * preload value)
	time = clock_src * 2 * preload;
	timer_adjust(m68306_duart.timer, time, 0, 0);
}

//Duart Timer Callback
static void duart_timer_callback (int param)
{
	//Invert output
	m68306_duart.timer_output = !m68306_duart.timer_output;
	//Look for 1->0 transition in the output to trigger an interrupt!
	if(!m68306_duart.timer_output) {
		//Set CTR/TMR RDY bit (3) in DUISR
		m68306_duart_set_duisr(m68306duartreg[dirDUISR] | 0x08);
	}
	//Restart timer!
	duart_start_timer();
}

//Can we generate an interrupt?
static void m68306_duart_check_int()
{
	//check if we should generate an interrupt (status bits must match interrupt mask bits)
	if(m68306duartreg[dirDUISR] & m68306duartreg[dirDUIMR]) {
		//Check for a Timer Interrupt.. (bit 3 in status)
		if(m68306duartreg[dirDUISR] & 0x08) {

			/*--Timer can either trigger /TIRQ, or regular serial /IRQ depending on MASK & IENT flags--*/

			//Timer generates an IRQ?
			int irq = (m68306duartreg[dirDUIMR] & 0x08) >> 3;
			//Timer generates an TIRQ?
			int tirq =(m68306intreg[irICR] & 0x8000) >> 14;

			switch (irq+tirq) {
				//NONE - It's Disabled
				case 0:
					break;
				//IRQ - Enabled
				case 1:
					LOG(("Timer /IRQ \n"));
					break;	//do nothing, and fall through to IRQ code below for handling
				//TIRQ - Enabled
				case 2:
					trigger_duart_int(0);
					return;
				//BOTH?!?
				case 3:
					LOG(("PROBLEM: Both /IRQ & /TIRQ Enabled for a 68306 Timer Overflow Interrupt!\n"));
					return;
			}
		}
		//Generate a serial interrupt? (/IRQ line from DUART)
		trigger_duart_int(1);
	}
}

//Set DUIMR status register
static void m68306_duart_set_duimr(int data)
{
	//Update register
	m68306duartreg[dirDUIMR] = data;
	m68306_duart_check_int();
}

//Set DUISR status register
//TODO: how to deal with a timer and serial interrupt occuring together? Timer has higher priority even if duart set to level 7 also!)
static void m68306_duart_set_duisr(int data)
{
	//Update register
	m68306duartreg[dirDUISR] = data;
	m68306_duart_check_int();
}

//Set DUSR status register
static void m68306_duart_set_dusr(int which,int data)
{
	int txRDY=0;
	int rxRDY=0;
	m68306duartreg[dirDUSRA+(which*0x10)] = data;
	//txRDY is duplicated in DUISR
	txRDY = (m68306duartreg[dirDUSRA+(which*0x10)] & 0x04)>>2;	//TxRDY in DUSR is bit 2)
	//txRDY in DUISR (bit 0 for channel a, bit 4 for channel b)
	data = m68306duartreg[dirDUISR] & ~(1<<(which*4));
	data |= txRDY<<(which*4);

	//Either FFUL or RxRDY is duplicated in DUISR (bit 1 for channel a, bit 5 for channel b)
	//(depending on how bit 6 is set in DUMR1)
	data &= ~(1<<((which*4)+1));		//Mask out either bit 1 or bit 5
	if(m68306duartreg[dirDUMR1A+(which*0x10)] & 0x40) {
		//FFUL (bit 1) is duplicated
		rxRDY = (m68306duartreg[dirDUSRA+(which*0x10)] & 0x02) >> 1;
	}
	else {
		//RxRDY (bit 0) is duplicated
		rxRDY = m68306duartreg[dirDUSRA+(which*0x10)] & 0x01;
	}
	//Update rxRDY
	data |= rxRDY<<((which*4)+1);
	//Update DUISR
	m68306_duart_set_duisr(data);
}

//Trigger a DUART Interrupt (0 = Timer (/TIRQ), 1 = Serial Port (/IRQ))
static void trigger_duart_int(int which)
{
	int push=0;
	//Make sure we're the active context!
	if(cpu_getactivecpu() != 0) {
		push = 1;
		cpuintrf_push_context(0);		//How can i not hardcode this to 0 as the cpu num?
	}

	m68306_duart_int = which+1;		//Store which interrupt (flags ack to avoid auto-vector also)

	//Serial Port? (/IRQ Line from DUART)
	if(which){
		//Determine Priority Level for Serial Port IRQ
		int level=(m68306intreg[irSYSTEM] & 0x700)>>8;	//Bits 8-10 of System Register
		m68306irq(level,1);
		m68306irq(level,0);
	}
	//Timer (/TIRQ)
	else {
		//Timer is always a Level 7 Priority
		m68306irq(7,1);
		m68306irq(7,0);
	}
	m68306_duart_int = 0;			//Clear flag
	if(push)
		cpuintrf_pop_context();
}

/* DUART COMMAND REGISTER WRITE */
static void duart_command_register_w(int which, int data)
{
	int cmd = 0;
	//Bit 7   = 0
	//Bit 6-4 = MISC Commands
	//Bit 3-2 = TC Command (Transmitter Channel)
	//Bit 1-0 = RC Command (Receiver Channel)

	if(which)
		LOG(("%8x:COMMAND REGISTER B (DUCRB) Write = %x\n",activecpu_get_pc(),data));
	else
		LOG(("%8x:COMMAND REGISTER A (DUCRA) Write = %x\n",activecpu_get_pc(),data));

	//PROCESS MISC COMMANDS
	if(data & 0x70) {
		cmd = (data & 0x70) >> 4;
		//Misc Commands
		//001 = Reset Mode Register Pointer
		//010 = Reset Receiver
		//011 = Reset Transmitter
		//100 = Reset Error Status
		//101 = Reset Break/Change Interrupt
		//110 = Start Break
		//111 = Stop Break
		switch(cmd) {
			//Reset Mode Register Pointer
			case 1:
				LOG(("- RESET MODE REGISTER!\n"));
				m68306_duart.mode_pointer[which] = 0;						//Mode Pointer
				break;
			//Reset Receiver
			case 2:
				LOG(("- RESET RECEIVER(misc)!\n"));
				//Disable receiver
				m68306_duart.channel[which].rx_enable=0;
				//Clear FFUL and RxRDY bits (bits 1 & 0) in DUSR
				m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] & (~0x03));
				//FIFO Pointer re-initialized
				break;
			//Reset Transmitter
			case 3:
				LOG(("- RESET TRANSMITTER(misc)!\n"));
				//Disable Transmitter
				m68306_duart.channel[which].tx_enable=0;
				//Clear txEMP,txRDY bits (bits 3 & 2) in DUSR
				m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] & (~0x0c));
				break;
			//Reset Error Status
			case 4:
				LOG(("- RESET ERROR STATUS!\n"));
				//Clear RB,FE,PE,OE bits (bits 7-4) in DUSR
				m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] & (~0xf0));
				break;

			//Reset Break/Change Interrupt
			case 5:
				LOG(("- RESET BREAK/CHANGE INTERRUPT!\n"));
				//Clear DBx bits (bits 6 & 2) in DUISR
				m68306_duart_set_duisr(m68306duartreg[dirDUISR] & (~0x44));
				break;
			//Start Break
			case 6:
				LOG(("- START BREAK!\n"));
				break;
			//Stop Break
			case 7:
				LOG(("- STOP BREAK!\n"));
				break;
		}
	}

	//PROCESS TRANSMITTER COMMANDS
	if(data & 0x0c) {
		cmd = (data & 0x0c) >> 2;
		//Transmitter Commands
		//01 = Enable Transmitter
		//10 = Disable Transmitter
		//11 = *Not used*
		switch(cmd) {
			//Enable Transmitter
			case 1:
				LOG(("- ENABLE TRANSMITTER!\n"));
				//Enable Transmitter
				m68306_duart.channel[which].tx_enable=1;
				//Set txEMP,txRDY bits (bits 3 & 2) in DUSR
				m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] | 0x0c);
				break;
			//Disable Transmitter
			case 2:
				LOG(("- DISABLE TRANSMITTER!\n"));
				//Disable Transmitter
				m68306_duart.channel[which].tx_enable=0;
				//Clear txEMP,txRDY bits (bits 3 & 2) in DUSR
				m68306_duart_set_dusr(which,m68306duartreg[dirDUSRA+(which*0x10)] & (~0x0c));
				break;
			//*Not used*
			default:
				LOG(("- INVALID TRANSMITTER COMMAND = %x\n",cmd));
		}
	}

	//PROCESS RECEIVER COMMANDS
	if(data & 0x03) {
		cmd = data & 0x03;
		//Receiver Commands
		//01 = Enable Receiver
		//10 = Disable Receiver
		//11 = *Not used*
		switch(cmd) {
			//Enable Receiver (No status flags changed)
			case 1:
				LOG(("- ENABLE RECEIVER!\n"));
				m68306_duart.channel[which].rx_enable=1;
				break;
			//Disable Receiver (No status flags changed)
			case 2:
				LOG(("- DISABLE RECEIVER!\n"));
				m68306_duart.channel[which].rx_enable=0;
				break;
			//*Not used*
			default:
				LOG(("- INVALID RECEIVER COMMAND = %x\n",cmd));
		}
	}
}

//----------------------------
//  Internal register write
//----------------------------
static void m68306_intreg_w(offs_t address, data16_t data, int word) {
  if (word && (address & 1)) {
    logerror("M68306reg_w odd word address %08x\n",address); return;
  }
  if (address >= 0xffffffc0) { /* internal regs */
    const int reg = (address & 0x3f)>>1;
    const UINT16 oldval = m68306intreg[reg];
    if (!word) { // handle byte writes
      if (address & 1) data = (oldval & 0xff00) | (data & 0x00ff);
      else             data = (oldval & 0x00ff) | (data << 8);
    }
    switch (reg) {
      case irCS0H: case irCS1H: case irCS2H: case irCS3H:
      case irCS4H: case irCS5H: case irCS6H: case irCS7H:
      case irDRAM0H: case irDRAM1H:
		//LOG(("writing to cs/dram hi=%x\n",data));
        m68306intreg[reg] = data;
        m68306cs[reg/2].addr = (data & 0xfffe)<<16;
        m68306cs[reg/2].rw = (m68306cs[reg/2].rw & 0x8000) | (data & 0x01);
        break;
      case irCS0L: case irCS1L: case irCS2L: case irCS3L:
      case irCS4L: case irCS5L: case irCS6L: case irCS7L:
      case irDRAM0L: case irDRAM1L:
		//LOG(("writing to cs/dram lo=%x\n",data));
        m68306intreg[reg] = data;
        m68306cs[reg/2].mask = ((UINT16)(0xffff0000>>((data & 0xf0)>>4)))<<16;
        m68306cs[reg/2].rw = (m68306cs[reg/2].rw & 0x01) | (data & 0x8000);
        break;
      case irPDATA: { /* Port Data */
        UINT16 tmp = (data ^ oldval) & m68306intreg[irPDIR];
        m68306intreg[irPDATA] = data;
        if (tmp & 0xff00) cpu_writeport16lew(M68306_PORTA_START, (data & m68306intreg[irPDIR])>>8);
        if (tmp & 0x00ff) cpu_writeport16lew(M68306_PORTB_START, data & m68306intreg[irPDIR]);
        // Writing to B4-B7 will also affect IRQ if configures as output.
        break;
      }
	  case irPDIR: { /* Port direction */
        UINT16 tmp = data & ~oldval;
        m68306intreg[irPDIR] = data;
        if (tmp & 0xff00) cpu_writeport16lew(M68306_PORTA_START, (data & m68306intreg[irPDATA])>>8);
        if (tmp & 0x00ff) cpu_writeport16lew(M68306_PORTB_START, data & m68306intreg[irPDATA]);
        break;
      }
      case irICR: /* interrupt control */
		LOG(("%8x:ICR=%04x\n",activecpu_get_pc(),data));
        m68306intreg[irICR] = data; m68306irq(0,0); // update irq level
        break;
      case irBUSERR: /* refresh + buserror */
        logerror("buserror_w %04x\n",data);
        break;
      case irSYSTEM: /* system */
		LOG(("%8x:SYSTEM REG=%04x\n",activecpu_get_pc(),data));
        m68306intreg[irSYSTEM] = (oldval & 0x8000) | (data & 0x7fff); // BTERR bit ignored
        if (data & 0x4000) logerror("Bus Timeout Error not implmented %x\n",data);
        break;
    } /* switch */
  }
  else if (address >= 0xfffff7e0) { /* DUART */
	m68306_duart_reg_w(address&0xffff,data,word);
  }
}
//----------------------------
//  Internal register read
//----------------------------
static data16_t m68306_intreg_r(offs_t address, int word) {
  data16_t data = 0;
  if (word && (address & 1)) {
    logerror("M68306reg_r odd word address %08x\n",address); return 0;
  }
  if (address >= 0xffffffc0) { /* internal regs */
    const int reg = (address & 0x3f)>>1;
    switch (reg) {
      case irCS0H: case irCS1H: case irCS2H: case irCS3H:
      case irCS4H: case irCS5H: case irCS6H: case irCS7H:
      case irDRAM0H: case irDRAM1H:
      case irCS0L: case irCS1L: case irCS2L: case irCS3L:
      case irCS4L: case irCS5L: case irCS6L: case irCS7L:
      case irDRAM0L: case irDRAM1L:
      case irPDATA:  case irPDIR:
	    //LOG(("reading from cs/dram\n"));
        data = m68306intreg[reg]; break;
      case irPPIN: /* port pins (read_only) */
        // B4-B7 is also IRQ pins (ignored for now)
        if (word)
          data = (((cpu_readport16lew(M68306_PORTA_START)<<8) | cpu_readport16lew(M68306_PORTB_START)) & ~m68306intreg[irPDIR]) |
                 (m68306intreg[irPDATA] & m68306intreg[irPDIR]);
        else if (address & 1)
          data = (cpu_readport16lew(M68306_PORTB_START) & ~m68306intreg[irPDIR]) | (m68306intreg[irPDATA] & (m68306intreg[irPDIR] | 0xff00));
        else
          data = ((cpu_readport16lew(M68306_PORTA_START)<<8) & ~m68306intreg[irPDIR]) | (m68306intreg[irPDATA] & (m68306intreg[irPDIR] | 0x00ff));
        break;
      case irISR: /* interrupt status (read only)*/
        data = m68306intreg[irISR]; break;
      case irICR: /* interrupt control */
        data = m68306intreg[irICR]; break;
        break;
      case irBUSERR: /* refresh + buserror */
        logerror("buserror_r\n");
        break;
      case irSYSTEM: /* system */
        data = m68306intreg[irSYSTEM];
		m68306intreg[irSYSTEM] &= 0x7fff; // clear BTERR bit
        break;
    } /* switch */
  }
  else if (address >= 0xfffff7e0) { /* DUART */
     data = m68306_duart_reg_r(address&0xffff,word);
  }
  return (word || (address & 1)) ? data : data>>8;
}

//---------------------
// Interrupt handling
//---------------------
static int m68306ack(int int_level) {
  if (int_level > 7)
	return M68K_INT_ACK_AUTOVECTOR;
  if (int_level) {
    UINT16 intbit = 1<<(int_level-1);
    if (m68306holdirq & intbit) { // lower irq line on ack
      m68306holdirq &= ~intbit;
      m68306irq(int_level, 0);
    }
	//Special DUART int? (DOES NOT USE AUTO-VECTORS)
	if(m68306_duart_int) {
		int vector;
		//Find Timer Vector
		if(m68306_duart_int==1)
			vector = m68306intreg[irSYSTEM] & 0x00FF;	//Lower byte of System Register
		else
		//Find Serial Vector
			vector = m68306duartreg[dirDUIVR];			//Special DUART register for serial vector
		return vector;
	}
	//Nope...external irq lines
    if (m68306intreg[irICR] & intbit) // use auto-vector?
      return M68K_INT_ACK_AUTOVECTOR;
    else { // no autovector, IACK is not emulated
      if (m68306intack)
        return m68306intack(int_level);
      else
        logerror("M68306 No-AutoVector but no callback IRQ=%d\n",int_level);
    }
  }
  return M68K_INT_ACK_AUTOVECTOR; // Whatever
}

static void m68306irq(int irqline, int state) {
  if (irqline) {
    if (state) {
	   //If Timer IRQ, set bit 15 of ISR
	   if(m68306_duart_int==1) {
		m68306intreg[irISR] |= 0x8000;
		irqline = (m68306intreg[irISR] & m68306intreg[irICR] & 0x7f00)>>7;
	   }
	   else
	   //Set appropriate IRQ line of ISR
		m68306intreg[irISR] |= 0x0080<<irqline;

	   //What the does this do Martin? (SE)
       if (state == 2) m68306holdirq |= (1<<(irqline-1));
    }
	else {
	   //If Timer IRQ, clear bit 15 of ISR
	   if(m68306_duart_int==1)
	    m68306intreg[irISR] &= ~0x8000;
	   else
		m68306intreg[irISR] &= ~(0x0080<<irqline);
	}
  }
  CPU_INT_LEVEL = 0;
  // If port B4-B7 are input  set_irq_line works
  // if port B4-B7 are output writing to port works
  // ignore this for now.
  if(m68306_duart_int) {
	  //Timer IRQ can be disabled by bit 15 of ICR
	  if(m68306_duart_int==1 && !(m68306intreg[irICR] & 0x8000))
		  irqline = 0;
	  else {
	  //There doesn't seem to be a way to disable the DUART interrupt (oddly)
	  //however we need to adjust it to make it work with Martin's logic here..
	  irqline = (m68306intreg[irISR]& 0x7f00)>>7;
	  }
  }
  else
	irqline = (m68306intreg[irISR] & m68306intreg[irICR] & 0x7f00)>>7;
  //don't waste time if no irq..
  if(irqline) {
	while (irqline >>= 1)
		CPU_INT_LEVEL += 1;
	CPU_INT_LEVEL <<= 8;
	m68ki_check_interrupts();
  }
}

//---------------------
// exported interface
//---------------------
void m68306_set_irq_line(int irqline, int state) {
  switch (state) {
    case PULSE_LINE:  m68306irq(irqline, 1); // no break;
    case CLEAR_LINE:  m68306irq(irqline, 0); break;
    case ASSERT_LINE: m68306irq(irqline, 1); break;
    case HOLD_LINE:   m68306irq(irqline, 2); break;
  }
}

void m68306_set_irq_callback(int (*callback)(int irqline)) {
  m68306intack = callback;
}

void m68306_init(void) {
  m68k_init();
  m68k_set_cpu_type(M68K_CPU_TYPE_68306); //
  m68k_memory_intf = interface_m68306;
  m68k_state_register("m68306");
  m68k_set_int_ack_callback(m68306ack);
}

void m68306_exit(void) {}

void m68306_reset(void* param) {
  memset(m68306intreg, 0, sizeof(m68306intreg));
  memset(m68306cs,     0, sizeof(m68306cs));
  m68306intack = NULL; m68306holdirq = 0;
  m68306intreg[irSYSTEM] = 0x040f;
  m68306intreg[irICR] = 0x00ff;
  m68306intreg[irCS0H] = 0x0001;
  m68306intreg[irCS0L] = 0xff0e;
  m68306cs[0].rw = 0x8001;
  //duart stuff
  memset(m68306duartreg, 0, sizeof(m68306duartreg));
  memset(&m68306_duart,0, sizeof(m68306_duart));
  m68306_duart.timer = timer_alloc(duart_timer_callback);	//setup timer
  timer_enable(m68306_duart.timer, 0);						//Reset the timer

  m68k_pulse_reset();
}

void m68306_set_context(void *src)
{
	if (m68k_memory_intf.read8 != m68306_read8)
            m68k_memory_intf = interface_m68306;
	m68k_set_context(src);
}
unsigned m68306_get_reg(int regnum) {
  if (regnum == REG_PC) return m68k_get_reg(NULL, M68K_REG_PC);
  return m68000_get_reg(regnum);
}
void m68306_set_reg(int regnum, unsigned val) {
  if ((short)regnum == (short)REG_PC) m68k_set_reg(M68K_REG_PC, val);
  else m68000_set_reg(regnum, val);
}

const char *m68306_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? '?':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? '?':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68306";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 2002 Martin Adrian, Steve Ellenoff and PinMAME team";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68000_win_layout;
	}
	return buffer[which];
}

unsigned m68306_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble( buffer, pc, M68K_CPU_TYPE_68306 );
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}


#endif // HAS_M68306
#endif // A68K2
