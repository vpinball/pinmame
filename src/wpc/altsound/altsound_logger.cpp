// ---------------------------------------------------------------------------
// altsound_logger.cpp
//
// Runtime and debug logger for AltSound
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#include "altsound_logger.hpp"

#include <algorithm>
#include <map>
#include <string>

// DAR@20230706
// This must be done here and only here to avoid breaking the 
// "One Definition Rule".  thread_local variables are tied to
// the compilation unit they are declared in
//
thread_local int AltsoundLogger::base_indent = 0;

// ----------------------------------------------------------------------------
// CTOR/DTOR
// ----------------------------------------------------------------------------

AltsoundLogger::AltsoundLogger(const std::string& filename)
: log_level(Debug), out(filename)
{
}

// ----------------------------------------------------------------------------
// Helper method to convert string representation of Level enum to enum value
// ----------------------------------------------------------------------------

AltsoundLogger::Level AltsoundLogger::toLogLevel(const std::string& lvl_in)
{
	static const std::map<std::string, Level> typeMap{
		{"NONE"     , Level::None},
		{"INFO"     , Level::Info},
		{"ERROR"    , Level::Error},
		{"WARNING"  , Level::Warning},
		{"DEBUG"    , Level::Debug},
		{"UNDEFINED", Level::UNDEFINED}
	};

	std::string str = lvl_in;
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);

	const auto it = typeMap.find(str);
	if (it != typeMap.end()) {
		return it->second;
	}
	else {
		return Level::UNDEFINED;
	}
}

// ----------------------------------------------------------------------------
// Helper method to convert Level enum value to a string
// ----------------------------------------------------------------------------

const char* AltsoundLogger::toString(Level lvl)
{
	switch (lvl) {
	case None    : return "NONE";
	case Error   : return "ERROR";
	case Warning : return "WARNING";
	case Info    : return "INFO";
	case Debug   : return "DEBUG";
	default      : return "UNDEFINED";
	}
}
