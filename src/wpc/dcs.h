#ifndef INC_DCS
#define INC_DCS

/* 200201 - added SOUNDROM5x for Dirty Harry */
/* 151200 - added sample support (SJE) */

#include "cpu/adsp2100/adsp2100.h"

/* 161200 Added dcs_ctrl_w */
/* 281100 Updated regions to MAME 37b9 */
/* 171000 Added SOUNDROM5m (for NBA) */

/*-- Sound ROM macros --*/
/*-- standard regions --*/
#define DCS_STDREG(size) \
   SOUNDREGION(ADSP2100_SIZE, WPC_MEMREG_SCPU) \
   SOUNDREGION(0x1000*2,      WPC_MEMREG_SBANK) \
   SOUNDREGION(size,          WPC_MEMREG_SROM)
#define DCS_ROMLOADx(start, n, chk) \
   ROM_LOAD(n, start, 0x080000, chk) ROM_RELOAD(start+0x080000, 0x080000)
#define DCS_ROMLOADm(start, n,chk) \
   ROM_LOAD(n, start, 0x100000, chk)

/*-- Games use different number of ROMS and different sizes --*/
#define DCS_SOUNDROM1x(n2,chk2) \
   DCS_STDREG(0x100000) \
   DCS_ROMLOADx(0x000000,n2,chk2)

#define DCS_SOUNDROM1m(n2,chk2) \
   DCS_STDREG(0x100000) \
   DCS_ROMLOADm(0x000000,n2,chk2)

#define DCS_SOUNDROM3m(n2,chk2,n3,chk3,n4,chk4) \
   DCS_STDREG(0x300000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4)

#define DCS_SOUNDROM4xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
   DCS_STDREG(0x400000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5)

#define DCS_SOUNDROM4mx(n2,chk2,n3,chk3,n4,chk4,n5,chk5) \
   DCS_STDREG(0x400000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5)

#define DCS_SOUNDROM5xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6)

#define DCS_SOUNDROM5x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6)

#define DCS_SOUNDROM5m(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6) \
   DCS_STDREG(0x500000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6)

#define DCS_SOUNDROM6x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7)

#define DCS_SOUNDROM6m(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADm(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6) \
   DCS_ROMLOADm(0x500000,n7,chk7)

#define DCS_SOUNDROM6xm(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7) \
   DCS_STDREG(0x600000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADm(0x100000,n3,chk3) \
   DCS_ROMLOADm(0x200000,n4,chk4) \
   DCS_ROMLOADm(0x300000,n5,chk5) \
   DCS_ROMLOADm(0x400000,n6,chk6) \
   DCS_ROMLOADm(0x500000,n7,chk7)

#define DCS_SOUNDROM7x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8) \
   DCS_STDREG(0x700000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7) \
   DCS_ROMLOADx(0x600000,n8,chk8)

#define DCS_SOUNDROM8x(n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8,n9,chk9) \
   DCS_STDREG(0x800000) \
   DCS_ROMLOADx(0x000000,n2,chk2) \
   DCS_ROMLOADx(0x100000,n3,chk3) \
   DCS_ROMLOADx(0x200000,n4,chk4) \
   DCS_ROMLOADx(0x300000,n5,chk5) \
   DCS_ROMLOADx(0x400000,n6,chk6) \
   DCS_ROMLOADx(0x500000,n7,chk7) \
   DCS_ROMLOADx(0x600000,n8,chk8) \
   DCS_ROMLOADx(0x700000,n9,chk9)

/*-- Machine structure externals --*/
extern const struct Memory_ReadAddress16  dcs2_readmem[];
extern const struct Memory_WriteAddress16 dcs2_writemem[];
extern const struct Memory_ReadAddress16  dcs1_readmem[];
extern const struct Memory_WriteAddress16 dcs1_writemem[];

extern struct CustomSound_interface 	dcs_custInt;
extern struct Samplesinterface 		samples_interface;


/*-- Sound interface communications --*/
extern READ_HANDLER (dcs_data_r);
extern WRITE_HANDLER(dcs_data_w);
extern WRITE_HANDLER(dcs_ctrl_w);
extern READ_HANDLER (dcs_ctrl_r);
extern void dcs_init(void);

#define DCS1_SOUNDCPU ,{ \
  CPU_ADSP2105 | CPU_AUDIO_CPU,	\
  10240000, /* 10.24 MHz */ \
  dcs1_readmem, dcs1_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define DCS2_SOUNDCPU ,{ \
  CPU_ADSP2105 | CPU_AUDIO_CPU,	\
  10240000, /* 10.24 MHz */ \
  dcs2_readmem, dcs2_writemem, 0, 0, \
  ignore_interrupt, 0 \
}

#define DCS_SOUND \
  { SOUND_CUSTOM, &dcs_custInt }, \
  { SOUND_SAMPLES, &samples_interface}

#endif /* INC_DCS */
