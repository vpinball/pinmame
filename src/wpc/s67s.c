#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "s67s.h"

#define S67S_PIA0 5

static void s67s_init(struct sndbrdData *brdData);
static WRITE_HANDLER(s67s_cmd_w);
static void s67s_diag(int button);

const struct sndbrdIntf s67sIntf = {
  s67s_init, NULL, s67s_diag, s67s_cmd_w, NULL, NULL, NULL, SNDBRD_NOCTRLSYNC
};

struct DACinterface      s67s_dacInt     = { 1, { 50 }};
struct hc55516_interface s67s_hc55516Int = { 1, { 80 }};

MEMORY_READ_START(s67s_readmem )
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0400, 0x0403, pia_r(S67S_PIA0) },
  { 0x3000, 0x7fff, MRA_ROM },
  { 0x8400, 0x8403, pia_r(S67S_PIA0) },
  { 0xb000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(s67s_writemem )
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0400, 0x0403, pia_w(S67S_PIA0) },
  { 0x3000, 0x7fff, MWA_ROM },
  { 0x8400, 0x8403, pia_w(S67S_PIA0) },
  { 0xb000, 0xffff, MWA_ROM },
MEMORY_END

static void s67s_piaIrq(int state);
static const struct pia6821_interface s67s_pia = {
 /* PIA0  (0400)
    PA0-7  DAC
    PB0-7  Sound input
    CB1    Sound input != 0x1f
    CB2    Speech clock
    CA2    Speech data
    CA1    NC */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ DAC_0_data_w, 0, hc55516_0_digit_w, hc55516_0_clock_w,
 /* irq : A/B             */ s67s_piaIrq, s67s_piaIrq
};

static struct {
  struct sndbrdData brdData;
} s67slocals;

static void s67s_init(struct sndbrdData *brdData) {
  s67slocals.brdData = *brdData;
  pia_config(S67S_PIA0, PIA_STANDARD_ORDERING, &s67s_pia);
}

static WRITE_HANDLER(s67s_cmd_w) {
  data = (data & 0x1f) | (core_getDip(0)<<5);
  pia_set_input_b(S67S_PIA0, data);
  pia_set_input_cb1(S67S_PIA0, !((data & 0x1f) == 0x1f));
  snd_cmd_log(data);
}
static void s67s_diag(int button) {
  cpu_set_nmi_line(s67slocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static void s67s_piaIrq(int state) {
  cpu_set_irq_line(s67slocals.brdData.cpuNo, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

