// license:BSD-3-Clause

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* sched_getaffinity */
#endif
#include "prop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PIN_DO   (1u << 0)
#define PIN_SCLK (1u << 1)
#define PIN_DI   (1u << 2)
#define PIN_CS   (1u << 3)
#define PIN_PGM  (1u << 13)
#define PIN_P24  (1u << 24)
#define PIN_P25  (1u << 25)
#define PIN_SCL  (1u << 28)
#define PIN_SDA  (1u << 29)
#define PIN_SND  ((1u << 14) | (1u << 15))

static void do_catch_up(pinheck_prop * const p, uint64_t pic_cycle);
static void do_pic_pins(pinheck_prop * const p, uint64_t pic_cycle, uint32_t pins);

static const uint32_t rate_num[8] = { 3, 1, 13, 13, 13, 13, 13, 13 };
static const uint32_t rate_den[8] = { 20, 4000, 160, 160, 80, 40, 20, 10 };

static const prop_seg *seg_pic(const pinheck_prop * const p, uint64_t a)
{
	int k = p->nseg - 1;
	while (k > 0 && p->seg[k].pic0 > a) k--;
	return &p->seg[k];
}

static uint64_t to_prop(const pinheck_prop * const p, uint64_t a)
{
	const prop_seg *s = seg_pic(p, a);
	if (a < s->pic0) return s->prop0;
	return s->prop0 + (a - s->pic0) * s->num / s->den;
}

static uint64_t to_pic(const pinheck_prop * const p, uint64_t t)
{
	int k = p->nseg - 1;
	while (k > 0 && p->seg[k].prop0 > t) k--;
	const prop_seg * const s = &p->seg[k];
	if (t < s->prop0) return s->pic0;
	return s->pic0 + ((t - s->prop0) * s->den + s->num - 1) / s->num;
}

/* the clock segments changed: convert the queued edges again */
static void retime(pinheck_prop * const p)
{
	int k;
	for (k = 0; k < p->count; k++) {
		prop_edge * const e = &p->edge[(p->head + k) % PROP_EDGES];
		e->prop = to_prop(p, e->pic);
	}
}

static void add_seg(pinheck_prop * const p, uint64_t t, uint8_t cfg)
{
	const uint64_t a = to_pic(p, t);
	if (p->nseg == PROP_SEGS) {
		memmove(p->seg, p->seg + 1, sizeof(p->seg[0]) * (PROP_SEGS - 1));
		p->nseg--;
	}
	p->seg[p->nseg].pic0 = a;
	p->seg[p->nseg].prop0 = t;
	p->seg[p->nseg].num = rate_num[cfg & 7];
	p->seg[p->nseg].den = rate_den[cfg & 7];
	p->nseg++;
	retime(p);
}

static uint32_t pins_in(void *ctx, uint64_t t)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	uint32_t v = p->base_pins;
	int k;
	for (k = 0; k < p->count; k++) {
		const prop_edge * const e = &p->edge[(p->head + k) % PROP_EDGES];
		if (e->prop > t) break;
		v = e->pins;
	}
	return v | p->ee_bits | (p->sd_do ? PIN_DO : 0) | PIN_CS | PIN_PGM;
}

static uint64_t pins_next(void *ctx, uint64_t t)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	int k;
	for (k = 0; k < p->count; k++) {
		uint64_t e = p->edge[(p->head + k) % PROP_EDGES].prop;
		if (e > t) return e;
	}
	return P8X32A_NEVER;
}

/* each device only acts on changes of its own pins */
static void pins_out(void *ctx, uint64_t t, uint32_t out, uint32_t dir)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	uint32_t ch;
	/* a lazy cog's pins as last sent */
	if (p->lz_mask) {
		out = (out & ~p->lz_mask) | p->lz_out;
		dir = (dir & ~p->lz_mask) | p->lz_dir;
	}
	ch = p->po_ok ? (out ^ p->po_out) | (dir ^ p->po_dir) : 0xFFFFFFFFu;
	p->po_out = out;
	p->po_dir = dir;
	p->po_ok = 1;
	if (ch & PIN_P25) {
		int tx = (dir & PIN_P25) ? (out & PIN_P25) != 0 : 1;
		if (p->tx && tx != p->tx_level) {
			p->tx_level = tx;
			p->tx(p->tx_ctx, to_pic(p, t), tx);
		}
	}
	if (p->snd_pins && (ch & PIN_SND)) p->snd_pins(p->snd_ctx, t, out, dir);
	if (ch & (PIN_SCL | PIN_SDA)) {
		int scl = (dir & PIN_SCL) ? (out & PIN_SCL) != 0 : 1;
		int sda = (dir & PIN_SDA) ? (out & PIN_SDA) != 0 : 1;
		p->ee_bits = PIN_SCL | (cat24m01_update(&p->eeprom, scl, sda) ? PIN_SDA : 0);
	}
	if (p->sd && (ch & (PIN_CS | PIN_SCLK | PIN_DI))) {
		int cs = (dir & PIN_CS) ? (out & PIN_CS) != 0 : 1;
		int sclk = (dir & PIN_SCLK) ? (out & PIN_SCLK) != 0 : 0;
		int mosi = (dir & PIN_DI) ? (out & PIN_DI) != 0 : 1;
		p->sd_do = p->sd(p->sd_ctx, cs, sclk, mosi) != 0;
	}
	if (p->pins && (ch & p->pins_mask)) p->pins(p->pins_ctx, t, out, dir);
}

static void lazy(void *ctx, uint64_t t, uint32_t mask, uint32_t out, uint32_t dir)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	(void)t;
	if (p->log && p->chip.lazies <= 16 && mask != p->lz_mask) {
		char msg[48];
		if (mask) sprintf(msg, "prop: a cog runs lazily on pins %08x", (unsigned)mask);
		else strcpy(msg, "prop: the lazy cog runs as the others");
		p->log(p->log_ctx, msg);
	}
	p->lz_mask = mask;
	p->lz_out = out & mask;
	p->lz_dir = dir & mask;
}

static void lazy_pins(void *ctx, uint64_t t, uint32_t out, uint32_t dir)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	p->lz_out = out;
	p->lz_dir = dir;
	if (p->pins_lazy) p->pins_lazy(p->pins_lazy_ctx, t, out, dir);
}

/* pins only pins_lazy watches, so a cog may run lazily on them */
static void lazy_update(pinheck_prop *p)
{
	const uint32_t dev = PIN_DO | PIN_SCLK | PIN_DI | PIN_CS | PIN_SND | PIN_P24 | PIN_P25 | (1u << 26) | PIN_SCL | PIN_SDA;
	p->chip.lazy_ok = p->pins_lazy ? ~(dev | (p->pins_mask & ~p->lazy_mask)) : 0;
}

static void ctr_state(void *ctx, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	if (p->snd_ctr) p->snd_ctr(p->snd_ctx, t, cog, ctr, ctr_reg, frq);
}

static void clkset(void *ctx, uint64_t t, uint8_t cfg)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	char msg[24];
	add_seg(p, t, cfg);
	if (p->log) {
		sprintf(msg, "prop: CLKSET %02x", (unsigned)cfg);
		p->log(p->log_ctx, msg);
	}
	if (cfg & 0x80) p->reset_pending = 1;
}

static void logmsg(void *ctx, const char *msg)
{
	pinheck_prop * const p = (pinheck_prop *)ctx;
	if (p->log) p->log(p->log_ctx, msg);
}

static void restart(pinheck_prop *p, uint64_t t)
{
	const uint64_t a = to_pic(p, t);
	p8x32a_reset(&p->chip, t);
	p->nseg = 1;
	p->seg[0].pic0 = a;
	p->seg[0].prop0 = t;
	p->seg[0].num = rate_num[0];
	p->seg[0].den = rate_den[0];
	cat24m01_init(&p->eeprom, p->eemem, 0);
	p->ee_bits = PIN_SCL | PIN_SDA;
	p->sd_do = 1;
	p->po_ok = 0;
	p->reset_pending = 0;
	retime(p);
}

void prop_init(pinheck_prop * const p, const uint8_t *rom32k, uint8_t *eemem)
{
	p8x32a_bus bus;
	memset(p, 0, sizeof(*p));
	memset(&bus, 0, sizeof(bus));
	bus.ctx = p;
	bus.pins_in = pins_in;
	bus.pins_next = pins_next;
	bus.pins_out = pins_out;
	bus.cog_start = NULL;
	bus.clkset = clkset;
	bus.log = logmsg;
	bus.ctr_state = ctr_state;
	bus.pure_in = ~(PIN_DO | PIN_SDA); /* SD DO and EEPROM SDA answer inside pins_out */
	bus.lazy = lazy;
	bus.lazy_pins = lazy_pins;
	p8x32a_init(&p->chip, &bus);
	memcpy(p->chip.hub + 0x8000, rom32k, 0x8000);
	p->eemem = eemem;
	p->nseg = 1;
	p->seg[0].num = rate_num[0];
	p->seg[0].den = rate_den[0];
	cat24m01_init(&p->eeprom, eemem, 0);
	p->ee_bits = PIN_SCL | PIN_SDA;
	p->sd_do = 1;
}

void prop_attach_sd(pinheck_prop * const p, prop_spi_fn fn, void *ctx)
{
	p->sd = fn;
	p->sd_ctx = ctx;
	p->sd_do = 1;
	p->po_ok = 0;
}

void prop_set_log(pinheck_prop * const p, prop_log_fn fn, void *ctx)
{
	p->log = fn;
	p->log_ctx = ctx;
}

void prop_set_tx(pinheck_prop * const p, prop_tx_fn fn, void *ctx)
{
	p->tx = fn;
	p->tx_ctx = ctx;
	p->po_ok = 0;
	p->tx_level = 1;
}

void prop_set_pins(pinheck_prop * const p, prop_pins_fn fn, void *ctx)
{
	p->pins = fn;
	p->pins_ctx = ctx;
	p->pins_mask = 0xFFFFFFFFu;
	lazy_update(p);
}

void prop_set_pins_mask(pinheck_prop * const p, uint32_t mask)
{
	p->pins_mask = mask;
	p->po_ok = 0;
	lazy_update(p);
}

/* set before the Propeller runs: fn gets a lazy cog's pin changes within mask */
void prop_set_pins_lazy(pinheck_prop * const p, prop_pins_fn fn, void *ctx, uint32_t mask)
{
	p->pins_lazy = fn;
	p->pins_lazy_ctx = ctx;
	p->lazy_mask = mask;
	lazy_update(p);
}

void prop_set_sound(pinheck_prop * const p, prop_ctr_fn ctr, prop_pins_fn pins, void *ctx)
{
	p->snd_ctr = ctr;
	p->snd_pins = pins;
	p->snd_ctx = ctx;
	p->po_ok = 0;
}

void prop_reset(pinheck_prop * const p, uint64_t pic_cycle)
{
	prop_sync(p);
	p->stamp = p->clock ? p->clock(p->clock_ctx) : 0;
	do_catch_up(p, p->pic_last);
	p->pic_last = pic_cycle;
	p->head = p->count = 0;
	p->base_pins = p->last_pins = 0;
	restart(p, p->chip.now);
	p->seg[0].pic0 = pic_cycle;
	retime(p);
}

uint64_t prop_time(pinheck_prop * const p, uint64_t pic_cycle)
{
	prop_sync(p);
	return to_prop(p, pic_cycle);
}

void prop_set_clock(pinheck_prop * const p, prop_clock_fn fn, void *ctx)
{
	p->clock = fn;
	p->clock_ctx = ctx;
}

uint64_t prop_stamp(const pinheck_prop * const p) { return p->stamp; }

static void do_catch_up(pinheck_prop * const p, uint64_t pic_cycle)
{
	uint64_t t;
	if (pic_cycle > p->pic_last) p->pic_last = pic_cycle;
	while ((t = to_prop(p, pic_cycle)) > p->chip.now + 1) {
		p8x32a_run_until(&p->chip, t - 1);
		if (p->reset_pending) restart(p, p->chip.now);
	}
	while (p->count && p->edge[p->head].prop <= p->chip.now) {
		p->base_pins = p->edge[p->head].pins;
		p->head = (p->head + 1) % PROP_EDGES;
		p->count--;
	}
}

static void do_pic_pins(pinheck_prop * const p, uint64_t pic_cycle, uint32_t pins)
{
	if (pic_cycle > p->pic_last) p->pic_last = pic_cycle;
	if (pins == p->last_pins) return;
	if (p->count == PROP_EDGES) do_catch_up(p, pic_cycle);
	p->edge[(p->head + p->count) % PROP_EDGES].pic = pic_cycle;
	p->edge[(p->head + p->count) % PROP_EDGES].pins = pins;
	p->edge[(p->head + p->count) % PROP_EDGES].prop = to_prop(p, pic_cycle);
	p->count++;
	p->last_pins = pins;
}

int prop_p24(pinheck_prop * const p, uint64_t pic_cycle)
{
	uint32_t dir, out;
	prop_catch_up(p, pic_cycle);
	prop_sync(p);
	out = p8x32a_pins(&p->chip, p->chip.now, &dir);
	return (dir & PIN_P24) ? (out & PIN_P24) != 0 : 0;
}

/* PIC32-side calls may run on a worker thread, queued in order with their PIC32 cycle; prop_sync drains the queue before Propeller state is read */
#define PROP_Q 4096
/* a wait spins this long, then blocks; longer on Windows, which wakes blocked threads slowly */
#ifdef _WIN32
#define PROP_SPIN_NS 200000
#else
#define PROP_SPIN_NS 50000
#endif

enum { CMD_PINS, CMD_CATCH_UP, CMD_SAMPLE, CMD_QUIT };

typedef struct prop_cmd { uint64_t pic, stamp; uint32_t pins; int kind; } prop_cmd;

#ifdef PINHECK_NO_THREADS
uint32_t prop_sample(pinheck_prop * const p, uint64_t pic_cycle) { (void)p; (void)pic_cycle; return 0; }
int prop_sample_get(pinheck_prop * const p, uint32_t token, int wait) { (void)p; (void)token; (void)wait; return 0; }
int prop_start_thread(pinheck_prop * const p) { (void)p; return -1; }
void prop_stop_thread(pinheck_prop * const p) { (void)p; }
void prop_sync(pinheck_prop * const p) { (void)p; }
static void post(pinheck_prop * const p, int kind, uint64_t pic, uint32_t pins) { (void)p; (void)kind; (void)pic; (void)pins; }
#else
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
// mingw.org headers lack both (x86 only)
#define MemoryBarrier() __asm__ __volatile__("lock; orl $0,(%%esp)" ::: "memory")
#define YieldProcessor() __asm__ __volatile__("rep; nop")
#endif
typedef struct prop_os { HANDLE th, ev, sev; } prop_os;
static unsigned get_acq(volatile unsigned *v) { unsigned r = *v; MemoryBarrier(); return r; }
static void put_rel(volatile unsigned *v, unsigned x) { MemoryBarrier(); *v = x; }
static unsigned xchg(volatile unsigned *v, unsigned x) { return (unsigned)InterlockedExchange((volatile LONG *)v, (LONG)x); }
static uint64_t self_id(void) { return GetCurrentThreadId(); }
#define CPU_RELAX_ANY() YieldProcessor()
static unsigned get_sc(volatile unsigned *v) { MemoryBarrier(); return *v; }
static void put_sc(volatile unsigned *v, unsigned x) { InterlockedExchange((volatile LONG *)v, (LONG)x); }
static uint64_t now_ns(void)
{
	static LARGE_INTEGER f;
	LARGE_INTEGER c;
	if (!f.QuadPart) QueryPerformanceFrequency(&f);
	QueryPerformanceCounter(&c);
	return (uint64_t)(c.QuadPart / f.QuadPart) * 1000000000u + (uint64_t)(c.QuadPart % f.QuadPart) * 1000000000u / (uint64_t)f.QuadPart;
}
static int cpus_allowed(void)
{
	DWORD_PTR pm, sm;
	int n = 0;
	if (!GetProcessAffinityMask(GetCurrentProcess(), &pm, &sm) || !pm) return 2;
	for (; pm; pm &= pm - 1) n++;
	return n;
}
static void worker_loop(void *arg);
static unsigned __stdcall os_entry(void *arg) { worker_loop(arg); return 0; }
/* CRT thread start, at the emulation thread's priority */
static int os_start(prop_os * const o, void *arg)
{
	o->ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	o->sev = CreateEvent(NULL, FALSE, FALSE, NULL);
	o->th = o->ev && o->sev ? (HANDLE)_beginthreadex(NULL, 0, os_entry, arg, CREATE_SUSPENDED, NULL) : NULL;
	if (!o->th) {
		if (o->ev) CloseHandle(o->ev);
		if (o->sev) CloseHandle(o->sev);
		return -1;
	}
	SetThreadPriority(o->th, GetThreadPriority(GetCurrentThread()));
	ResumeThread(o->th);
	return 0;
}
static void os_join(prop_os * const o)
{
	WaitForSingleObject(o->th, INFINITE);
	CloseHandle(o->th);
	CloseHandle(o->ev);
	CloseHandle(o->sev);
}
/* worker waits for a post; a post wakes it */
static void os_sleep(prop_os * const o) { WaitForSingleObject(o->ev, INFINITE); }
static void os_wake(prop_os * const o) { SetEvent(o->ev); }
/* poster waits for *head to reach want; the worker wakes it */
static void os_wait_head(prop_os * const o, volatile unsigned *head, unsigned want, volatile unsigned *waiting, volatile unsigned *wantp)
{
	*wantp = want;
	xchg(waiting, 1);
	while ((int)(get_sc(head) - want) < 0) WaitForSingleObject(o->sev, INFINITE);
	xchg(waiting, 0);
}
static void os_head_moved(prop_os * const o) { SetEvent(o->sev); }
#else
#include <pthread.h>
#include <sched.h>
#include <time.h>
typedef struct prop_os { pthread_t th; pthread_mutex_t m; pthread_cond_t c, sc; int wake; } prop_os;
static unsigned get_acq(volatile unsigned *v) { return __atomic_load_n(v, __ATOMIC_ACQUIRE); }
static void put_rel(volatile unsigned *v, unsigned x) { __atomic_store_n(v, x, __ATOMIC_RELEASE); }
static unsigned xchg(volatile unsigned *v, unsigned x) { return __atomic_exchange_n(v, x, __ATOMIC_SEQ_CST); }
#if defined(__x86_64__) || defined(__i386__)
#define CPU_RELAX() __asm__ __volatile__("pause")
#elif defined(__aarch64__) || (defined(__arm__) && (__ARM_ARCH >= 7 || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6KZ__)))
#define CPU_RELAX() __asm__ __volatile__("yield") /* ARMv6 before 6K has no yield */
#else
#define CPU_RELAX() ((void)0)
#endif
static uint64_t self_id(void)
{
	pthread_t t = pthread_self();
	uint64_t r = 0;
	memcpy(&r, &t, sizeof(t) < sizeof(r) ? sizeof(t) : sizeof(r));
	return r;
}
#define CPU_RELAX_ANY() CPU_RELAX()
static unsigned get_sc(volatile unsigned *v) { return __atomic_load_n(v, __ATOMIC_SEQ_CST); }
static void put_sc(volatile unsigned *v, unsigned x) { __atomic_store_n(v, x, __ATOMIC_SEQ_CST); }
static uint64_t now_ns(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec;
}
static int cpus_allowed(void)
{
#ifdef __linux__
	cpu_set_t s;
	if (sched_getaffinity(0, sizeof(s), &s)) return 2;
	return CPU_COUNT(&s);
#else
	return 2;
#endif
}
static void worker_loop(void *arg);
static void *os_entry(void *arg) { worker_loop(arg); return NULL; }
static int os_start(prop_os * const o, void *arg)
{
	pthread_mutex_init(&o->m, NULL);
	pthread_cond_init(&o->c, NULL);
	pthread_cond_init(&o->sc, NULL);
	if (!pthread_create(&o->th, NULL, os_entry, arg)) return 0;
	pthread_cond_destroy(&o->c);
	pthread_cond_destroy(&o->sc);
	pthread_mutex_destroy(&o->m);
	return -1;
}
static void os_join(prop_os * const o)
{
	pthread_join(o->th, NULL);
	pthread_cond_destroy(&o->c);
	pthread_cond_destroy(&o->sc);
	pthread_mutex_destroy(&o->m);
}
/* worker waits for a post; a post wakes it */
static void os_sleep(prop_os * const o)
{
	pthread_mutex_lock(&o->m);
	while (!o->wake) pthread_cond_wait(&o->c, &o->m);
	o->wake = 0;
	pthread_mutex_unlock(&o->m);
}
static void os_wake(prop_os * const o)
{
	pthread_mutex_lock(&o->m);
	o->wake = 1;
	pthread_cond_signal(&o->c);
	pthread_mutex_unlock(&o->m);
}
/* poster waits for *head to reach want; the worker wakes it */
static void os_wait_head(prop_os * const o, volatile unsigned *head, unsigned want, volatile unsigned *waiting, volatile unsigned *wantp)
{
	pthread_mutex_lock(&o->m);
	*wantp = want;
	xchg(waiting, 1);
	while ((int)(get_sc(head) - want) < 0) pthread_cond_wait(&o->sc, &o->m);
	xchg(waiting, 0);
	pthread_mutex_unlock(&o->m);
}
static void os_head_moved(prop_os * const o)
{
	pthread_mutex_lock(&o->m);
	pthread_cond_signal(&o->sc);
	pthread_mutex_unlock(&o->m);
}
#endif

static uint64_t spin_ns = PROP_SPIN_NS;

/* one step of a bounded spin; 1 after spin_ns */
static int spin(uint64_t * const t0, unsigned * const k)
{
	if (!(*k & 63)) {
		uint64_t n = now_ns();
		if (!*k) *t0 = n;
		else if (n - *t0 >= spin_ns) return 1;
	}
	++*k;
	CPU_RELAX_ANY();
	return 0;
}

/* the counters grouped by the thread that writes them, each group on cache lines of its own */
typedef struct prop_worker {
	prop_os os;
	prop_cmd q[PROP_Q];
	char pad0[PROP_PAD];
	/* the worker's: written after each command */
	volatile unsigned head;          /* next to run */
	char pad1[PROP_PAD];
	/* the poster's: written for each post */
	volatile unsigned tail;          /* next free */
	volatile unsigned sleeping;
	unsigned head_seen;              /* head as the poster last read it: behind, never ahead */
	char pad2[PROP_PAD];
	/* the poster's, written only when it blocks; the worker reads them after each command */
	volatile unsigned waiting, want; /* the poster blocks until head reaches want */
	char pad3[PROP_PAD];
} prop_worker;

/* an idle worker blocks until the next post */
static void idle_wait(prop_worker * const w, unsigned h)
{
	if (xchg(&w->sleeping, 1) == 0 && get_acq(&w->tail) == h) os_sleep(&w->os);
	xchg(&w->sleeping, 0);
}

static void wake(prop_worker * const w)
{
	if (xchg(&w->sleeping, 0)) os_wake(&w->os);
}

/* the poster spins, then blocks, until head reaches want */
static void wait_head(prop_worker * const w, unsigned want)
{
	uint64_t t0 = 0;
	unsigned k = 0;
	for (;;) {
		if ((int)(get_acq(&w->head) - want) >= 0) return;
		if (spin(&t0, &k)) break;
	}
	os_wait_head(&w->os, &w->head, want, &w->waiting, &w->want);
}

/* advance head to h and wake a waiting poster */
static void head_moved(prop_worker * const w, unsigned h)
{
	put_sc(&w->head, h);
	if (get_sc(&w->waiting) && (int)(h - w->want) >= 0) os_head_moved(&w->os);
}

static void run_cmd(pinheck_prop * const p, const prop_cmd * const c)
{
	p->stamp = c->stamp;
	if (c->kind == CMD_CATCH_UP) do_catch_up(p, c->pic);
	else if (c->kind == CMD_SAMPLE) {
		/* prop_p24's catch-up and read, on this thread */
		uint32_t dir, out;
		do_catch_up(p, c->pic);
		out = p8x32a_pins(&p->chip, p->chip.now, &dir);
		p->samp_val[c->pins % PROP_SAMPS] = (uint8_t)((dir & PIN_P24) ? (out & PIN_P24) != 0 : 0);
		put_rel(&p->samp_done, c->pins);
	} else do_pic_pins(p, c->pic, c->pins);
}

static void worker_loop(void *arg)
{
	pinheck_prop * const p = (pinheck_prop *)arg;
	prop_worker * const w = (prop_worker *)p->worker;
	unsigned h = w->head, t = h, k = 0;
	uint64_t t0 = 0;
	for (;;) {
		/* tail is read again only once the commands already seen are done: its line stays with the poster */
		if (h == t && (t = get_acq(&w->tail)) == h) {
			if (spin(&t0, &k)) { idle_wait(w, h); k = 0; }
			continue;
		}
		k = 0;
		if (w->q[h % PROP_Q].kind == CMD_QUIT) { put_rel(&w->head, h + 1); break; }
		run_cmd(p, &w->q[h % PROP_Q]);
		head_moved(w, ++h);
	}
}

static void post(pinheck_prop *p, int kind, uint64_t pic, uint32_t pins)
{
	prop_worker * const w = (prop_worker *)p->worker;
	unsigned t = w->tail;
	/* head is read only when the queue looks full by head_seen */
	if (t - w->head_seen >= PROP_Q && t - (w->head_seen = get_acq(&w->head)) >= PROP_Q) wait_head(w, t - PROP_Q + 1);
	prop_cmd * const c = &w->q[t % PROP_Q];
	c->kind = kind;
	c->pic = pic;
	c->pins = pins;
	c->stamp = p->clock ? p->clock(p->clock_ctx) : pic;
	put_rel(&w->tail, t + 1);
	wake(w);
}

/* only the owning thread waits; others read the state as is */
void prop_sync(pinheck_prop * const p)
{
	prop_worker *w;
	if (p->owner != self_id()) return;
	w = (prop_worker *)p->worker;
	if (w) wait_head(w, w->tail);
}

int prop_start_thread(pinheck_prop * const p)
{
	prop_worker *w;
	if (p->worker) return 0;
	/* on one CPU the threads would wait for each other at every handoff */
	if (cpus_allowed() < 2) {
		if (p->log) p->log(p->log_ctx, "prop: one CPU allowed, no worker thread");
		return -1;
	}
	spin_ns = PROP_SPIN_NS;
#ifdef PINHECK_TEST_HOOKS
	{
		/* PINHECK_SPIN_US (1-100000) */
		const char *us = getenv("PINHECK_SPIN_US");
		long long v = us ? strtoll(us, NULL, 10) : 0;
		if (v >= 1 && v <= 100000) spin_ns = (uint64_t)v * 1000;
	}
#endif
	w = (prop_worker *)calloc(1, sizeof(*w));
	if (!w) return -1;
	p->worker = w;
	p->owner = self_id();
	if (os_start(&w->os, p)) {
		free(w);
		p->worker = NULL;
		return -1;
	}
	return 0;
}

void prop_stop_thread(pinheck_prop * const p)
{
	prop_worker * const w = (prop_worker *)p->worker;
	if (!w) return;
	post(p, CMD_QUIT, 0, 0);
	os_join(&w->os);
	free(w);
	p->worker = NULL;
}

/* P24 at pic_cycle, read by the worker; collect with prop_sample_get */
uint32_t prop_sample(pinheck_prop * const p, uint64_t pic_cycle)
{
	prop_worker * const w = (prop_worker *)p->worker;
	uint32_t k = ++p->samp_post;
	p->samp_cmd[k % PROP_SAMPS] = w->tail;
	post(p, CMD_SAMPLE, pic_cycle, k);
	return k;
}

int prop_sample_get(pinheck_prop * const p, uint32_t token, int wait)
{
	if ((int)(get_acq(&p->samp_done) - token) < 0) {
		prop_worker *w = (prop_worker *)p->worker;
		if (!wait) return -1;
		if (w) wait_head(w, p->samp_cmd[token % PROP_SAMPS] + 1);
		get_acq(&p->samp_done);
	}
	return p->samp_val[token % PROP_SAMPS];
}
#endif

void prop_catch_up(pinheck_prop * const p, uint64_t pic_cycle)
{
	if (p->worker) { post(p, CMD_CATCH_UP, pic_cycle, 0); return; }
	p->stamp = p->clock ? p->clock(p->clock_ctx) : pic_cycle;
	do_catch_up(p, pic_cycle);
}

void prop_pic_pins(pinheck_prop * const p, uint64_t pic_cycle, uint32_t pins)
{
	pins &= PROP_PIC_PINS;
	if (p->worker) { post(p, CMD_PINS, pic_cycle, pins); return; }
	p->stamp = p->clock ? p->clock(p->clock_ctx) : pic_cycle;
	do_pic_pins(p, pic_cycle, pins);
}

/* Picks the faster of worker and inline in one-second windows: below 0.95x real time, inline must beat the
   worker's median by 15%; a rejected mode is retried after 10 s, doubling to 320 s. */
#define GOV_MARGIN 1.15
enum { GOV_OFF, GOV_THREADED, GOV_TRY_INLINE, GOV_CHECK_THREADED, GOV_INLINE, GOV_TRY_THREADED };

static void gov_log(pinheck_prop * const p, const char * const what, double a, double b)
{
	char msg[96];
	if (!p->log) return;
	sprintf(msg, "prop: worker thread %s (%.2fx with it, %.2fx without)", what, a, b);
	p->log(p->log_ctx, msg);
}

static double gov_median(const double * const v, int n)
{
	double a[PROP_GOV_RING], t;
	int i, j;
	for (i = 0; i < n; i++) a[i] = v[i];
	for (i = 1; i < n; i++)
		for (j = i; j > 0 && a[j - 1] > a[j]; j--) { t = a[j]; a[j] = a[j - 1]; a[j - 1] = t; }
	return a[n / 2];
}

static void gov_backoff(prop_gov * const g, double w)
{
	g->next = w + g->pause;
	if (g->pause < 320.0) g->pause *= 2.0;
}

/* starts the worker; 0 when it runs */
int prop_gov_start(pinheck_prop * const p, prop_gov * const g, double w, int flip)
{
	memset(g, 0, sizeof(*g));
	if (prop_start_thread(p)) return -1;
	g->state = GOV_THREADED;
	g->next = w + 5.0;
	g->pause = 10.0;
	g->flip = flip;
	return 0;
}

void prop_governor(pinheck_prop * const p, prop_gov * const g, double w, double e)
{
	double s, gap = w - g->wl, de = e - g->el;
	int stall = gap > 3.0 || (gap > 0.25 && gap > 4.0 * g->rate * de);
	if (g->state == GOV_OFF) return;
	g->wl = w;
	g->el = e;
	if (de > 0.0) g->rate = gap / de;
	if (g->w0 == 0.0 || e < g->e0 || stall) { /* start, reset or stall */
		g->w0 = w;
		g->e0 = e;
		g->slow = 0;
		return;
	}
	if (w - g->w0 < 1.0) return;
	s = (e - g->e0) / (w - g->w0);
	g->w0 = w;
	g->e0 = e;
	if (g->flip) {
		if (p->worker) prop_stop_thread(p);
		else prop_start_thread(p);
		if (p->log) p->log(p->log_ctx, p->worker ? "prop: worker thread switched on" : "prop: worker thread switched off");
		return;
	}
	switch (g->state) {
	case GOV_THREADED:
		g->thr[g->n++ % PROP_GOV_RING] = s;
		g->slow = s < 0.95 ? g->slow + 1 : 0;
		if (g->slow < 3 || w < g->next) break;
		prop_stop_thread(p);
		g->state = GOV_TRY_INLINE;
		g->k = 0;
		break;
	case GOV_TRY_INLINE: {
		double thr = gov_median(g->thr, g->n < PROP_GOV_RING ? g->n : PROP_GOV_RING);
		if (g->k++ == 0 || s < g->inl) g->inl = s;
		if (g->k < 2) break;
		g->slow = 0;
		if (g->inl <= thr * GOV_MARGIN) g->state = prop_start_thread(p) ? GOV_INLINE : GOV_THREADED;
		else if (prop_start_thread(p)) { /* one CPU allowed */
			gov_log(p, "off", thr, g->inl);
			g->state = GOV_INLINE;
		} else {
			g->state = GOV_CHECK_THREADED;
			g->warm = 1;
			break;
		}
		gov_backoff(g, w);
		break;
	}
	case GOV_CHECK_THREADED:
		if (g->warm) { g->warm = 0; break; }
		if (g->inl > s * GOV_MARGIN) {
			prop_stop_thread(p);
			gov_log(p, "off", s, g->inl);
			g->state = GOV_INLINE;
		} else g->state = GOV_THREADED;
		gov_backoff(g, w);
		break;
	case GOV_INLINE:
		g->inl = s;
		if (w < g->next) break;
		if (prop_start_thread(p)) gov_backoff(g, w); /* one CPU allowed */
		else {
			g->state = GOV_TRY_THREADED;
			g->warm = 1;
		}
		break;
	case GOV_TRY_THREADED:
		if (g->warm) { g->warm = 0; break; }
		if (s > g->inl * 1.05 || s >= 0.95) {
			gov_log(p, "on", s, g->inl);
			g->state = GOV_THREADED;
			g->n = 0;
			g->next = w + g->pause;
			g->pause = 10.0;
			break;
		}
		prop_stop_thread(p);
		g->state = GOV_INLINE;
		gov_backoff(g, w);
		break;
	}
}
