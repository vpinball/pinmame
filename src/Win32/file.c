/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/

/***************************************************************************

  file.c

    File handling code.

***************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <sys/stat.h>
#include <assert.h>
#include <zlib.h>
#include "mame.h"
#include "mzip.h"
#include "unzip.h"
#include "osdepend.h"
#include "FilePrivate.h"
#include "options.h"
#include "ScreenShot.h"
#include "M32Util.h"

#define MAX_FILES 500

/***************************************************************************
 Internal structures
***************************************************************************/

#define MAXPATHS 20 /* at most 20 path entries */

typedef struct
{
    char* m_Paths[MAXPATHS];
    int   m_NumPaths;
    char* m_Buf;
} tDirPaths;

/***************************************************************************
 function prototypes
***************************************************************************/

static int          File_init(void);
static void         File_exit(void);
static int          File_faccess(const char *filename, int filetype);
static void*        File_fopen(const char *gamename,const char *filename,int filetype,int write);
static int          File_fread(void *file,void *buffer,int length);
static int          File_fwrite(void *file,const void *buffer,int length);
static int          File_fseek(void *file,int offset,int whence);
static void         File_fclose(void *file);
static int          File_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum);
static int          File_fsize(void *file);
static unsigned int File_fcrc(void *file);
static int          File_fread_scatter(void *file,void *buffer,int length,int increment);
static int          File_fgetc(void* file);
static int          File_ungetc(int c, void* file);
static char*        File_fgets(char* s, int n, void* file);
static int          File_feof(void* file);
static int          File_ftell(void* file);

static void SetPaths(tDirPaths* pDirPath, const char *path);
static int checksum_file(const char* file, unsigned char **p, unsigned int *size, unsigned int *crc);

/***************************************************************************
 External function prototypes
***************************************************************************/


/***************************************************************************
 External variables
***************************************************************************/

struct OSDFile File =
{
    File_init,
    File_exit,
    File_faccess,
    File_fopen,
    File_fread,
    File_fwrite,
    File_fseek,
    File_fclose,
    File_fchecksum,
    File_fsize,
    File_fcrc,
    File_fread_scatter,
    File_fgetc,
    File_ungetc,
    File_fgets,
    File_feof,
    File_ftell
};

/***************************************************************************
 Internal variables
***************************************************************************/

static mame_file   *file_list[MAX_FILES];

static BOOL wrote_hi     = FALSE;
static BOOL wrote_cfg    = FALSE;
static BOOL wrote_nvram  = FALSE;
static char hifname[FILENAME_MAX]    = "";
static char cfgfname[FILENAME_MAX]   = "";
static char nvramfname[FILENAME_MAX] = "";

static tDirPaths RomDirPath;
static tDirPaths SampleDirPath;

/***************************************************************************
 External OSD functions
***************************************************************************/

static int File_init(void)
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
        file_list[i] = 0;

    memset(&RomDirPath,    0, sizeof(tDirPaths));
    memset(&SampleDirPath, 0, sizeof(tDirPaths));
    SetPaths(&RomDirPath,    GetRomDirs());
    SetPaths(&SampleDirPath, GetSampleDirs());

    return 0;
}

static void File_exit(void)
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
        if (file_list[i])
            assert(FALSE); /* file left open */

    if (RomDirPath.m_Buf != NULL)
    {
        free(RomDirPath.m_Buf);
        RomDirPath.m_Buf = NULL;
    }
    if (SampleDirPath.m_Buf != NULL)
    {
        free(SampleDirPath.m_Buf);
        SampleDirPath.m_Buf = NULL;
    }
}

/*
   check if roms/samples for a game exist at all
   return index+1 of the path vector component on success, otherwise 0
*/
int File_faccess(const char *newfilename, int filetype)
{
    static const char* filename;
    static int         index;

    char        name[FILENAME_MAX];
    struct stat file_stat;
    const char* dirname;
    tDirPaths*  pDirPaths;

    /* if newfilename == NULL, continue the search */
    if (newfilename == NULL)
    {
        index++;
    }
    else
    {
        index = 0;
        filename = newfilename;
    }

    switch (filetype)
    {
    case OSD_FILETYPE_ROM:
    case OSD_FILETYPE_SAMPLE:

        if (filetype == OSD_FILETYPE_ROM)
            pDirPaths = &RomDirPath;
        if (filetype == OSD_FILETYPE_SAMPLE)
            pDirPaths = &SampleDirPath;

        for (; index < pDirPaths->m_NumPaths; index++)
        {
            dirname = pDirPaths->m_Paths[index];

            /*
                1) dirname/filename.zip - zip file / ZipMagic
                2) dirname/filename
                3) dirname/filename.zif - ZipFolders
            */

            sprintf(name, "%s/%s.zip", dirname, filename);
            if (stat(name, &file_stat) == 0)
                return index + 1;

            sprintf(name, "%s/%s", dirname, filename);
            if (stat(name, &file_stat) == 0)
                return index + 1;
#ifdef USE_ZIPFOLDERS
            sprintf(name, "%s/%s.zif", dirname, filename);
            if (stat(name, &file_stat) == 0)
                return index + 1;
#endif
        }
        break;

    case OSD_FILETYPE_HIGHSCORE:
    case OSD_FILETYPE_NVRAM:
    case OSD_FILETYPE_CONFIG:
    case OSD_FILETYPE_INPUTLOG:
        break;

    case OSD_FILETYPE_SCREENSHOT:
        dirname = GetImgDir();
        {
            int i;
            for (i = 0; i < FORMAT_MAX; i++)
            {
                sprintf(name, "%s/%s.%s", dirname, filename, pic_format[i]);
                if (stat(name, &file_stat) == 0)
                    return 1;
            }
        }
        break;
#ifdef PINMAME_EXT
    case OSD_FILETYPE_WAVEFILE: {
        dirname = GetWaveDir();
	sprintf(name, "%s/%s.wav", dirname, filename);
        if (stat(name, &file_stat) == 0) return 1;
        break;
    }
#endif /* PINMAME_EXT */
    }
    /* no match */
    return 0;
}

/* gamename holds the driver name, filename is only used for ROMs and samples. */
/* if 'write' is not 0, the file is opened for write. Otherwise it is opened */
/* for read. */
static void *File_fopen(const char *gamename,const char *filename,int filetype,int write)
{
    char        name[FILENAME_MAX];
    char        subdir[25];
    char        typestr[10];
    int         i;
    int         found = 0;
    int         use_flyers = FALSE;
    struct stat stat_buffer;
    mame_file   *mf = NULL;
    const char* dirname;
    tDirPaths*  pDirPaths;
    int         imgType = PICT_SCREENSHOT;

    /* Writing to samples and roms are not allowed */
    if (write &&
        (OSD_FILETYPE_ROM == filetype || OSD_FILETYPE_SAMPLE == filetype))
        return NULL;

    for (i = 0; i < MAX_FILES; i++)
    {
        if (file_list[i] == NULL)
        {
            file_list[i] = (mame_file *)malloc(sizeof(mame_file));
            mf = file_list[i];
            if (mf != NULL)
            {
                memset(mf, '\0', sizeof(mame_file));
                mf->index = i;
            }
            break;
        }
    }
    if (mf == NULL)
        return NULL;

    mf->access_type = ACCESS_FILE;

    switch (filetype)
    {
    case OSD_FILETYPE_ROM:
    case OSD_FILETYPE_SAMPLE:

        /* only for reading */
        if (write)
        {
            logerror("osd_fopen: type %02x write not supported\n", filetype);
            break;
        }

        if (filetype == OSD_FILETYPE_ROM)
            pDirPaths = &RomDirPath;
        if (filetype == OSD_FILETYPE_SAMPLE)
            pDirPaths = &SampleDirPath;

        for (i = 0; i < pDirPaths->m_NumPaths && !found; i++)
        {
            dirname = pDirPaths->m_Paths[i];

            /* zip file. */
            if (!found)
            {
                sprintf(name, "%s/%s.zip", dirname, gamename);
                if (stat(name, &stat_buffer) == 0)
                {
                    if (load_zipped_file(name, filename, &mf->file_data, &mf->file_length) == 0)
                    {
                        mf->access_type = ACCESS_ZIP;
                        mf->crc = crc32(0L, mf->file_data, mf->file_length);
                        found = 1;
                    }
                }
            }

            /* rom directory */
            if (!found)
            {
                sprintf(name, "%s/%s", dirname, gamename);
                if (stat(name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR))
                {
                    sprintf(name, "%s/%s/%s", dirname, gamename, filename);
                    if (filetype == OSD_FILETYPE_ROM)
                    {
                        if (checksum_file(name, &mf->file_data, &mf->file_length, &mf->crc) == 0)
                        {
                            mf->access_type = ACCESS_RAMFILE;
                            found = 1;
                        }
                    }
                    else
                    {
                        mf->access_type = ACCESS_FILE;
                        mf->fptr = fopen(name, "rb");
                        found = mf->fptr != 0;
                    }
                }

            }

            /* try with a .zip directory (if ZipMagic is installed) */
            if (!found)
            {
                sprintf(name, "%s/%s.zip", dirname, gamename);
                if (stat(name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR))
                {
                    sprintf(name, "%s/%s.zip/%s",dirname,gamename,filename);
                    if (filetype == OSD_FILETYPE_ROM)
                    {
                        if (checksum_file(name, &mf->file_data, &mf->file_length, &mf->crc) == 0)
                        {
                            mf->access_type = ACCESS_RAMFILE;
                            found = 1;
                        }
                    }
                    else
                    {
                        mf->access_type = ACCESS_FILE;
                        mf->fptr = fopen(name, "rb");
                        found = mf->fptr != 0;
                    }
                }
            }

            if (mf->fptr != NULL)
                break;
        }
        break;

    case OSD_FILETYPE_FLYER:
        use_flyers = TRUE;
        imgType = GetShowPictType();

    case OSD_FILETYPE_SCREENSHOT:

        if (write && imgType != PICT_SCREENSHOT)
            return 0;

        switch (imgType)
        {
        case PICT_SCREENSHOT:   dirname = GetImgDir();      break;
        case PICT_FLYER:        dirname = GetFlyerDir();    break;
        case PICT_CABINET:      dirname = GetCabinetDir();  break;
        case PICT_MARQUEE:      dirname = GetMarqueeDir();  break;
        default:
            return 0;
        }

        {
            struct stat s;

            if (stat(dirname, &s) != 0)
                mkdir(dirname);
        }

        if (write && filetype == OSD_FILETYPE_SCREENSHOT)
        {
            /* Write mode for snapshots only */
            sprintf(name, "%s/%s.png", dirname, gamename);
            if ((mf->fptr = fopen(name, "wb")) != NULL)
                found = TRUE;
            break;
        }
        else
        if (! write)
        {
            int i;

            for (i = 0; i < FORMAT_MAX; i++)
            {
                /* Try Normal file */
                sprintf(name, "%s/%s.%s", dirname, gamename, pic_format[i]);
                if ((mf->fptr = fopen(name, "rb")) != NULL)
                {
                    mf->access_type = ACCESS_FILE;
                    found = 1;
                    break;
                }
            }
            if (found)
                break;

            for (i = 0; i < FORMAT_MAX; i++)
            {
                char imagename[32];

                /* then zip file. */
                mf->access_type = ACCESS_ZIP;
                sprintf(imagename, "%s.%s", gamename, pic_format[i]);

                if (use_flyers)
                {
                    switch (GetShowPictType())
                    {
                    case PICT_FLYER:    sprintf(name, "%s/flyers.zip",   dirname); break;
                    case PICT_CABINET:  sprintf(name, "%s/cabinets.zip", dirname); break;
                    case PICT_MARQUEE:  sprintf(name, "%s/marquees.zip", dirname); break;
                    default:
                        assert(FALSE);
                    }
                }
                else
                    sprintf(name, "%s/snap.zip", dirname);

                if (stat(name, &stat_buffer) == 0)
                {
                    if (load_zipped_file(name, imagename, &mf->file_data, &mf->file_length) == 0)
                    {
                        found = 1;
                        break;
                    }
                }
            }

            if (found)
                break;

            for (i = 0; i < FORMAT_MAX; i++)
            {
                /* Try ZipMagic in subfolder */
                if (use_flyers)
                {
                    switch (GetShowPictType())
                    {
                    case PICT_FLYER:    sprintf(name, "%s/flyers.zip/%s.%s", dirname, gamename, pic_format[i]);   break;
                    case PICT_CABINET:  sprintf(name, "%s/cabinets.zip/%s.%s", dirname, gamename, pic_format[i]); break;
                    case PICT_MARQUEE:  sprintf(name, "%s/marquees.zip/%s.%s", dirname, gamename, pic_format[i]); break;
                    default:
                        assert(FALSE);
                    }
                }
                else
                    sprintf(name, "%s/snap.zip/%s.%s", dirname, gamename, pic_format[i]);

                mf->fptr = fopen(name, "rb");
                mf->access_type = ACCESS_FILE;
                if ((found = mf->fptr != 0) != 0)
                    break;
            }
        }

        break;

    case OSD_FILETYPE_HISTORY:
        /* only for reading */
        if (write)
        {
            logerror("osd_fopen: type %02x write not supported\n", filetype);
            break;
        }
        mf->access_type = ACCESS_FILE;
        mf->fptr = fopen(filename, "rb");
        found = mf->fptr != 0;
        break;

    case OSD_FILETYPE_CHEAT:
        mf->access_type = ACCESS_FILE;
        /* open as ASCII files, not binary like the others */
        mf->fptr = fopen(filename, write ? "a" : "r");
        found = mf->fptr != 0;
        break;

    case OSD_FILETYPE_HIGHSCORE_DB:
    case OSD_FILETYPE_LANGUAGE:
        /* only for reading */
        if (write)
        {
            logerror("osd_fopen: type %02x write not supported\n", filetype);
            break;
        }
        mf->access_type = ACCESS_FILE;
        /* open as ASCII files, not binary like the others */
        mf->fptr = fopen(filename, "r");
        found = mf->fptr != 0;
        break;

    case OSD_FILETYPE_HIGHSCORE:
    case OSD_FILETYPE_NVRAM:
    case OSD_FILETYPE_CONFIG:

        if (filetype == OSD_FILETYPE_CONFIG)
        {
            strcpy(typestr, "cfg");
            dirname = GetCfgDir();
        }
        else
        if (filetype == OSD_FILETYPE_HIGHSCORE)
        {
            if (!mame_highscore_enabled())
                break;

            strcpy(typestr, "hi");
            dirname = GetHiDir();
        }
        else
        if (filetype == OSD_FILETYPE_NVRAM)
        {
            strcpy(typestr, "nv");
            dirname = GetNvramDir();
        }


        if (!found)
        {
            struct stat s;

            if (stat(dirname, &s) != 0)
                mkdir(dirname);

            /* Try Normal file */
            sprintf(name, "%s/%s.%s", dirname, gamename, typestr);
            if ((mf->fptr = fopen(name, write ? "wb" : "rb")) != NULL)
            {
                if (write == 1)
                {
                    if (OSD_FILETYPE_HIGHSCORE == filetype)
                    {
                        strcpy(hifname,name);
                        wrote_hi = TRUE;
                    }
                    else
                    if (OSD_FILETYPE_CONFIG == filetype)
                    {
                        strcpy(cfgfname,name);
                        wrote_cfg = TRUE;
                    }
                    else
                    if (OSD_FILETYPE_NVRAM == filetype)
                    {
                        strcpy(nvramfname, name);
                        wrote_nvram = TRUE;
                    }
                }
                mf->access_type = ACCESS_FILE;
                found = 1;
            }
            else
            if (write == 0)
            {
                /* then zip file. */
                mf->access_type = ACCESS_ZIP;
                sprintf(name, "%s.%s", gamename, typestr);
                sprintf(subdir, "%s.zip", typestr);
                if (stat(subdir, &stat_buffer) == 0)
                {
                    if (load_zipped_file(subdir, name, &mf->file_data, &mf->file_length) == 0)
                        found = 1;
                }
            }
        }

        /* Try ZipMagic */
        if (!found)
        {
            sprintf(name, "%s.zip/%s.%s", dirname, gamename, typestr);
            mf->fptr = fopen(name, write ? "wb" : "rb");
            mf->access_type = ACCESS_FILE;
            found = mf->fptr != 0;
        }

        break;

    case OSD_FILETYPE_INPUTLOG:
        {
            char fname[_MAX_FNAME];
            char ext[_MAX_EXT];

            _splitpath(gamename, NULL, NULL, fname, ext);

            if (!strcmp(ext, ".inp"))
            {
                mf->access_type = ACCESS_FILE;
                mf->fptr = fopen(gamename, write ? "wb" : "rb");
                found = mf->fptr != 0;
            }
            else
            if (!strcmp(ext, ".zip"))
            {
                if (write == 0)
                {
                    sprintf(name, "%s.inp", fname);
                    if (stat(gamename, &stat_buffer) == 0)
                    {
                        if (load_zipped_file(gamename,
                                             name,
                                             &mf->file_data,
                                             &mf->file_length) == 0)
                        {
                            mf->access_type = ACCESS_ZIP;
                            found = 1;
                        }
                    }
                }
            }
        }
        break;

    case OSD_FILETYPE_STATE:
        sprintf(name, "%s/%s.sta", GetStateDir(), gamename);
        mf->access_type = ACCESS_FILE;
        mf->fptr = fopen(name, write ? "wb" : "rb");
        if (mf->fptr == 0)
        {
            /* try with a .zip directory (if ZipMagic is installed) */
            sprintf(name, "%s.zip/%s.sta", GetStateDir(), gamename);
            mf->fptr = fopen(name, write ? "wb" : "rb");
            mf->access_type = ACCESS_FILE;
        }

        found = mf->fptr != 0;
        break;

    case OSD_FILETYPE_ARTWORK:
        /* only for reading */
        if (write)
            break;

        sprintf(name, "%s/%s", GetArtDir(), filename);
        mf->access_type = ACCESS_FILE;
        mf->fptr = fopen(name, "rb");
        if (mf->fptr == 0)
        {
            /* try with a .zip directory (if ZipMagic is installed) */
            sprintf(name, "%s/artwork.zip/%s", GetStateDir(), gamename);
            mf->fptr = fopen(name, "rb");
            mf->access_type = ACCESS_FILE;
        }

        found = mf->fptr != 0;

        if (!found)
        {
            /* try .zip file */
            sprintf(name, "%s/artwork.zip", GetArtDir());
            mf->access_type = ACCESS_ZIP;
            if (stat(name, &stat_buffer) == 0)
            {
                if (load_zipped_file(name, filename, &mf->file_data, &mf->file_length) == 0)
                    found = 1;
            }
        }
        else
            found = mf->fptr != 0;
        break;

    case OSD_FILETYPE_MEMCARD:
        sprintf(name, "%s/%s", GetMemcardDir(), filename);
        mf->access_type = ACCESS_FILE;
        mf->fptr =  fopen(name,write ? "wb" : "rb");
        found = mf->fptr != 0;
        break;
#ifdef PINMAME_EXT
    case OSD_FILETYPE_WAVEFILE: {
        sprintf(name, "%s/%s.wav", GetWaveDir(), filename);
        mf->access_type = ACCESS_FILE;
        mf->fptr =  fopen(name,write ? "wb" : "rb");
        found = mf->fptr != 0;
        break;
    }
#endif /* PINMAME_EXT */
    }


    if (!found)
    {
        file_list[mf->index] = NULL;
        free(mf);
        mf = NULL;
    }

    return mf;
}


static int File_fread(void *file, void *buffer, int length)
{
    mame_file *mf = file;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        return fread(buffer, 1, length, mf->fptr);

    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        if (mf->file_data)
        {
            if (length + mf->file_offset > mf->file_length)
                length = mf->file_length - mf->file_offset;
            memcpy(buffer, mf->file_offset + mf->file_data, length);
            mf->file_offset += length;
            return length;
        }
        break;
    }

    return 0;
}


static int File_fwrite(void *file, const void *buffer, int length)
{
    mame_file *mf = file;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        return fwrite(buffer, 1, length, mf->fptr);

    case ACCESS_ZIP:
        assert(FALSE); /* writing to zip files not supported */
        return 0;
    }

    assert(FALSE); /* mf->access_type invalid file type */
    return 0;
}


static int File_fread_scatter(void *file,void *buffer,int length,int increment)
{
    unsigned char *buf = buffer;
    mame_file *mf = file;
    unsigned char tempbuf[4096];
    int totread,r,i;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        totread = 0;
        while (length)
        {
            r = length;

            if (r > 4096)
                r = 4096;

            r = osd_fread(mf, tempbuf, r);

            if (r == 0)
                return totread; /* error */

            for (i = 0;i < r;i++)
            {
                *buf = tempbuf[i];
                buf += increment;
            }

            totread += r;
            length -= r;
        }
        return totread;
        break;

    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        /* reading from the RAM image of a file */
        if (mf->file_data)
        {
            if (length + mf->file_offset > mf->file_length)
                length = mf->file_length - mf->file_offset;

            for (i = 0;i < length;i++)
            {
                *buf = mf->file_data[mf->file_offset + i];
                buf += increment;
            }

            mf->file_offset += length;

            return length;
        }
        break;
    }
    return 0;
}

static int File_fseek(void *file, int offset, int whence)
{
    mame_file *mf = file;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        return fseek(mf->fptr, offset, whence);

    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        /* seeking within the RAM image of a file */
        switch (whence)
        {
        case SEEK_SET:
            mf->file_offset = offset;
            break;

        case SEEK_CUR:
            mf->file_offset += offset;
            break;

        case SEEK_END:
            mf->file_offset = mf->file_length + offset;
            break;

        default:
            return 0;
        }

        if (mf->file_offset < 0)
            mf->file_offset = 0;
        if (mf->file_offset > mf->file_length)
            mf->file_offset = mf->file_length;
        return 0;
    }

    assert(FALSE); /* mf->access_type invalid file type */
    return 0;
}


static void File_fclose(void *file)
{
    mame_file *mf = file;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        fclose(mf->fptr);
        if (wrote_hi == TRUE)   /* If hi.zip exists... */
        {
            /* Move the new highscore file into it */
            if (ZipFile("hi.zip", hifname) != 0)
                ErrorMsg("Error writing %s to hi.zip file", hifname);

            wrote_hi = FALSE;
            hifname[0] = '\0';
        }
        if (wrote_cfg == TRUE)  /* If cfg.zip exists... */
        {
            /* Move the new config file into it */
            if (ZipFile("cfg.zip", cfgfname) != 0)
                ErrorMsg("Error writing %s to cfg.zip file", cfgfname);

            wrote_cfg = FALSE;
            cfgfname[0] = '\0';
        }
        if (wrote_nvram == TRUE)  /* If nv.zip exists... */
        {
            /* Move the new nvram file into it */
            if (ZipFile("nv.zip", nvramfname) != 0)
                ErrorMsg("Error writing %s to nv.zip file", nvramfname);

            wrote_nvram = FALSE;
            nvramfname[0] = '\0';
        }
        break;

    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        /* freeing the file's memory allocated by the unzip code  */
        if (mf->file_data)
            free(mf->file_data);
        break;

    default:
        /* mf->access_type invalid file type */
        assert(FALSE);
    }

    file_list[mf->index] = NULL;
    free(mf);
}

int File_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum)
{
    char name[FILENAME_MAX];
    int  i;
    struct stat stat_buffer;
    int  found = 0;
    char *dirname;
    tDirPaths*  pDirPaths;

    pDirPaths = &RomDirPath;

    for (i = 0; i < pDirPaths->m_NumPaths && !found; i++)
    {
        dirname = pDirPaths->m_Paths[i];

        /* try with a .zip extension */
        if (!found)
        {
            sprintf(name, "%s/%s.zip", dirname, gamename);
            if (stat(name, &stat_buffer)==0)
            {
                if (checksum_zipped_file(name, filename, length, sum) == 0)
                {
                    logerror("Using (osd_fchecksum) zip file for %s\n", filename);
                    found = 1;
                }
            }
        }

        /* rom directory */
        if (!found)
        {
            sprintf(name,"%s/%s", dirname, gamename);
            if (stat(name, &stat_buffer) == 0
            && (stat_buffer.st_mode & S_IFDIR))
            {
                sprintf(name, "%s/%s/%s",dirname, gamename, filename);
                if (checksum_file(name, 0, length, sum) == 0)
                {
                    found = 1;
                }
            }
        }
    }

    if (!found)
        return -1;

    return 0;
}

static int File_fsize(void *file)
{
    long lPos, lRet;
    mame_file *mf = file;

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        lPos = ftell(mf->fptr);
        fseek(mf->fptr, 0, SEEK_END);
        lRet = ftell(mf->fptr);
        fseek(mf->fptr, lPos, SEEK_SET);
        return lRet;
    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        return mf->file_length;
    }

    return 0;
}

static unsigned int File_fcrc(void *file)
{
    mame_file *mf = file;

    return mf->crc;
}

static int File_fgetc(void* file)
{
    mame_file *mf = file;

    assert(mf != NULL);

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        assert(mf->fptr != NULL);
        return fgetc(mf->fptr);
    }

    return EOF;
}

static int File_ungetc(int c, void* file)
{
    mame_file *mf = file;

    assert(mf != NULL);

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        assert(mf->fptr != NULL);
        return ungetc(c, mf->fptr);
    }

    return EOF;
}

static char* File_fgets(char* s, int n, void* file)
{
    mame_file *mf = file;

    assert(mf != NULL);

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        assert(mf->fptr != NULL);
        return fgets(s, n, mf->fptr);
    }

    return NULL;
}

static int File_feof(void* file)
{
    mame_file *mf = file;

    assert(mf != NULL);

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        assert(mf->fptr != NULL);
        return feof(mf->fptr);
    }

    return 1;
}

static int File_ftell(void *file)
{
    mame_file *mf = file;

    assert(mf != NULL);

    switch (mf->access_type)
    {
    case ACCESS_FILE:
        return ftell(mf->fptr);

    case ACCESS_ZIP:
    case ACCESS_RAMFILE:
        return mf->file_offset;
    }

    assert(FALSE); /* mf->access_type invalid file type */
    return -1;
}


/***************************************************************************
 External functions
***************************************************************************/

void File_UpdateRomPath(const char *path)
{
    if (path == NULL)
        return;

    if (RomDirPath.m_Buf != NULL)
    {
        free(RomDirPath.m_Buf);
        RomDirPath.m_Buf = NULL;
    }
    SetPaths(&RomDirPath, path);
}

void File_UpdateSamplePath(const char *path)
{
    if (path == NULL)
        return;

    if (SampleDirPath.m_Buf != NULL)
    {
        free(SampleDirPath.m_Buf);
        SampleDirPath.m_Buf = NULL;
    }
    SetPaths(&SampleDirPath, path);
}

/* Only checks for a .zip file */
BOOL File_ExistZip(const char *gamename, int filetype)
{
    char        name[FILENAME_MAX];
    struct stat file_stat;
    tDirPaths*  pDirPaths;
    int         index;
    const char* dirname;

    switch (filetype)
    {
    case OSD_FILETYPE_ROM:
    case OSD_FILETYPE_SAMPLE:

        if (filetype == OSD_FILETYPE_ROM)
            pDirPaths = &RomDirPath;
        if (filetype == OSD_FILETYPE_SAMPLE)
            pDirPaths = &SampleDirPath;

        for (index = 0; index < pDirPaths->m_NumPaths; index++)
        {
            dirname = pDirPaths->m_Paths[index];

            sprintf(name, "%s/%s.zip", dirname, gamename);
            if (stat(name, &file_stat) == 0 && !(file_stat.st_mode & S_IFDIR))
                return TRUE;
        }
        break;

    case OSD_FILETYPE_HIGHSCORE:
    case OSD_FILETYPE_CONFIG:
    case OSD_FILETYPE_INPUTLOG:
        return FALSE;
    }

    return FALSE;
}

/* gamename holds the driver name, filename is only used for ROMs and samples. */
BOOL File_Status(const char *gamename,const char *filename,int filetype)
{
    struct stat sbuf;
    char        name[FILENAME_MAX];
    int         i;
    unsigned int len = 0, crc = 0;
    const char* dirname;
    BOOL        found = FALSE;
    tDirPaths*  pDirPaths;

    if (filetype == OSD_FILETYPE_ROM)
        pDirPaths = &RomDirPath;
    else
    if (filetype == OSD_FILETYPE_SAMPLE)
        pDirPaths = &SampleDirPath;
    else
        return FALSE; /* Only used for ROMS and Samples */

    found = FALSE;

    for (i = 0; i < pDirPaths->m_NumPaths; i++)
    {
        dirname = pDirPaths->m_Paths[i];

        /*
            1) dirname/gamename.zip
            2) dirname/gamename
            3) dirname/gamename.zif - ZipFolders
            4) dirname/gamename.zip - ZipMagic
         */

        /* zip file. */
        sprintf(name, "%s/%s.zip", dirname, gamename);
        found = (checksum_zipped_file(name, filename, &len, &crc) == 0);
        if (found)
            break;

        /* rom directory */
        sprintf(name, "%s/%s/%s", dirname, gamename, filename);
        found = (stat(name, &sbuf) == 0 && !(sbuf.st_mode & S_IFDIR));
        if (found)
            break;

        /* ZipMagic */
        sprintf(name, "%s/%s.zip/%s", dirname, gamename, filename);
        found = (stat(name, &sbuf) == 0 && !(sbuf.st_mode & S_IFDIR));
        if (found)
            break;
    }
    return found;
}

/***************************************************************************
 Internal functions
***************************************************************************/

static void SetPaths(tDirPaths* pDirPath, const char *path)
{
    char* token;

    if (pDirPath->m_Buf != NULL)
        free(pDirPath->m_Buf);

    pDirPath->m_Buf = strdup(path);

    pDirPath->m_NumPaths = 0;
    token = strtok(pDirPath->m_Buf, ";");
    while ((pDirPath->m_NumPaths < MAXPATHS) && token)
    {
        pDirPath->m_Paths[pDirPath->m_NumPaths] = token;
        pDirPath->m_NumPaths++;
        token = strtok(NULL, ";");
    }
}

static int checksum_file(const char* file, unsigned char **p, unsigned int *size, unsigned int *crc)
{
    int length;
    unsigned char *data;
    FILE* f;

    f = fopen(file,"rb");
    if (!f)
    {
        return -1;
    }

    /* determine length of file */
    if (fseek (f, 0L, SEEK_END) != 0)
    {
        fclose(f);
        return -1;
    }

    length = ftell(f);
    if (length == -1L)
    {
        fclose(f);
        return -1;
    }

    /* allocate space for entire file */
    data = (unsigned char*)malloc(length);
    if (!data)
    {
        fclose(f);
        return -1;
    }

    /* read entire file into memory */
    if (fseek(f, 0L, SEEK_SET) != 0)
    {
        free(data);
        fclose(f);
        return -1;
    }

    if (fread(data, sizeof (unsigned char), length, f) != (size_t)length)
    {
        free(data);
        fclose(f);
        return -1;
    }

    *size = length;
    *crc = crc32(0L, data, length);
    if (p)
        *p = data;
    else
        free(data);

    fclose(f);

    return 0;
}
