// ---------------------------------------------------------------------------
// gsound_csv_parser.hpp
// 06/23/23 - Dave Roscoe
//
// Parser for G-Sound format CSV files
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef GSOUND_CSV_PARSER_HPP
#define GSOUND_CSV_PARSER_HPP
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

class GSoundCsvParser {
public: // methods

	// Standard constructor
	explicit GSoundCsvParser(const std::string& path_in);

	bool parse(std::vector<GSoundSampleInfo>& samples_out);

public: // data

protected:

	// Default constructor
	GSoundCsvParser() {/* not used */ }

	// Copy constructor
	GSoundCsvParser(GSoundCsvParser&) {/* not used */ }

private: // functions

private: // data

	std::string altsound_path;
	std::string filename;
};

#endif // GSOUND_CSV_PARSER_HPP
