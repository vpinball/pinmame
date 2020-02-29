#include "driver.h"
#include <sys/time.h>

inline cycles_t osd_cycles(void) {
 	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	
	return ((unsigned long long)current_time.tv_sec * 1000LL + (current_time.tv_usec / 1000LL));
}


inline cycles_t osd_cycles_per_second(void) {
	return 1000;
}

inline cycles_t osd_profiling_ticks(void) {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	
	return (((unsigned long long)current_time.tv_sec * 1000000LL + current_time.tv_usec));
}
