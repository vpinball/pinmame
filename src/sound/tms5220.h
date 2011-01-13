#ifndef tms5220_h
#define tms5220_h
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void tms5220_reset(void);
void tms5220_set_irq(void (*func)(int));

void tms5220_data_write(int data);
int tms5220_status_read(void);
int tms5220_ready_read(void);
int tms5220_cycles_to_ready(void);
int tms5220_int_read(void);

void tms5220_process(INT16 *buffer, unsigned int size);
INT16 lattice_filter(void *chip);
INT16 clip_and_wrap(INT16 cliptemp);

/* three variables added by R Nabet */
void tms5220_set_read(int (*func)(int));
void tms5220_set_load_address(void (*func)(int));
void tms5220_set_read_and_branch(void (*func)(void));

typedef enum
{
	variant_tms5220,
	variant_tmc0285
} tms5220_variant;

void tms5220_set_variant(tms5220_variant new_variant);

#endif


