#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "core.h"
#include "gts3.h"
#include "gts3sound.h"

#define GTS3_NMIFREQ	50

static int GTS3_scpuno1;
static void cpu1nmi(int data);
static void u3_w(void);

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    u2;
  int	 oki_enable;
} GTS3locals;



/*Not hooked up according to Schem*/
static WRITE_HANDLER(GTS3_ym2151Port) { logerror("2151 port off=%x data=%x\n",offset,data>>1); }
static void GTS3_ym2151IRQ(int state) { logerror("ym2151 IRQ %x\n",state); }

//Read CPU #1 Sound Latch & Clear the IRQ!
static READ_HANDLER(snddata1_r)
{
	int data = soundlatch_r(0);
	//logerror("latch1_r %x \n",data);
	cpu_set_irq_line(GTS3_scpuno1, M65C02_INT_IRQ, CLEAR_LINE);
	return data;
}
//Read CPU #2 Sound Latch & Clear the IRQ!
static READ_HANDLER(snddata2_r)
{
	int data = soundlatch_r(1);
	//logerror("latch2_r %x \n",data);
	cpu_set_irq_line(GTS3_scpuno1+1, M65C02_INT_IRQ, CLEAR_LINE);
	return data;
}

/* AD7528 - Is a Dual DAC selectable by a control register
   We simulate that here by writing to either DAC_0 or DAC_1
*/
static WRITE_HANDLER(dac7528_w) { 
	if(offset == 0)
		DAC_0_data_w(offset,data); 
	else
		DAC_1_data_w(offset,data);
	//logerror("DAC7528_W %x %x\n",offset,data); 
}

static WRITE_HANDLER(s5_w) {}
static WRITE_HANDLER(cpu2_nmi) { cpu_cause_interrupt(GTS3_scpuno1+1,M65C02_INT_NMI); }

static WRITE_HANDLER(dac6295_w) {
	GTS3locals.u2 = data;
	if(GTS3locals.oki_enable)
		OKIM6295_data_0_w(offset,data);
}

/* G3 - LS377
D0 = Enable CPU #1 NMI - In conjunction with programmable timer circuit
D1 = CPU #1 Diag LED
D2-D4 = NA?
D5 = S4-11 = DCLCK2 = Clock in data to U3
D6 = S4-12 = ~WR = 6295 Write Enabled
D7 = S4-15 = YM2151 - A0
*/
static WRITE_HANDLER(clock1_w)  {
	//logerror("clock1_w %x\n",data);
	UpdateSoundLEDS(0,(data>>1)&1);
	if(data>>5&1) u3_w();
	if(data>>7&1)
		YM2151_data_port_0_w(0,data);
}

/* U3 - LS374
1Q = VUP/DOWN??
2Q = VSTEP??
3Q = 6295 !CS
4Q = ROM Enable
5Q = 6295 - SS (Selects 8Khz or 6.4Khz frequency)
6Q = LED
7Q = SRET1
8Q = SRET2
*/
static void u3_w(void)
{	int data = GTS3locals.u2;
	UpdateSoundLEDS(1,(data>>6)&1);
	GTS3locals.oki_enable = (data>>3)&1;	
}

static void cpu1nmi(int data) { cpu_cause_interrupt(GTS3_scpuno1,M65C02_INT_NMI); }


/*--------------
/  Memory map
/---------------
  CPU #1:
  -------

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

  CPU #2:
  -------

  S2 - LS139
  A14 A15
  -------
  0    0 = (<0x4000) = Y0 = RAM Enable
  1    0 = (0x4000) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x8000) = Y2 = Enable DAC (E2 - AD7528)
  
*/
MEMORY_READ_START(GTS3_sreadmem)
{ 0x0000, 0x1fff, MRA_RAM },
//{ 0x4001, 0x4001, YM2151_status_port_0_r },
{ 0x6800, 0x6800, snddata1_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(GTS3_swritemem)
{ 0x0000, 0x1fff, MWA_RAM },
{ 0x4000, 0x4000, YM2151_register_port_0_w },
//{ 0x4001, 0x4001, YM2151_data_port_0_w },
{ 0x6000, 0x6000, s5_w},
{ 0x7000, 0x7000, cpu2_nmi},
{ 0x7800, 0x7800, dac6295_w},
{ 0x8000, 0x9fff, MWA_ROM},
{ 0xa000, 0xa000, clock1_w},
{ 0xa001,0xffff, MWA_ROM},
MEMORY_END

MEMORY_READ_START(GTS3_sreadmem2)
{ 0x0000, 0x1fff, MRA_RAM },
{ 0x4000, 0x4000, snddata2_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(GTS3_swritemem2)
{ 0x0000, 0x1fff, MWA_RAM },
{ 0x8000, 0x8001, dac7528_w},
{ 0x8002, 0xffff, MWA_ROM},
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface GTS3_dacInt =
  { 2, { 50 }};

struct YM2151interface GTS3_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
  { GTS3_ym2151IRQ },
  { GTS3_ym2151Port}
};
struct OKIM6295interface GTS3_okim6295_interface = {															
	1,						/* 1 chip */
	{ 8000 },				/* 8000Hz frequency */
	{ GTS3_MEMREG_SROM1 },	/* memory region */
	{ 50 }
};

void GTS3_sinit(int num) { 
	GTS3_scpuno1=num; 
	timer_pulse(TIME_IN_HZ(GTS3_NMIFREQ), 0, cpu1nmi);
}
