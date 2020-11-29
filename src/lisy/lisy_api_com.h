#ifndef LISY_API_COM_H
#define LISY_API_COM_H

int lisy_api_send_str_to_disp(unsigned char num, char *str);
int lisy_api_send_SEG7_to_disp(unsigned char disp, int num, uint16_t *data);
int lisy_api_send_SEG14_to_disp(unsigned char disp, int num, uint16_t *data);
unsigned char lisy_api_ask_for_changed_switch(void);
unsigned char lisy_api_get_switch_status( unsigned char number);
void lisy_api_lamp_ctrl(int lamp_no,unsigned char action);
void lisy_api_sol_ctrl(int sol_no,unsigned char action);
void lisy_api_sol_pulse(int sol_no);
int lisy_api_read_string(unsigned char cmd, char *content);
unsigned char lisy_api_read_byte(unsigned char cmd, unsigned char *data);
void lisy_api_sol_set_hwrule(int sol_no, int special_switch);
void lisy_api_display_set_prot(unsigned char display_no,unsigned char protocol);
void lisy_api_sound_play_index( unsigned char board, unsigned char index );
void lisy_api_sound_play_file( unsigned char board, char *filename );
int lisy_api_print_hw_info(void);
void lisy_api_show_boot_message(char *software_version,char *system_id, int game_no, char *gamename);
int lisy_api_get_con_hw( char *idstr );
unsigned char lisy_api_get_dip_switch( unsigned char number);
int lisy_api_check_con_hw( char *idstr );

//mapping for segemnts
typedef union {
    unsigned char byte;
    struct {
    unsigned d:1, c:1, b:1, a:1, e:1, f:1, g:1, comma:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv_apc;
    struct {
    unsigned a:1, b:1, c:1, d:1, e:1, f:1, g:1, FREE:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv_pinmame;
    } map_byte1_t;

//mapping for segemnts
typedef union {
    unsigned char byte;
    struct {
    unsigned j:1, h:1, m:1, k:1, p:1, r:1, dot:1, n:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv_apc;
    struct {
    unsigned h:1, j:1, k:1, m:1, n:1, p:1, r:1, dot:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv_pinmame;
    } map_byte2_t;

#endif  /* LISY_API_COM_H */

