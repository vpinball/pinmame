/*********************************************************************

  cheat.h

*********************************************************************/

#pragma once

extern int he_did_cheat;

void InitCheat(void);
void StopCheat(void);

int cheat_menu(struct mame_bitmap *bitmap, int selection);
void DoCheat(struct mame_bitmap *bitmap);

void DisplayWatches(struct mame_bitmap * bitmap);
