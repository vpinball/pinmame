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
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>

#include "altsound_logger.hpp"

extern BehaviorInfo music_behavior;
extern BehaviorInfo callout_behavior;
extern BehaviorInfo sfx_behavior;
extern BehaviorInfo solo_behavior;
extern BehaviorInfo overlay_behavior;

extern AltsoundLogger alog;

// ----------------------------------------------------------------------------
// CTOR/DTOR
// ----------------------------------------------------------------------------

GSoundCsvParser::GSoundCsvParser(const std::string& path_in)
: altsound_path(path_in),
  filename()
{
	filename = altsound_path + "\\g-sound.csv";
}

// ----------------------------------------------------------------------------

bool GSoundCsvParser::parse(std::vector<SampleInfo>& samples_out)
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

			// Read GAIN field (float)
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				float val = std::stof(field);
				entry.gain = val < 0.0f ? 0.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}

			// Read DUCKING_PROFILE field (uint)
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				unsigned int val = std::stoul(field);
				entry.ducking_profile = val;
			}

			// Read FNAME field
			if (std::getline(ss, field, ','))
			{
				if (field.empty()) {
					ALT_ERROR(1, "Sample filename is blank");
					success = false;
					break;
				}

				field = trim(field);
				std::transform(field.begin(), field.end(), field.begin(), ::tolower);
				std::string sample_path = altsound_path + '\\' + field;
				entry.fname = sample_path;
			}

			samples_out.push_back(entry);
			ALT_DEBUG(0, "ID = %d, TYPE = %s, GAIN = %.02f, FNAME = '%s'", entry.id,
				 entry.type.c_str(), entry.gain, entry.fname.c_str());
		}
	}
	catch (std::exception e) {
		ALT_ERROR(0, "GSoundCsvParser::parse(): %s", e.what());
	}

	file.close();

	OUTDENT;
	ALT_DEBUG(0, "END GSoundCsvParser::parse()");
	return success;
}
