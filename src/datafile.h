#ifndef DATAFILE_H
#define DATAFILE_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct tDatafileIndex
{
	UINT64 offset;
	const struct GameDriver *driver;
};

extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);

#endif
