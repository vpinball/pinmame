/************************************************************************************************
  Nuova Bell Games
  ----------------
  by Steve Ellenoff
  
  Main CPU Board:

  CPU: Motorola M6802
  Clock: Unknown (1Mhz?)
  Interrupt: IRQ - Via the 6821 chips, NMI - Push Button? 
  I/O: 2 X 6821 

  Issues/Todo:

  This hardware is a total nightmare.. The display stuff makes no sense to me. I haven't tried looking at
  solenoids/lamps, or to see if switches are working.


  Notes:
  Manual shows only 48 switches, implying a 8x6 matrix (6 Strobes) - It shows 4 Switch Strobe & 2 Cab Strobe (shared)

  Lamps: Lamp Addr is 1-16 data, Lamp Data 0-3 (each bit selects different 1-16 mux) - Strobe 2 used for Aux Lamps

  Game is done with testing @ 14A3? 
  143E - Display some digits?
  152e - CLI - Clear Interrupt Disable
  RAM - 680 - Contains text for display driver
  DP @ 1098 for display out routine (loop begins @ 1089?) - similar @ 10c9 (starts @ 10ba?)



************************************************************************************************/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sim.h"

#define NUOVA_SOLSMOOTH 4
#define NUOVA_CPUFREQ		   500000			//0.5Mhz (NO IDEA)
#define F1GP_ZCFREQ				220/2			//220 Volt / 2 (NO IDEA)
#define F1GP_555TIMER_FREQ		500			//??

#define F1GP_SWCPUBUTT    -7
#define F1GP_SWCPUDIAG    -6

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static int f1gp_data_to_eseg(int data);

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int diagnosticLed;
  int piaIrq;
  int SwCol;
  int DispCol;
  int LampCol;
  int zero_cross;
  int timer_555;
  int last_nmi_state;
  int pia0_a;
  int pia1_a;
  int pia0_cb2;
  int pia0_da_enable;
} locals;
//static data8_t *f1gp_CMOS;

/***************/
/* ZERO CROSS? */
/***************/
static void f1gp_zeroCross(int data) {
	 locals.zero_cross = !locals.zero_cross;
	 pia_set_input_cb1(0,locals.zero_cross);
}

/********************/
/* 555 Timer CROSS? */
/********************/
static void f1gp_555timer(int data) {
	 locals.timer_555 = !locals.timer_555;
	 pia_set_input_ca1(1,locals.timer_555);
}

static WRITE_HANDLER(disp_w) { 
	//LOG(("%08x: disp1_w = %04x\n",activecpu_get_previouspc(),data)); 
	if(offset&0x80)
		locals.segments[locals.DispCol].w = f1gp_data_to_eseg(data);
	else
		locals.segments[locals.DispCol+16].w = f1gp_data_to_eseg(data);
}

static INTERRUPT_GEN(f1gp_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % NUOVA_SOLSMOOTH) == 0) {
	locals.solenoids = coreGlobals.pulsedSolState;
  }

  coreGlobals.diagnosticLed = locals.diagnosticLed;
  locals.diagnosticLed = 0;

  core_updateSw(1);
}

static SWITCH_UPDATE(f1gp) {
  if (inports) {
	  //Column 0 Switches
	  coreGlobals.swMatrix[0] = (inports[CORE_COREINPORT] & 0x00c0)>>6;		
	  //Column 1 Switches
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x9f) | ((inports[CORE_COREINPORT] & 0x03)<<5);
	  //Column 2 Switches
	  coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0x78) | 
		  ((inports[CORE_COREINPORT] & 0x1c)>>2) | ((inports[CORE_COREINPORT] & 0x20)<<2);     
  }
  // CPU DIAG SWITCH
  if (core_getSw(F1GP_SWCPUBUTT))
  {
	  if(!locals.last_nmi_state)
	  {
		  locals.last_nmi_state = 1;
		  cpu_set_nmi_line(0, ASSERT_LINE);
	  }
  }
  else
  {
	  if(locals.last_nmi_state)
	  {
		  locals.last_nmi_state = 0;
		  cpu_set_nmi_line(0, CLEAR_LINE);
	  }
  }
}

static void f1gp_irqline(int state) {
  if (state) {
    cpu_set_irq_line(0, M6808_IRQ_LINE, ASSERT_LINE);
    pia_set_input_ca1(0, (core_getSw(F1GP_SWCPUDIAG))?1:0);
  }
  //else if (!locals.piaIrq) {
  else {
    cpu_set_irq_line(0, M6808_IRQ_LINE, CLEAR_LINE);
    pia_set_input_ca1(0, 0);
  }
}

static void f1gp_piaIrq(int state) {
  f1gp_irqline(locals.piaIrq = state);
}

/* ----- */
/* PIA 0 */
/* ----- */

/*  i  PB0-7:  Dips  1-32 Read & Swtich Read 0-7 & Cabinet Switch Read 0-7 */
static READ_HANDLER(pia0_b_r)
{
	int data = 0;
	if(locals.pia0_a & 0xE0 || locals.pia0_cb2 )
	{
		int dipcol = ((locals.pia0_a)>>5) | (locals.pia0_cb2<<3);
		if(dipcol < 9)
		{
			dipcol = core_BitColToNum(dipcol);
			data = core_getDip(dipcol);
			LOG(("%04x: DIP SWITCH BANK #%d - READ: pia0_b_r =%x\n",activecpu_get_previouspc(),dipcol,data));
		}
	}
	else
	{
		data = coreGlobals.swMatrix[locals.SwCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers 
		LOG(("%04x: SWITCH COL #%d - READ: pia0_b_r =%x\n",activecpu_get_previouspc(),locals.SwCol,data));
	}
	//return data;
	return 0;
}
/*  i  CA1:    Diagnostic Switch? */
static READ_HANDLER(pia0_ca1_r)
{
	int data = (core_getSw(F1GP_SWCPUDIAG))?1:0;
	LOG(("%04x: DIAG SWITCH: pia0_ca1_r =%x \n",activecpu_get_previouspc(),data));
	return data;
}
/*   i  CB1:    Tied to 43V line (Some kind of zero cross detection?) */
static READ_HANDLER(pia0_cb1_r)
{
	int data = locals.zero_cross;
	LOG(("%04x: X-CROSS?: pia0_cb1_r = %x\n",activecpu_get_previouspc(),data));
	return data;
}

/*
  o  PA0-PA1 Switch Strobe 0 - 1 & Cabinet Switch Strobe 0 - 1 & Lamp Addr 0 - 1 & Disp Strobe 0 - 1
  o  PA2-PA3 Switch Strobe 2 - 3 & Lamp Addr 2 - 3 & Disp Strobe 2 - 3
  o  PA4:    Switch Strobe 4   & Lamp Data 0 & Disp Data 0
  o  PA5:    Dips  1-8 Strobe  & Lamp Data 1 & Disp Data 1
  o  PA6:    Dips  9-16 Strobe & Lamp Data 2 & Disp Data 2
  o  PA7:    Dips 17-24 Strobe & Lamp Data 3 & Disp Data 3
*/
static WRITE_HANDLER(pia0_a_w)
{
	locals.pia0_a = data;
	locals.SwCol = core_BitColToNum(data & 0xf);
	LOG(("%04x: EVERYTHING: pia0_a_w = %x \n",activecpu_get_previouspc(),data));
}
/*  o  CA2:    Display Blank */
static WRITE_HANDLER(pia0_ca2_w)
{
	locals.pia0_da_enable = data & 1;
	printf("%04x: DISP BLANK: pia0_ca2_w = %x \n",activecpu_get_previouspc(),data);
	LOG(("%04x: DISP BLANK: pia0_ca2_w = %x \n",activecpu_get_previouspc(),data));
}
/*  o  CB2:    Dips 25-32 Strobe & Lamp Strobe #1 */
static WRITE_HANDLER(pia0_cb2_w)
{
	locals.pia0_cb2 = data & 1;
	LOG(("%04x: DIP25 STR & LAMP STR 1: pia0_cb2_w = %x \n",activecpu_get_previouspc(),data));
}

 /* -----------*/
 /* PIA 1 (U10)*/
 /* -----------*/

/* i  CA1:    Tied to a 555 timer */
static READ_HANDLER(pia1_ca1_r)
{
	int data = locals.timer_555;
	LOG(("%04x: 555 Timer: pia1_ca1_r = %x\n",activecpu_get_previouspc(),data));
	return data;
}
/* i  CB1:    Marked FE */
static READ_HANDLER(pia1_cb1_r)
{
	LOG(("%04x: FE?: pia1_cb1_r \n",activecpu_get_previouspc()));
	return 0;
}

/* 
  o  PA0     Display Strobe 5
  o  PA1-7   Display Digit 1-7 (note: diagram shows PA7 =Digit 1, PA6 = Digit 2, etc..)
*/
static int lastval = 0;
static int lastcol = 0;
static WRITE_HANDLER(pia1_a_w)
{
	locals.pia1_a = data;

	//Display Blank must be 0 - for data to display
	if(!locals.pia0_da_enable)
	{
		printf("%04x: DISPLAY STR.5 & DIGIT: pia1_a_w = %x (BLANK=%x DIG=%x CHAR=%c) \n",activecpu_get_previouspc(),data,data&1,data>>1,locals.pia0_a);
		//LOG(("%04x: DISPLAY STR.5 & DIGIT: pia1_a_w = %x (STR.5=%x DIG=%x) \n",activecpu_get_previouspc(),data,data&1,data>>1));
		if( ((data>>1)==0) && lastval)
			{
				//hack to remove the spaces
				if((lastcol&0x40)==0)
					locals.DispCol = (locals.DispCol + 1) % 16;
				if(lastcol&0x80) printf("****************\n");
				disp_w(lastcol,locals.pia0_a);
				printf("dispcol = %d\n",locals.DispCol);
			}
		else
			lastcol = data;
		lastval = (data>>1)?1:0;
	}
}
/* 
  o  PB0:    A Solenoid
  o  PB1:    B Solenoid
  o  PB2:    C Solenoid
  o  PB3:    D Solenoid & AUX STS Signal
  o  PB4:    ? (sound?)
  o  PB5:    ? (sound?)
  o  PB6:    ? (sound?)
  o  PB7:    ? (sound?)
*/
static WRITE_HANDLER(pia1_b_w)
{
	LOG(("%04x: SOLS & (SOUND?): pia1_b_w = %x \n",activecpu_get_previouspc(),data));
}
/* o  CA2:    LED & Lamp Strobe #2 */
static WRITE_HANDLER(pia1_ca2_w)
{
	locals.diagnosticLed = data & 1;
	LOG(("%04x: LED & Lamp Strobe #2: pia1_ca2_w = %x \n",activecpu_get_previouspc(),data));
}
/* o  CB2:    Solenoid Bank Select */
static WRITE_HANDLER(pia1_cb2_w)
{
	LOG(("%04x: SOL BANK SELECT: pia1_cb2_w = %x \n",activecpu_get_previouspc(),data));
}

//All these should not be used according to schematic
static READ_HANDLER(pia0_a_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia0_a_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia0_ca2_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia0_ca2_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia0_cb2_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia0_cb2_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia1_a_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia1_a_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia1_b_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia1_b_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia1_ca2_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia1_ca2_r \n",activecpu_get_previouspc()));
	return 0;
}
static READ_HANDLER(pia1_cb2_r)
{
	LOG(("%04x: *WARNING* - Undocumented - pia1_cb2_r \n",activecpu_get_previouspc()));
	return 0;
}
static WRITE_HANDLER(pia0_b_w)
{
	LOG(("%04x: *WARNING* - Undocumented - pia0_b_w = %x \n",activecpu_get_previouspc(),data));
}

static const struct pia6821_interface f1gp_pia[] = {
{ /* PIA 0 (U9)
  -------------
  o  PA0-PA1 Switch Strobe 0 - 1 & Cabinet Switch Strobe 0 - 1 & Lamp Addr 0 - 1 & Disp Addr 0 - 1
  o  PA2-PA3 Switch Strobe 2 - 3 & Lamp Addr 2 - 3 & Disp Addr 2 - 3
  o  PA4:    Switch Strobe 4   & Lamp Data 0 & Disp Data 0
  o  PA5:    Dips  1-8 Strobe  & Lamp Data 1 & Disp Data 1
  o  PA6:    Dips  9-16 Strobe & Lamp Data 2 & Disp Data 2
  o  PA7:    Dips 17-24 Strobe & Lamp Data 3 & Disp Data 3
  i  PB0-7:  Dips  1-32 Read & Swtich Read 0-7 & Cabinet Switch Read 0-7
  i  CA1:    Diagnostic Switch?
  i  CB1:    Tied to 43V line (Some kind of zero cross detection?)
  o  CA2:    Activates Disp Addr Data
  o  CB2:    Dips 25-32 Strobe & Lamp Strobe #1 */
 /* in  : A/B,CA1/B1,CA2/B2 */ pia0_a_r, pia0_b_r, pia0_ca1_r, pia0_cb1_r, pia0_ca2_r, pia0_cb2_r,
 /* out : A/B,CA2/B2        */ pia0_a_w, pia0_b_w, pia0_ca2_w, pia0_cb2_w,
 /* irq : A/B               */ f1gp_piaIrq, f1gp_piaIrq
},{
 /* PIA 1 (U10)
 -------------
  o  PA0     Display Blanking
  o  PA1-7   Display Digit 1-7 (note: diagram shows PA7 =Digit 1, PA6 = Digit 2, etc..)
  o  PB0:    A Solenoid
  o  PB1:    B Solenoid
  o  PB2:    C Solenoid
  o  PB3:    D Solenoid & AUX STS Signal
  o  PB4:    ? (sound?)
  o  PB5:    ? (sound?)
  o  PB6:    ? (sound?)
  o  PB7:    ? (sound?)
  i  CA1:    Tied to a 555 timer
  i  CB1:    Marked FE
  o  CA2:    LED & Lamp Strobe #2
  o  CB2:    Solenoid Bank Select */
 /* in  : A/B,CA1/B1,CA2/B2 */ pia1_a_r, pia1_b_r, pia1_ca1_r, pia1_cb1_r, pia1_ca2_r, pia1_cb2_r,
 /* out : A/B,CA2/B2        */ pia1_a_w, pia1_b_w, pia1_ca2_w, pia1_cb2_w,
 /* irq : A/B               */ f1gp_piaIrq, f1gp_piaIrq
}
};

static MACHINE_INIT(f1gp) {
  memset(&locals, 0, sizeof(locals));
  pia_config(0, PIA_STANDARD_ORDERING, &f1gp_pia[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &f1gp_pia[1]);
#if 0
  sndbrd_0_init(SNDBRD_S67S, 1, NULL, NULL, NULL);
#endif
}

static MACHINE_RESET(f1gp) {
  pia_reset();
}

static MACHINE_STOP(f1gp) {
  //sndbrd_0_exit();
}


//Lamp Rows (actually columns) 1-8
static WRITE16_HANDLER(lamp1_w) { locals.LampCol = core_BitColToNum(data >> 8); }
//Lamp Cols (actually rows) 1-8
static WRITE16_HANDLER(lamp2_w) { 	coreGlobals.tmpLampMatrix[locals.LampCol] = data>>8; }

//Solenoids 1-16
static WRITE16_HANDLER(sol1_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF0000) | data; }
//Solenoids 17-32
static WRITE16_HANDLER(sol2_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x0000FFFF) | (data<<16); }




/*********************************************************************************************
    Convert F1 Grand Prix Display Driver Codes to standard core 16 segment characeters

   Core16 Segment Layout

         1                                              1
    ----------                                       ---------
   |\9 10| 11/|             9 \ 10|   / 11           |       |
 6 | \   |  / | 2              \  |  /             6 |       | 2 
   |  \  | /  |                 \ | /                |       |
 7  ----  ----  12          7 ---- ---- 12         7 ---- ---- 12  
   |15/ | \13 |                 / |\                 |       |
 5 | /  |  \  | 3           15 /  | \ 13           5 |       | 3 
   |/ 14|   \ |               / 14|  \               |       |
    ----------                                       ---------  
        4                                               4      

	8 = comma, 16 = period

	Empty:
	(20) = 0

	Numbers: (BITS ON)
	0 (30) = (1,2,3,4,5,6)					= 0x3f
	1 (31) = (2,3)							= 0x06
	2 (32) = (1,2,4,5,7,12)					= 0x85b
	3 (33) = (1,2,3,4,7,12)					= 0x84f
	4 (34) = (2,3,6,7,12)					= 0x866
	5 (35) = (1,3,4,6,7,12)					= 0x86d
	6 (36) = (1,3,4,5,6,7,12)				= 0x87d
	7 (37) = (1,2,3)						= 0x07
	8 (38) = (1,2,3,4,5,6,7,12)				= 0x87f
	9 (39) = (1,2,3,4,6,7,12)				= 0x86f
    : (3A) = (assume it means period) (16)	= 0x8000
	; (3B) = (assume it means comma)  (8)	= 0x80
	Special Chars 1:
	< (3C) = (11,13)						= 0x1400
	= (3D) = ??								= 0xffff (all)
	> (3E) = (9,15)							= 0x4100
	? (3F) = ?								= 0xffff (all)
	@ (40) = ?								= 0xffff (all)
	Letters:
	A (41) = (1,2,3,5,6,7,12)				= 0x877	
	B (42) = (1,2,3,4,10,12,14)				= 0x2a0f
	C (43) = (1,4,5,6)						= 0x39
	D (44) = (1,2,3,4,10,14)				= 0x220f
	E (45) = (1,4,5,6,7,12)					= 0x879
	F (46) = (1,5,6,7,12)					= 0x871
	G (47) = (1,3,4,5,6,12)					= 0x83d
	H (48) = (2,3,5,6,7,12)					= 0x876	
	I (49) = (1,4,10,14)					= 0x2209
	J (4A) = (2,3,4,5)						= 0x1e
	K (4B) = (5,6,7,11,13)					= 0x1470
	L (4C) = (4,5,6)						= 0x38
	M (4D) = (2,3,5,6,9,11)					= 0x536
	N (4E) = (2,3,5,6,9,13)					= 0x1136
	O (4F) = (1,2,3,4,5,6)					= 0x3f
	P (50) = (1,2,5,6,7,12)					= 0x873
	Q (51) = (1,2,3,4,5,6,13)				= 0x103f
	R (52) = (1,2,5,6,7,12,13)				= 0x1873
	S (53) = (1,3,4,6,7,12)					= 0x86d
	T (54) = (1,10,14)						= 0x2201
	U (55) = (2,3,4,5,6)					= 0x3e
	V (56) = (5,6,11,15)					= 0x4430
	W (57) = (2,3,5,6,13,15)				= 0x5036
	X (58) = (9,11,13,15)					= 0x5500
	Y (59) = (9,11,14)						= 0x2500
	Z (5A) = (1,4,11,15)					= 0x4409
	Chars:
	[ (5B) = (1,4,5,6)						= 0x39
	\ (5C) = (9,13)							= 0x1100
	] (5D) = (1,2,3,4)						= 0x0f
	^ (5E) = (13,15)						= 0x5000
	- (5F) = (7,12)							= 0x840
*/

static const int data_to_seg[] = {
	0x3f,0x06,0x85b,0x84f,0x866,0x86d,0x87d,0x07,0x87f,0x86f,0x8000,0x80,  	  //0-9 AND .,
	0x1400,0xffff,0x4100,0xffff,0xffff,										  //Spec Chars 1
	0x877,0x2a0f,0x39,0x220f,0x879,0x871,0x83d,0x876,0x2209,0x1e,0x1470,0x38, //A - L
	0x536,0x1136,0x3f,0x873,0x103f,0x1873,0x86d,0x2201,0x3e,0x4430,0x5036,	  //M - W
	0x5500,0x2500,0x4409,													  //X - Z
	0x39,0x1100,0x0f,0x5000,0x840											  //Spec Chars 2
};

static int f1gp_data_to_eseg(int data)
{
	if(data == 0x20) return 0;
	if(data > 0x5f) 
		{
			printf("missing seg # %x\n",data);
			return 0xffff;
		}
	return data_to_seg[data-0x30];
}

/*-----------------------------------
/  Memory map for Main CPU board
/------------------------------------*/
static MEMORY_READ_START(cpu_readmem)
  { 0x0000, 0x0087, MRA_RAM }, 
  { 0x0088, 0x008b, pia_r(0)}, 
  { 0x008c, 0x008f, MRA_RAM }, 
  { 0x0090, 0x0093, pia_r(1)},
  { 0x0094, 0x07ff, MRA_RAM }, 
  { 0x1000, 0x1fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_ROM },
  { 0x7000, 0x7fff, MRA_ROM },
  { 0x9000, 0x9fff, MRA_ROM },
  { 0xb000, 0xbfff, MRA_ROM },
  { 0xd000, 0xdfff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  { 0x0000, 0x0087, MWA_RAM }, 
  { 0x0088, 0x008b, pia_w(0)}, 
  { 0x008c, 0x008f, MWA_RAM }, 
  { 0x0090, 0x0093, pia_w(1)},
  { 0x0094, 0x07ff, MWA_RAM },
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_ROM },
  { 0x7000, 0x7fff, MWA_ROM },
  { 0x9000, 0x9fff, MWA_ROM },
  { 0xb000, 0xbfff, MWA_ROM },
  { 0xd000, 0xdfff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG16},
  {3, 0,16,16,CORE_SEG16},
  {0}
};
static core_tGameData f1gpGameData = {GEN_ZAC2, disp};
static void init_f1gp(void) {
  core_gameData = & f1gpGameData;
}

/* Manual starts with a switch # of 0 */
static int f1gp_sw2m(int no) { return no+7+1; }
static int f1gp_m2sw(int col, int row) { return col*8+row-7-1; }

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(f1gp) {
  //core_nvram(file, read_or_write, s6_CMOS, 0x0100, 0xff);
}

MACHINE_DRIVER_START(f1gp)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(f1gp,f1gp,f1gp)
  MDRV_CPU_ADD_TAG("mcpu", M6802, NUOVA_CPUFREQ)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_VBLANK_INT(f1gp_vblank, 1)
  MDRV_NVRAM_HANDLER(f1gp)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(f1gp)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(f1gp_zeroCross, F1GP_ZCFREQ)
  MDRV_TIMER_ADD(f1gp_555timer,  F1GP_555TIMER_FREQ)
MACHINE_DRIVER_END

INPUT_PORTS_START(f1gp) \
  CORE_PORTS \
  SIM_PORTS(4) \
  PORT_START /* 0 */ \
  /* Switch Column 1 */
    /* Switch  6 (SW Col 1?)*/
    COREPORT_BITDEF(  0x0001, IPT_START1,         IP_KEY_DEFAULT) \
    /* Switch  7 (SW Col 1?)*/
    COREPORT_BIT(     0x0002, "Ball Tilt",        KEYCODE_INSERT) \
    /* Switch  9 (SW Col 2?)*/
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          KEYCODE_3) \
	/* Switch 10 (SW Col 2?)*/
    COREPORT_BITDEF(  0x0008, IPT_COIN1,          IP_KEY_DEFAULT) \
	/* Switch 11 (SW Col 2?)*/
    COREPORT_BITDEF(  0x0010, IPT_COIN3,          KEYCODE_4) \
	/* Switch 16 (SW Col 2?)*/
    COREPORT_BIT   (  0x0020, "Slam Tilt",        KEYCODE_DEL) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0040, "CPU Button",       KEYCODE_7) \
	COREPORT_BIT(     0x0080, "Self Test",        KEYCODE_8) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

//U6 Ram Notes
// A0-A10 Used (0x800) size - accessible during E-cycle at any address?

//U7 Rom Notes
//
// The schematic has got to be wrong for a variety of reasons, but luckily, I found the startup
//
// After finally finding the right map to get the cpu started
// -Watching the rom boot up it actually tests different sections of rom, which is a huge help!
// - I have no idea how this actually makes any sense!
// Check - Data @ 0x1000 = 0x1A (found @ 0000)
// Check - Data @ 0x5000 = 0x55 (found @ 1000) (A14->A12? or not used?)
// Check - Data @ 0x7000 = 0x7A (found @ 5000) (A13 = 0?)
// Check - Data @ 0x9000 = 0x95 (found @ 2000) (A15 = 0, A12 -> A13)
// Check - Data @ 0xB000 = 0xBA (found @ 6000) (A15->A13, A12->A14, A14->A12)
// Check - Data @ 0xD000 = 0xD5 (found @ 3000) (A15->A13, A14=0)
// Check - Data @ 0xF000 = 0xFA (found @ 7000) (A15->A13, A12->A14, A14->A12)

ROM_START(f1gp) \
  NORMALREGION(0x100000, REGION_USER1) \
    ROM_LOAD("cpu_u7", 0x0000, 0x8000, CRC(2287dea1) SHA1(5438752bf63aadaa6b6d71bbf56a72d8b67b545a)) \
  NORMALREGION(0x100000, REGION_CPU1) \
  ROM_COPY(REGION_USER1, 0x0000, 0x1000,0x1000) \
  ROM_COPY(REGION_USER1, 0x1000, 0x5000,0x1000) \
  ROM_COPY(REGION_USER1, 0x5000, 0x7000,0x1000) \
  ROM_COPY(REGION_USER1, 0x2000, 0x9000,0x1000) \
  ROM_COPY(REGION_USER1, 0x6000, 0xb000,0x1000) \
  ROM_COPY(REGION_USER1, 0x3000, 0xd000,0x1000) \
  ROM_COPY(REGION_USER1, 0x7000, 0xf000,0x1000)
ROM_END

CORE_GAMEDEFNV(f1gp, "F1 Grand Prix", 1987, "Nuova Bell Games", f1gp, 0)

