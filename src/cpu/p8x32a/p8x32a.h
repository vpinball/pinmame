// license:BSD-3-Clause

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define P8X32A_NEVER 0xFFFFFFFFFFFFFFFFull
#define P8X32A_PEND_COG(n) (1u << (n))       /* pend_what: cog n's OUTA or DIRA */
#define P8X32A_PEND_CTR(j) (1u << (8 + (j))) /* pend_what: counter j (2 * cog + 0 for A, 1 for B) */

typedef struct p8x32a_bus {
	void *ctx;
	uint32_t (*pins_in)(void *ctx, uint64_t t);
	uint64_t (*pins_next)(void *ctx, uint64_t t); /* next input edge, or P8X32A_NEVER */
	void (*pins_out)(void *ctx, uint64_t t, uint32_t out, uint32_t dir);
	void (*cog_start)(void *ctx, uint64_t t, int cog, uint32_t ptr);
	void (*clkset)(void *ctx, uint64_t t, uint8_t cfg);
	void (*log)(void *ctx, const char *msg);
	void (*ctr_state)(void *ctx, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq); /* ctr: 0 = A, 1 = B */
	uint32_t pure_in; /* inputs that change only at pins_next edges */
	void (*lazy)(void *ctx, uint64_t t, uint32_t mask, uint32_t out, uint32_t dir); /* from t the lazy cog owns mask */
	void (*lazy_pins)(void *ctx, uint64_t t, uint32_t out, uint32_t dir);   /* the lazy cog's pins changed */
} p8x32a_bus;

typedef struct p8x32a_reg {
	uint32_t prev, cur;
	uint64_t at;
} p8x32a_reg;

typedef struct p8x32a_cog {
	uint32_t ram[512];
	uint32_t ptr, ix, nix, i, s, d;
	uint16_t p, px;
	uint8_t c, z, cancel, run, cond, pad[3]; /* pad: snapshots are compared with memcmp */
	int ev;
	uint64_t ev_t, t0, latch, disable_at, restart_at;
	p8x32a_reg outa, dira;
	uint32_t ctr[2], frq[2], phs[2], vcfg, vscl;
	uint64_t phs_t[2];
	uint32_t ctr_old[2], frq_old[2], phs_old[2];
	uint64_t phs_t_old[2], ctr_at[2];
	uint32_t ctr_seen[2], frq_seen[2];
} p8x32a_cog;

#define P8X32A_PAT 32
#define P8X32A_SNAP (sizeof(p8x32a_cog) - offsetof(p8x32a_cog, ptr))

/* a cog polling in a loop: a recorded iteration replays while its inputs stay the same */
typedef struct p8x32a_loop {
	uint8_t state, edge, dirty, hub;
	uint16_t head, edge_head, nins, nins0;
	uint64_t head_t, edge_t, period, t0;
	int nsnap, nin, nhub;
	uint32_t in_mask[4], in_val[4], hub_v[4], wake;
	uint16_t hub_a[4];
	uint8_t hub_n[4];
	uint64_t snap_t[P8X32A_PAT];
	unsigned char snap[P8X32A_PAT][P8X32A_SNAP];
} p8x32a_loop;

/* Translated runs of a cog's local instructions (p8x32ajit.cpp). fn runs a block and its successors, returns the
   instruction count and updates st as run_local would. A block ends after a taken jump or before a slot whose word
   no longer fits P8X32A_JDYN. */
#define P8X32A_JMAX 32
#define P8X32A_JDYN 0x3FFFFu /* S and D fields: a slot differing only here is read at run time */

typedef struct p8x32a_jblk p8x32a_jblk;
struct p8x32a_jst;

/* the block at a cog address as seen by a block exit: entry, first word under mask, length (~0: none) */
typedef struct p8x32a_jlink {
	const void *body;
	uint32_t word, mask, len, part; /* part: a lazy cog's block */
	uint32_t (*fn)(struct p8x32a_jst *st); /* its entry from run_local */
} p8x32a_jlink;

typedef struct p8x32a_jst {
	uint32_t *ram;
	const uint8_t *code; /* bit s: slot s is a fixed word of a block */
	p8x32a_jblk **tab;   /* the cog's blocks by address */
	const p8x32a_jlink *link; /* and their links */
	p8x32a_loop *loop;
	uint64_t t2;
	uint32_t budget, ix, fl, pc, px, nix, jmp, jc, edge, w, s, d; /* fl: Z bit 0, C bit 1; rest: last instruction */
	uint32_t inv, inv_old; /* 0, or the changed code slot + 1 and its old word; ~0: several */
	uint32_t on;           /* a lazy cog's OUTA writes (ot/ov) */
	uint64_t *ot;
	uint32_t *ov;
	uint64_t tl, slot;     /* lazy cog: last start time allowed; first hub slot */
	uint64_t latch;        /* slot of the last hub read run, 0 if none */
	const uint8_t *hub;
	const uint8_t *jmap;   /* p8x32a.jmap: a block stops before reading a journalled long */
	uint32_t par;
	/* a block's closing hub op runs inline via hubfn when event_run would run it now (h <= t, key below lim,
	   latch + 5 < dis, *pgen == gen); else the block stops before it with hiss = 1 */
	uint64_t t, lim, dis, hlatch;
	const unsigned *pgen;
	unsigned gen, n, hiss, hs, hd, hi;
	uint32_t (*hres)(struct p8x32a_jst *st); /* with hiss: resume entry */
	void *chip;
	uint32_t (*hubfn)(struct p8x32a_jst *st);
	uint64_t *pnow;        /* p8x32a.now */
	uint32_t ca, csz;      /* test builds: chkfn checks a hub read */
	void (*chkfn)(struct p8x32a_jst *st);
} p8x32a_jst;
#define P8X32A_JOUT 256 /* the OUTA writes a run of blocks may leave */
#define P8X32A_JN 64    /* journal entries */
#ifndef P8X32A_LZH
#define P8X32A_LZH 8    /* a lazy cog's pin changes after its catch-up end (at most 2) */
#endif

struct p8x32a_jblk {
	uint32_t (*fn)(p8x32a_jst *st);
	const void *body; /* entry for a block reached from another */
	unsigned len, valid, part; /* part: a lazy cog's block, stops at its budget */
	uint32_t words[P8X32A_JMAX], dyn; /* dyn bit k: slot k is read at run time */
};

/* translate the run at cog address a; var[k]: bits slot k has changed in; outa: record OUTA writes (lazy cog) */
typedef p8x32a_jblk *(*p8x32a_jit_fn)(void *jit, p8x32a_jblk *old, unsigned a, uint32_t ix, const uint32_t *ram, const uint32_t *var, int outa);

/* a decoded instruction word, valid while word matches the instruction being run */
typedef struct p8x32a_dec {
	uint32_t word;
	uint16_t src, dst;
	uint8_t kind, fl, cond, jh; /* jh: a hub op a block may end with */
} p8x32a_dec;

typedef struct p8x32a {
	p8x32a_bus bus;
	uint8_t hub[65536];
	p8x32a_cog cog[8];
	uint8_t cog_e, lock_e, lock_state, cfg, sys_q, sys_c;
	uint64_t now, horizon, flushed, slot_base;
	uint64_t pend[40];
	uint32_t pend_pins[40], pend_what[40]; /* pins and registers a pending point changes */
	int npend;
	uint32_t last_out, last_dir;
	uint32_t logged;
	int stop;
	int ctr_ok;
	uint64_t ctr_from, ctr_nt;
	uint16_t nco_mask, nco_ok;  /* counters (2 * cog + ctr) in an NCO mode; cached next changes */
	uint8_t nco_n, nco_list[16];
	int pins_ok; /* reg_out .. nco_lvl are current */
	uint32_t reg_out, reg_dir, cog_dir[8], cog_out[8], nco_lvl[16];
	uint64_t nco_from[16], nco_nt[16];
	uint8_t sleepers;
	uint8_t waiters;            /* cogs in WAITPEQ/WAITPNE (bit n); a bit may outlive its wait */
	uint8_t lz_on, lz, lz_nh;   /* the lazy cog; its pending pin changes */
	uint32_t lazy_ok;           /* pins a lazy cog may drive; 0 = none */
	uint32_t lz_pins, lz_out, lz_hout[P8X32A_LZH];
	p8x32a_reg lz_reg;          /* the lazy cog's OUTA */
	uint64_t lz_at, lz_to, lz_evt, lz_ht[P8X32A_LZH], lz_try[8]; /* lz_evt: its next event */
	uint32_t lz_wait[8];
	uint64_t lazies;            /* lazy cogs entered */
	/* journal: other cogs' hub writes the lazy cog has not reached, with the bytes replaced */
	uint8_t jn, jn_off;         /* entries; jn_off: journal disabled */
	uint8_t jn_sz[P8X32A_JN], jn_old[P8X32A_JN][4];
	uint16_t jn_a[P8X32A_JN];
	uint64_t jn_t[P8X32A_JN];
	uint8_t jmap[2048];         /* bit a / 4: hub long a has an entry */
	uint64_t jn_full, jn_writes;/* catch-ups forced by a full journal; entries made */
	unsigned sched_gen; /* bumped when one cog changes another's next event */
	uint64_t sleeps; /* idle loops entered */
	p8x32a_loop loop[8];
	p8x32a_dec dec[8][512];
	p8x32a_jit_fn jit_build;    /* NULL: no translation */
	void *jit;
	p8x32a_jblk *jblk[8][512];
	p8x32a_jlink jlink[8][512];
	/* resume entry of the block that issued cog n's pending hub op; valid while jep[n] == jres_ep[n] */
	uint32_t (*jres[8])(p8x32a_jst *st);
	uint32_t jres_i[8];
	unsigned jres_px[8], jres_ep[8], jep[8];
	uint32_t jvar[8][512];
	uint64_t jit_refused;       /* lookups left to the interpreter */
	uint8_t jcode[8][64];
	uint64_t jot[P8X32A_JOUT];
	uint32_t jov[P8X32A_JOUT];
} p8x32a;

void p8x32a_init(p8x32a *p, const p8x32a_bus *bus);
void p8x32a_reset(p8x32a *p, uint64_t t);
void p8x32a_run_until(p8x32a *p, uint64_t t);
uint32_t p8x32a_pins(p8x32a *p, uint64_t t, uint32_t *dir);
uint8_t p8x32a_hub_at(const p8x32a *p, uint32_t a, uint64_t t); /* hub RAM as the lazy cog reads it at t */

#ifdef __cplusplus
}
#endif
