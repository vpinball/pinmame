/************************************************************************************************
 Gottlieb System 1
 -----------------

   Hardware:
   ---------
		CPU:     PPS/4 @ 198.864 kHz
		IO:      PPS/4 Ports
		DISPLAY: 4 x 6 Digit 9 segment panels, 1 x 4 Digit 7 segment panels
		SOUND:   3x NE555 oscillator, System80 sound only board on later games
************************************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "gts1.h"
#include "gts80.h"
#include "sndbrd.h"
#include "gts80s.h"
#include "cpu/pps4/pps4.h"

#if 0
#define TRACE(x) logerror x
#else
#define TRACE(x) 
#endif

#if defined(PINMAME) && defined(LISY_SUPPORT)
 #include "lisy/lisy1.h"
#endif /* PINMAME && LISY_SUPPORT */


/*----------------
/  Local variables
/-----------------*/
static struct {
	int    vblankCount;
	int    strobe, swStrobe;
	UINT8  accu, lampData, ramE2, ramRW, ramAddr;
	UINT16 pgolAddress;
	UINT32 solenoids;
	int    tones;

	int Data_Reg_A_count;
	int Data_Reg_B_count;
} locals;

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(GTS1_vblank) {

#ifdef LISY_SUPPORT
    // Keep the LISY80 tickled each time we run around the interrupt
    // so it knows we are still alive
    // we use it also to handle special functions for play & test switch
    lisy1TickleWatchdog();
#endif
	locals.vblankCount++;
	/*-- lamps --*/
	if ((locals.vblankCount % GTS1_LAMPSMOOTH) == 0)
		memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.lampMatrix));
	/*-- solenoids --*/
	coreGlobals.solenoids = locals.solenoids;

	core_updateSw(core_getSol(17));
}

static SWITCH_UPDATE(GTS1) {
//with LISY80(LISY1) we read the switches from Lisy rather from the keyboard
//but we keep it as it is for the moment
	if (inports) {
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>8,0x80,0);
		CORE_SETKEYSW(inports[GTS1_COMINPORT],   0x01,1);
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>1,0x01,2);
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>2,0x01,3);
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>3,0x01,4);
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>4,0x01,5);
		CORE_SETKEYSW(inports[GTS1_COMINPORT]>>4,0xa0,7);
	}
	if (core_gameData->hw.soundBoard)
		sndbrd_0_diag(coreGlobals.swMatrix[0] & 0x80);
}

static int GTS1_sw2m(int no) {
  if (no < 0) return no + 8;
  else if (no > 77) return no + 7;
  else { no += 1; return (no%10)*8 + no/10; }
}

static int GTS1_m2sw(int col, int row) {
  if (col < 1) return -9 + row;
  else if (col > 8) return col*8 + row-7;
  else return row*10+col-1;
}

static WRITE_HANDLER(snd_w) {
	data = ((coreGlobals.tmpLampMatrix[0] & 0x01) << 6) // game not over
	  | ((coreGlobals.tmpLampMatrix[0] & 0x02) << 2) // tilt
	  | (locals.solenoids & 0x04) // 10's chime
	  | ((locals.solenoids & 0x08) >> 2) // 100's chime
	  | ((locals.solenoids & 0x10) >> 4) // 1000's chime
	  | (core_getDip(3) & 0x90); // sound dips
	sndbrd_0_data_w(offset, data);
}

static WRITE_HANDLER(lamp_w) {
	int snd = 0;
	data ^= 0x0f;
	if (!offset) {
		// set game enable solenoid
		locals.solenoids = (locals.solenoids & 0xfffeffff) | ((data & 0x03) == 0x01 ? 0x10000 : 0);
		if (core_gameData->hw.soundBoard && (coreGlobals.tmpLampMatrix[0] & 0x03) != (data & 0x03))
			snd = 1;
	}
	if (offset % 2)
		coreGlobals.tmpLampMatrix[offset/2] = (coreGlobals.tmpLampMatrix[offset/2] & 0x0f) | (data << 4);
	else
		coreGlobals.tmpLampMatrix[offset/2] = (coreGlobals.tmpLampMatrix[offset/2] & 0xf0) | data;
	if (snd || core_gameData->hw.soundBoard != SNDBRD_TABART2)
		snd_w(1, 0);
}

static WRITE_HANDLER(disp_w) {
	coreGlobals.segments[offset].w = offset > 23 ? core_bcd2seg7a[data] : core_bcd2seg9a[data];
#ifdef LISY_SUPPORT
	//send display data to LISY1
	lisy1_display_handler(offset, data);
#endif
}

static WRITE_HANDLER(port_w) {
	int device = offset >> 4;
	int ioport = offset & 0x0f;
	int sos = data & 0x10;
	int enable = (data & 0x08) ? 1 : 0;
	int rw = data & 0x40;
	int group = 0x07 ^ (((data & 0x80) >> 5) | ((data & 0x30) >> 4));
	locals.accu = data & 0x0f;
	switch (device) {
		case 0x02: // U5 RRIO A1752 - Switch matrix
			TRACE(("%03x: I/O on U5, port %x: %s %x\n", activecpu_get_pc(), ioport, sos ? "SOS" : "SES", enable));
			if (ioport < 6 && !enable) {
				locals.swStrobe = ioport + 1; //range is 1..6 here
#ifdef LISY_SUPPORT
				lisy1_throttle(); //slow down in case we are running with lisy
#endif
			} else if (!sos && ioport > 7 && locals.swStrobe) {
				locals.accu &= 0x07;
#ifdef LISY_SUPPORT
				//we read the switches from lisy1 via PIC (buffered)
				coreGlobals.swMatrix[locals.swStrobe] = lisy1_switch_handler(locals.swStrobe);
#endif
				locals.accu |= (coreGlobals.swMatrix[locals.swStrobe] & (1 << (ioport - 8))) ? 0: 0x08;
			} else
				locals.swStrobe = 0;
			sndbrd_0_ctrl_w(0, locals.swStrobe); // C. Tabart games read the switch returns for the current strobe
			break;
		case 0x03: // U3 GPIO 10696 - Lamps, strobe, dip switches, bits 8 & 9 of PGOL address
			if (rw) {
				if (group & 1) // outputs 1 - 4: lamp data
				{
					locals.lampData = locals.accu;
#ifdef LISY_SUPPORT
					lisy1_lamp_handler( locals.accu, 1); 
#endif
				}
				if (group & 2) // outputs 5 - 8: data strobe
				{
					locals.strobe = locals.accu < 12 ? locals.accu : -1;
#ifdef LISY_SUPPORT
					lisy1_lamp_handler( locals.accu, 0); 
#endif
					if (locals.strobe > 0) lamp_w(locals.strobe - 1, locals.lampData);
				}
				if (group & 4) { // outputs 9 & 10: PGOL address bits
					locals.pgolAddress = (locals.pgolAddress & 0x0ff) | ((locals.accu & 3) << 8);
				}
			} else {
				locals.accu = 0;
				if (locals.strobe > -1 && locals.strobe < 3) {
					if (group & 1) // inputs 1 - 4: dips
					{
#ifndef LISY_SUPPORT
						locals.accu ^= (core_getDip(locals.strobe) & 0x0f);
#else
						locals.accu ^= (lisy1_get_mpudips(locals.strobe) & 0x0f);
#endif
					}
					if (group & 2) // inputs 5 - 8: dips
					{
#ifndef LISY_SUPPORT
						locals.accu ^= (core_getDip(locals.strobe) >> 4);
#else
						locals.accu ^= (lisy1_get_mpudips(locals.strobe) >> 4);
#endif
					}
				}
				if (group & 4) // inputs 9 - 12: special switches
				{
#ifdef LISY_SUPPORT
					//special switches come from lisy
					//we read the switches from lisy1 via PIC (buffered)
					coreGlobals.swMatrix[7] = lisy1_switch_handler(7); //request special switches
#endif
					locals.accu = 0x08 ^ (coreGlobals.swMatrix[7] >> 4);
				}
			}
			if (rw && (group & 4))
			{
				TRACE(("%03x: I/O on U3: %s %x: %x\n", activecpu_get_pc(), rw ? "SET" : "READ", group, locals.accu));
			}
			break;
		case 0x04: // U4 RRIO A1753 - Solenoids, NVRAM R/W & enable
			if (ioport != 0x0e)
			{
				TRACE(("%03x: I/O on U4, port %x: %s %x\n", activecpu_get_pc(), ioport, sos ? "SOS" : "SES", enable));
			}
			if (sos) {
				if (ioport < 0x0c) {
					locals.solenoids = (locals.solenoids & ~(1 << ioport)) | (!enable << ioport);
#ifdef LISY_SUPPORT
					//send the solenoid states to LISY1->Coil PIC
					//enable = 1 means off; enable = 0 means ON
					lisy1_solenoid_handler(ioport,enable);
#endif
					if (ioport > 1 && ioport < 5) { // sound
						if (!core_gameData->hw.soundBoard) {
							if (locals.tones) discrete_sound_w(1 << (ioport - 2), !enable);
						} else
							snd_w(0, 0);
					}
				} else if (ioport == 0x0d)
					locals.ramE2 = enable;
				else if (ioport == 0x0e)
					locals.ramRW = enable;
			}
			break;
		case 0x06: // U2 GPIO 10696 - NVRAM in / out
			if (rw) {
				if (group & 1) // ram lo address
					locals.ramAddr = (locals.ramAddr & 0xf0) | locals.accu;
				if (group & 2) // ram hi address
					locals.ramAddr = (locals.ramAddr & 0x0f) | (locals.accu << 4);
				if (!locals.ramE2 && locals.ramRW && (group & 4)) // write to nvram
				{
					cpu_writemem16(0x1800 + locals.ramAddr, locals.accu);
				}
//					memory_region(GTS1_MEMREG_CPU)[0x1800 + locals.ramAddr] = locals.accu;
			} else {
				locals.accu = 0x0f;
				if (!locals.ramE2 && (group & 1)) // read from nvram
				{
					locals.accu = cpu_readmem16(0x1800 + locals.ramAddr);
				}
//					locals.accu = memory_region(GTS1_MEMREG_CPU)[0x1800 + locals.ramAddr];
			}
			TRACE(("%03x: I/O on U2: %s %x: %x\n", activecpu_get_pc(), rw ? "SET" : "READ", group, locals.accu));
			break;
		case 0x0d: // U6 GPKD 10788 - Display
			TRACE(("%03x: I/O on U6: %04x:%x\n", activecpu_get_pc(), data, locals.accu));
			switch (data >> 4) { //this is the command for the display chip
				case 3: // turn on display
					// not implemented, do we need it?
					break;
				case 7: // Blank the display of DB3 and DB4
					// not implemented, do we need it?
					break;
				case 0xb: // Blank the display of DA1, DA2, Da3, DA4, DB1, DB2
					// not implemented, do we need it?
					break;
				case 0xd: // Data Bus I/D5, I/D6,I/D7,I/D9 -> Data Reg. B
					switch(locals.Data_Reg_B_count) {
						case 3: case 4: case 5: case 6: case 7: case 8:
							disp_w(locals.Data_Reg_B_count - 3, locals.accu); //0 ... 5: player 3
							break;
						case 11: case 12: case 13: case 14: case 15: case 16:
							disp_w(locals.Data_Reg_B_count - 5, locals.accu); // 6 ... 11: player 4
							break;
					}
					locals.Data_Reg_B_count--;
					//we will receive 16 updates each time
					if (locals.Data_Reg_B_count <= 0) locals.Data_Reg_B_count = 16;
					break;
				case 0xe: //  Data Bus I/D5, I/D6,I/D7,I/D9 -> Data Reg. A
					switch(locals.Data_Reg_A_count) {
						case 1: case 2:
							disp_w(locals.Data_Reg_A_count + 39, locals.accu); //40 ... 41 status display  'ball in play' 
							break;
						case 3: case 4: case 5: case 6: case 7: case 8:
							disp_w(locals.Data_Reg_A_count + 9, locals.accu); //12 ... 17: player 1
							break;
						case 9: case 10:
							disp_w(locals.Data_Reg_A_count + 23, locals.accu); //32 ... 33 system1 status display credits
							break;
						case 11: case 12: case 13: case 14: case 15: case 16:
							disp_w(locals.Data_Reg_A_count + 7, locals.accu); //18 ... 23: player 2
							break;
					}
					locals.Data_Reg_A_count--;
					//we will receive 16 updates each time
					if (locals.Data_Reg_A_count <= 0) locals.Data_Reg_A_count = 16;
					break;
				default:
					TRACE(("%03x: unknown command for display %x: %02x\n", activecpu_get_pc(), data >> 4, locals.accu));
			}
			break;
		default:
			TRACE(("%03x: Write to unknown periphal chip %x: %02x\n", activecpu_get_pc(), device, data));
	}
}
static READ_HANDLER(port_r) {
	return locals.accu;
}

static WRITE_HANDLER(pgol_w) {
	data ^= 0x0f;
	if (offset) locals.pgolAddress = (locals.pgolAddress & 0x30f) | (data << 4);
	else        locals.pgolAddress = (locals.pgolAddress & 0x3f0) | data;
}
static READ_HANDLER(pgol_r) {
	UINT8 code = 0x0f & cpu_readmem16(0x2000 + locals.pgolAddress);
//	UINT8 code = 0x0f & memory_region(GTS1_MEMREG_CPU)[0x2000 + locals.pgolAddress];
	TRACE(("%03x: Reading PGOL prom @ %03x: %x\n", activecpu_get_pc(), 0x100 + locals.pgolAddress, code));
	return code;
}

/* port read / write */
static PORT_READ_START(GTS1_readport)
	{0x000,0x0ff,	port_r},
	{0x100,0x100,	pgol_r},
PORT_END

static PORT_WRITE_START(GTS1_writeport)
	{0x000,0x0ff,	port_w},
	{0x100,0x101,	pgol_w},
PORT_END

/*-----------------------------------------
/  Memory map for System1 CPU board
/------------------------------------------*/
static MEMORY_READ_START(GTS1_readmem)
	{0x0000,0x0fff,	MRA_ROM},
	{0x1000,0x10ff,	MRA_RAM},
	{0x1800,0x18ff,	MRA_RAM},
	{0x2000,0x23ff,	MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(GTS1_writemem)
	{0x1000,0x10ff,	MWA_RAM},
	{0x1800,0x18ff,	MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static MACHINE_INIT(GTS1) {
	memset(&locals, 0, sizeof locals);
	locals.Data_Reg_A_count = 16;
	locals.Data_Reg_B_count = 16;
	if (core_gameData->hw.soundBoard)
		sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);
}

static MACHINE_INIT(GTS1T) {
	memset(&locals, 0, sizeof locals);
	locals.Data_Reg_A_count = 16;
	locals.Data_Reg_B_count = 16;
	locals.tones = 1;
}

static MACHINE_RESET(GTS1) {
}

static MACHINE_RESET(GTS1T) {
}

static MACHINE_STOP(GTS1) {
	if (core_gameData->hw.soundBoard)
		sndbrd_0_exit();
}

static int type[1] = {0};
static DISCRETE_SOUND_START(b18555_discInt)
	DISCRETE_INPUT(NODE_01,1,0x0007,0)
	DISCRETE_INPUT(NODE_02,2,0x0007,0)
	DISCRETE_INPUT(NODE_03,4,0x0007,0)
	DISCRETE_555_ASTABLE(NODE_10,NODE_01,12.0,RES_K(2.7),RES_K(120),CAP_N(10),NODE_NC,type)
	DISCRETE_555_ASTABLE(NODE_20,NODE_02,12.0,RES_K(2.7),RES_K(270),CAP_N(10),NODE_NC,type)
	DISCRETE_555_ASTABLE(NODE_30,NODE_03,12.0,RES_K(2.7),RES_K(470),CAP_N(10),NODE_NC,type)
	DISCRETE_GAIN(NODE_40,NODE_10,1250)
	DISCRETE_GAIN(NODE_50,NODE_20,1250)
	DISCRETE_GAIN(NODE_60,NODE_30,1250)
	DISCRETE_ADDER3(NODE_70,1,NODE_40,NODE_50,NODE_60)
	DISCRETE_OUTPUT(NODE_70, 50)
DISCRETE_SOUND_END

static MACHINE_DRIVER_START(GTS1NS)
	MDRV_IMPORT_FROM(PinMAME)
	MDRV_CPU_ADD_TAG("mcpu", PPS4, 3579545./18.)
	MDRV_CPU_MEMORY(GTS1_readmem, GTS1_writemem)
	MDRV_CPU_PORTS(GTS1_readport,GTS1_writeport)
	MDRV_CPU_VBLANK_INT(GTS1_vblank, 1)
	MDRV_CORE_INIT_RESET_STOP(GTS1,GTS1,GTS1)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_SWITCH_UPDATE(GTS1)
	MDRV_SWITCH_CONV(GTS1_sw2m,GTS1_m2sw)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GTS1C)
	MDRV_IMPORT_FROM(GTS1NS)
	MDRV_DIPS(24)
#ifdef ENABLE_MECHANICAL_SAMPLES
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
#endif
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GTS1T)
	MDRV_IMPORT_FROM(GTS1NS)
	MDRV_CORE_INIT_RESET_STOP(GTS1T,GTS1T,GTS1)
	MDRV_DIPS(24)
	MDRV_SOUND_ADD(DISCRETE, b18555_discInt)
#ifdef ENABLE_MECHANICAL_SAMPLES
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
#endif
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GTS1S80)
	MDRV_IMPORT_FROM(GTS1NS)
	MDRV_DIPS(26)
	MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

extern MACHINE_DRIVER_EXTERN(TABART1);
MACHINE_DRIVER_START(GTS1TAB)
	MDRV_IMPORT_FROM(GTS1NS)
	MDRV_DIPS(26)
	MDRV_IMPORT_FROM(TABART1)
MACHINE_DRIVER_END

extern MACHINE_DRIVER_EXTERN(TABART2);
MACHINE_DRIVER_START(GTS1TAB2)
	MDRV_IMPORT_FROM(GTS1NS)
	MDRV_DIPS(28)
	MDRV_IMPORT_FROM(TABART2)
MACHINE_DRIVER_END
