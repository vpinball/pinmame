#ifndef tms5220_h
#define tms5220_h
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void tms5220_reset(void);
void tms5220_set_irq(void (*func)(int));
void tms5220_set_ready(void (*func)(int));

void tms5220_data_write(int data);
int tms5220_status_read(void);
int tms5220_ready_read(void);
int tms5220_cycles_to_ready(void);
int tms5220_int_read(void);

void tms5220_process(INT16 *buffer, unsigned int size);

/* three variables added by R Nabet */
void tms5220_set_read(int (*func)(int));
void tms5220_set_load_address(void (*func)(int));
void tms5220_set_read_and_branch(void (*func)(void));

/* Variants */

#define TMS5220_IS_5220C	(4)
#define TMS5220_IS_5200		(5)
#define TMS5220_IS_5220		(6)
#define TMS5220_IS_TMC0285	TMS5220_IS_5200

void tms5220_set_variant(int new_variant);

#endif
