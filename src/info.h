#ifndef __INFO_H
#define __INFO_H

/* Print the MAME database in INFO format */
void print_mame_info(FILE* out, const struct GameDriver* games[]);

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, const struct GameDriver* games[]);

#endif
