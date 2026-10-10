// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the guest's flat 32-bit address space
//
// A Spike 1 game is a Linux user-mode program, so there is no board memory map to model: the
// guest sees RAM where the program, its heap, its mmap()s and its stack live, plus the page of
// helper code the ARM kernel maps at 0xffff0000. Each of those is one block of host memory and one
// fast window of the CPU's address space; anything else is a fault

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class address_space;

class spike1_memory
{
public:
	// The low block holds the program image from 0x8000, the brk heap after it and the mmap area
	static constexpr uint32_t LOW_SIZE    = 0x20000000;
	static constexpr uint32_t MMAP_BASE   = 0x08000000;
	static constexpr uint32_t STACK_TOP   = 0xbf000000;
	static constexpr uint32_t STACK_SIZE  = 0x00800000;
	static constexpr uint32_t KUSER_BASE  = 0xffff0000;
	static constexpr uint32_t KUSER_SIZE  = 0x00001000;
	static constexpr uint32_t GUEST_PAGE_SIZE   = 0x1000;
	// A large file the game maps read-only (a Spike 1 game's asset image: 1.9 GB for Heavy Metal)
	// is mapped by the host straight into the space between the low block and the stack, from its
	// start, so it is paged in on demand rather than copied
	static constexpr uint32_t FILE_BASE   = LOW_SIZE;
	static constexpr uint32_t FILE_LIMIT  = STACK_TOP - STACK_SIZE;
	// Further read-only views (WWE's LCD clips, mapped one by one as they play) take that space
	// from its top down. The CPU has no fast window left for them: its reads there come through
	// read_view()

	spike1_memory();
	~spike1_memory();
	spike1_memory(const spike1_memory &) = delete;
	spike1_memory &operator=(const spike1_memory &) = delete;

	// Registers the blocks as fast windows of `space`
	void attach(address_space &space);

	// Host address of [addr, addr + len), or null when the range is not inside one block. A range
	// inside the read-only file view is only handed out when `for_write` is false
	uint8_t *host(uint32_t addr, uint32_t len, bool for_write = true);

	bool read(uint32_t addr, void *dst, uint32_t len);
	bool write(uint32_t addr, const void *src, uint32_t len);
	bool read32(uint32_t addr, uint32_t &value) { return read(addr, &value, 4); }
	bool write32(uint32_t addr, uint32_t value) { return write(addr, &value, 4); }
	bool write64(uint32_t addr, uint64_t value) { return write(addr, &value, 8); }
	// A NUL-terminated guest string of at most `limit` bytes
	bool read_string(uint32_t addr, std::string &out, size_t limit = 4096);

	// brk heap, growing up from the end of the program image
	void set_brk_base(uint32_t base) { m_brk_base = m_brk = (base + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1); }
	uint32_t brk(uint32_t request);

	// Anonymous memory from the mmap area, zero-filled; 0 when it is exhausted
	uint32_t map(uint32_t length);
	void unmap(uint32_t addr, uint32_t length);

	// Maps `length` bytes of a host file from `offset` read-only into the file range and returns
	// the guest address; 0 when the range is taken or the host cannot map the file. One file
	// for now: the CPU's address space has one fast window left for it
	uint32_t map_file(const std::string &host_path, uint64_t offset, uint32_t length);
	// The same for bytes the host already holds in memory; they must stay put while mapped. The
	// first takes the file range, later ones a view of their own until unmap()
	uint32_t map_view(const uint8_t *data, uint32_t length);
	// A dword the CPU reads outside its fast windows: from a view (bytes past the view's end read
	// as zero). False when no view holds addr
	bool read_view(uint32_t addr, uint32_t &value) const;

private:
	struct block { uint32_t base; uint32_t size; uint8_t *mem; };
	block m_low{}, m_stack{}, m_kuser{}, m_file{};
	address_space *m_space = nullptr;
	// the host view behind m_file: its start can sit below m_file.mem for offset alignment
	void *m_file_view = nullptr;
	size_t m_file_view_size = 0;
	void *m_file_handle = nullptr;

	uint32_t m_brk_base = 0, m_brk = 0;
	struct span { uint32_t base, size; };
	std::vector<span> m_free; // free mmap spans, sorted by address
	std::vector<block> m_views;
	std::vector<span> m_view_free; // free address space for views, sorted by address
	static void release_span(std::vector<span> &list, uint32_t addr, uint32_t length);
	static bool take_span(std::vector<span> &list, uint32_t addr, uint32_t length);
	bool place_file(uint32_t length);

	static uint8_t *allocate(size_t size);
	static void release(uint8_t *mem, size_t size);
};
