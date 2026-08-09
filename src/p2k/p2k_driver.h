// license:BSD-3-Clause

// PinMAME P2K subsystem - the Pinball 2000 machine.
//
// Ported from MAME's src/mame/drivers/pinball2k.cpp (skeleton by R. Belmont based on Ville
// Linde's mediagx.c, extended by E. van Son; BSD-3-Clause). The handler logic follows that
// driver; what differs is the plumbing: MAME's address_map is replaced by explicit decode
// functions, and the devices are assembled through p2k_machine instead of a machine_config

#pragma once

#include "emu.h"
#include "p2k_machine.h"
#include "i386.h"
#include "machine/lpci.h"

class pic8259_device;
class pit8253_device;
class am9517a_device;
class mc146818_device;
class kbdc8042_device;
class ns16550_device;

class p2k_state final
{
public:
	p2k_state();
	~p2k_state();

	// Both take PinMAME ROM regions, declared by ROM_START in src/wpc/p2k.c. The subsystem
	// does not open files: everything arrives through the normal ROM set machinery
	bool set_prism_roms(const u8 *data, size_t len, const char *prefix);
	// the update flash image (bootdata + im_flsh0 + game + symbols), 8 MB
	bool set_nvram_updates(const u8 *data, size_t len);

	// The three things worth keeping across runs, in MAME's terms: "nvram" is the CMOS the game
	// stores its settings, audits and error log in, "nvram2" the PLX EEPROM, and "nvram_updates"
	// the 8 MB update flash. Handed out as raw blobs so src/wpc/p2k.c can put them through
	// PinMAME's own NVRAM handler without knowing anything about the machine.
	enum nvram_block { NVRAM_CMOS = 0, NVRAM_EEPROM, NVRAM_UPDATES };
	u8 *nvram_block_ptr(nvram_block which, size_t *size);

	void build_machine(u32 cpu_clock);
	void reset();
	void run_cycles(u64 cycles);
	void apply_irq0();
	void maybe_write_ppm(u64 cycles);   // P2K_PPM=<path>: the framebuffer as a picture
	// decode the framebuffer into 0x00RRGGBB pixels; false if the geometry is not sane yet; fast_15bpp_path returns the raw 15bpp pixels instead (if applicable, otherwise fast_15bpp_path_success is false!)
	bool frame_rgb(u32* const __restrict dest, unsigned capacity, unsigned &width, unsigned &height, const bool fast_15bpp_path, bool& fast_15bpp_path_success) const; // re-evaluate the CPU's IRQ0 line (PIC request AND clkint gate)

	// bus access, called from the CPU's address spaces. The pure-memory ranges are also handed
	// to the space directly, so most accesses never reach mem_r/mem_w at all.
	void install_fast_windows();

	// A direct pointer to main RAM, or null when the address is not plain RAM. Same mapping as
	// mem_r's two RAM ranges; for callers that run per instruction and cannot afford the decode.
	// The low range stops one byte short so a two-byte peek stays inside it.
	const u8 *ram_peek(u32 addr) const
	{
		if (addr < 0x0009ffff)                                     return &m_main_ram[addr];
		if (addr >= 0x00100000 && addr + 1 < m_main_ram.size())    return &m_main_ram[addr];
		return nullptr;
	}
	u32 mem_r(offs_t addr, u32 mem_mask);
	void mem_w(offs_t addr, u32 data, u32 mem_mask);
	u32 io_r(offs_t addr, u32 mem_mask);
	void io_w(offs_t addr, u32 data, u32 mem_mask);
	u8 port_r(offs_t port);
	u8 port_read(offs_t port);   // the decode itself; port_r wraps it for the I/O watch
	u8 lpt_r(offs_t offset);
	void lpt_w(offs_t offset, u8 data);
	// the pinball I/O, seen from PinMAME's side
	void push_switches(const u8 *matrix, unsigned count);
	void pull_outputs(u8 *lamps, unsigned lamp_columns, u32 *solenoids, u32 *solenoids2) const;
	void port_w(offs_t port, u8 data);

	// The boot code logs over COM1. A minimal 16550-compatible console stands in for the real
	// UART during bring-up: enough to accept characters and report the transmitter as idle.
	const std::string &console_log() const { return m_console; }

	// diagnostics
	u64 unmapped_reads() const { return m_unmapped_r; }
	u64 unmapped_writes() const { return m_unmapped_w; }
	u32 cpu_pc();
	void set_trace(bool on) { m_trace = on; }

	p2k_machine &machine() { return *m_machine; }

private:
	// memory regions, sized as in the MAME driver's address map
	std::vector<u8> m_main_ram;        // 0x00000000-0x0009ffff and 0x00100000-0x0fffffff
	std::vector<u8> m_video_ram_a;     // 0x000a0000-0x000affff
	std::vector<u8> m_cga_ram;         // 0x000b0000-0x000bffff
	std::vector<u8> m_ram_c8;          // 0x000c8000-0x000cffff
	std::vector<u8> m_bios_ram;        // 0x000d0000-0x000fffff
	std::vector<u8> m_nvram;           // 0x11000000-0x1102ffff
	std::vector<u8> m_nvram_updates;   // 0x12000000 flash image, 8 MB
	std::vector<u8> m_prism_bank9;     // 0x18000000-0x18ffffff
	std::vector<u8> m_smm;             // 0x40400000-0x4047ffff
	std::vector<u8> m_vram;            // 0x40800000-0x40bfffff
	std::vector<u32> m_system_bios1;   // 0xfffd0000-0xffffffff
	std::vector<u32> m_eeprom;         // PLX EEPROM behind 0x10000000

	// ROM data (32-bit words, as interleaved by the driver's ROM_LOAD32_WORD pairs)
	std::vector<u32> m_prismdata[4];

	// MediaGX north bridge state
	u32 m_disp_ctrl_reg[256/4] = {};
	u32 m_memory_ctrl_reg[256/4] = {};
	u32 m_biu_ctrl_reg[256/4] = {};
	u32 m_gx_pipeline_reg[512/4] = {};
	u32 m_scratchpad[0x600/4] = {};
	int m_prismbank = 0;

	// PLX local bus registers at 0x10000000, and the serial EEPROM behind register 0x14
	u32 m_eeprom_regs[256/4] = {};
	int m_prism_eprom_clk = 0;
	int m_prism_clock_enabled = 0;
	int m_prism_eprom_counter = 0;
	int m_prism_eprom_offset = 0;
	int m_prism_eprom_wordtoggle = 0;

	// Intel-style flash command state for the update flash at 0x12000000
	int m_flash_mode = 0;
	int m_buffer_counter = 0;

	// Pinball driver board on the parallel port at 0x3bc, and the printer port at 0x378
	u8 m_pdb_index = 0;          // the board's index register, clocked in from the data port
	int m_pdb_phase_1 = 0;       // MAME's m_pdb_1/m_pdb_2: index written / I/O register selected
	int m_pdb_phase_2 = 0;
	u8 m_switch_column = 0;      // last value written to index register 5 (switch column strobe)
	u8 m_coin_switches = 0;      // inputs, still unwired: PinMAME's core model comes with M3.5
	u8 m_cabinet_switches = 0;
	u8 m_diag_switches = 0;
	u8 m_start_button = 0;
	u8 m_lpt_data = 0;           // printer port data latch at 0x378
	u8 m_lpt_control = 0;
	u8 m_sw_matrix[16] = {};     // switch state pushed in from PinMAME's core model
	u8 m_lamp_row_a = 0;         // driver board index 6/7/8: two lamp rows and the column strobe
	u8 m_lamp_row_b = 0;
	u8 m_lamp_col = 0;
	u8 m_lamp_matrix[16] = {};   // two row banks per driven column, the shape PinMAME wants
	u32 m_solenoids = 0;         // registers 09/0a/0b/0c, eight bits each
	u32 m_solenoids2 = 0;        // register 0d - forty outputs do not fit one word

	// PC97317 Super I/O configuration registers, reached through ports 0x2e/0x2f
	u8 m_superio_regs[256] = {};
	u8 m_superio_reg_sel = 0;

	// Cyrix configuration registers, reached through ports 0x22/0x23
	u8 m_mediagx_config_regs[256] = {};
	u8 m_mediagx_config_reg_sel = 0;

	// PCI configuration space of the three devices on the bus
	u32 m_mediagx_regs[65] = {};
	u32 m_cx5520_regs[256/4] = {};
	u32 m_prism_regs[256/4] = {};

	std::unique_ptr<p2k_machine> m_machine;
	mediagx_device *m_maincpu = nullptr;
	pic8259_device *m_pic1 = nullptr;
	pic8259_device *m_pic2 = nullptr;
	pit8253_device *m_pit = nullptr;
	am9517a_device *m_dma1 = nullptr;
	am9517a_device *m_dma2 = nullptr;
	mc146818_device *m_rtc = nullptr;
	kbdc8042_device *m_kbdc = nullptr;
	pci_bus_legacy_device *m_pcibus = nullptr;

	std::unique_ptr<address_space> m_program;
	std::unique_ptr<address_space> m_io;

	u32 m_cpu_clock = 20000000;
	u64 m_unmapped_r = 0;
	u64 m_unmapped_w = 0;
	bool m_trace = false;
	std::string m_console;
	u8 m_uart_reg[8] = {};        // COM1 stand-in
	u8 m_uart2_reg[8] = {};       // COM2 stand-in
	void update_uart_irq();       // 16550 interrupt logic for both ports

	// ported handlers
	u32 expansion_r(offs_t offset) const;
	u32 prism_1400_r(offs_t offset) const;
	void prism_1400_w(offs_t offset, u32 data);
	u32 prism_1000_r(offs_t offset);
	void prism_1000_w(offs_t offset, u32 data);
	u32 gx_pipeline_r(offs_t offset) const;
	void gx_pipeline_w(offs_t offset, u32 data, u32 mem_mask);
	void do_gfx_pipeline();
	u32 disp_ctrl_r(offs_t offset) const;
	void disp_ctrl_w(offs_t offset, u32 data, u32 mem_mask);
	u32 memory_ctrl_r(offs_t offset) const;
	void memory_ctrl_w(offs_t offset, u32 data, u32 mem_mask);
	u32 mediagx_pci_r(int function, int reg, u32 mem_mask);
	void mediagx_pci_w(int function, int reg, u32 data, u32 mem_mask);
	u32 cx5520_pci_r(int function, int reg, u32 mem_mask);
	void cx5520_pci_w(int function, int reg, u32 data, u32 mem_mask);
	u32 prism_pci_r(int function, int reg, u32 mem_mask);
	void prism_pci_w(int function, int reg, u32 data, u32 mem_mask);

	u8 nvram_updates_r(offs_t offset) const;
	void nvram_updates_w(offs_t offset, u16 data);

	u32 biu_ctrl_r(offs_t offset) const;
	void biu_ctrl_w(offs_t offset, u32 data, u32 mem_mask);

	static u32 read_le(const std::vector<u8> &buf, offs_t off, u32 mask);
	static void write_le(std::vector<u8> &buf, offs_t off, u32 data, u32 mask);
};
