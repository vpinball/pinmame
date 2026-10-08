// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - Linux, emulated at the system-call level
//
// The game program is loaded and started like the kernel would start it (ELF segments, the initial
// stack with argv/envp/auxv, the 0xffff0000 helper page), and every SWI it executes is answered
// here. Files come from the title's own files, read-only; what the game writes stays in memory
// and is saved with the rest of the machine's NVRAM (see spike1_vfs.h).
//
// The game's threads share the one emulated CPU: run() hands each runnable thread a quantum in
// turn and switches register sets in between. A thread that blocks (futex, sleep, poll) leaves
// the CPU; when every thread is blocked, time skips ahead to the next wake-up without running
// instructions. Time is emulated time: CPU cycles at the configured clock.
//
// Bring-up stage: any system call that is not implemented yet stops the process with a message -
// that is the work list.

#pragma once

#include "spike1_cpu.h"
#include "spike1_devices.h"
#include "spike1_memory.h"
#include "spike1_vfs.h"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class spike1_linux
{
public:
	struct memory_file { std::string path; const uint8_t *data = nullptr; size_t size = 0; };
	struct config
	{
		// The title's files, never written: a host directory holding the machine's extracted root
		// filesystem, and/or files in memory by guest path (the caller keeps them alive)
		std::string root;
		std::vector<memory_file> files;
		std::string executable;  // guest path of the game program, e.g. /games/<name>/game
		std::vector<uint8_t> nvram; // what nvram() gave the last time; empty for a new machine
		std::vector<std::string> environment;
		uint32_t clock_hz = 400000000;
		bool trace = false;      // log every system call, not only the noteworthy ones
		std::function<void (const std::string &)> log;
	};

	enum class status { running, exited, stopped };

	spike1_linux(spike1_cpu_device &cpu, spike1_memory &mem);
	~spike1_linux();

	bool start(const config &cfg, std::string &error);

	// One time slice of `cycles` CPU cycles
	status run(int cycles);

	// Switches the machine off the way the mains would: the game sees the power fail, commits
	// what it keeps in memory to its NVRAM files and syncs them. Runs the machine until that
	// sync, or for at most max_ns of emulated time; true when the game synced
	bool power_down(uint64_t max_ns, int slice_cycles);

	status state() const { return m_status; }
	const std::string &stop_reason() const { return m_stop_reason; }
	uint64_t cycles() const { return m_cycles; }
	uint64_t idle_cycles() const { return m_idle_cycles; }
	uint64_t now_ns() const;
	uint64_t syscall_count() const { return m_syscalls; }
	size_t thread_count() const;

	// Called by the CPU's address space for an access outside guest memory
	void fault(uint32_t addr, bool write);

	// Everything the machine keeps across power cycles - the files the game wrote and the board's
	// EEPROMs - as one block, for the host's NVRAM file
	std::vector<uint8_t> nvram() const;
	const spike1_devices &devices() const { return m_devices; }
	spike1_devices &devices() { return m_devices; }
	// Address of a symbol in the game program's symbol table, 0 when it has none by that name
	uint32_t symbol(const std::string &name) const;
	uint32_t symbol_size(const std::string &name) const;

private:
	struct file;
	struct sigaction_entry { uint32_t handler = 0, flags = 0, restorer = 0; uint64_t mask = 0; };

	struct thread
	{
		enum class state { runnable, blocked, done };
		uint32_t tid = 0;
		state st = state::runnable;
		spike1_cpu_device::context ctx{};
		uint32_t tls = 0;
		uint32_t clear_tid = 0;      // CLONE_CHILD_CLEARTID / set_tid_address: zeroed and woken at exit
		uint64_t sigmask = 0;
		uint64_t sig_pending = 0;    // signals sent to the thread, delivered when it next runs unmasked
		// for each handler running: the registers and signal mask it interrupted
		std::vector<std::pair<spike1_cpu_device::context, uint64_t>> sig_frames;
		std::string name;
		// while blocked
		uint64_t wake_ns = 0;        // 0: no timeout
		bool retry = false;          // re-run the system call when woken, rather than return a result
		int32_t timeout_result = 0;  // r0 when a non-retried block times out
		uint32_t futex_addr = 0;     // non-zero while waiting on a futex
		uint32_t futex_bitset = 0;
		uint64_t poll_deadline = 0;  // a retried poll/select keeps its deadline here
		int read_wait_fd = -1;       // the device a retried read waits on, to log it once
	};

	enum class block_kind { none, result_later, retry };

	spike1_cpu_device &m_cpu;
	spike1_memory &m_mem;
	spike1_devices m_devices;
	spike1_vfs m_vfs;
	config m_cfg;

	status m_status = status::stopped;
	std::string m_stop_reason;
	uint64_t m_cycles = 0;          // cycles of emulated time so far
	uint64_t m_idle_cycles = 0;     // of those, the ones that passed with every thread blocked
	uint64_t m_wall_base_ns = 0;    // host wall clock at start, for CLOCK_REALTIME
	uint64_t m_syscalls = 0;
	uint32_t m_syncs = 0;           // sync() calls: the game ends an NVRAM commit with one

	std::vector<std::unique_ptr<thread>> m_threads;
	thread *m_current = nullptr;
	uint32_t m_next_tid = 101;
	block_kind m_block = block_kind::none;
	bool m_sigreturned = false;     // the system call was a sigreturn: the registers are restored, not a result

	std::map<int, std::shared_ptr<file>> m_fds;
	std::map<std::string, uint32_t> m_symbols;
	std::map<std::string, uint32_t> m_symbol_sizes;
	std::string m_cwd = "/";
	sigaction_entry m_sigactions[65];

	void stop(const std::string &reason);
	void log(const std::string &line) const;
	void on_swi(uint32_t comment);
	int32_t syscall(uint32_t nr, const uint32_t a[7], bool &handled, std::string &note);

	// scheduling
	thread *pick_next();
	void switch_to(thread &t);
	void deliver_signal();
	void wake_due();
	uint64_t next_wake_ns() const;
	// Blocks the running thread until `wake_ns` (0: until woken). With `retry` the system call runs
	// again when the thread wakes; otherwise its result is `timeout_result` or what the waker gives
	void block(uint64_t wake_ns, bool retry, int32_t timeout_result = 0);
	void wake(thread &t, int32_t result);
	int futex_wake(uint32_t addr, uint32_t count, uint32_t bitset);
	int32_t sys_clone(const uint32_t a[7], std::string &note);
	void exit_thread(int32_t code, std::string &note);

	bool load_elf(const std::vector<uint8_t> &elf, uint32_t &entry, uint32_t &phdr, uint32_t &phnum, std::string &error);
	bool build_stack(uint32_t entry, uint32_t phdr, uint32_t phnum, uint32_t &sp, std::string &error);
	void write_kuser_page();

	// paths and files
	std::string guest_absolute(const std::string &path) const;
	bool read_whole(const std::string &guest, std::vector<uint8_t> &out) const;
	bool set_nvram(const std::vector<uint8_t> &block);
	int alloc_fd(std::shared_ptr<file> f, int lowest = 0);
	std::shared_ptr<file> get_fd(int fd) const;
	bool readable_now(int fd) const;

	int32_t sys_open(const std::string &path, uint32_t flags, uint32_t mode, std::string &note);
	int32_t sys_read(int fd, uint32_t buf, uint32_t len, std::string &note);
	int32_t sys_write(int fd, uint32_t buf, uint32_t len, std::string &note);
	int32_t sys_ioctl(int fd, uint32_t req, uint32_t arg, std::string &note);
	int32_t sys_stat(const std::string &guest, uint32_t buf, bool follow);
	int32_t sys_fstat(int fd, uint32_t buf);
	int32_t sys_getdents64(int fd, uint32_t buf, uint32_t len);
	int32_t sys_lseek(int fd, int64_t offset, int whence, int64_t &result);
	int32_t sys_mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags, int fd, uint64_t offset, std::string &note);
	int32_t sys_futex(const uint32_t a[7], std::string &note);
	int32_t sys_poll(uint32_t nr, const uint32_t a[7], std::string &note);
	int32_t write_stat64(uint32_t buf, uint32_t mode, uint64_t size, uint64_t inode);
};
