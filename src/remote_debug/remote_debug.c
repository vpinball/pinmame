// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - core                                          *
 * See remote_debug.h for the interface and the threading model.           *
 ***************************************************************************/
#ifdef REMOTE_DEBUG

#include "remote_debug.h"
#include "http_server.h"
#include "mame.h"
#include "cpuexec.h"
#include "cpuintrf.h"
#include "cpu/m6809/m6809.h"
#include "wpc/core.h"
#include "wpc/wpc.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* Control flags shared between the emulator and the HTTP server thread.
 * sig_atomic_t reads/writes are atomic; no lock needed for these. */
static volatile sig_atomic_t is_paused = 0;
static volatile sig_atomic_t step_requested = 0;
static volatile sig_atomic_t should_quit = 0;

static pthread_mutex_t mame_mutex;
volatile int remote_debug_ready = 0;

/* Provided by api_handler.c (HTTP request dispatch). */
extern void api_handler(const http_request_t *req, http_response_t *resp);
/* Provided by src/wpc/wpc.c; -1 when no WPC banking is active. */
extern int wpc_get_bank(void);
/* Provided by src/wpc/wpc.c; base of the WPC RAM (NULL on non-WPC games). */
extern UINT8 *wpc_ram;

/* ------------------------------------------------------------------ */
/* Breakpoints / watchpoints                                          */
/* ------------------------------------------------------------------ */

/* Condition operators for conditional breakpoints. */
enum {
	COND_NONE = 0,
	COND_EQ,
	COND_NE,
	COND_LT,
	COND_GT,
	COND_LE,
	COND_GE
};

#define MAX_POINTS 128

typedef struct {
	UINT32 adr;
	int bank;             /* -1: any bank */
	int enabled;
	int temp;             /* one-shot (run-to/step-over/step-out) */
	int cond_op;          /* COND_NONE for unconditional */
	int cond_reg;         /* cpu core register id */
	UINT32 cond_val;
	char cond_str[24];    /* original condition text for display */
	UINT32 hit_count;     /* number of (condition-true) hits so far */
	UINT32 ignore_count;  /* skip this many hits before halting */
} breakpoint_t;

static breakpoint_t breakpoints[MAX_POINTS];
static int breakpoint_count = 0;

typedef struct {
	UINT32 adr;
	int len;              /* watched range is [adr, adr+len) */
	int mode;             /* REMOTE_DEBUG_WP_* */
	int bank;             /* -1: any bank (only relevant for 0x4000-0x7FFF) */
	int cond_op;          /* COND_NONE, or compare the byte at adr to cond_val */
	UINT8 cond_val;
	int enabled;
} watchpoint_t;

static watchpoint_t watchpoints[MAX_POINTS];
static int watchpoint_count = 0;

/* ------------------------------------------------------------------ */
/* Memory access trace                                                */
/* ------------------------------------------------------------------ */

typedef struct {
	UINT32 adr;
	int length;
	int write;
	UINT32 pc;
	int cpunum;
	int bank;
} trace_entry_t;

#define TRACE_LOG_SIZE 1024
static trace_entry_t trace_log[TRACE_LOG_SIZE];
static int trace_head = 0;
static int trace_count = 0;

#define TRACE_ADDR_MAX 128
static UINT32 trace_addrs[TRACE_ADDR_MAX];
static int trace_addr_banks[TRACE_ADDR_MAX];  /* -1: any bank */
static int trace_addr_count = 0;

/* ------------------------------------------------------------------ */
/* Timed switch pulses                                                */
/* ------------------------------------------------------------------ */

typedef struct {
	int sw;
	int heldval;   /* value re-asserted every frame during the pulse */
	int offval;    /* value to restore when the pulse expires */
	UINT32 expiry; /* monotonic_ms() deadline */
} pulse_t;

#define PULSE_MAX 32
static pulse_t pulses[PULSE_MAX];
static int pulse_count = 0;

/* ------------------------------------------------------------------ */
/* Object monitoring / action log                                     */
/* ------------------------------------------------------------------ */

typedef struct {
	int type;       /* REMOTE_DEBUG_OBJ_* */
	int id;         /* switch/lamp number or solenoid number */
	int last;       /* last sampled state */
	int brk;        /* halt execution when this object changes */
} monitor_t;

#define MONITOR_MAX 64
static monitor_t monitors[MONITOR_MAX];
static int monitor_count = 0;

/* ------------------------------------------------------------------ */
/* Execution trace / coverage / tracepoints (per-instruction hook)    */
/* ------------------------------------------------------------------ */

/* Execution trace: ring buffer of the most recently executed instructions,
 * for answering "how did I get here" after a halt. */
typedef struct {
	UINT16 pc;
	INT16  bank;
	UINT8  a, b;
	UINT16 x;
} exec_trace_t;

#define EXEC_TRACE_SIZE 8192
static exec_trace_t exec_trace[EXEC_TRACE_SIZE];
static int exec_trace_head = 0;
static int exec_trace_count = 0;
static volatile sig_atomic_t exec_trace_enabled = 0;

/* Coverage: a bitmap over an "effective ROM address" so banked code is
 * tracked per bank. Allocated lazily when coverage is first enabled. */
static UINT8 *coverage_bitmap = NULL;
static UINT32 coverage_bits = 0;    /* number of addressable bits */
static UINT32 coverage_count = 0;   /* distinct addresses executed */
static volatile sig_atomic_t coverage_enabled = 0;

/* Tracepoints: like breakpoints but log a register snapshot and continue. */
typedef struct {
	UINT32 addr;
	int bank;
	UINT32 hits;
} tracepoint_t;

static tracepoint_t tracepoints[MAX_POINTS];
static int tracepoint_count = 0;

typedef struct {
	UINT32 pc;
	int bank;
	UINT16 x, y, u, s;
	UINT8 a, b, dp, cc;
} tp_log_t;

#define TP_LOG_SIZE 512
static tp_log_t tp_log[TP_LOG_SIZE];
static int tp_head = 0;
static int tp_count = 0;

typedef struct {
	int type;
	int id;
	int val;
	UINT32 time;
} action_t;

#define ACTION_LOG_SIZE 512
static action_t action_log[ACTION_LOG_SIZE];
static int action_head = 0;
static int action_count = 0;

/* ------------------------------------------------------------------ */
/* Code instrumentation (PC hit counting)                             */
/* ------------------------------------------------------------------ */

typedef struct {
	UINT32 addr;
	int bank;       /* -1: any bank */
	UINT32 count;
} instrument_t;

#define INSTRUMENT_MAX 128
static instrument_t instruments[INSTRUMENT_MAX];
static int instrument_count = 0;

/* ------------------------------------------------------------------ */
/* Memory value scan                                                  */
/* ------------------------------------------------------------------ */

#define SCAN_MAX 0x2000
static int    scan_cpu = 0;
static UINT32 scan_base = 0;
static int    scan_size = 0;          /* 0 = no active scan */
static UINT8  scan_snapshot[SCAN_MAX];
static UINT8  scan_candidate[SCAN_MAX]; /* 1 = offset still matches */

/* ------------------------------------------------------------------ */
/* Lightweight game-state checkpoints (WPC RAM + main CPU registers)  */
/* ------------------------------------------------------------------ */

#define SAVESTATE_SLOTS 8
#define SAVESTATE_RAM   0x3000   /* WPC RAM size */

typedef struct {
	char   name[32];
	int    used;
	int    ramlen;
	UINT8  ram[SAVESTATE_RAM];
	UINT16 pc, s, u, x, y;
	UINT8  a, b, dp, cc;
} savestate_t;

static savestate_t savestates[SAVESTATE_SLOTS];

/* ------------------------------------------------------------------ */
/* DMD frame recorder                                                 */
/* ------------------------------------------------------------------ */

#define DMD_REC_FRAMES 512    /* ring capacity (~8s at 60Hz) */
#define DMD_REC_PIX    4096   /* max pixels per frame (128x32) */

typedef struct {
	UINT32 t;                 /* monotonic_ms() timestamp */
	UINT8  pix[DMD_REC_PIX];  /* one luminance byte per pixel */
} dmd_frame_t;

static dmd_frame_t *dmd_rec = NULL;   /* allocated lazily on first use */
static int dmd_rec_head = 0;
static int dmd_rec_count = 0;
static int dmd_rec_w = 0, dmd_rec_h = 0;
static volatile sig_atomic_t dmd_recording = 0;

/* ------------------------------------------------------------------ */
/* Callstack                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
	UINT32 caller;    /* address of the call instruction */
	UINT32 receiver;  /* call target */
	int bank;
	UINT16 pc, u, s, x, y;  /* pc is the return address */
	UINT8 a, b, dp, cc;
} callstack_entry_t;

#define CALLSTACK_SIZE 128
static callstack_entry_t callstack[CALLSTACK_SIZE];
static int callstack_ptr = 0;

/* ------------------------------------------------------------------ */
/* Messages and SSE events                                            */
/* ------------------------------------------------------------------ */

#define MSG_QUEUE_SIZE 50
#define MSG_LEN 128
static char msg_queue[MSG_QUEUE_SIZE][MSG_LEN];
static int msg_head = 0;
static int msg_count = 0;

#define EVENT_QUEUE_SIZE 32
#define EVENT_LEN 320
static char event_queue[EVENT_QUEUE_SIZE][EVENT_LEN];
static int event_head = 0;  /* next slot to write */
static int event_tail = 0;  /* next slot to read */

void remote_debug_lock(void)   { pthread_mutex_lock(&mame_mutex); }
void remote_debug_unlock(void) { pthread_mutex_unlock(&mame_mutex); }

/* Read a byte, optionally from a specific WPC ROM bank.
 *
 * The WPC banked ROM window is 0x4000-0x7FFF; a bank of -1 (or any address
 * outside that window, or a non-WPC machine) reads through the CPU's current
 * memory map. When a bank is given for an address in the window, the byte is
 * read directly from the ROM region without touching live banking, so it is
 * side-effect free. The caller must hold the debugger lock. */
UINT8 remote_debug_read_byte(int cpu, UINT32 addr, int bank)
{
	if (bank >= 0 && cpu == 0 && wpc_ram && addr >= 0x4000 && addr < 0x8000) {
		UINT8 *rom = memory_region(WPC_ROMREGION);
		if (rom) {
			UINT32 off = (UINT32)bank * 0x4000 + (addr - 0x4000);
			if (off < (UINT32)memory_region_length(WPC_ROMREGION))
				return rom[off];
		}
	}
	return cpunum_read_byte(cpu, addr);
}

/* Monotonic milliseconds since an arbitrary epoch (for timestamps/pulses). */
static UINT32 monotonic_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (UINT32)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

char *remote_debug_json_escape(char *dst, int dstlen, const char *src)
{
	int o = 0;
	dst[0] = 0;
	if (!src)
		return dst;
	while (*src && o < dstlen - 7) {
		unsigned char c = (unsigned char)*src++;
		if (c == '"' || c == '\\') {
			dst[o++] = '\\';
			dst[o++] = (char)c;
		}
		else if (c < 0x20) {
			o += snprintf(dst + o, (size_t)(dstlen - o), "\\u%04x", c);
		}
		else {
			dst[o++] = (char)c;
		}
	}
	dst[o] = 0;
	return dst;
}

/* Queue a JSON event object for the SSE clients (drops the oldest event
 * when the queue is full). Takes the lock itself. */
static void publish_event(const char *fmt, ...)
{
	va_list ap;
	remote_debug_lock();
	va_start(ap, fmt);
	vsnprintf(event_queue[event_head], EVENT_LEN, fmt, ap);
	va_end(ap);
	event_head = (event_head + 1) % EVENT_QUEUE_SIZE;
	if (event_head == event_tail)
		event_tail = (event_tail + 1) % EVENT_QUEUE_SIZE;
	remote_debug_unlock();
}

/* forward declarations; defined with the pulse implementation below */
static void service_pulses(void);
static void pulse_tick(int reassert);

int remote_debug_next_event(char *buf, int maxlen)
{
	int have = 0;
	/* The HTTP server thread calls this on every poll tick, so it is a
	 * convenient pause-independent place to expire timed switch pulses. */
	service_pulses();
	remote_debug_lock();
	if (event_tail != event_head) {
		strncpy(buf, event_queue[event_tail], (size_t)maxlen - 1);
		buf[maxlen - 1] = 0;
		event_tail = (event_tail + 1) % EVENT_QUEUE_SIZE;
		have = 1;
	}
	remote_debug_unlock();
	return have;
}

void remote_debug_add_message(const char *msg)
{
	char esc[2 * MSG_LEN];
	remote_debug_lock();
	strncpy(msg_queue[msg_head], msg, MSG_LEN - 1);
	msg_queue[msg_head][MSG_LEN - 1] = 0;
	msg_head = (msg_head + 1) % MSG_QUEUE_SIZE;
	if (msg_count < MSG_QUEUE_SIZE)
		msg_count++;
	remote_debug_unlock();
	publish_event("{\"event\": \"message\", \"text\": \"%s\"}",
	              remote_debug_json_escape(esc, (int)sizeof(esc), msg));
}

/* ------------------------------------------------------------------ */
/* Growable string buffer for the JSON dump functions                 */
/* ------------------------------------------------------------------ */

typedef struct {
	char *buf;
	int len;
	int cap;
} strbuf_t;

static void sb_init(strbuf_t *sb, int initial)
{
	sb->buf = malloc((size_t)initial);
	sb->cap = sb->buf ? initial : 0;
	sb->len = 0;
	if (sb->buf)
		sb->buf[0] = 0;
}

static void sb_appendf(strbuf_t *sb, const char *fmt, ...)
{
	va_list ap;
	int need;

	if (!sb->buf)
		return;
	va_start(ap, fmt);
	need = vsnprintf(sb->buf + sb->len, (size_t)(sb->cap - sb->len), fmt, ap);
	va_end(ap);
	if (need < 0)
		return;
	if (need >= sb->cap - sb->len) {
		int newcap = sb->cap * 2;
		char *newbuf;
		while (newcap - sb->len <= need)
			newcap *= 2;
		newbuf = realloc(sb->buf, (size_t)newcap);
		if (!newbuf)
			return;  /* keep the truncated content */
		sb->buf = newbuf;
		sb->cap = newcap;
		va_start(ap, fmt);
		need = vsnprintf(sb->buf + sb->len, (size_t)(sb->cap - sb->len), fmt, ap);
		va_end(ap);
		if (need < 0)
			return;
	}
	sb->len += need;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                          */
/* ------------------------------------------------------------------ */

void remote_debug_init(void)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mame_mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	if (pmoptions.start_paused) {
		is_paused = 1;
		printf("Remote Debugger: paused on start\n");
	}
	should_quit = 0;
	step_requested = 0;
	breakpoint_count = 0;
	watchpoint_count = 0;
	msg_head = msg_count = 0;
	event_head = event_tail = 0;
	trace_head = trace_count = 0;
	trace_addr_count = 0;
	callstack_ptr = 0;
	pulse_count = 0;
	monitor_count = 0;
	action_head = action_count = 0;
	instrument_count = 0;
	scan_size = 0;
	dmd_recording = 0;
	dmd_rec_head = dmd_rec_count = 0;
	memset(savestates, 0, sizeof(savestates));
	exec_trace_enabled = 0;
	exec_trace_head = exec_trace_count = 0;
	coverage_enabled = 0;
	coverage_count = 0;
	tracepoint_count = 0;
	tp_head = tp_count = 0;
	remote_debug_ready = 1;
	http_server_start(pmoptions.http_port, api_handler, remote_debug_next_event);
}

void remote_debug_exit(void)
{
	remote_debug_ready = 0;
	/* http_server_stop() joins the server thread, so no handler can hold
	 * the mutex once it returns and destroying the mutex is safe. */
	http_server_stop();
	if (dmd_rec) {
		free(dmd_rec);
		dmd_rec = NULL;
	}
	if (coverage_bitmap) {
		free(coverage_bitmap);
		coverage_bitmap = NULL;
	}
	pthread_mutex_destroy(&mame_mutex);
}

/* ------------------------------------------------------------------ */
/* Execution control                                                  */
/* ------------------------------------------------------------------ */

int remote_debug_is_paused(void)
{
	if (should_quit)
		return 0;
	if (step_requested) {
		step_requested = 0;
		return 0;
	}
	if (is_paused)
		usleep(10000);
	return is_paused;
}

void remote_debug_set_paused(int paused)
{
	int was = is_paused;
	is_paused = paused ? 1 : 0;
	if (was != is_paused)
		publish_event("{\"event\": \"%s\", \"reason\": \"user\"}",
		              is_paused ? "halt" : "resume");
}

void remote_debug_step(void)
{
	is_paused = 1;
	step_requested = 1;
	publish_event("{\"event\": \"step\"}");
}

void remote_debug_quit(void)
{
	printf("Remote Debugger: quit requested\n");
	should_quit = 1;
	publish_event("{\"event\": \"quit\"}");
}

int remote_debug_should_quit(void)
{
	return should_quit;
}

/* ------------------------------------------------------------------ */
/* Breakpoints                                                        */
/* ------------------------------------------------------------------ */

/* Resolve an M6809 register name for breakpoint conditions. */
static int resolve_cond_register(const char *name, int len)
{
	if (len == 1) {
		switch (name[0]) {
			case 'A': return M6809_A;
			case 'B': return M6809_B;
			case 'X': return M6809_X;
			case 'Y': return M6809_Y;
			case 'U': return M6809_U;
			case 'S': return M6809_S;
			default: break;
		}
	}
	else if (len == 2) {
		if (strncmp(name, "PC", 2) == 0) return M6809_PC;
		if (strncmp(name, "SP", 2) == 0) return M6809_S;
		if (strncmp(name, "CC", 2) == 0) return M6809_CC;
		if (strncmp(name, "DP", 2) == 0) return M6809_DP;
	}
	return -1;
}

/* Parse "REG==HEXVAL" style conditions. Returns 0 on success. */
static int parse_condition(const char *cond, int *op, int *reg, UINT32 *val)
{
	static const struct { const char *sym; int op; } ops[] = {
		{"==", COND_EQ}, {"!=", COND_NE}, {"<=", COND_LE},
		{">=", COND_GE}, {"<", COND_LT}, {">", COND_GT}
	};
	size_t i;

	*op = COND_NONE;
	if (!cond || !cond[0])
		return 0;
	for (i = 0; i < sizeof(ops) / sizeof(ops[0]); i++) {
		const char *p = strstr(cond, ops[i].sym);
		if (p && p > cond) {
			int r = resolve_cond_register(cond, (int)(p - cond));
			const char *v = p + strlen(ops[i].sym);
			char *end = NULL;
			UINT32 value;
			if (r < 0 || !*v)
				return -1;
			value = (UINT32)strtoul(v, &end, 16);
			if (end == v)
				return -1;
			*op = ops[i].op;
			*reg = r;
			*val = value;
			return 0;
		}
	}
	return -1;
}

static int cond_matches(const breakpoint_t *bp)
{
	UINT32 v;
	if (bp->cond_op == COND_NONE)
		return 1;
	v = activecpu_get_reg(bp->cond_reg);
	switch (bp->cond_op) {
		case COND_EQ: return v == bp->cond_val;
		case COND_NE: return v != bp->cond_val;
		case COND_LT: return v <  bp->cond_val;
		case COND_GT: return v >  bp->cond_val;
		case COND_LE: return v <= bp->cond_val;
		case COND_GE: return v >= bp->cond_val;
		default:      return 1;
	}
}

/* Internal add; temp breakpoints are silent. */
static int breakpoint_add_internal(UINT32 adr, int bank, int temp,
                                   const char *cond, UINT32 ignore)
{
	int result = -1;
	remote_debug_lock();
	if (breakpoint_count < MAX_POINTS) {
		breakpoint_t *bp = &breakpoints[breakpoint_count];
		memset(bp, 0, sizeof(*bp));
		bp->adr = adr;
		bp->bank = bank;
		bp->enabled = 1;
		bp->temp = temp;
		bp->ignore_count = ignore;
		if (parse_condition(cond, &bp->cond_op, &bp->cond_reg, &bp->cond_val) == 0) {
			if (bp->cond_op != COND_NONE) {
				strncpy(bp->cond_str, cond, sizeof(bp->cond_str) - 1);
				bp->cond_str[sizeof(bp->cond_str) - 1] = 0;
			}
			breakpoint_count++;
			result = 0;
			if (!temp) {
				char b[MSG_LEN];
				if (bank != -1)
					snprintf(b, sizeof(b), "BP added at %02X:%04X", bank, adr);
				else
					snprintf(b, sizeof(b), "BP added at %04X", adr);
				remote_debug_add_message(b);
			}
		}
	}
	remote_debug_unlock();
	return result;
}

int remote_debug_breakpoint_add_ex(UINT32 adr, int bank, const char *cond, UINT32 ignore)
{
	return breakpoint_add_internal(adr, bank, 0, cond, ignore);
}

void remote_debug_breakpoint_add(UINT32 adr)
{
	breakpoint_add_internal(adr, -1, 0, NULL, 0);
}

void remote_debug_breakpoint_add_banked(UINT32 adr, int bank)
{
	breakpoint_add_internal(adr, bank, 0, NULL, 0);
}

void remote_debug_breakpoint_clear(void)
{
	remote_debug_lock();
	breakpoint_count = 0;
	remote_debug_unlock();
	remote_debug_add_message("Breakpoints cleared");
}

void remote_debug_breakpoint_toggle(int index)
{
	remote_debug_lock();
	if (index >= 0 && index < breakpoint_count)
		breakpoints[index].enabled = !breakpoints[index].enabled;
	remote_debug_unlock();
}

void remote_debug_breakpoint_delete(int index)
{
	int i;
	remote_debug_lock();
	if (index >= 0 && index < breakpoint_count) {
		for (i = index; i < breakpoint_count - 1; i++)
			breakpoints[i] = breakpoints[i + 1];
		breakpoint_count--;
	}
	remote_debug_unlock();
}

/* Per-instruction hook. Runs on the emulator thread which holds the
 * debugger lock for the whole timeslice (see cpuexec.c), so plain reads
 * of the breakpoint table are safe here without locking again. */
/* Effective address for the coverage bitmap: banked-window code is indexed
 * per bank so different banks at the same CPU address are distinguished. */
static UINT32 coverage_index(UINT32 pc, int bank)
{
	if (bank >= 0 && pc >= 0x4000 && pc < 0x8000)
		return 0x10000 + (UINT32)bank * 0x4000 + (pc - 0x4000);
	return pc & 0xFFFF;
}

void remote_debug_breakpoint_hook(void)
{
	UINT32 pc;
	int current_bank, i;

	if (breakpoint_count == 0 && instrument_count == 0 && tracepoint_count == 0
			&& !exec_trace_enabled && !coverage_enabled)
		return;
	pc = activecpu_get_reg(REG_PC);
	current_bank = wpc_get_bank();

	/* execution trace: record this instruction in the ring buffer */
	if (exec_trace_enabled) {
		exec_trace_t *e = &exec_trace[exec_trace_head];
		e->pc = (UINT16)pc;
		e->bank = (INT16)current_bank;
		e->a = (UINT8)activecpu_get_reg(M6809_A);
		e->b = (UINT8)activecpu_get_reg(M6809_B);
		e->x = (UINT16)activecpu_get_reg(M6809_X);
		exec_trace_head = (exec_trace_head + 1) % EXEC_TRACE_SIZE;
		if (exec_trace_count < EXEC_TRACE_SIZE)
			exec_trace_count++;
	}

	/* coverage: mark this address as executed */
	if (coverage_enabled && coverage_bitmap) {
		UINT32 idx = coverage_index(pc, current_bank);
		if (idx < coverage_bits) {
			UINT8 mask = (UINT8)(1 << (idx & 7));
			if (!(coverage_bitmap[idx >> 3] & mask)) {
				coverage_bitmap[idx >> 3] |= mask;
				coverage_count++;
			}
		}
	}

	/* tracepoints: log a register snapshot and continue (no halt) */
	for (i = 0; i < tracepoint_count; i++) {
		if (tracepoints[i].addr == pc
				&& (tracepoints[i].bank == -1 || tracepoints[i].bank == current_bank)) {
			tp_log_t *t = &tp_log[tp_head];
			tracepoints[i].hits++;
			t->pc = pc;
			t->bank = current_bank;
			t->a = (UINT8)activecpu_get_reg(M6809_A);
			t->b = (UINT8)activecpu_get_reg(M6809_B);
			t->x = (UINT16)activecpu_get_reg(M6809_X);
			t->y = (UINT16)activecpu_get_reg(M6809_Y);
			t->u = (UINT16)activecpu_get_reg(M6809_U);
			t->s = (UINT16)activecpu_get_reg(REG_SP);
			t->dp = (UINT8)activecpu_get_reg(M6809_DP);
			t->cc = (UINT8)activecpu_get_reg(M6809_CC);
			tp_head = (tp_head + 1) % TP_LOG_SIZE;
			if (tp_count < TP_LOG_SIZE)
				tp_count++;
		}
	}

	/* code instrumentation: count passes over marked addresses */
	for (i = 0; i < instrument_count; i++) {
		if (instruments[i].addr == pc
				&& (instruments[i].bank == -1 || instruments[i].bank == current_bank))
			instruments[i].count++;
	}

	for (i = 0; i < breakpoint_count; i++) {
		breakpoint_t *bp = &breakpoints[i];
		if (!bp->enabled || pc != bp->adr)
			continue;
		if (bp->bank != -1 && current_bank != bp->bank)
			continue;
		if (!cond_matches(bp))
			continue;
		bp->hit_count++;
		if (!bp->temp && bp->hit_count <= bp->ignore_count)
			continue;

		is_paused = 1;
		activecpu_abort_timeslice();
		{
			char b[MSG_LEN];
			if (current_bank != -1)
				snprintf(b, sizeof(b), "Halt: BP at %02X:%04X", current_bank, pc);
			else
				snprintf(b, sizeof(b), "Halt: BP at %04X", pc);
			if (bp->temp) {
				int j;
				for (j = i; j < breakpoint_count - 1; j++)
					breakpoints[j] = breakpoints[j + 1];
				breakpoint_count--;
			}
			remote_debug_add_message(b);
			publish_event("{\"event\": \"halt\", \"reason\": \"bp\", \"pc\": %u, \"bank\": %d}",
			              pc, current_bank);
			printf("Remote Debugger: %s\n", b);
		}
		break;
	}
}

/* ------------------------------------------------------------------ */
/* Watchpoints and memory access trace                                */
/* ------------------------------------------------------------------ */

void remote_debug_watchpoint_add(UINT32 adr, int len, int mode, int bank,
                                 int cond_op, UINT8 cond_val)
{
	remote_debug_lock();
	if (watchpoint_count < MAX_POINTS) {
		char b[MSG_LEN];
		watchpoints[watchpoint_count].adr = adr;
		watchpoints[watchpoint_count].len = (len > 0) ? len : 1;
		watchpoints[watchpoint_count].mode = mode;
		watchpoints[watchpoint_count].bank = bank;
		watchpoints[watchpoint_count].cond_op = cond_op;
		watchpoints[watchpoint_count].cond_val = cond_val;
		watchpoints[watchpoint_count].enabled = 1;
		watchpoint_count++;
		if (bank != -1)
			snprintf(b, sizeof(b), "WP added at %02X:%04X len %d", bank, adr, (len > 0) ? len : 1);
		else
			snprintf(b, sizeof(b), "WP added at %04X len %d", adr, (len > 0) ? len : 1);
		remote_debug_unlock();
		remote_debug_add_message(b);
		return;
	}
	remote_debug_unlock();
}

void remote_debug_watchpoint_clear(void)
{
	remote_debug_lock();
	watchpoint_count = 0;
	remote_debug_unlock();
	remote_debug_add_message("Watchpoints cleared");
}

void remote_debug_watchpoint_toggle(int index)
{
	remote_debug_lock();
	if (index >= 0 && index < watchpoint_count)
		watchpoints[index].enabled = !watchpoints[index].enabled;
	remote_debug_unlock();
}

void remote_debug_watchpoint_delete(int index)
{
	int i;
	remote_debug_lock();
	if (index >= 0 && index < watchpoint_count) {
		for (i = index; i < watchpoint_count - 1; i++)
			watchpoints[i] = watchpoints[i + 1];
		watchpoint_count--;
	}
	remote_debug_unlock();
}

void remote_debug_trace_add(UINT32 adr, int bank)
{
	remote_debug_lock();
	if (trace_addr_count < TRACE_ADDR_MAX) {
		trace_addrs[trace_addr_count] = adr;
		trace_addr_banks[trace_addr_count] = bank;
		trace_addr_count++;
	}
	remote_debug_unlock();
}

void remote_debug_trace_clear(void)
{
	remote_debug_lock();
	trace_addr_count = 0;
	trace_head = 0;
	trace_count = 0;
	remote_debug_unlock();
}

/* Memory access hook; like remote_debug_breakpoint_hook() this runs with
 * the debugger lock effectively held by the emulator thread. */
void remote_debug_memref(UINT32 adr, int length, int write)
{
	int i, active_cpu, current_bank;

	if (watchpoint_count == 0 && trace_addr_count == 0)
		return;
	active_cpu = cpu_getactivecpu();
	if (active_cpu < 0)
		return;
	current_bank = wpc_get_bank();

	for (i = 0; i < watchpoint_count; i++) {
		const watchpoint_t *wp = &watchpoints[i];
		int hit;
		if (!wp->enabled)
			continue;
		/* ranges [adr, adr+length) and [wp->adr, wp->adr+wp->len) overlap? */
		if (!(adr < wp->adr + (UINT32)wp->len && wp->adr < adr + (UINT32)length))
			continue;
		/* bank filter (only meaningful for the 0x4000-0x7FFF window) */
		if (wp->bank != -1 && current_bank != wp->bank)
			continue;
		hit = (wp->mode == REMOTE_DEBUG_WP_RW)
		   || (wp->mode == REMOTE_DEBUG_WP_READ && !write)
		   || (wp->mode == REMOTE_DEBUG_WP_WRITE && write);
		/* optional value condition: only halt when the byte at the
		 * watched address satisfies the comparison */
		if (hit && wp->cond_op != COND_NONE) {
			UINT8 v = cpunum_read_byte(active_cpu, wp->adr);
			int ok;
			switch (wp->cond_op) {
				case COND_EQ: ok = (v == wp->cond_val); break;
				case COND_NE: ok = (v != wp->cond_val); break;
				case COND_LT: ok = (v <  wp->cond_val); break;
				case COND_GT: ok = (v >  wp->cond_val); break;
				case COND_LE: ok = (v <= wp->cond_val); break;
				case COND_GE: ok = (v >= wp->cond_val); break;
				default:      ok = 1;                   break;
			}
			if (!ok)
				hit = 0;
		}
		if (hit) {
			UINT32 pc = activecpu_get_reg(REG_PC);
			char b[MSG_LEN];
			is_paused = 1;
			activecpu_abort_timeslice();
			snprintf(b, sizeof(b), "Halt: WP %s at %04X (PC=%04X, Bank=%02X)",
			         write ? "write" : "read", adr, pc, current_bank);
			remote_debug_add_message(b);
			publish_event("{\"event\": \"halt\", \"reason\": \"wp\", \"addr\": %u, \"pc\": %u, \"bank\": %d}",
			              adr, pc, current_bank);
			printf("Remote Debugger: %s\n", b);
		}
	}

	for (i = 0; i < trace_addr_count; i++) {
		if (trace_addr_banks[i] != -1 && current_bank != trace_addr_banks[i])
			continue;
		if (adr < trace_addrs[i] + 1 && trace_addrs[i] < adr + (UINT32)length) {
			trace_entry_t *e = &trace_log[trace_head];
			e->adr = adr;
			e->length = length;
			e->write = write;
			e->cpunum = active_cpu;
			e->pc = activecpu_get_reg(REG_PC);
			e->bank = current_bank;
			trace_head = (trace_head + 1) % TRACE_LOG_SIZE;
			if (trace_count < TRACE_LOG_SIZE)
				trace_count++;
			break;
		}
	}
}

/* ------------------------------------------------------------------ */
/* JSON dumps                                                         */
/* ------------------------------------------------------------------ */

void remote_debug_get_points(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"breakpoints\": [");
	for (i = 0; i < breakpoint_count; i++) {
		const breakpoint_t *bp = &breakpoints[i];
		char esc[64];
		sb_appendf(&sb,
			"%s{\"idx\": %d, \"addr\": %u, \"bank\": %d, \"enabled\": %d, "
			"\"temp\": %d, \"cond\": \"%s\", \"hits\": %u, \"ignore\": %u}",
			(i > 0) ? "," : "", i, bp->adr, bp->bank, bp->enabled, bp->temp,
			remote_debug_json_escape(esc, (int)sizeof(esc), bp->cond_str),
			bp->hit_count, bp->ignore_count);
	}
	sb_appendf(&sb, "], \"watchpoints\": [");
	for (i = 0; i < watchpoint_count; i++) {
		const watchpoint_t *wp = &watchpoints[i];
		static const char *opname[] = {"", "==", "!=", "<", ">", "<=", ">="};
		char cond[16];
		if (wp->cond_op != COND_NONE)
			snprintf(cond, sizeof(cond), "%s%02X", opname[wp->cond_op], wp->cond_val);
		else
			cond[0] = 0;
		sb_appendf(&sb,
			"%s{\"idx\": %d, \"addr\": %u, \"len\": %d, \"mode\": %d, \"bank\": %d, \"cond\": \"%s\", \"enabled\": %d}",
			(i > 0) ? "," : "", i, wp->adr, wp->len, wp->mode, wp->bank, cond, wp->enabled);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_messages(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"messages\": [");
	for (i = 0; i < msg_count; i++) {
		int idx = (msg_head - msg_count + i + MSG_QUEUE_SIZE) % MSG_QUEUE_SIZE;
		char esc[2 * MSG_LEN];
		sb_appendf(&sb, "%s\"%s\"", (i > 0) ? "," : "",
		           remote_debug_json_escape(esc, (int)sizeof(esc), msg_queue[idx]));
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_callstack(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"stack\": [");
	for (i = 0; i < callstack_ptr; i++) {
		const callstack_entry_t *e = &callstack[i];
		sb_appendf(&sb,
			"%s{\"caller\": %u, \"receiver\": %u, \"bank\": %d, \"pc\": %d, "
			"\"u\": %d, \"s\": %d, \"x\": %d, \"y\": %d, \"a\": %d, \"b\": %d, "
			"\"dp\": %d, \"cc\": %d}",
			(i > 0) ? "," : "", e->caller, e->receiver, e->bank, e->pc,
			e->u, e->s, e->x, e->y, e->a, e->b, e->dp, e->cc);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_trace(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"watched\": [");
	for (i = 0; i < trace_addr_count; i++)
		sb_appendf(&sb, "%s{\"addr\": %u, \"bank\": %d}",
		           (i > 0) ? "," : "", trace_addrs[i], trace_addr_banks[i]);
	sb_appendf(&sb, "], \"logs\": [");
	for (i = 0; i < trace_count; i++) {
		int idx = (trace_head - trace_count + i + TRACE_LOG_SIZE) % TRACE_LOG_SIZE;
		const trace_entry_t *e = &trace_log[idx];
		sb_appendf(&sb,
			"%s{\"cpu\": %d, \"pc\": %u, \"adr\": %u, \"len\": %d, \"write\": %d, \"bank\": %d}",
			(i > 0) ? "," : "", e->cpunum, e->pc, e->adr, e->length, e->write, e->bank);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ------------------------------------------------------------------ */
/* Memory / register operations                                       */
/* ------------------------------------------------------------------ */

/* Write one byte. Debugger writes into the WPC RAM range go directly to
 * the RAM array so the WPC memory protection (wpc_ram_w) cannot silently
 * discard them; everything else goes through the CPU's memory map. */
static void debug_write_byte(int cpu_idx, UINT32 addr, UINT8 val)
{
	if (cpu_idx == 0 && wpc_ram && addr < 0x3000)
		wpc_ram[addr] = val;
	else
		cpunum_write_byte(cpu_idx, addr, val);
}

void remote_debug_memory_fill(int cpu_idx, UINT32 addr, int size, UINT8 val)
{
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu()) {
		char b[MSG_LEN];
		int i;
		for (i = 0; i < size; i++)
			debug_write_byte(cpu_idx, addr + (UINT32)i, val);
		snprintf(b, sizeof(b), "Memory fill: %04X-%04X with %02X",
		         addr, addr + (UINT32)size - 1, val);
		remote_debug_add_message(b);
	}
	remote_debug_unlock();
}

int remote_debug_memory_write_block(int cpu_idx, UINT32 addr, const UINT8 *data, int len)
{
	int result = -1;
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu() && len > 0) {
		int i;
		for (i = 0; i < len; i++)
			debug_write_byte(cpu_idx, addr + (UINT32)i, data[i]);
		result = 0;
	}
	remote_debug_unlock();
	return result;
}

int remote_debug_memory_find(int cpu_idx, UINT32 addr, int size,
                             const UINT8 *pattern, int pat_len, UINT32 *found_addr, int bank)
{
	int result = 0;
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu()
			&& pat_len > 0 && size > 0 && pat_len <= size) {
		UINT32 a;
		for (a = addr; a <= addr + (UINT32)(size - pat_len); a++) {
			int i, match = 1;
			for (i = 0; i < pat_len; i++) {
				if (remote_debug_read_byte(cpu_idx, a + (UINT32)i, bank) != pattern[i]) {
					match = 0;
					break;
				}
			}
			if (match) {
				*found_addr = a;
				result = 1;
				break;
			}
		}
	}
	remote_debug_unlock();
	return result;
}

void remote_debug_set_register(int cpu_idx, int reg, UINT32 val)
{
	remote_debug_lock();
	if (Machine && cpu_idx >= 0 && cpu_idx < cpu_gettotalcpu()) {
		char b[MSG_LEN];
		/* switch context so the write reaches the right CPU core */
		cpuintrf_push_context(cpu_idx);
		activecpu_set_reg(reg, val);
		cpuintrf_pop_context();
		snprintf(b, sizeof(b), "Set reg: CPU %d, reg %d to %04X", cpu_idx, reg, val);
		remote_debug_add_message(b);
	}
	remote_debug_unlock();
}

/* ------------------------------------------------------------------ */
/* Step over / step out / run to                                      */
/* ------------------------------------------------------------------ */

void remote_debug_step_over(void)
{
	remote_debug_lock();
	if (Machine && cpu_gettotalcpu() > 0) {
		/* While paused there is no active CPU context (the HTTP thread
		 * calls this between timeslices), so push the main CPU. */
		int need_ctx = (cpu_getactivecpu() < 0);
		UINT32 pc;
		char dasm[64];
		int size;
		if (need_ctx)
			cpuintrf_push_context(0);
		pc = activecpu_get_reg(REG_PC);
		activecpu_set_op_base(pc);
		size = (int)activecpu_dasm(dasm, pc);
		if (need_ctx)
			cpuintrf_pop_context();
		if (size > 0) {
			breakpoint_add_internal(pc + (UINT32)size, -1, 1, NULL, 0);
			is_paused = 0;
			remote_debug_add_message("Stepping over...");
		}
		else {
			remote_debug_step();
		}
	}
	remote_debug_unlock();
}

void remote_debug_step_out(void)
{
	remote_debug_lock();
	if (callstack_ptr > 0) {
		const callstack_entry_t *e = &callstack[callstack_ptr - 1];
		char b[MSG_LEN];
		breakpoint_add_internal(e->pc, e->bank, 1, NULL, 0);
		is_paused = 0;
		snprintf(b, sizeof(b), "Stepping out to %04X...", e->pc);
		remote_debug_add_message(b);
	}
	else {
		remote_debug_add_message("Step out: callstack is empty, stepping instead");
		remote_debug_step();
	}
	remote_debug_unlock();
}

void remote_debug_run_to(UINT32 addr, int bank)
{
	char b[MSG_LEN];
	remote_debug_lock();
	breakpoint_add_internal(addr, bank, 1, NULL, 0);
	is_paused = 0;
	remote_debug_unlock();
	if (bank != -1)
		snprintf(b, sizeof(b), "Running to %02X:%04X...", bank, addr);
	else
		snprintf(b, sizeof(b), "Running to %04X...", addr);
	remote_debug_add_message(b);
}

/* ------------------------------------------------------------------ */
/* Callstack hooks                                                    */
/* ------------------------------------------------------------------ */

void remote_debug_push_call(UINT32 caller, UINT32 receiver)
{
	if (!remote_debug_ready)
		return;
	remote_debug_lock();
	if (callstack_ptr < CALLSTACK_SIZE) {
		callstack_entry_t *e = &callstack[callstack_ptr++];
		e->caller = caller;
		e->receiver = receiver;
		e->bank = wpc_get_bank();
		e->pc = (UINT16)activecpu_get_reg(REG_PC);  /* return address */
		e->u = (UINT16)activecpu_get_reg(M6809_U);
		e->s = (UINT16)activecpu_get_reg(REG_SP);
		e->x = (UINT16)activecpu_get_reg(M6809_X);
		e->y = (UINT16)activecpu_get_reg(M6809_Y);
		e->a = (UINT8)activecpu_get_reg(M6809_A);
		e->b = (UINT8)activecpu_get_reg(M6809_B);
		e->dp = (UINT8)activecpu_get_reg(M6809_DP);
		e->cc = (UINT8)activecpu_get_reg(M6809_CC);
	}
	remote_debug_unlock();
}

void remote_debug_pop_call(void)
{
	if (!remote_debug_ready)
		return;
	remote_debug_lock();
	if (callstack_ptr > 0)
		callstack_ptr--;
	remote_debug_unlock();
}

void remote_debug_reset_callstack(void)
{
	if (!remote_debug_ready)
		return;
	remote_debug_lock();
	callstack_ptr = 0;
	remote_debug_unlock();
}

/* ------------------------------------------------------------------ */
/* Display capture                                                    */
/* ------------------------------------------------------------------ */

static struct mame_display *last_display = NULL;

void remote_debug_frame_update(struct mame_display *display)
{
	remote_debug_lock();
	last_display = display;
	/* re-assert held switch pulses (the input port clears the coin-door
	 * column each frame) and expire finished ones */
	pulse_tick(1);
	remote_debug_unlock();
	/* sample monitored objects and record DMD frames once per frame */
	remote_debug_monitor_sample();
	remote_debug_dmdrec_sample();
}

void remote_debug_get_screenshot_info(int *w, int *h)
{
	remote_debug_lock();
	if (last_display && last_display->game_bitmap) {
		*w = last_display->game_bitmap->width;
		*h = last_display->game_bitmap->height;
	}
	else {
		*w = 0;
		*h = 0;
	}
	remote_debug_unlock();
}

void remote_debug_get_raw_screenshot(char **buffer, int *len)
{
	remote_debug_lock();
	if (last_display && last_display->game_bitmap) {
		struct mame_bitmap *bm = last_display->game_bitmap;
		int width = bm->width;
		int height = bm->height;
		int x, y;
		char *p;
		*len = width * height * 3;
		*buffer = malloc((size_t)*len);
		p = *buffer;
		if (p) {
			for (y = 0; y < height; y++) {
				for (x = 0; x < width; x++) {
					UINT32 c;
					if (bm->depth == 32) {
						c = ((UINT32 **)bm->line)[y][x];
					}
					else {
						rgb_t r;
						if (bm->depth == 16)
							c = ((UINT16 **)bm->line)[y][x];
						else
							c = ((UINT8 **)bm->line)[y][x];
						r = last_display->game_palette[c];
						c = (UINT32)r;
					}
					*p++ = (char)((c >> 16) & 0xFF);
					*p++ = (char)((c >> 8) & 0xFF);
					*p++ = (char)(c & 0xFF);
				}
			}
		}
		else {
			*len = 0;
		}
	}
	else {
		*buffer = NULL;
		*len = 0;
	}
	remote_debug_unlock();
}

void remote_debug_get_screenshot(char **buffer, int *len, char *content_type)
{
	char *raw = NULL;
	int raw_len = 0;
	int w, h;
	remote_debug_lock();
	remote_debug_get_screenshot_info(&w, &h);
	remote_debug_get_raw_screenshot(&raw, &raw_len);
	remote_debug_unlock();
	if (raw) {
		char head[64];
		int hlen = snprintf(head, sizeof(head), "P6\n%d %d\n255\n", w, h);
		*buffer = malloc((size_t)(raw_len + hlen));
		if (*buffer) {
			memcpy(*buffer, head, (size_t)hlen);
			memcpy(*buffer + hlen, raw, (size_t)raw_len);
			*len = raw_len + hlen;
			strcpy(content_type, "image/x-portable-pixmap");
		}
		else {
			*len = 0;
		}
		free(raw);
	}
	else {
		*buffer = NULL;
		*len = 0;
	}
}

/* Provided by src/wpc/core.c */
extern void core_get_dmd_data(int layout_idx, float **pixels, int *width, int *height);

/* Convert the float DMD luminance plane to one byte per pixel. */
static UINT8 *dmd_to_bytes(const float *px, int count)
{
	UINT8 *out = malloc((size_t)count);
	int i;
	if (!out)
		return NULL;
	for (i = 0; i < count; i++) {
		float v = px[i];
		if (v < 0.0f)
			v = 0.0f;
		if (v > 1.0f)
			v = 1.0f;
		out[i] = (UINT8)(v * 255.0f);
	}
	return out;
}

void remote_debug_get_dmd_screenshot(char **buffer, int *len, char *content_type)
{
	float *px;
	int w, h;
	remote_debug_lock();
	core_get_dmd_data(0, &px, &w, &h);
	if (px && w > 0 && h > 0) {
		*buffer = (char *)dmd_to_bytes(px, w * h);
		*len = *buffer ? w * h : 0;
		strcpy(content_type, "application/octet-stream");
	}
	else {
		*buffer = NULL;
		*len = 0;
	}
	remote_debug_unlock();
}

void remote_debug_get_dmd_pnm(char **buffer, int *len, char *content_type)
{
	float *px;
	int w, h;
	remote_debug_lock();
	core_get_dmd_data(0, &px, &w, &h);
	if (px && w > 0 && h > 0) {
		char head[64];
		int hlen = snprintf(head, sizeof(head), "P5\n%d %d\n255\n", w, h);
		UINT8 *pixels = dmd_to_bytes(px, w * h);
		if (pixels) {
			*buffer = malloc((size_t)(w * h + hlen));
			if (*buffer) {
				memcpy(*buffer, head, (size_t)hlen);
				memcpy(*buffer + hlen, pixels, (size_t)(w * h));
				*len = w * h + hlen;
				strcpy(content_type, "image/x-portable-pixmap");
			}
			else {
				*len = 0;
			}
			free(pixels);
		}
		else {
			*buffer = NULL;
			*len = 0;
		}
	}
	else {
		*buffer = NULL;
		*len = 0;
	}
	remote_debug_unlock();
}

/* ================================================================== */
/* Playfield objects: switches, lamps, solenoids                      */
/* ================================================================== */

/* Read the on/off state of a switch/lamp/solenoid by number.
 * The caller must hold the debugger lock. */
static int object_state(int type, int id)
{
	if (!Machine)
		return 0;
	if (type == REMOTE_DEBUG_OBJ_SWITCH)
		return core_getSw(id) ? 1 : 0;
	if (type == REMOTE_DEBUG_OBJ_LAMP) {
		int col = id / 10, row = id % 10 - 1;
		if (col < 0 || col >= CORE_MAXLAMPCOL || row < 0 || row > 7)
			return 0;
		return (coreGlobals.lampMatrix[col] >> row) & 1;
	}
	if (type == REMOTE_DEBUG_OBJ_SOL)
		return core_getSol(id) ? 1 : 0;
	return 0;
}

/* Standard WPC name for a switch number, or "" if none is known.
 * Covers the coin-door column and the flipper column plus the game's
 * dedicated start/tilt/slam/coin-door/shooter switches. */
static const char *switch_name(int num)
{
	static const char *coindoor[8] = {
		"Coin 1", "Coin 2", "Coin 3", "Coin 4",
		"Enter", "Up", "Down", "Escape"
	};
	static const char *flippers[8] = {
		"L.R Flipper EOS", "L.R Flipper", "L.L Flipper EOS", "L.L Flipper",
		"U.R Flipper EOS", "U.R Flipper", "U.L Flipper EOS", "U.L Flipper"
	};
	if (num >= 1 && num <= 8)
		return coindoor[num - 1];
	if (num >= CORE_FLIPPERSWCOL * 10 + 1 && num <= CORE_FLIPPERSWCOL * 10 + 8)
		return flippers[num - (CORE_FLIPPERSWCOL * 10 + 1)];
	if (core_gameData) {
		if (num == core_gameData->wpc.comSw.start)    return "Start";
		if (num == core_gameData->wpc.comSw.tilt)     return "Tilt";
		if (num == core_gameData->wpc.comSw.sTilt)    return "Slam Tilt";
		if (num == core_gameData->wpc.comSw.coinDoor) return "Coin Door";
		if (num == core_gameData->wpc.comSw.shooter)  return "Shooter";
	}
	return "";
}

/* Emit switches of one column as JSON objects; returns updated first flag. */
static int emit_switch_col(strbuf_t *sb, int col, int first)
{
	int row;
	for (row = 0; row < 8; row++) {
		int num = col * 10 + row + 1;
		char esc[64];
		sb_appendf(sb,
			"%s{\"num\": %d, \"col\": %d, \"row\": %d, \"active\": %d, \"name\": \"%s\"}",
			first ? "" : ",", num, col, row + 1, object_state(REMOTE_DEBUG_OBJ_SWITCH, num),
			remote_debug_json_escape(esc, (int)sizeof(esc), switch_name(num)));
		first = 0;
	}
	return first;
}

void remote_debug_get_switches(char **buffer, int *len)
{
	strbuf_t sb;
	int first = 1, col, custom;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"switches\": [");
	if (Machine) {
		/* coin door (col 0), playfield matrix (col 1-8), flippers (col 11) */
		first = emit_switch_col(&sb, 0, first);
		for (col = 1; col <= 8; col++)
			first = emit_switch_col(&sb, col, first);
		first = emit_switch_col(&sb, CORE_FLIPPERSWCOL, first);
		/* game-specific custom columns */
		custom = core_gameData ? core_gameData->hw.swCol : 0;
		for (col = 0; col < custom; col++)
			first = emit_switch_col(&sb, CORE_CUSTSWCOL + col, first);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_lamps(char **buffer, int *len)
{
	strbuf_t sb;
	int first = 1, col, row, cols;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"lamps\": [");
	if (Machine) {
		cols = 8 + (core_gameData ? core_gameData->hw.lampCol : 0);
		if (cols > CORE_MAXLAMPCOL)
			cols = CORE_MAXLAMPCOL;
		for (col = 0; col < cols; col++) {
			for (row = 0; row < 8; row++) {
				int num = col * 10 + row + 1;
				sb_appendf(&sb,
					"%s{\"num\": %d, \"col\": %d, \"row\": %d, \"active\": %d}",
					first ? "" : ",", num, col, row + 1,
					(coreGlobals.lampMatrix[col] >> row) & 1);
				first = 0;
			}
		}
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_solenoids(char **buffer, int *len)
{
	strbuf_t sb;
	int first = 1, n, count;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"solenoids\": [");
	if (Machine) {
		count = CORE_FIRSTCUSTSOL - 1 + (core_gameData ? core_gameData->hw.custSol : 0);
		if (count < 1 || count > CORE_MAXSOL)
			count = CORE_FIRSTCUSTSOL - 1;
		for (n = 1; n <= count; n++) {
			sb_appendf(&sb, "%s{\"num\": %d, \"active\": %d}",
			           first ? "" : ",", n, core_getSol(n) ? 1 : 0);
			first = 0;
		}
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Timed switch pulses                                                */
/* ================================================================== */

/* Advance timed pulses. reassert!=0 also re-applies the held value (needed
 * on the emulator thread because the coin-door column is overwritten by the
 * input port every frame). Expired pulses restore their off value.
 * The caller must hold the debugger lock. */
static void pulse_tick(int reassert)
{
	UINT32 now = monotonic_ms();
	int i;
	if (!Machine)
		return;
	for (i = 0; i < pulse_count; ) {
		/* signed diff handles UINT32 wraparound */
		if ((INT32)(now - pulses[i].expiry) >= 0) {
			core_setSw(pulses[i].sw, pulses[i].offval);
			pulses[i] = pulses[pulse_count - 1];
			pulse_count--;
		}
		else {
			if (reassert)
				core_setSw(pulses[i].sw, pulses[i].heldval);
			i++;
		}
	}
}

/* Expire pulses from the HTTP thread so they end even while paused (when
 * no frames are produced and the emulator-side re-assert does not run). */
static void service_pulses(void)
{
	if (pulse_count == 0)
		return;
	remote_debug_lock();
	pulse_tick(0);
	remote_debug_unlock();
}

int remote_debug_set_switch(int sw, int val, int pulse_ms)
{
	int result = -1;
	remote_debug_lock();
	if (coreData) {
		int on = val ? 1 : 0;
		core_setSw(sw, on);
		if (pulse_ms > 0 && pulse_count < PULSE_MAX) {
			pulses[pulse_count].sw = sw;
			pulses[pulse_count].heldval = on;
			pulses[pulse_count].offval = on ? 0 : 1;
			pulses[pulse_count].expiry = monotonic_ms() + (UINT32)pulse_ms;
			pulse_count++;
		}
		result = 0;
	}
	remote_debug_unlock();
	return result;
}

/* ================================================================== */
/* Object monitoring / action log                                     */
/* ================================================================== */

static const char *obj_type_name(int type)
{
	switch (type) {
		case REMOTE_DEBUG_OBJ_SWITCH: return "switch";
		case REMOTE_DEBUG_OBJ_LAMP:   return "lamp";
		case REMOTE_DEBUG_OBJ_SOL:    return "sol";
		default:                      return "?";
	}
}

int remote_debug_monitor_add(int obj_type, int id, int brk)
{
	int result = -1;
	remote_debug_lock();
	if (monitor_count < MONITOR_MAX) {
		monitors[monitor_count].type = obj_type;
		monitors[monitor_count].id = id;
		monitors[monitor_count].last = object_state(obj_type, id);
		monitors[monitor_count].brk = brk;
		monitor_count++;
		result = 0;
	}
	remote_debug_unlock();
	return result;
}

void remote_debug_monitor_clear(void)
{
	remote_debug_lock();
	monitor_count = 0;
	remote_debug_unlock();
}

void remote_debug_action_log_clear(void)
{
	remote_debug_lock();
	action_head = 0;
	action_count = 0;
	remote_debug_unlock();
}

void remote_debug_monitor_sample(void)
{
	int i;
	if (monitor_count == 0)
		return;
	remote_debug_lock();
	for (i = 0; i < monitor_count; i++) {
		int cur = object_state(monitors[i].type, monitors[i].id);
		if (cur != monitors[i].last) {
			action_t *a = &action_log[action_head];
			a->type = monitors[i].type;
			a->id = monitors[i].id;
			a->val = cur;
			a->time = monotonic_ms();
			action_head = (action_head + 1) % ACTION_LOG_SIZE;
			if (action_count < ACTION_LOG_SIZE)
				action_count++;
			monitors[i].last = cur;
			publish_event("{\"event\": \"action\", \"type\": \"%s\", \"id\": %d, \"val\": %d}",
			              obj_type_name(monitors[i].type), monitors[i].id, cur);
			if (monitors[i].brk) {
				is_paused = 1;
				publish_event("{\"event\": \"halt\", \"reason\": \"monitor\", \"type\": \"%s\", \"id\": %d, \"val\": %d}",
				              obj_type_name(monitors[i].type), monitors[i].id, cur);
			}
		}
	}
	remote_debug_unlock();
}

void remote_debug_get_monitors(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 2048);
	remote_debug_lock();
	sb_appendf(&sb, "{\"monitors\": [");
	for (i = 0; i < monitor_count; i++)
		sb_appendf(&sb, "%s{\"type\": \"%s\", \"id\": %d, \"state\": %d, \"break\": %d}",
		           (i > 0) ? "," : "", obj_type_name(monitors[i].type),
		           monitors[i].id, monitors[i].last, monitors[i].brk);
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

void remote_debug_get_action_log(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 8192);
	remote_debug_lock();
	sb_appendf(&sb, "{\"actions\": [");
	for (i = 0; i < action_count; i++) {
		int idx = (action_head - action_count + i + ACTION_LOG_SIZE) % ACTION_LOG_SIZE;
		const action_t *a = &action_log[idx];
		sb_appendf(&sb, "%s{\"type\": \"%s\", \"id\": %d, \"val\": %d, \"t\": %u}",
		           (i > 0) ? "," : "", obj_type_name(a->type), a->id, a->val, a->time);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Code instrumentation (PC hit counting)                             */
/* ================================================================== */

int remote_debug_instrument_add(UINT32 addr, int bank)
{
	int result = -1;
	remote_debug_lock();
	if (instrument_count < INSTRUMENT_MAX) {
		instruments[instrument_count].addr = addr;
		instruments[instrument_count].bank = bank;
		instruments[instrument_count].count = 0;
		instrument_count++;
		result = 0;
	}
	remote_debug_unlock();
	return result;
}

void remote_debug_instrument_clear(void)
{
	remote_debug_lock();
	instrument_count = 0;
	remote_debug_unlock();
}

void remote_debug_get_instrumentation(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"points\": [");
	for (i = 0; i < instrument_count; i++)
		sb_appendf(&sb, "%s{\"addr\": %u, \"bank\": %d, \"count\": %u}",
		           (i > 0) ? "," : "", instruments[i].addr,
		           instruments[i].bank, instruments[i].count);
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Memory value scan                                                  */
/* ================================================================== */

int remote_debug_scan_new(int cpu, UINT32 addr, int size)
{
	int i;
	if (size < 1 || size > SCAN_MAX)
		return -1;
	remote_debug_lock();
	if (!Machine || cpu < 0 || cpu >= cpu_gettotalcpu()) {
		remote_debug_unlock();
		return -1;
	}
	scan_cpu = cpu;
	scan_base = addr;
	scan_size = size;
	for (i = 0; i < size; i++) {
		scan_snapshot[i] = cpunum_read_byte(cpu, addr + (UINT32)i);
		scan_candidate[i] = 1;
	}
	remote_debug_unlock();
	return size;
}

int remote_debug_scan_filter(int op, UINT8 val)
{
	int i, remaining = 0;
	remote_debug_lock();
	if (scan_size == 0 || !Machine) {
		remote_debug_unlock();
		return -1;
	}
	for (i = 0; i < scan_size; i++) {
		UINT8 cur, old;
		int keep;
		if (!scan_candidate[i])
			continue;
		cur = cpunum_read_byte(scan_cpu, scan_base + (UINT32)i);
		old = scan_snapshot[i];
		switch (op) {
			case REMOTE_DEBUG_SCAN_EQ:        keep = (cur == val); break;
			case REMOTE_DEBUG_SCAN_NE:        keep = (cur != val); break;
			case REMOTE_DEBUG_SCAN_CHANGED:   keep = (cur != old); break;
			case REMOTE_DEBUG_SCAN_UNCHANGED: keep = (cur == old); break;
			case REMOTE_DEBUG_SCAN_INC:       keep = (cur > old);  break;
			case REMOTE_DEBUG_SCAN_DEC:       keep = (cur < old);  break;
			default:                          keep = 1;            break;
		}
		scan_candidate[i] = (UINT8)keep;
		scan_snapshot[i] = cur;
		if (keep)
			remaining++;
	}
	remote_debug_unlock();
	return remaining;
}

void remote_debug_get_scan(char **buffer, int *len)
{
	strbuf_t sb;
	int i, emitted = 0, total = 0;
	sb_init(&sb, 8192);
	remote_debug_lock();
	for (i = 0; i < scan_size; i++)
		if (scan_candidate[i])
			total++;
	sb_appendf(&sb, "{\"count\": %d, \"results\": [", total);
	for (i = 0; i < scan_size && emitted < 256; i++) {
		if (!scan_candidate[i])
			continue;
		sb_appendf(&sb, "%s{\"addr\": %u, \"val\": %u}",
		           (emitted > 0) ? "," : "", scan_base + (UINT32)i,
		           Machine ? (unsigned)cpunum_read_byte(scan_cpu, scan_base + (UINT32)i) : 0);
		emitted++;
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Lightweight game-state checkpoints                                 */
/* ================================================================== */

/* Find an existing slot by name, or -1. Caller holds the lock. */
static int savestate_find(const char *name)
{
	int i;
	for (i = 0; i < SAVESTATE_SLOTS; i++)
		if (savestates[i].used && strcmp(savestates[i].name, name) == 0)
			return i;
	return -1;
}

int remote_debug_savestate_save(const char *slot)
{
	int idx, i;
	savestate_t *s;
	if (!slot || !slot[0])
		return -1;
	remote_debug_lock();
	if (!Machine || !wpc_ram) {
		remote_debug_unlock();
		return -1;
	}
	idx = savestate_find(slot);
	if (idx < 0) {
		for (i = 0; i < SAVESTATE_SLOTS; i++)
			if (!savestates[i].used) { idx = i; break; }
	}
	if (idx < 0) {
		remote_debug_unlock();
		return -1;   /* all slots occupied */
	}
	s = &savestates[idx];
	memset(s, 0, sizeof(*s));
	strncpy(s->name, slot, sizeof(s->name) - 1);
	s->used = 1;
	s->ramlen = SAVESTATE_RAM;
	memcpy(s->ram, wpc_ram, SAVESTATE_RAM);
	cpuintrf_push_context(0);
	s->pc = (UINT16)activecpu_get_reg(M6809_PC);
	s->s  = (UINT16)activecpu_get_reg(M6809_S);
	s->u  = (UINT16)activecpu_get_reg(M6809_U);
	s->x  = (UINT16)activecpu_get_reg(M6809_X);
	s->y  = (UINT16)activecpu_get_reg(M6809_Y);
	s->a  = (UINT8)activecpu_get_reg(M6809_A);
	s->b  = (UINT8)activecpu_get_reg(M6809_B);
	s->dp = (UINT8)activecpu_get_reg(M6809_DP);
	s->cc = (UINT8)activecpu_get_reg(M6809_CC);
	cpuintrf_pop_context();
	remote_debug_unlock();
	{
		char b[MSG_LEN];
		snprintf(b, sizeof(b), "State saved to slot '%s'", slot);
		remote_debug_add_message(b);
	}
	return 0;
}

int remote_debug_savestate_load(const char *slot)
{
	int idx, i;
	const savestate_t *s;
	if (!slot || !slot[0])
		return -1;
	remote_debug_lock();
	if (!Machine || !wpc_ram) {
		remote_debug_unlock();
		return -1;
	}
	idx = savestate_find(slot);
	if (idx < 0) {
		remote_debug_unlock();
		return -1;
	}
	s = &savestates[idx];
	for (i = 0; i < s->ramlen; i++)
		wpc_ram[i] = s->ram[i];
	cpuintrf_push_context(0);
	activecpu_set_reg(M6809_PC, s->pc);
	activecpu_set_reg(M6809_S,  s->s);
	activecpu_set_reg(M6809_U,  s->u);
	activecpu_set_reg(M6809_X,  s->x);
	activecpu_set_reg(M6809_Y,  s->y);
	activecpu_set_reg(M6809_A,  s->a);
	activecpu_set_reg(M6809_B,  s->b);
	activecpu_set_reg(M6809_DP, s->dp);
	activecpu_set_reg(M6809_CC, s->cc);
	cpuintrf_pop_context();
	remote_debug_unlock();
	{
		char b[MSG_LEN];
		snprintf(b, sizeof(b), "State loaded from slot '%s'", slot);
		remote_debug_add_message(b);
	}
	return 0;
}

void remote_debug_savestate_delete(const char *slot)
{
	int idx;
	remote_debug_lock();
	idx = savestate_find(slot);
	if (idx >= 0)
		savestates[idx].used = 0;
	remote_debug_unlock();
}

void remote_debug_get_savestates(char **buffer, int *len)
{
	strbuf_t sb;
	int i, first = 1;
	sb_init(&sb, 1024);
	remote_debug_lock();
	sb_appendf(&sb, "{\"slots\": [");
	for (i = 0; i < SAVESTATE_SLOTS; i++) {
		if (savestates[i].used) {
			char esc[64];
			sb_appendf(&sb, "%s{\"name\": \"%s\", \"pc\": %u}", first ? "" : ",",
			           remote_debug_json_escape(esc, (int)sizeof(esc), savestates[i].name),
			           savestates[i].pc);
			first = 0;
		}
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* DMD frame recorder                                                 */
/* ================================================================== */

void remote_debug_dmdrec_start(void)
{
	remote_debug_lock();
	if (!dmd_rec)
		dmd_rec = malloc(sizeof(dmd_frame_t) * DMD_REC_FRAMES);
	if (dmd_rec)
		dmd_recording = 1;
	remote_debug_unlock();
}

void remote_debug_dmdrec_stop(void)
{
	dmd_recording = 0;
}

void remote_debug_dmdrec_clear(void)
{
	remote_debug_lock();
	dmd_rec_head = 0;
	dmd_rec_count = 0;
	remote_debug_unlock();
}

void remote_debug_dmdrec_sample(void)
{
	float *px;
	int w, h, n, i;
	if (!dmd_recording || !dmd_rec)
		return;
	remote_debug_lock();
	core_get_dmd_data(0, &px, &w, &h);
	n = w * h;
	if (px && n > 0 && n <= DMD_REC_PIX) {
		dmd_frame_t *f = &dmd_rec[dmd_rec_head];
		dmd_rec_w = w;
		dmd_rec_h = h;
		for (i = 0; i < n; i++) {
			float v = px[i];
			if (v < 0.0f) v = 0.0f;
			if (v > 1.0f) v = 1.0f;
			f->pix[i] = (UINT8)(v * 255.0f);
		}
		f->t = monotonic_ms();
		dmd_rec_head = (dmd_rec_head + 1) % DMD_REC_FRAMES;
		if (dmd_rec_count < DMD_REC_FRAMES)
			dmd_rec_count++;
	}
	remote_debug_unlock();
}

void remote_debug_get_dmdrec_info(char **buffer, int *len)
{
	char res[192];
	remote_debug_lock();
	*len = snprintf(res, sizeof(res),
		"{\"recording\": %d, \"count\": %d, \"width\": %d, \"height\": %d, \"capacity\": %d}",
		(int)dmd_recording, dmd_rec_count, dmd_rec_w, dmd_rec_h, DMD_REC_FRAMES);
	remote_debug_unlock();
	*buffer = strdup(res);
	if (!*buffer)
		*len = 0;
}

/* Append a little-endian UINT32 to a byte buffer. */
static void put_u32le(UINT8 *p, UINT32 v)
{
	p[0] = (UINT8)(v & 0xFF);
	p[1] = (UINT8)((v >> 8) & 0xFF);
	p[2] = (UINT8)((v >> 16) & 0xFF);
	p[3] = (UINT8)((v >> 24) & 0xFF);
}

void remote_debug_get_dmdrec_data(char **buffer, int *len)
{
	int frame_bytes, total, i;
	UINT8 *out, *p;
	remote_debug_lock();
	frame_bytes = dmd_rec_w * dmd_rec_h;
	if (!dmd_rec || dmd_rec_count == 0 || frame_bytes <= 0) {
		remote_debug_unlock();
		*buffer = NULL;
		*len = 0;
		return;
	}
	/* header: count,w,h (12 bytes) + per frame: t (4) + pixels */
	total = 12 + dmd_rec_count * (4 + frame_bytes);
	out = malloc((size_t)total);
	if (!out) {
		remote_debug_unlock();
		*buffer = NULL;
		*len = 0;
		return;
	}
	p = out;
	put_u32le(p, (UINT32)dmd_rec_count); p += 4;
	put_u32le(p, (UINT32)dmd_rec_w);     p += 4;
	put_u32le(p, (UINT32)dmd_rec_h);     p += 4;
	for (i = 0; i < dmd_rec_count; i++) {
		int idx = (dmd_rec_head - dmd_rec_count + i + DMD_REC_FRAMES) % DMD_REC_FRAMES;
		const dmd_frame_t *f = &dmd_rec[idx];
		put_u32le(p, f->t); p += 4;
		memcpy(p, f->pix, (size_t)frame_bytes);
		p += frame_bytes;
	}
	remote_debug_unlock();
	*buffer = (char *)out;
	*len = total;
}

/* ================================================================== */
/* Execution trace                                                    */
/* ================================================================== */

void remote_debug_exectrace_start(void)  { exec_trace_enabled = 1; }
void remote_debug_exectrace_stop(void)   { exec_trace_enabled = 0; }

void remote_debug_exectrace_clear(void)
{
	remote_debug_lock();
	exec_trace_head = 0;
	exec_trace_count = 0;
	remote_debug_unlock();
}

void remote_debug_get_exectrace(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 65536);
	remote_debug_lock();
	sb_appendf(&sb, "{\"enabled\": %d, \"count\": %d, \"capacity\": %d, \"trace\": [",
	           (int)exec_trace_enabled, exec_trace_count, EXEC_TRACE_SIZE);
	for (i = 0; i < exec_trace_count; i++) {
		int idx = (exec_trace_head - exec_trace_count + i + EXEC_TRACE_SIZE) % EXEC_TRACE_SIZE;
		const exec_trace_t *e = &exec_trace[idx];
		sb_appendf(&sb, "%s{\"pc\": %u, \"bank\": %d, \"a\": %u, \"b\": %u, \"x\": %u}",
		           (i > 0) ? "," : "", e->pc, e->bank, e->a, e->b, e->x);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Coverage map                                                       */
/* ================================================================== */

void remote_debug_coverage_start(void)
{
	remote_debug_lock();
	if (!coverage_bitmap) {
		UINT32 nbanks = 0;
		if (wpc_ram) {
			int romlen = memory_region_length(WPC_ROMREGION);
			if (romlen > 0)
				nbanks = (UINT32)romlen / 0x4000;
		}
		coverage_bits = 0x10000 + nbanks * 0x4000;
		coverage_bitmap = calloc((coverage_bits + 7) / 8, 1);
		if (!coverage_bitmap)
			coverage_bits = 0;
		coverage_count = 0;
	}
	if (coverage_bitmap)
		coverage_enabled = 1;
	remote_debug_unlock();
}

void remote_debug_coverage_stop(void) { coverage_enabled = 0; }

void remote_debug_coverage_clear(void)
{
	remote_debug_lock();
	if (coverage_bitmap)
		memset(coverage_bitmap, 0, (coverage_bits + 7) / 8);
	coverage_count = 0;
	remote_debug_unlock();
}

void remote_debug_get_coverage_info(char **buffer, int *len)
{
	char res[192];
	remote_debug_lock();
	*len = snprintf(res, sizeof(res),
		"{\"enabled\": %d, \"executed\": %u, \"addressable\": %u}",
		(int)coverage_enabled, coverage_count, coverage_bits);
	remote_debug_unlock();
	*buffer = strdup(res);
	if (!*buffer)
		*len = 0;
}

/* Per-address executed flags for a window (like a memory read of 0/1). */
void remote_debug_get_coverage_region(char **buffer, int *len, UINT32 addr, int size, int bank)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, (size_t)size * 2 + 256);
	remote_debug_lock();
	sb_appendf(&sb, "{\"addr\": %u, \"bank\": %d, \"executed\": [", addr, bank);
	for (i = 0; i < size; i++) {
		int hit = 0;
		if (coverage_bitmap) {
			UINT32 idx = coverage_index(addr + (UINT32)i, bank);
			if (idx < coverage_bits)
				hit = (coverage_bitmap[idx >> 3] >> (idx & 7)) & 1;
		}
		sb_appendf(&sb, "%s%d", (i > 0) ? "," : "", hit);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Tracepoints (log-and-continue)                                     */
/* ================================================================== */

int remote_debug_tracepoint_add(UINT32 addr, int bank)
{
	int result = -1;
	remote_debug_lock();
	if (tracepoint_count < MAX_POINTS) {
		tracepoints[tracepoint_count].addr = addr;
		tracepoints[tracepoint_count].bank = bank;
		tracepoints[tracepoint_count].hits = 0;
		tracepoint_count++;
		result = 0;
	}
	remote_debug_unlock();
	return result;
}

void remote_debug_tracepoint_clear(void)
{
	remote_debug_lock();
	tracepoint_count = 0;
	tp_head = 0;
	tp_count = 0;
	remote_debug_unlock();
}

void remote_debug_get_tracepoints(char **buffer, int *len)
{
	strbuf_t sb;
	int i;
	sb_init(&sb, 4096);
	remote_debug_lock();
	sb_appendf(&sb, "{\"points\": [");
	for (i = 0; i < tracepoint_count; i++)
		sb_appendf(&sb, "%s{\"addr\": %u, \"bank\": %d, \"hits\": %u}",
		           (i > 0) ? "," : "", tracepoints[i].addr,
		           tracepoints[i].bank, tracepoints[i].hits);
	sb_appendf(&sb, "], \"log\": [");
	for (i = 0; i < tp_count; i++) {
		int idx = (tp_head - tp_count + i + TP_LOG_SIZE) % TP_LOG_SIZE;
		const tp_log_t *t = &tp_log[idx];
		sb_appendf(&sb,
			"%s{\"pc\": %u, \"bank\": %d, \"a\": %u, \"b\": %u, \"x\": %u, "
			"\"y\": %u, \"u\": %u, \"s\": %u, \"dp\": %u, \"cc\": %u}",
			(i > 0) ? "," : "", t->pc, t->bank, t->a, t->b, t->x,
			t->y, t->u, t->s, t->dp, t->cc);
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

/* ================================================================== */
/* Save-state diff                                                    */
/* ================================================================== */

/* Diff the RAM of two save slots (or slot 'a' against the live RAM when
 * 'b' is NULL/empty). Returns the differing offsets as JSON. */
void remote_debug_savestate_diff(char **buffer, int *len, const char *a, const char *b)
{
	strbuf_t sb;
	int ia, ib, i, first = 1, count = 0;
	const UINT8 *ba = NULL, *bb = NULL;
	UINT8 live[SAVESTATE_RAM];

	sb_init(&sb, 8192);
	remote_debug_lock();
	ia = savestate_find(a);
	if (ia >= 0)
		ba = savestates[ia].ram;
	if (b && b[0]) {
		ib = savestate_find(b);
		if (ib >= 0)
			bb = savestates[ib].ram;
	}
	else if (Machine && wpc_ram) {
		memcpy(live, wpc_ram, SAVESTATE_RAM);
		bb = live;
	}
	if (ba && bb) {
		for (i = 0; i < SAVESTATE_RAM; i++)
			if (ba[i] != bb[i])
				count++;
	}
	sb_appendf(&sb, "{\"a\": \"%s\", \"b\": \"%s\", \"count\": %d, \"diffs\": [",
	           ba ? a : "", (b && b[0]) ? b : "(live)", ba && bb ? count : -1);
	if (ba && bb) {
		int emitted = 0;
		for (i = 0; i < SAVESTATE_RAM && emitted < 1024; i++) {
			if (ba[i] != bb[i]) {
				sb_appendf(&sb, "%s{\"addr\": %d, \"a\": %u, \"b\": %u}",
				           first ? "" : ",", i, ba[i], bb[i]);
				first = 0;
				emitted++;
			}
		}
	}
	sb_appendf(&sb, "]}");
	remote_debug_unlock();
	*buffer = sb.buf;
	*len = sb.len;
}

#endif /* REMOTE_DEBUG */
