/*********************************************************************

	common.c

	Generic functions, mostly ROM and graphics related.

*********************************************************************/

#include "driver.h"
#include "png.h"
#include <stdarg.h>


//#define LOG_LOAD



/***************************************************************************

	Constants

***************************************************************************/

// VERY IMPORTANT: osd_alloc_bitmap must allocate also a "safety area" 16 pixels wide all
// around the bitmap. This is required because, for performance reasons, some graphic
// routines don't clip at boundaries of the bitmap.
#define BITMAP_SAFETY			16

#define MAX_MALLOCS				1024



/***************************************************************************

	Type definitions

***************************************************************************/

struct rom_load_data
{
	int 		warnings;				/* warning count during processing */
	int 		errors;					/* error count during processing */

	int 		romsloaded;				/* current ROMs loaded count */
	int			romstotal;				/* total number of ROMs to read */

	void *		file;					/* current file */

	UINT8 *		regionbase;				/* base of current region */
	UINT32		regionlength;			/* length of current region */

	char		errorbuf[4096];			/* accumulated errors */
	UINT8		tempbuf[65536];			/* temporary buffer */
};


struct malloc_info
{
	int tag;
	void *ptr;
};



/***************************************************************************

	Global variables

***************************************************************************/

/* These globals are only kept on a machine basis - LBO 042898 */
unsigned int dispensed_tickets;
unsigned int coins[COIN_COUNTERS];
unsigned int lastcoin[COIN_COUNTERS];
unsigned int coinlockedout[COIN_COUNTERS];

int snapno;

/* malloc tracking */
static struct malloc_info malloc_list[MAX_MALLOCS];
static int malloc_list_index = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* generic NVRAM */
size_t generic_nvram_size;
data8_t *generic_nvram;



/***************************************************************************

	Prototypes

***************************************************************************/

static int rom_load_new(const struct RomModule *romp);



/***************************************************************************

	Functions

***************************************************************************/

void showdisclaimer(void)   /* MAURY_BEGIN: dichiarazione */
{
#ifdef PINMAME
        printf("PinMAME is an emulator: it reproduces, more or less faithfully, the behaviour\n"
               "of several pinball arcade machines. But hardware is useless without software,\n"
               "so an image of the ROMs which run on that hardware is required. Such ROMs, like\n"
               "any other commercial software, are copyrighted material and it is therefore\n"
               "illegal to use them if you don't own the original arcade machine. Needless to\n"
               "say, ROMs are not distributed together with PinMAME. Distribution of PinMAME\n"
               "together with ROM images is a violation of copyright law and should be promptly\n"
               "reported to the authors so that appropriate legal action can be taken.\n\n");
#else /* PINMAME */
	printf("MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		 "several arcade machines. But hardware is useless without software, so an image\n"
		 "of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
		 "commercial software, are copyrighted material and it is therefore illegal to\n"
		 "use them if you don't own the original arcade machine. Needless to say, ROMs\n"
		 "are not distributed together with MAME. Distribution of MAME together with ROM\n"
		 "images is a violation of copyright law and should be promptly reported to the\n"
		 "authors so that appropriate legal action can be taken.\n\n");
#endif /* PINMAME */
}                           /* MAURY_END: dichiarazione */



/***************************************************************************

	Sample handling code

	This function is different from readroms() because it doesn't fail if
	it doesn't find a file: it will load as many samples as it can find.

***************************************************************************/

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

/*-------------------------------------------------
	read_wav_sample - read a WAV file as a sample
-------------------------------------------------*/

static struct GameSample *read_wav_sample(void *f)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize, temp32;
	UINT16 bits, temp16;
	char buf[32];
	struct GameSample *result;

	/* read the core header and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 4)
		return NULL;
	if (memcmp(&buf[0], "RIFF", 4) != 0)
		return NULL;

	/* get the total size */
	offset += osd_fread(f, &filesize, 4);
	if (offset < 8)
		return NULL;
	filesize = intelLong(filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 12)
		return NULL;
	if (memcmp(&buf[0], "WAVE", 4) != 0)
		return NULL;

	/* seek until we find a format tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "fmt ", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* read the format -- make sure it is PCM */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* number of channels -- only mono is supported */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* sample rate */
	offset += osd_fread(f, &rate, 4);
	rate = intelLong(rate);

	/* bytes/second and block alignment are ignored */
	offset += osd_fread(f, buf, 6);

	/* bits/sample */
	offset += osd_fread_lsbfirst(f, &bits, 2);
	if (bits != 8 && bits != 16)
		return NULL;

	/* seek past any extra data */
	osd_fseek(f, length - 16, SEEK_CUR);
	offset += length - 16;

	/* seek until we find a data tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "data", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* allocate the game sample */
	result = auto_malloc(sizeof(struct GameSample) + length);
	if (result == NULL)
		return NULL;

	/* fill in the sample data */
	result->length = length;
	result->smpfreq = rate;
	result->resolution = bits;

	/* read the data in */
	if (bits == 8)
	{
		osd_fread(f, result->data, length);

		/* convert 8-bit data to signed samples */
		for (temp32 = 0; temp32 < length; temp32++)
			result->data[temp32] ^= 0x80;
	}
	else
	{
		/* 16-bit data is fine as-is */
		osd_fread_lsbfirst(f, result->data, length);
	}

	return result;
}


/*-------------------------------------------------
	readsamples - load all samples
-------------------------------------------------*/

struct GameSamples *readsamples(const char **samplenames,const char *basename)
/* V.V - avoids samples duplication */
/* if first samplename is *dir, looks for samples into "basename" first, then "dir" */
{
	int i;
	struct GameSamples *samples;
	int skipfirst = 0;

	/* if the user doesn't want to use samples, bail */
	if (!options.use_samples) return 0;

	if (samplenames == 0 || samplenames[0] == 0) return 0;

	if (samplenames[0][0] == '*')
		skipfirst = 1;

	i = 0;
	while (samplenames[i+skipfirst] != 0) i++;

	if (!i) return 0;

	if ((samples = auto_malloc(sizeof(struct GameSamples) + (i-1)*sizeof(struct GameSample))) == 0)
		return 0;

	samples->total = i;
	for (i = 0;i < samples->total;i++)
		samples->sample[i] = 0;

	for (i = 0;i < samples->total;i++)
	{
		void *f;

		if (samplenames[i+skipfirst][0])
		{
			if ((f = osd_fopen(basename,samplenames[i+skipfirst],OSD_FILETYPE_SAMPLE,0)) == 0)
				if (skipfirst)
					f = osd_fopen(samplenames[0]+1,samplenames[i+skipfirst],OSD_FILETYPE_SAMPLE,0);
			if (f != 0)
			{
				samples->sample[i] = read_wav_sample(f);
				osd_fclose(f);
			}
		}
	}

	return samples;
}



/***************************************************************************

	Memory region code

***************************************************************************/

/*-------------------------------------------------
	memory_region - returns pointer to a memory
	region
-------------------------------------------------*/

unsigned char *memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return Machine->memory_region[num].base;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
				return Machine->memory_region[i].base;
		}
	}

	return 0;
}


/*-------------------------------------------------
	memory_region_length - returns length of a
	memory region
-------------------------------------------------*/

size_t memory_region_length(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
		return Machine->memory_region[num].length;
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
				return Machine->memory_region[i].length;
		}
	}

	return 0;
}


/*-------------------------------------------------
	new_memory_region - allocates memory for a
	region
-------------------------------------------------*/

int new_memory_region(int num, size_t length, UINT32 flags)
{
    int i;

    if (num < MAX_MEMORY_REGIONS)
    {
        Machine->memory_region[num].length = length;
        Machine->memory_region[num].base = malloc(length);
        return (Machine->memory_region[num].base == NULL) ? 1 : 0;
    }
    else
    {
        for (i = 0;i < MAX_MEMORY_REGIONS;i++)
        {
            if (Machine->memory_region[i].base == NULL)
            {
                Machine->memory_region[i].length = length;
                Machine->memory_region[i].type = num;
                Machine->memory_region[i].flags = flags;
                Machine->memory_region[i].base = malloc(length);
                return (Machine->memory_region[i].base == NULL) ? 1 : 0;
            }
        }
    }
	return 1;
}


/*-------------------------------------------------
	free_memory_region - releases memory for a
	region
-------------------------------------------------*/

void free_memory_region(int num)
{
	int i;

	if (num < MAX_MEMORY_REGIONS)
	{
		free(Machine->memory_region[num].base);
		memset(&Machine->memory_region[num], 0, sizeof(Machine->memory_region[num]));
	}
	else
	{
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
		{
			if (Machine->memory_region[i].type == num)
			{
				free(Machine->memory_region[i].base);
				memset(&Machine->memory_region[i], 0, sizeof(Machine->memory_region[i]));
				return;
			}
		}
	}
}



/***************************************************************************

	Coin counter code

***************************************************************************/

/*-------------------------------------------------
	coin_counter_w - sets input for coin counter
-------------------------------------------------*/

void coin_counter_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;
	/* Count it only if the data has changed from 0 to non-zero */
	if (on && (lastcoin[num] == 0))
	{
		coins[num]++;
	}
	lastcoin[num] = on;
}


/*-------------------------------------------------
	coin_lockout_w - locks out one coin input
-------------------------------------------------*/

void coin_lockout_w(int num,int on)
{
	if (num >= COIN_COUNTERS) return;

	coinlockedout[num] = on;
}


/*-------------------------------------------------
	coin_lockout_global_w - locks out all the coin
	inputs
-------------------------------------------------*/

void coin_lockout_global_w(int on)
{
	int i;

	for (i = 0; i < COIN_COUNTERS; i++)
	{
		coin_lockout_w(i,on);
	}
}



/***************************************************************************

	Generic NVRAM code

***************************************************************************/

/*-------------------------------------------------
	nvram_handler_generic_0fill - generic NVRAM
	with a 0 fill
-------------------------------------------------*/

void nvram_handler_generic_0fill(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file)
		osd_fread(file, generic_nvram, generic_nvram_size);
	else
		memset(generic_nvram, 0, generic_nvram_size);
}


/*-------------------------------------------------
	nvram_handler_generic_1fill - generic NVRAM
	with a 1 fill
-------------------------------------------------*/

void nvram_handler_generic_1fill(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file)
		osd_fread(file, generic_nvram, generic_nvram_size);
	else
		memset(generic_nvram, 0xff, generic_nvram_size);
}



/*-------------------------------------------------
	set_visible_area - adjusts the visible portion
	of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int min_x,int max_x,int min_y,int max_y)
{
	Machine->visible_area.min_x = min_x;
	Machine->visible_area.max_x = max_x;
	Machine->visible_area.min_y = min_y;
	Machine->visible_area.max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		min_x = 0;
		max_x = Machine->scrbitmap->width - 1;
		min_y = 0;
		max_y = Machine->scrbitmap->height - 1;
	}
	else
	{
		int temp;

		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			temp = min_x; min_x = min_y; min_y = temp;
			temp = max_x; max_x = max_y; max_y = temp;
		}
		if (Machine->orientation & ORIENTATION_FLIP_X)
		{
			temp = Machine->scrbitmap->width - min_x - 1;
			min_x = Machine->scrbitmap->width - max_x - 1;
			max_x = temp;
		}
		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			temp = Machine->scrbitmap->height - min_y - 1;
			min_y = Machine->scrbitmap->height - max_y - 1;
			max_y = temp;
		}
	}

	osd_set_visible_area(min_x,max_x,min_y,max_y);

	Machine->absolute_visible_area.min_x = min_x;
	Machine->absolute_visible_area.max_x = max_x;
	Machine->absolute_visible_area.min_y = min_y;
	Machine->absolute_visible_area.max_y = max_y;
}



/***************************************************************************

	Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
	bitmap_alloc_core
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	struct mame_bitmap *bitmap;

	/* cheesy kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
	{
		depth = -depth;
	}

	/* adjust for orientation */
	else if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp = width; width = height; height = temp;
	}

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(struct mame_bitmap)) : malloc(sizeof(struct mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		unsigned char *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(unsigned char *);

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (unsigned char *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		set_pixel_functions(bitmap);
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
	bitmap_alloc - allocate a bitmap at the
	current screen depth
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->scrbitmap->depth,0);
}


/*-------------------------------------------------
	bitmap_alloc_depth - allocate a bitmap for a
	specific depth
-------------------------------------------------*/

struct mame_bitmap *bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,0);
}


/*-------------------------------------------------
	bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(struct mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



/***************************************************************************

	Resource tracking code

***************************************************************************/

/*-------------------------------------------------
	auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *auto_malloc(size_t size)
{
	void *result = malloc(size);
	if (result)
	{
		struct malloc_info *info;

		/* make sure we have space */
		if (malloc_list_index >= MAX_MALLOCS)
		{
			fprintf(stderr, "Out of malloc tracking slots!\n");
			return result;
		}

		/* fill in the current entry */
		info = &malloc_list[malloc_list_index++];
		info->tag = get_resource_tag();
		info->ptr = result;
	}
	return result;
}


/*-------------------------------------------------
	end_resource_tracking - stop tracking
	resources
-------------------------------------------------*/

void auto_free(void)
{
	int tag = get_resource_tag();

	/* start at the end and free everything on the current tag */
	while (malloc_list_index > 0 && malloc_list[malloc_list_index - 1].tag >= tag)
	{
		struct malloc_info *info = &malloc_list[--malloc_list_index];
		free(info->ptr);
	}
}


/*-------------------------------------------------
	bitmap_alloc - allocate a bitmap at the
	current screen depth
-------------------------------------------------*/

struct mame_bitmap *auto_bitmap_alloc(int width,int height)
{
	return bitmap_alloc_core(width,height,Machine->scrbitmap->depth,1);
}


/*-------------------------------------------------
	bitmap_alloc_depth - allocate a bitmap for a
	specific depth
-------------------------------------------------*/

struct mame_bitmap *auto_bitmap_alloc_depth(int width,int height,int depth)
{
	return bitmap_alloc_core(width,height,depth,1);
}


/*-------------------------------------------------
	begin_resource_tracking - start tracking
	resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
	end_resource_tracking - stop tracking
	resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	/* call everyone who tracks resources to let them know */
	auto_free();
	timer_free();

	/* decrement the tag counter */
	resource_tracking_tag--;
}



/***************************************************************************

	Screen snapshot code

***************************************************************************/

/*-------------------------------------------------
	save_screen_snapshot_as - save a snapshot to
	the given filename
-------------------------------------------------*/

void save_screen_snapshot_as(void *fp,struct mame_bitmap *bitmap)
{
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		png_write_bitmap(fp,bitmap);
	else
	{
		struct mame_bitmap *copy;
		int sizex, sizey, scalex, scaley;

		sizex = Machine->visible_area.max_x - Machine->visible_area.min_x + 1;
		sizey = Machine->visible_area.max_y - Machine->visible_area.min_y + 1;

		scalex = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_2_1) ? 2 : 1;
		scaley = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) ? 2 : 1;

		copy = bitmap_alloc_depth(sizex * scalex,sizey * scaley,bitmap->depth);

		if (copy)
		{
			int x,y,sx,sy;

			sx = Machine->absolute_visible_area.min_x;
			sy = Machine->absolute_visible_area.min_y;
			if (Machine->orientation & ORIENTATION_SWAP_XY)
			{
				int t;

				t = scalex; scalex = scaley; scaley = t;
			}

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			png_write_bitmap(fp,copy);
			bitmap_free(copy);
		}
	}
}



/*-------------------------------------------------
	save_screen_snapshot - save a screen snapshot
-------------------------------------------------*/

void save_screen_snapshot(struct mame_bitmap *bitmap)
{
	char name[20];
	void *fp;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.png" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (osd_faccess(name,OSD_FILETYPE_SCREENSHOT))
	{
		do
		{
			/* otherwise use "nameNNNN.png" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name,snapno++);
		} while (osd_faccess(name, OSD_FILETYPE_SCREENSHOT));
	}

	if ((fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_SCREENSHOT, 1)) != NULL)
	{
		save_screen_snapshot_as(fp,bitmap);
		osd_fclose(fp);
	}
}



/***************************************************************************

	ROM loading code

***************************************************************************/

/*-------------------------------------------------
	rom_first_region - return pointer to first ROM
	region
-------------------------------------------------*/

const struct RomModule *rom_first_region(const struct GameDriver *drv)
{
	return drv->rom;
}


/*-------------------------------------------------
	rom_next_region - return pointer to next ROM
	region
-------------------------------------------------*/

const struct RomModule *rom_next_region(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_first_file - return pointer to first ROM
	file
-------------------------------------------------*/

const struct RomModule *rom_first_file(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_next_file - return pointer to next ROM
	file
-------------------------------------------------*/

const struct RomModule *rom_next_file(const struct RomModule *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
	rom_first_chunk - return pointer to first ROM
	chunk
-------------------------------------------------*/

const struct RomModule *rom_first_chunk(const struct RomModule *romp)
{
	return (ROMENTRY_ISFILE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
	rom_next_chunk - return pointer to next ROM
	chunk
-------------------------------------------------*/

const struct RomModule *rom_next_chunk(const struct RomModule *romp)
{
	romp++;
	return (ROMENTRY_ISCONTINUE(romp)) ? romp : NULL;
}



/*-------------------------------------------------
	debugload - log data to a file
-------------------------------------------------*/

void CLIB_DECL debugload(const char *string, ...)
{
#ifdef LOG_LOAD
	static int opened;
	va_list arg;
	FILE *f;

	f = fopen("romload.log", opened++ ? "a" : "w");
	if (f)
	{
		va_start(arg, string);
		vfprintf(f, string, arg);
		va_end(arg);
		fclose(f);
	}
#endif
}


/*-------------------------------------------------
	count_roms - counts the total number of ROMs
	that will need to be loaded
-------------------------------------------------*/

static int count_roms(const struct RomModule *romp)
{
	const struct RomModule *region, *rom;
	int count = 0;

	/* loop over regions, then over files */
	for (region = romp; region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			count++;

	/* return the total count */
	return count;
}


/*-------------------------------------------------
	fill_random - fills an area of memory with
	random data
-------------------------------------------------*/

static void fill_random(UINT8 *base, UINT32 length)
{
	while (length--)
		*base++ = rand();
}


/*-------------------------------------------------
	handle_missing_file - handles error generation
	for missing files
-------------------------------------------------*/

static void handle_missing_file(struct rom_load_data *romdata, const struct RomModule *romp)
{
	/* optional files are okay */
	if (ROM_ISOPTIONAL(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "OPTIONAL %-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* no good dumps are okay */
	else if (ROM_NOGOODDUMP(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND (NO GOOD DUMP KNOWN)\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* anything else is bad */
	else
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->errors++;
	}
}


/*-------------------------------------------------
	verify_length_and_crc - verify the length
	and CRC of a file
-------------------------------------------------*/

static void verify_length_and_crc(struct rom_load_data *romdata, const char *name, UINT32 explength, UINT32 expcrc)
{
	UINT32 actlength, actcrc;

	/* we've already complained if there is no file */
	if (!romdata->file)
		return;

	/* get the length and CRC from the file */
	actlength = osd_fsize(romdata->file);
	actcrc = osd_fcrc(romdata->file);

	/* verify length */
	if (explength != actlength)
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG LENGTH (expected: %08x found: %08x)\n", name, explength, actlength);
		romdata->warnings++;
	}

	/* verify CRC */
	if (expcrc != actcrc)
	{
		/* expected CRC == 0 means no good dump known */
		if (expcrc == 0)
			sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NO GOOD DUMP KNOWN\n", name);

		/* inverted CRC means needs redump */
		else if (expcrc == BADCRC(actcrc))
			sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s ROM NEEDS REDUMP\n",name);

		/* otherwise, it's just bad */
		else
			sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG CRC (expected: %08x found: %08x)\n", name, expcrc, actcrc);
		romdata->warnings++;
	}
}


/*-------------------------------------------------
	display_rom_load_results - display the final
	results of ROM loading
-------------------------------------------------*/

static int display_rom_load_results(struct rom_load_data *romdata)
{
	int region;

	/* final status display */
	osd_display_loading_rom_message(NULL, romdata->romsloaded, romdata->romstotal);

	/* only display if we have warnings or errors */
	if (romdata->warnings || romdata->errors)
	{
		extern int bailing;

		/* display either an error message or a warning message */
		if (romdata->errors)
		{
			strcat(romdata->errorbuf, "ERROR: required files are missing, the game cannot be run.\n");
			bailing = 1;
		}
		else
			strcat(romdata->errorbuf, "WARNING: the game might not run correctly.\n");

		/* display the result */
		printf("%s", romdata->errorbuf);

		/* if we're not getting out of here, wait for a keypress */
		if (!options.gui_host && !bailing)
		{
			int k;

			/* loop until we get one */
			printf ("Press any key to continue\n");
			do
			{
				k = code_read_async();
			}
			while (k == CODE_NONE || k == KEYCODE_LCONTROL);

			/* bail on a control + C */
			if (keyboard_pressed(KEYCODE_LCONTROL) && keyboard_pressed(KEYCODE_C))
				return 1;
		}
	}

	/* clean up any regions */
	if (romdata->errors)
		for (region = 0; region < MAX_MEMORY_REGIONS; region++)
			free_memory_region(region);

	/* return true if we had any errors */
	return (romdata->errors != 0);
}


/*-------------------------------------------------
	region_post_process - post-process a region,
	byte swapping and inverting data as necessary
-------------------------------------------------*/

static void region_post_process(struct rom_load_data *romdata, const struct RomModule *regiondata)
{
	int type = ROMREGION_GETTYPE(regiondata);
	int datawidth = ROMREGION_GETWIDTH(regiondata) / 8;
	int littleendian = ROMREGION_ISLITTLEENDIAN(regiondata);
	UINT8 *base;
	int i, j;

	debugload("+ datawidth=%d little=%d\n", datawidth, littleendian);

	/* if this is a CPU region, override with the CPU width and endianness */
	if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
	{
		int cputype = Machine->drv->cpu[type - REGION_CPU1].cpu_type & ~CPU_FLAGS_MASK;
		if (cputype != 0)
		{
			datawidth = cputype_databus_width(cputype) / 8;
			littleendian = (cputype_endianess(cputype) == CPU_IS_LE);
			debugload("+ CPU region #%d: datawidth=%d little=%d\n", type - REGION_CPU1, datawidth, littleendian);
		}
	}

	/* if the region is inverted, do that now */
	if (ROMREGION_ISINVERTED(regiondata))
	{
		debugload("+ Inverting region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i++)
			*base++ ^= 0xff;
	}

	/* swap the endianness if we need to */
#ifdef LSB_FIRST
	if (datawidth > 1 && !littleendian)
#else
	if (datawidth > 1 && littleendian)
#endif
	{
		debugload("+ Byte swapping region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i += datawidth)
		{
			UINT8 temp[8];
			memcpy(temp, base, datawidth);
			for (j = datawidth - 1; j >= 0; j--)
				*base++ = temp[j];
		}
	}
}


/*-------------------------------------------------
	open_rom_file - open a ROM file, searching
	up the parent and loading via CRC
-------------------------------------------------*/

static int open_rom_file(struct rom_load_data *romdata, const struct RomModule *romp)
{
	const struct GameDriver *drv;
	char crc[9];

	/* update status display */
	if (osd_display_loading_rom_message(ROM_GETNAME(romp), ++romdata->romsloaded, romdata->romstotal) != 0)
       return 0;

	/* first attempt reading up the chain through the parents */
	romdata->file = NULL;
	for (drv = Machine->gamedrv; !romdata->file && drv; drv = drv->clone_of)
		if (drv->name && *drv->name)
			romdata->file = osd_fopen(drv->name, ROM_GETNAME(romp), OSD_FILETYPE_ROM, 0);

	/* if that failed, attempt to open via CRC */
	sprintf(crc, "%08x", ROM_GETCRC(romp));
	for (drv = Machine->gamedrv; !romdata->file && drv; drv = drv->clone_of)
		if (drv->name && *drv->name)
			romdata->file = osd_fopen(drv->name, crc, OSD_FILETYPE_ROM, 0);

	/* return the result */
	return (romdata->file != NULL);
}


/*-------------------------------------------------
	rom_fread - cheesy fread that fills with
	random data for a NULL file
-------------------------------------------------*/

static int rom_fread(struct rom_load_data *romdata, UINT8 *buffer, int length)
{
	/* files just pass through */
	if (romdata->file)
		return osd_fread(romdata->file, buffer, length);

	/* otherwise, fill with randomness */
	else
		fill_random(buffer, length);

	return length;
}


/*-------------------------------------------------
	read_rom_data - read ROM data for a single
	entry
-------------------------------------------------*/

static int read_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	int datashift = ROM_GETBITSHIFT(romp);
	int datamask = ((1 << ROM_GETBITWIDTH(romp)) - 1) << datashift;
	int numbytes = ROM_GETLENGTH(romp);
	int groupsize = ROM_GETGROUPSIZE(romp);
	int skip = ROM_GETSKIPCOUNT(romp);
	int reversed = ROM_ISREVERSED(romp);
	int numgroups = (numbytes + groupsize - 1) / groupsize;
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int i;

	debugload("Loading ROM data: offs=%X len=%X mask=%02X group=%d skip=%d reverse=%d\n", ROM_GETOFFSET(romp), numbytes, datamask, groupsize, skip, reversed);

	/* make sure the length was an even multiple of the group size */
	if (numbytes % groupsize != 0)
	{
		printf("Error in RomModule definition: %s length not an even multiple of group size\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure we only fill within the region space */
	if (ROM_GETOFFSET(romp) + numgroups * groupsize + (numgroups - 1) * skip > romdata->regionlength)
	{
		printf("Error in RomModule definition: %s out of memory region space\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: %s has an invalid length\n", ROM_GETNAME(romp));
		return -1;
	}

	/* special case for simple loads */
	if (datamask == 0xff && (groupsize == 1 || !reversed) && skip == 0)
		return rom_fread(romdata, base, numbytes);

	/* chunky reads for complex loads */
	skip += groupsize;
	while (numbytes)
	{
		int evengroupcount = (sizeof(romdata->tempbuf) / groupsize) * groupsize;
		int bytesleft = (numbytes > evengroupcount) ? evengroupcount : numbytes;
		UINT8 *bufptr = romdata->tempbuf;

		/* read as much as we can */
		debugload("  Reading %X bytes into buffer\n", bytesleft);
		if (rom_fread(romdata, romdata->tempbuf, bytesleft) != bytesleft)
			return 0;
		numbytes -= bytesleft;

		debugload("  Copying to %08X\n", (int)base);

		/* unmasked cases */
		if (datamask == 0xff)
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = *bufptr++;

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}
		}

		/* masked cases */
		else
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = (*base & ~datamask) | ((*bufptr++ << datashift) & datamask);

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}
		}
	}
	debugload("  All done\n");
	return ROM_GETLENGTH(romp);
}


/*-------------------------------------------------
	fill_rom_data - fill a region of ROM space
-------------------------------------------------*/

static int fill_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);

	/* make sure we fill within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: FILL out of memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: FILL has an invalid length\n");
		return 0;
	}

	/* fill the data */
	memset(base, ROM_GETCRC(romp) & 0xff, numbytes);
	return 1;
}


/*-------------------------------------------------
	copy_rom_data - copy a region of ROM space
-------------------------------------------------*/

static int copy_rom_data(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int srcregion = ROM_GETFLAGS(romp) >> 24;
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT32 srcoffs = ROM_GETCRC(romp);
	UINT8 *srcbase;

	/* make sure we copy within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: COPY out of target memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: COPY has an invalid length\n");
		return 0;
	}

	/* make sure the source was valid */
	srcbase = memory_region(srcregion);
	if (!srcbase)
	{
		printf("Error in RomModule definition: COPY from an invalid region\n");
		return 0;
	}

	/* make sure we find within the region space */
	if (srcoffs + numbytes > memory_region_length(srcregion))
	{
		printf("Error in RomModule definition: COPY out of source memory region space\n");
		return 0;
	}

	/* fill the data */
	memcpy(base, srcbase + srcoffs, numbytes);
	return 1;
}


/*-------------------------------------------------
	process_rom_entries - process all ROM entries
	for a region
-------------------------------------------------*/

static int process_rom_entries(struct rom_load_data *romdata, const struct RomModule *romp)
{
	UINT32 lastflags = 0;

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* if this is a continue entry, it's invalid */
		if (ROMENTRY_ISCONTINUE(romp))
		{
			printf("Error in RomModule definition: ROM_CONTINUE not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* if this is a reload entry, it's invalid */
		if (ROMENTRY_ISRELOAD(romp))
		{
			printf("Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* handle fills */
		if (ROMENTRY_ISFILL(romp))
		{
			if (!fill_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle copies */
		else if (ROMENTRY_ISCOPY(romp))
		{
			if (!copy_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle files */
		else if (ROMENTRY_ISFILE(romp))
		{
			const struct RomModule *baserom = romp;
			int explength = 0;

			/* open the file */
			debugload("Opening ROM file: %s\n", ROM_GETNAME(romp));
			if (!open_rom_file(romdata, romp))
				handle_missing_file(romdata, romp);

			/* loop until we run out of reloads */
			do
			{
				/* loop until we run out of continues */
				do
				{
					struct RomModule modified_romp = *romp++;
					int readresult;

					/* handle flag inheritance */
					if (!ROM_INHERITSFLAGS(&modified_romp))
						lastflags = modified_romp._length & ROM_INHERITEDFLAGS;
					else
						modified_romp._length = (modified_romp._length & ~ROM_INHERITEDFLAGS) | lastflags;

					explength += UNCOMPACT_LENGTH(modified_romp._length);

                    /* attempt to read using the modified entry */
					readresult = read_rom_data(romdata, &modified_romp);
					if (readresult == -1)
						goto fatalerror;
				}
				while (ROMENTRY_ISCONTINUE(romp));

				/* if this was the first use of this file, verify the length and CRC */
				if (baserom)
				{
					debugload("Verifying length (%X) and CRC (%08X)\n", explength, ROM_GETCRC(baserom));
					verify_length_and_crc(romdata, ROM_GETNAME(baserom), explength, ROM_GETCRC(baserom));
					debugload("Verify succeeded\n");
				}

				/* reseek to the start and clear the baserom so we don't reverify */
				if (romdata->file)
					osd_fseek(romdata->file, 0, SEEK_SET);
				baserom = NULL;
				explength = 0;
			}
			while (ROMENTRY_ISRELOAD(romp));

			/* close the file */
			if (romdata->file)
			{
				debugload("Closing ROM file\n");
				osd_fclose(romdata->file);
				romdata->file = NULL;
			}
		}
	}
	return 1;

	/* error case */
fatalerror:
	if (romdata->file)
		osd_fclose(romdata->file);
	romdata->file = NULL;
	return 0;
}


/*-------------------------------------------------
	readroms - load all the ROMs for this machine
-------------------------------------------------*/

int readroms(void)
{
	return rom_load_new(Machine->gamedrv->rom);
}


/*-------------------------------------------------
	rom_load_new - new, more flexible ROM
	loading system
-------------------------------------------------*/

int rom_load_new(const struct RomModule *romp)
{
	const struct RomModule *regionlist[REGION_MAX];
	const struct RomModule *region;
	static struct rom_load_data romdata;
	int regnum;

	/* reset the region list */
	for (regnum = 0;regnum < REGION_MAX;regnum++)
		regionlist[regnum] = NULL;

	/* reset the romdata struct */
	memset(&romdata, 0, sizeof(romdata));
	romdata.romstotal = count_roms(romp);

	/* loop until we hit the end */
	for (region = romp, regnum = 0; region; region = rom_next_region(region), regnum++)
	{
		int regiontype = ROMREGION_GETTYPE(region);

		debugload("Processing region %02X (length=%X)\n", regiontype, ROMREGION_GETLENGTH(region));

		/* the first entry must be a region */
		if (!ROMENTRY_ISREGION(region))
		{
			printf("Error: missing ROM_REGION header\n");
			return 1;
		}
#ifdef PINMAME /* MAME crahes if CPU regions are not created */
#  define ROMREGION_ISCPU(r) ((ROMREGION_GETTYPE(r) >= REGION_CPU1) && (ROMREGION_GETTYPE(r) <= REGION_CPU8))
              if (!ROMREGION_ISCPU(region))
#endif /* PINMAME */

		/* if sound is disabled and it's a sound-only region, skip it */
		if (Machine->sample_rate == 0 && ROMREGION_ISSOUNDONLY(region))
			continue;

		/* allocate memory for the region */
		if (new_memory_region(regiontype, ROMREGION_GETLENGTH(region), ROMREGION_GETFLAGS(region)))
		{
			printf("Error: unable to allocate memory for region %d\n", regiontype);
			return 1;
		}

		/* remember the base and length */
		romdata.regionlength = memory_region_length(regiontype);
		romdata.regionbase = memory_region(regiontype);
		debugload("Allocated %X bytes @ %08X\n", romdata.regionlength, (int)romdata.regionbase);

		/* clear the region if it's requested */
		if (ROMREGION_ISERASE(region))
			memset(romdata.regionbase, ROMREGION_GETERASEVAL(region), romdata.regionlength);

		/* or if it's sufficiently small (<= 4MB) */
		else if (romdata.regionlength <= 0x400000)
			memset(romdata.regionbase, 0, romdata.regionlength);

#ifdef MAME_DEBUG
		/* if we're debugging, fill region with random data to catch errors */
		else
			fill_random(romdata.regionbase, romdata.regionlength);
#endif

		/* now process the entries in the region */
		if (!process_rom_entries(&romdata, region + 1))
			return 1;

		/* add this region to the list */
		if (regiontype < REGION_MAX)
			regionlist[regiontype] = region;
	}

	/* post-process the regions */
	for (regnum = 0; regnum < REGION_MAX; regnum++)
		if (regionlist[regnum])
		{
			debugload("Post-processing region %02X\n", regnum);
			romdata.regionlength = memory_region_length(regnum);
			romdata.regionbase = memory_region(regnum);
			region_post_process(&romdata, regionlist[regnum]);
		}

	/* display the results and exit */
	return display_rom_load_results(&romdata);
}


/*-------------------------------------------------
	printromlist - print list of ROMs
-------------------------------------------------*/

void printromlist(const struct RomModule *romp,const char *basename)
{
	const struct RomModule *region, *rom, *chunk;

	if (!romp) return;

#ifdef MESS
	if (!strcmp(basename,"nes")) return;
#endif

	printf("This is the list of the ROMs required for driver \"%s\".\n"
			"Name              Size       Checksum\n",basename);

	for (region = romp; region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			const char *name = ROM_GETNAME(rom);
			int expchecksum = ROM_GETCRC(rom);
			int length = 0;

			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			if (expchecksum)
				printf("%-12s  %7d bytes  %08x\n",name,length,expchecksum);
			else
				printf("%-12s  %7d bytes  NO GOOD DUMP KNOWN\n",name,length);
		}
	}
}
