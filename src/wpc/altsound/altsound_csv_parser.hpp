// ---------------------------------------------------------------------------
// altsound_csv_parser.hpp
// 06/23/23 - Dave Roscoe
//
// Parser for legacy and traditional AltSound format CSV files
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND_CSV_PARSER_HPP
#define ALTSOUND_CSV_PARSER_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Std Library includes
#include <string>

// local includes
#include "altsound_data.hpp"

//extern std::string get_vpinmame_path();

// ---------------------------------------------------------------------------
// Class definitions
// ---------------------------------------------------------------------------

class AltsoundCsvParser {
public: // methods

	// Standard constructor
	explicit AltsoundCsvParser(const std::string& altsound_path_in);

	bool parse(std::vector<AltsoundSampleInfo>& samples_out);

public: // data

protected:

	// Default constructor
	AltsoundCsvParser() {/* not used */ };

	// Copy constructor
	AltsoundCsvParser(AltsoundCsvParser&) {/* not used */ };

private: // functions

private: // data

	std::string altsound_path;
	std::string filename;
};

#endif // ALTSOUND_CSV_PARSER_HPP
