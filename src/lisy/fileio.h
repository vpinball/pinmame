#ifndef _FILEIO_H
#define _FILEIO_H

typedef struct 
{
  unsigned char gamenr;  // internal game number ( 0..63)
  unsigned char is80B;  // >0 means 80B, taken from dipswitch
  char type_from_csv[10];  //type string from CSV SYS80|SYS80A|SYS80B
  char gamename[10];  // game name mame fromat (8 chars)
  char long_name[40];  // game name from csv
  int gtb_no;  // Gottlieb number from csv
  int throttle;  // throttle value per Gottlieb game, default is 1000
  char comment[80];  // comment from csv
} t_stru_lisy80_games_csv;

typedef struct 
{
  unsigned char gamenr;  // internal game number ( 0..63)
  char gamename[10];  // game name mame fromat (8 chars)
  char long_name[40];  // game name from csv
  char rom_id[10];  // system1 rom-id (1 char)
  int throttle;  // throttle value per Gottlieb game, default is 1000
  char comment[80];  // comment from csv
} t_stru_lisy1_games_csv;

typedef struct 
{
  unsigned char can_be_interrupted;  // if this sound can be interrupted be others
  unsigned char loop;  // sound needs to be playd in a 'loop' (e.g. background sound)
  unsigned char st_a_catchup;  // sound will be queued andplayd later when device is available
} t_stru_lisy80_sounds_csv;

int lisy80_file_get_gamename(t_stru_lisy80_games_csv *lisy80_game);
int lisy80_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename );
unsigned char lisy80_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filenamee, int re_init );
int lisy80_file_write_dipfile( int mode, char *line );
int  lisy80_file_get_soundopts(void);

int lisy1_file_get_gamename(t_stru_lisy1_games_csv *lisy1_game);
int lisy1_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename );
unsigned char lisy1_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filenamee, int re_init );
int lisy1_file_write_dipfile( int mode, char *line );


#define LISY80_GAMES_CSV "/boot/lisy80/cfg/lisy80games.csv"
#define LISY80_FIRST_80B 40
#define LISY80_DIPS_PATH "/boot/lisy80/dips/"
#define LISY80_DIPS_FILE "_lisy80_dips.csv"
#define LISY80_SOUND_PATH "/boot/lisy80/sounds/"
#define LISY80_SOUND_FILE "_lisy80_sounds.csv"

#define LISY1_DIPS_PATH "/boot/lisy1/dips/"
#define LISY1_DIPS_FILE "_lisy1_dips.csv"

#define LISY1_GAMES_CSV "/boot/lisy1/cfg/lisy1games.csv"



#endif  // _FILEIO_H

