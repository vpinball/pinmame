/* A simple text parser

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
#ifndef __PARSER_H
#define __PARSER_H

#include "begin_code.h"

struct parser_element
{
   const char *name;
   const char *shortname;
   int (*function)(const char *arg[], void *flags);
   int arg_count;
   void *flags;
};

struct parser_struct
{
   const struct parser_element **element;
};

struct parser_struct *parser_create(const struct parser_element *element[]);

void parser_destroy(struct parser_struct *parser);

int parser_parse_string(struct parser_struct *parser, char *string);

int parser_parse_tokens(struct parser_struct *parser, const char *command,
   int tokenc, const char *tokenv[], int *tokens_used);
   
int parser_get_arg_count(struct parser_struct *parser, const char *command,
   int *arg_count);

#include "end_code.h"
#endif /* ifndef __PARSER_H */
