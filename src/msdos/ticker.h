#include "osd_cpu.h"


typedef INT64 TICKER;

extern TICKER ticks_per_sec;

#define TICKS_PER_SEC ticks_per_sec

void init_ticker(void);
TICKER ticker(void);
