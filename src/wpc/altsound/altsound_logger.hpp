// ---------------------------------------------------------------------------
// altsound_logger.hpp
//
// Runtime and debug logger for AltSound
// ---------------------------------------------------------------------------
// license:BSD-3-Clause
// copyright-holders: Dave Roscoe
// ---------------------------------------------------------------------------

#ifndef ALTSOUND_LOGGER_H
#define ALTSOUND_LOGGER_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#if _MSC_VER >= 1700
 #ifdef inline
  #undef inline
 #endif
#endif

// Std Library includes
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

// convenience macros
#define ALT_INFO(indent, msg, ...) alog.info(indent, msg, ##__VA_ARGS__)
#define ALT_ERROR(indent, msg, ...) alog.error(indent, msg, ##__VA_ARGS__)
#define ALT_WARNING(indent, msg, ...) alog.warning(indent, msg, ##__VA_ARGS__)
#define ALT_DEBUG(indent, msg, ...) alog.debug(indent, msg, ##__VA_ARGS__)
//#define INDENT alog.indent()
#define INDENT
//#define OUTDENT alog.outdent()
#define OUTDENT
#define ALT_CALL(func) ([&]() { alog.indent(); auto ret = func; alog.outdent(); return ret; }())
#define ALT_RETURN(retval) do { alog.outdent(); return retval; } while (0)

class AltsoundLogger
{
public:
	enum Level {
		None = 0,
		Info,
		Error,
		Warning,
		Debug,
		UNDEFINED
	};

	explicit AltsoundLogger(const std::string& filename);

	~AltsoundLogger() = default;

	// DAR@20230706
	// Because these are variadic template functions, their definitions must
	// remain in the header
	//
	// Log INFO level messages
	template<typename... Args>
	void info(int rel_indent, const char* format, Args... args)
	{
		if (log_level >= Level::Info) {
			log(base_indent + rel_indent, Level::Info, format, args...);
		}
	}

	// Log ERROR level messages
	template<typename... Args>
	void error(int rel_indent, const char* format, Args... args)
	{
		if (log_level >= Level::Error) {
			log(base_indent + rel_indent, Level::Error, format, args...);
		}
	}

	// Log WARNING level messages
	template<typename... Args>
	void warning(int rel_indent, const char* format, Args... args)
	{
		if (log_level >= Level::Warning) {
			log(base_indent + rel_indent, Level::Warning, format, args...);
		}
	}

	// Log DEBUG level messages
	template<typename... Args>
	void debug(int rel_indent, const char* format, Args... args)
	{
		if (log_level >= Level::Debug) {
			log(base_indent + rel_indent, Level::Debug, format, args...);
		}
	}

	void setLogLevel(Level level);

	// increase base indent
	static void indent();

	// decrease base indent
	static void outdent();

	// convert string to Level enum value
	Level toLogLevel(const std::string& lvl_in);

private:  // methods

	// DAR@20230706
	// Because this is a variadic template function, it must remain in the header
	//
	// main logging method
	template<typename... Args>
	void log(int indentLevel, Level lvl, const char* format, Args... args)
	{
		char buffer[1024];
		std::snprintf(buffer, sizeof(buffer), format, args...);
		std::stringstream message;
		message << std::string(indentLevel * indentWidth, ' ')
			<< toString(lvl) << ": " << buffer << "\n";
		std::string finalMessage = message.str();

		if (out.is_open()) {
			out << finalMessage;
			out.flush();
		}
		std::cout << finalMessage;
	}

	// DAR@20230706
	// This is only used for logging message within the logger class
	//
	// Log DEBUG level messages
	template<typename... Args>
	void none(int rel_indent, const char* format, Args... args)
	{
		if (log_level >= Level::None) {
			log(base_indent + rel_indent, Level::None, format, args...);
		}
	}

	// convert Level enum value to a string
	const char* toString(Level lvl);

private: // data

	// Thread-local storage for the base indentation level
	static thread_local int base_indent;
	Level log_level;
	static constexpr int indentWidth = 4;
	std::ofstream out;
};

// ----------------------------------------------------------------------------
// Inline methods
// ----------------------------------------------------------------------------

inline void AltsoundLogger::setLogLevel(Level level)
{
	log_level = level;
	none(0, "New log level set: %s", toString(log_level));
}

// ----------------------------------------------------------------------------

inline void AltsoundLogger::indent()
{
	++base_indent;
}

// ----------------------------------------------------------------------------

inline void AltsoundLogger::outdent()
{
	if (base_indent > 0) {
		--base_indent;
	}
}

#endif //ALTSOUND_LOGGER_H
