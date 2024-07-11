/*
    Code for emulating the 16C450 UART found on the WPC95-AV board.

    This is a bare-bones emulator that only supports the features used by
    NBA Fastbreak and the printout features from the service menu.  For
    example, it does not include any code for interrupts.

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/
#include "memory.h"

// 16C450 emulation
data8_t uart_16c450_read(int reg);
int uart_16c450_write(int reg, data8_t value);

/*  Platform-specific code to connect with an actual serial port,
    Provided by either windows/serial.c or unix/serial.c.
*/
int uart_open(const char *device, data32_t baudrate);
int uart_baudrate(data32_t baudrate);
int uart_getch(void);
int uart_putch(data8_t value);
