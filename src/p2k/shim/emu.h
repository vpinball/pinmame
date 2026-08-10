// license:BSD-3-Clause

// PinMAME P2K subsystem - compatibility layer for imported MAME sources
//
// This provides the part of MAME's modern device API that the imported Pinball 2000 code
// actually uses: address spaces, a device tree with timers and callbacks, and no-op stubs for
// save state and the debugger. It is intentionally self-contained - nothing outside src/p2k/
// includes this header, and PinMAME's own core is untouched

#pragma once

// MAME sources use this to detect that the emu core has been included
#define __EMU_H__

#include "emucore.h"
#include "eminline.h"
#include "xtal.h"
#include "attotime.h"

// Optimization for address_space::fast(): 0 = scalar (four compares and four branches), 1 = SSE2, 2 = NEON
#if defined(_M_ARM64EC) || defined(__aarch64__) || defined(_M_ARM64) || defined(__ARM_NEON) || defined(__ARM_NEON__)
  #define P2K_FAST_SIMD_HAVE_NEON 1
#else
  #define P2K_FAST_SIMD_HAVE_NEON 0
#endif
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  #define P2K_FAST_SIMD_HAVE_SSE2 1
#else
  #define P2K_FAST_SIMD_HAVE_SSE2 0
#endif

#ifndef P2K_FAST_SIMD
  #if P2K_FAST_SIMD_HAVE_NEON
    #define P2K_FAST_SIMD 2
  #elif P2K_FAST_SIMD_HAVE_SSE2
    #define P2K_FAST_SIMD 1
  #else // 32-bit x86 without SSE2, ARM without NEON (including MSVC's ARM32, which defines neither __ARM_NEON nor _M_ARM64), anything else. Always correct, just not branchless
    #define P2K_FAST_SIMD 0
  #endif
#endif

#if P2K_FAST_SIMD == 1
  #if !P2K_FAST_SIMD_HAVE_SSE2
    #error "P2K_FAST_SIMD=1 selects the SSE2 backend; build for x64 or with -msse2 / /arch:SSE2"
  #endif
  #include <emmintrin.h>
#elif P2K_FAST_SIMD == 2
  #if !P2K_FAST_SIMD_HAVE_NEON
    #error "P2K_FAST_SIMD=2 selects the NEON backend, which this target does not have"
  #endif
  #include <arm_neon.h>
#endif

#if P2K_FAST_SIMD
static ATTR_FORCE_INLINE unsigned p2k_ctz32(unsigned v)
{
#if defined(_MSC_VER)
	unsigned long i; _BitScanForward(&i, v); return unsigned(i);
#else
	return unsigned(__builtin_ctz(v));
#endif
}
#endif

enum endianness_t { ENDIANNESS_LITTLE, ENDIANNESS_BIG };

#define NATIVE_ENDIAN_VALUE_LE_BE(leval, beval) (leval)
#define WORD_ALIGNED(a)   (((a) & 1) == 0)
#define DWORD_ALIGNED(a)  (((a) & 3) == 0)
#define QWORD_ALIGNED(a)  (((a) & 7) == 0)

class validity_checker;

constexpr int AS_PROGRAM = 0;
constexpr int AS_DATA    = 1;
constexpr int AS_IO      = 2;
constexpr int AS_OPCODES = 3;

constexpr int TRANSLATE_READ  = 0;
constexpr int TRANSLATE_WRITE = 1;
constexpr int TRANSLATE_FETCH = 2;
constexpr int TRANSLATE_TYPE_MASK  = 3;
constexpr int TRANSLATE_USER_MASK  = 4;
constexpr int TRANSLATE_READ_USER  = TRANSLATE_READ | TRANSLATE_USER_MASK;
constexpr int TRANSLATE_WRITE_USER = TRANSLATE_WRITE | TRANSLATE_USER_MASK;
constexpr int TRANSLATE_FETCH_USER = TRANSLATE_FETCH | TRANSLATE_USER_MASK;
constexpr int TRANSLATE_DEBUG_MASK = 8;

constexpr int INPUT_LINE_IRQ0 = 0;
constexpr int MAX_INPUT_LINES = 64;
constexpr int INPUT_LINE_NMI  = 0x20;
constexpr int INPUT_LINE_RESET= 0x21;
constexpr int INPUT_LINE_HALT = 0x22;

enum { CLEAR_LINE = 0, ASSERT_LINE, HOLD_LINE };

// ---------------------------------------------------------------- fwd decls
class device_t;
class device_memory_interface;
class machine_config;
class running_machine;
class address_space;
class device_interface;
class emu_timer;
class memory_share;
class memory_region;

using device_timer_id = int;

namespace util { class disasm_interface; }

// ---------------------------------------------------------------- disasm stub
namespace util {
class disasm_interface
{
public:
	virtual ~disasm_interface() = default;
	class data_buffer
	{
	public:
		virtual ~data_buffer() = default;
		virtual u8  r8 (offs_t pc) const = 0;
		virtual u16 r16(offs_t pc) const = 0;
		virtual u32 r32(offs_t pc) const = 0;
		virtual u64 r64(offs_t pc) const = 0;
	};
	static constexpr u32 SUPPORTED = 0x80000000;
	static constexpr u32 NONLINEAR_PC = 0x40000000;
	static constexpr u32 PAGED = 0x20000000;
	static constexpr u32 PAGED2LEVEL = 0x10000000;
	static constexpr u32 INTERNAL_DECRYPTION = 0x08000000;
	static constexpr u32 SPLIT_DECRYPTION = 0x04000000;
	static constexpr u32 STEP_OUT = 0x00000080;
	static constexpr u32 STEP_OVER = 0x00000040;
	static constexpr u32 STEP_COND = 0x00000020;
	virtual u32 interface_flags() const { return 0; }
	virtual u32 opcode_alignment() const = 0;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) = 0;
};
}

// ---------------------------------------------------------------- address space
class address_space_config final
{
public:
	address_space_config() = default;
	address_space_config(const char *name, endianness_t endian, int datawidth, int addrwidth,
		int addrshift = 0, int logaddrwidth = 0, int pageshift = 0)
		: m_name(name), m_endianness(endian), m_data_width(u8(datawidth)), m_addr_width(u8(addrwidth))
		, m_addr_shift(s8(addrshift)), m_logaddr_width(u8(logaddrwidth ? logaddrwidth : addrwidth))
		, m_page_shift(u8(pageshift)) {}
	template <typename MAP>
	address_space_config(const char *name, endianness_t endian, int datawidth, int addrwidth,
		int addrshift, MAP map)
		: m_name(name), m_endianness(endian), m_data_width(u8(datawidth)), m_addr_width(u8(addrwidth))
		, m_addr_shift(s8(addrshift)), m_logaddr_width(u8(addrwidth)) {}
	int page_shift() const { return m_page_shift; }
	int logaddr_width() const { return m_logaddr_width; }
	int addr_width() const { return m_addr_width; }

	const char *m_name = nullptr;
	endianness_t m_endianness = ENDIANNESS_LITTLE;
	u8 m_data_width = 0;
	u8 m_addr_width = 0;
	s8 m_addr_shift = 0;
	u8 m_logaddr_width = 0;
	u8 m_page_shift = 0;
};

// installed read/write tap handles (debug registers) - no-op here
class memory_passthrough_handler final
{
public:
	void remove() {}
};

// Backing memory is supplied by the host (the P2K machine glue). Like MAME's 32-bit little
// endian bus, every access is presented dword-aligned with a byte-lane mask; the byte and word
// entry points below do the lane shifting, so a handler never sees an unaligned address.
struct p2k_bus_callbacks
{
	u32 (*read32)(void *ctx, offs_t addr, u32 mask);
	void (*write32)(void *ctx, offs_t addr, u32 data, u32 mask);
	void *ctx;
};

class address_space final
{
public:
	explicit address_space(const p2k_bus_callbacks &cb) : m_cb(cb) {}

	// Hotspot for Pin2K emu, so the fast window lookup happens up front and on the byte
	// address itself: a hit is one load, with no dword to assemble, mask or shift back down.
	// Endian-neutral - *p is the byte living at `a` on any host, which is precisely what the
	// load32/shift pair in the fallback computes.
	// The miss path goes straight to the callback and does NOT re-enter through r32(), so a
	// non-fast access looks the fast windows up once, not twice. What buys that is the trimming in
	// add_fast_window(): with a dword-aligned base and a whole number of dwords, a byte or word
	// that misses here cannot be inside the window at its aligned dword address either - see the
	// proof there. Only the masked dword overloads still use r32()/w32(), and those look up once
	ATTR_FORCE_INLINE u8 read_byte(offs_t a)
	{
		if (const u8 *p = fast(a)) return *p;
		const unsigned shift = (a & 3) * 8;
		return u8(m_cb.read32(m_cb.ctx, a & ~3u, 0xffu << shift) >> shift);
	}
	// Same shape as read_byte. The word the dword form would shift out of the bus lives at
	// `a & ~1u` - (a & ~3u) + (a & 2) - so look the window up there and load just those two
	// bytes; a hit puts the offset at span - 2 or less, so both bytes are inside.
	//
	// The full-width overloads here and below are spelled out rather than forwarded to the masked
	// ones with an all-ones mask. Folding `& 0xffff` or a blend against `~mask == 0` away is
	// trivial for an optimizer, but it is only guaranteed once the callee has actually been
	// inlined - and these are hot paths, so they should not depend on it
	ATTR_FORCE_INLINE u16 read_word(offs_t a)
	{
		if (const u8 *p = fast(a & ~1u)) return load16(p);
		const unsigned shift = (a & 2) * 8;
		return u16(m_cb.read32(m_cb.ctx, a & ~3u, 0xffffu << shift) >> shift);
	}
	ATTR_FORCE_INLINE u16 read_word(offs_t a, u16 mask)
	{
		if (const u8 *p = fast(a & ~1u)) return u16(load16(p) & mask);
		const unsigned shift = (a & 2) * 8;
		return u16(m_cb.read32(m_cb.ctx, a & ~3u, u32(mask) << shift) >> shift);
	}
	ATTR_FORCE_INLINE u16 read_word_unaligned(offs_t a) { return read_word(a); }
	ATTR_FORCE_INLINE u16 read_word_unaligned(offs_t a, u16 mask) { return read_word(a, mask); }
	ATTR_FORCE_INLINE u32 read_dword(offs_t a)
	{
		if (const u8 *p = fast(a & ~3u)) return load32(p);
		return m_cb.read32(m_cb.ctx, a & ~3u, 0xffffffff);
	}
	ATTR_FORCE_INLINE u32 read_dword(offs_t a, u32 mask) { return r32(a & ~3u, mask); }
	ATTR_FORCE_INLINE u32 read_dword_unaligned(offs_t a) { return read_dword(a); }
	ATTR_FORCE_INLINE u32 read_dword_unaligned(offs_t a, u32 mask) { return read_dword(a, mask); }

	// The write side of the same idea, and it saves more than the read side does: a hit stores
	// the one byte outright, where w32() had to read the dword back, blend and store it again
	ATTR_FORCE_INLINE void write_byte(offs_t a, u8 d)
	{
		if (u8 *p = fast(a)) { *p = d; return; }
		const unsigned shift = (a & 3) * 8;
		m_cb.write32(m_cb.ctx, a & ~3u, u32(d) << shift, 0xffu << shift);
	}
	ATTR_FORCE_INLINE void write_word(offs_t a, u16 d)
	{
		if (u8 *p = fast(a & ~1u)) { store16(p, d); return; }
		const unsigned shift = (a & 2) * 8;
		m_cb.write32(m_cb.ctx, a & ~3u, u32(d) << shift, 0xffffu << shift);
	}
	ATTR_FORCE_INLINE void write_word(offs_t a, u16 d, u16 mask)
	{
		if (u8 *p = fast(a & ~1u)) { store16(p, u16((load16(p) & u16(~mask)) | (d & mask))); return; }
		const unsigned shift = (a & 2) * 8;
		m_cb.write32(m_cb.ctx, a & ~3u, u32(d) << shift, u32(mask) << shift);
	}
	ATTR_FORCE_INLINE void write_word_unaligned(offs_t a, u16 d) { write_word(a, d); }
	ATTR_FORCE_INLINE void write_word_unaligned(offs_t a, u16 d, u16 mask) { write_word(a, d, mask); }
	// A full-width store needs no read-back to blend into, so it does not go through w32()
	ATTR_FORCE_INLINE void write_dword(offs_t a, u32 d)
	{
		if (u8 *p = fast(a & ~3u)) { store32(p, d); return; }
		m_cb.write32(m_cb.ctx, a & ~3u, d, 0xffffffff);
	}
	ATTR_FORCE_INLINE void write_dword(offs_t a, u32 d, u32 mask) { w32(a & ~3u, d, mask); }
	ATTR_FORCE_INLINE void write_dword_unaligned(offs_t a, u32 d) { write_dword(a, d); }
	ATTR_FORCE_INLINE void write_dword_unaligned(offs_t a, u32 d, u32 mask) { write_dword(a, d, mask); }

	// Register a range that is plain memory. Accesses inside it are served from the buffer
	// instead of going through the callback - which for this machine means skipping an indirect
	// call and a chain of range compares for the accesses that make up most of the traffic.
	// Only valid for ranges with no side effect on access; anything a device watches must not be
	// registered, and neither must anything while a bus probe is armed.
	//
	// The head is trimmed up to the next dword boundary and the tail down to a whole dword. Both
	// are load-bearing rather than tidiness, because fast() is entered with a byte, word OR dword
	// address and the two properties are what make that safe:
	//
	//   in bounds  - with base dword-aligned, an offset keeps the alignment of its address, so a
	//                hit at off means off is 0/2/4-aligned to match the width; with span a whole
	//                number of dwords, off < span then puts the last byte of the access at
	//                off + w - 1 <= span - 1. Without the alignment a dword could hit at off =
	//                span - 2 and read two bytes past the buffer.
	//   miss agrees - if a byte or word misses, the dword covering it misses too: off & ~3 is the
	//                largest multiple of 4 not above off, and span is a multiple of 4, so
	//                off >= span implies off & ~3 >= span. That is what lets the byte and word
	//                paths go straight to the callback instead of looking the window up a second
	//                time through r32()/w32().
	//
	// Trimming does not narrow what the dword path reaches: its offsets are multiples of 4, and
	// off < 4*floor(size/4) is the same set as the old off < size-3
	void add_fast_window(u32 base, u8 *mem, size_t size)
	{
		if (m_fast_n >= MAX_FAST || !mem) return;
		const u32 head = (0u - base) & 3u; // bytes up to the next dword boundary
		if (size <= head) return;
		base += head; mem += head; size -= head;
		size &= ~size_t(3);
		if (size < 4) return;
		const unsigned i = m_fast_n++;
		m_base[i] = base;
		m_span[i] = u32(size);
		m_mem[i]  = mem;
		m_span_biased[i] = u32(size) ^ 0x80000000u; // see the SIMD form in fast()
	}
	// Zeroing the slots is the part that actually disarms them: fast() tests all MAX_FAST entries
	// unconditionally and never consults m_fast_n, so resetting the count alone would leave every
	// registered window still live - exactly the wrong outcome for the bus-probe case above.
	// An empty slot has span 0, and u32(a - 0) < 0 is false for every a; biased, that is the
	// smallest signed value, which no biased offset compares below. So zeroed really is disarmed
	// in both forms.
	void clear_fast_windows()
	{
		for (unsigned i = 0; i < MAX_FAST; i++)
		{
			m_base[i] = 0; m_span[i] = 0; m_mem[i] = nullptr;
			m_span_biased[i] = 0x80000000u;
		}
		m_fast_n = 0;
	}

	static constexpr int data_width() { return 32; }
	template <typename... T> memory_passthrough_handler *install_read_tap(T &&...)      { return nullptr; }
	template <typename... T> memory_passthrough_handler *install_write_tap(T &&...)     { return nullptr; }
	template <typename... T> memory_passthrough_handler *install_readwrite_tap(T &&...) { return nullptr; }
	template <typename T> void cache(T &c) { c.set(this); }
	template <typename T> void specific(T &c) { c.set(this); }
	template <typename... T> int add_change_notifier(T &&...) { return 0; }

private:
	static constexpr unsigned MAX_FAST = 4;

	// The guest is little endian and so is every host this is built for; the byte-wise form is
	// there so a big endian host still produces the same value as the machine's own read_le().
	static ATTR_FORCE_INLINE u32 load32(const u8 * const p)
	{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
		return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
#else
		u32 v; std::memcpy(&v, p, 4); return v;
#endif
	}
	static ATTR_FORCE_INLINE void store32(u8 * const p, u32 v)
	{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
		p[0] = u8(v); p[1] = u8(v >> 8); p[2] = u8(v >> 16); p[3] = u8(v >> 24);
#else
		std::memcpy(p, &v, 4);
#endif
	}
	static ATTR_FORCE_INLINE u16 load16(const u8 * const p)
	{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
		return u16(u16(p[0]) | (u16(p[1]) << 8));
#else
		u16 v; std::memcpy(&v, p, 2); return v;
#endif
	}
	static ATTR_FORCE_INLINE void store16(u8 * const p, u16 v)
	{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
		p[0] = u8(v); p[1] = u8(v >> 8);
#else
		std::memcpy(p, &v, 2);
#endif
	}

	// `a` is dword-aligned from r32()/w32() and a raw byte address from read_byte(). The unsigned
	// subtraction folds the lower bound into the same compare as the upper one. Every registered
	// window has a dword-aligned base, so a byte hit implies a hit on its aligned address too -
	// the byte form never reaches memory the dword form would have left to the callback
	ATTR_FORCE_INLINE u8 *fast(const offs_t a) const
	{
		// This runs for EVERY space, as e.g. the I/O space legitimately has none registered.
		// The count is not what makes the unrolled form below safe anyway: unregistered slots stay
		// zeroed, and u32(a - 0) < 0 is false for every a, so an empty slot can never match
		static_assert(MAX_FAST == 4);

#if P2K_FAST_SIMD
		// All four windows tested at once, branchlessly. The LOWEST set bit wins, which is the same first-match rule the
		// scalar form follows by falling through in order. Windows are allowed to overlap, so which
		// one wins has to stay the same across all three forms
  #if P2K_FAST_SIMD == 1
		// SSE2 has no unsigned compare, so both sides are biased into signed space. The spans are
		// stored pre-biased (add_fast_window does it) to keep the xor off the hot path
		const __m128i va  = _mm_set1_epi32(int(a));
		const __m128i d   = _mm_sub_epi32(va, _mm_load_si128(reinterpret_cast<const __m128i *>(m_base)));
		const __m128i hit = _mm_cmplt_epi32(_mm_xor_si128(d, _mm_set1_epi32(int(0x80000000u))),
		                                    _mm_load_si128(reinterpret_cast<const __m128i *>(m_span_biased)));
		const unsigned mask = unsigned(_mm_movemask_ps(_mm_castsi128_ps(hit)));
  #else
		// NEON has a native unsigned compare, so it uses the plain spans and needs no bias. It has
		// no movemask either: AND the all-ones lanes with {1,2,4,8} and OR the four lanes together.
		// vorr/vget_lane are ARMv7-NEON as well as AArch64, so this does not need vaddvq_u32
		static const u32 k_lane_bits[MAX_FAST] = { 1u, 2u, 4u, 8u };
		const uint32x4_t d   = vsubq_u32(vdupq_n_u32(u32(a)), vld1q_u32(m_base));
		const uint32x4_t hit = vcltq_u32(d, vld1q_u32(m_span));
		const uint32x4_t bits = vandq_u32(hit, vld1q_u32(k_lane_bits));
		const uint32x2_t pair = vorr_u32(vget_low_u32(bits), vget_high_u32(bits));
		const unsigned mask = vget_lane_u32(pair, 0) | vget_lane_u32(pair, 1);
  #endif
		if (!mask) return nullptr;
		const unsigned i = p2k_ctz32(mask);
		return m_mem[i] + (a - m_base[i]);
#else
		if (u32(a - m_base[0]) < m_span[0]) return m_mem[0] + (a - m_base[0]);
		if (u32(a - m_base[1]) < m_span[1]) return m_mem[1] + (a - m_base[1]);
		if (u32(a - m_base[2]) < m_span[2]) return m_mem[2] + (a - m_base[2]);
		if (u32(a - m_base[3]) < m_span[3]) return m_mem[3] + (a - m_base[3]);
		return nullptr;
#endif
	}

	ATTR_FORCE_INLINE u32 r32(offs_t a, u32 mask)
	{
		if (const u8 *p = fast(a)) return load32(p) & mask;
		return m_cb.read32(m_cb.ctx, a, mask);
	}
	ATTR_FORCE_INLINE void w32(offs_t a, u32 d, u32 mask)
	{
		if (u8 *p = fast(a)) { store32(p, (load32(p) & ~mask) | (d & mask)); return; }
		m_cb.write32(m_cb.ctx, a, d, mask);
	}

	// Windows of plain backing store that the space may reach directly, skipping the callback and
	// the machine's range decode behind it. Only for ranges that are pure memory: no side effect on
	// access, no device behind them. The machine registers these; see address_space::add_fast_window.

	// Structure of arrays, not an array of structs, and the compare operands first. The eight
	// values fast() actually tests then sit in 32 contiguous bytes instead of being interleaved
	// with the pointers it only needs once it has a hit - and the whole table is one aligned
	// cache line. It is also the layout P2K_FAST_SIMD needs: one aligned load per operand vector
	//
	// m_cb goes last: it is only touched on a miss.
	alignas(64) u32 m_base[MAX_FAST] {};
	u32 m_span[MAX_FAST] {};
	// The same spans biased into signed space, for the SSE2 backend only - the scalar and NEON
	// forms compare m_span directly, NEON having a native unsigned compare. It is kept
	// unconditionally rather than behind the backend switch so that the class layout does not
	// depend on P2K_FAST_SIMD; 16 bytes is worth not having an ODR hazard.
	//
	// NOT zero-initialised: an empty slot must be the smallest signed value so nothing compares
	// below it, and a zeroed entry here would instead match every address under 0x80000000 and
	// return m_mem[i] == nullptr. Kept in step with m_span by add_fast_window()/clear_fast_windows().
	u32 m_span_biased[MAX_FAST] { 0x80000000u, 0x80000000u, 0x80000000u, 0x80000000u };
	u8 *m_mem[MAX_FAST] {};
	unsigned m_fast_n = 0;
	p2k_bus_callbacks m_cb;
};

enum read_or_write { ROW_READ = 1, ROW_WRITE = 2, ROW_READWRITE = 3 };


#define FUNC(x) &x, #x

// memory_access<>::cache / ::specific replacement (opcode fetch path)
template <int AddrWidth, int DataShift, int AddrShift, endianness_t Endian>
struct memory_access
{
	class cache
	{
	public:
		void set(address_space *space) { m_space = space; }
		ATTR_FORCE_INLINE u8  read_byte (offs_t a) { return m_space->read_byte(a); }
		ATTR_FORCE_INLINE u16 read_word (offs_t a) { return m_space->read_word(a); }
		ATTR_FORCE_INLINE u32 read_dword(offs_t a) { return m_space->read_dword(a); }
	private:
		address_space *m_space = nullptr;
	};
	class specific final : public cache {};
};

// ---------------------------------------------------------------- device types
class device_type_base
{
public:
	constexpr device_type_base(const char *shortname, const char *fullname)
		: m_shortname(shortname), m_fullname(fullname) {}
	const char *shortname() const { return m_shortname; }
	const char *fullname() const { return m_fullname; }
private:
	const char *m_shortname;
	const char *m_fullname;
};

using device_type = const device_type_base &;

template <class DeviceClass>
class device_type_impl final : public device_type_base
{
public:
	constexpr device_type_impl(const char *shortname, const char *fullname)
		: device_type_base(shortname, fullname) {}

	// PIT8253(config, "pit", 0) - create the device and hang it under the current owner
	template <typename... Params>
	DeviceClass &operator()(machine_config &config, const char *tag, Params &&... args) const;

	// AT_KEYB(config, m_keyboard_dev, ...) - same, but tag and target come from a finder
	template <class Finder, typename... Params>
	DeviceClass &operator()(machine_config &config, Finder &finder, Params &&... args) const
	{
		DeviceClass &dev = (*this)(config, finder.finder_tag(), std::forward<Params>(args)...);
		finder.assign(&dev);
		return dev;
	}
};

#define DECLARE_DEVICE_TYPE(Type, Class) \
	class Class; \
	extern device_type_impl<Class> const Type;
#define DEFINE_DEVICE_TYPE(Type, Class, ShortName, FullName) \
	device_type_impl<Class> const Type(ShortName, FullName);

// ---------------------------------------------------------------- scheduler
// A timer callback bound to a device method; constructed as
// timer_expired_delegate(FUNC(cls::method), this), which expands to (&method, "name", this).
class timer_expired_delegate final
{
public:
	timer_expired_delegate() = default;
	template <typename T>
	timer_expired_delegate(void (T::*f)(void *, s32), const char *, T *obj)
		: m_func([obj, f](void *ptr, s32 param) { (obj->*f)(ptr, param); }) {}
	template <typename F> timer_expired_delegate(F &&f) : m_func(std::forward<F>(f)) {}

	explicit operator bool() const { return bool(m_func); }
	void operator()(void *ptr, s32 param) const { if (m_func) m_func(ptr, param); }

private:
	std::function<void (void *, s32)> m_func;
};

class emu_timer final
{
	friend class device_scheduler;
public:
	void adjust(attotime start_delay, s32 param = 0, attotime period = attotime::never);
	void reset() { adjust(attotime::never); }
	void enable(bool enable = true) { m_enabled = enable; }
	bool enabled() const { return m_enabled; }
	attotime expire() const { return m_expire; }
	attotime elapsed() const;
	attotime remaining() const;
	s32 param() const { return m_param; }
	void set_param(s32 param) { m_param = param; }

	device_t *m_device = nullptr;
	device_timer_id m_id = 0;
	void *m_ptr = nullptr;
	s32 m_param = 0;
	attotime m_expire = attotime::never;
	attotime m_period = attotime::never;
	bool m_enabled = false;
	bool m_temporary = false;
	bool m_in_use = false;
	timer_expired_delegate m_callback;
};

class device_scheduler final
{
public:
	attotime time() const { return m_time; }

	// MAME's scheduler-level allocation takes the callback and no device
	emu_timer *timer_alloc(timer_expired_delegate cb)
	{
		emu_timer *t = timer_alloc(nullptr, 0, nullptr);
		t->m_callback = std::move(cb);
		return t;
	}

	emu_timer *timer_alloc(device_t *device, device_timer_id id, void *ptr)
	{
		for (auto &t : m_timers)
			if (!t->m_in_use) { *t = emu_timer(); t->m_in_use = true; t->m_device = device; t->m_id = id; t->m_ptr = ptr; return t.get(); }
		m_timers.push_back(std::make_unique<emu_timer>());
		emu_timer *t = m_timers.back().get();
		t->m_in_use = true; t->m_device = device; t->m_id = id; t->m_ptr = ptr;
		return t;
	}

	void timer_set(device_t *device, attotime duration, device_timer_id id, s32 param, void *ptr)
	{
		emu_timer *t = timer_alloc(device, id, ptr);
		t->m_temporary = true;
		t->adjust(duration, param);
	}

	// earliest pending expiry, or never
	attotime next_expiry() const
	{
		attotime best = attotime::never;
		for (auto &t : m_timers)
			if (t->m_in_use && t->m_enabled && t->m_expire < best) best = t->m_expire;
		return best;
	}

	// advance time to `target`, firing every timer that comes due on the way
	void advance_to(attotime target);

	void set_time(attotime t) { m_time = t; }

private:
	std::vector<std::unique_ptr<emu_timer>> m_timers;
	attotime m_time;
};

// ---------------------------------------------------------------- machine
class save_manager final
{
public:
	template <typename... T> void register_postload(T &&...) {}
	template <typename... T> void save_item(T &&...) {}
};

class running_machine final
{
public:
	u32 debug_flags = 0;
	int side_effects_disabled() const { return 0; }
	void debug_break() {}
	std::string describe_context() const { return std::string("p2k"); }
	u32 rand() { m_rand_seed = m_rand_seed * 1664525 + 1013904223; return m_rand_seed; }
	attotime time() const { return m_scheduler.time(); }
	void base_datetime(struct system_time &systime);
	device_scheduler &scheduler() { return m_scheduler; }
	save_manager &save() { return m_save; }
private:
	u32 m_rand_seed = 0x12345678;
	device_scheduler m_scheduler;
	save_manager m_save;
};

#define STRUCT_MEMBER(s, m) nullptr
struct save_prepost_delegate { template <typename... T> save_prepost_delegate(T &&...) {} };

struct p2k_symtable { template <typename... T> void add(T &&...) {} };
struct p2k_debug_iface { p2k_symtable &symtable() { static p2k_symtable s; return s; } };

// The device tree. Devices are created through device_type_impl::operator() and owned here.
class machine_config final
{
public:
	device_t *current_owner = nullptr;
	std::vector<std::unique_ptr<device_t>> devices;

	device_t *find(const std::string &fulltag) const;
	void add(std::unique_ptr<device_t> dev) { devices.push_back(std::move(dev)); }
};

// Input ports come from PinMAME, not from MAME's ioport system. The port definitions in the
// imported devices are parsed away into empty functions.
class ioport_list;
using ioport_constructor = void (*)(device_t &owner, ioport_list &portlist, std::string &errorbuf);

#define INPUT_PORTS_START(name) \
	void P2K_IOPORTS_##name(device_t &, ioport_list &, std::string &) {
#define INPUT_PORTS_END }
#define INPUT_PORTS_NAME(name) (&P2K_IOPORTS_##name)
#define PORT_START(tag)
#define PORT_BIT(mask, default_value, type)
#define PORT_NAME(name)
#define PORT_CODE(code)
#define PORT_PLAYER(player)
#define PORT_MINMAX(min, max)
#define PORT_SENSITIVITY(sensitivity)
#define PORT_KEYDELTA(delta)
#define PORT_RESET

// ---------------------------------------------------------------- interfaces
class device_interface
{
public:
	device_interface(device_t &device, const char *type) : m_device(device) {}
	virtual ~device_interface() = default;
	virtual void interface_validity_check(validity_checker &) const {}
	virtual void interface_pre_start() {}
	virtual void interface_post_start() {}
	virtual void interface_pre_reset() {}
	virtual void interface_post_reset() {}
	device_t &device() { return m_device; }
	const device_t &device() const { return m_device; }
protected:
	device_t &m_device;
};

class device_resolver
{
public:
	virtual ~device_resolver() = default;
	virtual void resolve(device_t &owner) = 0;
};

// MAME's common base for device/memory finders
class finder_base : public device_resolver
{
public:
	// tag used by an unconfigured finder
	static constexpr const char *DUMMY_TAG = "finder_dummy_tag";
};

class device_t
{
public:
	device_t(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
		: m_type(&type), m_owner(owner), m_clock(clock)
	{
		m_basetag = tag ? tag : "";
		m_tag = (owner && *owner->tag()) ? std::string(owner->tag()) + ":" + m_basetag : m_basetag;
	}
	virtual ~device_t() = default;

	const char *tag() const { return m_tag.c_str(); }
	const char *basetag() const { return m_basetag.c_str(); }
	const char *shortname() const { return m_type->shortname(); }
	const char *name() const { return m_type->fullname(); }
	device_t *owner() const { return m_owner; }
	running_machine &machine() const { return *s_machine; }
	u32 clock() const { return m_clock; }
	void set_unscaled_clock(u32 clock, bool = false) { m_clock = clock; notify_clock_changed(); }
	void set_clock(u32 clock) { set_unscaled_clock(clock); }
	void notify_clock_changed() { device_clock_changed(); }
	attotime clocks_to_attotime(u64 clocks) const
	{ return m_clock ? attotime::from_ticks(clocks, double(m_clock)) : attotime::never; }

	void logerror(const char *fmt, ...) const
	{
		va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
	}

	template <typename... T> void save_item(T &&...) const {}
	template <typename... T> void save_pointer(T &&...) const {}

	// timers
	emu_timer *timer_alloc(device_timer_id id = 0, void *ptr = nullptr)
	{ return machine().scheduler().timer_alloc(this, id, ptr); }
	emu_timer *timer_alloc(timer_expired_delegate cb)
	{ return machine().scheduler().timer_alloc(std::move(cb)); }
	void timer_set(attotime duration, device_timer_id id = 0, s32 param = 0, void *ptr = nullptr)
	{ machine().scheduler().timer_set(this, duration, id, param, ptr); }
	void synchronize(device_timer_id id = 0, s32 param = 0, void *ptr = nullptr)
	{ timer_set(attotime::zero, id, param, ptr); }

	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) {}
	virtual ioport_constructor device_input_ports() const { return nullptr; }

	// sub-device lookup, by tag relative to this device
	device_t *subdevice_any(const char *tag) const;
	template <class T> T *subdevice(const char *tag) const
	{ return dynamic_cast<T *>(subdevice_any(tag)); }

	device_memory_interface &memory() const;

	// memory shares are owned by the machine glue; devices that look one up get nothing
	memory_share *memshare(const char *tag) const { return nullptr; }

	p2k_debug_iface *debug() const { static p2k_debug_iface d; return &d; }

	// lifecycle, driven by p2k_machine (see p2k_machine.h)
	// finders first, then the device's own resolve step - MAME does it in this order
	void p2k_resolve()  { for (auto r : m_resolvers) r->resolve(*this); device_resolve_objects(); }
	void p2k_start()    { p2k_start_interfaces(); device_start(); m_started = true; }
	bool started() const { return m_started; }
	bool configured() const { return m_started; }
	// MAME's suspend/trigger mechanism; nothing here suspends, so a trigger is a no-op
	void trigger(int trigid) {}
	void p2k_reset()    { device_reset(); }
	void p2k_add_config(machine_config &config) { device_add_mconfig(config); }
	void p2k_register_resolver(device_resolver *r) { m_resolvers.push_back(r); }
	virtual void p2k_start_interfaces() {}

	static running_machine *s_machine;

protected:
	virtual void device_start() {}
	virtual void device_reset() {}
	virtual void device_resolve_objects() {}
	virtual void device_add_mconfig(machine_config &config) {}
	virtual void device_clock_changed() {}
	virtual void device_debug_setup() {}
	virtual void device_config_complete() {}
	virtual void device_post_load() {}

	const device_type_base *m_type;
	device_t *m_owner;
	std::string m_tag;
	std::string m_basetag;
	u32 m_clock;
	std::vector<device_resolver *> m_resolvers;
	bool m_started = false;
};

#define NAME(x) x

// MAME's merge-with-mask helper, as used by device write handlers
#define ACCESSING_BITS_0_7    ((mem_mask & 0x000000ffU) != 0)
#define ACCESSING_BITS_8_15   ((mem_mask & 0x0000ff00U) != 0)
#define ACCESSING_BITS_16_23  ((mem_mask & 0x00ff0000U) != 0)
#define ACCESSING_BITS_24_31  ((mem_mask & 0xff000000U) != 0)
#define ACCESSING_BITS_0_15   ((mem_mask & 0x0000ffffU) != 0)
#define ACCESSING_BITS_16_31  ((mem_mask & 0xffff0000U) != 0)
#define ACCESSING_BITS_0_31   ((mem_mask & 0xffffffffU) != 0)
#define ACCESSING_BITS_32_63  ((mem_mask & 0xffffffff00000000ULL) != 0)

#define COMBINE_DATA(varptr) (*(varptr) = (*(varptr) & ~mem_mask) | (data & mem_mask))

// save-state type registration - no-op here


// ---------------------------------------------------------------- nvram / time
// MAME persists NVRAM through emu_file; here the interface exists so that devices with NVRAM
// compile and initialise, and the machine glue reads/writes the contents itself.
class emu_file final
{
public:
	u32 read(void *buffer, u32 length) { return 0; }
	u32 write(const void *buffer, u32 length) { return length; }
};

class device_nvram_interface : public device_interface
{
public:
	device_nvram_interface(const machine_config &mconfig, device_t &device)
		: device_interface(device, "nvram") {}
	void p2k_nvram_default() { nvram_default(); }
	void p2k_nvram_read(emu_file &file) { nvram_read(file); }
	void p2k_nvram_write(emu_file &file) { nvram_write(file); }
	bool p2k_nvram_can_write() { return nvram_can_write(); }
protected:
	virtual void nvram_default() = 0;
	virtual void nvram_read(emu_file &file) = 0;
	virtual void nvram_write(emu_file &file) = 0;
	virtual bool nvram_can_write() { return true; }
};

// MAME's wall-clock snapshot, as used by RTC devices
struct system_time
{
	struct full_time
	{
		u8 second = 0, minute = 0, hour = 0, mday = 1, month = 0, weekday = 0;
		u16 year = 1980;
		u16 day = 0;
		bool is_dst = false;
	};
	s64 time = 0;
	full_time local_time;
	full_time utc_time;
};

// memory shares and regions are supplied by the machine glue, not by MAME's ROM loader
class memory_share final
{
public:
	memory_share(void *ptr, u32 bytes) : m_ptr(ptr), m_bytes(bytes) {}
	void *ptr() const { return m_ptr; }
	u32 bytes() const { return m_bytes; }
private:
	void *m_ptr;
	u32 m_bytes;
};

class memory_region final
{
public:
	memory_region(u8 *base, u32 bytes) : m_base(base), m_bytes(bytes) {}
	u8 *base() const { return m_base; }
	u32 bytes() const { return m_bytes; }
private:
	u8 *m_base;
	u32 m_bytes;
};

#define DEVICE_SELF ""

class optional_memory_region final
{
public:
	optional_memory_region(device_t &, const char *) {}
	bool found() const { return false; }
	memory_region *operator->() const { return nullptr; }
	operator memory_region *() const { return nullptr; }
};


// The device currently being configured; MAME resolves .set(FUNC(cls::method)) against it.
device_t *p2k_config_owner();

// ---------------------------------------------------------------- devcb
// MAME's callbacks are configured through .bind()/.set(...) in a machine config and then
// resolved. Here a callback is just a std::function with a default value.
template <typename Ret, typename... Args>
class devcb_base
{
public:
	devcb_base(device_t &) {}

	template <typename F> devcb_base &set(F &&f) { m_func = std::forward<F>(f); return *this; }
	template <typename T, typename F> devcb_base &set(T *obj, F f)
	{ m_func = [obj, f](Args... a) -> Ret { return (obj->*f)(a...); }; return *this; }
	template <typename T, typename F> devcb_base &set(T &obj, F f)
	{ m_func = [&obj, f](Args... a) -> Ret { return (obj.*f)(a...); }; return *this; }
	// member function of the device being configured (MAME's FUNC(...) form)
	template <typename T> devcb_base &set(Ret (T::*f)(Args...), const char * = nullptr)
	{
		T *obj = static_cast<T *>(p2k_config_owner());
		m_func = [obj, f](Args... a) -> Ret { return (obj->*f)(a...); };
		return *this;
	}
	devcb_base &operator=(std::function<Ret(Args...)> f) { m_func = std::move(f); return *this; }

	void resolve() {}
	void resolve_safe() { }
	void resolve_safe(Ret def) { m_default = def; }
	bool isnull() const { return !m_func; }

	Ret operator()(Args... args) const
	{ return m_func ? m_func(args...) : m_default; }

	// MAME's accessors are declared `auto handler() { return m_cb.bind(); }`, so whatever bind()
	// returns gets copied. It must therefore refer back to the callback, not be one.
	class binder final
	{
	public:
		explicit binder(devcb_base *cb) : m_cb(cb) {}
		template <typename F> binder &set(F &&f) { m_cb->set(std::forward<F>(f)); return *this; }
		template <typename T, typename F> binder &set(T *obj, F f) { m_cb->set(obj, f); return *this; }
		template <typename T, typename F> binder &set(T &obj, F f) { m_cb->set(obj, f); return *this; }
		template <typename T, typename F> binder &set(F T::*f, const char *n = nullptr) { m_cb->set(f, n); return *this; }
		template <typename F> binder &operator=(F &&f) { m_cb->set(std::forward<F>(f)); return *this; }
	private:
		devcb_base *m_cb;
	};
	binder bind() { return binder(this); }

	template <typename Self, unsigned N> class array_of
	{
	public:
		array_of(device_t &dev) { for (unsigned i = 0; i < N; i++) m_cb.emplace_back(dev); }
		Self &operator[](unsigned i) { return m_cb[i]; }
		const Self &operator[](unsigned i) const { return m_cb[i]; }
		void resolve_all_safe(Ret def) { for (auto &c : m_cb) c.resolve_safe(def); }
		void resolve_all_safe() { }
	private:
		std::vector<Self> m_cb;
	};
	template <unsigned N> using array = array_of<devcb_base, N>;

private:
	std::function<Ret(Args...)> m_func;
	Ret m_default = Ret();
};

// void-returning specialisation (write callbacks)
template <typename... Args>
class devcb_base<void, Args...>
{
public:
	devcb_base(device_t &) {}
	template <typename F> devcb_base &set(F &&f) { m_func = std::forward<F>(f); return *this; }
	template <typename T, typename F> devcb_base &set(T *obj, F f)
	{ m_func = [obj, f](Args... a) { (obj->*f)(a...); }; return *this; }
	template <typename T, typename F> devcb_base &set(T &obj, F f)
	{ m_func = [&obj, f](Args... a) { (obj.*f)(a...); }; return *this; }
	// member function of the device being configured (MAME's FUNC(...) form)
	template <typename T> devcb_base &set(void (T::*f)(Args...), const char * = nullptr)
	{
		T *obj = static_cast<T *>(p2k_config_owner());
		m_func = [obj, f](Args... a) { (obj->*f)(a...); };
		return *this;
	}
	devcb_base &operator=(std::function<void(Args...)> f) { m_func = std::move(f); return *this; }
	void resolve() {}
	void resolve_safe() {}
	void resolve_safe(int) {}
	bool isnull() const { return !m_func; }
	void operator()(Args... args) const { if (m_func) m_func(args...); }

	// MAME's accessors are declared `auto handler() { return m_cb.bind(); }`, so whatever bind()
	// returns gets copied. It must therefore refer back to the callback, not be one.
	class binder final
	{
	public:
		explicit binder(devcb_base *cb) : m_cb(cb) {}
		template <typename F> binder &set(F &&f) { m_cb->set(std::forward<F>(f)); return *this; }
		template <typename T, typename F> binder &set(T *obj, F f) { m_cb->set(obj, f); return *this; }
		template <typename T, typename F> binder &set(T &obj, F f) { m_cb->set(obj, f); return *this; }
		template <typename T, typename F> binder &set(F T::*f, const char *n = nullptr) { m_cb->set(f, n); return *this; }
		template <typename F> binder &operator=(F &&f) { m_cb->set(std::forward<F>(f)); return *this; }
	private:
		devcb_base *m_cb;
	};
	binder bind() { return binder(this); }

	template <typename Self, unsigned N> class array_of
	{
	public:
		array_of(device_t &dev) { for (unsigned i = 0; i < N; i++) m_cb.emplace_back(dev); }
		Self &operator[](unsigned i) { return m_cb[i]; }
		const Self &operator[](unsigned i) const { return m_cb[i]; }
		void resolve_all_safe(int = 0) {}
	private:
		std::vector<Self> m_cb;
	};
	template <unsigned N> using array = array_of<devcb_base, N>;

private:
	std::function<void(Args...)> m_func;
};

using devcb_write_line = devcb_base<void, int>;
using devcb_read_line  = devcb_base<int>;

// Read/write callbacks carry an offset and a mask in MAME, but are frequently called without
// them. These wrappers add the defaulted arguments the device sources expect.
template <typename DataType>
class devcb_read_data final : public devcb_base<DataType, offs_t>
{
public:
	using devcb_base<DataType, offs_t>::devcb_base;
	DataType operator()(offs_t offset = 0) const { return devcb_base<DataType, offs_t>::operator()(offset); }

	template <unsigned N> using array =
		typename devcb_base<DataType, offs_t>::template array_of<devcb_read_data<DataType>, N>;
};

template <typename DataType>
class devcb_write_data final : public devcb_base<void, offs_t, DataType, DataType>
{
	using base = devcb_base<void, offs_t, DataType, DataType>;
public:
	using base::base;
	void operator()(offs_t offset, DataType data, DataType mem_mask = DataType(~DataType(0))) const
	{ base::operator()(offset, data, mem_mask); }
	void operator()(DataType data) const { base::operator()(0, data, DataType(~DataType(0))); }

	template <unsigned N> using array = typename base::template array_of<devcb_write_data<DataType>, N>;
};

using devcb_write8  = devcb_write_data<u8>;
using devcb_read8   = devcb_read_data<u8>;
using devcb_write16 = devcb_write_data<u16>;
using devcb_read16  = devcb_read_data<u16>;
using devcb_write32 = devcb_write_data<u32>;
using devcb_read32  = devcb_read_data<u32>;

#define DECLARE_WRITE_LINE_MEMBER(name)  void name(int state)
#define WRITE_LINE_MEMBER(name)          void name(int state)
#define DECLARE_READ_LINE_MEMBER(name)   int name()
#define READ_LINE_MEMBER(name)           int name()
#define IRQ_CALLBACK_MEMBER(name)        int name(device_t &device, int irqline)
#define TIMER_CALLBACK_MEMBER(name)      void name(void *ptr, s32 param)


// ---------------------------------------------------------------- delegates
// MAME's device_delegate: a callback bound to a device method, configured with .set(FUNC(...)).
// Here it is a std::function plus the same set() shapes the device sources use.
template <typename Signature> class device_delegate;

template <typename Ret, typename... Args>
class device_delegate<Ret (Args...)> final
{
public:
	device_delegate() = default;
	device_delegate(device_t &) {}
	template <typename... T> device_delegate(device_t &, T &&...) {}

	template <typename F> void set(F &&f) { m_func = std::forward<F>(f); }
	template <typename T, typename F> void set(T &obj, F f, const char * = nullptr)
	{ m_func = [&obj, f](Args... a) -> Ret { return (obj.*f)(a...); }; }
	template <typename T, typename F> void set(T *obj, F f, const char * = nullptr)
	{ m_func = [obj, f](Args... a) -> Ret { return (obj->*f)(a...); }; }
	template <typename F> void set(F &&f, const char *) { m_func = std::forward<F>(f); }

	template <typename... T> device_delegate &operator=(T &&...) { return *this; }

	void resolve() {}
	bool isnull() const { return !m_func; }
	explicit operator bool() const { return bool(m_func); }
	Ret operator()(Args... args) const { return m_func ? m_func(args...) : Ret(); }

	// device_delegate<...>::array<N>, as used for per-slot callbacks
	template <unsigned N> class array
	{
	public:
		template <typename... T> array(T &&...) {}
		device_delegate &operator[](unsigned i) { return m_d[i]; }
		const device_delegate &operator[](unsigned i) const { return m_d[i]; }
		void resolve_all() {}
	private:
		device_delegate m_d[N];
	};

private:
	std::function<Ret (Args...)> m_func;
};

template <typename... Args>
class device_delegate<void (Args...)> final
{
public:
	device_delegate() = default;
	device_delegate(device_t &) {}
	template <typename... T> device_delegate(device_t &, T &&...) {}
	template <typename F> void set(F &&f) { m_func = std::forward<F>(f); }
	template <typename T, typename F> void set(T &obj, F f, const char * = nullptr)
	{ m_func = [&obj, f](Args... a) { (obj.*f)(a...); }; }
	template <typename T, typename F> void set(T *obj, F f, const char * = nullptr)
	{ m_func = [obj, f](Args... a) { (obj->*f)(a...); }; }
	template <typename F> void set(F &&f, const char *) { m_func = std::forward<F>(f); }
	template <typename... T> device_delegate &operator=(T &&...) { return *this; }
	void resolve() {}
	bool isnull() const { return !m_func; }
	explicit operator bool() const { return bool(m_func); }
	void operator()(Args... args) const { if (m_func) m_func(args...); }

	// device_delegate<...>::array<N>, as used for per-slot callbacks
	template <unsigned N> class array
	{
	public:
		template <typename... T> array(T &&...) {}
		device_delegate &operator[](unsigned i) { return m_d[i]; }
		const device_delegate &operator[](unsigned i) const { return m_d[i]; }
		void resolve_all() {}
	private:
		device_delegate m_d[N];
	};

private:
	std::function<void (Args...)> m_func;
};

// optional counterpart to required_device; unset tags simply stay unresolved
template <class DeviceClass>
class optional_device final : public finder_base
{
public:
	optional_device(device_t &owner) : m_owner(&owner) { owner.p2k_register_resolver(this); }
	optional_device(device_t &owner, const char *tag) : m_owner(&owner), m_tag(tag ? tag : "")
	{ owner.p2k_register_resolver(this); }
	template <typename T> void set_tag(T &&tag) { m_tag = tag; }
	void resolve(device_t &owner) override
	{ if (!m_target && !m_tag.empty()) m_target = owner.subdevice<DeviceClass>(m_tag.c_str()); }

	const char *finder_tag() const { return m_tag.c_str(); }
	void assign(DeviceClass *dev) { m_target = dev; }

	DeviceClass *operator->() const { return m_target; }
	DeviceClass &operator*() const { return *m_target; }
	operator DeviceClass *() const { return m_target; }
	bool found() const { return m_target != nullptr; }
private:
	device_t *m_owner;
	std::string m_tag;
	DeviceClass *m_target = nullptr;
};

// input ports are supplied by PinMAME, not by MAME's ioport system
class ioport_port final
{
public:
	u32 read() { return 0; }
};

template <bool Required>
class ioport_finder final : public finder_base
{
public:
	ioport_finder(device_t &owner, const char *tag) { owner.p2k_register_resolver(this); }
	void resolve(device_t &) override {}
	ioport_port *operator->() const { static ioport_port p; return &p; }
	bool found() const { return false; }
	u32 read_safe(u32 defval) const { return defval; }
};
using required_ioport = ioport_finder<true>;
using optional_ioport = ioport_finder<false>;

// ---------------------------------------------------------------- device finders
template <class DeviceClass>
class required_device final : public finder_base
{
public:
	required_device(device_t &owner, const char *tag) : m_tag(tag)
	{ owner.p2k_register_resolver(this); }
	template <typename T>
	required_device(device_t &owner, const char *fmt, T index)
	{ char buf[64]; snprintf(buf, sizeof(buf), fmt, unsigned(index)); m_tag = buf; owner.p2k_register_resolver(this); }

	void resolve(device_t &owner) override
	{ if (!m_target) m_target = owner.subdevice<DeviceClass>(m_tag.c_str()); }

	const char *finder_tag() const { return m_tag.c_str(); }
	void assign(DeviceClass *dev) { m_target = dev; }

	DeviceClass *operator->() const { return m_target; }
	DeviceClass &operator*() const { return *m_target; }
	operator DeviceClass *() const { return m_target; }
	bool found() const { return m_target != nullptr; }

private:
	std::string m_tag;
	DeviceClass *m_target = nullptr;
};

template <class DeviceClass, unsigned Count>
class required_device_array final
{
public:
	template <typename T>
	required_device_array(device_t &owner, const char *fmt, T start)
	{
		for (unsigned i = 0; i < Count; i++)
			m_devices.push_back(std::make_unique<required_device<DeviceClass>>(owner, fmt, unsigned(start) + i));
	}
	required_device<DeviceClass> &operator[](unsigned i) { return *m_devices[i]; }
	const required_device<DeviceClass> &operator[](unsigned i) const { return *m_devices[i]; }
private:
	std::vector<std::unique_ptr<required_device<DeviceClass>>> m_devices;
};

// ---------------------------------------------------------------- exec interface
class device_execute_interface : public device_interface
{
public:
	device_execute_interface(const machine_config &mconfig, device_t &device)
		: device_interface(device, "execute") {}

	virtual u32 execute_min_cycles() const noexcept { return 1; }
	virtual u32 execute_max_cycles() const noexcept { return 1; }
	virtual u32 execute_input_lines() const noexcept { return 0; }
	virtual bool execute_input_edge_triggered(int inputnum) const noexcept { return false; }
	virtual void execute_run() = 0;
	virtual void execute_set_input(int inputnum, int state) {}

	// MAME's public way to drive an interrupt line
	void set_input_line(int line, int state) { execute_set_input(line, state); }
	void set_input_line_and_vector(int line, int state, int vector) { execute_set_input(line, state); }

	void set_icountptr(int &icount) { m_icountptr = &icount; }
	void eat_cycles(int c) { if (m_icountptr) *m_icountptr -= c; }
	template <typename T> void pulse_input_line(int line, T &&)
	{ execute_set_input(line, ASSERT_LINE); execute_set_input(line, CLEAR_LINE); }
	int standard_irq_callback(int irqline) { return m_irq_vector_cb ? m_irq_vector_cb(irqline) : 0; }
	int standard_irq_callback(int irqline, offs_t pc) { return m_irq_vector_cb ? m_irq_vector_cb(irqline) : 0; }
	void set_irq_acknowledge_callback(std::function<int(int)> cb) { m_irq_vector_cb = std::move(cb); }
	u64 cycles_to_attotime(u64 c) const { return c; }
	void suspend_until_trigger(int, bool) {}
	void spin_until_trigger(int) {}
	// end the current run() at the next instruction boundary - what a debugger halt needs
	void abort_timeslice() { if (m_icountptr) *m_icountptr = 0; }

	// run for `cycles` and report how many were actually consumed
	int run(int cycles)
	{
		m_icount = cycles;
		if (m_icountptr) *m_icountptr = cycles;
		execute_run();
		int left = m_icountptr ? *m_icountptr : 0;
		int used = cycles - left;
		m_total_cycles += used;
		return used;
	}

	int m_icount = 0;
protected:
	int *m_icountptr = nullptr;
	u64 m_total_cycles = 0;
	std::function<int(int)> m_irq_vector_cb;
};

// ---------------------------------------------------------------- memory interface
class device_memory_interface : public device_interface
{
public:
	using space_config_vector = std::vector<std::pair<int, const address_space_config *>>;

	device_memory_interface(const machine_config &mconfig, device_t &device)
		: device_interface(device, "memory") {}

	virtual space_config_vector memory_space_config() const = 0;
	virtual bool memory_translate(int spacenum, int intention, offs_t &address) { return true; }

	const address_space_config *space_config(int index = 0) const
	{
		for (auto &e : memory_space_config()) if (e.first == index) return e.second;
		return nullptr;
	}
	bool translate(int spacenum, int intention, offs_t &address)
	{ return memory_translate(spacenum, intention, address); }
	address_space &space(int index = 0) const { return *m_spaces[index]; }
	bool has_space(int index = 0) const { return index < int(m_spaces.size()) && m_spaces[index]; }
	void p2k_set_space(int index, address_space *sp)
	{
		if (int(m_spaces.size()) <= index) m_spaces.resize(index + 1, nullptr);
		m_spaces[index] = sp;
	}
private:
	mutable std::vector<address_space *> m_spaces;
};

// ---------------------------------------------------------------- state interface
// A CPU register (or any other exported value) as registered with state_add(). MAME keeps a
// pointer to the underlying variable; so does this, which is what lets a debugger read and
// write registers without the CPU core knowing about it.
class device_state_entry final
{
public:
	device_state_entry() = default;
	device_state_entry(int index, const char *symbol, void *ptr, u8 size)
		: m_index(index), m_symbol(symbol ? symbol : ""), m_ptr(ptr), m_size(size) {}

	device_state_entry &callimport() { m_callimport = true; return *this; }
	device_state_entry &callexport() { m_callexport = true; return *this; }
	device_state_entry &formatstr(const char *fmt) { m_format = fmt ? fmt : ""; return *this; }
	device_state_entry &noshow() { m_noshow = true; return *this; }
	device_state_entry &mask(u64 m) { m_mask = m; return *this; }

	int index() const { return m_index; }
	const char *symbol() const { return m_symbol.c_str(); }
	const char *format() const { return m_format.c_str(); }
	bool is_visible() const { return !m_noshow; }
	bool needs_import() const { return m_callimport; }
	bool needs_export() const { return m_callexport; }
	bool has_storage() const { return m_ptr != nullptr; }

	u64 value() const
	{
		if (!m_ptr) return 0;
		u64 v = 0;
		switch (m_size)
		{
			case 1: v = *reinterpret_cast<u8 *>(m_ptr); break;
			case 2: v = *reinterpret_cast<u16 *>(m_ptr); break;
			case 4: v = *reinterpret_cast<u32 *>(m_ptr); break;
			case 8: v = *reinterpret_cast<u64 *>(m_ptr); break;
		}
		return v & m_mask;
	}

	void set_value(u64 v) const
	{
		if (!m_ptr) return;
		v &= m_mask;
		switch (m_size)
		{
			case 1: *reinterpret_cast<u8 *>(m_ptr)  = u8(v);  break;
			case 2: *reinterpret_cast<u16 *>(m_ptr) = u16(v); break;
			case 4: *reinterpret_cast<u32 *>(m_ptr) = u32(v); break;
			case 8: *reinterpret_cast<u64 *>(m_ptr) = v;      break;
		}
	}

private:
	int m_index = 0;
	std::string m_symbol;
	std::string m_format;
	void *m_ptr = nullptr;
	u8 m_size = 0;
	bool m_callimport = false;
	bool m_callexport = false;
	bool m_noshow = false;
	u64 m_mask = ~u64(0);
};

class device_state_interface : public device_interface
{
public:
	device_state_interface(const machine_config &mconfig, device_t &device)
		: device_interface(device, "state") {}

	// the form the CPU cores use: a reference to the variable holding the register
	template <typename T>
	std::enable_if_t<std::is_integral<T>::value, device_state_entry &>
	state_add(int index, const char *symbol, T &value)
	{
		m_entries.emplace_back(index, symbol, &value, u8(sizeof(T)));
		return m_entries.back();
	}

	// anything not backed by a plain variable is registered without storage: it still shows up,
	// but reading it goes through state_export()/state_import() or yields zero
	template <typename T>
	std::enable_if_t<!std::is_integral<std::remove_reference_t<T>>::value, device_state_entry &>
	state_add(int index, const char *symbol, T &&)
	{
		m_entries.emplace_back(index, symbol, nullptr, 0);
		return m_entries.back();
	}

	virtual void state_import(const device_state_entry &entry) {}
	virtual void state_export(const device_state_entry &entry) {}
	virtual void state_string_export(const device_state_entry &entry, std::string &str) const {}

	// --- access for the host (debugger, PinMAME's cpuintrf bridge) ---
	const std::deque<device_state_entry> &state_entries() const { return m_entries; }

	const device_state_entry *state_find(int index) const
	{
		for (const auto &e : m_entries) if (e.index() == index) return &e;
		return nullptr;
	}

	u64 state_int(int index)
	{
		const device_state_entry *e = state_find(index);
		if (!e) return 0;
		if (e->needs_export()) state_export(*e);
		return e->value();
	}

	void set_state_int(int index, u64 value)
	{
		const device_state_entry *e = state_find(index);
		if (!e) return;
		e->set_value(value);
		if (e->needs_import()) state_import(*e);
	}

	std::string state_string(int index)
	{
		const device_state_entry *e = state_find(index);
		if (!e) return std::string();
		std::string custom;
		state_string_export(*e, custom);
		if (!custom.empty()) return custom;
		char buf[32];
		snprintf(buf, sizeof(buf), *e->format() ? e->format() : "%08X", u32(state_int(index)));
		return std::string(buf);
	}

private:
	// deque, not vector: state_add() hands out a reference that the caller chains on, and later
	// registrations must not invalidate it
	std::deque<device_state_entry> m_entries;
};

// ---------------------------------------------------------------- disasm interface
class device_disasm_interface : public device_interface
{
public:
	device_disasm_interface(const machine_config &mconfig, device_t &device)
		: device_interface(device, "disasm") {}
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() = 0;
	// public entry point: the override in a CPU core is usually protected
	std::unique_ptr<util::disasm_interface> p2k_disassembler() { return create_disassembler(); }
};

// ---------------------------------------------------------------- cpu_device
class cpu_device : public device_t,
	public device_execute_interface,
	public device_memory_interface,
	public device_state_interface,
	public device_disasm_interface
{
public:
	cpu_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
		: device_t(mconfig, type, tag, owner, clock)
		, device_execute_interface(mconfig, *this)
		, device_memory_interface(mconfig, *this)
		, device_state_interface(mconfig, *this)
		, device_disasm_interface(mconfig, *this)
	{}

	void p2k_start_interfaces() override
	{
		device_execute_interface::interface_pre_start();
		device_memory_interface::interface_pre_start();
		device_state_interface::interface_pre_start();
		device_disasm_interface::interface_pre_start();
	}
};

// state index constants used by CPU cores
enum { STATE_GENPC = -1, STATE_GENPCBASE = -2, STATE_GENSP = -3, STATE_GENFLAGS = -4 };

// ---------------------------------------------------------------- out-of-line
inline void running_machine::base_datetime(system_time &systime)
{
	time_t t = ::time(nullptr);
	systime.time = s64(t);
	struct tm lt = *localtime(&t), ut = *gmtime(&t);
	auto fill = [](system_time::full_time &ft, const struct tm &src)
	{
		ft.second = u8(src.tm_sec); ft.minute = u8(src.tm_min); ft.hour = u8(src.tm_hour);
		ft.mday = u8(src.tm_mday); ft.month = u8(src.tm_mon); ft.weekday = u8(src.tm_wday);
		ft.year = u16(src.tm_year + 1900); ft.day = u16(src.tm_yday); ft.is_dst = src.tm_isdst > 0;
	};
	fill(systime.local_time, lt);
	fill(systime.utc_time, ut);
}

inline device_memory_interface &device_t::memory() const
{
	return *const_cast<device_memory_interface *>(dynamic_cast<const device_memory_interface *>(this));
}

inline void emu_timer::adjust(attotime start_delay, s32 param, attotime period)
{
	m_param = param;
	m_period = period;
	if (start_delay.is_never())
	{
		m_expire = attotime::never;
		m_enabled = false;
	}
	else
	{
		m_expire = device_t::s_machine->time() + start_delay;
		m_enabled = true;
	}
}

inline attotime emu_timer::elapsed() const
{ return device_t::s_machine->time() - (m_expire - m_period); }

inline attotime emu_timer::remaining() const
{
	attotime now = device_t::s_machine->time();
	return (m_expire > now) ? m_expire - now : attotime::zero;
}

inline void device_scheduler::advance_to(attotime target)
{
	for (;;)
	{
		emu_timer *next = nullptr;
		for (auto &t : m_timers)
			if (t->m_in_use && t->m_enabled && (!next || t->m_expire < next->m_expire)) next = t.get();
		if (!next || next->m_expire > target) break;

		m_time = next->m_expire;
		device_t *dev = next->m_device;
		device_timer_id id = next->m_id;
		s32 param = next->m_param;
		void *ptr = next->m_ptr;

		if (next->m_period.is_never())
		{
			next->m_enabled = false;
			next->m_expire = attotime::never;
			if (next->m_temporary) next->m_in_use = false;
		}
		else
		{
			next->m_expire += next->m_period;
		}
		if (next->m_callback) next->m_callback(ptr, param);
		else if (dev) dev->device_timer(*next, id, param, ptr);
	}
	if (m_time < target) m_time = target;
}

inline device_t *device_t::subdevice_any(const char *tag) const
{
	extern machine_config *p2k_active_config;
	if (!p2k_active_config) return nullptr;
	std::string full = m_tag.empty() ? std::string(tag) : m_tag + ":" + tag;
	return p2k_active_config->find(full);
}

inline device_t *machine_config::find(const std::string &fulltag) const
{
	for (auto &d : devices) if (fulltag == d->tag()) return d.get();
	return nullptr;
}

template <class DeviceClass>
template <typename... Params>
DeviceClass &device_type_impl<DeviceClass>::operator()(machine_config &config, const char *tag, Params &&... args) const
{
	auto dev = std::make_unique<DeviceClass>(config, tag, config.current_owner, std::forward<Params>(args)...);
	DeviceClass &ref = *dev;
	config.add(std::move(dev));

	device_t *prev = config.current_owner;
	config.current_owner = &ref;
	ref.p2k_add_config(config);
	config.current_owner = prev;
	return ref;
}

// ---------------------------------------------------------------- misc helpers



#include "divtlb.h"
