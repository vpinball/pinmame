/************************************************************************************************
 Recel System III discrete sound
 --------------------------------
   Master Unit: one free-running oscillator into a 7493-style binary divider, with NAND gates
   letting each of the six 11696 PIO outputs (factory registers #0-#5) independently gate one
   sub-multiple of the base frequency onto a common TIP 132 power stage (Max/Med/Min taps, not
   modelled here). No schematic states which division ratio sits on which line, and the base
   frequency itself is uncertain: the schematic nominally says 25 kHz, but measurement on real
   hardware gives roughly 6 kHz and it is trimmer-dependent. RECEL_SND_F0 below is a single named,
   tunable guess, not a documented value.

   Topology confirmed by observation (docs/driver-notes.md §7 "Sound"): tracing dev=0xD writes
   against driven game events (games/r_fairfght.json switches, via /api/input) shows the six
   lines summing rather than selecting -- e.g. registers #1 and #2 go high together for one frame
   at the start of a coil kick, and #4/#5 are held together for the ~2s of a ball-serve buzz.  A
   mutually-exclusive tone selector could not produce that overlap, so this is six independent
   gated taps added together. What is *not* confirmed is
   which physical division ratio belongs to which line; F0/2..F0/64 in ascending register order is
   a labelled guess, not a measurement.

   The one thing the self-check step 5 proves independently: registers #0-#5 are exercised one at
   a time (0.4.7..5.4.7, recel.c's update_coil_sense comment) -- six lines, confirmed distinct.
************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "recel.h"

/* Uncertain -- see file header. Single named constant so it can be retuned. */
#define RECEL_SND_F0 6000.0

/* One DISCRETE_INPUT per PIO line, each at its own discrete_sound_w() offset (ADDR/MASK there is
   an address decode, like a memory-mapped port, not a bitmask on the data value -- see
   disc_inp.c's dss_input_init). recel_snd_w() below writes each line's 0/1 state to its own
   offset so the six squarewaves gate independently; one shared multi-bit "value" input feeding
   every SQUAREWAVE's ENAB would gate all six together on any bit, which is not what the trace
   shows. */
DISCRETE_SOUND_START(recel_discInt)
  DISCRETE_INPUT(NODE_01, 0, 0x3f, 0)  /* PIO out 0 / register #0 */
  DISCRETE_INPUT(NODE_02, 1, 0x3f, 0)  /* PIO out 1 / register #1 */
  DISCRETE_INPUT(NODE_03, 2, 0x3f, 0)  /* PIO out 2 / register #2 */
  DISCRETE_INPUT(NODE_04, 3, 0x3f, 0)  /* PIO out 3 / register #3 */
  DISCRETE_INPUT(NODE_05, 4, 0x3f, 0)  /* PIO out 4 / register #4 */
  DISCRETE_INPUT(NODE_06, 5, 0x3f, 0)  /* PIO out 5 / register #5 */

  DISCRETE_SQUAREWAVE(NODE_10, NODE_01, RECEL_SND_F0 / 2.0,  8000, 50, 0, 0)
  DISCRETE_SQUAREWAVE(NODE_11, NODE_02, RECEL_SND_F0 / 4.0,  8000, 50, 0, 0)
  DISCRETE_SQUAREWAVE(NODE_12, NODE_03, RECEL_SND_F0 / 8.0,  8000, 50, 0, 0)
  DISCRETE_SQUAREWAVE(NODE_13, NODE_04, RECEL_SND_F0 / 16.0, 8000, 50, 0, 0)
  DISCRETE_SQUAREWAVE(NODE_14, NODE_05, RECEL_SND_F0 / 32.0, 8000, 50, 0, 0)
  DISCRETE_SQUAREWAVE(NODE_15, NODE_06, RECEL_SND_F0 / 64.0, 8000, 50, 0, 0)

  DISCRETE_ADDER4(NODE_20, 1, NODE_10, NODE_11, NODE_12, NODE_13)
  DISCRETE_ADDER3(NODE_21, 1, NODE_20, NODE_14, NODE_15)
  DISCRETE_OUTPUT(NODE_21, 50)
DISCRETE_SOUND_END

MACHINE_DRIVER_START(recel_snd)
  MDRV_SOUND_ADD(DISCRETE, recel_discInt)
MACHINE_DRIVER_END

void recel_snd_w(int bits) {
  int i;
  bits &= 0x3f;
  for (i = 0; i < 6; i++)
    discrete_sound_w(i, (bits >> i) & 1);
}
