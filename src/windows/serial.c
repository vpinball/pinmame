/*
    Windows serial port code for uart_16c450.c based on the XBee ANSI C Library:
        https://github.com/tomlogic/xbee_ansic_library/

    Originally written by Tom Collins <tom@tomlogic.com> and released under
    an MPL 2.0 License.

    This PinMAME adaptation written July 2024 by Tom Collins.
*/

#include <stdio.h>
#include <windows.h>

#include "uart_host.h"

static HANDLE uart_hCom;

#if 0
#define ERRMSG(...)      fprintf(stderr, __VA_ARGS__)
#else
#define ERRMSG(...)
#endif

// Pass "COM99" as device.
int uart_open(const char *device, data32_t baudrate)
{
    char            buffer[20];
    HANDLE          hCom;
    COMMTIMEOUTS    timeouts;
    int             err;

    // filename needs to be \\.\COM99 where "99" is the comport number

    snprintf(buffer, sizeof buffer, "\\\\.\\%s", device);
    hCom = CreateFileA(buffer, GENERIC_READ | GENERIC_WRITE,
                       0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hCom == INVALID_HANDLE_VALUE) {
        ERRMSG("%s: error %lu opening handle to %s\n", __FUNCTION__,
               GetLastError(), buffer);
        return -EIO;
    }

    // Set up transmit and receive buffers.
    SetupComm(hCom, 128, 128);

    /*  Set the COMMTIMEOUTS structure.  Per MSDN documentation for
        ReadIntervalTimeout, "A value of MAXDWORD, combined with zero values
        for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier
        members, specifies that the read operation is to return immediately
        with the bytes that have already been received, even if no bytes have
        been received."
    */
    if (!GetCommTimeouts(hCom, &timeouts)) {
        ERRMSG("%s: %s failed (%lu initializing %s)\n",
               __FUNCTION__, "GetCommTimeouts", GetLastError(), device);
        CloseHandle(hCom);
        return -EIO;
    }

    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(hCom, &timeouts)) {
        ERRMSG("%s: %s failed (%lu initializing %s)\n",
               __FUNCTION__, "SetCommTimeouts", GetLastError(), device);
        CloseHandle(hCom);
        return -EIO;
    }

    PurgeComm(hCom, PURGE_TXCLEAR | PURGE_TXABORT |
              PURGE_RXCLEAR | PURGE_RXABORT);

    uart_hCom = hCom;

    err = uart_baudrate(baudrate);

    if (err) {
        uart_hCom = NULL;
    }

    return err;
}


int uart_close(void)
{
    CloseHandle(uart_hCom);
    uart_hCom = NULL;

    return 0;
}


int uart_baudrate(data32_t baudrate)
{
    DCB         dcb;

    if (uart_hCom == NULL) {
        return -EINVAL;
    }

    memset(&dcb, 0, sizeof dcb);
    dcb.DCBlength = sizeof dcb;
    if (!GetCommState(uart_hCom, &dcb)) {
        ERRMSG("%s: %s failed (%lu)\n",
               __FUNCTION__, "GetComState", GetLastError());
        return -EIO;
    }
    dcb.BaudRate = baudrate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fAbortOnError = FALSE;

    if (!SetCommState(uart_hCom, &dcb)) {
        ERRMSG("%s: %s failed (%lu)\n",
               __FUNCTION__, "SetComState", GetLastError());
        return -EIO;
    }

    return 0;
}


int uart_getch(void)
{
    DWORD dwRead;
    data8_t value;

    if (!ReadFile(uart_hCom, &value, 1, &dwRead, NULL)) {
        ERRMSG("%s: ReadFile error %lu\n", __FUNCTION__, GetLastError());
    }

    return dwRead ? value : -EIO;
}


int uart_putch(data8_t value)
{
    DWORD dwWrote;

    if (!WriteFile(uart_hCom, &value, 1, &dwWrote, NULL)) {
        ERRMSG("%s: WriteFile error %lu\n", __FUNCTION__, GetLastError());
        return -EIO;
    }

    return 0;
}
