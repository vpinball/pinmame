/***************************************************************************

  ay8910.c


  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

***************************************************************************/
/*
 * Couriersud, July 2014:
 *
 * This documents recent work on the AY8910. A YM2149 is now on it's way from
 * Hong Kong as well.
 *
 * TODO:
 *
 * - Create a true sound device nAY8910 driver.
 * - implement approach outlined below in this driver.
 *
 * For years I had a AY8910 in my drawer. Arduinos were around as well.
 * Using the approach documented in this blog post
 *    http://www.986-studio.com/2014/05/18/another-ay-entry/#more-476
 * I measured the output voltages using a Extech 520.
 *
 * Measurement Setup
 *
 * Laptop <--> Arduino <---> AY8910
 *
 * AY8910 Registers:
 * 0x07: 3f
 * 0x08: RV
 * 0x09: RV
 * 0x0A: RV
 *
 * Output was measured on Analog Output B with a resistor RD to
 * ground.
 *
 * Measurement results:
 *
 * RD      983  9.830k   99.5k  1.001M    open
 *
 * RV        B       B       B       B       B
 *  0   0.0000  0.0000  0.0001  0.0011  0.0616
 *  1   0.0106  0.0998  0.6680  1.8150  2.7260
 *  2   0.0150  0.1377  0.8320  1.9890  2.8120
 *  3   0.0222  0.1960  1.0260  2.1740  2.9000
 *  4   0.0320  0.2708  1.2320  2.3360  2.9760
 *  5   0.0466  0.3719  1.4530  2.4880  3.0440
 *  6   0.0665  0.4938  1.6680  2.6280  3.1130
 *  7   0.1039  0.6910  1.9500  2.7900  3.1860
 *  8   0.1237  0.7790  2.0500  2.8590  3.2340
 *  9   0.1986  1.0660  2.3320  3.0090  3.3090
 * 10   0.2803  1.3010  2.5050  3.0850  3.3380
 * 11   0.3548  1.4740  2.6170  3.1340  3.3590
 * 12   0.4702  1.6870  2.7340  3.1800  3.3730
 * 13   0.6030  1.8870  2.8410  3.2300  3.4050
 * 14   0.7530  2.0740  2.9280  3.2580  3.4170
 * 15   0.9250  2.2510  3.0040  3.2940  3.4380
 *
 * Using an equivalent model approach with two resistors
 *
 *      5V
 *       |
 *       Z
 *       Z Resistor Value for RV
 *       Z
 *       |
 *       +---> Output signal
 *       |
 *       Z
 *       Z External RD
 *       Z
 *       |
 *      GND
 *
 * will NOT work out of the box since RV = RV(RD).
 *
 * The following approach will be used going forward based on die pictures
 * of the AY8910 done by Dr. Stack van Hay:
 *
 *
 *              5V
 *             _| D
 *          G |      NMOS
 *     Vg ---||               Kn depends on volume selected
 *            |_  S Vs
 *               |
 *               |
 *               +---> VO Output signal
 *               |
 *               Z
 *               Z External RD
 *               Z
 *               |
 *              GND
 *
 *  Whilst conducting, the FET operates in saturation mode:
 *
 *  Id = Kn * (Vgs - Vth)^2
 *
 *  Using Id = Vs / RD
 *
 *  Vs = Kn * RD  * (Vg - Vs - Vth)^2
 *
 *  finally using Vg' = Vg - Vth
 *
 *  Vs = Vg' + 1 / (2 * Kn * RD) - sqrt((Vg' + 1 / (2 * Kn * RD))^2 - Vg'^2)
 *
 *  and finally
 *
 *  VO = Vs
 *
 *  and this can be used to re-Thenevin to 5V
 *
 *  RVequiv = RD * ( 5V / VO - 1)
 *
 *  The RV and Kn parameter are derived using least squares to match
 *  calculation results with measurements.
 *
 *  FIXME:
 *  There is voltage of 60 mV measured with the EX520 (Ri ~ 10M). This may
 *  be induced by cutoff currents from the 15 FETs.
 *
 */


/***************************************************************************

  ay8910.c

  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

  Mostly rewritten by couriersud in 2008

  Public documentation:

  - http://privatfrickler.de/blick-auf-den-chip-soundchip-general-instruments-ay-3-8910/
    Die pictures of the AY8910

  - US Patent 4933980

  Games using ADSR: gyruss

  A list with more games using ADSR can be found here:
        http://mametesters.org/view.php?id=3043

  TODO:
  * The AY8930 has an extended mode which is currently
    not emulated.
  * YM2610 & YM2608 will need a separate flag in their config structures
    to distinguish between legacy and discrete mode.

  The rewrite also introduces a generic model for the DAC. This model is
  not perfect, but allows channel mixing based on a parametrized approach.
  This model also allows to factor in different loads on individual channels.
  If a better model is developped in the future or better measurements are
  available, the driver should be easy to change. The model is described
  later.

  In order to not break hundreds of existing drivers by default the flag
  AY8910_LEGACY_OUTPUT is used by drivers not changed to take into account the
  new model. All outputs are normalized to the old output range (i.e. 0 .. 7ffff).
  In the case of channel mixing, output range is 0...3 * 7fff.

  The main difference between the AY-3-8910 and the YM2149 is, that the
  AY-3-8910 datasheet mentions, that fixed volume level 0, which is set by
  registers 8 to 10 is "channel off". The YM2149 mentions, that the generated
  signal has a 2V DC component. This is confirmed by measurements. The approach
  taken here is to assume the 2V DC offset for all outputs for the YM2149.
  For the AY-3-8910, an offset is used if envelope is active for a channel.
  This is backed by oscilloscope pictures from the datasheet. If a fixed volume
  is set, i.e. envelope is disabled, the output voltage is set to 0V. Recordings
  I found on the web for gyruss indicate, that the AY-3-8910 offset should
  be around 0.2V. This will also make sound levels more compatible with
  user observations for scramble.

  The Model:
                     5V     5V
                      |      |
                      /      |
  Volume Level x >---|       Z
                      >      Z Pullup Resistor RU
                       |     Z
                       Z     |
                    Rx Z     |
                       Z     |
                       |     |
                       '-----+-------->  >---+----> Output signal
                             |               |
                             Z               Z
               Pulldown RD   Z               Z Load RL
                             Z               Z
                             |               |
                            GND             GND

Each Volume level x will select a different resistor Rx. Measurements from fpgaarcade.com
where used to calibrate channel mixing for the YM2149. This was done using
a least square approach using a fixed RL of 1K Ohm.

For the AY measurements cited in e.g. openmsx as "Hacker Kay" for a single
channel were taken. These were normalized to 0 ... 65535 and consequently
adapted to an offset of 0.2V and a VPP of 1.3V. These measurements are in
line e.g. with the formula used by pcmenc for the volume: vol(i) = exp(i/2-7.5).

The following is documentation from the code moved here and amended to reflect
the changes done:

Careful studies of the chip output prove that the chip counts up from 0
until the counter becomes greater or equal to the period. This is an
important difference when the program is rapidly changing the period to
modulate the sound. This is worthwhile noting, since the datasheets
say, that the chip counts down.
Also, note that period = 0 is the same as period = 1. This is mentioned
in the YM2203 data sheets. However, this does NOT apply to the Envelope
period. In that case, period = 0 is half as period = 1.

Envelope shapes:
    C AtAlH
    0 0 x x  \___
    0 1 x x  /___
    1 0 0 0  \\\\
    1 0 0 1  \___
    1 0 1 0  \/\/
    1 0 1 1  \```
    1 1 0 0  ////
    1 1 0 1  /```
    1 1 1 0  /\/\
    1 1 1 1  /___

The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
has twice the steps, happening twice as fast.

****************************************************************************

    The bus control and chip selection signals of the AY PSGs and their
    pin-compatible clones such as YM2149 are somewhat unconventional and
    redundant, having been designed for compatibility with GI's CP1610
    series of microprocessors. Much of the redundancy can be finessed by
    tying BC2 to Vcc; AY-3-8913 and AY8930 do this internally.

                            /A9   A8    /CS   BDIR  BC2   BC1
                AY-3-8910   24    25    n/a   27    28    29
                AY-3-8912   n/a   17    n/a   18    19    20
                AY-3-8913   22    23    24    2     n/a   3
                            ------------------------------------
                Inactive            NACT      0     0     0
                Latch address       ADAR      0     0     1
                Inactive            IAB       0     1     0
                Read from PSG       DTB       0     1     1
                Latch address       BAR       1     0     0
                Inactive            DW        1     0     1
                Write to PSG        DWS       1     1     0
                Latch address       INTAK     1     1     1

***************************************************************************/

/**
AY-3-8910(A)/8914/8916/8917/8930/YM2149 (others?):
                        _______    _______
                      _|       \__/       |_
   [4] VSS (GND) --  |_|1  *            40|_|  -- VCC (+5v)
                      _|                  |_
             [5] NC  |_|2               39|_|  <- TEST 1 [1]
                      _|                  |_
ANALOG CHANNEL B <-  |_|3               38|_|  -> ANALOG CHANNEL C
                      _|                  |_
ANALOG CHANNEL A <-  |_|4               37|_|  <> DA0
                      _|                  |_
             [5] NC  |_|5               36|_|  <> DA1
                      _|                  |_
            IOB7 <>  |_|6               35|_|  <> DA2
                      _|                  |_
            IOB6 <>  |_|7               34|_|  <> DA3
                      _|   /---\          |_
            IOB5 <>  |_|8  \-/ |   A    33|_|  <> DA4
                      _|   .   .   Y      |_
            IOB4 <>  |_|9  |---|   - S  32|_|  <> DA5
                      _|   '   '   3 O    |_
            IOB3 <>  |_|10   8     - U  31|_|  <> DA6
                      _|     3     8 N    |_
            IOB2 <>  |_|11   0     9 D  30|_|  <> DA7
                      _|     8     1      |_
            IOB1 <>  |_|12         0    29|_|  <- BC1
                      _|     P            |_
            IOB0 <>  |_|13              28|_|  <- BC2
                      _|                  |_
            IOA7 <>  |_|14              27|_|  <- BDIR
                      _|                  |_                     Prelim. DS:   YM2149/8930:
            IOA6 <>  |_|15              26|_|  <- TEST 2 [2,3]   CS2           /SEL
                      _|                  |_
            IOA5 <>  |_|16              25|_|  <- A8 [3]         CS1
                      _|                  |_
            IOA4 <>  |_|17              24|_|  <- /A9 [3]        /CS0
                      _|                  |_
            IOA3 <>  |_|18              23|_|  <- /RESET
                      _|                  |_
            IOA2 <>  |_|19              22|_|  == CLOCK
                      _|                  |_
            IOA1 <>  |_|20              21|_|  <> IOA0
                       |__________________|

[1] Based on the decap, TEST 1 connects to the Envelope Generator and/or the
    frequency divider somehow. Is this an input or an output?
[2] The TEST 2 input connects to the same selector as A8 and /A9 do on the 8910
    and acts as just another active high enable like A8(pin 25).
    The preliminary datasheet calls this pin CS2.
    On the 8914, it performs the same above function but additionally ?disables?
    the DA0-7 bus if pulled low/active. This additional function was removed
    on the 8910.
    This pin has an internal pullup.
    On the AY8930 and YM2149, this pin is /SEL; if low, clock input is halved.
[3] These 3 pins are technically enables, and have pullups/pulldowns such that
    if the pins are left floating, the chip remains enabled.
[4] On the AY-3-8910 the bond wire for the VSS pin goes to the substrate frame,
    and then a separate bond wire connects it to a pad between pins 21 and 22.
[5] These pins lack internal bond wires entirely.


AY-3-8912(A):
                        _______    _______
                      _|       \__/       |_
ANALOG CHANNEL C <-  |_|1  *            28|_|  <> DA0
                      _|                  |_
          TEST 1 ->  |_|2               27|_|  <> DA1
                      _|                  |_
       VCC (+5V) --  |_|3               26|_|  <> DA2
                      _|                  |_
ANALOG CHANNEL B <-  |_|4               25|_|  <> DA3
                      _|    /---\         |_
ANALOG CHANNEL A <-  |_|5   \-/ |   A   24|_|  <> DA4
                      _|    .   .   Y     |_
       VSS (GND) --  |_|6   |---|   - S 23|_|  <> DA5
                      _|    '   '   3 O   |_
            IOA7 <>  |_|7    T 8    - U 22|_|  <> DA6
                      _|     A 3    8 N   |_
            IOA6 <>  |_|8    I 1    9 D 21|_|  <> DA7
                      _|     W 1    1     |_
            IOA5 <>  |_|9    A  C   2   20|_|  <- BC1
                      _|     N  D         |_
            IOA4 <>  |_|10      A       19|_|  <- BC2
                      _|                  |_
            IOA3 <>  |_|11              18|_|  <- BDIR
                      _|                  |_
            IOA2 <>  |_|12              17|_|  <- A8
                      _|                  |_
            IOA1 <>  |_|13              16|_|  <- /RESET
                      _|                  |_
            IOA0 <>  |_|14              15|_|  == CLOCK
                       |__________________|


AY-3-8913:
                        _______    _______
                      _|       \__/       |_
   [1] VSS (GND) --  |_|1  *            24|_|  <- /CHIP SELECT [2]
                      _|                  |_
            BDIR ->  |_|2               23|_|  <- A8
                      _|                  |_
             BC1 ->  |_|3               22|_|  <- /A9
                      _|    /---\         |_
             DA7 <>  |_|4   \-/ |   A   21|_|  <- /RESET
                      _|    .   .   Y     |_
             DA6 <>  |_|5   |---|   -   20|_|  == CLOCK
                      _|    '   '   3     |_
             DA5 <>  |_|6    T 8    -   19|_|  -- VSS (GND) [1]
                      _|     A 3    8     |_
             DA4 <>  |_|7    I 3    9   18|_|  -> ANALOG CHANNEL C
                      _|     W 2    1     |_
             DA3 <>  |_|8    A      3   17|_|  -> ANALOG CHANNEL A
                      _|     N C          |_
             DA2 <>  |_|9      -        16|_|  NC(?)
                      _|       A          |_
             DA1 <>  |_|10              15|_|  -> ANALOG CHANNEL B
                      _|                  |_
             DA0 <>  |_|11              14|_|  ?? TEST IN [3]
                      _|                  |_
    [4] TEST OUT ??  |_|12              13|_|  -- VCC (+5V)
                       |__________________|

[1] Both of these are ground, they are probably connected together internally. Grounding either one should work.
[2] This is effectively another enable, much like TEST 2 is on the AY-3-8910 and 8914, but active low
[3] This is claimed to be equivalent to TEST 1 on the datasheet
[4] This is claimed to be equivalent to TEST 2 on the datasheet


GI AY-3-8910/A Programmable Sound Generator (PSG): 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment but default was 0000 for the "common" part shipped
    (probably die "-100").
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
    and high chip enable, respectively.
  AY-3-8910:  Unused bits in registers have unknown behavior.
  AY-3-8910A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8910 die is labeled "90-32033" with a 1979 copyright and a "-100" die
    code.
  AY-3-8910A die is labeled "90-32128" with a 1983 copyright.
GI AY-3-8912/A: 1 I/O port
  /A9 pin doesn't exist and is considered pulled low.
  TEST2 pin doesn't exist and is considered pulled high.
  IOB pins do not exist and have unknown behavior if driven high/low and read
    back.
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment but default was 0000 for the "common" part shipped
  AY-3-8912:  Unused bits in registers have unknown behavior.
  AY-3-8912A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8912 die is unknown.
  AY-3-8912A or A/P die is unknown.
AY-3-8913: 0 I/O ports
  BC2 pin doesn't exist and is considered pulled high.
  IOA/B pins do not exist and have unknown behavior if driven high/low and read back.
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment but default was 0000 for the "common" part shipped
  AY-3-8913:  Unused bits in registers have unknown behavior.
  AY-3-8913 die is unknown.
GI AY-3-8914/A: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment but was 0000 for the part shipped with the
    Intellivision.
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
    and high chip enable, respectively.
  TEST2 additionally ?disables? the data bus if pulled low.
  The register mapping is different from the AY-3-8910, the AY-3-8914 register
    mapping matches the "preliminary" 1978 AY-3-8910 datasheet.
  The Envelope/Volume control register is 6 bits wide instead of 5 bits, and
    the additional bit combines with the M bit to form a bit pair C0 and C1,
    which shift the volume output of the Envelope generator right by 0, 1 or 2
    bits on a given channel, or allow the low 4 bits to drive the channel
    volume.
  AY-3-8914:  Unused bits in registers have unknown behavior.
  AY-3-8914A: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8914 die is labeled "90-32022" with a 1978 copyright.
  AY-3-8914A die is unknown.
GI AY-3-8916: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment; its mask is unknown. This chip was shipped
    with certain later Intellivision II systems.
  Pins 24, 25, and 26 are /A9, /A8(!), and TEST2, which are an active low,
    low(!) and high chip enable, respectively.
    NOTE: the /A8 enable polarity may be mixed up with AY-3-8917 below.
  AY-3-8916: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8916 die is unknown.
GI AY-3-8917: 2 I/O ports
  A7 thru A4 enable state for selecting a register can be changed with a
    factory mask adjustment but was 1111 for the part shipped with the
    Intellivision ECS module.
  Pins 24, 25, and 26 are /A9, A8, and TEST2, which are an active low, high
    and high chip enable, respectively.
    NOTE: the A8 enable polarity may be mixed up with AY-3-8916 above.
  AY-3-8917: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  AY-3-8917 die is unknown.
Microchip AY8930 Enhanced Programmable Sound Generator (EPSG): 2 I/O ports
  BC2 pin exists but is always considered pulled high. The pin might have no
    bond wire at all.
  Pins 2 and 5 might be additional test pins rather than being NC.
  A7 thru A4 enable state for selecting a register are 0000 for all? parts
    shipped.
  Pins 24 and 25 are /A9, A8 which are an active low and high chip enable.
  Pin 26 is /SELECT which if driven low divides the input clock by 2.
  Writing 0xAn or 0xBn to register 0x0D turns on extended mode, which enables
    an additional 16 registers (banked using 0x0D bit 0), and clears the
    contents of all of the registers except the high 3 bits of register 0x0D
    (according to the datasheet).
  If the AY8930's extended mode is enabled, it gains higher resolution
    frequency and volume control, separate volume per-channel, and the duty
    cycle can be adjusted for the 3 channels.
  If the mode is not enabled, it behaves almost exactly like an AY-3-8910(A?),
    barring the BC2 and /SELECT differences.
  AY8930: Unused bits in registers have unknown behavior, but the datasheet
    explicitly states that unused bits always read as 0.
  I/O current source/sink behavior is unknown.
  AY8930 die is unknown.
Yamaha YM2149 Software-Controlled Sound Generator (SSG): 2 I/O ports
  A7 thru A4 enable state for selecting a register are 0000 for all? parts
    shipped.
  Pins 24 and 25 are /A9, A8 which are an active low and high chip enable.
  Pin 26 is /SEL which if driven low divides the input clock by 2.
  The YM2149 envelope register has 5 bits of resolution internally, allowing
  for smoother volume ramping, though the register for setting its direct
  value remains 4 bits wide.
  YM2149: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  YM2149 die is unknown; only one die revision, 'G', has been observed
    from Yamaha chip/datecode silkscreen surface markings.
Yamaha YM2203: 2 I/O ports
  The pinout of this chip is completely different from the AY-3-8910.
  The entire way this chip is accessed is completely different from the usual
    AY-3-8910 selection of chips, so there is a /CS and a /RD and a /WR and
    an A0 pin; The chip status can be read back by reading the register
    select address.
  The chip has a 3-channel, 4-op FM synthesis sound core in it, not discussed
    in this source file.
  The first 16 registers are the same(?) as the YM2149.
  YM2203: Unused bits in registers have unknown behavior.
  I/O current source/sink behavior is unknown.
  YM2203 die is unknown; three die revisions, 'D', 'F' and 'H', have been
    observed from Yamaha chip/datecode silkscreen surface markings. It is
    unknown what behavioral differences exist between these revisions.
    The 'D' revision only appears during the first year of production, 1984, on chips marked 'YM2203B'
    The 'F' revision exists from 1984?-1991, chips are marked 'YM2203C'
    The 'H' revision exists from 1991 onward, chips are marked 'YM2203C'
Yamaha YM3439: limited info: CMOS version of YM2149?
Yamaha YMZ284: limited info: 0 I/O port, different clock divider
  The chip selection logic is again simplified here: pin 1 is /WR, pin 2 is
    /CS and pin 3 is A0.
  D0-D7 are conveniently all on one side of the 16-pin package.
  Pin 8 is /IC (initial clear), with an internal pullup.
Yamaha YMZ294: limited info: 0 I/O port
  Pinout is identical to YMZ284 except for two additions: pin 8 selects
    between 4MHz (H) and 6MHz (L), while pin 10 is /TEST.
OKI M5255, Winbond WF19054, JFC 95101, File KC89C72, Toshiba T7766A : differences to be listed

AY8930 Expanded mode registers :
	Bank Register Bits
	A    0        xxxx xxxx Channel A Tone period fine tune
	A    1        xxxx xxxx Channel A Tone period coarse tune
	A    2        xxxx xxxx Channel B Tone period fine tune
	A    3        xxxx xxxx Channel B Tone period coarse tune
	A    4        xxxx xxxx Channel C Tone period fine tune
	A    5        xxxx xxxx Channel C Tone period coarse tune
	A    6        xxxx xxxx Noise period
	A    7        x--- ---- I/O Port B input(0) / output(1)
	              -x-- ---- I/O Port A input(0) / output(1)
	              --x- ---- Channel C Noise enable(0) / disable(1)
	              ---x ---- Channel B Noise enable(0) / disable(1)
	              ---- x--- Channel A Noise enable(0) / disable(1)
	              ---- -x-- Channel C Tone enable(0) / disable(1)
	              ---- --x- Channel B Tone enable(0) / disable(1)
	              ---- ---x Channel A Tone enable(0) / disable(1)
	A    8        --x- ---- Channel A Envelope mode
	              ---x xxxx Channel A Tone volume
	A    9        --x- ---- Channel B Envelope mode
	              ---x xxxx Channel B Tone volume
	A    A        --x- ---- Channel C Envelope mode
	              ---x xxxx Channel C Tone volume
	A    B        xxxx xxxx Channel A Envelope period fine tune
	A    C        xxxx xxxx Channel A Envelope period coarse tune
	A    D        101- ---- 101 = Expanded mode enable, other AY-3-8910A Compatiblity mode
	              ---0 ---- 0 for Register Bank A
	              ---- xxxx Channel A Envelope Shape/Cycle
	A    E        xxxx xxxx 8 bit Parallel I/O on Port A
	A    F        xxxx xxxx 8 bit Parallel I/O on Port B

	B    0        xxxx xxxx Channel B Envelope period fine tune
	B    1        xxxx xxxx Channel B Envelope period coarse tune
	B    2        xxxx xxxx Channel C Envelope period fine tune
	B    3        xxxx xxxx Channel C Envelope period coarse tune
	B    4        ---- xxxx Channel B Envelope Shape/Cycle
	B    5        ---- xxxx Channel C Envelope Shape/Cycle
	B    6        ---- xxxx Channel A Duty Cycle
	B    7        ---- xxxx Channel B Duty Cycle
	B    8        ---- xxxx Channel C Duty Cycle
	B    9        xxxx xxxx Noise "And" Mask
	B    A        xxxx xxxx Noise "Or" Mask
	B    B        Reserved (Read as 0)
	B    C        Reserved (Read as 0)
	B    D        101- ---- 101 = Expanded mode enable, other AY-3-8910A Compatiblity mode
	              ---1 ---- 1 for Register Bank B
	              ---- xxxx Channel A Envelope Shape
	B    E        Reserved (Read as 0)
	B    F        Test (Function unknown)

Decaps:
AY-3-8914 - http://siliconpr0n.org/map/gi/ay-3-8914/mz_mit20x/
AY-3-8910 - http://privatfrickler.de/blick-auf-den-chip-soundchip-general-instruments-ay-3-8910/
AY-3-8910A - https://seanriddledecap.blogspot.com/2017/01/gi-ay-3-8910-ay-3-8910a-gi-8705-cba.html (TODO: update this link when it has its own page at seanriddle.com)

Links:
AY-3-8910 'preliminary' datasheet (which actually describes the AY-3-8914) from 1978:
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_100.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_101.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_102.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_103.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_104.png
  http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_105.png
AY-3-8910/8912 Feb 1979 manual: https://web.archive.org/web/20140217224114/http://dev-docs.atariforge.org/files/GI_AY-3-8910_Feb-1979.pdf
AY-3-8910/8912/8913 post-1983 manual: http://map.grauw.nl/resources/sound/generalinstrument_ay-3-8910.pdf or http://www.ym2149.com/ay8910.pdf
AY-8930 datasheet: http://www.ym2149.com/ay8930.pdf
YM2149 datasheet: http://www.ym2149.com/ym2149.pdf
YM2203 English datasheet: http://www.appleii-box.de/APPLE2/JonasCard/YM2203%20datasheet.pdf
YM2203 Japanese datasheet contents, translated: http://www.larwe.com/technical/chip_ym2203.html
*/

#include "driver.h"
#include "ay8910.h"
#include "state.h"

#define MAX_OUTPUT 0x7fff

#define STEP 2

#define SINGLE_CHANNEL_MIXER // mix all channels into one stream to avoid having to resample all separately, undef for debugging purpose

//#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//define LOG(x)	printf x
#else
#define LOG(x)
#endif

int ay8910_index_ym;
static int num = 0, ym_num = 0;

struct AY8910
{
	int Channel;
	int ready;
	read8_handler PortAread;
	read8_handler PortBread;
	write8_handler PortAwrite;
	write8_handler PortBwrite;
	INT32 register_latch;
	UINT8 Regs[16];
	INT32 lastEnable;
	INT32 PeriodA,PeriodB,PeriodC,PeriodN,PeriodE;
	INT32 CountA,CountB,CountC,CountN,CountE;
	UINT32 VolA,VolB,VolC,VolE;
	UINT8 EnvelopeA,EnvelopeB,EnvelopeC;
	UINT8 OutputA,OutputB,OutputC,OutputN;
	INT8 CountEnv;
	UINT8 Hold,Alternate,Attack,Holding;
	INT32 RNG;
	unsigned int VolTable[32];
#ifdef SINGLE_CHANNEL_MIXER
	unsigned int mix_vol[3];
#endif
};

/* register id's */
#define AY_AFINE	(0)
#define AY_ACOARSE	(1)
#define AY_BFINE	(2)
#define AY_BCOARSE	(3)
#define AY_CFINE	(4)
#define AY_CCOARSE	(5)
#define AY_NOISEPER	(6)
#define AY_ENABLE	(7)
#define AY_AVOL		(8)
#define AY_BVOL		(9)
#define AY_CVOL		(10)
#define AY_EFINE	(11)
#define AY_ECOARSE	(12)
#define AY_ESHAPE	(13)

#define AY_PORTA	(14)
#define AY_PORTB	(15)


static struct AY8910 AYPSG[MAX_8910];		/* array of PSG's */



static void _AYWriteReg(int n, int r, int v)
{
	struct AY8910 *PSG = &AYPSG[n];
	int old;


	PSG->Regs[r] = v;

	/* A note about the period of tones, noise and envelope: for speed reasons,*/
	/* we count down from the period to 0, but careful studies of the chip     */
	/* output prove that it instead counts up from 0 until the counter becomes */
	/* greater or equal to the period. This is an important difference when the*/
	/* program is rapidly changing the period to modulate the sound.           */
	/* To compensate for the difference, when the period is changed we adjust  */
	/* our internal counter.                                                   */
	/* Also, note that period = 0 is the same as period = 1. This is mentioned */
	/* in the YM2203 data sheets. However, this does NOT apply to the Envelope */
	/* period. In that case, period = 0 is half as period = 1. */
	switch( r )
	{
	case AY_AFINE:
	case AY_ACOARSE:
		PSG->Regs[AY_ACOARSE] &= 0x0f;
		old = PSG->PeriodA;
		PSG->PeriodA = (PSG->Regs[AY_AFINE] + (INT32)256 * PSG->Regs[AY_ACOARSE]) * STEP;
		if (PSG->PeriodA == 0) PSG->PeriodA = STEP;
		PSG->CountA += PSG->PeriodA - old;
		if (PSG->CountA <= 0) PSG->CountA = 1;
		break;
	case AY_BFINE:
	case AY_BCOARSE:
		PSG->Regs[AY_BCOARSE] &= 0x0f;
		old = PSG->PeriodB;
		PSG->PeriodB = (PSG->Regs[AY_BFINE] + (INT32)256 * PSG->Regs[AY_BCOARSE]) * STEP;
		if (PSG->PeriodB == 0) PSG->PeriodB = STEP;
		PSG->CountB += PSG->PeriodB - old;
		if (PSG->CountB <= 0) PSG->CountB = 1;
		break;
	case AY_CFINE:
	case AY_CCOARSE:
		PSG->Regs[AY_CCOARSE] &= 0x0f;
		old = PSG->PeriodC;
		PSG->PeriodC = (PSG->Regs[AY_CFINE] + (INT32)256 * PSG->Regs[AY_CCOARSE]) * STEP;
		if (PSG->PeriodC == 0) PSG->PeriodC = STEP;
		PSG->CountC += PSG->PeriodC - old;
		if (PSG->CountC <= 0) PSG->CountC = 1;
		break;
	case AY_NOISEPER:
		PSG->Regs[AY_NOISEPER] &= 0x1f;
		old = PSG->PeriodN;
		PSG->PeriodN = PSG->Regs[AY_NOISEPER] * (INT32)STEP;
		if (PSG->PeriodN == 0) PSG->PeriodN = STEP;
		PSG->CountN += PSG->PeriodN - old;
		if (PSG->CountN <= 0) PSG->CountN = 1;
		break;
	case AY_ENABLE:
		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x40) != (PSG->Regs[AY_ENABLE] & 0x40)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, (PSG->Regs[AY_ENABLE] & 0x40) ? PSG->Regs[AY_PORTA] : 0xff);
		}

		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x80) != (PSG->Regs[AY_ENABLE] & 0x80)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, (PSG->Regs[AY_ENABLE] & 0x80) ? PSG->Regs[AY_PORTB] : 0xff);
		}

		PSG->lastEnable = PSG->Regs[AY_ENABLE];
		break;
	case AY_AVOL:
		PSG->Regs[AY_AVOL] &= 0x1f;
		PSG->EnvelopeA = PSG->Regs[AY_AVOL] & 0x10;
		PSG->VolA = PSG->EnvelopeA ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_AVOL] ? PSG->Regs[AY_AVOL]*2+1 : 0];
		break;
	case AY_BVOL:
		PSG->Regs[AY_BVOL] &= 0x1f;
		PSG->EnvelopeB = PSG->Regs[AY_BVOL] & 0x10;
		PSG->VolB = PSG->EnvelopeB ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_BVOL] ? PSG->Regs[AY_BVOL]*2+1 : 0];
		break;
	case AY_CVOL:
		PSG->Regs[AY_CVOL] &= 0x1f;
		PSG->EnvelopeC = PSG->Regs[AY_CVOL] & 0x10;
		PSG->VolC = PSG->EnvelopeC ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_CVOL] ? PSG->Regs[AY_CVOL]*2+1 : 0];
		break;
	case AY_EFINE:
	case AY_ECOARSE:
		old = PSG->PeriodE;
		PSG->PeriodE = ((PSG->Regs[AY_EFINE] + (INT32)256 * PSG->Regs[AY_ECOARSE])) * STEP;
		if (PSG->PeriodE == 0) PSG->PeriodE = STEP / 2;
		PSG->CountE += PSG->PeriodE - old;
		if (PSG->CountE <= 0) PSG->CountE = 1;
		break;
	case AY_ESHAPE:
		/* envelope shapes:
		C AtAlH
		0 0 x x  \___

		0 1 x x  /___

		1 0 0 0  \\\\

		1 0 0 1  \___

		1 0 1 0  \/\/
		          ___
		1 0 1 1  \

		1 1 0 0  ////
		          ___
		1 1 0 1  /

		1 1 1 0  /\/\

		1 1 1 1  /___

		The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
		has twice the steps, happening twice as fast. Since the end result is
		just a smoother curve, we always use the YM2149 behaviour.
		*/
		PSG->Regs[AY_ESHAPE] &= 0x0f;
		PSG->Attack = (PSG->Regs[AY_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((PSG->Regs[AY_ESHAPE] & 0x08) == 0)
		{
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			PSG->Hold = 1;
			PSG->Alternate = PSG->Attack;
		}
		else
		{
			PSG->Hold = PSG->Regs[AY_ESHAPE] & 0x01;
			PSG->Alternate = PSG->Regs[AY_ESHAPE] & 0x02;
		}
		PSG->CountE = PSG->PeriodE;
		PSG->CountEnv = 0x1f;
		PSG->Holding = 0;
		PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
		if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
		if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
		if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
		break;
	case AY_PORTA:
		if (PSG->Regs[AY_ENABLE] & 0x40)
		{
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, PSG->Regs[AY_PORTA]);
			else {
				LOG(("PC %04x: warning - write %02x to 8910 #%d Port A\n",activecpu_get_pc(),PSG->Regs[AY_PORTA],n));
			}
		}
		else
		{
			LOG(("warning: write to 8910 #%d Port A set as input - ignored\n",n));
		}
		break;
	case AY_PORTB:
		if (PSG->Regs[AY_ENABLE] & 0x80)
		{
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, PSG->Regs[AY_PORTB]);
			else {
				LOG(("PC %04x: warning - write %02x to 8910 #%d Port B\n",activecpu_get_pc(),PSG->Regs[AY_PORTB],n));
			}
		}
		else
		{
			LOG(("warning: write to 8910 #%d Port B set as input - ignored\n",n));
		}
		break;
	}
}


/* write a register on AY8910 chip number 'n' */
void AYWriteReg(int chip, int r, int v)
{
	struct AY8910 *PSG = &AYPSG[chip];


	if (r > 15) return;
	if (r < 14)
	{
		if (r == AY_ESHAPE || PSG->Regs[r] != v)
		{
			/* update the output buffer before changing the register */
			stream_update(PSG->Channel,0);
		}
	}

	_AYWriteReg(chip,r,v);
}



unsigned char AYReadReg(int n, int r)
{
	struct AY8910 *PSG = &AYPSG[n];


	if (r > 15) return 0;

	switch (r)
	{
	case AY_PORTA:
		if ((PSG->Regs[AY_ENABLE] & 0x40) != 0) {
			LOG(("warning: read from 8910 #%d Port A set as output\n",n));
		}
		/*
		   even if the port is set as output, we still need to return the external
		   data. Some games, like kidniki, need this to work.
		 */
		if (PSG->PortAread) PSG->Regs[AY_PORTA] = (*PSG->PortAread)(0);
			else { LOG(("PC %04x: warning - read 8910 #%d Port A\n",activecpu_get_pc(),n)); }
		break;
	case AY_PORTB:
		if ((PSG->Regs[AY_ENABLE] & 0x80) != 0) {
			LOG(("warning: read from 8910 #%d Port B set as output\n",n));
		}
		if (PSG->PortBread) PSG->Regs[AY_PORTB] = (*PSG->PortBread)(0);
			else { LOG(("PC %04x: warning - read 8910 #%d Port B\n",activecpu_get_pc(),n)); }
		break;
	}
	return PSG->Regs[r];
}


void AY8910Write(int chip,int a,int data)
{
	struct AY8910 *PSG = &AYPSG[chip];

	if (a & 1)
	{	/* Data port */
		AYWriteReg(chip,PSG->register_latch,data);
	}
	else
	{	/* Register port */
		PSG->register_latch = data & 0x0f;
	}
}

int AY8910Read(int chip)
{
	struct AY8910 *PSG = &AYPSG[chip];

	return AYReadReg(chip,PSG->register_latch);
}


/* AY8910 interface */
READ_HANDLER( AY8910_read_port_0_r ) { return AY8910Read(0); }
READ_HANDLER( AY8910_read_port_1_r ) { return AY8910Read(1); }
READ_HANDLER( AY8910_read_port_2_r ) { return AY8910Read(2); }
READ_HANDLER( AY8910_read_port_3_r ) { return AY8910Read(3); }
READ_HANDLER( AY8910_read_port_4_r ) { return AY8910Read(4); }
READ16_HANDLER( AY8910_read_port_0_lsb_r ) { return AY8910Read(0); }
READ16_HANDLER( AY8910_read_port_1_lsb_r ) { return AY8910Read(1); }
READ16_HANDLER( AY8910_read_port_2_lsb_r ) { return AY8910Read(2); }
READ16_HANDLER( AY8910_read_port_3_lsb_r ) { return AY8910Read(3); }
READ16_HANDLER( AY8910_read_port_4_lsb_r ) { return AY8910Read(4); }
READ16_HANDLER( AY8910_read_port_0_msb_r ) { return AY8910Read(0) << 8; }
READ16_HANDLER( AY8910_read_port_1_msb_r ) { return AY8910Read(1) << 8; }
READ16_HANDLER( AY8910_read_port_2_msb_r ) { return AY8910Read(2) << 8; }
READ16_HANDLER( AY8910_read_port_3_msb_r ) { return AY8910Read(3) << 8; }
READ16_HANDLER( AY8910_read_port_4_msb_r ) { return AY8910Read(4) << 8; }

WRITE_HANDLER( AY8910_control_port_0_w ) { AY8910Write(0,0,data); }
WRITE_HANDLER( AY8910_control_port_1_w ) { AY8910Write(1,0,data); }
WRITE_HANDLER( AY8910_control_port_2_w ) { AY8910Write(2,0,data); }
WRITE_HANDLER( AY8910_control_port_3_w ) { AY8910Write(3,0,data); }
WRITE_HANDLER( AY8910_control_port_4_w ) { AY8910Write(4,0,data); }
WRITE16_HANDLER( AY8910_control_port_0_lsb_w ) { if (ACCESSING_LSB) AY8910Write(0,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_1_lsb_w ) { if (ACCESSING_LSB) AY8910Write(1,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_2_lsb_w ) { if (ACCESSING_LSB) AY8910Write(2,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_3_lsb_w ) { if (ACCESSING_LSB) AY8910Write(3,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_4_lsb_w ) { if (ACCESSING_LSB) AY8910Write(4,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_0_msb_w ) { if (ACCESSING_MSB) AY8910Write(0,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_1_msb_w ) { if (ACCESSING_MSB) AY8910Write(1,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_2_msb_w ) { if (ACCESSING_MSB) AY8910Write(2,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_3_msb_w ) { if (ACCESSING_MSB) AY8910Write(3,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_4_msb_w ) { if (ACCESSING_MSB) AY8910Write(4,0,data >> 8); }

WRITE_HANDLER( AY8910_write_port_0_w ) { AY8910Write(0,1,data); }
WRITE_HANDLER( AY8910_write_port_1_w ) { AY8910Write(1,1,data); }
WRITE_HANDLER( AY8910_write_port_2_w ) { AY8910Write(2,1,data); }
WRITE_HANDLER( AY8910_write_port_3_w ) { AY8910Write(3,1,data); }
WRITE_HANDLER( AY8910_write_port_4_w ) { AY8910Write(4,1,data); }
WRITE16_HANDLER( AY8910_write_port_0_lsb_w ) { if (ACCESSING_LSB) AY8910Write(0,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_1_lsb_w ) { if (ACCESSING_LSB) AY8910Write(1,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_2_lsb_w ) { if (ACCESSING_LSB) AY8910Write(2,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_3_lsb_w ) { if (ACCESSING_LSB) AY8910Write(3,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_4_lsb_w ) { if (ACCESSING_LSB) AY8910Write(4,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_0_msb_w ) { if (ACCESSING_MSB) AY8910Write(0,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_1_msb_w ) { if (ACCESSING_MSB) AY8910Write(1,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_2_msb_w ) { if (ACCESSING_MSB) AY8910Write(2,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_3_msb_w ) { if (ACCESSING_MSB) AY8910Write(3,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_4_msb_w ) { if (ACCESSING_MSB) AY8910Write(4,1,data >> 8); }



static void AY8910Update(int chip,
#ifdef SINGLE_CHANNEL_MIXER
                         INT16 *buffer,
#else
                         INT16 **buffer,
#endif
                         int length)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int outn;

	INT16 *buf1;
#ifdef SINGLE_CHANNEL_MIXER
	INT32 tmp_buf;
	buf1 = buffer;
#else
	INT16 *buf2,*buf3;
	buf1 = buffer[0];
	buf2 = buffer[1];
	buf3 = buffer[2];
#endif

	/* hack to prevent us from hanging when starting filtered outputs */
	if (!PSG->ready)
	{
		memset(buf1, 0, length * sizeof(*buf1));
#ifndef SINGLE_CHANNEL_MIXER
		if (buf2)
			memset(buf2, 0, length * sizeof(*buf2));
		if (buf3)
			memset(buf3, 0, length * sizeof(*buf3));
#endif
		return;
	}

	/* The 8910 has three outputs, each output is the mix of one of the three */
	/* tone generators and of the (single) noise generator. The two are mixed */
	/* BEFORE going into the DAC. The formula to mix each channel is: */
	/* (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable). */
	/* Note that this means that if both tone and noise are disabled, the output */
	/* is 1, not 0, and can be modulated changing the volume. */


	/* If the channels are disabled, set their output to 1, and increase the */
	/* counter, if necessary, so they will not be inverted during this update. */
	/* Setting the output to 1 is necessary because a disabled channel is locked */
	/* into the ON state (see above); and it has no effect if the volume is 0. */
	/* If the volume is 0, increase the counter, but don't touch the output. */
	if (PSG->Regs[AY_ENABLE] & 0x01)
	{
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
		PSG->OutputA = 1;
	}
	else if (PSG->Regs[AY_AVOL] == 0)
	{
		/* note that I do count += length, NOT count = length + 1. You might think */
		/* it's the same since the volume is 0, but doing the latter could cause */
		/* interferencies when the program is rapidly modulating the volume. */
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x02)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
		PSG->OutputB = 1;
	}
	else if (PSG->Regs[AY_BVOL] == 0)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x04)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
		PSG->OutputC = 1;
	}
	else if (PSG->Regs[AY_CVOL] == 0)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
	}

	/* for the noise channel we must not touch OutputN - it's also not necessary */
	/* since we use outn. */
	if ((PSG->Regs[AY_ENABLE] & 0x38) == 0x38)	/* all off */
		if (PSG->CountN <= length*STEP) PSG->CountN += length*STEP;

	outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);


	/* buffering loop */
	while (length)
	{
		int vola,volb,volc;
		int left;


		/* vola, volb and volc keep track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vola = volb = volc = 0;

		left = STEP;
		do
		{
			int nextevent;


			if (PSG->CountN < left) nextevent = PSG->CountN;
			else nextevent = left;

			if (outn & 0x08)
			{
				if (PSG->OutputA) vola += PSG->CountA;
				PSG->CountA -= nextevent;
				/* PeriodA is the half period of the square wave. Here, in each */
				/* loop I add PeriodA twice, so that at the end of the loop the */
				/* square wave is in the same status (0 or 1) it was at the start. */
				/* vola is also incremented by PeriodA, since the wave has been 1 */
				/* exactly half of the time, regardless of the initial position. */
				/* If we exit the loop in the middle, OutputA has to be inverted */
				/* and vola incremented only if the exit status of the square */
				/* wave is 1. */
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						if (PSG->OutputA) vola += PSG->PeriodA;
						break;
					}
					PSG->CountA += PSG->PeriodA;
					vola += PSG->PeriodA;
				}
				if (PSG->OutputA) vola -= PSG->CountA;
			}
			else
			{
				PSG->CountA -= nextevent;
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						break;
					}
					PSG->CountA += PSG->PeriodA;
				}
			}

			if (outn & 0x10)
			{
				if (PSG->OutputB) volb += PSG->CountB;
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						if (PSG->OutputB) volb += PSG->PeriodB;
						break;
					}
					PSG->CountB += PSG->PeriodB;
					volb += PSG->PeriodB;
				}
				if (PSG->OutputB) volb -= PSG->CountB;
			}
			else
			{
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						break;
					}
					PSG->CountB += PSG->PeriodB;
				}
			}

			if (outn & 0x20)
			{
				if (PSG->OutputC) volc += PSG->CountC;
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						if (PSG->OutputC) volc += PSG->PeriodC;
						break;
					}
					PSG->CountC += PSG->PeriodC;
					volc += PSG->PeriodC;
				}
				if (PSG->OutputC) volc -= PSG->CountC;
			}
			else
			{
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						break;
					}
					PSG->CountC += PSG->PeriodC;
				}
			}

			PSG->CountN -= nextevent;
			if (PSG->CountN <= 0)
			{
				/* Is noise output going to change? */
				if ((PSG->RNG + 1) & 2)	/* (bit0^bit1)? */
				{
					PSG->OutputN = ~PSG->OutputN;
					outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);
				}

				/* The Random Number Generator of the 8910 is a 17-bit shift */
				/* register. The input to the shift register is bit0 XOR bit3 */
				/* (bit0 is the output). This was verified on AY-3-8910 and YM2149 chips. */

				/* The following is a fast way to compute bit17 = bit0^bit3. */
				/* Instead of doing all the logic operations, we only check */
				/* bit0, relying on the fact that after three shifts of the */
				/* register, what now is bit3 will become bit0, and will */
				/* invert, if necessary, bit14, which previously was bit17. */
				if (PSG->RNG & 1) PSG->RNG ^= 0x24000; /* This version is called the "Galois configuration". */
				PSG->RNG >>= 1;
				PSG->CountN += PSG->PeriodN;
			}

			left -= nextevent;
		} while (left > 0);

		/* update envelope */
		if (PSG->Holding == 0)
		{
			PSG->CountE -= STEP;
			if (PSG->CountE <= 0)
			{
				do
				{
					PSG->CountEnv--;
					PSG->CountE += PSG->PeriodE;
				} while (PSG->CountE <= 0);

				/* check envelope current position */
				if (PSG->CountEnv < 0)
				{
					if (PSG->Hold)
					{
						if (PSG->Alternate)
							PSG->Attack ^= 0x1f;
						PSG->Holding = 1;
						PSG->CountEnv = 0;
					}
					else
					{
						/* if CountEnv has looped an odd number of times (usually 1), */
						/* invert the output. */
						if (PSG->Alternate && (PSG->CountEnv & 0x20))
 							PSG->Attack ^= 0x1f;

						PSG->CountEnv &= 0x1f;
					}
				}

				PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
				/* reload volume */
				if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
				if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
				if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
			}
		}

#ifdef SINGLE_CHANNEL_MIXER
		tmp_buf = (vola * (int)PSG->VolA * (int)PSG->mix_vol[0] + volb * (int)PSG->VolB * (int)PSG->mix_vol[1] + volc * (int)PSG->VolC * (int)PSG->mix_vol[2])/(int)(100*STEP);
		*(buf1++) = (tmp_buf < -32768) ? -32768 : ((tmp_buf > 32767) ? 32767 : tmp_buf);
#else
		*(buf1++) = (vola * PSG->VolA) / STEP;
		*(buf2++) = (volb * PSG->VolB) / STEP;
		*(buf3++) = (volc * PSG->VolC) / STEP;
#endif
		length--;
	}
}


void AY8910_set_clock(int chip, int clock)
{
	struct AY8910 *PSG = &AYPSG[chip];

#ifdef SINGLE_CHANNEL_MIXER
	stream_set_sample_rate(PSG->Channel, clock/8);
#else
	int ch;
	for (ch = 0; ch < 3; ch++)
		stream_set_sample_rate(PSG->Channel + ch, clock/8);
#endif
}

void AY8910_set_volume(int chip,int channel,int volume)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int ch;

	for (ch = 0; ch < 3; ch++)
		if (channel == ch || channel == ALL_8910_CHANNELS)
#ifdef SINGLE_CHANNEL_MIXER
			PSG->mix_vol[ch] = volume;
#else
			mixer_set_volume(PSG->Channel + ch, volume);
#endif
}


static void build_mixer_table(int chip)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int i;
	double out;


	/* calculate the volume->voltage conversion table */
	/* The AY-3-8910 has 16 levels, in a logarithmic scale (3dB per step) */
	/* The YM2149 still has 16 levels for the tone generators, but 32 for */
	/* the envelope generator (1.5dB per step). */
	out = MAX_OUTPUT;
	for (i = 31;i > 0;i--)
	{
		PSG->VolTable[i] = (unsigned int)(out + 0.5);	/* round to nearest */

		out /= 1.188502227;	/* = 10 ^ (1.5/20) = 1.5dB */
	}
	PSG->VolTable[0] = 0;
}



void AY8910_reset(int chip)
{
	int i;
	struct AY8910 *PSG = &AYPSG[chip];

	PSG->register_latch = 0;
	PSG->RNG = 1;
	PSG->OutputA = 0;
	PSG->OutputB = 0;
	PSG->OutputC = 0;
	PSG->OutputN = 0xff;
	PSG->lastEnable = -1;	/* force a write */
	for (i = 0;i < AY_PORTA;i++)
		_AYWriteReg(chip,i,0);	/* AYWriteReg() uses the timer system; we cannot */
								/* call it at this time because the timer system */
								/* has not been initialized. */
	PSG->ready = 1;
}

void AY8910_sh_reset(void)
{
	int i;

	for (i = 0;i < num + ym_num;i++)
		AY8910_reset(i);
}

static int AY8910_init(const char *chip_name,int chip,
		int clock,int volume,int sample_rate,
		mem_read_handler portAread,mem_read_handler portBread,
		mem_write_handler portAwrite,mem_write_handler portBwrite)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int i;
#ifdef SINGLE_CHANNEL_MIXER
	char buf[40];
	int gain = MIXER_GET_GAIN(volume);
	int pan = MIXER_GET_PAN(volume);
#else
	char buf[3][40];
	const char *name[3];
	int vol[3];
#endif

	/* the step clock for the tone and noise generators is the chip clock    */
	/* divided by 8; for the envelope generator of the AY-3-8910, it is half */
	/* that much (clock/16), but the envelope of the YM2149 goes twice as    */
	/* fast, therefore again clock/8.                                        */
// causes crashes with YM2610 games - overflow?
//	if (options.use_filter)
		sample_rate = clock/8;

	memset(PSG,0,sizeof(struct AY8910));
	PSG->PortAread = portAread;
	PSG->PortBread = portBread;
	PSG->PortAwrite = portAwrite;
	PSG->PortBwrite = portBwrite;

#ifdef SINGLE_CHANNEL_MIXER
	for (i = 0;i < 3;i++)
		PSG->mix_vol[i] = MIXER_GET_LEVEL(volume);
	sprintf(buf,"%s #%d",chip_name,chip);
	PSG->Channel = stream_init(buf,MIXERG(volume,gain,pan),sample_rate,chip,AY8910Update);
#else
	for (i = 0;i < 3;i++)
	{
		vol[i] = volume;
		name[i] = buf[i];
		sprintf(buf[i],"%s #%d Ch %c",chip_name,chip,'A'+i);
	}
	PSG->Channel = stream_init_multi(3,name,vol,sample_rate,chip,AY8910Update);
#endif

	if (PSG->Channel == -1)
		return 1;

	return 0;
}


static void AY8910_statesave(int chip)
{
	struct AY8910 *PSG = &AYPSG[chip];

	state_save_register_INT32("AY8910",  chip, "register_latch", &PSG->register_latch, 1);
	state_save_register_UINT8("AY8910",  chip, "Regs",           PSG->Regs,            8);
	state_save_register_INT32("AY8910",  chip, "lastEnable",     &PSG->lastEnable,     1);

	state_save_register_INT32("AY8910",  chip, "PeriodA",        &PSG->PeriodA,        1);
	state_save_register_INT32("AY8910",  chip, "PeriodB",        &PSG->PeriodB,        1);
	state_save_register_INT32("AY8910",  chip, "PeriodC",        &PSG->PeriodC,        1);
	state_save_register_INT32("AY8910",  chip, "PeriodN",        &PSG->PeriodN,        1);
	state_save_register_INT32("AY8910",  chip, "PeriodE",        &PSG->PeriodE,        1);

	state_save_register_INT32("AY8910",  chip, "CountA",         &PSG->CountA,         1);
	state_save_register_INT32("AY8910",  chip, "CountB",         &PSG->CountB,         1);
	state_save_register_INT32("AY8910",  chip, "CountC",         &PSG->CountC,         1);
	state_save_register_INT32("AY8910",  chip, "CountN",         &PSG->CountN,         1);
	state_save_register_INT32("AY8910",  chip, "CountE",         &PSG->CountE,         1);

	state_save_register_UINT32("AY8910", chip, "VolA",           &PSG->VolA,           1);
	state_save_register_UINT32("AY8910", chip, "VolB",           &PSG->VolB,           1);
	state_save_register_UINT32("AY8910", chip, "VolC",           &PSG->VolC,           1);
	state_save_register_UINT32("AY8910", chip, "VolE",           &PSG->VolE,           1);

	state_save_register_UINT8("AY8910",  chip, "EnvelopeA",      &PSG->EnvelopeA,      1);
	state_save_register_UINT8("AY8910",  chip, "EnvelopeB",      &PSG->EnvelopeB,      1);
	state_save_register_UINT8("AY8910",  chip, "EnvelopeC",      &PSG->EnvelopeC,      1);

	state_save_register_UINT8("AY8910",  chip, "OutputA",        &PSG->OutputA,        1);
	state_save_register_UINT8("AY8910",  chip, "OutputB",        &PSG->OutputB,        1);
	state_save_register_UINT8("AY8910",  chip, "OutputC",        &PSG->OutputC,        1);
	state_save_register_UINT8("AY8910",  chip, "OutputN",        &PSG->OutputN,        1);

	state_save_register_INT8("AY8910",   chip, "CountEnv",       &PSG->CountEnv,       1);
	state_save_register_UINT8("AY8910",  chip, "Hold",           &PSG->Hold,           1);
	state_save_register_UINT8("AY8910",  chip, "Alternate",      &PSG->Alternate,      1);
	state_save_register_UINT8("AY8910",  chip, "Attack",         &PSG->Attack,         1);
	state_save_register_UINT8("AY8910",  chip, "Holding",        &PSG->Holding,        1);
	state_save_register_INT32("AY8910",  chip, "RNG",            &PSG->RNG,            1);
}


int AY8910_sh_start(const struct MachineSound *msound)
{
	int chip;
	const struct AY8910interface *intf = msound->sound_interface;

	num = intf->num;

	for (chip = 0;chip < num;chip++)
	{
		if (AY8910_init(sound_name(msound),chip+ym_num,intf->baseclock,
				intf->mixing_level[chip] & 0xffff,
				Machine->sample_rate,
				intf->portAread[chip],intf->portBread[chip],
				intf->portAwrite[chip],intf->portBwrite[chip]) != 0)
			return 1;
		build_mixer_table(chip+ym_num);

		AY8910_statesave(chip+ym_num);
	}

	return 0;
}

void AY8910_sh_stop(void)
{
	num = 0;
}

int AY8910_sh_start_ym(const struct MachineSound *msound)
{
	int chip;
	const struct AY8910interface *intf = msound->sound_interface;

	ym_num = intf->num;
	ay8910_index_ym = num;

	for (chip = 0;chip < ym_num;chip++)
	{
		if (AY8910_init(sound_name(msound),chip+num,intf->baseclock,
				intf->mixing_level[chip] & 0xffff,
				Machine->sample_rate,
				intf->portAread[chip],intf->portBread[chip],
				intf->portAwrite[chip],intf->portBwrite[chip]) != 0)
			return 1;
		build_mixer_table(chip+num);


		AY8910_statesave(chip+num);
	}
	return 0;
}

void AY8910_sh_stop_ym(void)
{
	ym_num = 0;
}

#ifdef PINMAME
void AY8910_set_reverb_filter(int chip, float delay, float force)
{
	struct AY8910 *PSG = &AYPSG[chip];

#ifdef SINGLE_CHANNEL_MIXER
	//stream_update(PSG->Channel, 0); //!!?
	mixer_set_reverb_filter(PSG->Channel, delay, force);
#else
	int ch;
	for (ch = 0; ch < 3; ch++)
		//stream_update(PSG->Channel + ch, 0); //!!?
		mixer_set_reverb_filter(PSG->Channel + ch, delay, force);
#endif
}
#endif
