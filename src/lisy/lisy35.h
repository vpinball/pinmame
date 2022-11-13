// license:BSD-3-Clause

#ifndef LISY35_H
#define LISY35_H

// Generic LISY35 functions.
void lisy35_shutdown(void);
int lisy35_get_gamename(char* gamename);
//void lisy35_display_handler( int index, uint16_t seg );
void lisy35_display_handler(int index, int value);
//unsigned char lisy35_switch_handler(int sys35col);
void lisy35_switch_update(void);
void lisy35_solenoid_handler(unsigned char data, unsigned char soundselect);
void lisy35_sound_handler(unsigned char type, unsigned char data);
void lisy35_lamp_handler(int blanking, int board, int lampadr, int lampdata);
void lisy35_throttle(void);
int lisy35_get_mpudips(int switch_nr);
void lisy35_nvram_handler(void* by35_CMOS);
int lisy35_get_SW_Selftest(void);
int lisy35_get_SW_S33(void);

void lisy35_set_variant(void);
void lisy35_set_soundboard_variant(void);

//the IDs for the sound handler (called from by35.c)
#define LISY35_SOUND_HANDLER_IS_DATA 0
#define LISY35_SOUND_HANDLER_IS_CTRL 1

//cmos structure
typedef union {
    unsigned char byte[256];

    struct {
        unsigned char unused[30];           //0x1E (30) unused
        unsigned char disp1_val[6];         //$021E-$0223 ---- Display 1 value
        unsigned char disp2_val[6];         //$0224-$0229 ---- Display 2 value
        unsigned char disp3_val[6];         //$022A-$022F ---- Display 3 value
        unsigned char disp4_val[6];         //$0230-$0235 ---- Display 4 value
        unsigned char credit_val[6];        //$0236-$023B ---- Display Credit/Match/BallInPlay
        unsigned char disp1_backup[6];      //$023C-$0241 ---- Backup of Display 1 scores during Attract Mode
        unsigned char disp2_backup[6];      //$0242-$0247 ---- Backup of Display 2 scores during Attract Mode
        unsigned char disp3_backup[6];      //$0248-$024D ---- Backup of Display 3 scores during Attract Mode
        unsigned char disp4_backup[6];      //$024E-$0253 ---- Backup of Display 4 scores during Attract Mode
        unsigned char unused2[105];         //105 unused
        unsigned char score_treshold_l1[6]; // $02BD-$02C2 ---- 01 ---- Score Threshold Level 1
        unsigned char score_treshold_l2[6]; //$02C3-$02C8 ---- 02 ---- Score Threshold Level 2
        unsigned char score_treshold_l3[6]; //$02C9-$02CE ---- 03 ---- Score Threshold Level 3
        unsigned char hstd[6];              //$02CF-$02D4 ---- 04 ---- High Score To Date
        unsigned char credits[6];           //$02D5-$02DA ---- 05 ---- Current Credits
        unsigned char total_plays[6];       //$02DB-$02E0 ---- 06 ---- Total Plays (Payed and Free Games)
        unsigned char total_replayw[6];     //$02E1-$02E6 ---- 07 ---- Total Replays (Free Games)
        unsigned char hstd_beaten[6];       //$02E7-$02EC ---- 08 ---- Total times 'High Score To Date' is beaten
        unsigned char coins_chu1[6];        //$02ED-$02F2 ---- 09 ---- Coins dropped through Coin Chute #1
        unsigned char coins_chu2[6];        //$02F3-$02F8 ---- 10 ---- Coins dropped through Coin Chute #2
        unsigned char coins_chu3[6];        //$02F9-$02FE ---- 11 ---- Coins dropped through Coin Chute #3
        unsigned char unused3;              // unused
    } para;
} lisy35_cmos_data_t;

#endif /* LISY35_H */
