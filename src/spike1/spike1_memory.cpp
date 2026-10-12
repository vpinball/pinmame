// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the guest's flat 32-bit address space. See spike1_memory.h

#include "spike1_memory.h"

#include "emu.h"

#include <algorithm>
#include <cstring>

#if defined(_WIN32)
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
#else
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <unistd.h>
#endif

// Reserved and committed in one go, but the host only backs a page with RAM once it is touched,
// so the large low block costs what the guest actually uses
uint8_t *spike1_memory::allocate(size_t size)
{
#if defined(_WIN32)
	return static_cast<uint8_t *>(VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
	void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return p == MAP_FAILED ? nullptr : static_cast<uint8_t *>(p);
#endif
}

void spike1_memory::release(uint8_t *mem, size_t size)
{
	if (!mem) return;
#if defined(_WIN32)
	(void)size;
	VirtualFree(mem, 0, MEM_RELEASE);
#else
	munmap(mem, size);
#endif
}

spike1_memory::spike1_memory()
{
	m_low   = { 0, LOW_SIZE, allocate(LOW_SIZE) };
	m_stack = { STACK_TOP - STACK_SIZE, STACK_SIZE, allocate(STACK_SIZE) };
	m_kuser = { KUSER_BASE, KUSER_SIZE, allocate(KUSER_SIZE) };
	m_free.push_back({ MMAP_BASE, LOW_SIZE - MMAP_BASE });
	m_view_free.push_back({ FILE_BASE, FILE_LIMIT - FILE_BASE });
}

spike1_memory::~spike1_memory()
{
	release(m_low.mem, m_low.size);
	release(m_stack.mem, m_stack.size);
	release(m_kuser.mem, m_kuser.size);
	close_map(m_file_map);
	for (view &v : m_views) close_map(v.map);
}

void spike1_memory::attach(address_space &space)
{
	m_space = &space;
	space.clear_fast_windows();
	for (const block *b : { &m_low, &m_stack, &m_kuser })
		if (b->mem) space.add_fast_window(b->base, b->mem, b->size);
	// read-only: a guest write there reaches unmapped_write() and stops the machine, as the
	// game's own write to its read-only mapping would end it on the board
	if (m_file.mem) space.add_fast_window(m_file.base, m_file.mem, m_file.size, false);
}

uint8_t *spike1_memory::host(uint32_t addr, uint32_t len, bool for_write)
{
	for (block *b : { &m_low, &m_stack, &m_kuser, &m_file }) {
		if (!b->mem || (for_write && b == &m_file)) continue;
		const uint32_t off = addr - b->base;
		if (off < b->size && len <= b->size - off)
			return b->mem + off;
	}
	if (!for_write)
		for (view &v : m_views) {
			const uint32_t off = addr - v.b.base;
			if (off < v.b.size && len <= v.b.size - off)
				return v.b.mem + off;
		}
	return nullptr;
}

bool spike1_memory::read_view(uint32_t addr, uint32_t &value) const
{
	for (const view &v : m_views) {
		const uint32_t off = addr - v.b.base;
		if (off >= v.b.size) continue;
		value = 0;
		std::memcpy(&value, v.b.mem + off, std::min<uint32_t>(4, v.b.size - off));
		return true;
	}
	return false;
}

// Whether the asset image can go at FILE_BASE: the views are kept to the top of the space
bool spike1_memory::place_file(uint32_t length)
{
	if (m_file.mem || !length || length > FILE_LIMIT - FILE_BASE) return false;
	const uint32_t pages = uint32_t((uint64_t(length) + GUEST_PAGE_SIZE - 1) & ~uint64_t(GUEST_PAGE_SIZE - 1));
	return !m_view_free.empty() && m_view_free.front().base == FILE_BASE && m_view_free.front().size >= pages;
}

// The host's read-only mapping of [offset, offset + length) of a file; null when it cannot map it
uint8_t *spike1_memory::open_map(const std::string &host_path, uint64_t offset, uint32_t length, host_map &map)
{
#if defined(_WIN32)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	const uint64_t granularity = si.dwAllocationGranularity;
	const uint64_t start = offset - offset % granularity;
	const size_t view_size = size_t(offset - start) + length;
	const int wide = MultiByteToWideChar(CP_UTF8, 0, host_path.c_str(), -1, nullptr, 0);
	std::wstring wpath(size_t(wide), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, host_path.c_str(), -1, wpath.data(), wide);
	HANDLE file = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) return nullptr;
	HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CloseHandle(file); // the mapping keeps the file open
	if (!mapping) return nullptr;
	void *view = MapViewOfFile(mapping, FILE_MAP_READ, DWORD(start >> 32), DWORD(start), view_size);
	if (!view) { CloseHandle(mapping); return nullptr; }
	map.handle = mapping;
#else
	const uint64_t start = offset - offset % GUEST_PAGE_SIZE;
	const size_t view_size = size_t(offset - start) + length;
	const int fd = open(host_path.c_str(), O_RDONLY);
	if (fd < 0) return nullptr;
	void *view = mmap(nullptr, view_size, PROT_READ, MAP_PRIVATE, fd, off_t(start));
	close(fd);
	if (view == MAP_FAILED) return nullptr;
#endif
	map.view = view;
	map.size = view_size;
	return static_cast<uint8_t *>(view) + (offset - start);
}

void spike1_memory::close_map(host_map &map)
{
	if (!map.view) return;
#if defined(_WIN32)
	UnmapViewOfFile(map.view);
	CloseHandle(map.handle);
#else
	munmap(map.view, map.size);
#endif
	map = host_map();
}

uint32_t spike1_memory::map_file(const std::string &host_path, uint64_t offset, uint32_t length)
{
	if (!length) return 0;
	host_map map;
	const uint8_t *data = open_map(host_path, offset, length, map);
	if (!data) return 0;
	const uint32_t base = place(data, length, map);
	if (!base) close_map(map);
	return base;
}

uint32_t spike1_memory::map_view(const uint8_t *data, uint32_t length)
{
	return data && length ? place(data, length, host_map()) : 0;
}

// The file range when it is free (the CPU has one fast window left for it), else a view of its
// own from the top of the space; the block owns `map` from here
uint32_t spike1_memory::place(const uint8_t *data, uint32_t length, const host_map &map)
{
	const uint32_t pages = uint32_t((uint64_t(length) + GUEST_PAGE_SIZE - 1) & ~uint64_t(GUEST_PAGE_SIZE - 1));
	if (place_file(length)) {
		m_file = { FILE_BASE, length, const_cast<uint8_t *>(data) }; // host() never hands it out for writing
		m_file_map = map;
		take_span(m_view_free, FILE_BASE, pages);
		if (m_space) m_space->add_fast_window(m_file.base, m_file.mem, m_file.size, false);
		return m_file.base;
	}
	for (auto it = m_view_free.rbegin(); pages && it != m_view_free.rend(); ++it) {
		if (it->size < pages) continue;
		const uint32_t base = it->base + it->size - pages; // from the top, clear of the asset image's place
		take_span(m_view_free, base, pages);
		m_views.push_back({ { base, length, const_cast<uint8_t *>(data) }, map });
		return base;
	}
	return 0;
}

bool spike1_memory::read(uint32_t addr, void *dst, uint32_t len)
{
	if (!len) return true;
	const uint8_t *p = host(addr, len, false);
	if (!p) return false;
	std::memcpy(dst, p, len);
	return true;
}

bool spike1_memory::write(uint32_t addr, const void *src, uint32_t len)
{
	if (!len) return true;
	uint8_t *p = host(addr, len);
	if (!p) return false;
	std::memcpy(p, src, len);
	return true;
}

bool spike1_memory::read_string(uint32_t addr, std::string &out, size_t limit)
{
	out.clear();
	for (size_t i = 0; i < limit; i++) {
		const uint8_t *p = host(addr + uint32_t(i), 1, false);
		if (!p) return false;
		if (!*p) return true;
		out.push_back(char(*p));
	}
	return false;
}

uint32_t spike1_memory::brk(uint32_t request)
{
	// Linux answers an impossible request with the current break rather than an error
	if (request < m_brk_base || request > MMAP_BASE)
		return m_brk;
	const uint32_t end = (request + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1);
	const uint32_t old_end = (m_brk + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1);
	if (end < old_end) // shrinking: the pages read back as zero when they are grown again
		std::memset(m_low.mem + end, 0, old_end - end);
	m_brk = request;
	return m_brk;
}

uint32_t spike1_memory::map(uint32_t length)
{
	length = (length + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1);
	if (!length) return 0;
	for (auto it = m_free.begin(); it != m_free.end(); ++it) {
		if (it->size < length) continue;
		const uint32_t addr = it->base;
		it->base += length;
		it->size -= length;
		if (!it->size) m_free.erase(it);
		std::memset(m_low.mem + addr, 0, length);
		return addr;
	}
	return 0;
}

void spike1_memory::unmap(uint32_t addr, uint32_t length)
{
	for (auto v = m_views.begin(); v != m_views.end(); ++v)
		if (v->b.base == addr) {
			release_span(m_view_free, addr, uint32_t((uint64_t(v->b.size) + GUEST_PAGE_SIZE - 1) & ~uint64_t(GUEST_PAGE_SIZE - 1)));
			close_map(v->map);
			m_views.erase(v);
			return;
		}
	length = (length + GUEST_PAGE_SIZE - 1) & ~(GUEST_PAGE_SIZE - 1);
	if (addr < MMAP_BASE || addr >= LOW_SIZE || !length || length > LOW_SIZE - addr)
		return;
	release_span(m_free, addr, length);
}

// Removes [addr, addr + length) from the span that holds it, if one does
bool spike1_memory::take_span(std::vector<span> &list, uint32_t addr, uint32_t length)
{
	for (auto it = list.begin(); it != list.end(); ++it) {
		if (addr < it->base || addr - it->base > it->size || length > it->size - (addr - it->base)) continue;
		const span after = { addr + length, it->base + it->size - (addr + length) };
		it->size = addr - it->base;
		if (!it->size) it = list.erase(it);
		else ++it;
		if (after.size) list.insert(it, after);
		return true;
	}
	return false;
}

void spike1_memory::release_span(std::vector<span> &list, uint32_t addr, uint32_t length)
{
	auto it = std::lower_bound(list.begin(), list.end(), addr,
		[](const span &s, uint32_t a) { return s.base < a; });
	it = list.insert(it, { addr, length });
	// merge with the neighbours so the area does not fragment into page-sized holes
	if (it + 1 != list.end() && it->base + it->size == (it + 1)->base) {
		it->size += (it + 1)->size;
		list.erase(it + 1);
	}
	if (it != list.begin() && (it - 1)->base + (it - 1)->size == it->base) {
		(it - 1)->size += it->size;
		list.erase(it);
	}
}
