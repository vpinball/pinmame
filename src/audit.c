#include "driver.h"
#include <stdarg.h>
#include <string.h>
#include "audit.h"
#include "harddisk.h"

static tAuditRecord *gAudits = NULL;

static const struct GameDriver *hard_disk_gamedrv;

/*-------------------------------------------------
	audit_hard_disk_open - interface for opening
	a hard disk image
-------------------------------------------------*/

void *audit_hard_disk_open(const char *filename, const char *mode)
{
	return mame_fopen(hard_disk_gamedrv->name, filename, FILETYPE_IMAGE, 0);
}



/*-------------------------------------------------
	audit_hard_disk_close - interface for closing
	a hard disk image
-------------------------------------------------*/

void audit_hard_disk_close(void *file)
{
	mame_fclose((mame_file *)file);
}



/*-------------------------------------------------
	audit_hard_disk_read - interface for reading
	from a hard disk image
-------------------------------------------------*/

UINT32 audit_hard_disk_read(void *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



/*-------------------------------------------------
	audit_hard_disk_write - interface for writing
	to a hard disk image
-------------------------------------------------*/

UINT32 audit_hard_disk_write(void *file, UINT64 offset, UINT32 count, const void *buffer)
{
	return 0;
}



static struct hard_disk_interface audit_hard_disk_interface =
{
	audit_hard_disk_open,
	audit_hard_disk_close,
	audit_hard_disk_read,
	audit_hard_disk_write
};


/* returns 1 if rom is defined in this set */
int RomInSet (const struct GameDriver *gamedrv, const char* hash)
{
	const struct RomModule *region, *rom;

	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			/* Compare all the available checksums */
			if (hash_data_is_equal(ROM_GETHASHDATA(rom), hash, 0))
				return 1;

	return 0;
}


/* returns nonzero if romset is missing */
int RomsetMissing (int game)
{
	const struct GameDriver *gamedrv = drivers[game];

	if (gamedrv->clone_of)
	{
		tAuditRecord	*aud;
		int				count;
		int 			i;

#if 1
        int cloneRomsFound = 0;
        int uniqueRomsFound = 0;

		if ((count = AuditRomSet (game, &aud)) == 0)
			return 1;

		if (count == -1) return 0;

        /* count number of roms found that are unique to clone */
        for (i = 0; i < count; i++)
			if (!RomInSet (gamedrv->clone_of, aud[i].exphash))
			{
				uniqueRomsFound++;
				if (aud[i].status != AUD_ROM_NOT_FOUND)
					cloneRomsFound++;
			}
#else
		int cloneRomsFound = 0;

		if ((count = AuditRomSet (game, &aud)) == 0)
			return 1;

		if (count == -1) return 0;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!RomInSet (gamedrv->clone_of, aud[i].exphash))
					cloneRomsFound++;
#endif

		return !cloneRomsFound;
	}
	else
		return !mame_faccess (gamedrv->name, FILETYPE_ROM);
}

/* Fills in an audit record for each rom in the romset. Sets 'audit' to
   point to the list of audit records. Returns total number of roms
   in the romset (same as number of audit records), 0 if romset missing. */
int AuditRomSet (int game, tAuditRecord **audit)
{
	const struct RomModule *region, *rom, *chunk;
	const char *name;
	const struct GameDriver *gamedrv;

	int count = 0;
	tAuditRecord *aud;
	int	err;

	if (!gAudits)
	{
		gAudits = (tAuditRecord *)malloc (AUD_MAX_ROMS * sizeof (tAuditRecord));

		// Make sure the memory is cleared - it's needed by the hashing
		//  engine
		memset(gAudits, 0, AUD_MAX_ROMS * sizeof(tAuditRecord));
	}

	if (gAudits)
		*audit = aud = gAudits;
	else
		return 0;

	gamedrv = drivers[game];

	if (!gamedrv->rom) return -1;

	/* check for existence of romset */
	if (!mame_faccess (gamedrv->name, FILETYPE_ROM))
	{
		/* if the game is a clone, check for parent */
		if (gamedrv->clone_of == 0 || (gamedrv->clone_of->flags & NOT_A_DRIVER) ||
				!mame_faccess(gamedrv->clone_of->name,FILETYPE_ROM))
			return 0;
	}

	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			if (ROMREGION_ISROMDATA(region))
			{
				const struct GameDriver *drv;

				name = ROM_GETNAME(rom);
				strcpy (aud->rom, name);
				aud->explength = 0;
				aud->length = 0;
				aud->exphash = ROM_GETHASHDATA(rom);

				/* Copy into the variable we pass to the functions
				   to support load-by-checksum */
				hash_data_copy(aud->hash, aud->exphash);

				count++;

				/* obtain hash checksums and length of ROM file */
				drv = gamedrv;
				do
				{
					err = mame_fchecksum(drv->name, name, &aud->length, aud->hash);
					drv = drv->clone_of;
				} while (err && drv);

				/* spin through ROM_CONTINUEs, totaling length */
				for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
					aud->explength += ROM_GETLENGTH(chunk);

				if (err)
				{
					if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
						/* not found but it's not good anyway */
						aud->status = AUD_NOT_AVAILABLE;
					else
						/* not found */
						aud->status = AUD_ROM_NOT_FOUND;
				}
				/* all cases below assume the ROM was at least found */
				else if (aud->explength != aud->length)
					aud->status = AUD_LENGTH_MISMATCH;
				else if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
						aud->status = AUD_ROM_NEED_DUMP; /* new case - found but not known to be dumped */
				else if (!hash_data_is_equal(aud->exphash, aud->hash, 0))
				{
					/* non-matching hash */
						aud->status = AUD_BAD_CHECKSUM;
				}
				else
				{
					/* matching hash */
					if (hash_data_has_info(aud->exphash, HASH_INFO_BAD_DUMP))
						aud->status = AUD_ROM_NEED_REDUMP;
					else
					aud->status = AUD_ROM_GOOD;
				}

				aud++;
			}
			else if (ROMREGION_ISDISKDATA(region))
			{
				void *source;
				struct hard_disk_header header;

				name = ROM_GETNAME(rom);
				strcpy (aud->rom, name);
				aud->explength = 0;
				aud->length = 0;
				aud->exphash = ROM_GETHASHDATA(rom);
				hash_data_clear(aud->hash);
				count++;

				hard_disk_gamedrv = gamedrv;
				hard_disk_set_interface(&audit_hard_disk_interface);
				source = hard_disk_open( name, 0, NULL );
				if( source == NULL )
				{
					err = hard_disk_get_last_error();
					if( err == HDERR_OUT_OF_MEMORY )
					{
						aud->status = AUD_MEM_ERROR;
					}
					else
					{
						aud->status = AUD_DISK_NOT_FOUND;
					}
				}
				else
				{
					header = *hard_disk_get_header(source);

					hash_data_insert_binary_checksum(aud->hash, HASH_MD5, header.md5);
					
					if (hash_data_is_equal(aud->exphash, aud->hash, 0))
					{
						aud->status = AUD_DISK_GOOD;
					}
					else
					{
						aud->status = AUD_DISK_BAD_MD5;
					}

					hard_disk_close( source );
				}

				aud++;
			}
		}

        #ifdef MESS
        if (!count)
                return -1;
        else
        #endif
	return count;
}

static void VerifyDumpHashData(const char* hash, const char* head, verify_printf_proc verify_printf)
{
	char buf[512];

	verify_printf("\t%s", head ? head : "");
	hash_data_print(hash, 0, buf);
	verify_printf("%s\n", buf);
}


/* Generic function for evaluating a romset. Some platforms may wish to
   call AuditRomSet() instead and implement their own reporting (like MacMAME). */
int VerifyRomSet (int game, verify_printf_proc verify_printf)
{
	tAuditRecord			*aud;
	int						count;
	int						archive_status = 0;
	const struct GameDriver *gamedrv = drivers[game];

	if ((count = AuditRomSet (game, &aud)) == 0)
		return NOTFOUND;

	if (count == -1) return CORRECT;

#if 1
    if (gamedrv->clone_of)
    {
        int i;
        int cloneRomsFound = 0;
        int uniqueRomsFound = 0;

        /* count number of roms found that are unique to clone */
        for (i = 0; i < count; i++)
			if (!RomInSet (gamedrv->clone_of, aud[i].exphash))
			{
				uniqueRomsFound++;
				if (aud[i].status != AUD_ROM_NOT_FOUND)
					cloneRomsFound++;
			}
        #ifndef MESS
        /* Different MESS systems can use the same ROMs */
        if (uniqueRomsFound && !cloneRomsFound)
            return CLONE_NOTFOUND;
        #endif
    }
#else
	if (gamedrv->clone_of)
	{
		int i;
		int cloneRomsFound = 0;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!RomInSet (gamedrv->clone_of, aud[i].exphash))
					cloneRomsFound++;

                #ifndef MESS
                /* Different MESS systems can use the same ROMs */
		if (cloneRomsFound == 0)
			return CLONE_NOTFOUND;
                #endif
	}
#endif

	while (count--)
	{
		archive_status |= aud->status;

		switch (aud->status)
		{
			case AUD_ROM_NOT_FOUND:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND\n",
					drivers[game]->name, aud->rom, aud->explength);
				VerifyDumpHashData(aud->exphash, NULL, verify_printf);
				break;
			case AUD_NOT_AVAILABLE:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND - NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_ROM_NEED_DUMP:
				verify_printf ("%-8s: %-12s %7d bytes FOUND BUT NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_BAD_CHECKSUM:
				verify_printf ("%-8s: %-12s %7d bytes INCORRECT CHECKSUM:\n",
					drivers[game]->name, aud->rom, aud->explength);
				VerifyDumpHashData(aud->exphash, "EXPECTED: ", verify_printf);
				VerifyDumpHashData(aud->hash,    "   FOUND: ", verify_printf);
				break;
			case AUD_ROM_NEED_REDUMP:
				verify_printf ("%-8s: %-12s %7d bytes ROM NEEDS REDUMP\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_MEM_ERROR:
				verify_printf ("Out of memory reading ROM %s\n", aud->rom);
				break;
			case AUD_LENGTH_MISMATCH:
				verify_printf ("%-8s: %-12s %7d bytes INCORRECT LENGTH: %8d\n",
					drivers[game]->name, aud->rom, aud->explength, aud->length);
				VerifyDumpHashData(aud->exphash, NULL, verify_printf);
				break;
			case AUD_ROM_GOOD:
#if 0    /* if you want a full accounting of roms */
				verify_printf ("%-8s: %-12s %7d bytes ROM GOOD\n",
					drivers[game]->name, aud->rom, aud->explength);
				VerifyDumpHashData(aud->hash, NULL, verify_printf);
#endif
				break;
			case AUD_DISK_GOOD:
#if 0    /* if you want a full accounting of roms */
				verify_printf ("%-8s: %-12s GOOD\n",
					drivers[game]->name, aud->rom);
				VerifyDumpHashData(aud->hash, NULL, verify_printf);				
#endif
				break;
			case AUD_DISK_NOT_FOUND:
				verify_printf ("%-8s: %-12s NOT FOUND\n",
					drivers[game]->name, aud->rom);
				VerifyDumpHashData(aud->exphash, NULL, verify_printf);
				break;
			case AUD_DISK_BAD_MD5:
				verify_printf ("%-8s: %-12s INCORRECT CHECKSUM:\n",
					drivers[game]->name, aud->rom);
				VerifyDumpHashData(aud->exphash, "EXPECTED: ", verify_printf);
				VerifyDumpHashData(aud->hash,    "   FOUND: ", verify_printf);
				break;
		}
		aud++;
	}

	if (archive_status & (AUD_ROM_NOT_FOUND|AUD_BAD_CHECKSUM|AUD_MEM_ERROR|AUD_LENGTH_MISMATCH|AUD_DISK_NOT_FOUND|AUD_DISK_BAD_MD5))
		return INCORRECT;
	if (archive_status & (AUD_ROM_NEED_DUMP|AUD_ROM_NEED_REDUMP|AUD_NOT_AVAILABLE))
		return BEST_AVAILABLE;

	return CORRECT;

}


static tMissingSample *gMissingSamples = NULL;

/* Builds a list of every missing sample. Returns total number of missing
   samples, or -1 if no samples were found. Sets audit to point to the
   list of missing samples. */
int AuditSampleSet (int game, tMissingSample **audit)
{
	struct InternalMachineDriver drv;
	int skipfirst;
	mame_file *f;
	const char **samplenames, *sharedname;
	int exist;
	static const struct GameDriver *gamedrv;
	int j;
	int count = 0;
	tMissingSample *aud;

	gamedrv = drivers[game];
	expand_machine_driver(gamedrv->drv, &drv);

	samplenames = NULL;
#if (HAS_SAMPLES || HAS_VLM5030)
	for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
	{
#if (HAS_SAMPLES)
		if( drv.sound[j].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[j].sound_interface)->samplenames;
#endif
	}
#endif
    /* does the game use samples at all? */
	if (samplenames == 0 || samplenames[0] == 0)
		return 0;

	/* take care of shared samples */
	if (samplenames[0][0] == '*')
	{
		sharedname=samplenames[0]+1;
		skipfirst = 1;
	}
	else
	{
		sharedname = NULL;
		skipfirst = 0;
	}

	/* do we have samples for this game? */
	exist = mame_faccess (gamedrv->name, FILETYPE_SAMPLE);

	/* try shared samples */
	if (!exist && skipfirst)
		exist = mame_faccess (sharedname, FILETYPE_SAMPLE);

	/* if still not found, we're done */
	if (!exist)
		return -1;

	/* allocate missing samples list (if necessary) */
	if (!gMissingSamples)
		gMissingSamples = (tMissingSample *)malloc (AUD_MAX_SAMPLES * sizeof (tMissingSample));

	if (gMissingSamples)
		*audit = aud = gMissingSamples;
	else
		return 0;

	for (j = skipfirst; samplenames[j] != 0; j++)
	{
		/* skip empty definitions */
		if (strlen (samplenames[j]) == 0)
			continue;
		f = mame_fopen (gamedrv->name, samplenames[j], FILETYPE_SAMPLE, 0);
		if (f == NULL && skipfirst)
			f = mame_fopen (sharedname, samplenames[j], FILETYPE_SAMPLE, 0);

		if (f)
			mame_fclose(f);
		else
		{
			strcpy (aud->name, samplenames[j]);
			count++;
			aud++;
		}
	}
	return count;
}


/* Generic function for evaluating a sampleset. Some platforms may wish to
   call AuditSampleSet() instead and implement their own reporting (like MacMAME). */
int VerifySampleSet (int game, verify_printf_proc verify_printf)
{
	tMissingSample	*aud;
	int				count;

	count = AuditSampleSet (game, &aud);
	if (count==-1)
		return NOTFOUND;
	else if (count==0)
		return CORRECT;

	/* list missing samples */
	while (count--)
	{
		verify_printf ("%-8s: %s NOT FOUND\n", drivers[game]->name, aud->name);
		aud++;
	}

	return INCORRECT;
}
