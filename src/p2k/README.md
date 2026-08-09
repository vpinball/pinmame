# Pinball 2000

Midway's last pinball platform, and the only one in PinMAME that is a PC: a Cyrix MediaGX with a
CX5520 southbridge, a PC97317 Super-I/O and a "Prism" PCI card carrying the ADSP-2104 sound
hardware and the link to the pinball power driver board.

Every version of both games that the Encore set carries is a driver of its own, the newest of each
being the parent:

| Driver | Game | |
|---|---|---|
| `rfm_180` | Pinball 2000: Revenge From Mars (1.80) | 09/2003 |
| `rfm_160` | | 09/2003 |
| `rfm_150` | | 07/2000 |
| `rfm_140` | | 01/2000 |
| `rfm_120` | | 06/1999 |
| `swep1_150` | Pinball 2000: Star Wars Episode I (1.50) | 07/2000 |
| `swep1_140` | | 07/2000 |
| `swep1_130` | | 09/1999 |

Both boot, render, take coins, start a game, drive lamps and coils, respond to the flippers and
play sound.

## Where it comes from

The machine is a port of the MAME driver by **erikieNL** (`erikieNL/mame`, branch `pinball2k`, MAME
0.239), which builds on R. Belmont's skeleton and Ville Linde's `mediagx.c`. The x86 CPU, the
chipset devices and the handler logic are that driver's; what is new here is the plumbing that lets
MAME 0.239 code live inside a fork of MAME 0.76.

The DCS sound board is **not** ported - it is PinMAME's own Pin2K board in `src/wpc/wmssnd.c`
(`docs/pin2k_sound.md`), which this driver drives as its host.

Imported MAME files keep their original copyright and license headers (MAME is BSD-3-Clause) and
name the version they came from.

## A warning about speed

Pinball 2000 is a 233 MHz PC and this emulates the whole thing - protected mode, paging, the x87
FPU, the PCI bus, the display controller. Like MAME's driver, the CPU is clocked down to 20 MHz (for now),
and even so:

**On a low-power desktop (Pentium Silver J5040, Atom class) a running game reaches about 50 % of
real time.** On a current desktop or laptop core expect comfortably more than real time. The
workload is single-threaded, so only single-core performance matters.

Measured shares in a running game (Linux, and outdated): the MediaGX 44 %, the DCS sound board's ADSP-2100 38 %, the
mixer 9 %, the video renderer 9 %. Both figures predate the idle skips below; most of that sound
board share turned out to be the DSP waiting rather than working, so treat it as an upper bound.

Two switches change speed and both are already on - the idle skips in *Where the time goes*, worth
about 9 % together. Nothing else here has a fast setting yet.

Windows/Visual C++ performance measurement update: After reclocking the MediaGX and the PIT, and optimizing
the ADSP idle loops, the ADSP is now barely measurable anymore, so optimizing it may be void for now.
The major bottleneck at the moment is the MediaGX, with the majority of it being the i386 FETCH() routine.

## Building

CMake builds it by default; the unix makefile does not.

```sh
# Linux/X11, standalone
make -f makefile.unix P2K=1 -j$(nproc)

# ... plus the remote debugger and -headless (needed for scripted tests)
make -f makefile.unix P2K=1 REMOTE_DEBUG=1 -j$(nproc)

# libpinmame, for VPX and other frontends - Pinball 2000 is in by default
cmake -S . -B build && cmake --build build

# ... without it
cmake -S . -B build -DPINMAME_P2K=OFF && cmake --build build
```

The bring-up apparatus is a second switch, and off: the traces, watches, dumps and the stand-in
playfield described below, together with the fifty-odd `P2K_*` environment variables that drive
them. None of it is compiled into a normal build, and a variable left set in someone's
environment cannot reach it - `strings` on a stock binary finds no `P2K_` at all.

```sh
make -f makefile.unix P2K=1 P2K_DEBUG=1        # unix
cmake -S . -B build -DPINMAME_P2K_DEBUG=ON     # cmake
```

Visual Studio: add `P2K_DEBUG=1` to the project's preprocessor definitions. `src/p2k/p2k_debug.h`
is where this is documented in the source.

What is *not* behind it is anything the machine needs to run. The clock-interrupt gate and the DCS
idle skips are on unconditionally; they only lose the environment variables that used to override
them, which existed to A/B them without a rebuild.

Every CMake-driven build - VPinMAME, PinMAME, PinMAME32, libpinmame - has it on unless told
otherwise, and `vcproj\win_build.bat` passes that through:

```bat
rem VPinMAME64.dll without Pinball 2000
win_build PRJ:VPINMAME PLA:x64 GEN:11 CFG:RELEASE P2K:NO
```

The hand-kept Visual Studio projects take it through `vcproj/p2k.props`, imported by the
conversion scripts from VC2017 onwards - the same arrangement `asmjit.props` uses, and for the
same reason: only a **v141+ toolset** can compile C++17, so `create_vc2013`/`create_vc2015` leave
it out and the v110/v140 base projects are untouched.

```bat
create_vc2022_from_vc2012.bat   rem Pinball 2000 included
```

Requirements beyond PinMAME's own: a **C++17** compiler. No new libraries.

Three lists have to stay in step - `src/p2k/p2k.mak`, `cmake/p2k.cmake`, `vcproj/p2k.props`. A
source added to one belongs in all three.

Two things worth knowing:

* **Without `P2K=1` nothing here is built at all**, and `make p2kboot` then silently does nothing.
* The object directory is shared between configurations and make does not notice a changed define.
  After switching `REMOTE_DEBUG` on or off, delete `xpinmame.obj/`.

`make -f makefile.unix P2K=1 p2ktest` builds a standalone self-test of the CPU core and the
interrupt path; `p2kboot` is a bare harness that runs the firmware without PinMAME around it.

## Running it

### ROMs

A zip or folder named after the driver in the rompath. The driver
opens no files of its own and reads no environment variables to find them.

A set holds three things. The sound board's flash and its two sample chips; the MediaGX side's
eight Prism ROMs, `u100`-`u107`, which the loader interleaves in pairs into four 16 MB banks; and
the 8 MB update flash, which is what makes a version a version. The update flash is not a dump -
it is the four files of an official update package (`bootdata`, `im_flsh0`, `game`, `symbols`) laid
out back to back at the offsets the boot ROM looks for them at, and the machine does not boot
without it. Everything else is shared, so a clone's zip contains only its own four update files.

CMOS and the PLX EEPROM persist through PinMAME's normal NVRAM file. The update flash deliberately
does not: it belongs to the set, and an 8 MB NVRAM file would take precedence over it - a machine
would keep booting the version it was first started with even after selecting another driver.

#### The r2 boot ROMs are not a set

The Encore set also carries `rfm_u100r2.rom` / `rfm_u101r2.rom`, a second revision of Revenge From
Mars's bank-0 Prism pair. They are deliberately not declared, and that is not an oversight:

* **They are not a game version.** The version is the update flash, and none of the eight update
  packages is tied to a particular boot ROM - so nothing in the files says which of the eight an
  r2 set would be a clone of.
* Same 8 MB each, byte-identical to the stock pair from `0x17e9e0` upward and rewritten below it:
  about 15% of each chip, concentrated in the first 1.5 MB.
* **The driver would need work, not just a `ROM_START`.** It patches two bytes of bank 0 on the
  way in (`p2k_state::set_prism_roms`). `0x191` holds the same `retf` in both revisions, so that
  one carries over. `0x419a` does not: the stock pair has `b8 f9 ff ff ff`, the
  `mov eax,0FFFFFFF9h` whose immediate is forced to 1 to make a failing check report success,
  while r2 has `e8 9e 27 00 00` there - a `call`. Poking 1 into its displacement would corrupt the
  instruction. That check has to be found again in the new image before an r2 set can boot.

The hashes are in the ROM section of `src/wpc/p2k.c` so the files stay identifiable if someone
picks this up.

### Keys

| Key | |
|---|---|
| **5** / 6 / 3 / 4 | Coins |
| **1** | Start |
| **END** | Coin door - a toggle, and it starts *open* |
| Left / Right Shift | Flippers |
| Left / Right Ctrl | Left / Right Action Button |
| **Enter** | Launch Ball (Revenge From Mars; Episode I has a hand plunger and ignores it) |
| 7 / 8 / 9 / 0 | Enter / Up / Down / Escape - the service buttons behind the coin door |
| Insert / Home | Plumb bob tilt / slam tilt |

**Close the coin door first.** The machine starts with it open, says so on screen, and keeps high
voltage off until the door *closes* - the firmware raises the high-voltage relay on that edge and
there is no separate interlock input. Press END and the warning goes.

### Playing it standalone

A real Pinball 2000 gets its switch feedback from a playfield. Under VPinMAME/libPinMAME the table provides
that; standalone there is nothing, and a machine that sees an empty ball trough will not start a
game. The driver carries a small ball model for this (which should rather move to a simulator like all the other games). It needs a `P2K_DEBUG=1` build - see
[Building](#building) - and within one it is still off unless asked for:

```sh
P2K_TROUGH=4 P2K_BALLSAT=1800 P2K_DRAIN=100000 \
P2K_HANDPLUNGE=400 \
  ./xpinmame.x11 -rp ~/.xpinmame/roms swep1_150
```

| | |
|---|---|
| `P2K_TROUGH=<n>` | put n balls in the trough |
| `P2K_BALLSAT=<frame>` | when they appear. **Not at frame 0** - switches that change during power-up hang the machine |
| `P2K_DRAIN=<frames>` | how long a ball stays in play before the model drains it |
| `P2K_HANDPLUNGE=<frames>` | for a machine with no autoplunger coil - Episode I has none - let the ball leave the shooter lane by itself |
| `P2K_PLAYFIELD=1` | walk a ball over a fixed list of playfield switches, so the game scores on its own |

### The bring-up scaffolding

The ball model above is part of a larger block at the end of `SWITCH_UPDATE(p2k)` which is kept deliberately. It is what makes this machine testable
without a table:

* `P2K_PLAY="<frame>:<what>[:<hold>],…"` presses keys at given frames - `coin`, `start`, `enter`,
  `up`, `down`, `esc`, `lflip`, `rflip`, `laction`, `raction`, `launch`, or `sw<number>` for any
  playfield switch. The per-event hold matters: a coin held too long is not counted at all.
* `P2K_DOORCLOSE=<frame>` opens the coin door and closes it later, because it is the closing *edge*
  that brings up high voltage.
* the ball model and `P2K_PLAYFIELD=1`, which walks a ball over a fixed list of switches.

The block is compiled only with `P2K_DEBUG=1`, everything in it is behind those variables, it sits
behind a single early return, and a run that sets none of them never enters it. Frames are counted in `SWITCH_UPDATE` calls, so a scripted
run is reproducible to the frame regardless of how fast the host is - which is what makes it useful
as a regression test: every measurement in this port was taken this way, and a change that alters
behaviour shows up as a different frame number rather than as an impression.

### The switch matrix

The game numbers its switches `column * 10 + row`: columns 1-9 the playfield, 10 the coin door's
service buttons, 11 the cabinet. PinMAME numbers them sequentially, so the driver installs a
conversion (`MDRV_SWITCH_CONV`) and what you see in a debugger or a frontend is the game's own
numbering.

Coils follow the game's driver numbers, so PinMAME solenoid 1 is the machine's "Antr. 1". The
flipper coils are drivers 33-36 - right power, right hold, left power, left hold - which is exactly
the order PinMAME expects for its lower flipper solenoids, so nothing is remapped. The game data
declares `FLIP_SOL`, without which the core would overwrite those coils with its own copy of the
buttons once a frame.

Switch, lamp and coil names come out of each game's own tables; the extraction tool lives with the
port's working notes.

## How it works

```
   PinMAME                              |  src/p2k/  (this subsystem)
   -------------------------------------+------------------------------------------------
   src/wpc/p2k.c                        |
     machine driver, game data          |
     switch / lamp / coil mapping       |
     video renderer  -------------------+--> p2k_pinmame.cpp    the whole contract:
     NVRAM handler                      |      start/stop, nvram, bus, frame, pinball I/O
                                        |         |
   src/cpu/...  MEDIAGX entry  ---------+--> p2k_cpuintrf.cpp   PinMAME's CPU interface
                                        |         |
   src/wpc/wmssnd.c  <------------------+---  p2k_driver.cpp    the machine: bus decode, PCI,
     the DCS board                      |      |   MediaGX registers, flash, pinball board
                                        |    p2k_machine.cpp    device tree, timers
                                        |      |
                                        |    shim/     ~1.4k lines - the part of modern
                                        |      |       MAME's API the imported code uses
                                        |    mame/     ~28k lines imported unchanged:
                                        |              i386 core, softfloat, PIC, PIT, DMA,
                                        |              RTC, UART, 8042, PCI
```

**Why a shim.** PinMAME is a fork of MAME 0.76 (2003), which predates the current device model entirely -
no `device_t`, no `machine_config`, no `address_map`, no `required_device<>`. Rather than rewrite twenty thousand lines of CPU core into 2003-era C, the
imported files are taken **unchanged** and given the small part of the modern API they need:
address spaces, a device tree with timers and callbacks, and no-op stubs for save state and the
debugger. That is `shim/` - about 1400 lines against 28000 imported. The measured fact the approach
rests on is that MAME's i386 core needs almost nothing from MAME's core; the chipset devices are
what force the shim to grow.

**The bus.** The machine owns its own address spaces rather than going through PinMAME's memory
system. Plain-memory ranges - main RAM, the framebuffer and its `0xc0000000` alias - are served
straight out of the buffer by the address space, skipping an indirect call and a chain of range
compares. That is worth about 20 %; `P2K_FASTBUS=0` turns it off.

**Video.** No DMD: the picture is the MediaGX framebuffer, 640x240 RGB555, exported through
PinMAME's normal video path as a `CORE_VIDEO` layout with a renderer - the same shape `byvidpin.c`
uses for Baby Pac-Man - which is what feeds libPinMAME and any frontend behind it. The lines are
doubled to 640x480 because the display controller doubles them on the way to the CRT: this
machine's pixels are twice as tall as they are wide.

Doubling is a plain row copy - every pixel that reaches a frontend is one the machine produced -
and that is the default because it is what the hardware does. `P2K_LINE_INTERPOLATE` in
`src/wpc/p2k.c` switches it: set to 1, the second line of each pair becomes the average of the
machine's line above and the machine's line below. The machine's own lines stay exact either way,
so nothing blurs and nothing shifts by half a line; only the invented line between two of them is
softened, and the bottom line of the frame, which has nothing below it, is doubled as before. It
costs a second source line read and a 640-pixel average per output line, which is why it is off
unless asked for. Change it in the file, or define it from the build - it is wrapped in `#ifndef`,
so `-DP2K_LINE_INTERPOLATE=1` in a project's compile definitions is enough. Which line the
renderer used is in the `[p2k video]` line it prints for the first frame.

Video memory holds the picture upside down and the *right way round*; only the row order is turned
back. That is the opposite of what the cabinet's half-silvered mirror suggests, and it is what
makes the machine's own text legible.

Under VPinMAME a frontend reads that picture the same way it reads Baby Pac-Man's and Granny & the
Gators': `Controller.DmdWidth` / `DmdHeight` for the size and `Controller.updateDmdPixels` for the
pixels, which hands over the whole MAME screen as RGBA floats. Those two are video-display games
and this one is not, in exactly one respect: a `VIDEO_RGB_DIRECT` screen holds *packed colour*
where a paletted one holds an index, so `updateDmdPixels` unpacks by bitmap depth instead of going
through `palette_get_color()` - the same split `libpinmame.cpp`'s `UpdatePinmameDisplayBitmap()`
makes. Without it a 15 bpp pixel is read as a palette index up to 0x7fff and runs off the end of
the palette. `Controller.RawDmdPixels` and `RawDmdColoredPixels` are not involved: those are the
dot-matrix interface, capped at 256x64.

**Sound.** The Prism card's window at `0x13000000` is the host side of the DCS board - a command
port, a status register and an echo register. The driver forwards those and does nothing else; the
sound emulation is `wmssnd.c`'s. What lives here is the *host* half of the protocol, which is the
part that was missing: the stream is driven by the game code on the PC.

**Interrupts.** The game OS's clock handler services the pinball driver board one register at a
time and takes longer than a tick period, so a tick nests inside its own handler and the OS
complains. Suppressing the timer edge does not help - the request is already latched in the
interrupt controller. Holding the controller's line away from the CPU while the guest is inside its
clock handler does. `P2K_CLKINT_GATE=0` disables it.

## Where the time goes

Measured in a running game (Linux, and outdated):

| | |
|---|---|
| MediaGX | 44 % - interpreter ~19 %, address translation 7 %, instruction fetch 6 %, bus 2 % |
| DCS sound board (ADSP-2100) | 38 % |
| PinMAME's mixer and resampler | 9 % |
| Video renderer | 9 % |

The core retires 0.45 guest instructions per emulated cycle.

Measured and found not to help: `-O3 -march=native` gives nothing outside the noise.

### Two thirds of the sound board's time is a spin loop

Profiling the ADSP over 3 billion opcodes - boot, attract and a played game - puts **66.9 % of
everything that DSP executes** in two seven-instruction loops:

```
00bb  $80B5DA            00c5  $80B5DA
00bc  $227A0F            00c6  $227A0F
00bd  $1F7321            00c7  $1F7321
00be  $435E04            00c8  $435E04
00bf  $0D02A3  AR = I7   00c9  $0D02A3  AR = I7
00c0  $26E20F            00ca  $26E20F
00c1  JUMP 00bb          00cb  JUMP 00c5
```

143.3 million iterations each. Identical bodies, each ending in a conditional jump to its own head.
`AR = I7` reads the autobuffer index, so this is the DSP waiting for the DAC to consume samples -
not work, just waiting. The actual decode work is far down the profile: `00e0`-`00ed` at 354 k hits,
`008d`-`0095` at 17 M, `3a25` at 16 M.

That is what `DCS_useIdleSkip` exists for. The ADSP core has carried an idle-loop detector since the
WPC days - it spots `0d02a3` recurring at the same PC within ten instructions and calls
`cpu_spinuntil_int()` - but it was gated behind `DCS_useSpeedup` together with the WPC *decoder*
transcription, which cannot run here (see below). Splitting the two switches turns the idle skip on
for this board and leaves the decoder off. **Worth roughly +6 %** (129.2 -> 137.5 fps unthrottled
on the scripted workload above), and both games sound identical with it on and off. That figure is
one pair of runs against a run-to-run spread of about 3 %, so treat it as approximate - the latch
skip further down was measured properly and shows what that takes.

**The detector is not the limit, so do not spend time tuning it.** The obvious suspicion was that
its `lastpc` bookkeeping, written for a board with one idle loop, thrashes on this one's two.
Counting says otherwise, over a billion opcodes with the skip active:

```
head=391470  spin=173229 (44.3% of heads)  rearm=218233  thrash=45003  tooFar=8
```

* It suspends on 44.3 % of the times it reaches a loop head, against a **design ceiling of 50 %**:
  after suspending it clears `lastpc` ("amnesia"), so the next arrival can only re-arm.
* The two-loop thrashing is real but small - 20.6 % of re-arms are at the other head. Removing it
  entirely would move firing from 44.3 % to at most 50 %, on a mechanism that has already cut loop
  head executions by **244x** (95.5 M to 391 k per billion opcodes). That is rounding error.
* `tooFar` is 8. The ten-instruction window never binds.

So the gap between 66.9 % of opcodes and a few percent of wall clock is not a missed-detection
problem. What is left,
untested: those were unusually cheap instructions - a seven-instruction register-only loop is an
interpreter's best case, no memory handlers, one perfectly predicted branch, everything in cache -
so removing two billion of *them* is not worth removing two billion average ones. And the 38 % DSP
share at the top of this section was measured on a different machine under Linux; it need not hold
where this was measured. `cpu_spinuntil_int()` overhead is the third candidate and the weakest:
173 k suspends per billion opcodes is not many scheduler round trips.

Whatever the split, that is what the autobuffer skip alone is worth. The sound board's remaining
cost turned out to be more waiting, one layer down.

### And most of the rest is waiting for the host

With the spin above suppressed, a second profile - hottest PCs, disassembled - is just as lopsided:
a thirteen-instruction loop at program address `0x3d9c` is **56.8 %** of what the DSP executes, and
a second at `0x3dbe` follows it. Neither is audio work:

```
3d9c  I5 = $3FED
3d9d  AR = PM(I5,M4)        ; a flag an interrupt handler may set
3d9e  AR = AR
3d9f  IF NE JUMP $3DAC
3da0  I7 = $3FFB
3da1  AY0 = PM(I7,M4)
3da2  AR = AY0 + 1          ; bump a timeout counter
3da3  PM(I7,M4) = AR
3da4  IF AV CALL $3D65      ; on overflow, toggle a bit the host never reads
3da5  AX0 = DM($0403)       ; <- p2k_latch_status_r
3da6  AY0 = $0080           ;    bit 7: has the host sent a command?
3da7  AR = AX0 AND AY0
3da8  IF EQ JUMP $3D9C
```

No multiply, no transform - the DSP polling the host command latch. So there is no decode loop here
to speed up, and `p2k_latch_status_r` skips this the same way, on the register the loop reads rather
than by watching opcodes. `P2K_NOLATCHSKIP=1` turns it off.

It is worth **+1.8 %** - three alternating pairs of 4000-frame runs, +0.9/+2.5/+1.9, consistently
positive but well inside the 3 % run-to-run spread, so a single A/B of it means nothing in either
direction. Do not be tempted by a bigger number: an early build measured +8.9 %, and it was faster
only because it had hung the sound board (see below).

Which makes three attempts in a row where **opcode share badly overpredicted wall-clock share**:
66.9 % of opcodes bought 7.5 %, another 56.8 % bought 1.8 %. These loops are the cheapest
instructions in the machine - a few register operations and one polled read, all in cache - so
removing them is worth far less per opcode than removing average work. Anyone profiling this board
should measure wall clock before believing a histogram.

The comment on that handler lists every transition of the two polled bits and which of them wakes a
suspended DSP. That list is the whole safety argument and it is worth reading before touching any of
it: the first attempt covered the command path, which already woke by asserting IRQ2, and missed
`dcs_p2k_data_r()` - the host taking a reply, which asserts nothing. The DSP suspended there during
self-test with no audio running to interrupt it, and the board went silent: no start-up bong, then a
DCS error from the game. **The frame rate was fine throughout.** Nothing about a speed measurement
can see a sound board that has stopped answering, so changes in here get judged by listening.


## Possible extensions

### Sound and CPU on separate cores

The biggest single lever on paper - 44/56 should overlap to about 1.5x - and it was built,
measured and taken back out again. What is worth knowing before trying again:

* **Only the MediaGX can move.** PinMAME's memory system is a set of globals (`readmem_lookup`,
  `OP_RAM`, `cpu_bankbase`, …) that `memory_set_context()` swaps on every CPU switch, so no PinMAME
  CPU can run concurrently with another. The MediaGX is the one CPU here that touches none of them,
  because this subsystem owns its memory - which is also why routing this machine's memory through
  PinMAME's system would close that door.
* **The crossing is small.** The subsystem calls out to exactly two functions, the sound board's
  window. Over a boot and a game: 128036 status writes, 126524 status reads, 956 data writes and
  **189 data reads**. Only the last needs a handshake, because reading the reply register moves the
  board's state; everything else the worker can answer itself.
* **It works and it stays deterministic** - a pipeline one slice deep, counted in emulated cycles
  rather than wall clock, gives reproducible runs.
* **It bought +8 %, not 1.5x.** The parallelism is real (1.28x) but some 18 points of extra work
  appear, almost all on the worker. Ruled out by measurement: the hardware (two independent
  emulator processes lose only 8 %), thread pinning (slightly worse), the synchronisation points
  (0.1 %), the write queue (0.0 %). What is left is the handover itself - about 4000 per second,
  each waking a thread that then runs with cold caches. That fits the size of the loss but is not
  proven; `perf` was unavailable on the development machine, and a two-thread profile is the first
  thing a second attempt should get.
* **One trap worth repeating**: reporting the *previous* slice's cycle count to PinMAME looks
  harmless, but it leaves the CPU's local time one slice behind the scheduler's target and
  `cpu_timeslice()` then splits every timer interval in two - 6235 slices per emulated second
  instead of 3997.

### The WPC decoder speedup does not port

`dcs_speedup()` in `wmssnd.c` is a hand-written C copy of the audio decode loop in the WPC-era ADSP
ROMs, and it is tempting to point at this board too. It does not fit, for two measured reasons.

It is entered by sniffing the opcode sequence `NOP; DIS; DIS`, and **that sequence does not occur
here**: zero occurrences in 5 billion opcodes across boot, attract and a played game. And it is
welded to one ROM's memory map - hardcoded addresses (`0x2B44`, `0x15EB`, `0x1000/0x2000` against
`0x0700/0x3800`) and fourteen raw pointer accesses into flat data RAM, where this board banks
`0x0000`-`0x37ff` through the SDRC at run time and has to go through handlers.

Even if it did port, the profile above says it would buy little: the decode work is a few percent.
The idle skip is where the sound board's time actually is. `DCS_useSpeedup` stays 0 here.

### A C translation of *this* board's DSP code might still be worth it

What does not follow from the above is that the idea is dead - only that the WPC transcription is
the wrong thing to copy. A hand-written C version of Pinball 2000's own hot routines is still on
the table. The one profile that looked past the autobuffer spin was encouraging about the shape -
top 30 program counters covered 90 % of opcodes, only 2244 distinct addresses ran at all - which
is the sort of concentration that makes a transcription feasible rather than a rewrite of a whole
firmware.

**Measure before writing any of it - and the measurement that exists is the wrong kind.**
Everything known about this board's cost is *opcode counts*, and on this emulator an opcode count
is not a proxy for time at all. The instructions differ far too much: a register-only operation is
a few host instructions in the interpreter's switch, while one that reaches memory goes through
`p2k_data_r`/`p2k_data_hi_r` and the SDRC bank decode - easily an order of magnitude apart, and
this board takes that path far more often than the WPC ones do because its data space is banked at
run time.

The counting has already misled three times, always the same way: 66.9 % of opcodes bought 7.5 %,
another 56.8 % bought 1.8 %. Both were spin loops - the cheapest instructions in the machine, so
removing huge numbers of them bought little. Real decode work should be dearer per opcode, perhaps
much dearer, but *how much* is unknown and nothing in this file measures it. What is needed
first:

* **A general profile of the whole emulator**, in host time rather than opcodes - Visual Studio's
  CPU Usage profiler on a running game is the obvious way, and needs no code changes. `perf` does
  the same on Linux. That is the measurement nobody has taken, and it answers the first question:
  what fraction of a frame is the sound board at all, now that both idle skips are in?
* **What the sound board now costs in wall clock**, specifically. The 38 % at the top of this file
  is a pre-skip figure from a different machine under Linux, so it is an upper bound at best. If a
  cheaper route is wanted than an external profiler, `cpuexec.c` already brackets every CPU's
  execute with `profiler_mark(PROFILER_CPU1 + cpunum)`; those marks compile away unless
  `MAME_DEBUG` is defined, and enabling just them gives per-CPU shares without the rest of the
  debugger.
* **Where inside that the time goes** - interpreter dispatch, or the `p2k_data_r`/`p2k_data_hi_r`
  handler calls on every banked access. If it is the handlers, caching the resolved bank pointer
  is a fraction of the work of a transcription and cannot damage the audio.
* **A fresh profile.** The one described above was taken with only the autobuffer skip active, and
  more than half of what it measured was the host-latch poll that the second skip has since
  removed. What the DSP now spends its time on has not been looked at, so even the concentration
  claimed above is untested for the current build.
* **Whether the hot routine is stable in memory.** The firmware overlays code into SRAM: in that
  same profile, addresses around `0x3a25` showed hit counts wildly inconsistent with their
  neighbours, meaning different routines occupied the same addresses during the run. A
  transcription keyed to an address needs that ruled out first.

`src/cpu/adsp2100/2100dasm.c` disassembles the ADSP fully and is what to read the code with -
note that the public `adsp2104_dasm()` is a `MAME_DEBUG`-gated stub that only prints the raw
opcode, and that `dasm2100()` reads through `cpu_readop32(pc << 2)` without the
`ADSP2100_PGM_OFFSET` the CPU applies, so both need care. Dumping program memory and
disassembling it offline avoids both traps.

### A dynarec

The other half. PinMAME already ships `ext/asmjit`. With the MediaGX at 44 % of the work (outdated), Amdahl
caps a perfect one at about 1.8x, so on slow hardware it reaches real time only together with the
threading or ADSP optimizations above.

### Memory through PinMAME's system

This machine's memory lives in the subsystem, which is fast and costs debuggability: PinMAME's
`/api/debugger/memory` goes through `cpunum_read_byte()` and answers for a different machine. Two
steps, cheapest first - route the debugger's memory endpoint through this bus (small, and it solves
the actual pain), or declare a real memory map and have the subsystem's address spaces delegate to
it. The second has a run-time cost; measure before keeping it. Note that PinMAME 0.76's memory
system has no notion of the PCI-relocatable windows the MediaGX moves at run time.

### Smaller items

Caching instruction fetch - `FETCH()` translates the address per byte - and the per-slice overhead
of the 2000-cycle execution chunks.

## Known limitations

* Episode I plays its music but its sound *tracks* stay silent. The read path, the SDRC paging and
  the PCM producer are all proven correct on demand, so what the game's own track numbers resolve
  to is a question of game state rather than of emulation.
* All platforms are built *and run*: both games boot and render under
  the `PinMAME` executable built from CMake, vcproj files and lists them.
* The picture VPinMAME hands a frontend is 641 pixels wide for a 640-pixel screen. That extra
  column is `core_findSize()`'s `+ 1` and every `CORE_VIDEO` game has it - Baby Pac-Man and Granny
  report 257 for a 256-pixel screen. It is left alone deliberately: changing it would move those
  games' output too.
* `-frames_to_run` does not work headless - the counter it watches sits inside the display update,
  past the point where a headless run returns. The remote debugger's
  `/api/debugger/control?cmd=exit` ends a run cleanly instead, which is also what gets NVRAM
  written.

## Diagnostics

All of this needs a `P2K_DEBUG=1` build - see [Building](#building) - and within one is off unless
asked for. The ones that stay useful:

| | |
|---|---|
| `P2K_PROGRESS=<cycles>` | the firmware's serial console, plus cycles, instructions, time slices and registers |
| `P2K_PCTRAP=<hex>[=label],…` | report when the CPU reaches an address, with registers |
| `P2K_MEMWATCH=<from>[-<to>]` | writes to a range with the PC that made them; `P2K_MEMWATCH_CHANGED=1` for changes only |
| `P2K_READWATCH`, `P2K_DUMP`, `P2K_WATCH`, `P2K_BACKTRACE` | reads, memory dumps, per-instruction registers, a PC ring buffer |
| `P2K_DISPWATCH=1` | every change to a display controller register |
| `P2K_SOLWATCH=1` | every change of the coil register that carries the flippers |
| `P2K_DCSLOG=1` | the whole conversation with the sound board |
| `P2K_VIDEO_PPM=<path>` | write the picture out as a PPM |

The bring-up log - every measurement taken to get here, and every hypothesis that turned out wrong -
is kept with the port's working notes rather than in this tree. It records how the emulation was
arrived at, not what it does.
