// ---------------------------------------------------------------------------
// altsound_file_parser.hpp
//
// Parser for Legacy format sample files and directories
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders:Carsten Wächter, Dave Roscoe
// ---------------------------------------------------------------------------
#pragma once

#include "snd_alt.h"
#include "altsound_data.hpp"

class AltsoundFileParser {
public:

	// Standard constructor
	explicit AltsoundFileParser(const std::string& altsound_path_in);

	bool parse(std::vector<AltsoundSampleInfo>& samples_out);

protected:

	// Default constructor
	AltsoundFileParser() {};

private: // functions

	float parseFileValue(const std::string& filePath);

private: // data
	std::string altsound_path;
};
