// license:BSD-3-Clause

/******************************************************************************************
  Jeutel Games
  ------------
  by G. Volkenborn, 03/03/2011

  Jeutel is using two Z80 CPUs, and they are interacting heavily
  with lots of bus signals; the shared-RAM handshake between them is
  described above MDRV_CPU_ADD_TAG("mcpue", ...).
  For sound this is using a TMS5110A speak n spell
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "cpu/z80/z80.h"
#include "sound/tms5110.h"
#include "machine/8255ppi.h"

static struct {
  UINT8 lampData;
  UINT8 swStrobe;
  int lampStrobe;
  int dispStrobe;
  int dispBlank;
  int dipStrobe;

  int busGranted;   /* CPU M is stalled and CPU E owns the shared bus */
} locals;

/* Bus arbitration.  CPU E's HALT asks CPU M for the bus and the grant arrives
   back as CPU E's NMI; the resumed code writes 0 to the 0x4000 latch to drop
   the request.  The circuit and the matching ROM sequence are described above
   MDRV_CPU_ADD_TAG("mcpue", ...) further down.

   While the grant is outstanding CPU M is tri-stated, so it is suspended with
   SUSPEND_REASON_SPIN and eatcycles set: its local time advances but it executes
   nothing, which is what losing the bus costs it.  SPIN is not one of the
   reasons cpu_getstatus() tests, so the periodic and vblank timers hung off
   CPU M keep running -- jeutel_irq drives CPU E's interrupt line, and stopping
   it would stall the machine. */
static void jeutel_bus_grant(void) {
  if (!locals.busGranted) {
    locals.busGranted = 1;
    cpunum_suspend(1, SUSPEND_REASON_SPIN, 1);
  }
}
static void jeutel_bus_release(void) {
  if (locals.busGranted) {
    locals.busGranted = 0;
    cpunum_resume(1, SUSPEND_REASON_SPIN);
  }
}

static INTERRUPT_GEN(jeutel_vblank) {
  core_updateSw(core_getSol(16));
}

static INTERRUPT_GEN(jeutel_irq) {
  cpu_set_irq_line(0, 0, HOLD_LINE);
}

static void jeutel_nmi(int data) {
  if (cpunum_get_reg(0, Z80_HALT)) { // CPU E has released the bus
    jeutel_bus_grant();              // stall CPU M for as long as E holds it
    cpu_set_nmi_line(0, PULSE_LINE); // /BUSACK -> IC20 -> /NMIE
  }
}

static UINT8 *shared_RAM;
static WRITE_HANDLER(shared_ram_w) {
  shared_RAM[offset] = data;
}
static READ_HANDLER(shared_ram_r) {
  return shared_RAM[offset];
}

static WRITE_HANDLER(m4000_w) {
  if (!data) jeutel_bus_release(); // the resumed NMI code dropping /BUSRQ
}

static MEMORY_READ_START(cpu_readmem1)
  {0x0000, 0x0fff, MRA_ROM},
  {0x2000, 0x2003, ppi8255_0_r},
  {0x3000, 0x3003, ppi8255_1_r},
  {0x8000, 0x83ff, MRA_RAM},
  {0xc000, 0xc3ff, shared_ram_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem1)
  {0x2000, 0x2003, ppi8255_0_w},
  {0x3000, 0x3003, ppi8255_1_w},
  {0x4000, 0x4000, m4000_w},
  {0x8000, 0x83ff, MWA_RAM},
  {0xc000, 0xc3ff, shared_ram_w},
MEMORY_END

static MEMORY_READ_START(cpu_readmem2)
  {0x0000, 0x1fff, MRA_ROM},
  {0xc000, 0xc3ff, shared_ram_r},
  {0xc400, 0xc7ff, MRA_RAM},
  {0xe000, 0xe003, ppi8255_2_r},
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem2)
  {0xc000, 0xc3ff, shared_ram_w, &shared_RAM},
  {0xc400, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xe000, 0xe003, ppi8255_2_w},
MEMORY_END

// display row & data
static WRITE_HANDLER(ppi0_porta_w) {
  UINT16 digit;
  locals.dispBlank = data & 0x80 ? 0 : 1;
  if (data & 0x40) {
    coreGlobals.segments[32 + locals.dispStrobe].w = locals.dispBlank ? 0 : core_bcd2seg7[data & 0x0f];
    return;
  }
  switch (data & 0x0f) {
    case 0x0a: // letter T
      digit = 0x301; break;
    case 0x0b: // letter E
      digit = 0x79; break;
    case 0x0c: // letter L
      digit = 0x38; break;
    case 0x0d: // letter U
      digit = 0x3e; break;
    case 0x0e: // letter J
      digit = 0x1e; break;
    default:
      digit = core_bcd2seg9[data & 0x0f];
  }
  if (data & 0x10) {
    coreGlobals.segments[locals.dispStrobe].w = locals.dispBlank ? 0 : digit;
  } else if (data & 0x20) {
    coreGlobals.segments[16 + locals.dispStrobe].w = locals.dispBlank ? 0 : digit;
  }
}
// lamp & display strobe
static WRITE_HANDLER(ppi0_portb_w) {
  locals.lampStrobe = data >> 4;
  locals.dispStrobe = 15 - (data & 0x0f);
  if (locals.lampStrobe < 8) {
    coreGlobals.lampMatrix[locals.lampStrobe] = locals.lampData;
  } else if (locals.lampStrobe == 8) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xfff00) | locals.lampData;
  } else if (locals.lampStrobe == 9) {
    coreGlobals.solenoids = (coreGlobals.solenoids & 0xf00ff) | (locals.lampData << 8);
  }
}
// lamp data
static WRITE_HANDLER(ppi0_portc_w) {
  locals.lampData = data;
}

/* Switch matrix, from the Papillon manual's "MATRICE DES CONTACTS" (p.88).

   The contact numbering in the manual is OCTAL: Cxy is column x, row y.  There
   is no C08/C09/C18/C19/C28/C29/C38/C39 - the sequence runs C00-C07, C10-C17,
   C20-C27, C30/C35/C37, C40-C45.  Reading it as decimal misnumbers everything
   past C07.

   ppi1_portb_r() below returns swMatrix[1 + column], so hardware column x lives
   in swMatrix[x+1]:

     col 0  swMatrix[1]  C00 left outlane      C01 left auto bumper
                         C02 lower auto bumper C03 rotating gate
                         C04 right auto bumper C05 slingshot
                         C06 right outlane     C07 not connected
     col 1  swMatrix[2]  C10 upper left        C11 upper hole
                         C12-C15 upper targets A-D
                         C16 right star        C17 left star
     col 2  swMatrix[3]  C20-C23 drop targets A-D
                         C24 right contact     C25/C26 capture lane 1st/2nd
                         C27 capture hole
     col 3  swMatrix[4]  C30 lower lanes       C35 BALL TROUGH (receptacle)
                         C37 ball dispense
     col 4  swMatrix[5]  C40 tilt              C41 start ("engagement")
                         C42/C43/C44 left/centre/right coin
                         C45 test button

   Col 4: holding START sets
   swMatrix[5] to 0x02 (bit 1 = C41) and holding TEST sets it to 0x20
   (bit 5 = C45), which is what the manual predicts.

   C35 has to be closed for the game to see a ball in the trough and start one.
   Papillon's simulator supplies it; the other sets need it entered by hand
   through PinMAME's switch keys (column keys Q-I for columns 1-8, row keys A-K
   for rows 1-8, held together, so Cxy is column key x+1 plus row key y+1). */

// switch strobe
static WRITE_HANDLER(ppi1_porta_w) {
  locals.swStrobe = ~data;
}
// switch returns
static READ_HANDLER(ppi1_portb_r) {
  return ~coreGlobals.swMatrix[1 + core_BitColToNum(locals.swStrobe)];
}
static WRITE_HANDLER(ppi1_portb_w) { logerror("8255 #1 port B: %02x\n", data); }
static WRITE_HANDLER(ppi1_portc_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x0ffff) | ((data & 0x07) << 16);
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x04) | ((data & 0x30) >> 4);
}

// dip strobe, sound control
static WRITE_HANDLER(ppi2_porta_w) {
  locals.dipStrobe = ~data & 0x0f;
  sndbrd_ctrl_w(0, data >> 6);
}
// dip return
static READ_HANDLER(ppi2_portb_r) {
  return ~core_getDip(core_BitColToNum(locals.dipStrobe));
}
// sound command
static WRITE_HANDLER(ppi2_portc_w) {
  sndbrd_data_w(0, data);
}

static ppi8255_interface ppi8255_intf =
{
  3, /* 3 chips in total used by the 2 main CPUs. */
  {0, 0, 0},	/* Port A read */
  {0, ppi1_portb_r, ppi2_portb_r},	/* Port B read */
  {0, 0, 0},	/* Port C read */
  {ppi0_porta_w, ppi1_porta_w, ppi2_porta_w},	/* Port A write */
  {ppi0_portb_w, ppi1_portb_w, 0},	/* Port B write */
  {ppi0_portc_w, ppi1_portc_w, ppi2_portc_w},	/* Port C write */
};

/* Lamp numbering.

   The main CPU's lamp decoder at 0x005F turns a lamp number into a byte offset
   and a bit: it masks the number with 0x07 to index the single-bit table at
   0x0225, and shifts (number & 0x78) right by three to pick a byte of the lamp
   bitmap at 0xC019.  So lamp n lives at byte (n >> 3), bit (n & 7), and the
   display CPU strobes those bytes straight out to lampMatrix.

   The manual numbers the controlled lamps from 1, so manual lamp L is bit L-1:
   column (L-1)/8, row (L-1)%8.  Papillon's lamp list agrees: its gaps 29-32 and
   37-40 are the upper halves of columns 3 and 4, and it stops at 56, so
   lampMatrix[7] stays zero.

   The core's default conversion already gives these numbers (core_swSeq2m
   adds 7, vp_getLamp() takes 8 off), so no lamp conversion is set. */

static MACHINE_RESET(JEUTEL) {
  memset(&locals, 0x00, sizeof(locals));
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(REGION_CPU3), NULL, NULL);
  ppi8255_init(&ppi8255_intf);
}

static MACHINE_STOP(JEUTEL) {
  sndbrd_0_exit();
}

static SWITCH_UPDATE(JEUTEL) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x3f, 5);
  }
}


/* Sound section.

   The command protocol is in the A3 sound ROM's NMI handler at 0x0066, which
   sorts the latched byte by value:

     0x00         silence -- clears the two sound slots and resets the AY
     0x01-0x3f    an AY sound, held in the slot at 0x4008
     0x40-0x7f    a speech phrase, looked up in the table at 0x0266 and
                  clocked out to the TMS5110 a bit at a time
     0x80-0x85    passed to the routine at 0x010a
     0xfe / 0xff  clear / set the flag at 0x4015

   A 0x01-0x3f sound plays until the main CPU sends something else; silence is
   a command of its own, not something the sound board reaches by itself.  So a
   game left standing holds one sound: with a ball in play and no switch
   closing, sound 0x07 repeats its 194 ms sweep indefinitely, which is the
   period the real machine's sound has on video too. */

static struct {
  UINT8 sndCmd;
  UINT8 tmsLatch;
  UINT16 tmsAddr;
  int tmsBit;
} sndlocals;

static void jeutel_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0x00, sizeof(sndlocals));
  tms5110_reset();
  AY8910_reset(0);
}

static WRITE_HANDLER(jeutel_data_w) {
  sndlocals.sndCmd = data;
}

static WRITE_HANDLER(jeutel_ctrl_w) {
  if (!(data & 0x01)) {
    logerror(" CPU RESET\n");
    cpuint_reset_cpu(2);
    return;
  }
  if (data & 0x02) logerror(" CPU NMI\n");
  cpu_set_nmi_line(2, data & 0x02 ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(jeutel_manCmd_w) {
  jeutel_data_w(0, data);
  jeutel_ctrl_w(0, 3);
  jeutel_ctrl_w(0, 1);
}

static WRITE_HANDLER(m8000_w) {
  sndlocals.tmsLatch = data;
}

static MEMORY_READ_START(snd_readmem)
  {0x0000, 0x0fff, MRA_ROM},
  {0x4000, 0x43ff, MRA_RAM},
  {0x6000, 0x7fff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  {0x4000, 0x43ff, MWA_RAM},
  {0x8000, 0x8000, m8000_w},
MEMORY_END

static PORT_READ_START(snd_readport)
  {4, 4, AY8910_read_port_0_r},
PORT_END

static PORT_WRITE_START(snd_writeport)
  {0, 0, AY8910_control_port_0_w},
  {1, 1, AY8910_write_port_0_w},
PORT_END

/*
 * Bits 0 - 7: NC, NC, J6, LDH, CLR/LDL, CCLK, C1, C0
 */
static WRITE_HANDLER(ay8910_porta_w) {
  coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x03) | (data & 0x04);
  logerror("8910 port A: %c%c%c%c%c%cxx\n", data & 0x80 ? '0' : ' ', data & 0x40 ? '1' : ' ', data & 0x20 ? 'K' : ' ', data & 0x10 ? ' ' : 'L', data & 0x08 ? ' ' : 'H', data & 0x04 ? 'J' : ' ');
  if (~data & 0x10) { // load lower byte & reset bit counter
    sndlocals.tmsBit = 0;
    sndlocals.tmsAddr = (sndlocals.tmsAddr & 0xff00) | sndlocals.tmsLatch;
  }
  if (~data & 0x08) { // load upper byte
    sndlocals.tmsAddr = (sndlocals.tmsAddr & 0x00ff) | (sndlocals.tmsLatch << 8);
    logerror(" TMS offset:   %04x\n", sndlocals.tmsAddr);
  }
  if ((data & 0xf0) == 0xf0) {
    tms5110_CTL_w(0, TMS5110_CMD_RESET);
    tms5110_PDC_w(0, 1);
    tms5110_PDC_w(0, 0);
  }
  if ((data & 0xf0) == 0xd0) {
    tms5110_CTL_w(0, TMS5110_CMD_SPEAK);
    tms5110_PDC_w(0, 1);
    tms5110_PDC_w(0, 0);
  }
}
static READ_HANDLER(ay8910_portb_r) {
  logerror("8910 port B read: %02x\n", sndlocals.sndCmd);
  return sndlocals.sndCmd;
}

static struct AY8910interface jeutel_8910Int = {
  1,
  2000000,
  { 20 },
  { NULL },
  { ay8910_portb_r },
  { ay8910_porta_w },
};

static void tms5110_irq(int data) {
  cpu_set_irq_line(2, 0, tms5110_status_r(0) ? ASSERT_LINE : CLEAR_LINE);
}

static int tms5110_callback(void) {
  int value = (memory_region(REGION_SOUND1)[sndlocals.tmsAddr] >> sndlocals.tmsBit) & 1;
  sndlocals.tmsBit++;
  if (sndlocals.tmsBit > 7) {
    sndlocals.tmsAddr++;
    sndlocals.tmsBit = 0;
  }
  return value;
}

static struct TMS5110interface jeutel_5110Int = {
  640000,			/* clock rate = 80 * output sample rate,     */
					/* usually 640000 for 8000 Hz sample rate or */
					/* usually 800000 for 10000 Hz sample rate.  */
  50,				/* volume */
  tms5110_irq,		/* IRQ callback function (not implemented!) */
  tms5110_callback	/* function to be called when chip requests another bit*/
};

const struct sndbrdIntf jeutelIntf = {
  "JEUTEL", jeutel_init, NULL, NULL, jeutel_manCmd_w, jeutel_data_w, NULL, jeutel_ctrl_w
};

MACHINE_DRIVER_START(jeutel)
  MDRV_IMPORT_FROM(PinMAME)
/* Clocks and the E/M bus handshake, from the A1-84 schematics in the Papillon
   parts binder (printed pages 42 "PROCESSEURS MEMOIRES" and 43
   "CLOCK-DECODEUR-RESET"; PDF pages 48 and 49).

   Clock: an 8.000 MHz crystal oscillator (IC17 inverters, R6/R7) drives IC27,
   a 74LS393.  Its QA output (pin 3) is the 8 MHz divided by two, and that one
   4 MHz node feeds BOTH clock buffers -- IC17 pin 11->10 and IC17 pin 9->8,
   each with a 330 ohm pull-up (R9, R8).  ClockE and ClockM are both 4 MHz.

   The handshake:

     CPU E executes HALT, releasing the bus     (IC1 pin 18 = /HALTE)
     /HALTE is gated on the clock sheet into    /BUSRQ of CPU M (IC2 pin 25)
     CPU M finishes its machine cycle, grants   /BUSACK (IC2 pin 23), and
       tri-states its buses
     /BUSACK feeds IC20, a 74LS08 AND gate      (inputs pins 4/5, output pin 6)
     IC20's output drives /NMI of CPU E         (IC1 pin 17 = /NMIE)
     CPU E wakes from HALT, vectors through     0x0066 = "jp (hl)", so it
       resumes at an address it loaded into HL before halting
     CPU E does its shared-RAM work, writes 0   to 0x4000 to drop the request,
       and ends with RETN

   CPU E's ROM matches every step: each HALT is preceded by "ld hl,<resume>"
   (sub_010c at 0x010c), the NMI vector at 0x0066 is the single instruction
   "jp (hl)", and each resumed block ends "xor a / ld (04000h),a / retn"
   (0x0114).  A write of 0x12 to the same latch arms it again.

   jeutel_bus_grant/jeutel_bus_release at the top of this file model the
   arbitration: CPU M is suspended for as long as CPU E holds the bus, which is
   what losing it costs it on hardware.  Without that, both CPUs run freely and
   CPU M gets more work done than the real machine allows.

   The periodic interrupt is 8 MHz / 8192 = 976.6 Hz, a 74LS393 division of the
   crystal, delivered as one acknowledged interrupt.  It has to be acknowledged
   rather than held: CPU E's display work is interrupt-driven, so a level left
   asserted re-enters the handler for as long as it stands, and CPU E then
   rarely reaches its HALT.  This rate sets the display blank/unblank period,
   400 ms on real machines and 402 ms here, and leaves the game-timed lamp blink
   at 266.6 ms against 266.9 ms measured.

   Approximation: the bus grant is found by polling Z80_HALT from the jeutel_nmi
   timer rather than at the instant CPU E halts, so that timer has to stay fast.
   A halt callback in the Z80 core would remove the need for it. */
  MDRV_CPU_ADD_TAG("mcpue", Z80, 4000000) // the real rate, from the schematics and confirmed against a real machine
  MDRV_CPU_MEMORY(cpu_readmem1, cpu_writemem1)

  MDRV_CPU_ADD_TAG("mcpum", Z80, 4000000) // likewise: the real machine's game-timed lamp blink matches this to 0.3%
  MDRV_CPU_MEMORY(cpu_readmem2, cpu_writemem2)

  MDRV_INTERLEAVE(250)
  MDRV_CPU_VBLANK_INT(jeutel_vblank, 1)
  MDRV_CPU_PERIODIC_INT(jeutel_irq, 8000000.0/8192) // 976.6 Hz, a 74LS393 division of the crystal; calibrated against the real display flash
  MDRV_TIMER_ADD(jeutel_nmi, 2000) // polls for CPU E halting; stands in for a halt callback, so it has to stay fast
  MDRV_CORE_INIT_RESET_STOP(NULL,JEUTEL,JEUTEL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(JEUTEL)
  MDRV_DIPS(32)
  MDRV_DIAGNOSTIC_LEDH(3)

  MDRV_CPU_ADD_TAG("scpu", Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_TIMER_ADD(tms5110_irq, 100) // needed so the chip can tell the CPU it's ready

  MDRV_SOUND_ADD(AY8910, jeutel_8910Int)
  MDRV_SOUND_ADD(TMS5110, jeutel_5110Int)
MACHINE_DRIVER_END



/*============================================================================
/  Papillon playfield simulator
/
/  Switch numbers come from the "MATRICE DES CONTACTS" documented above.  The
/  core's default sequential conversion (core_swSeq2m: swNo -> swNo+7) lands
/  them exactly on the octal contact order, so contact Cxy is simply the
/  (8*x + y + 1)-th switch:
/
/     sw  1.. 8 = C00..C07      sw  9..16 = C10..C17
/     sw 17..24 = C20..C27      sw 25..32 = C30..C37
/     sw 33..40 = C40..C47
/
/  START is switch 34 (C41) and TEST is switch 38 (C45).
/
/  Solenoid 9 is the receptacle kicker (manual: bobine B73 "Réceptacle").  With
/  a credit on and C35 held closed the game pulses bit 8 every 0.96 s and never
/  stops: it is retrying the ball-serve because the trough switch never clears.
/  Clearing C35 on that pulse, which is what Papillon's simulator does, is what
/  puts a ball into play.
/===========================================================================*/

/*-- playfield switches (column 0: C00-C07) --*/
#define swLeftOutlane    1   /* C00 couloir de sortie gauche  */
#define swLeftBumper     2   /* C01 champignon auto. gauche   */
#define swLowerBumper    3   /* C02 champignon auto. inf.     */
#define swGate           4   /* C03 porte rotative            */
#define swRightBumper    5   /* C04 champignon auto. droit    */
#define swSling          6   /* C05 flipper automatique       */
#define swRightOutlane   7   /* C06 couloir de sortie droit   */
/*-- column 1: C10-C17 --*/
#define swUpperLeft      9   /* C10 contacts supérieur gauche */
#define swUpperHole     10   /* C11 contacts supérieur du trou*/
#define swTargetA       11   /* C12 cible supérieure A        */
#define swTargetB       12   /* C13 cible supérieure B        */
#define swTargetC       13   /* C14 cible supérieure C        */
#define swTargetD       14   /* C15 cible supérieure D        */
#define swRightStar     15   /* C16 étoile droite             */
#define swLeftStar      16   /* C17 étoile gauche             */
/*-- column 2: C20-C27 --*/
#define swDropA         17   /* C20 cible descendante A       */
#define swDropB         18   /* C21 cible descendante B       */
#define swDropC         19   /* C22 cible descendante C       */
#define swDropD         20   /* C23 cible descendante D       */
#define swRightContact  21   /* C24 contact droit             */
#define swCapture1      22   /* C25 1er couloir de capture    */
#define swCapture2      23   /* C26 2ème couloir de capture   */
#define swCaptureHole   24   /* C27 trou de capture           */
/*-- column 3: C30-C37 --*/
#define swLowerLanes    25   /* C30 couloirs inférieurs       */
#define swReceptacle    30   /* C35 réceptacle de boule       */
#define swDistribution  32   /* C37 contact de distribution   */
/*-- column 4: C40-C45 (driven by CORE_SETKEYSW, listed for reference) --*/
#define swTilt          33   /* C40 */
#define swStart         34   /* C41 */
#define swTest          38   /* C45 */

/*-- solenoids --------------------------------------------------------------
   Mapped by stimulus and response: with the simulator supplying a ball, each
   playfield switch was closed in turn and the solenoid word logged, so every
   entry below is a coil that fired in direct one-to-one response to the switch
   named beside it.  Bobine numbers are the Papillon manual's (p.94), tied to
   contacts by the playfield diagram on the same page (it draws B65 on C01,
   B66 on C04 and B70 at the holes).

   One coil pulse per switch closure:
     1  B65  upper bumper            fired by C01
     2  B66  lower bumper            fired by C04
     3       third bumper            fired by C02
     4  B68/B69 slingshot            fired by C05
     9  B73  receptacle kicker       serves the ball; see the simulator note
    19  B81  coin meter / lockout    fired by C42

   Observed but not pinned to a single cause:
     6   pulses in bursts after scoring; most likely the gong (B75)
    11   fired once when the third drop target went down
    15   on together with 16 while a ball is in play
    16   set at ball start -- the one jeutel_vblank reads through
         core_getSol(16) for flipper enable

   Not yet identified: a coil that resets the drop target bank.  None was seen
   when all four of C20-C23 were down, so the simulator leaves them down (see
   SIM_STSWKEEP below).  Papillon's bobine list has no explicit target-reset
   coil -- it assigns B70 to "trous" (the holes) where other Jeutel games use
   B70 for "remontee cibles" -- so this may need the solenoid test (test 5) or
   the schematic to settle. */
#define sUpperBumper     1   /* B65 */
#define sLowerBumper     2   /* B66 */
#define sThirdBumper     3
#define sSlingshot       4   /* B68 / B69 */
#define sBallServe       9   /* B73 réceptacle */
#define sCoinMeter      19   /* B81 */


/*-- Papillon controlled-lamp layout ----------------------------------------
   Positions are read by eye from the playfield diagram on page 94 of the
   Papillon parts binder, which prints each controlled lamp's number in a
   circle at its place on the playfield.  The grid below is schematic, not a
   pixel overlay: it keeps the groupings and the relative arrangement (the
   HEART ladder up the middle, the PAPILLON ladder down the left, the L-O-V-E
   lanes on the right) rather than exact playfield geometry.

   Lamp numbers are the manual's (lamp L is lampMatrix[(L-1)/8] bit (L-1)%8,
   see "Lamp numbering" above).  Numbers missing from the parts list (29-32,
   37-40, 47, 52) are left as {0}, although the ROM drives 29-32, 37-40 and 47.

   Two lamps drive more than one bulb, which core_tLampData handles with its
   lamppos array: 45 is "START HEART (2)" and 48 is "ETOILES HEART (5)".
   The structure allows at most four positions, so lamp 48 shows four of its
   five stars.

   The source is a 1984 photocopy and a few circled numbers are soft.  The
   ladders, the lanes and the bottom-of-playfield lamps are legible; two are
   not certain -- the pair drawn near B70 at the top right, and the circle below
   lamps 2/3/4, which reads as 47 although the parts list has no lamp 47. */
static core_tLampDisplay papillon_lampPos = {
  { 0, 0 },    /* top left */
  {15, 19},    /* size */
  {
    /* 1-8 : centre awards and the start of the HEART ladder */
    {1,{{ 6,14,YELLOW}}}, {1,{{ 6, 8,YELLOW}}}, {1,{{ 6,10,YELLOW}}}, {1,{{ 6,12,ORANGE}}},
    {1,{{ 6,16,GREEN }}}, {1,{{ 6,18,RED   }}}, {1,{{ 8,18,YELLOW}}}, {1,{{ 8,16,YELLOW}}},
    /* 9-16 : HEART ladder continues, then the PAPILLON ladder starts */
    {1,{{ 8,14,YELLOW}}}, {1,{{ 8,12,YELLOW}}}, {1,{{ 8,10,YELLOW}}}, {1,{{ 8, 8,YELLOW}}},
    {1,{{ 8, 6,YELLOW}}}, {1,{{ 8, 4,YELLOW}}}, {1,{{ 8, 2,YELLOW}}}, {1,{{ 2,18,ORANGE}}},
    /* 17-24 : PAPILLON ladder, 5,000 up to 50,000 */
    {1,{{ 2,16,ORANGE}}}, {1,{{ 2,14,ORANGE}}}, {1,{{ 2,12,ORANGE}}}, {1,{{ 2,10,ORANGE}}},
    {1,{{ 2, 8,ORANGE}}}, {1,{{ 2, 6,ORANGE}}}, {1,{{ 2, 4,ORANGE}}}, {1,{{ 2, 2,ORANGE}}},
    /* 25-28 : the L-O-V-E lanes down the right, then four unused */
    {1,{{12, 4,LBLUE }}}, {1,{{12, 6,LBLUE }}}, {1,{{12, 8,LBLUE }}}, {1,{{12,14,LBLUE }}},
    {0}, {0}, {0}, {0},
    /* 33-36 : HEART 20k/40k and the 2X/3X multipliers, then four unused */
    {1,{{ 4, 6,YELLOW}}}, {1,{{ 4, 8,YELLOW}}}, {1,{{ 4,12,RED   }}}, {1,{{ 4,10,RED   }}},
    {0}, {0}, {0}, {0},
    /* 41-48 : specials, credits, the two START HEART bulbs, the stars */
    {1,{{ 0,16,RED   }}}, {1,{{ 0,10,RED   }}}, {1,{{ 7,19,GREEN }}}, {1,{{ 0,19,WHITE }}},
    {2,{{ 4,16,YELLOW},{ 4,18,YELLOW}}},
    {1,{{ 0, 2,RED   }}},
    {0},
    {4,{{10, 6,WHITE},{12, 2,WHITE},{10,16,WHITE},{ 4,14,WHITE}}},
    /* 49-56 : fronton (backbox); 52 is not used */
    {1,{{ 0, 0,WHITE }}}, {1,{{ 2, 0,WHITE }}}, {1,{{ 4, 0,RED   }}}, {0},
    {1,{{ 8, 0,LPURPLE}}},{1,{{10, 0,LPURPLE}}},{1,{{12, 0,LPURPLE}}},{1,{{14, 0,LPURPLE}}}
  }
};

/*---------------------
/  Ball state handling
/----------------------*/
enum {
  stTrough=SIM_FIRSTSTATE, stBallOut, stDrain,
  stLeftOutlane, stRightOutlane, stLowerLanes,
  stLeftBumper, stLowerBumper, stRightBumper, stGate, stSling,
  stUpperLeft, stUpperHole, stTargetA, stTargetB, stTargetC, stTargetD,
  stRightStar, stLeftStar,
  stDropA, stDropB, stDropC, stDropD, stRightContact,
  stCapture1, stCapture2, stCaptureHole, stDistribution
};

static sim_tState papillon_stateDef[] = {
  {"Not Installed",   0,0,              0,           stDrain,    0,0,0, SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",       0,0,              0,           0,          0,0,0, SIM_STNOTEXCL},

  /* The ball sits on C35 until the game pulses the receptacle coil. */
  {"Receptacle",      1,swReceptacle,   sBallServe,  stBallOut,  5},
  {"Ball Out",        1,0,              0,           stFree,     5,0,0, SIM_STNOTEXCL},
  {"Drain",           1,0,              0,           stTrough,   0,0,0, SIM_STNOTEXCL},

  /* Outlanes and the lower lanes all end the ball. */
  {"Left Outlane",    1,swLeftOutlane,  0,           stDrain,    3},
  {"Right Outlane",   1,swRightOutlane, 0,           stDrain,    3},
  {"Lower Lanes",     1,swLowerLanes,   0,           stDrain,    3},

  {"Left Bumper",     1,swLeftBumper,   0,           stFree,     1},
  {"Lower Bumper",    1,swLowerBumper,  0,           stFree,     1},
  {"Right Bumper",    1,swRightBumper,  0,           stFree,     1},
  {"Rotating Gate",   1,swGate,         0,           stFree,     3},
  {"Slingshot",       1,swSling,        0,           stFree,     1},

  {"Upper Left",      1,swUpperLeft,    0,           stFree,     3},
  {"Upper Hole",      1,swUpperHole,    0,           stFree,     5},
  {"Upper Target A",  1,swTargetA,      0,           stFree,     3},
  {"Upper Target B",  1,swTargetB,      0,           stFree,     3},
  {"Upper Target C",  1,swTargetC,      0,           stFree,     3},
  {"Upper Target D",  1,swTargetD,      0,           stFree,     3},
  {"Right Star",      1,swRightStar,    0,           stFree,     3},
  {"Left Star",       1,swLeftStar,     0,           stFree,     3},

  /* Drop targets stay down until the game resets the bank (B70). */
  {"Drop Target A",   1,swDropA,        0,           stFree,     3,0,0, SIM_STSWKEEP},
  {"Drop Target B",   1,swDropB,        0,           stFree,     3,0,0, SIM_STSWKEEP},
  {"Drop Target C",   1,swDropC,        0,           stFree,     3,0,0, SIM_STSWKEEP},
  {"Drop Target D",   1,swDropD,        0,           stFree,     3,0,0, SIM_STSWKEEP},
  {"Right Contact",   1,swRightContact, 0,           stFree,     3},

  {"Capture Lane 1",  1,swCapture1,     0,           stCapture2, 3},
  {"Capture Lane 2",  1,swCapture2,     0,           stFree,     3},
  {"Capture Hole",    1,swCaptureHole,  0,           stFree,     5},
  {"Distribution",    1,swDistribution, 0,           stFree,     3},
  {0}
};

/*---------------------------
/  Keyboard conversion table
/----------------------------*/
static sim_tInportData papillon_inportData[] = {
  {0, 0x0001, stLeftOutlane},
  {0, 0x0002, stRightOutlane},
  {0, 0x0004, stLowerLanes},
  {0, 0x0008, stLeftBumper},
  {0, 0x0010, stLowerBumper},
  {0, 0x0020, stRightBumper},
  {0, 0x0040, stSling},
  {0, 0x0080, stGate},
  {0, 0x0100, stUpperLeft},
  {0, 0x0200, stUpperHole},
  {0, 0x0400, stTargetA},
  {0, 0x0800, stTargetB},
  {0, 0x1000, stTargetC},
  {0, 0x2000, stTargetD},
  {0, 0x4000, stRightStar},
  {0, 0x8000, stLeftStar},

  {1, 0x0001, stDropA},
  {1, 0x0002, stDropB},
  {1, 0x0004, stDropC},
  {1, 0x0008, stDropD},
  {1, 0x0010, stRightContact},
  {1, 0x0020, stCapture1},
  {1, 0x0040, stCaptureHole},
  {1, 0x0080, stDistribution},
  {1, 0x8000, stDrain},
  {0}
};

static void papillon_drawStatic(BMTYPE **line) {
  core_textOutf(30, 10, BLACK, "Help on this Simulator:");
  core_textOutf(30, 20, BLACK, "Q = Drain Ball, W/E = Outlanes");
  core_textOutf(30, 30, BLACK, "R = Lower Lanes, T/Y/U = Bumpers");
  core_textOutf(30, 40, BLACK, "I = Slingshot, O = Rotating Gate");
  core_textOutf(30, 50, BLACK, "P = Upper Left, B = Right Contact");
  core_textOutf(30, 60, BLACK, "J = Upper Hole, N = Distribution");
  core_textOutf(30, 70, BLACK, "A/S/D/F = Upper Targets A-D");
  core_textOutf(30, 80, BLACK, "G/H = Right/Left Star");
  core_textOutf(30, 90, BLACK, "Z/X/C/V = Drop Targets A-D");
  core_textOutf(30,100, BLACK, "K/L = Capture Lane / Hole");
}

static sim_tSimData papillonSimData = {
  2,                      /* 2 game specific input ports */
  papillon_stateDef,      /* Definition of all states */
  papillon_inportData,    /* Keyboard Entries */
  { stTrough, stTrough, stTrough, stTrough, stTrough, stTrough, stTrough },
  NULL,                   /* no custom init */
  NULL,                   /* no custom ball state handler */
  papillon_drawStatic,    /* Function to draw the on-screen help */
  FALSE,                  /* no manual shooter: the receptacle coil serves */
  NULL                    /* no custom key conditions */
};


// games below

#define INITGAME(name, disp, flip, simdata) \
static core_tGameData name##GameData = {0,disp,{flip,0,0,0,SNDBRD_JEUTEL},simdata}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/* As INITGAME, but also hands the core a controlled-lamp layout.  The lampData
   pointer is the 13th member of the hw struct, after the four custom-function
   pointers, hence the NULLs. */
#define INITGAME_LAMPS(name, disp, flip, simdata, lamps) \
static core_tGameData name##GameData = {0,disp,{flip,0,0,0,SNDBRD_JEUTEL,0,0,0,NULL,NULL,NULL,NULL,lamps},simdata}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/* DIP switches, named from the Papillon manual chapter 3 ("Ajustement des
   micro-interrupteurs de la carte Unite Centrale A1", p.10-12).

   The bit layout is not arbitrary: each physical 8-switch block is read as
   4 strobes x 2 bits, so block 1 is bits 6/7 of all four banks, block 2 is
   bits 4/5, block 3 bits 2/3 and block 4 bits 0/1.  That is exactly what the
   main CPU's DIP reader at 0x015E de-interleaves into 0xC0A4..0xC0A7, so
   manual switch n lives in block (n-1)/8 at position (n-1)%8.

   Polarity: ppi2_portb_r() returns ~core_getDip(), so a PinMAME setting of 1
   is the manual's "1" (switch closed, red dot visible).  Verified in the ROM
   at 0x0a9d, which reads block 2, tests bit 0 (switch 9) and loads a ball
   count of 5 when clear and 3 when set -- matching the manual once the
   inversion is applied.

   The names are from the Papillon manual, but they are not Papillon-specific.
   These are System 84 platform settings, shared across the range: read the
   game-adjustment wording (switches 8-22) as Papillon's and the behaviour as
   common.  The ball-count code is byte-identical in Le King, Olympic Games and
   Papillon (0x0a68, 0x0a8f, 0x0a9d).

   Switch numbers to hardware: the four DIP banks are strobed through 8255 #2
   (port A selects, port B returns).  sub_015e on the main CPU reads the raw
   banks into 0xC0A0-0xC0A3, then de-interleaves them into 0xC0A4-0xC0A7, taking
   two bit positions from all four banks per byte -- 0xC0A4 bits 6 and 7,
   0xC0A5 bits 4 and 5, 0xC0A6 bits 2 and 3, 0xC0A7 bits 0 and 1.  sub_01fe
   shifts the first bit gathered into bit 0, giving

       0xC0A4 = switches  1-8      0xC0A6 = switches 17-24
       0xC0A5 = switches  9-16     0xC0A7 = switches 25-32

   i.e. switch N at bit (N-1)%8 of 0xC0A4 + (N-1)/8, which is where the COREPORT
   masks below put them.  Switch 9 (balls per game) is read as 0xC0A5 bit 0 and
   switches 31/32 (maximum credits) as 0xC0A7 bits 6/7 via "and 0c0h".

   Not every switch is read.  Besides direct "bit n,a" tests, some reads stash
   the byte in b first, and the coin settings are used as a whole byte: 0x092c
   loads 0xC0A7, indexes a table at 0x0a3d and caches the result at 0xC0A8.
   Taking those into account, all four games read 27 of the 32:

       all four      1-9, 11, 12, 15-17, 19-22, 24, 25-32
       Le King only  18   (the multiball DIP; see the ball counts further down)
       none of them  10, 13, 14, 23

   Switches 10, 13, 14 and 23 are named below from the manual but have no effect
   in any of these ROMs, and are labelled so the menu does not offer a setting
   that does nothing.  The manual flags 23 and 24 as "pas utilises sur tous les
   modeles". */
#define JEUTEL_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(  0x0002, IPT_START1, IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN1,  IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN2,  KEYCODE_3) \
    COREPORT_BITDEF(  0x0010, IPT_COIN3,  KEYCODE_4) \
    COREPORT_BITDEF(  0x0001, IPT_TILT,   KEYCODE_INSERT) \
    COREPORT_BIT(     0x0020, "Test",     KEYCODE_7) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x4040, 0x4040, "Sw.1/2: Attract mode") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x4000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x0040, "Speech, no music" ) \
      COREPORT_DIPSET(0x4040, "Speech and music" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "Sw.5: Free games") \
      COREPORT_DIPSET(0x0000, "Replaced by extra balls" ) \
      COREPORT_DIPSET(0x0080, "Credited" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Sw.6: Highest score (with Sw.7)") \
      COREPORT_DIPSET(0x0000, "HSTD shown: 3 free games, or 0 if Sw.7 on" ) \
      COREPORT_DIPSET(0x8000, "2 free games, or HSTD off if Sw.7 on" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Sw.9: Balls per game") \
      COREPORT_DIPSET(0x0000, "3" ) \
      COREPORT_DIPSET(0x0010, "5" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Sw.10: Bonus column (no ROM here reads it)") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x1000, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Sw.13: Saucer lamps 1-3 (no ROM here reads it)") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x0020, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Sw.14: Capture hole bonus (no ROM here reads it)") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x2000, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Sw.17: Tilt") \
      COREPORT_DIPSET(0x0000, "Loses the ball" ) \
      COREPORT_DIPSET(0x0004, "Loses the game" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Sw.18: Multiball bonus (Le King only)") \
      COREPORT_DIPSET(0x0000, "Counted on 2nd ball lost" ) \
      COREPORT_DIPSET(0x0400, "Counted on each ball lost" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Sw.21: Upper targets A-D") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x0008, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "Sw.22: Gong on free games") \
      COREPORT_DIPSET(0x0000, DEF_STR(No)) \
      COREPORT_DIPSET(0x0800, DEF_STR(Yes)) \
    COREPORT_DIPNAME( 0x0101, 0x0101, "Sw.25/26: Left coin chute") \
      COREPORT_DIPSET(0x0000, "4 coins / 1 game" ) \
      COREPORT_DIPSET(0x0001, "3 coins / 1 game" ) \
      COREPORT_DIPSET(0x0100, "2 coins / 1 game" ) \
      COREPORT_DIPSET(0x0101, "1 coin / 1 game" ) \
    COREPORT_DIPNAME( 0x0202, 0x0000, "Sw.29/30: Right coin chute") \
      COREPORT_DIPSET(0x0000, "15 games / coin" ) \
      COREPORT_DIPSET(0x0002, "10 games / coin" ) \
      COREPORT_DIPSET(0x0200, "7 games / coin" ) \
      COREPORT_DIPSET(0x0202, "5 games / coin" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0040, 0x0000, "Sw.3: Attract mode time") \
      COREPORT_DIPSET(0x0000, "Short" ) \
      COREPORT_DIPSET(0x0040, "Long" ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "Sw.4: French regulation") \
      COREPORT_DIPSET(0x0000, "Max 5 free games per stake" ) \
      COREPORT_DIPSET(0x4000, "No limit" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Sw.7: Highest score (with Sw.6)") \
      COREPORT_DIPSET(0x0000, "Free games: 3, or 2 if Sw.6 on" ) \
      COREPORT_DIPSET(0x0080, "No free game; HSTD off if Sw.6 on" ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "Sw.8: Match") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off)) \
      COREPORT_DIPSET(0x8000, "On at end of game" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "Sw.11: Star bonus advances") \
      COREPORT_DIPSET(0x0000, "Go out after the ball passes" ) \
      COREPORT_DIPSET(0x0010, "Stay lit" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Sw.12: Drop targets") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x1000, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "Sw.15: Bonus multiplier 2X/4X") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x0020, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "Sw.16: Bumper scoring") \
      COREPORT_DIPSET(0x0000, "530 points" ) \
      COREPORT_DIPSET(0x2000, "1030 points" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "Sw.19: Yellow bonus advances") \
      COREPORT_DIPSET(0x0000, "Not kept between balls" ) \
      COREPORT_DIPSET(0x0004, "Kept between balls" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "Sw.20: Special (outlanes)") \
      COREPORT_DIPSET(0x0000, "Lit at 40,000" ) \
      COREPORT_DIPSET(0x0400, "Lit at 60,000" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Sw.23: Unused (no ROM here reads it)") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "Sw.24: Model-specific (read, purpose unknown)") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x0101, 0x0100, "Sw.27/28: Centre coin chute") \
      COREPORT_DIPSET(0x0000, "6 games / coin" ) \
      COREPORT_DIPSET(0x0001, "4 games / coin" ) \
      COREPORT_DIPSET(0x0100, "2 games / coin" ) \
      COREPORT_DIPSET(0x0101, "3 games / coin" ) \
    COREPORT_DIPNAME( 0x0202, 0x0000, "Sw.31/32: Maximum credits") \
      COREPORT_DIPSET(0x0000, "10" ) \
      COREPORT_DIPSET(0x0002, "25" ) \
      COREPORT_DIPSET(0x0200, "15" ) \
      COREPORT_DIPSET(0x0202, "8" ) \
  /* Simulator playfield ports, inports 5 and 6 after the two CORE_PORTS: \
     sim_run reads from CORE_COREINPORT + (MDRV_DIPS(32)+31)/16 = 2 + 3 = 5. */ \
  PORT_START /* 3 */ \
    COREPORT_BIT(   0x0001, "Left Outlane",    KEYCODE_W) \
    COREPORT_BIT(   0x0002, "Right Outlane",   KEYCODE_E) \
    COREPORT_BIT(   0x0004, "Lower Lanes",     KEYCODE_R) \
    COREPORT_BIT(   0x0008, "Left Bumper",     KEYCODE_T) \
    COREPORT_BIT(   0x0010, "Lower Bumper",    KEYCODE_Y) \
    COREPORT_BIT(   0x0020, "Right Bumper",    KEYCODE_U) \
    COREPORT_BIT(   0x0040, "Slingshot",       KEYCODE_I) \
    COREPORT_BIT(   0x0080, "Rotating Gate",   KEYCODE_O) \
    COREPORT_BIT(   0x0100, "Upper Left",      KEYCODE_P) \
    COREPORT_BIT(   0x0200, "Upper Hole",      KEYCODE_J) \
    COREPORT_BIT(   0x0400, "Upper Target A",  KEYCODE_A) \
    COREPORT_BIT(   0x0800, "Upper Target B",  KEYCODE_S) \
    COREPORT_BIT(   0x1000, "Upper Target C",  KEYCODE_D) \
    COREPORT_BIT(   0x2000, "Upper Target D",  KEYCODE_F) \
    COREPORT_BIT(   0x4000, "Right Star",      KEYCODE_G) \
    COREPORT_BIT(   0x8000, "Left Star",       KEYCODE_H) \
  PORT_START /* 4 */ \
    COREPORT_BIT(   0x0001, "Drop Target A",   KEYCODE_Z) \
    COREPORT_BIT(   0x0002, "Drop Target B",   KEYCODE_X) \
    COREPORT_BIT(   0x0004, "Drop Target C",   KEYCODE_C) \
    COREPORT_BIT(   0x0008, "Drop Target D",   KEYCODE_V) \
    COREPORT_BIT(   0x0010, "Right Contact",   KEYCODE_B) \
    COREPORT_BIT(   0x0020, "Capture Lane",    KEYCODE_K) \
    COREPORT_BIT(   0x0040, "Capture Hole",    KEYCODE_L) \
    COREPORT_BIT(   0x0080, "Distribution",    KEYCODE_N) \
    COREPORT_BIT(   0x8000, "Drain Ball",      KEYCODE_Q) \
  INPUT_PORTS_END

static core_tLCDLayout dispAlpha[] = {
  {0, 0,10,1,CORE_SEG10}, {0, 2,11,2,CORE_SEG9}, {0, 6,13,1,CORE_SEG10}, {0, 8,14,2,CORE_SEG9}, {0,12, 1,1,CORE_SEG9},
  {0,16, 2,1,CORE_SEG10}, {0,18, 3,2,CORE_SEG9}, {0,22, 5,1,CORE_SEG10}, {0,24, 6,2,CORE_SEG9}, {0,28, 9,1,CORE_SEG9},
  {5, 0,26,1,CORE_SEG10}, {5, 2,27,2,CORE_SEG9}, {5, 6,29,1,CORE_SEG10}, {5, 8,30,2,CORE_SEG9}, {5,12,17,1,CORE_SEG9},
  {5,16,18,1,CORE_SEG10}, {5,18,19,2,CORE_SEG9}, {5,22,21,1,CORE_SEG10}, {5,24,22,2,CORE_SEG9}, {5,28,25,1,CORE_SEG9},
  {3,27,37,2,CORE_SEG7S}, {3,33,39,1,CORE_SEG7S}, {3,35,41,1,CORE_SEG7S},
#ifdef MAME_DEBUG
  {3,25,36,1,CORE_SEG7S}, // 100's credits is actually stored & displayable!
#endif
  {0}
};

/*--------------------------------
/ Le King
/-------------------------------*/
ROM_START(leking)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-v.bin", 0x0000, 0x1000, CRC(cbbc8b55) SHA1(4fe150fa3b565e5618896c0af9d51713b381ed88))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-m.bin", 0x0000, 0x2000, CRC(4b66517a) SHA1(1939ea78932d469a16441507bb90b032c5f77b1e))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-v.bin", 0x0000, 0x1000, CRC(36130e7b) SHA1(d9b66d43b55272579b3972005355b8a18ce6b4a9))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("sound-p.bin", 0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d) BAD_DUMP)
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME(leking,dispAlpha,FLIP_SW(FLIP_L),NULL)
// The only one of the four whose ROM tests the multiball DIP (switch 18 =
// 0xC0A6 bit 1, at 0x1554), so the only multiball game here.  The manual calls
// that mode "2 billes sur le plateau"; the machine's ball complement is not
// documented for Le King, so the ball count is left as it was.
JEUTEL_COMPORTS(leking, 3)
CORE_GAMEDEFNV(leking, "Le King", 1983, "Jeutel", jeutel, 0)

/*--------------------------------
/ Olympic Games
/-------------------------------*/
ROM_START(olympic)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-v.bin",   0x0000, 0x1000, CRC(cd284a20) SHA1(94568e1247994c802266f9fbe4a6f6ed2b55a978))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("game-jo1.bin", 0x0000, 0x2000, CRC(c9f040cf) SHA1(c689f3a82d904d3f9fc8688d4c06082c51645b2f))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-j0.bin", 0x0000, 0x1000, CRC(5c70ce72) SHA1(b0b6cc7b6ec3ed9944d738b61a0d144b77b07000))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("sound-p.bin",  0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d))
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME(olympic,dispAlpha,FLIP_SW(FLIP_L),NULL)
JEUTEL_COMPORTS(olympic, 1)
CORE_GAMEDEFNV(olympic, "Olympic Games", 1984, "Jeutel", jeutel, 0)

/*--------------------------------
/ Papillon
/-------------------------------*/
ROM_START(jpapillon)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("game-v.bin",  0x0000, 0x1000, CRC(cd284a20) SHA1(94568e1247994c802266f9fbe4a6f6ed2b55a978))  // dump was the same as ones above
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("CPU_IC6.bin", 0x0000, 0x2000, CRC(9502fd05) SHA1(3369f51c56b32c8bde8b09f80c5f02bd579c5fbe))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("sound-j0.bin", 0x0000, 0x1000, CRC(5c70ce72) SHA1(b0b6cc7b6ec3ed9944d738b61a0d144b77b07000)) // dump was the same as Olympic Games
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("sound-p.bin",  0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d)) // dump was the same as Olympic Games
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME_LAMPS(jpapillon,dispAlpha,FLIP_SW(FLIP_L),&papillonSimData,&papillon_lampPos)
JEUTEL_COMPORTS(jpapillon, 1)
CORE_GAMEDEFNV(jpapillon, "Papillon (Jeutel)", 1984, "Jeutel", jeutel, 0)

/*--------------------------------
/ Apocalypse Now
/-------------------------------*/
// ROM names follow the board silkscreen: A1 is the Unite Centrale (CPU) board,
// A3 is the Carte Son (sound) board.
// The main CPU ROM (A1/IC6, an M5L2764K labelled "APO") is a consensus of 14
// reads of a chip with marginal D6/D7 contacts - the erased area verifies clean
// and every structural check passes, but it is not a single clean read.
// The other three are byte-identical to ROMs already in this file: the display
// ROM matches Olympic Games / Papillon, the sound CPU ROM matches Le King, and
// the speech ROM is the one shared across the whole range.
ROM_START(apocalypse_now)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("a1-ic3_disp.bin",  0x0000, 0x1000, CRC(cd284a20) SHA1(94568e1247994c802266f9fbe4a6f6ed2b55a978))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("a1-ic6_cpu.bin",   0x0000, 0x2000, CRC(8f8ad836) SHA1(0d422f12ff63655ba6da4535430de3b1359162e4))

  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("a3-z4_snd.bin",    0x0000, 0x1000, CRC(36130e7b) SHA1(d9b66d43b55272579b3972005355b8a18ce6b4a9))
  NORMALREGION(0x10000, REGION_SOUND1)
    ROM_LOAD("a3-z5_speech.bin", 0x0000, 0x2000, CRC(97eedd6c) SHA1(3bb8e5d32417c49ef97cbe407f2c5eeb214bf72d))
    ROM_RELOAD(0x2000, 0x2000)
    ROM_RELOAD(0x4000, 0x2000)
    ROM_RELOAD(0x6000, 0x2000)
    ROM_RELOAD(0x8000, 0x2000)
    ROM_RELOAD(0xa000, 0x2000)
    ROM_RELOAD(0xc000, 0x2000)
    ROM_RELOAD(0xe000, 0x2000)
ROM_END

INITGAME(apocalypse_now,dispAlpha,FLIP_SW(FLIP_L),NULL)
// One ball.  Switch 18, the multiball DIP, is 0xC0A6 bit 1 and this ROM never
// tests it; nor do Olympic Games' or Papillon's.  The ball is served by SOL 9,
// the receptacle B73; SOL 8, the B74 ejector the manual lists as fitted
// "uniquement sur jeu multibilles", is never driven.
JEUTEL_COMPORTS(apocalypse_now, 1)
CORE_GAMEDEFNV(apocalypse_now, "Apocalypse Now", 1985, "Jeutel", jeutel, 0)
