#include <string.h>
#include <libusb-1.0/libusb.h>

#include "pin2dmd.h"

//define PIN2DMD vendor id and product id
#define VID 0x0314
#define PID 0xe457

//endpoints for PIN2DMD communication
#define EP_IN 0x81
#define EP_OUT 0x01

bool Pin2dmd = false;
bool Pin2dmdXL = false;
bool Pin2dmdHD = false;

struct libusb_device **devs;
struct libusb_device_handle *MyLibusbDeviceHandle = NULL;
struct libusb_device_descriptor desc;
struct libusb_context *ctx = NULL;

UINT8 OutputBuffer[65536] = {};
char bytes[4] = {0};

int Pin2dmdInit() {
    static int ret = 0;
    static unsigned char product[256] = {};
    static const char* string = NULL;

    libusb_init(&ctx); /* initialize the library */

    int device_count = libusb_get_device_list(ctx, &devs);

    //Now look through the list that we just populated. We are trying to see if any of them match our device.
    int i;
    for (i = 0; i < device_count; i++) {
        libusb_get_device_descriptor(devs[i], &desc);
        if (VID == desc.idVendor && PID == desc.idProduct) {
            break;
        }
    }

    if (VID == desc.idVendor && PID == desc.idProduct) {
        ret = libusb_open(devs[i], &MyLibusbDeviceHandle);
        if (ret < 0) {
            return ret;
        }
    }
    else {
        return -99;
    }

    libusb_free_device_list(devs, 1);

    if (MyLibusbDeviceHandle == NULL) {
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return -99;
    }

    ret = libusb_get_string_descriptor_ascii(MyLibusbDeviceHandle, desc.iProduct, product, 256);

    if (libusb_claim_interface(MyLibusbDeviceHandle, 0) < 0)  //claims the interface with the Operating System
    {
        //Closes a device opened since the claim interface is failed.
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return -99;
    }

    string = (const char*)product;
    if (ret > 0) {
        if (strcmp(string, "PIN2DMD") == 0) {
            Pin2dmd = true;
            ret = 1;
        }
        else if (strcmp(string, "PIN2DMD XL") == 0) {
            Pin2dmdXL = true;
            ret = 2;
        }
        else if (strcmp(string, "PIN2DMD HD") == 0) {
            Pin2dmdHD = true;
            ret = 3;
        }
        else {
            ret = 0;
        }
    }

    return ret;
}

void Pin2dmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth, bool samSpa) {
    if (
        (width == 256 && height == 64 && Pin2dmdHD) ||
        (width == 192 && height == 64 && (Pin2dmdXL || Pin2dmdHD)) ||
        (width == 128 && height <= 32 && (Pin2dmd || Pin2dmdXL || Pin2dmdHD))
    ) {
        int outputBufferIndex = 4;
        int frameSizeInByte = width * height / 8;
        int chunksOf512Bytes = (frameSizeInByte / 512) * bitDepth;
        int bitShift = 0;

        OutputBuffer[0] = 0x81;
        OutputBuffer[1] = 0xc3;
        if (bitDepth == 4 && width == 128 && height == 32) {
            OutputBuffer[2] = 0xe7; // 4 bit header
            OutputBuffer[3] = 0x00;
        } else {
            OutputBuffer[2] = 0xe8; // non 4 bit header
            OutputBuffer[3] = chunksOf512Bytes; // number of 512 byte chunks
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (bitDepth == 2) {
                    switch (Buffer[y * width + x]) {
                        case 0x14: // 20%
                            //Activate if you want to have the entire Display to glow, a kind of background color.
                            //byte0 |= (1 << bitShift);
                            break;
                        case 0x21: // 33%
                            bytes[0] |= (1 << bitShift);
                            break;
                        case 0x43: // 67%
                            bytes[1] |= (1 << bitShift);
                            break;
                        case 0x64: // 100%
                            bytes[0] |= (1 << bitShift);
                            bytes[1] |= (1 << bitShift);
                            break;
                    }
                } else if (bitDepth == 4) {
                    if (samSpa) {
                        switch(Buffer[y * width + x]) {
                            case 0x00:
                                break;
                            case 0x14:
                                bytes[0] |= (1 << bitShift);
                                break;
                            case 0x19:
                                bytes[1] |= (1 << bitShift);
                                break;
                            case 0x1E:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                break;
                            case 0x23:
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x28:
                                bytes[0] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x2D:
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x32:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x37:
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x3C:
                                bytes[0] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x41:
                                bytes[1] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x46:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x4B:
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x50:
                                bytes[0] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x5A:
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x64:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                        }
                    } else {
                        switch (Buffer[y * width + x]) {
                            case 0x00:
                                break;
                            case 0x1E:
                                bytes[0] |= (1 << bitShift);
                                break;
                            case 0x23:
                                bytes[1] |= (1 << bitShift);
                                break;
                            case 0x28:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                break;
                            case 0x2D:
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x32:
                                bytes[0] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x37:
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x3C:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                break;
                            case 0x41:
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x46:
                                bytes[0] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x4B:
                                bytes[1] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x50:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x55:
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x5A:
                                bytes[0] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x5F:
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                            case 0x64:
                                bytes[0] |= (1 << bitShift);
                                bytes[1] |= (1 << bitShift);
                                bytes[2] |= (1 << bitShift);
                                bytes[3] |= (1 << bitShift);
                                break;
                        }
                    }
                }

                bitShift++;
                if (bitShift > 7) {
                    bitShift = 0;
                    for (int i = 0; i < bitDepth; i++) {
                        OutputBuffer[(frameSizeInByte * i) + outputBufferIndex] = bytes[i];
                        bytes[i] = 0;
                    }
                    outputBufferIndex++;
                }
            }
        }

        // The OutputBuffer to be sent consists of a 4 byte header and a number of chunks of 512 bytes.
        libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, (chunksOf512Bytes * 512) + 4, NULL, 1000);
    }
}

void Pin2dmdRenderRaw(UINT16 width, UINT16 height, UINT8* Buffer, UINT32 frames) {
    if (
        (width == 256 && height == 64 && Pin2dmdHD) ||
        (width == 192 && height == 64 && (Pin2dmdXL || Pin2dmdHD)) ||
        (width == 128 && height <= 32 && (Pin2dmd || Pin2dmdXL || Pin2dmdHD))
    ) {
        int frameSizeInByte = width * height / 8;
        int chunksOf512Bytes = (frameSizeInByte / 512) * frames;
        int bufferSizeInBytes = frameSizeInByte * frames;

        OutputBuffer[0] = 0x52; // RAW mode
        OutputBuffer[1] = 0x80;
        OutputBuffer[2] = 0x20;
        OutputBuffer[3] = chunksOf512Bytes; // number of 512 byte chunks
        memcpy(&OutputBuffer[4], Buffer, bufferSizeInBytes);

        libusb_bulk_transfer(MyLibusbDeviceHandle, EP_OUT, OutputBuffer, bufferSizeInBytes, NULL, 1000);
    }
}
