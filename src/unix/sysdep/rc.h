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
#ifndef __RC_H
#define __RC_H

#include <stdio.h>
#include "begin_code.h"
#include "fileio.h"

struct rc_struct;
struct rc_option;

enum { rc_ignore = -1, rc_end, rc_bool, rc_string, rc_int, rc_float,
   rc_set_int, rc_seperator, rc_file, rc_use_function,
   rc_use_function_no_arg, rc_link };

typedef int(*rc_func)(struct rc_option *option, const char *arg,
   int priority);

struct rc_option
{
   const char *name;  /* name of the option */
   const char *shortname;  /* shortcut name of the option, or clear for bool */
   int type;          /* type of the option */
   void *dest;        /* ptr to where the value of the option should be stored */
   const char *deflt; /* default value of the option in a c-string */
   float min;         /* used to verify rc_int or rc_float, this check is not */
   float max;         /* done if min == max. min is also used as value for
                         set_int, and as write flag for rc_file. */
   rc_func func;      /* function which is called for additional verification
                         of the value, or which is called to parse the value if
                         type == use_function, or NULL. Should return 0 on
                         success, -1 on failure */
   const char *help;  /* help text for this option */
   int priority;      /* priority of the current value, the current value
                         is only changed when the priority of the source
                         is higher as this, and then the priority is set to
                         the priority of the source */
};

/* open / close */
struct rc_struct *rc_create(void);
void rc_destroy(struct rc_struct *rc);

/* register / unregister */
int rc_register(struct rc_struct *rc, struct rc_option *option);
int rc_unregister(struct rc_struct *rc, struct rc_option *option);

/* load/save (read/write) a configfile */
int rc_load(struct rc_struct *rc, const char *name, int priority,
   int continue_on_errors);
int rc_save(struct rc_struct *rc, const char *name, int append);
int osd_rc_read(struct rc_struct *rc, mame_file *f, const char *description,
   int priority, int continue_on_errors);
int osd_rc_write(struct rc_struct *rc, mame_file *f, const char *description);
int rc_read(struct rc_struct *rc, FILE *f, const char *description,
   int priority, int continue_on_errors);
int rc_write(struct rc_struct *rc, FILE *f, const char *description);

/* commandline handling */
int rc_parse_commandline(struct rc_struct *rc, int argc, char *argv[],
   int priority, int(*arg_callback)(char *arg));
int rc_get_non_option_args(struct rc_struct *rc, int *argc, char **argv[]);

/* print help */
void rc_print_help(struct rc_struct *rc, FILE *f);

/* print commandline options in manpage style */
void rc_print_man_options(struct rc_struct *rc, FILE *f);

/* some default verify functions */
int rc_verify_power_of_2(struct rc_option *option, const char *arg,
   int priority);

/* functions which can be used in option functions or to build your own
   parser. */
/* 3 ways to query if an option needs arguments, to query it's priority and
   to set it:
   -by name, searching the options in a rc instance
   -by name, searching an array of options, as given to rc_register
   -using the option given (which could for example have been returned
    by rc_get_option) */
int rc_option_requires_arg(struct rc_struct *rc, const char *name);
int rc_option_requires_arg2(struct rc_option *option, const char *name);
int rc_option_requires_arg3(struct rc_option *option);

int rc_get_priority(struct rc_struct *rc, const char *name);
int rc_get_priority2(struct rc_option *option, const char *name);
int rc_get_priority3(struct rc_option *option);

int rc_set_option(struct rc_struct *rc, const char *name, const char *arg,
   int priority);
int rc_set_option2(struct rc_option *option, const char *name,
   const char *arg, int priority);
int rc_set_option3(struct rc_option *option, const char *arg, int priority);
   
/* 2 ways to get the option_struct belonging to a certain option:
   -by name, searching the options in a rc instance
   -by name, searching an array of options, as given to rc_register */
struct rc_option *rc_get_option(struct rc_struct *rc, const char *name);
struct rc_option *rc_get_option2(struct rc_option *option, const char *name);

/* gimmi the entire tree, I want todo all the parsing myself */
struct rc_option *rc_get_options(struct rc_struct *rc);

/* various utility functions which don't really belong to the rc object,
   but seem to fit here well */
int rc_check_and_create_dir(const char *name);
char *rc_get_home_dir(void);

#include "end_code.h"
#endif /* ifndef __RC_H */
