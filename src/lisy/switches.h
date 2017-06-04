#ifndef SWITCHES_H_
#define SWITCHES_H_

void monitor_switches(void);
//int switch_get_sw_version(void);
void lisy80_debug_switches (unsigned char data);


#define LS80SW_GET_SW_VERSION 1
#define LS80SW_GET_MATRIX 2


#endif  // SWITCHES_H_

