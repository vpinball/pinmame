// license:BSD-3-Clause

// PinMAME P2K subsystem - the Pinball 2000 machine. See p2k_driver.h for provenance

#include "p2k_driver.h"
#include "p2k_debug.h"
#include "p2k_weak.h"
#include "p2k_public.h"

// p2k_state::nvram_block is the subsystem's spelling of the block selector; P2K_NV_BLOCK_* is the
// one src/wpc/p2k.c passes across. Renumber either and the build stops here, rather than silently
// saving the EEPROM over the CMOS
static_assert(int(p2k_state::NVRAM_CMOS)   == P2K_NV_BLOCK_CMOS,   "NVRAM block selectors disagree");
static_assert(int(p2k_state::NVRAM_EEPROM) == P2K_NV_BLOCK_EEPROM, "NVRAM block selectors disagree");

// How to get the MediaGX PCI bring-up to report success, which the boot code demands before it
// will go on - without one of these, games halt at "[ PRISM BOARD NOT PRESENT ]".
//
//   1  the default: MAME's patch, the routine's already-run exit returns 1 instead of -7
//   2  remove that guard instead, so every call re-enumerates and re-programs the BARs
//   0  neither, for seeing the failure raw
//
// Keep 1. Mode 2 was tried and is worse: Revenge From Mars does not boot under it at all, in any
// version, while Episode I gets no further than it does under 1. p2k_state::set_prism_roms has the
// detail, including what mode 2 was for and why it is still worth having around.
#ifndef P2K_PATCH_PCI_INIT_RETRY
#define P2K_PATCH_PCI_INIT_RETRY 1
#endif

// Sense of the lamp status readback (registers 0x10/0x11). 0 echoes the row latches, so the bit the
// game is driving reads back set; 1 inverts them.
// Confirmed on both machines: with this the games' own single-lamp tests pass.
//
// Arrived at with P2K_PDBWATCH rather than from the buffer's part number. The test walks one row bit at a time - 0x01, 0x02, 0x04 ... 0x80 - and
// reads 0x10 and 0x11 after each. Both readings that produced "short" left the bit being driven
// CLEAR: the constant 0x00 this used to return, and an inverted echo (0xfe, 0xfd, 0xfb ...).
// Echoing un-inverted is what sets it
#ifndef P2K_LAMP_STATUS_INVERT
#define P2K_LAMP_STATUS_INVERT 0
#endif

// Build the CMOS error log's header on a machine whose CMOS has never been written, the way the
// game itself would have built it on an earlier power-up. Without it the newer system software
// cannot boot from a blank CMOS - see seed_error_log() for the failure chain, and the README
#ifndef P2K_SEED_ERROR_LOG
#define P2K_SEED_ERROR_LOG 1
#endif

extern u64 g_p2k_cycles_total;
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/am9517a.h"
#include "machine/mc146818.h"
#include "machine/8042kbdc.h"

#include <cstdio>
#include <cstdlib>
#include <string>

// counters for the interrupt path, reported with P2K_PROGRESS: how many rising edges the PIT
// puts on IR0, and how often the PIC asserts INT at the CPU. With the CPU's own dispatch count
// from the trap hook, the three together say where a tick is multiplied or lost.
u64 g_p2k_pit0_edges = 0;
u64 g_p2k_pic_int = 0;

// what the PIC currently wants from the CPU. The clkint gate may hold it back, so the line the
// CPU sees is this AND "not inside the clock handler" - see p2k_cpuintrf.cpp.
int g_p2k_pic_int_state = 0;

void p2k_apply_irq0()
{
	extern p2k_state *p2k_current_state();
	if (p2k_state *st = p2k_current_state()) st->apply_irq0();
}

namespace {
	// display controller registers, by dword index
	constexpr unsigned DC_TIMING_CFG = 0x08 / 4;
	constexpr unsigned DC_OUTPUT_CFG = 0x0c / 4;
	constexpr unsigned DC_FB_ST_OFFSET = 0x10 / 4;
	constexpr unsigned DC_LINE_DELTA = 0x24 / 4;
	constexpr unsigned DC_H_TIMING_1 = 0x30 / 4;
	constexpr unsigned DC_V_TIMING_1 = 0x40 / 4;
	constexpr unsigned DC_V_LINE_CNT = 0x54 / 4;
	constexpr unsigned VIDEO_LINES = 525;              // 640x480 with blanking
	constexpr unsigned VIDEO_FRAMES_PER_SECOND = 60;
	// graphics pipeline registers, by dword index
	constexpr unsigned GP_DST         = 0x00 / 4;
	constexpr unsigned GP_WIDTH       = 0x04 / 4;
	constexpr unsigned GP_SRC_X       = 0x08 / 4;
	constexpr unsigned GP_RASTER_MODE = 0x100 / 4;
	constexpr unsigned GP_VECTOR_MODE = 0x104 / 4;
	constexpr unsigned GP_BLT_MODE    = 0x108 / 4;
	constexpr unsigned GP_BLT_STATUS  = 0x10c / 4;
} // anonymous namespace

// P2K_WRITEMAP=1: count every write into 1 MB buckets and print the busiest at the PPM trigger.
// "Where does it draw" is otherwise a guessing game over a 4 GB address space.
// the DCS2 sound board lives on PinMAME's side; these are defined in src/wpc/p2k.c
extern "C" u32 p2k_dcs_read(u32 offset, u32 mem_mask) P2K_WEAK;
extern "C" void p2k_dcs_write(u32 offset, u32 data, u32 mem_mask) P2K_WEAK;

// One MediaGX/Prism ROM bank: two 8 MB chips interleaved as 32-bit words
static constexpr size_t   PRISM_BANK_BYTES = 0x1000000;
static constexpr size_t   PRISM_BANK_WORDS = PRISM_BANK_BYTES / 4;   // 0x400000
static constexpr unsigned PRISM_BANK_SHIFT = 22;
static constexpr size_t   PRISM_BANK_MASK  = PRISM_BANK_WORDS - 1;
// Both of these are what lets the wrap be an AND. If the bank size ever stops being a power of
// two this must fail to build rather than silently fold addresses on top of each other
static_assert((PRISM_BANK_WORDS & PRISM_BANK_MASK) == 0, "prism bank size must be a power of two");
static_assert((size_t(1) << PRISM_BANK_SHIFT) == PRISM_BANK_WORDS, "PRISM_BANK_SHIFT disagrees");

// P2K_WRITEMAP=1 counts writes per megabyte, which is how the missing 0xc0800000 alias window
// was found. A compile-time false without the switch, so mem_w keeps nothing of it
u64 g_p2k_writemap[4096] = {};
#if P2K_DEBUG
const bool g_writemap_on = getenv("P2K_WRITEMAP") != nullptr;
#else
const bool g_writemap_on = false;
#endif

namespace {

p2k_state *g_state = nullptr;   // the bus callbacks are plain functions

u32 bus_r(void *, offs_t a, u32 mask)            { return g_state->mem_r(a, mask); }
void bus_w(void *, offs_t a, u32 d, u32 mask)    { g_state->mem_w(a, d, mask); }
u32 bus_io_r(void *, offs_t a, u32 mask)         { return g_state->io_r(a, mask); }
void bus_io_w(void *, offs_t a, u32 d, u32 mask) { g_state->io_w(a, d, mask); }

} // anonymous namespace

p2k_state *p2k_current_state() { return g_state; }

p2k_state::p2k_state()
{
	m_main_ram.assign(0x10000000, 0);      // 256 MB, as mapped by the driver
	m_video_ram_a.assign(0x10000, 0);
	m_cga_ram.assign(0x10000, 0);
	m_ram_c8.assign(0x8000, 0);
	m_bios_ram.assign(0x30000, 0);
	m_nvram.assign(P2K_NV_CMOS_SIZE, 0);   // sized from the shared header, not a literal
	// 0xff because erased flash reads all-ones. A set carrying an update region overwrites every
	// byte and never sees this; one with no update flash boots against it, and then it could matter
	m_nvram_updates.assign(0x800000, 0xff);
	m_prismdata.assign(4 * PRISM_BANK_WORDS, 0); // always present, so the size is invariant
	m_prism_bank9.assign(0x1000000, 0);
	m_smm.assign(0x80000, 0);
	m_vram.assign(0x400000, 0);
	m_system_bios1.assign(0x30000 / 4, 0);
	m_eeprom.assign(P2K_NV_EEPROM_SIZE / 4, 0); // u32 elements, so the byte size is /4 here
#if P2K_SEED_ERROR_LOG
	seed_error_log();                      // before MACHINE_INIT copies a saved CMOS over it
#endif
	g_state = this;
}

// The CMOS error log, as the game's own checksum_errors() lays it out. Two ring buffers of fixed
// records live at 0x11000050; the header in front of them carries, per ring, the record size, the
// capacity, how many are in use and where the next one goes.
//
// Why this is seeded at all. A machine in the field always has this header: it is written on the
// first power-up and survives the software updates, which never clear CMOS. Emulated, the CMOS
// starts blank, and the newer system software then cannot boot - it never reaches the point that
// would build the header, because it needs the header first:
//
//   1. left_sling's constructor calls a hook that reads an adjustment resource whose own
//      constructor is linked later, so the resource is still zeroed BSS. That is a static
//      initialisation order bug in the game, and it reports NonFatal - by itself harmless.
//   2. The NonFatal reporter appends the report to the error log. With the header blank the
//      log's base pointer is 0, so the entry is written over address 0.
//   3. resched() checks the reserved dword at address 0 on every scheduling decision and now
//      finds it changed: "reserved memory at zero corrupted".
//   4. That is Fatal, and the Fatal reporter writes through the same null base pointer, so the
//      corruption it is reporting is re-made on every pass. The machine never leaves the handler
//      and walks the stack down until it runs out.
//
// Booting an older version once and keeping its CMOS is the same fix by hand: the older software
// has a different link order, does not report during construction, and so builds the header
// normally. Seeding it here means a fresh install boots.
//
// The constants are the ones the game computes, not invented: rfm 2.22 builds them at 0x288b48,
// and the routine is byte-identical in rfm 2.10/2.60 and swep1 2.10. Only the two base pointers
// and the geometry are set; the four in-use/index words stay 0, which is an empty log
void p2k_state::seed_error_log()
{
	constexpr u32 region  = 0x23b0;                       // the size the routine works from
	constexpr u32 base_a  = 0x11000050;                   // the records start right after the header
	const u32 a_count     = (region * 3) >> 12;           // 6
	const u32 a_recsize   = ((region * 3) >> 2) / a_count;// 0x476
	const u32 b_count     = region >> 10;                 // 8
	const u32 b_recsize   = (region >> 2) / b_count;      // 0x11d
	const u32 base_b      = base_a + a_recsize * a_count; // 0x11001b14

	if (m_nvram.size() < 0x50) return;
	write_le(m_nvram, 0x00, 0x2400,    0xffffffff);       // total size the two rings must fit in
	write_le(m_nvram, 0x08, a_count,   0xffffffff);
	write_le(m_nvram, 0x0c, a_recsize, 0xffffffff);
	write_le(m_nvram, 0x14, base_a,    0xffffffff);
	write_le(m_nvram, 0x1c, b_count,   0xffffffff);
	write_le(m_nvram, 0x20, b_recsize, 0xffffffff);
	write_le(m_nvram, 0x28, base_b,    0xffffffff);
}

p2k_state::~p2k_state()
{
	if (g_state == this) g_state = nullptr;
}

// ---------------------------------------------------------------- ROM loading
//
// Everything comes from PinMAME ROM regions (see ROM_START in src/wpc/p2k.c). Nothing here opens
// a file, so a machine is selected and audited exactly like any other: the set name is the zip

bool p2k_state::set_prism_roms(const u8 *data, size_t len)
{
	// The region is the four 16 MB banks back to back, already interleaved by the ROM loader:
	// ROM_LOAD32_WORD puts the u10x-even file in the low half of each dword and the odd one in
	// the high half, which is the layout this bus expects
	if (!data || len < 4 * PRISM_BANK_BYTES) return false;
	memcpy(m_prismdata.data(), data, 4 * PRISM_BANK_BYTES); // laid out bank after bank already

	// The MAME driver patches the boot ROM through ROM_FILL in its ROM_START blocks (MAME 0.239,
	// src/mame/drivers/pinball2k.cpp). Those patches are part of the driver, not of the ROM set,
	// so they are applied to our copy rather than declared as ROM_FILL - a ROM_FILL would make
	// the region disagree with the hashes the set is audited against.
	//
	//   0x191            retf -> nop. The option ROM's init entry ends by restoring the register
	//                    block at 0x300 and returning to whoever far-called it. Nothing ever
	//                    calls it: the reset vector jumps straight to 0xc0003, so the far return
	//                    reads a frame that was never pushed and the CPU lands at 0000:0000.
	//   0x419a (rfm)     the immediate of `mov eax,0FFFFFFF9h` -> 1, forcing a failing check to
	//   0x3b33 (swep1)   report success. Same shape in both games, different address. Note this
	//                    is the immediate: the instruction starts one byte earlier.
	//
	// What that check is, disassembled from the stock RFM pair (32-bit code, bank 0 offset 0x4184,
	// the function the patched instruction opens):
	//
	//     push ebp / mov ebp,esp / sub esp,0x50 / push edi,esi,ebx
	//     cmp dword [0x87278], 1      ; "have I already run?" - a one-shot guard in low RAM
	//     jne  do_the_work
	//     mov  eax, 0FFFFFFF9h        ; -7, "already initialised"   <-- patched to 1
	//     jmp  epilogue
	//
	// What it guards is the MediaGX PCI bring-up: it walks device numbers 0..0x14 reading config
	// dword 0, matches vendor 0x1078 (Cyrix), notes the host bridge (device ID 1) and the ISA
	// bridge (0 or 2, Cx5510 against Cx5520), and returns -1 if either is missing. Otherwise it
	// programs the BARs - 0x10000000, 0x11000000, 0x12000000, 0x13000000, 0x14000000, 0x18000001 -
	// sets the command register to 2 and returns 1. So the patch defeats no self-test: it makes the
	// already-run exit return the same 1 the success path does, so a second call is told the
	// chipset is ready instead of getting an error it has no handler for.
	//
	// The address is tied to the boot ROM image, not the game. RFM's alternate bank-0 pair
	// (rfm_u100r2/rfm_u101r2, not a declared set - see src/wpc/p2k.c) holds unrelated code at
	// 0x419a, so this poke would corrupt it; the same function is in there, moved, starting at
	// 0x4608 with its flag at 0x877c0 and the immediate to patch at 0x461b. To re-find it in any
	// image, search bank 0 for `78 10 00 00` (the 0x1078 compare) or `68 01 00 00 18`
	// (push 0x18000001); the prologue is about 0x93 bytes before the former.
	//
	// P2K_PATCH_PCI_INIT_RETRY exists because the patch looked unnecessary here. It is not, and
	// measurement says why. The enumeration is fine: with P2K_PCIWATCH=1 the sweep answers
	//
	//     device 0    1078:0001  MediaGX host bridge   -> its [0x87290]
	//     device 8    146e:0001  Prism card PLX bridge -> its [0x8729c]
	//     device 18   1078:0002  CX5520 ISA bridge     -> its [0x87294]
	//
	// all three, twice over, so nothing is missing from the bus. What fails is upstream of that.
	// The boot code calls this routine and insists on 1 coming back (Episode I, bank 0 offset
	// 0x924: `push 0; call 0x3b20; mov ebx,eax` ... `cmp ebx,1; jne` -> "[ PRISM BOARD NOT PRESENT ]"
	// at 0xb9c, which prints and then `jmp $`). It is reaching the routine with the guard flag
	// already set, so it gets -7 and halts - and the screen message is the halt, not an
	// enumeration failure. Mode 1 forces that exit to return 1 and the machine goes on.
	//
	// The catch is that the early exit returns before doing any of the work, so the BARs are never
	// programmed and [0x87274] stays 0. Episode I 1.50 tolerates that; 2.10 does not reach a boot
	// screen with mode 1 either, which is what a version that actually wants the Prism windows set
	// up would look like. Hence mode 2, which removes the guard rather than its return value: every
	// call re-enumerates and re-programs, so the caller gets a 1 that means something.
	//
	// Measured, and mode 1 stays the default: Revenge From Mars does not boot under mode 2 at all,
	// any version, and Episode I 2.10 reaches the same point under either. So mode 2 is not an
	// improvement - re-running the whole bring-up on every call evidently disturbs something RFM
	// depends on, which is itself a hint that the repeated calls are normal and only their return
	// value was ever wrong.
	//
	// What mode 2 did settle is that the PCI side is complete. Under it the writes go out in full
	// and correct - dev 8 reg 0x10/0x18/0x1c/0x20/0x24/0x30 taking 0x10000000, 0x11000000,
	// 0x12000000, 0x13000000, 0x14000000 and 0x18000001, with the command register set to 2 - and
	// mem_r decodes every one of those windows. Keep it for that: it is the way to prove the
	// bring-up end to end without reading the disassembly again.
	//
	// And 2.10's remaining hang is not in this boot ROM at all, which is worth writing down so the
	// next person does not start here. After this returns, bank 0 offset 0x98a walks the update
	// flash at 0x12000000 through four checks - "[ VALIDATING UPDATE BOOT DATA ]", SYS IMAGE, GAME
	// CODE, SYMBOLS, each with its own fatal exit - and 2.10 passes all four, reaching
	// "[ STARTING UPDATE GAME CODE ]" at 0xa47. Six instructions later:
	//
	//     0xa56  mov eax, [ebx+0x48]      ; ebx = 0x12000000, so the image's own entry
	//     0xa5e  call eax                 ; -> 0x00100000, into the update's system image
	//
	// so the ROM has handed over and what hangs is the version's own code, XINA 1.38 in 2.10's case
	// against 1.19 in 1.50's. The update flash, its checksums and every window it is read through
	// are therefore all good.
	//
	// That reading held. The hang was in the version's own code exactly as this said, and it was a
	// blank CMOS - see P2K_SEED_ERROR_LOG above, which fixes it, and the README for the chain. Not
	// the flash, not the windows, and not the display manager, which was the standing suspect here
	// and was wrong: nothing in the failure ever reached the blit pipeline.
	//
	//     P2K_PATCH_PCI_INIT_RETRY=1  MAME's, the default: already-run exit returns 1
	//     P2K_PATCH_PCI_INIT_RETRY=2  guard removed: init runs in full on every call
	//     P2K_PATCH_PCI_INIT_RETRY=0  neither, for seeing the failure raw
	//
	// Two things this is not. It is not a wrong address for 2.10: the patch lands in the Prism boot
	// ROM, which P2K_COMMON_SWEP1 shares across every Episode I set, so these are the same bytes for
	// 1.50 and 2.10 alike. And it is not really about this routine - something is calling it twice,
	// or entering it with the flag already set, and the 0x191 patch above is a fair suspect, since
	// it exists precisely because this firmware is entered differently here than on a real machine.
	// That is the thing to find; both modes are ways of living with it until then.
	//
	// 0x191 is not behind the switch, and is `cb` in both revisions and in Episode I so it carries
	// over unchanged: nothing far-calls the option ROM's init here, so its `retf` has no frame to
	// return to whatever the PCI bus does.
	auto peek = [this](size_t off) -> u8 {
		if (off / 4 >= PRISM_BANK_WORDS) return 0;   // bank 0 starts at 0, so index == offset
		return u8(m_prismdata[off / 4] >> (unsigned(off % 4) * 8));
	};
	auto poke = [this](size_t off, u8 value) {
		if (off / 4 < PRISM_BANK_WORDS)
		{
			const unsigned shift = unsigned(off % 4) * 8;
			u32 &w = m_prismdata[off / 4];
			w = (w & ~(0xffu << shift)) | (u32(value) << shift);
		}
	};
	// Patch only where the byte being replaced is the one the disassembly says is there. These are
	// offsets into a particular boot ROM image, not into a game, and an image that moves the code
	// would otherwise be corrupted silently - see the site table below for how real that is
	auto poke_if = [&](size_t off, u8 expect, u8 value) -> bool {
		if (peek(off) != expect) return false;
		poke(off, value);
		return true;
	};

	poke_if(0x191, 0xcb, 0x90);   // retf -> nop; cb in every boot ROM seen, V3.2 and V3.6 alike

#if P2K_PATCH_PCI_INIT_RETRY
	// Which image this is, by content rather than by which game is running. The instruction the
	// patch rewrites is `mov eax,0FFFFFFF9h` - b8 f9 ff ff ff - and the four loaders seen put it at
	// these offsets: RFM rev. 1, Episode I, RFM rev. 2. Matching all five bytes makes
	// it exact - each image hits precisely one candidate, and its other copies of that string are
	// all above 0xc0000. It has to be done this way: r2 and the stock pair are the same game, so no
	// prefix separates them, and r2 holds a call whose displacement starts where the stock pair
	// holds this immediate - patching r2 by name would redirect that call
	static constexpr size_t PCI_INIT_SITES[] = { 0x4199, 0x3b32, 0x3b7a, 0x461a };
	size_t mov_site = 0;
	for (const size_t site : PCI_INIT_SITES)
		if (peek(site) == 0xb8 && peek(site + 1) == 0xf9 && peek(site + 2) == 0xff
		    && peek(site + 3) == 0xff && peek(site + 4) == 0xff) { mov_site = site; break; }

	// An image nobody has seen leaves mov_site 0 and goes unpatched, which is the safe way to be
	// wrong: the patch is a convenience for a retry path, not something boot depends on
	const size_t ok_imm    = mov_site ? mov_site + 1 : 0;   // the -7 immediate
	const size_t guard_jne = mov_site ? mov_site - 2 : 0;   // the jne above it
	(void)ok_imm; (void)guard_jne;
#endif

#if P2K_PATCH_PCI_INIT_RETRY == 1
	// MAME's: the already-run exit returns 1 instead of -7
	if (mov_site && poke_if(ok_imm, 0xf9, 0x01)) { poke(ok_imm + 1, 0x00); poke(ok_imm + 2, 0x00); poke(ok_imm + 3, 0x00); }
#elif P2K_PATCH_PCI_INIT_RETRY == 2
	// Take the guard out instead: `jne do_the_work` becomes `jmp do_the_work`, so the routine can
	// never reach its early exit and every call enumerates and programs the BARs afresh. One byte,
	// 75 -> eb, on the jump just above the instruction mode 1 rewrites
	if (mov_site) poke_if(guard_jne, 0x75, 0xeb);
#endif
	return true;
}

bool p2k_state::set_nvram_updates(const u8 *data, size_t len)
{
	if (!data || len < m_nvram_updates.size()) return false;
	memcpy(m_nvram_updates.data(), data, m_nvram_updates.size());
	return true;
}

u8 *p2k_state::nvram_block_ptr(nvram_block which, size_t *size)
{
	switch (which)
	{
		case NVRAM_CMOS:    if (size) *size = m_nvram.size();         return m_nvram.data();
		case NVRAM_EEPROM:  if (size) *size = m_eeprom.size() * 4;    return reinterpret_cast<u8 *>(m_eeprom.data());
		case NVRAM_UPDATES: if (size) *size = m_nvram_updates.size(); return m_nvram_updates.data();
	}
	if (size) *size = 0;
	return nullptr;
}

// ---------------------------------------------------------------- fast bus windows
//
// Profiling a running game put 16% of all wall-clock time in the bus path and 13% in mem_r
// alone: every access left the CPU core through a function pointer, walked the range decode
// below and ended in read_le() on a std::vector. The great majority of that traffic is main RAM
// and the framebuffer, both of which are plain memory with nothing behind them, so the space can
// serve them straight from the buffer.
//
// The ranges here must stay in step with mem_r/mem_w below. They are deliberately only the ones
// that decode to a bare read_le()/write_le() with no device and no side effect:
//   0x00000000-0x0009ffff  main RAM, low
//   0x00100000-0x0fffffff  main RAM (the hole between the two is the video/BIOS area)
//   0x40800000-0x40bfffff  framebuffer
//   0xc0800000-0xc0bfffff  the same framebuffer through the MediaGX 0xc0000000 alias
// Everything else keeps going through the decode.
//
// Nothing is installed while a bus probe is armed - P2K_READWATCH, P2K_MEMWATCH and P2K_WRITEMAP
// all report from inside mem_r/mem_w, and a window would make them silently blind. P2K_FASTBUS=0
// turns the whole thing off, which is the way to check that a suspected fault is not this. Both
// are debugging concerns and neither exists without P2K_DEBUG
void p2k_state::install_fast_windows()
{
#if P2K_DEBUG
	const bool probing = getenv("P2K_READWATCH") || getenv("P2K_MEMWATCH") || getenv("P2K_WRITEMAP");
	const char *off = getenv("P2K_FASTBUS");
	if (probing || (off && off[0] == '0')) return;
#endif

	//!! hardcode these directly into emu.h?!
	m_program->add_fast_window(0x00100000, m_main_ram.data() + 0x00100000, 0x10000000 - 0x00100000);
	m_program->add_fast_window(0x00000000, m_main_ram.data(), 0x000a0000);
	m_program->add_fast_window(0x40800000, m_vram.data(), m_vram.size());
	m_program->add_fast_window(0xc0800000, m_vram.data(), m_vram.size());
}

// ---------------------------------------------------------------- machine
void p2k_state::build_machine(u32 cpu_clock)
{
	m_cpu_clock = cpu_clock;
	m_machine = std::make_unique<p2k_machine>();

	p2k_bus_callbacks mem_cb { bus_r, bus_w, nullptr };
	p2k_bus_callbacks io_cb  { bus_io_r, bus_io_w, nullptr };
	m_program = std::make_unique<address_space>(mem_cb);
	m_io = std::make_unique<address_space>(io_cb);
	install_fast_windows();

	m_maincpu = &m_machine->add(MEDIAGX, "maincpu", cpu_clock);
	m_maincpu->p2k_set_space(AS_PROGRAM, m_program.get());
	m_maincpu->p2k_set_space(AS_IO, m_io.get());

	m_pic1 = &m_machine->add(PIC8259, "pic1", 0u);
	m_pic2 = &m_machine->add(PIC8259, "pic2", 0u);
	m_pit  = &m_machine->add(PIT8253, "pit", 0u);   // an 8253, as in the MAME driver
	m_dma1 = &m_machine->add(AM9517A, "dma1", u32(14318181 / 3));
	m_dma2 = &m_machine->add(AM9517A, "dma2", u32(14318181 / 3));
	m_rtc  = &m_machine->add(MC146818, "rtc", u32(32768));
	m_kbdc = &m_machine->add(KBDC8042, "kbdc", 0u);

	// interrupt wiring, as in the driver: master PIC drives the CPU, slave cascades into IR2
	m_pic1->out_int_callback().set([this](int state) {
		if (state) g_p2k_pic_int++;
		g_p2k_pic_int_state = state;
		apply_irq0(); });
	m_pic1->in_sp_callback().set([]() { return 1; });
	m_pic1->read_slave_ack_callback().set([this](offs_t offset) -> u8
		{ return (offset == 2) ? m_pic2->acknowledge() : 0; });
	m_pic2->out_int_callback().set([this](int state) { m_pic1->ir2_w(state); });
	m_pic2->in_sp_callback().set([]() { return 0; });
	m_maincpu->set_irq_acknowledge_callback([this](int) { return int(m_pic1->acknowledge()); });

	// MAME clocked the PIT at 925 kHz with the standard 1.193182 MHz commented out - a deliberate
	// slowdown to go with the 20 MHz CPU. The firmware programs channel 0 as a rate generator
	// with divisor 298, so that was a tick every ~6400 CPU cycles
	double pit_hz = 1193182.0;
#if P2K_DEBUG
	// P2K_PIT_HZ moves it, which is how what the tick handler needs was measured
	if (const char *s = getenv("P2K_PIT_HZ")) { const double v = atof(s); if (v > 0) pit_hz = v; }
#endif
	m_pit->set_clk<0>(pit_hz);
	m_pit->set_clk<1>(1193182.0);
	m_pit->set_clk<2>(1193182.0);
	// the edge always reaches the PIC - holding it back was the wrong lever, the request it
	// latches is what nests. The gate only notes the edge, to bound how long it may hold delivery.
	m_pit->out_handler<0>().set([this](int state) {
		extern void p2k_clkint_note_edge();
		if (state) { g_p2k_pit0_edges++; p2k_clkint_note_edge(); }
		m_pic1->ir0_w(state); });

	m_rtc->set_binary(true);
	m_rtc->set_binary_year(true);
	m_rtc->set_epoch(1900);
	m_rtc->set_24hrs(true);
	m_rtc->irq().set([this](int state) { m_pic2->ir0_w(state); });

	m_kbdc->set_keyboard_type(kbdc8042_device::KBDC8042_STANDARD);

	// PCI bus: MediaGX north bridge at 0, the Prism card at 8, the CX5520 south bridge at 18
	m_pcibus = &m_machine->add(PCI_BUS_LEGACY, "pcibus", 0, 0);
	m_pcibus->set_device(0,  this, FUNC(p2k_state::mediagx_pci_r), FUNC(p2k_state::mediagx_pci_w));
	m_pcibus->set_device(8,  this, FUNC(p2k_state::prism_pci_r),   FUNC(p2k_state::prism_pci_w));
	m_pcibus->set_device(18, this, FUNC(p2k_state::cx5520_pci_r),  FUNC(p2k_state::cx5520_pci_w));

	// make the machine visible to PinMAME's cpuintrf bridge
	extern void p2k_bridge_attach(p2k_state *, mediagx_device *);
	p2k_bridge_attach(this, m_maincpu);

	m_machine->start();
}

void p2k_state::reset()
{
	m_machine->reset();

	// The driver plants the reset vector by hand: a far jump to the boot loader in the
	// expansion ROM window at 0xc0000.
	m_system_bios1[0xbffc] = 0x03ea;
	m_system_bios1[0xbffd] = 0xc0;

	m_prism_regs[0] = 0x0001146E;
	m_prism_regs[4] = 0x02800002;
	m_prism_regs[8] = 0x03000002;

	m_mediagx_regs[0] = 0x00011078;
	m_mediagx_regs[4] = 0x02800002;
	m_mediagx_regs[8] = 0x06000000;
	m_mediagx_regs[0x40] = 0x80009600;

	m_cx5520_regs[0] = 0x00021078;
	m_cx5520_regs[4] = 0x02800002;
	m_cx5520_regs[8] = 0x06010000;

	// PLX EEPROM defaults, as set up by the MAME driver when the EEPROM is blank. The firmware
	// clocks this image back out of register 0x14 and verifies it, so the whole table matters,
	// not just the first few words.
	static constexpr u32 defaults[] = {
		0x0001146e, 0x03000000, 0x00000000, 0x00000000, 0x0FFE0000, 0x0F800000, 0x0FFF8000,
		0x0C000008, 0x0FFF8001, 0x00100001, 0x01000001, 0x00000001, 0x08000001, 0x08000000,
		0x5403A1E0, 0x5473B940, 0x4041A060, 0x54B2B8C0, 0x54B2B8C0, 0x08800001, 0x09800001,
		0x0A800001, 0x0B800001, 0x00000000, 0x00789242
	};
	for (size_t i = 0; i < std::size(defaults) && i < m_eeprom.size(); i++)
		m_eeprom[i] = defaults[i];

	// the PLX registers come up holding the EEPROM image from word 4 on. MAME copies 32 words
	// here and reads past the end of its 32-word EEPROM region doing so; this stops at the end.
	for (size_t i = 0; i < 32 && i + 4 < m_eeprom.size(); i++)
		m_eeprom_regs[i] = m_eeprom[i + 4];

	// P2K_NVLOG=<base>:<entrysize>:<entries>, hexadecimal: seed the firmware's error-log
	// descriptor in NVRAM. This is a diagnostic, off by default.
	//
	// The reporter at 0x27a8a8 writes its formatted message to
	// `[0x11000028] + [0x11000020] * [0x11000024]` - base plus entry size times write index,
	// with `[0x1100001c]` the ring capacity and `[0x11000018]` the number of entries used. No
	// code in the loaded image ever writes base or entry size; they are factory-initialised
	// battery-backed NVRAM content, restored from the CMOS block in the update flash, and that
	// restore fails on an erased image. All five fields then read zero, so every diagnostic the
	// firmware wants to log lands at linear 0 and destroys the operating system's reserved
	// memory there. Seeding them makes the machine survive its own reports long enough to say
	// what it is complaining about.
#if P2K_DEBUG
	if (const char *s = getenv("P2K_NVLOG"))
	{
		char *end = nullptr;
		const u32 base = u32(strtoul(s, &end, 16));
		const u32 size = (end && *end == ':') ? u32(strtoul(end + 1, &end, 16)) : 0x100;
		const u32 count = (end && *end == ':') ? u32(strtoul(end + 1, nullptr, 16)) : 0x20;
		auto poke32 = [this](size_t off, u32 v) {
			if (off + 3 < m_nvram.size())
			{
				m_nvram[off] = u8(v); m_nvram[off+1] = u8(v >> 8);
				m_nvram[off+2] = u8(v >> 16); m_nvram[off+3] = u8(v >> 24);
			}
		};
		poke32(0x18, 0);        // entries used
		poke32(0x1c, count);    // ring capacity
		poke32(0x20, size);     // entry size
		poke32(0x24, 0);        // write index
		poke32(0x28, base);     // buffer base
		printf("p2k: seeding the NVRAM error log at %08x, %x entries of %x bytes\n", base, count, size);
	}
#endif

	// PC97317 Super I/O identity, read through ports 0x2e/0x2f. Without it the firmware's
	// io_setup_global() reports "SuperIOType unknown (0)" and every io_setup_* step after it
	// bails out - which is where the game code's failure chain starts.
	m_superio_regs[0x20] = 0xDF;
	m_superio_regs[0x21] = 0x01;
}


// ---------------------------------------------------------------- serial interrupts
// The MAME driver wires its two NS16550s to IRQ4 and IRQ3. This port answers the serial ports
// itself - COM1 is the firmware's console - so the interrupt has to come from here.
//
// Measured before writing this: the firmware never enables it. The interrupt enable register at
// 0x3f9 is only ever written with 0, from the boot loader. So this line is real 16550 behaviour
// and costs nothing, but nothing in Pinball 2000 depends on it today. The same measurement says
// the parallel port never gets bit 4 of its control register set, so there is no LPT interrupt
// source to wire either, and there are no IDE drives to raise IRQ14/15.
//
// The transmitter is always empty in this stand-in, so THRE (IER bit 1) is the only source; a
// receive interrupt would need a character to arrive, and nothing sends one.
void p2k_state::update_uart_irq()
{
	if (m_pic1)
	{
		m_pic1->ir4_w((m_uart_reg[1] & 0x02) ? 1 : 0);      // COM1
		m_pic1->ir3_w((m_uart2_reg[1] & 0x02) ? 1 : 0);     // COM2
	}
}

// the CPU sees the PIC's request only when the clkint gate is not holding it
void p2k_state::apply_irq0()
{
	extern bool p2k_clkint_blocks_irq();
	if (!m_maincpu) return;
	m_maincpu->set_input_line(INPUT_LINE_IRQ0,
		(g_p2k_pic_int_state && !p2k_clkint_blocks_irq()) ? 1 : 0);
}



// ---------------------------------------------------------------- pinball I/O, PinMAME side
// PinMAME keeps the whole switch matrix, and the driver board asks for one column at a time; the
// two halves meet here. Called from src/wpc/p2k.c, which owns the core model - the subsystem
// deliberately does not include PinMAME headers.
//
// What is here is the path - columns, rows, lamp strobes and coil registers in the shape both sides
// expect. The wiring of individual numbers is in src/wpc/p2k_names.h, read out of the games' own
// device tables and since checked against both machines' test menus
void p2k_state::push_switches(const u8 *matrix, unsigned count)
{
	if (!matrix) return;
	if (count > sizeof(m_sw_matrix)) count = sizeof(m_sw_matrix);
	for (unsigned i = 0; i < count; i++) m_sw_matrix[i] = matrix[i];

	// The board's own numbering, read out of the game's switch table (see src/p2k/README.md):
	// switch number = 100 + (column-1)*8 + (row-1), with columns 1-9 the playfield matrix,
	// column 10 the coin door's diagnostic buttons and column 11 the cabinet. Those two land in
	// PinMAME's columns of the same number; the coin inputs use PinMAME's coin door column 0.
	m_coin_switches = m_sw_matrix[0];
	m_diag_switches = m_sw_matrix[10];
	m_cabinet_switches = m_sw_matrix[11];
}

void p2k_state::pull_outputs(u8 *lamps, unsigned lamp_columns, u32 *solenoids, u32 *solenoids2) const
{
	if (lamps)
	{
		if (lamp_columns > sizeof(m_lamp_matrix)) lamp_columns = sizeof(m_lamp_matrix);
		for (unsigned i = 0; i < lamp_columns; i++) lamps[i] = m_lamp_matrix[i];
	}
	if (solenoids) *solenoids = m_solenoids;
	if (solenoids2) *solenoids2 = m_solenoids2;
}

// ---------------------------------------------------------------- framebuffer to a file
// P2K_PPM=<path> writes the MediaGX framebuffer once, after P2K_PPM_AT cycles (20 emulated
// seconds by default), as a binary PPM. The geometry and pixel format come from the display
// controller registers exactly as MAME's draw_framebuffer reads them, so what lands in the file
// is what MAME would put on its screen - a way to see whether the picture is right before there
// is anywhere to show it.
// Decode the MediaGX framebuffer into plain 0x00RRGGBB pixels. Geometry and format come from the
// display controller registers, read as MAME's draw_framebuffer reads them: width from
// DC_H_TIMING_1 (halved on pixel double, then +4), height from DC_V_TIMING_1, the pixel format
// from DC_OUTPUT_CFG, the start from DC_FB_ST_OFFSET.
//
// The row stride is DC_LINE_DELTA pixels. MAME doubles it; the two buffers the firmware's flip
// routine alternates between are 0x78000 apart, and 240 rows of 1024 pixels at two bytes each is
// exactly that, so the undoubled value is the right one - with the doubled one the reader runs
// over both buffers and the picture repeats.
//
// The image comes out horizontally mirrored, and it is meant to: Pinball 2000 reflects the
// monitor into the playfield through a half-silvered mirror, so the machine renders it mirrored.
// Callers that show it to a person want it flipped back; the decode leaves it as the hardware has it
bool p2k_state::frame_rgb(u32* const __restrict dest, unsigned capacity, unsigned &width, unsigned &height, const bool fast_15bpp_path, bool& fast_15bpp_path_success) const
{
	const unsigned line_delta = (m_disp_ctrl_reg[DC_LINE_DELTA] & 0x3ff) << 1;
	unsigned w = (m_disp_ctrl_reg[DC_H_TIMING_1] & 0x7ff) + 1;
	if (m_disp_ctrl_reg[DC_TIMING_CFG] & 0x8000) w >>= 1; // pixel double
	w += 4;
	const unsigned h = (m_disp_ctrl_reg[DC_V_TIMING_1] & 0x7ff) + 1;
	if (w < 2 || h < 2 || w > P2K_MAX_WIDTH || h > P2K_MAX_HEIGHT || w*h > capacity || !line_delta) return false;

	const u32 cfg = m_disp_ctrl_reg[DC_OUTPUT_CFG];
	const size_t start = m_disp_ctrl_reg[DC_FB_ST_OFFSET] % (m_vram.size() ? m_vram.size() : 1);
	const u8 * const __restrict fb = m_vram.data() + start;
	const u16* const __restrict fb16 = (const u16*)fb;
	const size_t room = m_vram.size() - start;

	const bool fast_15bpp_path_applicable = !(cfg & 1) && !((cfg & 2) == 0); // is RGB555 path below triggered? -> fast path possible
	fast_15bpp_path_success = fast_15bpp_path && fast_15bpp_path_applicable;

	width = w; height = h;

	if (fast_15bpp_path_success)
	{
	size_t offs = 0;
	for (unsigned y = 0; y < h; ++y)
	{
		unsigned off_fb = y * line_delta;
		for (unsigned x = 0; x < w; ++x,++offs,++off_fb)
		{
			const unsigned off = off_fb * 2;
			const u16 c = (off + 1 < room) ? fb16[off_fb] : 0;
			dest[offs] = c; //!! clear highest bit?
		}
	}
	return true;
	}

	// slow path / conversion needed

	size_t offs = 0;
	for (unsigned y = 0; y < h; ++y)
	{
		unsigned off_fb = y * line_delta;
		for (unsigned x = 0; x < w; ++x,++offs,++off_fb)
		{
			u8 r, g, b;
			if (cfg & 1)                                        //!! 8 bit, palette not modelled
			{
				r = g = b = (off_fb < room) ? fb[off_fb] : 0;
			}
			else
			{
				const unsigned off = off_fb * 2;
				const u16 c = (off + 1 < room) ? fb16[off_fb] : 0;
				if ((cfg & 2) == 0)                             // RGB565
				{
					r = u8(((c >> 11) & 0x1f) << 3);
					g = u8(((c >> 5) & 0x3f) << 2);
				}
				else                                            // RGB555
				{
					r = u8(((c >> 10) & 0x1f) << 3);
					g = u8(((c >> 5) & 0x1f) << 3);
				}
				b = u8((c & 0x1f) << 3);
			}
			dest[offs] = (u32(r) << 16) | (u32(g) << 8) | b;
		}
	}
	return true;
}

// P2K_PPM=<path> writes that picture once, after P2K_PPM_AT cycles (20 emulated seconds by
// default), as a binary PPM - a way to see whether it is right before there is anywhere to show
// it.
#if !P2K_DEBUG
void p2k_state::maybe_write_ppm(u64) {}
#else
void p2k_state::maybe_write_ppm(u64 cycles)
{
	static const char *path = getenv("P2K_PPM");
	if (!path) return;
	static const u64 at = []() {
		const char *s = getenv("P2K_PPM_AT");
		return s ? strtoull(s, nullptr, 0) : 400000000ull;
	}();
	static u64 elapsed = 0;
	static bool written = false;
	elapsed += cycles;
	if (written || elapsed < at) return;
	written = true;

	std::vector<u32> rgb(P2K_MAX_PIXELS);
	unsigned w = 0, h = 0;
	fprintf(stderr, "[p2k ppm] output cfg %08x, start %08x, line delta %u\n",
		m_disp_ctrl_reg[DC_OUTPUT_CFG], m_disp_ctrl_reg[DC_FB_ST_OFFSET],
		(m_disp_ctrl_reg[DC_LINE_DELTA] & 0x3ff) << 1);
	bool unused;
	if (!frame_rgb(rgb.data(), P2K_MAX_PIXELS, w, h, false, unused)) { fprintf(stderr, "[p2k ppm] geometry not sane - nothing written\n"); return; }
	rgb.resize(w * h);

	FILE *f = fopen(path, "wb");
	if (!f) { fprintf(stderr, "[p2k ppm] cannot write %s\n", path); return; }
	fprintf(f, "P6\n%u %u\n255\n", w, h);
	for (u32 p : rgb) { fputc(int(p >> 16), f); fputc(int((p >> 8) & 0xff), f); fputc(int(p & 0xff), f); }
	fclose(f);
	fprintf(stderr, "[p2k ppm] wrote %s (%ux%u)\n", path, w, h);

	if (g_writemap_on)
	{
		fprintf(stderr, "[p2k writemap] busiest megabytes:\n");
		for (int shown = 0; shown < 12; shown++)
		{
			unsigned best = 0;
			for (unsigned i = 1; i < 4096; i++) if (g_p2k_writemap[i] > g_p2k_writemap[best]) best = i;
			if (!g_p2k_writemap[best]) break;
			fprintf(stderr, "  %08x  %llu\n", best << 20, (unsigned long long)g_p2k_writemap[best]);
			g_p2k_writemap[best] = 0;
		}
		fflush(stderr);
	}
}
#endif // P2K_DEBUG

void p2k_state::run_cycles(u64 cycles)
{
	m_machine->run_cycles(*m_maincpu, m_cpu_clock, cycles);
	maybe_write_ppm(cycles);
}

u32 p2k_state::cpu_pc()
{
	return 0;   // the state interface is not modelled; the trace prints bus activity instead
}

// ---------------------------------------------------------------- helpers
u32 p2k_state::read_le(const std::vector<u8> &buf, offs_t off, u32 mask)
{
	if (off + 3 >= buf.size()) return 0;
	return (u32(buf[off]) | (u32(buf[off+1]) << 8) | (u32(buf[off+2]) << 16) | (u32(buf[off+3]) << 24)) & mask;
}

void p2k_state::write_le(std::vector<u8> &buf, offs_t off, u32 data, u32 mask)
{
	if (off + 3 >= buf.size()) return;
	u32 cur = u32(buf[off]) | (u32(buf[off+1]) << 8) | (u32(buf[off+2]) << 16) | (u32(buf[off+3]) << 24);
	cur = (cur & ~mask) | (data & mask);
	buf[off] = u8(cur); buf[off+1] = u8(cur >> 8); buf[off+2] = u8(cur >> 16); buf[off+3] = u8(cur >> 24);
}

// ---------------------------------------------------------------- ported handlers
u32 p2k_state::expansion_r(offs_t offset) const // bank 0
{
	return (offset < PRISM_BANK_WORDS) ? m_prismdata[offset] : 0; //!! % ?
}

// The four mask images - the games' art, im_mask0 through im_mask3 in the update package - live on
// the Prism card. This window is how they are read, and the driver and the firmware do not agree
// about it.
//
// Here (as in the MAME driver this came from) 0x14000000-0x14ffffff is one banked window: which of
// the four m_prismdata[] it serves is whatever prism_1400_w last selected. The other three are
// mapped again, fixed, at 0x15000000/0x16000000/0x17000000 in mem_r - so bank 0 is reachable only
// through the bank register, and the other three are reachable both ways.
//
// The firmware treats all four as fixed windows and never banks at all. boot_im_mask_bank_is_valid
// (rfm 2.22 at 0x2861bc) checksums bank n at, in order, 0x14400000, 0x15000000, 0x16000000 and
// 0x17000000, against the sizes and checksums at BootData +0x5c/+0x60, +0x64/+0x68, +0x6c/+0x70
// and +0x74/+0x78. Note the first: 0x144-, not 0x140-, so where the firmware expects mask 0 this
// window answers from 4 MB into whichever bank happens to be selected.
//
// Which of the two is right is unresolved. Nothing has been seen to depend on it: both games boot
// and play through this handler, and a normal power-up never validates the masks at all - the
// console prints the BOOT DATA, SYS IMAGE, GAME CODE and SYMBOLS banners and no mask one, so that
// routine looks to run only while an update is being written. It is recorded because a
// disagreement of this kind is worth resolving before something does depend on it, not because it
// is known to be a bug. Deciding it needs a machine that reads a mask through 0x14400000 with a
// bank other than 0 selected, or the Prism card's own address decode
u32 p2k_state::prism_1400_r(offs_t offset) const
{
	const size_t base = size_t(m_prismbank & 3) << PRISM_BANK_SHIFT;
	//!! past the end reads 0, where the three fixed windows in mem_r wrap modulo instead - another half of the same question, and equally untested
	return (offset < PRISM_BANK_WORDS) ? m_prismdata[base + offset] : 0;
}

void p2k_state::prism_1400_w(offs_t offset, u32 data)
{
	// the driver selects the prism data bank by magic values written here
	if (data == 0x00000098)      m_prismbank = 0;
	else if (data == 0x00009800) m_prismbank = 1;
	else if (data == 0x00980000) m_prismbank = 2;
	else if (data == 0x98000000) m_prismbank = 3;
}

// PLX local bus registers. Register 0x14 carries the serial EEPROM: bit 25 enables the clock,
// bit 24 is the clock itself, and the chip answers on bit 27, one bit per clock, high word of a
// dword first. The firmware reads the whole image back this way and refuses to run if it does
// not match (`plx_ee_verify(): failed`). Ported from the MAME driver (0.239).
// Careful: this READ MUTATES. Register 0x14 - byte 0x50, the PLX control register - is the
// EEPROM's serial line, and answering it advances the shift counter, the word toggle and the
// offset. Anything that reads it out of band moves the transfer on: a debugger inspecting
// 0x10000050, a read probe, a frontend polling memory. The firmware reads the whole image back
// and refuses to run if it does not match ("plx_ee_verify(): failed"), so a single stray read
// during the transfer is enough to stop the machine booting, with nothing to say why
u32 p2k_state::prism_1000_r(offs_t offset)
{
	offset &= 0x3f;
	if (offset == 0x14)
	{
		u32 t = 0x10000000;       // bit 28 always set: EEPROM present and OK
		t |= m_eeprom_regs[offset];

		if (m_prism_clock_enabled == 1 && m_prism_eprom_clk == 0x1 && m_prism_eprom_offset != -1)
		{
			if (m_prism_eprom_counter == 0)
			{
				if (m_prism_eprom_wordtoggle == 1)
				{
					m_prism_eprom_offset += 1;
					m_prism_eprom_wordtoggle = 0;
				}
				else if (m_prism_eprom_wordtoggle == 0)
					m_prism_eprom_wordtoggle += 1;
				m_prism_eprom_counter = 16;
			}
			m_prism_eprom_counter--;

			const size_t idx = size_t(m_prism_eprom_offset);
			const u32 word = (idx < m_eeprom.size()) ? m_eeprom[idx] : 0;
			const u16 val = u16((m_prism_eprom_wordtoggle == 0) ? (word >> 16) : word);

			const u32 bit = (val >> m_prism_eprom_counter) & 0x1;
			t = (t & ~(1u << 27)) | (bit << 27);
		}
		else if (m_prism_clock_enabled == 1 && m_prism_eprom_clk == 0x1 && m_prism_eprom_offset == -1)
		{
			t &= ~(1u << 27);     // the transfer starts with a zero bit
			m_prism_eprom_offset = 0;
		}
		return t;
	}

	if (offset == 0x13) return m_eeprom_regs[offset] | 0x4;
	return m_eeprom_regs[offset];
}

void p2k_state::prism_1000_w(offs_t offset, u32 data)
{
	offset &= 0x3f;
	if (offset == 0x14)
	{
		if (((data >> 24) & 0x2) == 0x2)
		{
			if (m_prism_clock_enabled == 0) m_prism_eprom_offset = -1;
			m_prism_clock_enabled = 1;
			m_prism_eprom_clk = int((data >> 24) & 0x1);
		}
		else
		{
			m_prism_clock_enabled = 0;
			m_prism_eprom_counter = 16;
			m_prism_eprom_wordtoggle = 0;
		}
	}
	m_eeprom_regs[offset] = data;
}


// ---------------------------------------------------------------- MediaGX graphics pipeline
// The Prism display manager kicks a blit and waits for the pipeline to report it done; without
// that the firmware's render-pass watchdog expires ("Display Manager(HD): Render pass watchdog
// has expired"). Ported from MAME 0.239, src/mame/drivers/pinball2k.cpp: only the raster modes
// the game uses are implemented there, and the blit is one row of `width` pixels.

u32 p2k_state::gx_pipeline_r(offs_t offset) const
{
	return m_gx_pipeline_reg[offset & 0x7f];
}

void p2k_state::gx_pipeline_w(offs_t offset, u32 data, u32 mem_mask)
{
	offset &= 0x7f;
	if (data > 0 && (offset == GP_BLT_MODE || offset == GP_VECTOR_MODE))
	{
		if (offset == GP_BLT_MODE) do_gfx_pipeline();
		// vector mode is not implemented in the MAME driver either
	}
	u32 &r = m_gx_pipeline_reg[offset];
	r = (r & ~mem_mask) | (data & mem_mask);
}

void p2k_state::do_gfx_pipeline()
{
	m_gx_pipeline_reg[GP_BLT_STATUS] |= 0x7;          // busy, as the firmware polls for

	const int line_delta = int((m_disp_ctrl_reg[DC_LINE_DELTA] & 0x3ff) << 1);   // dwords -> words
	const u8 rastermode = u8(m_gx_pipeline_reg[GP_RASTER_MODE] & 0xff);
	const int x     = int(m_gx_pipeline_reg[GP_DST] & 0xffff);
	const int y     = int(m_gx_pipeline_reg[GP_DST] >> 16);
	const int src_x = int(m_gx_pipeline_reg[GP_SRC_X] & 0xffff);
	const int src_y = int(m_gx_pipeline_reg[GP_SRC_X] >> 16);
	const int width = int(m_gx_pipeline_reg[GP_WIDTH] & 0xffff);

	const size_t pixels = m_vram.size() / 2;
	auto vram16 = [this](size_t i) -> u16 & { return *reinterpret_cast<u16 *>(&m_vram[i * 2]); };

	const size_t row = size_t(y) * size_t(line_delta);
	const size_t src_row = size_t(src_y) * size_t(line_delta);
	for (int j = 0; j < width; j++)
	{
		const size_t di = row + size_t(x + j);
		const size_t si = src_row + size_t(src_x + j);
		if (di >= pixels) break;
		switch (rastermode)
		{
			case 0x00: vram16(di) = 0x0000; break;                       // BLACKNESS
			case 0xff: vram16(di) = 0xffff; break;                       // WHITENESS
			case 0xc6:                                                    // transparent copy
				if (si < pixels)
				{
					const u16 pixel = vram16(si);
					if (pixel != 0x7c1f) vram16(di) = pixel;              // the driver's key colour
				}
				break;
			case 0xcc: if (si < pixels) vram16(di) = vram16(si); break;   // SRCCOPY
			default: break;                                               // the rest is unused
		}
	}

	m_gx_pipeline_reg[GP_BLT_STATUS] &= 0xfffffff8;   // done
}

u32 p2k_state::disp_ctrl_r(offs_t offset) const
{
	offset &= 0x3f;
	// The vertical line counter has to advance on its own - the MAME driver keeps it moving with
	// a per-scanline timer tied to its screen device (`m_disp_ctrl_reg[0x54/4] = scanline`). This
	// port has no screen yet, so the value is derived from emulated time instead: 525 lines at
	// 60 Hz, the mode the driver's default timings describe. Anything waiting for the display to
	// move sees it move.
	if (offset == DC_V_LINE_CNT)
	{
		// From the machine's own clock, which is the point: a beam position depends on the video
		// timings and not on how fast the CPU runs, so this is independent of the MediaGX clock
		// and does not need revisiting when that changes. What it replaced did depend on it - a
		// per-timeslice counter, and PinMAME hands out slices of one frame, which at the 20 MHz
		// this was first written for came to 333333 cycles against 525*635 = 333375 for a
		// synthesized frame. Near enough that the counter aliased and the line barely moved: it
		// read a constant 0x1d8, and the firmware's frame callback, which only acts while the line
		// is below 10, never fired. Those numbers are why the old approach failed, not a
		// description of this one
		const u64 ns = u64(m_machine->machine().time().as_double() * 1e9);
		constexpr u64 ns_per_line = 1000000000ull / (VIDEO_LINES * VIDEO_FRAMES_PER_SECOND);
		return u32((ns / ns_per_line) % VIDEO_LINES);
	}
	return m_disp_ctrl_reg[offset];
}
void p2k_state::disp_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_disp_ctrl_reg[offset & 0x3f];
	const u32 before = r;
	r = (r & ~mem_mask) | (data & mem_mask);

	// P2K_DISPWATCH=1: every write that changes a display controller register, with the PC that
	// made it. Which registers a game programs, and when, is the difference between a picture and
	// a black screen.
#if P2K_DEBUG
	static const bool watch = getenv("P2K_DISPWATCH") != nullptr;
	if (watch && r != before)
	{
		extern unsigned p2k_bridge_pc();
		fprintf(stderr, "[p2k disp] %llu: reg %02x  %08x -> %08x  from PC=%08x\n",
			(unsigned long long)g_p2k_cycles_total, unsigned((offset & 0x3f) * 4),
			before, r, p2k_bridge_pc());
		fflush(stderr);
	}
#else
	(void)before;
#endif
}

u32 p2k_state::memory_ctrl_r(offs_t offset) const { return m_memory_ctrl_reg[offset & 0x3f]; }
void p2k_state::memory_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_memory_ctrl_reg[offset & 0x3f];
	r = (r & ~mem_mask) | (data & mem_mask);
}

u32 p2k_state::biu_ctrl_r(offs_t offset) const { return m_biu_ctrl_reg[offset & 0x3f]; }
void p2k_state::biu_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_biu_ctrl_reg[offset & 0x3f];
	r = (r & ~mem_mask) | (data & mem_mask);
}

// The update flash behaves like an Intel 28F320J5: command writes put it into a mode, and reads
// then return either the array, the CFI query table or the status register. Ported from the
// MAME driver's nvram_updates_r/w.
u8 p2k_state::nvram_updates_r(offs_t offset) const
{
	if (m_flash_mode == 1)
	{
		// CFI query response, as tabulated in the MAME driver (8 Mbit part)
		static constexpr u8 cfi[] = {
			0x51,0x00,0x52,0x00,0x59,0x00,0x01,0x00, 0x00,0x00,0x31,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x45,0x00, 0x55,0x00,0x00,0x00,0x00,0x00,0x07,0x00,
			0x07,0x00,0x0a,0x00,0x00,0x00,0x04,0x00, 0x04,0x00,0x04,0x00,0x00,0x00,0x17,0x00,
			0x02,0x00,0x00,0x00,0x05,0x00,0x00,0x00, 0x01,0x00,0x3f,0x00,0x00,0x00,0x00,0x00,
			0x02,0x00,0x50,0x00,0x52,0x00,0x49,0x00,
		};
		if (offset >= 0x20 && offset - 0x20 < sizeof(cfi)) return cfi[offset - 0x20];
		return 0;
	}
	if (m_flash_mode == 2 || m_flash_mode == 5)
		return 0x80;                                     // status: ready
	return (offset < m_nvram_updates.size()) ? m_nvram_updates[offset] : 0xff;
}

void p2k_state::nvram_updates_w(offs_t offset, u16 data)
{
	if (m_flash_mode != 6)
	{
		if (data == 0x0098)      { m_flash_mode = 1; return; }   // read query
		if (data == 0x0070)      { m_flash_mode = 2; return; }   // read status register
		if (data == 0x00ff)      { m_flash_mode = 0; return; }   // read array
		// Block erase. Three different block sizes meet here and none of them agree: the CFI table
		// this same device answers with declares one region of 0x3f+1 blocks of 0x200*256 bytes,
		// so 64 blocks of 128 KB across the 8 MB part; this check aligns on a *word* offset of
		// 0x2000, which is 16 KB of bytes; and the loop below clears 0x2000 *bytes*, 8 KB. An
		// erase therefore clears an eighth of the block the part says it has.
		//
		// It does not bite today because programming below is a plain store rather than the AND a
		// real flash does, so a half-erased block still takes new data, and because the update
		// image is not persisted - a bad erase lasts one run. It would bite the moment either of
		// those changed, or if a firmware erased a block it then checked was blank
		if (data == 0x0020 && (offset % 0x2000) == 0) { m_flash_mode = 3; return; }
		if (m_flash_mode == 3 && data == 0x00d0)
		{
			for (u32 i = 0; i < 0x2000; i++)
				if (offset * 2 + i < m_nvram_updates.size()) m_nvram_updates[offset * 2 + i] = 0xff;
			m_flash_mode = 2;
			return;
		}
		if (data == 0x00e8) { m_flash_mode = 5; m_buffer_counter = 0; return; }   // write to buffer
		if (m_flash_mode == 5 && m_buffer_counter == 0)
		{
			m_flash_mode = 6;
			m_buffer_counter = data;
			return;
		}
		return;
	}

	// buffered program: the driver only lets the last 128 KB be rewritten
	if (offset >= 0x3e0000 && offset * 2 + 1 < m_nvram_updates.size())
	{
		m_nvram_updates[offset * 2] = u8(data);
		m_nvram_updates[offset * 2 + 1] = u8(data >> 8);
	}
	if (--m_buffer_counter < 0) m_flash_mode = 2;
}

// PCI configuration space. The identifiers come from the MAME driver: 1078:0001 MediaGX host
// bridge, 1078:0002 CX5520 ISA bridge, 146e:0001 the PLX bridge on the Prism card.
u32 p2k_state::mediagx_pci_r(int function, int reg, u32 mem_mask)
{
	return m_mediagx_regs[reg] & mem_mask;
}

void p2k_state::mediagx_pci_w(int function, int reg, u32 data, u32 mem_mask)
{
	u32 *varptr = m_mediagx_regs + reg;
	COMBINE_DATA(varptr);
}

u32 p2k_state::cx5520_pci_r(int function, int reg, u32 mem_mask) const
{
	return m_cx5520_regs[reg] & mem_mask;
}

void p2k_state::cx5520_pci_w(int function, int reg, u32 data, u32 mem_mask)
{
	u32 *varptr = m_cx5520_regs + reg;
	COMBINE_DATA(varptr);
}

// All three devices index by the byte offset lpci passes, which is what the [0]/[4]/[8]
// initialisation in reset() is written for: [4] is the status/command pair and [8] the class
// code, and the Prism's 0x02800002/0x03000002 are the same shape as the two Cyrix devices'.
// This one used to divide by 4, so only the vendor word at reg 0 landed on its initialiser and
// the card reported status 0 and class 0. The firmware does read both - P2K_PCIWATCH shows
// "dev 8 reg 0x04 -> 02800002" and "reg 0x08 -> 03000002" where it used to see zeros - and all
// games boot unchanged with them right, which is the measurement the old note here asked for and did not have.
//!! still, it may need additional verification!
u32 p2k_state::prism_pci_r(int function, int reg, u32 mem_mask) const
{
	return m_prism_regs[reg] & mem_mask;
}

void p2k_state::prism_pci_w(int function, int reg, u32 data, u32 mem_mask)
{
	u32 *varptr = &m_prism_regs[reg];
	COMBINE_DATA(varptr);
}

// ---------------------------------------------------------------- bus decode
u32 p2k_state::mem_r(offs_t addr, u32 mem_mask)
{
#if P2K_DEBUG
	// P2K_READWATCH=<from>[-<to>], hexadecimal: the first 40 reads from that range, with the PC.
	// The write watch answers who fills a structure; this answers whether a device is ever asked.
	static unsigned rwatch_from = 0, rwatch_to = 0;
	static long rwatch_left = 40;
	static const bool rwatch_init = []() {
		if (const char *s = getenv("P2K_READWATCH"))
		{
			char *end = nullptr;
			rwatch_from = unsigned(strtoul(s, &end, 16));
			rwatch_to = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : rwatch_from + 3;
		}
		return true;
	}();
	(void)rwatch_init;
	if (rwatch_to && rwatch_left > 0 && addr >= (rwatch_from & ~3u) && addr <= rwatch_to)
	{
		rwatch_left--;
		extern unsigned p2k_bridge_pc();
		fprintf(stderr, "[p2k memr] %08x mask %08x  from PC=%08x\n", addr, mem_mask, p2k_bridge_pc());
		fflush(stderr);
	}
#endif

	// the firmware reaches the MediaGX control registers through their 0xc0000000 alias
	// The firmware addresses the whole MediaGX region through its 0xc0000000 alias, not just the
	// control registers: its own framebuffer base pointer is 0xc0800000, and a write map showed
	// 30 M writes at 0xc0800000 and 13 M at 0xc0900000 - the picture, going nowhere, because the
	// alias window used to stop at the register block.
	if (addr >= 0xc0000000 && addr < 0xc1000000) addr -= 0x80000000;
	if (addr < 0x000a0000)                       return read_le(m_main_ram, addr, mem_mask);
	if (addr < 0x000b0000)                       return read_le(m_video_ram_a, addr - 0x000a0000, mem_mask);
	if (addr < 0x000c0000)                       return read_le(m_cga_ram, addr - 0x000b0000, mem_mask);
	if (addr < 0x000c8000)                       return expansion_r((addr - 0x000c0000) / 4) & mem_mask;
	if (addr < 0x000d0000)                       return read_le(m_ram_c8, addr - 0x000c8000, mem_mask);
	if (addr < 0x00100000)                       return read_le(m_bios_ram, addr - 0x000d0000, mem_mask);
	if (addr < 0x10000000)                       return read_le(m_main_ram, addr, mem_mask);
	if (addr < 0x10000080)                       return prism_1000_r((addr - 0x10000000) / 4) & mem_mask;
	if (addr >= 0x11000000 && addr < 0x11030000) return read_le(m_nvram, addr - 0x11000000, mem_mask);
	if (addr >= 0x13000000 && addr < 0x13800000)
		return P2K_HAVE_WEAK(p2k_dcs_read) ? p2k_dcs_read(addr - 0x13000000, mem_mask) : 0;
	if (addr >= 0x12000000 && addr < 0x13000000)
	{
		// byte-wide handler: serve each active lane separately
		offs_t base = (addr - 0x12000000) & 0xffffff;
		u32 result = 0;
		for (unsigned lane = 0; lane < 4; lane++)
			if (mem_mask & (0xffu << (lane * 8)))
				result |= u32(nvram_updates_r(base + lane)) << (lane * 8);
		return result;
	}
	// the four mask images. The first of these is banked and the other three are not; the firmware
	// expects four fixed windows, the first of them at 0x14400000. See prism_1400_r
	if (addr >= 0x14000000 && addr < 0x15000000) return prism_1400_r((addr - 0x14000000) / 4) & mem_mask;
	if (addr >= 0x15000000 && addr < 0x16000000) return m_prismdata[(size_t(1) << PRISM_BANK_SHIFT) + (((addr - 0x15000000) / 4) & PRISM_BANK_MASK)] & mem_mask;
	if (addr >= 0x16000000 && addr < 0x17000000) return m_prismdata[(size_t(2) << PRISM_BANK_SHIFT) + (((addr - 0x16000000) / 4) & PRISM_BANK_MASK)] & mem_mask;
	if (addr >= 0x17000000 && addr < 0x18000000) return m_prismdata[(size_t(3) << PRISM_BANK_SHIFT) + (((addr - 0x17000000) / 4) & PRISM_BANK_MASK)] & mem_mask;
	if (addr >= 0x18000000 && addr < 0x19000000) return read_le(m_prism_bank9, addr - 0x18000000, mem_mask);
	if (addr >= 0x40000400 && addr < 0x40001000) return m_scratchpad[((addr - 0x40000400) / 4) & 0x1ff] & mem_mask;
	if (addr >= 0x40008000 && addr < 0x40008100) return biu_ctrl_r((addr - 0x40008000) / 4) & mem_mask;
	if (addr >= 0x40008100 && addr < 0x40008300) return gx_pipeline_r((addr - 0x40008100) / 4) & mem_mask;
	if (addr >= 0x40008300 && addr < 0x40008400) return disp_ctrl_r((addr - 0x40008300) / 4) & mem_mask;
	if (addr >= 0x40008400 && addr < 0x40008500) return memory_ctrl_r((addr - 0x40008400) / 4) & mem_mask;
	if (addr >= 0x40400000 && addr < 0x40480000) return read_le(m_smm, addr - 0x40400000, mem_mask);
	if (addr >= 0x40800000 && addr < 0x40c00000) return read_le(m_vram, addr - 0x40800000, mem_mask);
	// the same framebuffer through the MediaGX 0xc0000000 alias, which is where the firmware's own
	// base pointer puts it. This has to agree with install_fast_windows() above: the fast path is
	// switched off whenever a probe is on, so a window that exists only there means P2K_MEMWATCH,
	// P2K_READWATCH and P2K_WRITEMAP each quietly change the machine they are measuring
	if (addr >= 0xc0800000 && addr < 0xc0c00000) return read_le(m_vram, addr - 0xc0800000, mem_mask);
	if (addr >= 0xf00c0000 && addr < 0xf00c8000) return expansion_r((addr - 0xf00c0000) / 4) & mem_mask;
	if (addr >= 0xfffd0000)                      return m_system_bios1[(addr - 0xfffd0000) / 4] & mem_mask;

	m_unmapped_r++;
	if (m_trace && m_unmapped_r < 40) printf("  [mem] unmapped read  %08x mask %08x\n", addr, mem_mask);
	return 0xffffffff & mem_mask;
}

void p2k_state::mem_w(offs_t addr, u32 data, u32 mem_mask)
{
	if (g_writemap_on) g_p2k_writemap[addr >> 20]++;

#if P2K_DEBUG
	// P2K_MEMWATCH=<from>[-<to>], hexadecimal: report every write to that range with the PC that
	// made it. The boot code hands a far return address to itself through low memory, so seeing
	// who writes there - and who wipes it - is what bring-up needs.
	static unsigned watch_from = 0, watch_to = 0;
	static const bool watch_init = []() {
		if (const char *s = getenv("P2K_MEMWATCH"))
		{
			char *end = nullptr;
			watch_from = unsigned(strtoul(s, &end, 16));
			watch_to = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : watch_from + 3;
		}
		return true;
	}();
	(void)watch_init;
	if (watch_to && addr >= (watch_from & ~3u) && addr <= watch_to)
	{
		// P2K_MEMWATCH_CHANGED=1 reports only writes that actually alter the word. The switch
		// scanner rewrites its bitmaps every 0.25 ms whether anything moved or not, so without
		// this the interesting write drowns in millions of identical ones.
		static const bool changed_only = getenv("P2K_MEMWATCH_CHANGED") != nullptr;
		bool report = true;
		if (changed_only)
		{
			u32 before = 0;
			if (addr + 3 < m_main_ram.size()) memcpy(&before, &m_main_ram[addr], 4);
			report = ((before ^ data) & mem_mask) != 0;
		}
		if (report)
		{
			extern unsigned p2k_bridge_pc();
			fprintf(stderr, "[p2k memw] %08x <- %08x mask %08x  from PC=%08x\n",
				addr, data, mem_mask, p2k_bridge_pc());
			fflush(stderr);
		}
	}
#endif

	// The firmware addresses the whole MediaGX region through its 0xc0000000 alias, not just the
	// control registers: its own framebuffer base pointer is 0xc0800000, and a write map showed
	// 30 M writes at 0xc0800000 and 13 M at 0xc0900000 - the picture, going nowhere, because the
	// alias window used to stop at the register block.
	if (addr >= 0xc0000000 && addr < 0xc1000000) addr -= 0x80000000;
	if (addr < 0x000a0000)                       { write_le(m_main_ram, addr, data, mem_mask); return; }
	if (addr < 0x000b0000)                       { write_le(m_video_ram_a, addr - 0x000a0000, data, mem_mask); return; }
	if (addr < 0x000c0000)                       { write_le(m_cga_ram, addr - 0x000b0000, data, mem_mask); return; }
	if (addr < 0x000c8000)                       { return; }   // expansion ROM window, writes ignored
	if (addr < 0x000d0000)                       { write_le(m_ram_c8, addr - 0x000c8000, data, mem_mask); return; }
	if (addr < 0x00100000)                       { write_le(m_bios_ram, addr - 0x000d0000, data, mem_mask); return; }
	if (addr < 0x10000000)                       { write_le(m_main_ram, addr, data, mem_mask); return; }
	if (addr < 0x10000080)                       { prism_1000_w((addr - 0x10000000) / 4, data); return; }
	if (addr >= 0x11000000 && addr < 0x11030000) { write_le(m_nvram, addr - 0x11000000, data, mem_mask); return; }
	if (addr >= 0x12000000 && addr < 0x13000000)
	{
		// word-wide handler: the flash sees 16-bit commands
		offs_t word = ((addr - 0x12000000) & 0xffffff) / 2;
		if (mem_mask & 0x0000ffff) nvram_updates_w(word, u16(data));
		if (mem_mask & 0xffff0000) nvram_updates_w(word + 1, u16(data >> 16));
		return;
	}
	if (addr >= 0x13000000 && addr < 0x13800000)
	{
		// the DCS2 sound board, upstream's, wired in src/wpc/p2k.c. Weak, so the standalone
		// harness links without one.
		if (P2K_HAVE_WEAK(p2k_dcs_write)) p2k_dcs_write(addr - 0x13000000, data, mem_mask);
		return;
	}
	if (addr >= 0x14000000 && addr < 0x15000000) { prism_1400_w((addr - 0x14000000) / 4, data); return; }
	if (addr >= 0x15000000 && addr < 0x18000000) { return; }   // prism data banks are read-only
	if (addr >= 0x18000000 && addr < 0x19000000) { write_le(m_prism_bank9, addr - 0x18000000, data, mem_mask); return; }
	if (addr >= 0x40000400 && addr < 0x40001000) { u32 &r = m_scratchpad[((addr - 0x40000400) / 4) & 0x1ff]; r = (r & ~mem_mask) | (data & mem_mask); return; }
	if (addr >= 0x40008000 && addr < 0x40008100) { biu_ctrl_w((addr - 0x40008000) / 4, data, mem_mask); return; }
	if (addr >= 0x40008100 && addr < 0x40008300) { gx_pipeline_w((addr - 0x40008100) / 4, data, mem_mask); return; }
	if (addr >= 0x40008300 && addr < 0x40008400) { disp_ctrl_w((addr - 0x40008300) / 4, data, mem_mask); return; }
	if (addr >= 0x40008400 && addr < 0x40008500) { memory_ctrl_w((addr - 0x40008400) / 4, data, mem_mask); return; }
	if (addr >= 0x40400000 && addr < 0x40480000) { write_le(m_smm, addr - 0x40400000, data, mem_mask); return; }
	if (addr >= 0x40800000 && addr < 0x40c00000) { write_le(m_vram, addr - 0x40800000, data, mem_mask); return; }
	if (addr >= 0xc0800000 && addr < 0xc0c00000) { write_le(m_vram, addr - 0xc0800000, data, mem_mask); return; }   // the alias; see mem_r
	if (addr >= 0xfffd0000)                      { u32 &r = m_system_bios1[(addr - 0xfffd0000) / 4]; r = (r & ~mem_mask) | (data & mem_mask); return; }

	m_unmapped_w++;
	if (m_trace && m_unmapped_w < 40) printf("  [mem] unmapped write %08x <- %08x mask %08x\n", addr, data, mem_mask);
}

unsigned p2k_bridge_pc();

namespace {

#if !P2K_DEBUG
inline void iowatch(const char *, offs_t, unsigned) {}
#else
// P2K_IOWATCH=<from>[-<to>], hexadecimal: accesses to that I/O port range, with the PC that made
// them. Bring-up of a port device starts with knowing how the firmware probes it.
//
// P2K_IOWATCH_AFTER=<cycles> holds the watch back until that many have run, and
// P2K_IOWATCH_MAX=<n> changes how many it then reports (200 by default). A device that is set up
// once at boot and used much later needs both: the interrupt controller's ICW sequence costs five
// accesses in the first millisecond, and the EOI and mask traffic that matters is 200 million
// cycles behind it. Without the delay the budget is spent before the interesting part starts
unsigned g_iowatch_from = 0, g_iowatch_to = 0;
int g_iowatch_left = 200;
u64 g_iowatch_after = 0;
const bool g_iowatch_init = []() {
	if (const char *s = getenv("P2K_IOWATCH"))
	{
		char *end = nullptr;
		g_iowatch_from = unsigned(strtoul(s, &end, 16));
		g_iowatch_to = (end && *end == '-') ? unsigned(strtoul(end + 1, nullptr, 16)) : g_iowatch_from;
	}
	if (const char *s = getenv("P2K_IOWATCH_AFTER")) g_iowatch_after = strtoull(s, nullptr, 0);
	if (const char *s = getenv("P2K_IOWATCH_MAX"))   g_iowatch_left  = (int)strtol(s, nullptr, 0);
	return true;
}();

void iowatch(const char *dir, offs_t port, unsigned value)
{
	if (!g_iowatch_to || g_iowatch_left <= 0) return;
	if (port < g_iowatch_from || port > g_iowatch_to) return;
	if (g_iowatch_after && g_p2k_cycles_total < g_iowatch_after) return;
	g_iowatch_left--;
	fprintf(stderr, "[p2k io%s] %04x = %02x  from PC=%08x  cyc=%llu\n", dir, unsigned(port), value, p2k_bridge_pc(), (unsigned long long)g_p2k_cycles_total);
	fflush(stderr);
}
#endif // P2K_DEBUG

} // anonymous namespace


// ---------------------------------------------------------------- pinball driver board
// The pinball I/O hangs off the parallel port. The board carries an index register and a set of
// I/O registers; the firmware writes an index to the data port, clocks it in, then writes or
// reads the selected register through the same port. The full sequence uses the control port's
// INIT and STROBE lines (documented in MAME's pinball2k.cpp around line 1337); the MAME driver
// tracks it with two phase flags on the data port alone, and that is what is ported here.
//
// The register map (MAME 0.239, src/mame/drivers/pinball2k.cpp):
//   00 switch-coin (r)   01 switch-flipper (r)  02 switch-dip (r)   03 switch-EOS/diag (r)
//   04 switch-row (r)    05 switch-column (w)   06/07 lamp row A/B  08 lamp column
//   09/0a/0b solenoid C/B/A   0c solenoid-flipper  0d solenoid D   0e solenoid-logic
//   0f switch-system (r) 10/11 lamp matrix diagnostics   12/13 fuse diagnostics
//
// The inputs answer as an idle machine for now: switches, lamps and solenoids reach PinMAME's
// core model in the second half of M3.5, and that is where the writes will go too.
#if P2K_DEBUG
// P2K_PDBWATCH selects which power driver board registers to trace: "1" or "all" for every one,
// otherwise a comma separated list of hex indices - "10,11,08" to watch the lamp status readback
// together with the column strobe that drives it. Worth narrowing, because register 04 is the
// switch row and cycles forever, which buries everything else in a whole-log trace
static bool p2k_pdbwatch(u8 reg)
{
	static int on = -1;
	static bool sel[256];
	if (on < 0)
	{
		const char* e = getenv("P2K_PDBWATCH");
		on = e ? 1 : 0;
		if (e)
		{
			if (!strcmp(e, "1") || !strcmp(e, "all")) { for (int i = 0; i < 256; i++) sel[i] = true; }
			else for (const char *q = e; *q; )
			{
				sel[(u8)strtol(q, nullptr, 16)] = true;
				const char *c = strchr(q, ',');
				q = c ? c + 1 : q + strlen(q);
			}
		}
	}
	return on && sel[reg];
}
#endif

u8 p2k_state::lpt_r(offs_t offset)
{
	if (offset == 1) return 0xff;      // status port
	if (offset != 0) return 0x00;      // control port

	// a read only answers with an I/O register once an index has been clocked in
	if (!(m_pdb_phase_1 == 1 && m_pdb_phase_2 == 0)) return 0;
	m_pdb_phase_2 = 1;

	const u8 v = pdb_reg_r();
#if P2K_DEBUG
	// P2K_PDBWATCH=1: every power driver board register the game reads, and what it got back.
	// Change-only per register, because the switch and lamp strobes read continuously and a full
	// log buries whatever is being looked for. This is how to find out which registers a test menu
	// page actually touches rather than inferring it from the register map
	{
		static int last[256];
		static bool seen[256];
		if (p2k_pdbwatch(m_pdb_index) && (!seen[m_pdb_index] || last[m_pdb_index] != (int)v))
		{
			seen[m_pdb_index] = true; last[m_pdb_index] = v;
			printf("[p2k pdb] r %02x -> %02x\n", m_pdb_index, v);
			fflush(stdout);
		}
	}
#endif
	return v;
}

u8 p2k_state::pdb_reg_r() const
{
	switch (m_pdb_index)
	{
		case 0x00: return m_coin_switches;
		case 0x01: return m_cabinet_switches;
		case 0x02: return 1; // dip switches. Read once at startup and the 1 is accepted - see the note below on what else was watched
		case 0x03: return m_diag_switches;
		case 0x04:
		{
			// The switch row for whichever column is being strobed. Measured: the column
			// register is one-hot and active HIGH -- the firmware writes 0x01, 0x02, 0x04,
			// 0x08, 0x10 ... and 0x80, exactly like the lamp column strobe below. (It was
			// read as active low here before, which shifted every column by one.)
			for (unsigned c = 0; c < 8; c++)
				if (m_switch_column & (1u << c))
					return m_sw_matrix[(c + 1) & 0xf];          // PinMAME numbers columns from 1
			return 0x00;
		}
		// These four return index+1, which was a placeholder rather than a model of anything.
		// Watched through boot, attract and the test menu with P2K_PDBWATCH=02,0c,0d,0e,0f,12,13:
		//
		//   0x0c/0x0d/0x0e  written constantly, never read - they are the solenoid registers
		//   0x0f            read exactly once, at startup, and 0x10 is accepted
		//
		// So the machine boots and plays on these values. 0x0f is the switch-system register and
		// carries zero cross, which on real hardware times coil firing and GI dimming - but a single
		// read at startup is a presence or version check, not polling, so nothing here is timed
		// against it. Modelling zero cross properly only becomes necessary if something starts
		// reading 0x0f repeatedly
		case 0x0c: return 0x0d;
		case 0x0d: return 0x0e;
		case 0x0e: return 0x0f;
		case 0x0f: return 0x10;
		case 0x10: case 0x11:
		{
			// Lamp matrix diagnostics - Pinball 2000's lamp fault detection, which is what lets an
			// operator see any dead bulb in the test menu directly. On the power
			// driver board it is a 74LS240 buffering the lamp row lines back to the CPU: the
			// operations manual's lamp matrix pages show it, marked "Lamp Status" and "used for
			// diagnostics only".
			//
			// The game drives a column, reads the rows back here and compares them with what it
			// drove. Agreement means the bulb is there and conducting; the two ways of disagreeing
			// are an open filament and a short, which is how one sense line yields three verdicts.
			// The game side of it is diagnostics_is_lamp_bad(), lamp_powerup_tests() and the
			// poweron_open_matrix it fills - all named in the packages' symbols.rom.
			//
			// Echoing the row latches models a playfield where every bulb is present and working. See P2K_LAMP_STATUS_INVERT for how the sense was
			// measured - the test drives one row bit at a time and reads back after each
			const u8 row = (m_pdb_index == 0x10) ? m_lamp_row_a : m_lamp_row_b;
			return P2K_LAMP_STATUS_INVERT ? (u8)~row : row;
		}
		case 0x05: case 0x06: case 0x07: case 0x08:
		case 0x09: case 0x0a: case 0x0b:
		// 0x12/0x13 are the fuse diagnostics (wms_pdb_fuse_status, pdb_fuses) and are still the same
		// stub the lamp registers were. Unlike those, though, they were never read once in the whole
		// watch above - so whatever the test menu shows for fuses, it is not coming from here
		case 0x12: case 0x13: return 0x00;
		default:   return 0xff;
	}
}

void p2k_state::lpt_w(offs_t offset, u8 data)
{
	if (offset != 0) return;

	if (m_pdb_phase_1 == 0)                                { m_pdb_phase_1 = 1; m_pdb_phase_2 = 0; }
	else if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 0)     { m_pdb_phase_2 = 1; }
	else if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 1)     { m_pdb_phase_1 = 1; m_pdb_phase_2 = 0; }

	if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 0) m_pdb_index = data;   // index register
	if (m_pdb_phase_2 != 1) return;

#if P2K_DEBUG
	// Writes are logged unconditionally, not change-only: a lamp test that drives the same row
	// twice in a row is exactly the case worth seeing, and pairing each write with the read that
	// follows it is the whole point of watching this side
	if (p2k_pdbwatch(m_pdb_index)) { printf("[p2k pdb] w %02x <- %02x\n", m_pdb_index, data); fflush(stdout); }
#endif

	// a write to the selected I/O register
	switch (m_pdb_index)
	{
		case 0x05: m_switch_column = data; break;               // switch column strobe, one-hot
		case 0x06: m_lamp_row_a = data; break;                  // lamp rows, latched until the
		case 0x07: m_lamp_row_b = data; break;                  //   column strobe below
		case 0x08:
			// Lamp column strobe. The board drives one column at a time and has two row banks
			// of eight (index 6 and 7), so a column carries sixteen lamps; PinMAME's matrix is
			// eight bits per column, so each driven column becomes two of them - bank A at 2c,
			// bank B at 2c+1. Eight columns therefore occupy sixteen of PinMAME's.
			m_lamp_col = data;
			for (unsigned c = 0; c < 8; c++)
				if (data & (1u << c))
				{
					m_lamp_matrix[c * 2 + 0] = m_lamp_row_a;
					m_lamp_matrix[c * 2 + 1] = m_lamp_row_b;
				}
			break;
		// Measured in the game's own coil test, which cycles the drivers in order and names each
		// one on screen: the driver numbering runs 0x0b, 0x0a, 0x09, 0x0d, 0x0c, eight per
		// register. PinMAME's solenoid bits follow the game's driver numbers, so bit 0 is
		// "Antr. 1" - Left Martian. See src/p2k/README.md for the whole table and its checks
		case 0x0b: m_solenoids = (m_solenoids & ~0x000000ffu) |  u32(data); break;        // drivers 1-8
		case 0x0a: m_solenoids = (m_solenoids & ~0x0000ff00u) | (u32(data) << 8); break;  // drivers 9-16
		case 0x09: m_solenoids = (m_solenoids & ~0x00ff0000u) | (u32(data) << 16); break; // drivers 17-24
		// Measured: 0x0c and 0x0d are eight bits wide like the rest -- register 0x0d takes 0x90
		// while the machine runs, so masking them to four dropped half of each. Five registers of
		// eight is forty outputs, which does not fit one word, so D goes into the second one.
		case 0x0d: m_solenoids  = (m_solenoids & ~0xff000000u) | (u32(data) << 24); break;// drivers 25-32
		case 0x0c: m_solenoids2 = (m_solenoids2 & ~0x000000ffu) | u32(data); break;       // drivers 33-40
		// 0x0e is the sixth group, "solenoid logic" in the register map above: drivers 41-48. Both
		// games' own driver tables reach into it - Revenge From Mars names 48 Ticket Dispenser and
		// Episode I 41 Neon Tube, 42 Knocker, 43 Shaker Motor, 44 Topper - and this is where the
		// last three of those come out.
		//
		// Still unverified, unlike 0x0c/0x0d, and one attempt has come back empty: Revenge From
		// Mars 1.60's own coil test walks 33-40 one at a time, 0x01 through 0x80 in 0x0c, and never
		// touches 0x0e at all. That fits - 48 is a ticket dispenser nobody fits, and 41-47 are Not
		// Used on that playfield, so there is nothing there to test. It leaves the mapping resting
		// on the register map's own name plus Episode I's driver table, and Episode I's 2.x sets
		// are the ones that would exercise it. First thing to doubt if a shaker misbehaves
		case 0x0e: m_solenoids2 = (m_solenoids2 & ~0x0000ff00u) | (u32(data) << 8); break;// drivers 41-48
		default: break; // diagnostics: later
	}
}

u8 p2k_state::port_r(offs_t port)
{
	const u8 value = port_read(port);
	iowatch("r", port, value);
	return value;
}

// What this costs has NOT been measured, and it is not free. pic8259_device schedules its
// zero-delay timer on every write, so by the time we get here the pending flag is essentially
// always set and this is a real scheduler pass: advance_to() scans the timer list, fires the
// callback, then scans again to find nothing and stop. From P2K_IOWATCH the guest writes these
// ports on the order of once per few hundred cycles, which puts it around 2-6% - an estimate off a
// log, not a number.
//
// Nor is the comparison with the thing it replaced as favourable as it looks. The clkint gate now
// defaults off, and off it costs nothing at all - push_int_frame() returns immediately and the
// per-instruction hook is never armed. So against the old arrangement this trades a per-instruction
// tax for a per-PIC-write one, and which is cheaper is genuinely open.
//
// It cannot be measured with what is here: report_progress()'s host= and mips= are P2K_DEBUG only,
// and the configuration in question is a release build. That needs PinMAME's own speed readout or a
// timed fixed run.
//
// If it does turn out to cost, the expense is not the pump but the route: two whole timer-list
// scans to reach one device we already know we want. A check_irqs_now() on pic8259_device doing
// what its device_timer(TIMER_CHECK_IRQ) does would let this call it straight - no scan, no flag,
// and note_zero_delay() would drop back to a backstop for other devices. That is not done here only
// because it means editing imported MAME code, which this port has otherwise kept pristine. Worth
// doing with a number in hand; not on an estimate
void p2k_state::pics_settle()
{
	m_machine->machine().scheduler().run_due_timers();
}

u8 p2k_state::port_read(offs_t port)
{
	if (port <= 0x001f)                   return m_dma1->read(port);
	if (port >= 0x0020 && port <= 0x0021) return m_pic1->read(port & 1);
	if (port >= 0x0040 && port <= 0x0043) return m_pit->read(port & 3);
	if (port >= 0x0060 && port <= 0x006f) return m_kbdc->data_r(port & 7);
	if (port >= 0x0070 && port <= 0x0071) return m_rtc->read(port & 1);
	if (port >= 0x00a0 && port <= 0x00a1) return m_pic2->read(port & 1);
	if (port >= 0x00c0 && port <= 0x00df) return m_dma2->read((port - 0x00c0) / 2);

	// Ports 0x22/0x23 are the Cyrix configuration registers. The MAME driver implements them as
	// a plain indexed register file, but wiring that up here derails the boot: the firmware then
	// takes a configuration path that ends in garbage execution, while leaving the ports
	// unmapped (reads return 0xff) lets it continue. Something behind those registers is not
	// ready yet - to be revisited when the SMM/GX_BASE handling is in place.
	if (port >= 0x002e && port <= 0x002f)
		return (port == 0x002f) ? m_superio_regs[m_superio_reg_sel] : 0;
	if (port >= 0x00e8 && port <= 0x00eb) return 0xff;         // I/O delay port
	if ((port >= 0x0170 && port <= 0x0177) || (port >= 0x01f0 && port <= 0x01f7) ||
		(port >= 0x0370 && port <= 0x0377) || (port >= 0x03f0 && port <= 0x03f7))
		return 0xff;                                            // IDE: no drives attached

	// parallel port: the pinball driver board sits at 0x3bc, the printer port itself at 0x378.
	// Both are probed by writing the data port and reading it back - an unmapped 0xff there is
	// what made the firmware report "PinIO failed: no printer port present".
	if (port >= 0x03bc && port <= 0x03bf) return lpt_r(port - 0x03bc);
	if (port >= 0x0378 && port <= 0x037b)
	{
		switch (port - 0x0378)
		{
			case 0:  return m_lpt_data;
			case 1:  return 0x00;                               // status: no printer attached
			default: return m_lpt_control;
		}
	}
	if (port == 0x0278) return 0x00;

	// COM1 console stand-in
	if (port >= 0x03f8 && port <= 0x03ff)
	{
		switch (port & 7)
		{
			case 2:                                 // IIR: what is asking for attention
				// the transmitter is always empty here, so THRE is the only source. Reading IIR
				// clears it, as on a real 16550 - the driver then writes the next character.
				if (m_uart_reg[1] & 0x02) { constexpr u8 iir = 0x02; update_uart_irq(); return iir; }
				return 0x01;                        // no interrupt pending
			case 5: return 0x60;                    // LSR: transmitter holding and shift both empty
			case 6: return 0xb0;                    // MSR: CTS/DSR/DCD asserted
			default: return m_uart_reg[port & 7];
		}
	}
	if (port >= 0x02f8 && port <= 0x02ff)           // COM2, same stand-in
	{
		switch (port & 7)
		{
			case 2: return (m_uart2_reg[1] & 0x02) ? 0x02 : 0x01;
			case 5: return 0x60;
			case 6: return 0xb0;
			default: return m_uart2_reg[port & 7];
		}
	}

	m_unmapped_r++;
	if (m_trace && m_unmapped_r < 40) printf("  [io ] unmapped read  %04x\n", port);
	return 0xff;
}

void p2k_state::port_w(offs_t port, u8 data)
{
	iowatch("w", port, data);
	if (port <= 0x001f)                   { m_dma1->write(port, data); return; }
	// The interrupt controllers get their pending work run before the next instruction, because
	// that is when the hardware would have done it. pic8259_device re-evaluates its INT output
	// from a zero-delay timer, and a timer the guest sets mid-slice would otherwise wait for the
	// slice to end - and a slice runs to the next timer expiry, which once the PIT is going is
	// ~19406 cycles, a whole clock tick. run_due_timers() explains the rest, and the clkint gate
	// in p2k_cpuintrf.cpp is what this makes unnecessary.
	//
	// Writes only. The guest's reads here are the mask readback in XINU's critical-section pair,
	// which changes nothing the CPU can see; a poll command would, but this firmware does not use
	// one - the ICW4 it writes is 0x01, no AEOI and no poll mode
	if (port >= 0x0020 && port <= 0x0021) { m_pic1->write(port & 1, data); pics_settle(); return; }
	if (port >= 0x0040 && port <= 0x0043) { m_pit->write(port & 3, data); return; }
	if (port >= 0x0060 && port <= 0x006f) { m_kbdc->data_w(port & 7, data); return; }
	if (port >= 0x0070 && port <= 0x0071) { m_rtc->write(port & 1, data); return; }
	if (port >= 0x00a0 && port <= 0x00a1) { m_pic2->write(port & 1, data); pics_settle(); return; }
	if (port >= 0x00c0 && port <= 0x00df) { m_dma2->write((port - 0x00c0) / 2, data); return; }

	if (port >= 0x00e8 && port <= 0x00eb) return;
	if ((port >= 0x0170 && port <= 0x0177) || (port >= 0x01f0 && port <= 0x01f7) ||
		(port >= 0x0370 && port <= 0x0377) || (port >= 0x03f0 && port <= 0x03f7)) return;
	if (port >= 0x02f8 && port <= 0x02ff)
	{
		m_uart2_reg[port & 7] = data;
		if ((port & 7) == 1) update_uart_irq();
		return;
	}
	if (port >= 0x03bc && port <= 0x03bf) { lpt_w(port - 0x03bc, data); return; }
	if (port >= 0x0378 && port <= 0x037b)
	{
		if (port == 0x0378)      m_lpt_data = data;
		else if (port == 0x037a) m_lpt_control = data;
		return;
	}
	if (port == 0x0278) return;
	if (port >= 0x002e && port <= 0x002f)
	{
		if (port == 0x002e) m_superio_reg_sel = data;
		else                m_superio_regs[m_superio_reg_sel] = data;
		return;
	}

	if (port >= 0x03f8 && port <= 0x03ff)
	{
		// with DLAB clear, a write to the base port is a character to transmit
		if ((port & 7) == 0 && !(m_uart_reg[3] & 0x80))
		{
			m_console.push_back(char(data));
			if (m_trace) { fputc(int(data), stdout); fflush(stdout); }
		}
		else
			m_uart_reg[port & 7] = data;
		return;
	}

	m_unmapped_w++;
	if (m_trace && m_unmapped_w < 40) printf("  [io ] unmapped write %04x <- %02x\n", port, data);
}

u32 p2k_state::io_r(offs_t addr, u32 mem_mask)
{
	// the PCI configuration window is a dword port pair, not four byte ports
	if (addr >= 0x0cf8 && addr <= 0x0cfc)
	{
		const u32 v = m_pcibus->read((addr - 0x0cf8) / 4, mem_mask);
#if P2K_DEBUG
		// P2K_PCIWATCH=1 prints the config cycles the firmware runs. The bring-up routine at bank 0
		// offset 0x4184 sweeps device 0..0x14 for vendor 0x1078 (Cyrix host and ISA bridges) and
		// 0x146e (the Prism card), and refuses to go on without all three - "prism card not
		// detected" is that last one coming back 0xffffffff. This is the trace that says which
		// device number the firmware asked for and what the bus answered
		if (addr == 0x0cfc && getenv("P2K_PCIWATCH"))
		{
			const u32 a = m_pci_cfg_addr;
			printf("[p2k pci] bus %2u dev %2u fn %u reg 0x%02x -> %08x%s\n",
			       (a >> 16) & 0xff, (a >> 11) & 0x1f, (a >> 8) & 7, a & 0xfc, v,
			       (v == 0xffffffffu) ? "   (nothing there)" : "");
			fflush(stdout);
		}
#endif
		return v;
	}

	u32 result = 0;
	for (unsigned lane = 0; lane < 4; lane++)
		if (mem_mask & (0xffu << (lane * 8)))
			result |= u32(port_r(addr + lane)) << (lane * 8);
	return result;
}

void p2k_state::io_w(offs_t addr, u32 data, u32 mem_mask)
{
	if (addr >= 0x0cf8 && addr <= 0x0cfc)
	{
#if P2K_DEBUG
		if (addr == 0x0cf8) m_pci_cfg_addr = data; // remembered so P2K_PCIWATCH can label the data cycle
		else if (getenv("P2K_PCIWATCH"))
		{
			// The writes are the half that matters once enumeration is known good: this is where the
			// bring-up programs the Prism's BARs, and a boot that gets no further than the splash is
			// one to check for these never arriving, or arriving as something other than the
			// 0x10000000/0x11000000/0x12000000/0x13000000/0x14000000/0x18000001 the routine intends
			const u32 a = m_pci_cfg_addr;
			printf("[p2k pci] bus %2u dev %2u fn %u reg 0x%02x <- %08x  (mask %08x)\n",
			       (a >> 16) & 0xff, (a >> 11) & 0x1f, (a >> 8) & 7, a & 0xfc, data, mem_mask);
			fflush(stdout);
		}
#endif
		m_pcibus->write((addr - 0x0cf8) / 4, data, mem_mask);
		return;
	}

	for (unsigned lane = 0; lane < 4; lane++)
		if (mem_mask & (0xffu << (lane * 8)))
			port_w(addr + lane, u8(data >> (lane * 8)));
}
