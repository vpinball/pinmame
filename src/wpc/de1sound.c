#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"
#include "core.h"
#include "de.h"
#include "de1sound.h"
#include "machine/6821pia.h"

static int sound_nmi_enable = 0;
static int msm_data = 0;
static int msm_play_lo_nibble = 0;
static int msm_srselect = 0;

static int de_sndCPUNo;

static void DE_ym2151IRQ(int state) {
  cpu_set_irq_line(de_sndCPUNo, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
  //logerror("ym2151 IRQ %x\n",state);
}

//Read Sound Latch - clear the FIRQ!
static READ_HANDLER(snd_r)
{
	int data = 0;
	data=soundlatch_r(0);
	//logerror("latch_r %x \n",data);
	cpu_set_irq_line(de_sndCPUNo, M6809_FIRQ_LINE, CLEAR_LINE);
	return data;
}

//Stereo logic
static WRITE_HANDLER(to4052_w)
{
 //logerror("to4052 %x\n",data);
}


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
static WRITE_HANDLER(chipselect_w)
{
	int addr = (((data>>0)&0x01)*0x4000) + 
			   (((data>>1)&0x01)*0x8000) + 
			   (((data>>3)&0x01)*0x10000) + 
			   (((data>>2)&0x01)*0x20000);

	/*Sample Rate Select is set by bit 4&5*/
	int srselect = (((data>>5)&1)<<1)|((data>>4)&1);

//	logerror("chipselect_w %x\n",data);
//	logerror("D4=%x, D5=%x, srselect = %x\n",((data>>4)&1),((data>>5)&1),srselect);
//	logerror("switchbank: addr=%x\n",addr);

	if(de_sndCPUNo==1)
	{
		cpu_setbank(1, memory_region(DE_MEMREG_SROM1)+addr);
	}
	else
	{
		cpu_setbank(1, memory_region(DE_MEMREG_SDROM1)+addr);
	}

	/*Change the MSM5205 sample rate, if we need to*/
	if(srselect!=msm_srselect)
	{
		switch(srselect){
		case 0:
			msm_srselect = 4;	//4kHz
			break;
		case 1:
			msm_srselect = 5;	//8kHz
			break;
		case 2: 
			msm_srselect = 6;	//6kHz
			break;
		default:
			//logerror("Illegal value of srselect!\n");
			break;
		}
		//logerror("setting to mode %d\n",msm_srselect);
		MSM5205_playmode_w(0,msm_srselect);
		msm_srselect = srselect;
	}
	MSM5205_reset_w(0,(data>>6)&1); /* bit 6*/
	sound_nmi_enable = !((data>>7)&0x01);
}

static WRITE_HANDLER(toMSM5025_w)
{
	//Fetch the High Nibble First!
	msm_data = data;
	msm_play_lo_nibble = 0;
}

extern READ_HANDLER(watchdog_reset_r);

static READ_HANDLER(unknown_r)
{
// logerror("%x: 0x3800_r\n",cpu_get_pc());
	watchdog_reset_r(offset);
 return 0;
}

extern WRITE_HANDLER(watchdog_reset_w);

static WRITE_HANDLER(toreset_w)
{
	watchdog_reset_w(offset,data);
// logerror("toreset_w? %x\n",data);
}

/*MSM5205 interrupt callback*/
static void msm_int( int data ) {
	static int counter = 0;
	//Write either low or high nibble
	if ( msm_play_lo_nibble )
		MSM5205_data_w( 0, msm_data & 0x0f );
	else
		MSM5205_data_w( 0, ( msm_data >> 4 ) & 0x0f );

	msm_play_lo_nibble ^= 1;
	//Are we done fetching both nibbles? Generate an NMI for more data!
	//Unless the nmi_enable has not been set!
	if ( !( counter ^= 1 ) ) {
		if ( sound_nmi_enable ) {
			cpu_cause_interrupt(de_sndCPUNo,M6809_INT_NMI);
		}
	}
}

/*Send CT2 data to Main CPU's PIA*/
static WRITE_HANDLER(DE_ym2151Port)
{
	pia_set_input_cb1(4, data>>1);	//CT2 is bit 7 - CT1 is not hooked up
	//logerror("2151 port off=%x data=%x\n",offset,data>>1);
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(DE_sreadmem)
{ 0x0000, 0x1fff, MRA_RAM },
{ 0x2001, 0x2001, YM2151_status_port_0_r },
{ 0x2400, 0x2400, snd_r},
{ 0x3800, 0x3800, unknown_r},
{ 0x4000, 0x7fff, MRA_BANK1},	/*Voice Samples*/
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(DE_swritemem)
{ 0x0000, 0x1fff, MWA_RAM },
{ 0x2000, 0x2000, YM2151_register_port_0_w },
{ 0x2001, 0x2001, YM2151_data_port_0_w },
{ 0x2800, 0x2800, chipselect_w},
{ 0x2c00, 0x2c00, to4052_w},
{ 0x3000, 0x3000, toMSM5025_w},
{ 0x3800, 0x3800, toreset_w},
{ 0x8000, 0xffff, MWA_ROM},
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/
struct MSM5205interface DE_msm5205Int = {
	1,					/* 1 chip             */
	384000,				/* 384KHz             */
	{ msm_int },		/* interrupt function */
	{ MSM5205_S48_4B },	/* 1 / 96 = 8KHz playback - DOESN'T MATTER IT'S CHANGED BY THE GAME ROM*/
	{ 60 }
};

struct YM2151interface DE_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
  { DE_ym2151IRQ },
  { DE_ym2151Port}
};

#if 0
struct YM2151interface DE_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { DE_ym2151IRQ },
  { DE_ym2151Port}
};
#endif

void DE_sinit(int num) {
	de_sndCPUNo = num;
	/*Start bank reading 0x0000-0x3fff from 6f*/
	if(de_sndCPUNo==1){
		cpu_setbank(1, memory_region(DE_MEMREG_SROM1));
	}
	else{
		cpu_setbank(1, memory_region(DE_MEMREG_SDROM1));
	}
	/*Arm the watchdog*/
	watchdog_reset_w(0,0);
	/*Start off the MSM5205 at 4khz sampling*/
	MSM5205_playmode_w(0,4);
	/*Reset the vars..*/
	sound_nmi_enable = 0;
	msm_data = 0;
	msm_play_lo_nibble = 0;
	msm_srselect = 0;
}
