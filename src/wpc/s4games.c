#include "driver.h"
#include "core.h"
#include "wmssnd.h"
#include "s4.h"

#define INITGAME(name, disp, flip) \
static core_tGameData name##GameData = { GEN_S4, disp, {flip} }; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define INITGAMEFULL(name, disp,lflip,rflip,ss17,ss18,ss19,ss20,ss21,ss22) \
static core_tGameData name##GameData = { GEN_S4, disp, {FLIP_SWNO(lflip,rflip)}, \
 NULL, {{0}},{0,{ss17,ss18,ss19,ss20,ss21,ss22}}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

S4_INPUT_PORTS_START(s4, 1) S4_INPUT_PORTS_END

/*--------------------------------
/ Phoenix - Sys.4 (Game #485)
/-------------------------------*/
INITGAMEFULL(phnix,s4_disp,0,0,18,17,16,30,46,0)
S4_ROMSTART(phnix,l1,"gamerom.716", CRC(3aba6eac) SHA1(3a9f669216b3214bc42a1501aa2b10cfbcc36315),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_phnix input_ports_s4
CORE_GAMEDEF(phnix,l1,"Phoenix (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Flash - Sys.4 (Game #486)
/-------------------------------*/
INITGAMEFULL(flash,s4_disp,0,0,19,18,20,42,41,0)
S4_ROMSTART(flash,l1,"gamerom.716", CRC(287f12d6) SHA1(ede0c5b0ea2586d8bdf71ecadbd9cc8552bd6934),
                     "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2.716",  CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_flash input_ports_s4
CORE_GAMEDEF(flash,l1,"Flash (L-1)",1979,"Williams",s4_mS4S,0)

S4_ROMSTART(flash,t1,"gamerom.716", CRC(287f12d6) SHA1(ede0c5b0ea2586d8bdf71ecadbd9cc8552bd6934),
                     "green1.716",  CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
CORE_CLONEDEF(flash,t1,l1,"Flash (T-1) Ted Estes",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Tri Zone - Sys.4 (Game #487)
/-------------------------------*/
INITGAMEFULL(trizn,s4_disp,0,0,13,14,15,26,24,0)
S4_ROMSTART(trizn,l1,"gamerom.716", CRC(757091c5) SHA1(00dac6c19b08d2528ea293619c4a39499a1a02c2),
                     "green1.716", CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2.716", CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_trizn input_ports_s4
CORE_GAMEDEF(trizn,l1,"Tri Zone (L-1)",1978,"Williams",s4_mS4S,0)

S4_ROMSTART(trizn,t1,"gamerom.716", CRC(757091c5) SHA1(00dac6c19b08d2528ea293619c4a39499a1a02c2),
                     "green1.716", CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
CORE_CLONEDEF(trizn,t1,l1,"Tri Zone (T-1) Ted Estes",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Pokerino - Sys.4 (Game #488)
/-------------------------------*/
INITGAMEFULL(pkrno,s4_disp,0,0,38,39,40,30,0,28)
S4_ROMSTART(pkrno,l1,"gamerom.716", CRC(9b4d01a8) SHA1(1bd51745f38381ffc66fde4b28b76aab33b573ca),
                     "white1.716", CRC(9bbbf14f) SHA1(b0542ffdd683fa0ea4a9819576f3789cd5a4b2eb),
                     "white2.716", CRC(4d4010dd) SHA1(11221124fef3b7bf82d353d65ce851495f6946a7))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_pkrno input_ports_s4
CORE_GAMEDEF(pkrno,l1,"Pokerino (L-1)",1978,"Williams",s4_mS4S,0)

/*--------------------------------
/ Time Warp - Sys.4 (Game #489)
/-------------------------------*/
INITGAMEFULL(tmwrp,s4_disp,0,0,19,20,21,22,23,0)
S4_ROMSTART(tmwrp,l2,"gamerom.716", CRC(b168df09) SHA1(d4c97714636ce51be2e5f8cc5af89e10a2f82ac7),
                     "green1.716", CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2.716", CRC(1c978a4a) SHA1(1959184764643d58f1740c54bb74c2aad7d667d2))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_tmwrp input_ports_s4
CORE_GAMEDEF(tmwrp,l2,"Time Warp (L-2)",1979,"Williams",s4_mS4S,0)

S4_ROMSTART(tmwrp,t2,"gamerom.716", CRC(b168df09) SHA1(d4c97714636ce51be2e5f8cc5af89e10a2f82ac7),
                     "green1.716", CRC(2145f8ab) SHA1(ddf63208559a3a08d4e88327c55426b0eed27654),
                     "green2a.716",  CRC(16621eec) SHA1(14e1cf5f7227860a3219b2b79fa66dcf252dce98))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
CORE_CLONEDEF(tmwrp,t2,l2,"Time Warp (T-2) Ted Estes",1979,"Williams",s4_mS4S,0)

/*--------------------------------
/ Stellar Wars - Sys.4 (Game #490)
/-------------------------------*/
INITGAMEFULL(stlwr,s4_disp,0,0,14,13,45,46,40,44)
S4_ROMSTART(stlwr,l2,"gamerom.716", CRC(874e7ef7) SHA1(271aeac2a0e61cb195811ae2e8d908cb1ab45874),
                     "yellow1.716", CRC(d251738c) SHA1(65ddbf5c36e429243331a4c5d2339df87a8a7f64),
                     "yellow2.716", CRC(5049326d) SHA1(3b2f4ea054962bf4ba41d46663b7d3d9a77590ef))
S67S_SOUNDROMS8(     "sound1.716",CRC(f4190ca3) SHA1(ee234fb5c894fca5876ee6dc7ea8e89e7e0aec9c))
S4_ROMEND
#define input_ports_stlwr input_ports_s4
CORE_GAMEDEF(stlwr,l2,"Stellar Wars (L-2)",1979,"Williams",s4_mS4S,0)



