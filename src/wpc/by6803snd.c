#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "sound/2151intf.h"
#include "sound/hc55516.h"
#include "core.h"
#include "by6803.h"
#include "snd_cmd.h"
#include "by6803snd.h"
#include "snd_cmd.h"
#include "by35snd.h"

extern void by35_soundInit(void);
extern void snt_cmd(int data);
extern void sp45_cmd(int data);

/*M68000 Interrupt Notes:
ipl2 ipl1 ipl0
--------------
1     1     1  = None
1     1     0  = IRQ_1
1     0     1  = IRQ_2
1     0     0  = IRQ_3
0     1     1  = IRQ_4
0     1     0  = IRQ_5
0     0     1  = IRQ_6
0     0     0  = IRQ_7 (NMI)
----------------------------

Sounds Deluxe: 
	IPL0 = IPL1 = Test Switch = IRQ_3
	IPL2 = PIA_IRQ = IRQ_4
*/

/*Main MPU Board uses 2 PIA Chips (0 & 1) - so we must start with 2.
  ------------------------------------------------------------------
  Generation 1: Squalk & Talk:		Uses 2 PIA - It will be #2 & #3
  Generation 2: Turbo Cheap Squalk: Uses 1 PIA - It will be #4
  Generation 3: Sounds Deluxe:		Uses 1 PIA - It will be #5
  Generation 4: Williams Sys 11C:	Uses 1 PIA - It will be #6

  Both Turbo Cheap Squalk & Sounds Deluxe use an AD7533 10-Bit Multiplying DAC
*/

static struct {
  int snddata;
  int cmdnum;
  UINT16 dacdata;
} locals;


/* Generation 2 */
// Sound data is sent as 2 nibbles, first low nibble, then high nibble
static READ_HANDLER(pia_s2_b_r) { 
	if((++locals.cmdnum)==1) {
		//printf("cmd = %x: return = %x\n",locals.cmdnum,locals.snddata &0x0f);
		return locals.snddata & 0x0f;
	}
	else {
		//printf("cmd = %x: return = %x\n",locals.cmdnum,(locals.snddata &0xf0)>>4);
		locals.cmdnum = 0;
		return (locals.snddata & 0xf0)>>4;
	}
} 

static WRITE_HANDLER(pia_s2_a_w) { 
	locals.dacdata = (locals.dacdata & ~0x3fc) | (data << 2);
	DAC_signed_data_16_w(0, locals.dacdata << 6);
}
static WRITE_HANDLER(pia_s2_b_w) { 
	locals.dacdata = (locals.dacdata & ~0x003) | (data >> 6);
	DAC_signed_data_16_w(0, locals.dacdata << 6);
}
static WRITE_HANDLER(pia_s2_cb2_w) { BY6803_UpdateSoundLED(data); }
static void pia_s2_irq (int state) {
  //if(state) printf("Triggering Sound IRQ\n"); else printf("Clearing Sound IRQ\n");
  locals.cmdnum = 0;
  cpu_set_irq_line(BY6803_SCPU1NO, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/* Generation 3 */
static READ_HANDLER(pia_s3_b_r) { return pia_s2_b_r(offset); } 
static WRITE_HANDLER(pia_s3_a_w) { pia_s2_a_w(offset,data); } 
static WRITE_HANDLER(pia_s3_b_w) { pia_s2_b_w(offset,data); } 
static WRITE_HANDLER(pia_s3_cb2_w) { BY6803_UpdateSoundLED(data); }
static void pia_s3_irq (int state) {
	//if(state) printf("Triggering Sound IRQ\n"); else printf("Clearing Sound IRQ\n");
    locals.cmdnum = 0;
	//M68000 DMD - Trigger IPL2 (Level 4) Interrupt!
	cpu_set_irq_line(BY6803_SCPU1NO, MC68000_IRQ_4, state ? ASSERT_LINE : CLEAR_LINE);
}

/* Generation 4 */
static READ_HANDLER(pia_s4_b_r) { logerror("reading sound data = %x\n",locals.snddata); return locals.snddata; }
static WRITE_HANDLER(pia_s4_a_w) { logerror("writing to DAC: %x\n",data); DAC_0_data_w(0,data); }
static WRITE_HANDLER(pia_s4_b_w) { logerror("S4_PB_W: Unknown: %x\n",data); }
static WRITE_HANDLER(pia_s4_ca2_w) { if (!data) YM2151_sh_reset(); }
static WRITE_HANDLER(pia_s4_cb2_w) { /*pia_set_input_cb1(5,data);*/ }
static WRITE_HANDLER(s4_rombank_w) {
#ifdef MAME_DEBUG
  /* this register can not be read but this makes debugging easier */
  *(memory_region(BY6803_MEMREG_SCPU) + 0x2000) = data;
#endif /* MAME_DEBUG */
  cpu_setbank(1, memory_region(BY6803_MEMREG_SROM) +
              0x10000*(data & 0x03) + 0x8000*((data & 0x04)>>2));
}
static void s4_ym2151IRQ(int state) {
  pia_set_input_ca1(6, !state);
}
static void s4_piaIrqA(int state) {
  cpu_set_irq_line(BY6803_SCPU1NO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void s4_piaIrqB(int state) {
  cpu_set_nmi_line(BY6803_SCPU1NO, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
-----------------------------------------
 PIA 0 - COMMON TO BOTH Generation 2 & 3
-----------------------------------------
(in)  PB0-3: Sound Data Input
(in)  CA1:   Sound Interrupt Data 
(in)  CB1:   NA
(in)  CA2:   NA
(in)  CB2:   NA
(out) PA0-7: DAC B8-B1
(out) PB6-7: DAC B10-B9
(out) CA2:   NA
(out) CB2:   LED
	  IRQ:	 Wired to Main 6809 IRQ (Generation 2)
	  IRQ:	 Wired to Main 68000 IPL2 IRQ (Generation 3)
*/
static struct pia6821_interface pia_s2 = {
  /*i: A/B,CA/B1,CA/B2 */ 0, pia_s2_b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia_s2_a_w, pia_s2_b_w, 0, pia_s2_cb2_w,
  /*irq: A/B           */ pia_s2_irq, pia_s2_irq
};
static struct pia6821_interface pia_s3 = {
  /*i: A/B,CA/B1,CA/B2 */ 0, pia_s3_b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia_s3_a_w, pia_s3_b_w, 0, pia_s3_cb2_w,
  /*irq: A/B           */ pia_s3_irq, pia_s3_irq
};

/*
---------------------
 PIA 0 - Generation 4
---------------------
(in)  PB0 - PB7 CPU interface (MDx)
(in)  CA1       YM2151 IRQ
(in)  CB1		CPU interface (MCB2)???
(out) PA0 - PA7 DAC 
(out) CA2       YM 2151 pin 3 (Reset ?)
(out) CB2       CPU interface (MCB1)???
*/
static struct pia6821_interface pia_s4 = {
  /*i: A/B,CA/B1,CA/B2 */ 0, pia_s4_b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia_s4_a_w, pia_s4_b_w, pia_s4_ca2_w, pia_s4_cb2_w,
  /*irq: A/B           */ s4_piaIrqA, s4_piaIrqB
};

//Generation 2: Turbo Cheap Squalk
MEMORY_READ_START(s2_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x6000, 0x6003, pia_4_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(s2_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x6000, 0x6003, pia_4_w },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END
//Generation 2a: Turbo Cheap Squalk (For 64K ROM)
MEMORY_READ_START(s2a_readmem)
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x0800, 0x0803, pia_4_r },
  { 0x0c00, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(s2a_writemem)
  { 0x0000, 0x07ff, MWA_RAM },
  { 0x0800, 0x0803, pia_4_w },
  { 0x0c00, 0xffff, MWA_ROM },
MEMORY_END
//Generation 3: Sounds Deluxe
MEMORY_READ16_START(s3_readmem)
{0x00000000,0x0003ffff, MRA16_ROM},		/*ROM (4 X 64K)*/
{0x00060000,0x00060007, pia_5_msb_r},	/*PIA - CPU D8-15 connected to PIA D0-7*/
{0x00070000,0x0007ffff, MRA16_RAM},		/*RAM*/
MEMORY_END
MEMORY_WRITE16_START(s3_writemem)
{0x00000000,0x0003ffff, MWA16_ROM},		/*ROM (4 X 64K)*/
{0x00060000,0x00060007, pia_5_msb_w},	/*PIA - CPU D8-15 connected to PIA D0-7*/
{0x00070000,0x0007ffff, MWA16_RAM},		/*RAM*/
MEMORY_END
//Generation 4: Williams System 11C
MEMORY_READ_START(s4_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2001, 0x2001, YM2151_status_port_0_r }, /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_6_r },                /* 4000-4fff */
  { 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END
MEMORY_WRITE_START(s4_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, YM2151_register_port_0_w },     /* 2000-2ffe even */
  { 0x2001, 0x2001, YM2151_data_port_0_w },         /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_6_w },                      /* 4000-4fff */
  { 0x6000, 0x6000, hc55516_0_digit_clock_clear_w },/* 6000-67ff */
  { 0x6800, 0x6800, hc55516_0_clock_set_w },        /* 6800-6fff */
  { 0x7800, 0x7800, s4_rombank_w },					/* 7800-7fff */
MEMORY_END


//Sound Init
void by6803_sndinit1(){
	memset(&locals, 0, sizeof(locals));
	core_gameData->gen = GEN_BY35_61;
	by35_soundInit();
}
void by6803_sndinit1a(){
	memset(&locals, 0, sizeof(locals));
}
void by6803_sndinit2(){ 
	memset(&locals, 0, sizeof(locals));
	//Configure PIA to use #4
	pia_config(4, PIA_ALTERNATE_ORDERING, &pia_s2);	//Alt Ordering for 16bit Write Access to Ports A+B
}
void by6803_sndinit3(){ 
	memset(&locals, 0, sizeof(locals));
	//Configure PIA to use #5
	pia_config(5, PIA_ALTERNATE_ORDERING, &pia_s3);	//Alt Ordering for 16bit Write Access to Ports A+B
}
void by6803_sndinit4(){ 
	memset(&locals, 0, sizeof(locals));
	//Configure PIA to use #6
    pia_config(6, PIA_STANDARD_ORDERING, &pia_s4);
    cpu_setbank(1, memory_region(BY6803_MEMREG_SROM));
}

//Sound Exit
void by6803_sndexit1(){ }
void by6803_sndexit2(){ }
void by6803_sndexit3(){ }
void by6803_sndexit4(){ }

//Sound Command
WRITE_HANDLER(by6803_sndcmd1) { 
	logerror("Sound Command %x\n",data);
	locals.snddata = data;
	snt_cmd(data);
}

WRITE_HANDLER(by6803_sndcmd1a) { 
	logerror("Sound Command %x\n",data);
	locals.snddata = data;
	sp45_cmd(data);
}

WRITE_HANDLER(by6803_sndcmd2) { 
	locals.snddata = data;
	//printf("Sound Command %x\n",data);
	snd_cmd_log(data);
	pia_set_input_ca1(4, 0);
	pia_set_input_ca1(4, 1);
}

WRITE_HANDLER(by6803_sndcmd3) { 
	locals.snddata = data;
	//printf("Sound Command %x\n",data);
	snd_cmd_log(data);
	pia_set_input_ca1(5, 0);
	pia_set_input_ca1(5, 1);
}

WRITE_HANDLER(by6803_sndcmd4) { 
	logerror("Sound Command %x\n",data);
	locals.snddata = data;
	pia_set_input_cb1(6, 0);
	pia_set_input_cb1(6, 1);
	snd_cmd_log(data);
}


void by6803_snddiag1(){ 
	//Trigger an NMI to sound cpu
	cpu_set_nmi_line(BY6803_SCPU1NO, PULSE_LINE); 
}
void by6803_snddiag2(){ 
	//Trigger an NMI to sound cpu
	cpu_set_nmi_line(BY6803_SCPU1NO, PULSE_LINE); 
}
void by6803_snddiag3(){ 
	//M68000 DMD - Trigger IPL1 & IPL0 (Level 3) Interrupt!
	cpu_set_irq_line(BY6803_SCPU1NO, MC68000_IRQ_3, PULSE_LINE);
}
void by6803_snddiag4(){ 
	//NO Test Switch??

	//Trigger an NMI to sound cpu
	//cpu_set_nmi_line(BY6803_SCPU1NO, PULSE_LINE); 
}

/*Interfaces*/
struct DACinterface s2_dacInt = { 1, { 100 }};
struct DACinterface s3_dacInt = { 1, { 100 }};
struct DACinterface s4_dacInt = { 1, { 50 }};
struct YM2151interface s4_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { s4_ym2151IRQ }
};
struct hc55516_interface s4_hc55516Int =
  { 1, { 80 }};
