#ifndef ALTSOUND_FILE_PARSER_HPP
#define ALTSOUND_FILE_PARSER_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "altsound_data.h"

class AltsoundFileParser {
public:
	// Standard constructor
	AltsoundFileParser(const char *gname_in);

	bool parse(PinSamples* psd);

protected:
	// Default constructor
	AltsoundFileParser();
private: // functions

private: // data
	const char* g_szGameName;
//	char* base_path;
	int base_path_length;
	char cvpmd[1024];
	const char* path_main = "\\altsound\\";
};

#endif //ALTSOUND_FILE_PARSER_HPP
