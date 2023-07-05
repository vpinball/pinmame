// ---------------------------------------------------------------------------
// altsound_logger.hpp
// 07/04/23 - Dave Roscoe
//
// Simple always-on file logger using printf-style formatting
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------
#ifndef ALTSOUND_LOGGER_H
#define ALTSOUND_LOGGER_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <cstdio>
#include <cstdarg>
#include <string>

class AltsoundLogger
{
public:
	explicit AltsoundLogger(const std::string& filename)
	{
		file = fopen(filename.c_str(), "w");
		if (!file) {
			// Handle error
		}
	}

	~AltsoundLogger()
	{
		fclose(file);
	}

	void log(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		va_end(args);
		fflush(file);
	}

private:
	FILE* file;
};

// Usage:
// Logger logger("logfile.txt");
// logger.log("Hello %s\n", "world");

#endif //ALTSOUND_LOGGER_H
