/***************************************************************************

	Hard disk compression frontend

***************************************************************************/

#include "harddisk.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif



/***************************************************************************
	PROTOTYPES
***************************************************************************/

static void *hdcomp_open(const char *filename, const char *mode);
static void hdcomp_close(void *file);
static UINT32 hdcomp_read(void *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 hdcomp_write(void *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 get_file_size(const char *file);



/***************************************************************************
	GLOBAL VARIABLES
***************************************************************************/

static struct hard_disk_interface hdcomp_interface =
{
	hdcomp_open,
	hdcomp_close,
	hdcomp_read,
	hdcomp_write
};

static void *inputdisk;

#define SPECIAL_DISK_NAME "??INPUTDISK??"

static const char *error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"sector out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file"
};



/***************************************************************************
	IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
	progress - generic progress callback
-------------------------------------------------*/

static void progress(const char *fmt, ...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, fmt);
	vprintf(fmt, arg);
	fflush(stdout);
	va_end(arg);
}



/*-------------------------------------------------
	error_string - return an error sting
-------------------------------------------------*/

static const char *error_string(int err)
{
	static char temp_buffer[100];

	if (err < sizeof(error_strings) / sizeof(error_strings[0]))
		return error_strings[err];

	sprintf(temp_buffer, "unknown error %d", err);
	return temp_buffer;
}



/*-------------------------------------------------
	error - generic usage error display
-------------------------------------------------*/

static void error(void)
{
	fprintf(stderr, "usage: hdcomp -create input.raw output.chd [inputoffs [cylinders heads sectors]]\n");
	fprintf(stderr, "   or: hdcomp -extract input.chd output.raw\n");
	fprintf(stderr, "   or: hdcomp -verify input.chd\n");
	fprintf(stderr, "   or: hdcomp -verifyfix input.chd\n");
	fprintf(stderr, "   or: hdcomp -info input.chd\n");
	fprintf(stderr, "   or: hdcomp -merge base.chd diff.chd output.chd\n");
	fprintf(stderr, "   or: hdcomp -setchs inout.chd cylinders heads sectors\n");
	exit(1);
}



/*-------------------------------------------------
	guess_chs - given a file and an offset,
	compute a best guess CHS value set
-------------------------------------------------*/

static void guess_chs(const char *file, int offset, struct hard_disk_header *header)
{
	UINT64 filesize = get_file_size(file);
	UINT32 totalsecs, hds, secs;

	/* validate the file */
	if (filesize == 0)
	{
		fprintf(stderr, "Invalid file '%s'\n", file);
		exit(1);
	}

	/* validate the size */
	if (filesize % 512 != 0)
	{
		fprintf(stderr, "Can't guess CHS values because data size is not divisible by 512\n");
		exit(1);
	}
	totalsecs = filesize / 512;

	/* now find a valid value */
	for (secs = 63; secs > 1; secs--)
		if (totalsecs % secs == 0)
		{
			size_t totalhds = totalsecs / secs;
			for (hds = 16; hds > 1; hds--)
				if (totalhds % hds == 0)
				{
					header->cylinders = totalhds / hds;
					header->heads = hds;
					header->sectors = secs;
					return;
				}
		}

	/* ack, it didn't work! */
	fprintf(stderr, "Can't guess CHS values because no logical combination works!\n");
	exit(1);
}



/*-------------------------------------------------
	do_create - create a new compressed disk
	image from a raw file
-------------------------------------------------*/

static void do_create(int argc, char *argv[])
{
	struct hard_disk_header header = { 0 };
	const char *inputfile, *outputfile;
	int offset, err;

	/* require 4, 5, or 8 args total */
	if (argc != 4 && argc != 5 && argc != 8)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];
	if (argc >= 5)
		offset = atoi(argv[4]);
	else
		offset = get_file_size(inputfile) % 512;
	if (argc >= 8)
	{
		header.cylinders = atoi(argv[5]);
		header.heads = atoi(argv[6]);
		header.sectors = atoi(argv[7]);
	}
	else
		guess_chs(inputfile, offset, &header);

	/* fill in the rest of the header */
	header.flags = 0;
	header.compression = HDCOMPRESSION_ZLIB;
	header.blocksize = 8;

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	printf("Input offset: %d\n", offset);
	printf("Cylinders:    %d\n", header.cylinders);
	printf("Heads:        %d\n", header.heads);
	printf("Sectors:      %d\n", header.sectors);

	/* compress the hard drive */
	hard_disk_set_interface(&hdcomp_interface);
	err = hard_disk_compress(inputfile, offset, outputfile, &header, NULL, progress);
	if (err != HDERR_NONE)
	{
		printf("Error during compression: %s\n", error_string(err));
		return;
	}
}



/*-------------------------------------------------
	do_extract - extract a raw file from a
	compressed disk image
-------------------------------------------------*/

static void do_extract(int argc, char *argv[])
{
	struct hard_disk_header header;
	const char *inputfile, *outputfile;
	void *disk = NULL, *outfile = NULL;
	int sectornum, totalsectors;
	void *block = NULL;
	clock_t lastupdate;

	/* require 4 args total */
	if (argc != 4)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);

	/* get the header */
	hard_disk_set_interface(&hdcomp_interface);
	disk = hard_disk_open(inputfile, 0, NULL);
	if (!disk)
	{
		printf("Error opening disk image '%s': %s\n", inputfile, error_string(hard_disk_get_last_error()));
		goto error;
	}
	header = *hard_disk_get_header(disk);
	totalsectors = header.cylinders * header.heads * header.sectors;

	/* allocate memory to hold a block */
	block = malloc(header.blocksize * HARD_DISK_SECTOR_SIZE);
	if (!block)
	{
		printf("Out of memory allocating block buffer!\n");
		goto error;
	}

	/* create the output file */
	outfile = (*hdcomp_interface.open)(outputfile, "wb");
	if (!outfile)
	{
		printf("Error opening output file '%s'\n", outputfile);
		goto error;
	}

	/* loop over blocks, reading and writing */
	lastupdate = 0;
	for (sectornum = 0; sectornum < totalsectors; sectornum += header.blocksize)
	{
		UINT32 bytestowrite, byteswritten;
		clock_t curtime = clock();

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			progress("Extracting sector %d/%d...  \r", sectornum, totalsectors);
			lastupdate = curtime;
		}

		/* read the block into a buffer */
		if (!hard_disk_read(disk, sectornum, header.blocksize, block))
		{
			printf("Error reading sectors %d-%d from disk image: %s\n", sectornum, sectornum + header.blocksize - 1, error_string(hard_disk_get_last_error()));
			goto error;
		}

		/* determine how much to write */
		bytestowrite = (sectornum + header.blocksize > totalsectors) ? (totalsectors - sectornum) : header.blocksize;
		bytestowrite *= HARD_DISK_SECTOR_SIZE;

		/* write the block to the file */
		byteswritten = (*hdcomp_interface.write)(outfile, (UINT64)sectornum * (UINT64)HARD_DISK_SECTOR_SIZE, bytestowrite, block);
		if (byteswritten != bytestowrite)
		{
			printf("Error writing sectors %d-%d to output file: %s\n", sectornum, sectornum + header.blocksize - 1, error_string(hard_disk_get_last_error()));
			goto error;
		}
	}
	progress("Extraction complete!                    \n");

	/* close everything down */
	(*hdcomp_interface.close)(outfile);
	free(block);
	hard_disk_close(disk);
	return;

error:
	/* clean up our mess */
	if (outfile)
		(*hdcomp_interface.close)(outfile);
	if (block)
		free(block);
	if (disk)
		hard_disk_close(disk);
}



/*-------------------------------------------------
	do_verify - validate the MD5 on a drive image
-------------------------------------------------*/

static void do_verify(int argc, char *argv[], int fix)
{
	UINT8 headermd5[16], actualmd5[16];
	const char *inputfile;
	int err;

	/* require 3 args total */
	if (argc != 3)
		error();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* compress the hard drive */
	hard_disk_set_interface(&hdcomp_interface);
	err = hard_disk_verify(inputfile, progress, headermd5, actualmd5);
	if (err != HDERR_NONE)
	{
		if (err == HDERR_CANT_VERIFY)
			printf("Can't verify this type of image (probably writeable)\n");
		else
			printf("Error during verify: %s\n", error_string(err));
		return;
	}

	/* verify the MD5 */
	if (!memcmp(headermd5, actualmd5, sizeof(headermd5)))
		printf("Verification successful!\n");
	else
	{
		printf("Error: MD5 in header = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				headermd5[0], headermd5[1], headermd5[2], headermd5[3],
				headermd5[4], headermd5[5], headermd5[6], headermd5[7],
				headermd5[8], headermd5[9], headermd5[10], headermd5[11],
				headermd5[12], headermd5[13], headermd5[14], headermd5[15]);
		printf("          actual MD5 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				actualmd5[0], actualmd5[1], actualmd5[2], actualmd5[3],
				actualmd5[4], actualmd5[5], actualmd5[6], actualmd5[7],
				actualmd5[8], actualmd5[9], actualmd5[10], actualmd5[11],
				actualmd5[12], actualmd5[13], actualmd5[14], actualmd5[15]);

		/* fix it */
		if (fix)
		{
			struct hard_disk_header header;
			void *disk;

			/* open the image */
			disk = hard_disk_open(inputfile, 0, NULL);
			if (!disk)
			{
				printf("Error opening disk image '%s': %s\n", inputfile, error_string(hard_disk_get_last_error()));
				return;
			}

			/* read and modify the header */
			header = *hard_disk_get_header(disk);
			memcpy(header.md5, actualmd5, sizeof(header.md5));
			hard_disk_close(disk);

			/* write the new header */
			err = hard_disk_set_header(inputfile, &header);
			if (err != HDERR_NONE)
				printf("Error writing new header: %s\n", error_string(err));
			else
				printf("Updated MD5 successfully\n");
		}
	}
}



/*-------------------------------------------------
	do_info - dump the header information from
	a drive image
-------------------------------------------------*/

static void do_info(int argc, char *argv[])
{
	static const char *compression_type[] =
	{
		"none",
		"zlib"
	};
	struct hard_disk_header header;
	const char *inputfile;
	void *disk;

	/* require 3 args total */
	if (argc != 3)
		error();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* get the header */
	hard_disk_set_interface(&hdcomp_interface);
	disk = hard_disk_open(inputfile, 0, NULL);
	if (!disk)
	{
		printf("Error opening disk image '%s': %s\n", inputfile, error_string(hard_disk_get_last_error()));
		return;
	}
	header = *hard_disk_get_header(disk);
	hard_disk_close(disk);

	/* print the info */
	printf("Header Size:  %d bytes\n", header.length);
	printf("File Version: %d\n", header.version);
	printf("Flags:        %s, %s\n",
			(header.flags & HDFLAGS_HAS_PARENT) ? "HAS_PARENT" : "NO_PARENT",
			(header.flags & HDFLAGS_IS_WRITEABLE) ? "WRITEABLE" : "READ_ONLY");
	if (header.compression < 2)
		printf("Compression:  %s\n", compression_type[header.compression]);
	else
		printf("Compression:  Unknown type %d\n", header.compression);
	printf("Block Size:   %d sectors\n", header.blocksize);
	printf("Total Blocks: %d\n", header.totalblocks);
	printf("Cylinders:    %d\n", header.cylinders);
	printf("Heads:        %d\n", header.heads);
	printf("Sectors:      %d\n", header.sectors);
	if (!(header.flags & HDFLAGS_IS_WRITEABLE))
		printf("MD5:          %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.md5[0], header.md5[1], header.md5[2], header.md5[3],
				header.md5[4], header.md5[5], header.md5[6], header.md5[7],
				header.md5[8], header.md5[9], header.md5[10], header.md5[11],
				header.md5[12], header.md5[13], header.md5[14], header.md5[15]);
	if (header.flags & HDFLAGS_HAS_PARENT)
		printf("Parent MD5:   %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.parentmd5[0], header.parentmd5[1], header.parentmd5[2], header.parentmd5[3],
				header.parentmd5[4], header.parentmd5[5], header.parentmd5[6], header.parentmd5[7],
				header.parentmd5[8], header.parentmd5[9], header.parentmd5[10], header.parentmd5[11],
				header.parentmd5[12], header.parentmd5[13], header.parentmd5[14], header.parentmd5[15]);
}



/*-------------------------------------------------
	do_merge - merge a parent and its child
	together
-------------------------------------------------*/

static void do_merge(int argc, char *argv[])
{
	const char *inputfile, *difffile, *outputfile;
	void *parentdisk = NULL;
	int err;

	/* require 5 args total */
	if (argc != 5)
		error();

	/* extract the data */
	inputfile = argv[2];
	difffile = argv[3];
	outputfile = argv[4];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Diff file:    %s\n", difffile);
	printf("Output file:  %s\n", outputfile);

	/* get the header */
	hard_disk_set_interface(&hdcomp_interface);
	parentdisk = hard_disk_open(inputfile, 0, NULL);
	if (!parentdisk)
	{
		printf("Error opening disk image '%s': %s\n", inputfile, error_string(hard_disk_get_last_error()));
		goto error;
	}

	/* open the diff disk */
	inputdisk = hard_disk_open(difffile, 0, parentdisk);
	if (!inputdisk)
	{
		printf("Error opening disk image '%s': %s\n", difffile, error_string(hard_disk_get_last_error()));
		goto error;
	}

	/* do the compression; our interface will route reads for us */
	err = hard_disk_compress(SPECIAL_DISK_NAME, 0, outputfile, hard_disk_get_header(parentdisk), NULL, progress);
	if (err != HDERR_NONE)
		printf("Error during compression: %s\n", error_string(err));

error:
	/* close everything down */
	if (inputdisk)
		hard_disk_close(inputdisk);
	if (parentdisk)
		hard_disk_close(parentdisk);
}



/*-------------------------------------------------
	do_setchs - change the CHS values on a disk
	image
-------------------------------------------------*/

static void do_setchs(int argc, char *argv[])
{
	struct hard_disk_header header;
	int cyls, hds, secs, err;
	const char *inoutfile;
	void *disk = NULL;

	/* require 6 args total */
	if (argc != 6)
		error();

	/* extract the data */
	inoutfile = argv[2];
	cyls = atoi(argv[3]);
	hds = atoi(argv[4]);
	secs = atoi(argv[5]);

	/* print some info */
	printf("Input file:   %s\n", inoutfile);
	printf("Cylinders:    %d\n", cyls);
	printf("Heads:        %d\n", hds);
	printf("Sectors:      %d\n", secs);

	/* open the file */
	hard_disk_set_interface(&hdcomp_interface);
	disk = hard_disk_open(inoutfile, 0, NULL);
	if (!disk)
	{
		printf("Error opening disk image '%s': %s\n", inoutfile, error_string(hard_disk_get_last_error()));
		return;
	}

	/* read and modify the header */
	header = *hard_disk_get_header(disk);
	header.cylinders = cyls;
	header.heads = hds;
	header.sectors = secs;
	hard_disk_close(disk);

	/* write the new header */
	err = hard_disk_set_header(inoutfile, &header);
	if (err != HDERR_NONE)
		printf("Error writing new header: %s\n", error_string(err));
}



/*-------------------------------------------------
	get_file_size - returns the 64-bit file size
	for a file
-------------------------------------------------*/

static UINT64 get_file_size(const char *file)
{
#ifdef _WIN32
	DWORD highSize = 0, lowSize;
	HANDLE handle;
	UINT64 result;

	/* attempt to open the file */
	handle = CreateFile(file, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;

	/* get the file size */
	lowSize = GetFileSize(handle, &highSize);
	result = lowSize | ((UINT64)highSize << 32);
	if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
		result = 0;

	/* close the file and return */
	CloseHandle(handle);
	return result;
#else
	size_t filesize;
	FILE *f;

	/* attempt to open the file */
	f = fopen(file, "rb");
	if (!f)
		return 0;

	/* get the size */
	fseek(f, 0, SEEK_END);
	filesize = ftell(f);
	fclose(f);

	return filesize;
#endif
}



/*-------------------------------------------------
	hdcomp_open - open file
-------------------------------------------------*/

static void *hdcomp_open(const char *filename, const char *mode)
{
	/* if it's the special disk filename, just hand back our handle */
	if (!strcmp(filename, SPECIAL_DISK_NAME))
		return inputdisk;

	/* otherwise, open normally */
	else
	{
#ifdef _WIN32
		DWORD disposition, access;
		HANDLE handle;

		/* convert the mode into disposition and access */
		if (!strcmp(mode, "rb"))
			disposition = OPEN_EXISTING, access = GENERIC_READ;
		else if (!strcmp(mode, "rb+"))
			disposition = OPEN_EXISTING, access = GENERIC_READ | GENERIC_WRITE;
		else if (!strcmp(mode, "wb"))
			disposition = CREATE_ALWAYS, access = GENERIC_WRITE;
		else
			return NULL;

		/* attempt to open the file */
		handle = CreateFile(filename, access, 0, NULL, disposition, 0, NULL);
		if (handle == INVALID_HANDLE_VALUE)
			return NULL;
		return (void *)handle;
#else
		return fopen(filename, mode);
#endif
	}
}



/*-------------------------------------------------
	hdcomp_close - close file
-------------------------------------------------*/

static void hdcomp_close(void *file)
{
	/* if it's the special disk handle, do nothing */
	if (file == inputdisk)
		return;

#ifdef _WIN32
	CloseHandle((HANDLE)file);
#else
	fclose(file);
#endif
}



/*-------------------------------------------------
	hdcomp_read - read from an offset
-------------------------------------------------*/

static UINT32 hdcomp_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	/* if it's the special disk handle, read from it */
	if (file == inputdisk)
	{
		if (offset % HARD_DISK_SECTOR_SIZE != 0)
		{
			printf("Error: hdcomp read from non-aligned offset %08X%08X\n", (UINT32)(offset >> 32), (UINT32)offset);
			return 0;
		}
		if (count % HARD_DISK_SECTOR_SIZE != 0)
		{
			printf("Error: hdcomp read non-aligned amount %08X\n", count);
			return 0;
		}
		return HARD_DISK_SECTOR_SIZE * hard_disk_read(inputdisk, offset / HARD_DISK_SECTOR_SIZE, count / HARD_DISK_SECTOR_SIZE, buffer);
	}

	/* otherwise, do it normally */
	else
	{
#ifdef _WIN32
		LONG upperPos = offset >> 32;
		DWORD result;

		/* attempt to seek to the new location */
		result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
		if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return 0;

		/* do the read */
		if (ReadFile((HANDLE)file, buffer, count, &result, NULL))
			return result;
		else
			return 0;
#else
		fseek(file, offset, SEEK_SET);
		return fread(buffer, 1, count, file);
#endif
	}
}



/*-------------------------------------------------
	hdcomp_write - write to an offset
-------------------------------------------------*/

static UINT32 hdcomp_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
{
	/* if it's the special disk handle, this is bad */
	if (file == inputdisk)
	{
		printf("Error: hdcomp write to disk image = bad!\n");
		return 0;
	}

	/* otherwise, do it normally */
	else
	{
#ifdef _WIN32
		LONG upperPos = offset >> 32;
		DWORD result;

		/* attempt to seek to the new location */
		result = SetFilePointer((HANDLE)file, (UINT32)offset, &upperPos, FILE_BEGIN);
		if (result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
			return 0;

		/* do the read */
		if (WriteFile((HANDLE)file, buffer, count, &result, NULL))
			return result;
		else
			return 0;
#else
		fseek(file, offset, SEEK_SET);
		return fwrite(buffer, 1, count, file);
#endif
	}
}



/*-------------------------------------------------
	main - entry point
-------------------------------------------------*/

int main(int argc, char **argv)
{
	/* require at least 1 argument */
	if (argc < 2)
		error();

	/* handle the appropriate command */
	if (!stricmp(argv[1], "-create"))
		do_create(argc, argv);
	else if (!stricmp(argv[1], "-extract"))
		do_extract(argc, argv);
	else if (!stricmp(argv[1], "-verify"))
		do_verify(argc, argv, 0);
	else if (!stricmp(argv[1], "-verifyfix"))
		do_verify(argc, argv, 1);
	else if (!stricmp(argv[1], "-info"))
		do_info(argc, argv);
	else if (!stricmp(argv[1], "-merge"))
		do_merge(argc, argv);
	else if (!stricmp(argv[1], "-setchs"))
		do_setchs(argc, argv);
	else
		error();

	return 0;
}
