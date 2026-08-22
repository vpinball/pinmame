# Pinball 2000 DCS2 sound board

Support for the Williams/Midway Pinball 2000 sound board (Revenge From Mars,
Star Wars Episode I and prototypes): **ADSP-2104 @ 16 MHz + SDRC ASIC, stereo output**.

This is info for the sound board only, maybe also a bit outdated since P2K emulation is working.

## Status

**The board works.** Driven by a real Pin2K host driver it produces correct
stereo audio: the games' own left/right/both sound test behaves as it should.
Verified at runtime against real RFM and SWEP1 firmware, identical on both ROM
sets:

- boots from the 28F800 flash (de-interleave and 512-word boot page confirmed
  byte-exact against the ROM, and the DSP executes)
- SDRC: window decode returns the correct flash words; the firmware pages its
  own program in through `EPM_PG`/`ROM_PG` and runs it from SRAM, then reaches
  sample data the same way
- host command interface, 16-bit path, latches and status flags
- SPORT1 autobuffer, stereo de-interleave, frame parity, 16-word aligned base
- stereo routing panned hard left/right, channel order matching the cabinet
- **31250 frames/sec** - see the sample-rate note in Design notes; deriving the
  rate from `SCLKDIV` plays everything 1.6x too fast

**Bug due to this**: The boot self-test "bong" may not sound right; see the sample-rate note in
Design notes for the likely reason and the shape of a fix.

The standalone `pin2ksnd`/`pin2ksw1` harness gets the board booted, initialised
and transmitting, but only ever silence, because it does not implement the
host-side protocol that starts a voice. That is a limitation of the harness,
not of the board - see "The host protocol wall".

> See "Known gaps" at the end before trusting it.

## Hooking it up on its own

Three things in the driver:

### 1. ROM regions

```c
#define MYGAME_SOUNDROM(flash,chkf, u109,chk109, u110,chk110) \
  DCS_P2K_STDREG(0x100000) \
  ROM_LOAD(flash, 0x000000, 0x100000, chkf) \
  SOUNDREGION(0x800000, DCS_P2K_SNDREGION) \
  ROM_LOAD16_BYTE(u109, 0x000000, 0x080000, chk109) \
  ROM_LOAD16_BYTE(u110, 0x000001, 0x080000, chk110)
```

| Region | Contents |
|---|---|
| `DCS_CPUREGION` | ADSP-2104 program/data space |
| `DCS_P2K_SRAMREGION` | 32K words of SDRC-banked board SRAM |
| `DCS_ROMREGION` | the unified sound address space (see below) |
| `DCS_P2K_SNDREGION` | separate sample region - **not** how the hardware works |

**Use one unified region.** The board sees a single `0x600000`-word sound
address space, and `EPM_PG` pages within it:

```
word 0x000000   28F800 sound flash   (also the boot device)
word 0x200000   U109
word 0x400000   U110
```

The gaps are real address-space gaps, not padding. The two sample chips are
16-bit words interleaved at 32-bit stride, so they load with
`ROM_LOAD32_WORD` at byte offsets `0x400000` and `0x400002` into a `0xc00000`
byte region. See the `pin2ksnd` harness for a worked example.

Declaring `DCS_P2K_SNDREGION` splits boot ROM from sample data, which makes
`ROM_PG` apply instead of `EPM_PG`. The firmware programs `EPM_PG`, so that
split is wrong for this board - the region exists only for the SDRC's
"RAM-based" configuration, which Pin2K does not use.

**The boot device is the flash, not U109/U110.** The ADSP boot port is byte
wide and wired to the low byte of each 16-bit flash word; `dcs_getBootROM()`
de-interleaves a `0x1000`-word page into a `0x1000`-byte boot image before
handing it to the loader, so load the flash image as-is.

This is verified, not assumed. Taking the low byte of each 16-bit word of
`rfm_28f800.rom` and `swe1_28f800.rom` yields, for both games identically:

```
srcdata[3] = 0x0e   ->  8 * (0x0e + 1) = 120 words
word 0     = 1801df ->  JUMP 0x01df   (inside the 2104's 512-word PM)
word 4     = 0a001f ->  next interrupt vector slot
```

The raw-byte and high-byte readings both give a `0xff` header, i.e. 2048
words - more than the 2104 can hold - and an all-`ffffff` opcode stream. Note
120 words is also exactly what the host's `0x000e` upload carries: the flash
boot block and the host upload are the same block by two different routes.

If `DCS_P2K_SNDREGION` is absent the SDRC treats `DCS_ROMREGION` as both boot
ROM and sample data (the "EPROM" case), which changes which page register
applies - `EPM_PG` instead of `ROM_PG`.

### 2. Machine driver

`MDRV_IMPORT_FROM(wmssnd_dcs3)` - adds the ADSP-2104, the Pin2K memory maps,
and a two-channel custom sound stream. Do not also import `wmssnd_dcs1`/`dcs2`.

### 3. Sound board init

```c
sndbrd_0_init(SNDBRD_DCSP2K, MY_SOUND_CPUNO, memory_region(DCS_ROMREGION), NULL, NULL);
```

`SNDBRD_DCSP2K` is `SNDBRD_TYPE(3,3)`, so it routes to the existing `dcsIntf`
and arrives as `brdData.subType == 3`. Everything Pin2K-specific in
`wmssnd.c` branches on that.

Then drive it with the **16-bit** entry points. The Pin2K protocol is 16-bit
end to end - boot-block headers, the boot payload and the DSP's replies are
all full words - so the 8-bit `sndbrd_0_*` calls truncate:

On real hardware these live in the PRISM BAR4 window at `0x13000000`:

| BAR4 | Access | Call |
|---|---|---|
| byte offset 0 | word | `dcs_p2k_data_w(cmd)` / `dcs_p2k_data_r()` |
| byte offset 0 | byte | `dcs_p2k_echo_w(v)` / `dcs_p2k_echo_r()` |
| byte offset 2 | - | `dcs_p2k_status_r()` / `dcs_p2k_status_w(v)` |
| - | - | `sndbrd_0_ctrl_w(0, x)` - board reset (also resets the SDRC and drops the queue) |

**Status bits are independent flags, not a state machine:** `0x40` = ready to
accept a command (always set), `0x80` = a response is available. A read also
ORs in whatever the host last wrote to the same register - the game does a
write-then-read sanity check during sound board detection, and a register that
drops the write reads as a dead board.

**The echo byte is a liveness probe.** The host writes a byte and expects to
read it back before it will talk to the board at all.

The 8-bit `sndbrd_0_data_w`/`_data_r`/`_ctrl_r` entry points still work and
forward to these, so the built-in manual sound-command UI keeps functioning -
but a driver should not use them.

Commands are queued (256 deep) and handed to the DSP one at a time; the next
one is released when the DSP acks by writing `0x0400`. One does not need to
touch the SDRC, the latches or the DACs directly.

### Boot-block uploads

If the host pushes a boot block down the command port (opening word `0x000e`,
then three bytes per 24-bit program word), `p2k_preprocess_write()` intercepts
the payload and writes the block straight into program memory. The opening
header is still delivered to the DSP for real - only the payload and the
closing repeat are swallowed - because this is an HLE of the DSP's *receive
loop*, not a replacement for its bootloader. A payload word with a non-zero
high byte aborts the transfer and re-sends the header.

## What the board looks like from the DSP

Data space, as mapped in `dcs3_readmem`/`dcs3_writemem`:

```
0000-03ff  SDRC ROM page / DRAM page window (decoded at runtime)
0400       host input latch (r) / input latch ack (w)
0401       host output latch
0402       output control (3 extra lines to the host; latched only)
0403       latch status
0413       Pin2K host handshake status
0480-0483  SDRC ASIC
0800-37ff  SDRC-banked SRAM, plus ROM/DRAM page windows at 3000/3400
3800-3fdf  internal RAM
3fe0-3fff  ADSP control registers
```

Program space: `0000-01ff` is the 2104's 512-word internal boot page (half the
2105's), `0800-3fff` is SRAM.

Because the SDRC remaps these windows at runtime and PinMAME's memory tables
are static, everything below `0x3800` goes through `p2k_data_r`/`p2k_data_w`,
which call `p2k_decode()` per access. That is slower than the WPC variants'
direct RAM entries, and it is fine here: this board may **not** need a
decode-loop speedup to run in real time (see below).

## Design notes

**Stereo comes from one SPORT, not two.** The board has two DMA-driven DACs,
but both are fed from the *same* SPORT1 autobuffer - the DSP writes the two
channels interleaved. So `adsp_txCallback()` still ignores `port != 1`, and
`dcs_txData()` simply de-interleaves into two ring buffers when
`dcs_dac.stereo` is set. There is no second DMA channel to service.

Consequently the SPORT word rate is twice the frame rate, which is why
`dcs_txData()`'s sample-rate assert reads `sRate * 0.5` in the stereo case.

**The frame rate is fixed at 31250 and must not be derived.** Real RFM and
SWE1 firmware programs `SCLKDIV=4` with `S1_CONTROL` bit 14 (nominally
"internal serial clock") set, which derives 50000 - and that plays everything
1.6x too fast, confirmed by ear. So the bit does not describe what actually
clocks the DACs; the board feeds them an external 31.25 kHz sample clock
regardless. Both references reached the same place independently: Encore
hardcodes 31250, and erikieNL's MAME branch added a Pin2K test specifically to
defeat MAME's derivation. `adsp_txCallback()` still computes the derived value
and logs it when it disagrees, so a board that genuinely does drive its own
clock would be visible rather than silently overridden.

> **Open, and probably caused by that decision:** the boot self-test "bong"
> may sound wrong. Encore notes the firmware brings up a **19.5 kHz, M=2**
> SPORT during board diagnostics before switching to the runtime one, so the
> diagnostic tone is generated at a different rate *and* a different
> autobuffer stride from normal playback. Forcing 31250 unconditionally will
> therefore play it at the wrong speed, and `M=2` may also mean the
> de-interleave should not be pairing adjacent words during that phase.
> Encore distinguishes the two by testing for `increment == 1` and a source
> rate above 30 kHz; the same test would let the diagnostic tone keep its
> derived rate while runtime audio stays pinned at 31250. Not implemented -
> runtime audio was the priority, and the bong is cosmetic.

Stereo routing is a second thing that is easy to get wrong and easy to miss:
the mixing levels handed to `stream_init_multi_float()` are `MIXER(level,pan)`
values, not raw levels. Passing a bare level leaves both channels panned
centre, so each comes out of both speakers and the board sounds mono no matter
how correct the de-interleave is.

Two further details the WPC boards get away with and this one does not:

- **The autobuffer base is aligned down to a 16-word boundary**, not derived
  as `I - M`. With interleaved channels a base one word out swaps left and
  right for the entire stream, so this is not a cosmetic difference.
- **`dcs_txData()` fetches through `p2k_decode()`**, not by indexing the CPU
  region. The transmit buffer can live in SDRC-banked SRAM, which is a
  different memory region entirely; reading the CPU region there would
  transmit unrelated memory. Frame parity is also carried across calls, since
  `adsp_irqGen()` hands over quadrant-sized chunks whose word count need not
  be even.

**No decoder HLE (yet).** `dcs_speedup()`/`dcs_speedup_1993()` are hand transcriptions
of specific WPC-era ADSP-2105 routines with hardcoded buffer addresses. They
must not run here - a Pin2K ROM can hit the same opcode patterns with a
completely different memory layout, which would silently corrupt DSP memory.
`dcs_init()` sets the new global `DCS_useSpeedup = 0` for subType 3. It is
checked at all three speedup entry points in `adsp2100.c` (idle-loop detector,
1994+ opcode sniffer, 1993 signature match) and at the two `cpu_triggerint()`
calls in `wmssnd.c` that exist only to undo the idle-loop suspend.

This may be okay on modern hosts: plain interpretation of a 16 MHz ADSP should keep up
with real time comfortably.
After the P2K emulation landed, this was found to be not so true though,
so additional idle skip detections had to be programmed to make the ADSP not
block the other emulation by too much.

**SDRC** is ported from the register model in MAME's `dcs.cpp`. MAME installs and removes address-map entries as the ASIC
is reprogrammed; here the derived page pointers live in `dcslocals.p2k` and the
window decode happens per access instead. Two consequences of that difference:

- The page pointers are refreshed on every write that MAME would have remapped
  on (reg0 `0x1833|0x0380`, reg1 `0x0003`, reg2 `0x1fff`), not just the
  page-register bits, or they go stale against a changed page size or source.
- `p2k_decode()` tests the **DRAM window before the ROM window**, because
  MAME installs SRAM, then ROM, then DRAM, and a later install wins where
  they overlap.

**Why `ROM_MS` gates the ROM window.** `ROM_MS` is the ADSP's boot-vs-data
memory select (0 = /BMS, 1 = /DMS). When it is clear the ROM page is routed to
*boot* memory select, so it is correctly not visible in data space - DSP code
reaches the DSP through `dcs_getBootROM()`/`adsp_boot()`, indexing the page
register directly, never through the data window. Encore exposes that page
through DM when `ROM_MS` is clear because it emulates the bootloader's flash
reads and has no separate boot path; do not copy that behaviour here. The
soft-reboot route (SYSCONTROL bit 9 ? `adsp_boot(1)`) already exists in
PinMAME, so firmware that pages through flash works without loosening the gate.

## Test harness

Apart from the Pin2K game drivers, nothing normally initialises the board or
sends it commands on its own. A diagnostic driver exists for exactly that, off by default:

```c
// src/pinmame.h
#define PIN2K_SOUND_TEST 1
```

Then run the `pin2ksnd` driver. It needs `rfm_28f800.rom` and U109/U110 (the Revenge From
Mars sound flash, etc) and nothing else - no playfield, no display, no switches.
The ADSP-2104 is the only CPU, so it attaches with `cpuNo = 0` rather than the
usual `DCS_CPUNO`.

On start it waits 2 s for the board to boot, runs the echo/liveness probe,
then sends `0x03ce / 0xff7f / 0x8180` - the triple Encore uses to start a
known runtime sample - 20 ms apart. What to watch for:

| Sign | Means |
|---|---|
| DSP runs past reset, boot block logged | boot path and de-interleave work |
| "SPORT1 autobuffer started", rate 62500 | autobuffer set up correctly |
| non-silent stereo PCM after the triple | SDRC paging and the DAC chain work |

## The host protocol wall

What the board actually does so far, observed on real firmware. Recorded so the next
person does not repeat these experiments:

| Stimulus | Result |
|---|---|
| reset | reply `0x000a` |
| `0x55aa`, `0x55ab` | reply `0x55ab` - a real sync handshake, and the only thing that has ever changed the board's behaviour |
| `0xace1` | full mixer-init page walk, SPORT1 enabled, PCM starts flowing (all zeros), reply `0x000a` |
| after that `0x000a` | **silent forever** - no reply to anything, including a repeated `0x55aa`/`0x55ab` sync |
| `0x8280`, `0xacef` | no observable effect |
| 1200+ play triples, IDs `0x0001`-`0x04b0`, with and without per-transaction sync | every one ignored, `peak=0` throughout |

The sync pair matters: **without it the board never replies after `0xace1` at all**,
with it there is one final `0x000a`. So `0x000a` reads as "ready/idle" and the
board is waiting for something specific that we have not found by probing.
Re-syncing does not reopen the conversation, so this is not a simple
sync-per-transaction protocol.

Probing the command space is exhausted - everything above was found by trying
words, and the only two that ever mattered (`0x55aa`/`0x55ab`) came from
reading the game code, not from guessing.

The silence after `0xace1` matches erikieNL's MAME script-mode handling, where the
DSP's output is redirected away from the host-visible output-full flag
(`SET_INPUT_FULL()` instead of `SET_OUTPUT_FULL()`). Sending the closing
`0xacef` does *not* restore replies, so the simple open/close bracket is not
the mechanism - something inside the script is required.

Encore does not synthesise this either: it waits for the *guest* to push its
ACE1/DCS runtime mixer initialisation and only watches for `0x8280` going past
to know the guest has finished. The stream itself lives in the PC-side game
code.

**To get sound for this special test code, the next step would be to recover that stream from `game.rom`**, with
`symbols.rom` alongside it (both in Encore's `updates/` tree), rather than
guessing further at command words. Everything below the protocol is done.

### How to read the game code

This has been checked and works; it is not a suggestion.

`symbols.rom` starts with the literal text `SYMBOL TABLE`, then 4 bytes of
magic, a 32-bit symbol count at offset 16, and another 32-bit field, followed
by 8-byte entries of `(address, name_offset)`, both little-endian, from offset
24. The name offset points *into* a name rather than at its start, so scan
back to the preceding NUL. Names are demangled C++ with full signatures.

`game.rom` is a raw flat 32-bit x86 image with **load base `0x00100000`**, so
`file_offset = address - 0x00100000`. Confirmed: file offset 0 is
`55 89 e5 ...` (push ebp / mov ebp,esp), matching the zero-argument destructor
the symbol table places at `0x00100000`.

The DCS API surface is all there - `DCSInit`, `DCSWarmBoot`, `DCSRunDiags`,
`DCSRequest(AudioCtlReqInfo *)`, `DCSGetSignal`, `DCSSetTrackVolume`,
`DCSSetTrackPan`, plus signal constants `DCS2_sig_sync_word`,
`DCS2_sig_streaming_handshk`, `DCS2_sig_playlist_sig`, `DCS2_sig_system_error`
and the `DCS2_flash_boot_*` symbols.

Searching the image for the known magic words as immediates is a fast way in:
`0xace1` and `0xacef` appear seven bytes apart at file offset `0x0d90e9` in a
`cmp eax, imm32` dispatch chain that also tests **`0x55aa` and `0x55ab`** -
two further protocol words neither reference mentioned.

## Known gaps

Things a driver author may hit first, roughly in the order they will hurt:

1. **Channel order is matched to the cabinet, not proven from the buffer.**
   The first word of each frame is sent to the right speaker because that is
   what makes the games' sound test come out right. The same result would
   occur if the board emitted left first and the hardware crossed the channels
   downstream (DAC-to-amp routing or speaker wiring). Encore maps it the other
   way, so its output is mirrored relative to ours.
2. **SPORT IRQ granularity is unresolved but appears harmless.** PinMAME pulses
   `IRQ1` once per buffer, at wrap (`adsp_irqGen`'s `else` branch; the four
   `DCS_IRQSTEPS` quadrants only pace DAC hand-off and I-register writeback).
   Encore fires *two* per buffer - half and wrap. Neither reference explains
   why, and no tearing has been heard, so this is left alone. If playback ever
   tears at a ~15ms period, raise the IRQ1 pulse to fire at the 2/4 boundary
   as well, for this subtype only.
3. **SRAM in program space is not gated by `SDRC_SM_EN`.** Program `0800-3fff`
   is a plain RAM entry; only the data-space windows honour the enable and the
   `SDRC_SM_BK` bank swap. Note this is *not* an aliasing bug - MAME also
   keeps PM and DM SRAM in separate storage.
4. **The input FIFO at `0404-0407` is stubbed** to read back `0xffff`, matching
   the reference. Nothing drives it.
5. **`SDRC_MUTE` is decoded but not acted on.**
6. **DSP data `0x0413` has a single, unconfirmed source.** erikieNL's MAME
   branch maps a 3-state handshake there and this follows it; Encore does not
   map that address at all. Note this is *not* the host status register - the
   host-side layout is now pinned down (see "Hooking it up"), so only the
   DSP-visible mirror is in doubt. Harmless if the firmware never reads it.
7. **The test harness cannot start a voice.** A real host driver can, so this
   is a harness limitation only. `0xace1` opens the host's ACE1/DCS runtime
   mixer initialisation (erikieNL's MAME calls this "script mode"; Encore acks
   it with `0x0100`/`0x000c`) and `0x8280` is its final word; everything in
   between lives in PC-side game code. Without it the harness gets the board
   booted, initialised and transmitting, but only silence.
8. **The 8-bit `sndbrd_0_*` compatibility path truncates**, by design. It is
   there so the manual sound-command UI keeps working, not for driver use.

## References

- erikieNL's MAME `pinball2k` branch - spec for the memory map
  and the host status port. Exploratory, stale, mostly working, was merged to MAME, but reverted due to nitpicking.
- ThomazPom/Encore-Pinball2000 - a working native ADSP engine under QEMU.
  Confirms the 2104, the SDRC, the interleaved-stereo autobuffer, and that a
  plain interpreter may hit 100% real-time delivery. **No license file: read it
  for hardware facts, do not copy code from it.**
- MAME `src/mame/shared/dcs.cpp` - source of the SDRC register model.
