#pragma once

struct tDatafileIndex
{
	UINT64 offset;
	const struct GameDriver *driver;
};

extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);
