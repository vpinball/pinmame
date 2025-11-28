#include "driver.h"
#include "vidhrdw/crtc6845.h" //!! use MAME mc6845? -> much more complex
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "sndbrd.h"
#include "dedmd.h"
#ifdef PROC_SUPPORT
#include "p-roc/p-roc.h"
#endif

#ifdef VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
  core_tDMDPWMState pwm_state;
  
  // dmd32 stuff
  UINT8* RAM;
  UINT8* RAMbankPtr;
  
  // dmd16 stuff
  UINT8 framedata[2][16][2][8]; // 2 PWM frames of 16 rows of 128 bits
  UINT32 row_latch, last_row_latch;
  UINT64 dot_latch, dot_shift;
  int blnk, rowdata, rowclk, test;// , laststat;
  mame_timer* nmi_timer;
} dmdlocals;

static UINT16 *dmd64RAM;

static WRITE_HANDLER(dmd_data_w)  { dmdlocals.ncmd = data; }
static READ_HANDLER(dmd_status_r) { return dmdlocals.status; }
static READ_HANDLER(dmd_busy_r)   { return dmdlocals.busy; }

/*------------------------------------------*/
/*Data East, Sega, Stern 128x32 DMD Handling*/
/*------------------------------------------*/
#define DMD32_BANK0    2

static WRITE_HANDLER(dmd32_ctrl_w);
static void dmd32_init(struct sndbrdData *brdData);
static void dmd32_exit(int boardNo);

const struct sndbrdIntf dedmd32Intf = {
  NULL, dmd32_init, dmd32_exit, NULL,NULL,
  dmd_data_w, dmd_busy_r, dmd32_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static READ_HANDLER(dmd32_io_r);
static WRITE_HANDLER(dmd32_io_w);
static WRITE_HANDLER(dmd32_status_w) { sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data & 0x0f); }
static READ_HANDLER(dmd32_RAMbank_r) { return dmdlocals.RAMbankPtr[offset]; }
static WRITE_HANDLER(dmd32_RAMbank_w) { dmdlocals.RAMbankPtr[offset] = data; }

static MEMORY_READ_START(dmd32_readmem)
  { 0x0000, 0x1fff, MRA_RAM },                 /* Static RAM (U16 uses CORE/ZA0 signal to map last 8k of RAM) */
  { 0x2000, 0x2fff, dmd32_RAMbank_r },         /* Banked RAM (8 x 4k) */
  { 0x3000, 0x3fff, dmd32_io_r },              /* CRTC6845 and IO latch */
  { 0x4000, 0x7fff, MRA_BANKNO(DMD32_BANK0) }, /* Banked ROM (32 x 16k) */
  { 0x8000, 0xffff, MRA_ROM },                 /* Static ROM (U16 uses CORE/ZA0 signal to map last 16k of ROM) */
MEMORY_END

static MEMORY_WRITE_START(dmd32_writemem)
  { 0x0000, 0x1fff, MWA_RAM },                 /* Static RAM (U16 uses CORE/ZA0 signal to map last 8k of RAM) */
  { 0x2000, 0x2fff, dmd32_RAMbank_w },         /* Banked RAM (8 x 4k) */
  { 0x3000, 0x3fff, dmd32_io_w },              /* CRTC6845 and ROM/RAM bank switching */
  { 0x4000, 0x7fff, dmd32_status_w },          /* Status */
  { 0x8000, 0xffff, MWA_NOP },
MEMORY_END

MACHINE_DRIVER_START(de_dmd32) // Board Part 520-5055-00
  MDRV_CPU_ADD(M6809, 2000000) // 8000000/4
  MDRV_CPU_MEMORY(dmd32_readmem, dmd32_writemem)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se_dmd32) // Board Part 520-5055-01 to 520-5055-03
  MDRV_CPU_ADD(M6809, 2000000) // 8000000/4
  MDRV_CPU_MEMORY(dmd32_readmem, dmd32_writemem)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

static READ_HANDLER(dmd32_io_r) {
  switch (offset & 0x0003) {
  // /CRTCCS: CRTC6845 chip access
  case 0:
    // Invalid operation: reading from address register is unsupported (see datasheet)
    // Strangely enough, all CRTC write sequences are followed by a read of CRTC register which the datasheet state as unsupported
	 //LOG(("%8.5f DEDMD32 PC %04x: Invalid read at 0x%04x\n", timer_get_time(), activecpu_get_pc(), offset + 0x3000));
    break;
  case 1:
    return crtc6845_register_0_r(0);
  // /PORTIN: Read CPU data latch, and clear IRQ/busy line
  case 2:
  case 3:
    sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 0);
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
    return dmdlocals.cmd;
  }
  return 0;
}

static WRITE_HANDLER(dmd32_io_w) {
  switch (offset & 0x0003) {
  // /CRTCCS: CRTC6845 chip access
  case 0: crtc6845_address_0_w(0, data); break;
  case 1: crtc6845_register_0_w(0, data); break;
  // /BSE: latch XA0..7, used to define ROM and RAM banks (RAM bank seems to be unused)
  case 2:
    // if (data & 0xe0) { LOG(("%8.5f DEDMD32 PC %04x: Switch RAM Bank 0x%02x\n", timer_get_time(), activecpu_get_pc(), data >> 5)); }
    dmdlocals.RAMbankPtr = dmdlocals.RAM + (data >> 5) * 0x1000;
    cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion + (data & 0x1f) * 0x4000);
    break;
  }
}

static WRITE_HANDLER(dmd32_ctrl_w) {
  if (~dmdlocals.ctrl & data & 0x01) { // Cmd ready
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, HOLD_LINE);
    sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 1);
    dmdlocals.cmd = dmdlocals.ncmd;
  }
  if (dmdlocals.ctrl & ~data & 0x02) { // Reset
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
    dmd32_io_w(0x0003, 0); // set ROM and RAM bank to 0
  }
  dmdlocals.ctrl = data;
}

/*
 Data East and Sega/Stern Whitestar share the (mostly) same DMD board: 520-5055-00 for Data East, 520-5055-01 to 520-5055-03 for Whitestar
 [Disclaimer, large parts of the following have been deduced from digital recordings and schematics analysis, so may be entirely wrong]

 The board is built around a CRTC6845 [U9] which clock is driven by a custom PAL chip [U2]. This design allows to create 0/33/66/100 shades: 
 - Each row is rasterized twice by the CRTC6845, first with a 500KHz clock, then with a 1MHz clock. To do so, the PAL chip toggles the 
   clock divider of the CRTC6845. RA0 is likely used to toggle the clock divider inside U2.
 - Data is fed from RAM at offset X for first frame (double PWM length) then at X+0x200 for second frame (single PWM length). To do so,
   a memory mapping is applied to the linear rasterization performed by the CRTC6845, MA is the memory address output, RA is the character
   output. Character is used as the PWM frame selector and wired to bit 9 of the memory address allowing to render a line of each frame
   alternatively:
    CA0..2   - Bit shift (0..7)  [always starts at 0, unimplemented in PinMame]
    CA3..6   - MA0..3            [always starts at 0, unimplemented in PinMame]
    RA1..4   - MA4..7   => DMD row lowest 4 bits from character row address (always starts at 0)
    CA7      - MA8      => DMD row highest bit [always starts at 0, unimplemented in PinMame]
    RA0      - MA9      => used to toggle between PWM frame while rasterizing each row
    CA8..9   - MA10..11
	ZA0      - MA12     => RAM bank (decoded by U16)
	XA6..7   - MA13..14 => RAM bank (decoded by U16)
 - RA0 is also wired to ROWCLOCK, therefore row is advanced once every 2 rasterized rows
 - The start address is usually either 0x2000 or 0x2100 (i.e. with CA13 set while it is not wire to RAM but to U2), with CURSOR always
   being defined to 0x2000 / row 0, which led to guess that CURSOR signal is used to generate FIRQ (frame IRQ to CPU, trigerring
   some animation update). Results looks good but this would be nice to check this assumption on real hardware.
   */
static void dmd32_vblank(int which) {
  //static double prev; printf("DMD VBlank %8.5fms => %8.5fHz Base address: %04x\n", timer_get_time() - prev, 1. / (timer_get_time() - prev), crtc6845_start_address_r(0)); prev = timer_get_time();
  // Store 2 next rasterized frame, as the CRTC is setup to render 2 contiguous full frames for each VBLANK
  unsigned int base = crtc6845_start_address_r(0); // MA0..13
  assert((base & 0x00FF) == 0x0000); // As the mapping of lowest 8 bits is not implemented (would need complex data copy and does not seem to be used by any game)
  assert(crtc6845_rasterized_height_r(0) == 64); // As the implementation requires this to be always true
  unsigned int src = /*((base >> 3) & 0x000F) | ((base << 1) & 0x0100) |*/ ((base << 2) & 0x7C00);
  core_dmd_submit_frame(&dmdlocals.pwm_state, dmdlocals.RAMbankPtr +  src,           2); // First frame has been displayed 2/3 of the time (500kHz row clock)
  core_dmd_submit_frame(&dmdlocals.pwm_state, dmdlocals.RAMbankPtr + (src | 0x0200), 1); // Second frame has been displayed 1/3 of the time (1MHz row clock)
  if (crtc6845_cursor_address_r(0)) // Guessing that the CURSOR signal is used to generate FIRQ
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_FIRQ_LINE, PULSE_LINE);
}

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  /* copy last 16K of ROM into last 16K of CPU region*/
  memcpy(memory_region(DE_DMD32CPUREGION) + 0x8000, memory_region(DE_DMD32ROMREGION) + memory_region_length(DE_DMD32ROMREGION)-0x8000,0x8000);
  dmdlocals.RAM = malloc(8 * 4096); // TODO move to default MAME memory management
  dmd32_io_w(0x0003, 0); // set ROM and RAM bank to 0

  // Init 6845
  crtc6845_init(0);
  //crtc6845_set_vsync(0, /* 8000000 from 6809 E clock output */, dmd32_vblank); // we can not use the default implementation as the clock divider is toggled after each HSYNC
  // The theory of operation given above would lead to a refresh rate of 80.1Hz (half of the frame with a CRTC6845 clocked at 1MHz, the other half at 500KHz)
  // Measures on the real hardware show that a few additional cycles are consumed while switching clock divider, this behavior having changed between initial revision 
  // (Data East, 25620 cycles per frame) and later ones (Sega/Stern Whitestar, 25720 cycles per frame).
  if ((core_gameData->gen & GEN_DEDMD32) != 0) // Board 520-5055-00
    // Measured at 78.07Hz for the 3 PWM frames (234.21Hz fps)
    timer_pulse(25620. / 2000000., 0, dmd32_vblank);
  else if ((core_gameData->gen & GEN_ALLWS) != 0) // Board 520-5055-01 to 520-5055-03
    // Measured at 77.77Hz for the 3 PWM frames (233.31Hz fps)
    timer_pulse(25720. / 2000000., 0, dmd32_vblank);
  else
    assert(0); // Unsupported board revision

  // Init PWM shading
  core_dmd_pwm_init(&dmdlocals.pwm_state, 128, 32, CORE_DMD_PWM_FILTER_DE_128x32, CORE_DMD_PWM_COMBINER_SUM_2_1);
}

static void dmd32_exit(int boardNo) {
   free(dmdlocals.RAM);
   core_dmd_pwm_exit(&dmdlocals.pwm_state);
}

PINMAME_VIDEO_UPDATE(dedmd32_update) {
  #ifdef PROC_SUPPORT
	if (coreGlobals.p_rocEn) {
    /* Whitestar games drive 4 colors using 2 subframes, which the P-ROC
	    has 4 subframes for up to 16 colors. Experimentation has showed
		 using P-ROC subframe 2 and 3 provides a pretty good color match. */
	  const int procSubFrame0 = 2;
	  const int procSubFrame1 = 3;

	  /* Start with an empty frame buffer */
	  procClearDMD();

	  /* Fill the P-ROC subframes from the video RAM */
     const UINT8* RAM = ((UINT8*)dmdlocals.RAMbankPtr) + ((crtc6845_start_address_r(0) & 0x0100) << 2);
     procFillDMDSubFrame(procSubFrame0, RAM        , 0x200);
	  procFillDMDSubFrame(procSubFrame1, RAM + 0x200, 0x200);

	  /* Each byte is reversed in the video RAM relative to the bit order the P-ROC
	     expects. So reverse each byte. */
	  procReverseSubFrameBytes(procSubFrame0);
	  procReverseSubFrameBytes(procSubFrame1);
	  procUpdateDMD();
	  /* Don't explicitly update the DMD from here. The P-ROC code
	     will update after the next DMD event. */
	}
  #endif
  core_dmd_video_update(bitmap, cliprect, layout, &dmdlocals.pwm_state);
  return 0;
}

/*-----------------------------
 Sega DMD controller Board 237-0139-00 driving a 192x64 DMD part 520-5075-00
 [Disclaimer, large parts of the following have been only deduced from schematics analysis, so may be entirely wrong]

 The board is built around a 68000 which prepares frames in a shared RAM memory for a 6845 rasterizer. The rasterizer is driven by QD,
 a 3MHz/16 clock, with a setup that leads to 2509 cycles per frame (with each row rasterized 3 times), that is to say 74.73Hz, which has
 been validated on real hardware. This leads to 224,2Hz PWM patterns of 3 frames with 2 of them identical, allowing for 0/33/66/100 shades.
 Additional hardware insights:
 - Clocks: everything is tied to CPU's 12MHz with QC is 3MHz/8, QD is 3MHz/16, RCO is 3MHz pulse every 3MHz/16 (before falling edge of QD)
 - 16 bit shift register to SDATA is load on each RCO pulse, then serialized at 3MHz
 - /RA0 is wired to row clock while rows are rasterized 3 times each, this leads to rasterizing twice the first row and once the third row.
 - Memory is mapped to rasterizer through:
    CA0..9   - A0..9 [always starts at 0, therefore mapping is not implemented in PinMame]
    RA1      - A10
    CA10..13 - A11..A14
	Therefore, 2 first frames are rasterized from base address, while third one is rasterized from base + 0x0400*2 (since this a 16 bit arch)
 - ROWDATA is generated by a little PAL (VSYNC from CRTC is unwired).
 - IRQ2 to CPU is raised by ROWDATA when starting a new frame. It is acknowledged on R/W access from CPU to CRTC.
  -----------------------------*/
static WRITE_HANDLER(dmd64_ctrl_w);
static void dmd64_init(struct sndbrdData *brdData);
static void dmd64_exit(int boardNo);
static void dmd64_vblank(int which);

const struct sndbrdIntf dedmd64Intf = {
  NULL, dmd64_init, dmd64_exit, NULL,NULL,
  dmd_data_w, dmd_busy_r, dmd64_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE16_HANDLER(crtc6845_msb_address_w)  { cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, CLEAR_LINE); if (ACCESSING_MSB) crtc6845_address_0_w(offset,data>>8);  }
static WRITE16_HANDLER(crtc6845_msb_register_w) { cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, CLEAR_LINE); if (ACCESSING_MSB) crtc6845_register_0_w(offset,data>>8); }
static READ16_HANDLER(crtc6845_msb_address_r)   { cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, CLEAR_LINE); return 0; }
static READ16_HANDLER(crtc6845_msb_register_r)  { cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, CLEAR_LINE); return ACCESSING_MSB ? crtc6845_register_0_r(offset)<<8 : 0; }
static WRITE16_HANDLER(dmd64_status_w)          { if (ACCESSING_LSB) sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data & 0xff); }
static READ16_HANDLER(dmd64_latch_r);
static READ16_HANDLER(dmd64_ram_r)              { const UINT16 v = dmd64RAM[offset]; return (v >> 8) | (v << 8); }
static WRITE16_HANDLER(dmd64_ram_w)             { const UINT16 rev_data = (data >> 8) | (data << 8); const UINT16 rev_mask = (mem_mask >> 8) | (mem_mask << 8); dmd64RAM[offset] = (dmd64RAM[offset] & rev_mask) | (rev_data & ~rev_mask); }

// Address decoding is performed by U26, a little PAL chip (no flip flop), using A21/A22 for RAM/ROM/Ext chip, then A3/A4 for chip select
// 00 => ROM 1Mb (can be physically switched by R1 jumper between 2 EPROM sets)
// 01 => ROM (not used, therefore not implemented)
// 10 => RAM 128kb
// 11 => Ext Chip: A3 = CRTC / A4 = Board IO (use synchronous data transfer, with VPA/VMA instead of asynchronous with DTACK)
static MEMORY_READ16_START(dmd64_readmem)
  { 0x00000000, 0x000fffff, MRA16_ROM}, /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, dmd64_ram_r}, /* RAM - 0x800000 Page 0, 0x801000 Page 1 - Reversed endianness */
  { 0x00c00010, 0x00c00011, crtc6845_msb_address_r},
  { 0x00c00012, 0x00c00013, crtc6845_msb_register_r},
  { 0x00c00020, 0x00c00021, dmd64_latch_r}, /* Read the Latch from CPU*/
MEMORY_END

static MEMORY_WRITE16_START(dmd64_writemem)
  { 0x00000000, 0x000fffff, MWA16_ROM},	 /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, dmd64_ram_w, &dmd64RAM},	 /* RAM - 0x800000 Page 0, 0x801000 Page 1 - Reversed endianness */
  { 0x00c00010, 0x00c00011, crtc6845_msb_address_w},
  { 0x00c00012, 0x00c00013, crtc6845_msb_register_w},
  { 0x00c00020, 0x00c00021, dmd64_status_w},/* Set the Status Line*/
MEMORY_END

MACHINE_DRIVER_START(de_dmd64)
  MDRV_CPU_ADD(M68000, 12000000) // schematics
  MDRV_CPU_MEMORY(dmd64_readmem, dmd64_writemem)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

static void dmd64_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  crtc6845_init(0);
  crtc6845_set_vsync(0, (12000000. / 4.) / 16., dmd64_vblank); // See explanation above for frequency (12MHz -> 3MHz -> 3MHz/16)
  core_dmd_pwm_init(&dmdlocals.pwm_state, 192, 64, CORE_DMD_PWM_FILTER_DE_192x64, CORE_DMD_PWM_COMBINER_SUM_2_1);
}

static void dmd64_exit(int boardNo) {
   core_dmd_pwm_exit(&dmdlocals.pwm_state);
}

static WRITE_HANDLER(dmd64_ctrl_w) {
  if (~dmdlocals.ctrl & data & 0x01) { // Cmd ready
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_1, HOLD_LINE);
    sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 1);
    dmdlocals.cmd = dmdlocals.ncmd;
  }
  if (dmdlocals.ctrl & ~data & 0x02) // Reset (applied at falling edge)
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
  dmdlocals.ctrl = data;
}

static READ16_HANDLER(dmd64_latch_r) {
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 0);
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_1, CLEAR_LINE);
  return dmdlocals.cmd;
}

static void dmd64_vblank(int which) {
  //static double prev; printf("DMD VBlank %8.5fms => %8.5fHz Base address: %04x\n", timer_get_time() - prev, 1. / (timer_get_time() - prev), crtc6845_start_address_r(0)); prev = timer_get_time();
  // Store 2 next rasterized frame, as the CRTC is set up to render 2 contiguous full frames for each VBLANK
  unsigned int base = crtc6845_start_address_r(0); // MA0..13
  assert((base & 0x03FF) == 0x0000); // As the mapping of lowest 10 bits is not implemented (would need complex data copy and does not seem to be used by any game)
  unsigned int src = (base & 0x3C00) << 2;
  core_dmd_submit_frame(&dmdlocals.pwm_state, (UINT8*)dmd64RAM +  src,           2); // First frame has been displayed 2/3 of the time
  core_dmd_submit_frame(&dmdlocals.pwm_state, (UINT8*)dmd64RAM + (src | 0x0800), 1); // Second frame has been displayed 1/3 of the time
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, HOLD_LINE); // Note that IRQ2 is not caused by VSYNC (unwired) but generated from ROWDATA, but this seems precise enough
}

PINMAME_VIDEO_UPDATE(dedmd64_update) {
  core_dmd_video_update(bitmap, cliprect, layout, &dmdlocals.pwm_state);
  return 0;
}

/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
#define DMD16_BANK0 2

#define DMD16_NMIFREQ       (4000000./2048.) // Main CPU clock divided by a 14 bit binary counter with 3 disabled outputs, so 2^11
#define DMD16_NMIFREQ_START (4000000./512.)  // Initial delay to falling edge of NMI, so after a 9 bit divider
// Steve said he measured this to 2000 on his game which validates these timings.
// Note: on previous version that would sometimes causes a new NMI to be triggered before the previous one is finished and it leads to stack overflow
// Doesn't seems to happen anymore (lots of things have been fixed since, so hopefully fixed too)

static void dmd16_init(struct sndbrdData *brdData);
static void dmd16_exit(int boardNo);

static WRITE_HANDLER(dmd16_ctrl_w);

const struct sndbrdIntf dedmd16Intf = {
  NULL, dmd16_init, dmd16_exit, NULL,NULL,
  dmd_data_w, dmd_busy_r, dmd16_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND | SNDBRD_NODATASYNC | SNDBRD_NOCTRLSYNC | SNDBRD_NOCBSYNC
};

static READ_HANDLER(dmd16_port_r);
static WRITE_HANDLER(dmd16_port_w);

static MEMORY_READ_START(dmd16_readmem)
  { 0x0000, 0x3fff, MRA_ROM },                 /* Z80 ROM CODE*/
  { 0x4000, 0x7fff, MRA_BANKNO(DMD16_BANK0) }, /* ROM BANK*/
  { 0x8000, 0x9fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(dmd16_writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x7fff, MWA_ROM },
  { 0x8000, 0x9fff, MWA_RAM },
MEMORY_END

static PORT_READ_START(dmd16_readport)
  { 0x00, 0xff, dmd16_port_r },
PORT_END

static PORT_WRITE_START(dmd16_writeport)
  { 0x00, 0xff, dmd16_port_w },
PORT_END

MACHINE_DRIVER_START(de_dmd16) // Board part 520-5042-00
  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_MEMORY(dmd16_readmem, dmd16_writemem)
  MDRV_CPU_PORTS(dmd16_readport, dmd16_writeport)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* HC74 updated events */
#define BUSY_UPD_PRESET    0x01 /* Update output after init/reset ot TEST signal state change */
#define BUSY_CLR_PULSE     0x02 /* CLR reversed pulse (only triggered by IDAT) */
#define BUSY_CLK_EDGE      0x03 /* Clock edge */

static void dmd16_updbusy(int evt);
static void dmd16_setbank(int bit, int value);
static void dmd16_nmi(int _) { cpu_set_nmi_line(dmdlocals.brdData.cpuNo, PULSE_LINE); }

static void dmd16_reset(void) {
   // U4 outputs
   dmd16_setbank(0x07, 0x07);
   dmdlocals.blnk    = 0;
   dmdlocals.status  = 0;
   dmdlocals.rowdata = 0;
   dmdlocals.rowclk  = 0;
   dmdlocals.test    = 1; // FIXME, I think this should be 0
   dmd16_updbusy(BUSY_UPD_PRESET);
   // NMI counter is also reseted
   timer_adjust(dmdlocals.nmi_timer, 1. / DMD16_NMIFREQ_START, 0, 1. / DMD16_NMIFREQ);
   // Not really reseted but makes things cleaner
   dmdlocals.last_row_latch = dmdlocals.row_latch = 0;
   dmdlocals.dot_latch = dmdlocals.dot_shift = 0;
}

static void dmd16_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  memcpy(memory_region(DE_DMD16CPUREGION), memory_region(DE_DMD16ROMREGION) + memory_region_length(DE_DMD16ROMREGION)-0x4000,0x4000);
  dmdlocals.nmi_timer = timer_alloc(dmd16_nmi);
  dmd16_reset();

  // Init PWM shading
  core_dmd_pwm_init(&dmdlocals.pwm_state, 128, 16, CORE_DMD_PWM_FILTER_DE_128x16, CORE_DMD_PWM_COMBINER_SUM_1_2);
}

static void dmd16_exit(int boardNo) {
   core_dmd_pwm_exit(&dmdlocals.pwm_state);
}

INLINE void dmd16_interlace(UINT64 v, UINT8* row)
{
  row[7] =  v        & 0x00FF;
  row[6] = (v >>  8) & 0x00FF;
  row[5] = (v >> 16) & 0x00FF;
  row[4] = (v >> 24) & 0x00FF;
  row[3] = (v >> 32) & 0x00FF;
  row[2] = (v >> 40) & 0x00FF;
  row[1] = (v >> 48) & 0x00FF;
  row[0] = (v >> 56) & 0x00FF;
}

static void dmd16_updrow(void) {
  if (dmdlocals.blnk && dmdlocals.row_latch) {
    // row_latch always has a single bit set, the rasterized row/side, which is odd for left 64x16 panel and even for right 64x16 panel
    assert(dmdlocals.row_latch == (dmdlocals.row_latch - (dmdlocals.row_latch & dmdlocals.row_latch - 1))); // check that we only have one bit set
    static const int MultiplyDeBruijnBitPosition[32] = 
      { 0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9 };
    const int row_n_side = MultiplyDeBruijnBitPosition[((UINT32)((dmdlocals.row_latch & -dmdlocals.row_latch) * 0x077CB531U)) >> 27]; // Compute index of the first trailing set bit (0 based)
    const int row = row_n_side >> 1;
    const int side = row_n_side & 1;
    //printf("%08x %2d %d %I64x\n", dmdlocals.row_latch, row, side, interlace(dmdlocals.hv5408, dmdlocals.hv5308));
    if (dmdlocals.last_row_latch != dmdlocals.row_latch) { // Write to a new row/side: consider it as a long row display (0.50ms)
      dmd16_interlace(dmdlocals.dot_latch, &(dmdlocals.framedata[0][row][side][0]));
      dmd16_interlace(dmdlocals.dot_latch, &(dmdlocals.framedata[1][row][side][0]));
    } else { // Second write to the same row/side: we have 2 frames, the first of 0.13ms and second (already stored, just keep it) of 0.37ms
      dmd16_interlace(dmdlocals.dot_latch, &(dmdlocals.framedata[0][row][side][0]));
    }
    dmdlocals.last_row_latch = dmdlocals.row_latch;
  }
  if (dmdlocals.blnk && dmdlocals.row_latch == 0) {
    //static double prev; printf("DMD VBlank %8.5fms => %8.5fHz for 3 frames so %8.5fHz\n", timer_get_time() - prev, 1. / (timer_get_time() - prev), 3. / (timer_get_time() - prev)); prev = timer_get_time();
    core_dmd_submit_frame(&dmdlocals.pwm_state, (UINT8*) dmdlocals.framedata[0], 1); // First frame has been displayed 1/3 of the time
    core_dmd_submit_frame(&dmdlocals.pwm_state, (UINT8*) dmdlocals.framedata[1], 2); // Second frame has been displayed 2/3 of the time
  }
}

/*--- Port decoding ----
  76543210 [Latched D0 data, reseted to 0 on reset]
  10-001-- .W Bank0 (reversed)
  10-011-- .W Bank1 (reversed)
  10-101-- .W Bank2 (reversed)
  10-111-- .W Blanking
  11-001-- .W Status (to controller board)
  11-011-- .W Row Data
  11-101-- .W Row Clock
  11-111-- .W Test
  ------ [Reversed pulsed (always high unless written to, then generates a low pulse]
  0----1-- .W CLATCH
  0----0-- .W COCLK
  1----0-- R. IDAT (read incoming command and acknowledge to controller board through BUSY signal)
--------------------*/

static READ_HANDLER(dmd16_port_r) {
  if ((offset & 0x84) == 0x80) { // IDAT reversed pulse
    dmd16_updbusy(BUSY_CLR_PULSE);
    return dmdlocals.cmd;
  }
  assert(0); // This would be invalid from the hardware point of view
  dmd16_port_w(offset,0xff);
  return 0xff;
}

static WRITE_HANDLER(dmd16_port_w) {
  switch (offset & 0x84) {
    case 0x00: // COCLK pulse (32 clock pulse per row side)
      data = (data >> ((offset & 0x03) * 2)) & 0x03;
      dmdlocals.dot_shift = (dmdlocals.dot_shift << 2) | ((UINT64)data);
      break;
    case 0x04: // CLATCH pulse (1 or 2 pulse per row side, to allow PWM shading)
      dmdlocals.dot_latch = dmdlocals.dot_shift;
      dmd16_updrow();
      break;
    case 0x80: // IDAT pulse
      assert(0); // Invalid as this would activate both the CPU and the latch
      break;
    case 0x84: // Latched outputs
      data &= 0x01;
      switch (offset & 0xdc) {
        case 0x84: // Bank0 (reversed)
          dmd16_setbank(0x01, !data); break;
        case 0x8c: // Bank1 (reversed)
          dmd16_setbank(0x02, !data); break;
        case 0x94: // Bank2 (reversed)
          dmd16_setbank(0x04, !data); break;
        case 0x9c: // Blanking
          dmdlocals.blnk = data;
          dmd16_updrow();
          break;
        case 0xc4: // Status
          dmdlocals.status = data;
          sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status);
          break;
        case 0xcc: // Row data
          dmdlocals.rowdata = data;
          break;
        case 0xd4: // Row clock
          if (dmdlocals.rowclk & ~data) // High to low edge
            dmdlocals.row_latch = (dmdlocals.row_latch << 1) | (dmdlocals.rowdata);
          dmdlocals.rowclk = data;
          dmd16_updrow();
          break;
        case 0xdc: // Test
          //printf("%8.5f Set TEST = %02x [Busy=%02x]\n", timer_get_time(), dmdlocals.test, dmdlocals.busy);
          dmdlocals.test = data;
          dmd16_updbusy(BUSY_UPD_PRESET);
          break;
      } // Switch
      break;
  } // switch
}

static WRITE_HANDLER(dmd16_ctrl_w) {
  if (~dmdlocals.ctrl & data & 0x01) { // J1-19 on raising edge, latch incoming command and set BUSY/INT
    //printf("%8.5f Cmd %02x\n", timer_get_time(), dmdlocals.ncmd);
    dmdlocals.cmd = dmdlocals.ncmd;
    dmd16_updbusy(BUSY_CLK_EDGE);
  }
  if (dmdlocals.ctrl & ~data & 0x02) { // J1-20 RESET (trigger on falling edge of signal while it should be a continuous reset state when high)
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
    dmd16_reset();
  }
  dmdlocals.ctrl = data;
}

static void dmd16_updbusy(int evt) {
  //const int prev_busy = dmdlocals.busy;
  // TODO CLR_PULSE will reset the flip flop to 0 even while PRESET is pulled down. This does not match the datasheet which states
  // that in this situation both output would be high (so keep BUSY at 1, but also trigger INT1).
  if (evt == BUSY_CLR_PULSE)
    dmdlocals.busy = 0;
  else if ((dmdlocals.test == 0) || (evt == BUSY_CLK_EDGE))
    dmdlocals.busy = 1;
  //if (dmdlocals.busy != prev_busy) printf("%8.5f Busy Change to %02x (cause: %02x)\n", timer_get_time(), dmdlocals.busy, evt);
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, Z80_INT_REQ, dmdlocals.busy ? HOLD_LINE : CLEAR_LINE);
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy);
}

static void dmd16_setbank(int bit, int value) {
  dmdlocals.bank = (dmdlocals.bank & ~bit) | (value ? bit : 0);
  cpu_setbank(DMD16_BANK0, dmdlocals.brdData.romRegion + (dmdlocals.bank & 0x07)*0x4000);
}

/*-- update display --*/
PINMAME_VIDEO_UPDATE(dedmd16_update) {
  core_dmd_video_update(bitmap, cliprect, layout, &dmdlocals.pwm_state);
  return 0;
}
