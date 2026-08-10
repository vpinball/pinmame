// license:BSD-3-Clause

// PinMAME P2K subsystem - bridge between the imported MediaGX core and PinMAME's cpuintrf.
//
// PinMAME addresses CPUs through a table of plain C function pointers (src/cpuintrf.c), and
// everything built on top of it - the debugger's activecpu_get_reg/activecpu_dasm among them -
// goes through that table. The MediaGX lives in this subsystem with its own bus and its own
// device model, so it needs an adapter to appear there. This is that adapter.
//
// It is deliberately thin: the machine itself is still assembled by p2k_state, and this only
// forwards. What it makes possible is inspection - registers and disassembly - which is what
// bring-up needs next

#include "p2k_driver.h"
#include "p2k_debug.h"
#include "p2k_weak.h"
#include "i386.h"
#include "i386priv.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Mirrored from PinMAME's src/cpuintrf.h. The subsystem deliberately does not include PinMAME
// headers - they belong to the other half of the build - so the handful of constants the table
// interface uses are restated here. Keep in sync if cpuintrf.h ever renumbers them.
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
	u8  r8 (offs_t pc) const override { return u8(read(pc, 1)); }
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
// built on, but instructions per second is the number to compare an interpreter against.
u64 g_p2k_instr_total = 0;

// PinMAME time slices, i.e. calls to mediagx_execute. Slices per emulated second is a number
// worth watching: anything that makes the scheduler split intervals shows up here first, and
// each slice carries a pass over the machine's timer queue.
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
		return s ? strtoull(s, nullptr, 0) : 0;
	}();
	if (!interval || !g_bridge_state || !g_bridge_cpu) return;

	static u64 elapsed = 0, next = interval;
	static size_t console_seen = 0;
	elapsed += cycles;
	if (elapsed < next) return;
	next = elapsed + interval;

	const std::string &console = g_bridge_state->console_log();
	if (console.size() > console_seen)
	{
		fprintf(stderr, "[p2k console] %s\n", console.c_str() + console_seen);
		console_seen = console.size();
	}
	fprintf(stderr, "[p2k %llu cycles] instr=%llu slices=%llu pit0=%llu picint=%llu irq=%llu in/out=%llu/%llu held=%llu PC=%08x EIP=%08x CS=%04x CSBASE=%08x ESP=%08x CR0=%08x\n",
		(unsigned long long)elapsed, (unsigned long long)g_p2k_instr_total,
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

// The P2K machine registers itself here so the bridge has something to talk to.
extern void (*p2k_instruction_hook)(unsigned pc);
extern void (*p2k_exception_hook)(int vector);
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
// mode changes and traps that happen on the way there must not consume the one dump.
static std::vector<unsigned> g_backtrace = []() {
	const char *s = getenv("P2K_BACKTRACE");
	long n = s ? strtol(s, nullptr, 0) : 0;
	return std::vector<unsigned>(n > 0 ? size_t(n) : 0);
}();
static const unsigned g_backtrace_at = []() {
	const char *s = getenv("P2K_BTAT");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();
// P2K_BTBELOW=<addr>: dump the ring the first time the PC drops below that address. A derail
// into low memory marches for a long time before it reaches any address one could name, so the
// useful trigger is the entry into the region, not a point inside it.
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
static long g_forward_left = 0;   // instructions still to follow after the mode change
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
// the machine's own bus, at a moment the caller chooses.
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
// n defaulting to 16. A fixed range cannot answer "who called this" - the stack moves.
static unsigned g_stack_at = 0;
static long g_stack_words = 16;
static const bool g_stack_init = []() {
	if (const char *s = getenv("P2K_STACKAT"))
	{
		char *end = nullptr;
		g_stack_at = unsigned(strtoul(s, &end, 16));
		if (end && *end == ':') g_stack_words = strtol(end + 1, nullptr, 0);
	}
	return true;
}();

// P2K_STACKBELOW=<esp>: dump the stack the first time ESP drops below that value. Runaway
// recursion is only readable at depth, and an address trigger fires on the shallow first call.
static const unsigned g_stack_below = []() {
	const char *s = getenv("P2K_STACKBELOW");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();

// P2K_STACKAFTER=<cycles>: hold the stack probes back until that many cycles have run. A hang
// looks the same on its first pass as on its millionth, but the call chain that matters is the
// one during the hang.
static const u64 g_stack_after = []() {
	const char *s = getenv("P2K_STACKAFTER");
	return s ? strtoull(s, nullptr, 0) : 0ull;
}();

static void dump_stack()
{
	if (!g_bridge_state || !g_bridge_cpu) return;
	const unsigned esp = unsigned(g_bridge_cpu->state_int(I386_ESP));
	fprintf(stderr, "[p2k stack] at pc %08x, ESP=%08x\n", g_stack_at, esp);
	for (long i = 0; i < g_stack_words; i++)
	{
		const offs_t a = esp + unsigned(i) * 4;
		fprintf(stderr, "  [esp+%02lx] %08x = %08x\n", i * 4, a, g_bridge_state->mem_r(a, 0xffffffff));
	}
	fflush(stderr);
}

// P2K_FIND=<lo>-<hi>: instead of a hex dump, report every dword in the P2K_DUMP range whose
// value falls between lo and hi. Finding which of 55 task stacks holds a return address into a
// given function is otherwise 55 runs.
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
// Pinball 2000's clock interrupt runs longer than a tick period on an emulated 20 MHz CPU: it
// services the pinball driver board one register at a time, and each access is bracketed by a
// critical section. Every time it leaves one, IF comes back and the tick that is already waiting
// is taken - measured, three times over, always at the same instruction (0x1debbd, right after
// the "leave critical section" call). XINU's resched() then sees its nesting counter non-zero,
// complains, and the complaint is itself a report: a feedback loop that overflows the task's
// 8 KB stack. Measured in detail in README.md.
//
// So the gate holds the **delivery**, not the edge. An earlier version suppressed the PIT's
// rising edge and changed nothing, for a good reason: the tick that nests was latched in the
// 8259's IRR before the handler started, so the request already exists and the PIC hands it over
// the moment IF returns. The edge is the wrong thing to block. What works is keeping the PIC's
// INT line away from the CPU while the guest is inside the handler, and letting it through again
// when the handler leaves - the PIC keeps its state either way.
//
// Both signals are generic - no game addresses:
//   entry: the CPU dispatches the tick vector. Which vector that is, is learned: the driver says
//          when it puts an edge on IR0, and the vector dispatched next is the tick's.
//   exit:  the handler executes IRET. The opcode is only inspected while inside the handler.
//
// It is bounded, and it has to be: XINU's clock handler does not always return by IRET - when it
// reschedules it switches tasks and the frame is popped later, in another context. So after
// four PIT edges with delivery held, the gate gives up and opens.
//
// The gate itself is not optional - the machine does not survive without it. What is optional is
// being able to argue with it, which is a debugging need: P2K_CLKINT_GATE=0 turns it off,
// P2K_CLKINT_MAX_SKIP=<n> moves the bound, and P2K_CLKINT_COUNTER=<addr> is an experiment that
// gates on the firmware's own nesting counter instead of the IRET heuristic. Without P2K_DEBUG
// these are the constants they default to, so the compiler drops the counter branch entirely
#if P2K_DEBUG
static const bool g_clkint_gate = []() {
	const char *s = getenv("P2K_CLKINT_GATE");
	return !s || strtol(s, nullptr, 0) != 0;
}();
static const long g_max_skips = []() {
	const char *s = getenv("P2K_CLKINT_MAX_SKIP");
	const long n = s ? strtol(s, nullptr, 0) : 4;
	return n > 0 ? n : 1;
}();
static const unsigned g_clkint_counter = []() {
	const char *s = getenv("P2K_CLKINT_COUNTER");
	return s ? unsigned(strtoul(s, nullptr, 16)) : 0u;
}();
#else
static const bool     g_clkint_gate    = true;
static const long     g_max_skips      = 4;
static const unsigned g_clkint_counter = 0;
#endif
static bool g_in_clkint = false;
static bool g_clkint_iret_seen = false;
static bool g_irq0_armed = false;      // an edge reached the PIC, no dispatch yet
static int g_tick_vector = -1;         // learned from the first dispatch after an edge
static long g_edges_while_held = 0;
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
	return g_in_clkint;
}

static void p2k_debug_step(unsigned pc); // defined below, installed by arm_instruction_hook()

// The per-instruction hook is the one thing in the bridge that EVERY emulated instruction pays
// for, so it is only installed while it has work to do. debugger_instruction_hook() is a null test
// on the pointer (see shim/debugger.h), so leaving it null costs the core a load and a perfectly
// predicted branch, and saves the non-inlinable indirect call plus the two global tests inside.
//
// It has work to do while g_in_clkint is set - that is the state whose exit it watches for, by
// looking for the handler's IRET. Outside that state p2k_debug_step() only bumps
// g_p2k_instr_total, and the sole reader of that counter is report_progress(), which is
// `inline void report_progress(u64) {}` unless P2K_DEBUG. So in a normal build skipping the call
// is not merely cheap, it is unobservable. In a P2K_DEBUG build the hook stays installed
// unconditionally, because there the counter is live.
//
// This is safe only because g_in_clkint has exactly one assignment site, right below.
static inline void arm_instruction_hook()
{
#if P2K_DEBUG
	p2k_instruction_hook = p2k_debug_step;
#else
	p2k_instruction_hook = g_in_clkint ? p2k_debug_step : nullptr;
#endif
}

static void set_in_clkint(bool inside)
{
	if (g_in_clkint == inside) return;
	g_in_clkint = inside;
	arm_instruction_hook();
	if (inside) { g_p2k_clkint_entered++; g_edges_while_held = 0; }
	else        { g_p2k_clkint_left++; }
	p2k_apply_irq0(); // the line may be free now, or has to go away
}

// called from the driver for every rising edge the PIT puts on IR0. The edge always reaches the
// PIC now - this only bounds how long the gate may hold delivery.
void p2k_clkint_note_edge()
{
	if (!g_clkint_gate) { g_irq0_armed = true; return; }
	if (p2k_clkint_blocks_irq())
	{
		g_p2k_tick_held++;
		if (++g_edges_while_held >= g_max_skips) set_in_clkint(false);
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
// P2K_PCTRAP_MAX caps the reports per address (default 8) so a trap inside a loop cannot flood.
namespace {

struct pc_trap { unsigned addr; char label[24]; unsigned hits; };
pc_trap  g_pctrap[16];
unsigned g_pctrap_n   = 0;
unsigned g_pctrap_lo  = ~0u;
unsigned g_pctrap_hi  = 0;
u64      g_pctrap_bits = 0;          // (addr & 63) presence, so the common case costs one test
unsigned g_pctrap_max = 8;

void pctrap_init()
{
	const char *s = getenv("P2K_PCTRAP");
	if (!s) return;
	if (const char *m = getenv("P2K_PCTRAP_MAX")) g_pctrap_max = unsigned(strtoul(m, nullptr, 0));
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
		fprintf(stderr, "[p2k pctrap] %u address(es) armed, max %u report(s) each\n",
			g_pctrap_n, g_pctrap_max);
}

inline void pctrap_check(unsigned pc)
{
	if (!g_pctrap_n || pc < g_pctrap_lo || pc > g_pctrap_hi) return;
	if (!((g_pctrap_bits >> (pc & 63)) & 1)) return;
	for (unsigned i = 0; i < g_pctrap_n; i++)
	{
		pc_trap &t = g_pctrap[i];
		if (t.addr != pc) continue;
		if (t.hits++ >= g_pctrap_max) return;
		/* EBX/ECX/EDX come along because the interesting value is not always the return
		   value: AProc::existc_range, for instance, carries the process id it matched in EBX,
		   and that id is what names the device. */
		unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
		if (g_bridge_cpu)
		{
			eax = unsigned(g_bridge_cpu->state_int(I386_EAX));
			ebx = unsigned(g_bridge_cpu->state_int(I386_EBX));
			ecx = unsigned(g_bridge_cpu->state_int(I386_ECX));
			edx = unsigned(g_bridge_cpu->state_int(I386_EDX));
		}
		fprintf(stderr, "[p2k pc] %08x %-20s eax=%08x ebx=%08x ecx=%08x edx=%08x  hit %u\n",
			pc, t.label[0] ? t.label : "-", eax, ebx, ecx, edx, t.hits);
		fflush(stderr);
		return;
	}
}

} // namespace
#endif // P2K_DEBUG

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
// falling back to the bus only when the PC is somewhere else.
#if P2K_DEBUG
static long g_hooktrace_left = []() -> long {
	const char *s = getenv("P2K_HOOKTRACE");
	return s ? strtol(s, nullptr, 0) : 0;
}();
static bool g_probes_armed = false;   // set in p2k_bridge_attach, once everything is parsed
#endif

static inline u8 p2k_peek_byte(unsigned a)
{
	if (const u8 *p = g_bridge_state->ram_peek(a)) return *p;
	const unsigned shift = (a & 3) * 8;
	return u8(g_bridge_state->mem_r(a & ~3u, 0xffu << shift) >> shift);
}

static void p2k_debug_step(unsigned pc)
{
	g_p2k_instr_total++;

	// the handler's IRET ends it. The hook runs before the instruction, so the flag clears one
	// instruction later - the point is only that no tick is delivered in between.
	if (g_in_clkint && g_bridge_state)
	{
		if (g_clkint_iret_seen) { g_clkint_iret_seen = false; set_in_clkint(false); }
		else
		{
			const u8 op = p2k_peek_byte(pc);
			if (op == 0xcf || (op == 0x66 && p2k_peek_byte(pc + 1) == 0xcf)) g_clkint_iret_seen = true;
		}
	}

	// Everything from here to the closing brace is a probe: off in a normal run, and the whole
	// chain is skipped in one test. What comes *after* the block is not optional - see there.
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
		// arm only once the machine has run with a stack above the mark - at reset ESP is 0 and
		// every value is "below"
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
		// The mode change is too late to show who called the mode-switch service; this is not.
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
		// register list, which is too much to do per instruction.
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
			g_forward_left = f ? strtol(f, nullptr, 0) : 40;
		}
		was_protected = protected_now;

		g_backtrace[g_backtrace_pos % g_backtrace.size()] = pc;
		g_backtrace_pos++;
	}
	// P2K_WATCH=<from>[-<to>]: a full register line for every instruction in that range. The
	// debugger's state endpoint reports only PC and SP for this CPU, and stepping through a
	// handful of instructions is exactly what a derail like this needs.
	if (g_forward_left > 0) g_forward_left--;
	if (((g_watch_to && pc >= g_watch_from && pc <= g_watch_to) || g_forward_left > 0) && g_bridge_cpu)
	{
		// the bytes at the PC, read the same way the core fetches them and at the same instant.
		// Reading them later through the debugger answers about memory as it is then, which is a
		// different question whenever the code region is being written to.
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
	// machine's timing shifted by two frames - that is how much they matter.
	P2K_REMOTE_DEBUG_HOOK();

	// A breakpoint hit ends the time slice through activecpu_abort_timeslice(), which subtracts
	// the cycles that are left from the CPU's icount - for this bridge that means mediagx_ICount
	// goes negative. Pass that on, or the machine would run to the end of the slice and report a
	// PC far past the breakpoint.
	if (mediagx_ICount < 0 && g_bridge_cpu)
	{
		g_bridge_cpu->abort_timeslice();
		g_bridge_aborted = true;
	}
}

// Every trap and interrupt the core dispatches, with the state it dispatches from. The boot
// derail ends in real mode executing cleared memory, so what matters is the last few entries
// before that - P2K_TRAPTRACE=<n> prints the first n and then stops.
static void p2k_trap_step(int vector)
{
	if (g_irq0_armed)
	{
		g_irq0_armed = false;
		if (g_tick_vector < 0) g_tick_vector = vector;      // learned once
	}
	if (vector == g_tick_vector)
	{
		g_clkint_iret_seen = false;
		set_in_clkint(true);
		g_p2k_irq_dispatched++;
	}
#if P2K_DEBUG
	static long trace_left = []() -> long {
		const char *s = getenv("P2K_TRAPTRACE");
		return s ? strtol(s, nullptr, 0) : 0;
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
#endif // P2K_DEBUG
}

void p2k_bridge_attach(p2k_state *state, mediagx_device *cpu)
{
	g_bridge_state = state;
	g_bridge_cpu = cpu;
	g_disasm.reset();
#if P2K_DEBUG
	pctrap_init();
	g_probes_armed = g_pctrap_n || g_hooktrace_left > 0 || g_dump_to || g_stack_at
		|| g_stack_below || !g_backtrace.empty() || g_watch_to;
#endif
	arm_instruction_hook(); // null outside clkint in a normal build - see there
	p2k_exception_hook = p2k_trap_step;
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
	const int chunk = 2000;
	int left = cycles;
	g_bridge_aborted = false;
	mediagx_ICount = 0;
	while (left > 0 && !g_bridge_aborted)
	{
		int n = (left > chunk) ? chunk : left;
		g_bridge_state->run_cycles(u64(n));
		left -= n;
		P2K_REMOTE_DEBUG_HOOK();
		if (mediagx_ICount < 0) g_bridge_aborted = true;
	}

	p2k_report_progress(u64(cycles - left));
	mediagx_ICount = 0;
	return cycles - left;
}

// Context switching is not modelled - there is exactly one MediaGX in the machine, and its
// state lives in the device object. PinMAME still needs a non-zero size here: cpuintrf_init_cpu
// treats a context size of 0 as an error ("claims to need no context buffer"), refuses the CPU
// and leaves totalcpu at zero, so nothing ever runs it.
unsigned mediagx_get_context(void *dst) { return sizeof(void *); }
void mediagx_set_context(void *src) {}

unsigned mediagx_get_reg(int regnum)
{
	if (!g_bridge_cpu) return 0;
	switch (regnum)
	{
		case P2K_REG_PC:  return unsigned(g_bridge_cpu->state_int(I386_PC));
		case P2K_REG_SP:  return unsigned(g_bridge_cpu->state_int(I386_ESP));
		default:      return unsigned(g_bridge_cpu->state_int(regnum));
	}
}

void mediagx_set_reg(int regnum, unsigned val)
{
	if (!g_bridge_cpu) return;
	switch (regnum)
	{
		case P2K_REG_PC:  g_bridge_cpu->set_state_int(I386_EIP, val); break;
		case P2K_REG_SP:  g_bridge_cpu->set_state_int(I386_ESP, val); break;
		default:      g_bridge_cpu->set_state_int(regnum, val); break;
	}
}

void mediagx_set_irq_line(int irqline, int linestate)
{
	if (g_bridge_cpu) g_bridge_cpu->set_input_line(irqline, linestate);
}

// Deliberately empty. PinMAME installs its own acknowledge callback on every CPU at reset
// (cpuint.c's cpu_N_irq_callback), which answers with the driver's default vector - zero for
// this table row. This machine has an 8259 pair and the interrupt vector comes from there:
// p2k_state wires the CPU's acknowledge to pic8259_device::acknowledge() when it builds the
// machine. Accepting PinMAME's callback overwrote that wiring, so the first hardware interrupt
// dispatched through vector 0 and the boot code vanished into cleared memory - the CPU was seen
// marching up through RAM in protected mode with an empty serial console.
void mediagx_set_irq_callback(int (*callback)(int irqline)) {}

const char *mediagx_info(void *context, int regnum)
{
	static char buffer[64];

	// These are asked for during cpuintrf_init_cpu(), before the machine exists, so they must
	// not depend on it - PinMAME builds the CPU's family name from them.
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
// mode is a separate knob the debugger will need.
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
