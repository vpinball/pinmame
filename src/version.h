#pragma once

#include "git_version.h"

#define VERSION_MAJOR    3 // X Digits
#define VERSION_MINOR    7 // Max 2 Digits
#define VERSION_REV      0 // Max 1 Digit

#define _STR(x)    #x
#define STR(x)     _STR(x)

#define PM_VERSION_DIGITS VERSION_MAJOR,VERSION_MINOR,VERSION_REV,GIT_REVISION
#define PM_VERSION_STRING_DIGITS STR(VERSION_MAJOR) STR(VERSION_MINOR) STR(VERSION_REV) STR(GIT_REVISION)
#define PM_VERSION_STRING_POINTS STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_REV) "." STR(GIT_REVISION)
#define PM_VERSION_STRING_COMMAS STR(VERSION_MAJOR) ", " STR(VERSION_MINOR) ", " STR(VERSION_REV) ", " STR(GIT_REVISION)
#define PM_VERSION_STRING_POINTS_FULL STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_REV) "." STR(GIT_REVISION) "." GIT_SHA

// Complete version string for log, crash handler,...
#define PM_VERSION_STRING_FULL_LITERAL "v" STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_REV) \
	" Beta (Rev. " STR(GIT_REVISION) " (" GIT_SHA "), " GET_PLATFORM_OS " " GET_PLATFORM_BITS "bits)"

#define PASTE2(a,b) a##b
#define PASTE(a,b) PASTE2(a,b)
