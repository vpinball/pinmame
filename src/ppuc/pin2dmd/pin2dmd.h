#ifndef PIN2DMD_H
#define PIN2DMD_H

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

int Pin2dmdInit();
void Pin2dmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSpa);
void Pin2dmdRenderRaw(UINT16 width, UINT16 height, UINT8* Buffer, UINT32 frames);

#endif /* PIN2DMD_H */