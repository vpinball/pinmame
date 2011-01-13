#ifndef INTCONFIG_H
#define INTCONFIG_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <stdlib.h>
#include <string.h>

#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024

#ifdef USE_LSB
#define BYTEORDER 1234
#else
#define BYTEORDER 4321
#endif

#define HAVE_MEMMOVE

#define XMLPARSEAPI(type) type

#include "expat.h"

#undef XMLPARSEAPI

#endif
