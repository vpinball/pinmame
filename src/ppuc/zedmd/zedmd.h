#ifndef ZEDMD_H
#define ZEDMD_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

int ZeDmdInit(const char* ignore_device);
void ZeDmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSPa);

#endif /* ZEDMD_H */