/***************************************************************************

	Generic MAME hard disk implementation, with differencing files

***************************************************************************/

#include "driver.h"


/***************************************************************************

	MAME compressed hard disk header format. All numbers are stored in
	Motorola (big-endian) byte ordering. The header is 76 (V1) or 80 (V2) bytes
	long.

	[  0] char   tag[8];		// 'MComprHD'
	[  8] UINT32 length;		// length of header (including tag and length fields)
	[ 12] UINT32 version;		// drive format version
	[ 16] UINT32 flags;			// flags (see below)
	[ 20] UINT32 compression;	// compression type
	[ 24] UINT32 blocksize;		// sectors per block
	[ 28] UINT32 totalblocks;	// total # of block represented
	[ 32] UINT32 cylinders;		// number of cylinders on hard disk
	[ 36] UINT32 heads;			// number of heads on hard disk
	[ 40] UINT32 sectors;		// number of sectors on hard disk
	[ 44] UINT8  md5[16];		// MD5 checksum for this drive
	[ 60] UINT8  parentmd5[16];	// MD5 checksum for parent drive
	[ 76] (V1 header length)
	[ 76] UINT32 seclen;		// number of bytes per sector (version 2)
	[ 80] (V2 header length)

	Flags:
		0x00000001 - set if this drive has a parent
		0x00000002 - set if this drive allows writes

***************************************************************************/

/*************************************
 *
 *	Constants
 *
 *************************************/

#define HARD_DISK_HEADER_VERSION	1
#define HARD_DISK_V1_HEADER_SIZE	76
#define HARD_DISK_V2_HEADER_SIZE	80
#define HARD_DISK_MAX_HEADER_SIZE	80

#define HDFLAGS_HAS_PARENT			0x00000001
#define HDFLAGS_IS_WRITEABLE		0x00000002

#define HDCOMPRESSION_NONE			0
#define HDCOMPRESSION_ZLIB			1
#define HDCOMPRESSION_MAX			2

enum
{
	HDERR_NONE,
	HDERR_NO_INTERFACE,
	HDERR_OUT_OF_MEMORY,
	HDERR_INVALID_FILE,
	HDERR_INVALID_PARAMETER,
	HDERR_INVALID_DATA,
	HDERR_FILE_NOT_FOUND,
	HDERR_REQUIRES_PARENT,
	HDERR_FILE_NOT_WRITEABLE,
	HDERR_READ_ERROR,
	HDERR_WRITE_ERROR,
	HDERR_CODEC_ERROR,
	HDERR_INVALID_PARENT,
	HDERR_SECTOR_OUT_OF_RANGE,
	HDERR_DECOMPRESSION_ERROR,
	HDERR_COMPRESSION_ERROR,
	HDERR_CANT_CREATE_FILE,
	HDERR_CANT_VERIFY
};



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct hard_disk_header
{
	UINT32	length;				/* length of header data */
	UINT32	version;			/* drive format version */
	UINT32	flags;				/* flags field */
	UINT32	compression;		/* compression type */
	UINT32	blocksize;			/* sectors per block */
	UINT32	totalblocks;		/* total # of blocks represented */
	UINT32	cylinders;			/* number of cylinders on hard disk */
	UINT32	heads;				/* number of heads on hard disk */
	UINT32	sectors;			/* number of sectors per track on hard disk */
	UINT32	seclen;				/* sector lenght in bytes on hard disk */
	UINT8	md5[16];			/* MD5 checksum for this drive */
	UINT8	parentmd5[16];		/* MD5 checksum for parent drive */
};


struct hard_disk_interface
{
	void *(*open)(const char *filename, const char *mode);
	void (*close)(void *file);
	UINT32 (*read)(void *file, UINT64 offset, UINT32 count, void *buffer);
	UINT32 (*write)(void *file, UINT64 offset, UINT32 count, const void *buffer);
};



/*************************************
 *
 *	Prototypes
 *
 *************************************/

void hard_disk_set_interface(struct hard_disk_interface *interface);
void hard_disk_save_interface(struct hard_disk_interface *interface_save);

int hard_disk_create(const char *filename, const struct hard_disk_header *header);
void *hard_disk_open(const char *filename, int writeable, void *parent);
void hard_disk_close(void *disk);
void hard_disk_close_all(void);

UINT32 hard_disk_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer);
UINT32 hard_disk_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer);

int hard_disk_get_last_error(void);
const struct hard_disk_header *hard_disk_get_header(void *disk);
int hard_disk_set_header(const char *filename, const struct hard_disk_header *header);

int hard_disk_compress(const char *rawfile, UINT32 offset, const char *newfile, const struct hard_disk_header *header, const char *difffile, void (*progress)(const char *, ...));
int hard_disk_verify(const char *hdfile, void (*progress)(const char *, ...), UINT8 headermd5[16], UINT8 actualmd5[16]);
