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

#define MAKE_WAVS 0

#if MAKE_WAVS
#include "sound/wavwrite.h"
#endif

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

/* Older 11 Voice Style BSMT Chip */
//NOTE: Do not put a volume adjustment here, otherwise 128x16 games have audible junk played at the beggining
static struct BSMT2000interface de2s_bsmt2000aInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, {0000}, 1, 0, 0
};
/* Newer 12 Voice Style BSMT Chip */
static struct BSMT2000interface de2s_bsmt2000bInt = {
  1, {24000000}, {12}, {DE2S_ROMREGION}, {100}, {2000}, 1, 0, 0
};
/* Older 11 Voice Style BSMT Chip but needs large volume adjustment */
static struct BSMT2000interface de2s_bsmt2000cInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, {4000}, 1, 0, 0
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

MACHINE_DRIVER_START(de2bs)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000bInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2cs)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000cInt)
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
static WRITE_HANDLER(de2s_bsmtreset_w) { /* Writing 0x80 here resets BSMT ?*/ }

static INTERRUPT_GEN(de2s_firq) {
  //NOTE: Odd that it will NOT WORK without HOLD_LINE - although we don't clear it anywaywhere!
  cpu_set_irq_line(de2slocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}


/************************************************/
/* GENERATION 3 Sound Hardware - AT91 Based CPU */
/************************************************/
/* by Steve Ellenoff ( 10/11/2004 - 10/28/2004 )

   Uses an Atmel AT9140008 CPU (ARM7TDMI Core) and Xilinx FPGA for sound.
   Board is 100% compatible with previous generation hardware (all games using 8Mb roms)
   by simply replacing the older game roms into the board.
   The BSMT chip is software emulated by the AT91 CPU and has added new capabilities such as
   16 bit sample support and ADPCM compression. The Xilinx FPGA is programmed to handle a variety of
   tasks. It controls 2 of the ROM Enable lines, acts as a sound command buffer from the main cpu, 
   and most importantly, acts as a simple DSP chip which converts the 16 bit sample data into a serial sound
   stream for output to a DAC. It may have other functionality as well, but not related to sound if so.

   A # of MAME related techincal challenges occurred which required some work arounds.
   First, I wanted to use the memory map to handle everything, but for some reason when I 
   tried it with the 32 bit sized address like 0x40000000, it took about 30 seconds to load and
   consumed massive amounts of memory. Second I tried installing memory handlers in the init code
   to handle it, but for some reason they never returned an offset value > 0. So finally I was stuck
   to create call backs right from the core, but while speed is fine, it's a yucky hack.
   Second, because the AT91 core swaps RAM into page 0 memory after a 
   certain register write occurs, and the boot code is also swapped, I needed to provide the
   AT91 code pointers to the memory region data so it could perform the swap. 

   The bios code will setup a simple overflow counter to trigger an IRQ at 1/2 the CPU freq. Ideally this should
   be handled by the CPU core itself, but for now, I'm doing it this way because it's much quicker to implement.
*/

#define USE_ACTUAL_FREQ 1
#define REMOVE_LED_CODE 1

void *		wavraw;					/* raw waveform */

#if USE_ACTUAL_FREQ
	#define ARMCPU_FREQ	40000000				//40 MHZ
	#define ARMIRQ_FREQ ARMCPU_FREQ/2/0x339		//Works out to be 24,242Hz
    #define WAVE_OUT_RATE ARMIRQ_FREQ			//Output rate is exactly the IRQ frequency
#else
//to speed up testing
	#define ARMCPU_FREQ	4000000		//4 MHZ
	#define ARMIRQ_FREQ ARMCPU_FREQ/2/0x339
	#define WAVE_OUT_RATE 40000000/2/0x339		//Accurate sample rate
#endif

#define ARMSNDBUFSIZE 400
#define BUFFSIZE 0x100000
static INT16 samplebuf[BUFFSIZE];

static READ_HANDLER(scmd_r);

static data32_t *de3as_reset_ram;
static data32_t *de3as_page0_ram;
static int arm_ready_irq = 0;
static int sndcmdbuf[ARMSNDBUFSIZE];
static int sbuf=0;
static int spos=0;

static int sampout = 0;
static int sampnum = 0;


static INTERRUPT_GEN(arm_irq)
{
	if(arm_ready_irq)
	{
		cpu_set_irq_line(de2slocals.brdData.cpuNo, AT91_IRQ_LINE, PULSE_LINE);
	}
}

static WRITE32_HANDLER(arm_irq_ready)
{
	arm_ready_irq = data;
}

static const int rommap[4] = {4,2,3,1};

#define READWRITE_METHOD 3

#if READWRITE_METHOD == 1
//This approach uses if/else/if and is not optimized but here for comparisons to the other ways
static READ32_HANDLER(arm_cs_r)
{
	data32_t data = 0;

	//CSR 0 Mapped to 0x10000000 - BIOS ROM U8
	if(offset < 0x1fffffff)
	{
		LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
	if(offset < 0x2fffffff)
	{
		//Xilinx Provides Sound Command from Main CPU
		if(offset == 0x20000000)
		{
			//static int lastcmd = 0;
			//data = soundlatch_r(0);
			data = scmd_r(0);
			#if 0
			if(lastcmd != data) {
				LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
				lastcmd = data;
			}
			#endif
		}
		else
		//Read from U17-U37 ROMS
		{
			//remove a29 & a22
			int romaddr = offset & 0xDFBFFFFF;
			//determine which chip (combine A21 & A0 into 2 bit #)
			int romchip = rommap[(((romaddr& 0x200000)>>20)|(romaddr&1))];
			//remove a21 and >>1 the address
			romaddr = (romaddr&0xFFDFFFFF)>>1;

			data = (data8_t)*((memory_region(REGION_SOUND2) + romaddr + ((romchip-1) * 0x100000)));
			LOG(("%08x: reading from U%d: %08x = %08x (%08x)\n",activecpu_get_pc(),romchip,romaddr,data,offset));	
		}
	}
	else
	//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
	if(offset < 0x3fffffff)
	{
		LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	//CSR 1 Mapped to 0x40000000 - U7 ROM
	if(offset < 0x4fffffff)
	{
		offset &= 0xffffff;	//strip off top 8 bits
		data = (data32_t)*(memory_region(REGION_SOUND1) + offset);
		LOG(("%08x: reading from u7: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	{
		LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	return data & mem_mask;
}

static WRITE32_HANDLER(arm_cs_w)
{
	data &= mem_mask;
	//CSR 0 Mapped to 0x10000000 - BIOS ROM U8
	if(offset < 0x1fffffff)
	{
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
	if(offset < 0x2fffffff)
	{
		//static data32_t last = 0;

		//Data Stream Output
		if( (offset == 0x20400000) || (offset == 0x20400002) )
		{
			//Store 16 bit data to our buffer
			samplebuf[sampnum] = data & 0xffff;
			sampnum = (sampnum + 1) % BUFFSIZE;

			//Dump to Wave File
			#if MAKE_WAVS
			if(wavraw)
			{
				INT16 d;
				d = (INT16)data;
				wav_add_data_16(wavraw, &d, 1);
			}
			#endif

			#if 0
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
			#endif
		}
	}
	else
	//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
	if(offset < 0x3fffffff)
	{
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	//CSR 1 Mapped to 0x40000000 - U7 ROM
	if(offset < 0x4fffffff)
	{
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));		
	}
	else
	{
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
}
#endif

#if READWRITE_METHOD == 2
//This approach attempts to use switch for optimization
static READ32_HANDLER(arm_cs_r)
{
	data32_t data = 0;

	switch( (offset & 0xF0000000) >> 28 )
	{
		case 0:
		//CSR 0 Mapped to 0x10000000 - BIOS ROM U8
		case 1:
			LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)	
		case 2:
			//Xilinx Provides Sound Command from Main CPU
			if(offset == 0x20000000)
			{
				//static int lastcmd = 0;
				//data = soundlatch_r(0);
				data = scmd_r(0);
				#if 0
				if(lastcmd != data) {
					LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
					lastcmd = data;
				}
				#endif
			}
			else
			//Read from U17-U37 ROMS
			{
				//remove a29 & a22
				int romaddr = offset & 0xDFBFFFFF;
				//determine which chip (combine A21 & A0 into 2 bit #)
				int romchip = rommap[(((romaddr& 0x200000)>>20)|(romaddr&1))];
				//remove a21 and >>1 the address
				romaddr = (romaddr&0xFFDFFFFF)>>1;

				data = (data8_t)*((memory_region(REGION_SOUND2) + romaddr + ((romchip-1) * 0x100000)));
				LOG(("%08x: reading from U%d: %08x = %08x (%08x)\n",activecpu_get_pc(),romchip,romaddr,data,offset));	
			}
			break;
		//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
		case 3:
			LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		//CSR 1 Mapped to 0x40000000 - U7 ROM
		case 4:
			offset &= 0xffffff;	//strip off top 8 bits
			data = (data32_t)*(memory_region(REGION_SOUND1) + offset);
			LOG(("%08x: reading from u7: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		default:
			LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	return data & mem_mask;
}


static WRITE32_HANDLER(arm_cs_w)
{
	data &= mem_mask;
	
	switch( (offset & 0xF0000000) >> 28 )
	{
		case 0:
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		//CSR 0 Mapped to 0x10000000 - BIOS ROM U8
		case 1:
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)
		case 2:
			//Data Stream Output
			//if( (offset == 0x20400000) || (offset == 0x20400002) )
			{
				//Store 16 bit data to our buffer
				samplebuf[sampnum] = data & 0xffff;
				sampnum = (sampnum + 1) % BUFFSIZE;

				//Dump to Wave File
				#if MAKE_WAVS
				if(wavraw)
				{
					INT16 d;
					d = (INT16)data;
					wav_add_data_16(wavraw, &d, 1);
				}
				#endif

				#if 0
				LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
				#endif
			}
			break;
		//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
		case 3:
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
			break;
		//CSR 1 Mapped to 0x40000000 - U7 ROM
		case 4:
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));		
			break;
		default:
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
}
#endif

#if READWRITE_METHOD == 3
//This approach attempts to use optimized if/else structure
static READ32_HANDLER(arm_cs_r)
{
	data32_t data = 0;
	data32_t offcheck = (offset & 0xF0000000);

	//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)	
	if(offcheck == 0x20000000)
	{
		//Xilinx Provides Sound Command from Main CPU
		if(offset == 0x20000000)
		{
			//static int lastcmd = 0;
			//data = soundlatch_r(0);
			data = scmd_r(0);
			#if 0
			if(lastcmd != data) {
				LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
				lastcmd = data;
			}
			#endif
		}
		else
		//Read from U17-U37 ROMS
		{
			//remove a29 & a22
			int romaddr = offset & 0xDFBFFFFF;
			//determine which chip (combine A21 & A0 into 2 bit #)
			int romchip = rommap[(((romaddr & 0x200000)>>20)|(romaddr&1))];
			//remove a21 and >>1 the address
			romaddr = (romaddr&0xFFDFFFFF)>>1;

			data = (data8_t)*((memory_region(REGION_SOUND2) + romaddr + ((romchip-1) * 0x100000)));
			LOG(("%08x: reading from U%d: %08x = %08x (%08x)\n",activecpu_get_pc(),romchip,romaddr,data,offset));	
		}
	}
	else
	//CSR 1 Mapped to 0x40000000 - U7 ROM
	if(offcheck == 0x40000000)
	{
		offset &= 0xffffff;	//strip off top 8 bits
		data = (data32_t)*(memory_region(REGION_SOUND1) + offset);
		LOG(("%08x: reading from u7: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	else
	{
		//CSR 0 Mapped to 0x10000000 - BIOS ROM U8 
		//OR
		//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
		//OR 
		//Whatever else..
		LOG(("%08x: reading from: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
	return data & mem_mask;
}

static WRITE32_HANDLER(arm_cs_w)
{
	data32_t offcheck = (offset & 0xF0000000);
	data &= mem_mask;

	//CSR 2 Mapped to 0x20000000 (Xilinx & U17-U37 ROMS)	
	if(offcheck == 0x20000000)	
	{
		//Data Stream Output
		//if( (offset == 0x20400000) || (offset == 0x20400002) )
		{
			//Store 16 bit data to our buffer
			samplebuf[sampnum] = data & 0xffff;
			sampnum = (sampnum + 1) % BUFFSIZE;

			//Dump to Wave File
			#if MAKE_WAVS
			if(wavraw)
			{
				INT16 d;
				d = (INT16)data;
				wav_add_data_16(wavraw, &d, 1);
			}
			#endif

			#if 0
			LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
			#endif
		}
	}
	else
	{	//CSR 0 Mapped to 0x10000000 - BIOS ROM U8 
		//OR
		//CSR 3 Mapped to 0x30000000 - U412 (Not Used)
		//OR 
		//CSR 1 Mapped to 0x40000000 - U7 ROM
		//Whatever else..
		LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
}
#endif

//Remove Delay from LED Flashing code to speed up the boot time of the cpu
static void remove_led_code(void)
{
  de3as_page0_ram[0xa854/4] = (UINT32)0;
  de3as_page0_ram[0xa860/4] = (UINT32)0;
  de3as_page0_ram[0xa86c/4] = (UINT32)0;
  de3as_page0_ram[0xa878/4] = (UINT32)0;
  de3as_page0_ram[0xa884/4] = (UINT32)0;
  de3as_page0_ram[0xa890/4] = (UINT32)0;
  de3as_page0_ram[0xa89c/4] = (UINT32)0;
  de3as_page0_ram[0xa8a8/4] = (UINT32)0;
}

static void setup_at91(void)
{
  //this crap is needed because for some reason installing memory handlers fails to work properly
  at91_cs_callback_r(0x00400000,0x8fffffff,arm_cs_r);
  at91_cs_callback_w(0x00400000,0x8fffffff,arm_cs_w);

  //because the boot rom code gets written to ram, and then remapped to page 0, we need an interface to handle this.
  at91_set_ram_pointers(de3as_reset_ram,de3as_page0_ram);

  //crappy hack to know when IRQ can be fired
  at91_ready_irq_callback_w(arm_irq_ready);
}

static void de3s_init(struct sndbrdData *brdData) {
  memset(&de2slocals, 0, sizeof(de2slocals));
  de2slocals.brdData = *brdData;
  setup_at91();

  #if MAKE_WAVS
	wavraw = wav_open("raw.wav", WAVE_OUT_RATE, 2);
  #endif

  #if REMOVE_LED_CODE
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
	//if(sbuf>0)
	//printf("sbuf[%d]=%x\n",sbuf,sndcmdbuf[sbuf]);
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
		#ifdef MAME_DEBUG
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

//Memory Map for Main CPU
static MEMORY_READ32_START(arm_readmem)
{0x00000000,0x000FFFFF,MRA32_RAM},
{0x00300000,0x003FFFFF,MRA32_RAM},
MEMORY_END

static MEMORY_WRITE32_START(arm_writemem)
{0x00000000,0x000FFFFF,MWA32_RAM,&de3as_page0_ram},
{0x00300000,0x003FFFFF,MWA32_RAM,&de3as_reset_ram},
MEMORY_END

MACHINE_DRIVER_START(de3as)
  MDRV_CPU_ADD(AT91, ARMCPU_FREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(arm_readmem, arm_writemem)
  MDRV_CPU_PERIODIC_INT(arm_irq, ARMIRQ_FREQ)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM,  at91CustIntf)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END


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
  MDRV_CPU_PERIODIC_INT(arm_irq, ARMIRQ_FREQ)
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

CORE_GAMEDEFNV(lotrsnd, "LOTR Sound CPU Test", 1987, "Stern", lotrsnd, 0)

#endif	//TEST_NEW_SOUND
