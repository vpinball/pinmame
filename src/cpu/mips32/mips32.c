// license:BSD-3-Clause

#include "mips32.h"
#include "../../bitops.h"
#include <stddef.h>
#include <string.h>

/* the layout mips32.h promises fast_run */
typedef char mips32_hot_line[offsetof(mips32_state, region) <= 64 ? 1 : -1];
typedef char mips32_hot_lines[offsetof(mips32_state, gpr) <= 192 ? 1 : -1];
typedef char mips32_gpr_aligned[offsetof(mips32_state, gpr) % 64 == 0 ? 1 : -1];

#define ST_IE    0x00000001u
#define ST_EXL   0x00000002u
#define ST_ERL   0x00000004u
#define ST_UM    0x00000010u
#define ST_BEV   0x00400000u
#define ST_CU0   0x10000000u
#define ST_WMASK 0x1A48FF17u
#define ST_IPL(v) (((v) >> 10) & 0x3F)

#define CA_BD    0x80000000u
#define CA_TI    0x40000000u
#define CA_IV    0x00800000u
#define CA_WMASK 0x08C00300u

#define RS(op)    (((op) >> 21) & 31)
#define RT(op)    (((op) >> 16) & 31)
#define RD(op)    (((op) >> 11) & 31)
#define SA(op)    (((op) >> 6) & 31)
#define FUNCT(op) ((op) & 63)
#define SIMM(op)  ((uint32_t)(int32_t)(int16_t)((op) & 0xFFFF))
#define UIMM(op)  ((op) & 0xFFFFu)

#define REGS(s) ((s)->gpr[(s)->srsctl & 7])
#define STATUS_CHANGED(s) ((s)->irq_chk = 1, (s)->fwords = 0)
#define NEVER (~(uint64_t)0)
#define USER(s) (((s)->status & (ST_UM | ST_EXL | ST_ERL)) == ST_UM)
#define SET(n, v) do { uint32_t v_ = (v); int n_ = (n); if (n_) REGS(s)[n_] = v_; } while (0)

static uint32_t mask32(const unsigned size) { return size >= 32 ? 0xFFFFFFFFu : (1u << size) - 1; }
/* WSBH: the bytes swapped within each halfword */
static uint32_t wsbh32(const uint32_t x) { return rotr_32(swap_byteorder_32(x), 16); }

int mips32_translate(const mips32_state *s, uint32_t va, uint32_t *pa)
{
	if (va < 0x80000000u) {
		*pa = (s->status & ST_ERL) ? va : va + 0x40000000u;
		return 1;
	}
	if (USER(s)) return 0;
	*pa = va < 0xC0000000u ? (va & 0x1FFFFFFFu) : va;
	return 1;
}

/* Count advances every other cycle; count and count_half hold its value at cycle count_at */
static uint32_t count_now(const mips32_state * const s, const uint64_t c)
{
	return s->count + (uint32_t)((c - s->count_at + (uint64_t)s->count_half) >> 1);
}

static void count_sync(mips32_state * const s, const uint64_t c)
{
	const uint64_t t = c - s->count_at + (uint64_t)s->count_half;
	s->count += (uint32_t)(t >> 1);
	s->count_half = (int)(t & 1);
	s->count_at = c;
}

/* the first cycle at which Count equals Compare, while the Timer interrupt is not pending */
static void count_arm(mips32_state * const s)
{
	uint64_t d = (uint32_t)(s->compare - s->count);
	if (s->cause & CA_TI) { s->ti_at = NEVER; return; }
	if (!d) d = (uint64_t)1 << 32;
	s->ti_at = s->count_at + 2 * d - (uint64_t)s->count_half;
}

static void take_exception(mips32_state * const s, const int code, const int ce)
{
	const int is_int = code == MIPS32_EXC_INT;
	uint32_t off = 0x180, base;

	s->exc_seq++;
	if (s->bus.exc_hook) {
		const int h = s->bus.exc_hook(s->bus.ctx, s, code);
		if (h == MIPS32_HOOK_SKIP) { s->pc = s->skip_pc; s->npc = s->pc + 4; s->delay = 0; return; }
		if (h == MIPS32_HOOK_STOP) { s->stop = 1; return; }
	}
	if (!(s->status & ST_EXL)) {
		s->epc = s->cur_delay ? s->cur_pc - 4 : s->cur_pc;
		if (s->cur_delay) s->cause |= CA_BD; else s->cause &= ~CA_BD;
		if (is_int && (s->cause & CA_IV))
			off = (s->status & ST_BEV) ? 0x200 : 0x200 + (uint32_t)s->eic_vector * (((s->intctl >> 5) & 0x1F) << 5);
		if (s->shadow_sets > 1 && !(s->status & ST_BEV)) {
			const uint32_t css = s->srsctl & 15;
			const uint32_t nss = (is_int ? (uint32_t)s->eic_srs : (s->srsctl >> 12)) & 7;
			s->srsctl = (s->srsctl & ~0x3CFu) | (css << 6) | nss;
		}
	}
	s->cause = (s->cause & ~0x3000007Cu) | ((uint32_t)code << 2) | ((uint32_t)ce << 28);
	if (is_int) s->cause = (s->cause & ~0xFC00u) | ((uint32_t)s->eic_ripl << 10);
	s->status |= ST_EXL;
	STATUS_CHANGED(s);
	base = (s->status & ST_BEV) ? 0xBFC00200u : (s->ebase & 0xFFFFF000u);
	s->pc = base + off;
	s->npc = s->pc + 4;
	s->delay = 0;
	s->waiting = 0;
	if (is_int && s->bus.irq_taken) s->bus.irq_taken(s->bus.ctx, s->eic_vector);
}

static const uint8_t *direct_rd(const mips32_state * const s, const uint32_t pa, const uint32_t size)
{
	int k;
	for (k = 0; k < MIPS32_REGIONS; k++) {
		const mips32_region * const m = &s->region[k];
		if (pa - m->base < m->size && m->size - (pa - m->base) >= size) return m->rd + (pa - m->base);
	}
	return NULL;
}

static uint8_t *direct_wr(const mips32_state * const s, const uint32_t pa, const uint32_t size)
{
	int k;
	for (k = 0; k < MIPS32_REGIONS; k++) {
		const mips32_region * const m = &s->region[k];
		if (m->wr && pa - m->base < m->size && m->size - (pa - m->base) >= size) return m->wr + (pa - m->base);
	}
	return NULL;
}

static uint32_t le32(const uint8_t * const p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(v));
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	v = __builtin_bswap32(v);
#elif defined(_MSC_VER) && !defined(__clang__)
	/* all MSVC targets supported are little-endian */
#endif
	return v;
}

static int load(mips32_state * const s, const uint32_t va, const int size, uint32_t * const out)
{
	uint32_t pa;
	const uint8_t *m;
	int err = 0;
	if ((va & (uint32_t)(size - 1)) || !mips32_translate(s, va, &pa)) {
		s->badvaddr = va;
		take_exception(s, MIPS32_EXC_ADEL, 0);
		return 0;
	}
	if ((m = direct_rd(s, pa, (uint32_t)size)) != NULL) {
		*out = size == 4 ? le32(m) : size == 2 ? (uint32_t)(m[0] | m[1] << 8) : m[0];
		return 1;
	}
	*out = s->bus.read(s->bus.ctx, pa, size, 0, &err);
	if (err) { take_exception(s, MIPS32_EXC_DBE, 0); return 0; }
	return 1;
}

static int store(mips32_state * const s, const uint32_t va, const uint32_t v, const int size)
{
	uint32_t pa;
	uint8_t *m;
	int err = 0, i;
	if ((va & (uint32_t)(size - 1)) || !mips32_translate(s, va, &pa)) {
		s->badvaddr = va;
		take_exception(s, MIPS32_EXC_ADES, 0);
		return 0;
	}
	if ((m = direct_wr(s, pa, (uint32_t)size)) != NULL) {
		for (i = 0; i < size; i++) m[i] = (uint8_t)(v >> (8 * i));
		return 1;
	}
	s->bus.write(s->bus.ctx, pa, v, size, &err);
	if (err) { take_exception(s, MIPS32_EXC_DBE, 0); return 0; }
	return 1;
}

uint32_t mips32_get_cp0(const mips32_state *s, int reg, int sel)
{
	switch (reg * 8 + sel) {
	case 7 * 8:      return s->hwrena;
	case 8 * 8:      return s->badvaddr;
	case 9 * 8:      return count_now(s, s->c0);
	case 11 * 8:     return s->compare;
	case 12 * 8:     return s->status;
	case 12 * 8 + 1: return s->intctl;
	case 12 * 8 + 2: return s->srsctl | ((uint32_t)(s->shadow_sets - 1) << 26) | (((uint32_t)s->eic_srs & 15) << 18);
	case 12 * 8 + 3: return s->srsmap;
	case 13 * 8:     return s->cause;
	case 14 * 8:     return s->epc;
	case 15 * 8:     return s->prid;
	case 15 * 8 + 1: return s->ebase;
	case 16 * 8:     return s->config0;
	case 16 * 8 + 1: return 0x80000000u;
	case 16 * 8 + 2: return 0x80000000u;
	case 16 * 8 + 3: return 0x00000060u;
	case 30 * 8:     return s->errorepc;
	}
	return 0;
}

static void set_cp0(mips32_state * const s, const int reg, const int sel, const uint32_t v)
{
	switch (reg * 8 + sel) {
	case 7 * 8:      s->hwrena = v & 0xFu; break;
	case 9 * 8:      count_sync(s, s->c0); s->count = v; s->count_half = 0; count_arm(s); break;
	case 11 * 8:     count_sync(s, s->c0); s->compare = v; s->cause &= ~CA_TI; count_arm(s); break;
	case 12 * 8:     s->status = (s->status & ~ST_WMASK) | (v & ST_WMASK); STATUS_CHANGED(s); break;
	case 12 * 8 + 1: s->intctl = v & 0x3E0u; break;
	case 12 * 8 + 2: s->srsctl = (s->srsctl & ~0xF3C0u) | (v & 0xF3C0u); break;
	case 12 * 8 + 3: s->srsmap = v; break;
	case 13 * 8: {
		const uint32_t was = s->cause;
		s->cause = (s->cause & ~CA_WMASK) | (v & CA_WMASK);
		if (s->cause & ~was & 0x300u) s->stop = 1; /* IP0/IP1 raised: return so the interrupt controller sees it */
		break;
	}
	case 14 * 8:     s->epc = v; break;
	case 15 * 8 + 1: s->ebase = 0x80000000u | (v & 0x3FFFF000u); break;
	case 16 * 8:     s->config0 = (s->config0 & ~7u) | (v & 7u); break;
	case 30 * 8:     s->errorepc = v; break;
	}
}

static void branch(mips32_state * const s, const int cond, const uint32_t op)
{
	if (cond) s->npc = s->cur_pc + 4 + (SIMM(op) << 2);
	s->delay = 1;
}

static void branch_likely(mips32_state * const s, const int cond, const uint32_t op)
{
	if (cond) {
		s->npc = s->cur_pc + 4 + (SIMM(op) << 2);
		s->delay = 1;
	} else {
		s->pc = s->npc;
		s->npc += 4;
	}
}

static void jump(mips32_state * const s, const uint32_t target)
{
	s->npc = target;
	s->delay = 1;
}

static void eret(mips32_state * const s)
{
	STATUS_CHANGED(s);
	if (s->status & ST_ERL) {
		s->pc = s->errorepc;
		s->status &= ~ST_ERL;
	} else {
		s->pc = s->epc;
		s->status &= ~ST_EXL;
		if (s->shadow_sets > 1 && !(s->status & ST_BEV)) {
			const uint32_t pss = (s->srsctl >> 6) & 7;
			s->srsctl = (s->srsctl & ~15u) | pss;
		}
	}
	s->npc = s->pc + 4;
	s->delay = 0;
	s->llbit = 0;
}

static void exec_special(mips32_state * const s, const uint32_t op)
{
	const uint32_t rs = REGS(s)[RS(op)], rt = REGS(s)[RT(op)];
	uint32_t res;
	int64_t p;
	uint64_t acc;

	switch (FUNCT(op)) {
	case 0x00: SET(RD(op), rt << SA(op)); break;
	case 0x02: SET(RD(op), (op & (1u << 21)) ? rotr_32(rt, SA(op)) : rt >> SA(op)); break;
	case 0x03: SET(RD(op), sar_32(rt, SA(op))); break;
	case 0x04: SET(RD(op), rt << (rs & 31)); break;
	case 0x06: SET(RD(op), (op & (1u << 6)) ? rotr_32(rt, rs & 31) : rt >> (rs & 31)); break;
	case 0x07: SET(RD(op), sar_32(rt, rs & 31)); break;
	case 0x08: jump(s, rs); break;
	case 0x09: SET(RD(op), s->cur_pc + 8); jump(s, rs); break;
	case 0x0A: if (!rt) SET(RD(op), rs); break;
	case 0x0B: if (rt) SET(RD(op), rs); break;
	case 0x0C: take_exception(s, MIPS32_EXC_SYS, 0); break;
	case 0x0D: take_exception(s, MIPS32_EXC_BP, 0); break;
	case 0x0F: break;
	case 0x10: SET(RD(op), s->hi); break;
	case 0x11: s->hi = rs; break;
	case 0x12: SET(RD(op), s->lo); break;
	case 0x13: s->lo = rs; break;
	case 0x18:
		p = (int64_t)(int32_t)rs * (int32_t)rt;
		s->lo = (uint32_t)p; s->hi = (uint32_t)((uint64_t)p >> 32);
		break;
	case 0x19:
		acc = (uint64_t)rs * rt;
		s->lo = (uint32_t)acc; s->hi = (uint32_t)(acc >> 32);
		break;
	case 0x1A:
		if (rt) {
			if (rs == 0x80000000u && rt == 0xFFFFFFFFu) { s->lo = 0x80000000u; s->hi = 0; }
			else { s->lo = (uint32_t)((int32_t)rs / (int32_t)rt); s->hi = (uint32_t)((int32_t)rs % (int32_t)rt); }
		}
		s->cycles += 34;
		break;
	case 0x1B:
		if (rt) { s->lo = rs / rt; s->hi = rs % rt; }
		s->cycles += 34;
		break;
	case 0x20:
		res = rs + rt;
		if (~(rs ^ rt) & (rs ^ res) & 0x80000000u) take_exception(s, MIPS32_EXC_OV, 0);
		else SET(RD(op), res);
		break;
	case 0x21: SET(RD(op), rs + rt); break;
	case 0x22:
		res = rs - rt;
		if ((rs ^ rt) & (rs ^ res) & 0x80000000u) take_exception(s, MIPS32_EXC_OV, 0);
		else SET(RD(op), res);
		break;
	case 0x23: SET(RD(op), rs - rt); break;
	case 0x24: SET(RD(op), rs & rt); break;
	case 0x25: SET(RD(op), rs | rt); break;
	case 0x26: SET(RD(op), rs ^ rt); break;
	case 0x27: SET(RD(op), ~(rs | rt)); break;
	case 0x2A: SET(RD(op), (int32_t)rs < (int32_t)rt); break;
	case 0x2B: SET(RD(op), rs < rt); break;
	case 0x30: if ((int32_t)rs >= (int32_t)rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x31: if (rs >= rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x32: if ((int32_t)rs < (int32_t)rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x33: if (rs < rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x34: if (rs == rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x36: if (rs != rt) take_exception(s, MIPS32_EXC_TR, 0); break;
	default: take_exception(s, MIPS32_EXC_RI, 0); break;
	}
}

static void exec_regimm(mips32_state * const s, const uint32_t op)
{
	const int32_t rs = (int32_t)REGS(s)[RS(op)];
	const uint32_t imm = SIMM(op);
	int cond;

	switch (RT(op)) {
	case 0x00: branch(s, rs < 0, op); break;
	case 0x01: branch(s, rs >= 0, op); break;
	case 0x02: branch_likely(s, rs < 0, op); break;
	case 0x03: branch_likely(s, rs >= 0, op); break;
	case 0x08: if (rs >= (int32_t)imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x09: if ((uint32_t)rs >= imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x0A: if (rs < (int32_t)imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x0B: if ((uint32_t)rs < imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x0C: if ((uint32_t)rs == imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x0E: if ((uint32_t)rs != imm) take_exception(s, MIPS32_EXC_TR, 0); break;
	case 0x10: cond = rs < 0; SET(31, s->cur_pc + 8); branch(s, cond, op); break;
	case 0x11: cond = rs >= 0; SET(31, s->cur_pc + 8); branch(s, cond, op); break;
	case 0x12: cond = rs < 0; SET(31, s->cur_pc + 8); branch_likely(s, cond, op); break;
	case 0x13: cond = rs >= 0; SET(31, s->cur_pc + 8); branch_likely(s, cond, op); break;
	case 0x1F: break;
	default: take_exception(s, MIPS32_EXC_RI, 0); break;
	}
}

static void exec_cop0(mips32_state * const s, const uint32_t op)
{
	uint32_t v;

	if (USER(s) && !(s->status & ST_CU0)) { take_exception(s, MIPS32_EXC_CPU, 0); return; }
	if (op & (1u << 25)) {
		switch (FUNCT(op)) {
		case 0x18: eret(s); break;
		case 0x20: s->waiting = 1; break;
		default: take_exception(s, MIPS32_EXC_RI, 0); break;
		}
		return;
	}
	switch (RS(op)) {
	case 0x00: SET(RT(op), mips32_get_cp0(s, RD(op), op & 7)); break;
	case 0x04: set_cp0(s, RD(op), op & 7, REGS(s)[RT(op)]); break;
	case 0x0A: SET(RD(op), s->gpr[(s->srsctl >> 6) & 7][RT(op)]); break;
	case 0x0B:
		v = s->status;
		if (op & 0x20) s->status |= ST_IE; else s->status &= ~ST_IE;
		STATUS_CHANGED(s);
		SET(RT(op), v);
		break;
	case 0x0E: if (RD(op)) s->gpr[(s->srsctl >> 6) & 7][RD(op)] = REGS(s)[RT(op)]; break;
	default: take_exception(s, MIPS32_EXC_RI, 0); break;
	}
}

static void exec_special2(mips32_state * const s, const uint32_t op)
{
	const uint32_t rs = REGS(s)[RS(op)], rt = REGS(s)[RT(op)];
	uint64_t acc = ((uint64_t)s->hi << 32) | s->lo;

	switch (FUNCT(op)) {
	case 0x00: acc += (uint64_t)((int64_t)(int32_t)rs * (int32_t)rt); break;
	case 0x01: acc += (uint64_t)rs * rt; break;
	case 0x02: SET(RD(op), (uint32_t)((int64_t)(int32_t)rs * (int32_t)rt)); s->cycles += 1; return;
	case 0x04: acc -= (uint64_t)((int64_t)(int32_t)rs * (int32_t)rt); break;
	case 0x05: acc -= (uint64_t)rs * rt; break;
	case 0x20: SET(RD(op), clz_32(rs)); return;
	case 0x21: SET(RD(op), clz_32(~rs)); return;
	default: take_exception(s, MIPS32_EXC_RI, 0); return;
	}
	s->lo = (uint32_t)acc;
	s->hi = (uint32_t)(acc >> 32);
}

static void exec_special3(mips32_state * const s, const uint32_t op)
{
	const uint32_t rs = REGS(s)[RS(op)], rt = REGS(s)[RT(op)];
	uint32_t m;
	const unsigned lsb = SA(op), msb = RD(op);

	switch (FUNCT(op)) {
	case 0x00: SET(RT(op), (rs >> lsb) & mask32(msb + 1)); break;
	case 0x04:
		m = mask32(msb - lsb + 1) << lsb;
		SET(RT(op), (rt & ~m) | ((rs << lsb) & m));
		break;
	case 0x20:
		switch (SA(op)) {
		case 0x02: SET(RD(op), wsbh32(rt)); break;
		case 0x10: SET(RD(op), (uint32_t)(int32_t)(int8_t)rt); break;
		case 0x18: SET(RD(op), (uint32_t)(int32_t)(int16_t)rt); break;
		default: take_exception(s, MIPS32_EXC_RI, 0); break;
		}
		break;
	case 0x3B:
		if (USER(s) && !(s->status & ST_CU0) && !(s->hwrena & (1u << RD(op)))) { take_exception(s, MIPS32_EXC_RI, 0); break; }
		switch (RD(op)) {
		case 0: SET(RT(op), s->ebase & 0x3FFu); break;
		case 1: SET(RT(op), 0); break;
		case 2: SET(RT(op), count_now(s, s->c0)); break;
		case 3: SET(RT(op), 2); break;
		default: take_exception(s, MIPS32_EXC_RI, 0); break;
		}
		break;
	default: take_exception(s, MIPS32_EXC_RI, 0); break;
	}
}

enum { PK_W, PK_B, PK_H, PK_BU, PK_HU };

static uint32_t prov_value(const int kind, const uint32_t v)
{
	switch (kind) {
	case PK_B: return (uint32_t)(int32_t)(int8_t)v;
	case PK_H: return (uint32_t)(int32_t)(int16_t)v;
	case PK_BU: return v & 0xFFu;
	case PK_HU: return v & 0xFFFFu;
	}
	return v;
}

/* the uncertain bits as they are in the register */
static uint32_t prov_regmask(const mips32_state * const s)
{
	const uint32_t m = s->prov_mask;
	switch (s->prov_kind) {
	case PK_B: return (m & 0x80u) ? m | 0xFFFFFF00u : m & 0xFFu;
	case PK_H: return (m & 0x8000u) ? m | 0xFFFF0000u : m & 0xFFFFu;
	case PK_BU: return m & 0xFFu;
	case PK_HU: return m & 0xFFFFu;
	}
	return m;
}

static void prov_apply(mips32_state * const s, const uint32_t bits)
{
	const uint32_t v = (s->prov_v & ~s->prov_mask) | (bits & s->prov_mask);
	s->gpr[s->prov_set][s->prov_reg] = prov_value(s->prov_kind, v);
	s->prov = 0;
}

void mips32_settle(mips32_state *s)
{
	uint32_t bits = 0;
	if (!s->prov) return;
	if (s->bus.settle) s->bus.settle(s->bus.ctx, s->prov_tok, 1, &bits);
	prov_apply(s, bits);
}

void mips32_uncertain(mips32_state *s, uint32_t mask, uint32_t token)
{
	s->unc = mask;
	s->unc_tok = token;
}

/* register reg (current set) was loaded with v, whose bits s->unc are not known yet */
static void prov_new(mips32_state * const s, const unsigned reg, const int kind, const uint32_t v)
{
	const uint32_t mask = s->unc;
	s->unc = 0;
	if (!reg) return;
	if (s->prov && !(s->prov_reg == reg && s->prov_set == (s->srsctl & 7))) mips32_settle(s);
	s->prov = 1;
	s->prov_kind = kind;
	s->prov_set = s->srsctl & 7;
	s->prov_reg = reg;
	s->prov_v = v;
	s->prov_mask = mask;
	s->prov_tok = s->unc_tok;
	s->prov_gen++;
	s->prov_n = 0;
}

static void exec_mem(mips32_state * const s, const uint32_t op)
{
	const uint32_t ea = REGS(s)[RS(op)] + SIMM(op), rt = REGS(s)[RT(op)];
	uint32_t v;
	const unsigned b = ea & 3;
	unsigned i, sh;

	s->unc = 0;
	switch (op >> 26) {
	case 0x20: if (load(s, ea, 1, &v)) { SET(RT(op), (uint32_t)(int32_t)(int8_t)v); if (s->unc) prov_new(s, RT(op), PK_B, v); } break;
	case 0x21: if (load(s, ea, 2, &v)) { SET(RT(op), (uint32_t)(int32_t)(int16_t)v); if (s->unc) prov_new(s, RT(op), PK_H, v); } break;
	case 0x22:
		if (load(s, ea & ~3u, 4, &v)) {
			if (s->unc) { uint32_t bits = 0; if (s->bus.settle) s->bus.settle(s->bus.ctx, s->unc_tok, 1, &bits); v = (v & ~s->unc) | (bits & s->unc); s->unc = 0; }
			sh = (3 - b) * 8;
			SET(RT(op), sh ? (rt & ((1u << sh) - 1)) | (v << sh) : v);
		}
		break;
	case 0x23: if (load(s, ea, 4, &v)) { SET(RT(op), v); if (s->unc) prov_new(s, RT(op), PK_W, v); } break;
	case 0x24: if (load(s, ea, 1, &v)) { SET(RT(op), v & 0xFFu); if (s->unc) prov_new(s, RT(op), PK_BU, v); } break;
	case 0x25: if (load(s, ea, 2, &v)) { SET(RT(op), v & 0xFFFFu); if (s->unc) prov_new(s, RT(op), PK_HU, v); } break;
	case 0x26:
		if (load(s, ea & ~3u, 4, &v)) {
			if (s->unc) { uint32_t bits = 0; if (s->bus.settle) s->bus.settle(s->bus.ctx, s->unc_tok, 1, &bits); v = (v & ~s->unc) | (bits & s->unc); s->unc = 0; }
			sh = b * 8;
			SET(RT(op), sh ? (rt & ~(0xFFFFFFFFu >> sh)) | (v >> sh) : v);
		}
		break;
	case 0x28: store(s, ea, rt & 0xFFu, 1); break;
	case 0x29: store(s, ea, rt & 0xFFFFu, 2); break;
	case 0x2A:
		for (i = 0; i <= b; i++)
			if (!store(s, (ea & ~3u) + i, (rt >> (8 * (3 - b + i))) & 0xFFu, 1)) break;
		break;
	case 0x2B: store(s, ea, rt, 4); break;
	case 0x2E:
		for (i = 0; i < 4 - b; i++)
			if (!store(s, ea + i, (rt >> (8 * i)) & 0xFFu, 1)) break;
		break;
	case 0x30: if (load(s, ea, 4, &v)) { SET(RT(op), v); s->llbit = 1; if (s->unc) prov_new(s, RT(op), PK_W, v); } break;
	case 0x38:
		if (s->llbit) { if (store(s, ea, rt, 4)) SET(RT(op), 1); }
		else SET(RT(op), 0);
		break;
	}
}

static void execute(mips32_state * const s, const uint32_t op)
{
	const uint32_t rs = REGS(s)[RS(op)], rt = REGS(s)[RT(op)];
	uint32_t res;

	switch (op >> 26) {
	case 0x00: exec_special(s, op); break;
	case 0x01: exec_regimm(s, op); break;
	case 0x02: jump(s, ((s->cur_pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2)); break;
	case 0x03: SET(31, s->cur_pc + 8); jump(s, ((s->cur_pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2)); break;
	case 0x04: branch(s, rs == rt, op); break;
	case 0x05: branch(s, rs != rt, op); break;
	case 0x06: branch(s, (int32_t)rs <= 0, op); break;
	case 0x07: branch(s, (int32_t)rs > 0, op); break;
	case 0x08:
		res = rs + SIMM(op);
		if (~(rs ^ SIMM(op)) & (rs ^ res) & 0x80000000u) take_exception(s, MIPS32_EXC_OV, 0);
		else SET(RT(op), res);
		break;
	case 0x09: SET(RT(op), rs + SIMM(op)); break;
	case 0x0A: SET(RT(op), (int32_t)rs < (int32_t)SIMM(op)); break;
	case 0x0B: SET(RT(op), rs < SIMM(op)); break;
	case 0x0C: SET(RT(op), rs & UIMM(op)); break;
	case 0x0D: SET(RT(op), rs | UIMM(op)); break;
	case 0x0E: SET(RT(op), rs ^ UIMM(op)); break;
	case 0x0F: SET(RT(op), UIMM(op) << 16); break;
	case 0x10: exec_cop0(s, op); break;
	case 0x11: case 0x13: case 0x31: case 0x35: case 0x39: case 0x3D:
		take_exception(s, MIPS32_EXC_CPU, 1); break;
	case 0x12: case 0x32: case 0x36: case 0x3A: case 0x3E:
		take_exception(s, MIPS32_EXC_CPU, 2); break;
	case 0x14: branch_likely(s, rs == rt, op); break;
	case 0x15: branch_likely(s, rs != rt, op); break;
	case 0x16: branch_likely(s, (int32_t)rs <= 0, op); break;
	case 0x17: branch_likely(s, (int32_t)rs > 0, op); break;
	case 0x1C: exec_special2(s, op); break;
	case 0x1F: exec_special3(s, op); break;
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26:
	case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2E: case 0x30: case 0x38:
		exec_mem(s, op); break;
	case 0x2F:
		if (USER(s) && !(s->status & ST_CU0)) take_exception(s, MIPS32_EXC_CPU, 0);
		break;
	case 0x33: break;
	default: take_exception(s, MIPS32_EXC_RI, 0); break;
	}
}

/* remember the direct region the fetch at pc (physical pa) came from */
static void fetch_cache(mips32_state * const s, const uint32_t pa)
{
	int k;
	for (k = 0; k < MIPS32_REGIONS; k++) {
		const mips32_region * const m = &s->region[k];
		if (pa - m->base < m->size && m->size >= 4 && !(m->base & 3)) {
			s->fva = s->pc - (pa - m->base);
			s->fwords = m->size >> 2;
			s->fbase = (uintptr_t)m->rd - s->fva;
			return;
		}
	}
}

/* pc - fva rotated right by 2 is below fwords exactly when aligned and inside the window: one compare for both */
#define FETCHABLE(s, a) (rotr_32((a) - (s)->fva, 2) < (s)->fwords)
#define FETCH(s, a) le32((const uint8_t *)((s)->fbase + (a)))

static void step(mips32_state * const s)
{
	uint32_t pa, op;
	const uint8_t *m;
	int err = 0;

	s->cur_pc = s->pc;
	s->cur_delay = s->delay;
	s->skip_pc = s->npc;
	s->cycles++;
	if (FETCHABLE(s, s->pc)) op = FETCH(s, s->pc);
	else {
		if ((s->pc & 3) || !mips32_translate(s, s->pc, &pa)) {
			s->badvaddr = s->pc;
			take_exception(s, MIPS32_EXC_ADEL, 0);
			return;
		}
		if ((m = direct_rd(s, pa, 4)) != NULL) {
			op = le32(m);
			fetch_cache(s, pa);
		} else {
			op = s->bus.read(s->bus.ctx, pa, 4, 1, &err);
			if (err) { take_exception(s, MIPS32_EXC_IBE, 0); return; }
		}
	}
	s->pc = s->npc;
	s->npc += 4;
	s->delay = 0;
	execute(s, op);
}

/* the direct region holding [pa, pa + size), writable if wr; -1 if none */
static int direct_slot(const mips32_state * const s, const uint32_t pa, const uint32_t size, const int wr)
{
	int k;
	for (k = 0; k < MIPS32_REGIONS; k++) {
		const mips32_region * const m = &s->region[k];
		if ((!wr || m->wr) && pa - m->base < m->size && m->size - (pa - m->base) >= size) return k;
	}
	return -1;
}

/* fast_run's dispatch. With GCC/Clang (labels as values) each handler ends in its own copy of write-back, fetch and
   dispatch; built with clang that is 16% faster on tight loops and 4% on branchy code than MSVC's switch. It needs tail
   merging off for this file (-fno-crossjumping, -mllvm -enable-tail-merge=false; see the CMake files): merged back,
   the copies cost 10% instead. Else, or with MIPS32_NO_THREADED, a switch with shared tails */
#if (defined(__GNUC__) || defined(__clang__)) && !defined(MIPS32_NO_THREADED)
#define FR_THREADED
#endif
/* d is 0 for none: written and cleared again, without a branch; delay is stored, not kept in a register: it is only read after the loop */
#define FR_SET() do { r[d] = v; r[0] = 0; pc = npc; npc += 4; s->delay = (int)nd; cyc++; } while (0)
/* a taken branch or jump: t is live only from its handler to here */
#define FR_JMPSET() do { r[d] = v; r[0] = 0; pc = npc; npc = t; s->delay = 1; cyc++; } while (0)
#ifdef FR_THREADED
#define FR_OP(n) L_##n
#define FR_DEFAULT L_def
#define FR_DISPATCH() do { \
	if (cyc >= lim || rotr_32(pc - fva, 2) >= fwords) goto out; \
	op = le32((const uint8_t *)(fbase + pc)); rs = r[RS(op)]; rt = r[RT(op)]; d = 0; nd = 0; \
	goto *fr_tab[op >> 26]; \
} while (0)
#define FR_NEXT() do { FR_SET(); FR_DISPATCH(); } while (0)
#define FR_JMP() do { FR_JMPSET(); FR_DISPATCH(); } while (0)
#else
#define FR_OP(n) case n
#define FR_DEFAULT default
#define FR_NEXT() goto set
#define FR_JMP() goto jmp
#endif

/* Runs instructions that raise no exception, touch only registers and direct memory and leave CP0 alone, until
   another one or lim; returns whether any ran. Kept to few live values, so that pc, npc and cyc stay in registers. */
static int fast_run(mips32_state * const s, const uint64_t lim)
{
	uint32_t * const r = REGS(s);
	uint32_t pc = s->pc, npc = s->npc;
	/* FETCHABLE and FETCH with the window in registers */
	const uint32_t fva = s->fva, fwords = s->fwords;
	const uintptr_t fbase = s->fbase;
	uint64_t cyc = s->cycles;
	/* address translation for loads and stores: bit 30 what kuseg adds (0 under ERL), bit 0 kernel mode */
	const uint32_t xl = ((s->status & ST_ERL) ? 0 : 0x40000000u) | (uint32_t)!USER(s);
	uint32_t op, rs, rt, v, t, nd;
	unsigned d;
	const mips32_region *m;
#ifdef FR_THREADED
	static const void * const fr_tab[64] = {
		&&FR_OP(0x00), &&FR_OP(0x01), &&FR_OP(0x02), &&FR_OP(0x03), &&FR_OP(0x04), &&FR_OP(0x05), &&FR_OP(0x06), &&FR_OP(0x07),
		&&FR_OP(0x08), &&FR_OP(0x09), &&FR_OP(0x0A), &&FR_OP(0x0B), &&FR_OP(0x0C), &&FR_OP(0x0D), &&FR_OP(0x0E), &&FR_OP(0x0F),
		&&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_OP(0x14), &&FR_OP(0x15), &&FR_OP(0x16), &&FR_OP(0x17),
		&&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_OP(0x1C), &&FR_DEFAULT, &&FR_DEFAULT, &&FR_OP(0x1F),
		&&FR_OP(0x20), &&FR_OP(0x21), &&FR_DEFAULT, &&FR_OP(0x23), &&FR_OP(0x24), &&FR_OP(0x25), &&FR_DEFAULT, &&FR_DEFAULT,
		&&FR_OP(0x28), &&FR_OP(0x29), &&FR_DEFAULT, &&FR_OP(0x2B), &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_OP(0x2F),
		&&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_OP(0x33), &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT,
		&&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT, &&FR_DEFAULT
	};
	FR_DISPATCH();
	{
#else
	while (cyc < lim) {
		if (rotr_32(pc - fva, 2) >= fwords) break;
		op = le32((const uint8_t *)(fbase + pc));
		rs = r[RS(op)];
		rt = r[RT(op)];
		d = 0;
		nd = 0;
		switch (op >> 26) {
#endif
		FR_OP(0x00):
			/* NOP (sll $0, $0, 0), most of some firmware's delay slots: no funct dispatch */
			if (!op) { v = 0; FR_NEXT(); }
			switch (FUNCT(op)) {
			case 0x00: d = RD(op); v = rt << SA(op); break;
			case 0x02: d = RD(op); v = (op & (1u << 21)) ? rotr_32(rt, SA(op)) : rt >> SA(op); break;
			case 0x03: d = RD(op); v = sar_32(rt, SA(op)); break;
			case 0x04: d = RD(op); v = rt << (rs & 31); break;
			case 0x06: d = RD(op); v = (op & (1u << 6)) ? rotr_32(rt, rs & 31) : rt >> (rs & 31); break;
			case 0x07: d = RD(op); v = sar_32(rt, rs & 31); break;
			case 0x08: t = rs; v = 0; FR_JMP();
			case 0x09: d = RD(op); v = pc + 8; t = rs; FR_JMP();
			case 0x0A: d = rt ? 0 : RD(op); v = rs; break;
			case 0x0B: d = rt ? RD(op) : 0; v = rs; break;
			case 0x0F: v = 0; break;
			case 0x10: d = RD(op); v = s->hi; break;
			case 0x11: s->hi = rs; v = 0; break;
			case 0x12: d = RD(op); v = s->lo; break;
			case 0x13: s->lo = rs; v = 0; break;
			case 0x18: { const int64_t p = (int64_t)(int32_t)rs * (int32_t)rt; s->lo = (uint32_t)p; s->hi = (uint32_t)((uint64_t)p >> 32); v = 0; break; }
			case 0x19: { const uint64_t p = (uint64_t)rs * rt; s->lo = (uint32_t)p; s->hi = (uint32_t)(p >> 32); v = 0; break; }
			case 0x1A:
				if (rt) {
					if (rs == 0x80000000u && rt == 0xFFFFFFFFu) { s->lo = 0x80000000u; s->hi = 0; }
					else { s->lo = (uint32_t)((int32_t)rs / (int32_t)rt); s->hi = (uint32_t)((int32_t)rs % (int32_t)rt); }
				}
				cyc += 34; v = 0;
				break;
			case 0x1B: if (rt) { s->lo = rs / rt; s->hi = rs % rt; } cyc += 34; v = 0; break;
			case 0x20: v = rs + rt; if (~(rs ^ rt) & (rs ^ v) & 0x80000000u) goto out; d = RD(op); break;
			case 0x21: d = RD(op); v = rs + rt; break;
			case 0x22: v = rs - rt; if ((rs ^ rt) & (rs ^ v) & 0x80000000u) goto out; d = RD(op); break;
			case 0x23: d = RD(op); v = rs - rt; break;
			case 0x24: d = RD(op); v = rs & rt; break;
			case 0x25: d = RD(op); v = rs | rt; break;
			case 0x26: d = RD(op); v = rs ^ rt; break;
			case 0x27: d = RD(op); v = ~(rs | rt); break;
			case 0x2A: d = RD(op); v = (int32_t)rs < (int32_t)rt; break;
			case 0x2B: d = RD(op); v = rs < rt; break;
			default: goto out;
			}
			FR_NEXT();
		FR_OP(0x01):
			switch (RT(op)) {
			case 0x00: v = 0; if ((int32_t)rs < 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; break;
			case 0x01: v = 0; if ((int32_t)rs >= 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; break;
			case 0x10: d = 31; v = pc + 8; if ((int32_t)rs < 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; break;
			case 0x11: d = 31; v = pc + 8; if ((int32_t)rs >= 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; break;
			default: goto out;
			}
			FR_NEXT();
		FR_OP(0x02): t = ((pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2); v = 0; FR_JMP();
		FR_OP(0x03): t = ((pc + 4) & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2); d = 31; v = pc + 8; FR_JMP();
		FR_OP(0x04): v = 0; if (rs == rt) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; FR_NEXT();
		FR_OP(0x05): v = 0; if (rs != rt) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; FR_NEXT();
		FR_OP(0x06): v = 0; if ((int32_t)rs <= 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; FR_NEXT();
		FR_OP(0x07): v = 0; if ((int32_t)rs > 0) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); } nd = 1; FR_NEXT();
		FR_OP(0x08): v = rs + SIMM(op); if (~(rs ^ SIMM(op)) & (rs ^ v) & 0x80000000u) goto out; d = RT(op); FR_NEXT();
		FR_OP(0x09): d = RT(op); v = rs + SIMM(op); FR_NEXT();
		FR_OP(0x0A): d = RT(op); v = (int32_t)rs < (int32_t)SIMM(op); FR_NEXT();
		FR_OP(0x0B): d = RT(op); v = rs < SIMM(op); FR_NEXT();
		FR_OP(0x0C): d = RT(op); v = rs & UIMM(op); FR_NEXT();
		FR_OP(0x0D): d = RT(op); v = rs | UIMM(op); FR_NEXT();
		FR_OP(0x0E): d = RT(op); v = rs ^ UIMM(op); FR_NEXT();
		FR_OP(0x0F): d = RT(op); v = UIMM(op) << 16; FR_NEXT();
		FR_OP(0x14): FR_OP(0x15): FR_OP(0x16): FR_OP(0x17): {
			const int c = (op >> 26) == 0x14 ? rs == rt : (op >> 26) == 0x15 ? rs != rt : (op >> 26) == 0x16 ? (int32_t)rs <= 0 : (int32_t)rs > 0;
			v = 0;
			if (c) { t = pc + 4 + (SIMM(op) << 2); FR_JMP(); }
			/* not taken: the delay slot is skipped */
			npc += 4;
			FR_NEXT();
		}
		FR_OP(0x1C): {
			uint64_t acc = ((uint64_t)s->hi << 32) | s->lo;
			switch (FUNCT(op)) {
			case 0x00: acc += (uint64_t)((int64_t)(int32_t)rs * (int32_t)rt); break;
			case 0x01: acc += (uint64_t)rs * rt; break;
			case 0x02: d = RD(op); v = (uint32_t)((int64_t)(int32_t)rs * (int32_t)rt); cyc++; FR_NEXT();
			case 0x04: acc -= (uint64_t)((int64_t)(int32_t)rs * (int32_t)rt); break;
			case 0x05: acc -= (uint64_t)rs * rt; break;
			case 0x20: d = RD(op); v = clz_32(rs); FR_NEXT();
			case 0x21: d = RD(op); v = clz_32(~rs); FR_NEXT();
			default: goto out;
			}
			s->lo = (uint32_t)acc;
			s->hi = (uint32_t)(acc >> 32);
			v = 0;
			FR_NEXT();
		}
		FR_OP(0x1F):
			switch (FUNCT(op)) {
			case 0x00: d = RT(op); v = (rs >> SA(op)) & mask32(RD(op) + 1); break;
			case 0x04: { const uint32_t mk = mask32(RD(op) - SA(op) + 1) << SA(op); d = RT(op); v = (rt & ~mk) | ((rs << SA(op)) & mk); break; }
			case 0x20:
				switch (SA(op)) {
				case 0x02: d = RD(op); v = wsbh32(rt); break;
				case 0x10: d = RD(op); v = (uint32_t)(int32_t)(int8_t)rt; break;
				case 0x18: d = RD(op); v = (uint32_t)(int32_t)(int16_t)rt; break;
				default: goto out;
				}
				break;
			default: goto out;
			}
			FR_NEXT();
		FR_OP(0x23): {
			/* LW, the most frequent load, without the size dispatch */
			const uint32_t ea = rs + SIMM(op);
			uint32_t pa;
			if (ea & 3) goto out;
			if (ea < 0x80000000u) pa = ea + (xl & 0x40000000u);
			else if ((xl & 1) && ea < 0xC0000000u) pa = ea & 0x1FFFFFFFu;
			else goto out;
			m = &s->region[s->dslot];
			if (!(pa - m->base < m->size && m->size - (pa - m->base) >= 4)) {
				const int k = direct_slot(s, pa, 4, 0);
				if (k < 0) goto out;
				s->dslot = k;
				m = &s->region[k];
			}
			v = le32(m->rd + (pa - m->base));
			d = RT(op);
			FR_NEXT();
		}
		FR_OP(0x20): FR_OP(0x21): FR_OP(0x24): FR_OP(0x25): {
			const uint32_t sz = ((op >> 26) & 1) ? 2 : 1;
			const uint8_t *h;
			uint32_t pa;
			const uint32_t ea = rs + SIMM(op);
			if (ea & (sz - 1)) goto out;
			if (ea < 0x80000000u) pa = ea + (xl & 0x40000000u);
			else if ((xl & 1) && ea < 0xC0000000u) pa = ea & 0x1FFFFFFFu;
			else goto out;
			m = &s->region[s->dslot];
			if (!(pa - m->base < m->size && m->size - (pa - m->base) >= sz)) {
				const int k = direct_slot(s, pa, sz, 0);
				if (k < 0) goto out;
				s->dslot = k;
				m = &s->region[k];
			}
			h = m->rd + (pa - m->base);
			switch (op >> 26) {
			case 0x20: v = (uint32_t)(int32_t)(int8_t)h[0]; break;
			case 0x21: v = (uint32_t)(int32_t)(int16_t)(h[0] | h[1] << 8); break;
			case 0x24: v = h[0]; break;
			default: v = (uint32_t)(h[0] | h[1] << 8); break;
			}
			d = RT(op);
			FR_NEXT();
		}
		FR_OP(0x28): FR_OP(0x29): FR_OP(0x2B): {
			const uint32_t sz = (op >> 26) == 0x2B ? 4 : (op >> 26) == 0x29 ? 2 : 1;
			uint8_t *h;
			int k;
			uint32_t pa;
			const uint32_t ea = rs + SIMM(op);
			if (ea & (sz - 1)) goto out;
			if (ea < 0x80000000u) pa = ea + (xl & 0x40000000u);
			else if ((xl & 1) && ea < 0xC0000000u) pa = ea & 0x1FFFFFFFu;
			else goto out;
			/* the last store's region first */
			k = s->wslot;
			if (!(k >= 0 && pa - s->region[k].base < s->region[k].size && s->region[k].size - (pa - s->region[k].base) >= sz)) {
				if ((k = direct_slot(s, pa, sz, 1)) < 0) goto out;
				s->wslot = k;
			}
			m = &s->region[k];
			h = m->wr + (pa - m->base);
			h[0] = (uint8_t)rt;
			if (sz > 1) h[1] = (uint8_t)(rt >> 8);
			if (sz > 2) { h[2] = (uint8_t)(rt >> 16); h[3] = (uint8_t)(rt >> 24); }
			v = 0;
			FR_NEXT();
		}
		FR_OP(0x2F): if (!(xl & 1)) goto out; v = 0; FR_NEXT();
		FR_OP(0x33): v = 0; FR_NEXT();
		FR_DEFAULT: goto out;
#ifdef FR_THREADED
	}
#else
		}
	set:
		FR_SET();
		continue;
	jmp:
		FR_JMPSET();
	}
#endif
out:
	s->pc = pc;
	s->npc = npc;
	/* every instruction takes a cycle at least */
	if (cyc == s->cycles) return 0;
	s->cycles = cyc;
	return 1;
}

/* registers an instruction reads (mask) and writes (*rd, -1: none or not always); -2: other register sets */
#define R1(n) (1u << (n))
static int gpr_use(const uint32_t op, uint32_t * const rd)
{
	const unsigned rs = RS(op), rt = RT(op), d = RD(op);
	*rd = R1(rs) | R1(rt);
	switch (op >> 26) {
	case 0x00:
		switch (FUNCT(op)) {
		case 0x00: case 0x02: case 0x03: *rd = R1(rt); return (int)d;
		case 0x04: case 0x06: case 0x07: return (int)d;
		case 0x08: *rd = R1(rs); return -1;
		case 0x09: *rd = R1(rs); return (int)d;
		case 0x0A: case 0x0B: *rd |= R1(d); return -1;
		case 0x0C: case 0x0D: case 0x0F: *rd = 0; return -1;
		case 0x10: case 0x12: *rd = 0; return (int)d;
		case 0x11: case 0x13: *rd = R1(rs); return -1;
		case 0x18: case 0x19: case 0x1A: case 0x1B: return -1;
		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: case 0x2A: case 0x2B: return (int)d;
		case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x36: return -1;
		}
		return -2;
	case 0x01:
		*rd = R1(rs);
		if (rt >= 0x10 && rt <= 0x13) return 31;
		return -1;
	case 0x02: *rd = 0; return -1;
	case 0x03: *rd = 0; return 31;
	case 0x04: case 0x05: case 0x14: case 0x15: return -1;
	case 0x06: case 0x07: case 0x16: case 0x17: *rd = R1(rs); return -1;
	case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: *rd = R1(rs); return (int)rt;
	case 0x0F: *rd = 0; return (int)rt;
	case 0x10:
		if (op & (1u << 25)) { *rd = 0; return FUNCT(op) == 0x20 ? -1 : -2; }
		if (rs == 0x00 || rs == 0x0B) { *rd = 0; return (int)rt; }
		return -2;
	case 0x1C:
		switch (FUNCT(op)) {
		case 0x00: case 0x01: case 0x04: case 0x05: return -1;
		case 0x02: return (int)d;
		case 0x20: case 0x21: *rd = R1(rs); return (int)d;
		}
		return -2;
	case 0x1F:
		switch (FUNCT(op)) {
		case 0x00: *rd = R1(rs); return (int)rt;
		case 0x04: return (int)rt;
		case 0x20: *rd = R1(rt); return (int)d;
		case 0x3B: *rd = 0; return (int)rt;
		}
		return -2;
	case 0x20: case 0x21: case 0x23: case 0x24: case 0x25: case 0x30: *rd = R1(rs); return (int)rt;
	case 0x22: case 0x26: return (int)rt;
	case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2E: case 0x2F: case 0x33: return -1;
	case 0x38: return -1;
	}
	return -2;
}

/* before an instruction while a register is uncertain: wait for its bits if read (not by an AND clearing them);
   returns the register written for careful_post, or -1 */
typedef struct careful { unsigned gen, seq; int wr; } careful;

#if defined(__GNUC__)
__attribute__((noinline))
#elif defined(_MSC_VER)
__declspec(noinline)
#endif
static void careful_pre(mips32_state * const s, careful * const k)
{
	uint32_t bits = 0, op, rd, m, other;
	const unsigned r = s->prov_reg;
	int wr, safe = 0;

	k->wr = -1;
	if (s->bus.settle && s->bus.settle(s->bus.ctx, s->prov_tok, 0, &bits)) { prov_apply(s, bits); return; }
	if (!FETCHABLE(s, s->pc) || ++s->prov_n > 256) { mips32_settle(s); return; }
	op = FETCH(s, s->pc);
	wr = gpr_use(op, &rd);
	if (wr == -2) { mips32_settle(s); return; }
	if ((s->srsctl & 7) != s->prov_set) return;
	if (rd >> r & 1) {
		m = prov_regmask(s);
		if ((op >> 26) == 0x00 && FUNCT(op) == 0x24 && RS(op) != RT(op)) {
			other = REGS(s)[RS(op) == r ? RT(op) : RS(op)];
			safe = !(other & m);
		} else if ((op >> 26) == 0x0C)
			safe = !(UIMM(op) & m);
		if (!safe) { mips32_settle(s); return; }
	}
	if (wr == (int)r) {
		k->wr = wr;
		k->gen = s->prov_gen;
		k->seq = s->exc_seq;
	}
}

static int irq_pending(const mips32_state * const s)
{
	return s->eic_ripl > (int)ST_IPL(s->status) && (s->status & (ST_IE | ST_EXL | ST_ERL)) == ST_IE;
}

int mips32_run(mips32_state *s, int cycles)
{
	const uint64_t start = s->cycles, end = start + (uint64_t)(cycles > 0 ? cycles : 0);

	s->stop = 0;
	s->count_at = s->c0 = s->cycles;
	count_arm(s);
	STATUS_CHANGED(s);
	while (s->cycles < end && !s->stop) {
		s->c0 = s->cycles;
		if (s->irq_chk) {
			s->irq_chk = 0;
			if (irq_pending(s)) {
				s->cur_pc = s->pc;
				s->cur_delay = s->delay;
				s->skip_pc = s->pc;
				take_exception(s, MIPS32_EXC_INT, 0);
				if (s->stop) break;
			}
		}
		if (!s->waiting && !s->prov && fast_run(s, end < s->ti_at ? end : s->ti_at)) {
		} else if (s->waiting) {
			uint64_t burn = end - s->cycles;
			count_sync(s, s->cycles);
			if (!(s->cause & CA_TI)) {
				uint64_t need = 2 * (uint64_t)(uint32_t)(s->compare - s->count);
				need = need > (uint64_t)s->count_half ? need - (uint64_t)s->count_half : 0;
				if (need && need < burn) burn = need;
			}
			s->cycles += burn;
		} else {
			careful k;
			k.wr = -1;
			if (s->prov) careful_pre(s, &k);
			step(s);
			/* written whole without being read: known */
			if (k.wr >= 0 && s->prov && s->prov_gen == k.gen && s->exc_seq == k.seq) s->prov = 0;
		}
		if (s->cycles >= s->ti_at) {
			s->cause |= CA_TI;
			s->stop = 1;
			s->ti_at = NEVER;
		}
	}
	count_sync(s, s->cycles);
	s->c0 = s->cycles;
	return (int)(s->cycles - start);
}

uint32_t *mips32_regs(mips32_state *s) { return REGS(s); }

void mips32_direct(mips32_state *s, int slot, uint32_t base, uint32_t size, const uint8_t *rd, uint8_t *wr)
{
	mips32_region *m;
	if (slot < 0 || slot >= MIPS32_REGIONS) return;
	m = &s->region[slot];
	m->base = base;
	m->size = rd ? size : 0;
	m->rd = rd;
	m->wr = wr;
	s->fwords = 0;
	s->wslot = -1;
}

void mips32_set_eic(mips32_state *s, int ripl, int vector, int srs)
{
	s->irq_chk = 1;
	s->eic_ripl = ripl;
	s->eic_vector = vector;
	s->eic_srs = srs;
}

int mips32_timer_irq(const mips32_state *s) { return (s->cause & CA_TI) != 0; }
int mips32_soft_irq(const mips32_state *s) { return (int)((s->cause >> 8) & 3); }

void mips32_reset(mips32_state *s)
{
	mips32_settle(s); /* the registers stay */
	s->unc = 0;
	s->pc = 0xBFC00000u;
	s->npc = s->pc + 4;
	s->delay = 0;
	s->status = ST_BEV | ST_ERL;
	s->cause = 0;
	s->srsctl = 0;
	s->srsmap = 0;
	s->intctl = 0;
	s->hwrena = 0;
	s->ebase = 0x80000000u;
	s->config0 = 0x80000582u;
	s->llbit = 0;
	s->waiting = 0;
	s->count_half = 0;
	s->eic_ripl = 0;
	s->eic_vector = 0;
	s->eic_srs = 0;
}

void mips32_init(mips32_state *s, const mips32_bus *bus, int shadow_sets, uint32_t prid)
{
	memset(s, 0, sizeof(*s));
	s->bus = *bus;
	s->shadow_sets = shadow_sets < 1 ? 1 : shadow_sets > 8 ? 8 : shadow_sets;
	s->prid = prid;
	s->wslot = -1;
	mips32_reset(s);
}
