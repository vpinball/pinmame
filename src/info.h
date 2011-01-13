#ifndef __INFO_H
#define __INFO_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

/* Print the MAME database in INFO format */
void print_mame_info(FILE* out, const struct GameDriver* games[]);

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, const struct GameDriver* games[]);

#endif
