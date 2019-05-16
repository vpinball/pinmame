#ifndef HW_LIB_H_
#define HW_LIB_H_

//Version 3.x

//global struct for debug messaging or not, set in lisy80_init
typedef union {
    unsigned char byte;
    struct {
    unsigned sound:1, coils:1, lamps:1, switches:1, displays:1, basic:1, FREE:2;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } ls80dbg_t;

//global struct for options, set in lisy80_init
typedef union {
    unsigned char byte;
    struct {
    unsigned freeplay:1, JustBoom_sound:1, watchdog:1, sevendigit:1, slam:1, test:1, debug:1, autostart:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } ls80opt_t;

//for the switches
typedef union {
       unsigned char byte;
       struct {
       unsigned STROBE:3, RETURN:3, ONOFF:1, PARITY:1;
       //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
       } bitv;
   } switchdata_t;

void lisy80_hwlib_wiringPI_init(void);
void lisy_hwlib_init( void );
void lisymini_hwlib_init( void );
void lisy80_hwlib_shutdown(void);

int lisy80_write_byte_pic( int fd, unsigned char buf);
int lisy80_write_byte_disp_pic( unsigned char buf);
int lisy80_write_multibyte_disp_pic( char *buf, int no);
int lisy80_write_byte_coil_pic( unsigned char buf);
int lisy80_write_multibyte_coil_pic( char *buf, int no);

int lisy80_read_byte_pic(int fd);
unsigned char lisy80_read_byte_disp_pic(void);
unsigned char lisy80_read_byte_coil_pic(void);
unsigned char lisy80_read_byte_sw_pic(void);

void lisy80_sound_set(int sound);
void lisy1_sound_set(int sound);
//void lisy35_sound_set(int sound);

int lisy80_switch_readycheck( void );
int lisy80_switch_pic_init( void );
int lisy80_switch_reader( unsigned char *action );
int lisy1_switch_reader( unsigned char *action );
int lisy35_switch_reader( unsigned char *action );

void lisy80_get_dips(void);
int lisy80_dip1_debug_option(void);

void toggle_dp_led(void);
void lisy80_set_dp_led ( int value );
void lisy80_set_green_led( int value );
void lisy80_set_yellow_led( int value );
void lisy80_set_red_led( int value );

int  lisy80_get_poti_val(void);

void lisy35_switch_pic_init(unsigned char variant);

unsigned char lisymini_get_dip( char* wantdip);


//vars
//remeber the flip_flop settings of each
union lisy80_flip_flop {
        unsigned char byte;
struct {
        unsigned FREE : 4;
        unsigned ld1 : 1;
        unsigned ld2 : 1;
        unsigned ld3 : 1;
        unsigned ld4 : 1;
  } bit;
} flip_flop_byte[13];

#define I2C_DEVICE "/dev/i2c-1"
#define DISP_PIC_ADDR 0x40              // Address of PIC for displays
#define COIL_PIC_ADDR 0x41              // Address of PIC for coils

//Transistor assigment for the 9 Solenoids LISY80
#define Q_SOL1 60
#define Q_SOL2 58
#define Q_SOL3 54
#define Q_SOL4 55
#define Q_SOL5 62
#define Q_SOL6 64
#define Q_SOL7 56
#define Q_SOL8 53
#define Q_SOL9 59

//Transistor assigment for the 8 Solenoids LISY1
#define Q_KNOCK 37
#define Q_TENS 38
#define Q_HUND 39
#define Q_TOUS 40
#define Q_OUTH 41
#define Q_SYS1_SOL6 42
#define Q_SYS1_SOL7 43
#define Q_SYS1_SOL8 44

//GPIO assigments as of LISY80 HW version 3.11
// what pins of (WiringPi)Pi driving what
#define LISY80_HW311_LED_DP 2	//LED to signal display activity
#define LISY80_HW311_LED_RED 1	//LED to signal errors
#define LISY80_LED_YELLOW 5	//LED to signal that Pi is running 
#define LISY80_LED_GREEN 4	//LED to signal that LISY80 is running
//communication with Switch PIC
#define LISY80_INIT 6             // Init/Reset (output)
#define LISY80_ACK_FROM_PI 10     // Output
#define LISY80_BUF_READY 11	   //Input Signal von PIC dass Buffer gefuellt
#define LISY80_SWPIC_RESET 0	  //goes to MCLR of Switch PIC, active low
//data
#define LISY80_HW311_DATA_D0   27    // Data from Pic ( D0 & D4)
#define LISY80_HW311_DATA_D1   26    // Data from Pic ( D1 & D5)
#define LISY80_HW311_DATA_D2   28    // Data from Pic ( D2 & D6)
#define LISY80_HW311_DATA_D3   29    // Data from Pic ( D3 & D7)
//DIP Switch S1 HW311
//S1 Dip 1       GPIO 22(3)      Freeplay
//#S1 Dip 2       GPIO 10(12)     Sound via USB
//#S1 Dip 3       GPIO 9(13)      Watchdog
//#S1 Dip 4       GPIO 11(14)     Coil Protection
//#S1 Dip 5       GPIO 5 (21)     Remote Control
//#S1 Dip 6       GPIO 6(22)      TEST
//#S1 Dip 7       GPIO 13(23)     DEBUG
//#S1 Dip 8       GPIO 19(24)     AUTOSTART OFF
#define LISY80_HW311_DIP1_S1	3
#define LISY80_HW311_DIP1_S2	12
#define LISY80_HW311_DIP1_S3	13
#define LISY80_HW311_DIP1_S4	14
#define LISY80_HW311_DIP1_S5	21
#define LISY80_HW311_DIP1_S6	22
#define LISY80_HW311_DIP1_S7	23
#define LISY80_HW311_DIP1_S8	24

//GPIO assigments as of LISY80 HW version 3.2x
// what pins of (WiringPi)Pi driving what
//DIP Switch S1 HW32x
//#S1 Dip 1       Freeplay        RA2 Switch PIC
//#S1 Dip 2       GPIO 10(12)     Sound via USB
//#S1 Dip 3       Ball Save       RC4
//#S1 Dip 4       Option 1        RC5
//#S1 Dip 5       Option 2        RC6
//#S1 Dip 6       GPIO 6(22)      TEST
//#S1 Dip 7       GPIO 13(23)     DEBUG
//#S1 Dip 8       GPIO 5(21)      AUTOSTART OFF
#define LISY80_HW320_DIP1_S2	12
#define LISY80_HW320_DIP1_S6	22
#define LISY80_HW320_DIP1_S7	23
#define LISY80_HW320_DIP1_S8	21
//data
#define LISY80_HW320_DATA_D0   27    // Data from Pic ( D0 & D4)
#define LISY80_HW320_DATA_D1   26    // Data from Pic ( D1 & D5)
#define LISY80_HW320_DATA_D2   28    // Data from Pic ( D2 & D6)
#define LISY80_HW320_DATA_D3   13    // Data from Pic ( D3 & D7)
//others changed in 320
#define LISY80_HW320_LED_RED 14	//LED to signal errors
#define LISY_MINI_LED_RED 6	//LED to signal errors LISY_Mini

//sound settings
#define LISY80_A_PIN     7    //GPIO 4 (7)
#define LISY80_B_PIN     3    //GPIO 22 (3)


#endif  // _HW_LIB_H

