#pragma once

#define MAX_VOTRAXSC01 1

typedef void (*VOTRAXSC01_BUSYCALLBACK)(int);

struct VOTRAXSC01interface
{
	int num;												/* total number of chips */
	int type[MAX_VOTRAXSC01];								/* ROM version 0=SC-01 or 1=SC-01-A */
	int mixing_level[MAX_VOTRAXSC01];						/* master volume */
	int baseFrequency[MAX_VOTRAXSC01];						/* base frequency */
	VOTRAXSC01_BUSYCALLBACK BusyCallback[MAX_VOTRAXSC01];	/* callback function when busy signal changes */
};

int VOTRAXSC01_sh_start(const struct MachineSound *msound);
void VOTRAXSC01_sh_stop(void);

WRITE_HANDLER(votraxsc01_w);
READ_HANDLER(votraxsc01_status_r);

void votraxsc01_set_base_frequency(double baseFrequency); // OLD_VOTRAX
void votraxsc01_set_clock(UINT32 newfreq); // new one
void votraxsc01_set_volume(int volume);
