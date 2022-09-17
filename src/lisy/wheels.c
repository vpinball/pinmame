// license:BSD-3-Clause

/*
 wheels.c
 part of lisy_home for Starship
 March 2022
 bontango
*/

#include "coils.h"
#include "displays.h"
#include "eeprom.h"
#include "externals.h"
#include "fadecandy.h"
#include "fileio.h"
#include "hw_lib.h"
#include "lisy.h"
#include "lisy35.h"
#include "lisy_home.h"
#include "sound.h"
#include "switches.h"
#include "utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>

extern char debugbuf[256]; // see hw_lib.c

//for Starship Switches
extern unsigned char swMatrixLISY35[9];

//from lisy_home.c
extern unsigned char lisy35_flipper_disable_status;
extern unsigned char lisy35_mom_solenoid_status_safe;

//internal to wheels
int oldpos[2][5];
int oldpos_credit[2] = {0, 0};
#define WHEEL_STATE_OFF   0
#define WHEEL_STATE_ON    1
#define WHEEL_STATE_DELAY 2
int wheel_pulses_needed[2][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
int wheel_state[2][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
int wheel_pulses_credits_needed = 0;
int wheel_credits_state = 0;
int wheel_score_credits_reset_done = 0;
int wheel_score_reset_done = 0;

/*
coil and switches
5 - 41;Display 1;Pos0  100K;
6 - 42;Display 1;Pos0  10K;
7 - 43;Display 1;Pos0  10K;
8 - 44;Display 1;Pos0  100;
9 - 45;Display 1;Pos0  10;
*/

//calculate solenoid from display digit
int
digit2sol(int display, int digit) {
    //fix mapping is
    // sol 5,6,7,8,9 for display1
    // sol 12,13,14,15,16 for display2

    return ((5 + digit) + (7 * display));
}

//this routine is called from lisy35_throttle
//in order not to block game while pulsing (slow) wheels
//RTH v1 with fix time ( expect to be called each 5 ms )
void
wheels_refresh(void) {
    int i, j, state, coil;
    static int pulse_time[2][5], pulse_time_credit;
    static int delay_time[2][5], delay_time_credit;
    static int credit_coil;

    //check status wheel
    //no activation before score reset is done
    if (wheel_score_credits_reset_done == 1) {
        switch (wheel_credits_state) {
            case WHEEL_STATE_OFF: //wheel is ready for pulse
                //only activate if state from mom solenoids is safe
                if (lisy35_mom_solenoid_status_safe != 1)
                    break;

                if (wheel_pulses_credits_needed != 0) //do we need to pulse?
                {
                    // wheel_pulse_reset(10); credit UP
                    // wheel_pulse_reset(11); credit down
                    if (wheel_pulses_credits_needed < 0) //step down?
                    {
                        if (wheel_score_reset_done == 0)
                            break; //step don only after wheel score reset
                        credit_coil = 11;
                        wheel_pulses_credits_needed += 1;
                    } else //step up
                    {
                        credit_coil = 10;
                        wheel_pulses_credits_needed -= 1;
                    }
                    lisyh_coil_set(lisy_home_ss_special_coil_map[credit_coil].mapped_to_coil, 1);
                    pulse_time_credit = lisy_home_ss_special_coil_map[credit_coil].pulsetime;
                    delay_time_credit = lisy_home_ss_special_coil_map[credit_coil].delay;
                    wheel_credits_state = WHEEL_STATE_ON;
                }
                break;
            case WHEEL_STATE_ON:            //wheel is active, count down pulstime
                pulse_time_credit -= 5;     //5ms per call
                if (pulse_time_credit <= 0) //pulse time expired?
                {
                    //yes deactivate sol and change state to DELAY
                    lisyh_coil_set(lisy_home_ss_special_coil_map[credit_coil].mapped_to_coil, 0);
                    wheel_credits_state = WHEEL_STATE_DELAY;
                }
                break;

            case WHEEL_STATE_DELAY:         //wheel inactive but in delay state ( we need to prevent too fast pulsing)
                delay_time_credit -= 5;     //5ms per call
                if (delay_time_credit <= 0) //delay_time time expired?
                {
                    //yes change state to DELAY
                    wheel_credits_state = WHEEL_STATE_OFF;
                }
                break;

        } //state
    }     //reset done

    //check all 10 wheels
    for (i = 0; i <= 1; i++) {
        for (j = 0; j <= 4; j++) {

            //state of current digit
            switch (wheel_state[i][j]) {
                case WHEEL_STATE_OFF: //wheel is ready for pulse
                    //only activate if state from mom solenoids is safe
                    if (lisy35_mom_solenoid_status_safe != 1)
                        break;

                    //complete round is not needed
                    while (wheel_pulses_needed[i][j] >= 10) {
                        wheel_pulses_needed[i][j] -= 10;
                    }

                    if (wheel_pulses_needed[i][j] > 0) //do we need to pulse?
                    {
                        //yes store pulse and delay time for this digit
                        //decrement puls couinter and change state to ON
                        wheel_pulses_needed[i][j] -= 1;
                        coil = digit2sol(i, j);
                        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
                        pulse_time[i][j] = lisy_home_ss_special_coil_map[coil].pulsetime;
                        delay_time[i][j] = lisy_home_ss_special_coil_map[coil].delay;
                        wheel_state[i][j] = WHEEL_STATE_ON;
                    }
                    break;

                case WHEEL_STATE_ON:                         //wheel is active, count down pulstime
                    pulse_time[i][j] = pulse_time[i][j] - 5; //5ms per call
                    if (pulse_time[i][j] <= 0)               //pulse time expired?
                    {
                        //yes deactivate sol and change state to DELAY
                        coil = digit2sol(i, j);
                        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
                        wheel_state[i][j] = WHEEL_STATE_DELAY;
                    }
                    break;

                case WHEEL_STATE_DELAY: //wheel inactive but in delay state ( we need to prevent too fast pulsing)
                    delay_time[i][j] = delay_time[i][j] - 5; //5ms per call
                    if (delay_time[i][j] <= 0)               //delay_time time expired?
                    {
                        //yes change state to DELAY
                        wheel_state[i][j] = WHEEL_STATE_OFF;
                    }
                    break;

            } //state
        }     //j
    }         //i
}

/* wheel_pulse_reset
        coil -> nr of coil
        this routine respect pulsetime and mapping
	delay is respected by reset routine
        from config file LisyH (Starship)
*/
void
wheel_pulse_reset(int coil) {
    if (lisy_home_ss_special_coil_map[coil].mapped_to_coil != 0) {
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
        delay(lisy_home_ss_special_coil_map[coil].pulsetime); // milliseconds delay from wiringpi library
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
        //delay (lisy_home_ss_special_coil_map[coil].delay); // milliseconds delay from wiringpi library
    }
}

//same with delay
void
wheel_pulse_reset_wdelay(int coil) {
    if (lisy_home_ss_special_coil_map[coil].mapped_to_coil != 0) {
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
        delay(lisy_home_ss_special_coil_map[coil].pulsetime); // milliseconds delay from wiringpi library
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
        delay(lisy_home_ss_special_coil_map[coil].delay); // milliseconds delay from wiringpi library
    }
}

/* wheel_pulse
	coil -> nr of coil
	this routine respect pulsetime, mapping and delay
	from config file LisyH (Starship)
*/
void
wheel_pulse(int coil) {
    if (lisy_home_ss_special_coil_map[coil].mapped_to_coil != 0) {
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
        delay(lisy_home_ss_special_coil_map[coil].pulsetime); // milliseconds delay from wiringpi library
        lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
        delay(lisy_home_ss_special_coil_map[coil].delay); // milliseconds delay from wiringpi library
    }
}

//set credit wheels to 'zero' position
void
wheel_score_credits_reset(void) {
    int i;

    //only do it once
    if (wheel_score_credits_reset_done == 1) {
        if (ls80dbg.bitv.coils) {
            sprintf(debugbuf, "Wheels: CREDIT wheels already reset at Boot, ignored");
            lisy80_debug(debugbuf);
        }

        return;
    }

    if (ls80dbg.bitv.coils) {
        sprintf(debugbuf, "Wheels: set CREDIT wheels to zero Boot");
        lisy80_debug(debugbuf);
    }

    //reset credit display
    // swMatrixLISY35[7],4) SW#53 credit >0
    // swMatrixLISY35[7],5) SW#54 credit <=24
    // wheel_pulse_reset(10); credit UP
    // wheel_pulse_reset(11); credit down

    //maximum 25 steps
    for (i = 1; i <= 26; i++) {
        lisy35_switchmatrix_update(20); //update internal matrix to detect zero switch
        //pulse down until '0' switch is open
        if (CHECK_BIT(swMatrixLISY35[7], 4))
            wheel_pulse_reset_wdelay(11);
        else
            break;
    }

    //reset postion as well
    //for(i=0; i<2; i++) oldpos_credit[i] = 0;

    //set flag
    wheel_score_credits_reset_done = 1;

    if (ls80dbg.bitv.coils) {
        sprintf(debugbuf, "Wheels: set CREDIT wheels to zero finished");
        lisy80_debug(debugbuf);
    }
}

//set all score wheels to 'zero' position
//new version
void
wheel_score_reset(void) {
    int wheel_is_zero[2][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
    int i, j, coil, zero_count;
    int tries = 0;

    //update zero position at start of routine
    lisy35_switchmatrix_update(1);
    if (CHECK_BIT(swMatrixLISY35[6], 0) == 0)
        wheel_is_zero[0][0] = 1;
    if (CHECK_BIT(swMatrixLISY35[6], 1) == 0)
        wheel_is_zero[0][1] = 1;
    if (CHECK_BIT(swMatrixLISY35[6], 2) == 0)
        wheel_is_zero[0][2] = 1;
    if (CHECK_BIT(swMatrixLISY35[6], 3) == 0)
        wheel_is_zero[0][3] = 1;
    if (CHECK_BIT(swMatrixLISY35[6], 4) == 0)
        wheel_is_zero[0][4] = 1;
    if (CHECK_BIT(swMatrixLISY35[8], 0) == 0)
        wheel_is_zero[1][0] = 1;
    if (CHECK_BIT(swMatrixLISY35[8], 1) == 0)
        wheel_is_zero[1][1] = 1;
    if (CHECK_BIT(swMatrixLISY35[8], 2) == 0)
        wheel_is_zero[1][2] = 1;
    if (CHECK_BIT(swMatrixLISY35[8], 3) == 0)
        wheel_is_zero[1][3] = 1;
    if (CHECK_BIT(swMatrixLISY35[8], 4) == 0)
        wheel_is_zero[1][4] = 1;
    zero_count = 0;
    for (i = 0; i <= 1; i++) {
        for (j = 0; j <= 4; j++) {
            zero_count += wheel_is_zero[i][j];
        }
    }

    while (zero_count != 10) {
        //limit number of tries
        if (tries++ > 15) {
            fprintf(stderr, "maximum tries(15) for wheel zero exceeded");
            for (i = 0; i <= 1; i++) {
                for (j = 0; j <= 4; j++) {
                    if (wheel_is_zero[i][j] == 0)
                        printf("wheel %d %d is not zero\n", i, j);
                }
            }
            break;
        }

        //do all 10 wheels
        for (i = 0; i <= 1; i++) {
            for (j = 0; j <= 4; j++) {

                //wheel not zero, need to pulse
                //fix pulse 60ms, delay 130ms + checktime
                if (wheel_is_zero[i][j] == 0) {
                    //change state to ON
                    coil = digit2sol(i, j);
                    lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
                }
            } //j
        }     // i

        //delay on time 60ms
        delay(60);

        //do all 10 wheels
        for (i = 0; i <= 1; i++) {
            for (j = 0; j <= 4; j++) {

                //wheel not zero, need to pulse
                //fix pulse 60ms, delay 130ms + checktime
                if (wheel_is_zero[i][j] == 0) {
                    //change state to OFF
                    coil = digit2sol(i, j);
                    lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
                }
            } //j
        }     // i

        // 130 milliseconds delay
        delay(130);

        //now check
        lisy35_switchmatrix_update(1);
        if (CHECK_BIT(swMatrixLISY35[6], 0) == 0)
            wheel_is_zero[0][0] = 1;
        if (CHECK_BIT(swMatrixLISY35[6], 1) == 0)
            wheel_is_zero[0][1] = 1;
        if (CHECK_BIT(swMatrixLISY35[6], 2) == 0)
            wheel_is_zero[0][2] = 1;
        if (CHECK_BIT(swMatrixLISY35[6], 3) == 0)
            wheel_is_zero[0][3] = 1;
        if (CHECK_BIT(swMatrixLISY35[6], 4) == 0)
            wheel_is_zero[0][4] = 1;
        if (CHECK_BIT(swMatrixLISY35[8], 0) == 0)
            wheel_is_zero[1][0] = 1;
        if (CHECK_BIT(swMatrixLISY35[8], 1) == 0)
            wheel_is_zero[1][1] = 1;
        if (CHECK_BIT(swMatrixLISY35[8], 2) == 0)
            wheel_is_zero[1][2] = 1;
        if (CHECK_BIT(swMatrixLISY35[8], 3) == 0)
            wheel_is_zero[1][3] = 1;
        if (CHECK_BIT(swMatrixLISY35[8], 4) == 0)
            wheel_is_zero[1][4] = 1;
        zero_count = 0;
        for (i = 0; i <= 1; i++) {
            for (j = 0; j <= 4; j++) {
                zero_count += wheel_is_zero[i][j];
            }
        }

    } //while not finished

    //reset postion as well
    for (i = 0; i < 5; i++)
        oldpos[0][i] = 0;
    for (i = 0; i < 5; i++)
        oldpos[1][i] = 0;

    //set flag
    wheel_score_reset_done = 1;

    if (ls80dbg.bitv.coils) {
        sprintf(debugbuf, "Wheels: set wheels to zero finished");
        lisy80_debug(debugbuf);
    }
}

void
wheels_show_int(int display, int digit, unsigned char dat, unsigned char attract_mode) {

    int i;
    int pos[2][5], pulses;
    int newcredits;
    static int oldcredits = 0;

    //inform the handler
    lisy_home_ss_event_handler(LISY_HOME_SS_EVENT_DISPLAY, digit, dat, display);

    //status display
    //digt 3&4 are credits
    //digit 6 Endzahl
    //digit 7 ball in Play
    if (display == 0) {
        if (dat > 9)
            return; //ignore spaces

        if (digit == 3) //tens changed
        {
            oldpos_credit[1] = dat; //tens no update oldcredits  here
        } else if (digit == 4) {
            oldpos_credit[0] = dat; //one
            //do update
            //caculate new
            newcredits = 10 * oldpos_credit[1] + oldpos_credit[0];
            pulses = newcredits - oldcredits;
            //set local var for pulses needed
            //will becoming active with wheels_refresh via lisy35_throtle
            wheel_pulses_credits_needed += pulses;
            if (ls80dbg.bitv.coils) {
                sprintf(debugbuf, "wheels_show_int: Status display old:%d new:%d (%d pulses needed)\n", oldcredits,
                        newcredits, pulses);
                lisy80_debug(debugbuf);
            }
            //store for next update
            oldcredits = newcredits;
        }

        return;
    } //status display

    //ignore 'spaces' , display >1 and digit >6
    if (dat > 9)
        return;
    if (display > 2)
        return;
    if (digit > 6)
        return;

    //flipper enabled or atract mode(hstd)?
    if ((lisy35_flipper_disable_status == 0) | (attract_mode == 1)) {
        //adjust numbers
        display--;
        digit--; //digit = digit -2
        digit--;
        //assign position
        pos[display][digit] = dat;
        //calculate pulses
        pulses = oldpos[display][digit] - pos[display][digit];
        if (pulses > 0)
            pulses = abs(10 - pulses);
        if (pulses < 0)
            pulses = abs(pulses);

        if (ls80dbg.bitv.displays) {
            sprintf(debugbuf, "wheels_show_int: display:%d digit:%d dat:%d (old dat%d   %d pulses needed)\n", display,
                    digit, dat, oldpos[display][digit], pulses);
            lisy80_debug(debugbuf);
        }

        //store new value
        oldpos[display][digit] = pos[display][digit];

        //set local var for pulses needed
        //will becoming active with wheels_refresh via lisy35_throtle
        wheel_pulses_needed[display][digit] += pulses;

    } //flipper enabled?
}

//************************************
//called from lisy200_control only
//************************************
void
wheel_score(int display, char* data) {
    int i, k;
    int pos[2][5], pulses[2][5];
    static unsigned char first = 1;

    // -48 to get an int out of the ascii code ( 0..9)
    for (i = 0; i < 5; i++)
        pos[display - 1][i] = data[i] - 48;

    //first call set to zero
    if (first) {
        if (ls80dbg.bitv.displays) {
            sprintf(debugbuf, "Wheels: first call, set wheels to zero");
            lisy80_debug(debugbuf);
        }
        first = 0;
        wheel_score_reset();
        if (ls80dbg.bitv.displays) {
            sprintf(debugbuf, "Wheels: set to zero done");
            lisy80_debug(debugbuf);
        }
    }

    if (ls80dbg.bitv.displays) {
        sprintf(debugbuf, "set display %d to %d%d%d%d%d", display, pos[display - 1][0], pos[display - 1][1],
                pos[display - 1][2], pos[display - 1][3], pos[display - 1][4]);
        lisy80_debug(debugbuf);
    }

    //calculate number of pulses needed
    for (i = 0; i < 5; i++) {
        pulses[0][i] = oldpos[0][i] - pos[0][i];
        if (pulses[0][i] > 0)
            pulses[0][i] = abs(10 - pulses[0][i]);
        if (pulses[0][i] < 0)
            pulses[0][i] = abs(pulses[0][i]);
        //store new value
        oldpos[0][i] = pos[0][i];
    }
    for (i = 0; i < 5; i++) {
        pulses[1][i] = oldpos[1][i] - pos[1][i];
        if (pulses[1][i] > 0)
            pulses[1][i] = abs(10 - pulses[1][i]);
        if (pulses[1][i] < 0)
            pulses[1][i] = abs(pulses[1][i]);
        //store new value
        oldpos[1][i] = pos[1][i];
    }

    //display2
    while (pulses[0][0]-- > 0)
        wheel_pulse(5);
    while (pulses[0][1]-- > 0)
        wheel_pulse(6);
    while (pulses[0][2]-- > 0)
        wheel_pulse(7);
    while (pulses[0][3]-- > 0)
        wheel_pulse(8);
    while (pulses[0][4]-- > 0)
        wheel_pulse(9);
    //display2
    while (pulses[1][0]-- > 0)
        wheel_pulse(12);
    while (pulses[1][1]-- > 0)
        wheel_pulse(13);
    while (pulses[1][2]-- > 0)
        wheel_pulse(14);
    while (pulses[1][3]-- > 0)
        wheel_pulse(15);
    while (pulses[1][4]-- > 0)
        wheel_pulse(16);
}

//set the wheel for HSTD
//RTH OLD
void
wheel_hstd_set(int w_pulses_needed[][5]) {
    int wheel_state[2][5] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
    int i, j, coil, not_finished;
    int pulse_time[2][5], pulse_time_credit;
    int delay_time[2][5], delay_time_credit;

    do {
        //check all 10 wheels
        for (i = 0; i <= 1; i++) {
            for (j = 0; j <= 4; j++) {

                //state of current digit
                switch (wheel_state[i][j]) {
                    case WHEEL_STATE_OFF: //wheel is ready for pulse

                        if (w_pulses_needed[i][j] > 0) //do we need to pulse?
                        {
                            //yes store pulse and delay time for this digit
                            //decrement puls couinter and change state to ON
                            w_pulses_needed[i][j] -= 1;
                            coil = digit2sol(i, j);
                            lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 1);
                            pulse_time[i][j] = lisy_home_ss_special_coil_map[coil].pulsetime;
                            delay_time[i][j] = lisy_home_ss_special_coil_map[coil].delay;
                            wheel_state[i][j] = WHEEL_STATE_ON;
                        }
                        break;

                    case WHEEL_STATE_ON:                         //wheel is active, count down pulstime
                        pulse_time[i][j] = pulse_time[i][j] - 5; //5ms per call
                        if (pulse_time[i][j] <= 0)               //pulse time expired?
                        {
                            //yes deactivate sol and change state to DELAY
                            coil = digit2sol(i, j);
                            lisyh_coil_set(lisy_home_ss_special_coil_map[coil].mapped_to_coil, 0);
                            wheel_state[i][j] = WHEEL_STATE_DELAY;
                        }
                        break;

                    case WHEEL_STATE_DELAY: //wheel inactive but in delay state ( we need to prevent too fast pulsing)
                        delay_time[i][j] = delay_time[i][j] - 5; //5ms per call
                        if (delay_time[i][j] <= 0)               //delay_time time expired?
                        {
                            //yes change state to OFF
                            wheel_state[i][j] = WHEEL_STATE_OFF;
                        }
                        break;

                } //state
            }     //j
        }         //i

        delay(5); // 5 milliseconds

        //calculate end condition
        //we are finished if all wheels are off
        //and all pulses are done
        not_finished = 0;
        for (i = 0; i <= 1; i++) {
            for (j = 0; j <= 4; j++) {

                if (wheel_state[i][j] != WHEEL_STATE_OFF)
                    not_finished++;
                not_finished += w_pulses_needed[i][j];
            }
        }

    } while (not_finished);
}

//high score today with wheels
void
wheel_set_hstd(unsigned char hstd[6]) {
    int i;
    unsigned char ss_hstd_disp[2][5];

    //we need the first nibble
    //all counts >9 are interpreted as '0'
    //rearrange position as it comes little endian
    for (i = 0; i <= 4; i++) {
        ss_hstd_disp[0][i] = hstd[5 - i] >> 4;
        if (ss_hstd_disp[0][i] > 9)
            ss_hstd_disp[0][i] = 0;
        ss_hstd_disp[1][i] = hstd[5 - i] >> 4;
        if (ss_hstd_disp[1][i] > 9)
            ss_hstd_disp[1][i] = 0;
        //show with show_int routine attract_mode = 1 ( display+1 digit+2)
        wheels_show_int(1, i + 2, ss_hstd_disp[0][i], 1);
        wheels_show_int(2, i + 2, ss_hstd_disp[1][i], 1);
    }
}

void
wheel_set_score(unsigned char player1[6], unsigned char player2[6]) {
    int i;
    unsigned char ss_game_disp[2][5];

    //we need the first nibble
    //all counts >9 are interpreted as '0'
    //rearrange position as it comes little endian
    for (i = 0; i <= 4; i++) {
        ss_game_disp[0][i] = player1[5 - i] >> 4;
        if (ss_game_disp[0][i] > 9)
            ss_game_disp[0][i] = 0;
        ss_game_disp[1][i] = player2[5 - i] >> 4;
        if (ss_game_disp[1][i] > 9)
            ss_game_disp[1][i] = 0;
        //show with show_int routine attract_mode = 1 ( display+1 digit+2)
        wheels_show_int(1, i + 2, ss_game_disp[0][i], 1);
        wheels_show_int(2, i + 2, ss_game_disp[1][i], 1);
    }
}

//high score today with wheels
//RTH OLD
void
wheel_hstd(unsigned char hstd[6], unsigned char player1[6], unsigned char player2[6], int sleeptime) {
    int i, j;
    int w_pulses_needed[2][5];
    unsigned char ss_hstd_disp[2][5];
    unsigned char ss_game_disp[2][5];

    //we need the first nibble
    //all counts >9 are interpreted as '0'
    //rearrange position as it comes little endian
    for (i = 0; i <= 4; i++) {
        ss_hstd_disp[0][i] = hstd[5 - i] >> 4;
        if (ss_hstd_disp[0][i] > 9)
            ss_hstd_disp[0][i] = 0;
        ss_hstd_disp[1][i] = hstd[5 - i] >> 4;
        if (ss_hstd_disp[1][i] > 9)
            ss_hstd_disp[1][i] = 0;
        ss_game_disp[0][i] = player1[5 - i] >> 4;
        if (ss_game_disp[0][i] > 9)
            ss_game_disp[0][i] = 0;
        ss_game_disp[1][i] = player2[5 - i] >> 4;
        if (ss_game_disp[1][i] > 9)
            ss_game_disp[1][i] = 0;
    }

    //set to zero
    wheel_score_reset();

    //calculate pulses needed from zero hstd
    for (i = 0; i <= 1; i++) {
        for (j = 0; j <= 4; j++) {
            w_pulses_needed[i][j] = ss_hstd_disp[i][j];
            //printf(" %d pulses needed\n",w_pulses_needed[i][j]);
        }
    }

    //call refresh state machine
    wheel_hstd_set(w_pulses_needed);

    //wait
    sleep(sleeptime);

    //set to zero again
    wheel_score_reset();

    //calculate pulses needed from zero to current score
    for (i = 0; i <= 1; i++) {
        for (j = 0; j <= 4; j++) {
            w_pulses_needed[i][j] = ss_game_disp[i][j];
            //printf(" %d pulses needed\n",w_pulses_needed[i][j]);
        }
    }

    //call refresh state machine
    wheel_hstd_set(w_pulses_needed);
}
