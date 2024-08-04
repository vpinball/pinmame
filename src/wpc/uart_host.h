/*  Platform-specific host code to connect with an actual serial port,
    Provided by either windows/serial.c or unix/serial.c.

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/

#include "memory.h"

int uart_open(const char *device, data32_t baudrate);
int uart_baudrate(data32_t baudrate);
int uart_getch(void);
int uart_putch(data8_t value);
