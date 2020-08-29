#ifndef intf5220_h
#define intf5220_h
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

struct TMS5220interface
{
	double baseclock;			/* clock rate = 80 * output sample rate,     */
								/* usually 640000 for 8000 Hz sample rate or */
								/* usually 800000 for 10000 Hz sample rate.  */
	int mixing_level;
	void (*irq)(int state);		/* IRQ callback function */
	void (*ready)(int state);		/* ready callback function */

	int (*read)(int count);			/* speech ROM read callback */
	void (*load_address)(int data);	/* speech ROM load address callback */
	void (*read_and_branch)(void);	/* speech ROM read and branch callback */
};

int tms5220_sh_start(const struct MachineSound *msound);
void tms5220_sh_stop(void);
void tms5220_sh_update(void);

WRITE_HANDLER( tms5220_data_w );
READ_HANDLER( tms5220_status_r );
int tms5220_ready_r(void);
double tms5220_time_to_ready(void);
int tms5220_int_r(void);

void tms5220_reset(void);
void tms5220_set_frequency(double frequency);

#ifdef PINMAME
void tms5220_set_reverb_filter(float delay, float force);
#endif

#endif

