# Lessons from Past Failures

## Severe Failures that had been fixed

**It was a blank CMOS**. Some of the aftermarket versions used not to boot, and what divided them was not the game but the system
software: each `game.rom` names the XINA it was built against, and everything from **1.16 to 1.31**
came up (`rfm_140` through `rfm_210`, `swep1_130` through `swep1_150`) while everything from
**1.34 to 1.38** stopped before the boot screen (`rfm_222` through `rfm_260`, and all three `swep1_2xx`).

The emulation was only indirectly at fault. The aftermarket software has a
static initialisation order bug: `left_sling`'s constructor calls a hook that reads an adjustment
resource whose own constructor is linked later, so the resource is still zeroed BSS and the read
reports NonFatal. That report is harmless on a machine in the field, and fatal here:

1. The NonFatal reporter appends the report to the error log in CMOS.
2. On a fresh install that log's header has never been built, so its base pointer is `0` and the
   entry is written **over address 0**.
3. `resched()` checks the reserved dword at address 0 on every scheduling decision, finds it
   changed, and calls it Fatal: *"reserved memory at zero corrupted"*.
4. The Fatal reporter writes through the same null pointer, so it re-makes the corruption it is
   reporting. The machine never leaves the handler and walks its stack down until it runs out -
   from the outside, a hang a few seconds after `STARTING UPDATE GAME CODE`.

A real machine always has that header: it is written on the first power-up and survives the
updates, which never clear CMOS. The older software has a different link order, never reports
during construction, and so builds it normally - which is why 1.31 and below were unaffected, and
why booting an older version once and keeping its NVRAM fixes the newer ones by hand. The driver
now seeds the header on a blank CMOS instead; see `P2K_SEED_ERROR_LOG` and `seed_error_log()` in
`p2k_driver.cpp`. Setting that define to 0 restores the old behaviour, and with it the hang.

The seed is written unconditionally in the constructor but only ever survives on a machine with
no NVRAM file: `MACHINE_INIT` copies a saved CMOS over the whole block immediately afterwards.
So it is new-machine-only in effect, and cannot disturb a machine that has been run before.

**PLX**. What used to stop early XINAs (<= 1.12) was the PLX serial EEPROM. `plx_ee_verify()` reads the image back and
rewrites it when it does not match, which these did on every boot, and only the read side
was modelled - so the ready poll after the first word never got its answer. Its escape is a
timeout counted in interval-timer ticks, and those do not run that early: the PIT is still
unprogrammed and every interrupt masked. Both dead ends at once, which is why it looked like a
silent freeze. `prism_1000_w` decodes the write commands now, and the image is in NVRAM, so a set
that rewrites it verifies clean on the next boot - which the machine says out loud the first
time, `PLX EEPROM data was stale - updated OK`, and never again.

Three of the prototypes used not to boot due to this, and did so identically: no timer ever programmed, no interrupt
ever arriving, and the CPU cycling between three addresses - `0x17b0ef`/`0x17b0f8`/`0x1baa89` for
RFM r1, `0x1b355f`/`0x1b3568`/`0x2020c5` for r2 and `0x1825df`/`0x1825e8`/`0x1db605` for Episode
I. It was the PLX serial EEPROM, and the write side of it - see the intro. The first two
addresses of each triple are `plx_eeprom_write` polling for the chip to report a word written,
and the third is the `GetIntervalTimer` its timeout is counted in - resolved against each
image's own symbol table, so that is all three read off rather than assumed from one.

The XINA boundary that this looked like is real but is a consequence, not the cause. Every set
that failed is built against **1.12 or older** and every set that boots against **1.16 or newer**,
with nothing in between in the collection - but what divides them is that the early images find
the EEPROM image they want missing and rewrite it, saying so (`pci_probe(): PLX EEPROM data was
stale - updated OK`), while the later ones are content with what is there. An earlier version of
this note pinned it on XINA 1.13's "Fixed exec code to avoid a timeout condition" instead, which
fit the evidence available then and was wrong. Worth keeping as a caution: every set in the
collection sat on one side of that boundary or the other, so the correlation was perfect and
still meant nothing.

## Quirks that had been fixed or do not matter much in practice

**Still existing workaround**: `P2K_PATCH_PCI_INIT_RETRY` mode 1 is not optional. Set it
to 0 and *nothing* boots: `rfm_080` halts at `0x81622`, and `rfm_160`
never reaches its first progress report.
Nevertheless, this workaround may come with sideeffects we do not know about yet.

**Interrupts used to arrive late**. The `clkint` gate held clock interrupts back while the guest was inside its own clock handler.
Without it the machine derailed a few seconds after the boot screen, and it took a long time to
find out why, because the gate looked like a timing workaround and every timing explanation was
wrong. The tick rate was not the variable, the latency was.

`pic8259_device` re-evaluates its INT output from a zero-delay timer - MAME's way of saying "do
this now" - and MAME's scheduler delivers on it by aborting the running CPU's timeslice.
`emu_timer::adjust()` in our PinMAME `shim/emu.h` did not, and `p2k_machine::run_cycles()` reads
`next_expiry()` before `cpu.run()` and never cuts the run short. So a timer the *guest* set
mid-slice waited for the slice to end. Timers set by other timers were always fine, because
`advance_to()` drains what its own callbacks schedule - which is why the PIT's own edges were
never late and only the guest's mask and EOI writes were.

XINU masks at the interrupt controller rather than with `cli`, constantly - its critical section
is a pair that writes `0xff` to the mask on the way in and restores it on the way out. It expects
an interrupt it has just unmasked to arrive *there*, at a point it has chosen. Ours arrived up to
a whole slice later, wherever that happened to break, which is how a tick lands inside a handler
that had masked against it. Two levels deep, the interrupt epilogue's nesting count no longer
reaches zero, `resched()` refuses with *"resched: called from interrupt handler"*, and the report
is itself a report - the loop that eats the stack.

Measured by varying the quantum, which is what set the latency (it capped the slice, and the slice was the latency):

| `P2K_QUANTUM_HZ` | latency | |
|---|---|---|
| 10000 (the old default) | 7766 cycles | derails |
| 150000 | 517 cycles | derails |
| 200000 | 388 cycles | boots |
| 1000000 | 77 cycles | boots |
| 100000000 | 1 cycle | boots |

A clock tick is 19406 cycles, so the machine tolerates about 2% of one and breaks by 3%. Real
hardware asserts INT within a bus cycle, orders of magnitude under that, which is why no real
machine ever needed the gate. `p2k_state::pics_settle()` fixes it at the source - the interrupt
controllers get their pending work run before the next instruction, as the hardware would have -
and with that the gate is unnecessary at any slice length.

The cap is off by default too, `next_expiry()` being the bound that matters: once the PIT runs it
already holds a slice to ~19406 cycles, and capping that to 7766 cost about twice the scheduler
passes for nothing the guest could tell apart. The knob stays because it is what turned this bug
into a number.

The gate is off by default now, both games having been through attract, a game and the test menus
without it. `P2K_CLKINT_GATE=1` puts it back - worth keeping, because it reproduces the old
behaviour in one run, and because `in/out` and `held` in the progress line measure interrupt
nesting directly, which is how this class of bug becomes visible at all.

**What the early RFM and SWEP1 versions disagree about is four registers**, and all three early images write the same four.
The default image here is MAME's, and `rfm_160` stores it back byte for byte - it never rewrites -
while 0.1, 0.80 and 0.40 each change LAS2BRD and the three chip-select bases:

| EEPROM dword | PLX register | default | what the early firmware writes |
|---|---|---|---|
| 14 | `0x28` LAS2BRD | `5403a1e0` | `5403a1c0` |
| 16 | `0x30` CS0BASE | `4041a060` | `4041a040` |
| 17 | `0x34` CS1BASE | `54b2b8c0` | `54b2b8f1` |
| 18 | `0x38` CS2BASE | `54b2b8c0` | `54b2b8f1` |

Local bus wait states and chip-select decoding, in other words - and nothing here models the
local bus decode, so all four are inert. They matter only in that the early firmware insists on
them. Nor is the default wrong: a real card is programmed once at the factory, and each firmware
verifies against what it wants, so a later-programmed card in an early machine would rewrite
exactly this way. To read the image back out of a `.nv`, find the vendor/device dword `6e 14 01
00` - it is the first of 32 little-endian dwords, two EEPROM words to a dword, high half first.

**The clock**. The machine shows real time: the date and time come from the host on every start,
the way a battery-backed RTC that kept running would present them, and run on from there.

Five things had to be right for this, each hidden by the one before. The clock was never seeded at all -
`mc146818` reads `machine().base_datetime()` in `nvram_default()`, which nothing called. The year
register counts from **1999**. The registers are **BCD** whatever the data mode bit says - seeded in
binary the 18th read as "12", `0x12`. And register 9 is not a year but a count of **year rollovers
since the last sync**: the firmware folds it into the clock it keeps in its own CMOS and writes zero
back, which works on the real board because the chip keeps running on its battery. Hand it the host
year every start and the displayed year climbs - 1999+27, then +27 again, 2053, 2080. So a start
refreshes everything *except* register 9, and the NVRAM block carries the host `time_t` it was saved
at so the years actually slept through can be counted into it.

And the **divider was held in reset**, which is what made a clock that started right stand still.
`nvram_default()` zeroes all 64 registers, so register A came up with DV2:DV0 `000` - the divider
chain stopped on a real MC146818, and in MAME's model a seconds update every 128 emulated seconds,
so a minute of machine time takes two hours. Nothing put it right either: `rtc_restore()` runs
before `clock_from_host()`, so a saved register A was wiped again every start, and the firmware
never writes it because on the real board it is battery-backed and was set once. `clock_from_host()`
seeds `0x26` through the port, the write being what re-arms the timers. A register no firmware has
to touch is exactly where an emulated chip reset to zeroes sits in a state no real board occupies.

That also corrects what this file used to say - that the RTC is never consulted again while
running, `one_second_proc` counting PIT ticks into the CMOS timestamps instead. If that were the
whole story the divider could not have mattered. How the two fit together is open;
`P2K_IOWATCH=70-71` with `P2K_IOWATCH_AFTER` past boot would show the reads. The drift arithmetic
by `pit_hz` stands either way, being about what the PIT's own second is worth.

Guest side, for anyone going further: `RTCLocationManagerClass` owns the hardware clock, a
`TimeStamp` is seven dwords `{year, month, day, day-of-week, hour, minute, second}` in plain
integers, and `timestamp_clock_last_set_data` at `0x0025e400` holds the 1 Jan 1999 12:00:00 default
that the console messages date from.
