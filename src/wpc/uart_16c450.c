/*
    Code for emulating the 16C450 UART found on the WPC95-AV board.

    This is a bare-bones emulator that only supports the features used by
    NBA Fastbreak and the printout features from the service menu.  For
    example, it does not include any code for interrupts.

    But, unlike the 8251 UART emulator, we attempt to simulate the baudrate
    for outbound bytes in case it matters for the Championship Link feature
    of NBA Fastbreak.

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "uart_16c450.h"
#include "uart_host.h"

#include "timer.h"

// Set to 1 to emulate the time it would take to transmit a byte at the given baudrate.
#define EMULATE_TX_TIME         0

#if 0
#define DEBUG_OUT(...)  printf(__VA_ARGS__)
#else
#define DEBUG_OUT(...)
#endif

enum uart_16c450_reg {
    RBR = 0,    /* DLAB=0, read-only */
    THR = 0,    /* DLAB=0, write-only */
    IER = 1,    /* DLAB=0 */
    DLL = 0,    /* DLAB=1 */
    DLM = 1,    /* DLAB=1 */
    IIR = 2,    /* read-only */
    LCR = 3,
    MCR = 4,
    LSR = 5,
    MSR = 6,
    SCR = 7,
};

static struct {
    data8_t reg_RBR;
    //data8_t reg_THR;
    data8_t reg_IER;
    /*const*/ data8_t reg_IIR;
    data8_t reg_LCR;
    data8_t reg_MCR;
    data8_t reg_LSR;
    data8_t reg_MSR;
    data8_t reg_SCR;

    data16_t reg_divisor;
    data32_t byte_time_us;       // time (in uS) to send a byte at the current baudrate

#if EMULATE_TX_TIME
    double THR_clear;
    double TX_clear;
#endif
} locals;

#define REGBIT(reg, bit)    ((locals.reg_ ## reg >> bit) & 1)

#define ERBF    REGBIT(IER, 0)
#define ETBE    REGBIT(IER, 1)
#define ELSI    REGBIT(IER, 2)
#define EDSSI   REGBIT(IER, 3)

#define WLS     (locals.reg_LCR & 0x03)
#define STB     REGBIT(LCR, 2)
#define PEN     REGBIT(LCR, 3)
#define EPS     REGBIT(LCR, 4)
#define MRKSPC  REGBIT(LCR, 5)
#define BREAK   REGBIT(LCR, 6)
#define DLAB    REGBIT(LCR, 7)

#define DR      REGBIT(LSR, 0)
#define OE      REGBIT(LSR, 1)
#define PE      REGBIT(LSR, 2)
#define FE      REGBIT(LSR, 3)
#define BI      REGBIT(LSR, 4)
#define THRE    REGBIT(LSR, 5)
#define TEMT    REGBIT(LSR, 6)


void uart_16c450_reset()
{
    memset(&locals, 0, sizeof(locals));
    locals.reg_IIR = 0x01; // const
    locals.reg_LSR = 0x60;

#if EMULATE_TX_TIME
    THR_clear = TX_clear = timer_get_time();
#endif
}

const char *uart_16c450_regname(int reg, int write)
{
    switch (reg) {
    case IIR:       return write ? "err" : "IIR";   // IIR is read-only
    case LCR:       return "LCR";
    case MCR:       return "MCR";
    case LSR:       return "LSR";
    case MSR:       return "MSR";
    case SCR:       return "SCR";
    default:
        if (DLAB) switch (reg) {
        case DLL:   return "DLL";
        case DLM:   return "DLM";
        }
        else switch (reg) {
        case THR:   return write ? "THR" : "RBR";
        case IER:   return "IER";
        }
    }

    return "???";
}


static data8_t update_LSR(void)
{
#if EMULATE_TX_TIME
    double now = timer_get_time();
    if (now > locals.THR_clear) {
        locals.reg_LSR |= (1 << 5);        // set THRE (Transmit Holding Register Empty)
    }
    if (now > locals.TX_clear) {
        locals.reg_LSR |= (1 << 6);        // set TEMPT (Transmitter Empty)
    }
#else
    // We can accept another byte, so set both THRE and TEMPT.
    locals.reg_LSR |= ((1 << 5) | (1 << 6));
#endif

    // If we don't already have a byte waiting in RBR, attempt to load one
    if (!DR) {
        int byte_read = uart_getch();
        if (byte_read >= 0) {
            locals.reg_RBR = (data8_t) byte_read;
            locals.reg_LSR |= (1 << 0);    // set DR (Data Ready)
        }
    }

    return locals.reg_LSR;
}

static void uart_send(data8_t value)
{
#if EMULATE_TX_TIME
    // Set up timers to simulate the time necessary to clear the Transmit Holding
    // Register and the actual transmitter shift register.
    double now = timer_get_time();
    if (now > locals.TX_clear) {
        // can go straight into transmitter
        locals.TX_clear = now + TIME_IN_USEC(locals.byte_time_us);
        locals.reg_LSR &= ~(1 << 6);         // clear transmitter empty (TEMPT) bit
    }
    else if (now > locals.THR_clear) {
        // Byte goes into Transmit Holding Register, then passed to transmitter once
        // it's done sending the current byte.
        locals.THR_clear = locals.TX_clear;

        // transmitter now has an extra byte to send
        locals.TX_clear += TIME_IN_USEC(locals.byte_time_us);

        // clear holding register & transmitter empty (THRE/TEMPT) bits
        locals.reg_LSR &= ~(1 << 5 | 1 << 6);
    }
    else {
        // record an error?  overwrite the outbound byte?
        DEBUG_OUT("uart: sending when THR in use\n");
    }
#endif

    uart_putch(value);          // Send the byte out on the UART interface
}

data8_t uart_16c450_read(int reg)
{
    //DEBUG_OUT("r %s\n", uart_16c450_regname(reg, 0));

    switch (reg) {
    case IIR:       return locals.reg_IIR;
    case LCR:       return locals.reg_LCR;
    case MCR:       return locals.reg_MCR;
    case LSR:       return update_LSR();
    case MSR:       return locals.reg_MSR;
    case SCR:       return locals.reg_SCR;
    default:
        if (DLAB) switch (reg) {
        case DLL:   return locals.reg_divisor >> 8;
        case DLM:   return locals.reg_divisor & 0xFF;
        }
        else switch (reg) {
        case RBR:   // read stored byte
            locals.reg_LSR &= ~0x01;       // clear data ready (DR) bit
            return locals.reg_RBR;
        case IER:   return locals.reg_IER;
        }
    }
    DEBUG_OUT("uart: unhandled register\n");
    return 0;
}

int uart_16c450_write(int reg, data8_t value)
{
    if (reg < 0 || reg > 7) {
        return -EINVAL;
    }
//    DEBUG_OUT("w %s=0x%02x\n", uart_16c450_regname(reg, 1), value);
    switch (reg) {
    case LCR:
        if ((locals.reg_LCR & 0x40) != (value & 0x40)) {
            DEBUG_OUT(" uart %s break\n", (value & 0x40) ? "set" : "clear");
        }
        if ((locals.reg_LCR & 0x80) != (value & 0x80)) {
            DEBUG_OUT(" uart %s DLAB\n", (value & 0x80) ? "set" : "clear");
        }
        locals.reg_LCR = value;
        DEBUG_OUT(" uart %u bits, %u stop, %s parity\n",
                  5 + WLS, 1 + STB,
                  PEN ? (MRKSPC ? (EPS ? "mark" : "space") : (EPS ? "even" : "odd")) : "no");
        break;

    case MCR:       locals.reg_MCR = value;    break;
    case LSR:
        // The line status register is intended for read operations only;
        // writing to this register is not recommended outside of a factory
        // testing environment.
        //locals.reg_LSR = value;   
        break;

    case MSR:       locals.reg_MSR = value;    break;
    case SCR:       locals.reg_SCR = value;    break;

    default:
        if (DLAB) {
            switch (reg) {
            case DLL:
                locals.reg_divisor = (locals.reg_divisor & 0xFF00) | value;
                break;
            case DLM:
                locals.reg_divisor = (locals.reg_divisor & 0x00FF) | (value << 8);
                break;
            }
            if (locals.reg_divisor) {
                // 2MHz system clock, 16x oversampling by UART
                int baudrate = 2000000 / (16 * locals.reg_divisor);

                // estimate byte time based on 10 bits/byte (start + data + stop)
                // (1,000,000 us/s) / (baudrate bits/s / 10 bits/byte)
                locals.byte_time_us = 1000000 / (baudrate / 10);

                DEBUG_OUT("divisor=%u (%ubps)\n", locals.reg_divisor, baudrate);

                // round baudrate to a multiple of 100 for our serial port
                baudrate = ((baudrate + 50) / 100) * 100;
                uart_baudrate(baudrate);
            }
        }
        else switch (reg) {
        case THR:   uart_send(value); break;
        case IER:   locals.reg_IER = value; break;
        }
    }

    return 0;
}
