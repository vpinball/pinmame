/***************************************************************************

	Generic (PC-style) IDE controller implementation

***************************************************************************/

#include "idectrl.h"


/*************************************
 *
 *	Debugging
 *
 *************************************/

#define PRINTF_IDE_COMMANDS			0

#define VERBOSE
#ifdef VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(X)
#endif



/*************************************
 *
 *	Constants
 *
 *************************************/

#define TIME_PER_SECTOR				(TIME_IN_USEC(100))

#define IDE_STATUS_ERROR			0x01
#define IDE_STATUS_BUFFER_READY		0x08
#define IDE_STATUS_SEEK_COMPLETE	0x10
#define IDE_STATUS_DRIVE_READY		0x40
#define IDE_STATUS_BUSY				0x80

#define IDE_CONFIG_REGISTERS		0x10

#define IDE_ADDR_CONFIG_UNK			0x034
#define IDE_ADDR_CONFIG_REGISTER	0x038
#define IDE_ADDR_CONFIG_DATA		0x03c

#define IDE_ADDR_DATA				0x1f0
#define IDE_ADDR_ERROR				0x1f1
#define IDE_ADDR_SECTOR_COUNT		0x1f2
#define IDE_ADDR_SECTOR_NUMBER		0x1f3
#define IDE_ADDR_CYLINDER_LSB		0x1f4
#define IDE_ADDR_CYLINDER_MSB		0x1f5
#define IDE_ADDR_HEAD_NUMBER		0x1f6
#define IDE_ADDR_STATUS_COMMAND		0x1f7

#define IDE_ADDR_STATUS_CONTROL		0x3f6

#define IDE_COMMAND_READ_MULTIPLE	0x20
#define IDE_COMMAND_WRITE_MULTIPLE	0x30
#define IDE_COMMAND_SET_CONFIG		0x91
#define IDE_COMMAND_GET_INFO		0xec

#define IDE_ERROR_NONE				0x00
#define IDE_ERROR_UNKNOWN_COMMAND	0x04
#define IDE_ERROR_BAD_LOCATION		0x10
#define IDE_ERROR_BAD_SECTOR		0x80



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct ide_state
{
	UINT8	adapter_control;
	UINT8	status;
	UINT8	error;
	UINT8	interrupt_pending;

	UINT8	buffer[HARD_DISK_SECTOR_SIZE];
	UINT16	buffer_offset;
	UINT16	sector_count;

	UINT16	cur_cylinder;
	UINT8	cur_sector;
	UINT8	cur_head;
	UINT8	cur_head_reg;

	UINT16	num_cylinders;
	UINT8	num_sectors;
	UINT8	num_heads;

	UINT8	config_unknown;
	UINT8	config_register[IDE_CONFIG_REGISTERS];
	UINT8	config_register_num;

	struct ide_interface *intf;
	void *	disk;
};



/*************************************
 *
 *	Local variables
 *
 *************************************/

static struct ide_state idestate[MAX_IDE_CONTROLLERS];



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void *open_source_and_diff(const char *diskname);



/*************************************
 *
 *	Interrupts
 *
 *************************************/

INLINE void signal_interrupt(struct ide_state *ide)
{
	/* signal an interrupt */
	if (ide->intf->interrupt)
		(*ide->intf->interrupt)(ASSERT_LINE);
	ide->interrupt_pending = 1;
}


INLINE void clear_interrupt(struct ide_state *ide)
{
	/* clear an interrupt */
	if (ide->intf->interrupt)
		(*ide->intf->interrupt)(CLEAR_LINE);
	ide->interrupt_pending = 0;
}



/*************************************
 *
 *	Initialization & reset
 *
 *************************************/

int ide_controller_init(int which, struct ide_interface *intf, const char *diskname1, const char *diskname2)
{
	struct ide_state *ide = &idestate[which];
	const struct hard_disk_header *header;

	/* NULL interface is immediate failure */
	if (!intf)
		return 1;

	/* only one hard disk is supported currently */
	if (diskname2)
		return 1;

	/* reset the IDE state */
	memset(ide, 0, sizeof(*ide));
	ide->intf = intf;

	/* attempt to open the first disk */
	if (diskname1)
		ide->disk = open_source_and_diff(diskname1);

	/* get and copy the geometry */
	if (ide->disk)
	{
		header = hard_disk_get_header(ide->disk);
		ide->num_cylinders = header->cylinders;
		ide->num_sectors = header->sectors;
		ide->num_heads = header->heads;
	}
	return 0;
}


void ide_controller_reset(int which)
{
	struct ide_state *ide = &idestate[which];

	/* reset the drive state */
	ide->status = IDE_STATUS_DRIVE_READY | IDE_STATUS_SEEK_COMPLETE;
	ide->error = IDE_ERROR_NONE;
	ide->buffer_offset = 0;
	clear_interrupt(ide);
}



static void reset_callback(int param)
{
	ide_controller_reset(param);
}



/*************************************
 *
 *	Open source hard drive and diff
 *
 *************************************/

static void *open_source_and_diff(const char *diskname)
{
	struct hard_disk_header header;
	char filename[1024], *c;
	void *source, *diff;
	int err;

	/* make the filename of the source */
	strcpy(filename, diskname);
	c = strrchr(filename, '.');
	if (c)
		strcpy(c, ".chd");
	else
		strcat(filename, ".chd");

	/* first open the source drive */
	source = hard_disk_open(filename, 0, NULL);
	if (!source)
		return NULL;

	/* make the filename of the diff */
	strcpy(filename, diskname);
	c = strrchr(filename, '.');
	if (c)
		strcpy(c, ".dif");
	else
		strcat(filename, ".dif");

	/* try to open the diff */
	diff = hard_disk_open(filename, 1, source);
	if (diff)
		return diff;

	/* didn't work; try creating it instead */

	/* first get the parent's header and modify that */
	header = *hard_disk_get_header(source);
	header.flags |= HDFLAGS_HAS_PARENT | HDFLAGS_IS_WRITEABLE;
	header.compression = HDCOMPRESSION_NONE;
	memcpy(header.parentmd5, header.md5, sizeof(header.parentmd5));
	memset(header.md5, 0, sizeof(header.md5));

	/* then do the create; if it fails, just fall back on the source */
	err = hard_disk_create(filename, &header);
	if (err != HDERR_NONE)
		return source;

	/* now try opening the drive */
	diff = hard_disk_open(filename, 1, source);
	if (diff)
		return diff;

	return source;
}



/*************************************
 *
 *	Convert offset/mem_mask to offset
 *	and size
 *
 *************************************/

INLINE int convert_to_offset_and_size(offs_t *offset, data32_t mem_mask)
{
	int size = 4;

	/* determine which real offset */
	if (mem_mask & 0x000000ff)
	{
		(*offset)++, size = 3;
		if (mem_mask & 0x0000ff00)
		{
			(*offset)++, size = 2;
			if (mem_mask & 0x00ff0000)
				(*offset)++, size = 1;
		}
	}

	/* determine the real size */
	if (!(mem_mask & 0xff000000))
		return size;
	size--;
	if (!(mem_mask & 0x00ff0000))
		return size;
	size--;
	if (!(mem_mask & 0x0000ff00))
		return size;
	size--;
	return size;
}



/*************************************
 *
 *	Advance to the next sector
 *
 *************************************/

INLINE void next_sector(struct ide_state *ide)
{
	/* sectors are 1-based */
	ide->cur_sector++;
	if (ide->cur_sector > ide->num_sectors)
	{
		/* heads are 0 based */
		ide->cur_sector = 1;
		ide->cur_head++;
		if (ide->cur_head >= ide->num_heads)
		{
			ide->cur_head = 0;
			ide->cur_cylinder++;
		}
	}
}



/*************************************
 *
 *	Compute the LBA address
 *
 *************************************/

INLINE UINT32 lba_address(struct ide_state *ide)
{
	return (ide->cur_cylinder * ide->num_heads + ide->cur_head) * ide->num_sectors + ide->cur_sector - 1;
}



/*************************************
 *
 *	Build a features page
 *
 *************************************/

static void ide_build_features(struct ide_state *ide)
{
	int bytes_per_track = ide->num_sectors * HARD_DISK_SECTOR_SIZE;

	memset(ide->buffer, 0, HARD_DISK_SECTOR_SIZE);

	/* basic geometry */
	ide->buffer[ 0] = 0x5a;							/* configuration bits */
	ide->buffer[ 1] = 0x04;
	ide->buffer[ 2] = ide->num_cylinders & 0xff;	/* cylinders */
	ide->buffer[ 3] = ide->num_cylinders >> 8;
	ide->buffer[ 4] = 0;							/* unused */
	ide->buffer[ 5] = 0;
	ide->buffer[ 6] = ide->num_heads & 0xff;		/* heads */
	ide->buffer[ 7] = ide->num_heads >> 8;
	ide->buffer[ 8] = bytes_per_track & 0xff;		/* bytes per track (obsolete) */
	ide->buffer[ 9] = bytes_per_track >> 8;
	ide->buffer[10] = HARD_DISK_SECTOR_SIZE & 0xff;	/* bytes per sector (obsolete) */
	ide->buffer[11] = HARD_DISK_SECTOR_SIZE >> 8;
	ide->buffer[12] = ide->num_sectors & 0xff;		/* sectors */
	ide->buffer[13] = ide->num_sectors >> 8;
	ide->buffer[14] = 0;							/* vendor-specific */
	ide->buffer[15] = 0;

	/* serial number */
	memset(&ide->buffer[20], ' ', 20);
}



/*************************************
 *
 *	Sector reading
 *
 *************************************/

static void read_sector_done(int which)
{
	struct ide_state *ide = &idestate[which];
	int lba = lba_address(ide), count = 0;

	/* now do the read */
	if (ide->disk)
		count = hard_disk_read(ide->disk, lba, 1, ide->buffer);

	/* by default, mark the buffer ready and the seek complete */
	ide->status |= IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_SEEK_COMPLETE;

	/* and clear the busy adn error flags */
	ide->status &= ~IDE_STATUS_ERROR;
	ide->status &= ~IDE_STATUS_BUSY;

	/* if we succeeded, advance to the next sector and set the nice bits */
	if (count == 1)
	{
		/* advance the pointers */
		next_sector(ide);

		/* clear the error value */
		ide->error = IDE_ERROR_NONE;

		/* signal an interrupt */
		signal_interrupt(ide);
	}

	/* if we got an error, we need to report it */
	else
	{
		/* set the error flag and the error */
		ide->status |= IDE_STATUS_ERROR;
		ide->error = IDE_ERROR_BAD_SECTOR;

		/* signal an interrupt */
		signal_interrupt(ide);
	}
}


static void read_next_sector(struct ide_state *ide)
{
	/* just set a timer and mark ourselves busy */
	timer_set(TIME_PER_SECTOR, ide - idestate, read_sector_done);
	ide->status |= IDE_STATUS_BUSY;
}



/*************************************
 *
 *	Sector writing
 *
 *************************************/

static void write_sector_done(int which)
{
	struct ide_state *ide = &idestate[which];
	int lba = lba_address(ide), count = 0;

	/* now do the write */
	if (ide->disk)
		count = hard_disk_write(ide->disk, lba, 1, ide->buffer);

	/* by default, mark the buffer ready and the seek complete */
	ide->status |= IDE_STATUS_BUFFER_READY;
	ide->status |= IDE_STATUS_SEEK_COMPLETE;

	/* and clear the busy adn error flags */
	ide->status &= ~IDE_STATUS_ERROR;
	ide->status &= ~IDE_STATUS_BUSY;

	/* if we succeeded, advance to the next sector and set the nice bits */
	if (count == 1)
	{
		/* advance the pointers */
		next_sector(ide);

		/* clear the error value */
		ide->error = IDE_ERROR_NONE;

		/* signal an interrupt if there's more data needed */
		if (ide->sector_count > 0)
			ide->sector_count--;
		if (ide->sector_count == 0)
			ide->status &= ~IDE_STATUS_BUFFER_READY;
		signal_interrupt(ide);
	}

	/* if we got an error, we need to report it */
	else
	{
		/* set the error flag and the error */
		ide->status |= IDE_STATUS_ERROR;
		ide->error = IDE_ERROR_BAD_SECTOR;

		/* signal an interrupt */
		signal_interrupt(ide);
	}
}


static void write_cur_sector(struct ide_state *ide)
{
	/* just set a timer and mark ourselves busy */
	timer_set(TIME_PER_SECTOR, ide - idestate, write_sector_done);
	ide->status |= IDE_STATUS_BUSY;
}



/*************************************
 *
 *	Handle IDE commands
 *
 *************************************/

void handle_command(struct ide_state *ide, UINT8 command)
{
	/* implicitly clear interrupts here */
	clear_interrupt(ide);

	switch (command)
	{
		case IDE_COMMAND_READ_MULTIPLE:
			LOG(("IDE Read multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));
#if PRINTF_IDE_COMMANDS
			fprintf(stderr, "IDE Read multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count);
#endif
			/* reset the buffer */
			ide->buffer_offset = 0;

			/* start the read going */
			read_next_sector(ide);
			break;

		case IDE_COMMAND_WRITE_MULTIPLE:
			LOG(("IDE Write multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count));
#if PRINTF_IDE_COMMANDS
			fprintf(stderr, "IDE Write multiple: C=%d H=%d S=%d LBA=%d count=%d\n",
				ide->cur_cylinder, ide->cur_head, ide->cur_sector, lba_address(ide), ide->sector_count);
#endif
			/* reset the buffer */
			ide->buffer_offset = 0;

			/* mark the buffer ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			break;

		case IDE_COMMAND_GET_INFO:
			LOG(("IDE Read features\n"));
#if PRINTF_IDE_COMMANDS
			fprintf(stderr, "IDE Read features\n");
#endif
			/* reset the buffer */
			ide->buffer_offset = 0;
			ide->sector_count = 1;

			/* build the features page */
			ide_build_features(ide);

			/* indicate everything is ready */
			ide->status |= IDE_STATUS_BUFFER_READY;
			ide->status |= IDE_STATUS_SEEK_COMPLETE;

			/* and clear the busy adn error flags */
			ide->status &= ~IDE_STATUS_ERROR;
			ide->status &= ~IDE_STATUS_BUSY;

			/* clear the error too */
			ide->error = IDE_ERROR_NONE;

			/* signal an interrupt */
			signal_interrupt(ide);
			break;

		case IDE_COMMAND_SET_CONFIG:
			LOG(("IDE Set configuration (%d heads, %d sectors)\n", ide->cur_head + 1, ide->sector_count));
#if PRINTF_IDE_COMMANDS
			fprintf(stderr, "IDE Set configuration (%d heads, %d sectors)\n", ide->cur_head + 1, ide->sector_count);
#endif
			ide->num_sectors = ide->sector_count;
			ide->num_heads = ide->cur_head + 1;

			/* signal an interrupt */
			signal_interrupt(ide);
			break;

		default:
			logerror("IDE unknown command (%02X)\n", command);
#if PRINTF_IDE_COMMANDS
			fprintf(stderr, "IDE unknown command (%02X)\n", command);
#endif
			break;
	}
}



/*************************************
 *
 *	IDE controller read
 *
 *************************************/

static UINT32 ide_controller_read(struct ide_state *ide, offs_t offset, int size)
{
	UINT32 result = 0;

	/* logit */
	if (offset != IDE_ADDR_DATA)
		LOG(("%08X:IDE read at %03X, size=%d\n", activecpu_get_previouspc(), offset, size));

	switch (offset)
	{
		/* unknown config register */
		case IDE_ADDR_CONFIG_UNK:
			return ide->config_unknown;

		/* active config register */
		case IDE_ADDR_CONFIG_REGISTER:
			return ide->config_register_num;

		/* data from active config register */
		case IDE_ADDR_CONFIG_DATA:
			if (ide->config_register_num < IDE_CONFIG_REGISTERS)
				return ide->config_register[ide->config_register_num];
			return 0;

		/* read data if there's data to be read */
		case IDE_ADDR_DATA:
			if (ide->status & IDE_STATUS_BUFFER_READY)
			{
				/* fetch the correct amount of data */
				result = ide->buffer[ide->buffer_offset++];
				if (size > 1)
					result |= ide->buffer[ide->buffer_offset++] << 8;
				if (size > 2)
				{
					result |= ide->buffer[ide->buffer_offset++] << 16;
					result |= ide->buffer[ide->buffer_offset++] << 24;
				}

				/* if we're at the end of the buffer, handle it */
				if (ide->buffer_offset >= HARD_DISK_SECTOR_SIZE)
				{
					/* reset the totals */
					ide->buffer_offset = 0;

					/* clear the buffer ready flag */
					ide->status &= ~IDE_STATUS_BUFFER_READY;

					/* if there is more data to read, keep going */
					if (ide->sector_count > 0)
						ide->sector_count--;
					if (ide->sector_count > 0)
						read_next_sector(ide);
				}
			}
			break;

		/* return the current error */
		case IDE_ADDR_ERROR:
			return ide->error;

		/* return the current sector count */
		case IDE_ADDR_SECTOR_COUNT:
			return ide->sector_count;

		/* return the current sector */
		case IDE_ADDR_SECTOR_NUMBER:
			return ide->cur_sector;

		/* return the current cylinder LSB */
		case IDE_ADDR_CYLINDER_LSB:
			return ide->cur_cylinder & 0xff;

		/* return the current cylinder MSB */
		case IDE_ADDR_CYLINDER_MSB:
			return ide->cur_cylinder >> 8;

		/* return the current head */
		case IDE_ADDR_HEAD_NUMBER:
			return ide->cur_head_reg;

		/* return the current status and clear any pending interrupts */
		case IDE_ADDR_STATUS_COMMAND:
			result = ide->status;
			if (ide->interrupt_pending)
				clear_interrupt(ide);
			break;

		/* return the current status but don't clear interrupts */
		case IDE_ADDR_STATUS_CONTROL:
			result = ide->status;
			break;

		/* log anything else */
		default:
			logerror("%08X:unknown IDE read at %03X, size=%d\n", activecpu_get_previouspc(), offset, size);
			break;
	}

	/* return the result */
	return result;
}



/*************************************
 *
 *	IDE controller write
 *
 *************************************/

static void ide_controller_write(struct ide_state *ide, offs_t offset, int size, UINT32 data)
{
	/* logit */
	if (offset != IDE_ADDR_DATA)
		LOG(("%08X:IDE write to %03X = %08X, size=%d\n", activecpu_get_previouspc(), offset, data, size));

	switch (offset)
	{
		/* unknown config register */
		case IDE_ADDR_CONFIG_UNK:
			ide->config_unknown = data;
			break;

		/* active config register */
		case IDE_ADDR_CONFIG_REGISTER:
			ide->config_register_num = data;
			break;

		/* data from active config register */
		case IDE_ADDR_CONFIG_DATA:
			if (ide->config_register_num < IDE_CONFIG_REGISTERS)
				ide->config_register[ide->config_register_num] = data;
			break;

		/* write data */
		case IDE_ADDR_DATA:
			if (ide->status & IDE_STATUS_BUFFER_READY)
			{
				/* store the correct amount of data */
				ide->buffer[ide->buffer_offset++] = data;
				if (size > 1)
					ide->buffer[ide->buffer_offset++] = data >> 8;
				if (size > 2)
				{
					ide->buffer[ide->buffer_offset++] = data >> 16;
					ide->buffer[ide->buffer_offset++] = data >> 24;
				}

				/* if we're at the end of the buffer, handle it */
				if (ide->buffer_offset >= HARD_DISK_SECTOR_SIZE)
				{
					/* reset the totals */
					ide->buffer_offset = 0;

					/* clear the buffer ready flag */
					ide->status &= ~IDE_STATUS_BUFFER_READY;

					/* do the write */
					write_cur_sector(ide);
				}
			}
			break;

		/* precompensation offset?? */
		case IDE_ADDR_ERROR:
			break;

		/* sector count */
		case IDE_ADDR_SECTOR_COUNT:
			ide->sector_count = data ? data : 256;
			break;

		/* current sector */
		case IDE_ADDR_SECTOR_NUMBER:
			ide->cur_sector = data;
			break;

		/* current cylinder LSB */
		case IDE_ADDR_CYLINDER_LSB:
			ide->cur_cylinder = (ide->cur_cylinder & 0xff00) | (data & 0xff);
			break;

		/* current cylinder MSB */
		case IDE_ADDR_CYLINDER_MSB:
			ide->cur_cylinder = (ide->cur_cylinder & 0x00ff) | ((data & 0xff) << 8);
			break;

		/* current head */
		case IDE_ADDR_HEAD_NUMBER:
			ide->cur_head = data & 0x0f;
			ide->cur_head_reg = data;
			// drive index = data & 0x10
			// LBA mode = data & 0x40
			break;

		/* command */
		case IDE_ADDR_STATUS_COMMAND:
			handle_command(ide, data);
			break;

		/* adapter control */
		case IDE_ADDR_STATUS_CONTROL:
			ide->adapter_control = data;

			/* handle controller reset */
			if (data == 0x04)
			{
				ide->status |= IDE_STATUS_BUSY;
				ide->status &= ~IDE_STATUS_DRIVE_READY;
				timer_set(TIME_IN_MSEC(5), ide - idestate, reset_callback);
			}
			break;
	}
}



/*************************************
 *
 *	32-bit IDE handlers
 *
 *************************************/

READ32_HANDLER( ide_controller32_0_r )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size(&offset, mem_mask);

	return ide_controller_read(&idestate[0], offset, size) << ((offset & 3) * 8);
}


WRITE32_HANDLER( ide_controller32_0_w )
{
	int size;

	offset *= 4;
	size = convert_to_offset_and_size(&offset, mem_mask);

	ide_controller_write(&idestate[0], offset, size, data >> ((offset & 3) * 8));
}



/*************************************
 *
 *	16-bit IDE handlers
 *
 *************************************/

READ16_HANDLER( ide_controller16_0_r )
{
	int size;

	offset *= 2;
	size = convert_to_offset_and_size(&offset, mem_mask);

	return ide_controller_read(&idestate[0], offset, size) << ((offset & 1) * 8);
}


WRITE16_HANDLER( ide_controller16_0_w )
{
	int size;

	offset *= 2;
	size = convert_to_offset_and_size(&offset, mem_mask);

	ide_controller_write(&idestate[0], offset, size, data >> ((offset & 1) * 8));
}
