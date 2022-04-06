#ifndef WHEELS_H
#define WHEELS_H

//starship wheel scores
void wheel_score_reset(void);
void wheel_score_credits_reset(void);
void wheel_score( int display, char *data);
void wheel_pulse( int wheel );
void wheels_show_int( int display, int digit, unsigned char dat);
void wheels_refresh(void);

#endif  /* WHEELS_H */
