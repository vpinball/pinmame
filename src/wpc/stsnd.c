#include "driver.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "stsnd.h"

/*----------------------------------------
/ Stern Sound System
/ 3 different boards:
/
/ ST-100:  discrete
/ ST-300:  discrete, but using ROMs (+ VS-1000 speech)
/ ASTRO:   discrete, can switch between SB-100 and SB-300
/-----------------------------------------*/
static struct {
  struct sndbrdData brdData;
} sts_locals;

static WRITE_HANDLER(sts_ctrl_w)
{
	logerror("snd_ctrl_w: %i\n", data);
}

static WRITE_HANDLER(sts_data_w)
{
    logerror("snd_data_w: %i\n", data);
}

static void sts_init(struct sndbrdData *brdData)
{
	memset(&sts_locals, 0x00, sizeof(sts_locals));
	sts_locals.brdData = *brdData;
}

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf st100Intf = {
  sts_init, NULL, NULL, sts_data_w, NULL, sts_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf st300Intf = {
  sts_init, NULL, NULL, sts_data_w, NULL, sts_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf astroIntf = {
  sts_init, NULL, NULL, sts_data_w, NULL, sts_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
