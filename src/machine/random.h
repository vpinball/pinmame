/***********************************************************

  random.h

  Mame's own random number generator.
  For now, this is the Mersenne Twister.

***********************************************************/

#ifndef MAME_RAND_H
#define MAME_RAND_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* initializes random number generator with a seed */
void mame_srand(unsigned long s);

/* generates a random number on [0,0xffffffff]-interval */
unsigned long mame_rand(void);

#ifdef __cplusplus
}
#endif

#endif
