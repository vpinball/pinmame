#include "dmd.h"

UINT8 DmdFrameBuffer[16384] = {0};

void* dmdConvertToFrame(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSpa) {
    int buffer_size = width * height;
 
    for (int i = 0; i < buffer_size; i++) {
        if (bitDepth == 2) {
            switch (Buffer[i]) {
                case 0x14: // 20%
                    DmdFrameBuffer[i] = 0;
                    break;
                case 0x21: // 33%
                    DmdFrameBuffer[i] = 1;
                    break;
                case 0x43: // 67%
                    DmdFrameBuffer[i] = 2;
                    break;
                case 0x64: // 100%
                    DmdFrameBuffer[i] = 3;
                    break;
            }
        } else if (bitDepth == 4) {
            if (samSpa) {
                switch(Buffer[i]) {
                    case 0x00:
                        DmdFrameBuffer[i] = 0;
                        break;
                    case 0x14:
                        DmdFrameBuffer[i] = 1;
                        break;
                    case 0x19:
                        DmdFrameBuffer[i] = 2;
                        break;
                    case 0x1E:
                        DmdFrameBuffer[i] = 3;
                        break;
                    case 0x23:
                        DmdFrameBuffer[i] = 4;
                        break;
                    case 0x28:
                        DmdFrameBuffer[i] = 5;
                        break;
                    case 0x2D:
                        DmdFrameBuffer[i] = 6;
                        break;
                    case 0x32:
                        DmdFrameBuffer[i] = 7;
                        break;
                    case 0x37:
                        DmdFrameBuffer[i] = 8;
                        break;
                    case 0x3C:
                        DmdFrameBuffer[i] = 9;
                        break;
                    case 0x41:
                        DmdFrameBuffer[i] = 10;
                        break;
                    case 0x46:
                        DmdFrameBuffer[i] = 11;
                        break;
                    case 0x4B:
                        DmdFrameBuffer[i] = 12;
                        break;
                    case 0x50:
                        DmdFrameBuffer[i] = 13;
                        break;
                    case 0x5A:
                        DmdFrameBuffer[i] = 14;
                        break;
                    case 0x64:
                        DmdFrameBuffer[i] = 15;
                        break;
                }
            } else {
                switch (Buffer[i]) {
                    case 0x00:
                        DmdFrameBuffer[i] = 0;
                        break;
                    case 0x1E:
                        DmdFrameBuffer[i] = 1;
                        break;
                    case 0x23:
                        DmdFrameBuffer[i] = 2;
                        break;
                    case 0x28:
                        DmdFrameBuffer[i] = 3;
                        break;
                    case 0x2D:
                        DmdFrameBuffer[i] = 4;
                        break;
                    case 0x32:
                        DmdFrameBuffer[i] = 5;
                        break;
                    case 0x37:
                        DmdFrameBuffer[i] = 6;
                        break;
                    case 0x3C:
                        DmdFrameBuffer[i] = 7;
                        break;
                    case 0x41:
                        DmdFrameBuffer[i] = 8;
                        break;
                    case 0x46:
                        DmdFrameBuffer[i] = 9;
                        break;
                    case 0x4B:
                        DmdFrameBuffer[i] = 10;
                        break;
                    case 0x50:
                        DmdFrameBuffer[i] = 11;
                        break;
                    case 0x55:
                        DmdFrameBuffer[i] = 12;
                        break;
                    case 0x5A:
                        DmdFrameBuffer[i] = 13;
                        break;
                    case 0x5F:
                        DmdFrameBuffer[i] = 14;
                        break;
                    case 0x64:
                        DmdFrameBuffer[i] = 15;
                        break;
                }
            }
        }
    }

    return DmdFrameBuffer;
}
