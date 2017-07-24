/*********************************************************************

  cheat.h

*********************************************************************/

#ifndef CHEAT_H
#define CHEAT_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

extern int he_did_cheat;

void InitCheat(void);
void StopCheat(void);

int cheat_menu(struct mame_bitmap *bitmap, int selection);
void DoCheat(struct mame_bitmap *bitmap);

void DisplayWatches(struct mame_bitmap * bitmap);

#endif
