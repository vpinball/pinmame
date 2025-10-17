// ---------------------------------------------------------------------------
// altsound_csv_parser.hpp
//
// Parser for legacy and traditional AltSound format CSV files
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders:Carsten Wächter, Dave Roscoe
// ---------------------------------------------------------------------------
#pragma once

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
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
