/* A simple rcfile and commandline parsing mechanism

   Copyright 1999,2000 Hans de Goede

   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
/* Changelog
Version 0.1, December 1999
-Initial release (Hans de Goede)
Version 0.2, January 2000
-Fixed priority parsing for booleans (Hans de Goede)
-Fixed error messages for: "error optionx requires an argument". (Hans de
 Goede)
-Fixed --boolean option parsing. (Hans de Goede)
Version 0.3, Februari 2000
-Reworked and cleaned up the interface, broke backward compatibility (Hans
 de Goede)
*/
#include <stdlib.h>
#include <string.h>
//#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rc.h"
#include "misc.h"

#include "rc.h"
#include "osdepend.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif
#define BUF_SIZE 512

struct rc_struct
{
   struct rc_option *option;
   int option_size;
   char **arg;
   int arg_size;
   int args_registered;
};

/* private variables */
static int rc_requires_arg[] = {0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0 };

/* private methods */
static int rc_verify(struct rc_option *option, float value)
{
   if(option->min == option->max)
      return 0;

   if( (value < option->min) || (value > option->max) )
      return -1;

   return 0;
}

static int rc_set_defaults(struct rc_option *option)
{
   int i;

   /* set the defaults */
   for(i=0; option[i].type; i++)
   {
      if (option[i].type == rc_link)
      {
         if(rc_set_defaults(option[i].dest))
            return -1;
      }
      else if (option[i].deflt && rc_set_option3(option+i, option[i].deflt,
         option[i].priority))
         return -1;
   }

   return 0;
}

static void rc_free_stuff(struct rc_option *option)
{
   int i;

   for(i=0; option[i].type; i++)
   {
      switch (option[i].type)
      {
         case rc_link:
            rc_free_stuff(option[i].dest);
            break;
         case rc_string:
            if(*(char **)option[i].dest)
			{
               free(*(char **)option[i].dest);
			   *(char **)option[i].dest = NULL;
			}
            break;
         case rc_file:
            if(*(FILE **)option[i].dest)
			{
               fclose(*(FILE **)option[i].dest);
			   *(FILE **)option[i].dest = NULL;
			}
            break;
      }
   }
}

/* public methods (in rc.h) */
struct rc_struct *rc_create(void)
{
   struct rc_struct *rc = NULL;

   if(!(rc = calloc(1, sizeof(struct rc_struct))))
   {
      fprintf(stderr, "error: malloc failed for: struct rc_struct\n");
      return NULL;
   }

   return rc;
}

void rc_destroy(struct rc_struct *rc)
{
   if(rc->option)
   {
      rc_free_stuff(rc->option);
      free (rc->option);
   }
   if(rc->arg)
      free(rc->arg);
   free(rc);
}

int rc_register(struct rc_struct *rc, struct rc_option *option)
{
   int i;

   /* try to find a free entry in our option list */
   for(i = 0; i < rc->option_size; i++)
      if(rc->option[i].type <= 0)
         break;

   /* do we have space to register this option list ? */
   if(i >= (rc->option_size-1))
   {
      struct rc_option *tmp = realloc(rc->option,
         (rc->option_size + BUF_SIZE) * sizeof(struct rc_option));
      if (!tmp)
      {
         fprintf(stderr, "error: malloc failed in rc_register_option\n");
         return -1;
      }
      rc->option = tmp;
      memset(rc->option + rc->option_size, 0, BUF_SIZE *
         sizeof(struct rc_option));
      rc->option_size += BUF_SIZE;
   }

   /* set the defaults */
   if(rc_set_defaults(option))
      return -1;

   /* register the option */
   rc->option[i].type = rc_link;
   rc->option[i].dest = option;

   return 0;
}

int rc_unregister(struct rc_struct *rc, struct rc_option *option)
{
   int i;

   /* try to find the entry in our option list, unregister later registered
      duplicates first */
   for(i = rc->option_size - 1; i >= 0; i--)
   {
      if(rc->option[i].dest == option)
      {
         memset(rc->option + i, 0, sizeof(struct rc_option));
         rc->option[i].type = rc_ignore;
         return 0;
      }
   }

   return -1;
}

int rc_load(struct rc_struct *rc, const char *name,
   int priority, int continue_on_errors)
{
   FILE *f;

   fprintf(stderr, "info: trying to parse: %s\n", name);

   if (!(f = fopen(name, "r")))
      return 0;

   return rc_read(rc, f, name, priority, continue_on_errors);
}

int rc_save(struct rc_struct *rc, const char *name, int append)
{
   FILE *f;

   if (!(f = fopen(name, append? "a":"w")))
      return -1;

   return rc_write(rc, f, name);
}

int osd_rc_read(struct rc_struct *rc, mame_file *f, const char *description,
   int priority, int continue_on_errors)
{
   char buf[BUF_SIZE];
   int line = 0;

   while(mame_fgets(buf, BUF_SIZE, f))
   {
      struct rc_option *option;
      char *name, *tmp, *arg = NULL;

      line ++;

      /* get option name */
      if(!(name = strtok(buf, " \t\r\n")))
         continue;
      if(name[0] == '#')
         continue;

      /* get complete rest of line */
      arg = strtok(NULL, "\r\n");

      if (arg)
      {
      /* ignore white space */
      for (; (*arg == '\t' || *arg == ' '); arg++) {}

      /* deal with quotations */
      if (arg[0] == '"')
         arg = strtok (arg, "\"");
      else if (arg[0] == '\'')
         arg = strtok (arg, "'");
      else
         arg = strtok (arg, " \t\r\n");
      }

      if(!(option = rc_get_option2(rc->option, name)))
      {
         fprintf(stderr, "error: unknown option %s, on line %d of file: %s\n",
            name, line, description);
      }
      else if (rc_requires_arg[option->type] && !arg)
      {
         fprintf(stderr,
            "error: %s requires an argument, on line %d of file: %s\n",
            name, line, description);
      }
      else if ( (tmp = strtok(NULL, " \t\r\n")) && (tmp[0] != '#') )
      {
         fprintf(stderr,
            "error: trailing garbage: \"%s\" on line: %d of file: %s\n",
            tmp, line, description);
      }
      else if (!rc_set_option3(option, arg, priority))
         continue;

      if (continue_on_errors)
         fprintf(stderr, "   ignoring line\n");
      else
         return -1;
   }
   return 0;
}

int rc_read(struct rc_struct *rc, FILE *f, const char *description,
   int priority, int continue_on_errors)
{
   char buf[BUF_SIZE];
   int line = 0;

   while(fgets(buf, BUF_SIZE, f))
   {
      struct rc_option *option;
      char *name, *tmp, *arg = NULL;

      line ++;

      /* get option name */
      if(!(name = strtok(buf, " \t\r\n")))
         continue;
      if(name[0] == '#')
         continue;

      /* get complete rest of line */
      arg = strtok(NULL, "\r\n");
	  if (!arg)
	  {
		  fprintf(stderr, "error: garbage \"%s\" on line %d of file: %s\n",
			  buf, line, description);
		  continue;
	  }

      /* ignore white space */
      for (; (*arg == '\t' || *arg == ' '); arg++) {}

      /* deal with quotations */
      if (arg[0] == '"')
         arg = strtok (arg, "\"");
      else if (arg[0] == '\'')
         arg = strtok (arg, "'");
      else
         arg = strtok (arg, " \t\r\n");

      if(!(option = rc_get_option2(rc->option, name)))
      {
         fprintf(stderr, "error: unknown option %s, on line %d of file: %s\n",
            name, line, description);
      }
      else if (rc_requires_arg[option->type] && !arg)
      {
         fprintf(stderr,
            "error: %s requires an argument, on line %d of file: %s\n",
            name, line, description);
      }
      else if ( (tmp = strtok(NULL, " \t\r\n")) && (tmp[0] != '#') )
      {
         fprintf(stderr,
            "error: trailing garbage: \"%s\" on line: %d of file: %s\n",
            tmp, line, description);
      }
      else if (!rc_set_option3(option, arg, priority))
         continue;

      if (continue_on_errors)
         fprintf(stderr, "   ignoring line\n");
      else
         return -1;
   }
   return 0;
}

static int real_rc_write(struct rc_option *option, mame_file *f, const char *description)
{
	int i;

	if (description)
		mame_fprintf(f, "### %s ###\n", description);

	for(i=0; option[i].type; i++)
	{
		switch (option[i].type)
		{
			case rc_seperator:
				mame_fprintf(f, "\n### %s ###\n", option[i].name);
				break;

			case rc_link:
				if (real_rc_write(option[i].dest, f, NULL))
					return -1;
				break;

			case rc_string:
				if(!*(char **)option[i].dest)
				{
					mame_fprintf(f, "# %-19s   <NULL> (not set)\n", option[i].name);
					break;
				}
				/* fall through */

			case rc_bool:
			case rc_int:
			case rc_float:
				mame_fprintf(f, "%-21s   ", option[i].name);
				switch(option[i].type)
				{
					case rc_bool:
					case rc_int:
						mame_fprintf(f, "%d\n", *(int *)option[i].dest);
						break;
					case rc_float:
						mame_fprintf(f, "%f\n", *(float *)option[i].dest);
						break;
					case rc_string:
						mame_fprintf(f, "%s\n", *(char **)option[i].dest);
						break;
				}
				break;
		}
	}
	if (description)
		mame_fprintf(f, "\n");
	return 0;
}

int osd_rc_write(struct rc_struct *rc, mame_file *f, const char *description)
{
	return real_rc_write(rc->option, f, description);
}

/* needed to walk the tree */
static int rc_real_write(struct rc_option *option, FILE *f,
   const char *description)
{
   int i;

   if (description)
      fprintf(f, "### %s ###\n", description);

   for(i=0; option[i].type; i++)
   {
      switch (option[i].type)
      {
         case rc_seperator:
            fprintf(f, "\n### %s ###\n", option[i].name);
            break;
         case rc_link:
            if(rc_real_write(option[i].dest, f, NULL))
               return -1;
            break;
         case rc_string:
            if(!*(char **)option[i].dest)
            {
               fprintf(f, "# %-19s   <NULL> (not set)\n", option[i].name);
               break;
            }
         case rc_bool:
         case rc_int:
         case rc_float:
            fprintf(f, "%-21s   ", option[i].name);
            switch(option[i].type)
            {
               case rc_bool:
               case rc_int:
                  fprintf(f, "%d\n", *(int *)option[i].dest);
                  break;
               case rc_float:
                  fprintf(f, "%f\n", *(float *)option[i].dest);
                  break;
               case rc_string:
                  fprintf(f, "%s\n", *(char **)option[i].dest);
                  break;
            }
            break;
      }
   }
   if (description)
      fprintf(f, "\n");
   return 0;
}

int rc_write(struct rc_struct *rc, FILE *f, const char *description)
{
   return rc_real_write(rc->option, f, description);
}

int rc_parse_commandline(struct rc_struct *rc, int argc, char *argv[],
   int priority, int (*arg_callback)(char *arg))
{
   int i;

   for(i=1; i<argc; i++)
   {
      if(argv[i][0] == '-')
      {
         int start = 1;
         struct rc_option *option;
         const char *arg = NULL;

         if(argv[i][1] == '-')
            start = 2;

         if((option = rc_get_option2(rc->option, argv[i] + start)))
         {
            if (option->type == rc_bool)
            {
               /* handle special bool set case */
               arg = "1";
            }
            else
            {
               /* normal option */
               if (rc_requires_arg[option->type])
               {
                  i++;
                  if (i >= argc)
                  {
                     fprintf(stderr, "error: %s requires an argument\n", argv[i-1]);
                     return -1;
                  }
                  arg = argv[i];
               }
            }
         }
         else if(!strncmp(argv[i] + start, "no", 2) &&
            (option = rc_get_option2(rc->option, argv[i] + start + 2)) &&
            (option->type == rc_bool))
         {
            /* handle special bool clear case */
            arg = "0";
         }
         else
         {
            fprintf(stderr, "error: unknown option %s\n", argv[i]);
            return -1;
         }

         if(rc_set_option3(option, arg, priority))
            return -1;
      }
      else
      {
         /* do we have space to register the non-option arg */
         if(rc->args_registered >= (rc->arg_size))
         {
            char **tmp = realloc(rc->arg, (rc->arg_size + BUF_SIZE) *
               sizeof(char *));
            if (!tmp)
            {
               fprintf(stderr,
                  "error: malloc failed in rc_parse_commadline\n");
               return -1;
            }
            rc->arg = tmp;
            memset(rc->arg + rc->arg_size, 0, BUF_SIZE * sizeof(char *));
            rc->arg_size += BUF_SIZE;
         }

         /* register the non-option arg */
         rc->arg[rc->args_registered] = argv[i];
         rc->args_registered++;

         /* call the callback if defined */
         if(arg_callback && (*arg_callback)(argv[i]))
            return -1;
      }
   }
   return 0;
}

int rc_get_non_option_args(struct rc_struct *rc, int *argc, char **argv[])
{
   *argv = rc->arg;
   *argc = rc->args_registered;
   return 0;
}

/* needed to walk the tree */
static void rc_real_print_help(struct rc_option *option, FILE *f)
{
   int i;
   char buf[BUF_SIZE];
   static const char *type_name[] = {"", "", " <string>", " <int>", " <float>",
      "", "", " <filename>", " <arg>", "", "" };

   for(i=0; option[i].type; i++)
   {
      switch (option[i].type)
      {
         case rc_ignore:
            break;
         case rc_seperator:
            fprintf(f, "\n*** %s ***\n", option[i].name);
            break;
         case rc_link:
            rc_real_print_help(option[i].dest, f);
            break;
         default:
            snprintf(buf, BUF_SIZE, "-%s%s%s%s%s%s",
               (option[i].type == rc_bool)? "[no]":"",
               option[i].name,
               (option[i].shortname)? " / -":"",
               (option[i].shortname && (option[i].type == rc_bool))? "[no]":"",
               (option[i].shortname)? option[i].shortname:"",
               type_name[option[i].type]);
            fprint_colums(f, buf,
               (option[i].help)? option[i].help:"no help available");
      }
   }
}

void rc_print_help(struct rc_struct *rc, FILE *f)
{
   rc_real_print_help(rc->option, f);
}

/* needed to walk the tree */
static void rc_real_print_man_options(struct rc_option *option, FILE *f)
{
   int i;
   static const char *type_name[] = {"", "", " Ar string", " Ar int",
      " Ar float", "", "", " Ar filename", " Ar arg", "", "" };

   for(i=0; option[i].type; i++)
   {
      switch (option[i].type)
      {
         case rc_ignore:
            break;
         case rc_seperator:
            fprintf(f, ".It \\fB*** %s ***\\fR\n", option[i].name);
            break;
         case rc_link:
            rc_real_print_man_options(option[i].dest, f);
            break;
         default:
            fprintf(f, ".It Fl %s%s%s%s%s%s\n%s\n",
               (option[i].type == rc_bool)? "[no]":"",
               option[i].name,
               (option[i].shortname)? " , ":"",
               (option[i].shortname && (option[i].type == rc_bool))? "[no]":"",
               (option[i].shortname)? option[i].shortname:"",
               type_name[option[i].type],
               (option[i].help)? option[i].help:"no help available");
      }
   }
}

void rc_print_man_options(struct rc_struct *rc, FILE *f)
{
   rc_real_print_man_options(rc->option, f);
}

int rc_verify_power_of_2(struct rc_option *option, const char *arg,
   int priority)
{
   int i, value;

   value = *(int *)option->dest;

   for(i=0; i<(sizeof(int)*8); i++)
      if(((int)0x01 << i) == value)
         break;
   if(i == (sizeof(int)*8))
   {
      fprintf(stderr, "error invalid value for %s: %s\n", option->name, arg);
      return -1;
   }

   option->priority = priority;

   return 0;
}

int rc_option_requires_arg(struct rc_struct *rc, const char *name)
{
   return rc_option_requires_arg2(rc->option, name);
}

int rc_option_requires_arg2(struct rc_option *option, const char *name)
{
   struct rc_option *my_option;

   if(!(my_option = rc_get_option2(option, name)))
   {
      fprintf(stderr, "error: unknown option %s\n", name);
      return -1;
   }
   return rc_requires_arg[my_option->type];
}

int rc_option_requires_arg3(struct rc_option *option)
{
   return rc_requires_arg[option->type];
}

int rc_get_priority(struct rc_struct *rc, const char *name)
{
   return rc_get_priority2(rc->option, name);
}

int rc_get_priority2(struct rc_option *option, const char *name)
{
   struct rc_option *my_option;

   if(!(my_option = rc_get_option2(option, name)))
   {
      fprintf(stderr, "error: unknown option %s\n", name);
      return -1;
   }
   return my_option->priority;
}

int rc_get_priority3(struct rc_option *option)
{
   return option->priority;
}

int rc_set_option(struct rc_struct *rc, const char *name, const char *arg,
   int priority)
{
   return rc_set_option2(rc->option, name, arg, priority);
}

int rc_set_option2(struct rc_option *option, const char *name,
   const char *arg, int priority)
{
   struct rc_option *my_option;

   if(!(my_option = rc_get_option2(option, name)))
   {
      fprintf(stderr, "error: unknown option %s\n", name);
      return -1;
   }
   return rc_set_option3(my_option, arg, priority);
}

int rc_set_option3(struct rc_option *option, const char *arg, int priority)
{
   char *end;

   /* check priority */
   if(priority < option->priority)
      return 0;

   switch(option->type)
   {
      case rc_string:
         {
            char *str;
            if ( !( str = malloc(strlen(arg)+1) ) )
            {
               fprintf(stderr, "error: malloc failed for %s\n", option->name);
               return -1;
            }
            strcpy(str, arg);
            if(*(char **)option->dest)
               free(*(char **)option->dest);
            *(char **)option->dest = str;
         }
         break;
      case rc_int:
      case rc_bool:
         {
            int x;
            x = strtol(arg, &end, 0);
            if (*end || rc_verify(option, x))
            {
               fprintf(stderr, "error invalid value for %s: %s\n", option->name, arg);
               return -1;
            }
            *(int *)option->dest = x;
         }
         break;
      case rc_float:
         {
            float x;
            x = strtod(arg, &end);
            if (*end || rc_verify(option, x))
            {
               fprintf(stderr, "error invalid value for %s: %s\n", option->name, arg);
               return -1;
            }
            *(float *)option->dest = x;
         }
         break;
      case rc_set_int:
         *(int*)option->dest = option->min;
         break;
      case rc_file:
         {
            FILE *f = fopen(arg, (option->min)? "w":"r");
            if(!f)
            {
               fprintf(stderr, "error: couldn't open file: %s\n", arg);
               return -1;
            }
            if (*(FILE **)option->dest)
               fclose(*(FILE **)option->dest);
            *(FILE **)option->dest = f;
         }
         break;
      case rc_use_function:
      case rc_use_function_no_arg:
         break;
      default:
         fprintf(stderr,
            "error: unknown option type: %d, this should not happen!\n",
            option->type);
         return -1;
   }
   /* functions should do there own priority handling, so that they can
      ignore priority handling if they wish */
   if(option->func)
      return (*option->func)(option, arg, priority);

   option->priority = priority;

   return 0;
}

struct rc_option *rc_get_option(struct rc_struct *rc, const char *name)
{
   return rc_get_option2(rc->option, name);
}

struct rc_option *rc_get_option2(struct rc_option *option, const char *name)
{
   int i;
   struct rc_option *result;

   for(i=0; option[i].type; i++)
   {
      switch(option[i].type)
      {
         case rc_ignore:
         case rc_seperator:
            break;
         case rc_link:
            if((result = rc_get_option2(option[i].dest, name)))
               return result;
            break;
         default:
            if(!strcmp(name, option[i].name) ||
               (option[i].shortname &&
                  !strcmp(name, option[i].shortname)))
               return &option[i];
      }
   }
   return NULL;
}

/* gimmi the entire tree, I want todo all the parsing myself */
struct rc_option *rc_get_options(struct rc_struct *rc)
{
   return rc->option;
}



#if 0
/* various utility methods which don't really belong to the rc object,
   but seem to fit here well */

/* locate user's home directory */
char *rc_get_home_dir(void)
{
   struct passwd *pw;
   char *s;

   if (!(pw=getpwuid(getuid())))
   {
      fprintf(stderr, "Who are you? Not found in passwd database!!\n");
      return NULL;
   }
   if (!(s=malloc(strlen(pw->pw_dir)+1)))
   {
      fprintf(stderr, "error: malloc faild for homedir string\n");
      return NULL;
   }
   strcpy(s, pw->pw_dir);
   return s;
}

/*
 * check and if nescesarry create dir
 */
int rc_check_and_create_dir(const char *name)
{
   struct stat stat_buffer;

   if (stat(name, &stat_buffer))
   {
      /* error check if it doesn't exist or something else is wrong */
      if (errno == ENOENT)
      {
         /* doesn't exist letts create it ;) */
#ifdef BSD43
	 if (mkdir(name, 0775))
#else
         if (mkdir(name, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH))
#endif
         {
            fprintf(stderr, "Error creating dir %s", name);
            perror(" ");
            return -1;
         }
      }
      else
      {
         /* something else went wrong yell about it */
         fprintf(stderr, "Error opening %s", name);
         perror(" ");
         return -1;
      }
   }
   else
   {
      /* file exists check it's a dir otherwise yell about it */
#ifdef BSD43
      if(!(S_IFDIR & stat_buffer.st_mode))
#else
      if(!S_ISDIR(stat_buffer.st_mode))
#endif
      {
         fprintf(stderr,"Error %s exists but isn't a dir\n", name);
         return -1;
      }
   }
   return 0;
}
#endif
