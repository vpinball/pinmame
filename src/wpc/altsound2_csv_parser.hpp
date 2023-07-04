// ---------------------------------------------------------------------------
// altsound2_csv_parser.hpp
// 06/23/23 - Dave Roscoe
//
// Parser for AltSound2 format CSV files
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND2_CSV_PARSER_HPP
#define ALTSOUND2_CSV_PARSER_HPP
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

// Std Library includes
#include <string>

// local includes
#include "altsound_data.hpp"

extern std::string get_vpinmame_path();

// ---------------------------------------------------------------------------
// Class definitions
// ---------------------------------------------------------------------------

class Altsound2CsvParser {
public: // methods

	// Standard constructor
	Altsound2CsvParser(const std::string& path_in);

	bool parse(Samples& psd);

public: // data

protected:

	// Default constructor
	Altsound2CsvParser() {/* not used */ };

	// Copy constructor
	Altsound2CsvParser(Altsound2CsvParser&) {/* not used */ };

private: // functions

	// trim any whitespace from field values
	std::string trim(const std::string& str);

private: // data

	std::string altsound_path;
	std::string filename;
};

#endif // ALTSOUND2_CSV_PARSER_HPP
