#include <stdio.h>
#include "dmd.h"

UINT8 DmdDefaultPalette2Bit[12] = { 0, 0, 0, 144, 34, 0, 192, 76, 0, 255, 127 ,0 };
UINT8 DmdDefaultPalette4Bit[48] = { 0, 0, 0, 51, 25, 0, 64, 32, 0, 77, 38, 0,
                                    89, 44, 0, 102, 51, 0, 115, 57, 0, 128, 64, 0,
                                    140, 70, 0, 153, 76, 0, 166, 83, 0, 179, 89, 0,
                                    191, 95, 0, 204, 102, 0, 230, 114, 0, 255, 127, 0 };

void dmdConvertToFrame(UINT16 width, UINT16 height, UINT8* Buffer, UINT8* Frame, int bitDepth, bool samSpa) {
    int buffer_size = width * height;

    for (int i = 0; i < buffer_size; i++) {
        if (bitDepth == 2) {
            switch (Buffer[i]) {
                case 0x21: // 33%
                    Frame[i] = 1;
                    break;
                case 0x43: // 67%
                    Frame[i] = 2;
                    break;
                case 0x64: // 100%
                    Frame[i] = 3;
                    break;
                case 0x14: // 20%
                default:
                    Frame[i] = 0;
                    break;
            }
        } else if (bitDepth == 4) {
            if (samSpa) {
                switch(Buffer[i]) {
                    case 0x00:
                        Frame[i] = 0;
                        break;
                    case 0x14:
                        Frame[i] = 1;
                        break;
                    case 0x19:
                        Frame[i] = 2;
                        break;
                    case 0x1E:
                        Frame[i] = 3;
                        break;
                    case 0x23:
                        Frame[i] = 4;
                        break;
                    case 0x28:
                        Frame[i] = 5;
                        break;
                    case 0x2D:
                        Frame[i] = 6;
                        break;
                    case 0x32:
                        Frame[i] = 7;
                        break;
                    case 0x37:
                        Frame[i] = 8;
                        break;
                    case 0x3C:
                        Frame[i] = 9;
                        break;
                    case 0x41:
                        Frame[i] = 10;
                        break;
                    case 0x46:
                        Frame[i] = 11;
                        break;
                    case 0x4B:
                        Frame[i] = 12;
                        break;
                    case 0x50:
                        Frame[i] = 13;
                        break;
                    case 0x5A:
                        Frame[i] = 14;
                        break;
                    case 0x64:
                        Frame[i] = 15;
                        break;
                }
            } else {
                switch (Buffer[i]) {
                    case 0x00:
                        Frame[i] = 0;
                        break;
                    case 0x1E:
                        Frame[i] = 1;
                        break;
                    case 0x23:
                        Frame[i] = 2;
                        break;
                    case 0x28:
                        Frame[i] = 3;
                        break;
                    case 0x2D:
                        Frame[i] = 4;
                        break;
                    case 0x32:
                        Frame[i] = 5;
                        break;
                    case 0x37:
                        Frame[i] = 6;
                        break;
                    case 0x3C:
                        Frame[i] = 7;
                        break;
                    case 0x41:
                        Frame[i] = 8;
                        break;
                    case 0x46:
                        Frame[i] = 9;
                        break;
                    case 0x4B:
                        Frame[i] = 10;
                        break;
                    case 0x50:
                        Frame[i] = 11;
                        break;
                    case 0x55:
                        Frame[i] = 12;
                        break;
                    case 0x5A:
                        Frame[i] = 13;
                        break;
                    case 0x5F:
                        Frame[i] = 14;
                        break;
                    case 0x64:
                        Frame[i] = 15;
                        break;
                }
            }
        }
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