#include "driver.h"
#include "core.h"
#include "sim.h"
#include "taito.h"

static const core_tLCDLayout dispTaito[] = {
  { 0, 0, 0, 16, CORE_SEG7 }, { 2, 0,16, 16, CORE_SEG7 },
  { 4, 0,32, 16, CORE_SEG7 }, {0}
};

TAITO_INPUT_PORTS_START(taito,1)        TAITO_INPUT_PORTS_END

#define INITGAME(name) \
static core_tGameData name##GameData = {0,dispTaito,{FLIP_SW(FLIP_L),0,0,0,0,0}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*--------------------------------
/ Zarza
/-------------------------------*/
INITGAME(zarza)
TAITO_ROMSTART4444(zarza,"zarza1.bin",0x81a35f85,
                         "zarza2.bin",0xcbf88eee,
                         "zarza3.bin",0xa5faf4d5,
                         "zarza4.bin",0xddfcdd20)
TAITO_ROMEND
#define input_ports_zarza input_ports_taito
CORE_GAMEDEFNV(zarza,"Zarza",1985,"Taito",taito,0)
