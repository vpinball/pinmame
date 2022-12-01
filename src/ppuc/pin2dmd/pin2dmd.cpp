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
        return 0;
    }

    libusb_free_device_list(devs, 1);

    if (MyLibusbDeviceHandle == NULL) {
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return 0;
    }

    ret = libusb_get_string_descriptor_ascii(MyLibusbDeviceHandle, desc.iProduct, product, 256);

    if (libusb_claim_interface(MyLibusbDeviceHandle, 0) < 0)  //claims the interface with the Operating System
    {
        //Closes a device opened since the claim interface is failed.
        libusb_close(MyLibusbDeviceHandle);
        libusb_exit(ctx);
        return 0;
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

        memcpy(&OutputBuffer[4], Buffer, (chunksOf512Bytes * 512));

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
