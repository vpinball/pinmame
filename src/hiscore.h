#ifndef HISCORE_H
#define HISCORE_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

void hs_open( const char *name );
void hs_init( void );
void hs_update( void );
void hs_close( void );

void computer_writemem_byte(int cpu, int addr, int value);
int computer_readmem_byte(int cpu, int addr);

#endif
