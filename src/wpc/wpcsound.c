#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"
#include "sound/hc55516.h"
#include "sound/dac.h"
#include "wpc.h"
#include "wpcsound.h"

/* 161200  Changed FIRQ to use wpc_firq function, added wpcs_ctrl_w */
/* 091200  Updated bank handler and confirmed some ? from shematics */
/*         bank errors still show up sometimes but AFAIK these will */
/*         have more than one ROMchip enabled */
/*         Added FIRQ interrupt on main CPU when data is availble */
/* 181100  Implemented volume handler */
/* 281000  Added volume handler (not implemented) */
/*-- bank handler --*/
static WRITE_HANDLER(wpcs_rombank_w);

/*-- sound interface handlers --*/
static WRITE_HANDLER(wpcs_latch_w);
static READ_HANDLER(wpcs_latch_r);

/*-- other memory handlers --*/
static WRITE_HANDLER(wpcs_volume_w);

/*-- sound generation --*/
static void wpcs_ym2151IRQ(int state);

/*-- local data --*/
static struct {
  int replyAvail;
  int volume;
} locals;
/*--------------
/  Memory maps
/---------------*/
MEMORY_READ_START(wpcs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2401, 0x2401, YM2151_status_port_0_r }, /* 2401-27ff odd */
  { 0x3000, 0x3000, wpcs_latch_r }, /* 3000-33ff */
  { 0x4000, 0xbfff, MRA_BANK4 },			//32K
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

/*-------------
/ Bank handler
/--------------*/
static WRITE_HANDLER(wpcs_rombank_w) {
  /* the hardware can actually handle 1M chip but no games uses it */
  /* if such ROM appear the region must be doubled and mask set to 0x1f */
  /* this would be much easier if the region was filled in opposit order */
  /* but I don't want to change it now */
  int bankBase = data & 0x0f;
#ifdef MAME_DEBUG
  /* this register can no be read but this makes debugging easier */
  *(memory_region(WPC_MEMREG_SCPU) + 0x2000) = data;
#endif /* MAME_DEBUG */

  switch ((~data) & 0xe0) {
    case 0x80: /* U18 */
      bankBase |= 0x00; break;
    case 0x40: /* U15 */
      bankBase |= 0x10; break;
    case 0x20: /* U14 */
      bankBase |= 0x20; break;
    default:
      logerror("Unknown sound bank %x\n",data); return;
  }
#if 0
  int bankBase;
  /*-- save value for read --*/
  /* *(memory_region(WPC_MEMREG_SCPU) + 0x2000) = data; */

  if ((data >= 0x70) && (data <= 0x7f))
    bankBase = data & 0x0f;
  else if ((data >= 0xb0) && (data <= 0xbf))
    bankBase = (data & 0x0f) | 0x10;
  else if ((data >= 0xd0) && (data <= 0xdf))
    bankBase = (data & 0x0f) | 0x20;
  else {
    logerror("unknown sound bank %x\n",data);
    return;
  }
#endif
  cpu_setbank(4, memory_region(WPC_MEMREG_SROM) + (bankBase<<15));
}
/*----------------------
/ Other memory handlers
/-----------------------*/
static WRITE_HANDLER(wpcs_latch_w) {
  DBGLOG(("wpcs_latch_w: %2x (PC=%4x)\n",data,cpu_get_pc()));
  soundlatch2_w(0, data);
  locals.replyAvail = TRUE;
  wpc_firq(TRUE, WPC_FIRQ_SOUND);
}

static READ_HANDLER(wpcs_latch_r) {
  cpu_set_irq_line(WPC_SCPUNO, M6809_IRQ_LINE, CLEAR_LINE);
  return soundlatch_r(0);
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

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface wpcs_dacInt =
  { 1, { 50 }};

struct YM2151interface wpcs_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(10,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
  { wpcs_ym2151IRQ }
};

struct hc55516_interface wpcs_hc55516Int =
  { 1, { 80 }};

/*-------------------
/ Exported interface
/---------------------*/
READ_HANDLER(wpcs_data_r) {
  locals.replyAvail = FALSE;
  return soundlatch2_r(0);
}

WRITE_HANDLER(wpcs_data_w) {
  soundlatch_w(0, data);
  cpu_set_irq_line(WPC_SCPUNO, M6809_IRQ_LINE, ASSERT_LINE);
}

READ_HANDLER(wpcs_ctrl_r) {
  wpc_firq(FALSE, WPC_FIRQ_SOUND);
  return locals.replyAvail;
}

WRITE_HANDLER(wpcs_ctrl_w) {
  /*-- a write here resets the CPU --*/
  cpu_set_reset_line(WPC_SCPUNO, PULSE_LINE);
}

void wpcs_init(void) {
  /* the non-paged ROM is at the end of the image. move it to its correct place */
  memcpy(memory_region(WPC_MEMREG_SCPU) + 0x00c000,
         memory_region(WPC_MEMREG_SROM) + 0x07c000, 0x4000);
}

/*-----------------
/  local functions
/------------------*/
static void wpcs_ym2151IRQ(int state) {
  cpu_set_irq_line(WPC_SCPUNO, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}



