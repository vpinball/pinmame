#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "snd_cmd.h"
#include "s67s.h"

struct DACinterface      s67s_dacInt     = { 1, { 50 }};
struct hc55516_interface s67s_hc55516Int = { 1, { 80 }};

static struct {
  int lastCmd;
} s67slocals;
static void s67s_cmd_sync(int data) {
  pia_set_input_b(5,data);
  pia_set_input_cb1(5, !((data & 0x1f) == 0x1f));
}

WRITE_HANDLER(s67s_cmd) {
  data &= 0x1f;
  if (coreGlobals.soundEn && (data != s67slocals.lastCmd)) {
	snd_cmd_log(data); data |= core_getDip(0)<<5;
    timer_set(TIME_NOW, data, s67s_cmd_sync);
  }
}

static void s7s_piaIrq(int state) {
  cpu_set_irq_line(S67S_CPUNO, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct pia6821_interface s67s_pia_intf = {
 /* PIA5  (0400)
    PA0-7  DAC
    PB0-7  Sound input
    CB1    Sound input != 0x1f
    CB2    Speech clock
    CA2    Speech data
    CA1    NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0/*soundlatch_r*/, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ DAC_0_data_w, 0, hc55516_0_digit_w, hc55516_0_clock_w,
 /* irq : A/B             */ s7s_piaIrq, s7s_piaIrq
};

void s67s_init(void) {
  pia_config(5, PIA_STANDARD_ORDERING, &s67s_pia_intf);
}

void s67s_exit(void) {}

/*---------------------------
/  Memory map
/----------------------------*/
MEMORY_READ_START(s67s_readmem )
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0400, 0x0403, pia_5_r },
  { 0x3000, 0x7fff, MRA_ROM },
  { 0x8400, 0x8403, pia_5_r },
  { 0xb000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(s67s_writemem )
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0400, 0x0403, pia_5_w },
  { 0x3000, 0x7fff, MWA_ROM },
  { 0x8400, 0x8403, pia_5_w },
  { 0xb000, 0xffff, MWA_ROM },
MEMORY_END

