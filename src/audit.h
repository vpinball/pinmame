
#ifndef AUDIT_H
#define AUDIT_H

/* return values from VerifyRomSet and VerifySampleSet */
#define CORRECT   		0
#define NOTFOUND  		1
#define INCORRECT 		2
#define CLONE_NOTFOUND	3
#define BEST_AVAILABLE	4

/* rom status values for tAuditRecord.status */
#define AUD_ROM_GOOD		0x00000001
#define AUD_ROM_NEED_REDUMP	0x00000002
#define AUD_ROM_NOT_FOUND	0x00000004
#define AUD_NOT_AVAILABLE	0x00000008
#define AUD_BAD_CHECKSUM	0x00000010
#define AUD_MEM_ERROR		0x00000020
#define AUD_LENGTH_MISMATCH	0x00000040
#define AUD_ROM_NEED_DUMP	0x00000080
#define AUD_DISK_GOOD		0x00000100
#define AUD_DISK_NOT_FOUND	0x00000200
#define AUD_DISK_BAD_MD5	0x00000400

#define AUD_MAX_ROMS		100	/* maximum roms per driver */
#define AUD_MAX_SAMPLES		200	/* maximum samples per driver */


typedef struct
{
	char rom[20];				/* name of rom file */
	unsigned int explength;		/* expected length of rom file */
	unsigned int length;		/* actual length of rom file */
	const char* exphash;        /* expected hash data */
	char hash[256];             /* computed hash informations */
	int status;					/* status of rom file */
} tAuditRecord;

typedef struct
{
	char	name[20];		/* name of missing sample file */
} tMissingSample;

typedef void (CLIB_DECL *verify_printf_proc)(const char *fmt,...);

int AuditRomSet (int game, tAuditRecord **audit);
int VerifyRomSet(int game,verify_printf_proc verify_printf);
int AuditSampleSet (int game, tMissingSample **audit);
int VerifySampleSet(int game,verify_printf_proc verify_printf);
int RomInSet (const struct GameDriver *gamedrv, const char* hash);
int RomsetMissing (int game);


#endif
