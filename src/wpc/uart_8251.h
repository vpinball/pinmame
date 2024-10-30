/*
    Code for emulating the 8251 UART found on the WPC Printer Option Kit.

    This is a bare-bones emulator that only supports the features used by
    the printout features from the service menu. 

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/
#include "memory.h"

// 8251 emulation
void uart_8251_reset();
data8_t uart_8251_read(int reg);
int uart_8251_write(int reg, data8_t value);
