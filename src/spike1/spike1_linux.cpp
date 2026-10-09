// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - Linux, emulated at the system-call level. See spike1_linux.h

#include "spike1_linux.h"
#include "spike1_cpu.h"

#include <algorithm>
#include <deque>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

// Linux errno values (asm-generic)
enum : int32_t {
	E_PERM = 1, E_NOENT = 2, E_INTR = 4, E_IO = 5, E_BADF = 9, E_AGAIN = 11, E_NOMEM = 12,
	E_ACCES = 13, E_FAULT = 14, E_EXIST = 17, E_NOTDIR = 20, E_ISDIR = 21, E_INVAL = 22,
	E_SRCH = 3, E_MFILE = 24, E_NOTTY = 25, E_SPIPE = 29, E_ROFS = 30, E_RANGE = 34, E_NOSYS = 38,
	E_NOTEMPTY = 39, E_AFNOSUPPORT = 97, E_TIMEDOUT = 110
};

// ARM open() flags
enum : uint32_t {
	O_ACCMODE_ = 3, O_WRONLY_ = 1, O_RDWR_ = 2, O_CREAT_ = 0x40, O_EXCL_ = 0x80, O_TRUNC_ = 0x200,
	O_APPEND_ = 0x400, O_NONBLOCK_ = 0x800, O_DIRECTORY_ = 0x4000
};

enum : uint32_t { S_IFDIR_ = 0040000, S_IFCHR_ = 0020000, S_IFREG_ = 0100000, S_IFLNK_ = 0120000 };

constexpr int32_t AT_FDCWD_ = -100;
// a thread waiting for input looks again this often: input has no wake-up of its own yet
constexpr uint64_t POLL_RECHECK_NS = 1000000;
constexpr uint32_t PROGRAM_HWCAP = 0x97;        // swp, half, thumb, fast mult, edsp: ARMv5TE, no VFP

struct syscall_name { uint32_t nr; const char *name; };
const syscall_name SYSCALL_NAMES[] = {
	{1,"exit"},{3,"read"},{4,"write"},{5,"open"},{6,"close"},{10,"unlink"},{12,"chdir"},{13,"time"},
	{15,"chmod"},{19,"lseek"},{20,"getpid"},{33,"access"},{36,"sync"},{37,"kill"},{38,"rename"},
	{39,"mkdir"},{40,"rmdir"},{41,"dup"},{42,"pipe"},{43,"times"},{45,"brk"},{54,"ioctl"},{55,"fcntl"},
	{57,"setpgid"},{60,"umask"},{63,"dup2"},{64,"getppid"},{66,"setsid"},{77,"getrusage"},
	{78,"gettimeofday"},{85,"readlink"},{91,"munmap"},{93,"ftruncate"},{96,"getpriority"},
	{97,"setpriority"},{106,"stat"},{108,"fstat"},{114,"wait4"},{116,"sysinfo"},{118,"fsync"},
	{120,"clone"},{122,"uname"},{125,"mprotect"},{140,"_llseek"},{141,"getdents"},{142,"_newselect"},
	{143,"flock"},{144,"msync"},{145,"readv"},{146,"writev"},{148,"fdatasync"},{150,"mlock"},
	{152,"mlockall"},{154,"sched_setparam"},{155,"sched_getparam"},{156,"sched_setscheduler"},
	{157,"sched_getscheduler"},{158,"sched_yield"},{159,"sched_get_priority_max"},
	{160,"sched_get_priority_min"},{162,"nanosleep"},{163,"mremap"},{168,"poll"},{172,"prctl"},
	{173,"rt_sigreturn"},{174,"rt_sigaction"},{175,"rt_sigprocmask"},{176,"rt_sigpending"},
	{177,"rt_sigtimedwait"},{179,"rt_sigsuspend"},{180,"pread64"},{181,"pwrite64"},{183,"getcwd"},
	{186,"sigaltstack"},{190,"vfork"},{191,"ugetrlimit"},{192,"mmap2"},{193,"truncate64"},
	{194,"ftruncate64"},{195,"stat64"},{196,"lstat64"},{197,"fstat64"},{199,"getuid32"},
	{200,"getgid32"},{201,"geteuid32"},{202,"getegid32"},{217,"getdents64"},{220,"madvise"},
	{221,"fcntl64"},{224,"gettid"},{238,"tkill"},{240,"futex"},{241,"sched_setaffinity"},
	{242,"sched_getaffinity"},{248,"exit_group"},{256,"set_tid_address"},{257,"timer_create"},
	{258,"timer_settime"},{262,"timer_delete"},{263,"clock_gettime"},{264,"clock_getres"},
	{265,"clock_nanosleep"},{268,"tgkill"},{281,"socket"},{282,"bind"},{283,"connect"},
	{322,"openat"},{327,"fstatat64"},{334,"faccessat"},{338,"set_robust_list"},{351,"eventfd"},
	{356,"eventfd2"},{359,"pipe2"},{373,"syncfs"},
	{0xf0002,"cacheflush"},{0xf0005,"set_tls"},
};

const char *name_of(uint32_t nr)
{
	for (const auto &s : SYSCALL_NAMES) if (s.nr == nr) return s.name;
	return nullptr;
}

std::string hex(uint32_t v) { char b[16]; std::snprintf(b, sizeof(b), "0x%x", v); return b; }

// 64-bit file positions and UTF-8 paths on every host
std::FILE *host_fopen(const std::string &utf8, bool write, bool truncate)
{
#if defined(_WIN32)
	return _wfopen(fs::u8path(utf8).c_str(), write ? (truncate ? L"w+b" : L"r+b") : L"rb");
#else
	return std::fopen(utf8.c_str(), write ? (truncate ? "w+b" : "r+b") : "rb");
#endif
}

int host_seek(std::FILE *fp, int64_t offset, int whence)
{
#if defined(_WIN32)
	return _fseeki64(fp, offset, whence);
#else
	return fseeko(fp, off_t(offset), whence);
#endif
}

int64_t host_tell(std::FILE *fp)
{
#if defined(_WIN32)
	return _ftelli64(fp);
#else
	return int64_t(ftello(fp));
#endif
}

uint64_t host_wall_ns()
{
	return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count());
}

// A printable excerpt of guest bytes for the log
std::string excerpt(const uint8_t *p, uint32_t len)
{
	std::string s;
	const uint32_t n = std::min<uint32_t>(len, 48);
	bool text = true;
	for (uint32_t i = 0; i < n; i++) if ((p[i] < 0x20 && p[i] != '\n' && p[i] != '\r' && p[i] != '\t') || p[i] > 0x7e) text = false;
	if (text) {
		s.push_back('"');
		for (uint32_t i = 0; i < n; i++) {
			if (p[i] == '\n') s += "\\n"; else if (p[i] == '\r') s += "\\r"; else if (p[i] == '\t') s += "\\t"; else s.push_back(char(p[i]));
		}
		s.push_back('"');
	} else {
		char b[4];
		for (uint32_t i = 0; i < n; i++) { std::snprintf(b, sizeof(b), "%02x", p[i]); s += b; }
	}
	if (len > n) s += "...";
	return s;
}

} // anonymous namespace

// ---------------------------------------------------------------- files

struct spike1_linux::file
{
	// host: a base file on the host; memory: a base file the host holds; written: one the game wrote
	enum class kind { host, memory, written, directory, device, null, zero, random, console, pipe_read, pipe_write } type = kind::host;
	std::string guest;            // the path the guest opened
	std::string host;             // the host path behind a host file
	std::FILE *fp = nullptr;
	const uint8_t *data = nullptr;  // a memory file
	size_t size = 0;
	std::shared_ptr<std::vector<uint8_t>> written;
	uint32_t flags = 0;
	// directories: entries, filled at open
	struct dirent { std::string name; uint8_t type; uint64_t inode; };
	std::vector<dirent> entries;
	// pipes: both ends share the buffer
	std::shared_ptr<std::deque<uint8_t>> pipe;
	// devices: what the device layer keeps per open file
	spike1_devices::handle dev;
	size_t position = 0;

	~file() { if (fp) std::fclose(fp); }
};

// ---------------------------------------------------------------- setup

spike1_linux::spike1_linux(spike1_cpu_device &cpu, spike1_memory &mem) : m_cpu(cpu), m_mem(mem), m_devices(mem)
{
}

spike1_linux::~spike1_linux() = default;

void spike1_linux::log(const std::string &line) const
{
	if (m_cfg.log) m_cfg.log(line);
}

void spike1_linux::stop(const std::string &reason)
{
	m_status = status::stopped;
	m_stop_reason = reason;
	m_cpu.end_slice();
	log("stop: " + reason);
}

uint64_t spike1_linux::now_ns() const
{
	// split so the product cannot overflow however long the machine runs
	return m_cycles / m_cfg.clock_hz * 1000000000ull + m_cycles % m_cfg.clock_hz * 1000000000ull / m_cfg.clock_hz;
}

size_t spike1_linux::thread_count() const
{
	size_t n = 0;
	for (const auto &t : m_threads) if (t->st != thread::state::done) n++;
	return n;
}

void spike1_linux::fault(uint32_t addr, bool write)
{
	if (m_status != status::running) return;
	stop(std::string("guest ") + (write ? "write to " : "read from ") + hex(addr) + " outside guest memory, thread " +
		std::to_string(m_current ? m_current->tid : 0) + " pc " + hex(m_cpu.pc()));
}

bool spike1_linux::load_elf(const std::vector<uint8_t> &elf, uint32_t &entry, uint32_t &phdr, uint32_t &phnum, std::string &error)
{
	auto u16 = [&](size_t o) { uint16_t v; std::memcpy(&v, &elf[o], 2); return v; };
	auto u32 = [&](size_t o) { uint32_t v; std::memcpy(&v, &elf[o], 4); return v; };
	if (elf.size() < 52 || std::memcmp(elf.data(), "\x7f" "ELF", 4) || elf[4] != 1 || elf[5] != 1 || u16(18) != 40) {
		error = "not a 32-bit little-endian ARM ELF"; return false;
	}
	if (u16(16) != 2) { error = "not a static executable (ET_EXEC)"; return false; }
	entry = u32(24);
	const uint32_t phoff = u32(28);
	const uint16_t phentsize = u16(42);
	phnum = u16(44);
	phdr = 0;
	uint32_t image_end = 0;
	for (uint32_t i = 0; i < phnum; i++) {
		const size_t o = phoff + size_t(i) * phentsize;
		if (o + 32 > elf.size()) { error = "truncated program headers"; return false; }
		const uint32_t type = u32(o), offset = u32(o + 4), vaddr = u32(o + 8), filesz = u32(o + 16), memsz = u32(o + 20);
		if (type == 3) { error = "dynamically linked programs (PT_INTERP) are not supported"; return false; }
		if (type != 1) continue;
		if (offset + uint64_t(filesz) > elf.size() || !m_mem.host(vaddr, memsz)) { error = "segment outside the image or guest memory"; return false; }
		uint8_t *dst = m_mem.host(vaddr, memsz);
		std::memcpy(dst, &elf[offset], filesz);
		std::memset(dst + filesz, 0, memsz - filesz);
		if (offset <= phoff && phoff < offset + filesz) phdr = vaddr + (phoff - offset);
		image_end = std::max(image_end, vaddr + memsz);
	}
	m_mem.set_brk_base(image_end);

	// The symbol table, when the program kept it: devices find the game's own tables by name
	m_symbols.clear();
	m_symbol_sizes.clear();
	const uint32_t shoff = u32(32);
	const uint16_t shentsize = u16(46), shnum = u16(48);
	for (uint32_t i = 0; i < shnum && shoff + uint64_t(i + 1) * shentsize <= elf.size(); i++) {
		const size_t o = shoff + size_t(i) * shentsize;
		if (u32(o + 4) != 2) continue; // SHT_SYMTAB
		const uint32_t symoff = u32(o + 16), symsize = u32(o + 20), link = u32(o + 24);
		if (link >= shnum) break;
		const uint32_t stroff = u32(shoff + size_t(link) * shentsize + 16);
		for (uint64_t s = symoff; s + 16 <= uint64_t(symoff) + symsize && s + 16 <= elf.size(); s += 16) {
			const uint32_t name = u32(size_t(s)), value = u32(size_t(s) + 4);
			const uint8_t type = elf[size_t(s) + 12] & 15;
			if (!value || (type != 1 && type != 2) || uint64_t(stroff) + name >= elf.size()) continue; // objects and functions
			m_symbols.emplace(reinterpret_cast<const char *>(&elf[stroff + name]), value);
			m_symbol_sizes.emplace(reinterpret_cast<const char *>(&elf[stroff + name]), u32(size_t(s) + 8));
		}
	}
	return true;
}

uint32_t spike1_linux::symbol_size(const std::string &name) const
{
	auto it = m_symbol_sizes.find(name);
	return it == m_symbol_sizes.end() ? 0 : it->second;
}

uint32_t spike1_linux::symbol(const std::string &name) const
{
	auto it = m_symbols.find(name);
	return it == m_symbols.end() ? 0 : it->second;
}

// The pre-ARMv6 kernel helpers (Documentation/arm/kernel_user_helpers.rst), as real code so no
// per-instruction hook is needed. get_tls reads the word the thread switch keeps up to date
void spike1_linux::write_kuser_page()
{
	auto w = [&](uint32_t a, uint32_t v) { m_mem.write32(a, v); };
	w(0xffff0fa0, 0xe1a0f00e);                   // __kuser_memory_barrier: mov pc, lr
	const uint32_t cmpxchg[] = {                 // __kuser_cmpxchg: r0 old, r1 new, r2 ptr; r0 == 0 and C set on success
		0xe5923000,                              // ldr   r3, [r2]
		0xe0533000,                              // subs  r3, r3, r0
		0x05821000,                              // streq r1, [r2]
		0xe2730000,                              // rsbs  r0, r3, #0
		0xe1a0f00e };                            // mov   pc, lr
	for (int i = 0; i < 5; i++) w(0xffff0fc0 + 4 * i, cmpxchg[i]);
	w(0xffff0fe0, 0xe59f0008);                   // __kuser_get_tls: ldr r0, [pc, #8] (the word at 0xffff0ff0)
	w(0xffff0fe4, 0xe1a0f00e);                   //                  mov pc, lr
	w(0xffff0ff0, 0);
	w(0xffff0ffc, 2);                            // __kuser_helper_version: the three helpers above
}

bool spike1_linux::build_stack(uint32_t entry, uint32_t phdr, uint32_t phnum, uint32_t &sp, std::string &error)
{
	uint32_t cursor = spike1_memory::STACK_TOP;
	auto push_bytes = [&](const void *p, uint32_t n) { cursor -= n; return m_mem.write(cursor, p, n) ? cursor : 0u; };
	auto push_string = [&](const std::string &s) { return push_bytes(s.c_str(), uint32_t(s.size() + 1)); };

	const uint32_t execfn = push_string(m_cfg.executable);
	std::vector<uint32_t> env;
	for (const auto &e : m_cfg.environment) env.push_back(push_string(e));
	const uint32_t argv0 = push_string(m_cfg.executable);
	const uint32_t platform = push_string("v5l");
	static const uint8_t random_bytes[16] = { 0x3c, 0x91, 0x07, 0xe2, 0x5a, 0x6d, 0xb4, 0x18, 0xc7, 0x2f, 0x80, 0x4e, 0x93, 0x15, 0xa9, 0x66 };
	const uint32_t random = push_bytes(random_bytes, 16);
	cursor &= ~15u;

	std::vector<uint32_t> v;
	v.push_back(1);                              // argc
	v.push_back(argv0); v.push_back(0);
	for (uint32_t e : env) v.push_back(e);
	v.push_back(0);
	const uint32_t aux[][2] = {
		{3, phdr}, {4, 32}, {5, phnum}, {6, spike1_memory::PAGE_SIZE}, {7, 0}, {8, 0}, {9, entry},
		{11, 0}, {12, 0}, {13, 0}, {14, 0}, {23, 0}, {17, 100}, {16, PROGRAM_HWCAP}, {26, 0},
		{25, random}, {15, platform}, {31, execfn}, {0, 0} };
	for (const auto &a : aux) { v.push_back(a[0]); v.push_back(a[1]); }
	cursor -= uint32_t(v.size() * 4);
	cursor &= ~7u;
	if (!m_mem.write(cursor, v.data(), uint32_t(v.size() * 4))) { error = "initial stack does not fit"; return false; }
	sp = cursor;
	return true;
}

bool spike1_linux::start(const config &cfg, std::string &error)
{
	m_cfg = cfg;
	if (!m_cfg.clock_hz) m_cfg.clock_hz = 400000000;
	m_vfs = spike1_vfs();
	m_vfs.set_root(m_cfg.root);
	// the board's writable mounts, empty on the title's image: the game keeps its NVRAM under /data
	m_vfs.add_directory("/data");
	m_vfs.add_directory("/tmp");
	for (const auto &f : m_cfg.files) m_vfs.add_file(f.path, f.data, f.size);
	std::vector<uint8_t> elf;
	if (!read_whole(m_cfg.executable, elf)) { error = "game program not found: " + m_cfg.executable; return false; }
	uint32_t entry, phdr, phnum, sp;
	if (!load_elf(elf, entry, phdr, phnum, error) || !build_stack(entry, phdr, phnum, sp, error))
		return false;
	write_kuser_page();
	const std::string game_dir = m_cfg.executable.substr(0, m_cfg.executable.rfind('/'));
	spike1_devices::config dev;
	dev.trace = m_cfg.trace;
	dev.log = m_cfg.log;
	dev.symbol = [this](const std::string &name) { return symbol(name); };
	dev.symbol_size = [this](const std::string &name) { return symbol_size(name); };
	dev.game_files = [this, game_dir]() {
		std::vector<std::string> names;
		for (const auto &e : m_vfs.list(game_dir)) if (!e.directory) names.push_back(e.name);
		return names;
	};
	dev.read_game_file = [this, game_dir](const std::string &name, std::vector<uint8_t> &out) { return read_whole(game_dir + "/" + name, out); };
	m_devices.start(dev);
	if (!m_cfg.nvram.empty() && !set_nvram(m_cfg.nvram)) log("nvram: the saved block is not a Spike 1 NVRAM block; starting with a new machine");

	m_fds.clear();
	for (int fd = 0; fd < 3; fd++) {
		auto f = std::make_shared<file>();
		f->type = file::kind::console;
		f->guest = fd == 0 ? "stdin" : fd == 1 ? "stdout" : "stderr";
		f->flags = fd == 0 ? 0 : O_WRONLY_;
		m_fds[fd] = f;
	}
	m_cwd = "/";
	m_wall_base_ns = host_wall_ns();
	m_cycles = m_idle_cycles = 0;

	m_threads.clear();
	auto main_thread = std::make_unique<thread>();
	main_thread->tid = 100;
	main_thread->name = "main";
	main_thread->ctx.r[13] = sp;
	main_thread->ctx.r[15] = entry & ~1u;
	main_thread->ctx.cpsr = (entry & 1) ? 0x20 : 0;
	m_current = main_thread.get();
	m_threads.push_back(std::move(main_thread));
	m_next_tid = 101;
	m_cpu.enter_user_mode(m_current->ctx);
	m_cpu.set_swi([this](uint32_t comment) { on_swi(comment); });
	m_status = status::running;
	m_stop_reason.clear();
	log("start: " + m_cfg.executable + " entry " + hex(entry) + " sp " + hex(sp) + " brk " + hex(m_mem.brk(0)));
	return true;
}

bool spike1_linux::power_down(uint64_t max_ns, int slice_cycles)
{
	const uint64_t start = now_ns();
	const uint32_t syncs = m_syncs;
	m_devices.power_off();
	while (m_status == status::running && m_syncs == syncs && now_ns() - start < max_ns) run(slice_cycles);
	const bool synced = m_syncs != syncs;
	log("nvram: power down - " + std::string(synced ? "the game committed its NVRAM after " : "no NVRAM commit within ") +
		std::to_string((now_ns() - start) / 1000000) + " ms");
	return synced;
}

spike1_linux::status spike1_linux::run(int cycles)
{
	// a thread keeps the CPU for at most this long before the next runnable one gets it
	constexpr int QUANTUM = 200000;
	int64_t budget = cycles;
	while (budget > 0 && m_status == status::running) {
		wake_due();
		thread *t = pick_next();
		if (!t) {
			// every thread is blocked: let time pass up to the next wake-up without running anything
			const uint64_t next = next_wake_ns();
			if (!next) { stop("deadlock: every thread waits with no timeout"); break; }
			const uint64_t now = now_ns();
			uint64_t idle = next > now ? uint64_t(double(next - now) * m_cfg.clock_hz / 1e9) + 1 : 1;
			if (idle > uint64_t(budget)) idle = uint64_t(budget);
			m_cycles += idle;
			m_idle_cycles += idle;
			budget -= int64_t(idle);
			continue;
		}
		switch_to(*t);
		int used = m_cpu.run(int(std::min<int64_t>(budget, QUANTUM)));
		if (used < 1) used = 1;
		m_cycles += uint64_t(used);
		budget -= used;
		if (t->st != thread::state::done) {
			m_cpu.save_context(t->ctx);
			// preempted between __kuser_cmpxchg's load and its store: start the helper over, as the
			// kernel does on pre-v6 CPUs, so another thread's write in between is not lost
			if (t->ctx.r[15] - 0xffff0fc0u < 0xcu) t->ctx.r[15] = 0xffff0fc0u;
		}
	}
	return m_status;
}

// Round robin, starting after the thread that ran last
spike1_linux::thread *spike1_linux::pick_next()
{
	const size_t n = m_threads.size();
	size_t first = 0;
	for (size_t i = 0; i < n; i++)
		if (m_threads[i].get() == m_current) { first = i + 1; break; }
	for (size_t k = 0; k < n; k++) {
		thread *t = m_threads[(first + k) % n].get();
		if (t->st == thread::state::runnable) return t;
	}
	return nullptr;
}

void spike1_linux::switch_to(thread &t)
{
	m_current = &t;
	m_cpu.load_context(t.ctx);
	m_mem.write32(0xffff0ff0, t.tls); // what __kuser_get_tls returns
	deliver_signal();
}

// Runs the handler of the current thread's lowest pending, unmasked signal, the way ARM Linux does:
// a frame below the stack with a siginfo, a ucontext holding the interrupted registers and a return
// trampoline (mov r7, #173; svc 0 - rt_sigreturn) for a handler without SA_RESTORER; r0 = the
// signal, r1 = the siginfo, r2 = the ucontext. The interrupted registers also stay on the host side,
// which is where rt_sigreturn takes them from
void spike1_linux::deliver_signal()
{
	thread &t = *m_current;
	const uint64_t ready = t.sig_pending & ~t.sigmask;
	if (!ready || t.st != thread::state::runnable) return;
	uint32_t sig = 1;
	while (!((ready >> (sig - 1)) & 1)) sig++;
	t.sig_pending &= ~(1ull << (sig - 1));
	const sigaction_entry sa = m_sigactions[sig];
	if (sa.handler <= 1) return; // default or ignored: nothing runs
	constexpr uint32_t SA_RESTORER = 0x04000000, SA_NODEFER = 0x40000000, SA_RESETHAND = 0x80000000;
	constexpr uint32_t SIGINFO = 128, UCONTEXT = 512, TRAMPOLINE = 8, CPSR_T = 0x20;
	spike1_cpu_device::context ctx;
	m_cpu.save_context(ctx);
	t.sig_frames.push_back({ ctx, t.sigmask });
	const uint32_t sp = (ctx.r[13] - SIGINFO - UCONTEXT - TRAMPOLINE) & ~7u;
	const uint32_t info = sp, uc = sp + SIGINFO, tramp = uc + UCONTEXT;
	const uint8_t zero[SIGINFO + UCONTEXT] = {};
	m_mem.write(sp, zero, sizeof(zero));
	m_mem.write32(info, sig);                              // si_signo; si_errno and si_code (SI_USER) 0
	m_mem.write32(uc + 20 + 8, uint32_t(t.sigmask));       // uc_mcontext.oldmask
	for (int i = 0; i < 16; i++) m_mem.write32(uc + 20 + 12 + 4 * i, ctx.r[i]); // arm_r0 .. arm_pc
	m_mem.write32(uc + 20 + 76, ctx.cpsr);                 // arm_cpsr
	m_mem.write64(uc + 104, t.sigmask);                    // uc_sigmask
	m_mem.write32(tramp, 0xe3a070ad);
	m_mem.write32(tramp + 4, 0xef000000);
	spike1_cpu_device::context h = ctx;
	h.r[0] = sig;
	h.r[1] = info;
	h.r[2] = uc;
	h.r[13] = sp;
	h.r[14] = (sa.flags & SA_RESTORER) && sa.restorer ? sa.restorer : tramp;
	h.r[15] = sa.handler & ~1u;
	h.cpsr = (sa.handler & 1) ? (ctx.cpsr | CPSR_T) : (ctx.cpsr & ~CPSR_T);
	m_cpu.load_context(h);
	t.sigmask |= sa.mask | ((sa.flags & SA_NODEFER) ? 0 : 1ull << (sig - 1));
	if (sa.flags & SA_RESETHAND) m_sigactions[sig].handler = 0;
	log("signal " + std::to_string(sig) + " handled by thread " + std::to_string(t.tid) + " (" + t.name + ")");
}

void spike1_linux::block(uint64_t wake_ns, bool retry, int32_t timeout_result)
{
	thread &t = *m_current;
	t.st = thread::state::blocked;
	t.wake_ns = wake_ns;
	t.retry = retry;
	t.timeout_result = timeout_result;
	m_block = retry ? block_kind::retry : block_kind::result_later;
	m_cpu.end_slice();
}

void spike1_linux::wake(thread &t, int32_t result)
{
	if (!t.retry) t.ctx.r[0] = uint32_t(result);
	t.st = thread::state::runnable;
	t.wake_ns = 0;
	t.futex_addr = 0;
}

void spike1_linux::wake_due()
{
	const uint64_t now = now_ns();
	for (auto &t : m_threads)
		if (t->st == thread::state::blocked && t->wake_ns && t->wake_ns <= now)
			wake(*t, t->timeout_result);
}

uint64_t spike1_linux::next_wake_ns() const
{
	uint64_t next = 0;
	for (const auto &t : m_threads)
		if (t->st == thread::state::blocked && t->wake_ns && (!next || t->wake_ns < next))
			next = t->wake_ns;
	return next;
}

int spike1_linux::futex_wake(uint32_t addr, uint32_t count, uint32_t bitset)
{
	int woken = 0;
	for (auto &t : m_threads) {
		if (uint32_t(woken) >= count) break;
		if (t->st == thread::state::blocked && t->futex_addr == addr && (t->futex_bitset & bitset)) {
			wake(*t, 0);
			woken++;
		}
	}
	return woken;
}

int32_t spike1_linux::sys_clone(const uint32_t a[7], std::string &note)
{
	enum : uint32_t { SETTLS = 0x80000, PARENT_SETTID = 0x100000, CHILD_CLEARTID = 0x200000, CHILD_SETTID = 0x1000000 };
	const uint32_t flags = a[0], stack = a[1], ptid = a[2], tls = a[3], ctid = a[4];
	if (!stack) return -E_INVAL;
	auto t = std::make_unique<thread>();
	t->tid = m_next_tid++;
	m_cpu.save_context(t->ctx); // the parent's registers, with the PC already past the SWI
	t->ctx.r[0] = 0;
	t->ctx.r[13] = stack;
	t->tls = (flags & SETTLS) ? tls : m_current->tls;
	t->sigmask = m_current->sigmask;
	t->name = m_current->name;
	if (flags & CHILD_CLEARTID) t->clear_tid = ctid;
	if (flags & PARENT_SETTID) m_mem.write32(ptid, t->tid);
	if (flags & CHILD_SETTID) m_mem.write32(ctid, t->tid);
	const uint32_t tid = t->tid;
	m_threads.push_back(std::move(t));
	note = "thread " + std::to_string(tid) + " created";
	return int32_t(tid);
}

void spike1_linux::exit_thread(int32_t code, std::string &note)
{
	thread &t = *m_current;
	t.st = thread::state::done;
	if (t.clear_tid) { // what pthread_join waits for
		m_mem.write32(t.clear_tid, 0);
		futex_wake(t.clear_tid, 1, 0xffffffffu);
	}
	note = "thread " + std::to_string(t.tid) + " (" + t.name + ") exited with " + std::to_string(code);
	m_cpu.end_slice();
	if (!thread_count()) {
		m_status = status::exited;
		m_stop_reason = "the last thread exited";
	}
}

// ---------------------------------------------------------------- system calls

void spike1_linux::on_swi(uint32_t comment)
{
	if (m_status != status::running) { m_cpu.end_slice(); return; }
	// EABI passes the number in r7 with SWI 0; the old ABI put 0x900000 + number in the instruction
	const uint32_t nr = comment ? comment - 0x900000 : m_cpu.reg(7);
	uint32_t a[7];
	for (int i = 0; i < 7; i++) a[i] = m_cpu.reg(i);
	m_syscalls++;
	bool handled = true;
	std::string note;
	m_block = block_kind::none;
	m_sigreturned = false;
	const int32_t result = syscall(nr, a, handled, note);
	if (!handled) {
		const char *n = name_of(nr);
		stop("unimplemented system call " + std::to_string(nr) + (n ? std::string(" (") + n + ")" : "") +
			"(" + hex(a[0]) + ", " + hex(a[1]) + ", " + hex(a[2]) + ", " + hex(a[3]) + ") at pc " + hex(m_cpu.pc() - 4));
		return;
	}
	if (m_block == block_kind::retry) // back to the SWI, so it runs again when the thread wakes
		m_cpu.set_reg(15, m_cpu.pc() - (m_cpu.thumb() ? 2 : 4));
	else if (m_block == block_kind::none && m_current->st != thread::state::done && !m_sigreturned)
		m_cpu.set_reg(0, uint32_t(result));
	if (m_block == block_kind::none) deliver_signal(); // a signal the call sent to its own thread, say
	if (!note.empty() || (m_cfg.trace && m_block != block_kind::retry)) {
		const char *n = name_of(nr);
		char t[64];
		std::snprintf(t, sizeof(t), "[%10.6f] %u ", double(now_ns()) / 1e9, m_current->tid);
		log(std::string(t) + m_current->name + ": " + (n ? n : ("syscall " + std::to_string(nr)).c_str()) + "(" + hex(a[0]) + ", " + hex(a[1]) + ", " + hex(a[2]) +
			")" + (m_block != block_kind::none ? " waits" : " = " + std::to_string(result)) + (note.empty() ? "" : "  " + note));
	}
}

int32_t spike1_linux::syscall(uint32_t nr, const uint32_t a[7], bool &handled, std::string &note)
{
	switch (nr) {
	// ---- process
	case 1:   // exit: this thread only
		exit_thread(int32_t(a[0]), note);
		return 0;
	case 248: // exit_group
		m_status = status::exited;
		m_stop_reason = "exit status " + std::to_string(int32_t(a[0]));
		m_cpu.end_slice();
		note = "process exited";
		return 0;
	case 20:  return 100;   // getpid
	case 224: return int32_t(m_current->tid); // gettid
	case 64:  return 1;     // getppid
	case 199: case 200: case 201: case 202: return 0; // get[e]uid32/get[e]gid32
	case 60:  return 022;   // umask
	case 66:  return 100;   // setsid
	case 57:  return 0;     // setpgid
	case 256: m_current->clear_tid = a[0]; return int32_t(m_current->tid); // set_tid_address
	case 338: return 0;     // set_robust_list
	case 0xf0005: m_current->tls = a[0]; m_mem.write32(0xffff0ff0, a[0]); return 0; // set_tls
	case 0xf0002: return 0; // cacheflush: the interpreter reads code straight from memory
	case 172: // prctl
		if (a[0] == 15) { std::string name; m_mem.read_string(a[1], name, 16); note = "thread name \"" + name + "\""; m_current->name = name; }
		return 0;
	case 122: { // uname
		char u[6][65] = {};
		std::strcpy(u[0], "Linux"); std::strcpy(u[1], "spike"); std::strcpy(u[2], "3.10.0");
		std::strcpy(u[3], "#1 PREEMPT"); std::strcpy(u[4], "armv5tejl"); std::strcpy(u[5], "(none)");
		return m_mem.write(a[0], u, sizeof(u)) ? 0 : -E_FAULT;
	}
	case 191: { // ugetrlimit
		const uint32_t lim[2] = { a[0] == 3 ? spike1_memory::STACK_SIZE : 0xffffffffu, 0xffffffffu };
		return m_mem.write(a[1], lim, 8) ? 0 : -E_FAULT;
	}
	case 116: { // sysinfo
		uint8_t si[64] = {};
		const uint32_t uptime = uint32_t(now_ns() / 1000000000ull), totalram = 256u << 20, freeram = 128u << 20, unit = 1;
		std::memcpy(si + 0, &uptime, 4); std::memcpy(si + 16, &totalram, 4); std::memcpy(si + 20, &freeram, 4);
		std::memcpy(si + 52, &unit, 4);
		return m_mem.write(a[0], si, sizeof(si)) ? 0 : -E_FAULT;
	}
	case 120: // clone
		if ((a[0] & 0x10100) != 0x10100) { // without CLONE_VM | CLONE_THREAD it is a fork: one process only
			note = "fork refused";
			return -E_NOSYS;
		}
		return sys_clone(a, note);
	case 190: note = "vfork refused"; return -E_NOSYS;
	case 37: case 238: case 268: { // kill, tkill, tgkill
		const uint32_t sig = nr == 268 ? a[2] : a[1];
		if (sig == 0) return 0;  // only asks whether the target exists
		if (sig > 64) return -E_INVAL;
		if (m_sigactions[sig].handler <= 1) { note = "signal " + std::to_string(sig) + " ignored"; return 0; }
		thread *target = m_current; // one process: a signal to it goes to the thread that sends it
		if (nr != 37) {
			const uint32_t tid = nr == 268 ? a[1] : a[0];
			target = nullptr;
			for (auto &t : m_threads) if (t->tid == tid && t->st != thread::state::done) target = t.get();
			if (!target) return -E_SRCH;
		}
		target->sig_pending |= 1ull << (sig - 1);
		note = "signal " + std::to_string(sig) + " to thread " + std::to_string(target->tid);
		return 0;
	}
	case 119: case 173: { // sigreturn, rt_sigreturn: back to what the handler interrupted
		thread &t = *m_current;
		if (t.sig_frames.empty()) return -E_INVAL;
		m_cpu.load_context(t.sig_frames.back().first);
		t.sigmask = t.sig_frames.back().second;
		t.sig_frames.pop_back();
		m_sigreturned = true;
		return 0;
	}

	// ---- signals
	case 174: { // rt_sigaction
		const uint32_t sig = a[0];
		if (sig < 1 || sig > 64) return -E_INVAL;
		if (a[2]) {
			const auto &o = m_sigactions[sig];
			uint32_t old[5] = { o.handler, o.flags, o.restorer, uint32_t(o.mask), uint32_t(o.mask >> 32) };
			if (!m_mem.write(a[2], old, 20)) return -E_FAULT;
		}
		if (a[1]) {
			uint32_t act[5];
			if (!m_mem.read(a[1], act, 20)) return -E_FAULT;
			m_sigactions[sig] = { act[0], act[1], act[2], uint64_t(act[3]) | (uint64_t(act[4]) << 32) };
		}
		return 0;
	}
	case 175: { // rt_sigprocmask
		uint64_t &mask = m_current->sigmask;
		if (a[2] && !m_mem.write64(a[2], mask)) return -E_FAULT;
		if (a[1]) {
			uint64_t set;
			if (!m_mem.read(a[1], &set, 8)) return -E_FAULT;
			if (a[0] == 0) mask |= set; else if (a[0] == 1) mask &= ~set; else if (a[0] == 2) mask = set; else return -E_INVAL;
		}
		return 0;
	}
	case 186: return 0; // sigaltstack

	// ---- memory
	case 45: return int32_t(m_mem.brk(a[0])); // brk
	case 192: return sys_mmap(a[0], a[1], a[2], a[3], int32_t(a[4]), uint64_t(a[5]) * 4096, note); // mmap2
	case 91: m_mem.unmap(a[0], a[1]); return 0; // munmap
	case 125: case 220: case 144: case 150: case 152: return 0; // mprotect, madvise, msync, mlock, mlockall
	case 163: return -E_NOMEM; // mremap

	// ---- time
	case 263: { // clock_gettime
		const uint64_t ns = (a[0] == 0 ? m_wall_base_ns : 0) + now_ns(); // CLOCK_REALTIME vs the monotonic ones
		const uint32_t ts[2] = { uint32_t(ns / 1000000000ull), uint32_t(ns % 1000000000ull) };
		return m_mem.write(a[1], ts, 8) ? 0 : -E_FAULT;
	}
	case 264: { // clock_getres
		const uint32_t ts[2] = { 0, 1 };
		return (!a[1] || m_mem.write(a[1], ts, 8)) ? 0 : -E_FAULT;
	}
	case 78: { // gettimeofday
		const uint64_t ns = m_wall_base_ns + now_ns();
		const uint32_t tv[2] = { uint32_t(ns / 1000000000ull), uint32_t(ns % 1000000000ull / 1000) };
		return (!a[0] || m_mem.write(a[0], tv, 8)) ? 0 : -E_FAULT;
	}
	case 13: { // time
		const uint32_t t = uint32_t((m_wall_base_ns + now_ns()) / 1000000000ull);
		if (a[0]) m_mem.write32(a[0], t);
		return int32_t(t);
	}
	case 43: { // times
		const uint32_t ticks = uint32_t(now_ns() / 10000000ull);
		if (a[0]) { const uint32_t tms[4] = { ticks, 0, 0, 0 }; m_mem.write(a[0], tms, 16); }
		return int32_t(ticks);
	}
	case 162: // nanosleep
	case 265: { // clock_nanosleep(clock, flags, req, rem)
		const uint32_t req = nr == 162 ? a[0] : a[2];
		uint32_t ts[2];
		if (!m_mem.read(req, ts, 8)) return -E_FAULT;
		const uint64_t ns = uint64_t(ts[0]) * 1000000000ull + ts[1];
		const bool absolute = nr == 265 && (a[1] & 1);
		uint64_t wake_ns = absolute ? (a[0] == 0 ? (ns > m_wall_base_ns ? ns - m_wall_base_ns : 0) : ns) : now_ns() + ns;
		if (wake_ns <= now_ns()) return 0;
		block(wake_ns, false, 0);
		return 0;
	}

	// ---- scheduling
	case 158: m_cpu.end_slice(); return 0; // sched_yield
	case 154: case 156: case 97: case 241: return 0;
	case 155: return m_mem.write32(a[1], 0) ? 0 : -E_FAULT;
	case 157: case 96: return 0;
	case 159: return 99; case 160: return 1;
	case 242: { const uint32_t mask = 1; return m_mem.write32(a[2], mask) ? 4 : -E_FAULT; }

	// ---- futex
	case 240: return sys_futex(a, note);

	// ---- files
	case 5: case 322: { // open, openat
		const uint32_t path_arg = nr == 5 ? a[0] : a[1];
		if (nr == 322 && int32_t(a[0]) != AT_FDCWD_) return -E_INVAL;
		std::string path;
		if (!m_mem.read_string(path_arg, path)) return -E_FAULT;
		const int32_t fd = sys_open(path, nr == 5 ? a[1] : a[2], nr == 5 ? a[2] : a[3], note);
		if (note.empty() && m_cfg.trace) note = "\"" + path + "\"";
		return fd;
	}
	case 6: { // close
		auto it = m_fds.find(int32_t(a[0]));
		if (it == m_fds.end()) return -E_BADF;
		if (it->second->type == file::kind::device) note = "close " + it->second->guest;
		m_fds.erase(it);
		return 0;
	}
	case 3: return sys_read(int32_t(a[0]), a[1], a[2], note);
	case 4: return sys_write(int32_t(a[0]), a[1], a[2], note);
	case 145: case 146: { // readv, writev
		int32_t total = 0;
		for (uint32_t i = 0; i < a[2]; i++) {
			uint32_t iov[2];
			if (!m_mem.read(a[1] + 8 * i, iov, 8)) return -E_FAULT;
			const int32_t r = nr == 145 ? sys_read(int32_t(a[0]), iov[0], iov[1], note) : sys_write(int32_t(a[0]), iov[0], iov[1], note);
			if (r < 0) return total ? total : r;
			total += r;
			if (uint32_t(r) < iov[1]) break;
		}
		return total;
	}
	case 180: case 181: { // pread64, pwrite64: fd, buf, count, pad, offset lo, hi - the file position stays where it was
		auto f = get_fd(int32_t(a[0]));
		if (!f) return -E_BADF;
		const int64_t offset = int64_t(uint64_t(a[4]) | (uint64_t(a[5]) << 32));
		if (offset < 0) return -E_INVAL;
		if (f->type == file::kind::memory || f->type == file::kind::written) {
			const size_t saved = f->position;
			f->position = size_t(offset);
			const int32_t r = nr == 180 ? sys_read(int32_t(a[0]), a[1], a[2], note) : sys_write(int32_t(a[0]), a[1], a[2], note);
			f->position = saved;
			return r;
		}
		if (f->type != file::kind::host || !f->fp) return -E_SPIPE;
		const int64_t saved = host_tell(f->fp);
		host_seek(f->fp, offset, SEEK_SET);
		const int32_t r = nr == 180 ? sys_read(int32_t(a[0]), a[1], a[2], note) : sys_write(int32_t(a[0]), a[1], a[2], note);
		host_seek(f->fp, saved, SEEK_SET);
		return r;
	}
	case 54: return sys_ioctl(int32_t(a[0]), a[1], a[2], note);
	case 19: { // lseek
		int64_t result;
		const int32_t r = sys_lseek(int32_t(a[0]), int32_t(a[1]), int32_t(a[2]), result);
		return r < 0 ? r : int32_t(result);
	}
	case 140: { // _llseek(fd, high, low, result*, whence)
		int64_t result;
		const int32_t r = sys_lseek(int32_t(a[0]), int64_t((uint64_t(a[1]) << 32) | a[2]), int32_t(a[4]), result);
		if (r < 0) return r;
		return m_mem.write64(a[3], uint64_t(result)) ? 0 : -E_FAULT;
	}
	case 195: case 196: case 106: { // stat64, lstat64, stat
		std::string path;
		if (!m_mem.read_string(a[0], path)) return -E_FAULT;
		return sys_stat(path, a[1], nr != 196);
	}
	case 327: { // fstatat64(dirfd, path, buf, flags)
		if (int32_t(a[0]) != AT_FDCWD_) return -E_INVAL;
		std::string path;
		if (!m_mem.read_string(a[1], path)) return -E_FAULT;
		return sys_stat(path, a[2], !(a[3] & 0x100));
	}
	case 197: case 108: return sys_fstat(int32_t(a[0]), a[1]); // fstat64, fstat
	case 33: case 334: { // access, faccessat
		std::string path;
		if (!m_mem.read_string(nr == 33 ? a[0] : a[1], path)) return -E_FAULT;
		const std::string g = guest_absolute(path);
		if (g.rfind("/dev/", 0) == 0) return 0;
		return m_vfs.lookup(g) == spike1_vfs::type::none ? -E_NOENT : 0;
	}
	case 217: return sys_getdents64(int32_t(a[0]), a[1], a[2]);
	case 85: { // readlink
		std::string path;
		if (!m_mem.read_string(a[0], path)) return -E_FAULT;
		if (m_cfg.trace) note = "\"" + path + "\"";
		if (path == "/proc/self/exe") {
			const uint32_t n = std::min<uint32_t>(a[2], uint32_t(m_cfg.executable.size()));
			return m_mem.write(a[1], m_cfg.executable.data(), n) ? int32_t(n) : -E_FAULT;
		}
		return -E_INVAL;
	}
	case 183: { // getcwd
		if (m_cwd.size() + 1 > a[1]) return -E_RANGE;
		return m_mem.write(a[0], m_cwd.c_str(), uint32_t(m_cwd.size() + 1)) ? int32_t(m_cwd.size() + 1) : -E_FAULT;
	}
	case 12: { // chdir
		std::string path;
		if (!m_mem.read_string(a[0], path)) return -E_FAULT;
		const std::string g = guest_absolute(path);
		if (m_cfg.trace) note = "\"" + g + "\"";
		const spike1_vfs::type t = m_vfs.lookup(g);
		if (t == spike1_vfs::type::none) return -E_NOENT;
		if (t != spike1_vfs::type::directory) return -E_NOTDIR;
		m_cwd = g;
		return 0;
	}
	case 39: { // mkdir
		std::string path;
		if (!m_mem.read_string(a[0], path)) return -E_FAULT;
		const std::string g = guest_absolute(path);
		if (m_vfs.lookup(g) != spike1_vfs::type::none) return -E_EXIST;
		note = "mkdir " + g;
		return m_vfs.make_directory(g) ? 0 : -E_NOENT;
	}
	case 10: case 40: { // unlink, rmdir
		std::string path;
		if (!m_mem.read_string(a[0], path)) return -E_FAULT;
		const std::string g = guest_absolute(path);
		const spike1_vfs::type t = m_vfs.lookup(g);
		if (t == spike1_vfs::type::none) return -E_NOENT;
		if ((t == spike1_vfs::type::directory) != (nr == 40)) return nr == 40 ? -E_NOTDIR : -E_ISDIR;
		return m_vfs.remove(g) ? 0 : -E_NOTEMPTY;
	}
	case 36: case 373: m_syncs++; return 0; // sync, syncfs
	case 118: case 148: return 0;          // fsync, fdatasync
	case 143: return 0; // flock
	case 15: return 0;  // chmod
	case 41: case 63: { // dup, dup2
		auto f = get_fd(int32_t(a[0]));
		if (!f) return -E_BADF;
		if (nr == 63) { m_fds[int32_t(a[1])] = f; return int32_t(a[1]); }
		return alloc_fd(f);
	}
	case 55: case 221: { // fcntl, fcntl64
		auto f = get_fd(int32_t(a[0]));
		if (!f) return -E_BADF;
		switch (a[1]) {
		case 0: return alloc_fd(f, int32_t(a[2]));        // F_DUPFD
		case 1: case 2: return 0;                          // F_GETFD, F_SETFD
		case 3: return int32_t(f->flags);                  // F_GETFL
		case 4: f->flags = (f->flags & ~O_NONBLOCK_) | (a[2] & O_NONBLOCK_); return 0; // F_SETFL
		default: return 0;                                 // locks always succeed for the only process
		}
	}
	case 142: case 168: return sys_poll(nr, a, note); // _newselect, poll
	case 281: return -E_AFNOSUPPORT; // socket
	case 42: case 359: { // pipe, pipe2
		auto buffer = std::make_shared<std::deque<uint8_t>>();
		auto r = std::make_shared<file>(), w = std::make_shared<file>();
		r->type = file::kind::pipe_read; w->type = file::kind::pipe_write;
		r->guest = w->guest = "pipe";
		r->pipe = w->pipe = buffer;
		r->flags = (nr == 359 ? a[1] : 0) & O_NONBLOCK_;
		w->flags = O_WRONLY_ | r->flags;
		const int32_t fds[2] = { alloc_fd(r, 3), alloc_fd(w, 3) };
		if (!m_mem.write(a[0], fds, 8)) return -E_FAULT;
		note = "pipe fds " + std::to_string(fds[0]) + ", " + std::to_string(fds[1]);
		return 0;
	}
	default:
		handled = false;
		return -E_NOSYS;
	}
}

// ---------------------------------------------------------------- paths

std::string spike1_linux::guest_absolute(const std::string &path) const
{
	std::vector<std::string> parts;
	const std::string full = (!path.empty() && path[0] == '/') ? path : m_cwd + "/" + path;
	size_t i = 0;
	while (i < full.size()) {
		const size_t j = full.find('/', i);
		const std::string part = full.substr(i, j == std::string::npos ? std::string::npos : j - i);
		if (part == "..") { if (!parts.empty()) parts.pop_back(); }
		else if (!part.empty() && part != ".") parts.push_back(part);
		if (j == std::string::npos) break;
		i = j + 1;
	}
	std::string out;
	for (const auto &p : parts) out += "/" + p;
	return out.empty() ? "/" : out;
}

bool spike1_linux::read_whole(const std::string &guest, std::vector<uint8_t> &out) const
{
	spike1_vfs::source src;
	if (!m_vfs.open(guest, src)) return false;
	if (src.written) { out = *src.written; return true; }
	if (src.data) { out.assign(src.data, src.data + src.size); return true; }
	std::ifstream in(fs::u8path(src.host_path), std::ios::binary);
	if (!in) return false;
	out.resize(src.size);
	in.read(reinterpret_cast<char *>(out.data()), std::streamsize(out.size()));
	out.resize(size_t(in.gcount()));
	return true;
}

// The NVRAM block: "S1ST", a format version, then the written files (spike1_vfs::save()) and the
// board's EEPROMs (spike1_devices::nvram()), each as [length, u32][bytes]
namespace { constexpr char NVRAM_MAGIC[4] = { 'S', '1', 'S', 'T' }; constexpr uint8_t NVRAM_VERSION = 1; }

std::vector<uint8_t> spike1_linux::nvram() const
{
	std::vector<uint8_t> out(NVRAM_MAGIC, NVRAM_MAGIC + 4);
	out.push_back(NVRAM_VERSION);
	for (const std::vector<uint8_t> &part : { m_vfs.save(), m_devices.nvram() }) {
		for (int i = 0; i < 4; i++) out.push_back(uint8_t(uint32_t(part.size()) >> (8 * i)));
		out.insert(out.end(), part.begin(), part.end());
	}
	return out;
}

bool spike1_linux::set_nvram(const std::vector<uint8_t> &block)
{
	const uint8_t *p = block.data();
	size_t left = block.size();
	if (left < 5 || std::memcmp(p, NVRAM_MAGIC, 4) != 0 || p[4] != NVRAM_VERSION) return false;
	p += 5; left -= 5;
	std::vector<std::pair<const uint8_t *, size_t>> parts;
	for (int i = 0; i < 2; i++) {
		if (left < 4) return false;
		const size_t n = p[0] | (p[1] << 8) | (p[2] << 16) | (size_t(p[3]) << 24);
		p += 4; left -= 4;
		if (left < n) return false;
		parts.push_back({ p, n });
		p += n; left -= n;
	}
	if (!m_vfs.load(parts[0].first, parts[0].second)) return false;
	m_devices.set_nvram(parts[1].first, parts[1].second);
	return true;
}

int spike1_linux::alloc_fd(std::shared_ptr<file> f, int lowest)
{
	int fd = std::max(lowest, 0);
	while (m_fds.count(fd)) fd++;
	if (fd >= 1024) return -E_MFILE;
	m_fds[fd] = std::move(f);
	return fd;
}

std::shared_ptr<spike1_linux::file> spike1_linux::get_fd(int fd) const
{
	auto it = m_fds.find(fd);
	return it == m_fds.end() ? nullptr : it->second;
}

int32_t spike1_linux::sys_open(const std::string &path, uint32_t flags, uint32_t mode, std::string &note)
{
	(void)mode;
	const std::string g = guest_absolute(path);
	auto f = std::make_shared<file>();
	f->guest = g;
	f->flags = flags;
	if (g == "/dev/null") f->type = file::kind::null;
	else if (g == "/dev/zero") f->type = file::kind::zero;
	else if (g == "/dev/random" || g == "/dev/urandom") f->type = file::kind::random;
	else if (g == "/dev/console" || g == "/dev/tty") f->type = file::kind::console;
	else if (g.rfind("/dev/", 0) == 0) {
		f->type = file::kind::device;
		f->dev.type = spike1_devices::kind_for_path(g);
		f->dev.path = g;
		const int fd = alloc_fd(f);
		note = "open device " + g + " flags " + hex(flags) + " -> fd " + std::to_string(fd);
		return fd;
	}
	else {
		const bool writing = (flags & O_ACCMODE_) != 0 || (flags & (O_CREAT_ | O_TRUNC_));
		const spike1_vfs::type t = m_vfs.lookup(g);
		if (t == spike1_vfs::type::directory) {
			if (writing) return -E_ISDIR;
			f->type = file::kind::directory;
			f->entries.push_back({ ".", 4, 1 });
			f->entries.push_back({ "..", 4, 1 });
			uint64_t inode = 2;
			for (const auto &e : m_vfs.list(g)) f->entries.push_back({ e.name, uint8_t(e.directory ? 4 : 8), inode++ });
			return alloc_fd(f);
		}
		if (flags & O_DIRECTORY_) return t == spike1_vfs::type::none ? -E_NOENT : -E_NOTDIR;
		if (writing) {
			// the title's files are never written: the first write copies one into the written layer
			if ((flags & O_CREAT_) && (flags & O_EXCL_) && t != spike1_vfs::type::none) return -E_EXIST;
			if (t == spike1_vfs::type::none && !(flags & O_CREAT_)) return -E_NOENT;
			f->written = m_vfs.open_write(g, (flags & O_TRUNC_) != 0);
			if (!f->written) return -E_NOENT; // the directory it would go in is missing
			f->type = file::kind::written;
			note = "open for writing " + g;
		} else {
			spike1_vfs::source src;
			if (!m_vfs.open(g, src)) {
				note = "not found: " + g;
				return -E_NOENT;
			}
			if (src.written) { f->type = file::kind::written; f->written = src.written; }
			else if (src.data) { f->type = file::kind::memory; f->data = src.data; f->size = src.size; }
			else {
				f->fp = host_fopen(src.host_path, false, false);
				if (!f->fp) return -E_ACCES;
				f->host = src.host_path;
				f->size = src.size;
			}
		}
	}
	return alloc_fd(f);
}

int32_t spike1_linux::sys_read(int fd, uint32_t buf, uint32_t len, std::string &note)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	if (!len) return 0;
	uint8_t *p = m_mem.host(buf, len);
	if (!p) return -E_FAULT;
	switch (f->type) {
	case file::kind::host: {
		if (!f->fp) return -E_BADF;
		return int32_t(std::fread(p, 1, len, f->fp));
	}
	case file::kind::memory: case file::kind::written: {
		const uint8_t *src = f->written ? f->written->data() : f->data;
		const size_t size = f->written ? f->written->size() : f->size;
		const uint32_t n = f->position < size ? uint32_t(std::min<size_t>(len, size - f->position)) : 0;
		if (n) std::memcpy(p, src + f->position, n);
		f->position += n;
		return int32_t(n);
	}
	case file::kind::zero: std::memset(p, 0, len); return int32_t(len);
	case file::kind::pipe_read: {
		if (f->pipe->empty()) {
			if (f->flags & O_NONBLOCK_) return -E_AGAIN;
			block(now_ns() + POLL_RECHECK_NS, true); // try again until another thread writes
			return 0;
		}
		const uint32_t n = std::min<uint32_t>(len, uint32_t(f->pipe->size()));
		std::copy_n(f->pipe->begin(), n, p);
		f->pipe->erase(f->pipe->begin(), f->pipe->begin() + n);
		return int32_t(n);
	}
	case file::kind::random: for (uint32_t i = 0; i < len; i++) p[i] = uint8_t(m_cycles * 2654435761u >> (i % 24)) ^ uint8_t(i * 151); return int32_t(len);
	case file::kind::device: {
		std::string device_note;
		uint64_t wait_ns = 0;
		const int32_t r = m_devices.read(f->dev, buf, len, now_ns(), wait_ns, device_note);
		if (r == spike1_devices::WAIT) {
			if (f->flags & O_NONBLOCK_) return -E_AGAIN;
			block(wait_ns, true);
			return 0;
		}
		if (r != 0) { m_current->read_wait_fd = -1; note = device_note; return r; }
		// no data yet: a blocking read waits for it, as the driver would
		if (f->flags & O_NONBLOCK_) return -E_AGAIN;
		if (m_current->read_wait_fd != fd) note = device_note.empty() ? "read " + std::to_string(len) + " from " + f->guest + " waits" : device_note;
		m_current->read_wait_fd = fd; // note the first attempt only
		block(now_ns() + POLL_RECHECK_NS, true);
		return 0;
	}
	case file::kind::directory: return -E_ISDIR;
	default: return 0;
	}
}

int32_t spike1_linux::sys_write(int fd, uint32_t buf, uint32_t len, std::string &note)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	const uint8_t *p = len ? m_mem.host(buf, len, false) : nullptr;
	if (len && !p) return -E_FAULT;
	switch (f->type) {
	case file::kind::pipe_write:
		f->pipe->insert(f->pipe->end(), p, p + len);
		return int32_t(len);
	case file::kind::console: {
		std::string text(reinterpret_cast<const char *>(p), len);
		while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back();
		log("[" + f->guest + "] " + text);
		return int32_t(len);
	}
	case file::kind::host: case file::kind::memory:
		return -E_BADF; // the title's files are only ever open for reading
	case file::kind::written: {
		if ((f->flags & O_ACCMODE_) == 0) return -E_BADF;
		std::vector<uint8_t> &bytes = *f->written;
		if (f->flags & O_APPEND_) f->position = bytes.size();
		if (f->position + len > bytes.size()) bytes.resize(f->position + len);
		if (len) std::memcpy(bytes.data() + f->position, p, len);
		f->position += len;
		return int32_t(len);
	}
	case file::kind::device: {
		uint64_t wait_ns = 0;
		const int32_t r = m_devices.write(f->dev, buf, len, now_ns(), wait_ns, note);
		if (r != spike1_devices::WAIT) return r;
		if (f->flags & O_NONBLOCK_) return -E_AGAIN;
		block(wait_ns, true);
		return 0;
	}
	case file::kind::directory: return -E_ISDIR;
	default: return int32_t(len);
	}
}

int32_t spike1_linux::sys_ioctl(int fd, uint32_t req, uint32_t arg, std::string &note)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	if (req == 0x541b) // FIONREAD
		return m_mem.write32(arg, 0) ? 0 : -E_FAULT;
	if (f->type == file::kind::device) {
		uint64_t wait_ns = 0;
		const int32_t r = m_devices.ioctl(f->dev, req, arg, now_ns(), wait_ns, note);
		if (r != spike1_devices::WAIT) return r;
		block(wait_ns, true);
		return 0;
	}
	return -E_NOTTY;
}

int32_t spike1_linux::write_stat64(uint32_t buf, uint32_t mode, uint64_t size, uint64_t inode)
{
	// glibc fails fstat() with EOVERFLOW when the inode does not fit its 32-bit st_ino
	inode = (inode & 0x7fffffff) | 1;
	// arch/arm stat64: 104 bytes, with the historical padding around the dev_t fields
	uint8_t st[104] = {};
	const uint64_t dev = 1;
	std::memcpy(st + 0, &dev, 8);
	const uint32_t ino32 = uint32_t(inode), nlink = 1, blksize = 4096;
	std::memcpy(st + 12, &ino32, 4);
	std::memcpy(st + 16, &mode, 4);
	std::memcpy(st + 20, &nlink, 4);
	std::memcpy(st + 48, &size, 8);
	std::memcpy(st + 56, &blksize, 4);
	const uint64_t blocks = (size + 511) / 512;
	std::memcpy(st + 64, &blocks, 8);
	std::memcpy(st + 96, &inode, 8);
	return m_mem.write(buf, st, sizeof(st)) ? 0 : -E_FAULT;
}

int32_t spike1_linux::sys_stat(const std::string &path, uint32_t buf, bool follow)
{
	(void)follow;
	const std::string g = guest_absolute(path);
	if (g.rfind("/dev/", 0) == 0) return write_stat64(buf, S_IFCHR_ | 0666, 0, std::hash<std::string>()(g));
	uint64_t size = 0;
	const spike1_vfs::type t = m_vfs.lookup(g, &size);
	if (t == spike1_vfs::type::none) return -E_NOENT;
	if (t == spike1_vfs::type::directory) return write_stat64(buf, S_IFDIR_ | 0755, 4096, std::hash<std::string>()(g));
	return write_stat64(buf, S_IFREG_ | 0755, size, std::hash<std::string>()(g));
}

int32_t spike1_linux::sys_fstat(int fd, uint32_t buf)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	const uint64_t inode = std::hash<std::string>()(f->guest);
	switch (f->type) {
	case file::kind::host: case file::kind::memory: return write_stat64(buf, S_IFREG_ | 0755, f->size, inode);
	case file::kind::written: return write_stat64(buf, S_IFREG_ | 0755, f->written->size(), inode);
	case file::kind::directory: return write_stat64(buf, S_IFDIR_ | 0755, 4096, inode);
	default: return write_stat64(buf, S_IFCHR_ | 0666, 0, inode);
	}
}

int32_t spike1_linux::sys_getdents64(int fd, uint32_t buf, uint32_t len)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	if (f->type != file::kind::directory) return -E_NOTDIR;
	uint32_t used = 0;
	while (f->position < f->entries.size()) {
		const auto &e = f->entries[f->position];
		const uint32_t reclen = uint32_t((19 + e.name.size() + 1 + 7) & ~size_t(7));
		if (used + reclen > len) { if (!used) return -E_INVAL; break; }
		std::vector<uint8_t> rec(reclen, 0);
		const int64_t off = int64_t(f->position + 1);
		const uint16_t rl = uint16_t(reclen);
		std::memcpy(&rec[0], &e.inode, 8);
		std::memcpy(&rec[8], &off, 8);
		std::memcpy(&rec[16], &rl, 2);
		rec[18] = e.type;
		std::memcpy(&rec[19], e.name.c_str(), e.name.size());
		if (!m_mem.write(buf + used, rec.data(), reclen)) return -E_FAULT;
		used += reclen;
		f->position++;
	}
	return int32_t(used);
}

int32_t spike1_linux::sys_lseek(int fd, int64_t offset, int whence, int64_t &result)
{
	auto f = get_fd(fd);
	if (!f) return -E_BADF;
	if (f->type == file::kind::directory) { if (whence == 0) f->position = size_t(offset); result = int64_t(f->position); return 0; }
	if (f->type == file::kind::memory || f->type == file::kind::written) {
		const int64_t size = int64_t(f->written ? f->written->size() : f->size);
		const int64_t base = whence == 0 ? 0 : whence == 1 ? int64_t(f->position) : whence == 2 ? size : -1;
		if (base < 0 || base + offset < 0) return -E_INVAL;
		f->position = size_t(base + offset);
		result = int64_t(f->position);
		return 0;
	}
	if (f->type != file::kind::host || !f->fp) { result = 0; return f->type == file::kind::console ? -E_SPIPE : 0; }
	if (host_seek(f->fp, offset, whence) != 0) return -E_INVAL;
	result = host_tell(f->fp);
	return 0;
}

int32_t spike1_linux::sys_mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags, int fd, uint64_t offset, std::string &note)
{
	if (!len) return -E_INVAL;
	const bool fixed = (flags & 0x10) != 0, anonymous = (flags & 0x20) != 0;
	uint32_t where;
	if (!fixed && !anonymous && len >= (1u << 20) && !(prot & 2)) {
		// a big read-only mapping (the asset image, WWE's LCD clips): a view of the file instead of
		// a copy - of a file the host holds, or from 16 MB of one on the host's disk
		auto f = get_fd(fd);
		const bool in_memory = f && f->type == file::kind::memory && offset + len <= f->size;
		if (in_memory || (f && f->type == file::kind::host && f->fp && len >= (16u << 20))) {
			const uint32_t view = in_memory ? m_mem.map_view(f->data + offset, len) : m_mem.map_file(f->host, offset, len);
			note = view ? "mapped " + f->guest + " (" + std::to_string(len) + " bytes) read-only at " + hex(view)
			            : "could not map " + f->guest + " (" + std::to_string(len) + " bytes)";
			return view ? int32_t(view) : -E_NOMEM;
		}
	}
	if (fixed) {
		if (!m_mem.host(addr, len)) return -E_NOMEM;
		where = addr;
		std::memset(m_mem.host(addr, len), 0, len);
	} else {
		where = m_mem.map(len);
		if (!where) { note = "out of mmap space for " + std::to_string(len) + " bytes"; return -E_NOMEM; }
	}
	if (!anonymous) {
		auto f = get_fd(fd);
		if (!f) { m_mem.unmap(where, len); return -E_BADF; }
		if (f->type == file::kind::device) {
			m_devices.mapped(f->dev, where, len);
			note = "mmap of device " + f->guest + " (" + std::to_string(len) + " bytes at offset " + std::to_string(offset) + ") gives plain memory";
		} else if (f->type == file::kind::memory || f->type == file::kind::written) {
			const uint8_t *src = f->written ? f->written->data() : f->data;
			const size_t size = f->written ? f->written->size() : f->size;
			if (offset < size) std::memcpy(m_mem.host(where, len), src + offset, size_t(std::min<uint64_t>(len, size - offset)));
			if (flags & 1) note = "shared mapping of " + f->guest + " is a private copy";
		} else if (f->type == file::kind::host && f->fp) {
			// a private copy: the game's Spike 1 files are read-only data
			const int64_t saved = host_tell(f->fp);
			host_seek(f->fp, int64_t(offset), SEEK_SET);
			std::fread(m_mem.host(where, len), 1, len, f->fp);
			host_seek(f->fp, saved, SEEK_SET);
			if (flags & 1) note = "shared mapping of " + f->guest + " is a private copy";
		} else {
			m_mem.unmap(where, len);
			return -E_INVAL;
		}
	}
	return int32_t(where);
}

int32_t spike1_linux::sys_futex(const uint32_t a[7], std::string &note)
{
	const uint32_t uaddr = a[0], op = a[1], val = a[2], timeout = a[3], uaddr2 = a[4], val3 = a[5];
	const uint32_t cmd = op & 0x7f;
	switch (cmd) {
	case 0: case 9: { // FUTEX_WAIT, FUTEX_WAIT_BITSET
		uint32_t current;
		if (!m_mem.read32(uaddr, current)) return -E_FAULT;
		if (current != val) return -E_AGAIN;
		uint64_t deadline = 0;
		if (timeout) {
			uint32_t ts[2];
			if (!m_mem.read(timeout, ts, 8)) return -E_FAULT;
			const uint64_t ns = uint64_t(ts[0]) * 1000000000ull + ts[1];
			// the bitset form takes an absolute time, on the realtime clock with FUTEX_CLOCK_REALTIME
			if (cmd == 9) deadline = (op & 256) ? (ns > m_wall_base_ns ? ns - m_wall_base_ns : 1) : ns;
			else deadline = now_ns() + ns;
			if (deadline <= now_ns()) return -E_TIMEDOUT;
		}
		m_current->futex_addr = uaddr;
		m_current->futex_bitset = cmd == 9 ? val3 : 0xffffffffu;
		block(deadline, false, -E_TIMEDOUT);
		return 0;
	}
	case 1: case 10: // FUTEX_WAKE, FUTEX_WAKE_BITSET
		return futex_wake(uaddr, val, cmd == 10 ? val3 : 0xffffffffu);
	case 3: case 4: { // FUTEX_REQUEUE, FUTEX_CMP_REQUEUE; the requeue count comes in the timeout slot
		if (cmd == 4) {
			uint32_t current;
			if (!m_mem.read32(uaddr, current)) return -E_FAULT;
			if (current != val3) return -E_AGAIN;
		}
		int done = futex_wake(uaddr, val, 0xffffffffu);
		uint32_t moved = 0;
		for (auto &t : m_threads)
			if (moved < timeout && t->st == thread::state::blocked && t->futex_addr == uaddr) { t->futex_addr = uaddr2; moved++; }
		return done + int(moved);
	}
	case 5: { // FUTEX_WAKE_OP: operate on uaddr2, wake on uaddr, and on uaddr2 if the old value passes the test
		uint32_t old;
		if (!m_mem.read32(uaddr2, old)) return -E_FAULT;
		auto sext12 = [](uint32_t v) { return int32_t(v << 20) >> 20; };
		const uint32_t opcode = (val3 >> 28) & 7, cmp = (val3 >> 24) & 15;
		int32_t oparg = sext12((val3 >> 12) & 0xfff);
		const int32_t cmparg = sext12(val3 & 0xfff);
		if (val3 & 0x80000000u) oparg = int32_t(1u << (oparg & 31)); // FUTEX_OP_OPARG_SHIFT
		uint32_t value;
		switch (opcode) {
		case 0: value = uint32_t(oparg); break;
		case 1: value = old + uint32_t(oparg); break;
		case 2: value = old | uint32_t(oparg); break;
		case 3: value = old & ~uint32_t(oparg); break;
		case 4: value = old ^ uint32_t(oparg); break;
		default: return -E_NOSYS;
		}
		m_mem.write32(uaddr2, value);
		int done = futex_wake(uaddr, val, 0xffffffffu);
		const int32_t o = int32_t(old);
		bool pass = false;
		switch (cmp) {
		case 0: pass = o == cmparg; break;
		case 1: pass = o != cmparg; break;
		case 2: pass = o < cmparg; break;
		case 3: pass = o <= cmparg; break;
		case 4: pass = o > cmparg; break;
		case 5: pass = o >= cmparg; break;
		}
		if (pass) done += futex_wake(uaddr2, timeout, 0xffffffffu);
		return done;
	}
	default:
		note = "futex op " + hex(op) + " is not implemented";
		return -E_NOSYS;
	}
}

bool spike1_linux::readable_now(int fd) const
{
	auto f = get_fd(fd);
	if (!f) return false;
	switch (f->type) {
	case file::kind::pipe_read: return !f->pipe->empty();
	case file::kind::device: return m_devices.readable(f->dev, now_ns());
	case file::kind::console: return false; // no input yet
	default: return true;
	}
}

int32_t spike1_linux::sys_poll(uint32_t nr, const uint32_t a[7], std::string &note)
{
	thread &t = *m_current;
	const uint64_t now = now_ns();
	int64_t timeout_ns = -1; // forever
	if (nr == 168) {
		if (int32_t(a[2]) >= 0) timeout_ns = int64_t(int32_t(a[2])) * 1000000;
	} else if (a[4]) {
		uint32_t tv[2];
		if (!m_mem.read(a[4], tv, 8)) return -E_FAULT;
		timeout_ns = int64_t(tv[0]) * 1000000000 + int64_t(tv[1]) * 1000;
	}

	int32_t ready = 0;
	std::vector<uint32_t> in_r, in_w, out_r, out_w;
	if (nr == 168) { // poll(fds, nfds, timeout_ms)
		for (uint32_t i = 0; i < a[1]; i++) {
			uint8_t pfd[8];
			if (!m_mem.read(a[0] + 8 * i, pfd, 8)) return -E_FAULT;
			int32_t fd; uint16_t events, revents = 0;
			std::memcpy(&fd, pfd, 4);
			std::memcpy(&events, pfd + 4, 2);
			if (fd >= 0) {
				if (!get_fd(fd)) revents = 0x20;                       // POLLNVAL
				else {
					if ((events & 0x01) && readable_now(fd)) revents |= 0x01; // POLLIN
					if (events & 0x04) revents |= 0x04;                      // POLLOUT: writes never wait
				}
			}
			m_mem.write(a[0] + 8 * i + 6, &revents, 2);
			if (revents) ready++;
		}
	} else { // select(nfds, readfds, writefds, exceptfds, timeout)
		const uint32_t words = (a[0] + 31) / 32;
		in_r.assign(words, 0); in_w.assign(words, 0); out_r.assign(words, 0); out_w.assign(words, 0);
		if (a[1] && !m_mem.read(a[1], in_r.data(), words * 4)) return -E_FAULT;
		if (a[2] && !m_mem.read(a[2], in_w.data(), words * 4)) return -E_FAULT;
		for (uint32_t fd = 0; fd < a[0]; fd++) {
			const uint32_t bit = 1u << (fd & 31);
			if ((in_r[fd / 32] & bit) && readable_now(int(fd))) { out_r[fd / 32] |= bit; ready++; }
			if ((in_w[fd / 32] & bit) && get_fd(int(fd))) { out_w[fd / 32] |= bit; ready++; }
		}
	}

	if (!t.poll_deadline) t.poll_deadline = timeout_ns < 0 ? UINT64_MAX : now + uint64_t(timeout_ns);
	if (ready || timeout_ns == 0 || now >= t.poll_deadline) {
		t.poll_deadline = 0;
		if (nr == 142) { // the sets change only when select returns
			const uint32_t bytes = uint32_t(out_r.size() * 4);
			std::vector<uint32_t> none(out_r.size(), 0);
			if (a[1]) m_mem.write(a[1], out_r.data(), bytes);
			if (a[2]) m_mem.write(a[2], out_w.data(), bytes);
			if (a[3]) m_mem.write(a[3], none.data(), bytes);
		}
		return ready;
	}
	block(std::min<uint64_t>(t.poll_deadline, now + POLL_RECHECK_NS), true); // look again soon
	(void)note;
	return 0;
}
