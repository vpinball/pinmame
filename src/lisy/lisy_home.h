#ifndef LISY_HOME_H
#define LISY_HOME_H

// This struct represents the color of a single LED. RGBW
typedef struct rgbw_color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char white;
} t_rgbw_color;

// This struct represents the color of a single LED. RGB
typedef struct rgb_color {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} t_rgb_color;

typedef struct {
    unsigned char mapped_to_no;
    unsigned char mapped_is_coil;
    unsigned char r, g, b;
} t_lisy_home_lamp_map;

typedef struct {
    unsigned char mapped_to_no;
    unsigned char mapped_is_coil;
} t_lisy_home_coil_map;

int lisy_home_init_event(void);
void lisy_home_event_handler(int id, int arg1, int arg2, char* str);

//new starship, old one (tom&Jerry) to be adapted
typedef struct {
    unsigned char no_of_maps;
    unsigned char mapped_to_line[8];
    unsigned char mapped_to_led[8];
} t_lisy_home_ss_lamp_map;

void lisy_home_ss_lamp_set(int lamp, int action);
void lisy_home_ss_special_lamp_set(int lamp, int action);

//new starship, old one (tom&Jerry) to be adapted
typedef struct {
    int mapped_to_coil;
} t_lisy_home_ss_coil_map;

//starship special solenoids ( counters and bells)
typedef struct {
    int mapped_to_coil;
    int pulsetime;
    int delay;
    char comment[30];
} t_lisy_home_ss_special_coil_map;

//starship special lamps ( Ball in Play, Match and others )
typedef struct {
    unsigned char no_of_maps;
    unsigned char mapped_to_line[8];
    unsigned char mapped_to_led[8];
    char comment[50];
} t_lisy_home_ss_special_lamp_map;

//starship GI
typedef struct {
    unsigned char line;
    unsigned char led;
} t_lisy_home_ss_GI_leds;

//starship general parms
typedef struct {
    int hstd_cycle;
    int hstd_sleep;
} t_lisy_home_ss_general;

void lisy_home_ss_mom_coil_set(unsigned char value);
void lisy_home_ss_cont_coil_set(unsigned char cont_data);
void lisy_home_ss_send_led_colors(void);
void lisy_home_ss_event_handler(int id, int arg1, int arg2, int arg3);

//the IDs for the event handler
#define LISY_HOME_EVENT_INIT        0
#define LISY_HOME_EVENT_SOUND       1
#define LISY_HOME_EVENT_COIL        2
#define LISY_HOME_EVENT_SWITCH      3
#define LISY_HOME_EVENT_LAMP        4
#define LISY_HOME_EVENT_DISPLAY     5

//the IDs for the Starship event handler
#define LISY_HOME_SS_EVENT_INIT     0
#define LISY_HOME_SS_EVENT_MOM_SOL  2
#define LISY_HOME_SS_EVENT_CONT_SOL 3
#define LISY_HOME_SS_EVENT_LAMP     4
#define LISY_HOME_SS_EVENT_DISPLAY  5
#define LISY_HOME_SS_EVENT_SWITCH   6
#define LISY_HOME_SS_EVENT_BOOT     7
#define LISY_HOME_SS_EVENT_SOUND    8

//special lamps on Starship
#define LISY_HOME_SS_LAMP_PLAYER1UP 14
#define LISY_HOME_SS_LAMP_PLAYER2UP 29
#define LISY_HOME_SS_LAMP_1CANPLAY  13
#define LISY_HOME_SS_LAMP_HSTD      27
#define LISY_HOME_SS_LAMP_2CANPLAY  28
#define LISY_HOME_SS_LAMP_GAMEOVER  42

//special digits on Starship
#define LISY_HOME_DIGIT_CREDITS10   3
#define LISY_HOME_DIGIT_CREDITS     4
#define LISY_HOME_DIGIT_MATCH       6
#define LISY_HOME_DIGIT_BALLINPLAY  7

#endif /* LISY_HOME_H */
