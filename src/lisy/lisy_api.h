#ifndef LISY_API_H
#define LISY_API_H

#define LISY_API_VERSION_STR	"0.11"

//info, parameter none
#define	LISY_G_HW		0	//get connected LISY hardware -	return "LISY1" or "LISY80"
#define	LISY_G_LISY_VER		1  	//get LISY Version - return String
#define	LISY_G_API_VER		2 	//get API Version - return String
#define	LISY_G_NO_LAMPS		3	//get number of lamps - return byte
#define	LISY_G_NO_SOL		4 	//get number of soleneoids - return byte
#define	LISY_G_NO_SOUNDS	5	//get number of sounds - return byte
#define	LISY_G_NO_DISP		6	//get number of displays - return byte
#define	LISY_G_DISP_DETAIL	7	//get display details
#define	LISY_G_GAME_INFO	8	//get game info - return string 'Gottlieb internal number/char'
#define	LISY_G_NO_SW		9	//get number of switches - return byte

//lamps, parameter byte
#define	LISY_G_STAT_LAMP	10	//get status of lamp # - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_S_LAMP_ON		11	//set lamp # to ON - return none
#define	LISY_S_LAMP_OFF		12	//set lamp # to OFF - return none
#define	LISY_FADE_MOD_LIGHTS	13	//fade number of modern lights
#define	LISY_G_NO_MOD_LIGHTS	19	//get number of modern lights

//solenoids, parameter byte
#define	LISY_G_STAT_SOL		20	//get status of solenoid # - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_S_SOL_ON		21	//set solenoid # to ON - return none
#define	LISY_S_SOL_OFF		22	//set solenoid # to OFF - return none
#define	LISY_S_PULSE_SOL	23	//pulse solenoid# - return none
#define	LISY_S_PULSE_TIME	24	//set pulse time for solenoid# 1-byte integer ( 0 - 255 )
#define	LISY_S_RECYCLE_TIME	25	//set recycle time for solenoid# 1-byte integer ( 0 - 255 )
#define	LISY_S_SOL_PULSE_PWM    26	//Pulse solenoid and then enable solenoid with PWM

//displays, parameter string
#define	LISY_S_DISP_0		30	//set display 0 (status) to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_1		31	//set display 1 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_2		32	//set display 2 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_3		33	//set display 3 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_4		34	//set display 4 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_5		35	//set display 5 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_6		36	//set display 6 to sequence of bytes, second arg is length - return none
#define	LISY_S_DISP_PROT	37	//set the protocoll for the display, first arg is display no, second arg is display no

//switches, parameter byte/none
#define	LISY_G_STAT_SW		40	//get status of switch# - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_G_CHANGED_SW	41	//get changed switches - return byte "bit7 is status; 0..6 is number" "127 means no change since last call"

//Sound (Hardware), parameter byte/none
#define	LISY_S_PLAY_SOUND	50	//play sound#  
#define	LISY_S_STOP_SOUND	51	//stop all sounds ( same as play sound 0 )
#define	LISY_S_PLAY_FILE 	52	//play a file (in ./hardware_sounds) - option(1byte) + string 'filename'
#define	LISY_S_TEXT_TO_SPEECH	53	//say the text - option(1byte) + string 'text'
#define	LISY_S_SET_VOLUME	54	//sound volume in percent

//special rules
#define LISY_S_SET_HWRULE       0x3C    //Configure Hardware Rule for Solenoid

//read parameter (e.g. for APC)
#define LISY_G_SW_SETTING      0x40    //get DIP-Switch setting, arg is switch no - return byte is value of DIP (8pos)

//general, parameter none
#define	LISY_INIT		100	//init/reset LISY - return byte 0=OK, >0 Errornumber Errornumbers TBD
#define	LISY_WATCHDOG		101	//watchdog - return byte 0=OK, >0 Errornumber Errornumbers TBD
#define LISY_BACK_WHEN_READY    102     //get back when ready, call blocks until answer received - if Arduino needs more time (e.g. for sound) - return byte 0=OK(continue), >0 Errornumber Errornumbers TBD 

#endif  /* LISY_API_H */
