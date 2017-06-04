#ifndef DISPLAYS_H_
#define DISPLAYS_H_

void display_show_int( int display, int digit, int dat);
void display_show_char( int display, int digit, char data);
void display_show_str( int display, char *data);
int display_get_sw_version(void);
int display_get_dipsw_value(void);
//System80 & 80A
void display_cmd2pic_noansw(unsigned char command);
int display_cmd2pic(unsigned char command);
int display_cmd2pic_2b(unsigned char command);
//System80B
void display_cmd2pic_80b_noansw(unsigned char command, unsigned char value);
void display_init(void);
void display_reset(void);
void display_send_byte_torow( unsigned char data, int LD1_was_set, int LD2_was_set );
void display_send_row_torow( char *data, int LD1_was_set, int LD2_was_set );
void display_sendtorow( unsigned char data, int LD1_was_set, int LD2_was_set, int debug );
//messages
void display_show_boot_message(char *s_lisy80_software_version,int lisy80_gtb_no,char *lisy80_gamename);
void display_show_boot_message_lisy1(char *s_lisy1_software_version,char *rom_id,char *lisy1_gamename);
void display_show_shut_message(void);
void display_show_error_message(int errornumber, char *msg80, char *msg80B );

#define LS80DPCMD_INIT 1
#define LS80DPCMD_GET_SW_VERSION_MAIN 2
#define LS80DPCMD_GET_SW_VERSION_SUB 3
#define LS80DPCMD_GET_DIPSW_VALUE 4
#define LS80DPCMD_RESET 5
#define LS80DPCMD_NEXT20_ROW1 6
#define LS80DPCMD_NEXT20_ROW2 7

#define LS80D_B_TOROW12 0
#define LS80D_B_TOROW1 1
#define LS80D_B_TOROW2 2

#endif  // DISPLAYS_H_

