#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"
#include "sound/hc55516.h"
#include "sound/dac.h"
#include "sndbrd.h"
#include "wpc.h"
#include "wpcsound.h"

#define WPCS_BANK0  4

/*-- internal sound interface --*/
static WRITE_HANDLER(wpcs_latch_w);
static READ_HANDLER(wpcs_latch_r);

/*-- external interfaces --*/
static void wpcs_init(struct sndbrdData *brdData);
static READ_HANDLER(wpcs_data_r);
static WRITE_HANDLER(wpcs_data_w);
static READ_HANDLER(wpcs_ctrl_r);
static WRITE_HANDLER(wpcs_ctrl_w);
const struct sndbrdIntf wpcsIntf = { wpcs_init, NULL, NULL, wpcs_data_w, wpcs_data_r, wpcs_ctrl_w, wpcs_ctrl_r };

/*-- other memory handlers --*/
static WRITE_HANDLER(wpcs_rombank_w);
static WRITE_HANDLER(wpcs_volume_w);
static void wpcs_ym2151IRQ(int state);

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  int replyAvail;
  int volume;
} locals;

static WRITE_HANDLER(wpcs_rombank_w) {
  /* the hardware can actually handle 1M chip but no games uses it */
  /* if such ROM appear the region must be doubled and mask set to 0x1f */
  /* this would be much easier if the region was filled in opposit order */
  /* but I don't want to change it now */
  int bankBase = data & 0x0f;
#ifdef MAME_DEBUG
  /* this register can no be read but this makes debugging easier */
  *(memory_region(REGION_CPU1+locals.brdData.cpuNo) + 0x2000) = data;
#endif /* MAME_DEBUG */

  switch ((~data) & 0xe0) {
    case 0x80: /* U18 */
      bankBase |= 0x00; break;
    case 0x40: /* U15 */
      bankBase |= 0x10; break;
    case 0x20: /* U14 */
      bankBase |= 0x20; break;
    default:
      DBGLOG(("WPCS:Unknown bank %x\n",data)); return;
  }
  cpu_setbank(WPCS_BANK0, locals.brdData.romRegion + (bankBase<<15));
}

static WRITE_HANDLER(wpcs_volume_w) {
  if (data & 0x01) {
    if ((locals.volume > 0) && (data & 0x02))
      locals.volume -= 1;
    else if ((locals.volume < 0xff) && ((data & 0x02) == 0))
      locals.volume += 1;
    /* DBGLOG(("Volume set to %d\n",locals.volume)); */
    {
      int ch;
      for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++) {
        if (mixer_get_name(ch) != NULL)
          mixer_set_volume(ch, locals.volume * 100 / 127);
      }
    }
  }
}

static WRITE_HANDLER(wpcs_latch_w) {
  locals.replyAvail = TRUE; soundlatch2_w(0,data);
  sndbrd_data_cb(locals.brdData.boardNo, data);
}

static READ_HANDLER(wpcs_latch_r) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
  return soundlatch_r(0);
}

static void wpcs_ym2151IRQ(int state) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


/*-------------------
/ Exported interface
/--------------------*/
struct DACinterface wpcs_dacInt = { 1, { 50 }};
struct hc55516_interface wpcs_hc55516Int = { 1, { 80 }};

struct YM2151interface wpcs_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { wpcs_ym2151IRQ }
};

MEMORY_READ_START(wpcs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2401, 0x2401, YM2151_status_port_0_r }, /* 2401-27ff odd */
  { 0x3000, 0x3000, wpcs_latch_r }, /* 3000-33ff */
  { 0x4000, 0xbfff, CAT2(MRA_BANK, WPCS_BANK0) }, //32K
  { 0xc000, 0xffff, MRA_ROM }, /* same as page 7f */	//16K
MEMORY_END

MEMORY_WRITE_START(wpcs_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, wpcs_rombank_w }, /* 2000-23ff */
  { 0x2400, 0x2400, YM2151_register_port_0_w }, /* 2400-27fe even */
  { 0x2401, 0x2401, YM2151_data_port_0_w },     /* 2401-27ff odd */
  { 0x2800, 0x2800, DAC_0_data_w }, /* 2800-2bff */
  { 0x2c00, 0x2c00, hc55516_0_clock_set_w },  /* 2c00-2fff */
  { 0x3400, 0x3400, hc55516_0_digit_clock_clear_w }, /* 3400-37ff */
  { 0x3800, 0x3800, wpcs_volume_w }, /* 3800-3bff */
  { 0x3c00, 0x3c00, wpcs_latch_w },  /* 3c00-3fff */
MEMORY_END

/*---------------------
/  Interface functions
/----------------------*/
static READ_HANDLER(wpcs_data_r) {
  locals.replyAvail = FALSE; return soundlatch2_r(0);
}

static WRITE_HANDLER(wpcs_data_w) {
  soundlatch_w(0, data); cpu_set_irq_line(locals.brdData.cpuNo, M6809_IRQ_LINE, ASSERT_LINE);
}

static READ_HANDLER(wpcs_ctrl_r) {
  return locals.replyAvail;
}

static WRITE_HANDLER(wpcs_ctrl_w) { /*-- a write here resets the CPU --*/
  cpu_set_reset_line(locals.brdData.cpuNo, PULSE_LINE);
}

static void wpcs_init(struct sndbrdData *brdData) {
  locals.brdData = *brdData;
  /* the non-paged ROM is at the end of the image. move it to its correct place */
  memcpy(memory_region(REGION_CPU1+locals.brdData.cpuNo) + 0x00c000, locals.brdData.romRegion + 0x07c000, 0x4000);
  wpcs_rombank_w(0,0);
}

