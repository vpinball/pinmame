/*
    POSIX serial port code for uart_host.h based on the XBee ANSI C Library:
        https://github.com/tomlogic/xbee_ansic_library/

    Originally written by Tom Collins <tom@tomlogic.com> and released under
    an MPL 2.0 License.

    This PinMAME adaptation written July 2024 by Tom Collins.
*/

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "uart_host.h"

static int uart_fd;

#if 0
#define ERRMSG(...)      fprintf(stderr, __VA_ARGS__)
#else
#define ERRMSG(...)
#endif

// Pass "/dev/ttyXXX" as device.
int uart_open(const char *device, data32_t baudrate)
{
    int err, fd;
    
    fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        ERRMSG("%s: open('%s') failed (errno=%d)\n", __FUNCTION__,
            device, errno);
        return -errno;
    }

    // Configure file descriptor to not block on read() if there aren't
    // any characters available.
    fcntl(fd, F_SETFL, FNDELAY);

    uart_fd = fd;
    err = uart_baudrate(baudrate);
    if (err) {
        uart_fd = 0;
    }
    
    return err;
}


int uart_close(void)
{
    int result = 0;
    
    if (close(uart_fd) == -1)
    {
        ERRMSG("%s: close(%d) failed (errno=%d)\n", __FUNCTION__,
            uart_fd, errno);
        result = -errno;
    }
    uart_fd = -1;

    return result;
}


#define _BAUDCASE(b)        case b: baud = B ## b; break
int uart_baudrate(data32_t baudrate)
{
    struct termios options;
    speed_t baud;

    switch (baudrate)
    {
        _BAUDCASE(0);
        _BAUDCASE(300);
        _BAUDCASE(600);
        _BAUDCASE(1200);
        _BAUDCASE(2400);
        _BAUDCASE(4800);
        _BAUDCASE(9600);
        _BAUDCASE(19200);
        _BAUDCASE(38400);
        _BAUDCASE(57600);
        _BAUDCASE(115200);
#ifdef B230400
        _BAUDCASE(230400);
#endif
#ifdef B460800
        _BAUDCASE(460800);
#endif
#ifdef B921600
        _BAUDCASE(921600);
#endif
        default:
            return -EINVAL;
    }

    // Get the current options for the port...
    if (tcgetattr(uart_fd, &options) == -1)
    {
        ERRMSG("%s: %s failed (%d) for %" PRIu32 "\n",
            __FUNCTION__, "tcgetattr", errno, baudrate);
        return -errno;
    }

    // Set the baud rates...
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // disable any processing of serial input/output
    cfmakeraw(&options);

    // ignore modem status lines (blocked write() on macOS)
    options.c_cflag |= CLOCAL;

    // Set the new options for the port, waiting until buffered data is sent
    if (tcsetattr(uart_fd, TCSADRAIN, &options) == -1)
    {
        ERRMSG("%s: %s failed (%d)\n", __FUNCTION__, "tcsetattr", errno);
        return -errno;
    }

    return 0;
}


int uart_getch(void)
{
    data8_t value;

    if (read(uart_fd, &value, 1) == -1) {
        if (errno == EAGAIN) {
            return -EAGAIN;
        }
        ERRMSG("%s: error %d\n", __FUNCTION__, errno);
        return -errno;
    }

    return value;
}


int uart_putch(data8_t value)
{
    if (write(uart_fd, &value, 1) < 0) {
        ERRMSG("%s: error %d\n", __FUNCTION__, errno);
        return -errno;
    }
    
    return 0;
}
