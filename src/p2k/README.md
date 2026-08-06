# Pinball 2000

Midway's last pinball platform, and the only one in PinMAME that is a PC: a Cyrix MediaGX with a
CX5520 southbridge, a PC97317 Super-I/O and a "Prism" PCI card carrying the ADSP-2104 sound
hardware and the link to the pinball power driver board.

| Driver | Game |
|---|---|
| `rfmpb` | Pinball 2000: Revenge From Mars (1999) |
| `swe1pb` | Pinball 2000: Star Wars Episode I (1999) |

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
FPU, the PCI bus, the display controller. Like MAME's driver, the CPU is clocked down to 20 MHz,
and even so:

**On a low-power desktop (Pentium Silver J5040, Atom class) a running game reaches about 50 % of
real time.** On a current desktop or laptop core expect comfortably more than real time. The
workload is single-threaded, so only single-core performance matters.

Measured shares in a running game: the MediaGX 44 %, the DCS sound board's ADSP-2100 38 %, the
mixer 9 %, the video renderer 9 %. The sound processor is not overhead - it runs at 16 MHz against
the main CPU's downclocked 20 MHz and retires more instructions than it does.

There is no switch that makes it faster. *Where the time goes* below says what would.

## Building

The subsystem is optional and off by default.

```sh
# Linux/X11, standalone
make -f makefile.unix P2K=1 -j$(nproc)

# ... plus the remote debugger and -headless (needed for scripted tests)
make -f makefile.unix P2K=1 REMOTE_DEBUG=1 -j$(nproc)

# libpinmame, for VPX and other frontends
cmake -S . -B build -DPINMAME_P2K=ON && cmake --build build
```

Requirements beyond PinMAME's own: a **C++17** compiler. No new libraries.

Two things worth knowing:

* **Without `P2K=1` nothing here is built at all**, and `make p2kboot` then silently does nothing.
* The object directory is shared between configurations and make does not notice a changed define.
  After switching `REMOTE_DEBUG` on or off, delete `xpinmame.obj/`.

`make -f makefile.unix P2K=1 p2ktest` builds a standalone self-test of the CPU core and the
interrupt path; `p2kboot` is a bare harness that runs the firmware without PinMAME around it.

## Running it

### ROMs

Each game needs its boot ROM set under PinMAME's rompath (`rfmpb/`, `swe1pb/`) plus two things this
driver takes from outside:

```sh
P2K_ROMS=/path/to/roms                 # the MediaGX ROM images
P2K_NVRAM_UPDATES=/path/to/image.bin   # the 8 MB update flash image
```

The update flash is not a dump - it is assembled from an official update package (`bootdata`,
`im_flsh0`, `game`, `symbols`, written back to back). The machine does not boot without it.

CMOS and the PLX EEPROM persist through PinMAME's normal NVRAM file. The update flash deliberately
does not, because it comes in from outside.

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

A real Pinball 2000 gets its switch feedback from a playfield. Under VPinMAME the table provides
that; standalone there is nothing, and a machine that sees an empty ball trough will not start a
game. The driver carries a small ball model for this, off unless asked for:

```sh
P2K_TROUGH=4 P2K_BALLSAT=1800 P2K_DRAIN=100000 \
P2K_HANDPLUNGE=400 \
  ./xpinmame.x11 -rp ~/.xpinmame/roms swe1pb
```

| | |
|---|---|
| `P2K_TROUGH=<n>` | put n balls in the trough |
| `P2K_BALLSAT=<frame>` | when they appear. **Not at frame 0** - switches that change during power-up hang the machine |
| `P2K_DRAIN=<frames>` | how long a ball stays in play before the model drains it |
| `P2K_HANDPLUNGE=<frames>` | for a machine with no autoplunger coil - Episode I has none - let the ball leave the shooter lane by itself |
| `P2K_PLAYFIELD=1` | walk a ball over a fixed list of playfield switches, so the game scores on its own |

### The bring-up scaffolding

The ball model above is part of a larger block at the end of `SWITCH_UPDATE(p2k)` - about 340 of
`src/wpc/p2k.c`'s 850 lines - which is kept deliberately. It is what makes this machine testable
without a table:

* `P2K_PLAY="<frame>:<what>[:<hold>],…"` presses keys at given frames - `coin`, `start`, `enter`,
  `up`, `down`, `esc`, `lflip`, `rflip`, `laction`, `raction`, `launch`, or `sw<number>` for any
  playfield switch. The per-event hold matters: a coin held too long is not counted at all.
* `P2K_DOORCLOSE=<frame>` opens the coin door and closes it later, because it is the closing *edge*
  that brings up high voltage.
* the ball model and `P2K_PLAYFIELD=1`, which walks a ball over a fixed list of switches.

Everything is behind those variables, the whole block sits behind a single early return, and a run
that sets none of them never enters it. Frames are counted in `SWITCH_UPDATE` calls, so a scripted
run is reproducible to the frame regardless of how fast the host is - which is what makes it useful
as a regression test: every measurement in this port was taken this way, and a change that alters
behaviour shows up as a different frame number rather than as an impression.

If you are integrating this driver into a frontend, this is also the quickest way to see what the
machine does with switch feedback before your own feedback exists.

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

**Why a shim.** PinMAME is a fork of MAME 0.76 (2003), which predates the device model entirely -
no `device_t`, no `machine_config`, no `address_map`, no `required_device<>`. The Pinball 2000
driver uses all of it. Rather than rewrite twenty thousand lines of CPU core into 2003-era C, the
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
uses for Baby Pac-Man - which is what feeds libpinmame and any frontend behind it. The lines are
doubled to 640x480 because the display controller doubles them on the way to the CRT: this
machine's pixels are twice as tall as they are wide. MAME's driver does not model that, so its
screenshots of this platform have the wrong aspect ratio.

Video memory holds the picture upside down and the *right way round*; only the row order is turned
back. That is the opposite of what the cabinet's half-silvered mirror suggests, and it is what
makes the machine's own text legible.

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

Measured in a running game:

| | |
|---|---|
| MediaGX | 44 % - interpreter ~19 %, address translation 7 %, instruction fetch 6 %, bus 2 % |
| DCS sound board (ADSP-2100) | 38 % |
| PinMAME's mixer and resampler | 9 % |
| Video renderer | 9 % |

The core retires 0.45 guest instructions per emulated cycle.

Measured and found not to help: `-O3 -march=native` gives nothing outside the noise. And `-nosound`
is not a valid comparison - the firmware needs the sound board and dies into its monitor without it.

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

### A dynarec

The other half. PinMAME already ships `ext/asmjit`. With the MediaGX at 44 % of the work, Amdahl
caps a perfect one at about 1.8x, so on slow hardware it reaches real time only together with the
threading above.

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
* Revenge From Mars needs sample chip dumps that match its sound flash. The widely circulating
  `rfm_u109`/`rfm_u110` images do not; Episode I's do. That is a ROM-set question, not an emulation
  one.
* Only Linux/X11 and libpinmame are built and tested. macOS and Windows should be mechanical - the
  subsystem is portable C++ with no assembly - but are unverified. **VPinMAME/COM is not wired up**:
  its path to a frontend is built around DMD formats, and a 640x480 true-colour picture does not fit
  through it without extending that interface. That is the open design question on the Windows side.
* `-frames_to_run` does not work headless - the counter it watches sits inside the display update,
  past the point where a headless run returns. The remote debugger's
  `/api/debugger/control?cmd=exit` ends a run cleanly instead, which is also what gets NVRAM
  written.

## Diagnostics

All off unless asked for. The ones that stay useful:

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
