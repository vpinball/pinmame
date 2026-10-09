# Stern Spike 1

Stern's first SPIKE platform (2015 on: Game of Thrones, KISS, WrestleMania, Ghostbusters, ...). The
game runs as a statically linked ARMv5TE Linux program on an ARM926-class CPU board and talks to
the playfield's node boards over a serial bus; the display is a 128x32 DMD with 16 shades.

**Status: bring-up.** The subsystem runs as a PinMAME driver (`src/wpc/spike1.c`, sets
`gbust_117h` Ghostbusters LE 1.17.0, `wnbjm_155` Whoa Nellie! Big Juicy Melons 1.55.0, `primus_103`
Primus 1.03.0, `pabst_101` Pabst Can Crusher 1.01.0 and `got_137h` Game of Thrones LE 1.37.0) and
standalone (`spike1boot`, below).

## How it works

There is no board to emulate below the game program: the subsystem emulates Linux at the
system-call level, the way the program sees it.

- `mame/cpu/arm7/` - MAME's ARM7/ARM9 core, imported. The game runs on the ARM9 (v5TE) variant in
  user mode with the MMU off.
- `spike1_cpu.*` - the CPU device: user-mode register sets, so several guest threads can share it,
  and SWI delivered to the host (see `set_swi_handler()` in `arm7.h`).
- `spike1_memory.*` - the guest's flat address space: the program image, brk heap and mmap area in
  one low block, the main stack, the `0xffff0000` kernel helper page, and a read-only view of the
  asset image (over 1 GB), mapped from the host's file or memory instead of copied. Each block is
  one fast window of the shim's `address_space`.
- `spike1_vfs.*` - the filesystem the game sees: the title's files, read-only (a host directory
  with the machine's extracted root, or files the host holds in memory - PinMAME's ROM regions),
  under an in-memory layer with everything the game writes. The board's `/data` and `/tmp` mounts
  start empty; the game keeps its NVRAM files under `/data/nv`.
- `spike1_linux.*` - the program loader (ELF, initial stack with argv/envp/auxv) and the system
  calls: files, memory, time, futexes, pipes, poll/select, and the game's threads. `nvram()` gives
  everything the machine keeps across power cycles - the written files and the board's EEPROMs -
  as one block for the host's NVRAM file; the next start takes it back in its config. Threads
  are scheduled round robin on the one CPU, in quanta; when every thread is blocked, time skips
  ahead to the next wake-up without running instructions. Time is emulated time - CPU cycles at
  the configured clock.
- `spike1_pinmame.cpp`, `spike1_public.h` - the plain C contract with PinMAME's half,
  `src/wpc/spike1.c`; the subsystem includes no PinMAME header.
- `spike1_boot.cpp` - the standalone harness.

The kernel helpers at `0xffff0fa0..0xffff0fff` are real ARM code (the pre-v6 sequences from the
kernel's `kernel_user_helpers.rst`), so no per-instruction hook is needed; `__kuser_get_tls` reads a
word the thread switch keeps current. A thread that loses the CPU between `__kuser_cmpxchg`'s load
and its store restarts the helper, as the kernel does on pre-v6 CPUs, so the helper stays atomic.

## The MAME import

From MAME 0.288 (tag `mame0288`, commit `27a8d9e`), `src/devices/cpu/arm7/`: `arm7.cpp`, `arm7.h`,
`arm7core.h`, `arm7core.hxx`, `arm7help.h`, `arm7ops.cpp`, `arm7thmb.cpp`, `arm7dasm.cpp`,
`arm7dasm.h`, `cecalls.hxx`, all `license:BSD-3-Clause`. Changes are marked `// PINMAME`:

| Where | Change |
|---|---|
| `arm7.h`, `arm7ops.cpp` `arm7ops_f`, `arm7thmb.cpp` `tg0d_f` | `set_swi_handler()`: SWI goes to the host instead of the exception vector |
| `arm7.cpp`, `arm7core.h` | `ARM7_PINMAME_DIRECT_FETCH` (default on): instructions are fetched straight from the address space, not through the modelled prefetch queue - nothing to model with the MMU off and no self-modifying code, and measured ~28% faster |
| `arm7.cpp` | no debugger console commands |
| `arm7ops.cpp` BLX (register) | **bug fix**: read the target before writing the link register. `blx lr`, which glibc's `clone()` uses to call a new thread's start routine, branched to its own return address (the Thumb form was already right) |

The core uses the MAME compatibility layer of the Pinball 2000 subsystem (`src/p2k/shim`), with a
few additions for modern MAME's arm7: `rotr_32`, `population_count_32`, `mul_32x32_shift`,
`util::sext`, the `TR_*` translation intents, `address_map_constructor`, the four-argument
`memory_translate()`, `address_space::read_ptr()`/`endianness()`, `total_cycles()` and
`currently_executing()`.

Speed, measured with real game functions on an i7-13700KF: about 93 M instructions/s as imported,
about 118 M/s with the direct fetch. A Spike 1 game needs about 36 M/s in attract mode and 50-61
M/s in play once the line-sense thread waits for its ADC samples as it does on the real board,
and the sound thread for its DAC.

## Building and running

```sh
cmake -S cmake/spike1boot -B build/spike1boot -DCMAKE_BUILD_TYPE=Release
cmake --build build/spike1boot
build/spike1boot/spike1boot --root <extracted root> --game /games/<name>/game --state <dir> --seconds 30 [--trace]
build/spike1boot/spike1boot --files <the game's folder> --game /games/<name>/game --state <dir> --seconds 30
```

Harness options: `--dmd <s>` prints the DMD every s seconds, `--list-switches` the switch, coil and LED maps,
`--switch <s>:<name>[:<ms>]` toggles a switch by name for a while (e.g. `--switch "20:LEFT COIN:120"`),
`--outputs` prints coil changes as they happen and the lit LEDs at the end, `--wav <file>` records
the sound, `--insert <s>` saves
the LCD insert every s seconds as a PPM file in the state directory, `--node-dump <file>`
writes every node-bus frame (except switch and status reads) to a file, `--peek <address>:<bytes>`
prints guest memory at the end, `--stepper-home <node>:<stepper>:<switch>` links a stepper to a
home switch on another board as a PinMAME set's `SPIKE1_HOMES` does (Game of Thrones: `10:0:88`
and `12:0:121`), `--slice <ms>` sets the emulated time per `run()` call (1 by
default; PinMAME uses one frame, 16.7). With `SPIKE1_NODE_STATS` set in the environment, it
prints the node-bus frame count per command and board.

`--root` is the machine's root filesystem extracted from its image (with `games/<name>/game` and
`image.bin` in it). `--files` instead loads the game's folder (`games/<name>`: the program,
`image.bin`, `lcdinsert.bin`, the node firmware) into memory, as PinMAME will hold it; the game
needs nothing else from the root. The state directory keeps the machine's NVRAM between runs as
`spike1.nv`, and the `--insert` pictures. `--trace` logs every system call; without it, only noteworthy ones (device
traffic, files not found, threads) are logged. From Git Bash, set `MSYS_NO_PATHCONV=1`, or it turns
`/games/...` into a Windows path.

## The devices (`spike1_devices.*`)

Each one answers the Linux driver interface the game uses, worked out from the game's own driver
code (its symbols are in the program) rather than from any other emulator:

- **I2C** (`/dev/i2c-0`, `I2C_RDWR` and plain read/write): 24xx EEPROMs at 0x50 and 0x51, blank on
  a new board - the game formats its NVRAM store itself - and kept in the NVRAM block; the
  amplifier's MCP4631 volume potentiometers at 0x28-0x2a.
- **AC line sense** (`/dev/adc`): 3600 samples/s, a 60 Hz valley-shaped waveform that the game's
  `LineSenseThread` measures as 60 Hz and accepts as a real line; reads wait for the samples' time.
- **DMD** (`/dev/spi0`): 2048-byte frames of four bitplanes, plane n weighing 2^n, decoded to 16
  shades; writes wait for the panel's 15.75 ms PWM pattern.
- **Node bus** (`/dev/ttyS4`): the game's own `NODEBUS_TransferMessage` framing. The board list comes
  from the title's `node_board_table`; each board's identity (firmware version, NXP part ID,
  checksum) from the firmware image the game itself loaded for it, so validation passes with no
  forcing. Modelled: poll, bridge version/status/state, GetVersion, GetStatus (frame counts),
  GetBootStatus, GetFullBoardID, GetChecksum, GetInputState, LCD insert info, and the outputs
  below. Other queries are answered with zeros and logged once; other commands are logged once.
  A board counts time in ticks at the rate its GetVersion reports (1000 Hz here); the game
  converts coil and motor times from ms with it.
- **Switches**: the title's own tables (`node_board_table`, `node_board_device_table`,
  `switch_table`, `switch_dedicated_table`) give each switch's board, position, number, name and
  polarity. A board's eight GetInputState bytes and the CPU board's 3-byte `/dev/spi1` transfer
  carry position p in byte p/8, bit p%8, active low unless the switch table marks it active high.
  At rest: a trough opto per installed ball (`hook_balls_installed_in_game`), the firmware's other
  closed-at-rest marks, and the coin door's power sense.
- **Numbers** are the factory manual's, which the title's own tables hold (`apply_manual_numbers()`):
  `node_board_device_sw_table` gives the Switch Reference numbers, `node_board_device_cl_table_data`
  the Driver Reference numbers of the coils, `node_board_device_led_table` the Light Reference
  numbers and class bits (lamp, GI string, flasher, motor drive, cabinet) of the LED channels. The
  manual numbers the CPU board's own switches C1-C16 apart; they are 101-116 here. An optional
  topper's board - one the game names `TOPPER...` in `lang_text_nb_<node>` - numbers its switches
  from 1 again (Game of Thrones' TOPPER DRAGON is 1, as is its LEFT RETURN LANE); they are 121 and
  up here.
- **Coils** (`node_board_device_table` type 2): a fire command gives a
  first power and time, then a second; a reflex configuration makes the board fire the coil on its
  own when a switch it watches closes - flippers pulse at full power, then hold at the second power
  while the button stays closed (the end-of-stroke switch ends the pulse early); pops and slings
  pulse once per closure and wait out a hold-off time. `coil_level()` gives a driver's PWM duty
  (0-255) at a given time; the coil mask (a set bit switches a driver off) applies.
- **LEDs** (type 4, up to 96 channels a board, three for an RGB LED): `NODEBUS_SetLEDMultiple2`'s
  packed updates (command 0x80-0xBF: an index, a list or a bitmap of channels, then levels and fade
  times in compact forms; see `led_update()`). Every update in a full attract-and-play run decodes to
  exactly its length. `led_level()` gives a channel's level (0-255); the LED mask applies. A
  title without `NODEBUS_SetLEDMultiple2` (Whoa Nellie, Primus, Pabst Can Crusher, Game of
  Thrones) sends runs of channels instead, on the same command bytes - `0x80 | first`, then a fade
  time and a level per channel, or 0xff and a time and level per channel (`led_update_run()`) -
  and Game of Thrones also command 0xC0 for channels from 64 on (its start-up test sweeps all 96 a
  board drives): a 16-bit first channel, then a run in the same two forms. The model picks the
  packed form by that symbol.
- **Motors**: home, go, stop and status, the move taking emulated time, so the game's encoder-motor
  code (Slimer) finds the motor homed and ready instead of re-configuring it in a loop.
- **Steppers** (Whoa Nellie's score and credit reels, Game of Thrones' dragons,
  `NODEBUS_Stepper*`): configure (0x32: steps a turn, home switch input), go (0x31: a target,
  forward or with bit 15 backward, at a time per step; a target past the turn goes once round),
  home (0x34) and status (0x38 + stepper: step, target, bit 1 moving, bit 2 in place). A home
  switch is closed while its stepper stands within two steps of step 0. Game of Thrones' dragons
  configure no home input: the game watches a switch on another board itself (DRAGON HOME, 88, on
  node 11 for the dragon on node 10), homes by going once round until it closes, then turns the
  wings on through it. Each time the dragon passes the switch during a move, the game sends 0x34
  with the stepper and a clear flag and keeps its own idea of the position, so the model reads it
  as "count from home again" and leaves the stepper as it is (older titles put the stepper where
  the reply length goes, and the stepper seeks step 0). The host links such a switch to its
  stepper (`link_stepper_home()`).
- **Board blocks**: the game keeps a block a board; `sys_node_board_get_next_block_ptr` gives its
  base and size, which differ between SDK versions (a loaded constant in Ghostbusters, a computed
  one in Whoa Nellie) - the model reads both forms, as the firmware image a board must report
  hangs off its block.
- **Sound** (`/dev/i2s`): the game's DAC handler writes 200 stereo frames of 16-bit samples at a
  time, at the rate it sets with a code from its own table (44.1 kHz). A write waits while the DAC
  still has more than the driver's buffer (2940 bytes) to play, so the sound thread runs at the
  DAC's pace instead of spinning; `audio_take()` hands the samples to a host. The volume is the
  amplifier's: the MCP4631 at 0x28 divides the left and right outputs by its two wipers, which the
  game sets from its volume (0-63) through a table, so the volume buttons and the volume
  adjustment work. Ghostbusters' attract mode is silent; coins and the game start play their
  sounds and music.
- **LCD insert** (Ghostbusters' Ecto goggles): a node board with a 160x128 colour display and the
  title's `lcdinsert.bin` in its own flash (a 32-byte header, then BGR565 frames stored with rows
  and columns swapped). The game shows one frame, or runs a range of frames at a period in 1/1280 s
  (the game's table: 84 for 15 fps), and sets the backlight; `insert_pixels()` gives the picture
  the player sees. Ghostbusters runs the Stern logo (frames 129-173) in attract mode and shows its
  own logo (frame 174) once credits are in.

## In PinMAME (`src/wpc/spike1.c`)

- **The set** is the title's game folder from Stern's update image - the program, `image.bin`,
  `lcdinsert.bin` if it has one, the node firmware - loaded one after another into one ROM region
  and audited like any other set's ROMs. The region is over 1 GB, so the games need a 64-bit
  build. `MACHINE_INIT` hands the files to the subsystem, which runs the program as
  `/games/<folder>/game`; `image.bin` is mapped from the region, not copied.
  `scripts/spike1/Build-Spike1RomSet.ps1` (or `Build ROM set.cmd` beside it, which opens a window)
  makes the set from Stern's SD-card image, `<title>-<version>.iso.zip`: it finds the game folder
  on the image's ext3 partitions, reads it straight out of the zip in a few forward passes, writes
  `<set>.zip` stored, and checks every file against the set's CRCs - a new set needs its files
  added to the script's `KnownSet` table. Stern's `.spk` update packages are not read yet.
- **The CPU** (`CPU_SPIKE1`, `src/cpuintrf.c`) runs the machine for the cycles PinMAME gives it,
  at 400 MHz; once a frame the driver passes switch changes in and takes coils, LEDs and the DMD out.
- **Numbers** are the factory manual's: switches by their Switch Reference numbers (the flipper
  buttons and EOS switches in PinMAME's flipper column - each title's numbers, with its coin, tilt
  and slam switches, are in its `SPIKE1_SWITCHES`, or `SPIKE1_SWITCHES_UPPER` with the upper
  flippers' buttons, which the flipper keys close with the lower ones; the CPU board's C1-C16 as
  101-116, its DIP switches 101-108 from the DIP settings; a topper's own switches from 121);
  coils 1-32 as solenoids 1-32 by their Driver Reference numbers, other coils from solenoid 51, and
  the coils the game names LEFT FLIPPER and RIGHT FLIPPER, or LEFT and RIGHT FLIPPER POWER (with
  their HOLD coils), also as the flipper outputs 48 and 46; LED channels as lamps by their Light
  Reference numbers (a motor drive is no lamp); the motors the boards run on their own, then their
  steppers, as mechs (`GetMech(n)`: Ghostbusters' Slimer is mech 0; the 1000s, 100s, 10s, 1s and
  credit reels of Whoa Nellie, Primus and Pabst Can Crusher - one hardware, one set of switch
  numbers - are mechs 0-4, as steps 0-199; Game of Thrones' dragon and topper dragon are mechs 0
  and 1, as steps 0-199, the wings at 23, 80, 125 or 180). A title's stepper home switches on other
  boards are its `SPIKE1_HOMES`. Coil and LED levels are modulated outputs.
  The keys: coins 5, 6, 3 and 4, start 1, the service buttons 7-0 (back, minus, plus, select), tilt
  Insert, slam Home, coin door End.
- **Displays**: the DMD as a core DMD (`CORE_DMD_PWM_PREINTEGRATED_LINEAR_16`: the device model
  already sums the four bitplanes to 16 even shades); the LCD insert below it as a video display,
  drawn through 32768 palette pens after the core's. libpinmame reads a video display at its
  layout position, so the two are separate displays there.
- **Sound**: a stereo stream from the device model's samples, at the rate the game sets on
  `/dev/i2s` (Ghostbusters 44.1 kHz, Whoa Nellie 24 kHz); the mixer resamples it. The machine makes
  samples in time slices, and in about half of the stream's updates it had fewer ready than the
  stream asked for - each a gap of silence, heard as crackle - so the samples pass through a FIFO
  that plays once 1/30 s is waiting (`SPIKE1_LOG` reports how often the machine was late and
  whether the FIFO ran dry).
- **NVRAM**: the subsystem's block (see `spike1_linux::nvram()`) as `nvram/<set>.nv`, taken in
  `MACHINE_STOP` because PinMAME saves after the machine is gone. A game keeps its settings, audits
  and high scores in memory and commits them to its NVRAM files only when it sees the power fail
  (or, during play, when the machine falls silent), so `MACHINE_STOP` first switches the machine
  off the way the mains would (`spike1_pinmame_power_down()`): the line sense reads 0 V, the game's
  line-sense thread sends itself SIGPWR, its handler wakes the power-loss thread, which commits and
  syncs - about 170 ms of emulated time.
- **Diagnostics**: with `SPIKE1_LOG` set in the environment, the subsystem's log goes to stderr,
  and every 300 frames the driver reports the CPU slices, the time spent in the machine, what the
  sound stream asked for and got (with the peak sample), and the switch changes it passed on.
- **Tables** (VPX with its PinMAME plugin): `cGameName = "gbust_117h"`; `Controller.SolMask(2) = 2`
  for the lamps as levels (0-255) - the coils then come as levels too, any level above 0 on; the
  DMD is `ctrl://PinMAME/display?id=0`, the LCD insert `ctrl://PinMAME/display?id=1`;
  `Controller.GetMech(0)` is the motor's position (leave `HandleMechanics` at its default). The
  table needs the `PinMAMETimer` and `PulseTimer` timers core.vbs drives, and switch pulses go
  through `vpmTimer.PulseSw`: the plugin's controller has no `PulseSwitch`. Spike 1 has no system
  script among VPinMAME's, so the table defines its cabinet switches and keys itself.

Build: `cmake/spike1.cmake` (on by default, `-DPINMAME_SPIKE1=OFF` leaves it out), hooked into
`cmake/pinmame/CMakeLists_win-x64.txt` and `cmake/libpinmame/CMakeLists.txt`. With the Pinball 2000
subsystem built too, the shim's two sources come from its library.

## Where bring-up stands

Ghostbusters LE 1.17.0 boots to **attract mode** (high scores, replay value, "PRESENTS"), takes
coins and **starts a game**: "CREDITS 1 / PRESS START", the start animation, then ball 1 with the
skill shot - about 2.6x real time on an i7-13700KF. At game start the trough coil ejects a ball,
the flippers pulse and hold on their buttons, the slings and pops fire on their switches, and the
GI and insert LEDs light. In `PinMAME.exe` (Windows x64) the set passes `-verifyroms`, boots to
attract mode with the DMD and the insert drawn, takes coins and starts a game from the keyboard,
plays the coin sounds and the start music, and keeps its NVRAM across a restart - at 60 frames
per second, the machine taking about half of one core of an i7-13700KF. Through libpinmame, in
VPX 10.8.1 (Rev 6113) with its PinMAME plugin, a converted Ghostbusters LE table plays: DMD and
insert, lamps and GI, coils, switches, sound and Slimer's motor.

Game of Thrones LE 1.37.0 (six boards, 241 LED channels, Light References up to 265) boots to
attract mode, takes coins (four to a credit at its default pricing) and starts a game in the
harness and through libpinmame; its dragon homes on its switch and stands at step 23, the upper
and lower flippers fire from their buttons. The topper's home switch has no handler in the game
(`swdf_dragon_topper_motor_home` is in no switch table), so the topper homes by its time-out.

## Open items

1. Ball physics are the table's job (PinMAME/VPX); the harness has none, so a game stops at ball 1.
2. Outputs, what is left: the fade time of an LED update (its unit is not established, so levels
   change at once); the extra time and switch condition some coil fires carry; command 0x43
   (coil priority, a query answered with zeros); the LCD insert's fill command, the meaning of
   its animation-status reply (answered with zeros) and of a run's loop count.
3. Sound, what is left: the amplifier's gain steps (set through `/dev/amp`) and the center and
   headphone volumes are not applied.
4. PinMAME, what is left: VPinMAME (the COM controller) has not been checked; only CMake builds
   the subsystem (not the makefiles or the Visual Studio projects); the game lists do not name the
   set yet. A Visual Studio build of the subsystem runs at about half the speed of a GCC one.
   Coils are sampled once a frame: the game's own fires last 30 ms or more, but the 1 ms pulses of
   its start-up driver check are sometimes seen, so a table can twitch a flipper or a post at
   power-on, and a flipper's coil reaches the table up to a frame late.
5. Signals: kill, tkill and tgkill deliver to a handler (a frame with siginfo and ucontext,
   rt_sigreturn back), but a signal to a thread blocked in a system call waits until the thread
   wakes instead of interrupting the call.
6. `/proc` and `/sys` entries the game reads (`/proc/self/task/<tid>/comm`, `/proc/cpuinfo`).
7. Sharing `src/p2k/shim`: `cmake/spike1.cmake` takes its sources from the Pinball 2000 library
   when there is one; a common library for both would be cleaner.
