/*
    Code for emulating the 16C450 UART found on the WPC95-AV board.

    This is a bare-bones emulator that only supports the features used by
    NBA Fastbreak and the printout features from the service menu.  For
    example, it does not include any code for interrupts.

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/
#include "memory.h"

// 16C450 emulation
void uart_16c450_reset();
data8_t uart_16c450_read(int reg);
int uart_16c450_write(int reg, data8_t value);
