/*Capcom Sound Hardware
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/i8051/i8051.h"
#include "capcom.h"
#include "capcoms.h"
#include "sndbrd.h"

#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//#define LOG(x)	printf x
#else
#define LOG(x)
#endif

/*Declarations*/
static void capcoms_init(struct sndbrdData *brdData);

/*Interfaces*/
static struct TMS320AV120interface capcoms_TMS320AV120Int1 = {
  1,		//# of chips
  {100}		//Volume levels
};
static struct TMS320AV120interface capcoms_TMS320AV120Int2 = {
  2,		//# of chips
  {50,50}	//Volume levels
};

/* Sound board */
const struct sndbrdIntf capcomsIntf = {
	"TMS320AV120", capcoms_init, NULL, NULL, NULL, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
   //"TMS320AV120", capcoms_init, NULL, NULL, alvg_sndCmd_w, alvgs_data_w, NULL, alvgs_ctrl_w, alvgs_ctrl_r, SNDBRD_NODATASYNC
};

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  void *buffTimer;
  UINT8 ram[0x8000+1];	//External 32K Ram
  int rombase_offset;	//Offset into current rom
  int rombase;			//Points to current rom
} locals;

static READ_HANDLER(port_r)
{
	LOG(("port read @ %x\n",offset));
	return 0;
}

static WRITE_HANDLER(port_w)
{
	static int last = 0;
	switch(offset) {
		//Used for external addressing...
		case 0:
		case 2:
			break;
		/*PORT 1:
			P1.0    (O) = LED (Inverted?)
			P1.1    (X) = NC
			P1.2    (?) = /CTS = ??
			P1.3    (?) = /RTS = ??
			P1.4    (O) = SCL = U10 - Pin 14 - EPOT CLOCK
			P1.5    (O) = SDA = U10 - Pin  9 - EPOT SERIAL DATA
			P1.6    (O) = /CBOF1 = CLEAR BOF1 IRQ
			P1.7    (O) = /CBOF2 = CLEAR BOF2 IRQ */
		case 1:
			//LED
			cap_UpdateSoundLEDS(~data&0x01);
			//CBOF1
			if((data&0x40)==0)	cpu_set_irq_line(locals.brdData.cpuNo, I8051_INT0_LINE, CLEAR_LINE);
			//CBOF1
			if((data&0x80)==0)	cpu_set_irq_line(locals.brdData.cpuNo, I8051_INT1_LINE, CLEAR_LINE);

			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		/*PORT 3:
			P3.0/RXD(?) = RXD
			P3.1/TXD(?) = TXD
			INT0    (I) = /BOF1  = BEG OF FRAME FOR MPG1
			INT1    (I) = /BOF2  = BEG OF FRAME FOR MPG2
			T0/P3.4 (I) = /SREQ1 = SOUND REQUEST MPG1
			T1/P3.5 (I) = /SREQ2 = SOUND REQUEST MPG2
			P3.6    (O) = /WR
			P3.7    (O) = /RD*/
		case 3:
			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		default:
			LOG(("writing to port %x data = %x\n",offset,data));
	}
}

READ_HANDLER(unk_r)
{
	LOG(("unk_r read @ %x\n",offset));
	return 0;
}

//Return a byte from the bankswitched roms
READ_HANDLER(rom_rd)
{
	int data;
	UINT8* base = (UINT8*)memory_region(REGION_SOUND1);	//Get pointer to 1st ROM..
	offset&=0xffff;	//strip off top bit
	LOG(("rom_r %x\n",offset));
	base+=(locals.rombase+locals.rombase_offset+offset);//Do bankswitching
	data = (int)*base;
	return data;
}

READ_HANDLER(ram_r)
{
	//LOG(("ram_r %x data = %x\n",offset,locals.ram[offset]));

	//Shift mirrored ram from it's current address range of 0x18000-0x1ffff to actual address range of 0-0x7fff
	if(offset > 0xffff)
		return locals.ram[offset-0x18000];
	//Return normal address range for offsets < 0xffff
	return locals.ram[offset];
}

WRITE_HANDLER(ram_w)
{
	offset&=0xffff;	//strip off top bit
	locals.ram[offset] = data;
	//LOG(("ram_w %x data = %x\n",offset,data));
}

//Set the /MPEG1 or /MPEG2 lines (active low) - clocks data into each of the tms320av120 chips
WRITE_HANDLER(mpeg_clock)
{
	LOG(("mpeg_clock %x data = %x\n",offset,data));
}

/*
U35 (BANK SWITCH)
D0 = A15 (+0x8000*1)
D1 = A16 (+0x8000*2)
D2 = A17 (+0x8000*3)
D3 = A18 (+0x8000*4)
D4 = A19 (+0x8000*5)*/
WRITE_HANDLER(bankswitch)
{
	LOG(("BANK SWITCH DATA=%x\n",data));
	data &= 0x1f;	//Keep only bits 0-4
	locals.rombase_offset = 0x8000*data;
}

/*
U36 (CONTROL)
D0 = /ROM0 Access (+0x100000*0) - Line is active lo
D1 = /ROM1 Access (+0x100000*1) - Line is active lo
D2 = /ROM2 Access (+0x100000*2) - Line is active lo
D3 = /ROM3 Access (+0x100000*3) - Line is active lo
D4 =  MPG1 Reset  (1 = Reset)
D5 =  MPG2 Reset  (1 = Reset)
D6 = /MPG1 Mute   (0 = MUTE)
D7 = /MPG2 Mute   (0 = MUTE)*/
WRITE_HANDLER(control_data)
{
	locals.rombase = 0x100000*(~data&0x0f);
	if(data&0x10)	TMS320AV120_reset(0);		//Reset TMS320AV120 Chip #1
	if(data&0x20)	TMS320AV120_reset(1);		//Reset TMS320AV120 Chip #2
	TMS320AV120_set_mute(0,(~data&0x40));		//Mute TMS320AV120 Chip #1 (Active low)
	TMS320AV120_set_mute(1,(~data&0x80));		//Mute TMS320AV120 Chip #2 (Active low)
	LOG(("CONTROL_DATA - DATA=%x\n",data));
}

/*Control Lines
a00 = U35 (BANK SWITCH)		(0x0001)
a01 = U36 (CONTROL)			(0x0002)
a02 = U33 (/MPEG1 CLOCK)	(0x0004)
a03 = U33 (/MPEG2 CLOCK)	(0x0008)   */
WRITE_HANDLER(control_w)
{
	offset&=0xffff;	//strip off top bit

	switch(offset) {
		case 0:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U35 - BANK SWITCH
		case 1:
			bankswitch(0,data);
			break;
		//U36 - CONTROL
		case 2:
			control_data(0,data);
			break;
		case 3:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U33 - /MPEG1 CLOCK
		case 4:
			mpeg_clock(0,data);
			break;
		case 5:
		case 6:
		case 7:
			LOG(("invalid control line - %x!\n",offset));
			break;
		//U33 - /MPEG2 CLOCK
		case 8:
			mpeg_clock(1,data);
			break;
		default:
			LOG(("control_w %x data = %x\n",offset,data));
	}
}

//The MC51 cpu's can all access up to 64K ROM & 64K RAM in the SAME ADDRESS SPACE
//It uses separate commands to distinguish which area it's reading/writing!
//So to handle this, the cpu core automatically adjusts all external memory access to the follwing setup..
//00000 -  FFFF is used for MOVC(/PSEN=0) commands
//10000 - 1FFFF is used for MOVX(/RD=0 or /WR=0) commands
static MEMORY_READ_START(capcoms_readmem)
{ 0x000000, 0x001fff, MRA_ROM },	//Internal ROM 
{ 0x002000, 0x007fff, unk_r },		//This should never be accessed
{ 0x008000, 0x00ffff, ram_r },		//MOVC can access external ram here!
{ 0x010000, 0x017fff, rom_rd},		//MOVX can access roms here!
{ 0x018000, 0x01ffff, ram_r },		//MOVX can access external ram (mirrored) here!
MEMORY_END
static MEMORY_WRITE_START(capcoms_writemem)
{ 0x000000, 0x001fff, MWA_ROM },	//Internal ROM
{ 0x002000, 0x00ffff, MWA_NOP },	//This cannot be accessed by the 8051 core.. (there's no MOVC command for writing!)
{ 0x010000, 0x017fff, control_w },	//MOVX can write to Control chips here!
{ 0x018000, 0x01ffff, ram_w },		//MOVX can write to external RAM here!
MEMORY_END

static PORT_READ_START( capcoms_readport )
	{ 0x00,0xff, port_r },
PORT_END
static PORT_WRITE_START( capcoms_writeport )
	{ 0x00,0xff, port_w },
PORT_END


//Driver template with 8752 cpu
MACHINE_DRIVER_START(capcoms)
  MDRV_CPU_ADD(I8752, 12000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(capcoms_readmem, capcoms_writemem)
  MDRV_CPU_PORTS(capcoms_readport, capcoms_writeport)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Driver with 1 x TMS320av120
MACHINE_DRIVER_START(capcom1s)
  MDRV_IMPORT_FROM(capcoms)
  MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int1)
MACHINE_DRIVER_END

//Driver with 2 x TMS320av120
MACHINE_DRIVER_START(capcom2s)
  MDRV_IMPORT_FROM(capcoms)
  MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int2)
MACHINE_DRIVER_END

extern void tms_FillBuff(int);

void cap_FillBuff(int dummy) {
	tms_FillBuff(0);
	tms_FillBuff(1);
}

static void capcoms_init(struct sndbrdData *brdData) {
  memset(&locals, 0, sizeof(locals));
  locals.brdData = *brdData;

  /* stupid timer/machine init handling in MAME */
  if (locals.buffTimer) timer_remove(locals.buffTimer);

  /*-- Create timer to fill our buffer --*/
  locals.buffTimer = timer_alloc(cap_FillBuff);

  /*-- start the timer --*/
  timer_adjust(locals.buffTimer, 0, 0, TIME_IN_HZ(10));		//Frequency is somewhat arbitrary but must be fast enough to work
}
