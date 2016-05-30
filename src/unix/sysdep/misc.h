/* Miscellaneous utility functions

   Copyright 2000 Hans de Goede
   
   This file and the accompanying files in this directory are free software;
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
#ifndef __MISC_H
#define __MISC_H
#include <stdio.h>
#include "begin_code.h"

#ifdef __BEOS__
#include <OS.h>
/* BeOS doesn't have the "usleep()" function. It has the "snooze()"
   one which sleeps in microseconds, not milliseconds. */
#define usleep(x) snooze((x)/1000)
#endif

/* clock stuff */
typedef long uclock_t;
uclock_t uclock(void);
#define UCLOCKS_PER_SEC 1000000

/* print colum stuff */
void print_columns(const char *text1, const char *text2);
void fprint_columns(FILE *f, const char *text1, const char *text2);

#include "end_code.h"
#endif /* ifndef __MISC_H */
