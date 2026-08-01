// license:BSD-3-Clause

/***************************************************************************
 * PinMAME Remote Debugger - core                                          *
 *                                                                         *
 * Debugger state (breakpoints, watchpoints, callstack, trace, messages)   *
 * and the glue between the emulator thread and the HTTP server thread.    *
 *                                                                         *
 * Threading model: the emulator thread holds the (recursive) debugger     *
 * lock for the whole duration of a CPU timeslice (see cpuexec.c). The     *
 * HTTP server thread takes the same lock around every access to emulator  *
 * or debugger state, so all per-instruction hooks below run with the      *
 * lock effectively held and do not lock themselves. Simple control flags  *
 * (pause/step/quit) are volatile sig_atomic_t and are accessed lock-free. *
 ***************************************************************************/
#ifndef REMOTE_DEBUG_H
#define REMOTE_DEBUG_H

#include "driver.h"

/* ------------------------------------------------------------------ */
/* Lifecycle (called from mame.c)                                     */
/* ------------------------------------------------------------------ */

/* Initialize debugger state and start the HTTP server thread. */
void remote_debug_init(void);

/* Stop the HTTP server thread and release resources. */
void remote_debug_exit(void);

/* Set while the debugger is initialized; checked by the callstack hooks
 * which may fire before init / after exit. */
extern volatile int remote_debug_ready;

/* ------------------------------------------------------------------ */
/* Locking                                                            */
/* ------------------------------------------------------------------ */

/* Recursive lock serializing emulator thread and HTTP server thread. */
void remote_debug_lock(void);
void remote_debug_unlock(void);

/* ------------------------------------------------------------------ */
/* Execution control (hooks called from the emulator)                 */
/* ------------------------------------------------------------------ */

/* Nonzero while execution is paused; sleeps briefly when paused so the
 * polling loop in cpuexec.c does not busy-wait. */
int remote_debug_is_paused(void);

/* Nonzero when the emulator should terminate. */
int remote_debug_should_quit(void);

/* Pause (1) or resume (0) execution. */
void remote_debug_set_paused(int paused);

/* Execute a single instruction, then pause again. */
void remote_debug_step(void);

/* Step over the instruction at PC (temporary breakpoint after it). */
void remote_debug_step_over(void);

/* Run until the current subroutine returns (temporary breakpoint at the
 * return address recorded on the callstack). */
void remote_debug_step_out(void);

/* Resume until addr is reached (temporary breakpoint). bank -1 matches any
 * bank; a specific bank only matters for the 0x4000-0x7FFF ROM window. */
void remote_debug_run_to(UINT32 addr, int bank);

/* Request emulator shutdown. */
void remote_debug_quit(void);

/* Per-instruction hook, called via CALL_MAME_DEBUG (see mamedbg.h);
 * checks breakpoints and halts execution on a hit. */
void remote_debug_breakpoint_hook(void);

/* Memory access hook, called from the READ/WRITE macros in memory.c;
 * checks watchpoints and records trace log entries. */
void remote_debug_memref(UINT32 adr, int length, int write);

/* ------------------------------------------------------------------ */
/* Breakpoints / watchpoints                                          */
/* ------------------------------------------------------------------ */

/* Add a breakpoint. bank -1 matches any bank. cond is an optional
 * condition of the form "REG==VAL", "REG!=VAL", "REG<VAL", "REG>VAL",
 * "REG<=VAL", "REG>=VAL" with an M6809 register name and a hex value
 * (NULL or "" for unconditional). ignore skips that many hits before
 * halting. Returns 0 on success, -1 on a bad condition or full table. */
int remote_debug_breakpoint_add_ex(UINT32 adr, int bank, const char *cond, UINT32 ignore);

void remote_debug_breakpoint_add(UINT32 adr);
void remote_debug_breakpoint_add_banked(UINT32 adr, int bank);
void remote_debug_breakpoint_clear(void);
void remote_debug_breakpoint_toggle(int index);
void remote_debug_breakpoint_delete(int index);

/* Watchpoint modes. */
#define REMOTE_DEBUG_WP_READ  1
#define REMOTE_DEBUG_WP_WRITE 2
#define REMOTE_DEBUG_WP_RW    3

/* Add a watchpoint covering len bytes starting at adr (len >= 1). bank -1
 * matches any bank; a specific bank only matters for 0x4000-0x7FFF.
 * cond_op is COND_NONE for an unconditional watchpoint, or one of the
 * REMOTE_DEBUG_COND_* codes to only halt when the byte at adr satisfies the
 * comparison against cond_val. */
void remote_debug_watchpoint_add(UINT32 adr, int len, int mode, int bank,
                                 int cond_op, UINT8 cond_val);

/* Value-condition operators (shared with breakpoints). */
#define REMOTE_DEBUG_COND_NONE 0
#define REMOTE_DEBUG_COND_EQ   1
#define REMOTE_DEBUG_COND_NE   2
#define REMOTE_DEBUG_COND_LT   3
#define REMOTE_DEBUG_COND_GT   4
#define REMOTE_DEBUG_COND_LE   5
#define REMOTE_DEBUG_COND_GE   6
void remote_debug_watchpoint_clear(void);
void remote_debug_watchpoint_toggle(int index);
void remote_debug_watchpoint_delete(int index);

/* JSON list of all break- and watchpoints. *buffer is malloc()ed. */
void remote_debug_get_points(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Callstack (hooks called from the M6809 core via m6809.h macros)    */
/* ------------------------------------------------------------------ */

void remote_debug_push_call(UINT32 caller, UINT32 receiver);
void remote_debug_pop_call(void);
void remote_debug_reset_callstack(void);

/* JSON dump of the callstack. *buffer is malloc()ed. */
void remote_debug_get_callstack(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Memory access trace                                                */
/* ------------------------------------------------------------------ */

/* Add an address to the set of traced addresses; every access to a
 * traced address is recorded in a ring buffer. bank -1 matches any bank;
 * a specific bank only matters for the 0x4000-0x7FFF ROM window. */
void remote_debug_trace_add(UINT32 adr, int bank);

/* Clear the traced address set and the recorded log. */
void remote_debug_trace_clear(void);

/* JSON dump of traced addresses and the access log. *buffer is malloc()ed. */
void remote_debug_get_trace(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Playfield objects: switches, lamps, solenoids                      */
/* ------------------------------------------------------------------ */

/* JSON arrays of switches / lamps / solenoids with number, matrix
 * position, active state and (where known) a standard WPC name.
 * *buffer is malloc()ed. */
void remote_debug_get_switches(char **buffer, int *len);
void remote_debug_get_lamps(char **buffer, int *len);
void remote_debug_get_solenoids(char **buffer, int *len);

/* Set a switch, optionally as a timed pulse: hold `val` for `pulse_ms`
 * milliseconds (wall clock) then restore the opposite value. pulse_ms <= 0
 * sets the level permanently. Returns 0 on success, -1 if the core is not
 * ready. */
int remote_debug_set_switch(int sw, int val, int pulse_ms);

/* ------------------------------------------------------------------ */
/* Object monitoring (change log)                                     */
/* ------------------------------------------------------------------ */

#define REMOTE_DEBUG_OBJ_SWITCH 0
#define REMOTE_DEBUG_OBJ_LAMP   1
#define REMOTE_DEBUG_OBJ_SOL    2

/* Watch an object for state changes; each transition is appended to the
 * action log with a timestamp. When brk is nonzero, execution is paused on
 * the next frame after the object changes. Returns 0 on success, -1 on a
 * full table. */
int remote_debug_monitor_add(int obj_type, int id, int brk);
void remote_debug_monitor_clear(void);

/* JSON list of monitored objects. *buffer is malloc()ed. */
void remote_debug_get_monitors(char **buffer, int *len);

/* JSON action log of recorded state changes. *buffer is malloc()ed. */
void remote_debug_get_action_log(char **buffer, int *len);
void remote_debug_action_log_clear(void);

/* Sample all monitored objects and log changes. Called from the frame
 * hook (emulator thread). */
void remote_debug_monitor_sample(void);

/* ------------------------------------------------------------------ */
/* Code instrumentation (PC hit counting)                             */
/* ------------------------------------------------------------------ */

/* Count how often execution passes a code address. bank -1 matches any
 * bank. Returns 0 on success, -1 on a full table. */
int remote_debug_instrument_add(UINT32 addr, int bank);
void remote_debug_instrument_clear(void);

/* JSON list of instrumented addresses with hit counts. *buffer malloc()ed. */
void remote_debug_get_instrumentation(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Execution trace (ring buffer of executed instructions)             */
/* ------------------------------------------------------------------ */

void remote_debug_exectrace_start(void);
void remote_debug_exectrace_stop(void);
void remote_debug_exectrace_clear(void);

/* JSON of the recorded instruction ring (oldest to newest). malloc()ed. */
void remote_debug_get_exectrace(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Coverage map (which addresses executed, per bank)                  */
/* ------------------------------------------------------------------ */

void remote_debug_coverage_start(void);
void remote_debug_coverage_stop(void);
void remote_debug_coverage_clear(void);

/* JSON summary {enabled, executed, addressable}. malloc()ed. */
void remote_debug_get_coverage_info(char **buffer, int *len);

/* JSON 0/1 executed flag per address over a window. malloc()ed. */
void remote_debug_get_coverage_region(char **buffer, int *len, UINT32 addr, int size, int bank);

/* ------------------------------------------------------------------ */
/* Tracepoints (log a register snapshot and continue, no halt)        */
/* ------------------------------------------------------------------ */

/* Add a tracepoint at addr (bank -1 = any). Returns 0, -1 if full. */
int remote_debug_tracepoint_add(UINT32 addr, int bank);
void remote_debug_tracepoint_clear(void);

/* JSON of tracepoints and their collected register-snapshot log. malloc()ed. */
void remote_debug_get_tracepoints(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Memory value scan (cheat-engine style variable search)             */
/* ------------------------------------------------------------------ */

/* Take a fresh snapshot of [addr, addr+size); every byte becomes a scan
 * candidate. Returns the candidate count, or -1 on error. */
int remote_debug_scan_new(int cpu, UINT32 addr, int size);

/* Filter scan candidates. op is one of the REMOTE_DEBUG_SCAN_* codes; val
 * is used by EQ/NE only. Returns the remaining candidate count, or -1. */
#define REMOTE_DEBUG_SCAN_EQ        0
#define REMOTE_DEBUG_SCAN_NE        1
#define REMOTE_DEBUG_SCAN_CHANGED   2
#define REMOTE_DEBUG_SCAN_UNCHANGED 3
#define REMOTE_DEBUG_SCAN_INC       4
#define REMOTE_DEBUG_SCAN_DEC       5
int remote_debug_scan_filter(int op, UINT8 val);

/* JSON of current scan candidates (addresses + current values, capped).
 * *buffer is malloc()ed. */
void remote_debug_get_scan(char **buffer, int *len);

/* ------------------------------------------------------------------ */
/* Memory / register operations                                       */
/* ------------------------------------------------------------------ */

void remote_debug_memory_fill(int cpu, UINT32 addr, int size, UINT8 val);

/* Write a block of bytes. Returns 0 on success, -1 on error. */
int remote_debug_memory_write_block(int cpu, UINT32 addr, const UINT8 *data, int len);

/* Search memory for a byte pattern. Returns 1 and sets *found_addr on a
 * match, 0 otherwise. bank -1 reads through the current memory map; a
 * specific bank reads the 0x4000-0x7FFF window from that WPC ROM bank. */
int remote_debug_memory_find(int cpu, UINT32 addr, int size, const UINT8 *pattern, int pat_len, UINT32 *found_addr, int bank);

/* Read a byte, optionally from a specific WPC ROM bank (see the .c file).
 * Caller must hold the debugger lock. */
UINT8 remote_debug_read_byte(int cpu, UINT32 addr, int bank);

/* Set a CPU register (reg is a cpu core register id). */
void remote_debug_set_register(int cpu, int reg, UINT32 val);

/* ------------------------------------------------------------------ */
/* Messages / events                                                  */
/* ------------------------------------------------------------------ */

/* Append a line to the message log (also published as an SSE event). */
void remote_debug_add_message(const char *msg);

/* JSON dump of the message log. *buffer is malloc()ed. */
void remote_debug_get_messages(char **buffer, int *len);

/* Pop the next pending SSE event into buf (NUL-terminated JSON object).
 * Returns 1 if an event was copied, 0 if the queue is empty.
 * Called by the HTTP server thread. */
int remote_debug_next_event(char *buf, int maxlen);

/* Copy src into dst as a JSON string body (without surrounding quotes),
 * escaping backslash, quote and control characters. dst is always
 * NUL-terminated; at most dstlen bytes are written. Returns dst. */
char *remote_debug_json_escape(char *dst, int dstlen, const char *src);

/* ------------------------------------------------------------------ */
/* Save states                                                        */
/* ------------------------------------------------------------------ */

/* Lightweight checkpoint of the game state (WPC RAM + main CPU registers)
 * into a named in-memory slot. This deliberately does not use MAME's full
 * state-save machinery (which is incomplete/unstable for WPC/DCS); it
 * captures exactly what is useful for reverse engineering and can be
 * saved/restored reliably while paused. Returns 0 on success, -1 on error
 * (bad name, no slot free, or unknown slot on load). */
int remote_debug_savestate_save(const char *slot);
int remote_debug_savestate_load(const char *slot);
void remote_debug_savestate_delete(const char *slot);

/* JSON list of occupied save slots. *buffer is malloc()ed. */
void remote_debug_get_savestates(char **buffer, int *len);

/* Diff the RAM of save slot a against slot b, or against the live RAM when
 * b is NULL/empty. JSON {a, b, count, diffs:[{addr,a,b}...]}. malloc()ed. */
void remote_debug_savestate_diff(char **buffer, int *len, const char *a, const char *b);

/* ------------------------------------------------------------------ */
/* DMD frame recorder                                                 */
/* ------------------------------------------------------------------ */

/* Start / stop / clear capturing DMD frames into a ring buffer (one frame
 * per emulated video frame while recording). */
void remote_debug_dmdrec_start(void);
void remote_debug_dmdrec_stop(void);
void remote_debug_dmdrec_clear(void);

/* JSON recorder status: {recording, count, width, height, capacity}. */
void remote_debug_get_dmdrec_info(char **buffer, int *len);

/* Packed binary dump of all recorded frames:
 *   u32 count, u32 width, u32 height, then per frame: u32 t_ms + w*h bytes.
 * All integers little-endian. *buffer is malloc()ed. */
void remote_debug_get_dmdrec_data(char **buffer, int *len);

/* Capture one DMD frame if recording is active. Called from the frame hook
 * (emulator thread). */
void remote_debug_dmdrec_sample(void);

/* ------------------------------------------------------------------ */
/* Display capture                                                    */
/* ------------------------------------------------------------------ */

/* Frame hook (called from the video backend) storing the most recent
 * display for the screenshot endpoints. */
void remote_debug_frame_update(struct mame_display *display);

void remote_debug_get_screenshot_info(int *w, int *h);
void remote_debug_get_raw_screenshot(char **buffer, int *len);
void remote_debug_get_screenshot(char **buffer, int *len, char *content_type);
void remote_debug_get_dmd_screenshot(char **buffer, int *len, char *content_type);
void remote_debug_get_dmd_pnm(char **buffer, int *len, char *content_type);

#endif /* REMOTE_DEBUG_H */
