// ---------------------------------------------------------------------------
// gsound_csv_parser.hpp
//
// Parser for G-Sound format CSV files
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
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
