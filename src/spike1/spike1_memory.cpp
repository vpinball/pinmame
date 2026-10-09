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
	m_view_free.push_back({ VIEW_LOW_BASE, FILE_BASE - VIEW_LOW_BASE });
	m_view_free.push_back({ VIEW_HIGH_BASE, STACK_TOP - STACK_SIZE - VIEW_HIGH_BASE });
}

spike1_memory::~spike1_memory()
{
	release(m_low.mem, m_low.size);
	release(m_stack.mem, m_stack.size);
	release(m_kuser.mem, m_kuser.size);
#if defined(_WIN32)
	if (m_file_view) UnmapViewOfFile(m_file_view);
	if (m_file_handle) CloseHandle(m_file_handle);
#else
	if (m_file_view) munmap(m_file_view, m_file_view_size);
#endif
}

void spike1_memory::attach(address_space &space)
{
	m_space = &space;
	space.clear_fast_windows();
	for (const block *b : { &m_low, &m_stack, &m_kuser, &m_file })
		if (b->mem) space.add_fast_window(b->base, b->mem, b->size);
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
		for (block &v : m_views) {
			const uint32_t off = addr - v.base;
			if (off < v.size && len <= v.size - off)
				return v.mem + off;
		}
	return nullptr;
}

bool spike1_memory::read_view(uint32_t addr, uint32_t &value) const
{
	for (const block &v : m_views) {
		const uint32_t off = addr - v.base;
		if (off >= v.size) continue;
		value = 0;
		std::memcpy(&value, v.mem + off, std::min<uint32_t>(4, v.size - off));
		return true;
	}
	return false;
}

uint32_t spike1_memory::map_file(const std::string &host_path, uint64_t offset, uint32_t length)
{
	if (m_file.mem || !length || length > FILE_LIMIT - FILE_BASE) return 0;
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
	if (file == INVALID_HANDLE_VALUE) return 0;
	HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CloseHandle(file); // the mapping keeps the file open
	if (!mapping) return 0;
	void *view = MapViewOfFile(mapping, FILE_MAP_READ, DWORD(start >> 32), DWORD(start), view_size);
	if (!view) { CloseHandle(mapping); return 0; }
	m_file_handle = mapping;
#else
	const uint64_t start = offset - offset % PAGE_SIZE;
	const size_t view_size = size_t(offset - start) + length;
	const int fd = open(host_path.c_str(), O_RDONLY);
	if (fd < 0) return 0;
	void *view = mmap(nullptr, view_size, PROT_READ, MAP_PRIVATE, fd, off_t(start));
	close(fd);
	if (view == MAP_FAILED) return 0;
#endif
	m_file_view = view;
	m_file_view_size = view_size;
	m_file = { FILE_BASE, length, static_cast<uint8_t *>(view) + (offset - start) };
	if (m_space) m_space->add_fast_window(m_file.base, m_file.mem, m_file.size);
	return m_file.base;
}

uint32_t spike1_memory::map_view(const uint8_t *data, uint32_t length)
{
	if (!data || !length) return 0;
	if (!m_file.mem && length <= FILE_LIMIT - FILE_BASE) {
		m_file = { FILE_BASE, length, const_cast<uint8_t *>(data) }; // host() never hands it out for writing
		if (m_space) m_space->add_fast_window(m_file.base, m_file.mem, m_file.size);
		return m_file.base;
	}
	const uint32_t pages = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	for (auto it = m_view_free.begin(); pages && it != m_view_free.end(); ++it) {
		if (it->size < pages) continue;
		const uint32_t base = it->base;
		it->base += pages;
		it->size -= pages;
		if (!it->size) m_view_free.erase(it);
		m_views.push_back({ base, length, const_cast<uint8_t *>(data) });
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
	const uint32_t end = (request + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	const uint32_t old_end = (m_brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	if (end < old_end) // shrinking: the pages read back as zero when they are grown again
		std::memset(m_low.mem + end, 0, old_end - end);
	m_brk = request;
	return m_brk;
}

uint32_t spike1_memory::map(uint32_t length)
{
	length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
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
		if (v->base == addr) {
			release_span(m_view_free, addr, (v->size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
			m_views.erase(v);
			return;
		}
	length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	if (addr < MMAP_BASE || addr >= LOW_SIZE || !length || length > LOW_SIZE - addr)
		return;
	release_span(m_free, addr, length);
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
