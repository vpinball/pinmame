/***************************************************************************

	Generic MAME hard disk implementation, with differencing files

***************************************************************************/

#include "harddisk.h"
#include "md5.h"
#include "zlib.h"
#include <time.h>



/*************************************
 *
 *	Debugging
 *
 *************************************/

#define LOG_DISK_ACCESSES	0



/*************************************
 *
 *	Constants
 *
 *************************************/

#define STACK_ENTRIES		1024		/* 16-bit array size on stack */
#define COOKIE_VALUE		0xbaadf00d
#define MAX_ZLIB_ALLOCS		64

#define END_OF_LIST_COOKIE	"EndOfListCookie"

#define HARD_DISK_V1_SECTOR_SIZE	512



/*************************************
 *
 *	Macros
 *
 *************************************/

#define SET_ERROR_AND_CLEANUP(err) do { last_error = (err); goto cleanup; } while (0)

#ifdef _MSC_VER
#define interface interface_
#endif

/*************************************
 *
 *	Type definitions
 *
 *************************************/

typedef UINT64 mapentry_t;

struct hard_disk_info
{
	UINT32					cookie;		/* cookie, should equal COOKIE_VALUE */
	struct hard_disk_info *	next;		/* pointer to next drive in the global list */

	void *					file;		/* handle to the open file */
	struct hard_disk_header header;		/* header, extracted from drive */

	struct hard_disk_info *	parent;		/* pointer to parent drive, or NULL */

	mapentry_t *			map;		/* array of map entries */
	UINT64					eof;		/* end of file, computed from the blocks */

	UINT8 *					cache;		/* block cache pointer */
	UINT32					cacheblock;	/* index of currently cached block */

	UINT8 *					compressed;	/* pointer to buffer for compressed data */
	void *					codecdata;	/* opaque pointer to codec data */
};

struct zlib_codec_data
{
	z_stream				inflater;
	z_stream				deflater;
	UINT32 *				allocptr[MAX_ZLIB_ALLOCS];
};



/*************************************
 *
 *	Local variables
 *
 *************************************/

static struct hard_disk_interface interface;
static struct hard_disk_info *first_disk;
static int last_error;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static int read_block_into_cache(struct hard_disk_info *info, UINT32 block);
static int write_block_from_cache(struct hard_disk_info *info, UINT32 block);
static int read_header(void *file, struct hard_disk_header *header);
static int write_header(void *file, const struct hard_disk_header *header);
static int read_sector_map(struct hard_disk_info *info);
static int init_codec(struct hard_disk_info *info);
static void free_codec(struct hard_disk_info *info);



/*************************************
 *
 *	Inline helpers
 *
 *************************************/

#if defined(__MWERKS__) && defined(__POWERPC__)
/* workaround for some bug in codewarrior pro 6.3 compiler */
INLINE UINT64 offset_from_mapentry(mapentry_t *entry)
{
	return (*entry) & 0x00000fffffffffffULL;
}
#else
INLINE UINT64 offset_from_mapentry(mapentry_t *entry)
{
	return (*entry << 20) >> 20;
}
#endif

INLINE UINT32 length_from_mapentry(mapentry_t *entry)
{
	return *entry >> 44;
}

#if defined(__MWERKS__) && defined(__POWERPC__)
/* workaround for some bug in codewarrior pro 6.3 compiler */
INLINE void encode_mapentry(mapentry_t *entry, UINT64 offset, UINT32 length)
{
	*entry = ((UINT64)length << 44) | (offset & 0x00000fffffffffffULL);
}
#else
INLINE void encode_mapentry(mapentry_t *entry, UINT64 offset, UINT32 length)
{
	*entry = ((UINT64)length << 44) | ((offset << 20) >> 20);
}
#endif

INLINE void byteswap_mapentry(mapentry_t *entry)
{
#ifdef LSB_FIRST
	mapentry_t temp = *entry;
	UINT8 *s = (UINT8 *)&temp;
	UINT8 *d = (UINT8 *)entry;
	int i;

	for (i = 0; i < sizeof(mapentry_t); i++)
		d[i] = s[sizeof(mapentry_t) - 1 - i];
#endif
}

INLINE UINT32 get_bigendian_uint32(UINT8 *base)
{
	return (base[0] << 24) | (base[1] << 16) | (base[2] << 8) | base[3];
}

INLINE void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
	base[0] = value >> 24;
	base[1] = value >> 16;
	base[2] = value >> 8;
	base[3] = value;
}



/*************************************
 *
 *	Interface setup
 *
 *************************************/

void hard_disk_set_interface(struct hard_disk_interface *new_interface)
{
	if (new_interface)
		interface = *new_interface;
	else
		memset(&interface, 0, sizeof(interface));
}



/*************************************
 *
 *	Interface save
 *
 *************************************/

void hard_disk_save_interface(struct hard_disk_interface *interface_save)
{
	*interface_save = interface;
}



/*************************************
 *
 *	Creating a new hard disk
 *
 *************************************/

int hard_disk_create(const char *filename, const struct hard_disk_header *_header)
{
	static const UINT8 nullmd5[16] = { 0 };
	mapentry_t blank_entries[STACK_ENTRIES];
	int fullchunks, remainder, count;
	struct hard_disk_header header;
	UINT64 fileoffset;
	void *file = NULL;
	int i, err;

	last_error = HDERR_NONE;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(HDERR_NO_INTERFACE);

	/* verify parameters */
	if (!filename || !_header)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* sanity check the header, filling in missing info */
 	header = *_header;
	switch (header.version)
	{
	case 1:
		header.length = HARD_DISK_V1_HEADER_SIZE;
		header.seclen = HARD_DISK_V1_SECTOR_SIZE;
		break;

	case 2:
		header.length = HARD_DISK_V2_HEADER_SIZE;
		break;

	default:
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);
		break;
	}

	/* require a valid MD5 if we're using a parent */
	if ((header.flags & HDFLAGS_HAS_PARENT) && !memcmp(header.parentmd5, nullmd5, sizeof(nullmd5)))
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* require a valid compression mechanism */
	if (header.compression >= HDCOMPRESSION_MAX)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* require a valid blocksize */
	if (header.blocksize == 0 || header.blocksize >= 2048)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* require either totalsectors or CHS values */
	if (!header.cylinders || !header.sectors || !header.heads)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* derive the remaining data */
	header.totalblocks = ((header.cylinders * header.sectors * header.heads) + header.blocksize - 1) / header.blocksize;

	/* attempt to create the file */
	file = (*interface.open)(filename, "wb");
	if (!file)
		SET_ERROR_AND_CLEANUP(HDERR_CANT_CREATE_FILE);

	/* write the resulting header */
	err = write_header(file, &header);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* prepare to write a blank sector map immediately following */
	memset(blank_entries, 0, sizeof(blank_entries));
	fileoffset = header.length;
	fullchunks = header.totalblocks / STACK_ENTRIES;
	remainder = header.totalblocks % STACK_ENTRIES;

	/* first write full chunks of blank entries */
	for (i = 0; i < fullchunks; i++)
	{
		count = (*interface.write)(file, fileoffset, sizeof(blank_entries), blank_entries);
		if (count != sizeof(blank_entries))
			SET_ERROR_AND_CLEANUP(HDERR_WRITE_ERROR);
		fileoffset += sizeof(blank_entries);
	}

	/* then write the remainder */
	if (remainder)
	{
		count = (*interface.write)(file, fileoffset, remainder * sizeof(blank_entries[0]), blank_entries);
		if (count != remainder * sizeof(blank_entries[0]))
			SET_ERROR_AND_CLEANUP(HDERR_WRITE_ERROR);
		fileoffset += remainder * sizeof(blank_entries[0]);
	}

	/* then write a special end-of-list cookie */
	memcpy(&blank_entries[0], END_OF_LIST_COOKIE, sizeof(blank_entries[0]));
	count = (*interface.write)(file, fileoffset, sizeof(blank_entries[0]), blank_entries);
	if (count != sizeof(blank_entries[0]))
		SET_ERROR_AND_CLEANUP(HDERR_WRITE_ERROR);

	/* all done */
	(*interface.close)(file);
	return HDERR_NONE;

cleanup:
	if (file)
		(*interface.close)(file);
	return last_error;
}



/*************************************
 *
 *	Opening a hard disk
 *
 *************************************/

void *hard_disk_open(const char *filename, int writeable, void *parent)
{
	struct hard_disk_info *finalinfo;
	struct hard_disk_info info = { 0 };
	int err;

	last_error = HDERR_NONE;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(HDERR_NO_INTERFACE);

	/* verify parameters */
	if (!filename)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* punt if invalid parent */
	info.parent = parent;
	if (info.parent && info.parent->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* first attempt to open the file */
	info.file = (*interface.open)(filename, writeable ? "rb+" : "rb");
	if (!info.file)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* now attempt to read the header */
	err = read_header(info.file, &info.header);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* make sure we don't open a read-only file writeable */
	if (writeable && !(info.header.flags & HDFLAGS_IS_WRITEABLE))
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_WRITEABLE);

	/* make sure we have a valid parent */
	if (parent && (memcmp(info.parent->header.md5, info.header.parentmd5, 16) || !(info.header.flags & HDFLAGS_HAS_PARENT)))
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARENT);

	/* now read the sector map */
	err = read_sector_map(&info);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* allocate and init the block cache */
	info.cache = malloc(info.header.blocksize * info.header.seclen);
	if (!info.cache)
		SET_ERROR_AND_CLEANUP(HDERR_OUT_OF_MEMORY);
	info.cacheblock = -1;

	/* allocate the temporary compressed buffer */
	info.compressed = malloc(info.header.blocksize * info.header.seclen);
	if (!info.compressed)
		SET_ERROR_AND_CLEANUP(HDERR_OUT_OF_MEMORY);

	/* now init the codec */
	err = init_codec(&info);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* okay, now allocate our entry and copy it */
	finalinfo = malloc(sizeof(info));
	if (!finalinfo)
		SET_ERROR_AND_CLEANUP(HDERR_OUT_OF_MEMORY);
	*finalinfo = info;

	/* hook us into the global list */
	finalinfo->cookie = COOKIE_VALUE;
	finalinfo->next = first_disk;
	first_disk = finalinfo;

	/* all done */
	return finalinfo;

cleanup:
	if (info.codecdata)
		free_codec(&info);
	if (info.compressed)
		free(info.compressed);
	if (info.cache)
		free(info.cache);
	if (info.map)
		free(info.map);
	if (info.file)
		(*interface.close)(info.file);
	return NULL;
}



/*************************************
 *
 *	Closing a hard disk
 *
 *************************************/

void hard_disk_close(void *disk)
{
	struct hard_disk_info *info = disk;
	struct hard_disk_info *curr, *prev;

	/* punt if NULL or invalid */
	if (!info || info->cookie != COOKIE_VALUE)
		return;

	/* deinit the codec */
	if (info->codecdata)
		free_codec(info);

	/* free the compressed data buffer */
	if (info->compressed)
		free(info->compressed);

	/* free the block cache */
	if (info->cache)
		free(info->cache);

	/* free the sector map */
	if (info->map)
		free(info->map);

	/* close the file */
	if (info->file)
		(*interface.close)(info->file);

	/* unlink ourselves */
	for (prev = NULL, curr = first_disk; curr; prev = curr, curr = curr->next)
		if (curr == info)
		{
			if (prev)
				prev->next = curr->next;
			else
				first_disk = curr->next;
			break;
		}

	/* free our memory */
	free(info);
}



/*************************************
 *
 *	Closing all open hard disks
 *
 *************************************/

void hard_disk_close_all(void)
{
	while (first_disk)
		hard_disk_close(first_disk);
}



/*************************************
 *
 *	Reading from a hard disk
 *
 *************************************/

UINT32 hard_disk_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer)
{
	struct hard_disk_info *info = disk;
	UINT32 block, offset;
	int err;

	last_error = HDERR_NONE;

	/* for now, just break down multisector reads into single sectors */
	if (numsectors > 1)
	{
		UINT32 total = 0;
		while (numsectors-- && last_error == HDERR_NONE)
			total += hard_disk_read(disk, lbasector++, 1, (UINT8 *)buffer + total * info->header.seclen);
		return total;
	}

	/* punt if NULL or invalid */
	if (!info || info->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* first determine the block number */
	block = lbasector / info->header.blocksize;
	offset = lbasector % info->header.blocksize;

	/* if we're past the end, fail */
	if (block >= info->header.totalblocks)
		SET_ERROR_AND_CLEANUP(HDERR_SECTOR_OUT_OF_RANGE);

	/* if we don't own the block, ask the parent to handle it */
	if (info->map[block] == 0)
		return hard_disk_read(info->parent, lbasector, numsectors, buffer);

	/* if the block is not cached, load and decompress it */
	if (info->cacheblock != block)
	{
		err = read_block_into_cache(info, block);
		if (err != HDERR_NONE)
			SET_ERROR_AND_CLEANUP(err);
	}

	/* now copy the data */
	memcpy(buffer, &info->cache[offset * info->header.seclen], info->header.seclen);
	return 1;

cleanup:
	return 0;
}



/*************************************
 *
 *	Writing to a hard disk
 *
 *************************************/

UINT32 hard_disk_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer)
{
	struct hard_disk_info *info = disk;
	UINT32 block, offset, count;
	UINT64 fileoffset;
	int err;

	last_error = HDERR_NONE;

	/* for now, just break down multisector writes into single sectors */
	if (numsectors > 1)
	{
		UINT32 total = 0;
		while (numsectors-- && last_error == HDERR_NONE)
			total += hard_disk_write(disk, lbasector++, 1, (const UINT8 *)buffer + total * info->header.seclen);
		return total;
	}

	/* punt if NULL or invalid */
	if (!info || info->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* first determine the block number */
	block = lbasector / info->header.blocksize;
	offset = lbasector % info->header.blocksize;

	/* if we're past the end, fail */
	if (block >= info->header.totalblocks)
		SET_ERROR_AND_CLEANUP(HDERR_SECTOR_OUT_OF_RANGE);

	/* determine the file offset of this block */
	fileoffset = offset_from_mapentry(&info->map[block]);

	/* if we already own the block, and we're not compressing things, just write through */
	if (fileoffset != 0 && info->header.compression == HDCOMPRESSION_NONE)
	{
		/* do the write */
		count = (*interface.write)(info->file, fileoffset + offset * info->header.seclen, info->header.seclen, buffer);
		if (count != info->header.seclen)
			SET_ERROR_AND_CLEANUP(HDERR_WRITE_ERROR);

		/* if this is going to the currently cached block, update the cache as well */
		if (info->cacheblock == block)
			memcpy(&info->cache[offset * info->header.seclen], buffer, info->header.seclen);
		return 1;
	}

	/* we don't own the block yet -- read it from the parent into the cache */
	count = hard_disk_read(disk, block * info->header.blocksize, info->header.blocksize, info->cache);
	if (count != info->header.blocksize)
	{
		info->cacheblock = -1;
		SET_ERROR_AND_CLEANUP(HDERR_READ_ERROR);
	}
	info->cacheblock = block;

	/* now copy in the modified data */
	memcpy(&info->cache[offset * info->header.seclen], buffer, info->header.seclen);

	/* then write out the block */
	err = write_block_from_cache(info, block);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	return 1;

cleanup:
	return 0;
}



/*************************************
 *
 *	Return last error
 *
 *************************************/

int hard_disk_get_last_error(void)
{
	return last_error;
}



/*************************************
 *
 *	Return pointer to header
 *
 *************************************/

const struct hard_disk_header *hard_disk_get_header(void *disk)
{
	const struct hard_disk_info *info = disk;

	/* punt if NULL or invalid */
	if (!info || info->cookie != COOKIE_VALUE)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	return &info->header;

cleanup:
	return NULL;
}



/*************************************
 *
 *	Set the header
 *
 *************************************/

int hard_disk_set_header(const char *filename, const struct hard_disk_header *header)
{
	void *file = NULL;
	int err;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(HDERR_NO_INTERFACE);

	/* punt if NULL or invalid */
	if (!filename || !header)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* first attempt to open the file */
	file = (*interface.open)(filename, "rb+");
	if (!file)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* write the header */
	err = write_header(file, header);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* close the file and return */
	(*interface.close)(file);
	return HDERR_NONE;

cleanup:
	if (file)
		(*interface.close)(file);
	return last_error;
}



/*************************************
 *
 *	All-in-one disk compressor
 *
 *************************************/

int hard_disk_compress(const char *rawfile, UINT32 offset, const char *newfile, const struct hard_disk_header *header, const char *difffile, void (*progress)(const char *, ...))
{
	const struct hard_disk_header *readbackheader;
	struct hard_disk_header tempheader = *header;
	struct hard_disk_info *comparefile = NULL;
	struct hard_disk_info *destfile = NULL;
	UINT64 sourceoffset = offset;
	void *sourcefile = NULL;
	struct MD5Context md5;
	UINT32 blocksizebytes;
	int totalsectors;
	int err, block = 0;
	clock_t lastupdate;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(HDERR_NO_INTERFACE);

	/* verify parameters */
	if (!rawfile || !newfile || !header)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* open the raw file */
	sourcefile = (*interface.open)(rawfile, "rb");
	if (!sourcefile)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* open the diff file */
	if (difffile)
	{
		comparefile = hard_disk_open(difffile, 0, NULL);
		if (!comparefile)
			SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);
	}

	/* create a new writeable disk with the header */
	tempheader = *header;
	tempheader.flags |= HDFLAGS_IS_WRITEABLE;
	err = hard_disk_create(newfile, &tempheader);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* now open it writeable */
	destfile = hard_disk_open(newfile, 1, comparefile);
	if (!destfile)
		SET_ERROR_AND_CLEANUP(HDERR_CANT_CREATE_FILE);
	readbackheader = hard_disk_get_header(destfile);

	/* init the MD5 computation */
	MD5Init(&md5);

	/* loop over source blocks until we run out */
	totalsectors = destfile->header.cylinders * destfile->header.heads * destfile->header.sectors;
	blocksizebytes = destfile->header.blocksize * destfile->header.seclen;
	memset(destfile->cache, 0, blocksizebytes);
	lastupdate = 0;
	while (block < readbackheader->totalblocks)
	{
		int write_this_block = 1;
		clock_t curtime = clock();
		int bytestomd5;

		/* read the data */
		(*interface.read)(sourcefile, sourceoffset, blocksizebytes, destfile->cache);

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			UINT64 sourcepos = (UINT64)block * blocksizebytes;
			if (progress && sourcepos)
				(*progress)("Compressing sector %d/%d... (ratio=%d%%)  \r", block * destfile->header.blocksize, totalsectors, 100 - destfile->eof * 100 / sourcepos);
			lastupdate = curtime;
		}

		/* determine how much to MD5 */
		bytestomd5 = blocksizebytes;
		if ((block + 1) * destfile->header.blocksize > totalsectors)
		{
			bytestomd5 = (totalsectors - block * destfile->header.blocksize) * destfile->header.seclen;
			if (bytestomd5 < 0)
				bytestomd5 = 0;
		}

		/* update the MD5 */
		MD5Update(&md5, destfile->cache, bytestomd5);

		/* see if we have an exact match */
		if (comparefile)
		{
			if (read_block_into_cache(comparefile, block) == HDERR_NONE && memcmp(comparefile->cache, destfile->cache, blocksizebytes) == 0)
				write_this_block = 0;
		}

		/* write out the block */
		if (write_this_block)
		{
			err = write_block_from_cache(destfile, block);
			if (err != HDERR_NONE)
				SET_ERROR_AND_CLEANUP(err);
		}

		/* prepare for the next block */
		block++;
		sourceoffset += blocksizebytes;
		memset(destfile->cache, 0, blocksizebytes);
	}

	/* compute the final MD5 and update to the final header */
	tempheader = destfile->header;
	if (!(header->flags & HDFLAGS_IS_WRITEABLE))
		tempheader.flags &= ~HDFLAGS_IS_WRITEABLE;
	MD5Final(tempheader.md5, &md5);
	err = write_header(destfile->file, &tempheader);
	if (err != HDERR_NONE)
		SET_ERROR_AND_CLEANUP(err);

	/* final progress update */
	if (progress)
	{
		UINT64 sourcepos = (UINT64)block * blocksizebytes;
		if (sourcepos)
			(*progress)("Compression complete ... final ratio = %d%%            \n", 100 - destfile->eof * 100 / sourcepos);
	}

	/* close the drives */
	hard_disk_close(destfile);
	if (comparefile)
		hard_disk_close(comparefile);
	(*interface.close)(sourcefile);

	return HDERR_NONE;

cleanup:
	if (destfile)
		hard_disk_close(destfile);
	if (comparefile)
		hard_disk_close(comparefile);
	if (sourcefile)
		(*interface.close)(sourcefile);
	return last_error;
}



/*************************************
 *
 *	All-in-one disk verifier
 *
 *************************************/

int hard_disk_verify(const char *hdfile, void (*progress)(const char *, ...), UINT8 headermd5[16], UINT8 actualmd5[16])
{
	struct hard_disk_info *sourcefile = NULL;
	struct MD5Context md5;
	UINT32 blocksizebytes;
	int totalsectors;
	int err, block = 0;
	clock_t lastupdate;

	/* punt if no interface */
	if (!interface.open)
		SET_ERROR_AND_CLEANUP(HDERR_NO_INTERFACE);

	/* verify parameters */
	if (!hdfile)
		SET_ERROR_AND_CLEANUP(HDERR_INVALID_PARAMETER);

	/* open the disk file */
	sourcefile = hard_disk_open(hdfile, 0, NULL);
	if (!sourcefile)
		SET_ERROR_AND_CLEANUP(HDERR_FILE_NOT_FOUND);

	/* if this is a writeable disk image, we can't verify */
	if (sourcefile->header.flags & HDFLAGS_IS_WRITEABLE)
		SET_ERROR_AND_CLEANUP(HDERR_CANT_VERIFY);

	/* set the MD5 from the header */
	memcpy(headermd5, sourcefile->header.md5, sizeof(sourcefile->header.md5));

	/* init the MD5 computation */
	MD5Init(&md5);

	/* loop over source blocks until we run out */
	totalsectors = sourcefile->header.cylinders * sourcefile->header.heads * sourcefile->header.sectors;
	blocksizebytes = sourcefile->header.blocksize * sourcefile->header.seclen;
	lastupdate = 0;
	while (block < sourcefile->header.totalblocks)
	{
		clock_t curtime = clock();
		int bytestomd5;

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			if (progress)
				(*progress)("Verifying sector %d/%d...\r", block * sourcefile->header.blocksize, totalsectors);
			lastupdate = curtime;
		}

		/* read the block into the cache */
		err = read_block_into_cache(sourcefile, block);
		if (err != HDERR_NONE)
			SET_ERROR_AND_CLEANUP(err);

		/* determine how much to MD5 */
		bytestomd5 = blocksizebytes;
		if ((block + 1) * sourcefile->header.blocksize > totalsectors)
		{
			bytestomd5 = (totalsectors - block * sourcefile->header.blocksize) * sourcefile->header.seclen;
			if (bytestomd5 < 0)
				bytestomd5 = 0;
		}

		/* update the MD5 */
		MD5Update(&md5, sourcefile->cache, bytestomd5);

		/* prepare for the next block */
		block++;
	}

	/* compute the final MD5 */
	MD5Final(actualmd5, &md5);

	/* final progress update */
	if (progress)
		(*progress)("Verification complete                                  \n");

	/* close the drive */
	hard_disk_close(sourcefile);
	return HDERR_NONE;

cleanup:
	if (sourcefile)
		hard_disk_close(sourcefile);
	return last_error;
}



/*************************************
 *
 *	Block read/decompress
 *
 *************************************/

static int read_block_into_cache(struct hard_disk_info *info, UINT32 block)
{
	UINT64 fileoffset = offset_from_mapentry(&info->map[block]);
	UINT32 length = length_from_mapentry(&info->map[block]);
	UINT32 bytes;

	/* if the data is uncompressed, just read it directly into the cache */
	if (length == info->header.blocksize * info->header.seclen)
	{
		bytes = (*interface.read)(info->file, fileoffset, length, info->cache);
		if (bytes != length)
			return HDERR_READ_ERROR;
		info->cacheblock = block;
		return HDERR_NONE;
	}

	/* otherwise, read it into the decompression buffer */
	bytes = (*interface.read)(info->file, fileoffset, length, info->compressed);
	if (bytes != length)
		return HDERR_READ_ERROR;

	/* now decompress based on the compression method */
	switch (info->header.compression)
	{
		case HDCOMPRESSION_ZLIB:
		{
			struct zlib_codec_data *codec = info->codecdata;
			int err;

			/* reset the decompressor */
			codec->inflater.next_in = info->compressed;
			codec->inflater.avail_in = length;
			codec->inflater.total_in = 0;
			codec->inflater.next_out = info->cache;
			codec->inflater.avail_out = info->header.blocksize * info->header.seclen;
			codec->inflater.total_out = 0;
			err = inflateReset(&codec->inflater);
			if (err != Z_OK)
				return HDERR_DECOMPRESSION_ERROR;

			/* do it */
			err = inflate(&codec->inflater, Z_FINISH);
			if (codec->inflater.total_out != info->header.blocksize * info->header.seclen)
				return HDERR_DECOMPRESSION_ERROR;

			/* mark the block successfully cached in */
			info->cacheblock = block;
			break;
		}
	}
	return HDERR_NONE;
}



/*************************************
 *
 *	Block write/compress
 *
 *************************************/

static int write_block_from_cache(struct hard_disk_info *info, UINT32 block)
{
	UINT64 fileoffset = offset_from_mapentry(&info->map[block]);
	UINT32 length = length_from_mapentry(&info->map[block]);
	UINT32 datalength = info->header.blocksize * info->header.seclen;
	void *data = info->cache;
	mapentry_t entry;
	UINT32 bytes;

	/* first compress the data */
	switch (info->header.compression)
	{
		case HDCOMPRESSION_ZLIB:
		{
			struct zlib_codec_data *codec = info->codecdata;
			int err;

			/* reset the decompressor */
			codec->deflater.next_in = info->cache;
			codec->deflater.avail_in = info->header.blocksize * info->header.seclen;
			codec->deflater.total_in = 0;
			codec->deflater.next_out = info->compressed;
			codec->deflater.avail_out = info->header.blocksize * info->header.seclen;
			codec->deflater.total_out = 0;
			err = deflateReset(&codec->deflater);
			if (err != Z_OK)
				return HDERR_COMPRESSION_ERROR;

			/* do it */
			err = deflate(&codec->deflater, Z_FINISH);

			/* if we didn't run out of space, override the raw data with compressed */
			if (err == Z_STREAM_END)
			{
				data = info->compressed;
				datalength = codec->deflater.total_out;
			}
			break;
		}
	}

	/* if the data doesn't fit into the previous entry, make a new one at the eof */
	if (fileoffset == 0 || length < datalength)
	{
		fileoffset = info->eof;
		info->eof += datalength;
	}

	/* write the data */
	bytes = (*interface.write)(info->file, fileoffset, datalength, data);
	if (bytes != datalength)
		return HDERR_WRITE_ERROR;

	/* update the entry */
	encode_mapentry(&info->map[block], fileoffset, datalength);

	/* update the map on disk */
	entry = info->map[block];
	byteswap_mapentry(&entry);
	bytes = (*interface.write)(info->file, info->header.length + block * sizeof(info->map[0]), sizeof(entry), &entry);
	if (bytes != sizeof(entry))
		return HDERR_WRITE_ERROR;

	return HDERR_NONE;
}



/*************************************
 *
 *	Header read
 *
 *************************************/

static int read_header(void *file, struct hard_disk_header *header)
{
	UINT8 rawheader[HARD_DISK_MAX_HEADER_SIZE];
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return HDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return HDERR_INVALID_FILE;

	/* punt if no interface */
	if (!interface.read)
		return HDERR_NO_INTERFACE;

	/* seek and read */
	count = (*interface.read)(file, 0, sizeof(rawheader), rawheader);
	if (count != sizeof(rawheader))
		return HDERR_READ_ERROR;

	/* verify the tag */
	if (strncmp((char *)rawheader, "MComprHD", 8) != 0)
		return HDERR_INVALID_DATA;

	/* extract the direct data */
	memset(header, 0, sizeof(*header));
	header->length        = get_bigendian_uint32(&rawheader[8]);
	header->version       = get_bigendian_uint32(&rawheader[12]);

	if ((header->version != 1) && (header->version != 2))
		return HDERR_INVALID_DATA;

	if (header->length < ((header->version == 1) ? HARD_DISK_V1_HEADER_SIZE : HARD_DISK_V2_HEADER_SIZE))
		return HDERR_INVALID_DATA;

	header->flags         = get_bigendian_uint32(&rawheader[16]);
	header->compression   = get_bigendian_uint32(&rawheader[20]);
	header->blocksize     = get_bigendian_uint32(&rawheader[24]);
	header->totalblocks   = get_bigendian_uint32(&rawheader[28]);
	header->cylinders     = get_bigendian_uint32(&rawheader[32]);
	header->heads         = get_bigendian_uint32(&rawheader[36]);
	header->sectors       = get_bigendian_uint32(&rawheader[40]);
	memcpy(header->md5, &rawheader[44], 16);
	memcpy(header->parentmd5, &rawheader[60], 16);
	header->seclen        = (header->version == 1) ? HARD_DISK_V1_SECTOR_SIZE : get_bigendian_uint32(&rawheader[76]);
	return HDERR_NONE;
}



/*************************************
 *
 *	Header write
 *
 *************************************/

static int write_header(void *file, const struct hard_disk_header *header)
{
	UINT8 rawheader[HARD_DISK_MAX_HEADER_SIZE];
	UINT32 length;
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return HDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return HDERR_INVALID_FILE;

	/* punt if no interface */
	if (!interface.write)
		return HDERR_NO_INTERFACE;

	/* assemble the data */
	memset(rawheader, 0, sizeof(rawheader));
	memcpy(rawheader, "MComprHD", 8);

	if ((header->version != 1) && (header->version != 2))
		return HDERR_INVALID_DATA;

	length = (header->version == 1) ? HARD_DISK_V1_HEADER_SIZE : HARD_DISK_V2_HEADER_SIZE;

	put_bigendian_uint32(&rawheader[8],  length);
	put_bigendian_uint32(&rawheader[12], header->version);
	put_bigendian_uint32(&rawheader[16], header->flags);
	put_bigendian_uint32(&rawheader[20], header->compression);
	put_bigendian_uint32(&rawheader[24], header->blocksize);
	put_bigendian_uint32(&rawheader[28], header->totalblocks);
	put_bigendian_uint32(&rawheader[32], header->cylinders);
	put_bigendian_uint32(&rawheader[36], header->heads);
	put_bigendian_uint32(&rawheader[40], header->sectors);
	memcpy(&rawheader[44], header->md5, 16);
	memcpy(&rawheader[60], header->parentmd5, 16);
	if (header->version == 2)
		put_bigendian_uint32(&rawheader[76], header->seclen);

	/* seek and write */
	count = (*interface.write)(file, 0, length, rawheader);
	if (count != length)
		return HDERR_WRITE_ERROR;

	return HDERR_NONE;
}



/*************************************
 *
 *	Read the sector map
 *
 *************************************/

static int read_sector_map(struct hard_disk_info *info)
{
	mapentry_t cookie;
	int i, err;
	UINT32 count;

	/* first allocate memory */
	info->map = malloc(sizeof(info->map[0]) * info->header.totalblocks);
	if (!info->map)
		return HDERR_OUT_OF_MEMORY;

	/* read the entire sector map */
	count = (*interface.read)(info->file, info->header.length, sizeof(info->map[0]) * info->header.totalblocks, info->map);
	if (count != sizeof(info->map[0]) * info->header.totalblocks)
	{
		err = HDERR_READ_ERROR;
		goto cleanup;
	}

	/* verify the cookie */
	count = (*interface.read)(info->file, info->header.length + sizeof(info->map[0]) * info->header.totalblocks, sizeof(cookie), &cookie);
	if (count != sizeof(cookie) || memcmp(&cookie, END_OF_LIST_COOKIE, sizeof(cookie)))
	{
		err = HDERR_INVALID_FILE;
		goto cleanup;
	}

	/* compute the EOF and byteswap as necessary */
	info->eof = info->header.length + sizeof(info->map[0]) * (info->header.totalblocks + 1);
	for (i = 0; i < info->header.totalblocks; i++)
	{
		UINT64 end;

		/* first byteswap properly */
		byteswap_mapentry(&info->map[i]);

		/* compute the end of this block; if past EOF, make it the EOF */
		end = offset_from_mapentry(&info->map[i]) + length_from_mapentry(&info->map[i]);
		if (end > info->eof)
			info->eof = end;
	}

	/* all done */
	return HDERR_NONE;

cleanup:
	if (info->map)
		free(info->map);
	info->map = NULL;
	return err;
}



/*************************************
 *
 *	ZLIB memory hooks
 *
 *************************************/

/*
	Because ZLIB allocates and frees memory frequently (once per compression cycle),
	we don't call malloc/free, but instead keep track of our own memory.
*/

static voidpf fast_alloc(voidpf opaque, uInt items, uInt size)
{
	struct zlib_codec_data *data = opaque;
	UINT32 *ptr;
	int i;

	/* compute the size, rounding to the nearest 1k */
	size = (size * items + 0x3ff) & ~0x3ff;

	/* reuse a block if we can */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
	{
		ptr = data->allocptr[i];
		if (ptr && size == *ptr)
		{
			/* set the low bit of the size so we don't match next time */
			*ptr |= 1;
			return ptr + 1;
		}
	}

	/* alloc a new one */
	ptr = malloc(size + sizeof(UINT32));
	if (!ptr)
		return NULL;

	/* put it into the list */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (!data->allocptr[i])
		{
			data->allocptr[i] = ptr;
			break;
		}

	/* set the low bit of the size so we don't match next time */
	*ptr = size | 1;
	return ptr + 1;
}


static void fast_free(voidpf opaque, voidpf address)
{
	struct zlib_codec_data *data = opaque;
	UINT32 *ptr = (UINT32 *)address - 1;
	int i;

	/* find the block */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (ptr == data->allocptr[i])
		{
			/* clear the low bit of the size to allow matches */
			*ptr &= ~1;
			return;
		}
}



/*************************************
 *
 *	Compression init
 *
 *************************************/

static int init_codec(struct hard_disk_info *info)
{
	int err = HDERR_NONE;

	/* now decompress based on the compression method */
	switch (info->header.compression)
	{
		case HDCOMPRESSION_NONE:
			/* nothing to do */
			break;

		case HDCOMPRESSION_ZLIB:
		{
			struct zlib_codec_data *data;

			/* allocate memory for the 2 stream buffers */
			info->codecdata = malloc(sizeof(struct zlib_codec_data));
			if (!info->codecdata)
				return HDERR_OUT_OF_MEMORY;

			/* clear the buffers */
			data = info->codecdata;
			memset(data, 0, sizeof(struct zlib_codec_data));

			/* init the first for decompression and the second for compression */
			data->inflater.next_in = info->compressed;
			data->inflater.avail_in = 0;
			data->inflater.zalloc = fast_alloc;
			data->inflater.zfree = fast_free;
			data->inflater.opaque = data;
			err = inflateInit2(&data->inflater, -MAX_WBITS);
			if (err == Z_OK)
			{
				data->deflater.next_in = info->compressed;
				data->deflater.avail_in = 0;
				data->deflater.zalloc = fast_alloc;
				data->deflater.zfree = fast_free;
				data->deflater.opaque = data;
				err = deflateInit2(&data->deflater, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
			}

			/* convert errors */
			if (err == Z_MEM_ERROR)
				err = HDERR_OUT_OF_MEMORY;
			else if (err != Z_OK)
				err = HDERR_CODEC_ERROR;
			else
				err = HDERR_NONE;

			/* handle an error */
			if (err != HDERR_NONE)
				free(info->codecdata);
			break;
		}
	}

	/* return the error */
	return err;
}



/*************************************
 *
 *	Compression de-init
 *
 *************************************/

static void free_codec(struct hard_disk_info *info)
{
	/* now decompress based on the compression method */
	switch (info->header.compression)
	{
		case HDCOMPRESSION_NONE:
			/* nothing to do */
			break;

		case HDCOMPRESSION_ZLIB:
		{
			struct zlib_codec_data *data = info->codecdata;

			/* deinit the streams */
			if (data)
			{
				int i;

				inflateEnd(&data->inflater);
				deflateEnd(&data->deflater);

				/* free our fast memory */
				for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
					if (data->allocptr[i])
						free(data->allocptr[i]);
				free(data);
			}
			break;
		}
	}
}
