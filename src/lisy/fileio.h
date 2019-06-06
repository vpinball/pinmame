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
  char lamp17_is_Coil;  // will be 1 for Joker Poker, Countdown,Pinball-Pool and Buck Rogers
  char lamp18_is_Coil;  // will be 1 for Countdown
} t_stru_lisy1_games_csv;

typedef struct 
{
  unsigned char gamenr;  // internal game number ( 0..63)
  char gamename[10];  // game name mame fromat (8 chars)
  char long_name[80];  // game name from csv
  int throttle;  // throttle value per Bally game, default is 1000
  int special_cfg;  // is there a special config? (some games )
  int aux_lamp_variant;  //  what AUX lampdriverboard variant the game has
  int soundboard_variant;  //  what soundboard variant the game has
  char comment[80];  // comment from csv
} t_stru_lisy35_games_csv;

typedef struct 
{
  unsigned char gamenr;  // internal game number ( 0..63)
  char gamename[10];  // game name mame fromat (8 chars)
  char long_name[80];  // game name from csv
  char type[10];  // type of game ( e.g. SYS7 for Williams system7)
  int throttle;  // throttle value per game, default is 300
  char comment[80];  // comment from csv
} t_stru_lisymini_games_csv;

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
int  lisy80_file_get_coilopts(void);

int lisy1_file_get_gamename(t_stru_lisy1_games_csv *lisy1_game);
int lisy1_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename );
unsigned char lisy1_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filenamee, int re_init );
int lisy1_file_write_dipfile( int mode, char *line );
int  lisy1_file_get_coilopts(void);

int lisy35_file_get_gamename(t_stru_lisy35_games_csv *lisy35_game);
int lisy35_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename );
unsigned char lisy35_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filenamee, int re_init );
int lisy35_file_write_dipfile( int mode, char *line );

int lisymini_file_get_gamename(t_stru_lisymini_games_csv *lisymini_game);

//fadecandy stuff
int  lisy_file_get_led_mappings(unsigned char system);

/* LISY80 stuff */
#define LISY80_GAMES_CSV "/boot/lisy/lisy80/cfg/lisy80games.csv"
#define LISY80_GAMES_7DIGIT_CSV "/boot/lisy/lisy80/cfg/lisy80games_7digit.csv"
#define LISY80_FIRST_80B 40
#define LISY80_DIPS_PATH "/boot/lisy/lisy80/dips/"
#define LISY80_DIPS_FILE "_lisy80_dips.csv"
#define LISY80_SOUND_PATH "/boot/lisy/lisy80/sounds/"
#define LISY80_SOUND_FILE "_lisy80_sounds.csv"
#define LISY80_COIL_PATH "/boot/lisy/lisy80/coils/"
#define LISY80_COIL_FILE "_lisy80_coils.csv"
#define LISY80_FADECANDY_PATH "/boot/lisy/lisy80/fadecandy/"
#define LISY80_FADECANDY_LAMP_FILE "_lisy80_fadecandy_lamps.csv"
#define LISY80_FADECANDY_GI_FILE "_lisy80_fadecandy_GI.csv"

/* LISY1 stuff */
#define LISY1_GAMES_CSV "/boot/lisy/lisy1/cfg/lisy1games.csv"
#define LISY1_DIPS_PATH "/boot/lisy/lisy1/dips/"
#define LISY1_DIPS_FILE "_lisy1_dips.csv"
#define LISY1_SOUND_PATH "/boot/lisy/lisy1/sounds/"
#define LISY1_COIL_PATH "/boot/lisy/lisy1/coils/"
#define LISY1_COIL_FILE "_lisy1_coils.csv"
#define LISY1_FADECANDY_PATH "/boot/lisy/lisy1/fadecandy/"
#define LISY1_FADECANDY_LAMP_FILE "_lisy1_fadecandy_lamps.csv"
#define LISY1_FADECANDY_GI_FILE "_lisy1_fadecandy_GI.csv"


/* LISY35 stuff */
#define LISY35_GAMES_CSV "/boot/lisy/lisy35/cfg/lisy35games.csv"
#define LISY35_DIPS_PATH "/boot/lisy/lisy35/dips/"
#define LISY35_DIPS_FILE "_lisy35_dips.csv"
#define LISY35_FADECANDY_PATH "/boot/lisy/lisy35/fadecandy/"
#define LISY35_FADECANDY_LAMP_FILE "_lisy35_fadecandy_lamps.csv"
#define LISY35_FADECANDY_GI_FILE "_lisy35_fadecandy_GI.csv"

/* LISYMINI stuff */
#define LISYMINI_GAMES_CSV "/boot/lisy/lisy_m/cfg/lisyminigames.csv"



#endif  // _FILEIO_H

