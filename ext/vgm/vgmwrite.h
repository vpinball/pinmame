#ifndef __VGMWRITE_H__
#define __VGMWRITE_H__

#include "driver.h"

#define int8_t   signed char
#define uint8_t  unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t unsigned long long

void vgm_start(struct RunningMachine *machine);
void vgm_stop(void);
uint16_t vgm_open(uint8_t chip_type, double clock);
void vgm_header_set(uint16_t chip_id, uint8_t attr, uint32_t data);
void vgm_write(uint16_t chip_id, uint8_t port, uint16_t r, uint8_t v);
void vgm_write_large_data(uint16_t chip_id, uint8_t type, uint32_t datasize, uint32_t value1, uint32_t value2, const void* data);
uint16_t vgm_get_chip_idx(uint8_t chip_type, uint8_t Num);
void vgm_change_rom_data(uint32_t oldsize, const void* olddata, uint32_t newsize, const void* newdata);
void vgm_dump_sample_rom(uint16_t chip_id, uint8_t type, int region);
//void vgm_dump_sample_rom(uint16_t chip_id, uint8_t type, address_space& space);

// VGM Chip Constants
// v1.00
#define VGMC_SN76496	0x00
#define VGMC_YM2413		0x01
#define VGMC_YM2612		0x02
#define VGMC_YM2151		0x03
// v1.51
#define VGMC_SEGAPCM	0x04
#define VGMC_RF5C68		0x05
#define VGMC_YM2203		0x06
#define VGMC_YM2608		0x07
#define VGMC_YM2610		0x08
#define VGMC_YM3812		0x09
#define VGMC_YM3526		0x0A
#define VGMC_Y8950		0x0B
#define VGMC_YMF262		0x0C
#define VGMC_YMF278B	0x0D
#define VGMC_YMF271		0x0E
#define VGMC_YMZ280B	0x0F
#define VGMC_T6W28		0x7F	// note: emulated via 2xSN76496
#define VGMC_RF5C164	0x10
#define VGMC_PWM		0x11
#define VGMC_AY8910		0x12
// v1.61
#define VGMC_GBSOUND	0x13
#define VGMC_NESAPU		0x14
#define VGMC_MULTIPCM	0x15
#define VGMC_UPD7759	0x16
#define VGMC_OKIM6258	0x17
#define VGMC_OKIM6295	0x18
#define VGMC_K051649	0x19
#define VGMC_K054539	0x1A
#define VGMC_C6280		0x1B
#define VGMC_C140		0x1C
#define VGMC_K053260	0x1D
#define VGMC_POKEY		0x1E
#define VGMC_QSOUND		0x1F
// v1.71
#define VGMC_SCSP		0x20
#define VGMC_WSWAN		0x21
#define VGMC_VSU		0x22
#define VGMC_SAA1099	0x23
#define VGMC_ES5503		0x24
#define VGMC_ES5506		0x25
#define VGMC_X1_010		0x26
#define VGMC_C352		0x27
#define VGMC_GA20		0x28

//#define VGMC_OKIM6376	0xFF
#endif /* __VGMWRITE_H__ */
