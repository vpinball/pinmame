// license:BSD-3-Clause

#ifndef MIPS32_H
#define MIPS32_H

#include <stdint.h>

enum {
	MIPS32_EXC_INT  = 0,
	MIPS32_EXC_ADEL = 4,
	MIPS32_EXC_ADES = 5,
	MIPS32_EXC_IBE  = 6,
	MIPS32_EXC_DBE  = 7,
	MIPS32_EXC_SYS  = 8,
	MIPS32_EXC_BP   = 9,
	MIPS32_EXC_RI   = 10,
	MIPS32_EXC_CPU  = 11,
	MIPS32_EXC_OV   = 12,
	MIPS32_EXC_TR   = 13
};

enum { MIPS32_HOOK_DELIVER = 0, MIPS32_HOOK_SKIP = 1, MIPS32_HOOK_STOP = 2 };

typedef struct mips32_state mips32_state;

#define MIPS32_REGIONS 4

/* physical memory served directly, without the bus callbacks; wr NULL = read-only (writes go to the bus) */
typedef struct mips32_region {
	uint32_t base, size;
	const uint8_t *rd;
	uint8_t *wr;
} mips32_region;

typedef struct mips32_bus {
	void *ctx;
	uint32_t (*read)(void *ctx, uint32_t pa, int size, int fetch, int *err);
	void (*write)(void *ctx, uint32_t pa, uint32_t data, int size, int *err);
	int (*exc_hook)(void *ctx, mips32_state *s, int exccode);
	void (*irq_taken)(void *ctx, int vector);
	/* the true bits of an uncertain read (mips32_uncertain), in the read's value; 1 when known (always with wait) */
	int (*settle)(void *ctx, uint32_t token, int wait, uint32_t *bits);
} mips32_bus;

struct mips32_state {
	uint32_t gpr[8][32];
	uint32_t pc, npc, hi, lo;
	int delay;
	uint32_t cur_pc, skip_pc;
	int cur_delay;
	int llbit;
	int waiting;
	int stop;
	int shadow_sets;
	uint32_t status, cause, epc, errorepc, badvaddr, count, compare, ebase;
	uint32_t intctl, srsctl, srsmap, hwrena, config0, prid;
	int count_half;
	int eic_ripl, eic_vector, eic_srs;
	uint64_t cycles;
	mips32_bus bus;
	mips32_region region[MIPS32_REGIONS];
	uint64_t c0, count_at, ti_at; /* instruction start; count/count_half hold Count at count_at; Timer fires at ti_at */
	int irq_chk;                  /* interrupt state may have changed */
	uint32_t fva, fsize;          /* instructions at [fva, fva + fsize) come from fptr */
	const uint8_t *fptr;
	int dslot;                    /* direct region of the last fast load */
	unsigned exc_seq;             /* exceptions taken */
	/* a register whose bits prov_mask await bus.settle; readers wait for them */
	int prov, prov_kind;
	unsigned prov_set, prov_reg, prov_gen, prov_n;
	uint32_t prov_v, prov_mask, prov_tok, unc, unc_tok;
};

void mips32_init(mips32_state *s, const mips32_bus *bus, int shadow_sets, uint32_t prid);
void mips32_reset(mips32_state *s);
void mips32_direct(mips32_state *s, int slot, uint32_t base, uint32_t size, const uint8_t *rd, uint8_t *wr);
int mips32_run(mips32_state *s, int cycles);
uint32_t *mips32_regs(mips32_state *s);
void mips32_uncertain(mips32_state *s, uint32_t mask, uint32_t token); /* from bus.read: bits of this read not known yet */
void mips32_settle(mips32_state *s);                                  /* a register not known yet gets its bits */
void mips32_set_eic(mips32_state *s, int ripl, int vector, int srs);
int mips32_timer_irq(const mips32_state *s);
int mips32_soft_irq(const mips32_state *s);
int mips32_translate(const mips32_state *s, uint32_t va, uint32_t *pa);
uint32_t mips32_get_cp0(const mips32_state *s, int reg, int sel);
unsigned mips32_dasm(char *buf, uint32_t pc, uint32_t op);

#endif
