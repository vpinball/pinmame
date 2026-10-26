# Stern SPIKE 1

Stern's first SPIKE platform (2012 to 2020: Game of Thrones, KISS, Wrestlemania, Ghostbusters, ...). The
game runs as a statically linked ARMv5TE Linux program on an ARM926-class CPU board and talks to
the playfield's node boards over a serial bus; the display is a 128x32 DMD with 16 shades
(on Spider-Man The Pin: a 64x32 RGB LED matrix in its place). The two earliest titles,
Transformers The Pin and Avengers The Pin, run Stern's earlier ELG framework instead (below): one
node board, an older node bus and a 16-character alphanumeric display.

## How it works

There is no low level emulation of boards below the game program: the subsystem emulates Linux at the
system-call level instead.

- `mame/cpu/arm7/` - MAME's ARM7/ARM9 core, imported. The game runs on the ARM9 (v5TE) variant in
  user mode with the MMU off.
- `spike1_cpu.*` - the CPU device: user-mode register sets, so several guest threads can share it,
  and SWI delivered to the host (see `set_swi_handler()` in `arm7.h`).
- `spike1_memory.*` - the guest's flat address space: the program image, brk heap and mmap area in
  one low block, the main stack, the `0xffff0000` kernel helper page, and a read-only view of the
  asset image (over 1 GB; 1.9 GB for Heavy Metal), mapped from the host's file or memory instead
  of copied, at the start of the space between the low block and the stack. Each block is one fast
  window of the shim's `address_space`; the asset image's is read-only, so a guest write there
  stops the machine (as the game's write to its read-only mapping would end it on the board)
  instead of faulting the host. Further read-only views (WWE's LCD clips, up to 195 MB,
  mapped one at a time as they play) take that space from its top down; the CPU reaches them
  through the bus callback (`read_view()`), as it has no fast window left. A read-only mapping of
  1 MB or more of a file the host holds, or of 16 MB or more of one on the host's disk (`--root`),
  is such a view; without room for one it is copied like any other mapping.
- `spike1_vfs.*` - the filesystem the game sees: the title's files, read-only (a host directory
  with the machine's extracted root, or files the host holds - PinMAME's mapped ROM files),
  under an in-memory layer with everything the game writes. The board's `/data` and `/tmp` mounts
  start empty; the game keeps its NVRAM files under `/data/nv`.
- `spike1_linux.*` - the program loader (ELF, initial stack with argv/envp/auxv) and the system
  calls: files, memory, time, futexes, pipes, poll/select (which sleep until a device's data is
  due, or look again every millisecond when nothing gives that time), and the game's threads.
  `nvram()` gives everything the machine keeps across power cycles - the written files and the
  board's EEPROMs - as one block for the host's NVRAM file; the next start takes it back in its
  config. Threads are scheduled round robin on the one CPU, in quanta; when every thread is
  blocked, time skips ahead to the next wake-up without running instructions. Time is emulated
  time - CPU cycles at the configured clock. The monotonic clocks (`CLOCK_MONOTONIC`, `sysinfo`'s
  uptime, `times`) count from the board's boot, which comes 10 s (`BOOT_NS`, not measured) before
  the program's first start, as the board boots Linux and runs its start-up scripts first; the
  ELG titles' `sys_time_init()` asserts that the clock is above 0. They count all their time from
  its value then (`sys_time_get_usec()` and `sys_time_get_msec()` subtract it, and no other code
  calls `clock_gettime`; their date comes from `gettimeofday`), so any value from 1 ms on runs them
  alike; the cards hold no log of the real one. When the program exits, the board's `game_monitor`
  (`/etc/init.d/game_monitor`: run the game, print `RESTARTING GAME`, sleep 1, run it again)
  starts it a second later as a new process: new memory, threads and open files, and the program
  loaded again. The files it wrote, the CPU board's devices, the node boards, the EEPROMs and the
  clock carry on. A slam tilt ends the program on purpose (`slam_tilt_debounce_proc()` calls
  `exit(0)`) and comes back this way. A fault or an unimplemented system call stops the machine.
- `spike1_pinmame.cpp`, `spike1_public.h` - the plain C contract with PinMAME's half,
  `src/wpc/spike1.c`; the subsystem includes no PinMAME header.
- `spike1_boot.cpp` - the standalone harness.
- `spike1_lptest.cpp` - runs a set through `pinmame64.dll` as VPX would (below).

The kernel helpers at `0xffff0fa0..0xffff0fff` are real ARM code (the pre-v6 sequences from the
kernel's `kernel_user_helpers.rst`), so no per-instruction hook is needed; `__kuser_get_tls` reads a
word the thread switch keeps current. A thread that loses the CPU between `__kuser_cmpxchg`'s load
and its store restarts the helper, as the kernel does on pre-v6 CPUs, so the helper stays atomic.

## Source imported from MAME

From MAME 0.288 (tag `mame0288`, commit `27a8d9e`), `src/devices/cpu/arm7/`: `arm7.cpp`, `arm7.h`,
`arm7core.h`, `arm7core.hxx`, `arm7help.h`, `arm7ops.cpp`, `arm7thmb.cpp`, `arm7dasm.cpp`,
`arm7dasm.h`, `cecalls.hxx`, all `license:BSD-3-Clause`. Changes are marked `// PINMAME`:

| Where | Change |
|---|---|
| `arm7.h`, `arm7ops.cpp` `arm7ops_f`, `arm7thmb.cpp` `tg0d_f` | `set_swi_handler()`: SWI goes to the host instead of the exception vector |
| `arm7.cpp`, `arm7core.h` | `ARM7_PINMAME_DIRECT_FETCH` (default on): instructions are fetched straight from the address space, not through the modelled prefetch queue - nothing to model with the MMU off and no self-modifying code, and measured ~28% faster |
| `arm7.cpp` | no debugger console commands |
| `arm7ops.cpp` BLX (register) | **bug fix**: read the target before writing the link register. `blx lr`, which glibc's `clone()` uses to call a new thread's start routine, branched to its own return address (the Thumb form was already right) |
| `arm7.h`, `arm7core.hxx`, `arm7.cpp`, `arm7thmb.cpp` | `set_fault_handler()`: an undefined instruction or an emulation dead end (`fatalerror`) calls the host, which stops the game, instead of taking the undefined instruction vector or aborting the process |
| `arm7ops.cpp` `arm7ops_4567`, `arm9ops_undef` | **bug fix**: undefined ARM instructions trap. A register offset LDR/STR with bit 4 set (e.g. `UDF`, from `__builtin_trap`) ran as a load or store, and the unconditional space (cond = 0xF) skipped unknown instructions |
| `arm7ops.cpp` `arm9ops_57` | PLD is valid (a no-op) from ARMv5TE, not only from ARMv6: it would trap now, and glibc's `memcpy` uses it |
| `arm7ops.cpp` `HandleALU` | **bug fix**: a register operand with an immediate shift takes no extra cycle, only a register specified shift does (e.g. every `add r0, r1, r2` was charged one cycle too many) |

The core uses the existing MAME compatibility layer of the Pinball 2000 subsystem (`src/p2k/shim`), with a
few additions for modern MAME's arm7: the `TR_*` translation intents, `address_map_constructor`, the four-argument
`memory_translate()`, `address_space::read_ptr()`/`endianness()`, `total_cycles()` and `currently_executing()`.

Speed, measured with real game functions on an i7-13700KF: about 93 M instructions/s as imported,
about 118 M/s with the direct fetch. A SPIKE 1 game needs about 36 M/s in attract mode and 50-61
M/s in play once the line-sense thread waits for its ADC samples as it does on the real board,
and the sound thread for its DAC.

## Building and running

```sh
cmake -S cmake/spike1boot -B build/spike1boot -DCMAKE_BUILD_TYPE=Release
cmake --build build/spike1boot
build/spike1boot/spike1boot --root <extracted root> --game /games/<name>/game --state <dir> --seconds 30 [--trace]
build/spike1boot/spike1boot --files <the game's folder> --game /games/<name>/game --state <dir> --seconds 30
```

The ELG titles' program is in `/usr/local/games/<name>` on their root filesystem (`--root <root>
--game /usr/local/games/tf-elg/gamer`), or `--files <the folder> --game /games/tf-elg/gamer`; the
folder's `sounds` directory comes along.

Harness options: `--dmd <s>` prints the DMD every s seconds (on the ELG titles the alphanumeric
display's text, as the game's own font draws it, and each character's brightest level), `--list-switches` the switch, coil and LED maps,
`--switch <s>:<name>[:<ms>]` toggles a switch by name for a while (e.g. `--switch "20:LEFT COIN:120"`;
the time is emulated, in steps of `--slice`, and 0 ms toggles it and back at once, before the game
can read it), `--led <name>` prints that LED channel's level as it changes, `--reads <file>` writes
the time and board of every switch read (GetInputState) to a file,
`--outputs` prints coil changes and each coil's reflex as the game arms and disarms it as they happen and the lit LEDs at the end, `--wav <file>` records
the sound, `--insert <s>` saves
the LCD insert every s seconds as a PPM file in the state directory (`--rgb <s>` the same for the
RGB matrix, which needs `--display-link rgbdotmatrix-0_1_1.hex` as the PinMAME set gives it), `--node-dump <file>`
writes every node-bus frame (except switch and status reads) to a file, `--peek <address>:<bytes>`
prints guest memory at the end, `--stepper-home <node>:<stepper>:<switch>` links a stepper to a
home switch on another board as a PinMAME set's `SPIKE1_HOMES` does (Game of Thrones: `10:0:88`
and `12:0:121`), `--limit-motor <forward LED>:<backward LED>:<home switch>:<away switch>:<ms>`
adds a limit motor as a set's `SPIKE1_LIMIT_MOTOR` does (KISS: `169:170:53:54:1000`),
`--encoder-motor <node>:<motor>:<low stop>:<high stop>:<counts a second>` links an encoder motor's
mechanism as a set's `SPIKE1_ENCODER_MOTOR` does (Ghostbusters: `8:0:-6:96:50`; `--outputs` then
prints the mechs' positions as they change),
`--video-key <32 hex digits>` hands the game the key its LCD clips are encrypted with, `--lcd <s>`
saves the CPU board's LCD every s seconds as a PPM file in the state directory,
`--poke <s>:<hex address>:<hex byte>` writes a byte of guest memory then (for experiments),
`--slice <ms>` sets the emulated time per `run()` call (1 by
default, the period PinMAME exchanges the I/O at). With `SPIKE1_NODE_STATS` set in the environment, it
prints the node-bus frame count per command and board (and per second), and how much of the time
the wire carried bytes.

`--root` is the machine's root filesystem extracted from its image (with `games/<name>/game` and
`image.bin` in it). `--files` instead loads the game's folder (`games/<name>`: the program,
`image.bin`, `lcdinsert.bin`, the node firmware, WWE's `video/` clips) into memory, as PinMAME hands them over; the game
needs nothing else from the root. The state directory keeps the machine's NVRAM between runs as
`spike1.nv`, and the `--insert` pictures. `--trace` logs every system call; without it, only noteworthy ones (device
traffic, files not found, threads) are logged. From Git Bash, set `MSYS_NO_PATHCONV=1`, or it turns
`/games/...` into a Windows path.

`spike1_lptest` (Windows; MinGW: `g++ -std=c++17 -O2 -static -o build/lptest/spike1_lptest.exe
src/spike1/spike1_lptest.cpp`) runs a set through `pinmame64.dll` as a VPX table does:

```sh
spike1_lptest <pinmame64.dll> <vpm path> <set> [--seconds <n>] [--coin <switch>] [--coins <n>] [--coin-at <s>]
              [--start <switch>] [--start-at <s>] [--press <s>:<switch>[:<ms>]]... [--lamp <n>]... [--mech <n>]...
              [--frames <dir>]
```

The vpm path holds `roms/` and `nvram/`; the times are seconds of a running machine, and the
defaults are Ghostbusters' LEFT COIN (81) and START BUTTON (78). A press shorter than 20 ms is
timed exactly, from the tool's own thread as a table sets switches, and 0 ms closes and opens the
switch back to back; a negative switch is one closed at rest, opened for the time. `--lamp` prints
a lamp's level as it changes, `--mech` a mech's `GetMech()` value (Slimer is mech 0; the tool
leaves `HandleMechanics` at its default, as a table does), `--frames` saves the DMD and the video
displays as `display<n>_<second>.png`, each second's last frame, and the library's "Average FPS"
line at the end gives the frame rate. `SPIKE1_LOG` passes the subsystem's log on, from a libpinmame
built with `SPIKE1_DEBUG` (see Diagnostics below).

## Devices (`spike1_devices.*`)

Each one answers the Linux driver interface the game uses, worked out from the game's own driver
code (its symbols are in the program) rather than from any other emulator:

- **I2C** (`/dev/i2c-0`, `I2C_RDWR` and plain read/write): 24xx EEPROMs at 0x50 and 0x51, blank on
  a new board - the game formats its NVRAM store itself - and kept in the NVRAM block; the
  amplifier's MCP4631 volume potentiometers at 0x28-0x2a.
- **AC line sense** (`/dev/adc`): 3600 samples/s, a 60 Hz valley-shaped waveform that the game's
  `LineSenseThread` measures as 60 Hz and accepts as a real line; reads wait for the samples' time.
- **DMD** (`/dev/spi0`): 2048-byte frames of four bitplanes, plane n weighing 2^n, decoded to 16
  shades; writes wait for the panel's 15.75 ms PWM pattern.
- **RGB matrix** (Spider-Man The Pin, `/dev/spi0` as well): 3072-byte frames of 64x32 dots, 4 bits each
  of red, green and blue as four bitplanes (`rgb_write()` has the layout). The game picks its
  display by the target of the root filesystem's `/usr/local/spike/display.hex` link
  (Spider-Man's program knows `rgbdotmatrix-*`, and the 64x32 mono `dmd_1x-*` and `dotmatrix-*`,
  its default); `spike1_linux::add_link()`
  answers that readlink with what the set says. Its display thread also paces the game's main
  loop, so writes wait for a frame time; the panel's is not known, so the DMD's is used.
- **Node bus** (`/dev/ttyS4`): the game's own `NODEBUS_TransferMessage` framing. The board list comes
  from the title's `node_board_table`; each board's identity (firmware version, NXP part ID,
  checksum) from the firmware image the game itself loaded for it, so validation passes with no
  forcing. Modelled: poll, bridge version/status/state, GetVersion, GetStatus (frame counts),
  GetBootStatus, GetFullBoardID, GetChecksum, GetInputState, LCD insert info, and the outputs
  below. Other queries are answered with zeros and logged once; other commands are logged once.
  A board counts time in ticks at the rate its GetVersion reports (1000 Hz here); the game
  converts coil and motor times from ms with it.
- **The node bus wire**: the bytes take the wire's time at the rate and character frame the
  game sets with TCSETS (460800 baud, 8 data bits, 2 stop bits: 23.9 us a byte). The link is half
  duplex: a frame goes out once the bytes before it are through, and its answer follows it, so
  `select()` returns, and `read()` finds the answer, once its last byte is in (the game sets
  `ASYNC_LOW_LATENCY`, so the driver passes received bytes on at once). The bridge's relaying and
  a board's time to answer are not known and count as nothing, so a transfer takes the least
  time the wire allows. `tcdrain()` waits until the written bytes have gone out. The game's
  node-bus loop polls every board, reads its switches, then sleeps 1 ms, so the wire sets its
  pace: in attract mode a board's switches are read every 2.0 ms with two boards (Supreme,
  Spider-Man) to every 4.6 ms with seven (Ghostbusters); WWE's six take 4.0 ms, Whoa Nellie's
  five 3.8 ms. A game in play reads at about the same pace (Ghostbusters 4.9 ms, at most 7.6).
- **Switches**: the title's own tables (`node_board_table`, `node_board_device_table`,
  `switch_table`, `switch_dedicated_table`) give each switch's board, position, number, name and
  polarity. A board's eight GetInputState bytes and the CPU board's 3-byte `/dev/spi1` transfer
  carry position p in byte p/8, bit p%8, active low unless the switch table marks it active high.
  At rest: a trough opto per installed ball (`hook_balls_installed_in_game`), the firmware's other
  closed-at-rest marks and the coin door's power sense. WWE's SDK asks GetInputState for the eight
  switch bytes alone.
- **Where the game reads a switch**: the device table is what the switch test names the inputs
  from, but the switch code reads each board through its compiled switch map (`g_node_data` gives
  each board's node address and its `g_smap_<board>`: per input, the switch's word and bit).
  The two disagree on a few switches, and a switch is wired where the map reads it. Supreme,
  Primus, Whoa Nellie and Pabst read SLAM TILT at cabinet input 21 where their tables have 22, and
  KISS the reverse, so a slam at the table's input does nothing. WWE's SDK reads the CPU board's
  DIP switches 2-8 and service buttons an input lower than its table (DIP 2 at 1, SERVICE SELECT
  at 8, BACK at 11, as the later titles' tables have them). Heavy Metal's SDK has no compiled maps:
  it builds them from the device table as it starts. `--list-switches` shows where each one is.
- **Short switch closures**: a ball through a VPX trigger closes it for a few ms, and VPX runs a
  frame's physics in a burst, so its Hit and UnHit can come within a fraction of a millisecond.
  The game reads each change once: `sys_node_board_device_switch_update_inputs()` counts the edges
  between two reads of a board and the switch code handles each one (the same code in all ten
  programs with symbols; Heavy Metal's has none), so one read that sees a closure is enough - a
  2 ms Start press that a read falls in starts a Ghostbusters game (a 1 ms one, Supreme), a 1 ms
  one between two reads does not. So each read shows a switch's next
  change (`wire_bytes()`): a switch that closed and opened again since the last read shows closed
  to this read and open to the next, and every closure reaches the game, at most a read late. The
  boards' reflexes (flippers, slingshots, pops) follow the switch itself, at once. A switch the
  game also debounces in time keeps its time: Whoa Nellie's Start takes between 25 and 30 ms
  held, however many reads see it (its playfield switches take one read). The CPU board's inputs are also read outside the node bus loop (the game's
  immediate switch tests), and such a read takes the change too; those are the service buttons
  and the coin door, which a person holds.
- **Numbers** are the factory manual's, which the title's own tables hold (`apply_manual_numbers()`):
  `node_board_device_sw_table` gives the Switch Reference numbers, `node_board_device_cl_table_data`
  the Driver Reference numbers of the coils, `node_board_device_led_table` the Light Reference
  numbers and class bits (lamp, GI string, flasher, motor drive, cabinet) of the LED channels. The
  manual numbers the CPU board's own switches C1-C16 apart; they are 101-116 here. An optional
  topper's board - one the game names `TOPPER...` in `lang_text_nb_<node>` - numbers its switches
  from 1 again (Game of Thrones' TOPPER DRAGON is 1, as is its LEFT RETURN LANE); they are 121 and
  up here. WWE's SDK lays the tables out its own way: 28-byte coil entries (the index in the low
  half at +20, the number at +24) and 64-byte switch entries with the index at +40 and the number
  at +60.
- **Coils** (`node_board_device_table` type 2): a fire command gives a
  first power and time, then a second; a reflex configuration makes the board fire the coil on its
  own when a switch it watches closes - flippers pulse at full power, then hold at the second power
  while the button stays closed (the end-of-stroke switch ends the pulse early); pops and slings
  pulse once per closure and wait out a hold-off time. Where the reflex frame keeps its switches
  depends on the SDK, which its length tells (see `coil_reflex()`: 23 bytes in WWE, 28 in Supreme,
  34 from node firmware 0.28 to 0.52, 43 in Heavy Metal); up to three switches can trigger one
  coil (Heavy Metal's slings have two). `coil_level()` gives a driver's PWM duty (0-255) at a
  given time; the coil mask (a set bit switches a driver off) applies. `coil_output()` is what a
  host shows, taken every millisecond in PinMAME: a pulse that came and went between two takes
  shows at its power, and a pulse shorter than 4 ms shows nothing. That is the game's power-on driver check:
  every title but WWE fires each driver alone for 1 ms and logs its current, too short to move
  anything (the shortest real pulse is 16 ms, Primus' slingshots).
- **LEDs** (type 4, up to 96 channels a board, three for an RGB LED): `NODEBUS_SetLEDMultiple2`'s
  packed updates (command 0x80-0xBF: an index, a list or a bitmap of channels, then levels and fade
  times in compact forms; see `led_update()`). Every update in a full attract-and-play run decodes to
  exactly its length. A channel ramps in a straight line from where it is to the new level over the
  fade time, in units of 16 ms (`led_set()`: the node boards' firmware steps each channel 1280
  times a second, a unit taking 20.48 steps; 0 and 1 set the level at once). `led_level()` gives a
  channel's level (0-255) at a given time; the LED mask applies. A limit motor's drive takes the
  level an update set, not the ramp. A
  title without `NODEBUS_SetLEDMultiple2` (Whoa Nellie, Primus, Pabst Can Crusher, Game of
  Thrones, KISS) sends runs of channels instead, on the same command bytes - `0x80 | first`, then a fade
  time and a level per channel, or 0xff and a time and level per channel (`led_update_run()`) -
  and Game of Thrones also command 0xC0 for channels from 64 on (its start-up test sweeps all 96 a
  board drives): a 16-bit first channel, then a run in the same two forms. The model picks the
  packed form by that symbol.
- **Encoder motors** (Ghostbusters' Slimer, the game's `EncoderMotor`; commands 0x50-0x57): the
  board runs the motor itself, and the model runs it as the board's firmware does (node firmware
  0.52.0, `pinnode-LPC1313`; `motor_command()`, `motor_tick()`). A loop 1280 times a second drives
  one of two outputs at a PWM duty, counts the encoder's two inputs once the motor is homed, and
  stops on the home, away or limit input, on the exact count, or at a time-out (which the watch
  input's edges restart). Home drives down until the home input is active, away up until the away
  input is; a go needs a homed motor, ramps the duty up a step a tick to its speed (the step is
  `51200 / ramp` of the configuration, scaled by the speed), holds it, ramps it down to the least
  duty from as far before the target as the ramp up took (or from half way), and stops on the
  count. The status (0x52) is the count and the firmware's bits: 0 driving, 1 down, 2 holding, 3
  homed, 4 away found, 5 target switch found, 6 time-out, 7 limit, 8 a go without an encoder. An
  input id names the board's input in bits 0-5, bit 6 marks it used and bit 7 makes it active high.
  The game homes twice: first with the outputs swapped and the home input's polarity turned (up,
  until out of the home zone), then as configured (down, until into it), so its zero is the zone's
  edge, reached from above. The host links the mechanism (`link_encoder_motor()`): its end stops in
  counts from that edge, the home zone reaching from there down to the lower stop, and the counts
  a second it makes at full drive (a duty of 255); it stands just inside the zone at power-on, and
  the home switch and the encoder's two phases follow its position. Without one it has no stops and
  makes 50 counts a second. Which way an output turns the motor comes from its LED channel's name
  (SLIMER MOTOR FORWARD, BACKWARD). The brake output (no title configures it) and a go to a target
  switch without an encoder are not modelled.
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
- **Limit motors** (KISS's Starchild, the game's `StarchildMotor`): a motor the game runs itself
  through an H-bridge on two LED channels, between a home and an away switch - one channel at full
  level drives it toward away, the other toward home, until the game sees the switch or gives up
  (KISS: 434 of its ticks, about 5 s). The host links the channels and switches and gives a travel
  time (`link_limit_motor()`); each switch is closed over the last 2% of the way, so the start-up
  LED test, a few ms on each channel, leaves the motor on its home switch, where it stands at
  power-on. The model moves it on at every LED update and switch read.
- **Board blocks**: the game keeps a block a board; `sys_node_board_get_next_block_ptr` gives its
  base and size, which differ between SDK versions (a loaded constant in Ghostbusters, a computed
  one in Whoa Nellie and WWE, 80 bytes there) - the model reads both forms, as the firmware image a
  board must report hangs off its block. The board type the game settles on for a block sits where
  `node_update_runtime_hex_image_id` first looks (+88 on most SDKs, +32 on WWE's).
- **Sound** (`/dev/i2s`): the game's DAC handler writes 200 stereo frames of 16-bit samples at a
  time, at the rate it sets with a code from its own table (44.1 kHz; ioctl 0x3e01 answers the code
  in force, which the ELG titles check after each change). A write waits while the DAC
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
- **CPU board LCD** (`/dev/fb0`, WWE's playfield screen): a Linux framebuffer, 320x240 RGB565 in
  two buffers one above the other. The game maps it, sets its timing (the model takes the frame
  rate from the pixel clock and margins: 58.95 Hz), draws into the buffer not shown and pans to it,
  then waits out the frame's vblanks - counted with Stern's ioctl 0x80204612 (32 bytes, the count
  at +4) and waited with `FBIO_WAITFORVSYNC`, whose argument is the count, so the clips play at
  their own rate (20 fps). `lcd_picture()` gives the shown buffer. The clips (`video/*.spv`: a
  36-byte header, then 153600-byte frames) are RC4-encrypted; the game decrypts each frame itself
  (`VideoPlayer::render_frame`, key byte 9 XORed with the frame number) with the key its factory
  key store gives it - a block in EEPROM 0x51 encrypted with a key derived from the board's MAC
  address, which the model cannot make. So the host hands the key over (`provide_video_key()`) and
  the model puts it in the game's `rc4_key` while that holds the game's 0xCC placeholder; the
  HMAC key beside it is never used. WWE picks its attract video once, at start-up, while its node
  bus start-up (a power cycle of the boards, some 5 s) still holds video effects back; on the
  board, the program's slower loading puts that after the start-up. The model sets the game's
  `video_effect_update_background_flag` once the start-up is over - what the game does itself
  when a video ends - and the attract clips play.

## The ELG titles (Transformers The Pin, Avengers The Pin)

Stern's earlier framework - the programs' build path is `system-elg`, their game folders `tf-elg`
and `avengers-elg` - is a code base of its own: its own node bus, tables, display and EEPROM. The
model takes it for a program with `node_query_t()` and `nodemap_init()` (`elg_*` in
`spike1_devices.cpp`):

- **Node bus** (`/dev/ttyS4`, 115200 baud, 8 data bits, 2 stop bits; node firmware 0.5.16 and
  0.6.0): `[0x80|node][n][cmd][n - 1 bytes]`, with no checksum and no reply length. A poll (00)
  gets the address of a board whose inputs changed since its last query, or 0; a query (0x11) the
  board's eight input bytes; status (0xff) six bytes, all 0; quadrature (0x60) one byte, 0. Coils
  (0x40|c: a power and a time, twice), reflexes (0x48|c: a pop or a sling on one input, with a
  hold-off; a flipper on its button and its EOS input, held at its second power while the button
  stays closed) and LED runs (0x80|first: a level each, set at once) have no answer. The game
  disarms a reflex by sending it with power 0. The times are taken as ms - the coil table has the
  flippers' pulse at 64 and the slings' at 20, and the game refreshes a coil it holds on, sent for
  255, every 250; the node firmware (not in the set) would settle it.
- **Tables**: the one switch map the program fills in (`g_smap3`: 64 inputs, each with its
  switch's byte and bit, an inverted flag and a name), which `nodemap_init()` registers for the
  one board the coil table (`__coil_table`: 8 coils, their type, powers, times, reflex inputs and
  names) and the lamp table (`__lamp_table`: 64 lamps) name, node 8. The game numbers a switch
  byte x 8 + bit, and counts its switches, coils and lamps from 0, as its test menu shows them;
  here they are one more, as PinMAME's numbers start at 1: START 1, the flipper buttons 2 and 3,
  their EOS switches 4 and 5. The names lose their `SWITCH_` prefix and underscores. At rest the
  trough holds a ball at each of its numbered optos (TROUGH 1-3). The CPU board's own switch map
  (`g_smap_gpio`) is empty in both titles.
- **The bridge's EEPROM**: the game keeps its settings and high scores in the bridge's 64-byte
  EEPROM (the bridge is an AVR, `netbridge-0_0_5.hex`), which it reaches over the node bus in the
  bridge's command mode (`sys_command_mode()` raises a GPIO line around each exchange):
  `[0x55][cmd][n][n bytes][sum]`, a read (cmd 0, a big-endian address) or a write (cmd 1, the
  address and the byte), answered `[0xaa][n][n bytes][sum]`, each sum making the bytes after the
  first add up to 0. Blank (0xff) on a new board, so the first start reports a checksum error and
  writes the defaults; it is kept in the NVRAM block beside the I2C EEPROMs.
- **Display**: the alphanumeric display's frames come on `/dev/spi0` (`dmd_update_t()`, 256
  bytes: four bitplanes of 32 positions of 16 segments, a 4-bit level each; `/dev/dmd`'s 0x3d01
  and 0x3d02 bracket them), and the game keeps its text to the first 16 positions
  (`ALPHANUMERIC_ConstrainString()`): the display's 16 characters, "two displays, eight characters
  each" in the start-up script. The segments are in the game's own order (its `segmentX` and
  `segmentY` tables, whose y runs upward): its segment k is PinMAME's `CORE_SEG16S` bit 15 - k,
  checked on the game's font (`_ZL4font`), whose 'A', '7', 'J' and 'L' come out as PinMAME's own. `/dev/ttyS3` is the display's text port: the
  game scrolls " STARTING GAME " there for its first 14 s (`proc_splash_display_t()`, `[0x02][16
  characters][0x03]` every 5 ms) while the frames already show attract mode. Which of the two the
  display shows is its own firmware's (`alphanumeric-1_0_4.hex`, not in the set), so the model
  shows the frames and takes the text without showing it.
- **Sound**: the game mixes its WAV files (`sounds/`) itself and writes the DAC as the later
  titles do, at 24 kHz.

## In PinMAME (`src/wpc/spike1.c`)

- **The set** is the title's game folder from Stern's update image - the program, `image.bin`,
  `lcdinsert.bin` if it has one, the node firmware, and WWE's 33 LCD clips (`.spv`) from the
  folder's `video` directory - in one ROM region and audited like any other set's ROMs. The region
  is `ROMREGION_MAPPED` (`src/common.h`): the ROM loader does not load it but maps each file
  read-only from the set on disk - an entry the set's zip stores uncompressed, or a file in a set
  folder (`roms/<set>/`) - and keeps it until the machine shuts down, so the host pages the 0.1 to
  2.9 GB in as the game reads it (WWE: about 175 MB of working set instead of 2.9 GB, and it
  starts about 10 s sooner). An ELG title's set is the program the machine runs (its
  `/usr/local/games/game` link's target: Transformers' `gamer`, which `SPIKE1_INIT_ALNUM` names)
  and the WAV files of the folder's `sounds` directory, which the subsystem puts back there; the
  folder's other program (Transformers' older `game`, Avengers' copy `game0`) is not in the set.
  Avengers' 366 files are the most a set has: the loader keeps up to 512 mapped files
  (`MAX_MAPPED_ROMS`, `src/common.c`) and the audit checks up to 512 ROMs a set (`AUD_MAX_ROMS`,
  `src/audit.h`; both were lower).
  The loader checks each file's length, and for a zip entry the CRC its
  directory records; `-verifyroms` checks them in full. A file it cannot map - a zip that a ROM
  manager recompressed, or a host that does not map files - it loads into memory as before, and
  the driver says so. The files are over 1 GB, so the games need a 64-bit build. `MACHINE_INIT`
  hands them to the subsystem (`get_mapped_rom()`), which runs the program as
  `/games/<folder>/game` and puts the clips in `video/` (a set's files sit side by side, as
  PinMAME matches a ROM by its name alone); `image.bin` and the clips are mapped into the guest
  from there, not copied. While the machine runs, the set's zip stays mapped: replacing it then
  fails on Windows, and on Linux truncating it would end the game with a bus error. `scripts/spike1/Build-Spike1RomSet.ps1` (or `Build ROM set.cmd` beside it, which
  opens a window) makes the set from Stern's SD-card image, `<title>-<version>.iso.zip` or the
  `.iso` inside it, or a card dump in a zip (Supreme's `.img`): it finds the game folder on the
  image's ext3 partitions, reads it straight out of the zip in a few forward passes (its own zip
  reader: .NET's refuses Supreme's zip, whose ZIP64 extra field carries fields its header does not
  call for), writes `<set>.zip` stored, and checks every file against the
  set's CRCs - a new set needs its files added to the script's `KnownSet` table. It reads Stern's
  `.spk` update package too (Heavy Metal comes only as one): `SPKS`, then groups (`SPK0`), each an
  index (`SIDX`: the group's name, a path table `STRS` and a `FINF` record per file) and the files'
  bytes (`SDAT`), stored; the game folder is the group holding `<name>/game` and `<name>/image.bin`.
  An ELG title's game folder is `/usr/local/games/<name>-elg` on the root file system, with no
  `image.bin` and its sounds in a `sounds` directory, whose WAV files the set holds under their own
  names: Transformers' from Stern's `transformers_pin-1.0.18.iso`, Avengers' from the raw dump of
  its card, `avengers.bin`, or the `.7z` it comes in, which the builder reads through 7-Zip's
  `7z.exe` (found where its installer records it, in Program Files or on the PATH; without it, it
  says so). Avengers' root file system lies in the dump's first 172 MB, so each pass unpacks no
  more than that: about 10 s for the set.
- **Editions**: Stern's Pro is a lesser-featured edition of a title. Its set is a clone of the
  Limited Edition's, named as PinMAME names Stern's SAM sets (`got_137` beside `got_137h`), with
  its own game data (`SPIKE1_CLONEDEF`: its folder, switch numbers and steppers). The files it
  shares with the parent - Game of Thrones Pro shares all but the game program - can come from the
  parent's ROM set, so a Pro set's zip can hold its game program alone; the builder writes the
  whole folder.
- **A program without a symbol table** (Heavy Metal): the set's `SPIKE1_INIT_SYMBOLS` lists the
  addresses of the tables and functions the subsystem reads by name, which `spike1_pinmame_start()`
  adds to the program's (empty) symbol table; `spike1boot --symbols <file>` takes the same list as
  `<hex address> <size> <name>` lines.
- **The CPU** (`CPU_SPIKE1`, `src/cpuintrf.c`) runs the machine for the cycles PinMAME gives it,
  at 400 MHz. Every millisecond (a PinMAME timer, `SPIKE1_IOFREQ`) the driver passes switch
  changes in - the game reads a board's switches every 2 to 5 ms, at the node bus wire's pace
  (above), so a change is there for its next read - and takes coils, LEDs and a new DMD frame
  out. The core reports solenoid changes to its hosts once a frame, so a coil stays on for a
  frame at least: a pulse that came and went between two frames still shows.
- **Short switch closures** in PinMAME: a table sets switches from its own thread, so a closure
  and its opening can both fall between two of the driver's looks at the switch matrix. The core
  calls `core_swChangedHook` (`src/wpc/core.c`) with the matrix entry each time `core_setSw()`
  changes a switch; the driver counts the changes, and a switch whose count moved while its level
  did not reaches the machine as a change and a change back (then `wire_bytes()` above shows it to
  the game). `core_setSw()` sets the level, clearing the bit first, before it counts, so a look
  that finds a level the count's parity does not account for leaves that switch to the next look;
  a change made without `core_setSw()` (the core's own flipper keys) is taken there too.
- **Numbers** are the factory manual's (the ELG titles have no manual: theirs are the game's own
  plus one, above; the driver finds their START by that name): switches by their Switch Reference numbers (the flipper
  buttons and EOS switches in PinMAME's flipper column - each title's numbers, with its coin, tilt
  and slam switches, are in its `SPIKE1_SWITCHES`, or `SPIKE1_SWITCHES_UPPER` with the upper
  flippers' buttons, which the flipper keys close with the lower ones; the CPU board's C1-C16 as
  101-116, its DIP switches 101-108 from the DIP settings; a topper's own switches from 121);
  coils 1-32 as solenoids 1-32 by their Driver Reference numbers, other coils from solenoid 51, and
  the coils the game names LEFT FLIPPER and RIGHT FLIPPER, or LEFT and RIGHT FLIPPER POWER (with
  their HOLD coils), also as the flipper outputs 48 and 46; solenoid 33 is GAME ON, as on SAM: the
  flippers enabled, that is their reflexes armed on the boards with power. The game arms the
  flipper, sling and pop reflexes while a ball is in play and disarms them at a tilt and when a
  ball or the game ends (verified in the harness on Supreme: node-bus command 0x41 with zeros for
  all five reflex coils at the third tilt warning), so a table switches its own slings and pops
  with it; the solenoids then run through 48 at least, so GAME ON and the flipper outputs are
  physical outputs too; LED channels as lamps by their Light
  Reference numbers (a motor drive is no lamp); the motors the boards run on their own, then their
  steppers, as mechs (`GetMech(n)`: Ghostbusters' Slimer is mech 0; the 1000s, 100s, 10s, 1s and
  credit reels of Whoa Nellie, Primus and Pabst Can Crusher - one hardware, one set of switch
  numbers - are mechs 0-4, as steps 0-199; Game of Thrones' dragon and topper dragon are mechs 0
  and 1, as steps 0-199, the wings at 23, 80, 125 or 180; then a limit motor, KISS's Starchild as
  mech 0, from 0 at home to 100 away). Slimer's mech is its encoder count, 0 at the home zone's
  edge. The driver takes the mechs' positions on the machine's thread at each I/O exchange, so
  `GetMech()` from the host's thread reads the last ones taken. A title's stepper home switches on
  other boards are its `SPIKE1_HOMES`, its limit motor its `SPIKE1_LIMIT_MOTOR`, its encoder motor's
  mechanism its `SPIKE1_ENCODER_MOTOR` (`SPIKE1_INIT_ENCODER`; Slimer: node 8, motor 0, stops at -6
  and 96, 50 counts a second). Coil and LED levels are modulated outputs.
  The keys: coins 5, 6, 3 and 4, start 1, the service buttons 7-0 (back, minus, plus, select), tilt
  Insert, slam Home, coin door End.
- **Displays**: the DMD as a core DMD (`CORE_DMD_PWM_PREINTEGRATED_LINEAR_16`: the device model
  already sums the four bitplanes to 16 even shades); the LCD insert (160x128) or the CPU board's
  LCD (WWE, 320x240) below it as a video display, drawn through 32768 palette pens after the
  core's. libpinmame reads a video display at its layout position, so the two are separate
  displays there. The key the encrypted LCD clips use is not in the set: the user puts it in
  `<set>.txt` beside the set (`wwe_135h.txt`, 32 hex digits; spaces, commas and `0x` prefixes are
  allowed), which the driver reads from the ROM path when the title has such clips
  (`spike1_pinmame_needs_video_key()`) and hands to the subsystem (`spike1_pinmame_video_key()`).
  Without it, a message names the file and the clips show as noise. Spider-Man's RGB matrix is its
  only display (layout `spike1_rgb`, `SPIKE1_INIT_RGB`): a 64x32 video display, display 0, in full
  colour; the set's `displayLink` goes to `spike1_pinmame_display_link()`. The ELG titles'
  alphanumeric display is their only one: a line of 16 `CORE_SEG16S` characters (layout
  `spike1_alnum`, `SPIKE1_INIT_ALNUM`), each segment's level (0-15) its modulated output.
- **Sound**: a stereo stream from the device model's samples, at the rate the game sets on
  `/dev/i2s` (Ghostbusters 44.1 kHz, Whoa Nellie 24 kHz); the mixer resamples it. The machine makes
  samples in time slices, and in about half of the stream's updates it had fewer ready than the
  stream asked for - each a gap of silence, heard as crackle - so the samples pass through a FIFO
  that plays once 1/30 s is waiting (`SPIKE1_LOG`, in a `SPIKE1_DEBUG` build, reports how often the
  machine was late and whether the FIFO ran dry).
- **NVRAM**: the subsystem's block (see `spike1_linux::nvram()`) as `nvram/<set>.nv`, taken in
  `MACHINE_STOP` because PinMAME saves after the machine is gone. A game keeps its settings, audits
  and high scores in memory and commits them to its NVRAM files only when it sees the power fail
  (or, during play, when the machine falls silent), so `MACHINE_STOP` first switches the machine
  off the way the mains would (`spike1_pinmame_power_down()`): the line sense reads 0 V, the game's
  line-sense thread sends itself SIGPWR, its handler wakes the power-loss thread, which commits and
  syncs - about 170 ms of emulated time.
- **Diagnostics**: all of the logging is compiled in only with `SPIKE1_DEBUG` (`src/spike1/spike1_debug.h`;
  `-DPINMAME_SPIKE1_DEBUG=ON` in CMake, `SPIKE1_DEBUG=1` in a Visual Studio project's preprocessor
  definitions; `spike1boot` always has it), so the builds users play on spend nothing on it - they
  only report a stop of the game program and what happened to the NVRAM. In a `SPIKE1_DEBUG` build,
  with `SPIKE1_LOG` set in the environment, the subsystem's log goes to stderr, and every 300 frames
  the driver reports the CPU slices, the time spent in the machine, what the sound stream asked for
  and got (with the peak sample), and the switch changes it passed on.
- **Tables** (VPX with its PinMAME plugin): `cGameName = "gbust_117h"`; `Controller.SolMask(2) = 2`
  for the lamps as levels (0-255) - the coils then come as levels too, any level above 0 on; the
  DMD is `ctrl://PinMAME/display?id=0`, the LCD insert or WWE's LCD `ctrl://PinMAME/display?id=1`;
  `Controller.GetMech(0)` is the motor's position (leave `HandleMechanics` at its default). The
  table needs the `PinMAMETimer` and `PulseTimer` timers core.vbs drives, and switch pulses go
  through `vpmTimer.PulseSw`: the plugin's controller has no `PulseSwitch`. The system script is
  `spike1.vbs`, which is not in this repository (but in VPX's Scripts folder): a table defines
  `cGameName` and calls `LoadVPM "", "spike1.vbs", 3.61`, and the script sets the title's cabinet
  switches (`swCoin1`-`swCoin4`, `swStartButton`, `swTournament`, `swLaunch`, `swTilt`,
  `swSlamTilt`, the flipper buttons' leaves) from the set name, as each title numbers them its own
  way, with the CPU board's (`swEnter`, `swUp`, `swDown`, `swCancel`, `swCoinDoor`), the keys, the
  help text and the DIP switch dialog. The flipper keys close the upper flippers' leaves too,
  unless the table sets `Spike1StagedFlippers = True` for staged flipper buttons.

Build: `cmake/spike1.cmake` (on by default, `-DPINMAME_SPIKE1=OFF` leaves it out), hooked into
`cmake/pinmame/CMakeLists_win-x64.txt` and `cmake/libpinmame/CMakeLists.txt`. With the Pinball 2000
subsystem built too, the shim's two sources come from its library.

## Where the emulation stands

Ghostbusters LE 1.17.0 boots to **attract mode** (high scores, replay value, "PRESENTS"), takes
coins and **starts a game**: "CREDITS 1 / PRESS START", the start animation, then ball 1 with the
skill shot - about 2.6x real time on an i7-13700KF. At game start the trough coil ejects a ball,
the flippers pulse and hold on their buttons, the slings and pops fire on their switches, and the
GI and insert LEDs light. In `PinMAME.exe` (Windows x64) the set passes `-verifyroms`, boots to
attract mode with the DMD and the insert drawn, takes coins and starts a game from the keyboard,
plays the coin sounds and the start music, and keeps its NVRAM across a restart - at 60 frames
per second, the machine taking about half of one core of an i7-13700KF. Through libpinmame, in
VPX 10.8.1 with its PinMAME plugin, a converted Ghostbusters LE table plays: DMD and
insert, lamps and GI, coils, switches, sound and Slimer's motor. In the harness, Slimer homes at
power-on and at each game start; the service menu's SLIMER MOTOR TEST moves it to each position it
asks for (42, 50, 60) and reads it back, and SLIMER MOTOR CALIBRATION ends "COMPLETE: PASSED"
(POS 0, MIN -5, MAX 97, RANGE 102); the ball search drives it up to its stop until its time-out.

Game of Thrones LE 1.37.0 (six boards, 241 LED channels, Light References up to 265) boots to
attract mode, takes coins (four to a credit at its default pricing) and starts a game in the
harness and through libpinmame; its dragon homes on its switch and stands at step 23, the upper
and lower flippers fire from their buttons. The topper's home switch has no handler in the game
(`swdf_dragon_topper_motor_home` is in no switch table), so the topper homes by its time-out.

Game of Thrones Pro 1.37.0 (folder `GOT`; five boards, 71 switches, 18 coils, 108 LED channels)
boots to attract mode, takes coins and starts a game in the harness and through libpinmame. It
needed no change to the subsystem: it is the same machine without the upper playfield (no upper
flippers, a coil for the dragon, the topper dragon its only stepper).

KISS LE 1.41.0 (Whoa Nellie's SDK, five boards, 158 LED channels) boots to attract mode, takes
coins and starts a game. Its Starchild stands on its home switch; the game's
ball search drives it to the away switch and back, and its switch checks stay good. Its spinning
disc is lamp 172 (the game runs disc motor 2 only). The optional USB topper (`/dev/ttyUSB0`,
`auto_topper`) is not modelled: its writes are taken and dropped. The set is `kiss15`, as PinMAME's
`kiss` is Bally's of 1979.

Wrestlemania LE 1.35.0 (the oldest SDK, node firmware 0.18.4, six boards, four balls) boots to
attract mode with its DMD, and its playfield LCD plays the attract clips (the shattering "Legends
of Wrestlemania" logo and the WWE logo, decrypted by the game with the key from `wwe_135h.txt`,
at 20 fps); it takes coins and
starts a game. It needed `syncfs` (system call 373), 8-byte switch reads, its own table layouts,
80-byte board blocks, its CPU board inputs where its switch map reads them (above; at its table's
positions, each service button acts as the next one), the framebuffer, views for its clips and the
attract video nudge above. BACK in attract mode adds a service credit, as the game intends. Stern's `WWE_LE-1_35.iso.zip` copy here had an unreadable area on its disk; the
`.iso` inside it builds the same set.

Heavy Metal 1.02.0 (the newest SDK, node firmware 0.67.0, four node boards - one a TMC2590 stepper
driver - and four balls; set `heavym20_102`, as `heavymtl` and `hvymetal` are taken) boots to attract mode, takes
coins and starts a game in the harness and through libpinmame. Stern ships it only as a `.spk`,
and its program without a symbol table. Its addresses were found by matching its code against
Ghostbusters 1.17.0's (the same compiler and libraries): functions by their instructions with
branch targets and literal-pool loads masked, data by the literal pools of matched functions and
by the order of the table descriptors, which is the same in both programs. Each was checked in a
running machine. It also needed a larger range for its asset image (1.9 GB), the device entry's
field order and the coil table's index flags of its SDK, and its firmware image records: the
images are encrypted, the game decrypts them into the content buffer, and an image with its own
header gives its version there.

Supreme 1.01.0 (node firmware 0.22.0, two boards, four balls; set `supreme_101`, from a dump of its
card in `supremexstern-001.zip`) boots to attract mode, takes coins and starts a game, and its
flippers fire from their buttons. It showed that the reflex frame that arms the flippers, pops and
slings differs between SDKs: the model had read the 34-byte form only, so WWE's flippers, pops and
slings (23 bytes) and Heavy Metal's (43 bytes) did not fire on their own either until
`coil_reflex()` read each form. Its right flipper coil is named RT FLIPPER POWER, which the driver
now takes as RIGHT FLIPPER POWER.

Spider-Man The Pin 1.01 (node firmware 0.19.1, two boards, four balls, free play; set `smancp_101`,
from a raw dump of its card, `spiderman.bin`, game folder `spiderman_cp`; no manual exists; the
game shows V1.01 - the 1.1.0 in its NVRAM files' names is their format's version) boots
to attract mode with its RGB matrix in colour, starts a game, and fires its flippers, slings and
pops from their switches (the 23-byte reflex form, as WWE's). Its switch table keeps each entry's
index and number 24 bytes before the end, a third layout for `apply_manual_numbers()`; the numbers
are the game's own, which its switch test shows, and its cabinet's are Supreme's. Its sound comes at 24 kHz.

Transformers The Pin 1.0.18 (the ELG framework, node firmware 0.5.16, one board, 46 switches, 8
coils, 64 lamps, a trough of three optos, free play; set `tfcp_1018`, from Stern's update image
`transformers_pin-1.0.18.iso`, game folder `tf-elg`, program `gamer`) and Avengers The Pin 1.0.3
(node firmware 0.6.0, 49 switches; set `avscp_103`, from a raw dump of its card, `avengers.bin`,
game folder `avengers-elg`) boot to attract mode on their alphanumeric display ("TRANSFORMERS"
or "THE AVENGERS" scrolling, "STERN PINBALL", "PRESS START", the high scores), start a game from
START ("PLAYER 1", the trough eject), and fire their flippers, slings and pops from their
switches, in the harness and through libpinmame; their sounds play from the game start (attract
mode is silent). They needed the model above, the clock's boot time, the rate read-back and a set
that names its program (`SPIKE1_INIT_ALNUM`). No manual exists for either; the set names follow
Spider-Man The Pin's (`smancp`: SAM's `sman` and `cp`), from PinMAME's SAM sets `tf_*` and `avs_*`.

## Open items

1. Ball physics are the table's job (PinMAME/VPX); the harness has none, so a game stops at ball 1.
2. Outputs, what is left: the extra time and switch condition some coil fires carry; command 0x43
   (coil priority, a query answered with zeros); the LCD insert's fill command, the meaning of
   its animation-status reply (answered with zeros) and of a run's loop count.
3. Sound, what is left: the amplifier's gain steps (set through `/dev/amp`) and the center and
   headphone volumes are not applied.
4. PinMAME, what is left: VPinMAME (the COM controller) has not been checked; only CMake builds
   the subsystem (not the makefiles or the Visual Studio projects); the game lists do not name the
   set yet. A Visual Studio build of the subsystem runs at about half the speed of a GCC one.
5. Signals: kill, tkill and tgkill deliver to a handler (a frame with siginfo and ucontext,
   rt_sigreturn back), but a signal to a thread blocked in a system call waits until the thread
   wakes instead of interrupting the call.
6. `/proc` and `/sys` entries the game reads (`/proc/self/task/<tid>/comm`, `/proc/cpuinfo`).
   WWE: the LCD's backlight (a GPIO line, `/dev/gpio` 0x3c03/0x3c04 on line 157) is not modelled,
   so the LCD shows whatever the game draws; the wrestler-select clips are asked for from frame
   157 on, past the end of their 147 frames, so the LCD stays dark there (the same game code and
   clips on the board - not established whether it does the same there).
7. Sharing `src/p2k/shim`: `cmake/spike1.cmake` takes its sources from the Pinball 2000 library
   when there is one; a common library for both would be cleaner.
8. Heavy Metal: its stepper driver board (TMC2590, node 2) and its WS2812 and QR scanner devices
   are not modelled beyond what the start-up needs; node commands 0xC0 (on its packed LED SDK),
   0xF0, 0xF1 and 0xF2 to node 4 are taken without a model. No Heavy Metal table exists yet.
9. Spider-Man The Pin: its node thread reads a chip at I2C address 0x59 over SMBus (`I2C_SMBUS`, not
   modelled) and sends each board a key (0xF3, `NODEBUS_SendKey`, taken without a model); the game
   runs on. The RGB panel's frame time is the DMD's (not known). The game writes its settings and
   audits to `/data` as they change and commits nothing at the power fail, so a power down waits
   its full time; nothing is lost.
10. Ghostbusters' Slimer: its speed at full drive, 50 counts a second, is an estimate from the
   manual's 10 RPM gearhead (041-5132-01), not a measurement (the game and the firmware set a PWM
   duty, not a speed). Its end stops are the game's: its calibration drives him to -25 and to 125
   and passes only when he stopped at -6 (+/-4) and 102 (+/-5) counts further on. The speed is
   taken as linear in the duty, and with it about one calibration in three ends "ERROR" with MIN 0
   (a second one passes). That is a race in the game: it drives Slimer out of the home zone for
   61 loops of its calibration (a DMD frame each, about 1 s), then re-homes him. The board's first
   homing pass finds him there already, so for one node-bus cycle he homes without moving, and a
   loop that falls in that cycle sets the next target (-25) at once. The board counts nothing
   while the second pass drives him back, so the game's next 61-loop wait for a still position
   ends first and puts its next target (125) over the first. The game wins only if the return at
   duty 0x50 takes under about 1 s; linear in the duty it takes about 1.3 s at any full speed (the
   way out, at duty 0x80, grows with it), and only a motor near full speed from duty 0x50 on comes
   back in time. How the real motor's speed follows the duty is not known.
11. The ELG titles: the node firmware (0.5.16, 0.6.0), the bridge's and the display's are not in
   the sets and were not disassembled, so the node bus times are taken as ms, the flipper reflex's
   last three bytes and a pop or sling reflex's second phase (0 in both titles' tables) have no
   model, and the display's text port is not shown (its splash, above). `/dev/gpio` (the amplifier's
   and the boards' power lines, the bridge's command mode line) and `/dev/backlight` (0x4001) are
   taken without a model.
