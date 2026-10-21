// license:BSD-3-Clause
// copyright-holders:Tony La Porta
/**************************************************************************

    Texas Instruments TMS320C1x DSP Emulator

    Copyright Tony La Porta

    C port for PinMAME of MAME's src/devices/cpu/tms320c1x/tms320c1x.cpp.
    Unlike the other PinMAME CPU cores this one is not registered in
    cpuintrf.c: it is driven directly by the device that embeds it (the
    BSMT2000, see src/sound/bsmt2000.c), which runs it in lock-step with
    its sound stream. Several instances can exist at the same time, as all
    state lives in struct tms320c1x_state.

    Only the TMS320C15 memory layout (4K words of program ROM, 256 words of
    data RAM) is modelled; the TMS320C10 differs only in having 144 words
    of data RAM.

**************************************************************************/

#pragma once

#include "osd_cpu.h"

struct tms320c1x_state
{
	/* CPU registers */
	UINT16 PC;
	UINT16 PREVPC;
	UINT16 STR;
	UINT32 ACC;
	UINT32 ALU;
	UINT32 Preg;
	UINT16 Treg;
	UINT16 AR[2];
	UINT16 STACK[4];

	/* internal state */
	UINT16 opcode;
	int    INTF;        /* pending interrupt flag */
	int    icount;
	int    abort;       /* set by tms320c1x_abort(): ends tms320c1x_execute() after the current instruction */
	UINT32 oldacc;
	UINT16 memaccess;
	UINT16 addr_mask;

	UINT16 ram[256];    /* internal data RAM */

	/* program ROM, as native-endian words; (addr_mask + 1) words long */
	const UINT16 *rom;

	/* I/O callbacks (ports 0-7) and the BIO input pin */
	void   *param;
	UINT16 (*io_r)(void *param, int port);
	void   (*io_w)(void *param, int port, UINT16 data);
	int    (*bio_r)(void *param);
};

/* set up a CPU instance; rom must stay valid as long as the instance is used */
void tms320c1x_init(struct tms320c1x_state *cpu, const UINT16 *rom, int addr_bits, void *param,
	UINT16 (*io_r)(void *param, int port), void (*io_w)(void *param, int port, UINT16 data), int (*bio_r)(void *param));

/* reset (the RS pin) */
void tms320c1x_reset(struct tms320c1x_state *cpu);

/* assert the INT pin (a pending interrupt cannot be cleared, as on the chip) */
void tms320c1x_set_irq(struct tms320c1x_state *cpu);

/* run for at least `cycles` machine cycles (1 cycle = 4 input clocks);
   returns the number of cycles actually run (can overshoot by up to 2) */
int tms320c1x_execute(struct tms320c1x_state *cpu, int cycles);

/* called from an I/O callback: return from tms320c1x_execute() once the current instruction is done */
void tms320c1x_abort(struct tms320c1x_state *cpu);
