#ifndef S80SOUND2_H
#define S80SOUND2_H

/*-- Sound Only Board, 1 X 2K Sound Rom --*/
#define S80SOUND2K_ROMSTART(n1,chk1,n2,chk2) \
  SOUNDREGION(0x10000, S80_MEMREG_SCPU1) \
    ROM_LOAD(n1, 0x0000,  0x0800, chk1) \
    ROM_RELOAD(  0xf000,  0x0800) \
    ROM_LOAD(n2, 0x0c00,  0x0400, chk2) \
    ROM_RELOAD(  0xfc00,  0x0400) /* reset vector */

#endif /* S80SOUND2_H */
