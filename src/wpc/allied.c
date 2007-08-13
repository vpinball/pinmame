/************************************************************************************************
 Allied Leisure
 --------------
 by Gaston

   Hardware:
   ---------
		CPU:	 M6504 (same as 6502 but with only 13 address lines)
				 No Eproms, but 3 x 6530 RRIOT chips with rom & ram content instead
				 5 x 6520 PIA
		DISPLAY: 7-segment LED panels, driven by serial to parallel converters
		SOUND:	 Chimes
 ************************************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6821pia.h"
#include "machine/6530riot.h"
#include "machine/4094.h"
#include "core.h"

#define ALI_IC1 0
#define ALI_IC2 1
#define ALI_IC4 2
#define ALI_IC7 3
#define ALI_IC8 4

#define ALI_IC3 0
#define ALI_IC5 1
#define ALI_IC6 2

static int bitToNum(UINT8 tmp) {
  return (tmp & 0x10) ? 5 : (tmp & 0x08) ? 4 : (tmp & 0x04) ? 3 : (tmp & 0x02) ? 2 : (tmp & 0x01) ? 1 : 0;
}

static struct {
  core_tSeg segments;
  UINT32 solenoids;
  int dipSel;
  int dispSel;
  int coin[3], players, balls, test, slam, mode, match, credunit, credtens, cred1, cred2, cred3, credOp;
  int repl1[3], repl2[3], repl3[3];
} locals;

static INTERRUPT_GEN(vblank) {
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids = locals.solenoids;
  core_updateSw(core_getSol(25));
}

static void piaIrq(int state) {
//  logerror("IRQ\n");
  cpu_set_irq_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static SWITCH_UPDATE(allied) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] & 0x80, 0x80, 4);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8,   0x1f, 5);
  }
  locals.test = (coreGlobals.swMatrix[5] & 0x10) ? 0 : 1;
  // J2-W (credit)
  pia_set_input_cb1(ALI_IC4, (coreGlobals.swMatrix[4] & 0x80) ? 0 : 1);
  // J2-20/X (coin 3)
  locals.coin[0] = (coreGlobals.swMatrix[5] & 0x02) ? 0 : 1;
  pia_set_input_ca1(ALI_IC8, locals.coin[0]);
  // J2-21/Y (coin 2)
  locals.coin[1] = (coreGlobals.swMatrix[5] & 0x04) ? 0 : 1;
  pia_set_input_cb1(ALI_IC8, locals.coin[1]);
  // J2-22/Z (coin 1)
  locals.coin[2] = (coreGlobals.swMatrix[5] & 0x08) ? 0 : 1;
  pia_set_input_ca2(ALI_IC8, locals.coin[2]);
  // J1-16 / J2-b (slam)
  locals.slam = (coreGlobals.swMatrix[5] & 0x01) ? 0 : 1;
  pia_set_input_cb1(ALI_IC7, locals.slam);
  pia_set_input_cb2(ALI_IC8, locals.slam);
}

/* PB0: to J2-8 (left 1,000 pts)
   PB1: to J2-9 (left 2,000 pts)
   PB2: to J2-10 (left 3,000 pts)
   PB3: to J2-M (left 4,000 pts)
   PB4: NC
   PB5: NC
   PB6: to J2-12 (double bonus)
   PB7: to J2-P (triple bonus)
*/
static WRITE_HANDLER(ic1_b_w) {
//  logerror("IC#1 B w: %02x\n", data);
  coreGlobals.tmpLampMatrix[2] = data;
}

/* to J6-10 */
static WRITE_HANDLER(ic1_ca2_w) {
//  logerror("IC#1 CA2 w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xfbffffff) | (!data << 26);
}

/* to J2-N (10,000 bonus) */
static WRITE_HANDLER(ic1_cb2_w) {
//  logerror("IC#1 CB2 w: %02x\n", data);
  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0xfb) | (data << 2);
}

/* PA0-4: NC
   PA5: from J1-C (bullseye target)
   PA6: from J1-4
   PA7: from J1-D (ball in play)
*/
static READ_HANDLER(ic1_a_r) {
//  logerror("IC#1 A r\n");
  return ~coreGlobals.swMatrix[2];
}

/* 3rd 100,000 replay (strobed) */
static READ_HANDLER(ic1_ca1_r) {
//  logerror("IC#1 CA1 r\n");
  return locals.repl3[2];
}

/* Number of balls (strobed) */
static READ_HANDLER(ic1_cb1_r) {
//  logerror("IC#1 CB1 r\n");
  return locals.balls;
}

/* PB0: to J2-13 (right 1,000 pts)
   PB1: to J2-14 (right 2,000 pts)
   PB2: to J2-S (right 3,0000 pts)
   PB3: to J2-R (right 4,000 pts)
   PB4: to J2-15 (special when lit)
   PB5: NC
   PB6: to J2-U (extra ball when lit)
   PB7: to J1-CC (display data bit & LED2)
*/
static WRITE_HANDLER(ic2_b_w) {
//  logerror("IC#2 B w: %02x\n", data);
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x0d) | ((data & 0x80) >> 6);
  HC4094_data_w(0, data >> 7);
  HC4094_strobe_w(0, 1);
  HC4094_data_w(3, data >> 7);
  HC4094_strobe_w(3, 1);
  HC4094_data_w(6, data >> 7);
  HC4094_strobe_w(6, 1);
  HC4094_data_w(9, data >> 7);
  HC4094_strobe_w(9, 1);
  HC4094_data_w(12, data >> 7);
  HC4094_strobe_w(12, 1);
  HC4094_data_w(13, data >> 7);
  HC4094_strobe_w(13, 1);
  coreGlobals.tmpLampMatrix[3] = data & 0x7f;
}

/* 4th bit of extra BCD strobe (display clock input) */
static WRITE_HANDLER(ic2_cb2_w) {
//  logerror("IC#2 CB2 w: %02x\n", data);
  if (locals.dispSel > 0 && locals.dispSel < 6) {
    HC4094_clock_w((locals.dispSel-1) * 3, !data);
    if (locals.dispSel < 5) {
      HC4094_clock_w((locals.dispSel-1) * 3 + 1, !data);
      HC4094_clock_w((locals.dispSel-1) * 3 + 2, !data);
    }
  } else if (!locals.dispSel) {
    HC4094_clock_w(13, !data);
    HC4094_clock_w(14, !data);
    HC4094_clock_w(15, !data);
  }
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x0e) | !locals.dispSel;
}

/* PA0: from J1-J (left thumper bumper)
   PA1: from J1-8 (center thumper bumper)
   PA2: from J1-H (right thumper bumper)
   PA3: from J1-7 (left bullseye)
   PA4: from J1-F (right bullseye)
   PA5: from J1-6 (left slingshot)
   PA6: from J1-E (right slingshot)
   PA7: from J1-5 (outhole)
*/
static READ_HANDLER(ic2_a_r) {
//  logerror("IC#2 A r\n");
  return ~coreGlobals.swMatrix[1];
}

/* Max. credits tens (strobed) */
static READ_HANDLER(ic2_ca1_r) {
//  logerror("IC#2 CA1 r\n");
  return locals.credtens;
}

/* Max. credits units (strobed) */
static READ_HANDLER(ic2_cb1_r) {
//  logerror("IC#2 CB1 r\n");
  return locals.credunit;
}

/* PB0: to J2-6 (player #1 LED)
   PB1: to J2-H (player #2 LED)
   PB2: to J2-F (player #3 LED)
   PB3: to J2-7 (player #4 LED)
   PB4: to J1-25 (score #1 blanking & LED3)
   PB5: to J1-b (score #2 blanking)
   PB6: to J1-24 (score #3 blanking)
   PB7: to J1-a (score #4 blanking)
*/
static WRITE_HANDLER(ic4_b_w) {
//  logerror("IC#4 B w: %02x\n", data);
  coreGlobals.tmpLampMatrix[5] = (coreGlobals.tmpLampMatrix[5] & 0xf0)| (data & 0x0f);
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x0b) | ((data & 0x10) >> 2);
  if (data & 0x10) {
    locals.segments[0].w = locals.segments[1].w = locals.segments[2].w = locals.segments[3].w = locals.segments[4].w = locals.segments[5].w = 0;
  } else {
    HC4094_oe_w(0, 0); HC4094_oe_w(1, 0); HC4094_oe_w(2, 0);
    HC4094_oe_w(0, 1); HC4094_oe_w(1, 1); HC4094_oe_w(2, 1);
  }
  if (data & 0x20) {
    locals.segments[6].w = locals.segments[7].w = locals.segments[8].w = locals.segments[9].w = locals.segments[10].w = locals.segments[11].w = 0;
  } else {
    HC4094_oe_w(3, 0); HC4094_oe_w(4, 0); HC4094_oe_w(5, 0);
    HC4094_oe_w(3, 1); HC4094_oe_w(4, 1); HC4094_oe_w(5, 1);
  }
  if (data & 0x40) {
    locals.segments[12].w = locals.segments[13].w = locals.segments[14].w = locals.segments[15].w = locals.segments[16].w = locals.segments[17].w = 0;
  } else {
    HC4094_oe_w(6, 0); HC4094_oe_w(7, 0); HC4094_oe_w(8, 0);
    HC4094_oe_w(6, 1); HC4094_oe_w(7, 1); HC4094_oe_w(8, 1);
  }
  if (data & 0x80) {
    locals.segments[18].w = locals.segments[19].w = locals.segments[20].w = locals.segments[21].w = locals.segments[22].w = locals.segments[23].w = 0;
  } else {
    HC4094_oe_w(9, 0); HC4094_oe_w(10, 0); HC4094_oe_w(11, 0);
    HC4094_oe_w(9, 1); HC4094_oe_w(10, 1); HC4094_oe_w(11, 1);
  }
  locals.segments[26].w = core_bcd2seg7e[bitToNum(data & 0x0f)];
}

/* to J6-15 (slingshot DC lights) */
static WRITE_HANDLER(ic4_ca2_w) {
//  logerror("IC#4 CA2 w: %02x\n", data);
  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0x7f) | (!data << 7);
}


/* PA0: from J3-24/b (drop target a)
   PA1: from J3-22/Z (drop target b)
   PA2: from J1-9 (drop target c)
   PA3: from J1-K (drop target d)
   PA4: NC
   PA5: NC
   PA6: from J1-11 (roll tilt)
   PA7: from J2-W (credit)
*/
static READ_HANDLER(ic4_a_r) {
//  logerror("IC#4 A r\n");
  return ~coreGlobals.swMatrix[4];
}

/* Credit 1 prog. (strobed) */
static READ_HANDLER(ic4_ca1_r) {
//  logerror("IC#4 CA1 r\n");
  return locals.cred1;
}

/* PB0-PB3: dip strobe, also
            to J4-3 (game over light) and
            to 1,000 bonus - 9,000 bonus lamps
   PB4-PB6: display strobe (display select)
   PB7: to J4-L (tilt lamp)
*/
static WRITE_HANDLER(ic7_b_w) {
//  logerror("IC#7 B w: %02x\n", data);
  locals.dipSel = data & 0x0f;
  // IC#7 CA1 = 2/4 Players
  locals.players = !(((core_getDip(0) & 0x02) ? 4 : 2) == locals.dipSel);
  pia_set_input_ca1(ALI_IC7, locals.players);
  // IC#6 PA2 = Replay / Add-a-ball
  locals.mode = (core_getDip(0) & 0x04) ? 0 : 1;
  // IC#6 PA3 = Match inhibit
  locals.match = (core_getDip(0) & 0x08) ? 0 : 1;
  // IC#1 CB1 = Number of balls
  locals.balls = !((core_getDip(0) >> 4) == locals.dipSel);
  pia_set_input_cb1(ALI_IC1, locals.balls);
  // IC#2 CA1 = Max. credit tens
  locals.credtens = !((core_getDip(1) & 0x0f) == locals.dipSel);
  pia_set_input_ca1(ALI_IC2, locals.credtens);
  // IC#2 CB1 = Max. credit units
  locals.credunit = !((core_getDip(1) >> 4) == locals.dipSel);
  pia_set_input_cb1(ALI_IC2, locals.credunit);
  // IC#4 CA1 = Credit 1 prog.
  locals.cred1 = !((core_getDip(2) & 0x0f) == locals.dipSel);
  pia_set_input_ca1(ALI_IC4, locals.cred1);
  // IC#6 PA1 = Credit 2 prog.
  locals.cred2 = !((core_getDip(2) >> 4) == locals.dipSel);
  // IC#6 PB4 = Credit 3 prog.
  locals.cred3 = !((core_getDip(3) & 0x0f) == locals.dipSel);
  // IC#6 PA0 = Credit options
  locals.credOp = !((core_getDip(3) >> 4) == locals.dipSel);
  // IC#5 PA0 = 1st 1,000 replay
  locals.repl1[0] = !((core_getDip(4) & 0x0f) == locals.dipSel);
  // IC#5 PA1 = 1st 10,000 replay
  locals.repl1[1] = !((core_getDip(4) >> 4) == locals.dipSel);
  // IC#5 PA2 = 1st 100,000 replay
  locals.repl1[2] = !((core_getDip(5) & 0x0f) == locals.dipSel);
  // IC#5 PA3 = 2nd 1,000 replay
  locals.repl2[0] = !((core_getDip(5) >> 4) == locals.dipSel);
  // IC#5 PA4 = 2nd 10,000 replay
  locals.repl2[1] = !((core_getDip(6) & 0x0f) == locals.dipSel);
  // IC#5 PA5 = 2nd 100,000 replay
  locals.repl2[2] = !((core_getDip(6) >> 4) == locals.dipSel);
  // IC#5 PA6 = 3rd 1,000 replay
  locals.repl3[0] = !((core_getDip(7) & 0x0f) == locals.dipSel);
  // IC#5 PA7 = 3rd 10,000 replay
  locals.repl3[1] = !((core_getDip(7) >> 4) == locals.dipSel);
  // IC#1 CA1 = 3rd 100,000 replay
  locals.repl3[2] = !((core_getDip(8) & 0x0f) == locals.dipSel);
  pia_set_input_ca1(ALI_IC1, locals.repl3[2]);

  if (locals.dipSel < 8) {
    coreGlobals.tmpLampMatrix[0] = 1 << locals.dipSel;
    coreGlobals.tmpLampMatrix[1] &= 0xfc;
  } else if (locals.dipSel < 10) {
    coreGlobals.tmpLampMatrix[0] = 0;
    coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0xfc) | (1 << (locals.dipSel - 8));
  } else {
    coreGlobals.tmpLampMatrix[0] = 0;
    coreGlobals.tmpLampMatrix[1] &= 0xfc;
  }

  locals.dispSel = data >> 4;
  if ((data & 0x0f) == 0x0f) {
    switch (locals.dispSel) {
      case 0:  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0x87) | 0x10; break;
      case 6:  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0x87) | 0x40; break;
      case 7:  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0x87) | 0x20; break;
      case 8: case 9: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e:
               coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & 0x87) | 0x08; break;
    }
  } else {
    coreGlobals.tmpLampMatrix[1] &= 0x87;
  }
}

/* to J4-2 */
static WRITE_HANDLER(ic7_ca2_w) {
//  logerror("IC#7 CA2 w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xfdffffff) | (!data << 25);
}

/* to J4-4 (flipper power relay) */
static WRITE_HANDLER(ic7_cb2_w) {
//  logerror("IC#7 CB2 w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xfeffffff) | (!data << 24);
}

/* PA0: from J1-12 (raise target a)
   PA1: from J1-N (raise target d)
   PA2: from J1-13
   PA3: from J1-P
   PA4: from J1-14 (500 pts rollover)
   PA5: from J1-R (raise target b)
   PA6: from J1-15 (raise target c)
   PA7: from J1-S (extra ball when lit)
*/
static READ_HANDLER(ic7_a_r) {
//  logerror("IC#7 A r\n");
  return ~coreGlobals.swMatrix[3];
}

/* from J1-T (2/4 player select) */
static READ_HANDLER(ic7_ca1_r) {
//  logerror("IC#7 CA1 r\n");
  return locals.players;
}

/* PA0: to J5-3 (left thumper bumper)
   PA1: to J5-2 (center thumper bumper)
   PA2: to J5-13 (right thumper bumper)
   PA3: NC
   PA4: NC
   PA5: to J5-15 (left slingshot)
   PA6: to J5-7 (right slingshot)
   PA7: to J5-12 (outhole kicker)
*/
static WRITE_HANDLER(ic8_a_w) {
//  logerror("IC#8 A w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xffffff00) | (data ^ 0xff);
}

/* PB0: to J2-17 (ball #1 LED)
   PB1: to J3-1 (ball #2 LED)
   PB2: to J3-2 (ball #3 LED)
   PB3: to J3-B (ball #4 LED)
   PB4: to J3-A (ball #5 LED)
   PB5: to J3-3 (same player shoots again)
   PB5: NC
   PB5: NC
*/
static WRITE_HANDLER(ic8_b_w) {
//  logerror("IC#8 B w: %02x\n", data);
  coreGlobals.tmpLampMatrix[4] = data;
  locals.segments[27].w = core_bcd2seg7e[bitToNum(data & 0x1f)];
}

static struct pia6821_interface allied_pia[] = {{
 /* 6520 PIA 0 Chip IC1 */
 /* in:  A/B,CA1/B1,CA2/B2 */ ic1_a_r, 0, ic1_ca1_r, ic1_cb1_r, 0, 0,
 /* out: A/B,CA2/B2        */ 0, ic1_b_w, ic1_ca2_w, ic1_cb2_w,
 /* irq: A/B               */ piaIrq, piaIrq
},{
 /* 6520 PIA 1 Chip IC2 */
 /* in:  A/B,CA1/B1,CA2/B2 */ ic2_a_r, 0, ic2_ca1_r, ic2_cb1_r, 0, 0,
 /* out: A/B,CA2/B2        */ 0, ic2_b_w, 0, ic2_cb2_w,
 /* irq: A/B               */ piaIrq, piaIrq
},{
 /* 6520 PIA 2 Chip IC4 */
 /* in:  A/B,CA1/B1,CA2/B2 */ ic4_a_r, 0, ic4_ca1_r, 0, 0, 0,
 /* out: A/B,CA2/B2        */ 0, ic4_b_w, ic4_ca2_w, 0,
 /* irq: A/B               */ piaIrq, piaIrq
},{
 /* 6520 PIA 3 Chip IC7 */
 /* in:  A/B,CA1/B1,CA2/B2 */ ic7_a_r, 0, ic7_ca1_r, 0, 0, 0,
 /* out: A/B,CA2/B2        */ 0, ic7_b_w, ic7_ca2_w, ic7_cb2_w,
 /* irq: A/B               */ piaIrq, piaIrq
},{
 /* 6520 PIA 4 Chip IC8 */
 /* in:  A/B,CA1/B1,CA2/B2 */ 0, 0, 0, 0, 0, 0,
 /* out: A/B,CA2/B2        */ ic8_a_w, ic8_b_w, 0, 0,
 /* irq: A/B               */ piaIrq, piaIrq
}};

/* PB0: to J6-3 (total play counter)
   PB1: to J6-6 (total replay counter)
   PB2: to J6-13 (1,000 pts chime)
   PB3: to J6-1 (100 pts chime)
   PB4: to J6-2 (10 pts chime)
   PB5: to J6-14 (replay knocker)
   PB7: IRQ
*/
static WRITE_HANDLER(ic5_b_w) {
//  logerror("IC#5 B w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xff00ffff) | ((data ^ 0x3f) << 16);
}

/* PA0: 1st 1,000 replay (strobed)
   PA1: 1st 10,000 replay (strobed)
   PA2: 1st 100,000 replay (strobed)
   PA3: 2nd 1,000 replay (strobed)
   PA4: 2nd 10,000 replay (strobed)
   PA5: 2nd 100,000 replay (strobed)
   PA6: 3rd 1,000 replay (strobed)
   PA7: 3rd 10,000 replay (strobed)
*/
static READ_HANDLER(ic5_a_r) {
  UINT8 data = locals.repl1[0] | (locals.repl1[1] << 1) | (locals.repl1[2] << 2) |
        (locals.repl2[0] << 3) | (locals.repl2[1] << 4) | (locals.repl2[2] << 5) |
        (locals.repl3[0] << 6) | (locals.repl3[1] << 7);
  logerror("IC#5 A r = %02x\n", data);
  return data;
}

/* PB6: Chip select (not needed) */
static READ_HANDLER(ic5_b_r) {
  UINT8 data = 0xff;
  logerror("IC#5 B r = %02x\n", data);
  return data;
}

/* PB0: to J5-9 (drop target a)
   PB1: to J5-4 (drop target b)
   PB2: to J5-1 (drop target c)
   PB3: to J5-6 (drop target d)
   PB7: IRQ
*/
static WRITE_HANDLER(ic6_b_w) {
//  logerror("IC#6 B w: %02x\n", data);
  locals.solenoids = (locals.solenoids & 0xffff00ff) | ((data ^ 0x9f) << 8);
}

/* PA0: Credit options (strobed)
   PA1: Credit 2 prog. (strobed)
   PA2: Replay / Add-a-ball
   PA3: Match inhibit
   PA4: from J2-20/X (coin 3)
   PA5: from J2-21/Y (coin 2)
   PA6: from J2-22/Z (coin 1)
   PA7: from J1-16 / J2-b (slam)
*/
static READ_HANDLER(ic6_a_r) {
  UINT8 data = locals.credOp | (locals.cred2 << 1) | (locals.mode << 2) | (locals.match << 3) |
    (locals.coin[2] << 4) | (locals.coin[1] << 5) | (locals.coin[0] << 6) | (locals.slam << 7);
  logerror("IC#6 A r = %02x\n", data);
  return data;
}

/* PB4: Credit 3 prog. (strobed)
   PB5: Run / Test
   PB6: Chip select (not needed)
*/
static READ_HANDLER(ic6_b_r) {
  UINT8 data = 0xcf | (locals.cred3 << 4) | (locals.test << 5);
  logerror("IC#6 B r = %02x\n", data);
  return data;
}

static struct riot6530_interface allied_riot[] = {{
 /* 6530 RIOT 1 Chip IC3 */
 /* in:  A/B */ 0, 0,
 /* out: A/B */ 0, 0,
 /* irq:     */ piaIrq
},{
 /* 6530 RIOT 1 Chip IC5 */
 /* in:  A/B */ ic5_a_r, ic5_b_r,
 /* out: A/B */ 0, ic5_b_w,
 /* irq:     */ piaIrq
},{
 /* 6530 RIOT 2 Chip IC6 */
 /* in:  A/B */ ic6_a_r, ic6_b_r,
 /* out: A/B */ 0, ic6_b_w,
 /* irq:     */ piaIrq
}};

static WRITE_HANDLER(disp_0_out) { data ^= 0xff; locals.segments[4].w = core_bcd2seg7[data & 0x0f]; locals.segments[3].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_0_out) { HC4094_data_w(1, data); HC4094_strobe_w(1, 1); }

static WRITE_HANDLER(disp_1_out) { data ^= 0xff; locals.segments[2].w = core_bcd2seg7[data & 0x0f]; locals.segments[1].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_1_out) { HC4094_data_w(2, data); HC4094_strobe_w(2, 1); }

static WRITE_HANDLER(disp_2_out) { data ^= 0xff; locals.segments[0].w = core_bcd2seg7[data & 0x0f]; locals.segments[5].w = core_bcd2seg7[0]; }

static WRITE_HANDLER(disp_3_out) { data ^= 0xff; locals.segments[10].w = core_bcd2seg7[data & 0x0f]; locals.segments[9].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_3_out) { HC4094_data_w(4, data); HC4094_strobe_w(4, 1); }

static WRITE_HANDLER(disp_4_out) { data ^= 0xff; locals.segments[8].w = core_bcd2seg7[data & 0x0f]; locals.segments[7].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_4_out) { HC4094_data_w(5, data); HC4094_strobe_w(5, 1); }

static WRITE_HANDLER(disp_5_out) { data ^= 0xff; locals.segments[6].w = core_bcd2seg7[data & 0x0f]; locals.segments[11].w = core_bcd2seg7[0]; }

static WRITE_HANDLER(disp_6_out) { data ^= 0xff; locals.segments[16].w = core_bcd2seg7[data & 0x0f]; locals.segments[15].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_6_out) { HC4094_data_w(7, data); HC4094_strobe_w(7, 1); }

static WRITE_HANDLER(disp_7_out) { data ^= 0xff; locals.segments[14].w = core_bcd2seg7[data & 0x0f]; locals.segments[13].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_7_out) { HC4094_data_w(8, data); HC4094_strobe_w(8, 1); }

static WRITE_HANDLER(disp_8_out) { data ^= 0xff; locals.segments[12].w = core_bcd2seg7[data & 0x0f]; locals.segments[17].w = core_bcd2seg7[0]; }

static WRITE_HANDLER(disp_9_out) { data ^= 0xff; locals.segments[22].w = core_bcd2seg7[data & 0x0f]; locals.segments[21].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_9_out) { HC4094_data_w(10, data); HC4094_strobe_w(10, 1); }

static WRITE_HANDLER(disp_10_out) { data ^= 0xff; locals.segments[20].w = core_bcd2seg7[data & 0x0f]; locals.segments[19].w = core_bcd2seg7[data >> 4]; }
static WRITE_HANDLER(qs_10_out) { HC4094_data_w(11, data); HC4094_strobe_w(11, 1); }

static WRITE_HANDLER(disp_11_out) { data ^= 0xff; locals.segments[18].w = core_bcd2seg7[data & 0x0f]; locals.segments[23].w = core_bcd2seg7[0]; }

static WRITE_HANDLER(disp_12_out) { data ^= 0xff; locals.segments[25].w = core_bcd2seg7e[data & 0x0f]; locals.segments[24].w = core_bcd2seg7e[data >> 4]; }

static WRITE_HANDLER(disp_13_out) { coreGlobals.tmpLampMatrix[7] = core_revbyte(data); }
static WRITE_HANDLER(qs_13_out) { HC4094_data_w(14, data); HC4094_strobe_w(14, 1); }

static WRITE_HANDLER(disp_14_out) { coreGlobals.tmpLampMatrix[6] = core_revbyte(data); }
static WRITE_HANDLER(qs_14_out) { HC4094_data_w(15, data); HC4094_strobe_w(15, 1); }

static WRITE_HANDLER(disp_15_out) { coreGlobals.tmpLampMatrix[5] = (coreGlobals.tmpLampMatrix[5] & 0x0f) | (core_revbyte(data) & 0xf0); }

static HC4094interface allied_74164 = {
  16, // 16 chips
  { disp_0_out, disp_1_out, disp_2_out, disp_3_out, disp_4_out, disp_5_out, disp_6_out, disp_7_out,
    disp_8_out, disp_9_out, disp_10_out, disp_11_out, disp_12_out, disp_13_out, disp_14_out, disp_15_out },
  { qs_0_out, qs_1_out, 0, qs_3_out, qs_4_out, 0, qs_6_out, qs_7_out, 0, qs_9_out, qs_10_out, 0, 0, qs_13_out, qs_14_out }
};

static MEMORY_READ_START(readmem)
  { 0x0000, 0x003f, MRA_RAM },
  { 0x0044, 0x0047, pia_r(ALI_IC2) },
  { 0x0048, 0x004b, pia_r(ALI_IC1) },
  { 0x0050, 0x0053, pia_r(ALI_IC7) },
  { 0x0060, 0x0063, pia_r(ALI_IC4) },
  { 0x0080, 0x008f, riot6530_1_r},
  { 0x00c0, 0x00c3, pia_r(ALI_IC8) },
  { 0x0100, 0x013f, MRA_RAM },
  { 0x0840, 0x084f, riot6530_2_r},
  { 0xf400, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x0000, 0x003f,	MWA_RAM, &generic_nvram, &generic_nvram_size},	/* fake NVRAM */
  { 0x0044, 0x0047, pia_w(ALI_IC2) },
  { 0x0048, 0x004b, pia_w(ALI_IC1) },
  { 0x0050, 0x0053, pia_w(ALI_IC7) },
  { 0x0060, 0x0063, pia_w(ALI_IC4) },
  { 0x0080, 0x008f, riot6530_1_w},
  { 0x00c0, 0x00c3, pia_w(ALI_IC8) },
  { 0x0100, 0x013f, MWA_RAM },
  { 0x0840, 0x084f, riot6530_2_w},
MEMORY_END

static MACHINE_INIT(allied) {
  memset(&locals, 0, sizeof(locals));

  pia_config(ALI_IC1, PIA_STANDARD_ORDERING, &allied_pia[0]);
  pia_config(ALI_IC2, PIA_STANDARD_ORDERING, &allied_pia[1]);
  pia_config(ALI_IC4, PIA_STANDARD_ORDERING, &allied_pia[2]);
  pia_config(ALI_IC7, PIA_STANDARD_ORDERING, &allied_pia[3]);
  pia_config(ALI_IC8, PIA_STANDARD_ORDERING, &allied_pia[4]);

  riot6530_config(ALI_IC3, &allied_riot[0]);
  riot6530_set_clock(ALI_IC3, Machine->drv->cpu[0].cpu_clock);
  riot6530_config(ALI_IC5, &allied_riot[1]);
  riot6530_set_clock(ALI_IC5, Machine->drv->cpu[0].cpu_clock);
  riot6530_config(ALI_IC6, &allied_riot[2]);
  riot6530_set_clock(ALI_IC6, Machine->drv->cpu[0].cpu_clock);
  riot6530_reset();

  HC4094_init(&allied_74164);
  HC4094_oe_w(12, 1);
  HC4094_oe_w(13, 1);
  HC4094_oe_w(14, 1);
  HC4094_oe_w(15, 1);

  coreGlobals.tmpLampMatrix[1] |= 0x80;
}

static MACHINE_STOP(allied) {
  riot6530_unconfig();
}

static MACHINE_DRIVER_START(allied)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(allied,NULL,allied)
  MDRV_CPU_ADD_TAG("mcpu", M6502, 3572549/4)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_DIPS(68)
  MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_UPDATE(allied)
  MDRV_NVRAM_HANDLER(generic_0fill)
MACHINE_DRIVER_END

static core_tLCDLayout dispAllied5[] = {
 {0, 0, 0, 5,CORE_SEG7}, {0,12, 6, 5,CORE_SEG7},
 {3, 0,12, 5,CORE_SEG7}, {3,12,18, 5,CORE_SEG7},
 {6, 9,24, 2,CORE_SEG7},
 {0}
};

static core_tLCDLayout dispAllied6[] = {
 {0, 0, 0, 6,CORE_SEG7}, {0,14, 6, 6,CORE_SEG7},
 {3, 0,12, 6,CORE_SEG7}, {3,14,18, 6,CORE_SEG7},
 {6,11,24, 2,CORE_SEG7},
 {0}
};

static core_tLCDLayout dispAllied6a[] = {
 {0, 0, 0, 6,CORE_SEG7}, {0,14, 6, 6,CORE_SEG7},
 {3, 0,12, 6,CORE_SEG7}, {3,14,18, 6,CORE_SEG7},
 {6, 8,24, 2,CORE_SEG7}, {6,14,26, 1,CORE_SEG7}, {6,18,27, 1,CORE_SEG7},
 {0}
};

#define INITGAME(name, disp) \
static core_tGameData name##GameData = {GEN_ZAC1, disp}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
INPUT_PORTS_START(name) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT(   0x0080, "Credit",		KEYCODE_1)  \
    COREPORT_BIT(   0x0100, "Slam Tilt",	KEYCODE_HOME)  \
    COREPORT_BIT(   0x0200, "Coin 1",		KEYCODE_3)  \
    COREPORT_BIT(   0x0400, "Coin 2",		KEYCODE_4)  \
    COREPORT_BIT(   0x0800, "Coin 3",		KEYCODE_5)  \
    COREPORT_BIT(   0x1000, "Diagnostic",	KEYCODE_7)  \
  PORT_START /* 1 */ \
/* DIP 0 */ \
    COREPORT_DIPNAME( 0x0002, 0x0000, "Number of players") \
      COREPORT_DIPSET(0x0000, "2 players" ) \
      COREPORT_DIPSET(0x0002, "4 players" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Play mode") \
      COREPORT_DIPSET(0x0000, "Replay" ) \
      COREPORT_DIPSET(0x0004, "Add-a-ball" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Match inhibit") \
      COREPORT_DIPSET(0x0000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0008, DEF_STR(Yes)) \
    COREPORT_DIPNAME( 0x0070, 0x0050, "Number of balls") \
      COREPORT_DIPSET(0x0010, "1" ) \
      COREPORT_DIPSET(0x0020, "2" ) \
      COREPORT_DIPSET(0x0030, "3" ) \
      COREPORT_DIPSET(0x0040, "4" ) \
      COREPORT_DIPSET(0x0050, "5" ) \
/* DIP 1 */ \
    COREPORT_DIPNAME( 0x0f00, 0x0100, "Max. credits x10") \
      COREPORT_DIPSET(0x0000, "00" ) \
      COREPORT_DIPSET(0x0100, "10" ) \
      COREPORT_DIPSET(0x0200, "20" ) \
      COREPORT_DIPSET(0x0300, "30" ) \
      COREPORT_DIPSET(0x0400, "40" ) \
      COREPORT_DIPSET(0x0500, "50" ) \
      COREPORT_DIPSET(0x0600, "60" ) \
      COREPORT_DIPSET(0x0700, "70" ) \
      COREPORT_DIPSET(0x0800, "80" ) \
      COREPORT_DIPSET(0x0900, "90" ) \
    COREPORT_DIPNAME( 0xf000, 0x0000, "Max. credits x1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
      COREPORT_DIPSET(0x2000, "2" ) \
      COREPORT_DIPSET(0x3000, "3" ) \
      COREPORT_DIPSET(0x4000, "4" ) \
      COREPORT_DIPSET(0x5000, "5" ) \
      COREPORT_DIPSET(0x6000, "6" ) \
      COREPORT_DIPSET(0x7000, "7" ) \
      COREPORT_DIPSET(0x8000, "8" ) \
      COREPORT_DIPSET(0x9000, "9" ) \
  PORT_START /* 2 */ \
/* DIP 2 */ \
    COREPORT_DIPNAME( 0x000f, 0x0001, "Credit #1 program") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
      COREPORT_DIPSET(0x0002, "2" ) \
      COREPORT_DIPSET(0x0003, "3" ) \
      COREPORT_DIPSET(0x0004, "4" ) \
      COREPORT_DIPSET(0x0005, "5" ) \
      COREPORT_DIPSET(0x0006, "6" ) \
      COREPORT_DIPSET(0x0007, "7" ) \
      COREPORT_DIPSET(0x0008, "8" ) \
      COREPORT_DIPSET(0x0009, "9" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0010, "Credit #2 program") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
      COREPORT_DIPSET(0x0020, "2" ) \
      COREPORT_DIPSET(0x0030, "3" ) \
      COREPORT_DIPSET(0x0040, "4" ) \
      COREPORT_DIPSET(0x0050, "5" ) \
      COREPORT_DIPSET(0x0060, "6" ) \
      COREPORT_DIPSET(0x0070, "7" ) \
      COREPORT_DIPSET(0x0080, "8" ) \
      COREPORT_DIPSET(0x0090, "9" ) \
/* DIP 3 */ \
    COREPORT_DIPNAME( 0x0f00, 0x0200, "Credit #3 program") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
      COREPORT_DIPSET(0x0200, "2" ) \
      COREPORT_DIPSET(0x0300, "3" ) \
      COREPORT_DIPSET(0x0400, "4" ) \
      COREPORT_DIPSET(0x0500, "5" ) \
      COREPORT_DIPSET(0x0600, "6" ) \
      COREPORT_DIPSET(0x0700, "7" ) \
      COREPORT_DIPSET(0x0800, "8" ) \
      COREPORT_DIPSET(0x0900, "9" ) \
    COREPORT_DIPNAME( 0xf000, 0x4000, "Credit option") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
      COREPORT_DIPSET(0x2000, "2" ) \
      COREPORT_DIPSET(0x3000, "3" ) \
      COREPORT_DIPSET(0x4000, "4" ) \
      COREPORT_DIPSET(0x5000, "5" ) \
      COREPORT_DIPSET(0x6000, "6" ) \
      COREPORT_DIPSET(0x7000, "7" ) \
      COREPORT_DIPSET(0x8000, "8" ) \
      COREPORT_DIPSET(0x9000, "9" ) \
  PORT_START /* 3 */ \
/* DIP 4 */ \
    COREPORT_DIPNAME( 0x000f, 0x0009, "Replay #1 x1K") \
      COREPORT_DIPSET(0x0000, "0,000" ) \
      COREPORT_DIPSET(0x0001, "1,000" ) \
      COREPORT_DIPSET(0x0002, "2,000" ) \
      COREPORT_DIPSET(0x0003, "3,000" ) \
      COREPORT_DIPSET(0x0004, "4,000" ) \
      COREPORT_DIPSET(0x0005, "5,000" ) \
      COREPORT_DIPSET(0x0006, "6,000" ) \
      COREPORT_DIPSET(0x0007, "7,000" ) \
      COREPORT_DIPSET(0x0008, "8,000" ) \
      COREPORT_DIPSET(0x0009, "9,000" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0030, "Replay #1 x10K") \
      COREPORT_DIPSET(0x0000, "00,000" ) \
      COREPORT_DIPSET(0x0010, "10,000" ) \
      COREPORT_DIPSET(0x0020, "20,000" ) \
      COREPORT_DIPSET(0x0030, "30,000" ) \
      COREPORT_DIPSET(0x0040, "40,000" ) \
      COREPORT_DIPSET(0x0050, "50,000" ) \
      COREPORT_DIPSET(0x0060, "60,000" ) \
      COREPORT_DIPSET(0x0070, "70,000" ) \
      COREPORT_DIPSET(0x0080, "80,000" ) \
      COREPORT_DIPSET(0x0090, "90,000" ) \
/* DIP 5 */ \
    COREPORT_DIPNAME( 0x0f00, 0x0100, "Replay #1 x100K") \
      COREPORT_DIPSET(0x0000, "000,000" ) \
      COREPORT_DIPSET(0x0100, "100,000" ) \
      COREPORT_DIPSET(0x0200, "200,000" ) \
      COREPORT_DIPSET(0x0300, "300,000" ) \
      COREPORT_DIPSET(0x0400, "400,000" ) \
      COREPORT_DIPSET(0x0500, "500,000" ) \
      COREPORT_DIPSET(0x0600, "600,000" ) \
      COREPORT_DIPSET(0x0700, "700,000" ) \
      COREPORT_DIPSET(0x0800, "800,000" ) \
      COREPORT_DIPSET(0x0900, "900,000" ) \
    COREPORT_DIPNAME( 0xf000, 0x2000, "Replay #2 x1K") \
      COREPORT_DIPSET(0x0000, "0,000" ) \
      COREPORT_DIPSET(0x1000, "1,000" ) \
      COREPORT_DIPSET(0x2000, "2,000" ) \
      COREPORT_DIPSET(0x3000, "3,000" ) \
      COREPORT_DIPSET(0x4000, "4,000" ) \
      COREPORT_DIPSET(0x5000, "5,000" ) \
      COREPORT_DIPSET(0x6000, "6,000" ) \
      COREPORT_DIPSET(0x7000, "7,000" ) \
      COREPORT_DIPSET(0x8000, "8,000" ) \
      COREPORT_DIPSET(0x9000, "9,000" ) \
  PORT_START /* 4 */ \
/* DIP 6 */ \
    COREPORT_DIPNAME( 0x000f, 0x0006, "Replay #2 x10K") \
      COREPORT_DIPSET(0x0000, "00,000" ) \
      COREPORT_DIPSET(0x0001, "10,000" ) \
      COREPORT_DIPSET(0x0002, "20,000" ) \
      COREPORT_DIPSET(0x0003, "30,000" ) \
      COREPORT_DIPSET(0x0004, "40,000" ) \
      COREPORT_DIPSET(0x0005, "50,000" ) \
      COREPORT_DIPSET(0x0006, "60,000" ) \
      COREPORT_DIPSET(0x0007, "70,000" ) \
      COREPORT_DIPSET(0x0008, "80,000" ) \
      COREPORT_DIPSET(0x0009, "90,000" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0010, "Replay #2 x100K") \
      COREPORT_DIPSET(0x0000, "000,000" ) \
      COREPORT_DIPSET(0x0010, "100,000" ) \
      COREPORT_DIPSET(0x0020, "200,000" ) \
      COREPORT_DIPSET(0x0030, "300,000" ) \
      COREPORT_DIPSET(0x0040, "400,000" ) \
      COREPORT_DIPSET(0x0050, "500,000" ) \
      COREPORT_DIPSET(0x0060, "600,000" ) \
      COREPORT_DIPSET(0x0070, "700,000" ) \
      COREPORT_DIPSET(0x0080, "800,000" ) \
      COREPORT_DIPSET(0x0090, "900,000" ) \
/* DIP 7 */ \
    COREPORT_DIPNAME( 0x0f00, 0x0400, "Replay #3 x1K") \
      COREPORT_DIPSET(0x0000, "0,000" ) \
      COREPORT_DIPSET(0x0100, "1,000" ) \
      COREPORT_DIPSET(0x0200, "2,000" ) \
      COREPORT_DIPSET(0x0300, "3,000" ) \
      COREPORT_DIPSET(0x0400, "4,000" ) \
      COREPORT_DIPSET(0x0500, "5,000" ) \
      COREPORT_DIPSET(0x0600, "6,000" ) \
      COREPORT_DIPSET(0x0700, "7,000" ) \
      COREPORT_DIPSET(0x0800, "8,000" ) \
      COREPORT_DIPSET(0x0900, "9,000" ) \
    COREPORT_DIPNAME( 0xf000, 0x7000, "Replay #3 x10K") \
      COREPORT_DIPSET(0x0000, "00,000" ) \
      COREPORT_DIPSET(0x1000, "10,000" ) \
      COREPORT_DIPSET(0x2000, "20,000" ) \
      COREPORT_DIPSET(0x3000, "30,000" ) \
      COREPORT_DIPSET(0x4000, "40,000" ) \
      COREPORT_DIPSET(0x5000, "50,000" ) \
      COREPORT_DIPSET(0x6000, "60,000" ) \
      COREPORT_DIPSET(0x7000, "70,000" ) \
      COREPORT_DIPSET(0x8000, "80,000" ) \
      COREPORT_DIPSET(0x9000, "90,000" ) \
  PORT_START /* 5 */ \
/* DIP 8 */ \
    COREPORT_DIPNAME( 0x000f, 0x0001, "Replay #3 x100K") \
      COREPORT_DIPSET(0x0000, "000,000" ) \
      COREPORT_DIPSET(0x0001, "100,000" ) \
      COREPORT_DIPSET(0x0002, "200,000" ) \
      COREPORT_DIPSET(0x0003, "300,000" ) \
      COREPORT_DIPSET(0x0004, "400,000" ) \
      COREPORT_DIPSET(0x0005, "500,000" ) \
      COREPORT_DIPSET(0x0006, "600,000" ) \
      COREPORT_DIPSET(0x0007, "700,000" ) \
      COREPORT_DIPSET(0x0008, "800,000" ) \
      COREPORT_DIPSET(0x0009, "900,000" ) \
INPUT_PORTS_END \
ROM_START(name) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("alliedu5.bin", 0xf400, 0x0400, CRC(e4fb64fb) SHA1(a3d9de7cbfb42180a860e0bbbeaeba96d8bd1e20)) \
    ROM_LOAD("alliedu6.bin", 0xf800, 0x0400, CRC(dca980dd) SHA1(3817d75413854d889fc1ce4fd6a51d820d1e0534)) \
    ROM_LOAD("alliedu3.bin", 0xfc00, 0x0400, CRC(13f42789) SHA1(baa0f73fda08a3c5d6f1423fb329e4febb07ef97)) \
ROM_END

INITGAME(allied, dispAllied5) GAMEX(1977, allied, 0, allied, allied, allied, ROT0, "Allied Leisure", "System", NOT_A_DRIVER)

// games below

INITGAME(suprpick,dispAllied5)  CORE_CLONEDEFNV(suprpick,allied,"Super Picker",1977,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(royclark,dispAllied6)  CORE_CLONEDEFNV(royclark,allied,"Roy Clark - The Entertainer",1977,"Fascination Int.",allied,GAME_USES_CHIMES)
INITGAME(thndbolt,dispAllied5)  CORE_CLONEDEFNV(thndbolt,allied,"Thunderbolt",1977,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(hoedown, dispAllied5)  CORE_CLONEDEFNV(hoedown, allied,"Hoe Down",1978,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(takefive,dispAllied6)  CORE_CLONEDEFNV(takefive,allied,"Take Five",1978,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(heartspd,dispAllied6)  CORE_CLONEDEFNV(heartspd,allied,"Hearts & Spades",1978,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(foathens,dispAllied6)  CORE_CLONEDEFNV(foathens,allied,"Flame of Athens",1978,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(disco79, dispAllied6a) CORE_CLONEDEFNV(disco79, allied,"Disco '79",1979,"Allied Leisure",allied,GAME_USES_CHIMES)
INITGAME(erosone, dispAllied6)  CORE_CLONEDEFNV(erosone, allied,"Eros One",1979,"Fascination Int.",allied,GAME_USES_CHIMES)
INITGAME(circa33, dispAllied6)  CORE_CLONEDEFNV(circa33, allied,"Circa 1933",1979,"Fascination Int.",allied,GAME_USES_CHIMES)
INITGAME(starshot,dispAllied6a) CORE_CLONEDEFNV(starshot,allied,"Star Shooter",1979,"Allied Leisure",allied,GAME_USES_CHIMES)
