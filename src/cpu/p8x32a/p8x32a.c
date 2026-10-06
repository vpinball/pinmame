// license:BSD-3-Clause

#include "p8x32a.h"
#include "../../bitops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { EV_NONE, EV_HUB, EV_EXEC, EV_DONE, EV_WAITPIN, EV_RESTART, EV_SLEEP };

/* run_local's helpers stay in its loop */
#if defined(__GNUC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define P8_INLINE static inline __attribute__((always_inline))
#define P8_COLD static __attribute__((noinline))
#elif defined(__GNUC__)
#define P8_INLINE static
#define P8_COLD static __attribute__((noinline))
#elif defined(_MSC_VER)
#define P8_INLINE static __forceinline
#define P8_COLD static __declspec(noinline)
#else
#define P8_INLINE static
#define P8_COLD static
#endif
enum { LOOP_SEARCH, LOOP_RECORD, LOOP_SLEEP };

#define OP(i)   ((unsigned)((i) >> 26))
#define FWZ(i)  (((i) >> 25) & 1)
#define FWC(i)  (((i) >> 24) & 1)
#define FWR(i)  (((i) >> 23) & 1)
#define FIM(i)  (((i) >> 22) & 1)
#define COND(i) (((i) >> 18) & 15)
#define DST(i)  (((i) >> 9) & 511)
#define SRC(i)  ((i) & 511)

enum {
	LOG_WAITVID = 1, LOG_CTR_MODE = 2, LOG_CTR_OUT = 4, LOG_REBOOT = 8, LOG_LAZY = 16
};

static const uint8_t unscr[32] = {
	10, 26, 24, 29, 27, 13, 22, 28, 2, 25, 18, 9, 5, 16, 31, 23,
	1, 30, 14, 0, 11, 8, 15, 20, 17, 4, 19, 6, 12, 21, 7, 3
};

static void log_once(p8x32a * const p, const uint32_t what, const char * const msg)
{
	if (p->logged & what) return;
	p->logged |= what;
	if (p->bus.log) p->bus.log(p->bus.ctx, msg);
}

static uint32_t unscramble(const uint32_t w)
{
	uint32_t r = 0;
	int k;
	for (k = 0; k < 32; k++) r |= ((w >> unscr[k]) & 1u) << k;
	return r;
}

static uint32_t rd32(const p8x32a * const p, uint32_t a)
{
	a &= 0xFFFC;
	return p->hub[a] | (uint32_t)p->hub[a + 1] << 8 | (uint32_t)p->hub[a + 2] << 16 | (uint32_t)p->hub[a + 3] << 24;
}

/* write sz bytes of v at hub address a; nonzero if any changed */
P8_INLINE int hub_store(p8x32a * const p, const unsigned a, const unsigned sz, const uint32_t v)
{
	uint8_t * const h = p->hub + a;
	int ch = h[0] != (uint8_t)v;
	h[0] = (uint8_t)v;
	if (sz == 1) return ch;
	ch |= h[1] != (uint8_t)(v >> 8);
	h[1] = (uint8_t)(v >> 8);
	if (sz == 2) return ch;
	ch |= h[2] != (uint8_t)(v >> 16) || h[3] != (uint8_t)(v >> 24);
	h[2] = (uint8_t)(v >> 16);
	h[3] = (uint8_t)(v >> 24);
	return ch;
}

static uint32_t regval(const p8x32a_reg * const r, const uint64_t t) { return t >= r->at ? r->cur : r->prev; }

#ifdef P8X32A_CHECK
/* test builds: check the time order of pin changes and hub accesses */
enum { CHK_COGE = 0x10000, CHK_LOCKE, CHK_LOCK };
static uint64_t chk_rd[CHK_LOCK + 8], chk_wr[CHK_LOCK + 8];
static void chk_fail(const char * const what, const uint64_t t, const uint64_t past)
{
	fprintf(stderr, "p8x32a: time order: %s at %llu after %llu\n", what, (unsigned long long)t, (unsigned long long)past);
	abort();
}
static void chk_read(const uint32_t a, const unsigned sz, const uint64_t t)
{
	unsigned k;
	for (k = 0; k < sz; k++) {
		if (t < chk_wr[a + k]) chk_fail(a < CHK_COGE ? "hub read after a later write" : "cog or lock state read after a later write", t, chk_wr[a + k]);
		if (t > chk_rd[a + k]) chk_rd[a + k] = t;
	}
}
static void chk_write(const uint32_t a, const unsigned sz, const uint64_t t)
{
	unsigned k;
	for (k = 0; k < sz; k++) {
		const uint32_t b = a + k;
		if (t < chk_rd[b]) chk_fail(a < CHK_COGE ? "hub write after a later read" : "cog or lock state write after a later read", t, chk_rd[b]);
		if (t < chk_wr[b]) chk_fail(a < CHK_COGE ? "hub write after a later write" : "cog or lock state write after a later write", t, chk_wr[b]);
		chk_wr[b] = t;
	}
}
#define CHK_READ(a, sz, t) chk_read(a, sz, t)
#define CHK_WRITE(a, sz, t) chk_write(a, sz, t)
#define CHK_SYS(cell, t) (chk_read(cell, 1, t), chk_write(cell, 1, t))
#else
#define CHK_READ(a, sz, t) ((void)0)
#define CHK_WRITE(a, sz, t) ((void)0)
#define CHK_SYS(cell, t) ((void)0)
#endif

static void loop_notify(p8x32a *p, uint64_t t, uint32_t pins);
static void jit_written(p8x32a *p, int n, unsigned s, uint32_t old);
P8_INLINE void jit_write(p8x32a * const p, const int n, const unsigned s, const uint32_t old)
{
	if (p->jit_build && (p->jcode[n][s >> 3] >> (s & 7) & 1)) jit_written(p, n, s, old);
}

P8_COLD void lazy_run(p8x32a *p, uint64_t t);
static void lazy_exit(p8x32a *p);
/* the lazy cog catches up to t */
#define lazy_catch(p, t) do { if ((p)->lz_on && (t) > (p)->lz_at) lazy_run((p), (t)); } while (0)

/* a hub write at h: journal the replaced bytes for the lazy cog */
P8_COLD void lazy_write(p8x32a * const p, const unsigned ha, const unsigned sz, const uint32_t v, const uint64_t h)
{
	unsigned k;
	for (k = 0; k < sz; k++)
		if (p->hub[ha + k] != (uint8_t)(v >> (8 * k))) break;
	if (k == sz || h - 1 <= p->lz_at) return;
	if (p->jn_off || p->jn == P8X32A_JN) {
		if (!p->jn_off) p->jn_full++;
		lazy_catch(p, h - 1);
		return;
	}
	p->jn_t[p->jn] = h;
	p->jn_a[p->jn] = (uint16_t)ha;
	p->jn_sz[p->jn] = (uint8_t)sz;
	for (k = 0; k < sz; k++) p->jn_old[p->jn][k] = p->hub[ha + k];
	p->jmap[ha >> 5] |= (uint8_t)(1u << ((ha >> 2) & 7));
	p->jn++;
	p->jn_writes++;
}

/* the aligned hub long at a as it was at t */
static uint32_t jn_rd32(const p8x32a * const p, const uint32_t a, const uint64_t t)
{
	uint8_t b[4];
	int k, j;
	memcpy(b, p->hub + a, 4);
	for (k = p->jn - 1; k >= 0; k--)
		if (p->jn_t[k] > t && (p->jn_a[k] & ~3u) == a)
			for (j = 0; j < p->jn_sz[k]; j++) b[(p->jn_a[k] & 3) + j] = p->jn_old[k][j];
	return b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

uint8_t p8x32a_hub_at(const p8x32a *p, uint32_t a, uint64_t t)
{
	a &= 0xFFFF;
	if (!p->jn || !(p->jmap[a >> 5] >> ((a >> 2) & 7) & 1)) return p->hub[a];
	return (uint8_t)(jn_rd32(p, a & ~3u, t) >> (8 * (a & 3)));
}

/* drop journal entries up to t */
static void jn_drop(p8x32a * const p, const uint64_t t)
{
	int k, j = 0;
	for (k = 0; k < p->jn; k++) p->jmap[p->jn_a[k] >> 5] = 0;
	for (k = 0; k < p->jn; k++)
		if (p->jn_t[k] > t) {
			p->jn_t[j] = p->jn_t[k];
			p->jn_a[j] = p->jn_a[k];
			p->jn_sz[j] = p->jn_sz[k];
			memcpy(p->jn_old[j], p->jn_old[k], 4);
			p->jmap[p->jn_a[j] >> 5] |= (uint8_t)(1u << ((p->jn_a[j] >> 2) & 7));
			j++;
		}
	p->jn = (uint8_t)j;
}
static void jit_drop(p8x32a *p, int n);

/* a block no longer runs: its link says so */
P8_INLINE void jit_void(p8x32a * const p, const int n, const unsigned a)
{
	p->jblk[n][a]->valid = 0;
	p->jlink[n][a].len = ~0u;
}


static void wait_notify(p8x32a *p, uint64_t t, uint32_t pins);

/* queue a pin change at t; what: P8X32A_PEND_COG / P8X32A_PEND_CTR bits */
static void add_pending(p8x32a * const p, const uint64_t t, const uint32_t pins, const uint32_t what)
{
	int k;
#ifdef P8X32A_CHECK
	if (t < p->flushed) chk_fail("pin change sent", t, p->flushed);
#endif
	if (p->sleepers) loop_notify(p, t, pins);
	if (p->waiters) wait_notify(p, t, pins);
	for (k = 0; k < p->npend; k++)
		if (p->pend[k] == t) { p->pend_pins[k] |= pins; p->pend_what[k] |= what; return; }
	if (p->npend < (int)(sizeof(p->pend) / sizeof(p->pend[0]))) {
		p->pend_pins[p->npend] = pins;
		p->pend_what[p->npend] = what;
		p->pend[p->npend++] = t;
	}
}

static void flush(p8x32a *p, uint64_t t);

static void regset(p8x32a * const p, const int n, p8x32a_reg * const r, const uint32_t v, const uint64_t at)
{
	const uint32_t pins = (r->cur ^ v) | (r->prev ^ v);
	flush(p, p->now);
	if (at > r->at) r->prev = r->cur;
	r->cur = v;
	r->at = at;
	add_pending(p, at, pins, P8X32A_PEND_COG(n));
}

static int nco(const uint32_t ctr)
{
	const unsigned m = (ctr >> 26) & 31;
	return m == 4 || m == 5;
}

static uint32_t nco_pins(const uint32_t ctr)
{
	if (!nco(ctr)) return 0;
	return 1u << (ctr & 31) | (((ctr >> 26) & 31) == 5 ? 1u << ((ctr >> 9) & 31) : 0);
}

static uint32_t ctr_pins(const p8x32a_cog * const c, const int k, const uint64_t t)
{
	const int old = t < c->ctr_at[k];
	const uint32_t ctr = old ? c->ctr_old[k] : c->ctr[k];
	uint32_t x, a, r;
	const uint32_t frq = old ? c->frq_old[k] : c->frq[k], ph = old ? c->phs_old[k] : c->phs[k];
	const uint64_t pt = old ? c->phs_t_old[k] : c->phs_t[k];
	if (!nco(ctr)) return 0;
	x = t <= pt ? ph : ph + frq * (uint32_t)(t - pt);
	a = x >> 31;
	r = a << (ctr & 31);
	if (((ctr >> 26) & 31) == 5) r |= (a ^ 1) << ((ctr >> 9) & 31);
	return r;
}

static uint64_t nco_toggle(const uint32_t ctr, const uint32_t f, const uint32_t ph, const uint64_t pt, const uint64_t t)
{
	uint32_t x;
	uint64_t dt;
	if (!nco(ctr)) return P8X32A_NEVER;
	if (t < pt) return pt;
	if (!f) return P8X32A_NEVER;
	x = (ph + f * (uint32_t)(t - pt)) & 0x7FFFFFFFu;
	/* both quotients fit 32 bits: 0x80000000 - x + f - 1 < 2^32 for f < 2^31, and x < 2^31 */
	if (f < 0x80000000u) dt = (uint32_t)(0x80000000u - x + f - 1) / f;
	else dt = x / (uint32_t)(0u - f) + 1;
	return t + dt;
}

static uint64_t ctr_toggle(const p8x32a_cog * const c, const int k, const uint64_t t)
{
	if (t < c->ctr_at[k]) {
		const uint64_t o = nco_toggle(c->ctr_old[k], c->frq_old[k], c->phs_old[k], c->phs_t_old[k], t);
		return o < c->ctr_at[k] ? o : c->ctr_at[k];
	}
	return nco_toggle(c->ctr[k], c->frq[k], c->phs[k], c->phs_t[k], t);
}

/* the next counter pin change after t (cached) */
P8_COLD uint64_t ctr_scan(p8x32a * const p, const uint64_t t)
{
	uint64_t nt = P8X32A_NEVER;
	int k;
	for (k = 0; k < p->nco_n; k++) {
		const int j = p->nco_list[k];
		if (!(p->nco_ok >> j & 1) || t < p->nco_from[j] || t >= p->nco_nt[j]) {
			p->nco_nt[j] = ctr_toggle(&p->cog[j >> 1], j & 1, t);
			p->nco_from[j] = t;
			p->nco_ok |= (uint16_t)(1u << j);
		}
		if (p->nco_nt[j] < nt) nt = p->nco_nt[j];
	}
	p->ctr_ok = 1;
	p->ctr_from = t;
	p->ctr_nt = nt;
	return nt;
}

P8_INLINE uint64_t ctr_next(p8x32a * const p, const uint64_t t)
{
	if (p->ctr_ok && t >= p->ctr_from && t < p->ctr_nt) return p->ctr_nt;
	return ctr_scan(p, t);
}

uint32_t p8x32a_pins(p8x32a *p, uint64_t t, uint32_t *dir)
{
	uint32_t o = 0, d = 0;
	int n;
	lazy_catch(p, t);
	if (p->lz_on) o = regval(&p->lz_reg, t) & p->lz_pins;
	for (n = 0; n < 8; n++) {
		const p8x32a_cog * const c = &p->cog[n];
		const uint32_t dd = regval(&c->dira, t);
		uint32_t v;
		if (!dd) continue;
		d |= dd;
		v = regval(&c->outa, t);
		if (p->nco_mask >> (2 * n) & 1) v |= ctr_pins(c, 0, t);
		if (p->nco_mask >> (2 * n) & 2) v |= ctr_pins(c, 1, t);
		o |= v & dd;
	}
	*dir = d;
	return o;
}

/* the pins at t (full), or with the NCO outputs changing at t flipped */
static uint32_t pins_nco(p8x32a * const p, const uint64_t t, const int full)
{
	uint32_t o = p->reg_out;
	int k;
	for (k = 0; k < p->nco_n; k++) {
		const int j = p->nco_list[k], n = j >> 1, c = j & 1;
		const p8x32a_cog * const g = &p->cog[n];
		if (full) p->nco_lvl[j] = ctr_pins(g, c, t);
		else if (p->nco_nt[j] == t) p->nco_lvl[j] ^= nco_pins(t < g->ctr_at[c] ? g->ctr_old[c] : g->ctr[c]);
		o |= p->nco_lvl[j] & p->cog_dir[n];
	}
	return o;
}

/* pins_nco(p, when, 0) and the following ctr_next(p, when) in one pass */
P8_INLINE uint64_t nco_step(p8x32a * const p, const uint64_t when, uint32_t * const out)
{
	uint32_t o = p->reg_out;
	uint64_t nt = P8X32A_NEVER;
	int k;
	for (k = 0; k < p->nco_n; k++) {
		const int j = p->nco_list[k];
		const p8x32a_cog * const g = &p->cog[j >> 1];
		if (p->nco_nt[j] == when) {
			const int c = j & 1;
			p->nco_lvl[j] ^= nco_pins(when < g->ctr_at[c] ? g->ctr_old[c] : g->ctr[c]);
			p->nco_nt[j] = ctr_toggle(g, c, when);
			p->nco_from[j] = when;
		}
		o |= p->nco_lvl[j] & p->cog_dir[j >> 1];
		if (p->nco_nt[j] < nt) nt = p->nco_nt[j];
	}
	p->ctr_from = when;
	p->ctr_nt = nt;
	*out = o;
	return nt;
}

static void pins_regs(p8x32a * const p, const uint64_t t)
{
	int n;
	p->reg_out = p->reg_dir = 0;
	for (n = 0; n < 8; n++) {
		const p8x32a_cog * const c = &p->cog[n];
		const uint32_t dd = regval(&c->dira, t);
		p->cog_dir[n] = dd;
		p->cog_out[n] = dd ? regval(&c->outa, t) & dd : 0;
		p->reg_dir |= dd;
		p->reg_out |= p->cog_out[n];
	}
	p->pins_ok = 1;
}

/* the pins at a pending point where only the registers in what change */
P8_INLINE uint32_t pins_what(p8x32a * const p, const uint64_t t, const uint32_t what)
{
	uint32_t o, d = 0;
	int k, n;
	o = 0;
	for (n = 0; n < 8; n++) {
		if (what >> n & 1) {
			const p8x32a_cog * const c = &p->cog[n];
			const uint32_t dd = regval(&c->dira, t);
			p->cog_dir[n] = dd;
			p->cog_out[n] = dd ? regval(&c->outa, t) & dd : 0;
		}
		d |= p->cog_dir[n];
		o |= p->cog_out[n];
	}
	p->reg_out = o;
	p->reg_dir = d;
	for (k = 0; k < p->nco_n; k++) {
		const int j = p->nco_list[k], c = j & 1;
		const p8x32a_cog * const g = &p->cog[j >> 1];
		if (what >> (8 + j) & 1) p->nco_lvl[j] = ctr_pins(g, c, t);
		else if (p->nco_nt[j] == t) p->nco_lvl[j] ^= nco_pins(t < g->ctr_at[c] ? g->ctr_old[c] : g->ctr[c]);
		o |= p->nco_lvl[j] & p->cog_dir[j >> 1];
	}
	return o;
}

static void flush(p8x32a * const p, const uint64_t t)
{
	for (;;) {
		int k, best = -1, full;
		uint32_t out, dir;
		uint64_t when = ctr_next(p, p->flushed);
		for (k = 0; k < p->npend; k++)
			if (p->pend[k] <= t && (best < 0 || p->pend[k] < p->pend[best])) best = k;
		if (best >= 0 && p->pend[best] < when) when = p->pend[best];
		if (when > t) break;
		/* between pending points only the counters move */
		full = !p->pins_ok || (best >= 0 && p->pend[best] == when);
		if (!full) {
			/* NCO changes up to the next pending point or t */
			const uint64_t end = best >= 0 ? p->pend[best] : t + 1;
			do {
				const uint64_t nt = nco_step(p, when, &out);
				if (out != p->last_out && p->bus.pins_out) p->bus.pins_out(p->bus.ctx, when, out, p->last_dir);
				p->last_out = out;
				p->flushed = when;
				when = nt;
			} while (when < end);
			continue;
		}
		if (p->pins_ok) {
			uint32_t what = 0;
			for (k = 0; k < p->npend; k++)
				if (p->pend[k] == when) what |= p->pend_what[k];
			out = pins_what(p, when, what);
		} else {
			pins_regs(p, when);
			out = pins_nco(p, when, 1);
		}
		dir = p->reg_dir;
		if ((out != p->last_out || dir != p->last_dir) && p->bus.pins_out) p->bus.pins_out(p->bus.ctx, when, out, dir);
		p->last_out = out;
		p->last_dir = dir;
		if (when > p->flushed) p->flushed = when;
		for (k = 0; k < p->npend; k++)
			if (p->pend[k] == when) { --p->npend; p->pend[k] = p->pend[p->npend]; p->pend_pins[k] = p->pend_pins[p->npend]; p->pend_what[k] = p->pend_what[p->npend]; k--; }
	}
	if (t > p->flushed) p->flushed = t;
}

/* INA after flush(t); a lazy cog's pins come from its own register */
P8_COLD uint32_t ina_lazy(p8x32a * const p, const uint64_t t)
{
	uint32_t ext;
	lazy_catch(p, t);
	flush(p, t);
	ext = p->bus.pins_in ? p->bus.pins_in(p->bus.ctx, t) : 0;
	return (((p->last_dir & p->last_out) | (~p->last_dir & ext)) & ~p->lz_pins) | (regval(&p->lz_reg, t) & p->lz_pins);
}

/* INA seen through mask m: the lazy cog catches up only if m has its pins */
static uint32_t ina(p8x32a * const p, const uint64_t t, const uint32_t m)
{
	uint32_t ext;
	if (p->lz_on && (m & p->lz_pins)) return ina_lazy(p, t);
	flush(p, t);
	ext = p->bus.pins_in ? p->bus.pins_in(p->bus.ctx, t) : 0;
	return (p->last_dir & p->last_out) | (~p->last_dir & ext);
}

/* CNT is the time's low 32 bits */
static uint32_t cnt(const uint64_t t) { return (uint32_t)t; }

static int ctr_free(const uint32_t ctr)
{
	const unsigned m = (ctr >> 26) & 31;
	return (m >= 1 && m <= 7) || m == 31;
}

static uint32_t phs_at(const p8x32a_cog * const c, const int k, const uint64_t t)
{
	if (!ctr_free(c->ctr[k]) || t <= c->phs_t[k]) return c->phs[k];
	return c->phs[k] + c->frq[k] * (uint32_t)(t - c->phs_t[k]);
}

static void ctr_rebase(p8x32a_cog * const c, const int k, const uint64_t t)
{
	c->phs[k] = phs_at(c, k, t);
	c->phs_t[k] = t;
}

static void ctr_check(p8x32a * const p, const uint32_t ctr)
{
	const unsigned m = (ctr >> 26) & 31;
	if ((m >= 8 && m <= 15) || (m >= 17 && m <= 30)) log_once(p, LOG_CTR_MODE, "p8x32a: pin-sensing counter mode not modelled");
	if (m >= 2 && m <= 3) log_once(p, LOG_CTR_OUT, "p8x32a: counter PLL pin outputs not modelled");
}

static void ctr_save(p8x32a * const p, p8x32a_cog * const c, const int k, const uint64_t e)
{
	const int j = 2 * (int)(c - p->cog) + k;
	flush(p, p->now);
	if (c->ctr_at[k] <= p->flushed && (p->nco_ok >> j & 1)) {
		/* cached next change stays, capped at e */
		if (e < p->nco_nt[j]) p->nco_nt[j] = e;
		if (e < p->ctr_nt) p->ctr_nt = e;
	} else {
		p->ctr_ok = 0;
		p->nco_ok &= (uint16_t)~(1u << j);
		/* a state not yet in effect is replaced: re-evaluate the pins */
		if (c->ctr_at[k] > p->flushed) p->pins_ok = 0;
	}
	add_pending(p, e, nco_pins(c->ctr[k]), P8X32A_PEND_CTR(j));
	c->ctr_old[k] = c->ctr[k];
	c->frq_old[k] = c->frq[k];
	c->phs_old[k] = c->phs[k];
	c->phs_t_old[k] = c->phs_t[k];
	c->ctr_at[k] = e;
}

/* after a counter write: is it an NCO now or before */
static void ctr_mask(p8x32a * const p, const int n, const int k)
{
	const p8x32a_cog * const c = &p->cog[n];
	const uint16_t b = (uint16_t)(1u << (2 * n + k)), m = p->nco_mask;
	int j;
	if (nco(c->ctr[k]) || nco(c->ctr_old[k])) p->nco_mask |= b;
	else p->nco_mask &= (uint16_t)~b;
	if (p->nco_mask == m) return;
	/* a new NCO has no level until its write takes effect */
	if (p->nco_mask & b) p->nco_lvl[2 * n + k] = 0;
	p->nco_n = 0;
	for (j = 0; j < 16; j++)
		if (p->nco_mask >> j & 1) p->nco_list[p->nco_n++] = (uint8_t)j;
}

static void ctr_notify(p8x32a * const p, const int n, const int k, const uint64_t t)
{
	p8x32a_cog * const c = &p->cog[n];
	if (c->ctr[k] == c->ctr_seen[k] && c->frq[k] == c->frq_seen[k]) return;
	c->ctr_seen[k] = c->ctr[k];
	c->frq_seen[k] = c->frq[k];
	if (p->bus.ctr_state) p->bus.ctr_state(p->bus.ctx, t, n, k, c->ctr[k], c->frq[k]);
}

#ifndef P8X32A_LZH_CAP
#define P8X32A_LZH_CAP P8X32A_LZH /* lower in a test build: the overflow ends the run */
#endif

/* the lazy cog's OUTA write in effect at e, sent to bus.lazy_pins on catch-up */
static void lazy_outa(p8x32a * const p, const uint32_t v, const uint64_t e)
{
	p8x32a_reg * const r = &p->lz_reg;
	const uint32_t g = v & p->lz_pins;
	if (e > r->at) r->prev = r->cur;
	r->cur = v;
	r->at = e;
	if (g == (p->lz_nh ? p->lz_hout[p->lz_nh - 1] : p->lz_out)) return;
	if (e > p->lz_to || p->lz_nh) {
		if (p->lz_nh == P8X32A_LZH_CAP) {
			log_once(p, LOG_LAZY, "p8x32a: lazy cog's pin changes overflow");
			abort();
		}
		p->lz_ht[p->lz_nh] = e;
		p->lz_hout[p->lz_nh++] = g;
		return;
	}
	p->lz_out = g;
	if (p->bus.lazy_pins) p->bus.lazy_pins(p->bus.ctx, e, g, p->lz_pins);
}

/* send the lazy cog's pin changes up to t */
static void lazy_due(p8x32a * const p, const uint64_t t)
{
	int k = 0, j;
	while (k < p->lz_nh && p->lz_ht[k] <= t) {
		p->lz_out = p->lz_hout[k];
		if (p->bus.lazy_pins) p->bus.lazy_pins(p->bus.ctx, p->lz_ht[k], p->lz_out, p->lz_pins);
		k++;
	}
	for (j = k; j < p->lz_nh; j++) { p->lz_ht[j - k] = p->lz_ht[j]; p->lz_hout[j - k] = p->lz_hout[j]; }
	p->lz_nh = (uint8_t)(p->lz_nh - k);
}

/* special_write for the counter and video registers */
P8_COLD void special_rare(p8x32a * const p, const int n, const unsigned a, const uint32_t v, const uint64_t e)
{
	p8x32a_cog * const c = &p->cog[n];
	const int k = a & 1;
	if (p->lz_on) {
		if (n == p->lz && a == 0x1F4) { lazy_outa(p, v, e); return; }
		/* another cog drives the lazy cog's pins: end lazy mode */
		if ((a == 0x1F6 && (v & p->lz_pins)) || ((a == 0x1F8 || a == 0x1F9) && (nco_pins(v) & p->lz_pins))) lazy_exit(p);
	}
	switch (a) {
	case 0x1F4: regset(p, n, &c->outa, v, e); break;
	case 0x1F6: if (e < c->disable_at) regset(p, n, &c->dira, v, e); break;
	case 0x1F8: case 0x1F9: ctr_save(p, c, k, e); ctr_rebase(c, k, e); c->ctr[k] = v; add_pending(p, e, nco_pins(v), P8X32A_PEND_CTR(2 * n + k)); ctr_mask(p, n, k); ctr_check(p, v); ctr_notify(p, n, k, e); break;
	case 0x1FA: case 0x1FB: ctr_save(p, c, k, e); ctr_rebase(c, k, e); c->frq[k] = v; ctr_notify(p, n, k, e); break;
	case 0x1FC: case 0x1FD: ctr_save(p, c, k, e); c->phs[k] = v; c->phs_t[k] = e; break;
	case 0x1FE: c->vcfg = v; break;
	case 0x1FF: c->vscl = v; break;
	}
}

P8_INLINE void special_write(p8x32a * const p, const int n, const unsigned a, const uint32_t v, const uint64_t m3)
{
	p8x32a_cog * const c = &p->cog[n];
	if (a == 0x1F4 && (!p->lz_on || n != p->lz)) regset(p, n, &c->outa, v, m3 + 1);
	else special_rare(p, n, a, v, m3 + 1);
}

static uint32_t sread(p8x32a * const p, const int n, const unsigned a, const uint64_t t)
{
	const p8x32a_cog * const c = &p->cog[n];
	switch (a) {
	case 0x1F0: return (c->ptr >> 14) << 2;
	case 0x1F1: return cnt(t);
	case 0x1F2: return ina(p, t, 0xFFFFFFFFu);
	case 0x1FC: return phs_at(c, 0, t);
	case 0x1FD: return phs_at(c, 1, t);
	}
	return c->ram[a];
}

/* the P1 ALU: hub ops (group 0), rotates (1), logic (2-3), adder (4-7) */
static uint32_t alu(const unsigned i, const uint32_t s, const uint32_t d, const unsigned pc, const int run, const int ci, const int zi,
                    const uint32_t bus_q, const int bus_c, int * const wr, int * const co, int * const zo)
{
	const unsigned g = (i >> 3) & 7;
	uint32_t r, log_r = s;

	*wr = 1;
	if (g == 0) {
		r = (run || (pc >> 4) != 31) ? bus_q : 0;
		*co = bus_c;
	} else if (g == 1) {
		const uint32_t dr = (i & 1) ? __brev(d) : 0;
		uint32_t fill;
		uint64_t rot;
		switch (i & 7) {
		case 0: fill = d & 0x7FFFFFFFu; break;
		case 1: fill = dr & 0x7FFFFFFFu; break;
		case 4: case 5: fill = ci ? 0x7FFFFFFFu : 0; break;
		case 6: fill = (d >> 31) ? 0x7FFFFFFFu : 0; break;
		default: fill = 0; break;
		}
		rot = (((uint64_t)fill << 32) | ((i & 1) ? dr : d)) >> (s & 31);
		r = (((i >> 1) & 3) != 3 && (i & 1)) ? __brev((uint32_t)rot) : (uint32_t)rot;
		*co = (((i >> 1) & 3) != 3 && (i & 1)) ? (int)(dr & 1) : (int)(d & 1);
	} else {
		if (g <= 3) {
			if (i & 8) {
				const unsigned ls = (i & 4) ? ((unsigned)(((i & 2) ? zi : ci) ^ (int)(i & 1))) << 1
				                      : (((i >> 1) & 1) << 1) | (unsigned)!(((i >> 1) ^ i) & 1);
				switch (ls) {
				case 0: log_r = d & ~s; break;
				case 1: log_r = d & s; break;
				case 2: log_r = d | s; break;
				default: log_r = d ^ s; break;
				}
			} else if (i & 4) {
				switch (i & 3) {
				case 0: log_r = (d & 0xFFFFFE00u) | (s & 511); break;
				case 1: log_r = (d & 0xFFFC01FFu) | ((s & 511) << 9); break;
				case 2: log_r = ((s & 511) << 23) | (d & 0x007FFFFFu); break;
				default: log_r = (d & 0xFFFFFE00u) | (pc & 511); break;
				}
			}
		}
		if (g == 3) {
			r = log_r;
			*co = (int)parity_32(log_r);
		} else {
			uint32_t add_d, add_s, sum_lo, add_r;
			int add_sub, add_ci, add_co, add_cm, add_cs, add_c, cin, c30, b31;
			if (g == 4 || g == 5) {
				int ads[4];
				ads[0] = 0; ads[1] = (int)(s >> 31); ads[2] = ci; ads[3] = zi;
				add_sub = ads[(i >> 1) & 3] ^ (int)(i & 1);
			} else if (i == 0x32 || i == 0x34 || i == 0x36 || (i >> 2) == 0xF)
				add_sub = 0;
			else
				add_sub = 1;
			add_ci = ((g == 6 && ((i & 7) == 1 || (i & 2))) && ci) || (((i >> 3) & 3) == 3 && (i & 3) == 1);
			add_d = (((i >> 3) & 3) == 1) ? 0 : d;
			add_s = ((i & 31) == 0x19 || ((i >> 1) & 15) == 0xD) ? 0xFFFFFFFFu : add_sub ? ~s : s;
			cin = add_ci ^ add_sub;
			sum_lo = (add_d & 0x7FFFFFFFu) + (add_s & 0x7FFFFFFFu) + (uint32_t)cin;
			c30 = (int)(sum_lo >> 31);
			b31 = (int)(add_d >> 31) + (int)(add_s >> 31) + c30;
			add_r = (sum_lo & 0x7FFFFFFFu) | ((uint32_t)(b31 & 1) << 31);
			add_co = b31 >> 1;
			add_cm = c30;
			add_cs = add_co ^ (int)(add_d >> 31) ^ (int)(add_s >> 31);
			if (i == 0x38) add_c = add_co;
			else if (g == 5) add_c = (int)(s >> 31);
			else if ((i & 0x20) && ((i >> 2) & 3) == 1) add_c = add_co ^ add_cm;
			else if (((i >> 1) & 15) == 8) add_c = add_cs;
			else add_c = add_co ^ add_sub;
			if ((i >> 2) == 4) *wr = (int)(i & 1) ^ ((i & 2) ? !add_co : add_cs);
			else if (i == 0x38) *wr = add_co;
			r = (i & 0x20) ? add_r : log_r;
			*co = add_c;
		}
	}
	*zo = !r && (zi || !(g == 6 && ((i & 7) == 1 || (i & 2))));
	return r;
}

/* Idle loops: a cog repeating an iteration that changes nothing sleeps until an input may change. */
#define SNAP(c) ((unsigned char *)(c) + offsetof(p8x32a_cog, ptr))

/* the common ALU ops; others go to alu(). *co is only set when wc */
static uint32_t alu_run(const unsigned i, const uint32_t s, const uint32_t d, const unsigned pc, const int ci, const int zi, const int bus_c, const int wc, int * const wr, int * const co, int * const zo)
{
	uint32_t r;
	const unsigned sh = s & 31;
	*wr = 1;
	*co = 0;
	switch (i) {
	case 0x08: r = rotr_32(d, sh); *co = (int)(d & 1); break;
	case 0x09: r = rotl_32(d, sh); *co = (int)(d >> 31); break;
	case 0x0A: r = d >> sh; *co = (int)(d & 1); break;
	case 0x0B: r = d << sh; *co = (int)(d >> 31); break;
	case 0x0C: r = sh ? d >> sh | (ci ? 0xFFFFFFFFu << (32 - sh) : 0) : d; *co = (int)(d & 1); break;
	case 0x0D: r = sh ? d << sh | (ci ? 0xFFFFFFFFu >> (32 - sh) : 0) : d; *co = (int)(d >> 31); break;
	case 0x0E: r = sar_32(d, sh); *co = (int)(d & 1); break;
	case 0x0F: r = __brev(d) >> sh; *co = (int)(d & 1); break;
	case 0x14: r = (d & 0xFFFFFE00u) | (s & 511); *co = d < s; break;
	case 0x15: r = (d & 0xFFFC01FFu) | ((s & 511) << 9); *co = d < s; break;
	case 0x16: r = ((s & 511) << 23) | (d & 0x007FFFFFu); *co = d < s; break;
	case 0x17: r = (d & 0xFFFFFE00u) | (pc & 511); *co = d < s; break;
	case 0x18: r = d & s; if (wc) *co = (int)parity_32(r); break;
	case 0x19: r = d & ~s; if (wc) *co = (int)parity_32(r); break;
	case 0x1A: r = d | s; if (wc) *co = (int)parity_32(r); break;
	case 0x1B: r = d ^ s; if (wc) *co = (int)parity_32(r); break;
	case 0x1C: r = ci ? d | s : d & ~s; if (wc) *co = (int)parity_32(r); break;
	case 0x1D: r = ci ? d & ~s : d | s; if (wc) *co = (int)parity_32(r); break;
	case 0x1E: r = zi ? d | s : d & ~s; if (wc) *co = (int)parity_32(r); break;
	case 0x1F: r = zi ? d & ~s : d | s; if (wc) *co = (int)parity_32(r); break;
	case 0x20: r = d + s; *co = r < d; break;
	case 0x21: r = d - s; *co = d < s; break;
	case 0x28: r = s; *co = (int)(s >> 31); break;
	case 0x30: r = d - s; *co = (int32_t)d < (int32_t)s; break;
	case 0x39: r = d - 1; *co = !d; break;
	case 0x3A: case 0x3B: r = d; break;
	default: return alu(i, s, d, pc, 1, ci, zi, 0, bus_c, wr, co, zo);
	}
	*zo = !r;
	return r;
}

static void loop_reset(p8x32a_loop * const l)
{
	l->state = LOOP_SEARCH;
	l->edge = l->dirty = l->hub = 0;
	l->head = 0xFFFF;
	l->nins = 0;
}

/* wake the sleepers whose inputs a pin change at t may touch */
static void loop_notify(p8x32a * const p, const uint64_t t, const uint32_t pins)
{
	int n;
	for (n = 0; n < 8; n++)
		if ((p->sleepers >> n & 1) && (pins & p->loop[n].wake) && t < p->cog[n].ev_t) { p->cog[n].ev_t = t; p->sched_gen++; }
}

static void loop_hub_write(p8x32a * const p, const int writer, const unsigned a, const unsigned sz, const uint64_t h)
{
	int n, k;
	for (n = 0; n < 8; n++) {
		const p8x32a_loop * const l = &p->loop[n];
		if (!(p->sleepers >> n & 1) || n == writer) continue;
		for (k = 0; k < l->nhub; k++)
			if (a < (unsigned)l->hub_a[k] + l->hub_n[k] && l->hub_a[k] < a + sz && h < p->cog[n].ev_t) { p->cog[n].ev_t = h; p->sched_gen++; }
	}
}

static void loop_hub_access(p8x32a * const p, const int n, const uint32_t a, const unsigned op)
{
	p8x32a_loop * const l = &p->loop[n];
	const unsigned sz = op == 2 ? 4 : op == 1 ? 2 : 1;
	if (l->nhub == 4) { l->dirty = 1; return; }
	l->hub_a[l->nhub] = (uint16_t)(a & ~(sz - 1));
	l->hub_n[l->nhub] = (uint8_t)sz;
	l->hub_v[l->nhub] = sz == 4 ? rd32(p, a) : sz == 2 ? (uint32_t)(p->hub[a & ~1u] | p->hub[(a & ~1u) + 1] << 8) : p->hub[a];
	l->nhub++;
	l->hub = 1;
}

static uint32_t loop_hub_now(const p8x32a * const p, const p8x32a_loop * const l, const int k)
{
	const unsigned a = l->hub_a[k];
	return l->hub_n[k] == 4 ? rd32(p, a) : l->hub_n[k] == 2 ? (uint32_t)(p->hub[a] | p->hub[a + 1] << 8) : p->hub[a];
}

/* do the sleeper's inputs at t still read as recorded? */
static int loop_same(p8x32a * const p, const int n, const uint64_t t)
{
	const p8x32a_loop * const l = &p->loop[n];
	int k;
	if (l->nin) {
		uint32_t v, m = 0;
		for (k = 0; k < l->nin; k++) m |= l->in_mask[k];
		v = ina(p, t, m);
		for (k = 0; k < l->nin; k++)
			if ((v & l->in_mask[k]) != l->in_val[k]) return 0;
	}
	for (k = 0; k < l->nhub; k++) {
		CHK_READ(l->hub_a[k], l->hub_n[k], t);
		if (loop_hub_now(p, l, k) != l->hub_v[k]) return 0;
	}
	return 1;
}

/* the next time a pin the sleeper reads may change, or the horizon */
static uint64_t loop_bound(p8x32a * const p, const int n, const uint64_t t)
{
	const uint32_t m = p->loop[n].wake;
	uint64_t w = p->horizon == P8X32A_NEVER ? P8X32A_NEVER : p->horizon + 1, e;
	int k;
	if (m) {
		for (k = 0; k < p->npend; k++)
			if (p->pend[k] > t && p->pend[k] < w && (p->pend_pins[k] & m)) w = p->pend[k];
		/* only counters in nco_list drive pins */
		for (k = 0; k < p->nco_n; k++) {
			const int j = p->nco_list[k] & 1;
			const p8x32a_cog * const c = &p->cog[p->nco_list[k] >> 1];
			if (!((nco_pins(c->ctr[j]) | (t < c->ctr_at[j] ? nco_pins(c->ctr_old[j]) : 0)) & m)) continue;
			if ((e = ctr_toggle(c, j, t)) > t && e < w) w = e;
		}
		if (p->bus.pins_next && (e = p->bus.pins_next(p->bus.ctx, t)) < w) w = e;
	}
	return w > t ? w : t + 1;
}

/* wake a sleeper at w */
static void loop_resume(p8x32a * const p, const int n, const uint64_t w)
{
	p8x32a_cog * const c = &p->cog[n];
	p8x32a_loop * const l = &p->loop[n];
	const uint64_t rel = w > l->t0 ? w - l->t0 : 0, off = rel % l->period;
	uint64_t k = rel / l->period, shift;
	int j = 0;
	while (j < l->nsnap && l->snap_t[j] - l->snap_t[0] < off) j++;
	if (j == l->nsnap) { j = 0; k++; }
	shift = l->t0 - l->snap_t[0] + k * l->period;
	memcpy(SNAP(c), l->snap[j], P8X32A_SNAP);
	p->pins_ok = 0;
	c->ev_t += shift;
	c->t0 += shift;
	if (l->hub) c->latch += shift;
	p->sleepers &= (uint8_t)~(1u << n);
	loop_reset(l);
}

static void loop_wake(p8x32a * const p, const int n)
{
	const uint64_t t = p->cog[n].ev_t;
	if (loop_same(p, n, t)) p->cog[n].ev_t = loop_bound(p, n, t);
	else loop_resume(p, n, t);
}

static void loop_in(p8x32a * const p, const int n, const unsigned op, const uint32_t s, const uint32_t d)
{
	p8x32a_loop * const l = &p->loop[n];
	const uint32_t mask = (op == 0x18 || op == 0x19) ? d : 0xFFFFFFFFu;
	if (l->nin == 4) { l->dirty = 1; return; }
	l->in_mask[l->nin] = mask;
	l->in_val[l->nin++] = s & mask;
}

/* a taken backward jump to head at time t */
static void loop_edge(p8x32a * const p, const int n, const unsigned head, const uint64_t t)
{
	p8x32a_loop * const l = &p->loop[n];
	if (l->state == LOOP_RECORD) {
		if (head == l->head || l->nins > P8X32A_PAT / 2) { l->edge = 1; l->edge_head = (uint16_t)head; l->edge_t = t; }
		return;
	}
	if (head == l->head && !l->dirty && l->nins <= P8X32A_PAT / 2 && t > l->head_t && !(p->lz_on && n == p->lz)) {
		l->state = LOOP_RECORD;
		l->period = t - l->head_t;
		l->nins0 = l->nins;
		l->nsnap = l->nin = l->nhub = 0;
		l->edge = 0;
	} else if (head != l->head && !l->dirty && l->nins <= P8X32A_PAT / 2 && l->head != 0xFFFF)
		return;
	l->head = (uint16_t)head;
	l->head_t = t;
	l->dirty = l->hub = 0;
	l->nins = 0;
}

/* after each event of a recording cog */
static void loop_post(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	p8x32a_loop * const l = &p->loop[n];
	const uint64_t per = l->period;
	uint32_t m = 0;
	int k, same;

	if (!l->edge) {
		if (l->dirty || l->nsnap == P8X32A_PAT || c->ev == EV_NONE || c->ev == EV_RESTART) { loop_reset(l); return; }
		l->snap_t[l->nsnap] = c->ev_t;
		memcpy(l->snap[l->nsnap++], SNAP(c), P8X32A_SNAP);
		return;
	}
	l->edge = 0;
	if (l->edge_head != l->head || l->dirty || l->edge_t - l->head_t != per || l->nins != l->nins0 ||
	    (l->hub && per % 16) || !l->nsnap || c->ev != EV_EXEC) {
		loop_reset(l);
		return;
	}
	c->ev_t -= per;
	c->t0 -= per;
	if (l->hub) c->latch -= per;
	same = !memcmp(SNAP(c), l->snap[0], P8X32A_SNAP);
	c->ev_t += per;
	c->t0 += per;
	if (l->hub) c->latch += per;
	if (!same || !loop_same(p, n, p->now)) { loop_reset(l); return; }
	for (k = 0; k < l->nin; k++) m |= l->in_mask[k];
	l->wake = (m & ~p->bus.pure_in) ? 0xFFFFFFFFu : m;
	/* the lazy cog's pin changes are not pending points */
	if (p->lz_on && (l->wake & p->lz_pins)) { loop_reset(l); return; }
	l->t0 = c->ev_t;
	l->state = LOOP_SLEEP;
	p->sleepers |= (uint8_t)(1u << n);
	p->sleeps++;
	c->ev = EV_SLEEP;
	c->ev_t = loop_bound(p, n, p->now);
}

static void idle(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	if (c->restart_at != P8X32A_NEVER) { c->ev = EV_RESTART; c->ev_t = c->restart_at + 2; }
	else c->ev = EV_NONE;
}

static void next_instr(p8x32a * const p, const int n, const uint64_t t0)
{
	p8x32a_cog * const c = &p->cog[n];
	if (t0 >= c->disable_at) { idle(p, n); return; }
	c->t0 = t0;
	c->ev = EV_EXEC;
	c->ev_t = t0 + 2;
}

static void complete(p8x32a * const p, const int n, const uint64_t m3, const uint32_t q, const int bus_c)
{
	p8x32a_cog * const c = &p->cog[n];
	const uint32_t i = c->i;
	uint32_t r;
	const unsigned op = OP(i);
	int wr, co, zo, jc = 0;

	if (op <= 3) {
		/* alu() for group 0: the hub result */
		r = (c->run || (c->p >> 4) != 31) ? q : 0;
		wr = 1;
		co = bus_c;
		zo = !r;
	} else
		r = alu(op, c->s, c->d, c->p, c->run, c->c, c->z, q, bus_c, &wr, &co, &zo);
	if (c->cond) {
		if (FWR(i)) {
			if (wr && c->ram[DST(i)] != r) { const uint32_t o = c->ram[DST(i)]; c->ram[DST(i)] = r; p->loop[n].dirty = 1; jit_write(p, n, DST(i), o); }
			if (DST(i) >= 0x1F0) { special_write(p, n, DST(i), r, m3); p->loop[n].dirty = 1; }
		}
		if (FWC(i)) c->c = (uint8_t)co;
		if (FWZ(i)) c->z = (uint8_t)zo;
	}
	if (c->cond) {
		const int dz = !(c->d >> 1);
		if (op == 0x39) jc = dz && (c->d & 1);
		else if (op == 0x3A) jc = dz && !(c->d & 1);
		else if (op == 0x3B) jc = !(dz && !(c->d & 1));
	}
	p->loop[n].nins++;
	if (c->cond && !jc && (op == 0x17 || (op >= 0x39 && op <= 0x3B)) && c->px < c->p) loop_edge(p, n, c->px, m3);
	if (!jc) c->p = (uint16_t)((c->px + 1) & 511);
	c->cancel = (uint8_t)(jc || c->px == 511);
	if (c->px == 511) c->run = 1;
	c->ix = c->nix;
	next_instr(p, n, m3 + 1);
}

static uint64_t next_slot(const p8x32a * const p, const int n, const uint64_t from)
{
	const uint64_t base = p->slot_base + 3 + 2 * (uint64_t)n;
	if (from <= base) return base;
	return base + ((from - base + 15) / 16) * 16;
}

static void stop_cog(p8x32a * const p, const int n, const uint64_t d)
{
	p8x32a_cog * const c = &p->cog[n];
	if (d < c->disable_at) c->disable_at = d;
	regset(p, n, &c->dira, 0, d);
	ctr_save(p, c, 0, d);
	ctr_save(p, c, 1, d);
	ctr_rebase(c, 0, d);
	ctr_rebase(c, 1, d);
	c->ctr[0] = c->ctr[1] = 0;
	ctr_mask(p, n, 0);
	ctr_mask(p, n, 1);
	ctr_notify(p, n, 0, d);
	ctr_notify(p, n, 1, d);
}

static void sys(p8x32a * const p, const int n, const uint64_t h)
{
	const p8x32a_cog * const c = &p->cog[n];
	const uint32_t dc = c->d;
	const unsigned op = c->s & 7;
	unsigned num, newx = 0;
	const uint8_t enc = (op & 4) ? p->lock_e : p->cog_e;
	uint8_t bit;
	const int all = enc == 0xFF;
	int old = 0;

	if (op >= 2) CHK_SYS(op >= 6 ? CHK_LOCK + (dc & 7) : op >= 4 ? CHK_LOCKE : CHK_COGE, h);
	p->sched_gen++;
	while (newx < 7 && (enc >> newx & 1)) newx++;
	num = ((op == 2 && (dc & 8)) || op == 4) ? newx : (dc & 7);
	bit = (uint8_t)(1u << num);
	if (p->lz_on && (op == 2 || op == 3) && num == p->lz) lazy_exit(p);
	if (op == 2 || op == 3) {
		if (p->sleepers >> num & 1) loop_resume(p, (int)num, h);
		loop_reset(&p->loop[num]);
	}
	switch (op) {
	case 0:
		p->cfg = (uint8_t)dc;
		/* send the lazy cog's earlier pin changes before the host retimes */
		lazy_catch(p, h);
		if (p->bus.clkset) p->bus.clkset(p->bus.ctx, h + 1, p->cfg);
		/* the host may retime its queued edges: sleepers re-check */
		if (p->sleepers) loop_notify(p, h + 1, 0xFFFFFFFFu);
		if (dc & 0x80) log_once(p, LOG_REBOOT, "p8x32a: CLKSET reset bit, the restart left to the host");
		break;
	case 2:
		if (!((dc & 8) && all)) {
			p8x32a_cog * const t = &p->cog[num];
			t->ptr = dc >> 4;
			if (p->bus.cog_start) p->bus.cog_start(p->bus.ctx, h, (int)num, t->ptr);
			p->lz_try[num] = p->lz_wait[num] = 0;
			stop_cog(p, (int)num, h + 1);
			t->restart_at = h + 4;
			if (!(t->ev == EV_HUB && t->latch <= h) && (int)num != n) idle(p, (int)num);
		}
		p->cog_e |= bit;
		break;
	case 3:
		p->cog_e &= (uint8_t)~bit;
		if (p->cog[num].ev != EV_NONE || p->cog[num].restart_at != P8X32A_NEVER) {
			stop_cog(p, (int)num, h + 3);
			if (p->cog[num].restart_at != P8X32A_NEVER) {
				p->cog[num].restart_at = P8X32A_NEVER;
				if (p->cog[num].ev == EV_RESTART) p->cog[num].ev = EV_NONE;
			}
		}
		break;
	case 4: p->lock_e |= bit; break;
	case 5: p->lock_e &= (uint8_t)~bit; break;
	case 6: old = p->lock_state >> (dc & 7) & 1; p->lock_state |= (uint8_t)(1u << (dc & 7)); break;
	case 7: old = p->lock_state >> (dc & 7) & 1; p->lock_state &= (uint8_t)~(1u << (dc & 7)); break;
	}
	p->sys_q = (uint8_t)(op == 1 ? (unsigned)n : num);
	p->sys_c = (uint8_t)(op >= 6 ? old : all);
}

static void do_hub(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	const uint64_t h = c->ev_t, m3 = c->latch + 4;
	uint32_t q, a, w;
	const unsigned op = OP(c->i);

	if (c->latch >= c->disable_at) { idle(p, n); return; }
	if (op == 3) {
		sys(p, n, h);
		q = p->sys_q;
	} else {
		a = c->run ? (c->s & 0xFFFF) : (((((c->ptr & 0x3FFF) + c->p) & 0x3FFF) << 2) | (c->s & 3));
		w = rd32(p, a);
		if (!c->run && (a & 0x8000)) w = unscramble(w);
		if (FWR(c->i) || !c->run) CHK_READ(c->run ? a & ~((op == 2 ? 4u : op == 1 ? 2u : 1u) - 1) : a & 0xFFFC, c->run ? (op == 2 ? 4 : op == 1 ? 2 : 1) : 4, h);
		if (!FWR(c->i) && a < 0x8000) {
			const uint32_t v = c->d;
			const unsigned sz = op == 2 ? 4 : op == 1 ? 2 : 1, ha = a & ~(sz - 1);
			CHK_WRITE(ha, sz, h);
			if (p->lz_on) lazy_write(p, ha, sz, v, h);
			if (hub_store(p, ha, sz, v)) {
				p->loop[n].dirty = 1;
				if (p->sleepers) loop_hub_write(p, n, ha, sz, h);
			}
		}
		q = op == 2 ? w : op == 1 ? (w >> ((a & 2) * 8)) & 0xFFFF : (w >> ((a & 3) * 8)) & 0xFF;
		if (p->loop[n].state == LOOP_RECORD) loop_hub_access(p, n, a, op);
	}
	if (m3 >= c->disable_at) { idle(p, n); return; }
	complete(p, n, m3, q, p->sys_c);
}

/* a pin change at t wakes the cogs waiting on those pins; on any pin, those waiting on an input that is not pure (one a device may change in answer to an output) */
static void wait_notify(p8x32a * const p, const uint64_t t, const uint32_t pins)
{
	int n;
	for (n = 0; n < 8; n++) {
		p8x32a_cog * const c = &p->cog[n];
		if (!(p->waiters >> n & 1)) continue;
		if (c->ev != EV_WAITPIN) { p->waiters &= (uint8_t)~(1u << n); continue; }
		if ((pins & ((c->s & ~p->bus.pure_in) ? 0xFFFFFFFFu : c->s)) && t < c->ev_t) { c->ev_t = t; p->sched_gen++; }
	}
}

/* WAITPEQ/WAITPNE: done, or wait for the next change it can see coming (pending points, counters, inputs, the slice end); another cog's pin change wakes it earlier (wait_notify) */
static void wait_pins(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	const uint64_t t = c->ev_t;
	uint64_t nt = P8X32A_NEVER;
	int k, match;

	if (t + 2 >= c->disable_at) { idle(p, n); return; }
	if (p->lz_on && (c->s & p->lz_pins)) lazy_exit(p);
	match = ((ina(p, t, c->s) & c->s) == c->d) ^ (OP(c->i) == 0x3D);
	if (match) { c->ev = EV_DONE; c->ev_t = t + 2; return; }
	for (k = 0; k < p->npend; k++)
		if (p->pend[k] > t && p->pend[k] < nt) nt = p->pend[k];
	{
		const uint64_t ct = ctr_next(p, t);
		if (ct < nt) nt = ct;
	}
	if (p->bus.pins_next) {
		const uint64_t e = p->bus.pins_next(p->bus.ctx, t);
		if (e < nt) nt = e;
	}
	if (p->horizon != P8X32A_NEVER && nt > p->horizon + 1) nt = p->horizon + 1;
	if (nt <= t) nt = t + 1;
	c->ev_t = nt;
}

static void exec(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	const uint64_t t2 = c->ev_t, t0 = t2 - 2;
	const uint32_t i = c->run ? c->ix : ((0x02u << 26) | (1u << 23) | (1u << 18) | ((uint32_t)c->p << 9));
	const unsigned op = OP(i);
	const int jump = op == 0x17 || op == 0x39 || op == 0x3A || op == 0x3B;

	c->i = i;
	c->cond = (uint8_t)(((COND(i) >> ((c->c << 1) | c->z)) & 1) && !c->cancel);
	c->s = FIM(i) ? SRC(i) : SRC(i) == 0x1F2 ? ina(p, t2, (op == 0x18 || op == 0x19) ? c->ram[DST(i)] : 0xFFFFFFFFu) : sread(p, n, SRC(i), t2);
	c->d = c->ram[DST(i)];
	if (!c->run || (c->cond && (op == 3 || op >= 0x3C)) || (!FIM(i) && (SRC(i) == 0x1F1 || SRC(i) == 0x1FC || SRC(i) == 0x1FD)))
		p->loop[n].dirty = 1;
	else if (!FIM(i) && SRC(i) == 0x1F2 && p->loop[n].state == LOOP_RECORD)
		loop_in(p, n, op, c->s, c->d);
	c->px = (uint16_t)((c->cond && jump) ? (c->s & 511) : c->p);
	c->nix = c->ram[c->px];
	if (c->cond && op <= 3) {
		c->latch = next_slot(p, n, t0 + 3);
		c->ev = EV_HUB;
		c->ev_t = c->latch + 2;
		return;
	}
	if (c->cond && op == 0x3E) {
		const uint64_t m = t0 + 3;
		c->ev = EV_DONE;
		c->ev_t = m + (uint32_t)(c->d - cnt(m)) + 2;
		return;
	}
	if (c->cond && (op == 0x3C || op == 0x3D)) {
		c->ev = EV_WAITPIN;
		p->waiters |= (uint8_t)(1u << n);
		c->ev_t = t0 + 3;
		return;
	}
	if (c->cond && op == 0x3F) log_once(p, LOG_WAITVID, "p8x32a: WAITVID not modelled");
	if (t0 + 3 >= c->disable_at) { idle(p, n); return; }
	complete(p, n, t0 + 3, 0, p->sys_c);
}

static void restart(p8x32a * const p, const int n)
{
	p8x32a_cog * const c = &p->cog[n];
	loop_reset(&p->loop[n]);
	/* new code: drop the old blocks and the changed-word history */
	if (p->jit_build) {
		jit_drop(p, n);
		memset(p->jvar[n], 0, sizeof(p->jvar[n]));
	}
	c->p = 0;
	c->c = c->z = c->cancel = c->run = 0;
	c->disable_at = P8X32A_NEVER;
	c->restart_at = P8X32A_NEVER;
	exec(p, n);
}

/* an instruction touching nothing other cogs or the pins can see */
static int local(const p8x32a_cog * const c)
{
	const uint32_t i = c->ix;
	const unsigned op = OP(i);
	if (!c->run || op <= 3 || op >= 0x3C) return 0;
	if (!FIM(i) && SRC(i) == 0x1F2) return 0;
	return !(FWR(i) && DST(i) >= 0x1F0);
}

/* a hub op or wait whose issue reads nothing shared, so exec() may run ahead */
static int issue_local(const p8x32a_cog * const c)
{
	const uint32_t i = c->ix;
	const unsigned op = OP(i);
	if (!c->run || (op > 3 && (op < 0x3C || op > 0x3E))) return 0;
	return FIM(i) || SRC(i) < 0x1F0;
}

enum { K_NL, K_GEN, K_JMP, K_DJNZ, K_TJ, K_AND, K_ANDN, K_OR, K_XOR, K_ADD, K_SUB, K_MOV, K_SHL, K_SHR, K_MOVS, K_MOVD, K_MOVI };
enum { F_IMM = 1, F_SPEC = 2, F_WR = 4, F_WC = 8, F_WZ = 16, F_INA = 32, F_OUTA = 64, F_HUBRD = 128 };

/* decode for run_local: K_NL if not local, F_INA for an INA source */
static void dec_fill(p8x32a_dec * const e, const uint32_t i)
{
	const unsigned op = OP(i);
	static const uint8_t kinds[64] = {
		K_NL, K_NL, K_NL, K_NL, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_SHR, K_SHL, K_GEN, K_GEN, K_GEN, K_GEN,
		K_GEN, K_GEN, K_GEN, K_GEN, K_MOVS, K_MOVD, K_MOVI, K_JMP, K_AND, K_ANDN, K_OR, K_XOR, K_GEN, K_GEN, K_GEN, K_GEN,
		K_ADD, K_SUB, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_MOV, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN,
		K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_GEN, K_DJNZ, K_TJ, K_TJ, K_NL, K_NL, K_NL, K_NL
	};
	e->word = i;
	e->src = (uint16_t)SRC(i);
	e->dst = (uint16_t)DST(i);
	e->cond = (uint8_t)COND(i);
	e->kind = kinds[op];
	e->fl = (uint8_t)((FIM(i) ? F_IMM : (SRC(i) >= 0x1F0 ? F_SPEC : 0)) | (FWR(i) ? F_WR : 0) | (FWC(i) ? F_WC : 0) | (FWZ(i) ? F_WZ : 0));
	if (!FIM(i) && SRC(i) == 0x1F2) e->fl |= F_INA;
	if (FWR(i) && DST(i) >= 0x1F0) e->kind = K_NL;
	if (FWR(i) && DST(i) == 0x1F4) e->fl |= F_OUTA;
	if (op <= 2 && FWR(i) && !FWC(i)) e->fl |= F_HUBRD;
	/* as p8x32a_jit_build takes it */
	e->jh = (uint8_t)(op <= 2 && !FWC(i) && (FIM(i) || SRC(i) <= 0x1F0) && (!FWR(i) || DST(i) < 0x1F0));
}

/* do_hub + complete for cog n's hub op i at h; returns fl (Z bit 0, C bit 1) */
static unsigned hub_rw(p8x32a * const p, const int n, const uint32_t i, const uint32_t s, const uint32_t d, const uint64_t h, const uint64_t m3, unsigned fl)
{
	p8x32a_loop * const l = &p->loop[n];
	uint32_t * const ram = p->cog[n].ram;
	const uint32_t a = s & 0xFFFF, w = p->jn && p->lz_on && n == p->lz ? jn_rd32(p, a & 0xFFFC, h) : rd32(p, a);
	uint32_t r;
	const unsigned op = OP(i), dst = DST(i);
	if (FWR(i) && !(p->lz_on && n == p->lz)) CHK_READ(a & ~((op == 2 ? 4u : op == 1 ? 2u : 1u) - 1), op == 2 ? 4 : op == 1 ? 2 : 1, h);
	if (!FWR(i) && a < 0x8000) {
		const unsigned sz = op == 2 ? 4 : op == 1 ? 2 : 1, ha = a & ~(sz - 1);
		CHK_WRITE(ha, sz, h);
		if (p->lz_on) lazy_write(p, ha, sz, d, h);
		if (hub_store(p, ha, sz, d)) {
			l->dirty = 1;
			if (p->sleepers) loop_hub_write(p, n, ha, sz, h);
		}
	}
	r = op == 2 ? w : op == 1 ? (w >> ((a & 2) * 8)) & 0xFFFF : (w >> ((a & 3) * 8)) & 0xFF;
	if (FWR(i)) {
		if (ram[dst] != r) { const uint32_t o = ram[dst]; ram[dst] = r; l->dirty = 1; jit_write(p, n, dst, o); }
		if (dst >= 0x1F0) { special_write(p, n, dst, r, m3); l->dirty = 1; }
	}
	if (FWC(i)) fl = (fl & ~2u) | (unsigned)p->sys_c << 1;
	if (FWZ(i)) fl = (fl & ~1u) | (unsigned)!r;
	return fl;
}

/* run cog n's event instruction inline if the scheduler would take it next; 0 if it must wait */
P8_INLINE int event_run(p8x32a * const p, const int n, const p8x32a_dec * const e, const uint32_t ix, const unsigned pc, unsigned * const fl, uint64_t * const t2, uint32_t * const nix, const uint64_t t, const uint64_t lim, const unsigned gen)
{
	p8x32a_cog * const c = &p->cog[n];
	p8x32a_loop * const l = &p->loop[n];
	uint32_t * const ram = c->ram;
	uint32_t s, d, r;
	const unsigned op = OP(ix);
	unsigned f = *fl;
	const uint64_t now = *t2;

	if (p->sched_gen != gen || p->stop || l->state != LOOP_SEARCH || !((e->cond >> f) & 1)) return 0;
	if (op <= 2) {
		const uint64_t latch = next_slot(p, n, now + 1), h = latch + 2, m3 = latch + 4;
		if ((e->fl & F_INA) || h > t || (h << 4 | (uint64_t)n) >= lim || m3 + 1 >= c->disable_at) return 0;
		if (e->fl & F_IMM) s = e->src;
		else if (!(e->fl & F_SPEC)) s = ram[e->src];
		else {
			s = sread(p, n, e->src, now);
			if (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD) l->dirty = 1;
		}
		*nix = ram[pc];
		p->now = h;
		c->latch = latch;
		f = hub_rw(p, n, ix, s, ram[e->dst], h, m3, f);
		*t2 = m3 + 3;
	} else {
		int wr, ci, zi;
		if (op == 3 || op > 0x3B || op == 0x17 || op >= 0x39) return 0;
		if (((now << 4) | 8 | (uint64_t)n) >= lim || now + 2 >= c->disable_at) return 0;
		p->now = now;
		if (e->fl & F_IMM) s = e->src;
		else if (e->fl & F_INA) s = ina(p, now, (op == 0x18 || op == 0x19) ? ram[e->dst] : 0xFFFFFFFFu);
		else if (!(e->fl & F_SPEC)) s = ram[e->src];
		else {
			s = sread(p, n, e->src, now);
			if (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD) l->dirty = 1;
		}
		d = ram[e->dst];
		*nix = ram[pc];
		r = alu_run(op, s, d, pc, (int)(f >> 1 & 1), (int)(f & 1), p->sys_c, (e->fl & F_WC) != 0, &wr, &ci, &zi);
		if ((e->fl & F_WR) && wr && ram[e->dst] != r) { const uint32_t o = ram[e->dst]; ram[e->dst] = r; l->dirty = 1; jit_write(p, n, e->dst, o); }
		if ((e->fl & F_WR) && e->dst >= 0x1F0) { special_write(p, n, e->dst, r, now + 1); l->dirty = 1; }
		if (e->fl & F_WC) f = (f & ~2u) | (unsigned)ci << 1;
		if (e->fl & F_WZ) f = (f & ~1u) | (unsigned)zi;
		*t2 = now + 4;
	}
	*fl = f;
	return 1;
}

/* code slot s of cog n changed: drop its blocks, accumulate the changed bits in jvar */
static void jit_written(p8x32a * const p, const int n, const unsigned s, const uint32_t old)
{
	int k;
	p->jvar[n][s] |= old ^ p->cog[n].ram[s];
	for (k = (int)s; k >= 0 && k > (int)s - P8X32A_JMAX; k--) {
		const p8x32a_jblk * const b = p->jblk[n][k];
		if (b && b->valid && (unsigned)k + b->len > s && !(b->dyn >> (s - (unsigned)k) & 1)) jit_void(p, n, (unsigned)k);
	}
	p->jcode[n][s >> 3] &= (uint8_t)~(1u << (s & 7));
}


/* a translated block's closing hub op, as event_run runs it */
static uint32_t jit_hub(p8x32a_jst * const st)
{
	p8x32a * const p = (p8x32a *)st->chip;
	const int n = (int)st->n;
	p->now = st->latch + 2;
	p->cog[n].latch = st->latch;
	return hub_rw(p, n, st->hi, st->hs, st->hd, st->latch + 2, st->latch + 4, st->fl);
}

#ifdef P8X32A_CHECK
static void jit_chk(p8x32a_jst * const st)
{
	CHK_READ(st->ca, st->csz, st->latch + 2);
}
#endif

/* the translated block for ix at cog address a */
P8_INLINE p8x32a_jblk *jit_get(p8x32a * const p, const int n, const unsigned a, const uint32_t ix)
{
	p8x32a_jblk *b = p->jblk[n][a];
	uint32_t * const var = p->jvar[n];
	unsigned k;
	if (var[a] & ~P8X32A_JDYN) { p->jit_refused++; return NULL; }
	if (b && b->valid) {
		const uint32_t x = (b->dyn & 1) ? (ix ^ b->words[0]) & ~P8X32A_JDYN : ix ^ b->words[0];
		if (!x) return b;
		var[a] |= x;
		if (var[a] & ~P8X32A_JDYN) { p->jit_refused++; return NULL; }
	}
	/* retranslating frees the old code and voids its resume entries */
	if (b) p->jep[n]++;
	b = p->jit_build(p->jit, b, a, ix, p->cog[n].ram, var, p->lz_on && n == p->lz);
	p->jblk[n][a] = b;
	if (b) {
		p8x32a_jlink * const ln = &p->jlink[n][a];
		ln->body = b->body;
		ln->word = b->words[0];
		ln->mask = (b->dyn & 1) ? ~P8X32A_JDYN : ~0u;
		ln->len = b->valid && b->len && b->fn ? b->len : ~0u;
		ln->part = b->part;
		ln->fn = b->fn;
	}

	if (b && b->valid)
		for (k = 0; k < b->len; k++)
			if (!(b->dyn >> k & 1)) p->jcode[n][(a + k) >> 3] |= (uint8_t)(1u << ((a + k) & 7));
	return b;
}

/* exec + complete for cog n's local instructions until one is not local or t is passed; 0 if the cog did not move */
static int run_local(p8x32a * const p, const int n, const uint64_t t, const uint64_t lim, const unsigned gen)
{
	p8x32a_cog * const c = &p->cog[n];
	p8x32a_loop * const l = &p->loop[n];
	p8x32a_dec * const dec = p->dec[n];
	p8x32a_dec *e;
	uint32_t * const ram = c->ram;
	uint64_t t2 = c->ev_t;
	const uint64_t dis = c->disable_at, tl = dis < 2 ? 0 : dis - 2 < t ? dis - 2 : t;
	uint32_t ix = c->ix;
	unsigned pc = c->p, fl = (unsigned)(c->c << 1 | c->z) | (unsigned)c->cancel << 2, nins = 0;
	int idled = 0, done = 0;
	const uint64_t t2in = t2;
	uint64_t hub = 0;
	p8x32a_jst st;
	unsigned px = 0, jc = 0;
	p8x32a_jblk *lb = NULL;

	st.ram = ram;
	st.code = p->jcode[n];
	st.tab = p->jblk[n];
	st.link = p->jlink[n];
	st.loop = l;
	st.par = (c->ptr >> 14) << 2;
	st.t = t;
	st.lim = p->stop || l->state != LOOP_SEARCH ? 0 : lim;
	st.dis = dis;
	st.pgen = &p->sched_gen;
	st.gen = gen;
	st.n = (unsigned)n;
	st.chip = p;
	st.hubfn = jit_hub;
	st.pnow = &p->now;
#ifdef P8X32A_CHECK
	st.chkfn = jit_chk;
#else
	st.chkfn = NULL;
#endif
	st.tl = tl;
	st.slot = p->slot_base + 3 + 2 * (uint64_t)n;
	st.hub = p->hub;

	if (c->ev == EV_HUB) {
		const uint64_t m3 = c->latch + 4;
		if (OP(c->i) > 2 || m3 + 1 >= dis) return 0;
		done = 1;
		/* the block that issued this hub op completes it and runs on */
		if (p->jres[n] && p->jres_ep[n] == p->jep[n] && p->jres_i[n] == c->i && p->jres_px[n] == c->px && m3 + 3 <= tl &&
		    !(p->lz_on && n == p->lz)) {
			st.hs = c->s;
			st.hd = c->d;
			st.latch = c->latch;
			st.nix = c->nix;
			pc = c->px;
			ix = c->i;
			e = &dec[(pc - 1) & 511];
			goto resume;
		}
		fl = hub_rw(p, n, c->i, c->s, c->d, t2, m3, fl);
		nins++;
		pc = (c->px + 1) & 511;
		fl = (fl & 3) | (c->px == 511) << 2;
		ix = c->nix;
		t2 = m3 + 3;
	}
	if (t2 > tl || dis < 2) goto limit;
	for (;;) {
		uint32_t s, d, r, nix;
		px = pc;
		jc = 0;
		lb = NULL;
		e = &dec[(pc - 1) & 511];
		if (p->jit_build && !(fl & 4)) {
			/* a linked block skips the decode and lookup below */
			const p8x32a_jlink * const ln = &p->jlink[n][(pc - 1) & 511];
			if (ln->len != ~0u && !ln->part && !((ix ^ ln->word) & ln->mask) && t2 + 4 * (uint64_t)(ln->len - 1) <= tl)
				lb = p->jblk[n][(pc - 1) & 511];
		}
		if (!lb && e->word != ix) dec_fill(e, ix);
		/* event instructions are never translated */
		if (lb || (p->jit_build && !(fl & 4) && (e->kind != K_NL || (p->lz_on && n == p->lz ? e->fl & (F_OUTA | F_HUBRD) : e->jh)) && !(e->fl & F_INA))) {
			const unsigned a = (pc - 1) & 511;
			p8x32a_jblk *b = lb ? lb : jit_get(p, n, a, ix);
			unsigned k;
			if (b && b->len && (b->part ? t2 <= tl : t2 + 4 * (uint64_t)(b->len - 1) <= tl)) {
				if (0) {
					/* a resume entry (b NULL) runs first */
				resume:
					b = NULL;
				}
				l->nins = (uint16_t)(l->nins + nins);
				nins = 0;
				st.t2 = t2;
				st.budget = b && b->part ? 0x7FFFFFFFu : (uint32_t)((tl - t2) / 4 + 1);
				st.ix = ix;
				st.fl = fl & 3;
				st.inv = st.edge = st.hiss = 0;
				if (!b) {
					k = p->jres[n](&st);
					c->latch = st.latch;
				} else if (b->part) {
					st.ot = p->jot;
					st.ov = p->jov;
					st.on = 0;
					st.jmap = p->jmap;
					st.latch = 0;
					k = b->fn(&st);
				} else {
					/* slot of a hub op the block ran */
					st.latch = c->latch;
					k = b->fn(&st);
					c->latch = st.latch;
				}
				if (b && b->part) {
					unsigned j;
					if (st.latch) c->latch = st.latch;
					for (j = 0; j < st.on; j++) lazy_outa(p, st.ov[j], st.ot[j]);
					st.on = 0;
				}
			} else
				k = 0;
			if (k) {
				if (st.inv == ~0u) {
					unsigned j;
					for (j = 0; j < 512; j++)
						if (p->jblk[n][j]) jit_void(p, n, j);
					memset(p->jcode[n], 0, sizeof(p->jcode[n]));
				} else if (st.inv)
					jit_written(p, n, st.inv - 1, st.inv_old);
				fl = st.fl;
				px = st.px;
				jc = st.jc;
				nix = st.nix;
				t2 = st.t2;
				if (st.edge) {
					loop_edge(p, n, px, t2 - 3);
					if (l->state == LOOP_RECORD) {
						c->i = st.w; c->s = st.s; c->d = st.d; c->px = (uint16_t)px; c->nix = nix; c->cond = 1;
						pc = (px + 1) & 511;
						fl |= (px == 511) << 2;
						ix = nix;
						break;
					}
				}
				if (!jc) pc = (px + 1) & 511;
				else pc = st.pc;
				fl |= (jc || px == 511) << 2;
				ix = nix;
				if (st.hiss) {
					/* the block stopped before a hub op that must wait: issue it */
					c->i = ix;
					c->cond = 1;
					c->s = st.hs;
					c->d = st.hd;
					c->px = (uint16_t)pc;
					c->nix = ram[pc];
					c->latch = st.hlatch;
					hub = st.hlatch + 2;
					p->jres[n] = st.hres;
					p->jres_i[n] = ix;
					p->jres_px[n] = pc;
					p->jres_ep[n] = p->jep[n];
					break;
				}
				if (t2 > tl) {
					if (t2 - 2 >= dis) { idled = 1; break; }
					goto limit;
				}
				continue;
			}
		}
		if (lb && e->word != ix) dec_fill(e, ix);
		if (e->kind == K_NL || (e->fl & F_INA)) {
			if (!((e->cond >> fl) & 1)) {
				if (e->fl & F_INA) break;
				if ((e->fl & F_SPEC) && (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD)) l->dirty = 1;
				nix = ram[px];
				nins++;
				goto next;
			}
			if (!event_run(p, n, e, ix, pc, &fl, &t2, &nix, t, lim, gen)) {
				if (OP(ix) <= 2 && !(e->fl & F_INA) && l->state == LOOP_SEARCH) {
					/* a hub op waiting for its slot */
					c->i = ix;
					c->cond = 1;
					c->s = (e->fl & F_IMM) ? e->src : (e->fl & F_SPEC) ? sread(p, n, e->src, t2) : ram[e->src];
					if ((e->fl & F_SPEC) && (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD)) l->dirty = 1;
					c->d = ram[e->dst];
					c->px = (uint16_t)pc;
					c->nix = ram[pc];
					c->latch = next_slot(p, n, t2 + 1);
					hub = c->latch + 2;
				}
				break;
			}
			nins++;
			t2 -= 4;
			goto next;
		}
		if (e->fl & F_IMM) s = e->src;
		else if (!(e->fl & F_SPEC)) s = ram[e->src];
		else {
			s = sread(p, n, e->src, t2);
			if (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD) l->dirty = 1;
		}
		if ((e->cond >> fl) & 1) {
			unsigned co = 0, zo;
			d = ram[e->dst];
			switch (e->kind) {
			case K_JMP: px = s & 511; r = (d & 0xFFFFFE00u) | (pc & 511); co = d < s; break;
			case K_DJNZ: px = s & 511; r = d - 1; co = !d; jc = d == 1; break;
			case K_AND: r = d & s; if (e->fl & F_WC) co = parity_32(r); break;
			case K_ANDN: r = d & ~s; if (e->fl & F_WC) co = parity_32(r); break;
			case K_OR: r = d | s; if (e->fl & F_WC) co = parity_32(r); break;
			case K_XOR: r = d ^ s; if (e->fl & F_WC) co = parity_32(r); break;
			case K_ADD: r = d + s; co = r < d; break;
			case K_SUB: r = d - s; co = d < s; break;
			case K_MOV: r = s; co = s >> 31; break;
			case K_SHL: r = d << (s & 31); co = d >> 31; break;
			case K_SHR: r = d >> (s & 31); co = d & 1; break;
			case K_MOVS: r = (d & 0xFFFFFE00u) | (s & 511); co = d < s; break;
			case K_MOVD: r = (d & 0xFFFC01FFu) | ((s & 511) << 9); co = d < s; break;
			case K_MOVI: r = ((s & 511) << 23) | (d & 0x007FFFFFu); co = d < s; break;
			default: {
				int wr, ci, zi;
				const unsigned op = OP(ix);
				if (e->kind == K_TJ) px = s & 511;
				nix = ram[px];
				r = alu_run(op, s, d, pc, (int)(fl >> 1 & 1), (int)(fl & 1), p->sys_c, (e->fl & F_WC) != 0, &wr, &ci, &zi);
				co = (unsigned)ci;
				zo = (unsigned)zi;
				if (op == 0x3A) jc = !d;
				else if (op == 0x3B) jc = d != 0;
				if ((e->fl & F_WR) && wr && ram[e->dst] != r) { const uint32_t o = ram[e->dst]; ram[e->dst] = r; l->dirty = 1; jit_write(p, n, e->dst, o); }
				goto flags;
			}
			}
			nix = ram[px];
			zo = !r;
			if ((e->fl & F_WR) && ram[e->dst] != r) { const uint32_t o = ram[e->dst]; ram[e->dst] = r; l->dirty = 1; jit_write(p, n, e->dst, o); }
		flags:
			if (e->fl & F_WC) fl = (fl & ~2u) | co << 1;
			if (e->fl & F_WZ) fl = (fl & ~1u) | zo;
			nins++;
			if (!jc && px < pc && e->kind >= K_JMP && e->kind <= K_TJ) {
				l->nins = (uint16_t)(l->nins + nins);
				nins = 0;
				loop_edge(p, n, px, t2 + 1);
				if (l->state == LOOP_RECORD) {
					c->i = ix; c->s = s; c->d = d; c->px = (uint16_t)px; c->nix = nix; c->cond = 1;
					pc = (px + 1) & 511;
					fl = (fl & 3) | (px == 511) << 2;
					ix = nix;
					t2 += 4;
					break;
				}
			}
		} else {
			nix = ram[px];
			nins++;
		}
	next:
		if (!jc) pc = (px + 1) & 511;
		fl = (fl & 3) | (jc || px == 511) << 2;
		ix = nix;
		t2 += 4;
		if (t2 > tl) {
			if (t2 - 2 >= dis) { idled = 1; break; }
			goto limit;
		}
	}
	goto out;
limit:
	/* t is passed, or the cog stops first */
	if (t2 <= t) {
		e = &dec[(pc - 1) & 511];
		if (e->word != ix) dec_fill(e, ix);
		if (e->kind != K_NL && !(e->fl & F_INA)) {
			if ((e->fl & F_SPEC)) {
				(void)sread(p, n, e->src, t2);
				if (e->src == 0x1F1 || e->src == 0x1FC || e->src == 0x1FD) l->dirty = 1;
			}
			idled = 1;
		}
	}
out:
	l->nins = (uint16_t)(l->nins + nins);
	c->p = (uint16_t)pc;
	c->ix = ix;
	c->c = (uint8_t)(fl >> 1 & 1);
	c->z = (uint8_t)(fl & 1);
	c->cancel = (uint8_t)(fl >> 2 & 1);
	c->t0 = t2 - 2;
	c->ev_t = t2;
	c->ev = EV_EXEC;
	if (hub) {
		c->ev = EV_HUB;
		c->ev_t = hub;
	}
	if (idled) idle(p, n);
	return done || t2 != t2in || hub || idled;
}

/* Lazy cogs: a cog that reads only hub RAM, PAR and CNT, jumps to fixed addresses and drives only lazy_ok pins
   runs behind the others, reading hub RAM through the journal. */

/* every word reachable from the next instruction qualifies and is never written */
static int lazy_code(const p8x32a * const p, const int n)
{
	const p8x32a_cog * const c = &p->cog[n];
	uint8_t seen[64], wr[64];
	uint16_t todo[512];
	int nt = 0, k;
	memset(seen, 0, sizeof(seen));
	memset(wr, 0, sizeof(wr));
	todo[nt++] = (uint16_t)((c->p - 1) & 511);
	seen[todo[0] >> 3] |= (uint8_t)(1u << (todo[0] & 7));
	while (nt) {
		const unsigned a = todo[--nt];
		unsigned op, nx[2], nn = 0, j;
		const uint32_t w = c->ram[a];
		if (a >= 0x1F0) return 0;
		op = OP(w);
		if (COND(w) == 0) nx[nn++] = (a + 1) & 511;
		else {
			if (op == 3 || op >= 0x3C) return 0;                           /* system ops, waits */
			if (!FIM(w) && SRC(w) >= 0x1F0 && SRC(w) != 0x1F0 && SRC(w) != 0x1F1 && SRC(w) != 0x1F4) return 0; /* INA, PHS, ... */
			if (op <= 2) {
				if (!FWR(w) || DST(w) >= 0x1F0) return 0;                  /* hub writes */
				if (FWC(w)) return 0;                                        /* C from the hub's sys_c */
			} else if (FWR(w) && DST(w) >= 0x1F0 && DST(w) != 0x1F4) return 0; /* special registers but OUTA */
			if (FWR(w) && DST(w) < 0x1F0) wr[DST(w) >> 3] |= (uint8_t)(1u << (DST(w) & 7));
			if (op == 0x17 || (op >= 0x39 && op <= 0x3B)) {
				if (!FIM(w)) return 0;                                       /* jumps to an address from a register */
				nx[nn++] = SRC(w);
				if (COND(w) != 15 || op != 0x17) nx[nn++] = (a + 1) & 511;
			} else nx[nn++] = (a + 1) & 511;
		}
		for (j = 0; j < nn; j++)
			if (!(seen[nx[j] >> 3] >> (nx[j] & 7) & 1)) {
				seen[nx[j] >> 3] |= (uint8_t)(1u << (nx[j] & 7));
				todo[nt++] = (uint16_t)nx[j];
			}
	}
	for (k = 0; k < 64; k++)
		if (seen[k] & wr[k]) return 0;                                          /* code it writes */
	return 1;
}

P8_COLD void lazy_run(p8x32a * const p, const uint64_t t)
{
	p8x32a_cog * const c = &p->cog[p->lz];
	const uint64_t now = p->now;
	p->lz_to = t;
	lazy_due(p, t);
	c->ev_t = p->lz_evt;
	while (c->ev_t <= t && (c->ev == EV_EXEC || c->ev == EV_HUB)) {
		p->now = c->ev_t;
		if (!run_local(p, p->lz, t, P8X32A_NEVER, p->sched_gen)) { log_once(p, LOG_LAZY, "p8x32a: lazy cog stopped"); break; }
	}
	lazy_due(p, t);
	p->lz_evt = c->ev_t;
	c->ev_t = P8X32A_NEVER; /* out of the schedule */
	p->now = now;
	p->lz_at = t;
	if (p->jn) jn_drop(p, t);
}

/* retranslate the cog's blocks, with or without OUTA writes */
static void jit_drop(p8x32a * const p, const int n)
{
	unsigned j;
	for (j = 0; j < 512; j++)
		if (p->jblk[n][j]) jit_void(p, n, j);
	memset(p->jcode[n], 0, sizeof(p->jcode[n]));
}

/* end lazy mode at p->now */
static void lazy_exit(p8x32a * const p)
{
	p8x32a_cog * const c = &p->cog[p->lz];
	int k;
	lazy_catch(p, p->now);
	flush(p, p->now);
	for (k = 0; k < p->lz_nh; k++) add_pending(p, p->lz_ht[k], p->lz_pins, P8X32A_PEND_COG(p->lz));
	p->lz_nh = 0;
	p->lz_on = 0;
	c->ev_t = p->lz_evt;
	c->outa = p->lz_reg;
	/* its pins were left out of last_out */
	add_pending(p, p->now, p->lz_pins, P8X32A_PEND_COG(p->lz));
	jit_drop(p, p->lz);
	p->pins_ok = 0;
	p->sched_gen++;
	p->lz_try[p->lz] = p->now + (1u << 16);
	if (p->bus.lazy) p->bus.lazy(p->bus.ctx, p->now, 0, 0, 0);
}

/* at the end of run_until(t): pick a cog that can run lazily */
static void lazy_try(p8x32a * const p, const uint64_t t)
{
	int n, k, j;
	if (p->stop) return;
	for (n = 0; n < 8; n++) {
		p8x32a_cog * const c = &p->cog[n];
		const uint32_t g = c->dira.cur;
		uint32_t other = 0;
		if (c->ev != EV_EXEC || t < p->lz_try[n]) continue;
		/* retry a rejected cog after 2^14 cycles, doubling to 2^22 */
		p->lz_wait[n] = p->lz_wait[n] ? (p->lz_wait[n] < (1u << 22) ? p->lz_wait[n] * 2 : p->lz_wait[n]) : 1u << 14;
		p->lz_try[n] = t + p->lz_wait[n];
		if (!c->run || (p->sleepers >> n & 1) || p->loop[n].state != LOOP_SEARCH) continue;
		if (c->disable_at != P8X32A_NEVER || c->restart_at != P8X32A_NEVER || c->ix != c->ram[(c->p - 1) & 511]) continue;
		if (c->ctr[0] || c->ctr[1] || c->ctr_old[0] || c->ctr_old[1] || c->ctr_at[0] > t || c->ctr_at[1] > t) continue;
		if (!g || (g & ~p->lazy_ok) || c->dira.at > t) continue;
		for (k = 0; k < 8; k++) {
			const p8x32a_cog * const o = &p->cog[k];
			for (j = 0; j < 2; j++) other |= nco_pins(o->ctr[j]) | nco_pins(o->ctr_old[j]);
			if (k == n) continue;
			other |= regval(&o->dira, t) | o->dira.cur;
			if ((p->sleepers >> k & 1) && (p->loop[k].wake & g)) other |= g;
			if (o->ev == EV_WAITPIN) other |= o->s;
		}
		if ((other & g) || !lazy_code(p, n)) continue;
		p->lz_on = 1;
		p->lz = (uint8_t)n;
		p->lz_pins = g;
		p->lz_at = p->lz_to = t;
		p->lz_nh = 0;
		/* an OUTA write not in effect yet goes on the lazy path */
		p->lz_out = regval(&c->outa, t) & g;
		if (c->outa.at > t && ((c->outa.cur ^ c->outa.prev) & g)) {
			p->lz_ht[0] = c->outa.at;
			p->lz_hout[0] = c->outa.cur & g;
			p->lz_nh = 1;
		}
		p->pins_ok = 0;
		p->sched_gen++;
		p->lazies++;
		p->lz_evt = c->ev_t;
		c->ev_t = P8X32A_NEVER;
		/* its OUTA lives in lz_reg */
		p->lz_reg = c->outa;
		c->outa.prev = c->outa.cur = 0;
		jit_drop(p, n);
		if (p->bus.lazy) p->bus.lazy(p->bus.ctx, t, g, p->lz_out, g);
		return;
	}
}

/* scheduling key: earliest event, then hub events, then lowest cog */
static uint64_t ev_key(const p8x32a_cog * const c, const int n)
{
	const uint64_t e = c->ev_t < ((uint64_t)1 << 59) ? c->ev_t : (uint64_t)1 << 59;
	return c->ev == EV_NONE ? P8X32A_NEVER : e << 4 | (uint64_t)(c->ev != EV_HUB) << 3 | (uint64_t)n;
}

void p8x32a_run_until(p8x32a *p, uint64_t t)
{
	uint64_t key[9];
	unsigned kgen = p->sched_gen - 1;
	p->horizon = t;
	for (;;) {
		int n, j, best;
		uint64_t bk, bk2;
		unsigned gen;
		p8x32a_cog *b;
		/* sorted keys; only the cog that ran changes, unless sched_gen moved */
		if (kgen != p->sched_gen) {
			for (n = 0; n < 8; n++) {
				const uint64_t k = ev_key(&p->cog[n], n);
				for (j = n; j > 0 && key[j - 1] > k; j--) key[j] = key[j - 1];
				key[j] = k;
			}
			key[8] = P8X32A_NEVER;
			kgen = p->sched_gen;
		}
		/* the next event, and bk2 the one after it */
		bk = key[0];
		bk2 = key[1];
		if (bk == P8X32A_NEVER) break;
		best = (int)(bk & 7);
		if (p->cog[best].ev_t > t || p->stop) break;
		b = &p->cog[best];
		gen = p->sched_gen;
	again:
		p->now = b->ev_t;
		if (b->ev == EV_HUB) {
			if (!b->run || p->loop[best].state != LOOP_SEARCH || !run_local(p, best, t, bk2, gen)) do_hub(p, best);
		} else switch (b->ev) {
		case EV_EXEC:
			if (!b->run || p->loop[best].state != LOOP_SEARCH || !run_local(p, best, t, bk2, gen)) exec(p, best);
			break;
		case EV_WAITPIN: wait_pins(p, best); break;
		case EV_RESTART: restart(p, best); break;
		case EV_DONE:
			if (b->ev_t >= b->disable_at) idle(p, best);
			else complete(p, best, b->ev_t, 0, p->sys_c);
			break;
		case EV_SLEEP: loop_wake(p, best); break;
		}
		if (p->loop[best].state == LOOP_RECORD) loop_post(p, best);
		/* local instructions may run ahead of other cogs */
		while (b->ev == EV_EXEC && b->ev_t <= t && (local(b) || issue_local(b))) {
			if (p->loop[best].state == LOOP_RECORD || !local(b)) exec(p, best);
			else run_local(p, best, t, bk2, gen);
			if (p->loop[best].state == LOOP_RECORD) loop_post(p, best);
		}
		/* run again while still the earliest */
		if (gen == p->sched_gen && b->ev != EV_NONE && b->ev_t <= t && !p->stop && ev_key(b, best) < bk2) goto again;
		/* best's new key moves back to its place */
		{
			const uint64_t k = ev_key(b, best);
			for (j = 0; key[j + 1] < k; j++) key[j] = key[j + 1];
			key[j] = k;
		}
	}
	lazy_catch(p, t);
	flush(p, t);
	p->now = t;
	if (!p->lz_on && p->lazy_ok) lazy_try(p, t);
}

void p8x32a_reset(p8x32a *p, uint64_t t)
{
	int n;
	for (n = 0; n < 8; n++) {
		p8x32a_cog * const c = &p->cog[n];
		uint32_t ram[512];
		c->ctr[0] = c->ctr[1] = 0;
		ctr_notify(p, n, 0, t);
		ctr_notify(p, n, 1, t);
		memcpy(ram, c->ram, sizeof(ram));
		memset(c, 0, sizeof(*c));
		memcpy(c->ram, ram, sizeof(ram));
		c->disable_at = P8X32A_NEVER;
		c->restart_at = P8X32A_NEVER;
		c->outa.at = c->dira.at = t;
	}
	p->cog_e = 1;
	p->lock_e = p->lock_state = p->cfg = p->sys_q = p->sys_c = 0;
	p->slot_base = t;
	p->npend = 0;
	p->ctr_ok = 0;
	p->nco_mask = p->nco_ok = 0;
	p->nco_n = 0;
	memset(p->nco_lvl, 0, sizeof(p->nco_lvl));
	p->pins_ok = 0;
	p->sleepers = p->waiters = 0;
	for (n = 0; n < 8; n++) loop_reset(&p->loop[n]);
	if (p->lz_on) {
		jit_drop(p, p->lz);
		if (p->bus.lazy) p->bus.lazy(p->bus.ctx, t, 0, 0, 0);
	}
	p->lz_on = p->lz_nh = 0;
	p->jn = 0;
	memset(p->jmap, 0, sizeof(p->jmap));
	memset(p->lz_try, 0, sizeof(p->lz_try));
	memset(p->lz_wait, 0, sizeof(p->lz_wait));
	p->flushed = t;
	p->last_out = p->last_dir = 0;
	p->cog[0].ptr = 0x3E00;
	p->cog[0].restart_at = t + 3;
	idle(p, 0);
	p->now = t;
}

void p8x32a_init(p8x32a *p, const p8x32a_bus *bus)
{
	int n;
	memset(p, 0, sizeof(*p));
	p->bus = *bus;
	for (n = 0; n < 8 * 512; n++) {
		dec_fill(&p->dec[0][0] + n, 0);
		(&p->jlink[0][0] + n)->len = ~0u;
	}
	p8x32a_reset(p, 0);
}
