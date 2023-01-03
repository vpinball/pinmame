#ifndef DMD_COMMON_H
#define DMD_COMMON_H

#if defined _WIN32
#define DMD_COMMON(RetType) extern "C" __declspec(dllexport) RetType
#else
#define DMD_COMMON(RetType) extern "C" RetType __attribute__((visibility("default")))
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned int UINT;

DMD_COMMON(void) DmdCommon_ConvertFrameToPlanes(UINT32 width, UINT32 height, UINT8* frame, UINT8* planes, int bitDepth);

#endif