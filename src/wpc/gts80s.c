#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "machine/6530riot.h"
#include "core.h"

#include "gts80.h"
#include "gts80s.h"

static int GTS80S_CPUNo;

/* switches on the board (to PB4/7) */
#define ATTRACT_MODE_TUNE		0x10
#define SOUND_TONES				0x80

#define ATTRACT_ENABLED	1
#define SOUND_ENABLED	1


#define BUFFER_SIZE 8192

typedef struct tagGTS80SL {
  int   stream;
  UINT16 buffer[BUFFER_SIZE+1];
  double clock[BUFFER_SIZE+1];
  
  int   buf_pos;
} GTS80SL;

static GTS80SL GTS80S_locals;

void GTS80S_sound_latch(int data)
{
//	logerror("sound latch: 0x%02x\n", GTS80S_locals.soundLatch);
	riot6530_set_input_b(0, ATTRACT_ENABLED*ATTRACT_MODE_TUNE | SOUND_ENABLED*SOUND_TONES | (data&0x0f));
}

/* configured as output, shouldn't be read */
READ_HANDLER(riot6530_0a_r)  {
	logerror("riot6530_0a_r\n");
	return 0x00;
}

/* digital sound data output */
WRITE_HANDLER(riot6530_0a_w) {
//	logerror("riot6530_0a_w: 0x%02x\n", data);

	if ( GTS80S_locals.buf_pos>=BUFFER_SIZE )
		return;

	GTS80S_locals.clock[GTS80S_locals.buf_pos] = timer_get_time();
	GTS80S_locals.buffer[GTS80S_locals.buf_pos++] = (0x80-data)*0x100;
}

/* configured as input, shouldn't be read */
WRITE_HANDLER(riot6530_0b_w) { 
	logerror("riot6530_0b_w: 0x%02x\n", data);
}

static UINT8 RIOT6532_RAM[0x0200]; 

static READ_HANDLER(riot6530_0_ram_r) {
	return RIOT6532_RAM[offset];
}

static WRITE_HANDLER(riot6530_0_ram_w) {
	RIOT6532_RAM[offset] = data;
}

struct riot6530_interface GTS80S_riot6530_intf = {
 /* 6530RIOT 0 (0x200) Chip U2 */
 /* PA0 - PA7 Digital sound data */
 /* PB0 - PB7 Sound latch & Dip-Switches */
 /* in  : A/B, */ riot6530_0a_r, NULL,
 /* out : A/B, */ riot6530_0a_w, riot6530_0b_w,
 /* irq :      */ NULL
};


/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80S_readmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x02ff, riot6530_0_r},
{ 0x0400, 0x07ff, MRA_ROM},
{ 0x0800, 0x0bff, MRA_ROM},
{ 0x0c00, 0x0fff, MRA_ROM},
{ 0xfc00, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80S_writemem)
{ 0x0000, 0x01ff, MWA_RAM},
{ 0x0200, 0x02ff, riot6530_0_w},
{ 0x0400, 0x07ff, MWA_ROM},
{ 0x0800, 0x0bff, MWA_ROM},
{ 0x0c00, 0x0fff, MWA_ROM},
{ 0xfc00, 0xffff, MWA_ROM},
MEMORY_END

static void GTS80S_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;
		
	dCurrentClock = GTS80S_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80S_locals.clock[0]) / length;

	i = 0;
	GTS80S_locals.clock[GTS80S_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80S_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80S_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80S_locals.clock[0] = dActClock;
	GTS80S_locals.buffer[0] = GTS80S_locals.buffer[GTS80S_locals.buf_pos-1];
	GTS80S_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/
void GTS80S_init(int num) {
	int i = 0;
	UINT8 *pMem;

	GTS80S_CPUNo = num;

	memset(&GTS80S_locals, 0x00, sizeof GTS80S_locals);

	GTS80S_locals.clock[0]  = 0;
	GTS80S_locals.buffer[0] = 0x8000;
	GTS80S_locals.buf_pos   = 1;

	/* init RAM */
	memset(RIOT6532_RAM, 0x00, sizeof RIOT6532_RAM);

	pMem = memory_region(GTS80_MEMREG_SCPU1)+0x0400;
	for(i=0x0400; i<0x0bff; i++) {
		*pMem = (*pMem&0x0f);
		pMem++;
		i++;
	}

	/* init the RIOT */
    riot6530_config(0, &GTS80S_riot6530_intf);
	riot6530_set_clock(0, Machine->drv->cpu[GTS80S_CPUNo].cpu_clock/4);
	riot6530_reset();

	GTS80S_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80S_Update);
	set_RC_filter(GTS80S_locals.stream, 270000, 15000, 0, 33000);
}

void GTS80S_exit() {
	riot6530_unconfig();
}
