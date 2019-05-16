#ifndef _EEPROM_H
#define _EEPROM_H


//typedef for eeprom block
typedef union {
    char byte[256];
    struct {
      char signature[40]; //lisy80 signature "LISY80 by bontango - www.lisy80.com"
      unsigned char gamenr; //gamenr to check for nvram block (+1 =41)
      int starts; //number of starts overall (+4= 45)
      int debugs; //number of debugging sessions overall (+4= 49)
      short int counts[64]; //number of starts per game (+128 = 177)
      int Software_Main; //last software version used (main) (+4 = 181)
      int Software_Sub; //last software version used (sub) (+4 = 185)
      char fill[71]; //reserved for future use
        } content;
    } eeprom_block_t;

int lisy_eeprom_256byte_write( char *wbuf, int block);
int lisy_eeprom_256byte_read( char *rbuf, int block);
int lisy_eeprom_init(void);
int lisy_eeprom_printstats(void);
eeprom_block_t lisy_eeprom_getstats(void);
int lisy_eeprom_checksignature(void);

//PIC eeprom routines
int lisy_eeprom_1byte_read( unsigned char address, int block);
int lisy_eeprom_1byte_write( unsigned char address, unsigned char data,  int block);


#define LISY80_EEPROM_I2C_BUS      "/dev/i2c-1"
#define LISY1_EEPROM_ADDR1  0x52         	/* the 24C16 sits on i2c address 0x52 */
#define LISY1_EEPROM_ADDR2  0x53         	/* and 0x53 for lisy1 */
#define LISY80_HW311_EEPROM_ADDR1  0x50         /* the 24C16 sits on i2c address 0x50 */
#define LISY80_HW311_EEPROM_ADDR2  0x51         /* and 0x51 for harwware 311 */
#define LISY80_HW320_EEPROM_ADDR1  0x54         /* the 24C16 sits on i2c address 0x54 */
#define LISY80_HW320_EEPROM_ADDR2  0x55         /* and 0x55 for hardware 320 */
#define LISY35_EEPROM_ADDR1  0x56         	/* the 24C16 sits on i2c address 0x56 */
#define LISY35_EEPROM_ADDR2  0x57         	/* and 0x57 for lisy1 */
#define BYTES_PER_PAGE       256          /* one eeprom page is 256 byte */
#define MAX_BYTES            16            /* max number of bytes to write in one chunk */
       /* ... note: 24C02 and 24C01 only allow 8 bytes to be written in one chunk.   *
        *  if you are going to write 24C04,8,16 you can change this to 16            */

#endif  // _EEPROM_H

