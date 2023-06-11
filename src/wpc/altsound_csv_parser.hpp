#ifndef ALTSOUND_CSV_PARSER_HPP
#define ALTSOUND_CSV_PARSER_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "altsound_data.h"

class AltsoundCsvParser {
public:
	// Standard constructor
	AltsoundCsvParser(char *gname_in);

	bool parse(PinSamples* psd);

protected:
//	// Default constructor
	AltsoundCsvParser();

private: // functions
	void csv_close();
	int csv_read_header();
	int csv_read_record();
	int csv_get_colnumber_for_field(const char* fieldname);
	int csv_get_hex_field(const int field_index, int* pValue);
	int csv_get_int_field(const int field_index, int* pValue);
	void trim(const char* const start, char* end);
	int fgetline(char* const buff, const int nchars, FILE* const file);
	int parse_line(char* line, const int header);
	void free_record();
	int csv_get_float_field(const int field_index, float* pValue);
	int csv_get_str_field(const int field_index, char** pValue);

private: // data
	CsvReader *reader;
	char* g_szGameName;
	char* filename;
	char cvpmd[1024];
	const char* path_main = "\\altsound\\";
};
#endif //ALTSOUND_CSV_PARSER_HPP
