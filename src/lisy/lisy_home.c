// license:BSD-3-Clause

/*
 LISY_HOME.c
 February 2019
 bontango
*/

#include "lisy_home.h"
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
#include "wheels.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>

extern char debugbuf[256]; // see hw_lib.c

//local vars
//our pointers to preloaded sounds
Mix_Chunk* lisy_H_sound[32];
//sound output
unsigned char lisy_home_starship_sound = LISY_HOME_SS_SOUND_PHAT;
//others
unsigned char lisy_game_running = 0; //for Starship event handler
//cmos
extern lisy35_cmos_data_t* lisy_by35_CMOS;

#define LISYH_SOUND_PATH "/boot/lisy/lisyH/sounds/"

//init
int
lisy_home_init_event(void) {

    int ret;

    //check for fadecandy config -> do the mapping
    ret = lisy_file_get_home_mappings();
    if (ret < 0) {
        fprintf(stderr, "Error: mapping for LISY Home error:%d\n", ret);
        return -1;
    }

    //RTH new, sound_init done by lisy80 with dip2 == ON

    //RTH test, play sound 7 after starting
    lisy80_play_wav(7);
    return ret;
}

//lamp and coils injkl. mapping
void
lisy_home_coil_event(int coil, int action) {

    int real_coil;
    int mycoil;
    int org_is_coil = 0; //is the org deivce a coil?
    int map_is_coil = 0; //is the lamp/coil mapped to a coil?
    //static unsigned int current_status = LISYH_EXT_CMD_FIRST_SOLBOARD; //RTH we need to add second board
    static unsigned int current_status = LISY35_EXT_CMD_AUX_BOARD_1; //RTH we need to add second board

    union two {
        unsigned char byte;

        struct {
            unsigned COIL : 6, ACTION : 1, IS_CMD : 1;
            //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } data;

    //look for possible mapping
    if (coil <= 48) //org is a lamp
    {
        real_coil = lisy_home_lamp_map[coil].mapped_to_no;
        map_is_coil = lisy_home_lamp_map[coil].mapped_is_coil;
        org_is_coil = 0;
    } else //org is coil
    {
        org_is_coil = 1;
        //determine which coil we use
        switch (coil) {
            case Q_SOL1:
                mycoil = 1;
                break;
            case Q_SOL2:
                mycoil = 2;
                break;
            case Q_SOL3:
                mycoil = 3;
                break;
            case Q_SOL4:
                mycoil = 4;
                break;
            case Q_SOL5:
                mycoil = 5;
                break;
            case Q_SOL6:
                mycoil = 6;
                break;
            case Q_SOL7:
                mycoil = 7;
                break;
            case Q_SOL8:
                mycoil = 8;
                break;
            case Q_SOL9:
                mycoil = 9;
                break;
            default:
                mycoil = 0;
                break;
        }
        real_coil = lisy_home_coil_map[mycoil].mapped_to_no;
        map_is_coil = lisy_home_coil_map[mycoil].mapped_is_coil;
    }

    if (!map_is_coil) //the mapped device is a lamp
    {
        //do we talk to the lampdriver currently?
        if (current_status != LISY35_EXT_CMD_AUX_BOARD_1) { //RTH need to be extended
            //RTH need to be lisyh_coil_select_lamp_driver();
            current_status = LISY35_EXT_CMD_AUX_BOARD_1;
        }
    } else //the mapped device is a coil
    {
        //do we talk to the solenoiddriver currently?
        if (current_status != LISY35_EXT_CMD_AUX_BOARD_1) { //RTH need to be extended
            //RTH need to be lisyh_coil_select_solenoid_driver();
            current_status = LISY35_EXT_CMD_AUX_BOARD_1;
        }
    }

    //debug
    if (ls80dbg.bitv.lamps | ls80dbg.bitv.coils) {
        sprintf(debugbuf, "LISY HOME:  map %s number:%d TO %s number:%d", org_is_coil ? "coil" : "lamp", coil,
                map_is_coil ? "coil" : "lamp", real_coil);
        lisy80_debug(debugbuf);
    }

    //now do the setting
    --real_coil; //we have only 6 bit, so we start at zero for coil 1

    // build control byte
    data.bitv.COIL = real_coil;
    data.bitv.ACTION = action;
    data.bitv.IS_CMD = 0; //this is a coil setting

    //write to PIC
    lisy80_write_byte_coil_pic(data.byte);
}

//the eventhandler
void
lisy_home_event_handler(int id, int arg1, int arg2, char* str) {

    switch (id) {
        case LISY_HOME_EVENT_INIT:
            lisy_home_init_event();
            break;
        //case LISY_HOME_EVENT_SOUND: sprintf(str_event_id,"LISY_HOME_EVENT_SOUND"); break;
        //case LISY_HOME_EVENT_COIL: sprintf(str_event_id,"LISY_HOME_EVENT_COIL"); break;
        //case LISY_HOME_EVENT_SWITCH: sprintf(str_event_id,"LISY_HOME_EVENT_SWITCH"); break;
        case LISY_HOME_EVENT_COIL:
            lisy_home_coil_event(arg1, arg2);
            break;
            //case LISY_HOME_EVENT_DISPLAY: sprintf(str_event_id,"LISY_HOME_EVENT_DISPLAY"); break;
    }
}

//send the colorcode of one lamp to PIC  (aware of mapping)
//one colorcode for all mapped leds
void
lisy_home_ss_lamp_set_colorcode(int lamp, unsigned char red, unsigned char green, unsigned char blue,
                                unsigned char white) {
    int i;

    //how many mappings?
    for (i = 0; i < lisy_home_ss_lamp_map[lamp].no_of_maps; i++) {
        if (lisy_home_ss_lamp_map[lamp].mapped_to_line[i] == 7) //coil mapping
            return;
        else if (lisy_home_ss_lamp_map[lamp].mapped_to_line[i] <= 6) //LED mapping
            lisyh_led_set_LED_color(lisy_home_ss_lamp_map[lamp].mapped_to_line[i],
                                    lisy_home_ss_lamp_map[lamp].mapped_to_led[i], red, green, blue, white);
    }
}

//do the led setting on starship
//aware of mapping
void
lisy_home_ss_lamp_set(int lamp, int action) {
    int i;

    //debug?
    if (ls80dbg.bitv.lamps) {
        sprintf(debugbuf, "lisy_home_ss_lamp_set: lamp:%d action:%d\n", lamp, action);
        lisy80_debug(debugbuf);
    } //debug

    //we may set other actions because of lamp status
    lisy_home_ss_event_handler(LISY_HOME_SS_EVENT_LAMP, lamp, action, 0);

    //how many mappings?
    for (i = 0; i < lisy_home_ss_lamp_map[lamp].no_of_maps; i++) {
        if (lisy_home_ss_lamp_map[lamp].mapped_to_line[i] == 7) //coil mapping
            lisyh_coil_set(lisy_home_ss_lamp_map[lamp].mapped_to_led[i], action);
        else if (lisy_home_ss_lamp_map[lamp].mapped_to_line[i] <= 6) //LED mapping
            lisyh_led_set(lisy_home_ss_lamp_map[lamp].mapped_to_led[i], lisy_home_ss_lamp_map[lamp].mapped_to_line[i],
                          action);
    }
}

//do the led setting on starship
//special lamps
//aware of mapping
void
lisy_home_ss_special_lamp_set(int lamp, int action) {
    int i;

    //debug?
    if (ls80dbg.bitv.lamps) {
        sprintf(debugbuf, "lisy_home_ss_special_lamp_set: lamp:%d action:%d\n", lamp, action);
        lisy80_debug(debugbuf);
    } //debug

    //how many mappings?
    for (i = 0; i < lisy_home_ss_special_lamp_map[lamp].no_of_maps; i++) {
        if (lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i] == 7) //coil mapping
            lisyh_coil_set(lisy_home_ss_special_lamp_map[lamp].mapped_to_led[i], action);
        if (lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i] <= 6) //LED mapping
            lisyh_led_set(lisy_home_ss_special_lamp_map[lamp].mapped_to_led[i],
                          lisy_home_ss_special_lamp_map[lamp].mapped_to_line[i], action);
    }
}

//map momentary solenoids to lisy home
void
lisy_home_ss_mom_coil_set(unsigned char value) {
    static unsigned char old_coil_active[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char current_coil_active[15];
    int i;

    //value is 0..14 for Solenoids 1..15, 15 for all OFF
    memset(current_coil_active, 0, sizeof(current_coil_active));
    if (value < 15)
        current_coil_active[value] = 1;

    for (i = 0; i <= 14; i++) {
        if (old_coil_active[i] != current_coil_active[i]) {
            lisyh_coil_set(lisy_home_ss_coil_map[i + 1].mapped_to_coil, current_coil_active[i]);
            old_coil_active[i] = current_coil_active[i];
        }
    }
}

//map continous solenoids to lisy home
//coils 16,17,18,19
void
lisy_home_ss_cont_coil_set(unsigned char cont_data) {
    //PB4 cont.2
    lisyh_coil_set(lisy_home_ss_coil_map[17].mapped_to_coil, !CHECK_BIT(cont_data, 0));
    //PB5 cont.4 (coin lockout)
    lisyh_coil_set(lisy_home_ss_coil_map[19].mapped_to_coil, !CHECK_BIT(cont_data, 1));
    //PB6 cont.1 (flipper enable)
    lisyh_coil_set(lisy_home_ss_coil_map[16].mapped_to_coil, !CHECK_BIT(cont_data, 2));
    //PB7 cont.3
    lisyh_coil_set(lisy_home_ss_coil_map[18].mapped_to_coil, !CHECK_BIT(cont_data, 3));
}

//send LED colorcodes  mappings to led driver
void
lisy_home_ss_send_led_colors(void) {

    int ledline, led;

    for (ledline = 0; ledline <= 4; ledline++) {
        for (led = 0; led <= 47; led++) {
            lisyh_led_set_LED_color(ledline, led, led_rgbw_color[ledline][led].red, led_rgbw_color[ledline][led].green,
                                    led_rgbw_color[ledline][led].blue, led_rgbw_color[ledline][led].white);
        }
    }
}

//vars for event handler actions
unsigned char lisy_home_ss_lamp_1canplay_status = 0;
unsigned char lisy_home_ss_lamp_2canplay_status = 0;
unsigned char lisy_home_ss_digit_ballinplay_status = 80;
unsigned char lisy_home_ss_switch_outhole_status = 0;
unsigned char lisy35_flipper_disable_status = 1;   //default flipper disbaled
unsigned char lisy35_mom_solenoid_status_safe = 1; //default mom solenoid status (safe)

void
lisy_home_ss_cont_sol_event(unsigned char cont_data) {
    lisy35_flipper_disable_status = CHECK_BIT(cont_data, 2);
}

//store status for safe lisy home solenoid activation
void
lisy_home_ss_mom_sol_event(unsigned char mom_data) {
    if (mom_data == 15)
        lisy35_mom_solenoid_status_safe = 1; //rest position is safe
    else
        lisy35_mom_solenoid_status_safe = 0;
}

//sound from internal sound card (bally generated)
void
lisy_home_ss_sound_event(unsigned char sound) {
    //play bonus sound ( fix #203 ) triggered with one of the org sounds
    if ((lisy_env.has_own_sounds) & (sound == 8) & (lisy_home_starship_sound == LISY_HOME_SS_SOUND_PHAT)) {
        if (lisy_home_ss_switch_outhole_status == 1)
            StarShip_play_wav(203);
    }
    //play org sounds mapped to lisy_home coils
    if (lisy_home_starship_sound == LISY_HOME_SS_SOUND_ORG) {
        //RTH todo:check
        lisyh_coil_set(lisy_home_ss_sound_map[0].mapped_to_coil, 1);

        lisyh_coil_set(lisy_home_ss_sound_map[1].mapped_to_coil, sound && 1);
        lisyh_coil_set(lisy_home_ss_sound_map[2].mapped_to_coil, sound && 2);
        lisyh_coil_set(lisy_home_ss_sound_map[3].mapped_to_coil, sound && 4);
        lisyh_coil_set(lisy_home_ss_sound_map[4].mapped_to_coil, sound && 8);
        lisyh_coil_set(lisy_home_ss_sound_map[5].mapped_to_coil, sound && 16);
    }
}

void
lisy_home_ss_display_event(int digit, int value, int display) {
    static int old_ballinplay_status = -1;
    static int old_match_status = -1;
    static int old_credit_status = -1;

    //printf("display event: display:%d digit:%d value:%d\n",display,digit,value);

    if (display == 0) //status display?
    {
        switch (digit) {
            //case LISY_HOME_DIGIT_CREDITS10:
            //		break;
            case LISY_HOME_DIGIT_CREDITS:
                if (old_credit_status < 0) {
                    old_credit_status = value;
                }
                break;
            case LISY_HOME_DIGIT_BALLINPLAY:
                //wheels reset when ballinplay changes from 0 to 1 ( start of game )
                lisy_home_ss_digit_ballinplay_status = value;
                if ((lisy_home_ss_digit_ballinplay_status == 1) & (old_ballinplay_status == 0)) {
                    //play start sound
                    if ((lisy_env.has_own_sounds) & (lisy_home_starship_sound == LISY_HOME_SS_SOUND_PHAT))
                        StarShip_play_wav(201); //fix setting RTH
                    //reset displays
                    wheel_score_reset();
                }
                //set/unset lamp for ball in play
                if ((lisy_home_ss_digit_ballinplay_status > 0) & (lisy_home_ss_digit_ballinplay_status <= 5))
                    lisy_home_ss_special_lamp_set(9 + lisy_home_ss_digit_ballinplay_status, 1); //Lamps 10...14
                if ((old_ballinplay_status > 0) & (old_ballinplay_status <= 5))
                    lisy_home_ss_special_lamp_set(9 + old_ballinplay_status, 0); //Lamps 10...14
                                                                                 //store old value
                old_ballinplay_status = lisy_home_ss_digit_ballinplay_status;
                break;
            case LISY_HOME_DIGIT_MATCH:
                //set/unset lamp for match
                if ((value >= 0) & (value <= 9))
                    lisy_home_ss_special_lamp_set(value, 1); //Lamps 0...9
                if ((old_match_status >= 0) & (old_match_status <= 9))
                    lisy_home_ss_special_lamp_set(old_match_status, 0); //Lamps 0...9
                old_match_status = value;
                break;
        }
    } //status display
}

void
lisy_home_ss_lamp_event(int lamp, int action) {
    static int hstd_count = 0;
    switch (lamp) {
        case LISY_HOME_SS_LAMP_1CANPLAY: //set light on player1 to ON if 1canplay and 2canplay is OFF, otherwise both ON
            lisy_home_ss_lamp_1canplay_status = action;
            if ((lisy_home_ss_lamp_1canplay_status == 1) & (lisy_home_ss_lamp_2canplay_status == 0)) {
                lisy_home_ss_special_lamp_set(15, 1);
                lisy_home_ss_special_lamp_set(16, 1);
                lisy_home_ss_special_lamp_set(17, 0);
                lisy_home_ss_special_lamp_set(18, 0);
            } else {
                lisy_home_ss_special_lamp_set(15, 1);
                lisy_home_ss_special_lamp_set(16, 1);
                lisy_home_ss_special_lamp_set(17, 1);
                lisy_home_ss_special_lamp_set(18, 1);
            }
            break;
        case LISY_HOME_SS_LAMP_2CANPLAY: //set light on player1 to ON if 1canplay and 2canplay is OFF, otherwise both ON
            lisy_home_ss_lamp_2canplay_status = action;
            if ((lisy_home_ss_lamp_1canplay_status == 1) & (lisy_home_ss_lamp_2canplay_status == 0)) {
                lisy_home_ss_special_lamp_set(15, 1);
                lisy_home_ss_special_lamp_set(16, 1);
                lisy_home_ss_special_lamp_set(17, 0);
                lisy_home_ss_special_lamp_set(18, 0);
            } else {
                lisy_home_ss_special_lamp_set(15, 1);
                lisy_home_ss_special_lamp_set(16, 1);
                lisy_home_ss_special_lamp_set(17, 1);
                lisy_home_ss_special_lamp_set(18, 1);
            }
            break;
        case LISY_HOME_SS_LAMP_GAMEOVER:
            if (action == 1) //end of game
            {
                //stop background sound
                if (lisy_env.has_own_sounds)
                    Mix_HaltChannel(202);
                lisy_game_running = 0;
                //activate scoring lights
                lisy_home_ss_special_lamp_set(15, 1);
                lisy_home_ss_special_lamp_set(16, 1);
                lisy_home_ss_special_lamp_set(17, 1);
                lisy_home_ss_special_lamp_set(18, 1);
            } else
            //start off game
            {
                //fix setting RTH //Start play background
                if ((lisy_env.has_own_sounds) & (lisy_home_starship_sound == LISY_HOME_SS_SOUND_PHAT))
                    StarShip_play_wav(202);
                lisy_game_running = 1;
                //deactivate special lamp hstd
                lisy_home_ss_special_lamp_set(22, 0);
                //reset hstd count
                hstd_count = 0;
            }
            break;
        case LISY_HOME_SS_LAMP_HSTD:
            //only if cycle parameter in ss_general_parms.csv is > 0
            if (lisy_home_ss_general.hstd_cycle > 0) {
                //count the events
                if (action)
                    hstd_count++;
                if (hstd_count == lisy_home_ss_general.hstd_cycle) {
                    //show high score to date
                    wheel_set_hstd(lisy_by35_CMOS->para.hstd);
                    //activate special lamp hstd
                    lisy_home_ss_special_lamp_set(22, 1);
                } else if (hstd_count == (lisy_home_ss_general.hstd_sleep + lisy_home_ss_general.hstd_cycle)) {
                    //reset counts
                    hstd_count = 0;
                    //show score attract mode
                    wheel_set_score(lisy_by35_CMOS->para.disp1_backup, lisy_by35_CMOS->para.disp2_backup);
                    //deactivate special lamp hstd
                    lisy_home_ss_special_lamp_set(22, 0);
                }
            }
            break;
    }
}

//things we want to do early after boot
void
lisy_home_ss_boot_event(int arg1) {

    //start intro sound in background if "lisy35_has_soundcard"
    if (arg1)
        system("/usr/bin/ogg123 -q /boot/lisy/lisyH/sounds/StarShip/Einschalt_Melodie.ogg &");
}

void
lisy_home_ss_init_event(void) {
    int i;
    unsigned char myled;

    //activate scoring lights
    lisy_home_ss_special_lamp_set(15, 1);
    lisy_home_ss_special_lamp_set(16, 1);
    lisy_home_ss_special_lamp_set(17, 1);
    lisy_home_ss_special_lamp_set(18, 1);

    //reset credit wheels
    wheel_score_credits_reset();
    //reset displays
    wheel_score_reset();

    //activate GI lamps line5&6 and all special ( e.g. for credit, drop targets 3000 and top rollover )
    for (i = 0; i <= 127; i++) {
        if (lisy_home_ss_GI_leds[i].line == 5) { //all active by default
           myled = lisy_home_ss_GI_leds[i].led;
           lisyh_led_set_line_GI_color( 5, led_rgbw_color[5][myled].red, led_rgbw_color[5][myled].green, led_rgbw_color[5][myled].blue);
           if (ls80dbg.bitv.lamps) {
                sprintf(debugbuf, "activate general GI on line 5 via led:%d line:%d RGB: %d %d %d",
                    lisy_home_ss_GI_leds[i].led, lisy_home_ss_GI_leds[i].line,
                    led_rgbw_color[5][myled].red, led_rgbw_color[5][myled].green, led_rgbw_color[5][myled].blue);
                lisy80_debug(debugbuf);
            }
        }
        else if (lisy_home_ss_GI_leds[i].line == 6) { //all active by default
           myled = lisy_home_ss_GI_leds[i].led;
           lisyh_led_set_line_GI_color( 6, led_rgbw_color[6][myled].red, led_rgbw_color[6][myled].green, led_rgbw_color[6][myled].blue);
           if (ls80dbg.bitv.lamps) {
                sprintf(debugbuf, "activate general GI on line 6 via led:%d line:%d RGB: %d %d %d",
                    lisy_home_ss_GI_leds[i].led, lisy_home_ss_GI_leds[i].line,
                    led_rgbw_color[6][myled].red, led_rgbw_color[6][myled].green, led_rgbw_color[6][myled].blue);
                lisy80_debug(debugbuf);
            }
        }
        else if (lisy_home_ss_GI_leds[i].line != 0) {
            lisyh_led_set(lisy_home_ss_GI_leds[i].led, lisy_home_ss_GI_leds[i].line, 1);
            if (ls80dbg.bitv.lamps) {
                sprintf(debugbuf, "activate GI led:%d line:%d", lisy_home_ss_GI_leds[i].led,
                        lisy_home_ss_GI_leds[i].line);
                lisy80_debug(debugbuf);
            }
        }
    } //for

    //set sound mode, read from coil PIC address 1
    lisy_home_ss_set_sound_mode(lisy_eeprom_1byte_read(1, 1), 1);
}

//set sound mode, color indicating included
//action == 1 fixed sound mode setting
//action == 0 sound mode switches to next (+1)
void
lisy_home_ss_set_sound_mode(int mode, int action) {
    if (action == 1) {
        lisy_home_starship_sound = mode;
        if (lisy_home_starship_sound > LISY_HOME_SS_SOUND_CHIMES)
            lisy_home_starship_sound = LISY_HOME_SS_SOUND_NONE;
    } else {
        lisy_home_starship_sound += 1;
        if (lisy_home_starship_sound > LISY_HOME_SS_SOUND_CHIMES)
            lisy_home_starship_sound = LISY_HOME_SS_SOUND_NONE;
    }

    //credit indicator shows sound status with color
    if (lisy_home_starship_sound == LISY_HOME_SS_SOUND_CHIMES)
        lisy_home_ss_lamp_set_colorcode(55, 128, 0, 0, 0);
    else if (lisy_home_starship_sound == LISY_HOME_SS_SOUND_ORG)
        lisy_home_ss_lamp_set_colorcode(55, 0, 128, 0, 0);
    else if (lisy_home_starship_sound == LISY_HOME_SS_SOUND_PHAT)
        lisy_home_ss_lamp_set_colorcode(55, 0, 0, 0, 128);
    else
        lisy_home_ss_lamp_set_colorcode(55, 0, 0, 128, 0);

    //store new mode to eeprom of coil PIC, address 1
    lisy_eeprom_1byte_write(1, lisy_home_starship_sound, 1);

    //if (  ls80dbg.bitv.sound )
    if (1) {
        sprintf(debugbuf, "sound status set to:%d", lisy_home_starship_sound);
        lisy80_debug(debugbuf);
    } //debug
}

//switch event, map sounds
void
lisy_home_ss_switch_event(int switch_no, int action) {

    switch (switch_no) {

        case 8: //remember status of outhole switch
            lisy_home_ss_switch_outhole_status = action;
            break;
        case 25: //switch soundoutput
            if (action == 1)
                lisy_home_ss_set_sound_mode(0, 0); //soundmode UP
            break;
    }

    //sound mapping
    if ((lisy_home_starship_sound == LISY_HOME_SS_SOUND_PHAT) & (lisy_env.has_own_sounds)
        & ((lisy35_flipper_disable_status == 0) | (lisy35_sound_stru[switch_no].onlyactiveingame == 0))) {
        if (action == lisy35_sound_stru[switch_no].trigger) {
            StarShip_play_wav(switch_no);

            //wait for sound finished?
            if (lisy35_sound_stru[switch_no].wait != 0) {
                while (Mix_Playing(switch_no) != 0)
                    ;
            }

            //delay after sound start?
            if (lisy35_sound_stru[switch_no].delay != 0) {
                sleep(lisy35_sound_stru[switch_no].delay);
            }
        }
    }
}

//the Starship eventhandler
void
lisy_home_ss_event_handler(int id, int arg1, int arg2, int arg3) {

    switch (id) {
        case LISY_HOME_SS_EVENT_BOOT:
            lisy_home_ss_boot_event(arg1);
            break;
        case LISY_HOME_SS_EVENT_INIT:
            lisy_home_ss_init_event();
            break;
        case LISY_HOME_SS_EVENT_LAMP:
            lisy_home_ss_lamp_event(arg1, arg2);
            break;
        case LISY_HOME_SS_EVENT_DISPLAY:
            lisy_home_ss_display_event(arg1, arg2, arg3);
            break;
        case LISY_HOME_SS_EVENT_CONT_SOL:
            lisy_home_ss_cont_sol_event(arg1);
            break;
        case LISY_HOME_SS_EVENT_MOM_SOL:
            lisy_home_ss_mom_sol_event(arg1);
            break;
        case LISY_HOME_SS_EVENT_SWITCH:
            lisy_home_ss_switch_event(arg1, arg2);
            break;
        case LISY_HOME_SS_EVENT_SOUND:
            lisy_home_ss_sound_event(arg1);
            break;
    }

    //2canplay lamp blocks credit switch when ball in play is 1 ( only 2 players on Starship)
    if ((lisy_home_ss_lamp_2canplay_status == 1) & (lisy_home_ss_digit_ballinplay_status == 1))
        lisy_home_ss_ignore_credit = 1;
    else
        lisy_home_ss_ignore_credit = 0;
}
