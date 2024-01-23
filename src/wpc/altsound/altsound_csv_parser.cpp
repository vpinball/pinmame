// ---------------------------------------------------------------------------
// altsound_csv_parser.cpp
//
// Parser for legacy and traditional altsound format CSV files
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders:Carsten Wächter, Dave Roscoe
// ---------------------------------------------------------------------------
#include "altsound_csv_parser.hpp"

#include <iomanip>
#include <algorithm>

#include "altsound_logger.hpp"

extern AltsoundLogger alog;

// ----------------------------------------------------------------------------
// CTOR/DTOR
// ----------------------------------------------------------------------------

AltsoundCsvParser::AltsoundCsvParser(const std::string& altsound_path_in)
: altsound_path(altsound_path_in)
{
	filename = altsound_path + "/altsound.csv";
}

// ----------------------------------------------------------------------------

bool AltsoundCsvParser::parse(std::vector<AltsoundSampleInfo>& samples_out)
{
	ALT_DEBUG(0, "BEGIN AltsoundCsvParser::parse()");
	INDENT;

	std::ifstream file(filename);
	if (!file.is_open()) {
		ALT_ERROR(0, "Unable to open file: %s", filename.c_str());

		OUTDENT;
		ALT_DEBUG(0, "END AltsoundCsvParser::parse()");
		return false;
	}

	std::string line;

	// skip header row
	std::getline(file, line);

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
			AltsoundSampleInfo entry;

			// Assume the fields are in the following order:
			// ID, CHANNEL, DUCK, GAIN, LOOP, STOP, NAME, FNAME

			// ID
			if (std::getline(ss, field, ',')) {
				entry.id = std::stoul(trim(field), nullptr, 16);
			}
			else {
				ALT_ERROR(0, "Failed to parse sample ID value");
				success = false;
				entry.id = 0;  // assign some default value
				break;
			}

			// CHANNEL
			if (std::getline(ss, field, ',')) {
				std::string trimmed = trim(field);

				if (trimmed.empty()) {
					entry.channel = -1;
				}
				else {
					int val = std::stoi(trimmed);
					if (val == 0 || val == 1 || val == -1) {
						entry.channel = val;
					}
					else {
						ALT_WARNING(1, "Invalid sample CHANNEL value: %d", val);
						entry.channel = -1;  // assign some default value
					}
				}
			}
			else {
				ALT_ERROR(0, "Failed to parse sample CHANNEL value");
				success = false;
				entry.channel = -1;  // assign some default value
				break;
			}

			// DUCK
			if (std::getline(ss, field, ',')) {
				float val = std::stof(trim(field));
				entry.ducking = entry.channel == 0 ? 100.0f : val < 0.0f ? -1.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}
			else {
				ALT_ERROR(0, "Failed to parse sample DUCK value");
				success = false;
				entry.ducking = 0.0f;  // assign some default value
				break;
			}

			// GAIN
			if (std::getline(ss, field, ',')) {
				float val = std::stof(trim(field));
				entry.gain = val < 0.0f ? 0.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}
			else {
				ALT_ERROR(0, "Failed to parse sample GAIN value");
				success = false;
				entry.gain = 0.0f;  // assign some default value
				break;
			}

			// LOOP
			if (std::getline(ss, field, ',')) {
				entry.loop = std::stoul(trim(field)) == 100;
			}
			else {
				ALT_ERROR(0, "Failed to parse sample LOOP value");
				success = false;
				entry.loop = false;  // assign some default value
				break;
			}

			// STOP
			if (std::getline(ss, field, ',')) {
				entry.stop = std::stoul(trim(field)) == 1;
			}
			else {
				ALT_ERROR(0, "Failed to parse sample STOP value");
				success = false;
				entry.stop = false;  // assign some default value
				break;
			}

			// NAME
			if (std::getline(ss, field, ',')) {
				entry.name = toLowerCase(trim(field));
			}
			else {
				success = false;
				entry.name.clear();  // assign some default value
			}

			// FNAME
			if (std::getline(ss, field, ',')) {
				field = trim(field);

				if (field.empty()) {
					ALT_ERROR(1, "Sample filename is blank");
					success = false;
					entry.fname.clear();  // assign some default value
					break;
				}

				std::string full_path = altsound_path + '/' + field;

				// Normalize to forward slashes
				std::replace(full_path.begin(), full_path.end(), '\\', '/');
				entry.fname = full_path;
			}
			else {
				ALT_ERROR(1, "Failed to parse FNAME");
				success = false;
				entry.fname.clear();  // assign some default value
				break;
			}

			samples_out.emplace_back(entry);

			std::ostringstream debug_stream;
			debug_stream << "ID = 0x" << std::setfill('0') << std::setw(4) << std::hex << entry.id << std::dec
						 << ", CHANNEL = " << entry.channel
						 << ", DUCKING = " << std::fixed << std::setprecision(2) << entry.ducking
						 << ", GAIN = " << std::fixed << std::setprecision(2) << entry.gain
						 << ", LOOP = " << entry.loop
						 << ", NAME = " << entry.name
						 << ", FNAME = " << entry.fname;

			ALT_DEBUG(0, debug_stream.str().c_str());
		}
	}
	catch (std::exception& e) { // catch by reference
		ALT_ERROR(0, "AltsoundCsvParser::parse(): %s", e.what());
		success = false;
	}

	OUTDENT;
	ALT_DEBUG(0, "END AltsoundCsvParser::parse()");
	return success;
}
