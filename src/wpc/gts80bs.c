/* System 80b Sound Hardware */

/* This code is based in part on the Gottlieb Video Game code used in MAME */

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "core.h"
#include "gts80.h"
#include "gts80bs.h"

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    ay_latch;			//Data Latch to AY-8913 chips
  int    ym2151_port;		//Bit determines if Registor or Data is written to YM2151
  int    nmi_rate;			//Programmable NMI rate
  void   *nmi_timer;		//Timer for NMI
  UINT8  dac_volume;
  UINT8  dac_data;
} GTS80BS_locals;

// Latch data for AY chips
WRITE_HANDLER(s80bs_ay8910_latch_w) { GTS80BS_locals.ay_latch = data;}

// Init 
void GTS80BS_init(void) { 
	GTS80BS_locals.nmi_timer = NULL; 
	memset(&GTS80BS_locals, 0, sizeof(GTS80BS_locals));
}

// Cleanup
void GTS80BS_exit(void) { 
	if(GTS80BS_locals.nmi_timer)
		timer_remove(GTS80BS_locals.nmi_timer);
	GTS80BS_locals.nmi_timer = NULL;
}


//Setup NMI timer and triggering code: Timed NMI occurs for the Y-CPU. Y-CPU triggers D-CPU NMI
void nmi_generate(int param) { cpu_cause_interrupt(1,M6502_INT_NMI); }
static void nmi_callback(int param) { cpu_cause_interrupt(cpu_gettotalcpu()-1, M6502_INT_NMI); }
WRITE_HANDLER(s80bs_nmi_rate_w) { GTS80BS_locals.nmi_rate = data; }
WRITE_HANDLER(s80bs_cause_dac_nmi_w) { cpu_cause_interrupt(cpu_gettotalcpu()-2, M6502_INT_NMI); }
READ_HANDLER(s80bs_cause_dac_nmi_r) { s80bs_cause_dac_nmi_w(offset, 0); return 0; }

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(s80bs_sh_w)
{
	data &= 0x3f;			//Not sure if this is needed for ALL generations.
	if ((data&0x0f) != 0xf) /* interrupt trigered by four low bits (not all 1's) */
	{
		soundlatch_w(offset,data);
		cpu_cause_interrupt(cpu_gettotalcpu()-1,M6502_INT_IRQ);
		cpu_cause_interrupt(cpu_gettotalcpu()-2,M6502_INT_IRQ);
	}
}

//Generation 1 Specific
READ_HANDLER(s80bs1_sound_input_r)
{
	/* bits 0-3 are probably unused (future expansion) */
	/* bits 4 & 5 are two dip switches. Unused? */
	/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */
	/* bit 7 comes from the speech chip DATA REQUEST pin */
	return 0xc0;
}

//Common to All Generations - Set NMI Timer parameters
static WRITE_HANDLER( common_sound_control_w )
{
	/* Bit 0 enables and starts NMI timer */
	if (GTS80BS_locals.nmi_timer)
	{
		timer_remove(GTS80BS_locals.nmi_timer);
		GTS80BS_locals.nmi_timer = 0;
	}

	if (data & 0x01)
	{
		/* base clock is 250kHz divided by 256 */
		double interval = TIME_IN_HZ(250000.0/256/(256-GTS80BS_locals.nmi_rate));
		GTS80BS_locals.nmi_timer = timer_pulse(interval, 0, nmi_callback);
	}

	/* Bit 1 controls a LED on the sound board */
}

//Generation 1 sound control
WRITE_HANDLER( s80bs1_sound_control_w )
{
	static int last;

	common_sound_control_w(offset, data);

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_0_w(0,GTS80BS_locals.ay_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_1_w(0,GTS80BS_locals.ay_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
	}

	/* bit 7 goes to the speech chip RESET pin */

	last = data & 0x44;
}


//Generation 3 sound control
WRITE_HANDLER( s80bs3_sound_control_w )
{
	common_sound_control_w(offset, data);
	/* Bit 7 selects YM2151 register or data port */
	GTS80BS_locals.ym2151_port = data & 0x80;
}

//Determine whether to write data to YM2151 Registers or Data Port
WRITE_HANDLER( s80bs_ym2151_w )
{
	if (GTS80BS_locals.ym2151_port)
		YM2151_data_port_0_w(offset, data);
	else
		YM2151_register_port_0_w(offset, data);
}

//DAC Handling.. Set volume
WRITE_HANDLER( s80bs_dac_vol_w )
{
	GTS80BS_locals.dac_volume = data ^ 0xff;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
}
//DAC Handling.. Set data to send
WRITE_HANDLER( s80bs_dac_data_w )
{
	GTS80BS_locals.dac_data = data;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
}

//Process command from Main CPU
void GTS80BS_sound_latch(int data)
{
//	logerror("Sound Command %x\n",data);
	s80bs_sh_w(0,data^0xff);	/*Data is inverted from main cpu*/
}

/***************************/
/* GENERATION 1 MEMORY MAP */
/***************************/
//Y-CPU
MEMORY_READ_START( GTS80BS1_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x6000, s80bs1_sound_input_r },
	{ 0xa800, 0xa800, soundlatch_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, MWA_NOP },	/* speech chip. The game sends strings */
									/* of 15 bytes (clocked by 4000). The chip also */
									/* checks a DATA REQUEST bit in 6000. */
	{ 0x4000, 0x4000, s80bs1_sound_control_w },
	{ 0x8000, 0x8000, s80bs_ay8910_latch_w },
	{ 0xa000, 0xa000, s80bs_nmi_rate_w },	   /* set Y-CPU NMI rate */
	{ 0xb000, 0xb000, s80bs_cause_dac_nmi_w }, /*Trigger D-CPU NMI*/
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END
//D-CPU
MEMORY_READ_START( GTS80BS1_readmem2 )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem2 )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x4001, DAC_0_data_w },	/*Not sure if this shouldn't use s80bs_dac_vol_w & s80bs_dac_data_w*/
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END
/***************************/
/* GENERATION 2 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x2000, 0x2000, MWA_NOP },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x8000, 0x8000, s80bs_ay8910_latch_w},
{ 0xa000, 0xa000, s80bs1_sound_control_w},
{ 0xa001,0xffff, MWA_ROM},
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

/***************************/
/* GENERATION 3 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_readmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0xa000, 0xa000, s80bs3_sound_control_w },
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_readmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_writemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface GTS80BS_dacInt =
{ 
  2,			/*2 Chips - but it seems we only access 1?*/
 {25,25}		/* Volume */
};

struct AY8910interface GTS80BS_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 25, 25 }, /* Volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

struct YM2151interface GTS80BS_ym2151Int =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};
