/* System 3 Sound:

   Far as I know there is only 1 generation.

   Hardware is almost the same as Generation 3 System 80b boards, except for the OKI chip.

   CPU: 2x(6502): DAC: 2x(AD7528): DSP: 1x(YM2151): OTHER: OKI6295 (Speech)
*/   

/*--------------
/  Memory map
/---------------
  CPU #1 (Y-CPU):
  ---------------

  S2 - LS139
  A11 A12
  -------
  0    0 = (0x6000) = Y0 = S5 LS374 Chip Select
  1    0 = (0x6800) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x7000) = Y2 = CPU #2 - Trigger NMI
  1    1 = (0x7800) = Y3 = S4-13 = Latch Data to 6295
  
  T4 - F138
  A13 A14 A15
  -----------
    0   0   0 = Y0 = RAM Enable (0x1fff)
    0   1   0 = Y2 = S4-14 = 2151 Enable  (0x4000)
	1   1   0 = Y3 = S2-LS139 Enable(0x6000)
	1   0   1 = Y5 = Enable G3-LS377 (0xA000)

  CPU #2 (D-CPU):
  ---------------

  S2 - LS139
  A14 A15
  -------
  0    0 = (<0x4000) = Y0 = RAM Enable
  1    0 = (0x4000) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x8000) = Y2 = Enable DAC (E2 - AD7528)
  
*/

#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "core.h"
#include "gts3.h"
#include "gts3sound.h"
#include "snd_cmd.h"

/*Functions used from the System 80b Sound Souce code*/
extern void GTS80BS_init(void);
extern void GTS80BS_exit(void);
extern WRITE_HANDLER( s80bs_sh_w );
extern READ_HANDLER( s80bs_cause_dac_nmi_r );
extern WRITE_HANDLER( s80bs_nmi_rate_w );
extern WRITE_HANDLER( s80bs_dac_vol_w );
extern WRITE_HANDLER( s80bs_dac_data_w );
extern WRITE_HANDLER( s80bs3_sound_control_w );
extern WRITE_HANDLER( s80bs_ym2151_w );

/*Locals*/
static int adpcm=0;
static int enable_cs=0;
static int u3=0;
static int enable_w=0;
static int rom_cs=0;

//#define logerror printf

void GTS3_SoundCommand(int data)
{
	s80bs_sh_w(0,data^0xff);	//Data is inverted from cpu
	snd_cmd_log(data^0xff);
}

//Send data to 6295, but only if Chip Selected and Write Enabled!
static void oki6295_w(void)
{
	if(enable_cs && enable_w) {
		OKIM6295_data_0_w(0,adpcm);
		logerror("OKI Data = %x\n",adpcm);
	}
	else
		logerror("NO OKI: cs=%x w=%x\n",enable_cs,enable_w);
}

static WRITE_HANDLER(adpcm_w)
{
	adpcm = data^0xff;	//Data is inverted from cpu
	logerror("adpcm: %x\n",adpcm);
	if(adpcm&0x80)
		logerror("START OF SAMPLE!\n");
	//Trigger a command to 6295
	oki6295_w();
}

/* G3 - LS377
   ----------
D0 = Enable CPU #1 NMI - In conjunction with programmable timer circuit
D1 = CPU #1 Diag LED
D2-D4 = NA?
D5 = S4-11 = DCLCK2 = Clock in 6295 Data from Latch to U3 Latch
D6 = S4-12 = ~WR = 6295 Write Enabled
D7 = S4-15 = YM2151 - A0

   U3 - LS374
   ----------
D0 = VUP/DOWN??
D1 = VSTEP??
D2 = 6295 Chip Select (Active Low)
D3 = ROM Select (Active Low)
D4 = 6295 - SS (Data = 1 = 8Khz; Data = 0 = 6.4Khz frequency)
D5 = LED
D6 = SRET1
D7 = SRET2
*/
static WRITE_HANDLER(sound_control_w)
{
	int hold_enable_w, hold_enable_cs, hold_rom_cs, hold_u3;
	hold_enable_w = enable_w;
	hold_enable_cs = enable_cs;
	hold_rom_cs = rom_cs;
	hold_u3 = u3;

	//Proc the YM2151 & Other Stuff
	s80bs3_sound_control_w(offset,data);
	UpdateSoundLEDS(0,(data>>1)&1);

	//Is 6295 Enabled for Writing(Active Low)?
	if(data & 0x40) enable_w = 0; else enable_w = 1;
	logerror("Enable W = %x\n",enable_w);

	//Trigger Command on Positive Edge
	if(enable_w && !hold_enable_w) oki6295_w();

	//Handle U3 Latch - On Positive Edge
	u3 = (data & 0x20);
	if(u3 && !hold_u3) {
		UpdateSoundLEDS(1,(adpcm>>5)&1);
		OKIM6295_set_frequency(0,((adpcm>>4)&1)?8000:6400);
		if(adpcm & 0x04) enable_cs = 0; else enable_cs = 1;
		//Select either Arom1 or Arom2 when Rom Select Changes
		rom_cs = (adpcm>>3)&1;
		if(rom_cs != hold_rom_cs)
			OKIM6295_set_bank_base(0, 0x40000-(rom_cs*0x40000));

		logerror("SS = %x\n",(adpcm>>4)&1);
		logerror("ROM CS = %x\n",rom_cs);
		logerror("Enable CS = %x\n",enable_cs);

		//Trigger Command on Positive Edge of 6295 Chip Enabled
		if(enable_cs && !hold_enable_cs) oki6295_w();
	}
}

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS3_sreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_swritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x7800, 0x7800, adpcm_w},
{ 0xa000, 0xa000, sound_control_w },
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS3_sreadmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_swritemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface GTS3_dacInt =
{ 2,
 {25,25}
};

struct YM2151interface GTS3_ym2151Int =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};

struct OKIM6295interface GTS3_okim6295_interface = {															
	1,						/* 1 chip */
	{ 8000 },				/* 8000Hz frequency */
	{ GTS3_MEMREG_SROM1 },	/* memory region */
	{ 100 }
};

void GTS3_sinit(int num) {
	GTS80BS_init();
}

void GTS3_sound_exit(void) {
	GTS80BS_exit();
}
