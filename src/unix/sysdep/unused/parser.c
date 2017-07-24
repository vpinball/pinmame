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
/* TODO:
- allow unlimited number of arguments
- check for comments ('#'), to allow parsing files
- check for to much arguments
- allow registering / removing parser elements
- make a copy of the string before parsing it, make string a const * ?
- put parser_struct in parser_priv.h
*/
/* Changelog
Version 0.1, November 1999
-initial release (Hans de Goede)
Version 0.2, December 1999
-changed parser_parse_tokens to take the command as a seperate argument
 instead of using token[0], this allows the caller to remove the - for
 a commandline option for example. (Hans de Goede)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

struct parser_struct *parser_create(const struct parser_element *element[])
{
   int i,j;
   struct parser_struct *parser = NULL;
   
   if (!element)
      return NULL;
      
   for(i=0; element[i]; i++)
   {
      for(j=0; element[i][j].name; j++)
      {
         if(element[i][j].arg_count > BUF_SIZE)
         {
            fprintf(stderr,
               "error parser doesn't support commands with more then %d arguments",
               BUF_SIZE);
            return NULL;
         }
      }
   }
   
   if (!(parser = calloc(1, sizeof(struct parser_struct))))
   {
      fprintf(stderr, "error malloc failed for struct parser_struct");
      return NULL;
   }
   parser->element = element;
   return parser;
}

void parser_destroy(struct parser_struct *parser)
{
   if (parser)
      free(parser);
}

static const struct parser_element *parser_find_element(
   struct parser_struct *parser, const char *command)
{
   int i,j;
   
   for(i=0; parser->element[i]; i++)
   {
      for(j=0; parser->element[i][j].name; j++)
      {
         if(!strcmp(command, parser->element[i][j].name) ||
            (parser->element[i][j].shortname &&
               !strcmp(command, parser->element[i][j].shortname)))
         {
            return &parser->element[i][j];
         }
      }
   }
   fprintf(stderr, "error unknown command: %s", command);
   return NULL;
}

int parser_parse_string(struct parser_struct *parser, char *string)
{
   int i;
   const char *command;
   const char *arg[BUF_SIZE];
   const struct parser_element *element = NULL;
  
   if(!(command = strtok(string, " \t\r\n")))
      return 0;
 
   if(!(element = parser_find_element(parser, command)))
      return -1;

   for(i=0; i < element->arg_count; i++)
   {
      arg[i] = strtok(NULL, " \t\r\n");
      if (!arg[i])
      {
         fprintf(stderr, "error %s requires %d arguments",
            element->name, element->arg_count);
         return -1;
      }
   }
   
   return (*element->function)(arg, element->flags);
}

int parser_parse_tokens(struct parser_struct *parser, const char *command,
   int tokenc, const char *tokenv[], int *tokens_used)
{
   const struct parser_element *element = NULL;
   
   *tokens_used = 0;
      
   if(!(element = parser_find_element(parser, command)))
      return -1;

   if (tokenc < element->arg_count)
   {
      fprintf(stderr, "error %s requires %d arguments",
         element->name, element->arg_count);
      return -1;
   }
   *tokens_used = element->arg_count;
   return (*element->function)(tokenv, element->flags);
}

int parser_get_arg_count(struct parser_struct *parser, const char *command,
   int *arg_count)
{
   const struct parser_element *element = NULL;
   
   *arg_count = 0;
      
   if(!(element = parser_find_element(parser, command)))
      return -1;
   
   *arg_count = element->arg_count;
   return 0;
}

