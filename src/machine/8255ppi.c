/* INTEL 8255 PPI I/O chip */


/* NOTE: When port is input, then data present on the ports
   outputs is 0xff */

/* KT 10/01/2000 - Added bit set/reset feature for control port */
/*               - Added more accurate port i/o data handling */
/*               - Added output reset when control mode is programmed */



#include "driver.h"
#include "machine/8255ppi.h"


static int num;

typedef struct
{
	mem_read_handler portAread;
	mem_read_handler portBread;
	mem_read_handler portCread;
	mem_write_handler portAwrite;
	mem_write_handler portBwrite;
	mem_write_handler portCwrite;
	int groupA_mode;
	int groupB_mode;
	int io[3];		/* input output status */
	int latch[3];	/* data written to ports */
} ppi8255;

static ppi8255 chips[MAX_8255];

static void set_mode(int which, int data, int call_handlers);


void ppi8255_init( ppi8255_interface *intfce )
{
	int i;


	num = intfce->num;

	for (i = 0; i < num; i++)
	{
		chips[i].portAread = intfce->portAread[i];
		chips[i].portBread = intfce->portBread[i];
		chips[i].portCread = intfce->portCread[i];
		chips[i].portAwrite = intfce->portAwrite[i];
		chips[i].portBwrite = intfce->portBwrite[i];
		chips[i].portCwrite = intfce->portCwrite[i];

		set_mode(i, 0x1b, 0);	/* Mode 0, all ports set to input */
	}
}


int ppi8255_r( int which, int offset )
{
	ppi8255 *chip;


	/* some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", cpu_get_pc());
		return 0xff;
	}

	chip = &chips[which];


	if (offset > 3)
	{
		logerror("Attempting to access an invalid 8255 register.  PC: %04X\n", cpu_get_pc());
		return 0xff;
	}


	switch(offset)
	{
	case 0: /* Port A read */
		if (chip->io[0] == 0)
			return chip->latch[0];	/* output */
		else
			if (chip->portAread)  return (*chip->portAread)(0);	/* input */
		break;

	case 1: /* Port B read */
		if (chip->io[1] == 0)
			return chip->latch[1];	/* output */
		else
			if (chip->portBread)  return (*chip->portBread)(0);	/* input */
		break;

	case 2: /* Port C read */
		if (chip->io[2] == 0)
			return chip->latch[2];	/* output */
		else
			/* return data - combination of input and latched output depending on
			   the input/output status of each half of port C */
			if (chip->portCread)  return ((chip->latch[2] & ~chip->io[2]) | ((*chip->portCread)(0) & chip->io[2]));
		break;

	case 3: /* Control word */
		return 0xff;
	}

	logerror("8255 chip %d: Port %c is being read but has no handler.  PC: %04X\n", which, 'A' + offset, cpu_get_pc());

	return 0xff;
}



#define PPI8255_PORT_A_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[0] & ~chip->io[0]) |		\
				 (0xff & chip->io[0]);					\
														\
	if (chip->portAwrite)								\
		(*chip->portAwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port A is being written to but has no handler.  PC: %08X - %02X\n", which, cpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_B_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[1] & ~chip->io[1]) |		\
				 (0xff & chip->io[1]);					\
														\
	if (chip->portBwrite)								\
		(*chip->portBwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, cpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_C_WRITE()							\
{														\
	int write_data;										\
														\
	write_data = (chip->latch[2] & ~chip->io[2]) |		\
				 (0xff & chip->io[2]);					\
														\
	if (chip->portCwrite)								\
		(*chip->portCwrite)(0, write_data);				\
	else												\
		logerror("8255 chip %d: Port C is being written to but has no handler.  PC: %08X - %02X\n", which, cpu_get_pc(), write_data);	\
}


void ppi8255_w( int which, int offset, int data )
{
	ppi8255	*chip;


	/* Some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", cpu_get_pc());
		return;
	}

	chip = &chips[which];


	if (offset > 3)
	{
		logerror("Attempting to access an invalid 8255 register.  PC: %04X\n", cpu_get_pc());
		return;
	}


	switch( offset )
	{
	case 0: /* Port A write */
		chip->latch[0] = data;
		PPI8255_PORT_A_WRITE();
		break;

	case 1: /* Port B write */
		chip->latch[1] = data;
		PPI8255_PORT_B_WRITE();
		break;

	case 2: /* Port C write */
		chip->latch[2] = data;
		PPI8255_PORT_C_WRITE();
		break;

	case 3: /* Control word */
		if ( data & 0x80 )
		{
			set_mode(which, data & 0x7f, 1);
		}
		else
		{
			/* bit set/reset */
			int bit;

			bit = (data >> 1) & 0x07;

			if (data & 1)
				chip->latch[2] |= (1<<bit);		/* set bit */
			else
				chip->latch[2] &= ~(1<<bit);	/* reset bit */

			if (chip->portCwrite)  PPI8255_PORT_C_WRITE();
		}
	}
}

#ifdef MESS
data8_t ppi8255_peek( int which, offs_t offset )
{
	ppi8255 *chip;


	/* Some bounds checking */
	if (which > num)
	{
		logerror("Attempting to access an unmapped 8255 chip.  PC: %04X\n", cpu_get_pc());
		return 0xff;
	}

	chip = &chips[which];


	if (offset > 2)
	{
		logerror("Attempting to access an invalid 8255 port.  PC: %04X\n", cpu_get_pc());
		return 0xff;
	}


	chip = &chips[which];

	return chip->latch[offset];
}
#endif


void ppi8255_set_portAread( int which, mem_read_handler portAread)
{
	ppi8255 *chip = &chips[which];

	chip->portAread = portAread;
}

void ppi8255_set_portBread( int which, mem_read_handler portBread)
{
	ppi8255 *chip = &chips[which];

	chip->portBread = portBread;
}

void ppi8255_set_portCread( int which, mem_read_handler portCread)
{
	ppi8255 *chip = &chips[which];

	chip->portCread = portCread;
}


void ppi8255_set_portAwrite( int which, mem_write_handler portAwrite)
{
	ppi8255 *chip = &chips[which];

	chip->portAwrite = portAwrite;
}

void ppi8255_set_portBwrite( int which, mem_write_handler portBwrite)
{
	ppi8255 *chip = &chips[which];

	chip->portBwrite = portBwrite;
}

void ppi8255_set_portCwrite( int which, mem_write_handler portCwrite)
{
	ppi8255 *chip = &chips[which];

	chip->portCwrite = portCwrite;
}


static void set_mode(int which, int data, int call_handlers)
{
	ppi8255 *chip;



	chip = &chips[which];

	chip->groupA_mode = ( data >> 5 ) & 3;
	chip->groupB_mode = ( data >> 2 ) & 1;

	if ((chip->groupA_mode != 0) || (chip->groupB_mode != 0))
	{
		logerror("8255 chip %d: Setting an unsupported mode %02X.  PC: %04X\n", which, data & 0x62, cpu_get_pc());
		return;
	}

	/* Port A direction */
	if ( data & 0x10 )
		chip->io[0] = 0xff;		/* input */
	else
		chip->io[0] = 0x00;		/* output */

	/* Port B direction */
	if ( data & 0x02 )
		chip->io[1] = 0xff;
	else
		chip->io[1] = 0x00;

	/* Port C upper direction */
	if ( data & 0x08 )
		chip->io[2] |= 0xf0;
	else
		chip->io[2] &= 0x0f;

	/* Port C lower direction */
	if ( data & 0x01 )
		chip->io[2] |= 0x0f;
	else
		chip->io[2] &= 0xf0;

	/* KT: 25-Dec-99 - 8255 resets latches when mode set */
	chip->latch[0] = chip->latch[1] = chip->latch[2] = 0;

	if (call_handlers)
	{
		if (chip->portAwrite)  PPI8255_PORT_A_WRITE();
		if (chip->portBwrite)  PPI8255_PORT_B_WRITE();
		if (chip->portCwrite)  PPI8255_PORT_C_WRITE();
	}
}


/* Helpers */
READ_HANDLER( ppi8255_0_r ) { return ppi8255_r( 0, offset ); }
READ_HANDLER( ppi8255_1_r ) { return ppi8255_r( 1, offset ); }
READ_HANDLER( ppi8255_2_r ) { return ppi8255_r( 2, offset ); }
READ_HANDLER( ppi8255_3_r ) { return ppi8255_r( 3, offset ); }
WRITE_HANDLER( ppi8255_0_w ) { ppi8255_w( 0, offset, data ); }
WRITE_HANDLER( ppi8255_1_w ) { ppi8255_w( 1, offset, data ); }
WRITE_HANDLER( ppi8255_2_w ) { ppi8255_w( 2, offset, data ); }
WRITE_HANDLER( ppi8255_3_w ) { ppi8255_w( 3, offset, data ); }
