#include "driver.h"
#include "cpu/i8051/i8051.h"
#include "core.h"
#include "sndbrd.h"
#include "alvgdmd.h"

/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
  UINT32 *framedata;
  int blnk, rowdata, rowclk, frame;
} dmdlocals;

static int vid_page;
static UINT8  *dmd32RAM;

static WRITE_HANDLER(dmd_data_w)  { dmdlocals.ncmd = data; }
static READ_HANDLER(dmd_status_r) { return dmdlocals.status; }
static READ_HANDLER(dmd_busy_r)   { return dmdlocals.busy; }

/*------------------------------------------*/
/*Data East, Sega, Stern 128x32 DMD Handling*/
/*------------------------------------------*/
#define DMD32_BANK0    1
#define DMD32_FIRQFREQ 480	//no idea on this one, just a wild guess

static WRITE_HANDLER(dmd32_ctrl_w);
static void dmd32_init(struct sndbrdData *brdData);

const struct sndbrdIntf alvgdmdIntf = {
  NULL, dmd32_init, NULL, NULL,NULL, 
  dmd_data_w, dmd_busy_r, dmd32_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE_HANDLER(dmd32_bank_w);
static WRITE_HANDLER(dmd32_status_w);
static READ_HANDLER(dmd32_latch_r);
static INTERRUPT_GEN(dmd32_firq);

static WRITE_HANDLER(control_w)
{
	UINT16 tmpoffset = offset;
	tmpoffset += 0x8000;	//Adjust the offset so we can work with the "actual address" as it really is (just makes it easier, it's not necessary to do this)
	tmpoffset &= 0xf000;	//Remove bits 0-11 (in other words, treat 0x8fff the same as 0x8000!)
	switch (tmpoffset)
	{
		//Read Main CPU DMD Command
		case 0x8000:
			logerror("WARNING! Writing to address 0x8000 - DMD Latch, data=%x!\n",data);
			break;

		//Send Data to the Main CPU
		case 0x9000:
			//printf("sending data to main cpu %x\n",data);
			sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, data);
			break;

		//ROM Bankswitching
		case 0xa000:
			//printf("setting bank to %x\n",data&0x1f);
			dmd32_bank_w(0,data);
			break;

		//ROWSTART Line
		case 0xb000:
			break;

		//COLSTART Line
		case 0xc000:
			break;

		//NC
		case 0xd000:
		case 0xe000:
			break;

		//SETSYNC Line
		case 0xf000:
			break;
		default:
			logerror("WARNING! Reading invalid control address %x\n", offset);
	}
}

static READ_HANDLER(control_r)
{
	UINT16 tmpoffset = offset;
	tmpoffset += 0x8000;	//Adjust the offset so we can work with the "actual address" as it really is (just makes it easier, it's not necessary to do this)
	tmpoffset &= 0xf000;	//Remove bits 0-11 (in other words, treat 0x8fff the same as 0x8000!)
	switch (tmpoffset)
	{
		//Read Main CPU DMD Command
		case 0x8000:
			//printf("reading dmd command %x\n",dmdlocals.ncmd);
			return dmdlocals.ncmd;
		//While unlikely, a READ to these addresses, can actually trigger control lines, so we call control_w()
		case 0x9000:
		case 0xa000:
		case 0xb000:
		case 0xc000:
		case 0xd000:
		case 0xe000:
		case 0xf000:
			control_w(offset,0);
			break;
		default:
			logerror("WARNING! Reading invalid control address %x\n", offset);
	}
	return 0;
}


static READ_HANDLER(port_r)
{
	//printf("port read @ %x\n",offset);
	logerror("port read @ %x\n",offset);
	return 0;
}

static WRITE_HANDLER(port_w)
{
	static int last = 0;
	switch(offset) {
		case 1:
		vid_page = (data&0x0f)<<11;
		if(last != data) {
			last = data;
			//printf("port write @ %x, data=%x\n",offset,data);
			//logerror("port write @ %x, data=%x\n",offset,data);
		}
	}
}

static WRITE_HANDLER(dmd32_bank_w)
{
//	printf("dmd32_bank_w: %x\n",data);
	cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion + ((data & 0x1f)*0x8000));
}


//The MC51 cpu's can all access up to 64K ROM & 64K RAM in the SAME ADDRESS SPACE
//It uses separate commands to distinguish which area it's reading/writing to (RAM or ROM).
//So to handle this, the cpu core automatically adjusts all external memory access to the follwing setup..
//0-FFFF is for ROM, 10000 - 1FFFF is for RAM
static MEMORY_READ_START(alvgdmd_readmem)
	{ 0x000000, 0x007fff, MRA_ROM },
	{ 0x008000, 0x00ffff, MRA_BANK1 },
	{ 0x010000, 0x017fff, MRA_RAM },
	{ 0x018000, 0x01ffff, control_r },
MEMORY_END

static MEMORY_WRITE_START(alvgdmd_writemem)
	{ 0x000000, 0x00ffff, MWA_ROM },		//This area can never really be accessed by the cpu core but we'll put this here anyway
	{ 0x010000, 0x017fff, MWA_RAM, &dmd32RAM },
	{ 0x018000, 0x01ffff, control_w },
MEMORY_END

static PORT_READ_START( alvgdmd_readport )
	{ 0x00,0xff, port_r },
PORT_END

static PORT_WRITE_START( alvgdmd_writeport )
	{ 0x00,0xff, port_w },
PORT_END

MACHINE_DRIVER_START(alvgdmd)
  MDRV_CPU_ADD(I8051, 12000000)		/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
MACHINE_DRIVER_START(test8031)
  MDRV_CPU_ADD(I8051, 12000000)		/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
MACHINE_DRIVER_END
#endif

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  dmd32_bank_w(0,0);	//Set DMD Bank to 0
}

static WRITE_HANDLER(dmd32_ctrl_w) {
//	printf("Sending DMD Strobe - current command = %x\n",dmdlocals.ncmd);
	//logerror("Sending DMD Strobe - current command = %x\n",dmdlocals.ncmd);
	cpu_set_irq_line(dmdlocals.brdData.cpuNo, I8051_INT0_LINE, PULSE_LINE);
}

static READ_HANDLER(dmd32_latch_r) {
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 0); // Clear Busy
//  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
  return dmdlocals.cmd;
}

static INTERRUPT_GEN(dmd32_firq) {
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, I8051_INT1_LINE, PULSE_LINE);
}

PINMAME_VIDEO_UPDATE(alvgdmd_update) {

  UINT8 *RAM  = ((UINT8 *)dmd32RAM);
  UINT8 *RAM2 = RAM;//+ 0x200;
  tDMDDot dotCol;
  int ii,jj;
  static int offset = 0;

  core_textOutf(50,20,1,"offset=%4x", offset);
  memset(&dotCol,0,sizeof(dotCol));

  if(keyboard_pressed_memory_repeat(KEYCODE_Z,2))
	  offset+=0x0001;
  if(keyboard_pressed_memory_repeat(KEYCODE_X,2))
	  offset-=0x0001;
  if(keyboard_pressed_memory_repeat(KEYCODE_C,2))
	  offset=0;
  if(keyboard_pressed_memory_repeat(KEYCODE_V,2))
	  offset+=0x800;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
	  offset-=0x800;

  RAM = RAM+offset+vid_page;
  RAM2 = RAM + 0x200;

  for (ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (128/8); jj++) {
      UINT8 intens1 = 2*(RAM[0] & 0x55) + (RAM2[0] & 0x55);
      UINT8 intens2 =   (RAM[0] & 0xaa) + (RAM2[0] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      RAM += 1; RAM2 += 1;
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
