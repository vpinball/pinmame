#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"
#include "sound/hc55516.h"
#include "machine/6821pia.h"
#include "sound/dac.h"
#include "core.h"
#include "s11.h"
#include "s11csoun.h"

/*-- bank handler --*/
static WRITE_HANDLER(s11cs_rombank_w);

static WRITE_HANDLER(pia7ca2_w) { if (!data) YM2151_sh_reset(); }
static WRITE_HANDLER(pia7cb2_w) { pia_set_input_cb1(5,data); }

static void s11cs_ym2151IRQ(int state);
static void s11cs_piaIrqA(int state);
static void s11cs_piaIrqB(int state);

/*--------------
/  Memory maps
/---------------*/
MEMORY_READ_START(s11cs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2001, 0x2001, YM2151_status_port_0_r }, /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_7_r },                 /* 4000-4fff */
  { 0x8000, 0xffff, MRA_BANK4 },
MEMORY_END

MEMORY_WRITE_START(s11cs_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, YM2151_register_port_0_w },     /* 2000-2ffe even */
  { 0x2001, 0x2001, YM2151_data_port_0_w },         /* 2001-2fff odd */
  { 0x4000, 0x4003, pia_7_w },                       /* 4000-4fff */
  { 0x6000, 0x6000, hc55516_0_digit_clock_clear_w },/* 6000-67ff */
  { 0x6800, 0x6800, hc55516_0_clock_set_w },        /* 6800-6fff */
  { 0x7800, 0x7800, s11cs_rombank_w },              /* 7800-7fff */
MEMORY_END

struct pia6821_interface s11cs_pia_intf = {
 /* PIA 7 (4000) */
 /* PA0 - PA7 DAC */
 /* PB0 - PB7 CPU interface (MDx) */
 /* CA1       YM2151 IRQ */
 /* CB1       (I) CPU interface (MCB2) */
 /* CA2       YM 2151 pin 3 (Reset ?) */
 /* CB2       CPU interface (MCB1) */
 /* in  : A/B,CA/B1,CA/B2 */
  0, soundlatch2_r, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */
  DAC_0_data_w, soundlatch3_w, pia7ca2_w, pia7cb2_w,
 /* irq : A/B             */
  s11cs_piaIrqA, s11cs_piaIrqB
};

/*-------------
/ Bank handler
/--------------*/
static WRITE_HANDLER(s11cs_rombank_w) {
#ifdef MAME_DEBUG
  /* this register can not be read but this makes debugging easier */
  *(memory_region(S11_MEMREG_SCPU1) + 0x2000) = data;
#endif /* MAME_DEBUG */
  cpu_setbank(4, memory_region(S11_MEMREG_SROM1) +
              0x10000*(data & 0x03) + 0x8000*((data & 0x04)>>2));
}

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface s11cs_dacInt =
  { 1, { 50 }};

struct YM2151interface s11cs_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { s11cs_ym2151IRQ }
};

struct hc55516_interface s11cs_hc55516Int =
  { 1, { 80 }};

void s11cs_init(void) {
  pia_config(7, PIA_STANDARD_ORDERING, &s11cs_pia_intf);
  cpu_setbank(4, memory_region(S11_MEMREG_SROM1));
}
void s11cs_exit(void) {}

/*-----------------
/  local functions
/------------------*/
static void s11cs_ym2151IRQ(int state) {
  pia_set_input_ca1(7, !state);
}
static void s11cs_piaIrqA(int state) {
  cpu_set_irq_line(S11_SCPU1NO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void s11cs_piaIrqB(int state) {
  cpu_set_nmi_line(S11_SCPU1NO, state ? ASSERT_LINE : CLEAR_LINE);
}


