/*Data East/Sega/Stern Sound Hardware:
  ------------------------------------
  Generation 1: YM2151 & MSM5205 (Games from Laser War to TMNT)
  Generation 2: BSMT 2000 (Games from Batman to Terminator 3)
  Generation 3: AT91 CPU (Game from LOTR and beyond)
*/
#include "driver.h"
#include "core.h"
#include "cpu/m6809/m6809.h"
#include "cpu/at91/at91.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"
#include "desound.h"
#include "machine/6821pia.h"
#include "sndbrd.h"

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

/****************************************/
/** GENERATION 1 - YM2151 & MSM5205    **/
/****************************************/
/* by Steve Ellenoff                    */
/****************************************/
#define DE1S_BANK0 1

static void de1s_init(struct sndbrdData *brdData);
static WRITE_HANDLER(de1s_data_w);
static WRITE_HANDLER(de1s_ctrl_w);
static WRITE_HANDLER(de1s_manCmd_w);
static READ_HANDLER(de1s_cmd_r);
static void de1s_msmIrq(int data);
static void de1s_ym2151IRQ(int state);
static WRITE_HANDLER(de1s_ym2151Port);
static WRITE_HANDLER(de1s_chipsel_w);
static WRITE_HANDLER(de1s_4052_w);
static WRITE_HANDLER(de1s_MSM5025_w);

const struct sndbrdIntf de1sIntf = {
  "DE", de1s_init, NULL, NULL, de1s_manCmd_w, de1s_data_w, NULL, de1s_ctrl_w, NULL
};

static struct MSM5205interface de1s_msm5205Int = {
/* chip          interrupt */
     1, 384000,	{ de1s_msmIrq }, { MSM5205_S48_4B }, { 60 }
};

static struct YM2151interface de1s_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
  { de1s_ym2151IRQ }, { de1s_ym2151Port }
};

static MEMORY_READ_START(de1s_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2001, 0x2001, YM2151_status_port_0_r },
  { 0x2400, 0x2400, de1s_cmd_r },
  { 0x3800, 0x3800, watchdog_reset_r},
  { 0x4000, 0x7fff, MRA_BANKNO(DE1S_BANK0) },	/*Voice Samples*/
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(de1s_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, YM2151_register_port_0_w },
  { 0x2001, 0x2001, YM2151_data_port_0_w },
  { 0x2800, 0x2800, de1s_chipsel_w },
  { 0x2c00, 0x2c00, de1s_4052_w },
  { 0x3000, 0x3000, de1s_MSM5025_w },
  { 0x3800, 0x3800, watchdog_reset_w },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static struct {
  struct sndbrdData brdData;
  int    msmread, nmiEn, cmd;
  UINT8  msmdata;
} de1slocals;

MACHINE_DRIVER_START(de1s)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(de1s_readmem, de1s_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(YM2151,  de1s_ym2151Int)
  MDRV_SOUND_ADD(MSM5205, de1s_msm5205Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static void de1s_init(struct sndbrdData *brdData) {
  memset(&de1slocals, 0, sizeof(de1slocals));
  de1slocals.brdData = *brdData;
  cpu_setbank(DE1S_BANK0, de1slocals.brdData.romRegion);
  watchdog_reset_w(0,0);
  MSM5205_playmode_w(0,MSM5205_S96_4B); /* Start off MSM5205 at 4khz sampling */
}

static WRITE_HANDLER(de1s_data_w) {
  de1slocals.cmd = data;
}

static WRITE_HANDLER(de1s_ctrl_w) {
  if (~data&0x1) cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_FIRQ_LINE, ASSERT_LINE);
}
static WRITE_HANDLER(de1s_manCmd_w) {
  de1s_data_w(0,data); de1s_ctrl_w(0,0);
}
static READ_HANDLER(de1s_cmd_r) {
  cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_FIRQ_LINE, CLEAR_LINE);
  return de1slocals.cmd;
}

static void de1s_ym2151IRQ(int state) {
  cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(de1s_4052_w) { /*logerror("to4052 %x\n",data);*/ }

/*
Chip 5e: LS273: (0x2800)
---------------
bit 0 = 2 = Select A14 on 4f/6f
bit 1 = 3 = Select A15 on 4f/6f
bit 2 = 4 = Chip select 4f or 6f. (0=6F,1=4F)
bit 3 = N/U
bit 4 = 3 = S1 on MSM5205
bit 5 = 2 = S2 on MSM5205
bit 6 = 1 = RSE on MSM5205
bit 7 = 0 = CLEAR NMI
********************
6f is loaded from 0-1ffff
4f is loaded from 20000-3ffff
*/
static WRITE_HANDLER(de1s_chipsel_w) {
  static const int prescaler[] = { MSM5205_S96_4B, MSM5205_S48_4B, MSM5205_S64_4B, 0};
  int addr = (((data>>0)&0x01)*0x4000) +
	     (((data>>1)&0x01)*0x8000) +
	     (((data>>3)&0x01)*0x10000) +
	     (((data>>2)&0x01)*0x20000);

  cpu_setbank(DE1S_BANK0, de1slocals.brdData.romRegion+addr);
  MSM5205_playmode_w(0, prescaler[(data & 0x30)>>4]); /* bit 4&5 */
  MSM5205_reset_w(0,   (data & 0x40)); /* bit 6 */
  de1slocals.nmiEn =  (~data & 0x80); /* bit 7 */
}

static WRITE_HANDLER(de1s_MSM5025_w) {
  de1slocals.msmdata = data;
  de1slocals.msmread = 0;
}

/* MSM5205 interrupt callback */
static void de1s_msmIrq(int data) {
  MSM5205_data_w(0, de1slocals.msmdata>>4);

  de1slocals.msmdata <<= 4;

  // Are we done fetching both nibbles? Generate an NMI for more data!
  if (de1slocals.msmread && de1slocals.nmiEn)
    cpu_set_nmi_line(de1slocals.brdData.cpuNo, PULSE_LINE);

  de1slocals.msmread ^= 1;
}

/*Send CT2 data to Main CPU's PIA*/
static WRITE_HANDLER(de1s_ym2151Port) {
  sndbrd_ctrl_cb(de1slocals.brdData.boardNo, data & 0x02);
}


/****************************************/
/** GENERATION 2 - BSMT SOUND HARDWARE **/
/****************************************/
/* by Steve Ellenoff, Martin Adrian, and Aaron Giles

 BSMT Clock rate of 24Mhz is confirmed to match the pitch on the real machines (10/31/04)
 Compressed Voice is not really understood and still sounds pretty bad
 There are some special effects the BSMT is programmed to do, but we don't know how, this
 can be heard on the BSMT 2000 Logo (for early games like DE - Star Wars)

 Missing things
 When a sound command is written from the Main CPU it generates a BUF-FULL signal
 and latches the data into U5
 The BUF-FULL signal goes into the PAL. No idea what happens there
 When the CPU reads the sound commands (BIN) a FIRQ is generated and BUF-FULL
 is cleared.
 A FIRQ is also generated from the 24MHz signal via two dividers U3 & U2
 (I think it is 24MHz/4/4096=1536 Hz)

 The really strange thing is the PAL. It can generate the following output
 BUSY  - Not used on BSMT board. Sent back to CPU board to STATUS latch?
 OSTAT - If D7 from the CPU is high this sends a reset to the BSMT
         D0 is sent to CPU board as SST0
 BROM  - ROM for CPU
 BRAM  - RAM for CPU
 DSP0  - Latches A0-A7 and D0-D7 (low) into BSMT
 DSP1  - Latches D0-D7 (high) into BSMT. Ack's IRQ
 BIN   - Gets sound command from LATCH, Clears BUF-FUL and Generates FIRQ
 Input to the PAL is
 A15,A14,A13,A2,A1
 BUF-FUL
 FIRQ
 IRQ
 (E, R/W)
***********************************************************************************/

static void de2s_init(struct sndbrdData *brdData);
static READ_HANDLER(de2s_bsmtready_r);
static WRITE_HANDLER(de2s_bsmtreset_w);
static WRITE_HANDLER(de2s_bsmtcmdHi_w);
static WRITE_HANDLER(de2s_bsmtcmdLo_w);
static INTERRUPT_GEN(de2s_firq);

const struct sndbrdIntf de2sIntf = {
  "BSMT", de2s_init, NULL, NULL, soundlatch_w, soundlatch_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

/* ---------------------------------------------------------------------------------------------------------------*/
/* The ONLY differences in the different BSMT interfaces here are for adjusting volume & # of voices used         */
/* The # of voices should really be handled by the reset lines and the bsmt emulation, but for now it's hardcoded */
/* ---------------------------------------------------------------------------------------------------------------*/

/* 11 Voice Style BSMT Chip used with Data East (Hook/Batman/Star Wars, etc..) */
static struct BSMT2000interface de2s_bsmt2000aaInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, 1, 1, 0
};

/* 11 Voice Style BSMT Chip used with Sega/Stern (Apollo13,ID4,Godzilla,Monopoly,RCTYCN)*/
static struct BSMT2000interface de2s_bsmt2000aInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, 1, 4, 0
};

/* 12 Voice Style BSMT Chip used with later model Stern (Austin Powers and forward) */
static struct BSMT2000interface de2s_bsmt2000bInt = {
  1, {24000000}, {12}, {DE2S_ROMREGION}, {100}, 1, 4, 0
};

/* 11 Voice Style BSMT Chip used on Titanic */
static struct BSMT2000interface de2s_bsmt2000tInt = {
  1, {28000000}, {11}, {DE2S_ROMREGION}, {100}, 1, 4, 0
};

static MEMORY_READ_START(de2s_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2002, 0x2003, soundlatch_r },
  { 0x2006, 0x2007, de2s_bsmtready_r },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(de2s_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2001, de2s_bsmtreset_w },
  { 0x2008, 0x5fff, MWA_ROM },
  { 0x6000, 0x6000, de2s_bsmtcmdHi_w },
  { 0x6001, 0x9fff, MWA_ROM },
  { 0xa000, 0xa0ff, de2s_bsmtcmdLo_w },
  { 0xa100, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(de2as)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(de2s_readmem, de2s_writemem)
  MDRV_CPU_PERIODIC_INT(de2s_firq, 489) /* Fixed FIRQ of 489Hz as measured on real machine*/
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD_TAG("bsmt", BSMT2000, de2s_bsmt2000aInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2aas)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000aaInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2bs)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000bInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2ts)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000tInt)
MACHINE_DRIVER_END

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  int bsmtData;
} de2slocals;

static void de2s_init(struct sndbrdData *brdData) {
  memset(&de2slocals, 0, sizeof(de2slocals));
  de2slocals.brdData = *brdData;
}

static char tmp[100];
static char *rtos( int n )
{
	switch(n)
	{
		case 0:
		sprintf(tmp,"CURRPOS");
		break;
		case 1:
		sprintf(tmp,"UNKNOWN");
		break;
		case 2:
		sprintf(tmp,"RATE");
		break;
		case 3:
		sprintf(tmp,"LOOPEND");
		break;
		case 4:
		sprintf(tmp,"LOOPSTART");
		break;
		case 5:
		sprintf(tmp,"BANK");
		break;
		case 6:
		sprintf(tmp,"RVOL");
		break;
		case 7:
		sprintf(tmp,"LVOL");
		break;
		case 8:
		sprintf(tmp,"RVOL");
		break;
		case 9:
		sprintf(tmp,"??");
		break;
		default:
		sprintf(tmp,"??");
		break;
	}
	return tmp;
}

static WRITE_HANDLER(de2s_bsmtcmdHi_w) {
	de2slocals.bsmtData = data;
//	LOG(("%04x: hi=%x\n",activecpu_get_pc(),data));
}

static WRITE_HANDLER(de2s_bsmtcmdLo_w)
{

#if 0
   int reg = offset ^ 0xff;
   LOG(("(BSMT write to %02X (V%X R%d) %s = %02X%02X\n",
        reg, reg % 12, reg/12, rtos(reg / 12), de2slocals.bsmtData, data));
#endif

  BSMT2000_data_0_w((~offset & 0xff), ((de2slocals.bsmtData<<8)|data), 0);

  //SJE: 10/2004 - Ironically, this actually is not needed it seems!
  //NOTE: Odd that it will NOT WORK without HOLD_LINE - although we don't clear it anywaywhere!
  //cpu_set_irq_line(de2slocals.brdData.cpuNo, M6809_IRQ_LINE, HOLD_LINE);
}

static READ_HANDLER(de2s_bsmtready_r) { return 0x80; } // BSMT is always ready
/* Writing 0x80 here resets BSMT ?*/
static WRITE_HANDLER(de2s_bsmtreset_w) {
	static int last_data = 0;
	//Watch for 0->1 transition in 8th bit to force a reset
	if(data & 0x80 && (last_data & 0x80) == 0)
	{
		BSMT2000_sh_reset();
	}
	last_data = data;
}

static INTERRUPT_GEN(de2s_firq) {
  //NOTE: Odd that it will NOT WORK without HOLD_LINE - although we don't clear it anywaywhere!
  cpu_set_irq_line(de2slocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}


/************************************************/
/* GENERATION 3 Sound Hardware - AT91 Based CPU */
/************************************************/
/* by Steve Ellenoff ( 10/11/2004 - 10/28/2004 )

   Hardware: CPU/Sound Bd. II w/ ATMEL Processor
			 SPI Part Nº: 520-5300-00

   Uses an Atmel AT9140008 CPU (ARM7TDMI Core) and Xilinx FPGA for sound.
   Board is 100% compatible with previous generation hardware (all games using 8Mb roms)
   by simply replacing the older game roms into the board.
   The BSMT chip is software emulated by the AT91 CPU and has added new capabilities such as
   16 bit sample support and ADPCM compression. The Xilinx FPGA is programmed to handle a variety of
   tasks. It controls 2 of the ROM Enable lines, acts as a sound command buffer from the main cpu,
   and most importantly, acts as a simple DSP chip which converts the 16 bit sample data into a serial sound
   stream for output to a DAC. It may have other functionality as well, but not related to sound if so.

   Because the AT91 core swaps RAM into page 0 memory after a
   certain register write occurs, and the boot code is also swapped, I needed to provide the
   AT91 code pointers to the memory region data so it could perform the swap, see init for details.

   Internally the code sets up a timer interrupt @ 24,242Hz (40Mhz/2/0x339).

   -10/13/2006
    Removed external irq call and silly irq ready hacks
	Implemented AT91 port handling
	Implemented SST0 & PLIN data writes (used by Elvis and later gen. games)
	Added Sound LED (though it's not getting displayed to screen as much as it should oddly)
   -03/25/2007
    Removed all memory callback handling
	Implemented proper memory mappped handling
	Simplified memory handlers
	Added logging flags & cleaned up readability of code a bit.
	Commented out support for flash bios and test driver as I don't have desire to convert them over to use proper memory handler
*/

//Switches
#define AT91IMP_REMOVE_LED_CODE			1	// Set to 1 to remove code that flashes the LED at startup.
#define AT91IMP_MAKE_WAVS				0	// Set to 1 to save sound output as wave file

//Logging Options
#define AT91IMP_LOG_TO_SCREEN			0	// Set to 1 to print log data to screen instead of logfile
#define AT91IMP_LOG_ROMS_U17_U37		0	// Set to 1 to log data reads from ROMS U17-U37
#define AT91IMP_LOG_PORT_WRITE			0	// Set to 1 to log port write access
#define AT91IMP_LOG_PORT_READ			0	// Set to 1 to log port read access
#define AT91IMP_LOG_DATA_STREAM			0	// Set to 1 to log sound data stream output
#define AT91IMP_LOG_SOUND_CMD			0	// Set to 1 to sound command read access
#define	AT91IMP_LOG_NO_SAMPLES_2PLAY	0	// Set to 1 to log when there are not enough sound samples to play

//Definitions
#define ARMCPU_FREQ	40000000				//40 MHZ
#define ARMIRQ_FREQ ARMCPU_FREQ/2/0x339		//Works out to be 24,242Hz
#define WAVE_OUT_RATE ARMIRQ_FREQ			//Output rate is exactly the IRQ frequency
#define ARMSNDBUFSIZE 400
#define BUFFSIZE 0x100000

//Includes
#if AT91IMP_MAKE_WAVS
#include "sound/wavwrite.h"
#endif

//Prototypes
extern void set_at91_data(int plin, int sst0, int led);
static READ_HANDLER(scmd_r);

//Variables
static data32_t *de3as_reset_ram;
static data32_t *de3as_page0_ram;
static data32_t *u7_base;
static int sndcmdbuf[ARMSNDBUFSIZE];
static int sbuf=0;
static int spos=0;
static int sampout = 0;
static int sampnum = 0;
static const int rommap[4] = {4,2,3,1};
static INT16 samplebuf[BUFFSIZE];

#if AT91IMP_MAKE_WAVS
static void * wavraw;					/* raw waveform */
#endif

#ifdef LOG
#undef LOG
#endif

#if AT91IMP_LOG_TO_SCREEN
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
static READ32_HANDLER(xilinx_r)
{
	data32_t data = 0;

	//Xilinx Provides Sound Command from Main CPU
	#if AT91IMP_LOG_SOUND_CMD
	static int lastcmd = 0;
	#endif

	data = scmd_r(0);

	#if AT91IMP_LOG_SOUND_CMD
	if(lastcmd != data) {
		LOG(("%08x: SND CMD: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
		lastcmd = data;
	}
	#endif

	return data;
}

//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
static READ32_HANDLER(roms_r)
{
	data32_t data = 0;
	int mask_adjust = 0;

	//Adjust offset due to the way MAME 32Bit handler works
	offset*=4;
	switch(mem_mask^0xffffffff)
	{
		case 0x000000ff:
			mask_adjust=0;
			break;
		case 0x0000ff00:
			mask_adjust=1;
			break;
		case 0x00ff0000:
			mask_adjust=2;
			break;
		case 0xff000000:
			mask_adjust=3;
			break;
	}
	offset+=mask_adjust;
	offset+=0x20000004;			//add back base offset
	//Read from U17-U37 ROMS
	{
		//remove a29 & a22
		int romaddr = offset & 0xDFBFFFFF;
		//determine which chip (combine A21 & A0 into 2 bit #)
		int romchip = rommap[(((romaddr & 0x200000)>>20)|(romaddr&1))];
		//remove a21 and >>1 the address
		romaddr = (romaddr&0xFFDFFFFF)>>1;

		data = (data8_t)*((memory_region(REGION_SOUND2) + romaddr + ((romchip-1) * 0x100000)));
		#if AT91IMP_LOG_ROMS_U17_U37
		LOG(("%08x: reading from U%d: %08x = %08x (%08x)\n",activecpu_get_pc(),romchip,romaddr,data,offset));
		#endif
	}
	//Adjust for Mask
	return data << (8*mask_adjust);
}

//CSR 2 Mapped to 0x20000000 (Xilinx) - Sound Data Stream Output
static WRITE32_HANDLER(xilinx_w)
{
	//Store 16 bit data to our buffer
	samplebuf[sampnum] = data & 0xffff;
	sampnum = (sampnum + 1) % BUFFSIZE;

	//Dump to Wave File
	#if AT91IMP_MAKE_WAVS
	if(wavraw)
	{
		INT16 d;
		d = (INT16)data;
		wav_add_data_16(wavraw, &d, 1);
	}
	#endif

	#if AT91IMP_LOG_DATA_STREAM
	LOG(("%08x: DATA STREAM: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	#endif
}

//Remove Delay from LED Flashing code to speed up the boot time of the cpu
static void remove_led_code(void)
{

  //LOTR OS Version -
  if(
	  (de3as_page0_ram[(0x2854+0x8000)/4] == 0xeb000061) &&
	  (de3as_page0_ram[(0x2860+0x8000)/4] == 0xeb00005e) &&
	  (de3as_page0_ram[(0x286c+0x8000)/4] == 0xeb00005b) &&
	  (de3as_page0_ram[(0x2878+0x8000)/4] == 0xeb000058) &&
	  (de3as_page0_ram[(0x2884+0x8000)/4] == 0xeb000055) &&
	  (de3as_page0_ram[(0x2890+0x8000)/4] == 0xeb000052) &&
	  (de3as_page0_ram[(0x289c+0x8000)/4] == 0xeb00004f) &&
	  (de3as_page0_ram[(0x28a8+0x8000)/4] == 0xeb00004c)
	)
		{
			de3as_page0_ram[(0x2854+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2860+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x286c+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2878+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2884+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2890+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x289c+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x28a8+0x8000)/4] = (UINT32)0;
			LOG(("LOTR OS - LED Code Removed!\n"));
		}

  //ELVIS OS Version -
  if(
	  (de3as_page0_ram[(0x2d90+0x8000)/4] == 0xeb00004b) &&
	  (de3as_page0_ram[(0x2d9c+0x8000)/4] == 0xeb000048) &&
	  (de3as_page0_ram[(0x2da8+0x8000)/4] == 0xeb000045) &&
	  (de3as_page0_ram[(0x2db4+0x8000)/4] == 0xeb000042) &&
	  (de3as_page0_ram[(0x2dc0+0x8000)/4] == 0xeb00003f) &&
	  (de3as_page0_ram[(0x2dcc+0x8000)/4] == 0xeb00003c) &&
	  (de3as_page0_ram[(0x2dd8+0x8000)/4] == 0xeb000039) &&
	  (de3as_page0_ram[(0x2de4+0x8000)/4] == 0xeb000036) &&
	  (de3as_page0_ram[(0x2df0+0x8000)/4] == 0xeb000033) &&
	  (de3as_page0_ram[(0x2dfc+0x8000)/4] == 0xeb000030)
	)
		{
			de3as_page0_ram[(0x2d90+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2d9c+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2da8+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2db4+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2dc0+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2dcc+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2dd8+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2de4+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2df0+0x8000)/4] = (UINT32)0;
			de3as_page0_ram[(0x2dfc+0x8000)/4] = (UINT32)0;
			LOG(("ELVIS OS - LED Code Removed!\n"));
		}
}

static void setup_at91(void)
{
  //because the boot rom code gets written to ram, and then remapped to page 0, we need an interface to handle this.
  at91_set_ram_pointers(de3as_reset_ram,de3as_page0_ram);
  //Copy U7 ROM into correct location (ie, starting at 0x40000000 where it is mapped)
  memcpy(u7_base, memory_region(REGION_SOUND1), memory_region_length(REGION_SOUND1));
}

static void de3s_init(struct sndbrdData *brdData) {
  memset(&de2slocals, 0, sizeof(de2slocals));
  de2slocals.brdData = *brdData;
  setup_at91();

  #if AT91IMP_MAKE_WAVS
	wavraw = wav_open("raw.wav", WAVE_OUT_RATE, 2);
  #endif

  #if AT91IMP_REMOVE_LED_CODE
	remove_led_code();
  #endif
}

//Write new sound command to the command queue
static WRITE_HANDLER(scmd_w)
{
	sndcmdbuf[sbuf]=data;
	sbuf = (sbuf + 1) % ARMSNDBUFSIZE;
}

//Buffer the sound commands (to account for timing offsets between the 6809 Main CPU & the AT91 CPU)
static READ_HANDLER(scmd_r)
{
	int data = sndcmdbuf[spos];
	if(spos < sbuf)
		spos = (spos + 1) % ARMSNDBUFSIZE;
	if(spos >= sbuf) {
		spos = sbuf = 0;
		sndcmdbuf[0] = data;
	}
	return data;
}


static int at91_stream = 0;
static INT16 lastsamp = 0;

static void at91_sh_update(int num, INT16 *buffer, int length)
{
 int ii=0;

 /* fill in with samples until we hit the end or run out */
 for (ii = 0; ii < length; ii++) {
	if(sampout == sampnum || sampnum < 500) {
		#if AT91IMP_LOG_NO_SAMPLES_2PLAY
		LOG(("not enough samples to play\n"));
		#endif
		break;	//drop out of loop
	}

	//Send next pcm sample to output buffer
	buffer[ii] = samplebuf[sampout];

	//Loop to beginning if we reach end of pcm buffer
	sampout = (sampout + 1) % BUFFSIZE;

	//Store last output
	lastsamp = buffer[ii];
 }
 /* fill the rest with last sample output */
 for ( ; ii < length; ii++)
	buffer[ii] = lastsamp;
}

int at91_sh_start(const struct MachineSound *msound)
{
	char stream_name[40];
	/*-- allocate a DAC stream at fixed frequency --*/
	sprintf(stream_name, "%s", "AT91");
	at91_stream = stream_init(stream_name, 100, WAVE_OUT_RATE*2, 0, at91_sh_update);		// RATE * 2 because we didn't separate out the left/right data stream
	return at91_stream < 0;
}

void at91_sh_stop(void)
{
}

static WRITE_HANDLER(man3_w)
{
	scmd_w(0,data);
}


const struct sndbrdIntf de3sIntf = {
	"AT91", de3s_init, NULL, NULL, man3_w, scmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

static struct CustomSound_interface at91CustIntf =
{
	at91_sh_start,
	at91_sh_stop,
	0,
};

/*********************/
/* Port I/O Section  */
/*********************/
READ32_HANDLER(arm_port_r)
{
	data32_t data;
	int logit = AT91IMP_LOG_PORT_READ;
	data = 0;
	if(logit)	LOG(("%08x: Read port - Data = %08x\n",activecpu_get_pc(),data));
	return data;
}

//P16,17,18,19,20,23,24,25 Connected to U404 (Read via PLIN -> DMD Input Enable)
//P4 Connected to SST0 line -> bit 6 of U202 (Read via STATUS -> DMD Status)
//P8 Connected to LED
static WRITE32_HANDLER(arm_port_w)
{
	int sst0, plin, led;

//for debugging
#if AT91IMP_LOG_PORT_WRITE
	char bitstr[33];
	int logit = 1;
	int i;
	for(i = 31; i >= 0; i--)
	{
		if( (data>>i) & 1 )
			bitstr[31-i]='1';
		else
			bitstr[31-i]='0';
	}
	bitstr[32]='\0';
	if(logit)	LOG(("%08x: Write port - Data = %08x  (%s)\n",activecpu_get_pc(),data,bitstr));
#endif

	plin = ((data & 0x1F0000) >> 16) | ((data & 0x3800000) >> 18);
	sst0 = (data & 0x10)>>4;
	led = (data & 0x100)>>8;
	set_at91_data(plin,sst0,led);
}

/******************************/
/*  Memory map for Sound CPU  */
/******************************/
//READ
//CSR 0 Mapped to 0x10000000 - BIOS ROM U8 (2MB)
//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS) - ROMS (4MB FOR ALL)
//	Xilinx Provides Sound Command from Main CPU
//	Read from U17-U37 ROMS
//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
//CSR 1 Mapped to 0x40000000 - U7 ROM (64K)
static MEMORY_READ32_START(arm_readmem)
{0x00000000,0x000FFFFF,MRA32_RAM},						//Boot RAM
{0x00300000,0x003FFFFF,MRA32_RAM},						//Swapped RAM
{0x00400000,0x005FFFFF,MRA32_RAM},						//Mirrored BIOS @ Boot Time
{0x20000000,0x20000003,xilinx_r},						//Xilinx Sound Command Input
{0x20000004,0x207FFFFF,roms_r},							//U17-U37 ROMS (4MB FOR ALL)
{0x40000000,0x4000FFFF,MRA32_ROM},						//U7 ROM (64K)
MEMORY_END

//WRITE
//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
//	Data Stream Output
static MEMORY_WRITE32_START(arm_writemem)
{0x00000000,0x000FFFFF,MWA32_RAM,&de3as_page0_ram},		//Boot RAM
{0x00300000,0x003FFFFF,MWA32_RAM,&de3as_reset_ram},		//Swapped RAM
{0x00400000,0x005FFFFF,MWA32_RAM},						//Mirrored BIOS @ Boot Time
{0x20400000,0x20400003,xilinx_w},						//Xilinx Sound Output
{0x20400004,0x207FFFFF,MWA32_ROM},						//U17-U37 ROMS (4MB FOR ALL)
{0x40000000,0x4000FFFF,MWA32_ROM,&u7_base},				//U7 ROM (64K)
MEMORY_END

/******************************/
/*  Port map for Sound CPU    */
/******************************/
//AT91 has only 1 port address it writes to - all 32 ports are send via each bit of a 32 bit double word.
//However, if I didn't use 0-0xFF as a range it crashed for some reason.
static PORT_READ32_START( arm_readport )
	{ 0x00,0xFF, arm_port_r },
PORT_END
static PORT_WRITE32_START( arm_writeport )
	{ 0x00,0xFF, arm_port_w },
PORT_END

/*************************************************/
/* AT91 Sound Generation #3 - Machine Definition */
/*************************************************/
MACHINE_DRIVER_START(de3as)
  MDRV_CPU_ADD(AT91, ARMCPU_FREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(arm_readmem, arm_writemem)
  MDRV_CPU_PORTS(arm_readport, arm_writeport)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM,  at91CustIntf)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END








//No longer supported, code needs to be rewritten to do memory handlers properly now that support removed from AT91 core
#if 0

// Test Driver for AT91 CPU
#ifdef TEST_NEW_SOUND

static MACHINE_INIT(lotrsnd) {
	de2slocals.brdData.cpuNo = 0;
	setup_at91();
    #if REMOVE_LED_CODE
    	remove_led_code();
    #endif
}

static core_tGameData lotrsndGameData = {0, 0};
static void init_lotrsnd(void) {
  core_gameData = &lotrsndGameData;
}

MACHINE_DRIVER_START(lotrsnd)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(lotrsnd, NULL, NULL)
  MDRV_CPU_ADD(AT91, ARMCPU_FREQ)
  MDRV_CPU_MEMORY(arm_readmem, arm_writemem)
  MDRV_SOUND_ADD(CUSTOM,  at91CustIntf)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

INPUT_PORTS_START(lotrsnd)
INPUT_PORTS_END

ROM_START(lotrsnd) \
    ROM_REGION32_LE(0x600000, REGION_CPU1, ROMREGION_ERASEMASK) \
    ROM_LOAD("bios.u8", 0x400000, 0x200000, CRC(c049bf99))\
	ROM_RELOAD(0x0,0x100000)\
	ROM_REGION(0x010000, REGION_SOUND1,0) \
    ROM_LOAD("lotr-u7.101",0x0000,0x10000,CRC(ba018c5c) SHA1(67e4b9729f086de5e8d56a6ac29fce1c7082e470)) \
ROM_END

CORE_GAMEDEFNV(lotrsnd, "LOTR Sound CPU Test", 2003, "Stern", lotrsnd, 0)


/** FLASH BIOS SETUP **/
#define FLASHU8_ADDRESS	0x40000
#define FLASHU8_SIZE    0x20000
static int readu8 = 0;
static int read_devcode = 0;
static int read_mancode = 0;

static READ32_HANDLER(flashb_cs_r)
{
	data32_t data = 0;
	if(offset < 0x10000000)
	{
		LOG(("%08x: reading from U7: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	if(offset < 0x20000000)
	{
		if(readu8) {
			offset &= 0xffffff;	//strip off top 8 bits
			data = (data32_t)*(memory_region(REGION_CPU1) + offset + FLASHU8_ADDRESS);
			data |= ((data32_t)*(memory_region(REGION_CPU1) + offset + FLASHU8_ADDRESS + 1)) << 8;
			//LOG(("%08x: reading from U8: %08x = %08x\n",activecpu_get_pc(),offset,data));
		}

		if(read_mancode)
			data = 0xc0;		//AT49BV1614
//			data = 0xc2;		//AT49BV1614T

		//Devcode is read 1st, then Mancode is read next
		if(read_devcode) {
			data = 0x1f;		//Atmel manufacturer code
			read_devcode = 0;
			read_mancode = 1;
		}
	}
	else
	{
		LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	return data & mem_mask;
}

static UINT8 cb_read = 0;
static WRITE32_HANDLER(flashb_cs_w)
{
	data &= mem_mask;
	if(offset < 0x10000000)
	{
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	if(offset < 0x20000000)
	{
		int cb_addr = (offset & 0xffff)>>1;
		//LOG(("%08x: writing to: %08x (%08x) = %08x\n",activecpu_get_pc(),offset,cb_addr,data));

		//Any commands left to read?
		if(cb_read == 0)
		{
			//All commands begin at this address with this data byte!
			if( cb_addr == 0x5555 && data == 0xaa)
			{
				cb_read = 2;	//read 2 more bytes
				readu8 = read_devcode = read_mancode = 0;
			}
			//Store Byte to U8
			if( readu8 )
			{
				offset &= 0xffffff;
				*(memory_region(REGION_CPU1) + offset+FLASHU8_ADDRESS) = (UINT32)data;
				*(memory_region(REGION_CPU1) + offset+FLASHU8_ADDRESS+1) = (UINT32)data>>8;
				LOG(("%08x: Writing Data to U8 %08x = %08x\n",activecpu_get_pc(),offset,data));
			}
		}
		else
		{
			cb_read--;
			//Capture the 3rd command byte - since it identifies the command to perform!
			if(cb_read == 0)
			{
				switch(data)
				{
					//Erase Chip
					case 0x80:
						break;
					case 0x10:
						{
						UINT8  *u8 = (memory_region(REGION_CPU1) + FLASHU8_ADDRESS);
						memset(u8, 0xff, FLASHU8_SIZE);
						read_devcode = read_mancode = 0;
						readu8 = 1;
						break;
						}
					//Program Chip
					case 0xA0:
						readu8 = 1;
						read_devcode = read_mancode = 0;
						break;
					//Software ID Start
					case 0x90:
						read_devcode = 1;
						readu8 = read_mancode = 0;
						break;
					//Software ID End
					case 0xF0:
						readu8 = read_devcode = read_mancode = 0;
						break;
					default:
						LOG(("%08x: Unknown Command Code! %08x = %08x\n",activecpu_get_pc(),offset,data));
				}
			}
		}
	}
	else
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
}

static MACHINE_INIT(seflashb) {
	de2slocals.brdData.cpuNo = 0;
	//because the boot rom code gets written to ram, and then remapped to page 0, we need an interface to handle this.
	at91_set_ram_pointers(de3as_reset_ram,de3as_page0_ram);
    //this crap is needed because for some reason installing memory handlers fails to work properly
    at91_cs_callback_r(0x00400000,0x8fffffff,flashb_cs_r);
    at91_cs_callback_w(0x00400000,0x8fffffff,flashb_cs_w);
	//Patch out the LED flashing routine
#if 0
	//Sound OS Version 5
	de3as_page0_ram[0x1170/4] = (UINT32)0xE12FFF1E;	//BX R14
#else
	//Sound OS Version 8
	de3as_page0_ram[0x11c0/4] = (UINT32)0xE12FFF1E;	//BX R14
#endif
}

static MACHINE_STOP(seflashb) {
	UINT8 data = 0;
	int offset = 0;
	FILE *rom = NULL;

	//Create output file
	rom = fopen("biosv8.u8","wb");
	if(rom == NULL)
	{
		printf("failed to create new bios file!\n");
		return;
	}

	//Copy U8 memory region to file
	for(offset = 0; offset < FLASHU8_SIZE; offset++)
	{
		data = *(memory_region(REGION_CPU1) + offset + FLASHU8_ADDRESS);
		fwrite((void*)&data,1,1,rom);
	}

	//Close it
	if(rom) fclose(rom);
}

static core_tGameData seflashbGameData = {0, 0};
static void init_seflashb(void) {
  core_gameData = &seflashbGameData;
}

MACHINE_DRIVER_START(seflashb)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(seflashb, NULL, seflashb)
  MDRV_CPU_ADD(AT91, ARMCPU_FREQ)
  MDRV_CPU_MEMORY(arm_readmem, arm_writemem)
  MDRV_SOUND_ADD(CUSTOM,  at91CustIntf)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

INPUT_PORTS_START(seflashb)
INPUT_PORTS_END

//Sound OS Version 5
#if 0
ROM_START(seflashb) \
    ROM_REGION32_LE(0x600000, REGION_CPU1, ROMREGION_ERASEMASK) \
    ROM_LOAD("flashv5.bin", 0, 0x10000, CRC(ad93688f) SHA1(b18e077af247bcb139377b3fa877e0c3906cb136))\
ROM_END
#else
//Sound OS Version 8
ROM_START(seflashb) \
    ROM_REGION32_LE(0x600000, REGION_CPU1, ROMREGION_ERASEMASK) \
    ROM_LOAD("flashv8.bin", 0, 0x10000, CRC(53f53672) SHA1(f7211857988f99429ecbd232e5833741dd1693ee))\
ROM_END
#endif

CORE_GAMEDEFNV(seflashb, "Stern Sound OS Flash Update", 2004, "Stern", seflashb, 0)

#endif	//TEST_NEW_SOUND
#endif	//0
