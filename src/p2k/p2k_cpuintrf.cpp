// license:BSD-3-Clause

// PinMAME P2K subsystem - bridge between the imported MediaGX core and PinMAME's cpuintrf.
//
// PinMAME addresses CPUs through a table of plain C function pointers (src/cpuintrf.c), and
// everything built on top of it - the debugger's activecpu_get_reg/activecpu_dasm among them -
// goes through that table. The MediaGX lives in this subsystem with its own bus and its own
// device model, so it needs an adapter to appear there. This is that adapter.
//
// It is deliberately thin: the machine itself is still assembled by p2k_state, and this only
// forwards. What it makes possible is inspection - registers and disassembly

#include "p2k_driver.h"
#include "p2k_debug.h"
#include "p2k_weak.h"
#include "i386.h"
#include "i386priv.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Mirrored from PinMAME's src/cpuintrf.h. The subsystem deliberately does not include PinMAME
// headers - they belong to the other half of the build - so the handful of constants the table
// interface uses are restated here. Keep in sync if cpuintrf.h ever renumbers them
enum
{
	P2K_REG_PC = -2,
	P2K_REG_SP = -3,
	P2K_MAX_REGS = 128,
	P2K_CPU_INFO_FLAGS = P2K_MAX_REGS,
	P2K_CPU_INFO_NAME,
	P2K_CPU_INFO_FAMILY,
	P2K_CPU_INFO_VERSION,
	P2K_CPU_INFO_FILE,
	P2K_CPU_INFO_CREDITS
};

// counters from the interrupt path (defined in p2k_driver.cpp and below)
extern u64 g_p2k_pit0_edges, g_p2k_pic_int;
extern u64 g_p2k_tick_held, g_p2k_clkint_entered, g_p2k_clkint_left;
u64 g_p2k_irq_dispatched = 0;

namespace {

p2k_state *g_bridge_state = nullptr;
mediagx_device *g_bridge_cpu = nullptr;
std::unique_ptr<util::disasm_interface> g_disasm;

// feeds the disassembler from the machine's program space
class p2k_opcode_buffer final : public util::disasm_interface::data_buffer
{
public:
	u8  r8 (offs_t pc) const override { return u8 (read(pc, 1)); }
	u16 r16(offs_t pc) const override { return u16(read(pc, 2)); }
	u32 r32(offs_t pc) const override { return u32(read(pc, 4)); }
	u64 r64(offs_t pc) const override { return read(pc, 8); }

private:
	static u64 read(offs_t pc, unsigned bytes)
	{
		u64 result = 0;
		if (!g_bridge_state) return result;
		for (unsigned i = 0; i < bytes; i++)
		{
			offs_t a = pc + i;
			unsigned shift = (a & 3) * 8;
			u8 byte = u8(g_bridge_state->mem_r(a & ~3u, 0xffu << shift) >> shift);
			result |= u64(byte) << (i * 8);
		}
		return result;
	}
};

// Instructions retired, counted in the per-instruction hook. Cycles are what the timing is
// built on, but instructions per second is the number to compare an interpreter against
u64 g_p2k_instr_total = 0;

// PinMAME time slices, i.e. calls to mediagx_execute. Slices per emulated second is a number
// worth watching: anything that makes the scheduler split intervals shows up here first, and
// each slice carries a pass over the machine's timer queue
u64 g_p2k_slices = 0;

// Running inside PinMAME, the machine has no console of its own: the boot code's serial output
// and the CPU's whereabouts are invisible. P2K_PROGRESS=<cycles> makes the bridge report both to
// stderr every so many cycles - the same view p2kboot prints, which is what makes the two comparable
#if !P2K_DEBUG
inline void report_progress(u64) {}
#else
void report_progress(u64 cycles)
{
	static const u64 interval = []() -> u64 {
		const char *s = getenv("P2K_PROGRESS");
		return s ? strtoull(s, nullptr, 0) : 0ull;
	}();
	if (!interval || !g_bridge_state || !g_bridge_cpu) return;

	static u64 elapsed = 0, next = interval;
	static size_t console_seen = 0;
	elapsed += cycles;
	if (elapsed < next) return;
	next = elapsed + interval;

	// Host time, because the emulated counters cannot show it. instr/cycles is guest IPC - a
	// property of the guest's own workload, the same however fast or slow the host runs - so
	// anything that costs host time, the per-instruction hook the clkint gate arms for one, is
	// invisible in it. "host" is seconds of wall clock since the first report and "mips" the guest
	// instructions retired per second of it, which is the pair to compare builds and switches on
	static const auto t0 = std::chrono::steady_clock::now();
	static const u64 instr0 = g_p2k_instr_total;
	const double host = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
	const double mips = host > 0.0 ? double(g_p2k_instr_total - instr0) / host / 1e6 : 0.0;

	const std::string &console = g_bridge_state->console_log();
	if (console.size() > console_seen)
	{
		fprintf(stderr, "[p2k console] %s\n", console.c_str() + console_seen);
		console_seen = console.size();
	}
	fprintf(stderr, "[p2k %llu cycles] host=%.2fs mips=%.1f instr=%llu slices=%llu pit0=%llu picint=%llu irq=%llu in/out=%llu/%llu held=%llu PC=%08x EIP=%08x CS=%04x CSBASE=%08x ESP=%08x CR0=%08x\n",
		(unsigned long long)elapsed, host, mips, (unsigned long long)g_p2k_instr_total,
		(unsigned long long)g_p2k_slices,
		(unsigned long long)g_p2k_pit0_edges, (unsigned long long)g_p2k_pic_int,
		(unsigned long long)g_p2k_irq_dispatched,
		(unsigned long long)g_p2k_clkint_entered, (unsigned long long)g_p2k_clkint_left,
		(unsigned long long)g_p2k_tick_held,
		unsigned(g_bridge_cpu->state_int(I386_PC)), unsigned(g_bridge_cpu->state_int(I386_EIP)),
		unsigned(g_bridge_cpu->state_int(I386_CS)), unsigned(g_bridge_cpu->state_int(I386_CS_BASE)),
		unsigned(g_bridge_cpu->state_int(I386_ESP)), unsigned(g_bridge_cpu->state_int(I386_CR0)));
	fflush(stderr);
}
#endif // P2K_DEBUG

} // anonymous namespace

u64 g_p2k_cycles_total = 0;
static void p2k_report_progress(u64 cycles) { g_p2k_cycles_total += cycles; report_progress(cycles); }

// The P2K machine registers itself here so the bridge has something to talk to. Both pointers
// exist only with P2K_DEBUG - shim/debugger.h compiles the call sites out otherwise
#if P2K_DEBUG
extern void (*p2k_instruction_hook)(unsigned pc);
extern void (*p2k_exception_hook)(int vector);
#endif
// PinMAME checks breakpoints from inside a CPU core's instruction loop. This core runs its own
// loop inside the subsystem, so the hook is called between execution chunks instead - fine for
// stopping on an address, just not instruction-exact. It is only linked in with REMOTE_DEBUG,
// which these files cannot test for; see p2k_weak.h for how each compiler decides
#if !defined(REMOTE_DEBUG)
  #define P2K_REMOTE_DEBUG_HOOK() ((void)0)
#elif defined(_MSC_VER)
  extern "C" void remote_debug_breakpoint_hook(void);
  #define P2K_REMOTE_DEBUG_HOOK() remote_debug_breakpoint_hook()
#else
  extern "C" void remote_debug_breakpoint_hook(void) P2K_WEAK;
  #define P2K_REMOTE_DEBUG_HOOK() \
	do { if (remote_debug_breakpoint_hook) remote_debug_breakpoint_hook(); } while (0)
#endif
extern "C" unsigned mediagx_get_reg(int regnum);
extern "C" int mediagx_ICount;

// set when PinMAME cuts the time slice short (a breakpoint hit), cleared at the next slice
static bool g_bridge_aborted = false;

#if P2K_DEBUG
// P2K_BACKTRACE=<n>: ring of the last n program counters. The boot derail leaves no trace of how
// it got there, so this records the way in. It is dumped once, at whichever comes first: the CPU
// leaving protected mode, or the first trap. The mode change is the earlier and more telling of
// the two - by the time a trap fires the machine has already marched through a lot of memory.
// P2K_BTAT=<addr> moves the trigger to an address instead, and then it is the only trigger -
// mode changes and traps that happen on the way there must not consume the one dump
static std::vector<unsigned> g_backtrace = []() {
	const char *s = getenv("P2K_BACKTRACE");
	int n = s ? (int)strtol(s, nullptr, 0) : 0;
	return std::vector<unsigned>(n > 0 ? size_t(n) : 0);
}();
static const unsigned g_backtrace_at = []() {
	const char *s = getenv("P2K_BTAT");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();
// P2K_BTBELOW=<addr>: dump the ring the first time the PC drops below that address. A derail
// into low memory marches for a long time before it reaches any address one could name, so the
// useful trigger is the entry into the region, not a point inside it
static const unsigned g_backtrace_below = []() {
	const char *s = getenv("P2K_BTBELOW");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();
static u64 g_backtrace_pos = 0;
static bool g_backtrace_dumped = false;

static void dump_backtrace(const char *reason)
{
	if (g_backtrace.empty() || g_backtrace_dumped) return;
	g_backtrace_dumped = true;
	size_t n = g_backtrace.size();
	size_t have = size_t((g_backtrace_pos < n) ? g_backtrace_pos : n);
	fprintf(stderr, "[p2k backtrace] last %zu instructions before %s:\n", have, reason);
	for (size_t i = 0; i < have; i++)
		fprintf(stderr, "  %08x\n", g_backtrace[(g_backtrace_pos - have + i) % n]);
	fflush(stderr);
}

// P2K_WATCH=<from>[-<to>], hexadecimal: register dump for instructions in this address range
static unsigned g_watch_from = 0, g_watch_to = 0;
static int g_forward_left = 0;   // instructions still to follow after the mode change
static const bool g_watch_init = []() {
	if (const char *s = getenv("P2K_WATCH"))
	{
		char *end = nullptr;
		g_watch_from = unsigned(strtoul(s, &end, 16));
		g_watch_to = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : g_watch_from;
	}
	return true;
}();

// per-instruction callback for the imported core; PinMAME checks its breakpoints here.
// P2K_HOOKTRACE=<n> prints the first n program counters the core reports, next to what the
// bridge answers when PinMAME asks - the two must agree or no breakpoint can ever match.
// P2K_DUMP=<from>[-<to>] with P2K_DUMPAT=<pc>, all hexadecimal: dump a memory range as hex and
// text when the CPU reaches that address. The debugger's memory endpoint goes through PinMAME's
// memory system, which has no map for this CPU and answers for a different machine - this reads
// the machine's own bus, at a moment the caller chooses
static unsigned g_dump_from = 0, g_dump_to = 0, g_dump_at = 0;
static const bool g_dump_init = []() {
	if (const char *s = getenv("P2K_DUMP"))
	{
		char *end = nullptr;
		g_dump_from = unsigned(strtoul(s, &end, 16));
		g_dump_to = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : g_dump_from + 0x3f;
	}
	if (const char *s = getenv("P2K_DUMPAT")) g_dump_at = unsigned(strtoul(s, nullptr, 16));
	return true;
}();

// P2K_STACKAT=<pc>[:<n>]: dump n dwords from the current ESP when the CPU reaches that address,
// n defaulting to 16. A fixed range cannot answer "who called this" - the stack moves
static unsigned g_stack_at = 0;
static int g_stack_words = 16;
static const bool g_stack_init = []() {
	if (const char *s = getenv("P2K_STACKAT"))
	{
		char *end = nullptr;
		g_stack_at = unsigned(strtoul(s, &end, 16));
		if (end && *end == ':') g_stack_words = (int)strtol(end + 1, nullptr, 0);
	}
	return true;
}();

// P2K_STACKBELOW=<esp>: dump the stack the first time ESP drops below that value. Runaway
// recursion is only readable at depth, and an address trigger fires on the shallow first call
static const unsigned g_stack_below = []() {
	const char *s = getenv("P2K_STACKBELOW");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();

// P2K_STACKAFTER=<cycles>: hold the stack probes back until that many cycles have run. A hang
// looks the same on its first pass as on its millionth, but the call chain that matters is the
// one during the hang
static const u64 g_stack_after = []() {
	const char *s = getenv("P2K_STACKAFTER");
	return s ? strtoull(s, nullptr, 0) : 0ull;
}();

static void dump_stack()
{
	if (!g_bridge_state || !g_bridge_cpu) return;
	const unsigned esp = unsigned(g_bridge_cpu->state_int(I386_ESP));
	fprintf(stderr, "[p2k stack] at pc %08x, ESP=%08x\n", g_stack_at, esp);
	for (int i = 0; i < g_stack_words; i++)
	{
		const offs_t a = esp + unsigned(i) * 4;
		fprintf(stderr, "  [esp+%02x] %08x = %08x\n", unsigned(i * 4), a, g_bridge_state->mem_r(a, 0xffffffff));
	}
	fflush(stderr);
}

// P2K_FIND=<lo>-<hi>: instead of a hex dump, report every dword in the P2K_DUMP range whose
// value falls between lo and hi. Finding which of 55 task stacks holds a return address into a
// given function is otherwise 55 runs
static unsigned g_find_lo = 0, g_find_hi = 0;
static const bool g_find_init = []() {
	if (const char *s = getenv("P2K_FIND"))
	{
		char *end = nullptr;
		g_find_lo = unsigned(strtoul(s, &end, 16));
		g_find_hi = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : g_find_lo;
	}
	return true;
}();

static void find_range()
{
	if (!g_bridge_state) return;
	fprintf(stderr, "[p2k find] dwords in %08x..%08x within %08x..%08x\n",
		g_find_lo, g_find_hi, g_dump_from, g_dump_to);
	for (offs_t a = g_dump_from & ~3u; a <= g_dump_to; a += 4)
	{
		const u32 v = g_bridge_state->mem_r(a, 0xffffffff);
		if (v >= g_find_lo && v <= g_find_hi)
			fprintf(stderr, "  %08x = %08x\n", a, v);
	}
	fflush(stderr);
}

static void dump_range()
{
	if (!g_bridge_state) return;
	fprintf(stderr, "[p2k dump] %08x-%08x\n", g_dump_from, g_dump_to);
	for (unsigned base = g_dump_from & ~0xfu; base <= g_dump_to; base += 16)
	{
		char hex[16 * 3 + 1], txt[17];
		for (unsigned i = 0; i < 16; i++)
		{
			const offs_t a = base + i;
			const unsigned shift = (a & 3) * 8;
			const u8 b = u8(g_bridge_state->mem_r(a & ~3u, 0xffu << shift) >> shift);
			snprintf(hex + i * 3, 4, "%02x ", b);
			txt[i] = (b >= 0x20 && b < 0x7f) ? char(b) : '.';
		}
		txt[16] = 0;
		fprintf(stderr, "  %08x  %s |%s|\n", base, hex, txt);
	}
	fflush(stderr);
}
#endif // P2K_DEBUG


// ---------------------------------------------------------------- the clkint gate
// Off by default, and no longer needed. It held clock interrupts back while the guest was inside
// its own clock handler, and what made that necessary was a defect in this shim: interrupt
// delivery was late. pic8259_device re-evaluates its INT output from a zero-delay timer and
// nothing here cut the CPU's slice short when one was set, so the guest unmasked and the interrupt
// arrived wherever the slice happened to break instead of at the instruction after the OUT. See
// p2k_state::pics_settle(), and "Interrupts used to arrive late" in README.md for the numbers.
//
// It stays as a switch and as an instrument: P2K_CLKINT_GATE=1 reproduces the old behaviour in one
// run, which is most of the diagnosis if a set ever derails again, and the frame tracking below
// makes in/out and held a direct measure of interrupt nesting.
//
// What it was protecting against is worth keeping, because it is about the guest and none of it is
// obvious. Measured on rfm_160 with P2K_IOWATCH on 20-21, P2K_TRAPTRACE, and a trap on resched's
// complaint:
//
//   * The 8259 is edge-triggered, vector base 0x20, ICW4 = 0x01 - so AEOI is off and the in-service
//     bit really is set at each acknowledge.
//   * But clkint EOIs at its FIRST instruction (out 0x20,0x20 from 0x25ba84), so that bit is clear
//     for the whole handler and guards nothing.
//   * The only remaining guard is the interrupt mask, and XINU's pair for it is misleadingly named:
//     enable() masks everything (out 0x21,0xff) and disable() restores the saved mask. They are a
//     critical section's entry and exit, not cli/sti - IF is untouched, which is why the interrupt
//     frames show it set.
//   * So every disable() inside the handler's call tree re-opens IR0. At depth two the interrupt
//     epilogue's `dec [0x325314]` leaves 1 rather than 0, resched refuses with "resched: called
//     from interrupt handler", and that report is itself a report - the loop that overflows the
//     task's 8 KB stack.
//
// Three timing explanations were tried before the real one and each is dead, recorded so nobody
// spends the time again: the tick rate (P2K_PIT_HZ=397727 restores the real cycles-per-tick ratio
// and it still derailed), the handler running longer than a tick period, and the pinball I/O
// critical section being long (paired PC traps put it under ~5000 cycles against 19406 in a tick).
// The rate was never the variable. The latency was.
//
// Switched on, it holds the delivery and not the edge: an earlier version suppressed the PIT's
// rising edge and changed nothing, the tick that nests having been latched in the IRR before the
// handler started. Both its signals are generic, with no game addresses in them - entry is the CPU
// dispatching the tick vector, which vector that is being learned from the first dispatch after the
// driver reports an edge on IR0; exit is the handler's IRET, inspected only while inside it. And it
// is bounded, which it has to be: XINU's clock handler does not always return by IRET - when it
// reschedules it switches tasks and the frame is popped later, in another context - so after four
// held edges it gives up and opens.
//
// P2K_CLKINT_MAX_SKIP=<n> moves that bound and P2K_CLKINT_COUNTER=<addr> gates on the firmware's own
// nesting counter instead of the IRET heuristic. Without P2K_DEBUG all three are the constants they
// default to, so the compiler drops the branches entirely
#if P2K_DEBUG
// Off by default now that pics_settle() removes the reason for it. P2K_CLKINT_GATE=1 puts it
// back, which is worth having: it is the one switch that reproduces the old behaviour if a set
// ever derails again, and telling the two apart in one run is most of the diagnosis
static const bool g_clkint_gate = []() {
	const char *s = getenv("P2K_CLKINT_GATE");
	return s && strtol(s, nullptr, 0) != 0;
}();
static const int g_max_skips = []() {
	const char *s = getenv("P2K_CLKINT_MAX_SKIP");
	const int n = s ? (int)strtol(s, nullptr, 0) : 4;
	return n > 0 ? n : 1;
}();
static const unsigned g_clkint_counter = []() {
	const char *s = getenv("P2K_CLKINT_COUNTER");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();
#else
static constexpr bool     g_clkint_gate    = false;
static constexpr int      g_max_skips      = 4;
static constexpr unsigned g_clkint_counter = 0;
// Load-bearing: with the gate off, nothing starts the frame tracking, so nothing reads what the CPU
// hooks produce. shim/debugger.h relies on that to compile debugger_instruction_hook() out of the
// i386 execute loop, and everything below that feeds the hooks is #if P2K_DEBUG for the same
// reason. Turning the gate on here without undoing that would leave the frames untracked and the
// gate holding interrupts it never releases - so fail the build instead
static_assert(!g_clkint_gate, "the clkint gate needs the per-instruction hook, which is P2K_DEBUG-only - see shim/debugger.h");
#endif
// Interrupt frames, innermost last, one bit each: 1 for a clock handler, 0 for anything else.
// A plain "inside clkint" flag is not enough, and the traces say why - the machine dispatches
// other vectors (0x23 among them) while the clock handler is open, and the clock handler nests
// into itself several deep. With one flag the first IRET seen ends the state, so a nested
// handler's return released the gate while an outer clock handler was still running, and a
// re-entry did not count at all. Frames pop in LIFO order, so a bitmask tracks them exactly.
//
// Tracking starts when the first clock frame opens and stops when the last one closes; nothing
// else can be open at that moment, because frames below it were pushed after it. Past 64 levels
// the extra frames are counted separately and popped first, which keeps the order right - by then
// the machine is dying anyway, and that is what the depth is being counted to see
static int  g_frame_depth = 0;
static int  g_clkint_depth = 0;        // how many of them are clock handlers
static bool g_gate_released = false;   // the skip bound fired; stop holding until depth is 0 again
static bool g_irq0_armed = false;      // an edge reached the PIC, no dispatch yet
static int  g_edges_while_held = 0;
#if P2K_DEBUG
// Only the CPU hooks maintain these, and those are compiled out without P2K_DEBUG
static u64  g_frame_bits = 0;          // bit n = frame n is a clock handler
static int  g_frame_extra = 0;         // frames past the 64 the mask holds
static bool g_clkint_iret_seen = false;
static int  g_tick_vector = -1;        // learned from the first dispatch after an edge
#endif
u64 g_p2k_tick_held = 0;
u64 g_p2k_clkint_entered = 0;
u64 g_p2k_clkint_left = 0;

extern void p2k_apply_irq0();          // the driver re-evaluates the CPU's IRQ0 line

// true while the guest is inside the clock handler and delivery should be held back
bool p2k_clkint_blocks_irq()
{
	if (!g_clkint_gate) return false;
	if (g_clkint_counter && g_bridge_state)
		return g_bridge_state->mem_r(g_clkint_counter & ~3u, 0xffffffff) != 0;
	return g_clkint_depth > 0 && !g_gate_released;
}

// The per-instruction hook is the one thing in the bridge that EVERY emulated instruction pays
// for. It has work while an interrupt frame is being tracked - that is the state whose exits it
// watches for, by looking for each handler's IRET - and outside it p2k_debug_step() only bumps
// g_p2k_instr_total, whose sole reader is report_progress().
//
// Without P2K_DEBUG neither is live: the gate is off so no frame is tracked (see the static_assert
// above), and report_progress() is an empty inline. So the whole chain - this arming, the frames,
// both step functions - is compiled out and shim/debugger.h drops the call site with it, leaving
// nothing at all in the i386 execute loop
#if P2K_DEBUG

static void p2k_debug_step(unsigned pc); // defined below, installed by arm_instruction_hook()

// In a P2K_DEBUG build the hook stays installed unconditionally, because there the counter is live
static inline void arm_instruction_hook()
{
	p2k_instruction_hook = p2k_debug_step;
}

// a handler was entered: push its frame. Only clock handlers start the tracking; once it is
// running every vector is pushed, so the IRETs pair up with the right frames
static void push_int_frame(bool is_clkint)
{
	if (!is_clkint && g_frame_depth == 0) return;   // not tracking, and this does not start it
	if (g_frame_depth < 64) g_frame_bits = (g_frame_bits << 1) | (is_clkint ? 1u : 0u);
	else                    g_frame_extra++;
	g_frame_depth++;
	if (is_clkint)
	{
		if (g_clkint_depth++ == 0) { g_edges_while_held = 0; g_gate_released = false; }
		g_p2k_clkint_entered++;
	}
	arm_instruction_hook();
	p2k_apply_irq0();
}

// an IRET retired: pop the innermost frame
static void pop_int_frame()
{
	if (g_frame_depth == 0) return;
	bool was_clkint = false;
	if (g_frame_extra > 0) g_frame_extra--; // the overflow frames are the innermost ones
	else { was_clkint = (g_frame_bits & 1) != 0; g_frame_bits >>= 1; }
	g_frame_depth--;
	if (was_clkint) { g_clkint_depth--; g_p2k_clkint_left++; }
	if (g_clkint_depth == 0)
	{
		// the outermost clock frame is gone, so nothing pushed after it can still be open
		g_frame_bits = 0; g_frame_depth = 0; g_frame_extra = 0; g_gate_released = false;
	}
	arm_instruction_hook();
	p2k_apply_irq0(); // the line may be free now, or has to go away
}

#endif // P2K_DEBUG

// called from the driver for every rising edge the PIT puts on IR0. The edge always reaches the
// PIC now - this only bounds how long the gate may hold delivery
void p2k_clkint_note_edge()
{
	if (!g_clkint_gate) { g_irq0_armed = true; return; }
	if (p2k_clkint_blocks_irq())
	{
		g_p2k_tick_held++;
		// the bound: stop holding, but leave the depth alone - it is real, and the handler is
		// still running. Holding resumes when the outermost clock frame closes
		if (++g_edges_while_held >= g_max_skips) { g_gate_released = true; p2k_apply_irq0(); }
		return;
	}
	g_irq0_armed = true;
}


#if P2K_DEBUG
// P2K_PCTRAP="<hex>[=<label>],...": report when execution reaches any of those addresses, with
// EAX alongside. The reason this exists: the game's start-button gates (AudioIsReady,
// MultiDevice::game_start_check, jts_game_start_check, the switch pre-filter) all reject the
// press silently, so their effect is indistinguishable from outside. Trapping the instruction
// after each call makes the decision itself visible - EAX at that point is the gate's answer.
// P2K_PCTRAP_MAX caps the reports per address (default 8) so a trap inside a loop cannot flood
namespace {

struct pc_trap { unsigned addr; char label[24]; unsigned hits; };
pc_trap  g_pctrap[16];
unsigned g_pctrap_n   = 0;
unsigned g_pctrap_lo  = ~0u;
unsigned g_pctrap_hi  = 0;
u64      g_pctrap_bits= 0; // (addr & 63) presence, so the common case costs one test
unsigned g_pctrap_max = 8;

// P2K_PCTRAP_AFTER=<cycles>: hold the traps back until then, the same way P2K_STACKAFTER holds the
// stack probes. Without it a trap on anything the machine does regularly reports its first few hits
// at boot and is quiet long before the interesting ones - swep1_210 wedges near cycle 2.74 billion,
// by which point a 0.25 ms task has fired a hundred thousand times. Watch the hit counter against
// P2K_PCTRAP_MAX too: a trace that stops because the cap was reached looks exactly like one that
// stops because the events did, and reading it the wrong way has cost real time here
u64 g_pctrap_after = 0;

void pctrap_init()
{
	const char *s = getenv("P2K_PCTRAP");
	if (!s) return;
	if (const char *m = getenv("P2K_PCTRAP_MAX")) g_pctrap_max = unsigned(strtoul(m, nullptr, 0));
	if (const char *a = getenv("P2K_PCTRAP_AFTER")) g_pctrap_after = strtoull(a, nullptr, 0);
	while (*s && g_pctrap_n < 16)
	{
		char *end = nullptr;
		const unsigned a = unsigned(strtoul(s, &end, 16));
		if (end == s) break;
		pc_trap &t = g_pctrap[g_pctrap_n++];
		t.addr = a; t.hits = 0; t.label[0] = 0;
		if (end && *end == '=')
		{
			const char *lb = end + 1;
			const char *e2 = strchr(lb, ',');
			size_t n = e2 ? size_t(e2 - lb) : strlen(lb);
			if (n >= sizeof t.label) n = sizeof t.label - 1;
			memcpy(t.label, lb, n); t.label[n] = 0;
			end = const_cast<char *>(lb + n);
		}
		if (a < g_pctrap_lo) g_pctrap_lo = a;
		if (a > g_pctrap_hi) g_pctrap_hi = a;
		g_pctrap_bits |= u64(1) << (a & 63);
		s = strchr(end, ','); if (!s) break; s++;
	}
	if (g_pctrap_n)
		fprintf(stderr, "[p2k pctrap] %u address(es) armed, max %u report(s) each\n", g_pctrap_n, g_pctrap_max);
}

inline void pctrap_check(unsigned pc)
{
	if (!g_pctrap_n || pc < g_pctrap_lo || pc > g_pctrap_hi) return;
	if (g_pctrap_after && g_p2k_cycles_total < g_pctrap_after) return;
	if (!((g_pctrap_bits >> (pc & 63)) & 1)) return;
	for (unsigned i = 0; i < g_pctrap_n; i++)
	{
		pc_trap &t = g_pctrap[i];
		if (t.addr != pc) continue;
		if (t.hits++ >= g_pctrap_max) return;
		/* EBX/ECX/EDX come along because the interesting value is not always the return
		   value: AProc::existc_range, for instance, carries the process id it matched in EBX,
		   and that id is what names the device */
		unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
		if (g_bridge_cpu)
		{
			eax = unsigned(g_bridge_cpu->state_int(I386_EAX));
			ebx = unsigned(g_bridge_cpu->state_int(I386_EBX));
			ecx = unsigned(g_bridge_cpu->state_int(I386_ECX));
			edx = unsigned(g_bridge_cpu->state_int(I386_EDX));
		}
		// This code is cdecl: arguments go on the stack, not in registers, so trapping a function
		// entry and reading only EAX-EDX shows the caller's leftovers rather than what it was
		// called with. [esp] is the return address, which names the caller once resolved against
		// symbols.rom, and [esp+4] onwards are the arguments - a trap on wait() or signal_waiter()
		// is only useful because ret/arg1 say which semaphore. Both are read through the bridge at
		// the instant of the trap
		unsigned ret = 0, arg1 = 0, arg2 = 0;
		if (g_bridge_cpu && g_bridge_state)
		{
			const unsigned esp = unsigned(g_bridge_cpu->state_int(I386_ESP));
			ret  = g_bridge_state->mem_r(esp,     0xffffffff);
			arg1 = g_bridge_state->mem_r(esp + 4, 0xffffffff);
			arg2 = g_bridge_state->mem_r(esp + 8, 0xffffffff);
		}
		// the cycle stamp makes a pair of traps a stopwatch: arm the two ends of a region and the
		// difference is what it cost the guest. How long the pinball I/O holds the PIC mask, for
		// one - see the clkint gate above, where that duration is the open question
		fprintf(stderr, "[p2k pc] %08x %-20s ret=%08x arg=%08x,%08x  eax=%08x ebx=%08x ecx=%08x edx=%08x  cyc=%llu  hit %u\n",
			pc, t.label[0] ? t.label : "-", ret, arg1, arg2, eax, ebx, ecx, edx,
			(unsigned long long)g_p2k_cycles_total, t.hits);
		fflush(stderr);
		return;
	}
}

} // namespace

// This runs before every single guest instruction, so what it does when nothing is asked of it
// is a performance decision, not a detail. Two things were costing measurably:
//
//  - function-local `static` initialisers. Each one compiles to a thread-safe guard that is
//    tested on every call. They are file-scope now, initialised once at load.
//  - the probe chain. Nine or so `if`s, all false in a normal run, were walked per instruction.
//    They sit behind one flag now, computed once.
//
// What stays unconditional is the instruction count and the clkint gate, which has to see the
// handler's IRET. That peek used to go through the bus for every instruction inside clkint - and
// the machine is inside clkint roughly a third of the time - so it reads main RAM directly now,
// falling back to the bus only when the PC is somewhere else
static int g_hooktrace_left = []() -> int {
	const char *s = getenv("P2K_HOOKTRACE");
	return s ? (int)strtol(s, nullptr, 0) : 0;
}();
static bool g_probes_armed = false; // set in p2k_bridge_attach, once everything is parsed


// Both step functions and the peek the first one needs; nothing installs them without P2K_DEBUG.
// The one non-probe thing that goes with them is the per-instruction P2K_REMOTE_DEBUG_HOOK()
// below - but mediagx_execute() calls it once per 2000-cycle chunk regardless, which is what the
// remote debugger has always run on in a release build, the hook being unarmed there

static inline u8 p2k_peek_byte(unsigned a)
{
	if (const u8 *p = g_bridge_state->ram_peek(a)) return *p;
	const unsigned shift = (a & 3) * 8;
	return u8(g_bridge_state->mem_r(a & ~3u, 0xffu << shift) >> shift);
}

static void p2k_debug_step(unsigned pc)
{
	g_p2k_instr_total++;

	// each handler's IRET pops its frame. The hook runs before the instruction, so the pop happens
	// one instruction later - the point is only that no tick is delivered in between
	if (g_frame_depth && g_bridge_state)
	{
		if (g_clkint_iret_seen) { g_clkint_iret_seen = false; pop_int_frame(); }
		else
		{
			const u8 op = p2k_peek_byte(pc);
			if (op == 0xcf || (op == 0x66 && p2k_peek_byte(pc + 1) == 0xcf)) g_clkint_iret_seen = true;
		}
	}

	// Everything from here to the closing brace is a probe: off in a normal run, and the whole
	// chain is skipped in one test. What comes *after* the block is not optional - see there
#if P2K_DEBUG
	if (g_probes_armed)
	{
	pctrap_check(pc);
	if (g_hooktrace_left > 0)
	{
		g_hooktrace_left--;
		fprintf(stderr, "[p2k hook] core pc=%08x bridge REG_PC=%08x\n", pc, mediagx_get_reg(-2));
		fflush(stderr);
	}
	if (g_dump_to && pc == g_dump_at)
	{
		static bool done = false;
		if (!done) { done = true; if (g_find_hi) find_range(); else dump_range(); }
	}
	if (g_stack_at && pc == g_stack_at && g_p2k_cycles_total >= g_stack_after)
	{
		static bool done = false;
		if (!done) { done = true; dump_stack(); }
	}
	if (g_stack_below && g_bridge_cpu)
	{
		static bool done = false;
		// arm only once the machine has run with a stack above the mark - at reset ESP is 0 and every value is "below"
		static bool armed = false;
		const unsigned esp = unsigned(g_bridge_cpu->state_int(I386_ESP));
		if (esp >= g_stack_below) armed = true;
		if (!done && armed && esp < g_stack_below)
		{
			done = true;
			g_stack_at = pc;          // so the dump says where it fired
			if (g_stack_words < 64) g_stack_words = 64;
			dump_stack();
		}
	}
	if (!g_backtrace.empty())
	{
		// P2K_BTAT=<addr>, hexadecimal: dump the ring the first time the CPU reaches that address.
		// The mode change is too late to show who called the mode-switch service; this is not
		if (g_backtrace_below && pc < g_backtrace_below)
		{
			char reason[56];
			snprintf(reason, sizeof(reason), "the PC dropped below %08x (at %08x)",
				g_backtrace_below, pc);
			dump_backtrace(reason);
		}
		if (g_backtrace_at && pc == g_backtrace_at)
		{
			char reason[48];
			snprintf(reason, sizeof(reason), "the CPU reached %08x", g_backtrace_at);
			dump_backtrace(reason);
		}
		// Watch CR0.PE through the state entry itself: a state_int() lookup walks the whole
		// register list, which is too much to do per instruction
		static const device_state_entry *cr0 = nullptr;
		static bool was_protected = false;
		if (!cr0 && g_bridge_cpu) cr0 = g_bridge_cpu->state_find(I386_CR0);
		const bool protected_now = cr0 && (cr0->value() & 1);
		if (was_protected && !protected_now)
		{
			if (!g_backtrace_at && !g_backtrace_below) dump_backtrace("the CPU left protected mode (CR0.PE 1 -> 0)");
			// what the machine does with the mode change is the other half of the question:
			// P2K_FORWARD=<n> follows the next n instructions with registers, 40 by default
			const char *f = getenv("P2K_FORWARD");
			g_forward_left = f ? (int)strtol(f, nullptr, 0) : 40;
		}
		was_protected = protected_now;

		g_backtrace[g_backtrace_pos % g_backtrace.size()] = pc;
		g_backtrace_pos++;
	}
	// P2K_WATCH=<from>[-<to>]: a full register line for every instruction in that range. The
	// debugger's state endpoint reports only PC and SP for this CPU, and stepping through a
	// handful of instructions is exactly what a derail like this needs
	if (g_forward_left > 0) g_forward_left--;
	if (((g_watch_to && pc >= g_watch_from && pc <= g_watch_to) || g_forward_left > 0) && g_bridge_cpu)
	{
		// the bytes at the PC, read the same way the core fetches them and at the same instant.
		// Reading them later through the debugger answers about memory as it is then, which is a
		// different question whenever the code region is being written to
		char bytes[3 * 8 + 1];
		for (unsigned i = 0; i < 8; i++)
		{
			const offs_t a = pc + i;
			const unsigned shift = (a & 3) * 8;
			const u8 b = u8(g_bridge_state->mem_r(a & ~3u, 0xffu << shift) >> shift);
			snprintf(bytes + i * 3, 4, "%02x ", b);
		}
		fprintf(stderr, "[p2k watch] %08x EAX=%08x EBX=%08x ECX=%08x EDX=%08x ESP=%08x "
			"EFLAGS=%08x CS=%04x/%08x DS=%04x SS=%04x/%08x [%s]\n", pc,
			unsigned(g_bridge_cpu->state_int(I386_EAX)), unsigned(g_bridge_cpu->state_int(I386_EBX)),
			unsigned(g_bridge_cpu->state_int(I386_ECX)), unsigned(g_bridge_cpu->state_int(I386_EDX)),
			unsigned(g_bridge_cpu->state_int(I386_ESP)), unsigned(g_bridge_cpu->state_int(I386_EFLAGS)),
			unsigned(g_bridge_cpu->state_int(I386_CS)), unsigned(g_bridge_cpu->state_int(I386_CS_BASE)),
			unsigned(g_bridge_cpu->state_int(I386_DS)),
			unsigned(g_bridge_cpu->state_int(I386_SS)), unsigned(g_bridge_cpu->state_int(I386_SS_BASE)),
			bytes);
	}
	}
#endif // P2K_DEBUG

	// Not probes, and skipping them changes how the machine runs: the breakpoint check is what
	// makes the remote debugger work at all, and the abort below is how a shortened time slice
	// reaches the core. An earlier version of this had them behind the probe test, and the
	// machine's timing shifted by two frames - that is how much they matter
	P2K_REMOTE_DEBUG_HOOK();

	// A breakpoint hit ends the time slice through activecpu_abort_timeslice(), which subtracts
	// the cycles that are left from the CPU's icount - for this bridge that means mediagx_ICount
	// goes negative. Pass that on, or the machine would run to the end of the slice and report a
	// PC far past the breakpoint
	if (mediagx_ICount < 0 && g_bridge_cpu)
	{
		g_bridge_cpu->abort_timeslice();
		g_bridge_aborted = true;
	}
}

// Every trap and interrupt the core dispatches, with the state it dispatches from. The boot
// derail ends in real mode executing cleared memory, so what matters is the last few entries
// before that - P2K_TRAPTRACE=<n> prints the first n and then stops
static void p2k_trap_step(int vector)
{
	if (g_irq0_armed)
	{
		g_irq0_armed = false;
		if (g_tick_vector < 0) g_tick_vector = vector; // learned once
	}
	// every vector is pushed while tracking is running, so a nested handler's IRET pops its own
	// frame and not the clock handler's - that miscount is what used to release the gate early
	g_clkint_iret_seen = false;
	push_int_frame(vector == g_tick_vector);
	if (vector == g_tick_vector) g_p2k_irq_dispatched++;
#if P2K_DEBUG
	static int trace_left = []() -> int {
		const char *s = getenv("P2K_TRAPTRACE");
		return s ? (int)strtol(s, nullptr, 0) : 0;
	}();
	if (!g_backtrace_at && !g_backtrace_below)
	{
		char reason[32];
		snprintf(reason, sizeof(reason), "vector %02x", unsigned(vector));
		dump_backtrace(reason);
	}
	if (trace_left <= 0 || !g_bridge_cpu) return;
	trace_left--;
	fprintf(stderr, "[p2k trap] vector=%02x PC=%08x CS=%04x:%08x ESP=%08x CR0=%08x EFLAGS=%08x\n",
		unsigned(vector), unsigned(g_bridge_cpu->state_int(I386_PC)),
		unsigned(g_bridge_cpu->state_int(I386_CS)), unsigned(g_bridge_cpu->state_int(I386_EIP)),
		unsigned(g_bridge_cpu->state_int(I386_ESP)), unsigned(g_bridge_cpu->state_int(I386_CR0)),
		unsigned(g_bridge_cpu->state_int(I386_EFLAGS)));
	fflush(stderr);
#endif
}

#endif // P2K_DEBUG

void p2k_bridge_attach(p2k_state *state, mediagx_device *cpu)
{
	g_bridge_state = state;
	g_bridge_cpu = cpu;
	g_disasm.reset();
#if P2K_DEBUG
	pctrap_init();
	g_probes_armed = g_pctrap_n || g_hooktrace_left > 0 || g_dump_to || g_stack_at || g_stack_below || !g_backtrace.empty() || g_watch_to;
	arm_instruction_hook();
	p2k_exception_hook = p2k_trap_step;
#endif
}

extern "C" {

int mediagx_ICount = 0;

void mediagx_init(void) {}
void mediagx_reset(void *param) { if (g_bridge_cpu) g_bridge_cpu->p2k_reset(); }
void mediagx_exit(void) { g_disasm.reset(); }

int mediagx_execute(int cycles)
{
	g_p2k_slices++;
	if (!g_bridge_state) { mediagx_ICount = 0; return cycles; }

	// run in chunks so the debugger gets a look in between; the machine's own runner keeps the
	// CPU and the timer queue in step within each chunk
	constexpr int chunk = 2000;
	int consumed = 0;
	g_bridge_aborted = false;

	// Count the cycles still owed, and return cycles - mediagx_ICount: same as
	// other cores (m6809.c, adsp2100.c) and both halves have to move together,
	// because cpuexec.c derives the cycles run from the return value less cycles_stolen.
	// Needed for the PWM integrator timestamps!
	//
	// Decremented after each chunk, not before, so the clock reads the chunk's start and never runs ahead of the machine
	mediagx_ICount = cycles;
	while (mediagx_ICount > 0 && !g_bridge_aborted)
	{
		const int n = (mediagx_ICount > chunk) ? chunk : mediagx_ICount;
		g_bridge_state->run_cycles(u64(n));
		mediagx_ICount -= n;
		consumed += n;
		P2K_REMOTE_DEBUG_HOOK();
		if (mediagx_ICount < 0) g_bridge_aborted = true;
	}

	// Not cycles - mediagx_ICount: on the abort path that count is driven to -1 whatever was left,
	// so the return value overshoots by exactly what cpuexec.c deducts as cycles_stolen. Only this running total is the cycles actually run
	p2k_report_progress(u64(consumed));
	return cycles - mediagx_ICount;
}

// Context switching is not modelled - there is exactly one MediaGX in the machine, and its
// state lives in the device object. PinMAME still needs a non-zero size here: cpuintrf_init_cpu
// treats a context size of 0 as an error ("claims to need no context buffer"), refuses the CPU
// and leaves totalcpu at zero, so nothing ever runs it
unsigned mediagx_get_context(void *dst) { return sizeof(void *); }
void mediagx_set_context(void *src) {}

unsigned mediagx_get_reg(int regnum)
{
	if (!g_bridge_cpu) return 0;
	switch (regnum)
	{
		case P2K_REG_PC: return unsigned(g_bridge_cpu->state_int(I386_PC));
		case P2K_REG_SP: return unsigned(g_bridge_cpu->state_int(I386_ESP));
		default:         return unsigned(g_bridge_cpu->state_int(regnum));
	}
}

void mediagx_set_reg(int regnum, unsigned val)
{
	if (!g_bridge_cpu) return;
	switch (regnum)
	{
		case P2K_REG_PC: g_bridge_cpu->set_state_int(I386_EIP, val); break;
		case P2K_REG_SP: g_bridge_cpu->set_state_int(I386_ESP, val); break;
		default:         g_bridge_cpu->set_state_int(regnum, val);   break;
	}
}

void mediagx_set_irq_line(int irqline, int linestate)
{
	if (g_bridge_cpu) g_bridge_cpu->set_input_line(irqline, linestate);
}

// Deliberately empty. PinMAME installs its own acknowledgement callback on every CPU at reset
// (cpuint.c's cpu_N_irq_callback), which answers with the driver's default vector - zero for
// this table row. This machine has an 8259 pair and the interrupt vector comes from there:
// p2k_state wires the CPU's acknowledge to pic8259_device::acknowledge() when it builds the
// machine. Accepting PinMAME's callback overwrote that wiring, so the first hardware interrupt
// dispatched through vector 0 and the boot code vanished into cleared memory - the CPU was seen
// marching up through RAM in protected mode with an empty serial console
void mediagx_set_irq_callback(int (*callback)(int irqline)) {}

const char *mediagx_info(void *context, int regnum)
{
	static char buffer[64];

	// These are asked for during cpuintrf_init_cpu(), before the machine exists, so they must
	// not depend on it - PinMAME builds the CPU's family name from them
	switch (regnum)
	{
		case P2K_CPU_INFO_NAME:    return "MediaGX";
		case P2K_CPU_INFO_FAMILY:  return "Cyrix x86";
		case P2K_CPU_INFO_VERSION: return "1.0";
		case P2K_CPU_INFO_FILE:    return __FILE__;
		case P2K_CPU_INFO_CREDITS: return "MAME i386 core (Ville Linde, Barry Rodewald, Carl, Philip Bennett)";
		default:
		{
			if (!g_bridge_cpu) return "";
			std::string s = g_bridge_cpu->state_string(regnum);
			snprintf(buffer, sizeof(buffer), "%s", s.c_str());
			return buffer;
		}
	}
}

// Note: the disassembler asks the CPU for its current mode (i386_disassembler::config::
// get_mode()), so it decodes at whatever width the machine is running in right now. Listing
// real-mode code while the CPU sits in protected mode therefore produces nonsense - forcing the
// mode is a separate knob the debugger will need
unsigned mediagx_dasm(char *buffer, unsigned pc)
{
	if (!g_bridge_cpu) { snprintf(buffer, 16, "???"); return 1; }
	if (!g_disasm) g_disasm = static_cast<device_disasm_interface *>(g_bridge_cpu)->p2k_disassembler();

	p2k_opcode_buffer opcodes;
	std::ostringstream stream;
	offs_t result = g_disasm->disassemble(stream, pc, opcodes, opcodes);
	snprintf(buffer, 128, "%s", stream.str().c_str());
	return unsigned(result & 0x0000ffff);
}

} // extern "C"

// the machine's own diagnostics ask for the CPU's whereabouts; only the bridge has it
unsigned p2k_bridge_pc()
{
	return g_bridge_cpu ? unsigned(g_bridge_cpu->state_int(I386_PC)) : 0u;
}
