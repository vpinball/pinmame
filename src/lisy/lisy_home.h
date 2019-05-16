#ifndef LISY_HOME_H
#define LISY_HOME_H

int lisy_home_init_event(void);
void lisy_home_event_handler( int id, int arg1, int arg2, char *str);

//the IDs for the event handler
#define LISY_HOME_EVENT_INIT 0
#define LISY_HOME_EVENT_SOUND 1
#define LISY_HOME_EVENT_SOLENOID 2
#define LISY_HOME_EVENT_SWITCH 3
#define LISY_HOME_EVENT_LAMP 4
#define LISY_HOME_EVENT_DISPLAY 5

#define LISY_HOME_HW_ID 21

#endif  /* LISY_HOME_H */

