#include "dmd.h"

UINT8 DmdRenderBuffer[2048] = {0};

void* dmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSpa) {
    int renderBufferIndex = 0;
    int frameSizeInByte = width * height / 8;
    int bitShift = 0;
    char planeBytes[4] = {0};

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (bitDepth == 2) {
                switch (Buffer[y * width + x]) {
                    case 0x14: // 20%
                        //Activate if you want to have the entire Display to glow, a kind of background color.
                        //byte0 |= (1 << bitShift);
                        break;
                    case 0x21: // 33%
                        planeBytes[0] |= (1 << bitShift);
                        break;
                    case 0x43: // 67%
                        planeBytes[1] |= (1 << bitShift);
                        break;
                    case 0x64: // 100%
                        planeBytes[0] |= (1 << bitShift);
                        planeBytes[1] |= (1 << bitShift);
                        break;
                }
            } else if (bitDepth == 4) {
                if (samSpa) {
                    switch(Buffer[y * width + x]) {
                        case 0x00:
                            break;
                        case 0x14:
                            planeBytes[0] |= (1 << bitShift);
                            break;
                        case 0x19:
                            planeBytes[1] |= (1 << bitShift);
                            break;
                        case 0x1E:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            break;
                        case 0x23:
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x28:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x2D:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x32:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x37:
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x3C:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x41:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x46:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x4B:
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x50:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x5A:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x64:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                    }
                } else {
                    switch (Buffer[y * width + x]) {
                        case 0x00:
                            break;
                        case 0x1E:
                            planeBytes[0] |= (1 << bitShift);
                            break;
                        case 0x23:
                            planeBytes[1] |= (1 << bitShift);
                            break;
                        case 0x28:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            break;
                        case 0x2D:
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x32:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x37:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x3C:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            break;
                        case 0x41:
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x46:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x4B:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x50:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x55:
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x5A:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x5F:
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                        case 0x64:
                            planeBytes[0] |= (1 << bitShift);
                            planeBytes[1] |= (1 << bitShift);
                            planeBytes[2] |= (1 << bitShift);
                            planeBytes[3] |= (1 << bitShift);
                            break;
                    }
                }
            }

            bitShift++;
            if (bitShift > 7) {
                bitShift = 0;
                for (int i = 0; i < bitDepth; i++) {
                    DmdRenderBuffer[(frameSizeInByte * i) + renderBufferIndex] = planeBytes[i];
                    planeBytes[i] = 0;
                }
                renderBufferIndex++;
            }
        }
    }

    return DmdRenderBuffer;
}
