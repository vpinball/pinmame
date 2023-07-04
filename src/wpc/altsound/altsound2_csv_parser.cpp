// ---------------------------------------------------------------------------
// altsound2_csv_parser.cpp
// 06/23/23 - Dave Roscoe
//
// Parser for AltSound2 format CSV files
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "altsound2_csv_parser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

// ----------------------------------------------------------------------------
// CTOR/DTOR
// ----------------------------------------------------------------------------

Altsound2CsvParser::Altsound2CsvParser(const std::string& path_in)
: altsound_path(path_in),
  filename()
{
	filename = altsound_path + "\\altsound2.csv";
}

// ----------------------------------------------------------------------------

bool Altsound2CsvParser::parse(Samples& samples)
{
	LOG(("BEGIN: Altsound2CsvParser::parse()\n"));
	bool success = false;

	std::ifstream file(filename);
	if (!file.is_open()) {
		LOG(("ERROR: Unable to open file: %s\n", filename.c_str()));
		return false;
	}

	std::string line;
	// DAR_TODO make this case-insensitive
	// skip header row
	std::getline(file, line);

	try {
		while (std::getline(file, line)) {
			std::stringstream ss(line);
			std::string field;
			SampleInfo entry;

			// Read ID field (unsigned hexadecimal)
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				entry.id = std::stoul(field, nullptr, 16);
			}

			// Read TYPE field
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				field = toLower(field);
				entry.type = field;

				if (field == "music") {
					entry.loop = true;
				}
			}

			// Read GAIN field (float)
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				float val = std::stof(field);
				entry.gain = val < 0.0f ? 0.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}

			// Read FNAME field
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				field = toLower(field);
				std::string sample_path = altsound_path + '\\' + field;
				entry.fname = sample_path;
			}


			samples.push_back(entry);
			LOG(("ID = %d, TYPE = %s, GAIN = %.02f, FNAME = '%s'\n" \
				, entry.id, entry.type.c_str(), entry.gain, entry.fname.c_str()));
		}
		success = true;
	}
	catch (std::exception e) {
		LOG(("FAILED: Altsound2CsvParser::parse(): %s", e.what()));
	}

	file.close();
	LOG(("BEGIN: Altsound2CsvParser::parse()\n"));
	return success;
}

// ----------------------------------------------------------------------------
// Helper function to trim whitespace from parsed field values
// ----------------------------------------------------------------------------

std::string Altsound2CsvParser::trim(const std::string& str)
{
	size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}
