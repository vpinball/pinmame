#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6800/m6800.h"
#include "sound/2151intf.h"
#include "sound/hc55516.h"
#include "machine/6821pia.h"
#include "sound/dac.h"
#include "core.h"
#include "sndbrd.h"
#include "s11.h"
#include "s11csoun.h"

#define S11CS_PIA0    7
#define S11CS_BANK0   4

static void s11cs_ym2151IRQ(int state);
static void s11cs_piaIrqA(int state);
static void s11cs_piaIrqB(int state);
static WRITE_HANDLER(s11cs_pia0ca2_w);
static WRITE_HANDLER(s11cs_pia0cb2_w);
static WRITE_HANDLER(s11cs_rombank_w);
static void s11cs_init(struct sndbrdData *brdData);

struct DACinterface s11_dacInt2 = { 2, { 50,50 }},
                    s11_dacInt  = { 1, { 50 }};

struct hc55516_interface s11_hc55516Int2 = { 2, { 80,80 }},
                         s11_hc55516Int  = { 1, { 80 }};

struct YM2151interface s11cs_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { s11cs_ym2151IRQ }
};

const struct sndbrdIntf s11csIntf = {
  s11cs_init, NULL, NULL,
  soundlatch2_w, NULL,
  CAT3(pia_,S11CS_PIA0,_cb1_w), NULL
};

static struct {
  struct sndbrdData brdData;
} locals;

/*--------------
/  Memory maps
/---------------*/
MEMORY_READ_START(s11cs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2001, 0x2001, YM2151_status_port_0_r }, /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_r(S11CS_PIA0) },      /* 4000-4fff */
  { 0x8000, 0xffff, MRA_BANKNO(S11CS_BANK0) },
MEMORY_END

MEMORY_WRITE_START(s11cs_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, YM2151_register_port_0_w },     /* 2000-2ffe even */
  { 0x2001, 0x2001, YM2151_data_port_0_w },         /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_w(S11CS_PIA0) },            /* 4000-4fff */
  { 0x6000, 0x6000, hc55516_0_digit_clock_clear_w },/* 6000-67ff */
  { 0x6800, 0x6800, hc55516_0_clock_set_w },        /* 6800-6fff */
  { 0x7800, 0x7800, s11cs_rombank_w },              /* 7800-7fff */
MEMORY_END

static const struct pia6821_interface s11cs_pia = {
 /* PIA 0 (4000) */
 /* PA0 - PA7 DAC */
 /* PB0 - PB7 CPU interface (MDx) */
 /* CA1       YM2151 IRQ */
 /* CB1       (I) CPU interface (MCB2) */
 /* CA2       YM 2151 pin 3 (Reset ?) */
 /* CB2       CPU interface (MCB1) */
 /* in  : A/B,CA/B1,CA/B2 */
  0, soundlatch2_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */
  DAC_0_data_w, soundlatch3_w, s11cs_pia0ca2_w, s11cs_pia0cb2_w,
 /* irq : A/B             */
  s11cs_piaIrqA, s11cs_piaIrqB
};

static WRITE_HANDLER(s11cs_rombank_w) {
  cpu_setbank(S11CS_BANK0, locals.brdData.romRegion + 0x10000*(data & 0x03) + 0x8000*((data & 0x04)>>2));
}
static void s11cs_init(struct sndbrdData *brdData) {
  locals.brdData = *brdData;
  pia_config(S11CS_PIA0, PIA_STANDARD_ORDERING, &s11cs_pia);
  cpu_setbank(S11CS_BANK0, locals.brdData.romRegion);
}

static WRITE_HANDLER(s11cs_pia0ca2_w) { if (!data) YM2151_sh_reset(); }
static WRITE_HANDLER(s11cs_pia0cb2_w) { sndbrd_data_cb(locals.brdData.boardNo,data); }

static void s11cs_ym2151IRQ(int state) { pia_set_input_ca1(S11CS_PIA0, !state); }
static void s11cs_piaIrqA(int state) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void s11cs_piaIrqB(int state) {
  cpu_set_nmi_line(locals.brdData.cpuNo, state ? ASSERT_LINE : CLEAR_LINE);
}

/*----------------------------
/  S11 CPU board sound
/-----------------------------*/
#define S11S_PIA0  6
#define S11S_BANK0 1
#define S11S_BANK1 2

static void s11s_init(struct sndbrdData *brdData);
static void s11s_diag(int button);
static WRITE_HANDLER(s11s_bankSelect);
const struct sndbrdIntf s11sIntf = {
  s11s_init, NULL, s11s_diag, soundlatch_w, NULL, CAT3(pia_,S11S_PIA0,_ca1_w), NULL
};

MEMORY_READ_START(s11s_readmem)
  { 0x0000, 0x0fff, MRA_RAM},
  { 0x2000, 0x2003, pia_r(S11S_PIA0) },
  { 0x8000, 0xbfff, MRA_BANKNO(S11S_BANK0)}, /* U22 */
  { 0xc000, 0xffff, MRA_BANKNO(S11S_BANK1)}, /* U21 */
MEMORY_END
MEMORY_WRITE_START(s11s_writemem)
  { 0x0000, 0x0fff, MWA_RAM },
  { 0x1000, 0x1000, s11s_bankSelect},
  { 0x2000, 0x2003, pia_w(S11S_PIA0)},
  { 0x8000, 0xffff, MWA_ROM},
MEMORY_END
MEMORY_READ_START(s9s_readmem)
  { 0x0000, 0x0fff, MRA_RAM},
  { 0x2000, 0x2003, pia_r(S11S_PIA0)},
  { 0x8000, 0xffff, MRA_ROM}, /* U22 */
MEMORY_END
MEMORY_WRITE_START(s9s_writemem)
  { 0x0000, 0x0fff, MWA_RAM },
  { 0x2000, 0x2003, pia_w(S11S_PIA0)},
  { 0x8000, 0xffff, MWA_ROM}, /* U22 */
MEMORY_END

static void s11s_piaIrq(int state);
static const struct pia6821_interface s11s_pia[] = {{
 /* PIA 0 (sound 2000) S11 */
 /* PA0 - PA7 (I) Sound Select Input (soundlatch) */
 /* PB0 - PB7 DAC */
 /* CA1       (I) Sound H.S */
 /* CB1       (I) 1ms */
 /* CA2       55516 Clk */
 /* CB2       55516 Dig */
 /* in  : A/B,CA/B1,CA/B2 */ soundlatch_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, DAC_0_data_w, hc55516_0_clock_w, hc55516_0_digit_w,
 /* irq : A/B             */ s11s_piaIrq, s11s_piaIrq
},{
 /* in  : A/B,CA/B1,CA/B2 */ soundlatch_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, DAC_1_data_w, hc55516_1_clock_w, hc55516_1_digit_w,
 /* irq : A/B             */ s11s_piaIrq, s11s_piaIrq
}};

static void s11s_init(struct sndbrdData *brdData) {
  locals.brdData = *brdData;
  pia_config(S11S_PIA0, PIA_STANDARD_ORDERING, &s11s_pia[locals.brdData.subType]);
  if (locals.brdData.subType) {
    cpu_setbank(S11S_BANK0,  locals.brdData.romRegion+0xc000);
    cpu_setbank(S11S_BANK1,  locals.brdData.romRegion+0x4000);
  }
}

static WRITE_HANDLER(s11s_bankSelect) {
  cpu_setbank(S11S_BANK0, locals.brdData.romRegion + 0x8000+((data&0x01)<<14));
  cpu_setbank(S11S_BANK1, locals.brdData.romRegion + 0x0000+((data&0x02)<<13));
}

static void s11s_piaIrq(int state) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void s11s_diag(int button) {
  cpu_set_nmi_line(locals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}
