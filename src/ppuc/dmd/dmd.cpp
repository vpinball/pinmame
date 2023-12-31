#include <stdio.h>
#include "dmd.h"

UINT8 DmdDefaultPalette2Bit[12] = { 0, 0, 0, 144, 34, 0, 192, 76, 0, 255, 127 ,0 };
UINT8 DmdDefaultPalette4Bit[48] = { 0, 0, 0, 51, 25, 0, 64, 32, 0, 77, 38, 0,
                                    89, 44, 0, 102, 51, 0, 115, 57, 0, 128, 64, 0,
                                    140, 70, 0, 153, 76, 0, 166, 83, 0, 179, 89, 0,
                                    191, 95, 0, 204, 102, 0, 230, 114, 0, 255, 127, 0 };

void dmdConvertToFrame(UINT16 width, UINT16 height, UINT8* frame, UINT8* planes, int bitDepth)
{
    UINT8 bitMask = 1;
    UINT32 tj = 0;
    const UINT32 frameSize = height * width;
    const UINT32 planeOffset = frameSize / 8;

    for (UINT8 tk = 0; tk < bitDepth; tk++) planes[tk * planeOffset + tj] = 0;

    for (UINT32 ti = 0; ti < frameSize; ti++)
    {
        UINT8 tl = 1;
        for (UINT8 tk = 0; tk < bitDepth; tk++)
        {
            if ((frame[ti] & tl) > 0)
            {
                planes[tk * planeOffset + tj] |= bitMask;
            }
            tl <<= 1;
        }

        if (bitMask == 0x80)
        {
            bitMask = 1;
            tj++;
            if (tj < planeOffset)
            {
                for (UINT8 tk = 0; tk < bitDepth; tk++)
                {
                    planes[tk * planeOffset + tj] = 0;
                }
            }
        }
        else bitMask <<= 1;
    }
}

void dmdConsoleRender(UINT16 width, UINT16 height, UINT8* buffer, int bitDepth) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            UINT8 value = buffer[y * width + x];
            if (bitDepth > 2) {
                printf("%2d", value);
            }
            else {
                switch (value) {
                    case 0:
                        printf("\033[0;40m⚫\033[0m");
                        break;
                    case 1:
                        printf("\033[0;40m🟤\033[0m");
                        break;
                    case 2:
                        printf("\033[0;40m🟠\033[0m");
                        break;
                    case 3:
                        printf("\033[0;40m🟡\033[0m");
                        break;
                }
            }
        }
        printf("\n");
    }
}

UINT8* dmdGetDefaultPalette(int bitDepth) {
    if (bitDepth == 2) {
        return DmdDefaultPalette2Bit;
    }

    return DmdDefaultPalette4Bit;
}