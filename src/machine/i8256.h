// license:BSD-3-Clause

/**********************************************************************************************
 *
 *   Intel 8256 / 8256AH MUART
 *   Multifunction Universal Asynchronous Receiver/Transmitter
 *
 *   Serial port + two parallel ports + five counter/timers + an eight level
 *   interrupt controller in one 40 pin package.
 *
 *   Written for PinMAME from the 8256AH datasheet (Intel Microsystem Components
 *   Handbook 1984, vol. 2 pp. 141-163), with the register layout and the overall
 *   shape of the device taken from MAME's i8256.cpp (BSD-3-Clause,
 *   copyright-holders:stonedDiscord).
 *
 *   Used by the Unidesa/Stargame 8088 boards: Mephisto and Cirsa Sport 2000.
 *
 **********************************************************************************************/

#ifndef INC_I8256
#define INC_I8256

#include "driver.h"

/*-- interrupt levels, highest priority first (datasheet table) --*/
#define I8256_INT_TIMER1  0   /* L0 */
#define I8256_INT_TIMER2  1   /* L1 - or Port 1 P17 edge when CMD1.BITI is set */
#define I8256_INT_EXTINT  2   /* L2 */
#define I8256_INT_TIMER3  3   /* L3 - or timers 3+5 cascaded */
#define I8256_INT_RX      4   /* L4 */
#define I8256_INT_TX      5   /* L5 */
#define I8256_INT_TIMER4  6   /* L6 - or timers 2+4 cascaded */
#define I8256_INT_TIMER5  7   /* L7 */

typedef struct {
  /* INT pin.  Called with 1 to request an interrupt, 0 to release it. */
  void  (*int_out)(int state);
  /* Port 1 / Port 2 pins.  The *_in callbacks supply the level on the pins
     configured as inputs -- by PORT1C for port 1 and by MODE.P2C for port 2;
     bits configured as outputs are taken from the internal latch and the
     callback's value for those bits is ignored.  Unconnected inputs should
     read back as 0. */
  UINT8 (*p1_in)(void);
  void  (*p1_out)(UINT8 data);
  UINT8 (*p2_in)(void);
  void  (*p2_out)(UINT8 data);
  /* A byte written to the transmit buffer.  May be NULL. */
  void  (*txd_out)(UINT8 data);
} I8256interface;

/* clock is the frequency at the CLK pin, in Hz.  The chip divides it by the
   CMD2 system-clock prescaler (C1,C0 = bits 5,4: 5 / 3 / 2 / 1), and that by
   64 (CMD1.FRQ = 0) or 1024 (FRQ = 1), to make the time base for all five
   counter/timers.  The datasheet's 16 kHz / 1 kHz hold only for a board whose
   CLK makes the prescaler output 1.024 MHz, so pass the real crystal */
void i8256_init(const I8256interface *intf, UINT32 clock);
void i8256_reset(void);

WRITE_HANDLER(i8256_w);
READ_HANDLER (i8256_r);

/* Interrupt acknowledge.  Returns the vector the MUART puts on the bus:
   0x40 + level in 8086 mode, an RSTn opcode in 8085 mode.  Install this from
   the driver with cpu_set_irq_callback(). */
int  i8256_inta(void);

/* EXTINT pin (level sensitive, active high) -> interrupt level 2. */
void i8256_set_extint(int state);

/* Drive one Port 1 input pin.  P17 is edge triggered and raises level 1 when
   CMD1.BITI is set, so power-fail and similar inputs go through here.  These
   pins are OR'd with whatever p1_in returns, so a driver may use either
   route, or both for different bits. */
void i8256_set_p1_pin(int bit, int state);

/* A byte arriving on RxD -> receive buffer, raises level 4. */
void i8256_receive(UINT8 data);

/* True while the chip is in 8086 mode (CMD1 bit 1), i.e. registers are
   selected by A1..A4 with A0 as a second, active-low chip select. */
int  i8256_is_8086_mode(void);

#endif /* INC_I8256 */
