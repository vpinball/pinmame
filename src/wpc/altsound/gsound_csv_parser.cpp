// ---------------------------------------------------------------------------
// gsound_csv_parser.cpp
// 06/23/23 - Dave Roscoe
//
// Parser for G-Sound format CSV files
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#include "gsound_csv_parser.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>

#include "altsound_logger.hpp"

extern AltsoundLogger alog;

// ----------------------------------------------------------------------------
// CTOR/DTOR
// ----------------------------------------------------------------------------

GSoundCsvParser::GSoundCsvParser(const std::string& path_in)
: altsound_path(path_in)
{
	filename = altsound_path + "/g-sound.csv";
}

// ----------------------------------------------------------------------------

bool GSoundCsvParser::parse(std::vector<GSoundSampleInfo>& samples_out)
{
	ALT_DEBUG(0, "BEGIN GSoundCsvParser::parse()");
	INDENT;

	std::ifstream file(filename);
	if (!file.is_open()) {
		ALT_ERROR(0, "Unable to open file: %s", filename.c_str());

		OUTDENT;
		ALT_DEBUG(0, "END GSoundCsvParser::parse()");
		return false;
	}

	std::string line;

	// skip header row
	std::getline(file, line);

	std::unordered_set<std::string> allowed_types = {
		"music",
		"callout",
		"solo",
		"sfx",
		"overlay" 
	};
	
	bool success = true;

	try {
		while (std::getline(file, line)) {
			if (line.empty()) {
				// ignore blank lines
				continue;
			}

			// Some Altsounds use quotes around fields.  These need to be removed.
			line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());

			std::stringstream ss(line);
			std::string field;
			GSoundSampleInfo entry;

			// Read ID field (unsigned hexadecimal)
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				entry.id = std::stoul(field, nullptr, 16);
			}
			else {
				ALT_ERROR(1, "Failed to parse ID field");
				success = false;
				break;
			}

			// Read TYPE field
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				//field = toLower(field);
				std::transform(field.begin(), field.end(), field.begin(), ::tolower);
				entry.type = field;

				if (allowed_types.find(field) == allowed_types.end()) {
					ALT_ERROR(1, "%s is not a known sample type");
					success = false;
					break;
				}

				if (field == "music") {
					entry.loop = true;
				}
			}
			else {
				ALT_ERROR(1, "Failed to parse TYPE field");
				entry.type.clear();
				success = false;
				break;
			}

			// Read GAIN field (float)
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				float val = std::stof(field);
				entry.gain = val < 0.0f ? 0.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}
			else {
				ALT_ERROR(1, "Failed to parse GAIN field");
				entry.gain = 1.0f;
				success = false;
				break;
			}

			// Read DUCKING_PROFILE field (uint)
			if (std::getline(ss, field, ','))
			{
				if (field.empty()) {
					field = "0"; // default value
				}

				field = trim(field);
				unsigned int val = std::stoul(field);
				entry.ducking_profile = val;
			}
			else {
				ALT_ERROR(1, "Failed to parse DUCKING_PROFILE field");
				entry.ducking_profile = 0;
				success = false;
				break;
			}

			// Read FNAME field
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				if (field.empty()) {
					ALT_ERROR(1, "Sample filename is blank");
					success = false;
					entry.fname.clear();  // assign some default value
					break;
				}

				std::string sample_path = altsound_path + '/' + field;

				// Normalize to forward slashes
				std::replace(sample_path.begin(), sample_path.end(), '\\', '/');
				entry.fname = sample_path;
			}
			else {
				ALT_ERROR(1, "Failed to parse FNAME");
				success = false;
				entry.fname.clear();  // assign some default value
				break;
			}

			samples_out.push_back(entry);

			std::ostringstream debug_stream;
			debug_stream << "ID = 0x" << std::setfill('0') << std::setw(4) << std::hex << entry.id << std::dec
				<< ", TYPE = " << entry.type
				<< ", GAIN = " << std::fixed << std::setprecision(2) << entry.gain
				<< ", DUCK_PRF = " << entry.ducking_profile
				<< ", FNAME = " << entry.fname;

			ALT_DEBUG(0, debug_stream.str().c_str());
		}
	}
	catch (const std::exception& e) {
		ALT_ERROR(0, "GSoundCsvParser::parse(): %s", e.what());
	}

	file.close();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundCsvParser::parse()");
	return success;
}
