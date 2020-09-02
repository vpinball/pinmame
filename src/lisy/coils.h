#ifndef _COILS_H_
#define _COILS_H_


void lisy80_coil_set( int coil, int action);
void lisy1_coil_set( int coil, int action);

void coil_set_str( char *str, int action);
void coil_test ( void );
void lisy80_coil_init ( void );
void lisy1_coil_init ( void );
void lisy35_coil_init ( void );
void lisy35_lamp_init ( void );

int coil_get_sw_version(void);
void lisy80_coil_sound_set( int sound );
void lisy1_coil_sound_set( int sound );
void coil_cmd2pic_noansw(unsigned char command);
int coil_cmd2pic(unsigned char command);
int coil_get_k3(void);
void coil_led_set( int action);

void lisy35_mom_coil_set( unsigned char value );
void lisy35_cont_coil_set( unsigned char value );
void lisy35_sound_std_sb_set( unsigned char value );
void lisy35_sound_ext_sb_set( unsigned char value );
void lisy35_coil_set ( int coil, int action);
void lisy35_lamp_set ( int board, int lamp, int action);
void coil_bally_led_set( int action);
void lisy35_coil_set_sound_select( unsigned char value);
void lisy35_coil_set_direction_J4PIN5( unsigned char direction);
void lisy35_coil_set_direction_J4PIN8( unsigned char direction);
void lisy35_coil_set_direction_J1PIN8( unsigned char direction);
void lisy35_coil_set_lampdriver_variant( unsigned char variant);
void lisy35_coil_set_extended_SB_type( unsigned char type);
int lisy35_coil_get_mpu_dip(int dip_number);
void lisy35_coil_read_mpu_dips(void);

void lisyh_coil_select_solenoid_driver(void);
void lisyh_coil_select_lamp_driver(void);
void lisy35_coil_set_sound_raw( unsigned char value);

/* pulse time for coils in milli sec */
#define COIL_PULSE_TIME 150

//the commands
#define LS80COILCMD_EXT_CMD_ID 0  //extended command for ver 4
#define LS80COILCMD_GET_SW_VERSION_MAIN 1 
#define LS80COILCMD_GET_SW_VERSION_SUB 2
#define LS80COILCMD_INIT 3
#define LS35COIL_CONT_SOL 3  //continous solenoid for LISY35
#define LS80COILCMD_SETSOUND 4
#define LS35COIL_MOM_SOL 4   //momentary solenoid for LISY35
#define LISY35_STANDARD_SOUND 5
#define LISY35_EXTENDED_SOUND 6
#define LS80COILCMD_GET_K3 7

#define LISY_EXT_CMD_EEPROM_READ 0
#define LISY_EXT_CMD_EEPROM_WRITE 1

#define LISY35_EXT_CMD_AUX_BOARD_0 2  // no aux board
#define LISY35_EXT_CMD_AUX_BOARD_1 3  // AS-2518-43 12 lamps
#define LISY35_EXT_CMD_AUX_BOARD_2 4  // AS-2518-52 28 lamps
#define LISY35_EXT_CMD_AUX_BOARD_3 5  // AS-2518-23 60 lamps
#define LISY35_EXT_CMD_SB_IS_51 6  //Soundboard is a 2581-51 (extended mode)
#define LISY35_EXT_CMD_SB_IS_SAT 7  //Soundboard is a S&T (extended mode)
#define LISY35_EXT_CMD_J4PIN5_INPUT 8  // CO2_PB4 LATEbits.LE0   cont Sol 1
#define LISY35_EXT_CMD_J4PIN5_OUTPUT 9  // CO2_PB4 LATEbits.LE0  cont Sol 1
#define LISY35_EXT_CMD_J4PIN8_INPUT 10  //CO3_PB7 LATAbits.LA2   cont Sol 4
#define LISY35_EXT_CMD_J4PIN8_OUTPUT 11  //CO3_PB7 LATAbits.LA2  cont Sol 4 
#define LISY35_EXT_CMD_J1PIN8_INPUT 12  //LAMPSTR2 LATCbits.LC6  Lamp Strobe 2
#define LISY35_EXT_CMD_J1PIN8_OUTPUT 13  //LAMPSTR2 LATCbits.LC6  Lamp Strobe 2
#define LISY35_READ_DIP_SWITCHES 14  //read dip switches and buffer value
#define LISY35_GET_DIP_SWITCHES 15  //dip switches 1...4, need to be called four times

#define LISYH_EXT_CMD_FIRST_SOLBOARD 2 //first solenoidboard (default))
#define LISYH_EXT_CMD_SECOND_SOLBOARD 3 //second solenoidboard
#define LISYH_EXT_CMD_LED_ROW_1 4
#define LISYH_EXT_CMD_LED_ROW_2 5
#define LISYH_EXT_CMD_LED_ROW_3 6
#define LISYH_EXT_CMD_LED_ROW_4 7
#define LISYH_EXT_CMD_LED_ROW_5 8
#define LISYH_EXT_CMD_LED_ROW_6 9

//special coil numbers
#define LISY35_COIL_GREEN_LED 60
#define LISY35_COIL_SOUNDSELECT 61
#define LISY35_COIL_SOUNDRAW 62
#define LISY35_COIL_LAMPBOARD 63

//the aux lampdriverboards variants
#define NO_AUX_BOARD 0
#define AS_2518_43_12_LAMPS 1
#define AS_2518_52_28_LAMPS 2
#define AS_2518_23_60_LAMPS 3
#define AS_2518_147_LAMP_COMBO 4

//soundboard variants
#define LISY35_SB_CHIMES 0 //0 chimes
#define LISY35_SB_STANDARD 1 //1 standard soundboard
#define LISY35_SB_EXTENDED 2 //2 special soundboard ( S&T)


//the PIN Directions
#define LISY35_PIC_PIN_INPUT 1
#define LISY35_PIC_PIN_OUTPUT 0

//other
#define LISY35_SOLENOIDS_SELECTED 0
#define LISY35_SOUND_SELECTED 1


#endif  // _COILS_H_

