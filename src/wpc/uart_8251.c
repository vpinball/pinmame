/*
    Code for emulating the 8251 UART found on the WPC Printer Option Kit.

    This is a bare-bones emulator that only supports the features used by
    the printout features from the service menu.  There isn't any attempt
    to simulate the baudrate setting.  The emulated UART is always ready
    to receive bytes, and hands them off to the host UART immediately.

    Written July 2024 by Tom Collins <tom@tomlogic.com>
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "uart_8251.h"
#include "uart_host.h"

#if 0
#define DEBUG_OUT(...)  printf(__VA_ARGS__)
#else
#define DEBUG_OUT(...)
#endif

// 8251 UART mapped to address WPC_BASE + 0x13 (DATA) and WPC_BASE + 0x14 (CTRL)
#define REG_DATA    0x13
#define REG_CTRL    0x14

// The kit's baud rate generator is a separate circuit, but we handle it here as well.
// Writes to REG_BAUD are a clock divider.  0=9600, 1=4800, 2=2400, 3=1200, 4=600, 5=300.
#define REG_BAUD    0x15

#define MODE_BAUD(m)        ((m >> 0) & 0x03)
#define MODE_LENGTH(m)      ((m >> 2) & 0x03)
#define MODE_PARITY_EN(m)   ((m >> 4) & 0x01)
#define MODE_PARITY_EVEN(m) ((m >> 5) & 0x01)

// If MODE_BAUD(m) != 0, we're in asynchronous mode.
#define MODE_STOP(m)        ((m >> 6) & 0x03)

// If MODE_BAUD(m) == 0, we're in synchronous mode.
#define MODE_SYNC_DETECT(m) ((m >> 6) & 0x01)
#define MODE_SINGLE_SYNC(m) ((m >> 7) & 0x01)

// Macros to parse bits in the command byte sent from the host.
#define CMD_TX_ENABLE(c)    ((c >> 0) & 0x01)
#define CMD_DTR_ENABLE(c)   ((c >> 1) & 0x01)
#define CMD_RX_ENABLE(c)    ((c >> 2) & 0x01)
#define CMD_SEND_BREAK(c)   ((c >> 3) & 0x01)
#define CMD_ERROR_RESET(c)  ((c >> 4) & 0x01)
#define CMD_RTS_ENABLE(c)   ((c >> 5) & 0x01)
#define CMD_CHIP_RESET(c)   ((c >> 6) & 0x01)
#define CMD_ENTER_HUNT(c)   ((c >> 7) & 0x01)


// Bitmasks for setting the status byte sent to the host
#define STATUS_TX_READY     (1 >> 0)
#define STATUS_RX_READY     (1 >> 1)
#define STATUS_TX_EMPTY     (1 >> 2)
#define STATUS_PARITY_ERR   (1 >> 3)
#define STATUS_OVERRUN_ERR  (1 >> 4)
#define STATUS_FRAME_ERR    (1 >> 5)
#define STATUS_SYNC_DET     (1 >> 6)
#define STATUS_DSR          (1 >> 7)


static struct {
    int mode_loaded;

    data8_t mode;
    data8_t command;
    data8_t rx_byte;
    data8_t status;
    int baud;
} locals;

void uart_8251_reset()
{
    memset(&locals, 0, sizeof(locals));
    locals.status = (STATUS_DSR | STATUS_TX_READY | STATUS_TX_EMPTY);
    locals.baud = 9600;
}

data8_t uart_8251_read(int reg)
{
//    DEBUG_OUT("r 0x%x\n", reg);
    switch (reg) {
    case REG_CTRL:
        // if we don't already have a byte ready, try to read one
        if (!(locals.status & STATUS_RX_READY)) {
            int byte_read = uart_getch();
            if (byte_read >= 0) {
                locals.rx_byte = (data8_t)byte_read;
                locals.status |= STATUS_RX_READY;
            }
        }
        return locals.status;

    case REG_DATA:
        // clear the RX_READY bit and return the buffered byte
        locals.status &= ~STATUS_RX_READY;
        return locals.rx_byte;
    }

    // ROM should never read REG_BAUD.  We need a copy of the schematic to
    // know how actual hardware would respond.
    return 0x00;
}

int uart_8251_write(int reg, data8_t value)
{
    switch (reg) {
        case REG_CTRL:
            if (!locals.mode_loaded) {
                locals.mode_loaded = 1;
                locals.mode = value;
                DEBUG_OUT("mode=0x%02x\n", locals.mode);
            }
            else {
                locals.command = value;
                if (CMD_CHIP_RESET(locals.command)) {
                    // next write will set a new mode
                    locals.mode_loaded = 0;
                }
                DEBUG_OUT("cmd=0x%02x\n", locals.command);
            }
            break;

        case REG_DATA:
            uart_putch(value);
            if (isprint(value) || value == '\r' || value == '\n') {
                DEBUG_OUT("%c", value);
            }
            else {
                DEBUG_OUT("\nprt 0x%x\n", value);
            }
            break;

        case REG_BAUD:
            locals.baud = 9600 / (1 << value);
            DEBUG_OUT("%ubps\n", locals.baud);
            break;
    }
    return 0;
}
