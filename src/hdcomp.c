/***************************************************************************

	Hard disk compression frontend

***************************************************************************/

#include "harddisk.h"
#include <stdarg.h>
#include <stdio.h>


static void *hdcomp_open(const char *filename, const char *mode);
static void hdcomp_close(void *file);
static UINT32 hdcomp_read(void *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 hdcomp_write(void *file, UINT64 offset, UINT32 count, const void *buffer);

static struct hard_disk_interface hdcomp_interface =
{
	hdcomp_open,
	hdcomp_close,
	hdcomp_read,
	hdcomp_write
};


static void progress(const char *fmt, ...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, fmt);
	vprintf(fmt, arg);
	va_end(arg);
}


int main(int argc, char **argv)
{
	struct hard_disk_header header = { 0 };
	int offset, err;
	FILE *temp;

	/* cheesy for now: require all args */
	if (argc != 7)
	{
		fprintf(stderr, "usage: hdcomp inputfile outputfile inputoffs cylinders heads sectors\n");
		return 1;
	}

	/* extract the data */
	offset = atoi(argv[3]);
	header.cylinders = atoi(argv[4]);
	header.heads = atoi(argv[5]);
	header.sectors = atoi(argv[6]);

	/* fill in the rest of the header */
	header.flags = 0;
	header.compression = HDCOMPRESSION_ZLIB;
	header.blocksize = 8;

	/* print some info */
	printf("Input file:   %s\n", argv[1]);
	printf("Output file:  %s\n", argv[2]);
	printf("Input offset: %d\n", offset);
	printf("Cylinders:    %d\n", header.cylinders);
	printf("Heads:        %d\n", header.heads);
	printf("Sectors:      %d\n", header.sectors);

	/* verify the file size */
	temp = fopen(argv[1], "rb");
	if (!temp)
	{
		fprintf(stderr, "Can't open source file %s!\n", argv[1]);
		return 1;
	}

	/* compress the hard drive */
	hard_disk_set_interface(&hdcomp_interface);
	err = hard_disk_compress(argv[1], offset, argv[2], &header, NULL, progress);
	if (err != HDERR_NONE)
	{
		printf("Error during compression: %d\n", err);
		return 1;
	}

	return 0;
}


static void *hdcomp_open(const char *filename, const char *mode)
{
	return fopen(filename, mode);
}


static void hdcomp_close(void *file)
{
	fclose(file);
}


static UINT32 hdcomp_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	fseek(file, offset, SEEK_SET);
	return fread(buffer, 1, count, file);
}


static UINT32 hdcomp_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
{
	fseek(file, offset, SEEK_SET);
	return fwrite(buffer, 1, count, file);
}

