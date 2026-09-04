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
static_assert(int(p2k_state::NVRAM_RTC)    == P2K_NV_BLOCK_RTC,    "NVRAM block selectors disagree");

// How to get the MediaGX PCI bring-up to report success, which the boot code demands before it
// will go on - without one of these, games halt at "[ PRISM BOARD NOT PRESENT ]".
//
//   1  the default: MAME's patch, the routine's already-run exit returns 1 instead of -7
//   2  remove that guard instead, so every call re-enumerates and re-programs the BARs
//   0  neither, for seeing the failure raw
//
// Keep 1. Mode 2 was tried and is worse: Revenge From Mars does not boot under it at all, in any
// version, while Episode I gets no further than it does under 1. p2k_state::set_prism_roms has the
// detail, including what mode 2 was for and why it is still worth having around
#ifndef P2K_PATCH_PCI_INIT_RETRY
#define P2K_PATCH_PCI_INIT_RETRY 1
#endif

// Sense of the lamp status readback (registers 0x10/0x11). 0 echoes the row latches, so the bit the
// game is driving reads back set; 1 inverts them.
// Confirmed on both machines: with this the games' own single-lamp tests pass.
//
// Arrived at with P2K_PDBWATCH rather than from the buffer's part number. The test walks one row bit at a time - 0x01, 0x02, 0x04 ... 0x80 - and
// reads 0x10 and 0x11 after each. Both readings that produced "short" left the bit being driven
// CLEAR: the constant 0x00 this used to return, and an inverted echo (0xfe, 0xfd, 0xfb ...). Echoing un-inverted is what sets it
#ifndef P2K_LAMP_STATUS_INVERT
#define P2K_LAMP_STATUS_INVERT 0
#endif

// Build the CMOS error log's header on a machine whose CMOS has never been written, the way the
// game itself would have built it on an earlier power-up. Without it the newer system software
// cannot boot from a blank CMOS - see seed_error_log() for the failure chain, and the README
#ifndef P2K_SEED_ERROR_LOG
#define P2K_SEED_ERROR_LOG 1
#endif

// A vertical blank flag in the Prism BAR2 window, mirroring the one Encore and MAME models. It did not fix
// the XINA 1.38 service menu, which is what it was added for - that still does not repaint with it
// on - but it is a real signal the hardware has and this driver otherwise does not. 0 restores the
// old behaviour, where the address is plain CMOS
#ifndef P2K_VBLANK_FLAG
#define P2K_VBLANK_FLAG 1
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
// from the trap hook, the three together say where a tick is multiplied or lost
u64 g_p2k_pit0_edges = 0;
u64 g_p2k_pic_int = 0;

// what the PIC currently wants from the CPU. The clkint gate may hold it back, so the line the
// CPU sees is this AND "not inside the clock handler" - see p2k_cpuintrf.cpp
int g_p2k_pic_int_state = 0;

void p2k_apply_irq0()
{
	extern p2k_state *p2k_current_state();
	if (p2k_state *st = p2k_current_state()) st->apply_irq0();
}

namespace {
	// display controller registers, by dword index
	constexpr unsigned DC_GENERAL_CFG = 0x04 / 4; // bit 6 VIEN enables the vertical interrupt
	constexpr unsigned DC_TIMING_CFG  = 0x08 / 4; // bit 31 VINT pending, bit 30 VNA
	constexpr unsigned DC_OUTPUT_CFG  = 0x0c / 4;
	constexpr unsigned DC_FB_ST_OFFSET= 0x10 / 4;
	constexpr unsigned DC_LINE_DELTA  = 0x24 / 4;
	constexpr unsigned DC_H_TIMING_1  = 0x30 / 4;
	constexpr unsigned DC_V_TIMING_1  = 0x40 / 4;
	constexpr unsigned DC_V_LINE_CNT  = 0x54 / 4;
	constexpr unsigned VIDEO_LINES    = 525;      // 640x480 with blanking
	constexpr unsigned VIDEO_FRAMES_PER_SECOND = 60;
	constexpr unsigned VIDEO_ACTIVE_LINES = 480;  // the rest of the 525 is blanking
	// graphics pipeline registers, by dword index
	constexpr unsigned GP_DST         = 0x00 / 4;
	constexpr unsigned GP_WIDTH       = 0x04 / 4;
	constexpr unsigned GP_SRC_X       = 0x08 / 4;
	constexpr unsigned GP_RASTER_MODE = 0x100 / 4;
	constexpr unsigned GP_VECTOR_MODE = 0x104 / 4;
	constexpr unsigned GP_BLT_MODE    = 0x108 / 4;
	constexpr unsigned GP_BLT_STATUS  = 0x10c / 4;
	// What the games actually program, watched with P2K_GPWATCH: 0x00 dst x|y<<16, 0x04 width|
	// height<<16, 0x08 src x|y<<16, and 0x10/0x14/0x20-0x2c set once to ffffffff and never touched
	// again. Raster mode only ever takes 0x10c6 or 0x00cc, so bit 12 is the source transparency
	// enable - set with the 0xc6 copy, clear with the opaque 0xcc - and masking to 8 bits below is
	// equivalent only because those are the sole two values. Since checked against the GXm databook
	// (gxmdb_v20.pdf, Table 4-24/4-25), which confirms all seven offsets above and names the rest:
	// bit 12 is TB, Transparent BLIT - a colour key compare, not the monochrome source transparency
	// on bit 11 - bits 9:8 are the pattern mode (00, solid, in both values here) and ROP really is
	// bits 7:0, so the 8 bit mask is right by specification rather than by luck. The six registers
	// held at ffffffff are GP_PAT_COLOR_A/B (0x10/0x14) and GP_PAT_DATA_0-3 (0x20-0x2c), which the
	// transparent copy requires; its key colour is not a register at all. See color_key in
	// do_gfx_pipeline for both, and what rests on them. Encore's independent implementation
	// (qemu/p2k-gp-blt.c) agrees on bit 12 and on one row per trigger, hardcodes the key, and does
	// strictly less besides - no fills, no vector mode, no other ROPs. README.md has the full
	// comparison, and the XINA 1.38 investigation that prompted it
} // anonymous namespace

// P2K_WRITEMAP=1: count every write into 1 MB buckets and print the busiest at the PPM trigger.
// "Where does it draw" is otherwise a guessing game over a 4 GB address space.
// the DCS2 sound board lives on PinMAME's side; these are defined in src/wpc/p2k.c
extern "C" u32 p2k_dcs_read(u32 offset, u32 mem_mask) P2K_WEAK;
extern "C" void p2k_dcs_write(u32 offset, u32 data, u32 mem_mask) P2K_WEAK;

// One MediaGX/Prism ROM bank: two 8 MB chips interleaved as 32-bit words
static constexpr size_t   PRISM_BANK_BYTES = 0x1000000;
static constexpr size_t   PRISM_BANK_WORDS = PRISM_BANK_BYTES / 4; // 0x400000
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
	m_main_ram.assign(0x10000000, 0);            // 256 MB, as mapped by the driver
	m_video_ram_a.assign(0x10000, 0);
	m_cga_ram.assign(0x10000, 0);
	m_ram_c8.assign(0x8000, 0);
	m_bios_ram.assign(0x30000, 0);
	m_nvram.assign(P2K_NV_CMOS_SIZE, 0);         // sized from the shared header, not a literal
	// 0xff because erased flash reads all-ones. A set carrying an update region overwrites every
	// byte and never sees this; one with no update flash boots against it, and then it could matter
	m_nvram_updates.assign(0x800000, 0xff);
	m_prismdata.assign(4 * PRISM_BANK_WORDS, 0); // always present, so the size is invariant
	m_prism_bank9.assign(0x1000000, 0);
	m_smm.assign(0x80000, 0);
	m_vram.assign(0x400000, 0);
	m_system_bios1.assign(0x30000 / 4, 0);
	m_eeprom.assign(P2K_NV_EEPROM_SIZE / 4, 0);  // u32 elements, so the byte size is /4 here
#if P2K_SEED_ERROR_LOG
	seed_error_log();                            // before MACHINE_INIT copies a saved CMOS over it
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
	constexpr u32 region = 0x23b0;                       // the size the routine works from
	constexpr u32 base_a = 0x11000050;                   // the records start right after the header
	const u32 a_count    = (region * 3) >> 12;           // 6
	const u32 a_recsize  = ((region * 3) >> 2) / a_count;// 0x476
	const u32 b_count    = region >> 10;                 // 8
	const u32 b_recsize  = (region >> 2) / b_count;      // 0x11d
	const u32 base_b     = base_a + a_recsize * a_count; // 0x11001b14

	if (m_nvram.size() < 0x50) return;
	write_le(m_nvram, 0x00, 0x2400,    0xffffffff);      // total size the two rings must fit in
	write_le(m_nvram, 0x08, a_count,   0xffffffff);
	write_le(m_nvram, 0x0c, a_recsize, 0xffffffff);
	write_le(m_nvram, 0x14, base_a,    0xffffffff);
	write_le(m_nvram, 0x1c, b_count,   0xffffffff);
	write_le(m_nvram, 0x20, b_recsize, 0xffffffff);
	write_le(m_nvram, 0x28, base_b,    0xffffffff);
}

// The real-time clock, taken from the host. mc146818 reads machine().base_datetime() inside
// nvram_default(), but nothing here drives MAME's NVRAM machinery - this subsystem moves its own
// blocks across p2k_pinmame_nvram_set() - so without this the device sat at its constructed state
// and every machine booted with the clock at zero, which the firmware reports as 1 Jan 1999.
//
// keep_year is the whole subtlety. Register 9 is documented as the year, and this firmware does not
// use it as one: it reads the register, folds it into the clock it keeps in its own CMOS, and then
// writes zero back. It is a count of year rollovers since the last sync, not a date. On the real
// board that works because the RTC is battery-backed and keeps running while the machine is off,
// so the register is normally 0 and reads 1 only if a New Year passed in the meantime.
//
// Which is why handing it the host year on every start makes the displayed year climb: a fresh
// machine has 1999 stored and adds 27 to reach 2026, and every start after that adds 27 again -
// 2053, 2080. Refreshing everything *except* register 9 gives the guest what a ticking
// battery-backed clock would: the current date and time, and no years to add. A New Year passing
// while the machine is off does want that register set to the number of years crossed, and
// rtc_restore() below is what puts it there, off the saved stamp.
//
// The clock as PinMAME saves it: the 64 registers, then the host time_t they were taken at. The
// stamp is the whole point - the emulated chip stops when the machine does, where the real one
// keeps running on its battery, so the only way to know how long it was off is to ask the host
// twice and subtract. What the firmware wants out of that is not the elapsed time but the number
// of New Years in it, because it reads register 9 as a count of years to fold into its own clock
void p2k_state::rtc_save()
{
	if (!m_rtc) return;
	memcpy(m_rtc_nv, m_rtc->p2k_data(), 0x40);
	const u64 now = u64(::time(nullptr));
	memcpy(m_rtc_nv + 0x40, &now, sizeof now);
}

void p2k_state::rtc_restore()
{
	if (!m_rtc) return;
	memcpy(m_rtc->p2k_data(), m_rtc_nv, 0x40);

	u64 saved = 0;
	memcpy(&saved, m_rtc_nv + 0x40, sizeof saved);

	// Calendar years crossed, not elapsed years: a machine switched off on 31 December and started
	// the next morning slept through one New Year, and that is the one the firmware needs to add.
	// Skipped for a block saved by a build that did not stamp it, and for a host clock that has
	// moved backwards since - neither has any number of years to offer
	const time_t then_t = time_t(saved), now_t = ::time(nullptr);
	if (saved && now_t > then_t)
	{
		struct tm a = *localtime(&then_t), b = *localtime(&now_t);
		int years = b.tm_year - a.tm_year;
		if (years <  0) years = 0;
		if (years > 99) years = 99; // a register, not a span to be trusted blindly
		m_rtc->p2k_data()[9] = u8(m_rtc_nv[9] + years);
	}

	// The register file went in behind the device's back, so the timers and the interrupt line are
	// still set up for whatever it last saw written rather than for the divider and enables just
	// restored. The device does this much for itself when it loads the same bytes in nvram_read()
	m_rtc->p2k_reload();
}
void p2k_state::clock_from_host(bool keep_year)
{
	if (!m_rtc) return;
	const uint8_t year = m_rtc->p2k_data()[9];
	m_rtc->p2k_nvram_default();
	if (keep_year) m_rtc->p2k_data()[9] = year;

	// Register A is the divider and nvram_default() zeroes it with the rest, leaving DV2:DV0 = 000 -
	// the divider chain stopped on a real chip, and in this model a seconds update every 128 seconds,
	// so the clock starts right and then stands still. The firmware never writes this register: on
	// the real board it is battery-backed and was set once, so zeroed is a state no machine is in.
	// Seed what a live chip holds - DV 010, the 32.768 kHz base, and RS 0110, the usual PC periodic
	// rate, which only raises PF unless the guest sets PIE - through the port, that being what
	// re-arms the timers; poking p2k_data() would not, update_timer() having already run
	m_rtc->write(0, 0x0a);
	m_rtc->write(1, 0x26);
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
	// return to whatever the PCI bus does
	auto peek = [this](size_t off) -> u8 {
		if (off / 4 >= PRISM_BANK_WORDS) return 0; // bank 0 starts at 0, so index == offset
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

	poke_if(0x191, 0xcb, 0x90); // retf -> nop; cb in every boot ROM seen, V3.2 and V3.6 alike

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
	const size_t ok_imm    = mov_site ? mov_site + 1 : 0; // the -7 immediate
	const size_t guard_jne = mov_site ? mov_site - 2 : 0; // the jne above it
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
		// The real-time clock's 64 registers. Battery-backed on the real board, and the firmware
		// depends on that: it keeps its own clock in the CMOS and advances it by the difference it
		// sees here, so a clock that starts afresh every run reads as a jump of decades and the
		// year climbs run after run. Saved alongside the CMOS it has to agree with
		case NVRAM_RTC:     if (size) *size = sizeof m_rtc_nv;        return m_rtc_nv;
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
	m_pit  = &m_machine->add(PIT8253, "pit",  0u); // an 8253, as in the MAME driver
	m_dma1 = &m_machine->add(AM9517A, "dma1", u32(14318181 / 3));
	m_dma2 = &m_machine->add(AM9517A, "dma2", u32(14318181 / 3));
	m_rtc  = &m_machine->add(MC146818,"rtc",  u32(32768));
	m_kbdc = &m_machine->add(KBDC8042,"kbdc", 0u);

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
	// with divisor 298, so that was a tick every ~6400 CPU cycles.
	//
	// This is also the machine's time base: one_second_proc counts these ticks and bumps the date and
	// time the firmware keeps in its own CMOS. Not the whole of its clock though - it tracks the RTC
	// while running too, which is how a stopped divider in that chip showed up as a displayed clock
	// that never advanced (see PAST_FAILURES.md). What follows is about this tick and holds either
	// way, however the two divide the work. 1193182/298 is 4004 Hz and
	// the count it makes a second out of is round, so its second comes up about 0.13% short - measured
	// at 77563110 cycles against the 77666666 a second really takes, a gain of roughly two minutes a
	// day. A real board has the same crystal and the same divisor, so it drifts the same way; do not
	// "fix" it by nudging pit_hz, that would only make the emulated machine keep better time than the one it copies
	double pit_hz = 1193182.0;
#if P2K_DEBUG
	// P2K_PIT_HZ moves it, which is how what the tick handler needs was measured
	if (const char *s = getenv("P2K_PIT_HZ")) { const double v = atof(s); if (v > 0) pit_hz = v; }
#endif
	m_pit->set_clk<0>(pit_hz);
	m_pit->set_clk<1>(1193182.0);
	m_pit->set_clk<2>(1193182.0);
	// the edge always reaches the PIC - holding it back was the wrong lever, the request it
	// latches is what nests. The gate only notes the edge, to bound how long it may hold delivery
	m_pit->out_handler<0>().set([this](int state) {
		extern void p2k_clkint_note_edge();
		if (state) { g_p2k_pit0_edges++; p2k_clkint_note_edge(); }
		m_pic1->ir0_w(state); });

	// BCD, which is what the firmware decodes whatever the data mode bit says: seeded with the host
	// date in binary it showed the 18th as "12" - 18 is 0x12 - so it reads the register as two
	// packed digits. And the year counts from 1999, not 1900: presented with a 1900-based year it
	// rejected it outright, wrote year 0 / 1 Jan back into the RTC and reported the date as
	// 1 Jan 1999, so year 0 is 1999 to this firmware. Between the two the clock could not survive a
	// boot, which is why every machine started in 1999 no matter what the host clock said
	m_rtc->set_binary(false);
	m_rtc->set_binary_year(false);
	m_rtc->set_epoch(1999);
	// The firmware zeroes the year register itself, a few hundred million cycles into the boot,
	// after reading it three times. That is its own bookkeeping rather than a rejection: it takes
	// the clock into the CMOS it keeps and works from there, and the date it displays stays right
	// afterwards. Nothing to fix - just do not read a zero in that register as the clock being lost
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

	// The driver plants the reset vector by hand: a far jump to the boot loader in the expansion ROM window at 0xc0000
	m_system_bios1[0xbffc] = 0x03ea;
	m_system_bios1[0xbffd] = 0xc0;

	// BC_DRAM_TOP's reset value, from the databook's Table 4-9. It read back 0 here, which says
	// "no memory at all" to anything that asks. The firmware does ask once before writing its own
	// answer - 0x003e0000 and then 0x006e0000, the top of DRAM at roughly 4 MB and 7 MB - so what
	// it read was wrong even if nothing was seen to depend on it. MAME's driver answers 0xffffff
	// here, which is neither the reset value nor anything the firmware writes. The other three BIU
	// registers, BC_XMAP_1 to _3, reset to 0 and no set writes them; see biu_ctrl_w for what would
	// change if one ever did
	m_biu_ctrl_reg[0] = 0x3fffffff;

	m_prism_regs[0] = 0x0001146E;
	m_prism_regs[4] = 0x02800002;
	m_prism_regs[8] = 0x03000002;

	// The MediaGX's own configuration space, reset values from the databook's Table 4-40: vendor
	// 1078h Cyrix, device 0001h, status 0280h, revision 00h, class 060000h host bridge. The dword
	// at 0x40 is four single-byte registers - 00h PCI Control Function 1, 96h Function 2, 00h
	// reserved, 80h Arbitration Control 1 - which is why it looks arbitrary and is not.
	// Command was 0x0002 and the latency timer 0, against defaults of 0x0007 and 0x0d; nothing was
	// seen to depend on either, but a reset value is not ours to choose
	m_mediagx_regs[0] = 0x00011078;
	m_mediagx_regs[4] = 0x02800007;
	m_mediagx_regs[8] = 0x06000000;
	m_mediagx_regs[0x0c] = 0x00000d00; // cache line size 00h, latency timer 0dh
	m_mediagx_regs[0x40] = 0x80009600;

	m_cx5520_regs[0] = 0x00021078;
	m_cx5520_regs[4] = 0x02800002;
	m_cx5520_regs[8] = 0x06010000;

	// PLX EEPROM defaults, as set up by the MAME driver when the EEPROM is blank. The firmware
	// clocks this image back out of register 0x14 and verifies it, so the whole table matters,
	// not just the first few words. This is the image the shipped software wants: the prototypes
	// want four registers different and rewrite them on their first boot, which is what a real
	// card programmed by a later firmware would make them do - see src/p2k/README.md
	static constexpr u32 defaults[] = {
		0x0001146e, 0x03000000, 0x00000000, 0x00000000, 0x0FFE0000, 0x0F800000, 0x0FFF8000,
		0x0C000008, 0x0FFF8001, 0x00100001, 0x01000001, 0x00000001, 0x08000001, 0x08000000,
		0x5403A1E0, 0x5473B940, 0x4041A060, 0x54B2B8C0, 0x54B2B8C0, 0x08800001, 0x09800001,
		0x0A800001, 0x0B800001, 0x00000000, 0x00789242
	};
	for (size_t i = 0; i < std::size(defaults) && i < m_eeprom.size(); i++)
		m_eeprom[i] = defaults[i];

	// the PLX registers come up holding the EEPROM image from word 4 on. MAME copies 32 words
	// here and reads past the end of its 32-word EEPROM region doing so; this stops at the end
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
	// what it is complaining about
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
		poke32(0x18, 0);     // entries used
		poke32(0x1c, count); // ring capacity
		poke32(0x20, size);  // entry size
		poke32(0x24, 0);     // write index
		poke32(0x28, base);  // buffer base
		printf("p2k: seeding the NVRAM error log at %08x, %x entries of %x bytes\n", base, count, size);
	}
#endif

	// PC97317 Super I/O identity, read through ports 0x2e/0x2f. Without it the firmware's
	// io_setup_global() reports "SuperIOType unknown (0)" and every io_setup_* step after it
	// bails out - which is where the game code's failure chain starts
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
// receive interrupt would need a character to arrive, and nothing sends one
void p2k_state::update_uart_irq()
{
	if (m_pic1)
	{
		m_pic1->ir4_w((m_uart_reg [1] & 0x02) ? 1 : 0); // COM1
		m_pic1->ir3_w((m_uart2_reg[1] & 0x02) ? 1 : 0); // COM2
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
// Encore writes the vertical blank flag into the SRAM rather than deriving it on read. This is
// that, tried and left commented out because it changed nothing - and the XINA 1.38 wedge it was
// aimed at turned out to be the UART divisor latch, see port_w. Kept because it is an obvious
// thing to try twice. Two caveats if it is ever revived: it writes into the CMOS, which PinMAME
// saves to the .nv, and once per frame is the finest this hook offers where Encore updates thirty
// times a frame, so the flag alternates instead of pulsing briefly - a guest polling for a short 1
// could miss it either way
//
// static bool p2k_vblank_written()
// {
// #if P2K_DEBUG
// 	static const bool on = getenv("P2K_VBLANK_WRITE") != nullptr;
// 	return on;
// #else
// 	return false;
// #endif
// }

void p2k_state::push_switches(const u8 *matrix, unsigned count)
{
	// the SRAM-written vblank flag, see the note above:
	// if (p2k_vblank_written() && m_nvram.size() >= 8)
	// {
	// 	static u32 phase = 0;
	// 	const u32 v = (++phase & 1) ? 1u : 0u;
	// 	write_le(m_nvram, 4, v, 0xffffffff);
	// }

#if P2K_DEBUG
	// The display controller's vertical interrupt, off unless asked for. The databook gives it an
	// enable and a pending flag: DC_GENERAL_CFG bit 6, VIEN, generates "a vertical interrupt on the
	// occurrence of the next vertical sync pulse", and DC_TIMING_CFG bit 31, VINT, says one is
	// pending. Both games turn it on - DC_GENERAL_CFG reads 0x00106541, and 0x41 is bits 0 and 6 -
	// so they ask for a per-frame interrupt that this driver otherwise never delivers.
	//
	// XINA does not consume it. It was tried as the cause of the XINA 1.38 wedge and is not - that
	// was the UART divisor latch, see the note in port_w - and P2K_IDTDUMP shows why it could not
	// have been: every hardware vector holds a generic XINU trampoline except IRQ 0's clkint.
	// Delivered on IRQ 9 it is accepted and handled, changes nothing on swep1_210 and leaves
	// rfm_160 running normally. Kept because the enable is real and the signal is hardware this
	// driver otherwise ignores, but off by default and unfinished: the line is a guess (9 is where
	// a VGA-compatible retrace interrupt lands on an AT, and both games unmask it), and it arrives
	// at frame rate from this hook rather than at the real vertical sync.

	// P2K_IDTDUMP=1: the interrupt descriptor table, hardware vectors only, once. Which lines the
	// guest has handlers for is the thing to know before delivering an interrupt on one: an
	// unpopulated vector is a triple fault and the machine resets, which is exactly what happened
	// when the vertical interrupt below was first tried on IRQ 9
	{
		static const bool idtdump = getenv("P2K_IDTDUMP") != nullptr;
		static bool done = false;
		// Triggered on a protected-mode IDT appearing rather than at a frame number: limit 0x3ff
		// with base 0 is the real-mode vector table the machine starts on, and how many frames it
		// takes to leave it is not fixed
		// The base comes from a guest register, and this reads through mem_r, so it is bounded to
		// main RAM first: a probe that reads wherever a register happens to point can disturb the
		// machine it is measuring - reading the PLX control register advances the EEPROM, and one
		// stray read there stops the machine booting. An IDT outside RAM is not something to chase
		// by poking at it
		if (idtdump && !done && m_maincpu->idtr_limit() != 0x3ff &&
		    size_t(m_maincpu->idtr_base()) + 0x400 < m_main_ram.size())
		{
			done = true;
			const u32 base = m_maincpu->idtr_base();
			fprintf(stderr, "[p2k idt] base=%08x limit=%04x\n", base, m_maincpu->idtr_limit());
			for (unsigned v = 0x20; v <= 0x7f; v++)
			{
				const u32 e = base + v * 8;
				const u32 lo = mem_r(e, 0xffffffff), hi = mem_r(e + 4, 0xffffffff);
				const u32 off = (lo & 0xffff) | (hi & 0xffff0000);
				if ((hi & 0x8000) && off)   // present bit set and a real offset
					fprintf(stderr, "[p2k idt] vector %02x -> %08x  sel %04x  type %02x%s\n",
					        v, off, lo >> 16, (hi >> 8) & 0x1f,
					        (v >= 0x20 && v <= 0x27) ? "   (IRQ 0-7)"  :
					        (v >= 0x70 && v <= 0x77) ? "   (IRQ 8-15)" : "");
			}
			fflush(stderr);
		}
	}

	{
		static const char *const s = getenv("P2K_VBLANK_IRQ");
		static const int line = s ? int(strtol(s, nullptr, 0)) : -1;
		// Not before the guest has an interrupt table. VIEN is set at cycle 129444, by the boot ROM,
		// long before any handler exists - delivering from there walks into the real-mode vector
		// table and the machine triple-faults back to real mode, which is what the first attempt at
		// this did. The guest's IDT has limit 0x17f, 48 vectors, with the slave PIC remapped to
		// 0x28-0x2f rather than the PC's usual 0x70, so IRQ 9 is vector 0x29
		if (line >= 0 && (m_disp_ctrl_reg[DC_GENERAL_CFG] & 0x40) && m_maincpu->idtr_limit() != 0x3ff)
		{
			auto ir = [this](int l, int state) {
				switch (l)
				{
					case 2:  m_pic1->ir2_w(state); break;
					case 5:  m_pic1->ir5_w(state); break;
					case 9:  m_pic2->ir1_w(state); break;
					case 10: m_pic2->ir2_w(state); break;
					case 11: m_pic2->ir3_w(state); break;
					default: break;
				}
			};
			ir(line, 0);                                      // drop last frame's, so this is an edge
			// VINT, the pending flag. Set and never cleared here: the databook clears it when VIEN
			// goes to 0, and nothing in these games reads it, so the experiment does not model the
			// acknowledge side. It would have to before this became anything but an experiment
			m_disp_ctrl_reg[DC_TIMING_CFG] |= 0x80000000u;
			ir(line, 1);
		}
	}
#endif

	if (!matrix) return;
	if (count > sizeof(m_sw_matrix)) count = sizeof(m_sw_matrix);
	for (unsigned i = 0; i < count; i++) m_sw_matrix[i] = matrix[i];
}

// The switch matrix, by column, as the read side wants it: the caller's own array where one was
// handed over, so the game sees a switch at the moment it strobes for it, otherwise the pushed
// copy. wpc.c and se.c index coreGlobals.swMatrix from their read handlers in the same way.
//
// The board's own numbering, read out of the game's switch table (see src/p2k/README.md):
// switch number = 100 + (column-1)*8 + (row-1), with columns 1-9 the playfield matrix,
// column 10 the coin door's diagnostic buttons and column 11 the cabinet. Those two land in
// PinMAME's columns of the same number; the coin inputs use PinMAME's coin door column 0
u8 p2k_state::sw_column(unsigned col) const
{
	col &= 0xf;
	return m_sw_live ? m_sw_live[col] : m_sw_matrix[col];
}

// Hand over everything the board did since the last call and start a fresh window.
//
// Lamps clear to nothing, so a column the game stops strobing goes dark, as the bulb does.
// Solenoids reseed with the current level, so an output still held reads on in the next window and
// only a released one falls away. That asymmetry is se.c:182-193, and it is what turns a chopped
// burst into the single "on" it physically is without inventing one that was never driven
void p2k_state::pull_outputs(u8 * const lamps, unsigned lamp_columns, u32 * const solenoids, u32 * const solenoids2, u32 * const solNow, u32 * const sol2Now)
{
	if (solNow ) *solNow  = m_solenoids;
	if (sol2Now) *sol2Now = m_solenoids2;
	if (lamps)
	{
		if (lamp_columns > sizeof(m_lamp_acc)) lamp_columns = sizeof(m_lamp_acc);
		for (unsigned i = 0; i < lamp_columns; i++) { lamps[i] = m_lamp_acc[i]; m_lamp_acc[i] = 0; }
	}
	if (solenoids ) { *solenoids  = m_sol_acc;  m_sol_acc  = m_solenoids;  }
	if (solenoids2) { *solenoids2 = m_sol2_acc; m_sol2_acc = m_solenoids2; }
}

// Expand a 5 or 6 bit colour channel to 8 bits, replicating the high bits into the low ones so that the channel maximum maps to 255
static inline u8 expand5to8(const u32 v) { return u8((v << 3) | (v >> 2)); }
static inline u8 expand6to8(const u32 v) { return u8((v << 2) | (v >> 4)); }

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
			// 8 bit indexed, and the palette is not modelled - this hands back the index as grey, so a
			// game that used this mode would draw in shades rather than colour. No P2K set does: both
			// games run 15 bpp throughout, which is why it has never been worth wiring up.
			//
			// MAME has a whole path in src/mame/atari/mediagx.cpp. The table is fed
			// through memory_ctrl_w() at offset 0x20/4, which routes on DC_GENERAL_CFG bits 20-23:
			// 0x00000000 sets the index, 0x00100000 writes a component and post-increments it. Three
			// bytes per entry, six bits each, so drawing is r = pal[c*3+0] << 2 and so on. Our own
			// memory_ctrl_w stores and does nothing, so that is where the routing would go
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
					r = expand5to8((c >> 11) & 0x1f);
					g = expand6to8((c >> 5) & 0x3f);
				}
				else                                            // RGB555
				{
					r = expand5to8((c >> 10) & 0x1f);
					g = expand5to8((c >> 5) & 0x1f);
				}
				b = expand5to8(c & 0x1f);
			}
			dest[offs] = (u32(r) << 16) | (u32(g) << 8) | b;
		}
	}
	return true;
}

// P2K_PPM=<path> writes that picture once, after P2K_PPM_AT cycles (20 emulated seconds by
// default), as a binary PPM - a way to see whether it is right before there is anywhere to show it
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
	return 0; // the state interface is not modelled; the trace prints bus activity instead
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
		u32 t = 0x10000000; // bit 28 always set: EEPROM present and OK
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
			t &= ~(1u << 27); // the transfer starts with a zero bit
			// Start where the READ command asked to, which prism_1000_w decoded on the way in.
			// Two words to the element, high half first, so the word address splits into an index
			// and a half - and the counter has to be primed here rather than left to whatever the
			// previous frame ended on, or an odd address streams out of step
			m_prism_eprom_offset = m_prism_ee_read_word / 2;
			m_prism_eprom_wordtoggle = m_prism_ee_read_word & 1;
			m_prism_eprom_counter = 16;
		}
		else
		{
			// Nothing is being clocked out, so this is either idle or the ready poll that follows
			// a write: the chip holds its answer line low while the write cycle runs and takes it
			// high when it is done, and this one is done as soon as it is asked for. Without an
			// answer here `plx_eeprom_write` waits for a timeout it measures in interval-timer
			// ticks, which do not run yet this early in the boot - see prism_1000_w
			t &= ~(1u << 27);
			if (m_prism_ee_ready) t |= 1u << 27;
		}
		return t;
	}

	if (offset == 0x13) return m_eeprom_regs[offset] | 0x4;
	return m_eeprom_regs[offset];
}

// The write side of register 0x14 is a 93C46 - 64 words of 16 bits, which is what
// `plx_eeprom_write` bounds its address against. Commands are bit-banged: chip select is bit 25,
// the clock bit 24, the bit going in bit 26, and the chip answers on bit 27. A frame is the bits
// clocked in while chip select is high, MSB first, and the chip acts on it when chip select drops:
// nine bits `1 01 aaaaaa` plus sixteen data bits is a word write, and nine bits `1 00 11xxxx` /
// `1 00 00xxxx` are the write enable and disable that have to bracket it.
//
// This exists because the firmware writes the EEPROM: `plx_ee_verify` reads the image back, and
// if it does not match what `plx_ee_init` wants it rewrites it. The prototype sets (`rfm_080`,
// `rfm_120`) do exactly that on every boot, and without a write path the poll after the first
// word never ends - its escape is a timeout counted in interval-timer ticks, and the interval
// timer is not running yet, the PIT being unprogrammed and every interrupt still masked. The
// image is in NVRAM, so once written it stays written and the next boot verifies clean
void p2k_state::prism_1000_w(offs_t offset, u32 data)
{
	offset &= 0x3f;
	if (offset == 0x14)
	{
		const bool cs = (data & 0x02000000) != 0;
		const bool sk = (data & 0x01000000) != 0;
		const bool di = (data & 0x04000000) != 0;

		if (cs && !m_prism_ee_cs)                 // a frame starts when chip select rises
			m_prism_ee_frame = 0, m_prism_ee_nbits = 0;
		if (cs && sk && !m_prism_ee_sk)           // and takes one bit per rising clock edge
		{
			m_prism_ee_ready = false;
			if (m_prism_ee_nbits < 32) m_prism_ee_frame = (m_prism_ee_frame << 1) | (di ? 1u : 0u);
			m_prism_ee_nbits++;
			// A READ is `1 10 aaaaaa` and then the chip clocks words out for as long as chip select
			// stays high, so the address has to be taken here, mid-frame - by the time it drops the
			// read is long over. The firmware asks for word 0 and streams all 64 in one frame, so
			// this changes nothing today; it is what makes any other address right
			if (m_prism_ee_nbits == 9 && (m_prism_ee_frame >> 6) == 0x6)
				m_prism_ee_read_word = int(m_prism_ee_frame & 0x3f);
		}
		if (!cs && m_prism_ee_cs)                 // and is acted on when it drops
		{
			static unsigned reported = 0;         // both reports below are once-only, as elsewhere here
			if (m_prism_ee_nbits == 9 && (m_prism_ee_frame >> 6) == 0x4)
				m_prism_ee_wen = (m_prism_ee_frame & 0x30) == 0x30;   // EWEN, against EWDS
			else if (m_prism_ee_nbits == 25 && (m_prism_ee_frame >> 22) == 0x5)
			{
				// One word per frame, and the vector holds two of them per element, high half
				// first - the same packing the read side above streams out
				const unsigned word = (m_prism_ee_frame >> 16) & 0x3f;
				const u16 value = u16(m_prism_ee_frame);
				if (m_prism_ee_wen && word / 2 < m_eeprom.size())
				{
					u32 &slot = m_eeprom[word / 2];
					slot = (word & 1) ? ((slot & 0xffff0000u) | value)
					                  : ((slot & 0x0000ffffu) | (u32(value) << 16));
				}
				else if (!m_prism_ee_wen && reported < 8)
				{
					reported++;
					fprintf(stderr, "[p2k plx] EEPROM word %02x written without a write enable\n", word);
				}
				m_prism_ee_ready = true;
			}
			else if ((m_prism_ee_nbits > 25 && (m_prism_ee_frame >> 29) == 0x6) ||
			         (m_prism_ee_nbits == 9 && (m_prism_ee_frame >> 6) == 0x6))
				;                     // a read, which the streaming path above answers - either
				                      // clocked out in this frame, or the command on its own
			else if (m_prism_ee_nbits && reported < 8)
			{
				reported++;
				fprintf(stderr, "[p2k plx] EEPROM frame of %d bits (%08x) not understood\n", m_prism_ee_nbits, m_prism_ee_frame);
			}
		}
		m_prism_ee_cs = cs;
		m_prism_ee_sk = sk;

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
// the game uses are implemented there, and the blit is one row of `width` pixels

#if P2K_DEBUG
// Consecutive GP_BLT_STATUS reads with nothing happening in between - see gx_pipeline_r
static u64 g_blt_status_run = 0;
#endif

u32 p2k_state::gx_pipeline_r(offs_t offset) const
{
#if P2K_DEBUG
	// P2K_GPWATCH also counts reads of GP_BLT_STATUS. Its bottom three bits - BLT Busy, Pipeline
	// Busy and BLT Pending - are the handshake the databook tells software to use before touching
	// the frame buffer, a BLT buffer or the pipeline registers. The blit here runs to completion
	// inside the register write that starts it, so all three are always clear by the time the guest
	// can look, which is the truthful answer for a blitter that is already finished. Worth knowing
	// whether anything actually asks
	if ((offset & 0x7f) == GP_BLT_STATUS)
	{
		static const bool gpwatch = getenv("P2K_GPWATCH") != nullptr;
		static u64 reads = 0, next = 1;
		if (gpwatch && ++reads >= next)
		{
			next *= 10;
			fprintf(stderr, "[p2k gp] GP_BLT_STATUS read %llu times, answering %08x\n", (unsigned long long)reads, m_gx_pipeline_reg[GP_BLT_STATUS]);
			fflush(stderr);
		}

		// A blitter that finishes inside the write that starts it never reads as busy, and software
		// written for real timing may wait for the busy bit to *appear* before waiting for it to
		// clear. That loop never ends here, and could not fail on the hardware - a deadlock this
		// driver would be causing on its own. Cheap to detect: count reads with no blit and no
		// pipeline register write in between, which g_blt_status_run is reset by, and report a run
		// far longer than any real handshake
		if (++g_blt_status_run == 200000)
		{
			fprintf(stderr, "[p2k gp] GP_BLT_STATUS read 200000 times with no blit in between - the guest may be waiting for a busy bit this driver never sets\n");
			fflush(stderr);
		}
	}
#endif
	return m_gx_pipeline_reg[offset & 0x7f];
}

void p2k_state::gx_pipeline_w(offs_t offset, u32 data, u32 mem_mask)
{
	offset &= 0x7f;
	// The register is stored before anything acts on it, not after: "writing to this register
	// initiates a BLT operation", so the value being written is that BLT's mode, not the previous
	// one. It made no difference while nothing in the blit read GP_BLT_MODE; it does now that the
	// colour key follows the buffer its bits 4:2 select, and the first transparent blit of a run
	// used to see the register still at 0. GP_BLT_STATUS's bottom three bits - BLT Busy, Pipeline
	// Busy, BLT Pending - are read only (Table 4-25), while bits 8 and 9 in the same register are
	// not, so the write is masked rather than dropped
	if (offset == GP_BLT_STATUS) mem_mask &= ~0x7u;
	{
		u32 &r = m_gx_pipeline_reg[offset];
		r = (r & ~mem_mask) | (data & mem_mask);
	}
#if P2K_DEBUG
	// The pipeline registers are master/slave: on a BLT the masters latch into the slaves, and a
	// register the guest does *not* write before the next BLT is not simply reused - databook Table
	// 4-21 says the destination Y holds its slave value while the source Y advances by +/- the
	// height, for a bitmap source. This driver has one register file and reuses whatever is in it,
	// so a game that leant on that auto-advance would re-blit the same source row here and draw a
	// smear where the hardware draws an image. Report a BLT that did not rewrite its coordinates,
	// once, with what it would have inherited
	{
		static bool wrote_dst = false, wrote_src = false, reported = false;
		if (offset == GP_DST)   wrote_dst = true;
		if (offset == GP_SRC_X) wrote_src = true;
		if (offset == GP_BLT_MODE && data > 0)
		{
			if (!reported && (!wrote_dst || !wrote_src))
			{
				reported = true;
				fprintf(stderr, "[p2k blit] BLT without rewriting %s%s%s - the hardware would have advanced the source Y by the height (master/slave, databook Table 4-21); this reuses dst=%08x src=%08x\n",
				        wrote_dst ? "" : "GP_DST", (!wrote_dst && !wrote_src) ? " and " : "",
				        wrote_src ? "" : "GP_SRC_X",
				        m_gx_pipeline_reg[GP_DST], m_gx_pipeline_reg[GP_SRC_X]);
				fflush(stderr);
			}
			wrote_dst = wrote_src = false;
		}
	}
#endif

	if (data > 0 && (offset == GP_BLT_MODE || offset == GP_VECTOR_MODE))
	{
		if (offset == GP_BLT_MODE) do_gfx_pipeline();
#if P2K_DEBUG
		// Vector mode does solid fills, which is how a screen gets cleared. It was never implemented
		// here or in the MAME driver this came from, and a fill that does nothing leaves the previous
		// picture behind for the next one to draw over. Report it, with the registers a fill would
		// need, so it is visible when a game asks for one
		else if (offset == GP_VECTOR_MODE)
		{
			static unsigned reported = 0;
			if (reported < 8)
			{
				reported++;
				fprintf(stderr, "[p2k blit] vector mode %08x asked for and not implemented - dst=%08x width=%04x raster=%02x\n",
				        data, m_gx_pipeline_reg[GP_DST], unsigned(m_gx_pipeline_reg[GP_WIDTH] & 0xffff), unsigned(m_gx_pipeline_reg[GP_RASTER_MODE] & 0xff));
				fflush(stderr);
			}
		}
#endif
	}
#if P2K_DEBUG
	// P2K_GPWATCH=1: which pipeline registers a game actually programs - each register's first
	// write, then every later *change* to the ones the blit never reads, being the raster mode's
	// upper bits and the six that sit at ffffffff. This is what established the register map at the top of this file
	{
		static const bool gpwatch = getenv("P2K_GPWATCH") != nullptr;
		static bool seen[128] = {};
		static u32 last[128] = {};
		const unsigned o = offset & 0x7f;
		const bool watched = (o == 0x100/4) || (o == 0x10/4) || (o == 0x14/4) || (o >= 0x20/4 && o <= 0x2c/4);
		if (gpwatch && (!seen[o] || (watched && data != last[o])))
		{
			seen[o] = true; last[o] = data;
			fprintf(stderr, "[p2k gp] register %03x = %08x", unsigned(offset) * 4u, data);
			fputc(10, stderr);
			fflush(stderr);
		}
	}

	g_blt_status_run = 0; // something happened, so any status poll was not a spin
#endif
}

void p2k_state::do_gfx_pipeline()
{
	// BLT Busy, Pipeline Busy and BLT Pending, which the firmware polls. Never observable: the blit
	// completes inside the register write that started it, so the guest only ever sees them clear -
	// truthful for a blitter already finished, and harmless while nothing waits for busy to appear
	m_gx_pipeline_reg[GP_BLT_STATUS] |= 0x7;

	const int line_delta = int((m_disp_ctrl_reg[DC_LINE_DELTA] & 0x3ff) << 1);   // dwords -> words
	const u8 rastermode = u8(m_gx_pipeline_reg[GP_RASTER_MODE] & 0xff);
	const int x     = int(m_gx_pipeline_reg[GP_DST] & 0xffff); //!! should these all stay in 16bit, and also the loop-arithmetic on them (NOT including the actual final address computation!)
	const int y     = int(m_gx_pipeline_reg[GP_DST] >> 16);
	const int src_x = int(m_gx_pipeline_reg[GP_SRC_X] & 0xffff);
	const int src_y = int(m_gx_pipeline_reg[GP_SRC_X] >> 16);
	const int width = int(m_gx_pipeline_reg[GP_WIDTH] & 0xffff);

	// Width 0 draws nothing, which is what already happens: cols clamps to it and every path below
	// is skipped. The databook settles it - "no pixels are rendered for a width of zero", and the
	// same sentence for height, which is why the row loop takes height at face value
	//
	// Two fields below disagree with the databook, and are left as they are because they are what
	// renders correctly. Table 4-25 puts PIXEL_WIDTH in GP_WIDTH bits 31:16 and PIXEL_HEIGHT in
	// 15:0, and SRC_X in bits 31:16 of GP_SRC with SRC_Y in 15:0 - the opposite of both readings
	// here. GP_DST it agrees with (Y high, X low), so the asymmetry is the databook's, stated the
	// same way in Tables 4-24 and 4-25. Measured on rfm_160: GP_WIDTH = 0x000100ad and GP_SRC =
	// 0x02d00000, so the databook reads that blit as 1 wide by 173 tall from column 720, and this
	// driver as 173 wide by 1 tall from row 720. Both are self-consistent - one draws the line
	// vertically, the other horizontally - and only this one produces a correct picture, on every
	// set. Swapping to match the databook transposes every blit. Worth revisiting only with a
	// second source; see README.md

#if P2K_DEBUG
	// Every path below assumes the pattern is all ones. Table 4-22 makes the ROP a per-bit truth
	// table over pattern, source and destination, so a pattern bit stuck at 1 means only ROP bits
	// 4-7 are ever selected - which is what reduces both 0xc6 and 0xcc to a plain source copy, and
	// why the databook requires "the pattern registers must be all F's" for the transparent copy.
	// The games set GP_PAT_COLOR_A/B and GP_PAT_DATA_0-3 to ffffffff once and never touch them
	// again. If one ever did, every mode here would be drawing the wrong thing
	{
		// GP_PAT_COLOR_A/B then GP_PAT_DATA_0-3, by byte offset
		static constexpr unsigned PATTERN_REGS[] = { 0x10/4, 0x14/4, 0x20/4, 0x24/4, 0x28/4, 0x2c/4 };
		static bool reported = false;
		for (const unsigned p : PATTERN_REGS)
			if (m_gx_pipeline_reg[p] != 0xffffffffu && !reported)
			{
				reported = true;
				fprintf(stderr, "[p2k blit] pattern register %02x is %08x, not all ones - every "
				                "raster mode here assumes it is\n", p * 4u, m_gx_pipeline_reg[p]);
				fflush(stderr);
			}
	}

	// The four the games (supposedly only) use have their own paths below; anything else goes through the general
	// ROP rather than drawing nothing, but report anyway, so we could optimize
	if (rastermode != 0x00 && rastermode != 0xff && rastermode != 0xc6 && rastermode != 0xcc)
	{
		static bool seen[256] = {};
		if (!seen[rastermode])
		{
			seen[rastermode] = true;
			fprintf(stderr, "[p2k blit] raster mode %02x taking the general ROP path - no set has been seen to ask for one yet\n", unsigned(rastermode));
			fflush(stderr);
		}
	}
#endif

	const size_t pixels = m_vram.size() / 2;
	auto vram16 = [this](size_t i) -> u16 & { return *reinterpret_cast<u16 *>(&m_vram[i * 2]); };

#if P2K_DEBUG
	// What a copy does when its source falls outside VRAM is unknown - the hardware presumably
	// returns something and writes it, so the destination would change. Raster 0xc6 reads anyway;
	// 0xcc stops at the end of VRAM, because it moves the row in one block and cannot step past the
	// buffer safely. Either way the tail of such a row is wrong, and a region left exactly as it
	// was is indistinguishable from one that never repaints - hence the report. No set has triggered it so far
	auto note_oor = [&](unsigned mode, size_t si) {
		static unsigned n = 0;
		if (n++ < 12)
		{
			fprintf(stderr, "[p2k blit] raster %02x source out of VRAM: src=%d,%d dst=%d,%d w=%d si=%zu limit=%zu", mode, src_x, src_y, x, y, width, si, pixels);
			fputc(10, stderr);
			fflush(stderr);
		}
	};
#endif

	// The transparent copy's key colour is neither a constant nor a pipeline register: the databook
	// puts it "in the BLIT buffer as destination data", the rest of that sentence being the
	// all-ones pattern the check above guards. P2K_KEYWATCH found it - rfm_160 fills
	// 0x40000400-0x400008ff, 640 words and exactly one row, with 0x7c1f before drawing anything -
	// so 0x7c1f is this firmware's choice rather than a hardware default. Every word of the row is
	// the same, so whether the hardware keys on one value or column by column cannot be told apart
	// here; the first is taken and the check below watches that.
	//
	// Which buffer is GP_BLT_MODE bits 4:2, and where it sits comes from the CPU-access registers
	// the guest programs with CPU_WRITE: 010 is Buffer 0 at L1_BB0_BASE, 011 Buffer 1 at
	// L1_BB1_BASE. Measured on rfm_160: BB0_BASE=0x930, BB1_BASE=0x400, so XINA swaps them against
	// Table 4-5's layout - and every transparent blit selects Buffer 1, the 0x400 the key is written
	// to. Reading offset 0 unconditionally was right only by that coincidence. 000 means no
	// destination data, all ones into the raster unit; 100/101 take it from the frame buffer, which
	// is not a colour key and has not been seen with C6h.
	//
	// Not modelled: 4.4.1 stages each source scan line into a BLT buffer as the hardware blits, so
	// Buffer 0 would hold the last line copied. This reads VRAM directly, and nothing reads it back
	const unsigned blt_rd = (m_gx_pipeline_reg[GP_BLT_MODE] >> 2) & 7u;
	const u32 key_base = (blt_rd == 2) ? m_maincpu->cpu_access_reg(mediagx_device::L1_BB0_BASE)
	                   : (blt_rd == 3) ? m_maincpu->cpu_access_reg(mediagx_device::L1_BB1_BASE)
	                                   : 0u;
	const size_t key_index = (key_base >= 0x400 && key_base < 0x1000)
	                       ? size_t(key_base - 0x400) / 4 : 0;
	const u16 color_key = key_base ? u16(m_scratchpad[key_index] & 0xffff) : 0xffff;

#if P2K_DEBUG
	// A transparent copy that does not take its key from a BLT buffer. 010 and 011 are the two the
	// key can come from and the only ones seen; 000 is defined - no destination data, so the raster
	// unit sees all ones and 0xffff is keyed out - while 100 and 101 read the destination from the
	// frame buffer, which is not a colour key at all and is untested here. Any of them keys on the
	// wrong colour rather than failing, so it would show as transparency going wrong in one place
	// and nothing else. Reported once per distinct value, like the rest of the should-never-happens
	if (rastermode == 0xc6 && blt_rd != 2 && blt_rd != 3)
	{
		static bool seen[8] = {};
		if (!seen[blt_rd])
		{
			seen[blt_rd] = true;
			fprintf(stderr, "[p2k blit] transparent copy with GP_BLT_MODE RD=%u%u%u - the key is not coming from a BLT buffer, so %04x is keyed out on a guess\n",
			        (blt_rd >> 2) & 1u, (blt_rd >> 1) & 1u, blt_rd & 1u, unsigned(color_key));
			fflush(stderr);
		}
	}

	// P2K_KEYWATCH=1: which buffer a transparent copy keyed on and what it found there, reported
	// whenever any of it changes. The write half of the same watch, in mem_w, is what located the
	// buffer in the first place. A set that lays its buffers out differently shows up here rather
	// than silently keying on the wrong colour
	if (rastermode == 0xc6)
	{
		static const bool keywatch = getenv("P2K_KEYWATCH") != nullptr;
		static u32 lastmode = 0xffffffff, lastkey = 0xffffffff;
		if (keywatch && (m_gx_pipeline_reg[GP_BLT_MODE] != lastmode || color_key != lastkey))
		{
			lastmode = m_gx_pipeline_reg[GP_BLT_MODE];
			lastkey = color_key;
			fprintf(stderr, "[p2k key] 0xc6 blit: BLT_MODE=%08x (RD=%u RS=%u)  BB0_BASE=%03x BB1_BASE=%03x  key from %03x = %04x\n",
			        m_gx_pipeline_reg[GP_BLT_MODE], blt_rd, m_gx_pipeline_reg[GP_BLT_MODE] & 3u,
			        m_maincpu->cpu_access_reg(mediagx_device::L1_BB0_BASE),
			        m_maincpu->cpu_access_reg(mediagx_device::L1_BB1_BASE),
			        unsigned(0x400 + key_index * 4), unsigned(color_key));
			fflush(stderr);
		}
	}

	// The assumption above, checked: a row that is not uniform means the hardware is being asked
	// for per-column destination data, which one key cannot express, and the copy below would draw
	// the wrong thing with no other sign of it. 320 compares against a copy of 640 pixels, never detected so far
	// Only re-scanned when the buffer or the selected base has actually changed - the games fill it
	// once and blit from it thousands of times, and scanning 320 dwords on each transparent blit
	// cost more than the blit itself, which is ~173 pixels
	static u64 checked_gen = ~0ull;
	static size_t checked_idx = ~size_t(0);
	if (rastermode == 0xc6 && (m_scratchpad_gen != checked_gen || key_index != checked_idx))
	{
		checked_gen = m_scratchpad_gen;
		checked_idx = key_index;
		for (unsigned k = 1; k < 0x500 / 4; k++)   // 320 dwords = 640 words = the row the games fill
			if (key_index + k < std::size(m_scratchpad) && m_scratchpad[key_index + k] != m_scratchpad[key_index])
			{
				static unsigned reported = 0;
				if (reported++ < 8)
				{
					fprintf(stderr, "[p2k blit] BLT buffer is not one colour: word %u = %08x against %08x at word 0 - the transparent copy keys on word 0 alone\n",
					        k, m_scratchpad[key_index + k], m_scratchpad[key_index]);
					fflush(stderr);
				}
				break;
			}
	}
#endif

	// Note: A P2K dev responsible for the firmware's graphics pipeline code said that they only ever
	// used a single line blit for performance reasons. BUT unknown if this is always true,
	// especially for the newer homebrew versions!

	// The high half of GP_WIDTH is the height, but the MAME driver this came from read only the width and drew a single row. Taking it as a
	// height costs nothing and would handle a rectangular blit if one ever arrived - but nothing
	// has been seen to send one. Every set measured writes exactly 1 here, through boot, attract
	// and the service menu, so this loop always runs once
	const int height = int(m_gx_pipeline_reg[GP_WIDTH] >> 16);
	for (int i = 0; i < height; i++)
	{
		//!! also in theory, it should respect y-mirroring/reverse, but apparently never triggered, as height == 1, so..
		const size_t row = size_t(y + i) * size_t(line_delta);
		const size_t src_row = size_t(src_y + i) * size_t(line_delta);
		// Where the row runs off the end of VRAM is fixed before the loop rather than tested inside it:
		// the destination advances one pixel per step, so the point it crosses is arithmetic
		// Note the HW does not clip writes!
		const size_t dst_base = row + size_t(x);
		size_t cols = (dst_base < pixels) ? (pixels - dst_base) : 0;
		if (cols > size_t(width)) cols = size_t(width);

		// BLACKNESS and WHITENESS write a constant to every pixel of the row and read no source, so
		// they are a memset of the row rather than a per-pixel switch. Both fill bytes happen to
		// equal the raster mode itself: 0x00 -> 0x0000, 0xff -> 0xffff
		if (rastermode == 0x00 || rastermode == 0xff)
		{
			if (cols) memset(&m_vram[dst_base * 2], rastermode, cols * 2);
			continue;
		}

		const size_t src_base = src_row + size_t(src_x);
		// SRCCOPY reads one contiguous run and writes another, so the row is a single move. The
		// source is clipped separately from the destination: the row length above bounds only the
		// write, and a block move reading past the end of VRAM would run off the vector in one go,
		// where the per-pixel version stepped past one element at a time. The out-of-range report
		// therefore moves up here too - it fires once for the row instead of once per pixel, with
		// the first offending source offset. memmove rather than memcpy because a blit whose source
		// and destination overlap is not forbidden by anything here; none has been seen
		if (rastermode == 0xcc)
		{
			size_t n = (src_base < pixels) ? (pixels - src_base) : 0;
			if (n > cols) n = cols;
#if P2K_DEBUG
			if (n < cols) note_oor(0xcc, src_base + n); //!! if this triggers, what does the HW do? apparently just allow the read!
#endif
			if (n) memmove(&m_vram[dst_base * 2], &m_vram[src_base * 2], n * 2);
			continue;
		}

		// Raster 0xc6, the transparent copy: every pixel is tested against the key colour before it
		// is written. The source is read past the end of VRAM here rather than clipped - what the
		// hardware returns is the open question note_oor exists tries to catch
		if (rastermode == 0xc6)
		{
			for (size_t j = 0; j < cols; j++)
			{
				const size_t di = dst_base + j;
				const size_t si = src_base + j;
				const u16 pixel = vram16(si);
				if (pixel != color_key) vram16(di) = pixel;
#if P2K_DEBUG
				if (si >= pixels) note_oor(0xc6, si); //!! if this triggers, what does the HW do? apparently just allow the read!
#endif
			}
			continue;
		}

		// Any other ROP, from its truth table. With the pattern all ones only bits 4-7 are
		// reachable, one per (source, destination) bit pair, so the whole operation is four masks
		// over 16-bit words. It reproduces the four fast paths above exactly - 0xcc and 0xc6 both
		// give bits 4-7 = 1100, or "output = source", and 0x00/0xff give all zeroes and all ones -
		// which is the check that this is the right reading of the table
		const u16 m00 = (rastermode & 0x10) ? 0xffffu : 0u; // source 0, destination 0
		const u16 m01 = (rastermode & 0x20) ? 0xffffu : 0u; // source 0, destination 1
		const u16 m10 = (rastermode & 0x40) ? 0xffffu : 0u; // source 1, destination 0
		const u16 m11 = (rastermode & 0x80) ? 0xffffu : 0u; // source 1, destination 1
		for (size_t j = 0; j < cols; j++)
		{
			const size_t di = dst_base + j;
			const size_t si = src_base + j;
			const u16 s = vram16(si), d = vram16(di);
			vram16(di) = u16((~s & ~d & m00) | (~s & d & m01) | (s & ~d & m10) | (s & d & m11));
#if P2K_DEBUG
			if (si >= pixels) note_oor(rastermode, si); //!! if this triggers, what does the HW do? apparently just allow the read!
#endif
		}
	}

#if P2K_DEBUG
	// The distinct values of GP_WIDTH's high half, the height the row loop above reads. Reported so
	// it stays knowable whether any game ever asks for more than the one row
	{
		static const bool gpwatch2 = getenv("P2K_GPWATCH") != nullptr;
		const unsigned hi = unsigned(m_gx_pipeline_reg[GP_WIDTH] >> 16);
		static bool seen_hi[8] = {};
		const unsigned slot = hi > 6 ? 7 : hi;
		if (gpwatch2 && !seen_hi[slot])
		{
			seen_hi[slot] = true;
			fprintf(stderr, "[p2k blit] GP_WIDTH high half (height?) = %u seen", hi);
			fputc(10, stderr);
			fflush(stderr);
		}
	}
	// P2K_FILLWATCH=1: the solid fills only - raster 00 and ff - with where they land. A screen that
	// is not cleared is a fill that went somewhere the display is not reading from, and the
	// destination here is relative to VRAM base 0 while the display reads from DC_FB_ST_OFFSET
	static const bool fillwatch = getenv("P2K_FILLWATCH") != nullptr;
	if (fillwatch && (rastermode == 0x00 || rastermode == 0xff))
	{
		static unsigned n = 0;
		if (n++ < 60)
			fprintf(stderr, "[p2k fill] raster %02x dst=%d,%d w=%d h=%d delta=%d -> vram %08x  fb_start=%08x\n",
			        unsigned(rastermode), x, y, width, height, line_delta,
			        unsigned(size_t(y) * size_t(line_delta) + size_t(x)) * 2u,
			        m_disp_ctrl_reg[DC_FB_ST_OFFSET]);
	}
#endif
	m_gx_pipeline_reg[GP_BLT_STATUS] &= 0xfffffff8; // done, in the same write that set it busy
}

// Active and total vertical lines, from the timings the game programs rather than a constant.
// DC_V_TIMING_1 packs active-1 in its low half and total-1 in its high half: e.g. Episode I
// writes 0x010400ef, which is 240 active of 261 total. The 525 this used to assume is the
// VGA 640x480 default, describing the output after the line doubling rather than what the
// controller counts, and is about twice the real figure. MAME and Encore's counter runs 0..241, which is
// the same number from the other direction. Falls back to the old constants before the game has programmed anything
void p2k_state::video_lines(unsigned &active, unsigned &total) const
{
	const u32 vt = m_disp_ctrl_reg[DC_V_TIMING_1];
	active = (vt & 0x7ff) + 1;
	total  = ((vt >> 16) & 0x7ff) + 1;
	if (vt == 0 || total <= active) { active = VIDEO_ACTIVE_LINES; total = VIDEO_LINES; }
}

// Where the beam is, from the machine's own clock: a position on the display depends on the video
// timings and not on how fast the CPU runs, so this is independent of the MediaGX clock and does
// not need revisiting when that changes. What it replaced did once depend on it - a per-timeslice
// counter, and PinMAME hands out slices of one frame, which at the 20 MHz this was first written
// for came to 333333 cycles against 525*635 = 333375 for a synthesized frame. Near enough that the
// counter aliased and the line barely moved: it read a constant 0x1d8, and the firmware's frame
// callback, which only acts while the line is below 10, never fired. Those numbers are why the old
// approach failed, not a description of this one
u32 p2k_state::video_line() const
{
	unsigned active, total; video_lines(active, total);
	const u64 ns = u64(m_machine->machine().time().as_double() * 1e9);
	const u64 ns_per_line = 1000000000ull / (u64(total) * VIDEO_FRAMES_PER_SECOND);
	return u32((ns / (ns_per_line ? ns_per_line : 1)) % total);
}

// Past the last active line is blanking, which is what MAME tests as `vpos() >= m_frame_height`
bool p2k_state::in_vblank() const
{
	unsigned active, total; video_lines(active, total);
	return video_line() >= active;
}

u32 p2k_state::disp_ctrl_r(offs_t offset) const
{
	offset &= 0x3f;
	// The vertical line counter has to advance on its own - the MAME driver keeps it moving with
	// a per-scanline timer tied to its screen device (`m_disp_ctrl_reg[0x54/4] = scanline`). This
	// port has no screen yet, so the value is derived from emulated time instead - see video_line(),
	// which counts the lines the controller is actually programmed for. Anything waiting for the
	// display to move sees it move.
	// Bit 30 of DC_TIMING_CFG is a vertical blank status: set during active display, clear while
	// blanking. MAME's own MediaGX driver does this - src/mame/atari/mediagx.cpp, `r |= 0x40000000;
	// if (m_screen->vpos() >= m_frame_height) r &= ~0x40000000;` - and the pinball2k driver this
	// port came from dropped it along with the screen device it needed. Without it the register
	// reads back exactly what was written, so anything polling for the edge waits for ever. The
	// games do write this register: 0x0002804f and 0x0002806f, so they know it is there. A status bit frozen at whatever was
	// last written is wrong however little depends on it here
	if (offset == DC_TIMING_CFG)
	{
#if P2K_DEBUG
		// P2K_DISPWATCH counts the reads. Measured: **once**, during boot, and never again in a
		// minute of running - on rfm_160 at cycle 129444 and swep1_210 at 103556. So nothing here seems(!)
		// poll this register so far, which is why supplying the bit properly changed no behaviour, and why
		// MAME's spin_until_interrupt in this branch would buy nothing: it exists to skip a guest
		// burning host time in a vblank poll, and these games do not have one (needs more verification in-game though!)
		static const bool watch = getenv("P2K_DISPWATCH") != nullptr;
		if (watch)
		{
			static u64 reads = 0, next = 1;
			if (++reads >= next)
			{
				next *= 10;
				fprintf(stderr, "[p2k disp] DC_TIMING_CFG read %llu times by cycle %llu", (unsigned long long)reads, (unsigned long long)g_p2k_cycles_total);
				fputc(10, stderr); fflush(stderr);
			}
		}
#endif
		// Bit 30 is VNA, "Vertical Not Active", and the databook pins its polarity down: it "cor-
		// responds to VGA port 3BA/3DA bit 3", which is set during vertical retrace. So it reads 1
		// while blanking and 0 during active display. MAME's MediaGX driver has it the other way
		// round - `r |= 0x40000000; if (vpos >= frame_height) r &= ~0x40000000;` - and this was
		// copied from there before the databook was to hand. Nothing here polls the register, so
		// the inversion changed nothing either way, but a status bit is either right or it is not
		const u32 r = m_disp_ctrl_reg[DC_TIMING_CFG] & ~0x40000000u;
		return in_vblank() ? (r | 0x40000000u) : r;
	}
	if (offset == DC_V_LINE_CNT)
	{
#if P2K_DEBUG
		// Counted because the guest detects vertical blank by polling it: gx_0_25ms, called from
		// the 0.25 ms interval task, reads this and signals dispmgr's semaphore when the value
		// crosses 240. So a counter this driver synthesises from emulated time drives the guest's
		// per-frame display heartbeat - measured at one read per ~19,100 cycles, the tick rate
		static const bool watch = getenv("P2K_DISPWATCH") != nullptr;
		if (watch)
		{
			static u64 reads = 0, next = 1;
			if (++reads >= next)
			{
				next *= 10;
				fprintf(stderr, "[p2k disp] DC_V_LINE_CNT read %llu times by cycle %llu, now %u\n",
				        (unsigned long long)reads, (unsigned long long)g_p2k_cycles_total, video_line());
				fflush(stderr);
			}
		}
#endif
		return video_line();
	}
	return m_disp_ctrl_reg[offset];
}
void p2k_state::disp_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_disp_ctrl_reg[offset & 0x3f];
	const u32 before = r;
	r = (r & ~mem_mask) | (data & mem_mask);

	// P2K_DISPWATCH=1: every write that changes a display controller register, with the PC that made it.
	// Which registers a game programs, and when, is the difference between a picture and a black screen
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

// Stores only. MAME's MediaGX driver uses offset 0x20/4 here as the palette port, routed by
// DC_GENERAL_CFG bits 20-23 - see the note in the 8 bit branch of frame_rgb(). Nothing here
// needs it while all games run 15 bpp
void p2k_state::memory_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_memory_ctrl_reg[offset & 0x3f];
	r = (r & ~mem_mask) | (data & mem_mask);
}

// The Internal Bus Interface Unit, four registers at GX_BASE+8000h (databook Table 4-9/4-10):
// BC_DRAM_TOP, then BC_XMAP_1 to _3. Stored and otherwise ignored, which is only safe as long as
// the guest leaves them alone - BC_XMAP_1 decides whether A0000/B0000/B8000 go to RAM or to the
// graphics pipeline (bits 4, 20 and 28), and whether VGA I/O at 3C0-3DF traps to SMM (bits 13-15),
// both of which this driver hardcodes the other way. P2K_BIUWATCH=1 reports what is actually there
u32 p2k_state::biu_ctrl_r(offs_t offset) const
{
#if P2K_DEBUG
	static const bool biuwatch = getenv("P2K_BIUWATCH") != nullptr;
	static bool seen_r[4] = {};
	if (biuwatch && (offset & 0x3f) < 4 && !seen_r[offset & 3])
	{
		seen_r[offset & 3] = true;
		fprintf(stderr, "[p2k biu] read  %s -> %08x\n",
		        (offset & 3) == 0 ? "BC_DRAM_TOP" : (offset & 3) == 1 ? "BC_XMAP_1" :
		        (offset & 3) == 2 ? "BC_XMAP_2" : "BC_XMAP_3", m_biu_ctrl_reg[offset & 0x3f]);
		fflush(stderr);
	}
#endif
	return m_biu_ctrl_reg[offset & 0x3f];
}

void p2k_state::biu_ctrl_w(offs_t offset, u32 data, u32 mem_mask)
{
	u32 &r = m_biu_ctrl_reg[offset & 0x3f];
	r = (r & ~mem_mask) | (data & mem_mask);
#if P2K_DEBUG
	static const bool biuwatch = getenv("P2K_BIUWATCH") != nullptr;
	if (biuwatch && (offset & 0x3f) < 4)
	{
		fprintf(stderr, "[p2k biu] write %s <- %08x\n",
		        (offset & 3) == 0 ? "BC_DRAM_TOP" : (offset & 3) == 1 ? "BC_XMAP_1" :
		        (offset & 3) == 2 ? "BC_XMAP_2" : "BC_XMAP_3", r);
		fflush(stderr);
	}
#endif
}

// The update flash behaves like an Intel 28F320J5: command writes put it into a mode, and reads
// then return either the array, the CFI query table or the status register. Ported from the
// MAME driver's nvram_updates_r/w
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
		return 0x80; // status: ready
	return (offset < m_nvram_updates.size()) ? m_nvram_updates[offset] : 0xff;
}

void p2k_state::nvram_updates_w(offs_t offset, u16 data)
{
	if (m_flash_mode != 6)
	{
		if (data == 0x0098) { m_flash_mode = 1; return; } // read query
		if (data == 0x0070) { m_flash_mode = 2; return; } // read status register
		if (data == 0x00ff) { m_flash_mode = 0; return; } // read array
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
// bridge, 1078:0002 CX5520 ISA bridge, 146e:0001 the PLX bridge on the Prism card
u32 p2k_state::mediagx_pci_r(int function, int reg, u32 mem_mask) const
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
// games boot unchanged with them right, which is the measurement the old note here asked for and did not have
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
	// The write watch answers who fills a structure; this answers whether a device is ever asked
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
	// alias window used to stop at the register block
	if (addr >= 0xc0000000 && addr < 0xc1000000) addr -= 0x80000000;
	if (addr < 0x000a0000)                       return read_le(m_main_ram, addr, mem_mask);
	if (addr < 0x000b0000)                       return read_le(m_video_ram_a, addr - 0x000a0000, mem_mask);
	if (addr < 0x000c0000)                       return read_le(m_cga_ram, addr - 0x000b0000, mem_mask);
	if (addr < 0x000c8000)                       return expansion_r((addr - 0x000c0000) / 4) & mem_mask;
	if (addr < 0x000d0000)                       return read_le(m_ram_c8, addr - 0x000c8000, mem_mask);
	if (addr < 0x00100000)                       return read_le(m_bios_ram, addr - 0x000d0000, mem_mask);
	// On a MediaGX the frame buffer is carved out of system DRAM, so physical 0x800000-0xbfffff is
	// the same memory as the 0x40800000 window below - Encore keeps one backing store for exactly
	// that reason (qemu/p2k-gx.c: "the FB window is a mirror of physical RAM 0x800000"). Here they
	// are two buffers, m_main_ram and m_vram, and nothing has needed them joined: the guest reaches
	// the frame buffer through 0xc0800000, which is where Allegro's screen bitmap points (its line
	// pointers run down from 0xc08ef800). If something ever draws through the low address instead,
	// this is where the alias goes - and mem_w needs the mirror of it
	if (addr < 0x10000000)                       return read_le(m_main_ram, addr, mem_mask);
	if (addr < 0x10000080)                       return prism_1000_r((addr - 0x10000000) / 4) & mem_mask;
#if P2K_VBLANK_FLAG
	// Encore models a vertical blank flag here (qemu/p2k-vsync.c) because, in its words, several
	// "poll loops in XINU display setup wait for this dword to flip from 0 to 1 each frame before
	// continuing", gating retrace-only work like palette updates and layer flips. This driver has a
	// line counter but no such event, and no display interrupt either.
	//
	// Derived from emulated time rather than written into the array the way Encore writes its SRAM:
	// this region is the CMOS here and is saved to PinMAME's NVRAM file, so a flag stored in it
	// would be written into battery-backed memory every frame and persist across runs. The header
	// seed_error_log() builds skips offset 4, which is consistent with it not being storage.
	// Writing it into the SRAM the way Encore does was tried too - see the commented-out block
	// above push_switches - and changed nothing either. It did not fix the service menu it was
	// added for (README.md), so either the address is wrong or that is not what blocks; kept
	// because the signal is real and the games are unaffected by it
	if (addr == 0x11000004)
	{
		return (in_vblank() ? 1u : 0u) & mem_mask; // 1 while in vertical blank
	}
#endif
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
	if (addr >= 0x40000400 && addr < 0x40001000) return m_scratchpad[(addr - 0x40000400) / 4] & mem_mask;
	if (addr >= 0x40008000 && addr < 0x40008100) return biu_ctrl_r((addr - 0x40008000) / 4) & mem_mask;
	if (addr >= 0x40008100 && addr < 0x40008300) return gx_pipeline_r((addr - 0x40008100) / 4) & mem_mask;
	if (addr >= 0x40008300 && addr < 0x40008400) return disp_ctrl_r((addr - 0x40008300) / 4) & mem_mask;
	if (addr >= 0x40008400 && addr < 0x40008500) return memory_ctrl_r((addr - 0x40008400) / 4) & mem_mask;
	// Nothing answers at 0x40020000, where Encore puts BC_DRAM_TOP and preloads 0x007fffff so the
	// guest BIOS can size RAM (qemu/p2k-gx.c). No set here has been seen to read it so far, and the two
	// versions that want 8 MB - rfm_180 and Episode I 1.60 - boot without it, main RAM being 256 MB
	// regardless. If a machine ever sizes its own memory, that register is what it will ask
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
	// who writes there - and who wipes it - is important
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
		// this the interesting write drowns in millions of identical ones
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
			fprintf(stderr, "[p2k memw] %08x <- %08x mask %08x  from PC=%08x\n", addr, data, mem_mask, p2k_bridge_pc());
			fflush(stderr);
		}
	}
#endif

	// The firmware addresses the whole MediaGX region through its 0xc0000000 alias, not just the
	// control registers: its own framebuffer base pointer is 0xc0800000, and a write map showed
	// 30 M writes at 0xc0800000 and 13 M at 0xc0900000 - the picture, going nowhere, because the
	// alias window used to stop at the register block
	if (addr >= 0xc0000000 && addr < 0xc1000000) addr -= 0x80000000;

#if P2K_DEBUG
	// P2K_KEYWATCH=1: every write carrying 0x7c1f in either half, with the PC, the alias already
	// folded above. This is the half of the watch that located the colour key - see color_key in
	// do_gfx_pipeline for what it found - and it stays armed because a set laying its BLT buffers
	// out differently would show up here first. VRAM is excluded: a picture containing magenta
	// pixels would bury the answer
	{
		static const bool keywatch = getenv("P2K_KEYWATCH") != nullptr;
		const bool in_vram = (addr >= 0x40800000 && addr < 0x40c00000);
		if (keywatch && !in_vram && (((data & 0xffff) == 0x7c1f) || ((data >> 16) == 0x7c1f)))
		{
			static unsigned n = 0;
			if (n++ < 40) // the extent is already known and recorded; this is just the entry point
			{
				extern unsigned p2k_bridge_pc();
				fprintf(stderr, "[p2k key] %08x <- %08x mask %08x  from PC=%08x\n", addr, data, mem_mask, p2k_bridge_pc());
				fflush(stderr);
			}
		}
	}
#endif

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
		// the DCS2 sound board, upstream's, wired in src/wpc/p2k.c. Weak, so the standalone harness links without one
		if (P2K_HAVE_WEAK(p2k_dcs_write)) p2k_dcs_write(addr - 0x13000000, data, mem_mask);
		return;
	}
	if (addr >= 0x14000000 && addr < 0x15000000) { prism_1400_w((addr - 0x14000000) / 4, data); return; }
	if (addr >= 0x15000000 && addr < 0x18000000) { return; } // prism data banks are read-only
	if (addr >= 0x18000000 && addr < 0x19000000) { write_le(m_prism_bank9, addr - 0x18000000, data, mem_mask); return; }
	if (addr >= 0x40000400 && addr < 0x40001000)
	{
		u32 &r = m_scratchpad[(addr - 0x40000400) / 4];
		r = (r & ~mem_mask) | (data & mem_mask);
#if P2K_DEBUG
		m_scratchpad_gen++; // lets the blit's BLT buffer check re-scan only when it has to
#endif
		return;
	}
	if (addr >= 0x40008000 && addr < 0x40008100) { biu_ctrl_w((addr - 0x40008000) / 4, data, mem_mask); return; }
	if (addr >= 0x40008100 && addr < 0x40008300) { gx_pipeline_w((addr - 0x40008100) / 4, data, mem_mask); return; }
	if (addr >= 0x40008300 && addr < 0x40008400) { disp_ctrl_w((addr - 0x40008300) / 4, data, mem_mask); return; }
	if (addr >= 0x40008400 && addr < 0x40008500) { memory_ctrl_w((addr - 0x40008400) / 4, data, mem_mask); return; }
	if (addr >= 0x40400000 && addr < 0x40480000) { write_le(m_smm, addr - 0x40400000, data, mem_mask); return; }
	if (addr >= 0x40800000 && addr < 0x40c00000) { write_le(m_vram, addr - 0x40800000, data, mem_mask); return; }
	if (addr >= 0xc0800000 && addr < 0xc0c00000) { write_le(m_vram, addr - 0xc0800000, data, mem_mask); return; } // the alias; see mem_r
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
	if (offset == 1) return 0xff; // status port
	if (offset != 0) return 0x00; // control port

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
		case 0x00: return sw_column(0);
		case 0x01: return sw_column(11);
		// The power driver board's DIP switches, read once during startup: they select the country,
		// which is what the pricing tables key off (the changelogs talk about "the country dipswitch
		// setting"). Answers with whatever the user set, through core_getDip(0) and p2k_pinmame_set_dips(), and 1 is still
		// the default so nothing changes unless someone moves a switch. The machine's own DIP Switch
		// Test in the service menu shows what it sees, which is how a value is checked.
		//
		// Only bits 0-3 matter, as a country code: 0 USA/Canada, 1 Germany, 2 France, 3 United
		// Kingdom, 4 Spain, 7 Europe, 8 Japan, and the machine calls 5, 6 and 9-15 Unused. Measured
		// by walking every combination against its own DIP Switch Test. The default is now 0, USA/Canada.
		//
		// Encore hardcodes this one to 0xf0 and calls it a "status hi nibble"
		// (qemu/p2k-lpt-board.c). Both constants boot, which fits a country selector where any
		// value picks some country - but the manual has a DIP Switch Test and both changelogs talk
		// about the country dipswitch setting, so switches is what this is. Encore agrees with the
		// 0x00 for the fuses below, which the service menu fuse test confirms is the healthy reading
		case 0x02: return m_dip_switches;
		case 0x03: return sw_column(10);
		case 0x04:
		{
			// The switch row for whichever column is being strobed. Measured: the column
			// register is one-hot and active HIGH -- the firmware writes 0x01, 0x02, 0x04,
			// 0x08, 0x10 ... and 0x80, exactly like the lamp column strobe below. (It was
			// read as active low here before, which shifted every column by one)
			for (unsigned c = 0; c < 8; c++)
				if (m_switch_column & (1u << c))
					return sw_column(c + 1); // PinMAME numbers columns from 1
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
		// against it. Modelling zero cross properly only becomes necessary if something starts reading 0x0f repeatedly
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
		// 0x12/0x13 are the fuse diagnostics, read as a pair by wms_pdb_fuse_status(unsigned char &,
		// unsigned char &) - the names are in the packages' symbols.rom. They are read, contrary to
		// what an earlier watch here concluded: that watch simply never entered the service menu's
		// fuse test, which is the only thing that asks for them. Walking into it on rfm_160 gives
		// "r 12 -> 00" and "r 13 -> 00" and draws every fuse green, so 0 is not a placeholder that
		// happens to be ignored - it is the healthy reading, one bit per fuse with blown being set.
		// A machine that should show a blown fuse is the only thing this cannot express (yet)
		case 0x12: case 0x13: return 0x00;
		default:   return 0xff;
	}
}

void p2k_state::lpt_w(offs_t offset, u8 data)
{
	if (offset != 0) return;

	if (m_pdb_phase_1 == 0)                            { m_pdb_phase_1 = 1; m_pdb_phase_2 = 0; }
	else if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 0) { m_pdb_phase_2 = 1; }
	else if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 1) { m_pdb_phase_1 = 1; m_pdb_phase_2 = 0; }

	if (m_pdb_phase_1 == 1 && m_pdb_phase_2 == 0) m_pdb_index = data; // index register
	if (m_pdb_phase_2 != 1) return;

#if P2K_DEBUG
	// Writes are logged unconditionally, not change-only: a lamp test that drives the same row
	// twice in a row is exactly the case worth seeing, and pairing each write with the read that
	// follows it is the whole point of watching this side
	if (p2k_pdbwatch(m_pdb_index)) { printf("[p2k pdb] w %02x <- %02x\n", m_pdb_index, data); fflush(stdout); }
#endif

	// a write to the selected I/O register
	const u32 solWas = m_solenoids, sol2Was = m_solenoids2;
	switch (m_pdb_index)
	{
		case 0x05: m_switch_column = data; break; // switch column strobe, one-hot
		case 0x06: m_lamp_row_a = data; break;    // lamp rows, latched until the
		case 0x07: m_lamp_row_b = data; break;    //   column strobe below
		case 0x08:
			// Lamp column strobe. The board drives one column at a time and has two row banks
			// of eight (index 6 and 7), so a column carries sixteen lamps; PinMAME's matrix is
			// eight bits per column, so each driven column becomes two of them - bank A at 2c,
			// bank B at 2c+1. Eight columns therefore occupy sixteen of PinMAME's.
			//
			// ORed rather than stored: the board blanks the strobe to 0x00 between columns, and a
			// column that is being dimmed is simply strobed less often, so the window is what says whether a bulb was lit
			for (unsigned c = 0; c < 8; c++)
				if (data & (1u << c))
				{
					m_lamp_acc[c * 2 + 0] |= m_lamp_row_a;
					m_lamp_acc[c * 2 + 1] |= m_lamp_row_b;
				}
			// Unconditionally, blanking writes included: see set_lamp_notify()
			if (m_lamp_notify) m_lamp_notify(data, m_lamp_row_a, m_lamp_row_b);
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

	// The live edge, for the PWM integrator: only on a real change, because that is what an edge is
	// and because the integrator keeps a bounded history of them
	if (m_sol_notify && (m_solenoids != solWas || m_solenoids2 != sol2Was))
		m_sol_notify(m_solenoids, m_solenoids2);

	// Every driver level the registers ever hold, ORed up for pull_outputs(). Here rather than in
	// the six cases above, and without asking which register was written: a solenoid register only
	// changes in one of those cases, so sampling after any board write catches each level at the
	// moment it is established, and re-ORing an unchanged one costs nothing
	m_sol_acc |= m_solenoids;
	m_sol2_acc |= m_solenoids2;
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
// defaults off, and off it costs nothing at all - without P2K_DEBUG the frame tracking and the
// per-instruction hook are not compiled in, and shim/debugger.h leaves the i386 execute loop with
// no call site at all. So against the old arrangement this trades a per-instruction tax for a
// per-PIC-write one, and which is cheaper is genuinely open.
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
	// unmapped (reads return 0xff) lets it continue. This used to say the missing piece was SMM.
	// It is not: the SMM region at 0x40400000 is never read or written by any set, and the only
	// SMI sources the databook gives the bus interface unit are the VGA I/O traps in BC_XMAP_1
	// bits 13-15, which no set sets (4.2.3, and P2K_BIUWATCH shows the XMAP registers untouched).
	// What is behind these registers is GX_BASE itself, among the rest of the configuration - so
	// answering them wrongly can move the whole register aperture out from under the driver
	if (port >= 0x002e && port <= 0x002f)
		return (port == 0x002f) ? m_superio_regs[m_superio_reg_sel] : 0;
	if (port >= 0x00e8 && port <= 0x00eb) return 0xff; // I/O delay port
	if ((port >= 0x0170 && port <= 0x0177) || (port >= 0x01f0 && port <= 0x01f7) ||
		(port >= 0x0370 && port <= 0x0377) || (port >= 0x03f0 && port <= 0x03f7))
		return 0xff;                                // IDE: no drives attached

	// parallel port: the pinball driver board sits at 0x3bc, the printer port itself at 0x378.
	// Both are probed by writing the data port and reading it back - an unmapped 0xff there is
	// what made the firmware report "PinIO failed: no printer port present"
	if (port >= 0x03bc && port <= 0x03bf) return lpt_r(port - 0x03bc);
	if (port >= 0x0378 && port <= 0x037b)
	{
		switch (port - 0x0378)
		{
			case 0:  return m_lpt_data;
			case 1:  return 0x00;                   // status: no printer attached
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
			case 0: case 1:                         // divisor latch or RBR/IER, by DLAB - see port_w
				if (m_uart_reg[3] & 0x80) return m_uart_dl[port & 1];
				return m_uart_reg[port & 7];
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
			case 0: case 1:
				if (m_uart2_reg[3] & 0x80) return m_uart2_dl[port & 1];
				return m_uart2_reg[port & 7];
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
	if (port <= 0x001f) { m_dma1->write(port, data); return; }
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
	if (port >= 0x0020 && port <= 0x0021)
	{
#if P2K_DEBUG
		// P2K_IRQWATCH=1: the interrupt mask as the guest sets it. An IRQ the guest unmasks and
		// this driver never asserts is a device it is waiting on and will wait on for ever
		static const bool irqwatch = getenv("P2K_IRQWATCH") != nullptr;
		static u8 last1 = 0xff;
		if (irqwatch && port == 0x0021 && u8(data) != last1)
		{
			last1 = u8(data);
			fprintf(stderr, "[p2k irq] PIC1 mask <- %02x  (unmasked:", last1);
			for (int i = 0; i < 8; i++) if (!(last1 & (1 << i))) fprintf(stderr, " %d", i);
			fprintf(stderr, ")\n");
			fflush(stderr);
		}
#endif
		m_pic1->write(port & 1, data); pics_settle(); return;
	}
	if (port >= 0x0040 && port <= 0x0043) { m_pit->write(port & 3, data); return; }
	if (port >= 0x0060 && port <= 0x006f) { m_kbdc->data_w(port & 7, data); return; }
	if (port >= 0x0070 && port <= 0x0071) { m_rtc->write(port & 1, data); return; }
	if (port >= 0x00a0 && port <= 0x00a1)
	{
#if P2K_DEBUG
		static const bool irqwatch = getenv("P2K_IRQWATCH") != nullptr;
		static u8 last2 = 0xff;
		if (irqwatch && port == 0x00a1 && u8(data) != last2)
		{
			last2 = u8(data);
			fprintf(stderr, "[p2k irq] PIC2 mask <- %02x  (unmasked:", last2);
			for (int i = 0; i < 8; i++) if (!(last2 & (1 << i))) fprintf(stderr, " %d", 8 + i);
			fprintf(stderr, ")\n");
			fflush(stderr);
		}
#endif
		m_pic2->write(port & 1, data); pics_settle(); return;
	}
	if (port >= 0x00c0 && port <= 0x00df) { m_dma2->write((port - 0x00c0) / 2, data); return; }

	if (port >= 0x00e8 && port <= 0x00eb) return;
	if ((port >= 0x0170 && port <= 0x0177) || (port >= 0x01f0 && port <= 0x01f7) ||
		(port >= 0x0370 && port <= 0x0377) || (port >= 0x03f0 && port <= 0x03f7)) return;
	if (port >= 0x02f8 && port <= 0x02ff) // COM2, with the same DLAB split as COM1 below
	{
		const bool dlab = (m_uart2_reg[3] & 0x80) != 0;
		if (dlab && (port & 7) < 2) m_uart2_dl[port & 1] = data;
		else
		{
			m_uart2_reg[port & 7] = data;
			if ((port & 7) == 1) update_uart_irq();
		}
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
		// Ports 0 and 1 are two registers each: with DLAB - bit 7 of the line control register -
		// set they are the baud rate divisor latch, and only with it clear are they the transmit
		// register and the interrupt enable.
		//
		// Honouring that for port 0 but not for port 1 is what wedged swep1_210. XINA calls
		// tty_set_port_param(), which sets DLAB, writes divisor 0x000c for 9600 baud, and clears
		// DLAB again - and the divisor's high byte, zero, landed on the interrupt enable shadow.
		// That switched the transmit interrupt off for good about 3.8 seconds in, so the console
		// queue stopped draining; it filled after some 35 seconds of ordinary messages, ttyputc
		// blocked on the semaphore that guards it, and the 0.25 ms watchdog reported the process
		// behind it as hung. Anything that printed more - a coin, the service menu - got there
		// sooner, which is why those looked like the trigger for a long time
		const bool dlab = (m_uart_reg[3] & 0x80) != 0;
		switch (port & 7)
		{
			case 0:
				if (dlab) m_uart_dl[0] = data;
				else
				{
					m_console.push_back(char(data));
					if (m_trace) { fputc(int(data), stdout); fflush(stdout); }
				}
				break;
			case 1:
				if (dlab) m_uart_dl[1] = data;
				else { m_uart_reg[1] = data; update_uart_irq(); }
				break;
			default:
				m_uart_reg[port & 7] = data;
				break;
		}
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
