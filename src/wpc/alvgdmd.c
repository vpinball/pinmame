#include "driver.h"
#include "cpu/i8051/i8051.h"
#include "core.h"
#include "sndbrd.h"
#include "alvg.h"
#include "alvgdmd.h"

#ifdef VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/*---------------------------*/
/*ALVIN G 128x32 DMD Handling*/
/*---------------------------*/
#define DMD32_BANK0    1
#define DMD32_FIRQFREQ 481	//no idea on this one, just a wild guess

/*static vars*/
static int vid_page;
static UINT8  *dmd32RAM;

static struct {
  struct sndbrdData brdData;
  int cmd, planenable, disenable, setsync;
} dmdlocals;

/*declarations*/
static WRITE_HANDLER(dmd32_bank_w);
static READ_HANDLER(dmd32_latch_r);
static INTERRUPT_GEN(dmd32_firq);
static WRITE_HANDLER(dmd32_data_w);
static void dmd32_init(struct sndbrdData *brdData);
static WRITE_HANDLER(dmd32_ctrl_w);
static WRITE_HANDLER(control_w);
static READ_HANDLER(control_r);
static WRITE_HANDLER(port_w);
static READ_HANDLER(port_r);

/*Interface*/
const struct sndbrdIntf alvgdmdIntf = {
  NULL, dmd32_init, NULL, NULL,NULL,
  dmd32_data_w, NULL, dmd32_ctrl_w, NULL, SNDBRD_NOTSOUND
};


/*Main CPU sends command to DMD*/
static WRITE_HANDLER(dmd32_data_w)  {
	dmdlocals.cmd = data;
}

static WRITE_HANDLER(control_w)
{
	UINT16 tmpoffset = offset;
	tmpoffset += 0x8000;	//Adjust the offset so we can work with the "actual address" as it really is (just makes it easier, it's not necessary to do this)
	tmpoffset &= 0xf000;	//Remove bits 0-11 (in other words, treat 0x8fff the same as 0x8000!)
	switch (tmpoffset)
	{
		//Read Main CPU DMD Command
		case 0x8000:
			LOG(("WARNING! Writing to address 0x8000 - DMD Latch, data=%x!\n",data));
			break;

		//Send Data to the Main CPU
		case 0x9000:
			sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, data);
			break;

		//ROM Bankswitching
		case 0xa000:
			dmd32_bank_w(0,data);
			break;

		//ROWSTART Line
		case 0xb000:
			LOG(("rowstart = %x\n",data));
			break;

		//COLSTART Line
		case 0xc000:
			LOG(("colstart = %x\n",data));
			break;

		//NC
		case 0xd000:
		case 0xe000:
			LOG(("writing to not connected address: %x data=%x\n",offset,data));
			break;

		//SETSYNC Line
		case 0xf000:
			dmdlocals.setsync=data;
			LOG(("setsync=%x\n",data));
			break;
		default:
			LOG(("WARNING! Reading invalid control address %x\n", offset));
	}
	//Setsync line goes hi for all address except it's own(0xf000)
	if(offset != 0xf000) {
		dmdlocals.setsync=1;
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
		case 0x8000: {
			int data = dmd32_latch_r(0);
			return data;
		}
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
			LOG(("WARNING! Reading invalid control address %x\n", offset));
	}
	return 0;
}


static READ_HANDLER(port_r)
{
	//Port 1 is read in the wait for interrupt loop.. Not sure why this is done.
	if(offset == 1)
		return vid_page;
	else
		LOG(("port read @ %x\n",offset));
	return 0;
}

static WRITE_HANDLER(port_w)
{
	static int last = 0;
	switch(offset) {
		case 0:
			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		case 1:
			vid_page = (data&0x0f)<<11;
			if(last != data) {
				last = data;
				LOG(("port write @ %x, data=%x\n",offset,data));
			}
			break;
		case 2:
			LOG(("writing to port %x data = %x\n",offset,data));
			break;
		/*PORT 3:
			P3.0 = PLANENABLE (Plane Enable)
			P3.1 = LED
			P3.2 = INT0 (Not used as output)
			P3.3 = INT1 (Not used as output)
			P3.4 = TO   (Not used as output)
			P3.5 = DISENABLE (Display Enable)
			P3.6 = /WR  (Used Internally only?)
			P3.7 = /RD  (Used Internally only?) */
		case 3:
			dmdlocals.planenable = data & 0x01;
			alvg_UpdateSoundLEDS(1,(data&0x02)>>1);
			dmdlocals.disenable = ((data & 0x20) >> 5);
			break;
		default:
			LOG(("writing to port %x data = %x\n",offset,data));
	}
}

static WRITE_HANDLER(dmd32_bank_w)
{
	LOG(("setting dmd32_bank to: %x\n",data));
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
  MDRV_CPU_ADD(I8051, 12000000)	/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
MACHINE_DRIVER_START(test8031)
  MDRV_CPU_ADD(I8051, 12000000)	/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
MACHINE_DRIVER_END
#endif

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  dmd32_bank_w(0,0);			//Set DMD Bank to 0
  dmdlocals.setsync = 1;		//Start Sync @ 1
}

//Send data from Main CPU to latch - Set's the INT0 Line
static WRITE_HANDLER(dmd32_ctrl_w) {
	LOG(("INT0: Sending DMD Strobe - current command = %x\n",dmdlocals.cmd));
	cpu_set_irq_line(dmdlocals.brdData.cpuNo, I8051_INT0_LINE, HOLD_LINE);
}

//Read data from latch sent by Main CPU - Clear's the INT0 Line
static READ_HANDLER(dmd32_latch_r) {
  LOG(("INT0: reading latch: data = %x\n",dmdlocals.cmd));
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, I8051_INT0_LINE, CLEAR_LINE);
  return dmdlocals.cmd;
}

//Pulse the INT1 Line
static INTERRUPT_GEN(dmd32_firq) {
	if(dmdlocals.setsync) {
		LOG(("INT1 Pulse\n"));
		cpu_set_irq_line(dmdlocals.brdData.cpuNo, I8051_INT1_LINE, PULSE_LINE);
	}
	else
		printf("Skipping INT1\n");
}

PINMAME_VIDEO_UPDATE(alvgdmd_update) {
#ifdef MAME_DEBUG
  static int offset = 0;
#endif
  UINT8 *RAM  = ((UINT8 *)dmd32RAM);
  UINT8 *RAM2;
  tDMDDot dotCol;
  int ii,jj;
  RAM = RAM + vid_page;
  RAM2 = RAM + (dmdlocals.planenable*0x200);

#ifdef MAME_DEBUG
  core_textOutf(50,20,1,"offset=%08x", offset);
  memset(&dotCol,0,sizeof(dotCol));

  if(!debugger_focus) {
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
  if(keyboard_pressed_memory_repeat(KEYCODE_N,2))
	  //offset=0xb4;
	  offset=0xc3;
  if(keyboard_pressed_memory_repeat(KEYCODE_M,2))
  {
	  dmd32_data_w(0,offset);
	  dmd32_ctrl_w(0,0);
  }
  }
  RAM += offset;
  RAM2 += offset;
#endif

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
