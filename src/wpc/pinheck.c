// license:BSD-3-Clause

/************************************************************************************************
  pinHeck system (Spooky Pinball, 2014-2017)

  A PIC32MX795 (MIPS32 M4K, 80 MHz) runs the game. A Parallax Propeller P8X32A (8 cogs, 80 MHz)
  drives the display (128x32 RGB332 serial module, The Jetsons' 128x64 one, or a raw 128x32
  DMD), makes the sound (counters on P14/P15) and plays media from an SD card; the two talk over
  a serial link. A CAT24M01 EEPROM and a DS1340 clock sit on the PIC32's I2C bus; the playfield
  board (switches, coils, lamps, RGB LEDs, servos) hangs off its ports.

  The PIC32 runs on its own bus (src/cpu/pic32mx), not MAME's memory map, so the debugger's
  memory views show nothing. The Propeller (src/cpu/p8x32a) runs inside the driver, by default
  on a worker thread (pinheck/prop.c), with identical output.
************************************************************************************************/

#include "driver.h"
#include <ctype.h>
#include "core.h"
#include "cpu/pic32mx/pic32mxcpu.h"
#include "pinheck/prop.h"
#ifdef PINMAME_JIT_ASMJIT
#include "cpu/p8x32a/p8x32ajit.h"
#endif
#include "pinheck/rtc.h"
#include "pinheck/bootldr.h"
#include "pinheck/sd.h"
#include "pinheck/zipsrc.h"
#include "pinheck/audio.h"
#include "pinheck/display.h"
#include "pinheck/board.h"
#include "pinheck/dmd.h"
#include "pinheck/hexload.h"
#include "pinheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PINHECK_CLOCK 80000000
#define PINHECK_LOG_MAX 64
#define PINHECK_ZIP_CACHE (64u << 20)
#define PINHECK_REFUSE_SECS 30 /* how long a game that cannot run shows why */
#define RF5  (1u << 5)
#define RF12 (1u << 12)
#define RF13 (1u << 13)

static pinheck_prop prop;
static cat24m01 u13;
static ds1340 rtc;
static pic32_boot boot;
static zipsrc zip;
static vfat vol;
static sd_card card;
static uint8_t u13mem[131072], propmem[131072];
static int reset_done, opened;

static struct {
	FILE *uart1, *proplog;
	const char *send;
	uint64_t send_at, send_gap, rtc_at;
	double reset_at;
	int have_zip, have_vol, idle;
	uint32_t logged[PINHECK_LOG_MAX];
	int nlogged;
} locals;

/* board I/O: lamps, coils, switches, GI, RGB and servos (board.c) */
#define PINHECK_SOL_GI0 24  /* GI 0-7: solenoids 25-32 */
#define PINHECK_SOL_GI8 40  /* GI 8-15: solenoids 37-44 through core.c's S11 layout */
#define PINHECK_SOL_RGB 50  /* on-board RGB left R,G,B, right R,G,B: 51-56 */
#define PINHECK_SOL_SRV 56  /* servos 0-4: 57-61 */
#define PINHECK_SOL_EXT 61  /* external WS2801 LED 0 R,G,B, or on-board LED 2 R,G,B: 62-64 */
#define PINHECK_EXT_LEDS 1  /* the firmware drives one external LED */
#define PINHECK_ONB_LEDS 3  /* on-board LEDs 0 and 1; onbLed2 (America's Most Haunted's ghost) on 62-64 */
#define PINHECK_NSOLS   64
#define PINHECK_LAMP_ST 64  /* start button lamp: lamp 91 */
#define PINHECK_NLAMPS  72

static pinheck_board brd;
static FILE *brd_log;
static int brd_opened, brd_rgb_extra;
static UINT32 brd_sols_seen;
static UINT16 brd_gi8_seen;
static UINT8 brd_cust[PINHECK_NSOLS - PINHECK_SOL_RGB], brd_logged[PINHECK_NLAMPS + PINHECK_NSOLS];
static int brd_servo_us[BOARD_SERVOS];
static UINT8 brd_sw_logged[10], brd_lamps_logged[9];

/* PINHECK_* environment variables, read only with PINHECK_TEST_HOOKS */
static const char *pinheck_env(const char *name)
{
#ifdef PINHECK_TEST_HOOKS
	return getenv(name);
#else
	(void)name;
	return NULL;
#endif
}

static const pinheck_tGameData *pinheck_game(void) { return (const pinheck_tGameData *)core_gameData; }

static void pinheck_warn(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	logerror("%s\n", msg);
}

static void pinheck_unsupported(void)
{
	const pinheck_tGameData *g = pinheck_game();
	char msg[160];
	if (g->rgbInverted) {
		sprintf(msg, "pinheck: %.16s: inverted WS2801 lines are not supported, the RGB outputs are as sent", Machine->gamedrv->name);
		pinheck_warn(msg);
	}
	if (g->dmdHub && (g->width != DMD_W || g->height != DMD_H)) {
		sprintf(msg, "pinheck: %.16s: a %dx%d raw DMD is not supported, it is taken as 128x32", Machine->gamedrv->name, g->width, g->height);
		pinheck_warn(msg);
	} else if (!g->dmdHub && !DISPLAY_SIZE_OK(g->width, g->height)) {
		sprintf(msg, "pinheck: %.16s: a %dx%d display is not supported, frames are taken as 128x32", Machine->gamedrv->name, g->width, g->height);
		pinheck_warn(msg);
	}
}

static uint8_t pinheck_brd_swcol(void *ctx, int col) { (void)ctx; return coreGlobals.swMatrix[col + 1]; }
static uint16_t pinheck_brd_cab(void *ctx) { (void)ctx; return (uint16_t)(coreGlobals.swMatrix[0] | coreGlobals.swMatrix[9] << 8); }

static void pinheck_brd_lamps(void *ctx, uint64_t t, uint8_t cols, uint8_t rows)
{
	(void)ctx;
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0, cols, rows, 8);
	if (brd_log) fprintf(brd_log, "%.9f L %02x %02x %llu\n", timer_get_time(), cols, rows, (unsigned long long)t);
}

static void pinheck_brd_sols(void *ctx, uint64_t t, uint32_t sols)
{
	int i;
	(void)ctx;
	for (i = 0; i < 3; i++) core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8 * i, (UINT8)(sols >> (8 * i)));
	coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF000000u) | sols;
	brd_sols_seen |= sols;
	if (brd_log) fprintf(brd_log, "%.9f S %06x %llu\n", timer_get_time(), (unsigned)sols, (unsigned long long)t);
}

static void pinheck_brd_gi(void *ctx, uint64_t t, uint16_t gi)
{
	(void)ctx;
	core_write_pwm_output_8b(CORE_MODOUT_SOL0 + PINHECK_SOL_GI0, (UINT8)gi);
	core_write_pwm_output_8b(CORE_MODOUT_SOL0 + PINHECK_SOL_GI8, (UINT8)(gi >> 8));
	coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFFu) | ((UINT32)(gi & 0xFF) << 24);
	brd_sols_seen |= (UINT32)(gi & 0xFF) << 24;
	brd_gi8_seen |= (UINT16)(gi & 0xFF00);
	if (brd_log) fprintf(brd_log, "%.9f G %04x %llu\n", timer_get_time(), gi, (unsigned long long)t);
}

static void pinheck_brd_start(void *ctx, uint64_t t, int on)
{
	(void)ctx;
	core_write_pwm_output(CORE_MODOUT_LAMP0 + PINHECK_LAMP_ST, 1, (UINT8)on);
	if (on) coreGlobals.tmpLampMatrix[8] |= 1;
	if (brd_log) fprintf(brd_log, "%.9f T %d %llu\n", timer_get_time(), on, (unsigned long long)t);
}

static void pinheck_brd_level(int idx, UINT8 v)
{
	brd_cust[idx - PINHECK_SOL_RGB] = v;
	coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + idx].value = v / 255.0f;
}

static void pinheck_brd_rgb(void *ctx, uint64_t t, int chain, int led, uint8_t r, uint8_t g, uint8_t b)
{
	(void)ctx;
	if (brd_log) fprintf(brd_log, "%.9f R %d %d %02x%02x%02x %llu\n", timer_get_time(), chain, led, r, g, b, (unsigned long long)t);
	if ((chain == BOARD_RGB_ONBOARD && led < (pinheck_game()->onbLed2 ? PINHECK_ONB_LEDS : 2)) || (chain == BOARD_RGB_EXTERNAL && led < PINHECK_EXT_LEDS)) {
		int idx = chain == BOARD_RGB_ONBOARD && led < 2 ? PINHECK_SOL_RGB + 3 * led : PINHECK_SOL_EXT + 3 * (chain == BOARD_RGB_ONBOARD ? led - 2 : led);
		pinheck_brd_level(idx, r);
		pinheck_brd_level(idx + 1, g);
		pinheck_brd_level(idx + 2, b);
	} else if (!brd_rgb_extra) {
		brd_rgb_extra = 1;
		logerror("pinheck: WS2801 chain %d carries LED %d, which has no output on this board\n", chain, led);
	}
}

static void pinheck_brd_servo(void *ctx, uint64_t t, int servo, uint32_t pulse)
{
	const uint32_t tpu = PINHECK_CLOCK / 1000000; /* clock ticks per us */
	const uint32_t lo = (uint32_t)pinheck_game()->servoMin * tpu, hi = (uint32_t)pinheck_game()->servoMax * tpu;
	(void)ctx;
	if (brd_log) fprintf(brd_log, "%.9f V %d %.1f %llu\n", timer_get_time(), servo, pulse / (double)tpu, (unsigned long long)t);
	brd_servo_us[servo] = (int)((pulse + tpu / 2) / tpu);
	if (!pulse) return; /* no pulses: the servo holds its position */
	/* servoMin..servoMax us map to levels 0..255, rounded half up */
	pinheck_brd_level(PINHECK_SOL_SRV + servo, (UINT8)(pulse <= lo ? 0 : pulse >= hi ? 255 : ((uint64_t)(pulse - lo) * 255 + (hi - lo) / 2) / (hi - lo)));
}

/* switch n (0-63) is PinMAME (n/8+1)*10 + n%8+1, lamps likewise; cabinet inputs are columns 0 and 9.
   Out-of-range switches map to column 15 row 8 (unread), lamps to lamp 72 (never lit) */
#define PINHECK_NOSUCH (15 * 8 + 7)
#define PINHECK_NOLAMP (8 + PINHECK_NLAMPS) /* lamp2m results are offset by 8 */
static int pinheck_2m(int no, int cols, int none) { return no > 0 && no / 10 < cols && no % 10 >= 1 && no % 10 <= 8 ? (no / 10) * 8 + no % 10 - 1 : none; }
static int pinheck_sw2m(int no) { return pinheck_2m(no, CORE_STDSWCOLS, PINHECK_NOSUCH); }
static int pinheck_lamp2m(int no) { return pinheck_2m(no, CORE_CUSTLAMPCOL + 2, PINHECK_NOLAMP); }
static int pinheck_m2sw(int col, int row) { return col * 10 + row + 1; }

/* servo 0-4 pulse width in us, 0 without pulses (for the simulator) */
int pinheck_servo(int servo)
{
	return servo >= 0 && servo < BOARD_SERVOS ? brd_servo_us[servo] : 0;
}

int pinheck_getsol(int solNo)
{
	return solNo > PINHECK_SOL_RGB && solNo <= PINHECK_NSOLS ? brd_cust[solNo - 1 - PINHECK_SOL_RGB] : 0;
}

/* test log: lamp and solenoid level changes */
static void pinheck_brd_log_outputs(void)
{
	float sol[CORE_MODOUT_SOL_MAX];
	char line[2048];
	int i, n = 0;
	core_update_pwm_lamps();
	core_update_pwm_solenoids();
	core_getAllPhysicSols(sol);
	for (i = 0; i < PINHECK_NLAMPS + PINHECK_NSOLS; i++) {
		float f = i < PINHECK_NLAMPS ? coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + i].value : sol[i - PINHECK_NLAMPS];
		UINT8 v = f <= 0.0f ? 0 : f >= 1.0f ? 255 : (UINT8)(f * 255.0f + 0.5f);
		if (v == brd_logged[i]) continue;
		brd_logged[i] = v;
		if (i < PINHECK_NLAMPS) n += sprintf(line + n, " L%d=%d", coreData->m2lamp(i / 8 + 1, i % 8), v);
		else n += sprintf(line + n, " S%d=%d", i - PINHECK_NLAMPS + 1, v);
	}
	if (n) fprintf(brd_log, "%.9f P%s\n", timer_get_time(), line);
	/* test log: switch changes */
	for (n = 0, i = 0; i < 80; i++)
		if (((coreGlobals.swMatrix[i / 8] ^ brd_sw_logged[i / 8]) >> (i % 8)) & 1)
			n += sprintf(line + n, " %d=%d", coreData->m2sw(i / 8, i % 8), (coreGlobals.swMatrix[i / 8] >> (i % 8)) & 1);
	memcpy(brd_sw_logged, (void *)coreGlobals.swMatrix, sizeof(brd_sw_logged));
	if (n) fprintf(brd_log, "%.9f W%s\n", timer_get_time(), line);
	/* test log: the binary lamp matrix when it changed */
	if (memcmp(brd_lamps_logged, (void *)coreGlobals.lampMatrix, 9)) {
		memcpy(brd_lamps_logged, (void *)coreGlobals.lampMatrix, 9);
		for (n = 0, i = 0; i < 9; i++) n += sprintf(line + n, "%02x", brd_lamps_logged[i]);
		fprintf(brd_log, "%.9f B %s\n", timer_get_time(), line);
	}
}

static void pinheck_brd_init(void)
{
	const char *path = pinheck_env("PINHECK_OUT_LOG");
	if (brd_log) fclose(brd_log);
	brd_log = path ? fopen(path, brd_opened ? "a" : "w") : NULL;
	brd_opened = 1;
	options.usemodsol |= CORE_MODOUT_FORCE_ON;
	coreGlobals.nLamps = PINHECK_NLAMPS;
	coreGlobals.nSolenoids = PINHECK_NSOLS;
	coreGlobals.nGI = 0;
	/* a lamp is lit at most 2 of every 24 lamp-timer periods: 12x brings full strobe to 1.0 */
	core_set_pwm_output_led_vfd(CORE_MODOUT_LAMP0, 64, 0, 12.0f);
	core_set_pwm_output_type(CORE_MODOUT_LAMP0 + PINHECK_LAMP_ST, PINHECK_NLAMPS - 64, CORE_MODOUT_LED);
	core_set_pwm_output_type(CORE_MODOUT_SOL0, BOARD_SOLS, CORE_MODOUT_SOL_2_STATE);
	core_set_pwm_output_type(CORE_MODOUT_SOL0 + PINHECK_SOL_GI0, 8, CORE_MODOUT_LED);
	core_set_pwm_output_type(CORE_MODOUT_SOL0 + PINHECK_SOL_GI8, 8, CORE_MODOUT_LED);
	core_set_pwm_output_type(CORE_MODOUT_SOL0 + PINHECK_SOL_RGB, PINHECK_NSOLS - PINHECK_SOL_RGB, CORE_MODOUT_NONE);
}

/* PIC32 reset: all pins are inputs, all outputs off */
static void pinheck_brd_reset(void)
{
	pinheck_board_io io = { NULL, pinheck_brd_swcol, pinheck_brd_cab, pinheck_brd_lamps, pinheck_brd_sols,
	                        pinheck_brd_gi, pinheck_brd_start, pinheck_brd_rgb, pinheck_brd_servo };
	int i;
	pinheck_board_init(&brd, &io, PINHECK_CLOCK);
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0, 0, 0, 8);
	core_write_pwm_output(CORE_MODOUT_LAMP0 + PINHECK_LAMP_ST, 1, 0);
	for (i = 0; i < 3; i++) core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8 * i, 0);
	core_write_pwm_output_8b(CORE_MODOUT_SOL0 + PINHECK_SOL_GI0, 0);
	core_write_pwm_output_8b(CORE_MODOUT_SOL0 + PINHECK_SOL_GI8, 0);
	for (i = PINHECK_SOL_RGB; i < PINHECK_NSOLS; i++) pinheck_brd_level(i, 0);
	memset(brd_servo_us, 0, sizeof(brd_servo_us));
	coreGlobals.pulsedSolState = 0;
	brd_rgb_extra = 0;
	brd_sols_seen = 0;
	brd_gi8_seen = 0;
	coreGlobals.swMatrix[0] |= 1;
}

static void pinheck_brd_vblank(void)
{
	pinheck_board_tick(&brd, pic32cpu_soc()->cpu.cycles);
	memcpy((void *)coreGlobals.lampMatrix, (void *)coreGlobals.tmpLampMatrix, 9);
	memset((void *)coreGlobals.tmpLampMatrix, 0, 9);
	coreGlobals.solenoids = brd_sols_seen | coreGlobals.pulsedSolState;
	coreGlobals.solenoids2 = (coreGlobals.solenoids2 & ~0xFF00u) | brd_gi8_seen | (brd.gi & 0xFF00u);
	brd_sols_seen = 0;
	brd_gi8_seen = 0;
	if (brd_log) pinheck_brd_log_outputs();
}

static void pinheck_brd_stop(void)
{
	if (brd_log) fclose(brd_log);
	brd_log = NULL;
}

/* cabinet inputs: firmware cabinet switch n = PinMAME switch n (1-8), n+82 (9-15) */
static SWITCH_UPDATE(pinheck)
{
	UINT8 c0;
	if (!inports) return;
	c0 = coreGlobals.swMatrix[0] & 0x0Cu;
	if (!(inports[CORE_COREINPORT] & 0x0008)) c0 |= 0x01;
	if (inports[CORE_COREINPORT] & 0x0040) c0 |= 0x02;
	if (inports[CORE_COREINPORT] & 0x0010) c0 |= 0x10;
	if (inports[CORE_COREINPORT] & 0x0020) c0 |= 0x20;
	if (inports[CORE_COREINPORT] & 0x0002) c0 |= 0x40;
	if (inports[CORE_COREINPORT] & 0x0001) c0 |= 0x80;
	coreGlobals.swMatrix[0] = c0;
	coreGlobals.swMatrix[9] = (coreGlobals.swMatrix[9] & ~0x08u) | (inports[CORE_COREINPORT] & 0x0004 ? 0x08u : 0);
}

static void pinheck_uart_tx(void *ctx, int uart, uint8_t byte, uint64_t cycle)
{
	(void)ctx; (void)cycle;
	if (uart == 1 && locals.uart1) fputc(byte, locals.uart1);
}

/* test log (PINHECK_LINK_LOG): link packets, 16 bytes LSB first, sampled on the rising clock */
static FILE *link_log;
static int link_opened;
static struct { int clk, nbits; uint64_t last, gap; uint8_t b[16]; } lnk;

static void pinheck_link_bit(uint32_t drv, uint64_t cycle)
{
	int clk = (drv & RF12) != 0, i;
	if (clk && !lnk.clk) {
		if (lnk.nbits && cycle - lnk.last > lnk.gap) lnk.gap = cycle - lnk.last;
		if (drv & RF5) lnk.b[lnk.nbits >> 3] |= (uint8_t)(1u << (lnk.nbits & 7));
		else lnk.b[lnk.nbits >> 3] &= (uint8_t)~(1u << (lnk.nbits & 7));
		lnk.last = cycle;
		if (++lnk.nbits == 128) {
			fprintf(link_log, "%.6f", cycle / (double)PINHECK_CLOCK);
			for (i = 0; i < 16; i++) fprintf(link_log, " %02x", lnk.b[i]);
			fprintf(link_log, " gap %llu\n", (unsigned long long)lnk.gap);
			lnk.nbits = 0;
			lnk.gap = 0;
		}
	}
	lnk.clk = clk;
}

static void pinheck_port_write(void *ctx, int port, uint32_t lat, uint32_t tris, uint64_t cycle)
{
	uint32_t drv = lat & ~tris;
	(void)ctx;
	if (port == PIC32MX_PORTF && link_log) pinheck_link_bit(drv, cycle);
	if (port == PIC32MX_PORTF)
		prop_pic_pins(&prop, cycle, (drv & RF12 ? 1u << 25 : 0) | (drv & RF5 ? 1u << 26 : 0));
	pinheck_board_port(&brd, port, lat, tris, cycle);
}

/* RF13 (P24) with the worker thread: settled only when an instruction uses it (mips32_uncertain) */
static int rf13_guess, rf13_exact = -1;

static uint32_t pinheck_port_read(void *ctx, int port, uint64_t cycle)
{
	uint32_t v = pinheck_board_read(&brd, port, cycle);
	(void)ctx;
	if (port != PIC32MX_PORTF) return v;
	if (rf13_exact < 0) rf13_exact = pinheck_env("PINHECK_RF13") && atoi(pinheck_env("PINHECK_RF13")) == 0;
	if (prop.worker && !rf13_exact) {
		pic32mx_uncertain(pic32cpu_soc(), RF13, prop_sample(&prop, cycle));
		return (v & ~RF13) | (rf13_guess ? RF13 : 0);
	}
	return (v & ~RF13) | (prop_p24(&prop, cycle) ? RF13 : 0);
}

static int pinheck_port_settle(void *ctx, uint32_t token, int wait, uint32_t *bits)
{
	int v = prop_sample_get(&prop, token, wait);
	(void)ctx;
	if (v < 0) return 0;
	rf13_guess = v;
	*bits = v ? RF13 : 0;
	return 1;
}

static int pinheck_i2c_pins(void *ctx, int module, int scl, int sda, uint64_t cycle)
{
	(void)ctx;
	if (module != 1) return sda;
	ds1340_tick(&rtc, cycle - locals.rtc_at);
	locals.rtc_at = cycle;
	return cat24m01_update(&u13, scl, sda) & ds1340_update(&rtc, scl, sda);
}

static void pinheck_unmapped(void *ctx, uint32_t pa, int write)
{
	(void)ctx;
	logerror("pinheck: unmodelled SFR %s %08x\n", write ? "write" : "read", (unsigned)(pa | 0xA0000000u));
}

static void pinheck_exception(void *ctx, int code, uint32_t pc)
{
	uint32_t key = pc ^ ((uint32_t)code << 27);
	int i;
	(void)ctx;
	for (i = 0; i < locals.nlogged; i++)
		if (locals.logged[i] == key) return;
	if (locals.nlogged < PINHECK_LOG_MAX) locals.logged[locals.nlogged++] = key;
	logerror("pinheck: PIC32 exception %d at %08x\n", code, (unsigned)pc);
}

static void pinheck_prop_log(void *ctx, const char *msg)
{
	(void)ctx;
	logerror("pinheck: %s\n", msg);
	if (locals.proplog) fprintf(locals.proplog, "%s\n", msg);
}

static int pinheck_blk_read(void *ctx, uint32_t lba, uint8_t *buf) { (void)ctx; return vfat_read(&vol, lba, buf); }
static int pinheck_spi(void *ctx, int cs, int sclk, int mosi) { (void)ctx; return sd_update(&card, cs, sclk, mosi); }
static void pinheck_boot_tx(void *ctx, uint64_t t, int level) { (void)ctx; prop_pic_pins(&prop, t, level ? 1u << 24 : 0); }
static void pinheck_prop_tx(void *ctx, uint64_t t, int level) { (void)ctx; boot_rx(&boot, t, level); }

/* once the bootloader stand-in has released the application, the Propeller can run on its own thread */
static uint64_t pinheck_hold(void *ctx, uint64_t cycle)
{
	(void)ctx;
	prop_catch_up(&prop, cycle);
	if (boot.state == BOOT_APP && prop.worker) return 0;
	prop_sync(&prop);
	boot_advance(&boot, cycle);
	return boot_hold(&boot, cycle);
}

static uint64_t pinheck_pic_now(void *ctx) { (void)ctx; return pic32cpu_soc()->cpu.cycles; }

static int pinheck_starts(const char *n, const char *p)
{
	while (*p) if (toupper((unsigned char)*n++) != *p++) return 0;
	return 1;
}

static void pinheck_check_card(const vfat_source *s)
{
	char msg[160];
	int i, dmd = 0, sfx = 0;
	for (i = 0; i < s->count; i++) {
		const char *n = s->name(s->ctx, i);
		if (pinheck_starts(n, "DMD/")) dmd = 1;
		if (pinheck_starts(n, "SFX/")) sfx = 1;
	}
	if (dmd && sfx) return;
	sprintf(msg, "pinheck: the SD card from %.16s.zip has no %s%s%s; display and sound stay blank", Machine->gamedrv->name,
	        dmd ? "" : "DMD/", !dmd && !sfx ? " and no " : "", sfx ? "" : "SFX/");
	pinheck_warn(msg);
}

static void pinheck_open_card(void)
{
	char name[32], msg[96];
	int i, n = osd_get_path_count(FILETYPE_ROM);
	sd_blockdev dev;

	sprintf(name, "%.16s.zip", Machine->gamedrv->name);
	/* probe first: openzip reports a missing file (a message box in VPinMAME) */
	for (i = 0; i < n && !locals.have_zip; i++)
		if (osd_get_path_info(FILETYPE_ROM, i, name) == PATH_IS_FILE && zipsrc_open(&zip, FILETYPE_ROM, i, name, PINHECK_ZIP_CACHE) == 0)
			locals.have_zip = 1;
	if (!locals.have_zip) {
		sprintf(msg, "pinheck: no %s on the ROM path, the SD card is empty", name);
		pinheck_warn(msg);
		return;
	}
	pinheck_check_card(zipsrc_source(&zip));
	if (vfat_init(&vol, zipsrc_source(&zip)) != 0) { pinheck_warn("pinheck: cannot build the SD volume"); return; }
	locals.have_vol = 1;
	dev.ctx = NULL;
	dev.sectors = vfat_sectors(&vol);
	dev.read = pinheck_blk_read;
	sd_init(&card, &dev);
	prop_attach_sd(&prop, pinheck_spi, NULL);
}

static void pinheck_in_service(uint8_t *mem, uint32_t version)
{
	uint32_t w0 = version << 24 | 0xBAFAu, w1 = 0xABBA0002u;
	int i;
	for (i = 0; i < 4; i++) {
		mem[0x8000 + i] = (uint8_t)(w0 >> (8 * i));
		mem[0x8004 + i] = (uint8_t)(w1 >> (8 * i));
	}
}

static int64_t pinheck_local_now(void)
{
	time_t t = time(NULL);
	struct tm u = *gmtime(&t);
	u.tm_isdst = -1;
	return (int64_t)t + (int64_t)difftime(t, mktime(&u));
}

static void pinheck_tick(int param)
{
	pic32mx *soc = pic32cpu_soc();
	(void)param;
	if (locals.idle) return;
	prop_catch_up(&prop, soc->cpu.cycles);
	if (locals.reset_at > 0.0 && timer_get_time() >= locals.reset_at) {
		locals.reset_at = 0.0;
		reset_done = 1;
		machine_reset();
		return;
	}
	if (locals.send && *locals.send == '~' && soc->cpu.cycles >= locals.send_at) {
		locals.send_at += locals.send_gap;
		locals.send++;
	}
	if (locals.send && *locals.send && soc->cpu.cycles >= locals.send_at) {
		int n = soc->uart[0].rx_count;
		pic32mx_uart_rx(soc, 0, (uint8_t)*locals.send);
		if (soc->uart[0].rx_count > n) locals.send++;
	}
}

static display disp;
static uint8_t disp_shown[DISPLAY_FRAME_MAX], disp_cfg[DISPLAY_CFG_MAX];
static int disp_cfg_n, disp_opened;
static FILE *disp_log;
static UINT32 disp_rgb32[256];
static UINT16 disp_rgb15[256];
static display_look disp_look;
static uint8_t disp_img[DISPLAY_LOOK_W * DISPLAY_LOOK_H * 3];
static int disp_dirty;

static void pinheck_disp_frame(void *ctx, const uint8_t *frame, uint64_t t)
{
	uint8_t stamp[20];
	uint64_t pic = prop_stamp(&prop);
	uint32_t at = 0xFFFFFFFFu, a;
	const size_t n = (size_t)disp.frame;
	int k;
	(void)ctx;
	memcpy(disp_shown, frame, n);
	disp_dirty = 1;
	if (!disp_log) return;
	for (a = 0; a + n <= 0x8000; a++)
		if (prop.chip.hub[a] == frame[0] && !memcmp(prop.chip.hub + a, frame, n)) { at = a; break; }
	for (k = 0; k < 8; k++) stamp[k] = (uint8_t)(t >> (8 * k));
	for (k = 0; k < 8; k++) stamp[8 + k] = (uint8_t)(pic >> (8 * k));
	for (k = 0; k < 4; k++) stamp[16 + k] = (uint8_t)(at >> (8 * k));
	fwrite(stamp, 1, 20, disp_log);
	fwrite(frame, 1, n, disp_log);
}

static void pinheck_disp_config(void *ctx, const uint8_t *bytes, int n, uint64_t t)
{
	char msg[16 + 3 * DISPLAY_CFG_MAX];
	int k, len;
	(void)ctx; (void)t;
	if (n == disp_cfg_n && !memcmp(bytes, disp_cfg, (size_t)n)) return;
	memcpy(disp_cfg, bytes, (size_t)n);
	disp_cfg_n = n;
	len = sprintf(msg, "display: config");
	for (k = 0; k < n; k++) len += sprintf(msg + len, " %02x", bytes[k]);
	pinheck_prop_log(NULL, msg);
	if (!pinheck_display_look(&disp_look, bytes, n)) pinheck_prop_log(NULL, "display: unknown config packet, exact pixels");
	else disp_look.position += DISPLAY_ALIGNED - pinheck_game()->aligned; /* the game's factory POSITION is aligned */
	disp_dirty = 1;
}

static void pinheck_disp_pins(void *ctx, uint64_t t, uint32_t out, uint32_t dir)
{
	(void)ctx;
	pinheck_display_pins(&disp, t, out, dir);
}

/* Raw DMD (dmdHub): the Propeller thread decodes subframes from the scan pins (P16-P20) into a ring that is
   handed to the core's PWM integration at each vblank. PINHECK_DMD_PROOF cross-checks a row model. */
#define PINHECK_DMD_RING 64
static pinheck_dmd dmd, dmd_row;
static int dmd_on, dmd_proof;
static uint8_t dmd_ring[PINHECK_DMD_RING][DMD_SUB];
static unsigned dmd_head, dmd_tail, dmd_dropped;
static unsigned long dmd_subs, dmd_differ;
static uint64_t dmd_first;

static void pinheck_dmd_sub(void *ctx, const uint8_t *sub, int level, uint64_t t)
{
	(void)ctx; (void)level; (void)t;
	if (dmd_head - dmd_tail == PINHECK_DMD_RING) { dmd_tail++; dmd_dropped++; }
	memcpy(dmd_ring[dmd_head % PINHECK_DMD_RING], sub, DMD_SUB);
	dmd_head++;
}

/* runs after the decoder for the same row clock */
static unsigned long dmd_rows;

static void pinheck_dmd_row_sub(void *ctx, const uint8_t *sub, int level, uint64_t t)
{
	int r, n = 0;
	(void)ctx; (void)level;
	dmd_subs++;
	for (r = 0; r < DMD_H; r++) n += memcmp(sub + r * DMD_ROW, dmd.sub + r * DMD_ROW, DMD_ROW) != 0;
	if (!n) return;
	if (!dmd_differ++) dmd_first = t;
	dmd_rows += (unsigned long)n;
}

/* test log (PINHECK_FRAME_LOG): each cycle of 16 subframes, one byte per dot (0-15) */
static void pinheck_dmd_frame(void *ctx, const uint8_t *shades, uint64_t t)
{
	uint8_t stamp[20];
	uint64_t pic = prop_stamp(&prop);
	uint32_t at = (uint32_t)pinheck_game()->dmdHub;
	int k;
	(void)ctx;
	for (k = 0; k < DMD_W * DMD_H && at != 0xFFFFFFFFu; k += 2)
		if (p8x32a_hub_at(&prop.chip, (at + k / 2) & 0xFFFF, t) != (shades[k] << 4 | shades[k + 1])) at = 0xFFFFFFFFu;
	for (k = 0; k < 8; k++) stamp[k] = (uint8_t)(t >> (8 * k));
	for (k = 0; k < 8; k++) stamp[8 + k] = (uint8_t)(pic >> (8 * k));
	for (k = 0; k < 4; k++) stamp[16 + k] = (uint8_t)(at >> (8 * k));
	fwrite(stamp, 1, 20, disp_log);
	fwrite(shades, 1, DMD_W * DMD_H, disp_log);
}

/* test hook PINHECK_PROP_SERIAL: the raw DMD Propeller's watchdog output on P30 (57,600 baud 8N1 at 104 MHz) */
#define PINHECK_SER_PIN (1u << 30)
static FILE *ser_log;
static int ser_opened;
static struct { int level, bit; uint64_t edge, start; uint8_t byte; char line[256]; int n; } ser;

static void pinheck_ser_pins(uint64_t t, uint32_t out, uint32_t dir)
{
	const uint64_t bit = 104000000u / 57600u;
	int level = !(dir & PINHECK_SER_PIN) || (out & PINHECK_SER_PIN);
	/* the data bits whose centres passed since the last edge had the old level */
	while (ser.bit >= 0 && ser.bit < 8 && ser.start + bit * (uint64_t)(2 * ser.bit + 3) / 2 < t) {
		if (ser.level) ser.byte |= (uint8_t)(1u << ser.bit);
		if (++ser.bit == 8) {
			if (ser.byte == '\n' || ser.byte == '\r' || ser.n == (int)sizeof(ser.line) - 1) {
				if (ser.n) { ser.line[ser.n] = 0; fprintf(ser_log, "%.6f %s\n", t / 104e6, ser.line); }
				ser.n = 0;
			} else if (ser.byte >= 32 && ser.byte < 127) ser.line[ser.n++] = (char)ser.byte;
			ser.bit = -1;
		}
	}
	if (level == ser.level) return;
	if (!level && ser.bit < 0 && t - ser.edge >= bit) { ser.start = t; ser.bit = 0; ser.byte = 0; }
	ser.level = level;
	ser.edge = t;
}

/* hub RAM as the lazy scan cog read it at t */
static uint8_t pinheck_dmd_hub_at(void *ctx, uint32_t a, uint64_t t) { (void)ctx; return p8x32a_hub_at(&prop.chip, a, t); }

static void pinheck_dmd_pins_cb(void *ctx, uint64_t t, uint32_t out, uint32_t dir)
{
	(void)ctx;
	if (ser_log) pinheck_ser_pins(t, out, dir);
	pinheck_dmd_pins(&dmd, t, out, dir);
	if (dmd_proof) pinheck_dmd_pins(&dmd_row, t, out, dir);
}

static void pinheck_dmd_lazy_cb(void *ctx, uint64_t t, uint32_t out, uint32_t dir)
{
	(void)ctx;
	pinheck_dmd_pins(&dmd, t, out, dir);
	if (dmd_proof) pinheck_dmd_pins(&dmd_row, t, out, dir);
}

/* test hook PINHECK_DMD_LOG: each vblank's subframe count (uint32) and subframes */
static FILE *dmd_log;
static int dmd_log_opened;

/* at each vblank: the subframes the Propeller drew since the last one */
static void pinheck_dmd_vblank(void)
{
	uint32_t n;
	unsigned k;
	prop_sync(&prop);
	n = dmd_head - dmd_tail;
	if (dmd_log) {
		fwrite(&n, 4, 1, dmd_log);
		for (k = dmd_tail; k != dmd_head; k++) fwrite(dmd_ring[k % PINHECK_DMD_RING], 1, DMD_SUB, dmd_log);
	}
	for (; dmd_tail != dmd_head; dmd_tail++) core_dmd_submit_frame(core_gameData->lcdLayout, dmd_ring[dmd_tail % PINHECK_DMD_RING], 1);
}

static void pinheck_dmd_stop(void)
{
	char msg[160];
	if (dmd_proof) {
		sprintf(msg, "dmd: proof: %lu subframes, the row model differs in %lu (%lu rows)", dmd_subs, dmd_differ, dmd_rows);
		if (dmd_differ) sprintf(msg + strlen(msg), ", the first at Propeller cycle %llu", (unsigned long long)dmd_first);
		pinheck_prop_log(NULL, msg);
	}
	if (dmd_dropped) {
		sprintf(msg, "dmd: %u subframes dropped between vblanks", dmd_dropped);
		pinheck_prop_log(NULL, msg);
	}
	dmd_subs = dmd_differ = dmd_rows = 0;
	dmd_dropped = 0;
	if (dmd_log) fclose(dmd_log);
	if (ser_log) fclose(ser_log);
	dmd_log = ser_log = NULL;
}

static void pinheck_disp_init(void)
{
	const char *path = pinheck_env("PINHECK_FRAME_LOG");
	int v;
	for (v = 0; v < 256; v++) {
		int r = ((v >> 5) & 7) * 255 / 7, g = ((v >> 2) & 7) * 255 / 7, b = (v & 3) * 255 / 3;
		disp_rgb32[v] = MAKE_RGB(r, g, b);
		disp_rgb15[v] = (UINT16)(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
	}
	if (disp_log) fclose(disp_log);
	disp_log = path ? fopen(path, disp_opened ? "ab" : "wb") : NULL;
	disp_opened = 1;
	dmd_on = pinheck_game()->dmdHub != 0;
	if (dmd_on) {
		dmd_proof = pinheck_env("PINHECK_DMD_PROOF") != NULL;
		if (!dmd_log && pinheck_env("PINHECK_DMD_LOG")) dmd_log = fopen(pinheck_env("PINHECK_DMD_LOG"), dmd_log_opened++ ? "ab" : "wb");
		prop_set_pins(&prop, pinheck_dmd_pins_cb, NULL);
		if (!ser_log && pinheck_env("PINHECK_PROP_SERIAL")) ser_log = fopen(pinheck_env("PINHECK_PROP_SERIAL"), ser_opened++ ? "a" : "w");
		memset(&ser, 0, sizeof(ser));
		ser.level = 1;
		ser.bit = -1;
		prop_set_pins_mask(&prop, DMD_ALL_PINS | (ser_log ? PINHECK_SER_PIN : 0));
		if (!pinheck_env("PINHECK_LAZY") || atoi(pinheck_env("PINHECK_LAZY")) != 0) prop_set_pins_lazy(&prop, pinheck_dmd_lazy_cb, NULL, DMD_ALL_PINS);
		return;
	}
	prop_set_pins_lazy(&prop, NULL, NULL, 0);
	prop_set_pins(&prop, pinheck_disp_pins, NULL);
	prop_set_pins_mask(&prop, DISPLAY_P17 | DISPLAY_P20 | DISPLAY_P21 | DISPLAY_P22);
}

static void pinheck_disp_reset(void)
{
	if (dmd_on) {
		pinheck_dmd_init(&dmd, 1, NULL, 0, NULL, pinheck_dmd_sub, disp_log ? pinheck_dmd_frame : NULL);
		pinheck_dmd_init(&dmd_row, 0, prop.chip.hub, (uint32_t)pinheck_game()->dmdHub, NULL, pinheck_dmd_row_sub, NULL);
		dmd_row.hub_at = pinheck_dmd_hub_at;
		dmd_head = dmd_tail = 0;
		return;
	}
	pinheck_display_init(&disp, NULL, pinheck_disp_frame, pinheck_disp_config, pinheck_prop_log);
	pinheck_display_size(&disp, pinheck_game()->width, pinheck_game()->height); /* any other size is named at start */
	memset(disp_shown, 0, sizeof(disp_shown));
	disp_cfg_n = 0;
	pinheck_display_look(&disp_look, NULL, 0);
	disp_dirty = 1;
}

static void pinheck_disp_stop(void)
{
	if (disp_log) fclose(disp_log);
	disp_log = NULL;
}

PINMAME_VIDEO_UPDATE(pinheck_video)
{
	const int x0 = layout->left, y0 = layout->top, h = (int)(disp.frame / DISPLAY_W);
	int x, y;
	prop_sync(&prop);
	(void)cliprect;
#if !defined(LIBPINMAME) && PINHECK_VIDEO_SCALE == 2
	/* the 128x32 module's look; the 128x64 module and libpinmame get the raw frame */
	if (h == DISPLAY_H) {
		if (disp_dirty) pinheck_display_render(&disp_look, disp_shown, disp_img);
		disp_dirty = 0;
		for (y = 0; y < DISPLAY_LOOK_H && y0 + y < bitmap->height; y++)
			for (x = 0; x < DISPLAY_LOOK_W && x0 + x < bitmap->width; x++) {
				const uint8_t *p = disp_img + (y * DISPLAY_LOOK_W + x) * 3;
				if (bitmap->depth == 32) ((UINT32 *)bitmap->line[y0 + y])[x0 + x] = MAKE_RGB(p[0], p[1], p[2]);
				else ((UINT16 *)bitmap->line[y0 + y])[x0 + x] = (UINT16)(((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3));
			}
		return;
	}
#endif
	for (y = 0; y < h * PINHECK_VIDEO_SCALE && y0 + y < bitmap->height; y++)
		for (x = 0; x < DISPLAY_W * PINHECK_VIDEO_SCALE && x0 + x < bitmap->width; x++) {
			const uint8_t v = disp_shown[(y / PINHECK_VIDEO_SCALE) * DISPLAY_W + x / PINHECK_VIDEO_SCALE];
			if (bitmap->depth == 32) ((UINT32 *)bitmap->line[y0 + y])[x0 + x] = disp_rgb32[v];
			else ((UINT16 *)bitmap->line[y0 + y])[x0 + x] = disp_rgb15[v];
		}
}

/* test hook PINHECK_TIME_LOG: emulated and host time per vblank */
static FILE *time_log;

/* keep the worker thread while it is faster (prop_governor) */
static prop_gov gov;

static double pinheck_host_s(void) { return (double)osd_cycles() / (double)osd_cycles_per_second(); }

static INTERRUPT_GEN(pinheck_vblank)
{
	if (time_log) fprintf(time_log, "%.6f %.6f\n", timer_get_time(), (double)osd_cycles() / (double)osd_cycles_per_second());
	if (!locals.idle) prop_governor(&prop, &gov, pinheck_host_s(), timer_get_time());
	if (!locals.idle) pinheck_brd_vblank();
	if (!locals.idle && dmd_on) pinheck_dmd_vblank();
	core_updateSw(0);
}

static int hex_bad;   /* the game's Intel HEX did not convert: the machine refuses to run */
static long hex_bytes; /* data bytes programmed */

/* program flash once per launch from PINHECK_HEXREGION */
void pinheck_flash_hex(void)
{
	char err[96], msg[200];
	long n;
	hex_bad = 0;
	hex_bytes = 0;
	if (!memory_region(PINHECK_HEXREGION) || !memory_region(PINHECK_CPUREGION)) return;
	n = pinheck_hex_flash(memory_region(PINHECK_HEXREGION), memory_region_length(PINHECK_HEXREGION), memory_region(PINHECK_CPUREGION),
	                      memory_region_length(PINHECK_CPUREGION), 0x1D000000u, err);
	if (n >= 0) {
		hex_bytes = n;
		return;
	}
	hex_bad = 1;
	sprintf(msg, "pinheck: %.16s: the PIC32 image (Intel HEX) does not convert: %.80s", Machine->gamedrv->name, err);
	pinheck_warn(msg);
}

/* sound: Propeller DUTY counters on P15/P14 integrated by audio.c */
static audio snd;
static struct {
	int started, rate;
	uint64_t samples;
	FILE *wav, *lag;
	uint32_t wav_bytes;
} sndl;

static void pinheck_snd_ctr(void *ctx, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq) { (void)ctx; audio_ctr(&snd, t, cog, ctr, ctr_reg, frq); }
static void pinheck_snd_pins(void *ctx, uint64_t t, uint32_t out, uint32_t dir) { (void)ctx; audio_pins(&snd, t, out, dir); }
static void pinheck_snd_log(void *ctx, const char *msg) { pinheck_prop_log(ctx, msg); }

static void pinheck_wav_le(FILE *f, uint32_t v, int n)
{
	while (n--) { fputc((int)(v & 0xFF), f); v >>= 8; }
}

static void pinheck_wav_header(FILE *f, uint32_t rate, uint32_t bytes)
{
	fseek(f, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, f); pinheck_wav_le(f, 36 + bytes, 4); fwrite("WAVEfmt ", 1, 8, f);
	pinheck_wav_le(f, 16, 4); pinheck_wav_le(f, 1, 2); pinheck_wav_le(f, 2, 2); pinheck_wav_le(f, rate, 4);
	pinheck_wav_le(f, rate * 4, 4); pinheck_wav_le(f, 4, 2); pinheck_wav_le(f, 16, 2);
	fwrite("data", 1, 4, f); pinheck_wav_le(f, bytes, 4);
	fseek(f, 0, SEEK_END);
}

static void pinheck_snd_update(int param, INT16 **buffer, int length)
{
	int16_t tmp[2 * 512];
	int done = 0, i;
	uint64_t now, t0, t1;
	(void)param;
	if (locals.idle) {
		memset(buffer[0], 0, length * sizeof(INT16));
		memset(buffer[1], 0, length * sizeof(INT16));
		return;
	}
	/* render up to the current emulated time; the per-frame sample count varies */
	now = pic32cpu_soc()->cpu.cycles;
	prop_catch_up(&prop, now);
	t0 = snd.t_render;
	t1 = prop_time(&prop, now);
	if (t1 < t0) t1 = t0;
	while (done < length) {
		int n = length - done > 512 ? 512 : length - done;
		audio_render(&snd, tmp, n, t0 + (t1 - t0) * (uint64_t)(done + n) / (uint64_t)length);
		for (i = 0; i < n; i++) {
			buffer[0][done + i] = tmp[2 * i];
			buffer[1][done + i] = tmp[2 * i + 1];
			if (sndl.wav) { pinheck_wav_le(sndl.wav, (uint16_t)tmp[2 * i], 2); pinheck_wav_le(sndl.wav, (uint16_t)tmp[2 * i + 1], 2); }
		}
		if (sndl.wav) sndl.wav_bytes += 4 * (uint32_t)n;
		sndl.samples += (uint64_t)n;
		done += n;
	}
	if (sndl.lag) fprintf(sndl.lag, "%llu %lld\n", (unsigned long long)now, (long long)(t1 - snd.t_render));
}

static int pinheck_sh_start(const struct MachineSound *msound)
{
	const char *names[] = { "Propeller Left", "Propeller Right" };
	const int vol[2] = { MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) };
	const char *wav = pinheck_env("PINHECK_WAV"), *lag = pinheck_env("PINHECK_SND_LAG");
	(void)msound;
	memset(&sndl, 0, sizeof(sndl));
	if (Machine->sample_rate <= 0) return 0;
	sndl.rate = Machine->sample_rate;
	audio_init(&snd, sndl.rate, pinheck_snd_log, NULL);
	if (wav && (sndl.wav = fopen(wav, "wb")) != NULL) pinheck_wav_header(sndl.wav, (uint32_t)sndl.rate, 0);
	if (lag) sndl.lag = fopen(lag, "w");
	sndl.started = 1;
	return stream_init_multi(2, names, vol, sndl.rate, 0, pinheck_snd_update) < 0;
}

static void pinheck_sh_stop(void)
{
	if (sndl.wav) {
		pinheck_wav_header(sndl.wav, (uint32_t)sndl.rate, sndl.wav_bytes);
		fclose(sndl.wav);
		sndl.wav = NULL;
	}
	if (sndl.lag) fclose(sndl.lag);
	sndl.lag = NULL;
	sndl.started = 0;
}

static struct CustomSound_interface pinheck_sndInt = { pinheck_sh_start, pinheck_sh_stop, 0 };

static MACHINE_INIT(pinheck)
{
	pic32mx_board board = { NULL, pinheck_port_write, pinheck_port_read, pinheck_uart_tx, pinheck_i2c_pins, pinheck_unmapped, pinheck_exception, pinheck_hold, pinheck_port_settle };
	const char *log = pinheck_env("PINHECK_UART1_LOG"), *plog = pinheck_env("PINHECK_PROP_LOG");

	prop_stop_thread(&prop);
#ifdef PINMAME_JIT_ASMJIT
	p8x32a_jit_free(prop.chip.jit);
	prop.chip.jit = NULL;
#endif
	if (locals.uart1) fclose(locals.uart1);
	if (locals.proplog) fclose(locals.proplog);
	memset(&locals, 0, sizeof(locals));
	if (log && (locals.uart1 = fopen(log, opened ? "ab" : "wb")) != NULL) setvbuf(locals.uart1, NULL, _IONBF, 0);
	if (plog && (locals.proplog = fopen(plog, opened ? "a" : "w")) != NULL) setvbuf(locals.proplog, NULL, _IONBF, 0);
	opened = 1;
	locals.reset_at = !reset_done && pinheck_env("PINHECK_RESET_AT") ? atof(pinheck_env("PINHECK_RESET_AT")) : 0.0;
	/* the core draws CORE_DMD layouts from its PWM integration */
	if (pinheck_game()->dmdHub) core_dmd_pwm_init(core_gameData->lcdLayout, CORE_DMD_PWM_FILTER_PINHECK_16, CORE_DMD_PWM_COMBINER_SUM_16, 0);
	if (hex_bad && memory_region(PINHECK_HEXREGION)) {
		locals.idle = 1;
		usrintf_showmessage_secs(PINHECK_REFUSE_SECS, "%.16s: the PIC32 image (Intel HEX) does not convert.", Machine->gamedrv->name);
		return;
	}
	pinheck_unsupported();
	memcpy(propmem, memory_region(PINHECK_PROPREGION), 0x8000);
	prop_init(&prop, memory_region(PINHECK_BIOSREGION), propmem);
	prop.chip.jn_off = pinheck_env("PINHECK_JOURNAL") && atoi(pinheck_env("PINHECK_JOURNAL")) == 0;
#ifdef PINMAME_JIT_ASMJIT
	if ((!pinheck_env("PINHECK_JIT") || atoi(pinheck_env("PINHECK_JIT")) != 0) && (prop.chip.jit = p8x32a_jit_new()) != NULL)
		prop.chip.jit_build = p8x32a_jit_build;
#endif
	prop_set_log(&prop, pinheck_prop_log, NULL);
	prop_set_tx(&prop, pinheck_prop_tx, NULL);
	/* a reset re-runs MACHINE_INIT but not the sound start: restart the audio's time base with the Propeller's */
	if (sndl.started) audio_init(&snd, sndl.rate, pinheck_snd_log, NULL);
	if (sndl.started) prop_set_sound(&prop, pinheck_snd_ctr, pinheck_snd_pins, NULL);
	if (sndl.started && pinheck_env("PINHECK_SND_SELFTEST")) {
		audio_pins(&snd, 0, 0, 1u << AUDIO_PIN_L);
		audio_ctr(&snd, 0, 7, 0, (2u << 26) | AUDIO_PIN_L, 0);
		audio_ctr(&snd, 0, 7, 0, 0, 0);
		audio_pins(&snd, 0, 0, 0);
	}
	boot_init(&boot, memory_region(PINHECK_CPUREGION), memory_region_length(PINHECK_CPUREGION), pinheck_boot_tx, NULL);
	boot_set_log(&boot, pinheck_prop_log, NULL);
	if (hex_bytes) {
		char msg[64];
		sprintf(msg, "hex: %ld bytes of program flash", hex_bytes);
		pinheck_prop_log(NULL, msg);
	}
	boot_set_window(&boot, (uint64_t)pinheck_game()->bootHold * (PINHECK_CLOCK / 1000));
	pinheck_open_card();
	pinheck_disp_init();
	pinheck_brd_init();
	if (!link_log && pinheck_env("PINHECK_LINK_LOG")) link_log = fopen(pinheck_env("PINHECK_LINK_LOG"), link_opened ? "a" : "w");
	link_opened = 1;
	if (!time_log && pinheck_env("PINHECK_TIME_LOG")) time_log = fopen(pinheck_env("PINHECK_TIME_LOG"), "w");
	pic32cpu_set_board(&board);
	prop_set_clock(&prop, pinheck_pic_now, NULL);
	memset(&gov, 0, sizeof(gov));
	if (!pinheck_env("PINHECK_THREADS") || atoi(pinheck_env("PINHECK_THREADS")) != 0)
		prop_gov_start(&prop, &gov, pinheck_host_s(), pinheck_env("PINHECK_THREAD_FLIP") != NULL);
}

static MACHINE_RESET(pinheck)
{
	const char *at = pinheck_env("PINHECK_UART1_SEND_AT");
	if (locals.idle) return;
	prop_reset(&prop, 0);
	boot_reset(&boot, 0);
	pinheck_disp_reset();
	memset(&lnk, 0, sizeof(lnk));
	cat24m01_init(&u13, u13mem, 0);
	/* test hook PINHECK_RTC: start the clock at this Unix time, for repeatable runs */
	ds1340_init(&rtc, pinheck_env("PINHECK_RTC") ? strtoll(pinheck_env("PINHECK_RTC"), NULL, 10) : pinheck_local_now(), PINHECK_CLOCK);
	locals.rtc_at = 0;
	locals.send = pinheck_env("PINHECK_UART1_SEND");
	locals.send_at = (uint64_t)((at ? atof(at) : 0.0) * PINHECK_CLOCK);
	at = pinheck_env("PINHECK_UART1_SEND_GAP");
	locals.send_gap = (uint64_t)((at ? atof(at) : 1.0) * PINHECK_CLOCK);
	pinheck_brd_reset();
}

static NVRAM_HANDLER(pinheck)
{
	const int first = !read_or_write && !file;
	/* also while idle: the load runs before MACHINE_INIT, and an idle session must save what it loaded */
	prop_sync(&prop);
	core_nvram(file, read_or_write, u13mem, sizeof(u13mem), 0xFF);
	core_nvram(file, read_or_write, propmem + 0x8000, sizeof(propmem) - 0x8000, 0xFF);
	if (first && pinheck_env("PINHECK_INSERVICE") && pinheck_game()->inService) pinheck_in_service(propmem, core_gameData->hw.gameSpecific1);
}

static MACHINE_STOP(pinheck)
{
	if (!locals.idle) boot_stop(&boot);
	prop_stop_thread(&prop);
#ifdef PINMAME_JIT_ASMJIT
	p8x32a_jit_free(prop.chip.jit);
	prop.chip.jit = NULL;
	prop.chip.jit_build = NULL;
#endif
	if (!locals.idle && dmd_on) pinheck_dmd_stop();
	if (locals.uart1) fclose(locals.uart1);
	if (locals.proplog) fclose(locals.proplog);
	locals.uart1 = locals.proplog = NULL;
	hex_bytes = 0; /* the next session's game may have no HEX */
	if (locals.have_vol) vfat_free(&vol);
	if (locals.have_zip) zipsrc_close(&zip);
	locals.have_vol = locals.have_zip = 0;
	pinheck_disp_stop();
	pinheck_brd_stop();
	if (time_log) fclose(time_log);
	time_log = NULL;
	if (link_log) fclose(link_log);
	link_log = NULL;
	locals.idle = 0; /* the next session's NVRAM load runs before MACHINE_INIT clears locals */
}

static MEMORY_READ32_START(pinheck_readmem)
	{ 0x00000000, 0x00000003, MRA32_NOP },
MEMORY_END

static MEMORY_WRITE32_START(pinheck_writemem)
	{ 0x00000000, 0x00000003, MWA32_NOP },
MEMORY_END

MACHINE_DRIVER_START(PINHECK)
	MDRV_IMPORT_FROM(PinMAME)
	MDRV_CORE_INIT_RESET_STOP(pinheck, pinheck, pinheck)
	MDRV_CPU_ADD_TAG("mcpu", PIC32MX, PINHECK_CLOCK)
	MDRV_CPU_MEMORY(pinheck_readmem, pinheck_writemem)
	MDRV_CPU_VBLANK_INT(pinheck_vblank, 1)
	MDRV_TIMER_ADD(pinheck_tick, 1000)
	MDRV_NVRAM_HANDLER(pinheck)
	MDRV_SWITCH_UPDATE(pinheck)
	MDRV_SWITCH_CONV(pinheck_sw2m, pinheck_m2sw)
	MDRV_LAMP_CONV(pinheck_lamp2m, pinheck_m2sw)
	MDRV_SOUND_ADD(CUSTOM, pinheck_sndInt)
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
MACHINE_DRIVER_END

/* raw DMD (dmdHub): the core draws it with its palette pens */
MACHINE_DRIVER_START(PINHECKDMD)
	MDRV_IMPORT_FROM(PINHECK)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
MACHINE_DRIVER_END
