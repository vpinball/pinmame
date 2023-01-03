#ifndef DMD_H
#define DMD_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

void* dmdConvertToFrame(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSPa);

#endif /* DMD_H */