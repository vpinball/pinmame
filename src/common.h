/*********************************************************************

	common.h

	Generic functions, mostly ROM related.

*********************************************************************/

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************

	Type definitions

***************************************************************************/

struct RomModule
{
	const char *_name;	/* name of the file to load */
	UINT32 _offset;		/* offset to load it to */
	UINT32 _length;		/* length of the file */
	UINT32 _crc;		/* standard CRC-32 checksum */
};


struct GameSample
{
	int length;
	int smpfreq;
	int resolution;
	signed char data[1];	/* extendable */
};


struct GameSamples
{
	int total;	/* total number of samples */
	struct GameSample *sample[1];	/* extendable */
};



/***************************************************************************

	Constants and macros

***************************************************************************/

enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_MAX
};

#define BADCRC( crc ) (~(crc))



/***************************************************************************

	Core macros for the ROM loading system

***************************************************************************/

/* ----- length compaction macros ----- */
#define INVALID_LENGTH 				0x7ff
#define COMPACT_LENGTH(x)									\
	((((x) & 0xffffff00) == 0) ? (0x000 | ((x) >> 0)) :		\
	 (((x) & 0xfffff00f) == 0) ? (0x100 | ((x) >> 4)) :		\
	 (((x) & 0xffff00ff) == 0) ? (0x200 | ((x) >> 8)) :		\
	 (((x) & 0xfff00fff) == 0) ? (0x300 | ((x) >> 12)) : 	\
	 (((x) & 0xff00ffff) == 0) ? (0x400 | ((x) >> 16)) : 	\
	 (((x) & 0xf00fffff) == 0) ? (0x500 | ((x) >> 20)) : 	\
	 (((x) & 0x00ffffff) == 0) ? (0x600 | ((x) >> 24)) : 	\
	 INVALID_LENGTH)
#define UNCOMPACT_LENGTH(x)	(((x) == INVALID_LENGTH) ? 0 : (((x) & 0xff) << (((x) & 0x700) >> 6)))


/* ----- per-entry constants ----- */
#define ROMENTRYTYPE_REGION			1					/* this entry marks the start of a region */
#define ROMENTRYTYPE_END			2					/* this entry marks the end of a region */
#define ROMENTRYTYPE_RELOAD			3					/* this entry reloads the previous ROM */
#define ROMENTRYTYPE_CONTINUE		4					/* this entry continues loading the previous ROM */
#define ROMENTRYTYPE_FILL			5					/* this entry fills an area with a constant value */
#define ROMENTRYTYPE_COPY			6					/* this entry copies data from another region/offset */
#define ROMENTRYTYPE_COUNT			7

#define ROMENTRY_REGION				((const char *)ROMENTRYTYPE_REGION)
#define ROMENTRY_END				((const char *)ROMENTRYTYPE_END)
#define ROMENTRY_RELOAD				((const char *)ROMENTRYTYPE_RELOAD)
#define ROMENTRY_CONTINUE			((const char *)ROMENTRYTYPE_CONTINUE)
#define ROMENTRY_FILL				((const char *)ROMENTRYTYPE_FILL)
#define ROMENTRY_COPY				((const char *)ROMENTRYTYPE_COPY)

/* ----- per-entry macros ----- */
#define ROMENTRY_GETTYPE(r)			((FPTR)(r)->_name)
#define ROMENTRY_ISSPECIAL(r)		(ROMENTRY_GETTYPE(r) < ROMENTRYTYPE_COUNT)
#define ROMENTRY_ISFILE(r)			(!ROMENTRY_ISSPECIAL(r))
#define ROMENTRY_ISREGION(r)		((r)->_name == ROMENTRY_REGION)
#define ROMENTRY_ISEND(r)			((r)->_name == ROMENTRY_END)
#define ROMENTRY_ISRELOAD(r)		((r)->_name == ROMENTRY_RELOAD)
#define ROMENTRY_ISCONTINUE(r)		((r)->_name == ROMENTRY_CONTINUE)
#define ROMENTRY_ISFILL(r)			((r)->_name == ROMENTRY_FILL)
#define ROMENTRY_ISCOPY(r)			((r)->_name == ROMENTRY_COPY)
#define ROMENTRY_ISREGIONEND(r)		(ROMENTRY_ISREGION(r) || ROMENTRY_ISEND(r))


/* ----- per-region constants ----- */
#define ROMREGION_WIDTHMASK			0x00000003			/* native width of region, as power of 2 */
#define		ROMREGION_8BIT			0x00000000			/*    (non-CPU regions only) */
#define		ROMREGION_16BIT			0x00000001
#define		ROMREGION_32BIT			0x00000002
#define		ROMREGION_64BIT			0x00000003

#define ROMREGION_ENDIANMASK		0x00000004			/* endianness of the region */
#define		ROMREGION_LE			0x00000000			/*    (non-CPU regions only) */
#define		ROMREGION_BE			0x00000004

#define ROMREGION_INVERTMASK		0x00000008			/* invert the bits of the region */
#define		ROMREGION_NOINVERT		0x00000000
#define		ROMREGION_INVERT		0x00000008

#define ROMREGION_DISPOSEMASK		0x00000010			/* dispose of the region after init */
#define		ROMREGION_NODISPOSE		0x00000000
#define		ROMREGION_DISPOSE		0x00000010

#define ROMREGION_SOUNDONLYMASK		0x00000020			/* load only if sound is enabled */
#define		ROMREGION_NONSOUND		0x00000000
#define		ROMREGION_SOUNDONLY		0x00000020

#define ROMREGION_LOADUPPERMASK		0x00000040			/* load into the upper part of CPU space */
#define		ROMREGION_LOADLOWER		0x00000000			/*     (CPU regions only) */
#define		ROMREGION_LOADUPPER		0x00000040

#define ROMREGION_ERASEMASK			0x00000080			/* erase the region before loading */
#define		ROMREGION_NOERASE		0x00000000
#define		ROMREGION_ERASE			0x00000080

#define ROMREGION_ERASEVALMASK		0x0000ff00			/* value to erase the region to */
#define		ROMREGION_ERASEVAL(x)	((((x) & 0xff) << 8) | ROMREGION_ERASE)
#define		ROMREGION_ERASE00		ROMREGION_ERASEVAL(0)
#define		ROMREGION_ERASEFF		ROMREGION_ERASEVAL(0xff)

/* ----- per-region macros ----- */
#define ROMREGION_GETTYPE(r)		((r)->_crc)
#define ROMREGION_GETLENGTH(r)		((r)->_length)
#define ROMREGION_GETFLAGS(r)		((r)->_offset)
#define ROMREGION_GETWIDTH(r)		(8 << (ROMREGION_GETFLAGS(r) & ROMREGION_WIDTHMASK))
#define ROMREGION_ISLITTLEENDIAN(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ENDIANMASK) == ROMREGION_LE)
#define ROMREGION_ISBIGENDIAN(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ENDIANMASK) == ROMREGION_BE)
#define ROMREGION_ISINVERTED(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_INVERTMASK) == ROMREGION_INVERT)
#define ROMREGION_ISDISPOSE(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_DISPOSEMASK) == ROMREGION_DISPOSE)
#define ROMREGION_ISSOUNDONLY(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_SOUNDONLYMASK) == ROMREGION_SOUNDONLY)
#define ROMREGION_ISLOADUPPER(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_LOADUPPERMASK) == ROMREGION_LOADUPPER)
#define ROMREGION_ISERASE(r)		((ROMREGION_GETFLAGS(r) & ROMREGION_ERASEMASK) == ROMREGION_ERASE)
#define ROMREGION_GETERASEVAL(r)	((ROMREGION_GETFLAGS(r) & ROMREGION_ERASEVALMASK) >> 8)


/* ----- per-ROM constants ----- */
#define ROM_LENGTHMASK				0x000007ff			/* the compacted length of the ROM */
#define		ROM_INVALIDLENGTH		INVALID_LENGTH

#define ROM_OPTIONALMASK			0x00000800			/* optional - won't hurt if it's not there */
#define		ROM_REQUIRED			0x00000000
#define		ROM_OPTIONAL			0x00000800

#define ROM_GROUPMASK				0x0000f000			/* load data in groups of this size + 1 */
#define		ROM_GROUPSIZE(n)		((((n) - 1) & 15) << 12)
#define		ROM_GROUPBYTE			ROM_GROUPSIZE(1)
#define		ROM_GROUPWORD			ROM_GROUPSIZE(2)
#define		ROM_GROUPDWORD			ROM_GROUPSIZE(4)

#define ROM_SKIPMASK				0x000f0000			/* skip this many bytes after each group */
#define		ROM_SKIP(n)				(((n) & 15) << 16)
#define		ROM_NOSKIP				ROM_SKIP(0)

#define ROM_REVERSEMASK				0x00100000			/* reverse the byte order within a group */
#define		ROM_NOREVERSE			0x00000000
#define		ROM_REVERSE				0x00100000

#define ROM_BITWIDTHMASK			0x00e00000			/* width of data in bits */
#define		ROM_BITWIDTH(n)			(((n) & 7) << 21)
#define		ROM_NIBBLE				ROM_BITWIDTH(4)
#define		ROM_FULLBYTE			ROM_BITWIDTH(8)

#define ROM_BITSHIFTMASK			0x07000000			/* left-shift count for the bits */
#define		ROM_BITSHIFT(n)			(((n) & 7) << 24)
#define		ROM_NOSHIFT				ROM_BITSHIFT(0)
#define		ROM_SHIFT_NIBBLE_LO		ROM_BITSHIFT(0)
#define		ROM_SHIFT_NIBBLE_HI		ROM_BITSHIFT(4)

#define ROM_INHERITFLAGSMASK		0x08000000			/* inherit all flags from previous definition */
#define		ROM_INHERITFLAGS		0x08000000

#define ROM_INHERITEDFLAGS			(ROM_GROUPMASK | ROM_SKIPMASK | ROM_REVERSEMASK | ROM_BITWIDTHMASK | ROM_BITSHIFTMASK)

/* ----- per-ROM macros ----- */
#define ROM_GETNAME(r)				((r)->_name)
#define ROM_SAFEGETNAME(r)			(ROMENTRY_ISFILL(r) ? "fill" : ROMENTRY_ISCOPY(r) ? "copy" : ROM_GETNAME(r))
#define ROM_GETOFFSET(r)			((r)->_offset)
#define ROM_GETCRC(r)				((r)->_crc)
#define ROM_GETLENGTH(r)			(UNCOMPACT_LENGTH((r)->_length & ROM_LENGTHMASK))
#define ROM_GETFLAGS(r)				((r)->_length & ~ROM_LENGTHMASK)
#define ROM_ISOPTIONAL(r)			((ROM_GETFLAGS(r) & ROM_OPTIONALMASK) == ROM_OPTIONAL)
#define ROM_GETGROUPSIZE(r)			(((ROM_GETFLAGS(r) & ROM_GROUPMASK) >> 12) + 1)
#define ROM_GETSKIPCOUNT(r)			((ROM_GETFLAGS(r) & ROM_SKIPMASK) >> 16)
#define ROM_ISREVERSED(r)			((ROM_GETFLAGS(r) & ROM_REVERSEMASK) == ROM_REVERSE)
#define ROM_GETBITWIDTH(r)			(((ROM_GETFLAGS(r) & ROM_BITWIDTHMASK) >> 21) + 8 * ((ROM_GETFLAGS(r) & ROM_BITWIDTHMASK) == 0))
#define ROM_GETBITSHIFT(r)			((ROM_GETFLAGS(r) & ROM_BITSHIFTMASK) >> 24)
#define ROM_INHERITSFLAGS(r)		((ROM_GETFLAGS(r) & ROM_INHERITFLAGSMASK) == ROM_INHERITFLAGS)
#define ROM_NOGOODDUMP(r)			(ROM_GETCRC(r) == 0)



/***************************************************************************

	Derived macros for the ROM loading system

***************************************************************************/

/* ----- start/stop macros ----- */
#define ROM_START(name)								static const struct RomModule rom_##name[] = {
#define ROM_END										{ ROMENTRY_END, 0, 0, 0 } };

/* ----- ROM region macros ----- */
#define ROM_REGION(length,type,flags)				{ ROMENTRY_REGION, flags, length, type },
#define ROM_REGION16_LE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_16BIT | ROMREGION_LE)
#define ROM_REGION16_BE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_16BIT | ROMREGION_BE)
#define ROM_REGION32_LE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_32BIT | ROMREGION_LE)
#define ROM_REGION32_BE(length,type,flags)			ROM_REGION(length, type, (flags) | ROMREGION_32BIT | ROMREGION_BE)

/* ----- core ROM loading macros ----- */
#define ROMX_LOAD(name,offset,length,crc,flags)		{ name, offset, (flags) | COMPACT_LENGTH(length), crc },
#define ROM_LOAD(name,offset,length,crc)			ROMX_LOAD(name, offset, length, crc, 0)
#define ROM_LOAD_OPTIONAL(name,offset,length,crc)	ROMX_LOAD(name, offset, length, crc, ROM_OPTIONAL)
#define ROM_CONTINUE(offset,length)					ROMX_LOAD(ROMENTRY_CONTINUE, offset, length, 0, ROM_INHERITFLAGS)
#define ROM_RELOAD(offset,length)					ROMX_LOAD(ROMENTRY_RELOAD, offset, length, 0, ROM_INHERITFLAGS)
#define ROM_FILL(offset,length,value)				ROM_LOAD(ROMENTRY_FILL, offset, length, value)
#define ROM_COPY(rgn,srcoffset,offset,length)		ROMX_LOAD(ROMENTRY_COPY, offset, length, srcoffset, (rgn) << 24)

/* ----- nibble loading macros ----- */
#define ROM_LOAD_NIB_HIGH(name,offset,length,crc)	ROMX_LOAD(name, offset, length, crc, ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI)
#define ROM_LOAD_NIB_LOW(name,offset,length,crc)	ROMX_LOAD(name, offset, length, crc, ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO)

/* ----- new-style 16-bit loading macros ----- */
#define ROM_LOAD16_BYTE(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_SKIP(1))
#define ROM_LOAD16_WORD(name,offset,length,crc)		ROM_LOAD(name, offset, length, crc)
#define ROM_LOAD16_WORD_SWAP(name,offset,length,crc)ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_REVERSE)

/* ----- new-style 32-bit loading macros ----- */
#define ROM_LOAD32_BYTE(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_SKIP(3))
#define ROM_LOAD32_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(2))
#define ROM_LOAD32_WORD_SWAP(name,offset,length,crc)ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_REVERSE | ROM_SKIP(2))



/***************************************************************************

	Function prototypes

***************************************************************************/

void showdisclaimer(void);

const struct RomModule *rom_first_region(const struct GameDriver *drv);
const struct RomModule *rom_next_region(const struct RomModule *romp);
const struct RomModule *rom_first_file(const struct RomModule *romp);
const struct RomModule *rom_next_file(const struct RomModule *romp);
const struct RomModule *rom_first_chunk(const struct RomModule *romp);
const struct RomModule *rom_next_chunk(const struct RomModule *romp);


/* LBO 042898 - added coin counters */
#define COIN_COUNTERS	4	/* total # of coin counters */
void coin_counter_w(int num,int on);
void coin_lockout_w(int num,int on);
void coin_lockout_global_w(int on);  /* Locks out all coin inputs */


int readroms(void);
void printromlist(const struct RomModule *romp,const char *name);

/* helper function that reads samples from disk - this can be used by other */
/* drivers as well (e.g. a sound chip emulator needing drum samples) */
struct GameSamples *readsamples(const char **samplenames,const char *name);
void freesamples(struct GameSamples *samples);

/* return a pointer to the specified memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
UINT8 *memory_region(int num);
size_t memory_region_length(int num);
/* allocate a new memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
int new_memory_region(int num, size_t length, UINT32 flags);
void free_memory_region(int num);

extern int flip_screen_x, flip_screen_y;

void flip_screen_set(int on);
void flip_screen_x_set(int on);
void flip_screen_y_set(int on);
#define flip_screen flip_screen_x

/* sets a variable and schedules a full screen refresh if it changed */
void set_vh_global_attribute( int *addr, int data );

void set_visible_area(int min_x,int max_x,int min_y,int max_y);

struct osd_bitmap *bitmap_alloc(int width,int height);
struct osd_bitmap *bitmap_alloc_depth(int width,int height,int depth);
void bitmap_free(struct osd_bitmap *bitmap);

void save_screen_snapshot_as(void *fp,struct osd_bitmap *bitmap);
void save_screen_snapshot(struct osd_bitmap *bitmap);



/***************************************************************************

	Useful macros to deal with bit shuffling encryptions

***************************************************************************/

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B7)) & 1) << 7) | \
		 ((((val) >> (B6)) & 1) << 6) | \
		 ((((val) >> (B5)) & 1) << 5) | \
		 ((((val) >> (B4)) & 1) << 4) | \
		 ((((val) >> (B3)) & 1) << 3) | \
		 ((((val) >> (B2)) & 1) << 2) | \
		 ((((val) >> (B1)) & 1) << 1) | \
		 ((((val) >> (B0)) & 1) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B15)) & 1) << 15) | \
		 ((((val) >> (B14)) & 1) << 14) | \
		 ((((val) >> (B13)) & 1) << 13) | \
		 ((((val) >> (B12)) & 1) << 12) | \
		 ((((val) >> (B11)) & 1) << 11) | \
		 ((((val) >> (B10)) & 1) << 10) | \
		 ((((val) >> ( B9)) & 1) <<  9) | \
		 ((((val) >> ( B8)) & 1) <<  8) | \
		 ((((val) >> ( B7)) & 1) <<  7) | \
		 ((((val) >> ( B6)) & 1) <<  6) | \
		 ((((val) >> ( B5)) & 1) <<  5) | \
		 ((((val) >> ( B4)) & 1) <<  4) | \
		 ((((val) >> ( B3)) & 1) <<  3) | \
		 ((((val) >> ( B2)) & 1) <<  2) | \
		 ((((val) >> ( B1)) & 1) <<  1) | \
		 ((((val) >> ( B0)) & 1) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		(((((val) >> (B23)) & 1) << 23) | \
		 ((((val) >> (B22)) & 1) << 22) | \
		 ((((val) >> (B21)) & 1) << 21) | \
		 ((((val) >> (B20)) & 1) << 20) | \
		 ((((val) >> (B19)) & 1) << 19) | \
		 ((((val) >> (B18)) & 1) << 18) | \
		 ((((val) >> (B17)) & 1) << 17) | \
		 ((((val) >> (B16)) & 1) << 16) | \
		 ((((val) >> (B15)) & 1) << 15) | \
		 ((((val) >> (B14)) & 1) << 14) | \
		 ((((val) >> (B13)) & 1) << 13) | \
		 ((((val) >> (B12)) & 1) << 12) | \
		 ((((val) >> (B11)) & 1) << 11) | \
		 ((((val) >> (B10)) & 1) << 10) | \
		 ((((val) >> ( B9)) & 1) <<  9) | \
		 ((((val) >> ( B8)) & 1) <<  8) | \
		 ((((val) >> ( B7)) & 1) <<  7) | \
		 ((((val) >> ( B6)) & 1) <<  6) | \
		 ((((val) >> ( B5)) & 1) <<  5) | \
		 ((((val) >> ( B4)) & 1) <<  4) | \
		 ((((val) >> ( B3)) & 1) <<  3) | \
		 ((((val) >> ( B2)) & 1) <<  2) | \
		 ((((val) >> ( B1)) & 1) <<  1) | \
		 ((((val) >> ( B0)) & 1) <<  0))


#ifdef __cplusplus
}
#endif

#endif
