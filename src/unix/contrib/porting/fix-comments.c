#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#if 0
#define DEBUG_OUT(s)    printf(s)
#define DEBUG_OUT1(s,a) printf(s,a)
#else
#define DEBUG_OUT(s)
#define DEBUG_OUT1(s,a)
#endif

int main (int argc, char **argv)
{
   int n, i, len;
   int status, line;
   FILE *fpin, *fpout;
   char input_filename[BUFSIZ], output_filename[BUFSIZ];
   char buffer[BUFSIZ];

   if (argc < 2)
   {
      printf ("Usage: fix [sourcefilenames]\n");
      return 1;
   }

   /* multiple argument */
   for (n = 1; n < argc; n++)
   {
      strcpy (output_filename, argv[n]);  /* filename to write */
      strcpy (input_filename, argv[n]);

      if ((fpin = fopen (input_filename, "r")) == NULL)
      {
         printf ("Error opening input file %s\n", input_filename);
         printf ("Continuing with next file\n");
         continue;
      }

      strcat (input_filename, ".orig");  /* filename to backup */

      if (rename (output_filename, input_filename) == -1)
      {
         printf ("Error renaming %s to %s\n", output_filename, input_filename);
         printf ("Continuing with next file\n");
         continue;
      }

      if ((fpout = fopen (output_filename, "w")) == NULL)
      {
         printf ("Error opening output file %s\n", output_filename);
         printf ("Continuing with next file\n");
         continue;
      }

      status = 0;
      line = 1;
      
      DEBUG_OUT1("Processing File: %s\n", output_filename);

      while (fgets (buffer, BUFSIZ, fpin) != NULL)
      {
         DEBUG_OUT1("  Line: %d\n", line);
         len = strlen (buffer);
         /* strip newline */
         if (buffer[len - 1] == '\n')
            len--;
         /* if dos format and len > 0, strip cariage return(s) and EOF */
         while (len && (buffer[len - 1] == 0x0D || buffer[len - 1] == 0x1A))
               len--;

         /* search and lowercase #include's */
         if (strncmp (buffer, "#include", 8) == 0)
         {
            DEBUG_OUT("Lowercasing #include\n");
            for (i = 8; i < len && buffer[i] != '<' && buffer[i] != '\"'; i++)
            {
            }
            for (i++; i < len && buffer[i] != '>' && buffer[i] != '\"'; i++)
               if (buffer[i] == '\\')
                  buffer[i] = '/';
               else
                  buffer[i] = tolower (buffer[i]);

            if (i >= len)
            {
               fprintf (stderr, "File: %s, Malformed #include at line %d, aborting\n",
                        output_filename, line);
               exit (1);
            }
         }

         /* search and transform // comments to /* */
         for (i = 0; i < len; i++)
         {
            switch (status)
            {

               case 0:         /* nothing found sofar */
                  switch (buffer[i])
                  {
                     case '/':
                        DEBUG_OUT1("Found a / at: %d\n", i+1);
                        status = 1;
                        break;
                     case '\"':
                     case '\'':
                     {
                        int escapes=0;
                        /* to avoid constructs like "\\" and or "\\\"" we have
                           to count the number of backslashes */
                        while (i && buffer[i-1]=='\\')
                        {
                           escapes++;
                           i--;
                        }
                        /* restore i */
                        i += escapes;
                        /* if the number of \ is odd the " or ' is escaped ;) */
                        if (escapes&0x01) break;
                        DEBUG_OUT1("Found an unescaped \" or ' at: %d, entering string\n", i+1);
                        if (buffer[i] == '\"')
                           status = 7;
                        else
                           status = 8;
                        break;
                     }
                  }
                  break;
               case 1:         /* found a / */
                  switch (buffer[i])
                  {
                     case '*':
                        DEBUG_OUT1("Found a * at: %d, entering old style comment\n", i+1);
                        status = 2;
                        break;
                     case '/': /* found cpp-style comment bah */
                        DEBUG_OUT1("Found a / at: %d, entering cpp style comment\n", i+1);
                        buffer[i] = '*';
                        buffer[len] = ' ';
                        buffer[len + 1] = '*';
                        buffer[len + 2] = '/';
                        len += 3;
                        status = 4;
                        break;
                     default:
                        status = 0;
                        break;
                  }
                  break;
               case 2:         /* currently in old-style commented  text */
                  if (buffer[i] == '*')
                  {
                     DEBUG_OUT1("Found a * at: %d\n", i+1);
                     status = 3;
                  }
                  break;
               case 3:         /* in old-style comment, found * */
                  switch (buffer[i])
                  {
                     case '/':
                        DEBUG_OUT1("Found a / at: %d, leaving old style comment\n", i);
                        status = 0;
                        break;
                     case '*':
                        DEBUG_OUT1("Found a * at: %d\n", i+1);
                        break;
                     default:
                        status = 2;
                  }
                  break;
               case 4:         /* in cpp-style comment */
                  if (buffer[i] == '/')
                  {
                     DEBUG_OUT1("Found a / at: %d\n", i+1);
                     status = 5;
                  }
                  if (buffer[i] == '*')
                  {
                     DEBUG_OUT1("Found a * at: %d\n", i+1);
                     status = 6;
                  }
                  break;
               case 5:         /* in cpp-style comment, found / */
                  if (buffer[i] == '*')
                  {
                     DEBUG_OUT1("Found a * at: %d, removing old style comment start\n", i+1);
                     buffer[i - 1] = ' ';
                     buffer[i] = ' ';
                     status = 4;
                  }
                  else if (buffer[i] != '/')
                     status = 4;
                  break;
               case 6:         /* in cpp-style comment, found * */
                  if (buffer[i] == '/' && i < (len - 1))
                  {
                     DEBUG_OUT1("Found a / at: %d, removing old style comment end\n", i+1);
                     buffer[i - 1] = ' ';
                     buffer[i] = ' ';
                     status = 4;
                  }
                  else if (buffer[i] == '/' && i == (len - 1))
                  {
                     DEBUG_OUT1("Found a / at: %d, end of line\n", i+1);
                     status = 0;
                  }
                  else if (buffer[i] != '*')
                     status = 4;
                  break;
               case 7: /* in a string ("string") */
               case 8: /* in a string ('string') */
                  if ((status == 7 && buffer[i] == '\"') ||
                      (status == 8 && buffer[i] == '\''))
                  {
                     int escapes=0;
                     /* to avoid constructs like "\\" and or "\\\"" we have
                        to count the number of backslashes */
                     while (i && buffer[i-1]=='\\')
                     {
                        escapes++;
                        i--;
                     }
                     /* restore i */
                     i += escapes;
                     /* if the number of \ is odd the " is escaped ;) */
                     if (escapes&0x01) break;
                     DEBUG_OUT1("Found an unescaped \" or ' at: %d, leaving string\n", i+1);
                     status = 0;
                  }
                  break;
            }
         }
         /* add newline */
         buffer[len] = '\n';
         buffer[len + 1] = 0;
         fputs (buffer, fpout);
         line++;
      }
      if(status)
         printf("Warning, file ends abnormally: %s\n", output_filename);

      fclose (fpout);
      fclose (fpin);
   }

   return 0;
}
