#include "xmame.h"
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zlib.h>

#ifdef BSD43 /* old style directory handling */
#include <sys/types.h>
#include <sys/dir.h>
#define dirent direct
#endif

/* #define FILEIO_DEBUG */
#define MAXPATHC 20 /* at most 20 path entries */
#define MAXPATHL BUF_SIZE /* at most BUF_SIZE-1 character path length */
 
#ifdef MESS
int osd_num_devices(void)
{
   return 0; /* unix doesn't have devices ( a device = a: b: etc) */
}

void osd_change_device(const char *device)
{
}

const char *osd_get_device_name(int idx)
{
   return "";
}


struct osd_dir {
   DIR *dir;
   char dirname[MAXPATHL];
   char filemask[MAXPATHL];
};

void *osd_dir_open(const char *dirname, const char *filemask)
{
  struct osd_dir *dir = NULL;
  
  if(!(dir = calloc(1, sizeof(struct osd_dir))))
     return NULL;
  
  if(!(dir->dir = opendir(dirname)))
  {
     osd_dir_close(dir);
     return NULL;
  }
  
  strncpy(dir->dirname,  dirname,  MAXPATHL-1);
  strncpy(dir->filemask, filemask, MAXPATHL-1);
  
  return dir;
}

void osd_dir_close(void *dir)
{
  struct osd_dir *my_dir = dir;
  
  if(my_dir->dir)
    closedir(my_dir->dir);
  
  free(my_dir);
}

#ifndef __QNXNTO__
static int fnmatch(const char *f1, const char *f2)
{
	while (*f1 && *f2)
	{
		if (*f1 == '*')
		{
			/* asterisk is not the last character? */
			if (f1[1])
			{
				/* skip until first occurance of the character after the asterisk */
                while (*f2 && toupper(f1[1]) != toupper(*f2))
					f2++;
				/* skip repetitions of the character after the asterisk */
				while (*f2 && toupper(f1[1]) == toupper(f2[1]))
					f2++;
			}
			else
			{
				/* skip until end of string */
                while (*f2)
					f2++;
			}
        }
		else
		if (*f1 == '?')
		{
			/* skip one character */
            f2++;
		}
		else
		{
			/* mismatch? */
            if (toupper(*f1) != toupper(*f2))
				return 0;
            /* skip one character */
			f2++;
		}
		/* skip mask */
        f1++;
	}
	/* no match if anything is left */
	if (*f1 || *f2)
		return 0;
    return 1;
}
#endif

int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir)
{
   struct osd_dir *my_dir = dir;
   struct dirent *d = NULL;
   struct stat stat_buf;
   char buf[MAXPATHL];
    
   *is_dir = 0;
  
   while((d = readdir(my_dir->dir)))
   {
      snprintf(buf, MAXPATHL, "%s/%s", my_dir->dirname, d->d_name);
      
      /* stat it */
      if(stat(buf, &stat_buf))
         continue;
         
      /* check that it is a dir or matches our filemask */
#ifdef BSD43
      if(S_IFDIR & stat_buf.st_mode)
#else
      if(S_ISDIR(stat_buf.st_mode))
#endif
      {
         *is_dir = 1;
      }
#ifndef __QNXNTO__
      else if (!fnmatch(my_dir->filemask, d->d_name))
#else
      else if (!fnmatch(my_dir->filemask, d->d_name,0))   
#endif
         continue;
      
      strncpy(name, d->d_name, namelength-1);
      name[namelength-1] = 0;
      return strlen(name);
  }
  
  return 0;
}

void osd_change_directory(const char *directory)
{
   chdir(directory);
}

const char *osd_get_cwd(void)
{
   static char cwd[MAXPATHL + 1];
   
#ifdef BSD43
   getwd(cwd);
#else
   getcwd(cwd, MAXPATHL);
#endif
   strcat(cwd, "/");
   return cwd;
}

/*============================================================ */
/*	osd_dirname */
/*============================================================ */

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	/* NULL begets NULL */
	if (!filename)
		return NULL;

	/* allocate space for it */
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
	{
		fprintf(stderr_file, "error: malloc failed in osd_dirname\n");
		return NULL;
	}

	/* copy in the name */
	strcpy(dirname, filename);

	/* search backward for a slash */
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/')
		{
			/* found it: NULL terminate and return */
			*(c + 1) = 0;
			return dirname;
		}

	/* otherwise, return an empty string */
	dirname[0] = 0;
	return dirname;
}

/*============================================================ */
/*	osd_basename */
/*============================================================ */

char *osd_basename(char *filename)
{
	char *c;

	/* NULL begets NULL */
	if (!filename)
		return NULL;

	/* start at the end and return when we hit a slash */
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/')
			return c + 1;

	/* otherwise, return the whole thing */
	return filename;
}

/*============================================================ */
/*	osd_path_separator */
/*============================================================ */

const char *osd_path_separator(void)
{
	return "/";
}

/*============================================================ */
/*	osd_is_path_separator */
/*============================================================ */

int osd_is_path_separator(char ch)
{
	return (ch == '\\') || (ch == '/');
}

/*============================================================ */
/*	osd_is_absolute_path */
/*============================================================ */
int osd_is_absolute_path(const char *path)
{
	int result;

	if ((path[0] == '/') || (path[0] == '/'))
		result = 1;
	else
		result = 0;
	return result;
}

#endif
