#include <stdio.h>
#include <chrono>
#include <thread>

#include "zedmd.h"
#include "../serialib/serialib.h"

// Serial object
serialib device;

UINT16 deviceWidth = 0;
UINT16 deviceHeight = 0;

UINT8 ZeDMDControlCharacters[6] = {0x5a, 0x65, 0x64, 0x72, 0x75, 0x6d};
UINT8 ZeDMDDefaultPalette2Bit[12] = { 0, 0, 0, 144, 34, 0, 192, 76, 0, 255, 127 ,0 };
UINT8 ZeDMDDefaultPalette4Bit[48] = { 0, 0, 0, 51, 25, 0, 64, 32, 0, 77, 38, 0,
                                    89, 44, 0, 102, 51, 0, 115, 57, 0, 128, 64, 0,
                                    140, 70, 0, 153, 76, 0, 166, 83, 0, 179, 89, 0,
                                    191, 95, 0, 204, 102, 0, 230, 114, 0, 255, 127, 0 };

int ZeDmdInit(const char* ignore_device) {
    static int ret = 0;
    char device_name[22];

    // To shake hands, PPUC must send 7 bytes {0x5a, 0x65, 0x64, 0x72, 0x75, 0x6d, 12}.
    // The ESP32 will answer {0x5a, 0x65, 0x64, 0x72, width_low, width_high, height_low, height_high}.
    // Width (=width_low+width_high * 256) and height (=height_low+height_high * 256) are the
    // dimensions of the LED panel (128 * 32 or 256 * 64).
    UINT8 handshake[7] = {0x5a, 0x65, 0x64, 0x72, 0x75, 0x6d, 12};
    UINT8 acknowledge[8] = {0};

    for (int i = 1; i < 99; i++) {

#if defined (_WIN32) || defined( _WIN64)
        // Prepare the port name (Windows).
        sprintf(device_name, "\\\\.\\COM%d", i);
#elif defined (__linux__)
        // Prepare the port name (Linux).
        sprintf(device_name, "/dev/ttyUSB%d", i - 1);
#else
        // Prepare the port name (macOS).
        sprintf(device_name, "/dev/cu.usbserial-%04d", i);
#endif

        if (strcmp(device_name, ignore_device) == 0) {
            continue;
        }

        // Try to connect to the device.
        if (device.openDevice(device_name, 921600) == 1) {
            //printf("Device %s\n", device_name);

            // Reset the device.
            device.clearDTR();
            device.setRTS();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            device.clearRTS();
            device.clearDTR();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // The ESP32 sends some information about itself first. That needs to be removed before handshake.
            while (device.available() > 0) {
                device.readBytes(acknowledge, 8, 100);
                // @todo avoid endless loop in case of a different device that sends permanently.
            }

            if (device.writeBytes(handshake, 7)) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (device.readBytes(acknowledge, 8, 1000)) {
                    if (
                            acknowledge[0] == ZeDMDControlCharacters[0] &&
                            acknowledge[1] == ZeDMDControlCharacters[1] &&
                            acknowledge[2] == ZeDMDControlCharacters[2] &&
                            acknowledge[3] == ZeDMDControlCharacters[3]
                    ) {
                        deviceWidth = acknowledge[4] + acknowledge[5] * 256;
                        deviceHeight = acknowledge[6] + acknowledge[7] * 256;

                        if (deviceWidth > 0 && deviceHeight > 0) {
                            //printf("Width  %d\n", deviceWidth);
                            //printf("Height %d\n", deviceHeight);
                            return 1;
                        }
                    }
                }
            }
            // Close the device before testing the next port.
            device.closeDevice();
        }
    }

    return 0;
}

void ZeDmdRender(UINT16 width, UINT16 height, UINT8* Buffer, int bitDepth) {
    if (width <= deviceWidth && height <= deviceHeight) {
        // To send a 4-color frame, send {0x5a, 0x65, 0x64, 0x72, 0x75, 0x6d, 8} followed by 3 * 4 bytes for the palette
        // (R, G, B) followed by 2 planes of width * height / 8 bytes for the frame. It is possible to send a colored
        // palette or standard colors (orange (255,127,0) gradient).
        // To send a 16-color frame, send {0x5a, 0x65, 0x64, 0x72, 0x75, 0x6d, 9} followed by 3 * 16 bytes for the
        // palette (R, G, B) followed by 4 planes of width * height / 8 bytes for the frame. Once again, if you want to
        // use the standard colors, send (orange (255,127,0) gradient).
        //
        // Don't send the entire buffer at once. To avoid timing or buffer issues with the CP210x driver we send chunks
        // of 256 bytes. First we wait for a (R)eady signal from ZeDMD. In between the chunks we wait for an
        // (A)cknowledge signal that indicates that the entire chunk has been received. The (E)rror signal is ignored.
        // We don't have time to re-start the transmission from the beginning. Instead, we skip this frame and let
        // libpinmame provide the next frame as usual.
        char response = 0;
        if (device.readChar(&response, 100) && response == 'R') {
            device.writeBytes(ZeDMDControlCharacters, 6);

            int bytesSent = 0;
            if (bitDepth == 2) {
                // Command byte.
                device.writeChar(8);
                device.writeBytes(ZeDMDDefaultPalette2Bit, 12);
                bytesSent = 19;
            } else {
                // Command byte.
                device.writeChar(9);
                device.writeBytes(ZeDMDDefaultPalette4Bit, 48);
                bytesSent = 55;
            }

            int totalBytes = (width * height / 8 * bitDepth) + bytesSent;
            int chunk = 256 - bytesSent;
            int bufferPosition = 0;
            while (bufferPosition < totalBytes) {
                device.writeBytes(&Buffer[bufferPosition], ((totalBytes - bufferPosition) < chunk) ? (totalBytes - bufferPosition) : chunk);
                if (device.readChar(&response, 100) && response == 'A') {
                    // Received (A)cknowledge, ready to send the next chunk.
                    bufferPosition += chunk;
                    chunk = 256;
                } else {
                    // Something went wrong. Terminate current transmission of the buffer and return.
                    return;
                }
            }
        }
    }
}

#if defined(SERUM_SUPPORT)
void ZeDmdRenderSerum(UINT16 width, UINT16 height, UINT8* Buffer, UINT8* palette, UINT8* rotation) {
    if (width <= deviceWidth && height <= deviceHeight) {
        char response = 0;
        if (device.readChar(&response, 100) && response == 'R') {
            int planeBytes = (width * height / 8 * 6);
            int totalBytes = 6 + 1 + 192 + planeBytes + 24;
            int chunk = 256;
            UINT8* outputBuffer = (UINT8*) malloc(totalBytes);
            memcpy(&outputBuffer[0], ZeDMDControlCharacters, 6);
            outputBuffer[6] = 11;
            memcpy(&outputBuffer[7], palette, 192);
            memcpy(&outputBuffer[199], Buffer, planeBytes);
            memcpy(&outputBuffer[199 + planeBytes], rotation, 24);

            int bufferPosition = 0;
            while (bufferPosition < totalBytes) {
                device.writeBytes(&outputBuffer[bufferPosition], ((totalBytes - bufferPosition) < chunk) ? (totalBytes - bufferPosition) : chunk);
                if (device.readChar(&response, 100) && response == 'A') {
                    // Received (A)cknowledge, ready to send the next chunk.
                    bufferPosition += chunk;
                } else {
                    // Something went wrong. Terminate current transmission of the buffer and return.
                    free(outputBuffer);
                    return;
                }
            }

            free(outputBuffer);
        }
    }
}
#endif
