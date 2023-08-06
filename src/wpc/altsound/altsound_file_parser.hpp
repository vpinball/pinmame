#ifndef ALTSOUND_FILE_PARSER_HPP
#define ALTSOUND_FILE_PARSER_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "snd_alt.h"
#include "altsound_data.hpp"

class AltsoundFileParser {
public:
	
	// Standard constructor
	AltsoundFileParser(const std::string& altsound_path_in);

	bool parse(std::vector<AltsoundSampleInfo>& samples_out);

protected:
	
	// Default constructor
	AltsoundFileParser() {};

private: // functions

private: // data
	std::string altsound_path;
};

#endif //ALTSOUND_FILE_PARSER_HPP
