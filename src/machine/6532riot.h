/**********************************************************************

	6532 RIOT interface and emulation

	This function emulates all the functionality of up to 8 6532 RIOT
	peripheral interface adapters.

**********************************************************************/

#ifndef RIOT_6532
#define RIOT_6532

#define MAX_RIOT 8

#define RIOT_PORTA	0x00
#define RIOT_DDRA	0x01
#define RIOT_PORTB	0x02
#define RIOT_DDRB	0x03

#define RIOT_TIMER	0x00
#define RIOT_IRF	0x01

struct riot6532_interface
{
	mem_read_handler in_a_func;
	mem_read_handler in_b_func;
	mem_write_handler out_a_func;
	mem_write_handler out_b_func;
	void (*irq_func)(int state);
};

#ifdef __cplusplus
extern "C" {
#endif

void riot_unconfig(void);
void riot_config(int which, const struct riot6532_interface *intf);
void riot_reset(void);
int  riot_read(int which, int offset);
void riot_write(int which, int offset, int data);
void riot_set_input_a(int which, int data);
void riot_set_input_b(int which, int data);
void riot_clk(int which);

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ_HANDLER( riot_0_r );
READ_HANDLER( riot_1_r );
READ_HANDLER( riot_2_r );
READ_HANDLER( riot_3_r );
READ_HANDLER( riot_4_r );
READ_HANDLER( riot_5_r );
READ_HANDLER( riot_6_r );
READ_HANDLER( riot_7_r );

WRITE_HANDLER( riot_0_w );
WRITE_HANDLER( riot_1_w );
WRITE_HANDLER( riot_2_w );
WRITE_HANDLER( riot_3_w );
WRITE_HANDLER( riot_4_w );
WRITE_HANDLER( riot_5_w );
WRITE_HANDLER( riot_6_w );
WRITE_HANDLER( riot_7_w );

/******************* Standard 16-bit CPU interfaces, D0-D7 *******************/

READ16_HANDLER( riot_0_lsb_r );
READ16_HANDLER( riot_1_lsb_r );
READ16_HANDLER( riot_2_lsb_r );
READ16_HANDLER( riot_3_lsb_r );
READ16_HANDLER( riot_4_lsb_r );
READ16_HANDLER( riot_5_lsb_r );
READ16_HANDLER( riot_6_lsb_r );
READ16_HANDLER( riot_7_lsb_r );

WRITE16_HANDLER( riot_0_lsb_w );
WRITE16_HANDLER( riot_1_lsb_w );
WRITE16_HANDLER( riot_2_lsb_w );
WRITE16_HANDLER( riot_3_lsb_w );
WRITE16_HANDLER( riot_4_lsb_w );
WRITE16_HANDLER( riot_5_lsb_w );
WRITE16_HANDLER( riot_6_lsb_w );
WRITE16_HANDLER( riot_7_lsb_w );

/******************* Standard 16-bit CPU interfaces, D8-D15 *******************/

READ16_HANDLER( riot_0_msb_r );
READ16_HANDLER( riot_1_msb_r );
READ16_HANDLER( riot_2_msb_r );
READ16_HANDLER( riot_3_msb_r );
READ16_HANDLER( riot_4_msb_r );
READ16_HANDLER( riot_5_msb_r );
READ16_HANDLER( riot_6_msb_r );
READ16_HANDLER( riot_7_msb_r );

WRITE16_HANDLER( riot_0_msb_w );
WRITE16_HANDLER( riot_1_msb_w );
WRITE16_HANDLER( riot_2_msb_w );
WRITE16_HANDLER( riot_3_msb_w );
WRITE16_HANDLER( riot_4_msb_w );
WRITE16_HANDLER( riot_5_msb_w );
WRITE16_HANDLER( riot_6_msb_w );
WRITE16_HANDLER( riot_7_msb_w );

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER( riot_0_porta_w );
WRITE_HANDLER( riot_1_porta_w );
WRITE_HANDLER( riot_2_porta_w );
WRITE_HANDLER( riot_3_porta_w );
WRITE_HANDLER( riot_4_porta_w );
WRITE_HANDLER( riot_5_porta_w );
WRITE_HANDLER( riot_6_porta_w );
WRITE_HANDLER( riot_7_porta_w );

WRITE_HANDLER( riot_0_portb_w );
WRITE_HANDLER( riot_1_portb_w );
WRITE_HANDLER( riot_2_portb_w );
WRITE_HANDLER( riot_3_portb_w );
WRITE_HANDLER( riot_4_portb_w );
WRITE_HANDLER( riot_5_portb_w );
WRITE_HANDLER( riot_6_portb_w );
WRITE_HANDLER( riot_7_portb_w );

READ_HANDLER( riot_0_porta_r );
READ_HANDLER( riot_1_porta_r );
READ_HANDLER( riot_2_porta_r );
READ_HANDLER( riot_3_porta_r );
READ_HANDLER( riot_4_porta_r );
READ_HANDLER( riot_5_porta_r );
READ_HANDLER( riot_6_porta_r );
READ_HANDLER( riot_7_porta_r );

READ_HANDLER( riot_0_portb_r );
READ_HANDLER( riot_1_portb_r );
READ_HANDLER( riot_2_portb_r );
READ_HANDLER( riot_3_portb_r );
READ_HANDLER( riot_4_portb_r );
READ_HANDLER( riot_5_portb_r );
READ_HANDLER( riot_6_portb_r );
READ_HANDLER( riot_7_portb_r );

#ifdef __cplusplus
}
#endif

#endif
